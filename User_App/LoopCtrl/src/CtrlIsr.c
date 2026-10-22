/**
 * @file CtrlIsr.c
 * @author yetingheng (004182)
 * @brief 20kHz 控制中断实现：SRF-PLL + 三相调制波下发 + 封波检测 + MUX 轮换
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 * @note 本文件目前只搭骨架：
 *       - PLL 是标准 SRF-PLL，参数为骨架值，实测后整定；
 *       - 环路（电压/电流 PI）尚未接入，调制比由状态机通过 CtrlSetModIndex() 给出；
 *       - 调制比为 0 时三相占空比为 0，桥臂处于中点钳位，不会发波。
 */

#include "CtrlIsr.h"
#include "User_ClaTask.h"
#include "Hw_InvDriver.h"

#include <math.h>

/* MUX 地址脚：GPIO46 = bit0，GPIO47 = bit1，GPIO42 = bit2（均在 GPB 口） */
#define CTRL_MUX_MASK_BIT0      (0x4000U)       /* GPIO46 */
#define CTRL_MUX_MASK_BIT1      (0x8000U)       /* GPIO47 */
#define CTRL_MUX_MASK_BIT2      (0x0400U)       /* GPIO42 */
#define CTRL_MUX_MASK_ALL       (Uint16)(CTRL_MUX_MASK_BIT0 | CTRL_MUX_MASK_BIT1 | CTRL_MUX_MASK_BIT2)

/* MUX 轮换顺序：PV1~PV4 电压、BUSL、BUSH（VISO 只在初始化阶段单独采，不进轮换） */
static const Uint16 gCtrlMuxTable[] =
{
    CLA_MUX_ADDR_PV1,
    CLA_MUX_ADDR_PV2,
    CLA_MUX_ADDR_PV3,
    CLA_MUX_ADDR_PV4,
    CLA_MUX_ADDR_BUSL,
    CLA_MUX_ADDR_BUSH
};

#define CTRL_MUX_TABLE_NUM      (sizeof(gCtrlMuxTable) / sizeof(gCtrlMuxTable[0]))

static float  gCtrlTheta;               /* PLL 输出角度 (rad, 0~2pi) */
static float  gCtrlOmega;               /* PLL 输出角速度 (rad/s) */
static float  gCtrlModIndex;            /* 调制比 0~CTRL_MOD_INDEX_MAX */
static Uint16 gCtrlFault;               /* E_CTRL_FAULT */
static Uint16 gCtrlMuxIdx;              /* 轮换表下标 */
static Uint16 gCtrlMuxAddr;             /* 当前选通地址 */
static Uint16 gCtrlSeqLast;             /* 上一次看到的 CLA 序号 */
static Uint16 gCtrlSeqStall;            /* 序号停滞计数 */
static Uint16 gCtrlCycleCnt;            /* 已运行控制拍数 */

/* 12位码值 -> 工程值（双极性：0~3V 对应 -满量程 ~ +满量程） */
static float CtrlCodeToBipolar(float fCode16, float fRange)
{
    float fCode12 = fCode16 * CTRL_ADC_16BIT_SCALE;

    return (fCode12 - CTRL_ADC_MID) * (2.0f * fRange / CTRL_ADC_FULL);
}

/* 写三位 MUX 地址到 GPIO46/47/42，用 SET/CLEAR 寄存器避免读改写干扰其它引脚 */
static void CtrlMuxWrite(Uint16 u16Addr)
{
    Uint16 u16Set = 0U;

    if ((u16Addr & 0x0001U) != 0U)
    {
        u16Set |= CTRL_MUX_MASK_BIT0;
    }

    if ((u16Addr & 0x0002U) != 0U)
    {
        u16Set |= CTRL_MUX_MASK_BIT1;
    }

    if ((u16Addr & 0x0004U) != 0U)
    {
        u16Set |= CTRL_MUX_MASK_BIT2;
    }

    GpioDataRegs.GPBCLEAR.all = CTRL_MUX_MASK_ALL;

    if (u16Set != 0U)
    {
        GpioDataRegs.GPBSET.all = u16Set;
    }
}

/* 角度归一化到 [0, 2pi) */
static float CtrlWrapTheta(float fTheta)
{
    while (fTheta >= CTRL_TWO_PI)
    {
        fTheta -= CTRL_TWO_PI;
    }

    while (fTheta < 0.0f)
    {
        fTheta += CTRL_TWO_PI;
    }

    return fTheta;
}

/* 初始化 */
void CtrlIsrInit(void)
{
    gCtrlTheta = 0.0f;
    gCtrlOmega = CTRL_TWO_PI * 50.0f;
    gCtrlModIndex = 0.0f;
    gCtrlFault = (Uint16)CTRL_FAULT_NONE;
    gCtrlMuxIdx = 0U;
    gCtrlMuxAddr = CLA_MUX_ADDR_PV1;
    gCtrlSeqLast = 0U;
    gCtrlSeqStall = 0U;
    gCtrlCycleCnt = 0U;

    /* CLA 侧默认使能滤波归档 */
    CpuToCla1Msg.u16Enable = 1U;
    CpuToCla1Msg.u16MuxAddr = gCtrlMuxAddr;
    CpuToCla1Msg.fTheta = 0.0f;

    CtrlMuxWrite(gCtrlMuxAddr);
}

/* 控制中断主体 */
void CtrlIsrRun(void)
{
    float fValpha;
    float fVbeta;
    float fSin;
    float fCos;
    float fDTheta;
    float fSinNew;
    float fCosNew;
    float fVq;
    float fVmag;
    float fModR;
    float fModS;
    float fModT;
    float fCur;
    Uint16 u16Seq;

    gCtrlCycleCnt++;

    /* ---------- 1. CLA 心跳检测 ---------- */
    u16Seq = Cla1ToCpuMsg.u16Seq;

    if (u16Seq == gCtrlSeqLast)
    {
        if (gCtrlSeqStall < 0xFFFFU)
        {
            gCtrlSeqStall++;
        }
    }
    else
    {
        gCtrlSeqStall = 0U;
        gCtrlSeqLast = u16Seq;
    }

    if (gCtrlSeqStall > CTRL_CLA_TIMEOUT_NUM)
    {
        gCtrlFault = (Uint16)CTRL_FAULT_CLA_TIMEOUT;
    }

    /* ---------- 2. 过流封波检测（三路逆变电流瞬时值） ---------- */
    if (gCtrlFault == (Uint16)CTRL_FAULT_NONE)
    {
        fCur = CtrlCodeToBipolar(Cla1ToCpuMsg.fInvCur[CLA_IDX_R], CTRL_CUR_RANGE);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = CtrlCodeToBipolar(Cla1ToCpuMsg.fInvCur[CLA_IDX_S], CTRL_CUR_RANGE);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = CtrlCodeToBipolar(Cla1ToCpuMsg.fInvCur[CLA_IDX_T], CTRL_CUR_RANGE);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }
    }

    if (gCtrlFault != (Uint16)CTRL_FAULT_NONE)
    {
        /* 故障：三相封波到中点钳位，调制比清零 */
        Hw_InvStopAll();
        gCtrlModIndex = 0.0f;
        return;
    }

    /* ---------- 3. SRF-PLL：三相电网电压 -> Clarke -> 锁相 ---------- */
    fValpha = (2.0f / 3.0f) * (CtrlCodeToBipolar(Cla1ToCpuMsg.fGridVolt[CLA_IDX_R], CTRL_VOLT_RANGE)
                               - (0.5f * CtrlCodeToBipolar(Cla1ToCpuMsg.fGridVolt[CLA_IDX_S], CTRL_VOLT_RANGE))
                               - (0.5f * CtrlCodeToBipolar(Cla1ToCpuMsg.fGridVolt[CLA_IDX_T], CTRL_VOLT_RANGE)));

    fVbeta = 0.577350269f * (CtrlCodeToBipolar(Cla1ToCpuMsg.fGridVolt[CLA_IDX_R], CTRL_VOLT_RANGE)
                             + (2.0f * CtrlCodeToBipolar(Cla1ToCpuMsg.fGridVolt[CLA_IDX_S], CTRL_VOLT_RANGE)));

    fSin = sinf(gCtrlTheta);
    fCos = cosf(gCtrlTheta);

    /* 本工程 Clarke 定义为 Vbeta = (1/sqrt3)(Va + 2Vb) = -Vcos(theta)，
     * 因此锁相误差取 Vq = Valpha*cos(theta) + Vbeta*sin(theta)，
     * 锁定后 Vq = 0；theta 超前时 Vq < 0，经 KI 积分后角速度下降，构成负反馈。 */
    fVq = (fValpha * fCos) + (fVbeta * fSin);

    /* 用电压幅值归一化，使 KP/KI 的量纲固定为 rad/s 与 1/s */
    fVmag = sqrtf((fValpha * fValpha) + (fVbeta * fVbeta));

    if (fVmag > CTRL_PLL_VMAG_MIN)
    {
        fVq = fVq / fVmag;
    }
    else
    {
        fVq = 0.0f;      /* 电网掉电：误差清零，角速度按当前值自由运行 */
    }

    /* 角速度积分 + 比例校正 */
    gCtrlOmega += CTRL_PLL_KI * fVq * CTRL_TS_SEC;

    if (gCtrlOmega < (CTRL_TWO_PI * CTRL_PLL_FREQ_MIN))
    {
        gCtrlOmega = CTRL_TWO_PI * CTRL_PLL_FREQ_MIN;
    }

    if (gCtrlOmega > (CTRL_TWO_PI * CTRL_PLL_FREQ_MAX))
    {
        gCtrlOmega = CTRL_TWO_PI * CTRL_PLL_FREQ_MAX;
    }

    fDTheta = (gCtrlOmega + (CTRL_PLL_KP * fVq)) * CTRL_TS_SEC;
    gCtrlTheta = CtrlWrapTheta(gCtrlTheta + fDTheta);

    /* 用角度增量的小角度展开递推 sin/cos：
     * fDTheta 最大约 0.022rad（70Hz），三阶截断误差 < 1e-9，
     * 这样一拍控制中断只需要 1 次 sinf + 1 次 cosf 库调用。 */

    fSinNew = (fSin * (1.0f - (0.5f * fDTheta * fDTheta)))
            + (fCos * (fDTheta - ((fDTheta * fDTheta * fDTheta) * 0.16666667f)));
    fCosNew = (fCos * (1.0f - (0.5f * fDTheta * fDTheta)))
            - (fSin * (fDTheta - ((fDTheta * fDTheta * fDTheta) * 0.16666667f)));

    /* 角度同步给 CLA（预留给 CLA 侧做坐标变换） */
    CpuToCla1Msg.fTheta = gCtrlTheta;

    /* ---------- 4. 生成三相调制波并下发 ----------
     * 三电平单极性：|m*sin| 就是斩波占空比，sin 的符号决定哪一对管子斩波，
     * 过零点处 |sin| -> 0，占空比自然归零，所以换相是软换相。 */
    fModR = gCtrlModIndex * fSinNew;
    fModS = gCtrlModIndex * ((-0.5f * fSinNew) - (0.86602540f * fCosNew));
    fModT = gCtrlModIndex * ((-0.5f * fSinNew) + (0.86602540f * fCosNew));

    Hw_InvSetDuty((Uint16)HW_INV_PHASE_R, fabsf(fModR), (fModR >= 0.0f) ? 1U : 0U);
    Hw_InvSetDuty((Uint16)HW_INV_PHASE_S, fabsf(fModS), (fModS >= 0.0f) ? 1U : 0U);
    Hw_InvSetDuty((Uint16)HW_INV_PHASE_T, fabsf(fModT), (fModT >= 0.0f) ? 1U : 0U);

    /* ---------- 5. MUX 轮换（放在最后：本拍写入的地址供下一周期采样） ---------- */
    gCtrlMuxIdx++;
    if (gCtrlMuxIdx >= (Uint16)CTRL_MUX_TABLE_NUM)
    {
        gCtrlMuxIdx = 0U;
    }

    gCtrlMuxAddr = gCtrlMuxTable[gCtrlMuxIdx];

    CtrlMuxWrite(gCtrlMuxAddr);
    CpuToCla1Msg.u16MuxAddr = gCtrlMuxAddr;
}

/* 调制比设定 */
void CtrlSetModIndex(float fModIndex)
{
    if (fModIndex < CTRL_MOD_INDEX_MIN)
    {
        fModIndex = CTRL_MOD_INDEX_MIN;
    }

    if (fModIndex > CTRL_MOD_INDEX_MAX)
    {
        fModIndex = CTRL_MOD_INDEX_MAX;
    }

    gCtrlModIndex = fModIndex;
}

float CtrlGetModIndex(void)
{
    return gCtrlModIndex;
}

float CtrlGetTheta(void)
{
    return gCtrlTheta;
}

float CtrlGetGridFreq(void)
{
    return gCtrlOmega / CTRL_TWO_PI;
}

Uint16 CtrlGetFault(void)
{
    return gCtrlFault;
}

Uint16 CtrlGetMuxAddr(void)
{
    return gCtrlMuxAddr;
}

Uint16 CtrlGetCycleCnt(void)
{
    return gCtrlCycleCnt;
}

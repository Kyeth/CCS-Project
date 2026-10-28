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
#include "Hw_PwmDriver.h"
#include "AdcCalib.h"
#include "AdcSample.h"
#include "Schedule.h"
#include "ProtectFault.h"
#include "ProtectInput.h"

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
static Uint16 gCtrlProtectDiv;          /* 保护框架 10ms 分频计数 */

/* 发波授权状态机 */
static float  gCtrlModActual;           /* 实际下发调制比：按斜率跟随 gCtrlModIndex */
static Uint16 gCtrlAuthState;           /* CTRL_AUTH_xxx */
static Uint16 gCtrlAuthCnt;             /* WAIT 态已等待拍数 */
static Uint16 gCtrlAuthFlags;           /* CTRL_AUTH_FLAG_xxx */

/* 观测影子量：非 static，供软示波器直接按地址观测（只写不读） */
volatile float  gCtrlObsThetaDeg  = 0.0F;
volatile float  gCtrlObsGridFreq  = 0.0F;
volatile float  gCtrlObsModIndex  = 0.0F;
volatile Uint16 gCtrlObsFault     = 0U;
volatile Uint16 gCtrlObsAuthState = 0U;
volatile Uint16 gCtrlObsAuthFlags = 0U;
volatile float  gCtrlObsModActual = 0.0F;

/* 发波授权：0 = 只算环路不发波，1 = 允许把占空比写进 ePWM */
volatile Uint16 gCtrlPwmEnable    = 0U;

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

    gCtrlModActual = 0.0f;
    gCtrlAuthState = (Uint16)CTRL_AUTH_OFF;
    gCtrlAuthCnt   = 0U;
    gCtrlAuthFlags = 0U;

    gCtrlObsAuthState = gCtrlAuthState;
    gCtrlObsAuthFlags = gCtrlAuthFlags;
    gCtrlObsModActual = gCtrlModActual;

    /* CLA 侧默认使能滤波归档 */
    CpuToCla1Msg.u16Enable = 1U;
    CpuToCla1Msg.u16MuxAddr = gCtrlMuxAddr;
    CpuToCla1Msg.fTheta = 0.0f;

    CtrlMuxWrite(gCtrlMuxAddr);

    ProtectInit();
    ProtectInputInit();
}

/* ==================== 发波授权 ==================== */

/* 严重封锁：把三相压回中点钳位，调制比清零，授权退回 OFF。
 * 只在"真的需要动硬件"时调一次 Hw_InvStopAll()，避免 20kHz 每拍写 AQCSFRC。 */
static void CtrlAuthBlock(void)
{
    if ((gCtrlAuthState != (Uint16)CTRL_AUTH_OFF) || (gCtrlModActual > 0.0f))
    {
        Hw_InvStopAll();
    }

    gCtrlModActual = 0.0f;
    gCtrlAuthCnt   = 0U;
    gCtrlAuthState = (Uint16)CTRL_AUTH_OFF;
}

/* 把桥臂从"软件钳位"切到"正常斩波"。
 * Hw_InvSetDuty() 只在换相时释放模块，而 Hw_InvInit() 把正负两路都压低了：
 * 若放行后的第一拍正好落在正半周，正模块不会被释放，表现为"第一个正半周没有波形"。
 * 这里先按负半周、再按正半周各写一拍零占空比，让两路都走一遍释放流程。 */
static void CtrlAuthArm(void)
{
    Uint16 i;

    for (i = 0U; i < (Uint16)HW_INV_PHASE_MAX; i++)
    {
        Hw_InvSetDuty(i, 0.0f, 0U);
        Hw_InvSetDuty(i, 0.0f, 1U);
    }
}

/* 发波授权状态机：每拍 20kHz 调用一次，裁定本拍是否允许把占空比写进硬件。 */
static void CtrlAuthRun(void)
{
    float fStep;

    gCtrlAuthFlags = 0U;

    /* 封锁条件 1：控制层故障（CLA 停摆、逆变过流、硬件过流/短路） */
    if (gCtrlFault != (Uint16)CTRL_FAULT_NONE)
    {
        gCtrlAuthFlags |= (Uint16)CTRL_AUTH_FLAG_FAULT;
    }

    /* 封锁条件 2：ADC 零漂标定未完成，此时的占空比没有物理意义 */
    if (AdcCalibGetZeroState() != ADC_CAL_ZERO_DONE)
    {
        gCtrlAuthFlags |= (Uint16)CTRL_AUTH_FLAG_CALIB;
    }

    if (gCtrlAuthFlags != 0U)
    {
        CtrlAuthBlock();
        return;
    }

    switch (gCtrlAuthState)
    {
        case CTRL_AUTH_OFF:
            gCtrlModActual = 0.0f;
            if (gCtrlPwmEnable != 0U)
            {
                gCtrlAuthCnt   = 0U;
                gCtrlAuthState = (Uint16)CTRL_AUTH_WAIT;
            }
            break;

        case CTRL_AUTH_WAIT:
            gCtrlModActual = 0.0f;
            if (gCtrlPwmEnable == 0U)
            {
                gCtrlAuthState = (Uint16)CTRL_AUTH_OFF;
                break;
            }

            gCtrlAuthCnt++;
            if (gCtrlAuthCnt >= CTRL_AUTH_WAIT_MAX)
            {
                CtrlAuthArm();
                gCtrlAuthCnt   = 0U;
                gCtrlAuthState = (Uint16)CTRL_AUTH_ON;
            }
            break;

        case CTRL_AUTH_ON:
            if (gCtrlPwmEnable == 0U)
            {
                gCtrlAuthState = (Uint16)CTRL_AUTH_SOFT_OFF;
                break;
            }

            /* 实际调制比向请求值双侧跟随（请求值由 CtrlSetModIndex 限幅在 0~0.90） */
            fStep = CTRL_AUTH_SLEW_UP;
            if (gCtrlModActual < gCtrlModIndex)
            {
                gCtrlModActual += fStep;
                if (gCtrlModActual > gCtrlModIndex)
                {
                    gCtrlModActual = gCtrlModIndex;
                }
            }
            else
            {
                gCtrlModActual -= fStep;
                if (gCtrlModActual < gCtrlModIndex)
                {
                    gCtrlModActual = gCtrlModIndex;
                }
            }
            break;

        case CTRL_AUTH_SOFT_OFF:
            fStep = CTRL_AUTH_SLEW_DN;
            if (gCtrlModActual > fStep)
            {
                gCtrlModActual -= fStep;
            }
            else
            {
                /* 调制比已归零：真正封波，回 OFF 等下一次请求 */
                gCtrlModActual = 0.0f;
                Hw_InvStopAll();
                gCtrlAuthCnt   = 0U;
                gCtrlAuthState = (Uint16)CTRL_AUTH_OFF;
            }
            break;

        default:
            CtrlAuthBlock();
            break;
    }
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

    /* ---------- 0. 保护框架：10ms 节拍推进记录、去抖与协议聚合 ---------- */
    if (gCtrlProtectDiv >= 200U)
    {
        gCtrlProtectDiv = 0U;
        ProtectInputRun10ms();
        ProtectRun10ms();
    }
    else
    {
        gCtrlProtectDiv++;
    }

    gCtrlCycleCnt++;

    /* ---------- 0. ADC 标定刷新：码值 -> 工程值 ---------- */
    /* 本拍所有消费者（过流判定、PLL、软示波器）都从 AdcCalib 取数，
     * 保证同一拍看到的是同一份数据；上电零漂检测状态机也在里面推进 */
    AdcCalibRefresh();

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

    /* ---------- 1b. 后台任务活性：停摆即按故障处理，撤销发波授权 ---------- */
    if (Task_GetHealthFault() != 0U)
    {
        gCtrlFault = (Uint16)CTRL_FAULT_TASK_STALL;
    }

    /* ---------- 1c. 保护框架：未屏蔽的 STOP 及以上故障直接封锁发波 ---------- */
    if (ProtectGetTotalSign() != 0U)
    {
        gCtrlFault = (Uint16)CTRL_FAULT_PROTECT;
    }

    /* ---------- 2. 过流封波检测（三路逆变电流瞬时值） ---------- */
    if (gCtrlFault == (Uint16)CTRL_FAULT_NONE)
    {
        fCur = AdcCalibInvCur(CLA_IDX_R);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = AdcCalibInvCur(CLA_IDX_S);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = AdcCalibInvCur(CLA_IDX_T);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            gCtrlFault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }
    }

    if (gCtrlFault != (Uint16)CTRL_FAULT_NONE)
    {
        /* 故障：三相封波到中点钳位，调制比清零，授权退回 OFF */
        CtrlAuthBlock();
        gCtrlModIndex = 0.0f;
        gCtrlObsFault = gCtrlFault;
        return;
    }

    /* ---------- 2c. 发波授权：裁定本拍能否把占空比写进硬件 ---------- */
    CtrlAuthRun();

    /* ---------- 3. SRF-PLL：三相电网电压 -> Clarke -> 锁相 ---------- */
    fValpha = (2.0f / 3.0f) * (AdcCalibGridVolt(CLA_IDX_R)
                               - (0.5f * AdcCalibGridVolt(CLA_IDX_S))
                               - (0.5f * AdcCalibGridVolt(CLA_IDX_T)));

    fVbeta = 0.577350269f * (AdcCalibGridVolt(CLA_IDX_R)
                             + (2.0f * AdcCalibGridVolt(CLA_IDX_S)));

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
    fModR = gCtrlModActual * fSinNew;
    fModS = gCtrlModActual * ((-0.5f * fSinNew) - (0.86602540f * fCosNew));
    fModT = gCtrlModActual * ((-0.5f * fSinNew) + (0.86602540f * fCosNew));

    /* 只有授权态 ON 且实际调制比非 0 才写硬件：其余状态下桥臂由授权状态机
     * 负责压回钳位（CtrlAuthBlock / SOFT_OFF 归零时各做一次），这里不再每拍
     * 调 Hw_InvStopAll()，避免以 20kHz 反复写 AQCSFRC。 */
    if ((gCtrlAuthState == (Uint16)CTRL_AUTH_ON) && (gCtrlModActual > 0.0f))
    {
        Hw_InvSetDuty((Uint16)HW_INV_PHASE_R, fabsf(fModR), (fModR >= 0.0f) ? 1U : 0U);
        Hw_InvSetDuty((Uint16)HW_INV_PHASE_S, fabsf(fModS), (fModS >= 0.0f) ? 1U : 0U);
        Hw_InvSetDuty((Uint16)HW_INV_PHASE_T, fabsf(fModT), (fModT >= 0.0f) ? 1U : 0U);
    }
    else
    {
        /* 未授权发波：占空比不写硬件，桥臂保持当前钳位状态 */
    }

    /* ---------- 5. MUX 轮换（放在最后：本拍写入的地址供下一周期采样） ---------- */
    gCtrlMuxIdx++;
    if (gCtrlMuxIdx >= (Uint16)CTRL_MUX_TABLE_NUM)
    {
        gCtrlMuxIdx = 0U;
    }

    gCtrlMuxAddr = gCtrlMuxTable[gCtrlMuxIdx];

    CtrlMuxWrite(gCtrlMuxAddr);
    CpuToCla1Msg.u16MuxAddr = gCtrlMuxAddr;

    /* ---------- 6. 刷新观测影子量（软示波器用） ---------- */
    gCtrlObsThetaDeg = gCtrlTheta * 57.2957795F;   /* rad -> deg */
    gCtrlObsGridFreq = gCtrlOmega * 0.15915494F;   /* rad/s -> Hz */
    gCtrlObsModIndex  = gCtrlModIndex;
    gCtrlObsModActual = gCtrlModActual;
    gCtrlObsFault     = gCtrlFault;
    gCtrlObsAuthState = gCtrlAuthState;
    gCtrlObsAuthFlags = gCtrlAuthFlags;

    /* ---------- 7. 采样快照：瞬时值进控制环，RMS/慢速值留给对外 ---------- */
    /* 放在最后：本拍的 AdcCalib 与锁相角都已经是新值 */
    AdcSampleFast();
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

void CtrlSetPwmEnable(Uint16 u16Enable)
{
    gCtrlPwmEnable = (u16Enable != 0U) ? 1U : 0U;
}

Uint16 CtrlGetPwmEnable(void)
{
    return gCtrlPwmEnable;
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

/* 外部保护任务置故障码：只允许“无故障 -> 有故障”方向升级，避免覆盖已锁存的故障。 */
void CtrlSetFault(Uint16 u16Fault)
{
    if (u16Fault == (Uint16)CTRL_FAULT_NONE)
    {
        return;
    }

    if (gCtrlFault == (Uint16)CTRL_FAULT_NONE)
    {
        gCtrlFault = u16Fault;
        gCtrlObsFault = gCtrlFault;
    }
}

/* 清故障码：同时把调制比清零，要求重新给授权才可能再发波。 */
void CtrlClearFault(void)
{
    ProtectReset();
    gCtrlFault = (Uint16)CTRL_FAULT_NONE;
    gCtrlObsFault = gCtrlFault;
    gCtrlModIndex = 0.0f;
    gCtrlPwmEnable = 0U;

    CtrlAuthBlock();
    gCtrlObsAuthState = gCtrlAuthState;
    gCtrlObsAuthFlags = gCtrlAuthFlags;
    gCtrlObsModActual = gCtrlModActual;
}

/* ==================== 发波授权对外接口 ==================== */

Uint16 CtrlGetAuthState(void)
{
    return gCtrlAuthState;
}

float CtrlGetModActual(void)
{
    return gCtrlModActual;
}

Uint16 CtrlGetAuthFlags(void)
{
    return gCtrlAuthFlags;
}

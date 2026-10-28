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

/* 台架调试计数：CLA1_1 中断次数 / Timer1 驱动次数 */
volatile Uint16 gCtrlTestClaCnt   = 0U;
volatile Uint16 gCtrlTestTmrCnt   = 0U;

/* 台架静态自检快照：在 CtrlTestPwmStart() 末尾抓，烧一次就能判读
 *   TbCtlInv  : bit0-1 = CTRMODE（0=UP 1=DOWN 2=UPDOWN），bit2 = PHSEN
 *   AqcsfrcInv: 0x0000 = 已放行；0x0005 = A/B 双路强制低（仍被封）
 *   CmpaInv   : 应等于 1250
 *   TzflgInv  : 应为 0（清完立即抓）
 *   TzselInv  : 应为 0（TZ 闸门已拆）
 *   TbCtrA/TbCtrB: EPwm1 前后两次采样，不同 => 载波在跑 */
volatile Uint16 gCtrlDiagTbCtlInv   = 0U;
volatile Uint16 gCtrlDiagAqcsfrcInv = 0U;
volatile Uint16 gCtrlDiagCmpaInv    = 0U;
volatile Uint16 gCtrlDiagTzflgInv   = 0U;
volatile Uint16 gCtrlDiagTzselInv   = 0U;
volatile Uint16 gCtrlDiagTbCtrA     = 0U;
volatile Uint16 gCtrlDiagTbCtrB     = 0U;
volatile Uint16 gCtrlDiagTbCtrPvA   = 0U;
volatile Uint16 gCtrlDiagTbCtrPvB   = 0U;
volatile Uint16 gCtrlDiagTbCtrC     = 0U;
volatile Uint16 gCtrlDiagTbCtrD     = 0U;
volatile Uint16 gCtrlDiagInv2Cmpa    = 0U;
volatile Uint16 gCtrlDiagInv2Aqcsfrc = 0U;
volatile Uint16 gCtrlDiagInv2Tzctl   = 0U;
volatile Uint16 gCtrlDiagPvCmpa      = 0U;
volatile Uint16 gCtrlDiagPvAqcsfrc   = 0U;
volatile Uint16 gCtrlDiagPvAqctla    = 0U;
volatile Uint16 gCtrlDiagPvTzctl     = 0U;
volatile Uint16 gCtrlDiagPvGldctl    = 0U;
volatile Uint16 gCtrlDiagPvTbprd     = 0U;
volatile Uint16 gCtrlDiagPvXlink     = 0U;
volatile Uint16 gCtrlDiagBusDbctl    = 0U;
volatile Uint16 gCtrlDiagBusAqctlb   = 0U;
volatile Uint16 gCtrlDiagGpioMuxA1   = 0U;
volatile Uint16 gCtrlDiagGpioGmuxA1  = 0U;
volatile Uint16 gCtrlDiagGpioMuxA2   = 0U;
volatile Uint16 gCtrlDiagGpioGmuxA2  = 0U;
volatile Uint16 gCtrlDiagMuxCand     = 0U;
volatile Uint16 gCtrlDiagInvDbctl    = 0U;

volatile Uint16 gCtrlDiagPvAq[4]  = {0U, 0U, 0U, 0U};

volatile Uint16 gCtrlDiagXbarIn[6]  = {0U, 0U, 0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagTripCfg[4] = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagTripEn[4]  = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagGpioADat   = 0U;
volatile Uint16 gCtrlDiagGpioADir   = 0U;
volatile Uint16 gCtrlDiagGpioBDat   = 0U;
volatile Uint16 gCtrlDiagGpioBDir   = 0U;
volatile Uint16 gCtrlDiagPvTzflg[4] = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagPvDcsel[4] = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagPvCmp[4] = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagPvCtl[4] = {0U, 0U, 0U, 0U};
volatile Uint16 gCtrlDiagPvRun[4] = {0U, 0U, 0U, 0U};

/* CPU Timer1 中断排查用现场快照 + XINT 触发计数 */
volatile Uint16 gCtrlDiagPieIfr1   = 0U;
volatile Uint16 gCtrlDiagPieAck   = 0U;
volatile Uint16 gCtrlDiagT1Tcr    = 0U;
volatile Uint32 gCtrlDiagT1Tim    = 0UL;
volatile Uint16 gCtrlDiagIer      = 0U;
volatile Uint16 gCtrlDiagXint1Cnt = 0U;
volatile Uint16 gCtrlDiagXint2Cnt = 0U;

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

/* ==================== 发波授权 / 台架发波 ==================== */

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

    /* 台架模式：无条件放行，固定调制比（上面 CtrlIsrRun 已经分流，这里只兜底） */
    if (CTRL_TEST_PWM_MODE != 0)
    {
        gCtrlAuthFlags = (Uint16)CTRL_AUTH_FLAG_TEST;
        gCtrlAuthState = (Uint16)CTRL_AUTH_ON;
        gCtrlModActual = CTRL_TEST_MOD_INDEX;
        return;
    }

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

/* 台架发波：开环角度 + 固定调制比，不看锁相、不看零漂、不判故障。 */
static void CtrlTestRun(void)
{
    float fSin;
    float fCos;
    float fModR;
    float fModS;
    float fModT;

    gCtrlTheta += (CTRL_TWO_PI * CTRL_TEST_FREQ_HZ * CTRL_TS_SEC);

    if (gCtrlTheta >= CTRL_TWO_PI)
    {
        gCtrlTheta -= CTRL_TWO_PI;
    }

    fSin = sinf(gCtrlTheta);
    fCos = cosf(gCtrlTheta);

    fModR = CTRL_TEST_MOD_INDEX * fSin;
    fModS = CTRL_TEST_MOD_INDEX * ((-0.5f * fSin) - (0.86602540f * fCos));
    fModT = CTRL_TEST_MOD_INDEX * ((-0.5f * fSin) + (0.86602540f * fCos));

    Hw_InvSetDuty((Uint16)HW_INV_PHASE_R, fabsf(fModR), (fModR >= 0.0f) ? 1U : 0U);
    Hw_InvSetDuty((Uint16)HW_INV_PHASE_S, fabsf(fModS), (fModS >= 0.0f) ? 1U : 0U);
    Hw_InvSetDuty((Uint16)HW_INV_PHASE_T, fabsf(fModT), (fModT >= 0.0f) ? 1U : 0U);

    /* MUX 轮换：保持与正常模式相同的采样节奏 */
    gCtrlMuxIdx++;
    if (gCtrlMuxIdx >= (Uint16)CTRL_MUX_TABLE_NUM)
    {
        gCtrlMuxIdx = 0U;
    }

    gCtrlMuxAddr = gCtrlMuxTable[gCtrlMuxIdx];
    CtrlMuxWrite(gCtrlMuxAddr);
    CpuToCla1Msg.u16MuxAddr = gCtrlMuxAddr;

    /* 观测影子 */
    gCtrlObsThetaDeg  = gCtrlTheta * 57.2957795F;
    gCtrlObsGridFreq  = CTRL_TEST_FREQ_HZ;
    gCtrlObsModIndex  = CTRL_TEST_MOD_INDEX;
    gCtrlObsModActual = CTRL_TEST_MOD_INDEX;
    gCtrlObsAuthState = (Uint16)CTRL_AUTH_ON;
    gCtrlObsAuthFlags = (Uint16)CTRL_AUTH_FLAG_TEST;
    gCtrlObsFault     = gCtrlFault;

    gCtrlCycleCnt++;
}

/* 控制中断主体 */
/* 台架专用诊断：GPIO16 = EPWM9A(PV2)。
 * 其他 ePWM 输出脚都是 (GMUX=00,MUX=01) 就能出波，唯独 GPIO16 不出，
 * 高度怀疑该引脚的 EPWM9A 不在 PERIPH1 这一列。
 * 每 5 秒（20kHz × 100000）换一个 (GMUX,MUX) 候选，1..15 循环，
 * 示波器一旦看到 20kHz / 30% 方波，立刻读 gCtrlDiagMuxCand 即为正确复用号。
 * 台架专用：GPIO16 可能被临时映射成别的外设，仅功率级不带电时使用。 */
#if (CTRL_TEST_MUX_SWEEP != 0)
static void CtrlTestMuxSweep(void)
{
    static Uint32 sSweepCnt = 0UL;

    sSweepCnt++;
    if (sSweepCnt < 100000UL)
    {
        return;
    }

    sSweepCnt = 0UL;
    gCtrlDiagMuxCand++;
    if ((gCtrlDiagMuxCand < 1U) || (gCtrlDiagMuxCand > 15U))
    {
        gCtrlDiagMuxCand = 1U;
    }

    EALLOW;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO16 = (Uint16)((gCtrlDiagMuxCand >> 2U) & 0x3U);
    GpioCtrlRegs.GPAMUX2.bit.GPIO16  = (Uint16)(gCtrlDiagMuxCand & 0x3U);
    EDIS;
}
#endif

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

    /* ---------- 2b. 台架发波：开环角度 + 固定调制比，不判故障 ---------- */
    if (CTRL_TEST_PWM_MODE != 0)
    {
        /* 静态台架：波形由 CtrlTestPwmStart 一次写死，这里绝不能再改逆变，
         * 否则 ISR 每 50us 覆盖一次 CMPA/AQCSFRC，读回的是 ISR 的中间值，
         * 现场看到的 CMPA=2 / AQCSFRC=5 就是被这里覆盖的结果。 */
        if (CTRL_TEST_DYNAMIC != 0)
        {
            CtrlTestRun();
        }
#if (CTRL_TEST_MUX_SWEEP != 0)
        CtrlTestMuxSweep();
#endif
        AdcSampleFast();
        return;
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

/* ==================== 发波授权 / 台架发波的对外接口 ==================== */

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

/* 台架发波：把逆变六路、PV 四路、半母线平衡桥从软件钳位态释放。
 * 只在 CTRL_TEST_PWM_MODE = 1 时由 StartSystem() 调用。 */
/* 台架专用：把一路 ePWM 的跳闸闸门整个拆掉（TZSEL/TZCTL 全 0），
 * 并把它自己的强制位清零（AQCSFRC=0 才算真正放行输出）。
 * 功率级不带电时才允许这么干，正式固件必须靠 TZ 保护。 */
#define CTRL_TEST_TZ_PURGE(pwmRegs)                 \
    do {                                            \
        EALLOW;                                     \
        (pwmRegs).TZSEL.all   = 0x0000U;            \
        (pwmRegs).TZDCSEL.all = 0x0000U;            \
        (pwmRegs).TZCLR.all   = 0xFFFFU;            \
        (pwmRegs).TZCTL.all   = 0xFFFFU;            \
        (pwmRegs).AQCSFRC.all = 0x0000U;            \
        EDIS;                                       \
    } while (0)

void CtrlTestPwmStart(void)
{
    static const Uint16 sTestPvPwmId[4] = {HW_PWM_ID6, HW_PWM_ID8, HW_PWM_ID4, HW_PWM_ID5};
    Uint16 i;
    Uint16 u16Cmp;

    /* 先把 OST 锁存清掉，再把 11 路（不含风扇 EPWM12）的跳闸闸门整个拆掉。
     * 用户实测故障脚全高却仍被封波，说明压住输出的是 TZ 侧，
     * 台架阶段直接把 TZSEL/TZCTL 归零，让输出只受 AQCTL / CMPA 控制。 */
    Hw_PwmClearAllTripZone();

    CTRL_TEST_TZ_PURGE(EPwm1Regs);
    CTRL_TEST_TZ_PURGE(EPwm2Regs);
    CTRL_TEST_TZ_PURGE(EPwm3Regs);
    CTRL_TEST_TZ_PURGE(EPwm4Regs);
    CTRL_TEST_TZ_PURGE(EPwm5Regs);
    CTRL_TEST_TZ_PURGE(EPwm6Regs);
    CTRL_TEST_TZ_PURGE(EPwm7Regs);
    CTRL_TEST_TZ_PURGE(EPwm8Regs);
    CTRL_TEST_TZ_PURGE(EPwm9Regs);
    CTRL_TEST_TZ_PURGE(EPwm10Regs);
    CTRL_TEST_TZ_PURGE(EPwm11Regs);

    /* 逆变六路：初值给 0% 占空比（UPDOWN + CAU=SET/CAD=CLEAR 时 CMPA=TBPRD 即无脉冲），
     * 真正的三电平单极性波形由 20 kHz 控制中断里的 CtrlTestRun() -> Hw_InvSetDuty() 生成：
     *   正半周：正桥臂(EPWM1/2/3)斩波，负桥臂(EPWM4/10/11)两管全关、相点钳中点；
     *   负半周：负桥臂(EPWM4/10/11)斩波，正桥臂钳中点；过零两拍换相。
     * 这样上电瞬间不会出现六个桥臂同时 50% 对打。 */
    Hw_PwmSetCmpA(HW_PWM_ID0,  2500U);
    Hw_PwmSetCmpA(HW_PWM_ID1,  2500U);
    Hw_PwmSetCmpA(HW_PWM_ID2,  2500U);
    Hw_PwmSetCmpA(HW_PWM_ID3,  2500U);
    Hw_PwmSetCmpA(HW_PWM_ID9,  2500U);
    Hw_PwmSetCmpA(HW_PWM_ID10, 2500U);

    /* PV 四路 Boost：ZRO 置高、CAU 置低，占空比 = CMPA / 周期 = 1500/5000 = 30% */
    u16Cmp = (Uint16)((CTRL_TEST_PV_DUTY * (float)CTRL_TEST_PV_PERIOD) + 0.5f);

    for (i = 0U; i < 4U; i++)
    {
        Hw_PwmSetCmpA(sTestPvPwmId[i], u16Cmp);
    }

    /* PV 四路再绕过驱动层直接写一遍，排除任何封装/链路副作用：
     *   AQCTLA = 0x0012 => ZRO=SET(0x2) + CAU=CLEAR(0x1)，UP 计数下高电平占 CMPA/PRD
     *   AQCSFRC = 0     => 撤掉 A/B 强制
     *   TBCTL.CTRMODE  => UP
     * 然后把每路的 AQCTLA / CMPA / TBCTL / TBCTR 抓进快照，
     * 若 EPWM6 与 EPWM5/7/9 快照完全一致而只有 6 出波，问题就在芯片外（引脚/驱动/测量点）。 */
    EPwm7Regs.AQCTLA.all  = 0x0012U;
    EPwm7Regs.AQCSFRC.all = 0x0000U;
    EPwm7Regs.CMPA.bit.CMPA = u16Cmp;
    EPwm7Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;

    EPwm9Regs.AQCTLA.all  = 0x0012U;
    EPwm9Regs.AQCSFRC.all = 0x0000U;
    EPwm9Regs.CMPA.bit.CMPA = u16Cmp;
    EPwm9Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;

    EPwm5Regs.AQCTLA.all  = 0x0012U;
    EPwm5Regs.AQCSFRC.all = 0x0000U;
    EPwm5Regs.CMPA.bit.CMPA = u16Cmp;
    EPwm5Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;

    EPwm6Regs.AQCTLA.all  = 0x0012U;
    EPwm6Regs.AQCSFRC.all = 0x0000U;
    EPwm6Regs.CMPA.bit.CMPA = u16Cmp;
    EPwm6Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;

    /* 逆变六路同样直写 AQ：UPDOWN + CAU=SET(0x2@bits5:4) + CAD=CLEAR(0x1@bits7:6) = 0x0060 */
    EPwm1Regs.AQCTLA.all  = 0x0060U;
    EPwm1Regs.AQCSFRC.all = 0x0000U;
    EPwm2Regs.AQCTLA.all  = 0x0060U;
    EPwm2Regs.AQCSFRC.all = 0x0000U;
    EPwm3Regs.AQCTLA.all  = 0x0060U;
    EPwm3Regs.AQCSFRC.all = 0x0000U;
    EPwm4Regs.AQCTLA.all  = 0x0060U;
    EPwm4Regs.AQCSFRC.all = 0x0000U;
    EPwm10Regs.AQCTLA.all = 0x0060U;
    EPwm10Regs.AQCSFRC.all = 0x0000U;
    EPwm11Regs.AQCTLA.all = 0x0060U;
    EPwm11Regs.AQCSFRC.all = 0x0000U;

    /* 半母线平衡桥 EPWM8：CMPA 由配置表置 1250 => 50%，这里不用再动 */

    Hw_TrigGlobalLoad();

    /* --- 台架静态自检：一次烧写就能判定载波与强制位 --- */
    gCtrlDiagTbCtlInv   = EPwm1Regs.TBCTL.all;
    gCtrlDiagAqcsfrcInv = EPwm1Regs.AQCSFRC.all;
    gCtrlDiagCmpaInv    = EPwm1Regs.CMPA.bit.CMPA;
    gCtrlDiagTzflgInv   = EPwm1Regs.TZFLG.all;
    gCtrlDiagTzselInv   = EPwm1Regs.TZSEL.all;
    gCtrlDiagTbCtrA     = EPwm1Regs.TBCTR;
    gCtrlDiagTbCtrPvA   = EPwm7Regs.TBCTR;

    /* EPWM4 = R 相负半周，修好相配置表后它必须被写进 1250 */
    gCtrlDiagInv2Cmpa    = EPwm4Regs.CMPA.bit.CMPA;
    gCtrlDiagInv2Aqcsfrc = EPwm4Regs.AQCSFRC.all;
    gCtrlDiagInv2Tzctl   = EPwm4Regs.TZCTL.all;

    /* EPWM7 = PV1 组主，静态值应为 CMPA=1500、AQCSFRC=0、AQCTLA 的 ZRO=SET/CAU=CLEAR */
    gCtrlDiagPvCmpa    = EPwm7Regs.CMPA.bit.CMPA;
    gCtrlDiagPvAqcsfrc = EPwm7Regs.AQCSFRC.all;
    gCtrlDiagPvAqctla  = EPwm7Regs.AQCTLA.all;
    gCtrlDiagPvTzctl   = EPwm7Regs.TZCTL.all;
    gCtrlDiagPvGldctl  = EPwm7Regs.GLDCTL.all;
    gCtrlDiagPvTbprd   = EPwm7Regs.TBPRD;
    gCtrlDiagPvXlink   = EPwm7Regs.EPWMXLINK.all;

    /* EPWM8 = 半母线平衡桥，B 通道要靠死区模块反相生成 */
    gCtrlDiagBusDbctl  = EPwm8Regs.DBCTL.all;
    gCtrlDiagBusAqctlb = EPwm8Regs.AQCTLB.all;

    /* GPIO0~15 的复用与 GMUX，确认 EPWM1A/4A/7A/8B 对应的脚确实选了 ePWM */
    gCtrlDiagGpioMuxA1  = GpioCtrlRegs.GPAMUX1.all;
    gCtrlDiagGpioGmuxA1 = GpioCtrlRegs.GPAGMUX1.all;
    /* GPIO16~31 的复用：GPIO16 = EPWM9A(PV2)，在 GPAMUX2 里 */
    gCtrlDiagGpioMuxA2  = GpioCtrlRegs.GPAMUX2.all;
    gCtrlDiagGpioGmuxA2 = GpioCtrlRegs.GPAGMUX2.all;

    /* EPWM1 死区控制字：修正后应为 0x000B（OUT_MODE=3, POLSEL=2, IN_MODE=0） */
    gCtrlDiagInvDbctl = EPwm1Regs.DBCTL.all;

    /* PV 四路逐模块快照：[0]=EPWM7 [1]=EPWM9 [2]=EPWM5 [3]=EPWM6 */
    gCtrlDiagPvAq[0]  = EPwm7Regs.AQCTLA.all;
    gCtrlDiagPvAq[1]  = EPwm9Regs.AQCTLA.all;
    gCtrlDiagPvAq[2]  = EPwm5Regs.AQCTLA.all;
    gCtrlDiagPvAq[3]  = EPwm6Regs.AQCTLA.all;

    gCtrlDiagPvCmp[0] = EPwm7Regs.CMPA.bit.CMPA;
    gCtrlDiagPvCmp[1] = EPwm9Regs.CMPA.bit.CMPA;
    gCtrlDiagPvCmp[2] = EPwm5Regs.CMPA.bit.CMPA;
    gCtrlDiagPvCmp[3] = EPwm6Regs.CMPA.bit.CMPA;

    gCtrlDiagPvCtl[0] = EPwm7Regs.TBCTL.all;
    gCtrlDiagPvCtl[1] = EPwm9Regs.TBCTL.all;
    gCtrlDiagPvCtl[2] = EPwm5Regs.TBCTL.all;
    gCtrlDiagPvCtl[3] = EPwm6Regs.TBCTL.all;

    gCtrlDiagPvRun[0] = EPwm7Regs.TBCTR;
    gCtrlDiagPvRun[1] = EPwm9Regs.TBCTR;
    gCtrlDiagPvRun[2] = EPwm5Regs.TBCTR;
    gCtrlDiagPvRun[3] = EPwm6Regs.TBCTR;

    for (i = 0U; i < 4000U; i++)
    {
        asm(" NOP");
    }

    gCtrlDiagTbCtrB     = EPwm1Regs.TBCTR;
    gCtrlDiagTbCtrPvB   = EPwm7Regs.TBCTR;
    gCtrlDiagTbCtrC     = EPwm4Regs.TBCTR;
    gCtrlDiagTbCtrD     = EPwm8Regs.TBCTR;

    /* --- X-BAR 链路快照：PV1~PV4 的引脚到底有没有接进 TRIP4/5/7/8 --- */
    gCtrlDiagXbarIn[0] = InputXbarRegs.INPUT1SELECT;
    gCtrlDiagXbarIn[1] = InputXbarRegs.INPUT2SELECT;
    gCtrlDiagXbarIn[2] = InputXbarRegs.INPUT3SELECT;
    gCtrlDiagXbarIn[3] = InputXbarRegs.INPUT4SELECT;
    gCtrlDiagXbarIn[4] = InputXbarRegs.INPUT5SELECT;
    gCtrlDiagXbarIn[5] = InputXbarRegs.INPUT6SELECT;

    gCtrlDiagTripCfg[0] = (Uint16)EPwmXbarRegs.TRIP4MUX0TO15CFG.all;
    gCtrlDiagTripCfg[1] = (Uint16)EPwmXbarRegs.TRIP5MUX0TO15CFG.all;
    gCtrlDiagTripCfg[2] = (Uint16)EPwmXbarRegs.TRIP7MUX0TO15CFG.all;
    gCtrlDiagTripCfg[3] = (Uint16)EPwmXbarRegs.TRIP8MUX0TO15CFG.all;

    gCtrlDiagTripEn[0] = (Uint16)EPwmXbarRegs.TRIP4MUXENABLE.all;
    gCtrlDiagTripEn[1] = (Uint16)EPwmXbarRegs.TRIP5MUXENABLE.all;
    gCtrlDiagTripEn[2] = (Uint16)EPwmXbarRegs.TRIP7MUXENABLE.all;
    gCtrlDiagTripEn[3] = (Uint16)EPwmXbarRegs.TRIP8MUXENABLE.all;

    gCtrlDiagGpioADat = GpioDataRegs.GPADAT.all;
    gCtrlDiagGpioADir = GpioCtrlRegs.GPADIR.all;
    gCtrlDiagGpioBDat = GpioDataRegs.GPBDAT.all;
    gCtrlDiagGpioBDir = GpioCtrlRegs.GPBDIR.all;

    /* TZ 拆完之后每路的 TZFLG 必须为 0，TZDCSEL 也必须为 0 */
    gCtrlDiagPvTzflg[0] = EPwm7Regs.TZFLG.all;
    gCtrlDiagPvTzflg[1] = EPwm9Regs.TZFLG.all;
    gCtrlDiagPvTzflg[2] = EPwm5Regs.TZFLG.all;
    gCtrlDiagPvTzflg[3] = EPwm6Regs.TZFLG.all;

    gCtrlDiagPvDcsel[0] = EPwm7Regs.TZDCSEL.all;
    gCtrlDiagPvDcsel[1] = EPwm9Regs.TZDCSEL.all;
    gCtrlDiagPvDcsel[2] = EPwm5Regs.TZDCSEL.all;
    gCtrlDiagPvDcsel[3] = EPwm6Regs.TZDCSEL.all;

    gCtrlAuthState = (Uint16)CTRL_AUTH_ON;
    gCtrlModActual = CTRL_TEST_MOD_INDEX;
    gCtrlObsAuthState = gCtrlAuthState;
    gCtrlObsModActual = gCtrlModActual;
}

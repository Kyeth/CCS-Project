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
#include "InvStateMachine.h"
#include "PvStateMachine.h"
#include "SystemStateMachine.h"
#include "BusStateMachine.h"
#include "ProtectTask.h"

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

/* 控制中断全部运行状态：单个实例 stCtrlState，函数内一律经结构体访问 */
ST_CTRL_STATE stCtrlState;

/* 观测影子量：供软示波器与 Modbus 只读镜像按地址取用 */
ST_CTRL_OBS stCtrlObs;

/* 中断执行周期占用量测 */
ST_CTRL_MARGIN stCtrlMargin;

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

/* ==================== 状态机分层调度 ====================
 * 20kHz 控制中断是唯一节拍源，状态机按固定整数拍分频，抖动由硬件保证。
 * 分频只决定状态机多久跑一次，控制环路（PLL + 调制 + 下发）仍每拍 20kHz。
 *   Protect÷2 -> 10kHz（软件保护判定，先于状态机执行）
 *   Inv    ÷2  -> 10kHz（含 PV 电压/电流环，环路上限要求 10kHz）
 *   Bus    ÷4  ->  5kHz
 *   Pv     ÷2  -> 10kHz（与 Inv 同拍，PI 环率不变；MPPT 外层由状态内计数器分频）
 *   Sys    ÷20 ->  1kHz
 */
#define CTRL_SM_DIV_INV_PV      (2U)
#define CTRL_SM_DIV_BUS         (4U)
#define CTRL_SM_DIV_SYS         (20U)

static void CtrlSmSchedulerRun(void)
{
    stCtrlState.u16SmDivInvPv++;
    if (stCtrlState.u16SmDivInvPv >= CTRL_SM_DIV_INV_PV)
    {
        stCtrlState.u16SmDivInvPv = 0U;
        RunProtectTask();        /* 软件保护判定先跑，本拍结论供状态机读取 */
        RunInvStateMachine();
        RunPvStateMachine();
    }

    stCtrlState.u16SmDivBus++;
    if (stCtrlState.u16SmDivBus >= CTRL_SM_DIV_BUS)
    {
        stCtrlState.u16SmDivBus = 0U;
        RunBusStateMachine();
    }

    stCtrlState.u16SmDivSys++;
    if (stCtrlState.u16SmDivSys >= CTRL_SM_DIV_SYS)
    {
        stCtrlState.u16SmDivSys = 0U;
        RunSysStateMachine();
    }
}

/* 初始化 */
void CtrlIsrInit(void)
{
    stCtrlState.fTheta = 0.0f;
    stCtrlState.fOmega = CTRL_TWO_PI * 50.0f;
    stCtrlState.fModIndex = 0.0f;
    stCtrlState.u16Fault = (Uint16)CTRL_FAULT_NONE;
    stCtrlState.u16MuxIdx = 0U;
    stCtrlState.u16MuxAddr = CLA_MUX_ADDR_PV1;
    stCtrlState.u16SeqLast = 0U;
    stCtrlState.u16SeqStall = 0U;
    stCtrlState.u16CycleCnt = 0U;

    stCtrlState.fModActual = 0.0f;
    stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_OFF;
    stCtrlState.u16AuthCnt   = 0U;
    stCtrlState.u16AuthFlags = 0U;

    stCtrlObs.u16AuthState = stCtrlState.u16AuthState;
    stCtrlObs.u16AuthFlags = stCtrlState.u16AuthFlags;
    stCtrlObs.fModActual = stCtrlState.fModActual;

    /* CLA 侧默认使能滤波归档 */
    CpuToCla1Msg.u16Enable = 1U;
    CpuToCla1Msg.u16MuxAddr = stCtrlState.u16MuxAddr;
    CpuToCla1Msg.fTheta = 0.0f;

    CtrlMuxWrite(stCtrlState.u16MuxAddr);

    ProtectInit();
    ProtectInputInit();
    InitBusStateMachineData();
}

/* ==================== 发波授权 ==================== */

/* 严重封锁：把三相压回中点钳位，调制比清零，授权退回 OFF。
 * 只在"真的需要动硬件"时调一次 Hw_InvStopAll()，避免 20kHz 每拍写 AQCSFRC。 */
static void CtrlAuthBlock(void)
{
    if ((stCtrlState.u16AuthState != (Uint16)CTRL_AUTH_OFF) || (stCtrlState.fModActual > 0.0f))
    {
        Hw_InvStopAll();
    }

    stCtrlState.fModActual = 0.0f;
    stCtrlState.u16AuthCnt   = 0U;
    stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_OFF;
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

    stCtrlState.u16AuthFlags = 0U;

    /* 封锁条件 1：控制层故障（CLA 停摆、逆变过流、硬件过流/短路） */
    if (stCtrlState.u16Fault != (Uint16)CTRL_FAULT_NONE)
    {
        stCtrlState.u16AuthFlags |= (Uint16)CTRL_AUTH_FLAG_FAULT;
    }

    /* 封锁条件 2：ADC 零漂标定未完成，此时的占空比没有物理意义 */
    if (AdcCalibGetZeroState() != ADC_CAL_ZERO_DONE)
    {
        stCtrlState.u16AuthFlags |= (Uint16)CTRL_AUTH_FLAG_CALIB;
    }

    if (stCtrlState.u16AuthFlags != 0U)
    {
        CtrlAuthBlock();
        return;
    }

    switch (stCtrlState.u16AuthState)
    {
        case CTRL_AUTH_OFF:
            stCtrlState.fModActual = 0.0f;
            if (stCtrlState.u16PwmEnable != 0U)
            {
                stCtrlState.u16AuthCnt   = 0U;
                stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_WAIT;
            }
            break;

        case CTRL_AUTH_WAIT:
            stCtrlState.fModActual = 0.0f;
            if (stCtrlState.u16PwmEnable == 0U)
            {
                stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_OFF;
                break;
            }

            stCtrlState.u16AuthCnt++;
            if (stCtrlState.u16AuthCnt >= CTRL_AUTH_WAIT_MAX)
            {
                CtrlAuthArm();
                stCtrlState.u16AuthCnt   = 0U;
                stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_ON;
            }
            break;

        case CTRL_AUTH_ON:
            if (stCtrlState.u16PwmEnable == 0U)
            {
                stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_SOFT_OFF;
                break;
            }

            /* 实际调制比向请求值双侧跟随（请求值由 CtrlSetModIndex 限幅在 0~0.90） */
            fStep = CTRL_AUTH_SLEW_UP;
            if (stCtrlState.fModActual < stCtrlState.fModIndex)
            {
                stCtrlState.fModActual += fStep;
                if (stCtrlState.fModActual > stCtrlState.fModIndex)
                {
                    stCtrlState.fModActual = stCtrlState.fModIndex;
                }
            }
            else
            {
                stCtrlState.fModActual -= fStep;
                if (stCtrlState.fModActual < stCtrlState.fModIndex)
                {
                    stCtrlState.fModActual = stCtrlState.fModIndex;
                }
            }
            break;

        case CTRL_AUTH_SOFT_OFF:
            fStep = CTRL_AUTH_SLEW_DN;
            if (stCtrlState.fModActual > fStep)
            {
                stCtrlState.fModActual -= fStep;
            }
            else
            {
                /* 调制比已归零：真正封波，回 OFF 等下一次请求 */
                stCtrlState.fModActual = 0.0f;
                Hw_InvStopAll();
                stCtrlState.u16AuthCnt   = 0U;
                stCtrlState.u16AuthState = (Uint16)CTRL_AUTH_OFF;
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

    stCtrlState.u16CycleCnt++;

    /* ---------- 0. 保护框架：10ms 节拍推进记录、去抖与协议聚合 ---------- */
    if (stCtrlState.u16ProtectDiv >= 200U)
    {
        stCtrlState.u16ProtectDiv = 0U;
        ProtectInputRun10ms();
        ProtectRun10ms();
    }
    else
    {
        stCtrlState.u16ProtectDiv++;
    }

    stCtrlState.u16CycleCnt++;

    /* ---------- 0. ADC 标定刷新：码值 -> 工程值 ---------- */
    /* 本拍所有消费者（过流判定、PLL、软示波器）都从 AdcCalib 取数，
     * 保证同一拍看到的是同一份数据；上电零漂检测状态机也在里面推进 */
    AdcCalibRefresh();

    /* ---------- 1. CLA 心跳检测 ---------- */
    u16Seq = Cla1ToCpuMsg.u16Seq;

    if (u16Seq == stCtrlState.u16SeqLast)
    {
        if (stCtrlState.u16SeqStall < 0xFFFFU)
        {
            stCtrlState.u16SeqStall++;
        }
    }
    else
    {
        stCtrlState.u16SeqStall = 0U;
        stCtrlState.u16SeqLast = u16Seq;
    }

    if (stCtrlState.u16SeqStall > CTRL_CLA_TIMEOUT_NUM)
    {
        stCtrlState.u16Fault = (Uint16)CTRL_FAULT_CLA_TIMEOUT;
    }

    /* ---------- 1b. 后台任务活性：停摆即按故障处理，撤销发波授权 ---------- */
    if (Task_GetHealthFault() != 0U)
    {
        stCtrlState.u16Fault = (Uint16)CTRL_FAULT_TASK_STALL;
    }

    /* ---------- 1c. 保护框架：未屏蔽的 STOP 及以上故障直接封锁发波 ---------- */
    if (ProtectGetTotalSign() != 0U)
    {
        stCtrlState.u16Fault = (Uint16)CTRL_FAULT_PROTECT;
    }

    /* ---------- 2. 过流封波检测（三路逆变电流瞬时值） ---------- */
    if (stCtrlState.u16Fault == (Uint16)CTRL_FAULT_NONE)
    {
        fCur = AdcCalibInvCur(CLA_IDX_R);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            stCtrlState.u16Fault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = AdcCalibInvCur(CLA_IDX_S);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            stCtrlState.u16Fault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }

        fCur = AdcCalibInvCur(CLA_IDX_T);
        if (fabsf(fCur) > CTRL_INV_CUR_LIMIT)
        {
            stCtrlState.u16Fault = (Uint16)CTRL_FAULT_OVERCURRENT;
        }
    }

    if (stCtrlState.u16Fault != (Uint16)CTRL_FAULT_NONE)
    {
        /* 故障：三相封波到中点钳位，调制比清零，授权退回 OFF */
        CtrlAuthBlock();
        stCtrlState.fModIndex = 0.0f;
        stCtrlObs.u16Fault = stCtrlState.u16Fault;
        return;
    }

    /* ---------- 2b. 状态机分层调度：状态机才是控制主体 ---------- */
    CtrlSmSchedulerRun();

    /* ---------- 2c. 发波授权：裁定本拍能否把占空比写进硬件 ---------- */
    CtrlAuthRun();

    /* ---------- 3. SRF-PLL：三相电网电压 -> Clarke -> 锁相 ---------- */
    fValpha = (2.0f / 3.0f) * (AdcCalibGridVolt(CLA_IDX_R)
                               - (0.5f * AdcCalibGridVolt(CLA_IDX_S))
                               - (0.5f * AdcCalibGridVolt(CLA_IDX_T)));

    fVbeta = 0.577350269f * (AdcCalibGridVolt(CLA_IDX_R)
                             + (2.0f * AdcCalibGridVolt(CLA_IDX_S)));

    fSin = sinf(stCtrlState.fTheta);
    fCos = cosf(stCtrlState.fTheta);

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
    stCtrlState.fOmega += CTRL_PLL_KI * fVq * CTRL_TS_SEC;

    if (stCtrlState.fOmega < (CTRL_TWO_PI * CTRL_PLL_FREQ_MIN))
    {
        stCtrlState.fOmega = CTRL_TWO_PI * CTRL_PLL_FREQ_MIN;
    }

    if (stCtrlState.fOmega > (CTRL_TWO_PI * CTRL_PLL_FREQ_MAX))
    {
        stCtrlState.fOmega = CTRL_TWO_PI * CTRL_PLL_FREQ_MAX;
    }

    fDTheta = (stCtrlState.fOmega + (CTRL_PLL_KP * fVq)) * CTRL_TS_SEC;
    stCtrlState.fTheta = CtrlWrapTheta(stCtrlState.fTheta + fDTheta);

    /* 用角度增量的小角度展开递推 sin/cos：
     * fDTheta 最大约 0.022rad（70Hz），三阶截断误差 < 1e-9，
     * 这样一拍控制中断只需要 1 次 sinf + 1 次 cosf 库调用。 */

    fSinNew = (fSin * (1.0f - (0.5f * fDTheta * fDTheta)))
            + (fCos * (fDTheta - ((fDTheta * fDTheta * fDTheta) * 0.16666667f)));
    fCosNew = (fCos * (1.0f - (0.5f * fDTheta * fDTheta)))
            - (fSin * (fDTheta - ((fDTheta * fDTheta * fDTheta) * 0.16666667f)));

    /* 角度同步给 CLA（预留给 CLA 侧做坐标变换） */
    CpuToCla1Msg.fTheta = stCtrlState.fTheta;

    /* ---------- 4. 生成三相调制波并下发 ----------
     * 三电平单极性：|m*sin| 就是斩波占空比，sin 的符号决定哪一对管子斩波，
     * 过零点处 |sin| -> 0，占空比自然归零，所以换相是软换相。 */
    fModR = stCtrlState.fModActual * fSinNew;
    fModS = stCtrlState.fModActual * ((-0.5f * fSinNew) - (0.86602540f * fCosNew));
    fModT = stCtrlState.fModActual * ((-0.5f * fSinNew) + (0.86602540f * fCosNew));

    /* 只有授权态 ON 且实际调制比非 0 才写硬件：其余状态下桥臂由授权状态机
     * 负责压回钳位（CtrlAuthBlock / SOFT_OFF 归零时各做一次），这里不再每拍
     * 调 Hw_InvStopAll()，避免以 20kHz 反复写 AQCSFRC。 */
    if ((stCtrlState.u16AuthState == (Uint16)CTRL_AUTH_ON) && (stCtrlState.fModActual > 0.0f))
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
    stCtrlState.u16MuxIdx++;
    if (stCtrlState.u16MuxIdx >= (Uint16)CTRL_MUX_TABLE_NUM)
    {
        stCtrlState.u16MuxIdx = 0U;
    }

    stCtrlState.u16MuxAddr = gCtrlMuxTable[stCtrlState.u16MuxIdx];

    CtrlMuxWrite(stCtrlState.u16MuxAddr);
    CpuToCla1Msg.u16MuxAddr = stCtrlState.u16MuxAddr;

    /* ---------- 6. 刷新观测影子量（软示波器用） ---------- */
    stCtrlObs.fThetaDeg = stCtrlState.fTheta * 57.2957795F;   /* rad -> deg */
    stCtrlObs.fGridFreq = stCtrlState.fOmega * 0.15915494F;   /* rad/s -> Hz */
    stCtrlObs.fModIndex  = stCtrlState.fModIndex;
    stCtrlObs.fModActual = stCtrlState.fModActual;
    stCtrlObs.u16Fault     = stCtrlState.u16Fault;
    stCtrlObs.u16AuthState = stCtrlState.u16AuthState;
    stCtrlObs.u16AuthFlags = stCtrlState.u16AuthFlags;

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

    stCtrlState.fModIndex = fModIndex;
}

float CtrlGetModIndex(void)
{
    return stCtrlState.fModIndex;
}

void CtrlSetPwmEnable(Uint16 u16Enable)
{
    stCtrlState.u16PwmEnable = (u16Enable != 0U) ? 1U : 0U;
}

Uint16 CtrlGetPwmEnable(void)
{
    return stCtrlState.u16PwmEnable;
}

float CtrlGetTheta(void)
{
    return stCtrlState.fTheta;
}

float CtrlGetGridFreq(void)
{
    return stCtrlState.fOmega / CTRL_TWO_PI;
}

Uint16 CtrlGetFault(void)
{
    return stCtrlState.u16Fault;
}

Uint16 CtrlGetMuxAddr(void)
{
    return stCtrlState.u16MuxAddr;
}

Uint16 CtrlGetCycleCnt(void)
{
    return stCtrlState.u16CycleCnt;
}

/* 外部保护任务置故障码：只允许“无故障 -> 有故障”方向升级，避免覆盖已锁存的故障。 */
void CtrlSetFault(Uint16 u16Fault)
{
    if (u16Fault == (Uint16)CTRL_FAULT_NONE)
    {
        return;
    }

    if (stCtrlState.u16Fault == (Uint16)CTRL_FAULT_NONE)
    {
        stCtrlState.u16Fault = u16Fault;
        stCtrlObs.u16Fault = stCtrlState.u16Fault;
    }
}

/* 清故障码：同时把调制比清零，要求重新给授权才可能再发波。 */
void CtrlClearFault(void)
{
    ProtectReset();
    stCtrlState.u16Fault = (Uint16)CTRL_FAULT_NONE;
    stCtrlObs.u16Fault = stCtrlState.u16Fault;
    stCtrlState.fModIndex = 0.0f;
    stCtrlState.u16PwmEnable = 0U;

    CtrlAuthBlock();
    stCtrlObs.u16AuthState = stCtrlState.u16AuthState;
    stCtrlObs.u16AuthFlags = stCtrlState.u16AuthFlags;
    stCtrlObs.fModActual = stCtrlState.fModActual;
}

/* ==================== 发波授权对外接口 ==================== */

Uint16 CtrlGetAuthState(void)
{
    return stCtrlState.u16AuthState;
}

float CtrlGetModActual(void)
{
    return stCtrlState.fModActual;
}

Uint16 CtrlGetAuthFlags(void)
{
    return stCtrlState.u16AuthFlags;
}

/* ==================== 中断余量测量 ====================
 * Timer2 是 200MHz 自由运行递减计数器：入口读一次、出口读一次，
 * 两者之差就是本拍控制中断占用的 CPU 周期数（1 周期 = 5ns）。 */
static Uint32 sCtrlIsrStartCnt = 0UL;

void CtrlIsrMarginBegin(void)
{
    sCtrlIsrStartCnt = CpuTimer2Regs.TIM.all;
}

void CtrlIsrMarginEnd(void)
{
    Uint32 u32Used;
    Uint16 u16Used;

    /* 递减计数器：入口值减出口值即经过的周期数 */
    u32Used = sCtrlIsrStartCnt - CpuTimer2Regs.TIM.all;

    /* 单拍不可能超过 1ms（200000 周期），超出说明计数器被复位过，丢弃该样本 */
    if (u32Used > 200000UL)
    {
        return;
    }

    u16Used = (Uint16)u32Used;

    stCtrlMargin.u16LastCnt = u16Used;

    if (u16Used > stCtrlMargin.u16MaxCnt)
    {
        stCtrlMargin.u16MaxCnt = u16Used;
    }

    if ((stCtrlMargin.u16SampleCnt == 0U) || (u16Used < stCtrlMargin.u16MinCnt))
    {
        stCtrlMargin.u16MinCnt = u16Used;
    }

    if (stCtrlMargin.u16SampleCnt < 0xFFFFU)
    {
        stCtrlMargin.u16SampleCnt++;
    }

    stCtrlMargin.u32AccCnt += u32Used;

    if (u16Used > (Uint16)CTRL_ISR_BUDGET_CNT)
    {
        if (stCtrlMargin.u16OverCnt < 0xFFFFU)
        {
            stCtrlMargin.u16OverCnt++;
        }
    }
}

/* 最近一拍占用周期数 */
Uint16 CtrlGetIsrCnt(void)
{
    return stCtrlMargin.u16LastCnt;
}

/* 历史最小余量（周期数），预算已耗尽时返回 0 */
Uint16 CtrlGetMarginMinCnt(void)
{
    if (stCtrlMargin.u16MaxCnt >= (Uint16)CTRL_ISR_BUDGET_CNT)
    {
        return 0U;
    }

    return (Uint16)((Uint16)CTRL_ISR_BUDGET_CNT - stCtrlMargin.u16MaxCnt);
}

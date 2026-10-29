/**
 * @file InvStateMachine.c
 * @author yetingheng (004182)
 * @brief 逆变状态机实现（后台1kHz）
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 */

#include "InvStateMachine.h"
#include "Hw_InvDriver.h"
#include "CtrlIsr.h"
#include "AdcSample.h"
#include "RelayCtl.h"
#include "AtsCtl.h"
#include "AdcCalib.h"

ST_INV_SM gInvSm;

/******************************************************************************
 * 内部工具
 ******************************************************************************/

/* 状态切换：清计时与上次状态 */
static void InvSmEnter(Uint16 u16State)
{
    gInvSm.u16LastState = gInvSm.u16State;
    gInvSm.u16State = u16State;
    gInvSm.u16Cnt = 0U;
}

/* 故障：先封波断继电器，再进 ERROR */
static void InvSmFault(Uint16 u16Fault)
{
    gInvSm.u16Fault |= u16Fault;
    CtrlSetPwmEnable(0U);
    CtrlSetModIndex(0.0f);
    Hw_InvTripAll();
    RelayCtlInvAll(0U);
    gInvSm.fModRef = 0.0f;
    InvSmEnter((Uint16)INV_ST_ERROR);
}

/* 刷新电压快照：桥臂侧取逆变电压 RMS，负载侧取电网电压 RMS（ATS 闭合时即电网） */
static void InvSmSample(void)
{
    gInvSm.fVoutRms = stSampleRms.fInvVoltRms[SAMPLE_PH_R];
    gInvSm.fLoadRms = stSampleRms.fGridVoltRms[SAMPLE_PH_R];
}

/* 绝对值（避免引入 math.h） */
static float InvSmAbs(float fVal)
{
    return (fVal >= 0.0f) ? fVal : -fVal;
}

/* 继电器两端电压差：桥臂侧与负载侧 RMS 之差 */
static float InvSmVoltDiff(void)
{
    return InvSmAbs(gInvSm.fLoadRms - gInvSm.fVoutRms);
}

/* 幅值闭环：按 RMS 误差修正调制比，限幅在 [0, MOD_MAX] */
static void InvSmAmpLoop(void)
{
    gInvSm.fModRef += INV_SM_AMP_KP * (gInvSm.fAmpTarget - gInvSm.fVoutRms);
    if (gInvSm.fModRef > INV_SM_MOD_MAX)
    {
        gInvSm.fModRef = INV_SM_MOD_MAX;
    }
    if (gInvSm.fModRef < 0.0f)
    {
        gInvSm.fModRef = 0.0f;
    }
    CtrlSetModIndex(gInvSm.fModRef);
}

/* 幅值目标：并网（ATS 闭合）时跟随电网，离网时取额定值 */
static void InvSmAmpTarget(void)
{
    if ((AtsGetState() != 0U) && (gInvSm.fLoadRms > 100.0f))
    {
        gInvSm.fAmpTarget = gInvSm.fLoadRms;
    }
    else
    {
        gInvSm.fAmpTarget = INV_SM_AMP_REF;
    }
}

/******************************************************************************
 * 初始化
 ******************************************************************************/
void InvSmInit(void)
{
    gInvSm.u16State = (Uint16)INV_ST_INIT;
    gInvSm.u16LastState = (Uint16)INV_ST_INIT;
    gInvSm.u16Fault = INV_SM_FAULT_NONE;
    gInvSm.u16StartReq = 0U;
    gInvSm.u16StopReq = 0U;
    gInvSm.fModRef = 0.0f;
    gInvSm.fVoutRms = 0.0f;
    gInvSm.fLoadRms = 0.0f;
    gInvSm.fAmpTarget = INV_SM_AMP_REF;
    gInvSm.u16Cnt = 0U;

    CtrlSetPwmEnable(0U);
    CtrlSetModIndex(0.0f);
    RelayCtlInvAll(0U);
}

void InvSmStart(void)
{
    if (gInvSm.u16Fault == INV_SM_FAULT_NONE)
    {
        gInvSm.u16StartReq = 1U;
        gInvSm.u16StopReq = 0U;
    }
}

void InvSmStop(void)
{
    gInvSm.u16StartReq = 0U;
    gInvSm.u16StopReq = 1U;
}

void InvSmReset(void)
{
    gInvSm.u16Fault = INV_SM_FAULT_NONE;
    gInvSm.u16StartReq = 0U;
    gInvSm.u16StopReq = 0U;
    gInvSm.fModRef = 0.0f;
    CtrlSetPwmEnable(0U);
    CtrlSetModIndex(0.0f);
    RelayCtlInvAll(0U);
    InvSmEnter((Uint16)INV_ST_SELF);
}

Uint16 InvSmGetState(void)
{
    return gInvSm.u16State;
}

Uint16 InvSmGetFault(void)
{
    return gInvSm.u16Fault;
}

/******************************************************************************
 * 主任务：后台1kHz
 ******************************************************************************/
void RunInvStateMachine(void)
{
    if (gInvSm.u16Cnt < 0xFFFFU)
    {
        gInvSm.u16Cnt++;
    }

    InvSmSample();

    switch (gInvSm.u16State)
    {
        case (Uint16)INV_ST_INIT:
            CtrlSetPwmEnable(0U);
            CtrlSetModIndex(0.0f);
            RelayCtlInvAll(0U);
            InvSmEnter((Uint16)INV_ST_SELF);
            break;

        case (Uint16)INV_ST_SELF:
            CtrlSetPwmEnable(0U);
            RelayCtlInvAll(0U);
            gInvSm.fModRef = 0.0f;
            if (gInvSm.u16StartReq != 0U)
            {
                gInvSm.u16StartReq = 0U;
                gInvSm.u16Cnt = 0U;
                InvSmEnter((Uint16)INV_ST_CHK);
            }
            break;

        case (Uint16)INV_ST_CHK:
            /* 继电器保持断开：按幅值闭环把桥臂建到目标电压，再判建压/粘连 */
            InvSmAmpTarget();
            InvSmAmpLoop();
            CtrlSetPwmEnable(1U);

            if (gInvSm.u16Cnt >= INV_SM_CHK_TICK)
            {
                CtrlSetPwmEnable(0U);
                CtrlSetModIndex(0.0f);
                gInvSm.fModRef = 0.0f;

                if (gInvSm.fVoutRms < INV_SM_CHK_VOUT_MIN)
                {
                    /* 桥臂建不起压：驱动、采样或功率级异常 */
                    InvSmFault(INV_SM_FAULT_SELFTEST);
                }
                else if (gInvSm.fVoutRms < (gInvSm.fAmpTarget - INV_SM_CLOSE_DIFF_V))
                {
                    /* 桥臂能起压但建不到目标：母线/占空比/采样链路异常 */
                    InvSmFault(INV_SM_FAULT_SELFTEST);
                }
                else if ((AtsGetState() == 0U) && (InvSmVoltDiff() < INV_SM_CHK_STICK_VOLT))
                {
                    /* ATS 断开、继电器断开时两端压差应接近桥臂全压（>50V）；
                       压差过小说明负载侧被桥臂带起来了 -> 触点粘连 */
                    InvSmFault(INV_SM_FAULT_STICK);
                }
                else
                {
                    gInvSm.u16Cnt = 0U;
                    InvSmEnter((Uint16)INV_ST_SOFT);
                }
            }
            break;

        case (Uint16)INV_ST_SOFT:
            /* 并网电流环、PLL锁定与相位/频差许可尚未实现，禁止RMS相等即合闸。 */
            if (AtsGetState() != 0U)
            {
                InvSmFault(INV_SM_FAULT_ATS);
                break;
            }
            InvSmAmpTarget();

            /* 离网：先闭合逆变继电器再升压；并网：保持断开，先建压 */
            if (AtsGetState() == 0U)
            {
                RelayCtlInvAll(1U);
            }

            CtrlSetPwmEnable(1U);

            /* 并网（ATS 闭合）：建压到电网幅值、且两端电压差 < 10V 才闭合，避免冲击
               离网（ATS 断开）：已先闭合继电器，直接按额定幅值升压 */
            if (AtsGetState() != 0U)
            {
                InvSmAmpLoop();
                if (InvSmVoltDiff() < INV_SM_CLOSE_DIFF_V)
                {
                    RelayCtlInvAll(1U);
                    gInvSm.u16Cnt = 0U;
                    InvSmEnter((Uint16)INV_ST_GRIDON);
                }
            }
            else
            {
                /* 离网：继电器已闭合，斜坡上升，接近目标后交幅值闭环收口 */
                if (gInvSm.fModRef < INV_SM_MOD_MAX)
                {
                    gInvSm.fModRef += INV_SM_RAMP_STEP;
                    if (gInvSm.fModRef > INV_SM_MOD_MAX)
                    {
                        gInvSm.fModRef = INV_SM_MOD_MAX;
                    }
                    CtrlSetModIndex(gInvSm.fModRef);
                }
                else
                {
                    InvSmAmpLoop();
                }

                /* 建压 300ms 且幅值到位才进运行，避免带偏压进闭环 */
                if ((gInvSm.u16Cnt >= 300U) &&
                    ((gInvSm.fAmpTarget - gInvSm.fVoutRms) < INV_SM_AMP_DIFF))
                {
                    gInvSm.u16Cnt = 0U;
                    InvSmEnter((Uint16)INV_ST_RUN);
                }
            }
            break;

        case (Uint16)INV_ST_GRIDON:
            CtrlSetPwmEnable(1U);
            if (gInvSm.u16Cnt >= INV_SM_GRIDON_TICK)
            {
                if ((gInvSm.fVoutRms < (INV_SM_GRIDON_MIN * gInvSm.fAmpTarget)) ||
                    (InvSmVoltDiff() > INV_SM_CLOSE_DIFF_V))
                {
                    /* 闭合后电压跌落，或两端仍有明显压差：触点没真正合上 */
                    InvSmFault(INV_SM_FAULT_GRIDON);
                }
                else
                {
                    InvSmEnter((Uint16)INV_ST_RUN);
                }
            }
            break;

        case (Uint16)INV_ST_RUN:
            if (AtsGetState() != 0U)
            {
                InvSmFault(INV_SM_FAULT_ATS);
                break;
            }
            InvSmAmpTarget();

            /* 幅值闭环：调制比按 RMS 误差积分式修正，限幅在 [MIN, MAX] */
            InvSmAmpLoop();
            CtrlSetPwmEnable(1U);

            if (CtrlGetFault() != 0U)
            {
                InvSmFault(INV_SM_FAULT_CTRL);
            }
            else if (gInvSm.u16StopReq != 0U)
            {
                gInvSm.u16StopReq = 0U;
                gInvSm.u16Cnt = 0U;
                InvSmEnter((Uint16)INV_ST_STOP);
            }
            break;

        case (Uint16)INV_ST_STOP:
            if (gInvSm.fModRef > INV_SM_RAMP_STEP_DN)
            {
                gInvSm.fModRef -= INV_SM_RAMP_STEP_DN;
                CtrlSetModIndex(gInvSm.fModRef);
            }
            else
            {
                gInvSm.fModRef = 0.0f;
                CtrlSetModIndex(0.0f);
                CtrlSetPwmEnable(0U);
                Hw_InvStopAll();
                RelayCtlInvAll(0U);
                InvSmEnter((Uint16)INV_ST_SELF);
            }
            break;

        case (Uint16)INV_ST_ERROR:
        default:
            CtrlSetPwmEnable(0U);
            CtrlSetModIndex(0.0f);
            RelayCtlInvAll(0U);
            break;
    }
}

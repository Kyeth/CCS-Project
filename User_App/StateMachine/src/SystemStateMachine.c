/**
 * @file SystemStateMachine.c
 * @author yetingheng (004182)
 * @brief 系统总状态机实现（1kHz）
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 */

#include "SystemStateMachine.h"
#include "Hw_InvDriver.h"
#include "InvStateMachine.h"
#include "AdcCalib.h"
#include "AdcSample.h"
#include "CtrlIsr.h"
#include "RelayCtl.h"
#include "AtsCtl.h"

ST_SYS_SM gSysSm;

/******************************************************************************
 * 内部工具
 ******************************************************************************/
static void SysSmEnter(Uint16 u16State)
{
    gSysSm.u16LastState = gSysSm.u16State;
    gSysSm.u16State = u16State;
    gSysSm.u16Cnt = 0U;
}

static void SysSmFault(Uint16 u16Fault)
{
    gSysSm.u16Fault |= u16Fault;
    InvSmStop();
    CtrlSetPwmEnable(0U);
    CtrlSetModIndex(0.0f);
    Hw_InvStopAll();
    RelayCtlInvAll(0U);
    SysSmEnter((Uint16)SYS_ST_FAULT);
}

/* 聚合子状态机与通讯侧的状态到镜像字段 */
static void SysSmMirror(void)
{
    gSysSm.u16InvState = InvSmGetState();
    gSysSm.u16InvFault = InvSmGetFault();

    if ((gSysSm.u16InvFault != INV_SM_FAULT_NONE) ||
        (CtrlGetFault() != 0U))
    {
        gSysSm.u16Fault |= SYS_FAULT_INV;
    }
    if (AtsIsTimeout() != 0U)
    {
        gSysSm.u16Fault |= SYS_FAULT_ATS;
    }
}

/******************************************************************************
 * API
 ******************************************************************************/
void SysSmInit(void)
{
    gSysSm.u16State = (Uint16)SYS_ST_INIT;
    gSysSm.u16LastState = (Uint16)SYS_ST_INIT;
    gSysSm.u16Fault = SYS_FAULT_NONE;
    gSysSm.u16StartCmd = 0U;
    gSysSm.u16ResetCmd = 0U;
    gSysSm.u16InvState = (Uint16)INV_ST_INIT;
    gSysSm.u16InvFault = INV_SM_FAULT_NONE;
    gSysSm.u32RunMs = 0UL;
    gSysSm.u16Cnt = 0U;

    CtrlSetPwmEnable(0U);
    RelayCtlInvAll(0U);
}

Uint16 SysSmGetState(void)
{
    return gSysSm.u16State;
}

Uint16 SysSmGetFault(void)
{
    return gSysSm.u16Fault;
}

Uint16 SysSmIsRun(void)
{
    return (gSysSm.u16State == (Uint16)SYS_ST_RUN) ? 1U : 0U;
}

/******************************************************************************
 * 主任务：1kHz
 ******************************************************************************/
void RunSysStateMachine(void)
{
    if (gSysSm.u16Cnt < 0xFFFFU)
    {
        gSysSm.u16Cnt++;
    }

    SysSmMirror();

    switch (gSysSm.u16State)
    {
        case (Uint16)SYS_ST_INIT:
            CtrlSetPwmEnable(0U);
            RelayCtlInvAll(0U);
            if (gSysSm.u16Cnt >= SYS_SM_INIT_MS)
            {
                SysSmEnter((Uint16)SYS_ST_WAIT_ZERO);
            }
            break;

        case (Uint16)SYS_ST_WAIT_ZERO:
            if (AdcCalibGetZeroState() == ADC_CAL_ZERO_DONE)
            {
                SysSmEnter((Uint16)SYS_ST_SELFTEST);
            }
            else if (AdcCalibGetZeroState() == ADC_CAL_ZERO_FAILED)
            {
                SysSmFault(SYS_FAULT_ZERO);
            }
            else
            {
                /* 零漂还在跟踪，继续等 */
            }
            break;

        case (Uint16)SYS_ST_SELFTEST:
            /* 本版自检：子状态机已就绪 + 采样层有有效数据。
             * 绝缘检测、继电器粘连自检已经在 InvSm 的 CHK 里，这里只做系统级闸门。 */
            if ((gSysSm.u16Fault & SYS_FAULT_INV) != 0U)
            {
                SysSmFault(SYS_FAULT_INV);
            }
            else if ((InvSmGetState() != (Uint16)INV_ST_SELF) &&
                     (InvSmGetState() != (Uint16)INV_ST_INIT))
            {
                SysSmFault(SYS_FAULT_SELFTEST);
            }
            else if (gSysSm.u16Cnt >= SYS_SM_SELFTEST_MS)
            {
                SysSmEnter((Uint16)SYS_ST_STANDBY);
            }
            else
            {
                /* 自检窗口内继续等 */
            }
            break;

        case (Uint16)SYS_ST_STANDBY:
            gSysSm.u32RunMs = 0UL;
            if (gSysSm.u16Fault != SYS_FAULT_NONE)
            {
                SysSmEnter((Uint16)SYS_ST_FAULT);
            }
            else if (gSysSm.u16StartCmd != 0U)
            {
                InvSmStart();
                SysSmEnter((Uint16)SYS_ST_STARTUP);
            }
            else
            {
                /* 待机 */
            }
            break;

        case (Uint16)SYS_ST_STARTUP:
            if ((gSysSm.u16Fault & (SYS_FAULT_INV | SYS_FAULT_ATS)) != 0U)
            {
                SysSmFault(gSysSm.u16Fault & (SYS_FAULT_INV | SYS_FAULT_ATS));
            }
            else if (InvSmGetState() == (Uint16)INV_ST_RUN)
            {
                gSysSm.u32RunMs = 0UL;
                SysSmEnter((Uint16)SYS_ST_RUN);
            }
            else if (gSysSm.u16Cnt >= SYS_SM_STARTUP_MS)
            {
                SysSmFault(SYS_FAULT_INV);
            }
            else
            {
                /* 等逆变侧建压/并网完成 */
            }
            break;

        case (Uint16)SYS_ST_RUN:
            gSysSm.u32RunMs++;
            if ((gSysSm.u16Fault & (SYS_FAULT_INV | SYS_FAULT_ATS)) != 0U)
            {
                SysSmFault(gSysSm.u16Fault & (SYS_FAULT_INV | SYS_FAULT_ATS));
            }
            else if (gSysSm.u16StartCmd == 0U)
            {
                InvSmStop();
                SysSmEnter((Uint16)SYS_ST_STOPPING);
            }
            else
            {
                /* 运行中 */
            }
            break;

        case (Uint16)SYS_ST_STOPPING:
            if (InvSmGetState() == (Uint16)INV_ST_SELF)
            {
                SysSmEnter((Uint16)SYS_ST_STANDBY);
            }
            else if (gSysSm.u16Cnt >= SYS_SM_STOPPING_MS)
            {
                /* 停机超时：强制回待机，避免卡死 */
                InvSmReset();
                SysSmEnter((Uint16)SYS_ST_STANDBY);
            }
            else
            {
                /* 等调制比回零、继电器断开 */
            }
            break;

        case (Uint16)SYS_ST_FAULT:
            InvSmStop();
            CtrlSetPwmEnable(0U);
            CtrlSetModIndex(0.0f);
            RelayCtlInvAll(0U);

            if (gSysSm.u16ResetCmd != 0U)
            {
                gSysSm.u16ResetCmd = 0U;
                if (InvSmGetFault() == INV_SM_FAULT_NONE)
                {
                    CtrlClearFault();
                    gSysSm.u16Fault = SYS_FAULT_NONE;
                    InvSmReset();
                    SysSmEnter((Uint16)SYS_ST_RECOVER);
                }
            }
            break;

        case (Uint16)SYS_ST_RECOVER:
            if (gSysSm.u16Cnt >= SYS_SM_RECOVER_MS)
            {
                SysSmEnter((Uint16)SYS_ST_STANDBY);
            }
            break;

        default:
            SysSmEnter((Uint16)SYS_ST_INIT);
            break;
    }
}

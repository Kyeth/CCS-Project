/**
 * @file BusStateMachine.c
 * @author yetingheng (004182)
 * @brief 半母线平衡状态机实现：三态迁移与压差测量（平衡环留 TODO）
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

#include "BusStateMachine.h"
#include "SampleData.h"

BUS_SYSTEM_DATA stBusSystem;
BUS_STATE_DATA  stBusState;

/* 压差慢平均系数：5kHz 调用，0.02 对应约 10ms 时间常数 */
#define BUS_SM_ALPHA            (0.02f)

static void BusSmEnterState(Uint16 u16State)
{
    stBusState.u16WorkState = u16State;
    stBusState.u16StateCnt  = 0U;

    /* 迁移沿动作：只有 Work 态需要把占空比拉回中点再爬 */
    if (u16State == (Uint16)BUS_WORK_STATE)
    {
        stBusSystem.u16StartCnt = 0U;
        stBusSystem.fBalDuty    = 0.5f;
    }
    else if (u16State == (Uint16)BUS_ERROR_STATE)
    {
        stBusSystem.fBalDuty    = 0.5f;
        stBusSystem.u16OcpCnt   = 0U;
    }
}

void InitBusStateMachineData(void)
{
    stBusState.u16WorkState = (Uint16)BUS_INIT_STATE;
    stBusState.u16StateCnt  = 0U;
    stBusState.u16LastState = (Uint16)BUS_INIT_STATE;

    stBusSystem.fBusPosVolt  = 0.0f;
    stBusSystem.fBusNegVolt  = 0.0f;
    stBusSystem.fBusDiff     = 0.0f;
    stBusSystem.fBusDiffAvg  = 0.0f;
    stBusSystem.fBalDuty     = 0.5f;
    stBusSystem.u16StartCnt  = 0U;
    stBusSystem.u16OcpCnt    = 0U;
}

void RunBusStateMachine(void)
{
    /* 压差测量：正母线减负母线，先做一阶慢平均滤掉开关纹波 */
    stBusSystem.fBusPosVolt = stSampleIns.fBusPosVoltIns;
    stBusSystem.fBusNegVolt = stSampleIns.fBusNegVoltIns;
    stBusSystem.fBusDiff    = stBusSystem.fBusPosVolt - stBusSystem.fBusNegVolt;
    stBusSystem.fBusDiffAvg += BUS_SM_ALPHA * (stBusSystem.fBusDiff - stBusSystem.fBusDiffAvg);

    if (stBusState.u16StateCnt < 0xFFFFU)
    {
        stBusState.u16StateCnt++;
    }

    switch (stBusState.u16WorkState)
    {
        case BUS_INIT_STATE:
            /* 等母线建立与采样稳定，再进平衡调节 */
            if (stBusState.u16StateCnt >= BUS_SM_INIT_TICK)
            {
                BusSmEnterState((Uint16)BUS_WORK_STATE);
            }
            break;

        case BUS_WORK_STATE:
            /* 压差异常退出调节；正常时在这里做平衡 PI 并下发 EPWM8
             * TODO(批 1.0)：PI 增益按台架实测压差整定后再接入 Hw_PwmSetDuty */
            if (fabsf(stBusSystem.fBusDiffAvg) > BUS_SM_DIFF_MAX_VOLT)
            {
                BusSmEnterState((Uint16)BUS_ERROR_STATE);
            }
            else if (fabsf(stBusSystem.fBusDiffAvg) < BUS_SM_DIFF_MIN_VOLT)
            {
                stBusSystem.fBalDuty = 0.5f;
            }
            break;

        case BUS_ERROR_STATE:
            /* 压差回落后重新进 Init，避免一直卡在故障态 */
            if (stBusState.u16StateCnt >= BUS_SM_ERR_TICK)
            {
                BusSmEnterState((Uint16)BUS_INIT_STATE);
            }
            break;

        default:
            BusSmEnterState((Uint16)BUS_INIT_STATE);
            break;
    }

    stBusState.u16LastState = stBusState.u16WorkState;
}

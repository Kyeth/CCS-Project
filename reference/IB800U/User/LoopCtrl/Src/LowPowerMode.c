/**
 * @file
 * @author yetingheng  (004182)
 * @brief
 * @version 0.1
 * @date 2026-03-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "LowPowerMode.h"
#include "LLCControl.h"
#include "PlatformManager.h"
#include <SysStateMachine.h>

ST_POWERMODE LowPowerMode;

void InitLowPowerModeData(void)
{
    LowPowerMode.Status = 0;
    LowPowerMode.FltCnt = 0;
    LowPowerMode.FltCntSet = 1000;
    LowPowerMode.WaitTimeSet = 5;
    LowPowerMode.BusVH = LLCSystemCtrl.VrefSet + 3.0f;
    LowPowerMode.BusVL = LLCSystemCtrl.VrefSet - 3.0f;
    LowPowerMode.RiseCounter = 0;
    LowPowerMode.FallCounter = 0;
}

void LowPowerModeFunc(float VBus, float IBus, ST_POWERMODE *pstData, Uint16 u16TaskCyc)
{
    if(pstData->Status == 0)
    {
        // 判断进入低功耗模式的条件
        // 电流小于5A，持续10s，进入低功耗模式：调节BusRef = 500V,上升=1
//        if((IBus > 0.01f)&&(IBus < 3.0f)&&(ObjSystemCtrl.LLCState == LLC_DISCHARGE_STATE))  // 充电条件不进入打嗝
        if((CtrlPara.fClaFreq > CpuAdcSample.MaxFreq - 0.1f)&&(ObjSystemCtrl.LLCState == LLC_DISCHARGE_STATE))
        {
            if(pstData->FltCnt > pstData->FltCntSet)  // 持续10s
            {
                pstData->FltCnt = 0;
                pstData->Status = 1;
                pstData->Rise = 0;
                pstData->WaitTime = 0;
            }
            else
            {
                pstData->FltCnt += u16TaskCyc;
            }
        }
        else
        {
            pstData->FltCnt = 0;
        }
    }
    else
    {
        if(pstData->Rise == 0)  // 延时等待其他包进入待机，时间到后调节母线
        {
            pstData->WaitTime += u16TaskCyc;
            if(pstData->WaitTime > pstData->WaitTimeSet)  // 20ms
            {
                pstData->WaitTime = 0;
                SetBusV(pstData->BusVH);
                pstData->Rise = 1;
            }
        }
        else if(pstData->Rise == 1)  // 上升段
        {
            pstData->RiseCounter += u16TaskCyc;
            // 判断电压是否达到设定的MAX
            if(VBus > pstData->BusVH)  // 2V误差带
            {
                pstData->RiseCounter = 0;
                pstData->Rise = 2;
                SetBusV(pstData->BusVL);
                pstData->EPWM_Start = 2;  // 发停波命令
//                TrigFlautTurnOffWaveForm();
            }

            // 上升段检测电流大小：有负载退出，有充电也退出
//            if((IBus > 3.0f)||(IBus < -1.0f)||(pstData->RiseCounter > 1000))
            if((VBus > pstData->BusVH + 8.0f)||(VBus < pstData->BusVL - 8.0f))
            {
                LowPower_Exit();
            }
        }
        else if(pstData->Rise == 2)// 下降段
        {
            pstData->FallCounter += u16TaskCyc;
            // 上升条件
            if(VBus < pstData->BusVL)
            {
                // 判断电压小于MIN，但是跌落速度很小时，BusRef = MAX
                pstData->FallCounter = 0;
                SetBusV(pstData->BusVH);
                pstData->Rise = 1;
                pstData->EPWM_Start = 1;

            }
//            else if((pstData->FallCounter > 1000)||(VBus > (pstData->BusVH + 5.0f)))
            else if((VBus > (pstData->BusVH + 8.0f)))
            {
                LowPower_Exit();
            }
        }
    }
}

void SetBusV(float Vset)
{
    LLCSystemCtrl.Vref = Vset;
}

void LowPower_Reset(void)
{
    LowPowerMode.Status = 0;
    LowPowerMode.FltCnt = 0;
    LowPowerMode.BusVH = 480;
    LowPowerMode.BusVL = 400;
    LowPowerMode.RiseCounter = 0;
    LowPowerMode.FallCounter = 0;
}

void LowPower_Exit(void)
{
    // 退出前设置到400V
    if(LowPowerMode.Status == 1)
    {
        if(LowPowerMode.Rise == 1)
        {
            SetBusV(LLCSystemCtrl.VrefSet);
        }
        else
        {
            SetBusV(LLCSystemCtrl.VrefSet);
            LowPowerMode.EPWM_Start = 1;
        }
        LowPower_Reset();
    }
}

void TaskLowPowerMode(void)
{
    Uint16 u16TaskCyc = 1;
    LowPowerMode.BusVH = LLCSystemCtrl.VrefSet + 1;
    LowPowerMode.BusVL = LLCSystemCtrl.VrefSet - 1;
    LowPowerModeFunc(ObjSample.VBus, ObjSample.IBus, &LowPowerMode, u16TaskCyc);
}


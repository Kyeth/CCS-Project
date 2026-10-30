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
#include "ParallelProcess.h"
#include "ParallelCfg.h"
#include "EquipmentParallelTramsport.h"
#include "PlatformManager.h"
#include "string.h"
#include "FaultProtect.h"
#include "VersionUser.h"
#include "ParallelSubFunc.h"
#include "LLCControl.h"
#include "Schedule.h"
#include "Hw_MCanDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif

ST_BATTERY_HANDLE stBatteryHandle;
void BatteryWireBreakageTimeOutInit(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16TimeOutSet)
{
    Uint16 i = 0;

    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutCnt = 0;
        pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutSet = u16TimeOutSet;
    }
}

void BatteryParallelInit(void)
{
    ST_PARALLEL_TRANSPORT_INTF stTransportIntf;

    stTransportIntf.CanRead = ParallelReadTramsport;
    stTransportIntf.CanWrite = ParallelWriteTramsport;
    stTransportIntf.ClearBusError = ClearBusError;
    EquipmentParallelCoreInit(&stBatteryHandle, &stBatteryCfg, &stTransportIntf);
    stBatteryHandle.u16LocalAddr = 2;
    stBatteryHandle.stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsEquipmentNormal = 1;
    stBatteryHandle.u16Soc = 60;
    stBatteryHandle.stBatteryPack[0].u16IsInit = 1;
    BatteryWireBreakageTimeOutInit(&stBatteryHandle, 50);
    stBatteryHandle.u16DeviceMode = STABLE_VOLTAGE_MODE;

}

#pragma CODE_SECTION(GetDroopPower, ".ISR.ramfunc");
float GetDroopPower(void)
{
    switch(LLCSystemCtrl.DroopState)
    {
    case DROOP_IDLE:
        if(ObjSample.VBus < (LLCSystemCtrl.Vref - LLCSystemCtrl.VDROP_ENTRY1))
        {
            LLCSystemCtrl.DroopState = DROOP_DISCHARGE;
            LLCSystemCtrl.fDelta = LLCSystemCtrl.Vref - LLCSystemCtrl.VDROP_ENTRY1 - ObjSample.VBus;
            LLCSystemCtrl.fDroopPower = LLCSystemCtrl.Kdroop1 * LLCSystemCtrl.fDelta;
            // 限幅
            if(LLCSystemCtrl.fDroopPower > ObjPlatformCtrl.u16AllowDischargePowerMax)
            {
                LLCSystemCtrl.fDroopPower = ObjPlatformCtrl.u16AllowDischargePowerMax;
            }
        }
        else if(ObjSample.VBus > (LLCSystemCtrl.Vref + LLCSystemCtrl.VDROP_ENTRY2))
        {
            LLCSystemCtrl.DroopState = DROOP_CHARGE;
            LLCSystemCtrl.fDelta = LLCSystemCtrl.Vref + LLCSystemCtrl.VDROP_ENTRY2 - ObjSample.VBus;
            LLCSystemCtrl.fDroopPower = LLCSystemCtrl.Kdroop2 * LLCSystemCtrl.fDelta;
            // 限幅
            if(LLCSystemCtrl.fDroopPower < (-(int16)ObjPlatformCtrl.u16AllowChargePowerMax))
            {
                LLCSystemCtrl.fDroopPower = -(int16)ObjPlatformCtrl.u16AllowChargePowerMax;
            }
        }
        else
        {
            LLCSystemCtrl.fDroopPower = 0;
        }
        break;
    case DROOP_DISCHARGE:
        if(ObjSample.VBus > (LLCSystemCtrl.Vref - LLCSystemCtrl.VDROP_EXIT1))
        {
            LLCSystemCtrl.DroopState = DROOP_IDLE;
            LLCSystemCtrl.fDroopPower = 0;
        }
        else
        {
            LLCSystemCtrl.fDelta = LLCSystemCtrl.Vref - LLCSystemCtrl.VDROP_EXIT1 - ObjSample.VBus;
            if(LLCSystemCtrl.fDelta < 0)
            {
                LLCSystemCtrl.fDelta = 0;
            }
            LLCSystemCtrl.fDroopPower = LLCSystemCtrl.Kdroop1 * LLCSystemCtrl.fDelta;
            // 限幅
            if(LLCSystemCtrl.fDroopPower > ObjPlatformCtrl.u16AllowDischargePowerMax)
            {
                LLCSystemCtrl.fDroopPower = ObjPlatformCtrl.u16AllowDischargePowerMax;
            }
        }
        break;
    case DROOP_CHARGE:
        if(ObjSample.VBus < LLCSystemCtrl.Vref + LLCSystemCtrl.VDROP_EXIT2)
        {
            LLCSystemCtrl.DroopState = DROOP_IDLE;
            LLCSystemCtrl.fDroopPower = 0;
        }
        else
        {
            LLCSystemCtrl.fDelta = LLCSystemCtrl.Vref + LLCSystemCtrl.VDROP_EXIT2 - ObjSample.VBus;
            if(LLCSystemCtrl.fDelta > 0)
            {
                LLCSystemCtrl.fDelta = 0;
            }
            LLCSystemCtrl.fDroopPower = LLCSystemCtrl.Kdroop2 * LLCSystemCtrl.fDelta;
            // 限幅
            if(LLCSystemCtrl.fDroopPower < (-(int16)ObjPlatformCtrl.u16AllowChargePowerMax))
            {
                LLCSystemCtrl.fDroopPower = -(int16)ObjPlatformCtrl.u16AllowChargePowerMax;
            }
        }
        break;
    }
    return LLCSystemCtrl.fDroopPower;
}

#pragma CODE_SECTION(GetAvgCurr, ".ISR.ramfunc");
float GetAvgCurr(ST_BATTERY_HANDLE *pstBatteryHandle, ST_SYS_CTRL *pObjSystemCtrl)
{
    float fCurrRef = 0.0f;
    LLCSystemCtrl.fPowerTar = (float)pstBatteryHandle->i16CtrlPackPower;

    if(pstBatteryHandle->u16DeviceMode == FOLLOW_MODE)
    {
        if((pObjSystemCtrl->LLCState == LLC_DISCHARGE_STATE)||(pObjSystemCtrl->LLCState == LLC_CHARGE_STATE))
        {
            // 获取下垂功率
            LLCSystemCtrl.fDroopPower = GetDroopPower();
        }
        else
        {
            LLCSystemCtrl.fDroopPower = 0;
        }

        if(pstBatteryHandle->i16CtrlPackPower < 0)
        {
            // 功率为负需求
            if(pObjSystemCtrl->LLCState == LLC_DISCHARGE_STATE)
            {
                // 大于400W以上的需求
                if(pstBatteryHandle->i16CtrlPackPower < -400)
                {
                    LLCSystemCtrl.PowerRamp.SetpointValue = 0;
                    LLCSystemCtrl.SlaveStateChange = 3;
                }
                else
                {
                    LLCSystemCtrl.fPowerTar = 0;
                }
            }
            else
            {
                // 方向不变
                LLCSystemCtrl.SlaveStateChange = 0;
            }
        }
        else
        {
            // 功率为正需求
            if(pObjSystemCtrl->LLCState == LLC_CHARGE_STATE)
            {
                // 大于400W以上的需求
                if(pstBatteryHandle->i16CtrlPackPower > 400)
                {
                    LLCSystemCtrl.PowerRamp.SetpointValue = 0;
                    LLCSystemCtrl.SlaveStateChange = 4;
                }
                else
                {
                    LLCSystemCtrl.fPowerTar = 0;
                }
            }
            else
            {
                // 方向不变
                LLCSystemCtrl.SlaveStateChange = 0;
            }
        }
        LLCSystemCtrl.PowerRamp.TargetValue = LLCSystemCtrl.fPowerTar + LLCSystemCtrl.fDroopPower;
        LLCSystemCtrl.PowerRamp.RampLowLimit = -(int16)ObjPlatformCtrl.u16AllowChargePowerMax;
        DNLMT(LLCSystemCtrl.PowerRamp.RampLowLimit,-4000.0f);
        LLCSystemCtrl.PowerRamp.RampHighLimit = ObjPlatformCtrl.u16AllowDischargePowerMax;
        UPLMT(LLCSystemCtrl.PowerRamp.RampHighLimit,6000.0f);
        LLCSystemCtrl.fPowerRef = RmpCntlCal(&LLCSystemCtrl.PowerRamp);
        fCurrRef = LLCSystemCtrl.fPowerRef / ObjSample.VBusFilter1;
    }
    else
    {
        LLCSystemCtrl.PowerRamp.TargetValue = 0;
        LLCSystemCtrl.PowerRamp.SetpointValue = 0;
        fCurrRef = 0;
        LLCSystemCtrl.SlaveStateChange = 0;
        LLCSystemCtrl.fDroopPower = 0;
    }
    return fCurrRef;
}

void CurrStatistics(ST_BATTERY_HANDLE *pstBatteryHandle)
{

}

void CurrentBalance(ST_BATTERY_HANDLE *pstBatteryHandle)
{

}

void SocBalance(ST_BATTERY_HANDLE *pstBatteryHandle)
{

}

void EnergyBalance(ST_BATTERY_HANDLE *pstBatteryHandle)
{

}

void UpdataCommData(ST_BATTERY_HANDLE *pstBatteryHandle, LLC_SYSTEM_CONTROL_PARA *pLLCSystemCtrl, ST_SAMPLE *pObjSample, ST_VERSION *pstVersion)
{
    int32 sum;
    Uint16 cnt;

    sum = pObjSample->PowerSum;
    cnt = pObjSample->PowerCnt;
    pObjSample->PowerSum = 0;
    pObjSample->PowerCnt = 0;
    if(cnt > 0)
    {
        pObjSample->ComPower = (int16)(sum/cnt);
    }
    else
    {
        pObjSample->ComPower = 0;
    }

    pstBatteryHandle->stBatteryPack[0].stBatteryData.u32Version = pstVersion->uVersion.u32VersionAll;
    pstBatteryHandle->stBatteryPack[0].u16HeartCount++;
    pstBatteryHandle->stBatteryPack[0].u16PackAddr = pstBatteryHandle->u16LocalAddr;

    // 这里的电压是根据电池电压计算出来
    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSelfBus = pLLCSystemCtrl->BaseVref;
    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsStableKey = pLLCSystemCtrl->CanTStable;
    pstBatteryHandle->u16ChargeCoefficient = pLLCSystemCtrl->u16ChargeCoefficient;
    pstBatteryHandle->u16DischargeCoefficient = pLLCSystemCtrl->u16DischargeCoefficient;
    pstBatteryHandle->u16ChargeCurrentAllowMax = pLLCSystemCtrl->u16ChargeCurrentAllowMax;
    pstBatteryHandle->u16DischargeCurrentAllowMax = pLLCSystemCtrl->u16DischargeCurrentAllowMax;
    pstBatteryHandle->u16PackPower = pLLCSystemCtrl->u16PackPower;
    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSoc = pstBatteryHandle->u16Soc / 10;
    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bGetDeviceMode = pstBatteryHandle->u16DeviceMode;
    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_2.bPower = pObjSample->ComPower;
}

void TaskBatteryParallelComm(void)
{
    UpdataCommData(&stBatteryHandle, &LLCSystemCtrl, &ObjSample, (ST_VERSION*)&stVersion);

    stBatteryHandle.stParallelTranIntf.ClearBusError();

    ParallelReceive(&stBatteryHandle);
    ParallelTransmit(&stBatteryHandle);

    ParallelReceiveFrameProcess(&stBatteryHandle);
    ParallelTransmitFrameProcess(&stBatteryHandle);

    WireBreakageDetection(&stBatteryHandle, 3);     // 断线检测
    HostDetection(&stBatteryHandle);                // 主从检测
    SetTaskRunStatus(TASK_BAT_PARAL_COMM_BIT);
}



void TaskBatteryParallelProcess(void)
{
    // 处于无故障状态时，才允许参与通信并机
    if (!stSysProtect.u16TotalFaultSign)
    {
        stBatteryHandle.stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsEquipmentNormal = 1;
    }
    else
    {
        stBatteryHandle.stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsEquipmentNormal = 0;
    }

    // 软启结束后，才允许参与控制并机
    if (LLCSystemCtrl.SoftFinish == 1)
    {
        stBatteryHandle.stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsSoftFinsh = 1;
    }
    else
    {
        stBatteryHandle.stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsSoftFinsh = 0;
    }

    ParallelCheck(&stBatteryHandle);                // 并机标志检测
//    VersionCheck(&stBatteryHandle);                 // 版本校验检测
    CheckDevieMode(&stBatteryHandle);
//    AutoSoc(&stBatteryHandle, &ObjSample);           // 自动设置SOC,测试使用

    // 需要获取设备正常数，只有设备正常的才能参加计算
    if (GetParallelMode(&stBatteryHandle))
    {
        if (ObjSystemCtrl.LLCState > LLC_SOFT_WAIT_STATE)
        {
            EnergyBalance(&stBatteryHandle);
        }

        PowerCheck(&stBatteryHandle);

    }
    else    // 走本地包逻辑
    {

    }
    SetTaskRunStatus(TASK_BAT_PARAL_BIT);
}

#ifdef __cplusplus
    }
#endif

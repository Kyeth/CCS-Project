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
#include "SysStateMachine.h"
#include "LLCControl.h"
#include "Hw_GpioDriver.h"
#include "PlatformManager.h"
#include "Hw_PwmDriver.h"
#include "FaultProtect.h"
#include "ParallelProcess.h"
#include "Schedule.h"

#ifdef __cplusplus
    extern "C" {
#endif

ST_SYS_CTRL ObjSystemCtrl;

static E_SYS_STATE SystemMachine(ST_SYS_CTRL *pObjSystemCtrl)
{
    switch (pObjSystemCtrl->SysState)
    {
        case SYS_STATE_IDLE:
        {
            if(ObjSystemCtrl.Open == 1)
            {
                Hw_GpioWritePin(5,1);
                pObjSystemCtrl->SysState = SYS_STATE_CHECK;
            }
            break;
        }
        case SYS_STATE_CHECK:
        {
            if((ObjPlatformCtrl.u16ZeroDirftIsInit == 1)&&(stSysProtect.u16IoscCheckFinish == 1))
            {
                pObjSystemCtrl->SysState = SYS_STATE_SOFT_START;
            }
            break;
        }
        case SYS_STATE_SOFT_START:
        {
            pObjSystemCtrl->SysState = SYS_STATE_WAIT_TO_RUN;

            break;
        }
        case SYS_STATE_WAIT_TO_RUN:
        {
            pObjSystemCtrl->SysState = SYS_STATE_NORMAL_OPERATION;
            break;
        }
        case SYS_STATE_NORMAL_OPERATION:
        {
            // 风扇开启
            Hw_GpioWritePin(16, 1);

            if(ObjSystemCtrl.Open == 0)
            {
                pObjSystemCtrl->SysState = SYS_STATE_IDLE;
                ObjSystemCtrl.LLCState = LLC_IDLE_STATE;
            }
            if (stSysProtect.u16TotalFaultSign)
            {
                ObjSystemCtrl.unSysCtrlEnable.u16Word = 0;
                ObjSystemCtrl.Open = 0;
                ObjSystemCtrl.LLCState = LLC_IDLE_STATE;
                pObjSystemCtrl->SysState = SYS_STATE_FAULT_SHUTDOWN;
            }

            break;
        }
        case SYS_STATE_FAULT_SHUTDOWN:
        {

            if (!stSysProtect.u16TotalFaultSign)
            {
                pObjSystemCtrl->SysState = SYS_STATE_IDLE;
            }
            break;
        }
        case SYS_STATE_POWER_RESTRICTION:
        {
            break;
        }
    }
    return pObjSystemCtrl->SysState;
}

void ChargeAndDischargeEnableDetection(ST_SYS_CTRL *pObjSystemCtrl, LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl)
{
    if ((pObjSystemCtrl->unSysCtrlEnable.Bits.bit0 == 1) && (pstLLCSystemCtrl->u16ChargeCurrentAllowMax > 0))
    {
        pstLLCSystemCtrl->u16AllowCharge = 1;
    }
    else
    {
        pstLLCSystemCtrl->u16AllowCharge = 0;
    }

    if ((pObjSystemCtrl->unSysCtrlEnable.Bits.bit1 == 1) && (pstLLCSystemCtrl->u16DischargeCurrentAllowMax > 0))
    {
        pstLLCSystemCtrl->u16AllowDischarge = 1;
    }
    else
    {
        pstLLCSystemCtrl->u16AllowDischarge = 0;
    }
}

void StartupConditionsDetection(ST_SYS_CTRL *pObjSystemCtrl, ST_BATTERY_HANDLE *pstBatteryHandle)
{
    if ((pObjSystemCtrl->unSysCtrlEnable.Bits.bit3 == 1) && (pstBatteryHandle->u16LocalAddr > 0))
    {
        pObjSystemCtrl->Open = 1;
    }
    else
    {
        pObjSystemCtrl->Open = 0;
    }
}

void AgingDetection(ST_SYS_CTRL *pObjSystemCtrl, LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl)
{
    if(pObjSystemCtrl->unAgingSigns.u16Word == 0xAA)
    {
        pstLLCSystemCtrl->AgingFlag = 1;
    }
    else
    {
        pstLLCSystemCtrl->AgingFlag = 0;
    }
}

void CommDataTransform(ST_SAMPLE *pObjSample, LLC_SYSTEM_CONTROL_PARA *pLLCSystemCtrl, ST_CommData *pCommDataTrans)
{
    pCommDataTrans->VNTC_2          = (int16)pObjSample->VNTC_2;
    pCommDataTrans->VEnvironment    = (int16)pObjSample->VEnvironment;
    pCommDataTrans->VTransformer_1  = (int16)pObjSample->VTransformer_1;
    pCommDataTrans->VTransformer_2  = (int16)pObjSample->VTransformer_2;
    pCommDataTrans->Heating         = (int16)(pObjSample->Heating * 100.0f);
    pCommDataTrans->VAirBack        = (Uint16)(pObjSample->VAirBack * 10.0f);
    pCommDataTrans->VAirFront       = (Uint16)(pObjSample->VAirFront * 10.0f);
    pCommDataTrans->VBusFilter1     = (Uint16)(pObjSample->VBusFilter1 * 10.0f);
    pCommDataTrans->VBat            = (Uint16)(pObjSample->VBat * 10.0f);
    pCommDataTrans->IBus            = (int16)(pObjSample->IBus * 100.0f);
    pCommDataTrans->IBranch_1       = (Uint16)(pObjSample->IBranch_1 * 100.0f);
    pCommDataTrans->IBranch_2       = (Uint16)(pObjSample->IBranch_2 * 100.0f);
    pCommDataTrans->IErr            = (Uint16)(pObjSample->IErr * 100.0f);
    pCommDataTrans->AvgPower        = (int16)pObjSample->AvgPower;
    pCommDataTrans->VBusRaw         = (Uint16)(pObjSample->VBusRaw * 10.0f);
    pCommDataTrans->VBus            = (Uint16)(pObjSample->VBus * 10.0f);
    pCommDataTrans->VBusFilter      = (int16)(pObjSample->VBusFilter * 10.0f);
    pCommDataTrans->VBatRaw         = (Uint16)(pObjSample->VBatRaw * 10.0f);
    pCommDataTrans->IBusRaw         = (int16)(pObjSample->IBusRaw* 100.0f);
    pCommDataTrans->Vref            = (Uint16)(pLLCSystemCtrl->Vref * 10.0f);
    pCommDataTrans->LimitBrustHigh  = (Uint16)((pLLCSystemCtrl->Vref + pLLCSystemCtrl->DBackVoltGap) * 10.0f);
    pCommDataTrans->LimitBrustLow   = (Uint16)((pLLCSystemCtrl->Vref - pLLCSystemCtrl->CBackVoltGap) * 10.0f);
}

void SysParamProcess(void)
{
    ObjSample.i16MdsIBus = ObjSample.IBus * 10;
    ObjSample.u16MdsVBus = ObjSample.VBus * 10;
    ObjSample.i16MdsPower = (int16)(ObjSample.IBus * ObjSample.VBus);
    ObjSample.u16MdsIHeatingFilm = ObjSample.Heating * 10;
    if(ObjSystemCtrl.SysState == SYS_STATE_IDLE)
    {
        ObjSystemCtrl.DCState = DC_Init;
    }
    else if((ObjSystemCtrl.SysState == SYS_STATE_CHECK)||(ObjSystemCtrl.SysState == SYS_STATE_SOFT_START)||(ObjSystemCtrl.SysState == SYS_STATE_WAIT_TO_RUN))
    {
        ObjSystemCtrl.DCState = DC_StandBy;
    }
    else if(ObjSystemCtrl.SysState == SYS_STATE_NORMAL_OPERATION)
    {
        if(ObjSystemCtrl.LLCState == LLC_IDLE_STATE)
        {
            ObjSystemCtrl.DCState = DC_StandBy;
        }
        else if((ObjSystemCtrl.LLCState == LLC_SOFT_STATE)||(ObjSystemCtrl.LLCState == LLC_SOFT_WAIT_STATE))
        {
            ObjSystemCtrl.DCState = DC_Soft;
        }
        else if((ObjSystemCtrl.LLCState == LLC_CHARGE_STATE)||(ObjSystemCtrl.LLCState == LLC_DISCHARGE_STATE)||(ObjSystemCtrl.LLCState == LLC_STANDBY_STATE)||(ObjSystemCtrl.LLCState == LLC_AGING_STATE))
        {
            ObjSystemCtrl.DCState = DC_Work;
        }
    }
    else if(ObjSystemCtrl.SysState == SYS_STATE_FAULT_SHUTDOWN)
    {
        ObjSystemCtrl.DCState = DC_Error;
    }
    CommDataTransform(&ObjSample, &LLCSystemCtrl, &CommDataTrans);
}

void TaskSysMachine(void)
{
    SystemMachine(&ObjSystemCtrl);
    ChargeAndDischargeEnableDetection(&ObjSystemCtrl, &LLCSystemCtrl);
    StartupConditionsDetection(&ObjSystemCtrl, &stBatteryHandle);
    AgingDetection(&ObjSystemCtrl, &LLCSystemCtrl);
    SysParamProcess();
    SetTaskRunStatus(TASK_SYS_MACHINE_BIT);
}

#ifdef __cplusplus
    }
#endif


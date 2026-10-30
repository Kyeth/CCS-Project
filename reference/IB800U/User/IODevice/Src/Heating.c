#include "Heating.h"
#include "Hw_GpioDriver.h"
#include "Hw_PwmDriver.h"
#include "DeviceAbstraction.h"
#include "SysStateMachine.h"
#include "Schedule.h"

#ifdef __cplusplus
    extern "C" {
#endif

//-------------------------------------------------------------------------
// 驱动接口
static void WritePin(Uint32 u32Pin, Uint32 u32Value)
{
    Hw_GpioWritePin(u32Pin, u32Value);
}

static void StartPwm(void)
{
    Hw_StartPWM3();
    Hw_StartPWM6();
}

static void StopPwm(void)
{
    Hw_StopPWM3();
    Hw_StopPWM6();
}

static void SetDuty(Uint16 u16Duty)
{
    Hw_SetPWM3_CMPA(0);
    Hw_SetPWM3_CMPB(u16Duty);
}

ST_HEATING_DRIVER stHeatingDriver =
{
    .WritePin = WritePin,
    .StartPwm = StartPwm,
    .StopPwm = StopPwm,
    .SetDuty = SetDuty
};

//-------------------------------------------------------------------------
// 设备操作接口
static void Init(ST_DEVICE *pstSelf)
{
    ST_HEATING_DATA *pstHeatingData = (ST_HEATING_DATA *)pstSelf->pvDeviceData;
    ST_HEATING_DRIVER *pstHeatingDriver = (ST_HEATING_DRIVER *)pstSelf->pvDriver;

    if ((pstHeatingDriver != NULL))
    {
        if ((pstHeatingDriver->WritePin != NULL))
        {
            pstHeatingDriver->WritePin(pstHeatingData->u16Pin_1, 0);
            pstHeatingDriver->WritePin(pstHeatingData->u16Pin_2, 0);
        }

        if (pstHeatingDriver->StopPwm != NULL)
        {
            pstHeatingDriver->StopPwm();
        }
    }
    stHeatingHandle.u16HeatingLevel = 0;
    stHeatingHandle.u16HeatingLevelBak = 0;
}

static void Open(ST_DEVICE *pstSelf)
{
    ST_HEATING_HANDLE *pstHeatingHandle = (ST_HEATING_HANDLE *)pstSelf;
    ST_HEATING_DATA *pstHeatingData = (ST_HEATING_DATA *)pstSelf->pvDeviceData;
    ST_HEATING_DRIVER *pstHeatingDriver = (ST_HEATING_DRIVER *)pstSelf->pvDriver;

    if (pstHeatingDriver != NULL)
    {
        if ((pstHeatingDriver->WritePin != NULL) && (pstHeatingDriver->StartPwm != NULL))
        {
            pstHeatingDriver->StartPwm();
            ObjSystemCtrl.u16HeatingFilmEnableFlag = 1;
            switch(pstHeatingHandle->u16HeatingLevel)
            {

            case 1:
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_1, 1);
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_2, 0);
                break;

            case 2:
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_1, 0);
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_2, 1);
                break;

            case 3:
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_1, 1);
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_2, 1);
                break;
            }
        }
        if (pstHeatingDriver->SetDuty != NULL)
        {
            pstHeatingDriver->SetDuty(1200 * pstHeatingHandle->u16Duty * 0.01);
        }
    }
}

static void Close(ST_DEVICE *pstSelf)
{
    ST_HEATING_HANDLE *pstHeatingHandle = (ST_HEATING_HANDLE *)pstSelf;
    ST_HEATING_DATA *pstHeatingData = (ST_HEATING_DATA *)pstSelf->pvDeviceData;
    ST_HEATING_DRIVER *pstHeatingDriver = (ST_HEATING_DRIVER *)pstSelf->pvDriver;

    if (pstHeatingDriver != NULL)
    {
        if ((pstHeatingDriver->WritePin != NULL) && (pstHeatingDriver->StopPwm != NULL))
        {
            pstHeatingDriver->StopPwm();

            switch(pstHeatingHandle->u16HeatingLevel)
            {
            case 0:
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_1, 0);
                pstHeatingDriver->WritePin(pstHeatingData->u16Pin_2, 0);
                ObjSystemCtrl.u16HeatingFilmEnableFlag = 0;
                break;
            }
        }
    }
}

ST_DEVIVE_OPERATIONS stHeatingOps =
{
    .Init = Init,
    .Open = Open,
    .Close = Close,
    .Toggle = NULL,
    .Ctrl = NULL
};

//-------------------------------------------------------------------------
// 加热膜设备配置
ST_HEATING_DATA stHeatingData1 =
{
    .u16Pin_1 = 50,
    .u16Pin_2 = 51,
};

//-------------------------------------------------------------------------
ST_HEATING_HANDLE stHeatingHandle =
{
    .stDevice1 = {
               .pstOps = &stHeatingOps,
               .pvDeviceData = &stHeatingData1,
               .pvDriver = &stHeatingDriver,
               .u16DeviceId = 0,
              },
    .u16HeatingLevel = 0,
    .u16HeatingLevelBak = 0,
    .u16Duty = 50
};

//-------------------------------------------------------------------------
void HeatingInit(void)
{
    stHeatingHandle.stDevice1.pstOps->Init(&stHeatingHandle.stDevice1);
}

void TaskHeating(void)
{
    if(ObjSystemCtrl.unSysCtrlEnable.Bits.bit2 == 1)
    {
        if (stHeatingHandle.u16HeatingLevel != stHeatingHandle.u16HeatingLevelBak)
        {
            if (stHeatingHandle.u16HeatingLevel < 1)
            {
                stHeatingHandle.stDevice1.pstOps->Close(&stHeatingHandle.stDevice1);
            }
            else
            {
                stHeatingHandle.stDevice1.pstOps->Open(&stHeatingHandle.stDevice1);
            }
            stHeatingHandle.u16HeatingLevelBak = stHeatingHandle.u16HeatingLevel;
        }
    }
    else
    {
        stHeatingHandle.stDevice1.pstOps->Close(&stHeatingHandle.stDevice1);
        stHeatingHandle.u16HeatingLevelBak = 0;
    }
    SetTaskRunStatus(TASK_HEATING_BIT);
}

#ifdef __cplusplus
    }
#endif

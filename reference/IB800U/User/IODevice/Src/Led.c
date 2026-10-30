#include "Led.h"
#include "Hw_GpioDriver.h"
#include "DeviceAbstraction.h"
#include "FaultProtect.h"
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
static void TogglePin(Uint32 u32Pin)
{
    Hw_GpioTogglePin(u32Pin);
}

ST_LED_DRIVER stLedDriver =
{
    .WritePin = WritePin,
    .TogglePin = TogglePin
};

//-------------------------------------------------------------------------
// 设备操作接口
static void Init(ST_DEVICE *pstSelf)
{
    ST_LED_DATA *pstLedData =  (ST_LED_DATA *)pstSelf->pvDeviceData;
    ST_LED_DRIVER *pstLedDriver = (ST_LED_DRIVER *)pstSelf->pvDriver;

    if ((pstLedDriver != NULL) && (pstLedDriver->WritePin != NULL))
    {
        pstLedDriver->WritePin(pstLedData->u16Pin, 1);
    }
    pstLedData->u16CurrentState = 0;
}

static void Open(ST_DEVICE *pstSelf)
{
    ST_LED_DATA *pstLedData =  (ST_LED_DATA *)pstSelf->pvDeviceData;
    ST_LED_DRIVER *pstLedDriver = (ST_LED_DRIVER *)pstSelf->pvDriver;

    if ((pstLedDriver != NULL) && (pstLedDriver->WritePin != NULL))
    {
        pstLedDriver->WritePin(pstLedData->u16Pin, 0);
    }
    pstLedData->u16CurrentState = 1;
}

static void Close(ST_DEVICE *pstSelf)
{
    ST_LED_DATA *pstLedData =  (ST_LED_DATA *)pstSelf->pvDeviceData;
    ST_LED_DRIVER *pstLedDriver = (ST_LED_DRIVER *)pstSelf->pvDriver;

    if ((pstLedDriver != NULL) && (pstLedDriver->WritePin != NULL))
    {
        pstLedDriver->WritePin(pstLedData->u16Pin, 1);
    }
    pstLedData->u16CurrentState = 0;
}

static void Toggle(ST_DEVICE *pstSelf)
{
    ST_LED_DATA *pstLedData =  (ST_LED_DATA *)pstSelf->pvDeviceData;
    ST_LED_DRIVER *pstLedDriver = (ST_LED_DRIVER *)pstSelf->pvDriver;

    if ((pstLedDriver != NULL) && (pstLedDriver->TogglePin != NULL))
    {
        pstLedDriver->TogglePin(pstLedData->u16Pin);
    }
    pstLedData->u16CurrentState = !pstLedData->u16CurrentState;
}

ST_DEVIVE_OPERATIONS stLedOps =
{
 .Init = Init,
 .Open = Open,
 .Close = Close,
 .Toggle = Toggle,
 .Ctrl = NULL
};

//-------------------------------------------------------------------------
// LED设备配置
ST_LED_DATA stGreenLedData =
{
 .u16Pin = 6,
 .u16CurrentState = 0
};

ST_LED_DATA stRedLedData =
{
 .u16Pin = 34,
 .u16CurrentState = 0
};

//-------------------------------------------------------------------------
// LED实例化
ST_DEVICE stGreenLed =
{
 .pstOps = &stLedOps,
 .pvDeviceData = &stGreenLedData,
 .pvDriver = &stLedDriver,
 .u16DeviceId = 1
};

ST_DEVICE stRedLed =
{
 .pstOps = &stLedOps,
 .pvDeviceData = &stRedLedData,
 .pvDriver = &stLedDriver,
 .u16DeviceId = 2
};

//-------------------------------------------------------------------------
void LedInit(void)
{
    stGreenLed.pstOps->Init(&stGreenLed);
    stRedLed.pstOps->Init(&stRedLed);
}

void LedCloseFaultStatus(void)
{
    stRedLed.pstOps->Open(&stRedLed);
    stGreenLed.pstOps->Close(&stGreenLed);
}

void TaskLed(void)
{
    Uint16 u16Cnt = 0;

    if (stSysProtect.u16TotalFaultSign == 0)
    {
        u16Cnt++;
        stRedLed.pstOps->Close(&stRedLed);
        switch (GetLLCState(&ObjSystemCtrl))
        {
        case LLC_SOFT_STATE:
        case LLC_SOFT_WAIT_STATE:
        case LLC_CHARGE_STATE:
        case LLC_DISCHARGE_STATE:
            if (u16Cnt % 2)
            {
                stGreenLed.pstOps->Toggle(&stGreenLed);
            }
            break;

        case LLC_IDLE_STATE:
            stGreenLed.pstOps->Toggle(&stGreenLed);
            break;
        }
    }
    else
    {
        stRedLed.pstOps->Open(&stRedLed);
        stGreenLed.pstOps->Close(&stGreenLed);
    }
    SetTaskRunStatus(TASK_LED_BIT);
}

#ifdef __cplusplus
    }
#endif

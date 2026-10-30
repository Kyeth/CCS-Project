#include "Fan.h"
#include "Hw_GpioDriver.h"
#include "Hw_PwmDriver.h"
#include "PlatformManager.h"
#include "FaultProtect.h"
#include "Schedule.h"
#include <LLCControl.h>

#ifdef __cplusplus
    extern "C" {
#endif

//-------------------------------------------------------------------------
// 驱动接口
static void StartPwm(void)
{
    Hw_StartPWM8();
}

static void StopPwm(void)
{
    Hw_StopPWM8();
}

static void SetDuty(Uint16 u16Duty)
{
    Hw_SetPWM8_CMPA(0);
    Hw_SetPWM8_CMPB(u16Duty);
}

static void SetPrd(Uint16 u16Prd)
{
    Hw_SetPWM8_PRD(u16Prd);
}

static void WritePin(Uint32 u32Pin, Uint32 u32Value)
{
    Hw_GpioWritePin(u32Pin, u32Value);
}

ST_FAN_DRIVER stFanDriver =
{
 .StartPwm = StartPwm,
 .StopPwm = StopPwm,
 .SetDuty = SetDuty,
 .SetPrd = SetPrd,
 .WritePin = WritePin
};

//-------------------------------------------------------------------------
// 设备操作接口
void Init(ST_DEVICE *pstSelf)
{
    ST_FAN_DRIVER *pstFanDriver = (ST_FAN_DRIVER *)pstSelf->pvDriver;
    ST_FAN_DATA *pstFanData = (ST_FAN_DATA *)pstSelf->pvDeviceData;

    if (pstFanDriver == NULL)
    {
        return;
    }
    if ((pstFanDriver->StopPwm != NULL))
    {
        pstFanDriver->StopPwm();
    }

    if ((pstFanDriver->SetPrd != NULL))
    {
        pstFanDriver->SetPrd(pstFanData->u16PwmPrd);
    }

    if ((pstFanDriver->SetDuty != NULL))
    {
        pstFanDriver->SetDuty(0);
    }


    stFanHandle.u16FanSpeedLevel = 0;
    stFanHandle.u16FanSpeedLevelBak = 0;

}

void Open(ST_DEVICE *pstSelf)
{
    Uint16 u16Duty = 0;
    ST_FAN_HANDLE *pstFanHandle = (ST_FAN_HANDLE *)pstSelf;
    ST_FAN_DRIVER *pstFanDriver = (ST_FAN_DRIVER *)pstSelf->pvDriver;
    ST_FAN_DATA *pstFanData = (ST_FAN_DATA *)pstSelf->pvDeviceData;

    if (pstFanDriver == NULL)
    {
        return;
    }

    if ((pstFanDriver->SetPrd != NULL))
    {
        pstFanDriver->SetPrd(pstFanData->u16PwmPrd);
    }

    if ((pstFanDriver->StartPwm != NULL))
    {
        pstFanDriver->StartPwm();
    }


    if ((pstFanDriver->SetDuty != NULL))
    {
        switch (pstFanHandle->u16FanSpeedLevel)
        {
            case 0:
                u16Duty = 0;
                break;

            case 1:
                u16Duty = (Uint16)(pstFanData->u16PwmPrd * 0.2f);
                break;

            case 2:
                u16Duty = (Uint16)(pstFanData->u16PwmPrd * 0.3f);
                break;

            case 3:
            default:
                u16Duty = pstFanData->u16PwmPrd;
                break;
        }
        pstFanDriver->SetDuty(u16Duty);
    }
}

void Close(ST_DEVICE *pstSelf)
{
    ST_FAN_DRIVER *pstFanDriver = (ST_FAN_DRIVER *)pstSelf->pvDriver;
    ST_FAN_DATA *pstFanData = (ST_FAN_DATA *)pstSelf->pvDeviceData;

    if (pstFanDriver == NULL)
    {
        return;
    }
    if ((pstFanDriver->StopPwm != NULL))
    {
        pstFanDriver->StopPwm();
    }

    if ((pstFanDriver->SetDuty != NULL))
    {
        pstFanDriver->SetDuty(0);
    }

}

ST_DEVIVE_OPERATIONS stFanOps =
{
    .Init = Init,
    .Open = Open,
    .Close = Close,
    .Ctrl = NULL,
    .Toggle = NULL
};

//-------------------------------------------------------------------------
// FAN设备配置
ST_FAN_DATA stFanData =
{
    .u16PwmPrd = 6000, // 20k
    .u16Pin = 33,
};

//-------------------------------------------------------------------------
// 风扇实例化
ST_FAN_HANDLE stFanHandle =
{
    .stDevice = {
               .pstOps = &stFanOps,
               .pvDeviceData = &stFanData,
               .pvDriver = &stFanDriver,
               .u16DeviceId = 1,
              },
    .u16FanSpeedLevel = 0,
    .u16FanSpeedLevelBak = 0
};

//-------------------------------------------------------------------------
void FanInit(void)
{
    stFanHandle.stDevice.pstOps->Init(&stFanHandle.stDevice);

    stFanHandle.stFanAppData[0].pchName = "NTC2";
    stFanHandle.stFanAppData[0].fTemperature = 0;
    stFanHandle.stFanAppData[0].u16LastGear = 0;
    stFanHandle.stFanAppData[0].u16Thresholds[0] = 68;
    stFanHandle.stFanAppData[0].u16Thresholds[1] = 65;
    stFanHandle.stFanAppData[0].u16Thresholds[2] = 60;
    stFanHandle.stFanAppData[0].u16ChaThresholds = 74 - 1;
    stFanHandle.stFanAppData[0].u16DisThresholds = 71 - 1;

    stFanHandle.stFanAppData[1].pchName = "VEnvironment";
    stFanHandle.stFanAppData[1].fTemperature = 0;
    stFanHandle.stFanAppData[1].u16LastGear = 0;
    stFanHandle.stFanAppData[1].u16Thresholds[0] = 76;
    stFanHandle.stFanAppData[1].u16Thresholds[1] = 73;
    stFanHandle.stFanAppData[1].u16Thresholds[2] = 65;
    stFanHandle.stFanAppData[1].u16ChaThresholds = 82 - 1;
    stFanHandle.stFanAppData[1].u16DisThresholds = 79 - 1;

    stFanHandle.stFanAppData[2].pchName = "VTransformer_1";    // 低压管
    stFanHandle.stFanAppData[2].fTemperature = 0;
    stFanHandle.stFanAppData[2].u16LastGear = 0;
    stFanHandle.stFanAppData[2].u16Thresholds[0] = 82;
    stFanHandle.stFanAppData[2].u16Thresholds[1] = 77;
    stFanHandle.stFanAppData[2].u16Thresholds[2] = 66;
    stFanHandle.stFanAppData[2].u16ChaThresholds = 88 - 1;
    stFanHandle.stFanAppData[2].u16DisThresholds = 85 - 1;

    stFanHandle.stFanAppData[3].pchName = "VTransformer_2";     // 变压器
    stFanHandle.stFanAppData[3].fTemperature = 0;
    stFanHandle.stFanAppData[3].u16LastGear = 0;
    stFanHandle.stFanAppData[3].u16Thresholds[0] = 82;
    stFanHandle.stFanAppData[3].u16Thresholds[1] = 77;
    stFanHandle.stFanAppData[3].u16Thresholds[2] = 64;
    stFanHandle.stFanAppData[3].u16ChaThresholds = 88 - 1;
    stFanHandle.stFanAppData[3].u16DisThresholds = 85 - 1;
}

void FanProcess(void)
{
    if(stSysProtect.unFaultAlarmWord1.Bits.bit8 == 1)
    {
        stFanHandle.stFanAppData[0].u16ChaThresholds = 63 - 1;
        stFanHandle.stFanAppData[0].u16DisThresholds = 60 - 1;
        stFanHandle.stFanAppData[1].u16ChaThresholds = 76 - 1;
        stFanHandle.stFanAppData[1].u16DisThresholds = 73 - 1;
        stFanHandle.stFanAppData[2].u16ChaThresholds = 80 - 1;
        stFanHandle.stFanAppData[2].u16DisThresholds = 77 - 1;
        stFanHandle.stFanAppData[3].u16ChaThresholds = 76 - 1;
        stFanHandle.stFanAppData[3].u16DisThresholds = 73 - 1;
    }
    else
    {
        stFanHandle.stFanAppData[0].u16ChaThresholds = 74 - 1;
        stFanHandle.stFanAppData[0].u16DisThresholds = 71 - 1;
        stFanHandle.stFanAppData[1].u16ChaThresholds = 82 - 1;
        stFanHandle.stFanAppData[1].u16DisThresholds = 79 - 1;
        stFanHandle.stFanAppData[2].u16ChaThresholds = 88 - 1;
        stFanHandle.stFanAppData[2].u16DisThresholds = 85 - 1;
        stFanHandle.stFanAppData[3].u16ChaThresholds = 88 - 1;
        stFanHandle.stFanAppData[3].u16DisThresholds = 85 - 1;
    }
}

void FanUpdateInnerFanCmd(Uint16 u16OuterFanGear)
{
    switch (u16OuterFanGear)
    {
        case 0:
            LLCSystemCtrl.FanTestCmd = 0;
            break;

        case 1:
            LLCSystemCtrl.FanTestCmd = 1;
            break;

        case 2:
        case 3:
        default:
            LLCSystemCtrl.FanTestCmd = 2;
            break;
    }
}

void FanControl(ST_FAN_HANDLE *pstFanHandle)
{
    Uint16 u16MaxGear = 0, u16Gear = 0, u16GearFromPower = 0;
    int16 i;
    static Uint16 u16LastPowerGear = 0;
    float fPower = 0.0f;
    float fPowerTemp = 0.0f;
    float fChaTempCoefficient = 0.0f;
    float fDisTempCoefficient = 0.0f;
    float fChaTempCoefficientMax = 0.0f;
    float fDisTempCoefficientMax = 0.0f;

    stFanHandle.stFanAppData[0].fTemperature = ObjSample.VNTC_2;
    stFanHandle.stFanAppData[1].fTemperature = ObjSample.VEnvironment;
    stFanHandle.stFanAppData[2].fTemperature = ObjSample.VTransformer_1;
    stFanHandle.stFanAppData[3].fTemperature = ObjSample.VTransformer_2;

    fPowerTemp = ObjSample.IBus * ObjSample.VBusFilter1;
    if (fPowerTemp < 0.0f)
    {
        fPower = 0.0f - fPowerTemp;
    }
    else
    {
        fPower = fPowerTemp;
    }

    if (fPower >= 2500.0f)
    {
        u16LastPowerGear = 1;
    }
    else if (fPower < 2000.0f)
    {
        u16LastPowerGear = 0;
    }

    u16GearFromPower = u16LastPowerGear;


    for (i = 0; i < 4; i++)
    {

        // 查找当前温度挡位
        if (stFanHandle.stFanAppData[i].fTemperature >= stFanHandle.stFanAppData[i].u16Thresholds[0])
        {
            u16Gear = 3;
        }
        else if(stFanHandle.stFanAppData[i].fTemperature >= stFanHandle.stFanAppData[i].u16Thresholds[1])
        {
            u16Gear = 2;
        }
        else if(stFanHandle.stFanAppData[i].fTemperature >= stFanHandle.stFanAppData[i].u16Thresholds[2])
        {
            u16Gear = 1;
        }

        else
        {
            u16Gear = 0;
        }


        // 检查是否需要降档(温度回差)
        if (u16Gear < stFanHandle.stFanAppData[i].u16LastGear)
        {
            // 如果当前温度与上一次挡位设置的温度差值不足3度时，则不降档
            switch (pstFanHandle->stFanAppData[i].u16LastGear)
            {
            case 3:
                if (pstFanHandle->stFanAppData[i].fTemperature >=
                        pstFanHandle->stFanAppData[i].u16Thresholds[0] - 2.5f)
                {
                    u16Gear = 3;
                }
                break;

            case 2:
                if (pstFanHandle->stFanAppData[i].fTemperature >=
                        pstFanHandle->stFanAppData[i].u16Thresholds[1] - 2.5f)
                {
                    u16Gear = 2;
                }
                break;

            case 1:
                if (pstFanHandle->stFanAppData[i].fTemperature >=
                        pstFanHandle->stFanAppData[i].u16Thresholds[2] - 2.5f)
                {
                    u16Gear = 1;
                }
                break;

            default:
                break;
            }
        }

        // 如果当前温度比最低阈值还小，那么挡位更新为0档
        if (stFanHandle.stFanAppData[i].fTemperature < stFanHandle.stFanAppData[i].u16Thresholds[2] - 3)
        {
            u16Gear = 0;
        }

        stFanHandle.stFanAppData[i].u16LastGear = u16Gear;

        if (u16Gear > u16MaxGear)
        {
            u16MaxGear = u16Gear;
        }


        // 计算当前温度距离降额阈值的差值(温差系数)
        fChaTempCoefficient = stFanHandle.stFanAppData[i].fTemperature - stFanHandle.stFanAppData[i].u16ChaThresholds;
        fDisTempCoefficient = stFanHandle.stFanAppData[i].fTemperature - stFanHandle.stFanAppData[i].u16DisThresholds;

        if (fChaTempCoefficient < 0.5f)
        {
            fChaTempCoefficient = 0.0f;
        }
        if (fDisTempCoefficient < 0.5f)
        {
            fDisTempCoefficient = 0.0f;
        }

        if (fChaTempCoefficient < stFanHandle.stFanAppData[i].u16LastChaCoefficient )
        {
            if (stFanHandle.stFanAppData[i].fTemperature < (stFanHandle.stFanAppData[i].u16ChaThresholds + stFanHandle.stFanAppData[i].u16LastChaCoefficient - 2.5f))
            {
                fChaTempCoefficient = stFanHandle.stFanAppData[i].u16LastChaCoefficient - 1;
            }
            else
            {
                fChaTempCoefficient = stFanHandle.stFanAppData[i].u16LastChaCoefficient;
            }
        }

        if (fDisTempCoefficient < stFanHandle.stFanAppData[i].u16LastDisCoefficient )
        {
            if (stFanHandle.stFanAppData[i].fTemperature < (stFanHandle.stFanAppData[i].u16DisThresholds + stFanHandle.stFanAppData[i].u16LastDisCoefficient - 3))
            {
                fDisTempCoefficient = stFanHandle.stFanAppData[i].u16LastDisCoefficient - 1;
            }
            else
            {
                fDisTempCoefficient = stFanHandle.stFanAppData[i].u16LastDisCoefficient;
            }
        }

        stFanHandle.stFanAppData[i].u16LastChaCoefficient = fChaTempCoefficient;
        stFanHandle.stFanAppData[i].u16LastDisCoefficient = fDisTempCoefficient;

        if (fChaTempCoefficient > fChaTempCoefficientMax)
        {
            fChaTempCoefficientMax = fChaTempCoefficient;
        }

        if (fDisTempCoefficient > fDisTempCoefficientMax)
        {
            fDisTempCoefficientMax = fDisTempCoefficient;
        }

    }
    // 取功率挡位和温度挡位的最大值
    if (u16GearFromPower > u16MaxGear)
    {
        u16MaxGear = u16GearFromPower;
    }

    stFanHandle.u16FanSpeedLevel = u16MaxGear;
    stFanHandle.u16ChaThresholdsCoefficient = fChaTempCoefficientMax;
    stFanHandle.u16DisThresholdsCoefficient = fDisTempCoefficientMax;
}

void FanSetTestDutyPercent(Uint16 u16DutyPercent)
{
    Uint16 u16DutyCmp;
    ST_FAN_DATA *pstFanData = &stFanData;


    if (u16DutyPercent > 100)
    {
        u16DutyPercent = 100;
    }

    u16DutyCmp = (Uint16)(((Uint32)pstFanData->u16PwmPrd *
                          (Uint32)u16DutyPercent) / 100);

    if (u16DutyPercent == 0)
    {
        SetDuty(0);
        StopPwm();
        WritePin(pstFanData->u16Pin, 0);
    }
    else
    {
        SetPrd(pstFanData->u16PwmPrd);
        SetDuty(u16DutyCmp);

        WritePin(pstFanData->u16Pin, 1);
        StartPwm();
    }
}

void TaskFan(void)
{
    static Uint16 u16LastFanTestEnable = 0;

    // 风扇数据更新
    FanProcess();
    if(LLCSystemCtrl.FanTestEnable == 1)
    {
        FanSetTestDutyPercent(LLCSystemCtrl.FanTestGear);
        u16LastFanTestEnable = 1;
    }
    else
    {
        if(u16LastFanTestEnable == 1)
        {
            stFanHandle.u16FanSpeedLevelBak = 0xFFFF;
            u16LastFanTestEnable = 0;
        }

        if(ObjSystemCtrl.LLCState == LLC_AGING_STATE)
        {
            stFanHandle.u16FanSpeedLevel = 3;
        }
        else
        {
            // 风扇挡位控制
            FanControl(&stFanHandle);
        }
        FanUpdateInnerFanCmd(stFanHandle.u16FanSpeedLevel);

        if (stFanHandle.u16FanSpeedLevel != stFanHandle.u16FanSpeedLevelBak)
        {
            if (stFanHandle.u16FanSpeedLevel > 0)
            {
                stFanHandle.stDevice.pstOps->Open(&stFanHandle.stDevice);
            }
            else
            {
                stFanHandle.stDevice.pstOps->Close(&stFanHandle.stDevice);
            }

            stFanHandle.u16FanSpeedLevelBak = stFanHandle.u16FanSpeedLevel;
        }
    }
    SetTaskRunStatus(TASK_FAN_BIT);
}

#ifdef __cplusplus
    }
#endif


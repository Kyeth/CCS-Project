#ifndef __FAN_H
#define __FAN_H

#include "F28003x_device.h"
#include "DeviceAbstraction.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct
{
    void (*StartPwm)(void);
    void (*StopPwm)(void);
    void (*SetDuty)(Uint16 u16Duty);
    void (*SetPrd)(Uint16 u16Prd);
    void (*WritePin)(Uint32 u32Pin, Uint32 u32Value);
}ST_FAN_DRIVER;

typedef struct
{
    Uint16 u16PwmPrd;
    Uint16 u16Pin;
}ST_FAN_DATA;

typedef struct
{
    const char *pchName;
    float fTemperature; // 当前温度
    Uint16 u16LastGear; // 上次设定的风扇挡位
    Uint16 u16Thresholds[4];    // 4个挡位的阈值
    Uint16 u16DisThresholds; // 放电降额
    Uint16 u16ChaThresholds; // 充电降额
    Uint16 u16LastDisCoefficient; // 放电降额系数
    Uint16 u16LastChaCoefficient; // 充电降额系数
}ST_FAN_APP_DATA;

typedef struct
{
    ST_DEVICE stDevice;
    ST_FAN_APP_DATA stFanAppData[4];
    Uint16 u16FanSpeedLevel;
    Uint16 u16FanSpeedLevelBak;
    Uint16 u16DisThresholdsCoefficient; // 放电降额系数
    Uint16 u16ChaThresholdsCoefficient; // 充电降额系数

}ST_FAN_HANDLE;

extern void FanInit(void);
extern void FanProcess(void);
extern void TaskFan(void);

extern ST_FAN_HANDLE stFanHandle;

#ifdef __cplusplus
    }
#endif

#endif

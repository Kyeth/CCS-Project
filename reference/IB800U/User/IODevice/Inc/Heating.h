#ifndef __HEATING_H
#define __HEATING_H

#include "F28003x_device.h"
#include "DeviceAbstraction.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct
{
    void (*WritePin)(Uint32 u32Pin, Uint32 u32Value);
    void (*StartPwm)(void);
    void (*StopPwm)(void);
    void (*SetDuty)(Uint16 u16Duty);
}ST_HEATING_DRIVER;

typedef struct
{
    Uint16 u16Pin_1;
    Uint16 u16Pin_2;
}ST_HEATING_DATA;

typedef struct
{
    ST_DEVICE stDevice1;
    Uint16 u16HeatingLevel; // 最大3级：0-1-2-3(1：开上管，2：开下管，3：全开)
    Uint16 u16HeatingLevelBak;
    Uint16 u16Duty; // 占空比(0-100)
}ST_HEATING_HANDLE;

extern void HeatingInit(void);
extern void TaskHeating(void);
extern ST_HEATING_HANDLE stHeatingHandle;

#ifdef __cplusplus
    }
#endif

#endif

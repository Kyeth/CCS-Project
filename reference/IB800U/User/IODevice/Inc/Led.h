#ifndef __LED_H
#define __LED_H

#include "F28003x_device.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct
{
    void (*WritePin)(Uint32 u32Pin, Uint32 u32Value);
    void (*TogglePin)(Uint32 u32Pin);
}ST_LED_DRIVER;

typedef struct
{
    Uint16 u16Pin;
    Uint16 u16CurrentState;
}ST_LED_DATA;

extern void LedInit(void);
extern void TaskLed(void);
extern void LedCloseFaultStatus(void);

#ifdef __cplusplus
    }
#endif

#endif

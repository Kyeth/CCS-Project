#ifndef HW_CLA_TASK_H
#define HW_CLA_TASK_H

#include "F2837xS_device.h"

/* Diagnostic data only. No application control or PWM writes in this task. */
typedef struct
{
    Uint32 u32Sequence;
    Uint16 u16AdcaAnchor;
    Uint16 u16MuxAddress;
    Uint16 u16NextMuxAddress;
    float fMuxB3;
    float fMuxD3;
} ST_CLA_TO_CPU_MIN;

typedef struct
{
    Uint16 u16MuxAddress;
    Uint16 u16NextMux[8];
    float fAlpha;
} ST_CPU_TO_CLA_MIN;

extern volatile ST_CLA_TO_CPU_MIN Cla1ToCpuMsg;
extern volatile ST_CPU_TO_CLA_MIN CpuToCla1Msg;
extern __interrupt void Cla1Task1(void);

#endif

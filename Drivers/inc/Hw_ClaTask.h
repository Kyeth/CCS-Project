#ifndef HW_CLA_TASK_H
#define HW_CLA_TASK_H

#include "F2837xS_device.h"

/* CLA Task1 only advances the MUX address. No ADC read, no filter, no control. */
typedef struct
{
    Uint32 u32Sequence;
    Uint16 u16NextMuxAddress;
} ST_CLA_TO_CPU_MIN;

typedef struct
{
    Uint16 u16MuxAddress;
    Uint16 u16NextMux[8];
} ST_CPU_TO_CLA_MIN;

extern volatile ST_CLA_TO_CPU_MIN Cla1ToCpuMsg;
extern volatile ST_CPU_TO_CLA_MIN CpuToCla1Msg;
extern __interrupt void Cla1Task1(void);

#endif

#ifndef HW_CLOCK_DRIVER_H
#define HW_CLOCK_DRIVER_H

#include "F2837xS_device.h"

typedef struct
{
    Uint16 u16OscSource;
    Uint16 u16PllImult;
    Uint16 u16PllFmult;
    Uint16 u16SysclkDiv;
    Uint16 u16EpwmclkDiv;
    Uint16 u16LspclkDiv;
    Uint32 u32XtalHz;
    Uint32 u32SysclkHz;
    Uint32 u32EpwmclkHz;
    Uint32 u32LspclkHz;
} ST_CLOCK_CFG;

Uint16 Hw_ClockInit(const ST_CLOCK_CFG *cfg);

#endif

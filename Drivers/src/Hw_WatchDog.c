/**
 * @file Hw_WatchDog.c
 * @brief F28374S watchdog enable and service operations.
 */

#include "Hw_WatchDog.h"
#include "F2837xS_device.h"
#include "F2837xS_Examples.h"

void Hw_EnableWatchDog(void)
{
    ServiceDog();
    EALLOW;
    WdRegs.WDCR.all = 0x0028U | (WdRegs.WDCR.all & 0x0007U);
    EDIS;
}

void Hw_FeedWatchDog(void)
{
    ServiceDog();
}

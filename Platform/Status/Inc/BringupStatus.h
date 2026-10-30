/**
 * @file BringupStatus.h
 * @brief Shared observables for the minimal peripheral bring-up image.
 */

#ifndef BRINGUP_STATUS_H
#define BRINGUP_STATUS_H

#include "F2837xS_device.h"

typedef union
{
    Uint16 all;
    struct
    {
        Uint16 bClaInitError : 1;   // CLA initialization failed.
        Uint16 bSciInitError : 1;   // SCI initialization failed.
        Uint16 bCanInitError : 1;   // CAN A or CAN B initialization failed.
        Uint16 bClockInitError : 1; // Clock initialization or clock-table check failed.
        Uint16 reserved : 12;      // Reserved for future initialization errors.
    } Bits;
} ST_BRINGUP_ERROR;

typedef struct
{
    Uint32 u32HeartbeatMs;       // CPU Timer1 heartbeat count, nominally 1 ms per tick.
    Uint32 u32ControlIsrCount;   // Completed CLA-to-CPU control interrupt count.
    Uint16 u16ControlStallMs;    // Consecutive heartbeats without a control interrupt.
    ST_BRINGUP_ERROR unError;    // Initialization error flags; all == 0 means no reported error.
    Uint16 bReady : 1;           // Initialization passed and ePWM time bases were started.
    Uint16 reserved : 15;        // Reserved for future one-bit status flags.
} ST_BRINGUP_STATUS;

extern volatile ST_BRINGUP_STATUS stBringupStatus;

#endif /* BRINGUP_STATUS_H */

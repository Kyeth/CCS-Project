/**
 * @file BringupStatus.h
 * @author yetingheng (004182)
 * @brief Shared observables for the minimal peripheral bring-up image.
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
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
        Uint16 bPortInitError : 1;  // Port layer setup failed (CAN-B frame type binding).
        Uint16 bTaskCreateError : 1;  // A task could not be registered (AdcSampleTask / PortTask).
        Uint16 reserved : 10;      // Reserved for future initialization errors.
    } Bits;
} ST_BRINGUP_ERROR;

typedef struct
{
    Uint32 u32HeartbeatMs;       // 1 ms heartbeat count, advanced once per 10 hardware timer interrupts.
    Uint32 u32ControlIsrCount;   // Completed CLA-to-CPU control interrupt count.
    Uint32 u32Tick100us;         // CPU Timer1 ISR count, nominally 100 us per tick.
                                 // Software time base only: ticks skipped while
                                 // interrupts were disabled are not back-filled.
    Uint16 u16ControlStallMs;    // Consecutive heartbeats without a control interrupt.
    ST_BRINGUP_ERROR unError;    // Initialization error flags; all == 0 means no reported error.
    Uint16 u16CalState;          // Sample zero-calibration state (E_ADC_CAL_STATE).
    Uint16 bReady : 1;           // Initialization passed and ePWM time bases were started.
    Uint16 bCalReady : 1;        // Sample zero calibration finished; L2 values are live.
    Uint16 reserved : 14;        // Reserved for future one-bit status flags.
} ST_BRINGUP_STATUS;

extern volatile ST_BRINGUP_STATUS stBringupStatus;

#endif /* BRINGUP_STATUS_H */

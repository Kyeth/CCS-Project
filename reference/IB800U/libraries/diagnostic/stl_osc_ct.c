//###########################################################################
//
// FILE:  stl_osc_ct.c
//
// TITLE: Diagnostic Library Oscillator CPU Timer software module source
//
//###########################################################################
// $TI Release: C2000 Diagnostic Library v4.00.00 $
// $Release Date: Thu Feb  2 23:06:40 IST 2023 $
// $Copyright:
// Copyright (C) 2023 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Includes
//
#include "stl_osc_ct.h"
#include "stl_util.h"
#include "cputimer.h"

//*****************************************************************************
//
// STL_OSC_CT_startTest(const STL_OSC_CT_Handle oscTimer2Handle)
//
//*****************************************************************************
void STL_OSC_CT_startTest(const STL_OSC_CT_Handle oscTimer2Handle)
{
    //
    // Stop Timer2.
    //
    CPUTimer_stopTimer(CPUTIMER2_BASE);

    //
    // Clear overflow flag.
    //
    CPUTimer_clearOverflowFlag(CPUTIMER2_BASE);

    //
    // Disable interrupt.
    //
    CPUTimer_disableInterrupt(CPUTIMER2_BASE);

    //
    // Initialize Timer2 period to maximum.
    //
    CPUTimer_setPeriod(CPUTIMER2_BASE, STL_OSC_CT_PERIOD);

    //
    // Initialize pre-scaler counter to divide by 1 (SYSCLKOUT).
    //
    CPUTimer_setPreScaler(CPUTIMER2_BASE, 0U);

    //
    // Reload Timer2 counter.
    //
    CPUTimer_reloadTimerCounter(CPUTIMER2_BASE);

    //
    // Set prescaler to the input prescaler value.
    // Select Timer2 clock source.
    //
    CPUTimer_selectClockSource(CPUTIMER2_BASE, oscTimer2Handle->clockSource,
                               oscTimer2Handle->prescaler);

    //
    // Start CPU Timer2.
    //
    CPUTimer_startTimer(CPUTIMER2_BASE);
}

//*****************************************************************************
//
// STL_OSC_CT_stopTest(const STL_OSC_CT_Handle oscTimer2Handle)
//
//*****************************************************************************
uint32_t counterDelta;
uint16_t STL_OSC_CT_stopTest(const STL_OSC_CT_Handle oscTimer2Handle)
{
//    uint32_t counterDelta;
    uint16_t testStatus;

    //
    // Stop CPU Timer 2.
    //
    CPUTimer_stopTimer(CPUTIMER2_BASE);

    //x
    // Get the Timer2 counter delta.
    //
    uint32_t timetest = CPUTimer_getTimerCount(CPUTIMER2_BASE);
    counterDelta = (uint32_t)STL_OSC_CT_PERIOD -
                   CPUTimer_getTimerCount(CPUTIMER2_BASE);

    //
    // Check if Timer2 has overflowed or if Timer2 counter
    // is outside of the bounds.
    //
    if((counterDelta <= oscTimer2Handle->minCount) ||
       (counterDelta >= oscTimer2Handle->maxCount) ||
       CPUTimer_getTimerOverflowStatus(CPUTIMER2_BASE))
    {
        //
        // Set the global error flag.
        //
        STL_Util_setErrorFlag(STL_UTIL_OSC_TIMER2);

        testStatus = STL_OSC_CT_FAIL;
    }
    else
    {
        testStatus = STL_OSC_CT_PASS;
    }

    return(testStatus);
}

//
// End of File
//

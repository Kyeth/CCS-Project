//#############################################################################
//
// FILE:  stl_cpu_reg.h
//
// TITLE: Diagnostic Library CPU register test module header
//
//#############################################################################
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
//#############################################################################

#ifndef STL_CPU_REG_H
#define STL_CPU_REG_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

//
// Includes
//
#include <stdint.h>
#include "inc/hw_types.h"
#include "stl_util.h"

//*****************************************************************************
//
//! \addtogroup stl_cpu_reg CPU Register API Functions
//!
//! @{
//
//*****************************************************************************

//
// Defines
//
#define STL_CPU_REG_PASS        0x0U
#define STL_CPU_REG_FAIL        0x1U

//
// Prototypes
//

//*****************************************************************************
//
// C-callable assembly routines to test the core registers for stuck at bits.
// They are called by the high level inline test functions below. They return
// STL_CPU_REG_PASS on success and STL_CPU_REG_FAIL on a failure.
//
// The scratchRAM parameter should be a pointer to RAM memory that will be
// used by the function to transfer data between the CPU and extended
// instruction set registers.
//
//*****************************************************************************
extern uint16_t STL_CPU_REG_testCPURegisters(bool injectError);
extern uint16_t STL_CPU_REG_testFPURegisters(bool injectError);
extern uint16_t STL_CPU_REG_testVCRCRegisters(uint32_t *scratchRAM,
                                              bool injectError);

//*****************************************************************************
//
//! \brief Tests CPU registers
//!
//! \param injectError when \e false the test writes the test pattern to the
//! registers as expected. When \e true, the test will write an unexpected
//! error pattern to ACC to simulate a stuck bit, causing the test to fail.
//!
//! This function tests CPU core registers for stuck bits. The following
//! registers are tested:
//! - ACC
//! - P
//! - XAR0 to XAR7
//! - XT
//! - SP
//! - IFR, IER and DBGIER
//! - ST0
//! - ST1 (excluding IDLESTAT and LOOP bits)
//! - DP
//!
//! The values of ST0, ST1, DP, IER, IFR, and DBGIER and the save-on-entry XAR
//! registers, as defined by the compiler calling convention, are saved and
//! restored in this test.
//!
//! Note that the IDLESTAT and LOOP bits of ST1 are not tested by this function
//! as they are read-only. IDLESTAT is set when the IDLE instruction is used to
//! enter a low-power mode. Performing a functional test of the low-power mode
//! will indirectly serve as a test of IDLESTAT.
//!
//! The LOOP bit is set by the LOOPZ and LOOPNZ instructions. The C2000
//! compiler will not generate code that uses these instructions. If an
//! application contains hand-coded assembly that uses them, the LOOP bit may
//! also be tested by performing a functional test of the applicable LOOP
//! instruction.
//!
//! \note You should disable interrupts before calling this test. Also note
//! this test was written with POST use in mind, particularly before interrupts
//! are enabled even at the PIE or CPU IER level. Keep in mind that INTM being
//! set doesn't prevent interrupts from propagating to the IFR registers, so
//! although this function saves and restores IFR, an interrupt arriving in IFR
//! during this test will be cleared by the restore or could cause a failure of
//! this test. If this test needs to be used during runtime, you may want to
//! modify STL_CPU_REG_testCPURegisters() in stl_cpu_reg.asm to remove the IFR
//! test (and related save and restore). Alternatively, you can disable all
//! interrupts at the PIE level (PIEIER) or in the case of non-PIE interrupts
//! (like CPU Timers 1 and 2) at the peripheral level and restore them after
//! STL_CPU_REG_checkCPURegisters() completes.
//!
//! \return If the test passes, the routine returns \b STL_CPU_REG_PASS.
//!         Otherwise, it returns \b STL_CPU_REG_FAIL.
//
//*****************************************************************************
static inline uint16_t STL_CPU_REG_checkCPURegisters(bool injectError)
{
    uint16_t returnVal;

    returnVal = STL_CPU_REG_testCPURegisters(injectError);

    //
    // If test failed, set global error flag.
    //
    if(STL_CPU_REG_PASS != returnVal)
    {
        STL_Util_setErrorFlag(STL_UTIL_CPU_REG);
    }

    return(returnVal);
}

//*****************************************************************************
//
//! \brief Tests FPU registers
//!
//! \param injectError when \e false the test writes the test pattern to the
//! registers as expected. When \e true, the test will write an unexpected
//! error pattern to R5H to simulate a stuck bit, causing the test to fail.
//!
//! This function tests FPU registers for stuck bits. The following registers
//! are tested:
//! - R0 to R7
//! - RND32, TF, ZI, NI, ZF, NF bits of STF register.
//! - Shadow registers for R0 to R7 and STF
//!
//! The values of STF and the save-on-entry RnH registers, as defined by the
//! compiler calling convention, are saved and restored in this test.
//!
//! \note You should disable interrupts before calling this test.
//!
//! \return If the test passes, the routine returns \b STL_CPU_REG_PASS.
//!         Otherwise, it returns \b STL_CPU_REG_FAIL.
//
//*****************************************************************************
static inline uint16_t STL_CPU_REG_checkFPURegisters(bool injectError)
{
    uint16_t returnVal;

    returnVal = STL_CPU_REG_testFPURegisters(injectError);

    //
    // If test failed, set global error flag.
    //
    if(STL_CPU_REG_PASS != returnVal)
    {
        STL_Util_setErrorFlag(STL_UTIL_FPU_REG);
    }

    return(returnVal);
}

//*****************************************************************************
//
//! \brief Tests VCRC registers
//!
//! \param injectError when \e false the test writes the test pattern to the
//! registers as expected. When \e true, the test will write an unexpected
//! error pattern to VCRCPOLY to simulate a stuck bit, causing the test to fail.
//!
//! This function tests VCRC (Cyclic Redundancy Check Unit) registers for
//! stuck bits. The following registers are tested:
//! - VCRCPOLY
//! - VCRCSIZE
//! - VCRC
//! - VSTATUS
//!
//! The values of VCRCPOLY, VCRCSIZE, and VSTATUS are saved and restored in this
//! test.
//!
//! \note You should disable interrupts before calling this test.
//!
//! \return If the test passes, the routine returns \b STL_CPU_REG_PASS.
//!         Otherwise, it returns \b STL_CPU_REG_FAIL.
//
//*****************************************************************************
static inline uint16_t STL_CPU_REG_checkVCRCRegisters(bool injectError)
{
    uint16_t returnVal;
    uint32_t scratchPad;

    returnVal = STL_CPU_REG_testVCRCRegisters(&scratchPad, injectError);

    //
    // If test failed, set global error flag.
    //
    if(STL_CPU_REG_PASS != returnVal)
    {
        STL_Util_setErrorFlag(STL_UTIL_VCRC_REG);
    }

    return(returnVal);
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // STL_CPU_REG_H

//
// End of File
//

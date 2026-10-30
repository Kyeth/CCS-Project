//#############################################################################
//
// FILE: stl_mcan_ram.h
//
// TITLE: Diagnostic Library MCAN message RAM test software module header
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

#ifndef STL_MCAN_RAM_H
#define STL_MCAN_RAM_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//
// Includes
//
#include <stdbool.h>
#include <stdint.h>
#include "stl_march.h"
#include "mcan.h"

//*****************************************************************************
//
//!
//! \addtogroup stl_mcan_ram MCAN RAM API Functions
//!
//! @{
//
//*****************************************************************************

//
// Defines
//
#define STL_MCAN_RAM_PASS            0U
#define STL_MCAN_RAM_ERROR           1U

//
// Values for address calculation and checking
//
#define STL_MCAN_RAM_MAX_ADDR_OFFSET 0x1FFFU // Offset from MCAN base address
                                             // to message RAM end

//
//! Value to indicate to STL_MCAN_RAM_testRAM() through the \b copyAddress
//! parameter that the test should run a destructive test on the region of MCAN
//! message RAM without saving and restoring its contents.
//
#define STL_MCAN_RAM_NO_COPY         0xFFFFFFFFU

//
// Prototypes
//
//*****************************************************************************
//
//! \brief Performs a March13N memory test on the specified range of MCAN
//! message RAM objects.
//!
//! \param mcanBase is the base address of the MCAN instance.
//! \param pattern is the test pattern to use.
//! \param startAddress is the starting address (inclusive) of the MCAN message
//! RAM range to test.
//! \param endAddress is the end address (inclusive) of the MCAN message RAM
//! range to test.
//! \param copyAddress is the address to copy the original contents of the
//! memory under test. It will be used to restore the original memory at the
//! end of the March13N memory test. If no save and restore is required, use
//! a value of \b STL_MCAN_RAM_NO_COPY.
//!
//! This function performs a March13N memory test on the range of MCAN message
//! RAM specified by \b mcanBase, \b startAddress and \b endAddress.
//! The test can save and restore the original contents of the message RAM
//! by passing an address to a back up buffer through the \b copyAddress
//! parameter. The test will copy the original contents of the memory to
//! \b copyAddress, perform the memory test, and then copy the original
//! contents back to the memory under test. To skip the save and restore, use
//! a value of \b STL_MCAN_RAM_NO_COPY for the \b copyAddress and a destructive
//! test will be performed instead.
//!
//! The test patterns and the March13N memory test algorithm provided test
//! the memory for stuck-at-faults. The message RAM ECC logic will be used to
//! detect issues. Use STL_MCAN_RAM_checkErrorStatus() to read the ECC status.
//!
//! \note Note to take care calculating the size of memory needed for
//! \b copyAddress. The March13N function is not specific to the MCAN message
//! RAM and does not take the byte addressability of the memory into account.
//! Use a buffer twice the size of the actual message RAM region you are
//! testing.
//!
//! \note The STL_March functions called by this test disable global CPU
//! interrupts (DINT) and then re-enable them after the test has completed.
//!
//! \return None.
//
//*****************************************************************************
extern void STL_MCAN_RAM_testRAM(const uint32_t mcanBase,
                                 const STL_March_Pattern pattern,
                                 const uint32_t startAddress,
                                 const uint32_t endAddress,
                                 const uint32_t copyAddress);

//*****************************************************************************
//
//! \brief Injects an error into a MCAN message RAM address.
//!
//! \param mcanBase is the base address of the MCAN instance.
//! \param address is the address of the word in the message RAM where the
//! error will be injected.
//! \param xorMask mask of the bit or bits to flip in \b address.
//!
//! This function injects an error at a specific memory \b address. \b xorMask
//! specifies which bit to flip in order to corrupt the data.
//!
//! \return None.
//
//*****************************************************************************
extern void STL_MCAN_RAM_injectError(const uint32_t mcanBase,
                                     const uint32_t address,
                                     const uint32_t xorMask);

//*****************************************************************************
//
//! \brief Returns the MCAN message RAM ECC error status.
//!
//! \param mcanBase is the base address of the MCAN instance.
//! \param errStatus is a pointer to the structure in which the detailed ECC
//! error status will be returned.
//!
//! This function checks if any ECC errors have been detected or corrected by
//! the MCAN message RAM ECC logic.
//!
//! \return If the MCAN error status register indicates an ECC error the
//! function returns \b STL_MCAN_RAM_ERROR and \b errStatus will contain
//! additional details about the type of error. Otherwise, the function returns
//! \b STL_MCAN_RAM_PASS.
//
//*****************************************************************************
extern uint16_t STL_MCAN_RAM_checkErrorStatus(const uint32_t mcanBase,
                                              MCAN_ECCErrStatus *errStatus);

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

#endif  //  STL_MCAN_RAM_H

//
// End of File
//

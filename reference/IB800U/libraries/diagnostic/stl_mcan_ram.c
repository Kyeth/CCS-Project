//###########################################################################
//
// FILE:  stl_mcan_ram.c
//
// TITLE: Diagnostic Library MCAN message RAM test software module source
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
#include "stl_mcan_ram.h"
#include "stl_util.h"
#include "inc/hw_types.h"

//*****************************************************************************
//
// STL_MCAN_RAM_testRAM(const uint32_t mcanBase,
//                     const STL_March_Pattern pattern,
//                     const uint16_t startAddress,
//                     const uint16_t endAddress,
//                     const uint32_t copyAddress)
//
//*****************************************************************************
void STL_MCAN_RAM_testRAM(const uint32_t mcanBase,
                          const STL_March_Pattern pattern,
                          const uint32_t startAddress,
                          const uint32_t endAddress,
                          const uint32_t copyAddress)
{
    uint32_t length;

    ASSERT(mcanBase == MCANA_DRIVER_BASE);
    ASSERT(endAddress >= startAddress);
    ASSERT(mcanBase <= startAddress);
    ASSERT((mcanBase + STL_MCAN_RAM_MAX_ADDR_OFFSET) >= endAddress);

    //
    // The length needs to be divided by 2 since the test function expects a
    // length in terms of 32-bit words.
    //
    if(endAddress >= startAddress)
    {
        length = (endAddress - startAddress) / 2U;
    }
    else
    {
        length = 0U;
    }

    //
    // Run the March test.
    //
    if(STL_MCAN_RAM_NO_COPY == copyAddress)
    {
        STL_March_testRAM(pattern, startAddress, length);
    }
    else
    {
        STL_March_testRAMCopy(pattern, startAddress, length, copyAddress);
    }
}

//*****************************************************************************
//
// STL_MCAN_RAM_injectError(const uint32_t mcanBase, const uint32_t address,
//                          const uint32_t xorMask);
//
//*****************************************************************************
void STL_MCAN_RAM_injectError(const uint32_t mcanBase, const uint32_t address,
                              const uint32_t xorMask)
{
    MCAN_ECCConfigParams eccConfig;

    ASSERT(mcanBase == MCANA_DRIVER_BASE);
    ASSERT((address >= mcanBase) &&
           (address <= (mcanBase + STL_MCAN_RAM_MAX_ADDR_OFFSET)));

    //
    // Turn off ECC, so we can flip a bit without updating it.
    //
    eccConfig.enable        = 0U;
    eccConfig.enableChk     = 0U;
    eccConfig.enableRdModWr = 0U;
    MCAN_eccConfig(mcanBase, &eccConfig);

    //
    // Flip a bit (or multiple bits).
    //
    HWREG_BP(address) ^= xorMask;

    //
    // Turn ECC back on.
    //
    eccConfig.enable        = 1U;
    eccConfig.enableChk     = 1U;
    eccConfig.enableRdModWr = 0U;
    MCAN_eccConfig(mcanBase, &eccConfig);
}

//*****************************************************************************
//
// STL_MCAN_RAM_checkErrorStatus(const uint32_t mcanBase,
//                               MCAN_ECCErrStatus *errStatus)
//
//*****************************************************************************
uint16_t STL_MCAN_RAM_checkErrorStatus(const uint32_t mcanBase,
                                       MCAN_ECCErrStatus *errStatus)
{
    uint16_t status;

    ASSERT(mcanBase == MCANA_DRIVER_BASE);

    //
    // Read the error status fields from the MCAN registers.
    //
    MCAN_eccGetErrorStatus(mcanBase, errStatus);

    if((errStatus->dedErr == 0U) && (errStatus->secErr == 0U))
    {
        status = STL_MCAN_RAM_PASS;
    }
    else
    {
        status = STL_MCAN_RAM_ERROR;

        //
        // Set global error flag.
        //
        STL_Util_setErrorFlag(STL_UTIL_MCAN_RAM_ECC);
    }

    return(status);
}

//
// End of File
//

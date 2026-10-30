//###########################################################################
//
// FILE:    Fapi_UserDefinedFunctions.c
//
// Description: Contains all user defined functions that the Fapi functions use.
//
//###########################################################################
// $TI Release: F2837xS Support Library v170 $
// $Release Date: Mon Sep 21 16:56:07 CDT 2015 $
// $Copyright: Copyright (C) 2014-2015 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

/* Uncomment the appropriate include file for your device */

/*#include "F021_FMC_BE.h"*/
/*#include "F021_FMC_LE.h"*/
/*#include "F021_Concerto_C28x.h"*/
/*#include "F021_Concerto_Cortex.h"*/
#include "F021_F2837xD_C28x.h"

Fapi_StatusType Fapi_serviceWatchdogTimer(void)
{
   /* User to add their own watchdog servicing code here */

   return(Fapi_Status_Success);
}

Fapi_StatusType Fapi_setupEepromSectorEnable(void)
{
   Fapi_GlobalInit.m_poFlashControlRegisters->Fbse.u32Register = 0xFFFF;   /* Value must be 0xFFFF to enable erase and programming of the EEPROM bank, 0 to disable */
   FAPI_WRITE_LOCKED_FSM_REGISTER(Fapi_GlobalInit.m_poFlashControlRegisters->FsmSector.u32Register, 0x0U); /* Enables sectors 32-63 for bank and sector erase */
   FAPI_WRITE_LOCKED_FSM_REGISTER(Fapi_GlobalInit.m_poFlashControlRegisters->FsmSector1.u32Register, 0x0U); /* Enables sectors 0-31 for bank and sector erase */
   FAPI_WRITE_LOCKED_FSM_REGISTER(Fapi_GlobalInit.m_poFlashControlRegisters->FsmSector2.u32Register, 0x0U); /* Enables sectors 32-63 for bank and sector erase */

   return(Fapi_Status_Success);
}

Fapi_StatusType Fapi_setupBankSectorEnable(void)
{
   Fapi_GlobalInit.m_poFlashControlRegisters->Fbse.u32Register = 0xFFFF;                  /* Enable sectors 0-15 for erase and programming */
   FAPI_WRITE_LOCKED_FSM_REGISTER(Fapi_GlobalInit.m_poFlashControlRegisters->FsmSector.u32Register, 0x0U);

   return(Fapi_Status_Success);
}

/* End of File */


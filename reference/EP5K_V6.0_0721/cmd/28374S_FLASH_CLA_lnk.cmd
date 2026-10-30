#define CLA_C

// The user must define CLA_C in the project linker settings if using the
// CLA C compiler
// Project Properties -> C2000 Linker -> Advanced Options -> Command File
// Preprocessing -> --define
#ifdef CLA_C
// Define a size for the CLA scratchpad area that will be used
// by the CLA compiler for local symbols and temps
// Also force references to the special symbols that mark the
// scratchpad are.
CLA_SCRATCHPAD_SIZE = 0x100;
--undef_sym=__cla_scratchpad_end
--undef_sym=__cla_scratchpad_start
#endif //CLA_C

MEMORY
{
PAGE 0 :  /* Program Memory */
          /* Memory (RAM/FLASH) blocks can be moved to PAGE1 for data allocation */
          /* BEGIN is used for the "boot to Flash" bootloader mode   */

   BEGIN            : origin = 0xA8004, length = 0x000002
   RAMM0            : origin = 0x000122, length = 0x0006DE     /* on-chip RAM block M1 */

   RAMD0            : origin = 0x00B000, length = 0x001000
//   RAMD1            : origin = 0x00B800, length = 0x000800

    RAMGS1          : origin = 0x00D000, length = 0x004000  //1234


   RAMLS2  			: origin = 0x008000, length = 0x001000	//Cla1Prog 0 1
   RAMLS03          : origin = 0x009000, length = 0x001000

   RESET            : origin = 0x3FFFC0, length = 0x000002

   /* Flash sectors */
  FLASHC           : origin = 0x084000, length = 0x002000  /* on-chip Flash */
  FLASHD           : origin = 0x086000, length = 0x002000  /* on-chip Flash */

  FLASHE           : origin = 0x088000, length = 0x008000  /* on-chip Flash */
  FLASHF           : origin = 0x090000, length = 0x008000  /* on-chip Flash */

    FLASH_APP_START :origin = 0xA8000, length = 0x000001
    FLASH_APP :      origin = 0xA8006, length = 0x0FFF9  /* on-chip Flash 0xBBFFF */
    FLASH_APP_END  : origin = 0xB7FFF, length = 0x000001    /* on-chip FLASH */
    FLASH_APP_VERSION :origin = 0xA8002, length = 0x000001

PAGE 1 : /* Data Memory */
         /* Memory (RAM/FLASH) blocks can be moved to PAGE0 for program allocation */

    BOOT_RSVD       : origin = 0x000002, length = 0x000120     /* Part of M0, BOOT rom will use this for stack */
    RAMLS5          : origin = 0x00A000, length = 0x001000
    RAMGS0          : origin = 0x00C000, length = 0x001000
    RAMGS5          : origin = 0x011000, length = 0x001000
    RAMGS6          : origin = 0x012000, length = 0x001000
    RAMGS7          : origin = 0x013000, length = 0x001000

   CLA1_MSGRAMLOW   : origin = 0x001480,   length = 0x000080
   CLA1_MSGRAMHIGH  : origin = 0x001500,   length = 0x000080
}

SECTIONS
{
   /* Allocate program areas: */
   .cinit              : > FLASH_APP      PAGE = 0, ALIGN(4)
   .pinit              : > FLASH_APP,     PAGE = 0, ALIGN(4)
   .text               : > FLASH_APP       PAGE = 0, ALIGN(4)
   codestart           : > BEGIN       PAGE = 0, ALIGN(4)

#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
    .TI.ramfunc : {} LOAD = FLASH_APP,
                         RUN = RAMGS1// |RAMLS3,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #else
   ramfuncs            : LOAD = FLASH_APP,
                         RUN = RAMGS1// |RAMLS3,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #endif
#endif

   /* Allocate uninitalized data sections: */
   .stack              : > RAMD0      PAGE = 0
   .ebss               : >>  RAMGS5 | RAMGS6 | RAMGS7 | RAMGS0       PAGE = 1
   .esysmem            : > RAMGS5       PAGE = 1
    flashflag_app_start : > FLASH_APP_START,             PAGE = 0
   flashflag_app_end   : > FLASH_APP_END,             PAGE = 0
   flashflag_app_version : > FLASH_APP_VERSION,         PAGE = 0
   /* Initalized sections go in Flash */
   .econst             : >> FLASH_APP       PAGE = 0, ALIGN(4)

   .switch             : > FLASH_APP      PAGE = 0, ALIGN(4)

   .reset              : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

       /* CLA specific sections */
	Cla1Prog    : LOAD = FLASH_APP,
                  RUN = RAMLS2,
                  LOAD_START(_Cla1funcsLoadStart),
                  LOAD_END(_Cla1funcsLoadEnd),
                  RUN_START(_Cla1funcsRunStart),
                  LOAD_SIZE(_Cla1funcsLoadSize),
                  PAGE = 0, ALIGN(8)

   Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW,   PAGE = 1
   CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH,  PAGE = 1


   #ifdef CLA_C
   /* CLA C compiler sections */
   //
   // Must be allocated to memory the CLA has write access to
   //
   CLAscratch       :
                     { *.obj(CLAscratch)
                     . += CLA_SCRATCHPAD_SIZE;
                     *.obj(CLAscratch_end) } >  RAMLS5,  PAGE = 1

   .scratchpad      : > RAMLS5,       PAGE = 1
   .bss_cla		    : > RAMLS5,       PAGE = 1
   /*
   .const_cla	    :  LOAD = FLASH_APP,
                       RUN = RAMLS5,
                       RUN_START(_Cla1ConstRunStart),
                       LOAD_START(_Cla1ConstLoadStart),
                       LOAD_SIZE(_Cla1ConstLoadSize),
                       PAGE = 1*/
                       #endif //CLA_C

}

/*
//===========================================================================
// End of file.
//===========================================================================


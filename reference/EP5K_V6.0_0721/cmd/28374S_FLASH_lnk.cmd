
MEMORY
{
PAGE 0 :  /* Program Memory */
          /* Memory (RAM/FLASH) blocks can be moved to PAGE1 for data allocation */
   RAMM0            : origin = 0x000122, length = 0x0002DE

   /* M0 M1 for stack */
   RAMD0            : origin = 0x00B000, length = 0x000800
   RAMD1            : origin = 0x00B800, length = 0x000800

   /* 20KW:GS0~GS4 */
   RAMLS_CODE      : origin = 0x00C000, length = 0x005000
   /* RAMGS0      : origin = 0x00C000, length = 0x001000 */
   /* RAMGS1      : origin = 0x00D000, length = 0x001000 */
   /* RAMGS2      : origin = 0x00E000, length = 0x001000 */
   /* RAMGS3      : origin = 0x00F000, length = 0x001000 */
   /* RAMGS4      : origin = 0x010000, length = 0x001000 */

   RESET            : origin = 0x3FFFC0, length = 0x000002
   

#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 20012000
GROUP {      /* GROUP memory ranges for crc/checksum of entire flash */
   #endif
#endif   
   /* BEGIN is used for the "boot to Flash" bootloader mode   */
   BEGIN            : origin = 0xA8004, length = 0x000002

   /* Flash sectors 64KW */
   /* FLASH_APP        : origin = 0x080002, length = 0x00FFFE */ /* on-chip Flash */
   FLASH_APP_START  : origin = 0xA8000, length = 0x01
   FLASH_APP        : origin = 0xA8006, length = 0x00FFF9
   FLASH_APP_END    : origin = 0xB7FFF, length = 0x000001
   FLASH_APP_VERSION :origin = 0xA8002, length = 0x000001
   /* FLASHA           : origin = 0x080002, length = 0x001FFE    on-chip Flash */
   /* FLASHB           : origin = 0x082000, length = 0x002000    on-chip Flash */
   /* FLASHC           : origin = 0x084000, length = 0x002000    on-chip Flash */
   /* FLASHD           : origin = 0x086000, length = 0x002000    on-chip Flash */
   /* FLASHE           : origin = 0x088000, length = 0x008000    on-chip Flash */

#ifdef __TI_COMPILER_VERSION__
  #if __TI_COMPILER_VERSION__ >= 20012000
}  crc(_table_name, algorithm=C28_CHECKSUM_16)
  #endif
#endif


PAGE 1 : /* Data Memory */
         /* Memory (RAM/FLASH) blocks can be moved to PAGE0 for program allocation */

   BOOT_RSVD       : origin = 0x000002, length = 0x000120     /* Part of M0, BOOT rom will use this for stack */
   RAMM1           : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */

   RAMGS5      : origin = 0x011000, length = 0x001000
   /* RAMGS6      : origin = 0x012000, length = 0x001000 */

   RAMGS6L      : origin = 0x012000, length = 0x000800
   RAMGS6H      : origin = 0x012800, length = 0x000800

   /* RAMGS7      : origin = 0x013000, length = 0x001000 */
   RAMGS7L      : origin = 0x013000, length = 0x000800
   RAMGS7H      : origin = 0x013800, length = 0x000800

   RAMGS8      : origin = 0x014000, length = 0x001000
   RAMGS9      : origin = 0x015000, length = 0x001000
   RAMGS10     : origin = 0x016000, length = 0x001000
   RAMGS11     : origin = 0x017000, length = 0x001000


   /* 12KW:LS0~LS5 */
   RAMGS_DATA       : origin = 0x008000, length = 0x003000
   /* RAMLS0            : origin = 0x008000, length = 0x000800 */
   /* RAMLS1            : origin = 0x008800, length = 0x000800 */
   /* RAMLS2            : origin = 0x009000, length = 0x000800 */
   /* RAMLS3            : origin = 0x009800, length = 0x000800 */
   /* RAMLS4            : origin = 0x00A000, length = 0x000800 */
   /* RAMLS5            : origin = 0x00A800, length = 0x000800 */

}

SECTIONS
{
   /* Allocate program areas: */
   .cinit              : > FLASH_APP      PAGE = 0, ALIGN(4)
   .pinit              : > FLASH_APP      PAGE = 0, ALIGN(4)
   .text               : >> FLASH_APP     PAGE = 0, ALIGN(4)
   codestart           : > BEGIN          PAGE = 0, ALIGN(4)

#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
    .TI.ramfunc : {} LOAD = FLASH_APP,
                         RUN = RAMLS_CODE,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #else
   ramfuncs            : LOAD = FLASH_APP,
                         RUN = RAMLS_CODE,
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
   .stack              : > RAMD0 | RAMD1     PAGE = 0		/* local variables,arguments to functions */
   .ebss               : > RAMGS_DATA        PAGE = 1		/* fill=0x02for global and static variables defined */
   .esysmem            : > RAMGS_DATA        PAGE = 1		/* for malloc */

   flashflag_app_start : > FLASH_APP_START,           PAGE = 0
   flashflag_app_end   : > FLASH_APP_END,             PAGE = 0
   flashflag_app_version : > FLASH_APP_VERSION,       PAGE = 0

   user_var_data1      : > RAMGS6L,                   PAGE = 1
   user_var_data2      : > RAMGS6H,                   PAGE = 1
   user_var_data3      : > RAMGS7L,                   PAGE = 1
   user_var_data4      : > RAMGS7H,                   PAGE = 1


   /* Initalized sections go in Flash */
   .econst             : >> FLASH_APP     PAGE = 0, ALIGN(4)
   .switch             : > FLASH_APP      PAGE = 0, ALIGN(4)
   
   .reset              : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */
   
   /* crc/checksum section configured as COPY section to avoid including in executable */
   .TI.memcrc          : type = COPY   
}

/*
//===========================================================================
// End of file.
//===========================================================================
*/

MEMORY
{
    PAGE 0: /* PROGRAM MEMORY (FLASH) */
    BEGIN            : origin = 0x00086000, length = 0x00000002
    VERSION  : origin = 0x00086004,length = 0x00000020   /* 起始4 Word: 软件版本 */
    ROM      : origin = 0x00086024, length = 0x00014FD3  /* 主程序空间 (168KB - 8字节) */

    PAGE 1:   /* DATA MEMORY (RAM) */

    RAMM0_1          : origin = 0x00000000 length = 0x00000800

    RAMLS0           : origin = 0x00008000, length = 0x00000800
    RAMLS1_2         : origin = 0x00008800, length = 0x00001000
//    RAMLS2           : origin = 0x00009000, length = 0x00000800
    RAMLS3           : origin = 0x00009800, length = 0x00000800
    RAMLS4_5_6           : origin = 0x0000A000, length = 0x00001800
//    RAMLS5           : origin = 0x0000A800, length = 0x00000800
//    RAMLS5           : origin = 0x0000A800, length = 0x00001000
//    RAMLS6           : origin = 0x0000B000, length = 0x00000800
    RAMLS7           : origin = 0x0000B800, length = 0x00000800
//    RAMLS6           : origin = 0x0000B000, length = 0x00001000


    RAMGS0123        : origin = 0x0000C000, length = 0x00003FF8     /* 包括了RAMGS0 RAMGS1 RAMGS2 RAMGS3 */

    CLA1_MSGRAMLOW   : origin = 0x001480, length = 0x000080
    CLA1_MSGRAMHIGH  : origin = 0x001500, length = 0x000080
//      RAMGS0           : origin = 0x0000C000, length = 0x00001000
//      RAMGS1           : origin = 0x0000D000, length = 0x00001000
//      RAMGS2           : origin = 0x0000E000, length = 0x00001FF8     /* 包括了RAMGS2和RAMGS3 */

}


SECTIONS
{
//----------------------------------------------------------------------

    /*===== 程序段分配 (PAGE 0 - FLASH) =====*/
    codestart        : > BEGIN, PAGE = 0, ALIGN(4)              /* 启动代码 */
    version         : >  VERSION, PAGE = 0               /* 软件版本段 */
    .cinit          : >  ROM, PAGE = 0, ALIGN(4)     /* C初始化表 */
    .pinit          : >  ROM, PAGE = 0, ALIGN(4)     /* C++初始化表 */
    .init_array     : >  ROM, PAGE = 0, ALIGN(4)     /* C++初始化表 */
    .const          : >  ROM, PAGE = 0, ALIGN(4)     /* 常量数据 */
    .switch         : >  ROM, PAGE = 0, ALIGN(4)     /* Switch跳转表 */
    .crc            : >  ROM, PAGE = 0               /* CRC校验值 */
    .text           : >  ROM, PAGE = 0, ALIGN(4)     /* 可执行代码 */


    /*===== 数据段分配 (PAGE 1 - RAM) =====*/
    .bss >> RAMGS0123, PAGE = 1, ALIGN(4)    /* 未初始化全局变量 */
    .data >> RAMGS0123, PAGE = 1, ALIGN(4)   /* 已初始化全局变量 */
    .stack > RAMM0_1, PAGE = 1, ALIGN(4)                                        /* 系统堆栈 */
    .sysmem >> RAMGS0123, PAGE = 1, ALIGN(4)                                    /* 动态内存池 */

    /*===== 特殊段处理 =====*/
    .cio : {} > RAMGS0123 , ALIGN(4)        /* C I/O缓冲区 (自动对齐) */
    .output : {} > RAMGS0123 , ALIGN(4)     /* C I/O缓冲区 (自动对齐) */

    GROUP
    {
		.TI.ramfunc

    }   LOAD = ROM,
        RUN = RAMLS3,
        LOAD_START(RamfuncsLoadStart),
        LOAD_SIZE(RamfuncsLoadSize),
        RUN_START(RamfuncsRunStart),
        ALIGN(4)

    GROUP
    {
		.ISR.ramfunc

    }   LOAD = ROM,
        RUN = RAMLS1_2,
        LOAD_START(RamfuncsISRLoadStart),
        LOAD_SIZE(RamfuncsISRLoadSize),
        RUN_START(RamfuncsISRRunStart),
        ALIGN(4)

    GROUP
    {
       //.text:Fapi_*

       //.text:Fapi_doVerify
       //.text:Fapi_flushPipeline
       //.text:Fapi_issueAsyncCommand
       //.text:Fapi_setActiveFlashBank
       //.text:Fapi_initializeAPI
       //.text:Fapi_getFsmStatus
		//.text:Fapi_issueProgrammingCommand
		//       .text:Fapi_issueAsyncCommand
       //.text:.Fapi_issueAsyncCommandWithAddress
       //.text:Fapi_checkFsmForReady
	   //.text:Fapi_doBlankCheck

    }   LOAD = ROM,
        RUN = RAMLS4_5_6,
        LOAD_START(FlashAPILoadStart),
        LOAD_SIZE(FlashAPILoadSize),
        RUN_START(FlashAPIRunStart),
        ALIGN(4)

	CLA1mathTables :  LOAD = ROM,
                      RUN = RAMLS7,                 // 运行时在RAMLS7
                	  LOAD_START(Cla1mathTablesLoadStart),
					  LOAD_END(Cla1mathTablesLoadEnd),
             	  	  LOAD_SIZE(Cla1mathTablesLoadSize),
                	  RUN_START(Cla1mathTablesRunStart),
                      ALIGN(4)

    /* ===== 添加自动创建段的处理 ===== */
    hwbist : {} > RAMGS0123, type = NOLOAD
    hwbiststack : {} > RAMGS0123, type = NOLOAD

    // CLA和CPU之间的通信内存
    Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW
    CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH

   .scratchpad      : > RAMLS7      /* CLA临时工作内存 */
   .bss_cla         : > RAMLS7      /* CLA专用的未初始化数据段 */

    Cla1DataRam      : > RAMLS7     /* CLA和CPU之间的共享内存 */

    /* CLA代码运行区 */
    Cla1Prog        : LOAD = ROM,
                      RUN = RAMLS4_5_6,
                      LOAD_START(Cla1ProgLoadStart),
                      RUN_START(Cla1ProgRunStart),
                      LOAD_SIZE(Cla1ProgLoadSize),
                      ALIGN(4)

    /* CLA常量运行区 */
    .const_cla      : LOAD = ROM,
                      RUN = RAMLS4_5_6,
                      RUN_START(Cla1ConstRunStart),
                      LOAD_START(Cla1ConstLoadStart),
                      LOAD_SIZE(Cla1ConstLoadSize),
                      ALIGN(4)

}


#if 0

MEMORY
{
    BEGIN            : origin = 0x00080000, length = 0x00000002
   //BEGIN            : origin = 0x00085200, length = 0x00000002
//   BOOT_RSVD        : origin = 0x00000002, length = 0x00000126

    RAMM0_1            : origin = 0x00000000 length = 0x00000800

    RAMLS0           : origin = 0x00008000, length = 0x00000800
    RAMLS1           : origin = 0x00008800, length = 0x00000800
    RAMLS2           : origin = 0x00009000, length = 0x00000800
    RAMLS3           : origin = 0x00009800, length = 0x00000800
    RAMLS4           : origin = 0x0000A000, length = 0x00000800
    RAMLS5           : origin = 0x0000A800, length = 0x00000800

//  CLARam总计12KWord
//  CLAProgRam       : origin = 0x00008000, length = 0x00003000
    RAMLS6           : origin = 0x0000B000, length = 0x00000800
    RAMLS7           : origin = 0x0000B800, length = 0x00000800

    CLA1_MSGRAMLOW   : origin = 0x001480, length = 0x000080
    CLA1_MSGRAMHIGH  : origin = 0x001500, length = 0x000080



   RAMGS0           : origin = 0x0000C000, length = 0x00001000
   RAMGS1           : origin = 0x0000D000, length = 0x00001000
   RAMGS2           : origin = 0x0000E000, length = 0x00001FF8
   // RAMGS3_RSVD      : origin = 0x0000FFF8, length = 0x00000008 /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

   BOOTROM          : origin = 0x003F8000, length = 0x00007FC0
   SECURE_ROM       : origin = 0x003F2000, length = 0x00006000

   RESET            : origin = 0x003FFFC0, length = 0x00000002
//------------------------------------------------------------
   /* Flash sectors */
   /* BANK 0 */
   // ==>BOOTLOADER(5��BANKԤ������)
   OTA_ID              : origin = 0x084000, length = 0x000002

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>APP>>>>>>>>>>>>>>>>>>>>>>>>>>>
   APP_DATA          : origin = 0x085000, length = 0x000200
   //APP_STARTFLAG     : origin = 0x085200, length = 0x000002
   APP_INI_CONST_SWTICH : origin = 0x085202, length = 0x000DFE

   APP_PROG_COMMON   : origin = 0x086000, length = (0x013000-0x000A)

   // ���������������Ϊ������־��������CRC32��
   APP_ENDFLAG         : origin = 0x098FF6, length = 0x000002

   BAK_DATA          : origin = 0x099000, length = 0x000200
   //BAK_STARTFLAG     : origin = 0x099200, length = 0x000002
   BAK_INI_CONST_SWTICH : origin = 0x099202, length = 0x000DFE

   // ���������������Ϊ������־��������CRC32��
   BAK_ENDFLAG         : origin = 0x0ACFF6, length = 0x000002
   //--------------------------------------------------------
   // Ԥ������
   FLASH_BANK2_SEC13 : origin = 0x0AD000, length = 0x001000
   FLASH_BANK2_SEC14 : origin = 0x0AE000, length = 0x001000
   FLASH_BANK2_SEC15 : origin = 0x0AF000, length = 0x000FF0

// FLASH_BANK0_SEC15_RSVD     : origin = 0x0AFFF0, length = 0x000010  /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */


}

SECTIONS
{
//----------------------------------------------------------------------
// flash prog sections
   .cinit           : > APP_INI_CONST_SWTICH, ALIGN(4)
   .text            : >>APP_PROG_COMMON, ALIGN(4)
   codestart        : > BEGIN, ALIGN(4)

   .stack           : > RAMM0_1
   .switch          : > APP_INI_CONST_SWTICH, ALIGN(4)

   otaFlag          : > OTA_ID,     PAGE = 0, ALIGN(4)
   data             : > APP_DATA    PAGE = 0

    //ClaProgFlash    : >>FLASH_BANK1_SEC0 | FLASH_BANK1_SEC1 | FLASH_BANK1_SEC2 | FLASH_BANK1_SEC3, ALIGN(4)
//-----------------------------------------------------------------------
// variable value section
    //CLAProgRam      :>>RAMLS0 | RAMLS1 | RAMLS2 | RAMLS3 | RAMLS4 | RAMLS5
#if defined(__TI_EABI__)
   .init_array      : > APP_INI_CONST_SWTICH,       ALIGN(4)
   //.bss             : >>RAMGS2 | RAMGS3 | RAMGS1
   .bss             : >>RAMGS2 | RAMGS1
   .bss:output      : > RAMGS2
   .bss:cio         : > RAMGS2
   .data            : >> RAMGS2 | RAMLS0
   .sysmem          : > RAMGS2
   .const           : > APP_INI_CONST_SWTICH       ALIGN(4)
#else
   .pinit           : > APP_INI_CONST_SWTICH       ALIGN(4)
   .ebss            : >>RAMGS2
   .esysmem         : > RAMGS2
   .cio             : > RAMGS2
   .econst          : > APP_INI_CONST_SWTICH
#endif

   ramgs0           : > RAMGS0
   ramgs1           : > RAMGS1

   .reset           : > RESET,          TYPE = DSECT /* not used, */

   dclfuncs         : > APP_PROG_COMMON,     ALIGN(4)

#if defined(__TI_EABI__)

//   .TI.ramfunc      : LOAD = APP_PROG_COMMON,
   GROUP
   {
       .TI.ramfunc
       isrcodefuncs
       dclfuncs
   } LOAD = APP_PROG_COMMON,
                      RUN = RAMGS0,
                      LOAD_START(RamfuncsLoadStart),
                      LOAD_SIZE(RamfuncsLoadSize),
                      LOAD_END(RamfuncsLoadEnd),
                      RUN_START(RamfuncsRunStart),
                      RUN_SIZE(RamfuncsRunSize),
                      RUN_END(RamfuncsRunEnd),
                      ALIGN(4)
#else
   .TI.ramfunc      : LOAD = APP_PROG_COMMON,
                      RUN = RAMGS0
                      LOAD_START(_RamfuncsLoadStart),
                      LOAD_SIZE(_RamfuncsLoadSize),
                      LOAD_END(_RamfuncsLoadEnd),
                      RUN_START(_RamfuncsRunStart),
                      RUN_SIZE(_RamfuncsRunSize),
                      RUN_END(_RamfuncsRunEnd),
                      ALIGN(4)
#endif
//---------------------------------------------------------------------
// FLASHAPIǨ����RAM
    GROUP
   {
       .text:Fapi_setActiveFlashBank
       .text:Fapi_setupFlashStateMachine
       .text:Fapi_issueProgrammingCommand
       .text:Fapi_issueAsyncCommandWithAddress
       .text:Fapi_configureFMAC
       .text:Fapi_issueFsmCommand
       .text:Fapi_initializeAPI
       .text:Fapi_setupBankSectorEnable
       .text:Fapi_calculateFletcherChecksum
       .text:Fapi_divideUnsignedLong
       .text:Fapi_setupSectorsForWrite
       .text:Fapi_WriteReadBack
       .text:Fapi_checkFsmForReady
       .text:Fapi_calculateOtpChecksum
       .text:Fapi_scaleCycleValues
       .text:Fapi_getFsmStatus
   } LOAD = APP_PROG_COMMON,
     RUN = RAMGS1,
     LOAD_START(FlashAPILoadStart),
     LOAD_SIZE(FlashAPILoadSize),
     LOAD_END(FlashAPILoadEnd),
     RUN_START(FlashAPIRunStart),
     RUN_SIZE(FlashAPIRunSize),
     RUN_END(FlashAPIRunEnd),
     ALIGN(4)
//---------------------------------------------------------------------
// CLA����ڴ滮��
#if defined(__TI_EABI__)
    /* CLA specific sections */
    Cla1Prog        : LOAD = APP_PROG_COMMON,
                      RUN = RAMLS6,
                      LOAD_START(Cla1ProgLoadStart),
                      RUN_START(Cla1ProgRunStart),
                      LOAD_SIZE(Cla1ProgLoadSize),
                      ALIGN(4)
#else
    /* CLA specific sections */
    Cla1Prog        : LOAD = APP_PROG_COMMON,
                      RUN = RAMLS6,
                      LOAD_START(_Cla1ProgLoadStart),
                      RUN_START(_Cla1ProgRunStart),
                      LOAD_SIZE(_Cla1ProgLoadSize),
                      ALIGN(4)
#endif


    Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW
    CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH

   .scratchpad      : > RAMLS7
   .bss_cla         : > RAMLS7

   Cla1DataRam      : > RAMLS7

#if defined(__TI_EABI__)
 .const_cla      : LOAD = APP_INI_CONST_SWTICH,
                      RUN = RAMLS6,
                      RUN_START(Cla1ConstRunStart),
                      LOAD_START(Cla1ConstLoadStart),
                      LOAD_SIZE(Cla1ConstLoadSize),
                      ALIGN(4)
#else
.const_cla      : LOAD = APP_INI_CONST_SWTICH,
                      RUN = RAMLS6,
                      RUN_START(_Cla1ConstRunStart),
                      LOAD_START(_Cla1ConstLoadStart),
                      LOAD_SIZE(_Cla1ConstLoadSize),
                      ALIGN(4)
#endif
}

#endif


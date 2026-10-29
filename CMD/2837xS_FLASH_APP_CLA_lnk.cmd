/******************************************************************************
 * @file    2837xS_FLASH_APP_CLA_lnk.cmd
 * @brief   TMS320F28374S APP 工程链接命令文件（OTA 分区 + CLA）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-10-21
 *
 * @note    OTA 分区表（F28374S 256KB Flash，扇区 A~N）：
 *          区域    扇区           地址范围              大小     用途
 *          BOOT    A + B + C      0x080000~0x085FFF     24KB     boot 工程（本工程不链接）
 *          APP     D + E+F+G      0x086000~0x09FFFF    104KB    本工程，codestart = 0x086000
 *          BACKUP  H + I + J + K  0x0A0000~0x0B9FFF    104KB    OTA 升级包备份区
 *          SPARE   L + M          0x0BA000~0x0BDFFF     16KB    预留，暂不使用
 *          CFG     N              0x0BE000~0x0BFFEF   8176B    升级配置区（升级命令/包类型/长度/CRC）
 *
 * @note    扇区粒度：A~D、K~N 为 8KB；E~J 为 32KB。
 *          - APP  区 = D(8KB) + E(32KB) + F(32KB) + G(32KB) = 104KB
 *          - BACKUP 区 = H(32KB) + I(32KB) + J(32KB) + K(8KB) = 104KB
 *          两者总容量相同，但扇区划分不同：备份/回拷时必须在代码里分别维护
 *          两张扇区表（APP: D,E,F,G / BACKUP: H,I,J,K），不能假设扇区一一对应。
 *
 * @note    N 区尾部 0x0BFFF0~0x0BFFFF 为 TI errata（预取越界）保留地址，
 *          配置区实际可用 0x0BE000~0x0BFFEF（8176 字节）。
 *
 * @note    CLA 空间预留：
 *          - Cla1Prog   : LOAD = FLASH_APP（占用 APP 区 Flash），RUN = RAMLS0~RAMLS3
 *          - .const_cla : LOAD = FLASH_APP，RUN = RAMLS4
 *          - CLA 数据   : RAMLS5
 *          - F2837xS 内置 TMU，不需要加载 CLA1mathTables
 *
 * @note    内存对齐：代码段 ALIGN(8)（Flash 预取单元优化），数据段 ALIGN(4)。
 *
 * @note    本文件基于 EP5K_V4.0_0514 BMS debug/cmd/28374S_FLASH_CLA_lnk.cmd (v1.1)
 *          改造：重定 OTA 分区映射，APP codestart 由 0x080000 移到 0x086000。
 ******************************************************************************/
// CLA_C 宏定义（由工程 --define 传入，使用 CLA C 编译器时打开）
#ifdef CLA_C
    CLA_SCRATCHPAD_SIZE = 0x100;
    --undef_sym=__cla_scratchpad_end
    --undef_sym=__cla_scratchpad_start
#endif
// 内存映射定义
MEMORY
{
    PAGE 0:
    /* ---- OTA 分区：本工程只分配 APP 区，其余区域仅声明，防止误分配 ---- */
    BEGIN             : origin = 0x086000, length = 0x000002      /* APP codestart（D 区首字）*/
    FLASH_APP         : origin = 0x086002, length = 0x00019FFE    /* D+E+F+G 共 104KB，扣掉 codestart 2 字 */

    FLASH_BOOT_RSVD   : origin = 0x080000, length = 0x00006000    /* A+B+C  24KB  : boot 工程专用 */
    FLASH_BACKUP_RSVD : origin = 0x0A0000, length = 0x0001A000    /* H+I+J+K 104KB : OTA 备份区 */
    FLASH_SPARE_RSVD  : origin = 0x0BA000, length = 0x00004000    /* L+M    16KB  : 预留 */
    FLASH_CFG_RSVD    : origin = 0x0BE000, length = 0x00001FF0    /* N      8176B : 升级配置区 */

    RAMM0             : origin = 0x000123, length = 0x0002DD
    RAMD0             : origin = 0x00B000, length = 0x000800

    RAMLS0            : origin = 0x008000, length = 0x000800
    RAMLS1            : origin = 0x008800, length = 0x000800
    RAMLS2            : origin = 0x009000, length = 0x000800
    RAMLS3            : origin = 0x009800, length = 0x000800
    RAMLS4            : origin = 0x00A000, length = 0x000800
    RAMLS5            : origin = 0x00A800, length = 0x000800

    /* RAMGS14 : 0x01A000 已注释 —— F28374S 无此 RAM（TI: Only Available on F28377S/F28375S） */
    /* RAMGS15 : 0x01B000 已注释 —— 同上，此片子上是空洞 */

    RESET             : origin = 0x3FFFC0, length = 0x000002

    PAGE 1:
    BOOT_RSVD         : origin = 0x000002, length = 0x000121
    RAMM1             : origin = 0x000400, length = 0x0003F8

    RAMGS0            : origin = 0x00C000, length = 0x001000
    RAMGS1            : origin = 0x00D000, length = 0x001000
    RAMGS2            : origin = 0x00E000, length = 0x001000
    RAMGS3            : origin = 0x00F000, length = 0x001000
    RAMGS4            : origin = 0x010000, length = 0x001000
    RAMGS5            : origin = 0x011000, length = 0x001000
    RAMGS6            : origin = 0x012000, length = 0x001000
    RAMGS7            : origin = 0x013000, length = 0x001000
    RAMGS8            : origin = 0x014000, length = 0x001000
    RAMGS9            : origin = 0x015000, length = 0x001000
    RAMGS10           : origin = 0x016000, length = 0x001000
    RAMGS11           : origin = 0x017000, length = 0x001000
    /* RAMGS12 : 0x018000 已注释 —— F28374S 的 RAM 只到 RAMGS11(0x017FFF) */
    /* RAMGS13 : 0x019000 已注释 —— 同上 */

    CLA1_MSGRAMLOW    : origin = 0x001480, length = 0x000080
    CLA1_MSGRAMHIGH   : origin = 0x001500, length = 0x000080
}
// 段分配定义
SECTIONS
{
    /* 程序段 (PAGE 0 - FLASH_APP) */
    codestart        : > BEGIN,  PAGE = 0, ALIGN(8)
    .cinit           : > FLASH_APP, PAGE = 0, ALIGN(8)
    .text            : >> FLASH_APP, PAGE = 0, ALIGN(8)
    .switch          : > FLASH_APP, PAGE = 0, ALIGN(8)

#if defined(__TI_EABI__)
    .init_array      : > FLASH_APP, PAGE = 0, ALIGN(8)
    .const           : >> FLASH_APP, PAGE = 0, ALIGN(8)
#else
    .pinit           : > FLASH_APP, PAGE = 0, ALIGN(8)
    .econst          : >> FLASH_APP, PAGE = 0, ALIGN(8)
#endif

    /*  RAM 函数段（FlashAPI / 擦写例程必须搬 RAM 运行）   */
#ifdef __TI_COMPILER_VERSION__
    #if __TI_COMPILER_VERSION__ >= 15009000
        #if defined(__TI_EABI__)
            .TI.ramfunc : {} LOAD = FLASH_APP,
                             RUN = RAMD0,
                             LOAD_START(RamfuncsLoadStart),
                             LOAD_SIZE(RamfuncsLoadSize),
                             RUN_START(RamfuncsRunStart),
                             PAGE = 0, ALIGN(8)
        #else
            .TI.ramfunc : {} LOAD = FLASH_APP,
                             RUN = RAMD0,
                             LOAD_START(_RamfuncsLoadStart),
                             LOAD_SIZE(_RamfuncsLoadSize),
                             RUN_START(_RamfuncsRunStart),
                             PAGE = 0, ALIGN(8)
        #endif
    #else
        ramfuncs     : LOAD = FLASH_APP,
                       RUN = RAMD0,
                       LOAD_START(_RamfuncsLoadStart),
                       LOAD_SIZE(_RamfuncsLoadSize),
                       RUN_START(_RamfuncsRunStart),
                       PAGE = 0, ALIGN(8)
    #endif
#endif

    .reset           : > RESET, PAGE = 0, TYPE = DSECT

    /* 数据段 (PAGE 1 - RAM) */
    .stack           : > RAMM1,  PAGE = 1, ALIGN(4)

#if defined(__TI_EABI__)
    .bss             : >> RAMGS0 | RAMGS1 | RAMGS2 | RAMGS3, PAGE = 1, ALIGN(4)
    .bss:output      : > RAMGS0, PAGE = 1, ALIGN(4)
    .data            : >> RAMGS0 | RAMGS1, PAGE = 1, ALIGN(4)
    .sysmem          : > RAMGS0, PAGE = 1, ALIGN(4)
#else
    .ebss            : >> RAMGS0 | RAMGS1 | RAMGS2 | RAMGS3, PAGE = 1, ALIGN(4)
    .esysmem         : > RAMGS0, PAGE = 1, ALIGN(4)
#endif

    /*  CLA 段  */
#if defined(__TI_EABI__)
    Cla1Prog         : LOAD = FLASH_APP,
                       RUN = RAMLS0 | RAMLS1 | RAMLS2 | RAMLS3,
                       LOAD_START(Cla1ProgLoadStart),
                       RUN_START(Cla1ProgRunStart),
                       LOAD_SIZE(Cla1ProgLoadSize),
                       PAGE = 0, ALIGN(8)

    .const_cla       : LOAD = FLASH_APP,
                       RUN = RAMLS4,
                       RUN_START(Cla1ConstRunStart),
                       LOAD_START(Cla1ConstLoadStart),
                       LOAD_SIZE(Cla1ConstLoadSize),
                       PAGE = 0, ALIGN(8)
#else
    Cla1Prog         : LOAD = FLASH_APP,
                       RUN = RAMLS0 | RAMLS1 | RAMLS2 | RAMLS3,
                       LOAD_START(_Cla1ProgLoadStart),
                       RUN_START(_Cla1ProgRunStart),
                       LOAD_SIZE(_Cla1ProgLoadSize),
                       PAGE = 0, ALIGN(8)

    .const_cla       : LOAD = FLASH_APP,
                       RUN = RAMLS4,
                       RUN_START(_Cla1ConstRunStart),
                       LOAD_START(_Cla1ConstLoadStart),
                       LOAD_SIZE(_Cla1ConstLoadSize),
                       PAGE = 0, ALIGN(8)
#endif

    Cla1DataRam      : > RAMLS5, PAGE = 0, ALIGN(4)

    CpuToCla1MsgRAM  : > CLA1_MSGRAMHIGH, PAGE = 1
    Cla1ToCpuMsgRAM  : > CLA1_MSGRAMLOW,  PAGE = 1

    /*  CLA 暂存段：CLA 编译器为每个函数生成 .scratchpad:Cla1Prog:<func>，
     *  由父段名 .scratchpad 统一收集，必须放在 CLA 可访问的 RAMLS5 中 */
    .scratchpad      : > RAMLS5, PAGE = 0, ALIGN(4)
    .bss_cla         : > RAMLS5, PAGE = 0, ALIGN(4)

    /*  CLA C 编译器专用段  */
#ifdef CLA_C
    CLAscratch       : { *.obj(CLAscratch)
                         . += CLA_SCRATCHPAD_SIZE;
                         *.obj(CLAscratch_end) } > RAMLS5, PAGE = 0
#endif

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/

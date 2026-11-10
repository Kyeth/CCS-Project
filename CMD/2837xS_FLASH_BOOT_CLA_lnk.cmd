/******************************************************************************
 * @file    2837xS_FLASH_BOOT_CLA_lnk.cmd
 * @brief   TMS320F28374S BOOT 工程链接命令文件（最小 Bootloader）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    OTA 分区表（F28374S 256KB Flash，扇区 A~N）：
 *          区域    扇区           地址范围              大小     用途
 *          BOOT    A + B + C      0x080000~0x085FFF     24KB    本工程，codestart = 0x080000
 *          APP     D + E+F+G      0x086000~0x09FFFF    104KB    应用，由 BOOT 跳转/回拷
 *          BACKUP  H + I + J + K  0x0A0000~0x0B9FFF    104KB    OTA 升级包备份区
 *          SPARE   L + M          0x0BA000~0x0BDFFF     16KB    预留，暂不使用
 *          CFG     N              0x0BE000~0x0BFFEF   8176B    升级配置区
 *
 * @note    本文件由 APP 工程的 CMD/2837xS_FLASH_APP_CLA_lnk.cmd 改造：
 *          - codestart 由 0x086000 移到 0x080000，程序段改落 FLASH_BOOT（A+B+C）；
 *          - APP/BACKUP/SPARE/CFG 只声明、不分配，防止 BOOT 误占应用区；
 *          - 去掉 CLA 段（BOOT 不跑 CLA 任务），保留 .TI.ramfunc 与 .flashfunc：
 *            F021 擦写全程在 RAM（RUN = RAMGS3_RUN / RAMD0），必须搬。
 *
 * @note    N 区尾部 0x0BFFF0~0x0BFFFF 为 TI errata（预取越界）保留地址，
 *          配置区实际可用 0x0BE000~0x0BFFEF（8176 字节）。
 ******************************************************************************/
// 内存映射定义
MEMORY
{
    PAGE 0:
    /* ---- OTA 分区：本工程只分配 BOOT 区，其余区域仅声明，防止误分配 ---- */
    BEGIN             : origin = 0x080000, length = 0x000002      /* BOOT codestart（A 区首字）*/
    FLASH_BOOT        : origin = 0x080002, length = 0x00005FFE    /* A+B+C 共 24KB，扣掉 codestart 2 字 */

    FLASH_APP_RSVD    : origin = 0x086000, length = 0x0001A000    /* D+E+F+G 104KB : 应用区 */
    FLASH_BACKUP_RSVD : origin = 0x0A0000, length = 0x0001A000    /* H+I+J+K 104KB : OTA 备份区 */
    FLASH_SPARE_RSVD  : origin = 0x0BA000, length = 0x00004000    /* L+M    16KB  : 预留 */
    FLASH_CFG_RSVD    : origin = 0x0BE000, length = 0x00001FF0    /* N      8176B : 升级配置区 */

    RAMM0             : origin = 0x000123, length = 0x0002DD
    RAMD0             : origin = 0x00B000, length = 0x000800

    /* .flashfunc 的运行区（程序空间别名，与 PAGE 1 的 RAMGS3 是同一块 RAM）。
        PAGE 0 / PAGE 1 是两个地址空间，程序段只能引用 PAGE 0 的区间名。 */
    RAMGS3_RUN        : origin = 0x00F000, length = 0x001000

    RESET             : origin = 0x3FFFC0, length = 0x000002

    PAGE 1:
    BOOT_RSVD         : origin = 0x000002, length = 0x000121
    RAMM1             : origin = 0x000400, length = 0x0003F8

    RAMGS0            : origin = 0x00C000, length = 0x001000
    RAMGS1            : origin = 0x00D000, length = 0x000C00
    /* .portbuf 专用预留区（1024 字），从 RAMGS1 尾部切出 */
    RAMGS1_PORTBUF    : origin = 0x00DC00, length = 0x000400
    RAMGS2            : origin = 0x00E000, length = 0x001000
    RAMGS3            : origin = 0x00F000, length = 0x001000
}
// 段分配定义
SECTIONS
{
    /* 程序段 (PAGE 0 - FLASH_BOOT) */
    codestart        : > BEGIN,  PAGE = 0, ALIGN(8)
    .cinit           : > FLASH_BOOT, PAGE = 0, ALIGN(8)
    .text            : >> FLASH_BOOT, PAGE = 0, ALIGN(8)
    .switch          : > FLASH_BOOT, PAGE = 0, ALIGN(8)

#if defined(__TI_EABI__)
    .init_array      : > FLASH_BOOT, PAGE = 0, ALIGN(8)
    .const           : >> FLASH_BOOT, PAGE = 0, ALIGN(8)
#else
    .pinit           : > FLASH_BOOT, PAGE = 0, ALIGN(8)
    .econst          : >> FLASH_BOOT, PAGE = 0, ALIGN(8)
#endif

    /*  RAM 函数段（FlashAPI / 擦写例程必须搬 RAM 运行）   */
#ifdef __TI_COMPILER_VERSION__
    #if __TI_COMPILER_VERSION__ >= 15009000
        #if defined(__TI_EABI__)
            .TI.ramfunc : {} LOAD = FLASH_BOOT,
                             RUN = RAMD0,
                             LOAD_START(RamfuncsLoadStart),
                             LOAD_SIZE(RamfuncsLoadSize),
                             RUN_START(RamfuncsRunStart),
                             PAGE = 0, ALIGN(8)
        #else
            .TI.ramfunc : {} LOAD = FLASH_BOOT,
                             RUN = RAMD0,
                             LOAD_START(_RamfuncsLoadStart),
                             LOAD_SIZE(_RamfuncsLoadSize),
                             RUN_START(_RamfuncsRunStart),
                             PAGE = 0, ALIGN(8)
        #endif
    #else
        ramfuncs     : LOAD = FLASH_BOOT,
                       RUN = RAMD0,
                       LOAD_START(_RamfuncsLoadStart),
                       LOAD_SIZE(_RamfuncsLoadSize),
                       RUN_START(_RamfuncsRunStart),
                       PAGE = 0, ALIGN(8)
    #endif
#endif

    /* Flash 擦写历程。F021 库与调用链都必须跑在 RAM：FSM 忙的时候
       不能再从仍在编程的 Flash 上取指，也不能进 Flash 里的 ISR。
       库自身没有 .TI.ramfunc 标记（节名是 .text:Fapi_*），所以用 GROUP
       把这些节整体收进 .flashfunc，再 LOAD 到 Flash、RUN 到 RAMGS3。 */
#if defined(__TI_EABI__)
        GROUP
        {
            .flashfunc
            {
                -l F021_API_F2837xS_FPU32_EABI.lib
            }
        } LOAD = FLASH_BOOT,
          RUN  = RAMGS3_RUN,
          LOAD_START(_FlashfuncLoadStart),
          LOAD_SIZE(_FlashfuncLoadSize),
          RUN_START(_FlashfuncRunStart),
          PAGE = 0, ALIGN(8)
#else
        GROUP
        {
            .flashfunc
            {
                -l F021_API_F2837xS_FPU32.lib
            }
        } LOAD = FLASH_BOOT,
          RUN  = RAMGS3_RUN,
          LOAD_START(_FlashfuncLoadStart),
          LOAD_SIZE(_FlashfuncLoadSize),
          RUN_START(_FlashfuncRunStart),
          PAGE = 0, ALIGN(8)
#endif

    .reset           : > RESET, PAGE = 0, TYPE = DSECT

    /* 数据段 (PAGE 1 - RAM) */
    .stack           : > RAMM1,  PAGE = 1, ALIGN(4)

#if defined(__TI_EABI__)
    .bss             : >> RAMGS0 | RAMGS1 | RAMGS2 | RAMGS3, PAGE = 1, ALIGN(4)
    .bss:output      : > RAMGS0, PAGE = 1, ALIGN(4)
    .data            : >> RAMGS0 | RAMGS1, PAGE = 1, ALIGN(4)
    .sysmem          : > RAMGS0, PAGE = 1, ALIGN(4)

    /* SCI TX 环（3 x 128 字 = 0x180 字）落在独立预留区 RAMGS1_PORTBUF */
    .portbuf         : > RAMGS1_PORTBUF, PAGE = 1, ALIGN(4)

    /* 端口层统计、协议缓冲（BOOT 里只有端口层会用到，段保留以便同一套端口层源码） */
    .protobuf        : > RAMGS1, PAGE = 1, ALIGN(4)
    .mbregs          : > RAMGS1, PAGE = 1, ALIGN(4)
    .portstat        : > RAMGS1, PAGE = 1, ALIGN(4)

    /* XMODEM 包缓冲 1030 字（每字节一个 Uint16）+ 64 字余字节缓存。
       RAMGS2 整段 4096 字，放这里不挤 RAMGS0/RAMGS1。 */
    .upybuf          : > RAMGS2, PAGE = 1, ALIGN(4)
#else
    .ebss            : >> RAMGS0 | RAMGS1 | RAMGS2 | RAMGS3, PAGE = 1, ALIGN(4)
    .esysmem         : > RAMGS0, PAGE = 1, ALIGN(4)
#endif
}

/*
//===========================================================================
// End of file.
//===========================================================================
*/

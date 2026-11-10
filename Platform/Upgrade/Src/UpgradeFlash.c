/******************************************************************************
 * @file    UpgradeFlash.c
 * @brief   F021 Flash API 适配层（RAM 执行）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    本文件所有函数都在 .flashfunc 段，链接时与 F021 库一起搬到
 *          RAMGS3 执行。函数体内只允许使用立即数、寄存器与 RAM 变量，
 *          不得调用任何位于 Flash 里的函数（含 RTS 库的除法/取模助手），
 *          否则 FSM 忙时会因为取指被拖住而死等。
 ******************************************************************************/

#include "UpgradeFlash.h"

/* F021 的 Constants.h 用 _F2837xS（前导下划线）选器件寄存器地址，
 * 而本工程的编译选项只定义 F2837xS，这里补上局部定义再包含 F021。 */
#ifndef _F2837xS
#define _F2837xS
#endif

#include "F021.h"
#include "F2837xS_device.h"
#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 段声明 ---------------- */

#pragma CODE_SECTION(UpgradeFlashWaitReady, ".flashfunc");
#pragma CODE_SECTION(UpgradeFlashInit, ".flashfunc");
#pragma CODE_SECTION(UpgradeFlashEraseSector, ".flashfunc");
#pragma CODE_SECTION(UpgradeFlashProgramBlock, ".flashfunc");
#pragma CODE_SECTION(UpgradeFlashReadWords, ".flashfunc");

/* ---------------- 操作表 ---------------- */

const ST_UPGRADE_FLASH_OPS g_stUpgradeFlashOps =
{
    UpgradeFlashEraseSector,
    UpgradeFlashProgramBlock,
    UpgradeFlashReadWords
};

/* ---------------- 内部 ---------------- */

/**
 * @brief  等 FSM 回到就绪态
 * @return 1 就绪，0 超时
 * @note   只有"等 FSM 就绪"用相等判断；忙状态是 Fapi_Status_FsmBusy，
 *         其它非成功返回也一并当失败，交给调用方读状态字。
 */
static Uint16 UpgradeFlashWaitReady(void)
{
    Uint32 u32Guard = UPGRADE_FLASH_WAIT_GUARD;

    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
    {
        if (u32Guard == 0UL)
        {
            return 0U;
        }
        u32Guard--;
    }

    return 1U;
}

/* ---------------- 对外接口 ---------------- */

Uint16 UpgradeFlashInit(void)
{
    if (Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, UPGRADE_FLASH_HCLK_MHZ) != Fapi_Status_Success)
    {
        return 0U;
    }

    if (Fapi_setActiveFlashBank(Fapi_FlashBank0) != Fapi_Status_Success)
    {
        return 0U;
    }

    return 1U;
}

Uint16 UpgradeFlashEraseSector(Uint32 u32Addr)
{
    Uint16 i;
    volatile Uint16 *pu16Flash = (volatile Uint16 *)u32Addr;

    /* 撤销指令写保护只在这条命令前后需要 */
    EALLOW;
    DINT;

    if (Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, (uint32 *)u32Addr) != Fapi_Status_Success)
    {
        EINT;
        EDIS;
        return 0U;
    }

    /* 等待必须留在 RAM 里完成，不能把忙着的 FSM 交回调度器 */
    if (UpgradeFlashWaitReady() == 0U)
    {
        (void)Fapi_issueAsyncCommandWithAddress(Fapi_ClearStatus, (uint32 *)u32Addr);
        EINT;
        EDIS;
        return 0U;
    }

    if (Fapi_getFsmStatus() != 0UL)
    {
        (void)Fapi_issueAsyncCommandWithAddress(Fapi_ClearStatus, (uint32 *)u32Addr);
        EINT;
        EDIS;
        return 0U;
    }

    EINT;
    EDIS;

    /* 回读前 8 字确认为擦除态：擦除步骤自带校验，这里再确认一次 */
    for (i = 0U; i < UPGRADE_FLASH_BLOCK_WORDS; i++)
    {
        if (pu16Flash[i] != 0xFFFFU)
        {
            return 0U;
        }
    }

    return 1U;
}

Uint16 UpgradeFlashProgramBlock(Uint32 u32Addr, const Uint16 *pu16Data, Uint16 u16Words)
{
    Uint16 i;
    volatile Uint16 *pu16Flash = (volatile Uint16 *)u32Addr;

    if ((pu16Data == 0) || (u16Words != UPGRADE_FLASH_BLOCK_WORDS))
    {
        return 0U;
    }

    EALLOW;
    DINT;

    if (Fapi_issueProgrammingCommand((uint32 *)u32Addr,
                                     (uint16 *)pu16Data,
                                     UPGRADE_FLASH_BLOCK_WORDS,
                                     0,
                                     0,
                                     Fapi_AutoEccGeneration) != Fapi_Status_Success)
    {
        EINT;
        EDIS;
        return 0U;
    }

    if (UpgradeFlashWaitReady() == 0U)
    {
        (void)Fapi_issueAsyncCommandWithAddress(Fapi_ClearStatus, (uint32 *)u32Addr);
        EINT;
        EDIS;
        return 0U;
    }

    if (Fapi_getFsmStatus() != 0UL)
    {
        (void)Fapi_issueAsyncCommandWithAddress(Fapi_ClearStatus, (uint32 *)u32Addr);
        EINT;
        EDIS;
        return 0U;
    }

    EINT;
    EDIS;

    /* 回读比对：编程步骤自带校验，这里再确认一次数据真的落进去了 */
    for (i = 0U; i < UPGRADE_FLASH_BLOCK_WORDS; i++)
    {
        if (pu16Flash[i] != pu16Data[i])
        {
            return 0U;
        }
    }

    return 1U;
}

Uint16 UpgradeFlashReadWords(Uint32 u32Addr, Uint16 *pu16Dst, Uint16 u16Words)
{
    Uint16 i;
    volatile Uint16 *pu16Flash = (volatile Uint16 *)u32Addr;

    if ((pu16Dst == 0) || (u16Words == 0U))
    {
        return 0U;
    }

    for (i = 0U; i < u16Words; i++)
    {
        pu16Dst[i] = pu16Flash[i];
    }

    return 1U;
}

#ifdef __cplusplus
}
#endif

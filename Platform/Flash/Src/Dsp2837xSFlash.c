#include "Dsp2837xSFlash.h"

/* 芯片适配：F021 的 F2837xS 分支（F021_CPU0_W0_BASE_ADDRESS）只在定义了
 * _F2837xS 时才可见，而本工程编译只定义 F2837xS。 */
#ifndef _F2837xS
#define _F2837xS
#endif
#include "F021.h"
#include "flash.h"
#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 擦写期间不能从 Flash 取指：本文件所有函数（含喂狗与等待）都放到 .flashfunc，
 * 由启动代码搬到 RAMGS3 运行（见 main.c 的 CopyRamSections）。 */
#pragma CODE_SECTION(Dsp2837xSFlashInit, ".flashfunc")
#pragma CODE_SECTION(FlashFeedWatchDog, ".flashfunc")
#pragma CODE_SECTION(FlashForceReset,   ".flashfunc")
#pragma CODE_SECTION(FlashSectorWords,  ".flashfunc")
#pragma CODE_SECTION(FlashSaveInt,      ".flashfunc")
#pragma CODE_SECTION(FlashRestoreInt,   ".flashfunc")
#pragma CODE_SECTION(FlashWaitReady,    ".flashfunc")
#pragma CODE_SECTION(EraseSingleSector, ".flashfunc")
#pragma CODE_SECTION(FlashProgramWords, ".flashfunc")
#pragma CODE_SECTION(WriteFourWord,     ".flashfunc")
#pragma CODE_SECTION(WriteFlashData,    ".flashfunc")
#pragma CODE_SECTION(ReadFlashData,     ".flashfunc")

/* 一个 64 位编程单元 = 8 个 16 位字；同一单元只能编程一次（ECC 一起写） */
#define FLASH_UNIT_WORDS (8U)
#define FLASH_WAIT_LIMIT (0x1FFFFUL)


/**
 * @brief RAM 内的看门狗喂狗
 * @note  Hw_FeedWatchDog() 链接在 Flash（0x8C1FC），F021 发命令后 Flash 不可取指，
 *        等待循环与 Fapi_serviceWatchdogTimer() 都不能调它，这里直接写 WDKEY。
 */
void FlashFeedWatchDog(void)
{
    EALLOW;
    WdRegs.WDKEY.all = 0x0055U;
    WdRegs.WDKEY.all = 0x00AAU;
    EDIS;
}


/**
 * @brief 擦写等待超时而 FSM 仍忙时的唯一安全出口
 * @note  此时既不能返回 Flash 里的调用者，也不能让看门狗饿死后无提示地复位；
 *        这里打开看门狗到最短周期后不再喂狗，由看门狗复位整机。
 */
void FlashForceReset(void)
{
    EALLOW;
    WdRegs.WDCR.all = 0x0028U;              /* WDDIS=0, WDCHK=101, WDPS=000（最短周期） */
    EDIS;
    WdRegs.WDCNTR.all = 0U;

    while (1)
    {
        /* 故意不喂狗 */
    }
}


/* 返回该地址所在扇区的字长度（用于擦除后的空片检查） */
static Uint16 FlashSectorWords(Uint32 u32Addr)
{
    if (u32Addr < SectorE_start)
    {
        return SectorSmall_u16length;       /* A~D */
    }
    if (u32Addr < SectorH_start)
    {
        return SectorLarge_u16length;       /* E~G */
    }
    if (u32Addr < SectorK_start)
    {
        return SectorLarge_u16length;       /* H~J */
    }
    return SectorSmall_u16length;           /* K~N */
}


/* 擦写期间保存并屏蔽中断：INTM 置位后任何 Flash 里的 ISR 都不会被取指 */
Uint16 FlashSaveInt(void)
{
    Uint16 u16Ier = (Uint16)IER;

    DINT;
    return u16Ier;
}


void FlashRestoreInt(Uint16 u16Ier)
{
    IER = u16Ier;
    EINT;
}


/* 等待 FSM 空闲；超时则从 RAM 强制复位（不返回） */
static void FlashWaitReady(void)
{
    Uint32 u32TimeOutCnt = 0;

    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
    {
        FlashFeedWatchDog();
        u32TimeOutCnt++;
        if (u32TimeOutCnt >= FLASH_WAIT_LIMIT)
        {
            FlashForceReset();
        }
    }
}


int16 Dsp2837xSFlashInit(void)
{
    /* 芯片适配：F28374S 只有一个 Flash 块，BOOT/APP/BACKUP/CFG 全在 Bank0；
     * 参考工程（F28003x）选了 Bank0/Bank1 两个块，这里删掉块选择。 */
    Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, FLASH_HCLK_MHZ);
    FLASH_DELAY_CONFIG;

    Flash_disablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    if (Fapi_setActiveFlashBank(Fapi_FlashBank0) != Fapi_Status_Success)
    {
        return -1;
    }
    FLASH_DELAY_CONFIG;

    Flash_enablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    if (Fapi_issueAsyncCommand(Fapi_ClearMore) != Fapi_Status_Success)
    {
        return -2;
    }

    return 0;
}


int16 EraseSingleSector(Uint32 u32SectorAddr)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType oFlashStatus;
    Uint16 u16Ier = 0;

    u16Ier = FlashSaveInt();

    oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                       (Uint32 *)u32SectorAddr);

    if (oReturnCheck != Fapi_Status_Success)
    {
        FlashRestoreInt(u16Ier);
        return -1;
    }

    FlashWaitReady();

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0)
    {
        (void)Fapi_issueAsyncCommand(Fapi_ClearStatus);
        FLASH_DELAY_CONFIG;
        FlashRestoreInt(u16Ier);
        return -2;
    }

    /* 必要适配修正：Fapi_doBlankCheck() 的长度单位是 32 位字，原实现传的是
     * 16 位字数，检查范围扩大一倍（擦 H 会顺带查 I，擦 N 会查到 Flash 之外）。
     * 参考位置：Dsp280039Flash.c EraseSingleSector()。 */
    oReturnCheck = Fapi_doBlankCheck((Uint32 *)u32SectorAddr,
                   (Uint32)(FlashSectorWords(u32SectorAddr) / 2UL),
                   &oFlashStatusWord);

    FlashRestoreInt(u16Ier);

    if (oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }

    return 0;
}


/**
 * @brief 编程 u16Words 个 16 位字（8 或 4 个，按 64 位单元对齐）
 * @note  F021 发命令后不等待，必须在 RAM 里等 FSM 就绪再返回，
 *        否则回到 Flash 里运行的调度器会在 FSM 忙时取指。
 */
static int16 FlashProgramWords(Uint32 u32Addr, Uint16 *pu16Data, Uint16 u16Words)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType oFlashStatus;
    Uint16 u16Ier = 0;

    if ((pu16Data == 0) || (u16Words == 0U) || (u16Words > FLASH_UNIT_WORDS))
    {
        return -4;
    }

    u16Ier = FlashSaveInt();

    oReturnCheck = Fapi_issueProgrammingCommand((Uint32 *)u32Addr,
                   pu16Data,
                   u16Words,
                   0,
                   0,
                   Fapi_AutoEccGeneration);

    if (oReturnCheck != Fapi_Status_Success)
    {
        FlashRestoreInt(u16Ier);
        return -1;
    }

    FlashWaitReady();

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0)
    {
        (void)Fapi_issueAsyncCommand(Fapi_ClearStatus);
        FLASH_DELAY_CONFIG;
        FlashRestoreInt(u16Ier);
        return -2;
    }

    Fapi_flushPipeline();

    oReturnCheck = Fapi_doVerify((Uint32 *)u32Addr,
                                 (Uint32)(u16Words / 2U),
                                 (Uint32 *)pu16Data,
                                 &oFlashStatusWord);

    FlashRestoreInt(u16Ier);

    if (oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }

    return 0;
}


/**
 * @brief 保持参考工程的接口：一次写 4 个字（= 一个 64 位编程单元）
 * @note  整片写入走 WriteFlashData 的 8 字块；本接口仅供单点调试。
 */
int16 WriteFourWord(Uint32 u32TagAddr, Uint16 *pu16DataBuff)
{
    return FlashProgramWords(u32TagAddr, pu16DataBuff, 4U);
}


int16 WriteFlashData(Uint32 u32TagAddr, void *pu16DataBuff, Uint32 u32DataLen)
{
    Uint32 u32Index = 0;
    Uint32 u32Chunk = 0;
    Uint16 *pu16Src = (Uint16 *)pu16DataBuff;
    Uint16 au16Stage[FLASH_UNIT_WORDS];
    Uint16 u16Idx = 0;
    int16 i16Ret = 0;

    if ((pu16DataBuff == 0) || (u32DataLen == 0U))
    {
        return -1;
    }

    /* 必要适配修正（两条）：
     * 1) 按 8 字（长）整块两两配对齐写入，块内不再二次编程同一单元。
     * 2) 长度不是 8 的倍数时，尾部不足部分用 0xFFFF 补齐，且不越界读源数据。 */
    for (u32Index = 0; u32Index < u32DataLen; u32Index += FLASH_UNIT_WORDS)
    {
        u32Chunk = u32DataLen - u32Index;
        if (u32Chunk >= FLASH_UNIT_WORDS)
        {
            i16Ret = FlashProgramWords(u32TagAddr + u32Index,
                                       pu16Src + u32Index,
                                       FLASH_UNIT_WORDS);
            if (i16Ret != 0)
            {
                return i16Ret;
            }
            continue;
        }

        for (u16Idx = 0U; u16Idx < FLASH_UNIT_WORDS; u16Idx++)
        {
            au16Stage[u16Idx] = (u16Idx < u32Chunk) ? pu16Src[u32Index + u16Idx]
                                                    : 0xFFFFU;
        }

        i16Ret = FlashProgramWords(u32TagAddr + u32Index, &au16Stage[0],
                                   FLASH_UNIT_WORDS);
        if (i16Ret != 0)
        {
            return i16Ret;
        }
    }

    return 0;
}


int16 ReadFlashData(Uint32 u32TagAddr, Uint16 *pu16DataBuff, Uint32 u32DataLen)
{
    Uint32 u32Index = 0;
    volatile Uint16 *pu16Src = (volatile Uint16 *)u32TagAddr;

    if (pu16DataBuff == 0)
    {
        return -1;
    }

    /* 保留逐字 volatile 读：语义与参考的 memcpy 等价（C28x 上一个字节就是一个
     * 16 位字，memcpy 长度按本机字节计），这里显式逐字读，避免依赖库实现。 */
    for (u32Index = 0U; u32Index < u32DataLen; u32Index++)
    {
        pu16DataBuff[u32Index] = pu16Src[u32Index];
    }

    return 0;
}


#ifdef __cplusplus
}
#endif

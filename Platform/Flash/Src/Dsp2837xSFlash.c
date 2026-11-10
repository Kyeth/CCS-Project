#include "Dsp2837xSFlash.h"

/* 芯片适配：F021 的 F2837xS 分支（F021_CPU0_W0_BASE_ADDRESS）只在定义了
 * _F2837xS 时才可见，而本工程编译只定义 F2837xS。 */
#ifndef _F2837xS
#define _F2837xS
#endif
#include "F021.h"
#include "flash.h"
#include "Hw_WatchDog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 擦写期间不能从 Flash 取指：本文件所有函数都放到 .flashfunc，由启动代码
 * 搬到 RAMGS3 运行（见 main.c 的 CopyRamSections）。 */
#pragma CODE_SECTION(Dsp2837xSFlashInit, ".flashfunc")
#pragma CODE_SECTION(FlashSelectBank,    ".flashfunc")
#pragma CODE_SECTION(FlashSectorWords,   ".flashfunc")
#pragma CODE_SECTION(EraseSingleSector,  ".flashfunc")
#pragma CODE_SECTION(FlashProgramWords,  ".flashfunc")
#pragma CODE_SECTION(WriteFourWord,      ".flashfunc")
#pragma CODE_SECTION(WriteFlashData,     ".flashfunc")
#pragma CODE_SECTION(ReadFlashData,      ".flashfunc")

/* 一个 64 位编程单元 = 8 个 16 位字；同一单元只能编程一次（ECC 一起写） */
#define FLASH_UNIT_WORDS (8U)
#define FLASH_WAIT_LIMIT (0x1FFFFUL)


/* 按地址选活动块：本工程会写 Bank0（BOOT/APP）与 Bank1（备份区/配置区） */
static void FlashSelectBank(Uint32 u32Addr)
{
    if (u32Addr < 0xA0000U)
    {
        Fapi_setActiveFlashBank(Fapi_FlashBank0);
    }
    else if (u32Addr < 0xC0000U)
    {
        Fapi_setActiveFlashBank(Fapi_FlashBank1);
    }
    else
    {
        Fapi_setActiveFlashBank(Fapi_FlashBank2);
    }
    FLASH_DELAY_CONFIG;
}


/* 返回该地址所在扇区的字长度（用于擦除后的空片检查） */
static Uint16 FlashSectorWords(Uint32 u32Addr)
{
    if (u32Addr < 0x88000U)
    {
        return SectorSmall_u16length;      /* A~D */
    }
    if (u32Addr < 0xA0000U)
    {
        return SectorLarge_u16length;      /* E~G */
    }
    if (u32Addr < 0xB8000U)
    {
        return SectorLarge_u16length;      /* H~J */
    }
    return SectorSmall_u16length;          /* K~N */
}


/* 等待 FSM 空闲；超时返回 0 */
static Uint16 FlashWaitReady(void)
{
    Uint32 u32TimeOutCnt = 0;

    while ((Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
            && (++u32TimeOutCnt < FLASH_WAIT_LIMIT))
    {
        Hw_FeedWatchDog();
    }

    return (u32TimeOutCnt < FLASH_WAIT_LIMIT) ? 1U : 0U;
}


void Dsp2837xSFlashInit(void)
{
    // 初始化API接口的时钟，单位为Mhz，需要与系统时钟保持一致
    Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, FLASH_HCLK_MHZ);
    FLASH_DELAY_CONFIG;

    // 关闭指令预取
    Flash_disablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    // 设置块为活动
    Fapi_setActiveFlashBank(Fapi_FlashBank0);
    FLASH_DELAY_CONFIG;

    Fapi_setActiveFlashBank(Fapi_FlashBank1);
    FLASH_DELAY_CONFIG;

    Flash_enablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    (void)Fapi_issueAsyncCommand(Fapi_ClearMore);
}


int16 EraseSingleSector(Uint32 u32SectorAddr)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType oFlashStatus;

    FlashSelectBank(u32SectorAddr);

    oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                       (Uint32 *)u32SectorAddr);

    if ((oReturnCheck != Fapi_Status_Success) || (FlashWaitReady() == 0U))
    {
        return -1;
    }

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0)
    {
        (void)Fapi_issueAsyncCommand(Fapi_ClearStatus);
        FLASH_DELAY_CONFIG;
        return -2;
    }

    /* 必要缺陷修复：参考工程这里固定检查 0x1000 字，32K 扇区只查了八分之一；
     * 改成按该地址所在扇区的真实长度检查。 */
    oReturnCheck = Fapi_doBlankCheck((Uint32 *)u32SectorAddr,
                   (Uint32)FlashSectorWords(u32SectorAddr),
                   &oFlashStatusWord);

    if (oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }

    return 0;
}


/**
 * @brief 编程 u16Words 个 16 位字（u16Words 必须为 8 且按 64 位单元对齐）
 * @note  F021 发命令后不等待，必须在 RAM 里等 FSM 就绪再返回，
 *        否则回到 Flash 里运行的调度器会在 FSM 忙时取指。
 */
static int16 FlashProgramWords(Uint32 u32Addr, Uint16 *pu16Data, Uint16 u16Words)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType oFlashStatus;

    if ((pu16Data == 0) || (u16Words == 0U) || (u16Words > FLASH_UNIT_WORDS))
    {
        return -4;
    }

    FlashSelectBank(u32Addr);

    oReturnCheck = Fapi_issueProgrammingCommand((Uint32 *)u32Addr,
                   pu16Data,
                   u16Words,
                   0,
                   0,
                   Fapi_AutoEccGeneration);

    if ((oReturnCheck != Fapi_Status_Success) || (FlashWaitReady() == 0U))
    {
        return -1;
    }

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0)
    {
        (void)Fapi_issueAsyncCommand(Fapi_ClearStatus);
        FLASH_DELAY_CONFIG;
        return -2;
    }

    Fapi_flushPipeline();

    oReturnCheck = Fapi_doVerify((Uint32 *)u32Addr,
                                 (Uint32)(u16Words / 2U),
                                 (Uint32 *)pu16Data,
                                 &oFlashStatusWord);

    if (oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }

    return 0;
}


/**
 * @brief 保持参考工程的接口：一次写 4 个字
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

    /* 必要缺陷修复（两条）：
     * 1) 参考工程按 i += 4 每次只编程 4 个字，同一 64 位编程单元会被写两次
     *    （ECC 也写两次），F2837xS 上会报编程错误或写坏。这里按 8 字对齐整块写。
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

    /* 必要缺陷修复：参考工程写成 memcpy(pu16DataBuff, u32TagAddr, u32DataLen)，
     * 把字地址当指针、长度按字节，实际只拷了一半字数。这里按字地址逐字读。 */
    for (u32Index = 0U; u32Index < u32DataLen; u32Index++)
    {
        pu16DataBuff[u32Index] = pu16Src[u32Index];
    }

    return 0;
}


#ifdef __cplusplus
}
#endif

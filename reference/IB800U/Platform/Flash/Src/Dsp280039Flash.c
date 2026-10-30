#include "Dsp280039Flash.h"
#include "Device.h"
#include "F021.h"
#include "flash.h"
#include "Hw_GpioDriver.h"
#include "Hw_WatchDog.h"
#include "PlatformManager.h"

#ifdef __cplusplus
extern "C" {
#endif



void Dsp280039FlashInit(void)
{
    // 初始化API接口的时钟，单位为Mhz，需要与系统时钟保持一致
    Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, DEVICE_SYSCLK_FREQ/1000000U);
    FLASH_DELAY_CONFIG;

    // 关闭指令预取
    Flash_disablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    // 设置块为活动
    Fapi_setActiveFlashBank(Fapi_FlashBank0);
    FLASH_DELAY_CONFIG;

    Fapi_setActiveFlashBank(Fapi_FlashBank1);
    FLASH_DELAY_CONFIG;

    Fapi_setActiveFlashBank(Fapi_FlashBank2);
    FLASH_DELAY_CONFIG;

//    Flash_EnablePrefetch_SW_Workaround(FLASH0CTRL_BASE);
    Flash_enablePrefetch(FLASH0CTRL_BASE);
    FLASH_DELAY_CONFIG;

    Fapi_issueAsyncCommand(Fapi_ClearMore);
}

Uint32 u32Flag1 = 0xFEFE;
Uint32 u32Flag2 = 0xFEFE;
Uint32 u32Flag3 = 0xFEFE;
Uint32 u32Flag4 = 0xFEFE;

int16 EraseSingleSector(Uint32 u32SectorAddr)
{
    Uint32 u32TimeOutCnt = 0;
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType  oFlashStatus;

    oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                       (Uint32 *)u32SectorAddr);

    while ((Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
            && (++u32TimeOutCnt < 0x1FFFF))
    {
        Hw_FeedWatchDog();
    }
//    while ((Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
//            );

    if ((oReturnCheck != Fapi_Status_Success) || (u32TimeOutCnt >= 0x1FFFF))
    {
        return -1;
    }

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0)
    {
        return -2;
    }

    oReturnCheck = Fapi_doBlankCheck((uint32 *)u32SectorAddr,
                   0x1000,  // 0x1000是扇区的大小
                   &oFlashStatusWord);

    if (oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }
    return 0;
}


int16 WriteFourWord(Uint32 u32TagAddr, Uint16 *pu16DataBuff)
{
    Uint32 u32TimeOutCnt = 0;
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusWordType oFlashStatusWord;
    Fapi_FlashStatusType  oFlashStatus;

    oReturnCheck = Fapi_issueProgrammingCommand((Uint32 *)u32TagAddr,
                   pu16DataBuff,
                   4,
                   0,
                   0,
                   Fapi_AutoEccGeneration);

    while ((Fapi_checkFsmForReady() != Fapi_Status_FsmReady)
            && (++u32TimeOutCnt < 0x1FFFF))
    {
        Hw_FeedWatchDog();
    }

//    while ((Fapi_checkFsmForReady() != Fapi_Status_FsmReady));

    u32Flag1 = oReturnCheck;
    if ((oReturnCheck != Fapi_Status_Success) || (u32TimeOutCnt >= 0x1FFFF))
    {
        return -1;
    }

    oFlashStatus = Fapi_getFsmStatus();
    u32Flag2 = oFlashStatus;
    if (oFlashStatus != 0)
    {
        return -2;
    }
    Fapi_flushPipeline();

    oReturnCheck = Fapi_doVerify((Uint32 *)u32TagAddr,
                                 2, (Uint32 *)pu16DataBuff,
                                 &oFlashStatusWord);
    u32Flag3 = oReturnCheck;
    if(oReturnCheck != Fapi_Status_Success)
    {
        return -3;
    }

    return 0;
}

int16 WriteFlashData(Uint32 u32TagAddr, void *pu16DataBuff, Uint32 u32DataLen)
{
    Uint32 i = 0;
    Uint16 *p = (Uint16 *)pu16DataBuff;
    for (i = 0; i < u32DataLen; i+=4)
    {
        WriteFourWord(u32TagAddr + i, p + i);
    }
    return 1;
}


int16 ReadFlashData(Uint32 u32TagAddr, Uint16 *pu16DataBuff, Uint32 u32DataLen)
{
    memcpy((Uint16 *)pu16DataBuff, (Uint16 *)u32TagAddr, u32DataLen);
    return 1;
}




#ifdef __cplusplus
}
#endif


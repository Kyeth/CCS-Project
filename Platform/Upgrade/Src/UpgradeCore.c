#include "UpgradeCore.h"
#include "DspUpgradeCrc.h"
#include "Dsp2837xSFlash.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 参考工程的拷片区；本工程 RAMGS0 已用满，放到 RAMGS2 的 .upybuf */
#pragma DATA_ALIGN(u16AppBuff, 8)
#pragma DATA_SECTION(u16AppBuff, ".upybuf")
Uint16 u16AppBuff[1024];

void UpgradeCoreInit(ST_UPGRADE_HANDLE *pstUpgradeHandle, ST_UPGRADE_HANDLE stUpgradeParam)
{
    pstUpgradeHandle->stUpgradeCfg      = stUpgradeParam.stUpgradeCfg;
    pstUpgradeHandle->u16UpgradeOver    = stUpgradeParam.u16UpgradeOver;
    pstUpgradeHandle->u32AppAreaAddr    = stUpgradeParam.u32AppAreaAddr;
    pstUpgradeHandle->u32BackUpAreaAddr = stUpgradeParam.u32BackUpAreaAddr;
    pstUpgradeHandle->u32UpgradeCfgAddr = stUpgradeParam.u32UpgradeCfgAddr;
    pstUpgradeHandle->u32TimeOut        = stUpgradeParam.u32TimeOut;
    pstUpgradeHandle->u32TimeOutCnt     = stUpgradeParam.u32TimeOutCnt;
}


int32 GetUpgradeFlag(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    if (pstUpgradeHandle->stUpgradeCfg.u16UpgradeFlag == 0xAAEE)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int32 GetUpgradeFirmwareFlag(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    if (pstUpgradeHandle->stUpgradeCfg.u16FirmwareOver == 0xAAEE)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


int32 VerifyFireware(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 u32FirmwareCrc = 0;
    Uint32 u32Size = 0;

    /* 必要适配修正：CRC 读取长度直接来自配置，配置损坏时会读到备份区之外。
     * 这里按备份区容量自检（两个字节压一个字），调用方仍要按镜像类型限制长度。 */
    if (pstUpgradeHandle == 0)
    {
        return 0;
    }

    u32Size = pstUpgradeHandle->stUpgradeCfg.u32FileSize;
    if ((u32Size == 0UL) || ((u32Size & 1UL) != 0UL) ||
        (u32Size > (Uint32)(FlashBackupAreaWords * 2UL)))
    {
        return 0;
    }

    u32FirmwareCrc = UpgradeCrc32((Uint16*)pstUpgradeHandle->u32BackUpAreaAddr, pstUpgradeHandle->stUpgradeCfg.u32FileSize >> 1, 0xFFFFFFFF);
    if (u32FirmwareCrc == pstUpgradeHandle->stUpgradeCfg.u32FileCrc)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


int32 VerifyAppArea(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 u32FirmwareCrc;
    int32 i32Ret = 0;
    u32FirmwareCrc = UpgradeCrc32((Uint16*)pstUpgradeHandle->u32AppAreaAddr, pstUpgradeHandle->stUpgradeCfg.u32FileSize >> 1, 0xFFFFFFFF);
    if (u32FirmwareCrc == pstUpgradeHandle->stUpgradeCfg.u32FileCrc)
    {
        i32Ret = 1;
    }
    return i32Ret;
}





Uint32 CalcUpgradeCfgSum(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 u32Sum = 0;
    u32Sum = pstUpgradeHandle->stUpgradeCfg.bChipType + pstUpgradeHandle->stUpgradeCfg.bFirmwareType +
            pstUpgradeHandle->stUpgradeCfg.u16UpgradeFlag + pstUpgradeHandle->stUpgradeCfg.u32FileCrc +
            pstUpgradeHandle->stUpgradeCfg.u16UpgradeUart + pstUpgradeHandle->stUpgradeCfg.u16UpgradeBaud +
            pstUpgradeHandle->stUpgradeCfg.u16FirmwareOver + pstUpgradeHandle->stUpgradeCfg.u32FileSize + 1;

    return u32Sum;
}


int16 ReadUpgradeCfg(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{

    int16 i16Ret = 0;
    Uint32 u32Sum = 0;

    memcpy(&pstUpgradeHandle->stUpgradeCfg, (void*)pstUpgradeHandle->u32UpgradeCfgAddr, sizeof(ST_UPGRADE_CFG));

    u32Sum = CalcUpgradeCfgSum(pstUpgradeHandle);

    // 判断是否升级配置信息是否正确
    if (pstUpgradeHandle->stUpgradeCfg.u32UpgradeCfgSum == u32Sum)
    {
        i16Ret = 1;
    }
    else
    {
        i16Ret = 0;
    }

    return i16Ret;
}




int CopyAppFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 i = 0;
    Uint32 u32Words = 0;
    Uint32 Result = 0;
    Uint32 Remainder = 0;

    /* 必要缺陷修复：参考工程按 u32FileSize/1024 计块，又把块长按字节传给 Flash
     * 接口，而备份区是两个字节压一个字，实际会多拷一倍。这里统一按字数计算。 */
    u32Words = pstUpgradeHandle->stUpgradeCfg.u32FileSize >> 1;
    Result = u32Words / 1024U;
    Remainder = u32Words % 1024U;

    for (i = 0; i < Result; i++)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024U), u16AppBuff, 1024U);
        if (WriteFlashData(pstUpgradeHandle->u32AppAreaAddr + (i * 1024U), u16AppBuff, 1024U) != 0)
        {
            return -1;
        }
    }

    if (Remainder != 0U)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024U), u16AppBuff, Remainder);
        if (WriteFlashData(pstUpgradeHandle->u32AppAreaAddr + (i * 1024U), u16AppBuff, Remainder) != 0)
        {
            return -1;
        }
    }

    return 0;
}


/* 必要修正：BOOT 自升级要擦掉自己所在的 A/B/C，回拷过程必须全程在 RAM 执行，
 * 否则擦除完成后这条回拷链路本身的取指就落在已擦除的 Flash 上。 */
#pragma CODE_SECTION(CopyBootFirmware, ".flashfunc")
int CopyBootFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 i = 0;
    Uint32 u32Words = 0;
    Uint32 Result = 0;
    Uint32 Remainder = 0;

    /* 同 CopyAppFirmware：统一按字数计算，避免多拷一倍 */
    u32Words = pstUpgradeHandle->stUpgradeCfg.u32FileSize >> 1;
    Result = u32Words / 1024U;
    Remainder = u32Words % 1024U;

    for (i = 0; i < Result; i++)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024U), u16AppBuff, 1024U);
        if (WriteFlashData(pstUpgradeHandle->u32BootAreaAddr + (i * 1024U), u16AppBuff, 1024U) != 0)
        {
            return -1;
        }
    }

    if (Remainder != 0U)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024U), u16AppBuff, Remainder);
        if (WriteFlashData(pstUpgradeHandle->u32BootAreaAddr + (i * 1024U), u16AppBuff, Remainder) != 0)
        {
            return -1;
        }
    }

    return 0;
}


#ifdef __cplusplus
}
#endif

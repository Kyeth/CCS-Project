#include "UpgradeCore.h"
#include "DspUpgradeCrc.h"
#include "Device.h"
#include "F021.h"
#include "flash.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int16 ReadFlashData(Uint32 u32TagAddr, Uint16 *pu16DataBuff, Uint32 u32DataLen);

__attribute__((aligned(8))) Uint16 u16AppBuff[1024];

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
    Uint32 u32Sum = 0;
    int32 i32Ret = 0;

    memcpy(&pstUpgradeHandle->stUpgradeCfg, (void*)pstUpgradeHandle->u32UpgradeCfgAddr, sizeof(ST_UPGRADE_CFG));

    u32Sum = pstUpgradeHandle->stUpgradeCfg.bChipType + pstUpgradeHandle->stUpgradeCfg.bFirmwareType +
            pstUpgradeHandle->stUpgradeCfg.u16UpgradeFlag + pstUpgradeHandle->stUpgradeCfg.u32FileCrc +
            pstUpgradeHandle->stUpgradeCfg.u16UpgradeUart + pstUpgradeHandle->stUpgradeCfg.u16UpgradeBaud +
            pstUpgradeHandle->stUpgradeCfg.u16FirmwareOver + pstUpgradeHandle->stUpgradeCfg.u32FileSize + 1;


    // 判断是否升级配置信息是否正确
    if (pstUpgradeHandle->stUpgradeCfg.u32UpgradeCfgSum == u32Sum)
    {
        i32Ret = 1;
    }
    else
    {
        i32Ret = 0;
    }

    return i32Ret;
}




int CopyAppFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 i = 0;
    Uint16 Result = 0;
    Uint16 Remainder = 0;

    Result = pstUpgradeHandle->stUpgradeCfg.u32FileSize / 1024;
    Remainder = pstUpgradeHandle->stUpgradeCfg.u32FileSize % 1024;

    for (i = 0; i < Result; i++)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024), u16AppBuff, 1024);
        WriteFlashData(pstUpgradeHandle->u32AppAreaAddr + (i * 1024), u16AppBuff, 1024);
    }

    ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024), u16AppBuff, Remainder);
    WriteFlashData(pstUpgradeHandle->u32AppAreaAddr + (i * 1024), u16AppBuff, Remainder);
    return 1;
}


int CopyBootFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle)
{
    Uint32 i = 0;
    Uint16 Result = 0;
    Uint16 Remainder = 0;

    Result = pstUpgradeHandle->stUpgradeCfg.u32FileSize / 1024;
    Remainder = pstUpgradeHandle->stUpgradeCfg.u32FileSize % 1024;

    for (i = 0; i < Result; i++)
    {
        ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024), u16AppBuff, 1024);
        WriteFlashData(pstUpgradeHandle->u32BootAreaAddr + (i * 1024), u16AppBuff, 1024);
    }

    ReadFlashData(pstUpgradeHandle->u32BackUpAreaAddr + (i * 1024), u16AppBuff, Remainder);
    WriteFlashData(pstUpgradeHandle->u32BootAreaAddr + (i * 1024), u16AppBuff, Remainder);
    return 1;
}


#ifdef __cplusplus
}
#endif

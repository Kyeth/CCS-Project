//#include "XmodemDataHandle.h"
//
//#include "Flash280039C.h"
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//__attribute__((aligned(8))) Uint16 u16DataBuff[1024];
//
//int ReadBlock(Uint16 u16BlockNum, Uint16 *u16Buffer)
//{
//    return 0;
//}
//
//
//int WriteBlock(Uint16 u16BlockNum, Uint16 *u16Buffer, Uint16 u16Length)
//{
//    int i = 0;
//    Uint16 u16Value = 0;
//    Uint16 u16Cnt = 0;
//
//    for (i = 0; i < u16Length; i += 2)
//    {
//        u16Value = u16Buffer[i] | (u16Buffer[i+1] << 8);
//        u16DataBuff[u16Cnt++] = u16Value;
//    }
//
//    return u16Cnt;
//}
//
//
//int Trig(void)
//{
//    TrigFlashWrite(&stDsp280039Flash);
//    return 1;
//}
//
//
//int Reset(void)
//{
//    ResetFlashWrite(&stDsp280039Flash);
//    return 1;
//}
//
//int TrigWriteCfg(ST_UPGRADE_FILE_HEADER *pstUpgradeFile, Uint16 u16UpgradeFlag)
//{
//    Uint32 Sum = 0;;
//    stDsp280039Flash.pstUpgradeCfg->bChipType = pstUpgradeFile->bChipType;
//    stDsp280039Flash.pstUpgradeCfg->bFirmwareType = pstUpgradeFile->bFirmwareType;
//    stDsp280039Flash.pstUpgradeCfg->u32FileSize = pstUpgradeFile->u32FileSize;
//    stDsp280039Flash.pstUpgradeCfg->u32FileCrc = pstUpgradeFile->u32FileCrc;
//    stDsp280039Flash.pstUpgradeCfg->u16UpgradeFlag = u16UpgradeFlag;
//    stDsp280039Flash.pstUpgradeCfg->u16UpgradeAllow = 0;
//    stDsp280039Flash.pstUpgradeCfg->u16UpgradeBaud = 0;
//
//    Sum = stDsp280039Flash.pstUpgradeCfg->bChipType +
//            stDsp280039Flash.pstUpgradeCfg->bFirmwareType +
//            stDsp280039Flash.pstUpgradeCfg->u32FileSize +
//            stDsp280039Flash.pstUpgradeCfg->u32FileCrc +
//            stDsp280039Flash.pstUpgradeCfg->u16UpgradeFlag +
//            stDsp280039Flash.pstUpgradeCfg->u16UpgradeAllow +
//            stDsp280039Flash.pstUpgradeCfg->u16UpgradeBaud;
//
//    stDsp280039Flash.pstUpgradeCfg->u32UpgradeCfgCrc = Sum;
//
//
//    return 1;
//}
//
//int ResetWriteCfg(void)
//{
//    return 0;
//}
//
//int SetTotal(Uint32 u32Total)
//{
//    SetFlashWriteTotal(&stDsp280039Flash, u32Total >> 1);
//    return 1;
//}
//
//
//
//
//#ifdef __cplusplus
//    }
//#endif

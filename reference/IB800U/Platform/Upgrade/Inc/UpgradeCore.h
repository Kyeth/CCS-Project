#ifndef __UPGRADECORE_H
#define __UPGRADECORE_H

#include "F28003x_device.h"
#include "Dsp280039Flash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    UPGARADE_IDLE,
    UPGARADE_ERASE,
    UPGARADE_ERASE_WAIT,
    UPGARADE_WRITE,
    UPGARADE_PROGRAM_REQUEST,
    UPGARADE_PROGRAM,
    UPGARADE_VERIFY,
    UPGARADE_CLEAR,
    UPGARADE_CLEAR_WAIT,
    UPGARADE_ERROR,
    UPGARADE_END,
}E_UPGARADE_STATE;


typedef struct
{
    Uint16 bChipType:8;                 // 芯片类型
    Uint16 bFirmwareType:8;             // 固件类型
    Uint32 u32FileSize;                 // 文件大小
    Uint32 u32FileCrc;                  // 32位CRC，不包括文件头
    Uint16 u16UpgradeFlag;              // 需要升级标志
    Uint16 u16UpgradeUart;              // 升级串口
    Uint16 u16UpgradeBaud;              // 升级波特率
    Uint16 u16FirmwareOver;             // 固件接收完成标志
    Uint32 u32UpgradeCfgSum;            // 升级配置文件头的和校验

}ST_UPGRADE_CFG;

typedef struct
{
    ST_UPGRADE_CFG stUpgradeCfg;        // 升级配置信息
    Uint32 u32UpgradeCfgAddr;           // 升级配置信息保存的地址
    Uint32 u32BackUpAreaAddr;           // 备份区起始地址
    Uint32 u32AppAreaAddr;              // App区起始地址
    Uint32 u32BootAreaAddr;             // Boot区起始地址
    Uint16 u16UpgradeOver;              // 升级结束标志
    Uint32 u32TimeOutCnt;               // 超时计数
    Uint32 u32TimeOut;                  // 超时时间
    Uint32 u32ReSetTimeCountDown;
    
}ST_UPGRADE_HANDLE;


extern int16 ReadUpgradeCfg(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int32 GetUpgradeCfg(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int32 GetUpgradeFirmwareFlag(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int32 GetUpgradeFlag(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int32 VerifyFireware(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int32 VerifyAppArea(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int CopyBootFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern int CopyAppFirmware(ST_UPGRADE_HANDLE *pstUpgradeHandle);

extern Uint32 CalcUpgradeCfgSum(ST_UPGRADE_HANDLE *pstUpgradeHandle);

#ifdef __cplusplus
}
#endif

#endif

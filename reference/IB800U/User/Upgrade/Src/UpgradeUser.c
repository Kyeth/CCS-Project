#include "UpgradeUser.h"
#include "PlatformManager.h"
#include "Hw_WatchDog.h"
#include "string.h"
#include "ModbusSlaveComm.h"
#include "Hw_SciDriver.h"
#include "XmodemCore.h"
#include "XmodemSlaveProcess.h"
#include "F021.h"
#include "Schedule.h"

__attribute__((aligned(8))) ST_UPGRADE_HANDLE stUpgradeHandle;
__attribute__((aligned(8))) Uint16 u16DataBuff[16];

void UpgradeUserInit(void)
{
    memset(&stUpgradeHandle, 0, sizeof(ST_UPGRADE_HANDLE));
    stUpgradeHandle.u32TimeOut = 50;
    stUpgradeHandle.u32TimeOutCnt = 0;
    stUpgradeHandle.u32UpgradeCfgAddr = 0xAF000U;
    stUpgradeHandle.u32AppAreaAddr = 0x86000U;
    stUpgradeHandle.u32BackUpAreaAddr = 0x9B000U;
    stUpgradeHandle.u32BootAreaAddr = 0x80000U;
}

void CheckUpgrade(void)
{
    // APP区的起始地址
//    FuncAppStart pFuncAppStart;
//    pFuncAppStart  = (Uint32*)0x00086000;

    FuncAppStart pFuncAppStart  = (FuncAppStart)0x00086000;

    Uint32 u32Baud = 0;

    Uint32 u32Sum = 0;

    if (ReadUpgradeCfg(&stUpgradeHandle) == 1)
    {
        // 如果没有接收到完整的升级固件，则需要重新接收
        if (GetUpgradeFirmwareFlag(&stUpgradeHandle) == 0)
        {
            if (((stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 96)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 192)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 384)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 576)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 1152)) &&
                    ((stUpgradeHandle.stUpgradeCfg.u16UpgradeUart == SCI_ID0 )
                            || (stUpgradeHandle.stUpgradeCfg.u16UpgradeUart == SCI_ID1)))
            {
                u32Baud = stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud * 100;
                // 这里需要重新初始化串口波特率
                Hw_SciCtrl(stUpgradeHandle.stUpgradeCfg.u16UpgradeUart, SCI_COMM_CFG, &u32Baud);
            }
            else
            {
                pFuncAppStart();
            }
        }
        else    // 这里接收到完整的固件，需要进行固件校验与拷贝
        {
            if (VerifyFireware(&stUpgradeHandle))
            {

                if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == 1)
                {
                    // 升级APP程序
                    CopyAppFirmware(&stUpgradeHandle);
                    VerifyAppArea(&stUpgradeHandle);

                }
                else if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == 2)
                {
                    // 升级Boot程序
                    CopyBootFirmware(&stUpgradeHandle);
                }
                stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0;
                stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0;
                stUpgradeHandle.stUpgradeCfg.u32FileSize = ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize;
                stUpgradeHandle.stUpgradeCfg.u32FileCrc = ObjXmodemCtrl.stUpgradeFileHeader.u32FileCrc;
                u32Sum = CalcUpgradeCfgSum(&stUpgradeHandle);
                stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = u32Sum;
                EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr, &stUpgradeHandle.stUpgradeCfg, sizeof(ST_UPGRADE_CFG));
            }
            else
            {
                pFuncAppStart();
            }
        }
    }
    else
    {
        pFuncAppStart();
    }

}

void TaskUpgrade(void)
{
    Uint32 u32Sum = 0;
    if ((GetUpgradeCfg(&stUpgradeHandle) == 1) && (GetUpgradeFirmwareFlag(&stUpgradeHandle) == 0))
    {
        // XmodemReceive(&ObjXmodemCtrl);
        //if (GetFileWriteOver(&ObjXmodemCtrl))
        {
            ReadUpgradeCfg(&stUpgradeHandle);

            stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0xAAEE;
            stUpgradeHandle.stUpgradeCfg.u32FileSize = ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize;
            stUpgradeHandle.stUpgradeCfg.u32FileCrc = ObjXmodemCtrl.stUpgradeFileHeader.u32FileCrc;
            u32Sum = CalcUpgradeCfgSum(&stUpgradeHandle);
            stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = u32Sum;

            EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
            WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr, &stUpgradeHandle.stUpgradeCfg, sizeof(ST_UPGRADE_CFG));

            // 写入配置信息后，重启
            Hw_EnableWatchDog();
            Hw_FeedWatchDog();
            Hw_SysReset();
        }
    }
}

Uint16 u16Flag = 0;
Uint16 u16DataBuffUpgrade[128];
void TaskCheckUpgrade(void)
{
//    Uint16 u16DataBuff[128];
    Uint32 u32Sum = 0;

    if (ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16MdsIsUpdate == 1)
    {
        ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16MdsIsUpdate = 2;

        u16Flag = 1;
    }

    if (u16Flag == 1)
    {
        u16Flag = 2;
        stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0xAAEE;
        stUpgradeHandle.stUpgradeCfg.bChipType = 1;
        stUpgradeHandle.stUpgradeCfg.bFirmwareType = ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16FirmwareType;
        stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud = ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16UpdateBaud;
        stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0;
        stUpgradeHandle.stUpgradeCfg.u16UpgradeUart = 0;
        stUpgradeHandle.stUpgradeCfg.u32FileCrc = 0;
        stUpgradeHandle.stUpgradeCfg.u32FileSize = 0;

        u32Sum = CalcUpgradeCfgSum(&stUpgradeHandle);
        stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = u32Sum;
        EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        memcpy(u16DataBuffUpgrade, &stUpgradeHandle.stUpgradeCfg, sizeof(ST_UPGRADE_CFG));
        WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr, u16DataBuffUpgrade, sizeof(ST_UPGRADE_CFG));
    }

    if (u16Flag == 2)
    {
        stUpgradeHandle.u32ReSetTimeCountDown++;
        if (stUpgradeHandle.u32ReSetTimeCountDown > 200)
        {
            EALLOW;
            DINT;   // Disable Global interrupt INTM
            EDIS;
            Hw_SysReset();
        }
    }
    SetTaskRunStatus(TASK_CHECK_UPGRADE_BIT);
}

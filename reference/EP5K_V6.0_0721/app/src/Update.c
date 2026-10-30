/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: Update.c
Author: tzz
Version : 0.0.2
Date:2022年5月5日
Description: 代码升级检测主函数入口
Function List:
***********************************************************/
#include "string.h"
#include "Update.h"
#include "ModbusTable.h"
#include "ConfigSystemPara.h"
#include "Sector.h"

extern void ServiceDog(void);

UPDATE_DATA UpdateData = {0,0,1};

/*************************************************
Function: WriteFlashUpdateFlag
Description: 写 flash 升级标志,返回 0 写入成功;返回 1 写入失败
Input: None
Output: None
Frequency: Once
*************************************************/
Uint16 WriteFlashUpdateFlag(void)
{
    Uint16 readTemp = 0;
    Fapi_StatusType oReturnCheck;
    // 1.写升级标志
    // 0X55BB 用于标识接收到升级指令--复位DSP
    UpdateData.WriteBuffer[2] = BootFLAG_BOOTLOADER;
    sharedErase(BK_FLAG_SECTOR, APP_BK_BANKX);
    DINT;
    oReturnCheck = programFlashOneWord(0x97FFF - 3, UpdateData.WriteBuffer);
    EINT;

    // 检查是否写入成功
    readTemp = *(Uint16 *)(0x97FFF - 1);

    // 2.跳转到 bootloader
    if((Fapi_Status_Success == oReturnCheck)
            && (BootFLAG_BOOTLOADER == readTemp))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
/*************************************************
Function: WriteFlashUpdateFlag
Description: 检查升级标志位
Input: None
Output: None
Frequency: 1ms
*************************************************/
void CheckUpdateFlag(void)
{
    Uint16 readTemp = 0;

    if (UpdateData.InitFlag != 0xABCD)
    {
        memset(&UpdateData, 0, sizeof(UPDATE_DATA));
        UpdateData.InitFlag = 0xABCD;
        UpdateData.KillDog = 1;
    }

    if (UpdateData.KillDog == 1)
    {
        ServiceDog();   //喂狗
    }

    // -------- DSP 升级指令,需要关机并跳转--------
    if (UpdateData.UpdateFlag == 1)
    {
        // 系统关机
        SystemPara.SystemPowerOn = 0;

        // 50ms 后设置ARM关闭硬件驱动标志
        if (UpdateData.JumpCount == 500)
        {
            ArmReadData.ARMEnable = 0x00;
        }

        // 1.5s后跳转到BootLoader
        if (++UpdateData.JumpCount >= 1500)
        {
            // 写 flash 标志位
            readTemp = WriteFlashUpdateFlag();
            // 写标志位成功, 复位重启
            if (readTemp == 0 && UpdateData.JumpCount == 1500)
            {
                DINT;

                // 重启
                EALLOW;
                WdRegs.WDCR.all = 0x0028;
                WdRegs.SCSR.all = 0x0000;
                EDIS;

                EINT;

                UpdateData.KillDog = 0;         // 停止喂狗
            }
            else
            {
                UpdateData.JumpCount = 0;
            }
        }
    }
    // ARM 升级指令,关机封锁驱动,不需要跳转
    else if (UpdateData.UpdateFlag == 2)
    {
        ArmWriteData.LoadVolt1 = 0;
        ArmWriteData.LoadVolt2 = 0;
        // 系统关机
        SystemPara.SystemPowerOn = 0;

        // 50ms 后关闭驱动
        if (UpdateData.JumpCount == 500)
        {
            ArmReadData.ARMEnable = 0x00;
        }
    }
    else
    {
        return;
    }
}

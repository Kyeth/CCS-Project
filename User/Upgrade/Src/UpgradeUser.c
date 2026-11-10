#include "UpgradeUser.h"
#include "Hw_WatchDog.h"
#include "Hw_SciDriver.h"
#include "SciPort.h"
#include "string.h"
#include "sysctl.h"
#include "XmodemCore.h"
#include "XmodemSlaveProcess.h"
#include "Dsp2837xSFlash.h"


ST_UPGRADE_HANDLE stUpgradeHandle;
//__attribute__((aligned(8))) Uint16 u16DataBuff[16];

void UpgradeUserInit(void)
{
    memset(&stUpgradeHandle, 0, sizeof(ST_UPGRADE_HANDLE));
    stUpgradeHandle.u32TimeOut = 50;
    stUpgradeHandle.u32TimeOutCnt = 0;
    stUpgradeHandle.u32UpgradeCfgAddr = 0xBE000U;
    stUpgradeHandle.u32AppAreaAddr = 0x086000U;
    stUpgradeHandle.u32BackUpAreaAddr = 0x0A0000U;
    stUpgradeHandle.u32BootAreaAddr = 0x080000U;
}

int16 EraseBackUpBank(void)
{
    int16 i16Ret = 0;

    /* 芯片适配：备份区 = H/I/J（各 32K 字节）+ K（8K 字节），按真实扇区起点擦除。
     * 必要适配修正：参考实现丢弃了每次擦除的返回值，擦除失败后仍然继续收包。 */
    i16Ret = EraseSingleSector(SectorH_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }
    i16Ret = EraseSingleSector(SectorI_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }
    i16Ret = EraseSingleSector(SectorJ_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }

    return EraseSingleSector(SectorK_start);
}

void EraseAppBank(void)
{
    /* APP 区 = D（8K 字节）+ E/F/G（各 32K 字节） */
    (void)EraseSingleSector(SectorD_start);
    (void)EraseSingleSector(SectorE_start);
    (void)EraseSingleSector(SectorF_start);
    (void)EraseSingleSector(SectorG_start);
}


void EraseBootBank(void)
{
    /* BOOT 区 = A/B/C（各 8K 字节） */
    (void)EraseSingleSector(SectorA_start);
    (void)EraseSingleSector(SectorB_start);
    (void)EraseSingleSector(SectorC_start);
}

void jump_to_app(void)
{
    // APP区的起始地址
    FuncAppStart pFuncAppStart  = (FuncAppStart)0x00086000;

    EALLOW;
    DINT;   // Disable Global interrupt INTM
    EDIS;
    pFuncAppStart();
}

void CheckUpgrade(void)
{
    Uint32 u32Baud = 0;
    int16 i16Temp = 0;

    i16Temp = ReadUpgradeCfg(&stUpgradeHandle);
    if (i16Temp)
    {
        // 如果没有接收到完整的升级固件，则需要重新接收
        if (GetUpgradeFirmwareFlag(&stUpgradeHandle) == 0)
        {
            if (((stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 96)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 192)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 384)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 576)
                    || (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 1152)) &&
                    (stUpgradeHandle.stUpgradeCfg.u16UpgradeUart == HW_SCI_ID_A))
            {
                u32Baud = stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud * 100;
                // 这里需要重新初始化串口波特率
                (void)Hw_SciSetBaud(HW_SCI_ID_A, u32Baud);

                EraseBackUpBank();
            }
            else if (stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud == 0)
            {
                u32Baud = 38400;
                // 这里需要重新初始化串口波特率
                (void)Hw_SciSetBaud(HW_SCI_ID_A, u32Baud);

                EraseBackUpBank();
            }
            else
            {
                EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                jump_to_app();
            }
        }
        else    // 这里接收到完整的固件，需要进行固件校验与拷贝
        {
            if (VerifyFireware(&stUpgradeHandle))
            {

                if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == 1)
                {
                    // 升级APP程序
                    EraseAppBank();
                    CopyAppFirmware(&stUpgradeHandle);
                    VerifyAppArea(&stUpgradeHandle);

                    EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                    jump_to_app();

                }
                else if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == 2)
                {
                    // 升级Boot程序
                    EraseBootBank();
                    CopyBootFirmware(&stUpgradeHandle);
                    EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                    jump_to_app();
                }

            }
            else
            {
                EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                jump_to_app();
            }
        }
    }
    else
    {
        jump_to_app();
    }

}




/* 已收完、只等发送排空后写配置的标志（本工程发送由主循环推进，不能阻塞等） */
static Uint16 s_u16UpgradeDone = 0;

/* 必要修正：上一次会话在尾部置位后，中止路径不清它，下一次会话
 * 会直接跳过 XmodemReceive() 而去走写配置的尾巴。开始与中止时都要清。 */
void UpgradeUserSessionReset(void)
{
    s_u16UpgradeDone = 0U;
}

Uint16 TaskUpgrade(void)
{
    Uint32 u32Sum = 0;
    E_XMODEM_ERROR eXmodemError = XMODEM_OK;
    int16 i16Ret = 0;

    if ((ReadUpgradeCfg(&stUpgradeHandle) != 1) || (GetUpgradeFirmwareFlag(&stUpgradeHandle) != 0))
    {
        return 0U;      /* 没有"进行中"配置：会话已经结束 */
    }

    if (s_u16UpgradeDone == 0U)
    {
        eXmodemError = XmodemReceive(&ObjXmodemCtrl);
        if ((eXmodemError == XMODEM_END) &&
            ((GetFileWriteOver(&ObjXmodemCtrl) != 0) ||
             (ObjXmodemCtrl.u32RecvLength >= ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize)))
        {
            s_u16UpgradeDone = 1U;
        }
    }
    else
    {
        eXmodemError = XMODEM_END;
    }

    if (eXmodemError == XMODEM_END)
    {
        if ((GetFileWriteOver(&ObjXmodemCtrl) == 0) &&
            (ObjXmodemCtrl.u32RecvLength < ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize))
        {
            return 0U;
        }

        /* 端口适配：参考工程在这里阻塞等 SCI 移位寄存器排空。本工程的发送由
         * 主循环的 PortTaskService 推进，这里阻塞会把自己等死，所以改成"没排空就下一拍再来"。 */
        if (SciPortIsTxIdle(HW_SCI_ID_A) == 0U)
        {
            return 0U;
        }

        stUpgradeHandle.stUpgradeCfg.bChipType = ObjXmodemCtrl.stUpgradeFileHeader.bChipType;
        stUpgradeHandle.stUpgradeCfg.bFirmwareType = ObjXmodemCtrl.stUpgradeFileHeader.bFirmwareType;
        stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0xAAEE;
        stUpgradeHandle.stUpgradeCfg.u32FileSize = ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize;
        stUpgradeHandle.stUpgradeCfg.u32FileCrc = ObjXmodemCtrl.stUpgradeFileHeader.u32FileCrc;

        /* 必要缺陷修复：参考工程在这里直接写配置并复位，把长度/CRC 校验留给
         * 下次上电的 CheckUpgrade()。交接约定是备份区校验通过才写配置，所以先校验。 */
        if (VerifyFireware(&stUpgradeHandle) != 1)
        {
            (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
            return 2U;
        }

        u32Sum = CalcUpgradeCfgSum(&stUpgradeHandle);
        stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = u32Sum;

        /* 必要适配修正：配置区擦除/写入失败也要上报，不能“不管成不成都复位”。 */
        i16Ret = EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        if (i16Ret != 0)
        {
            return 2U;
        }

        i16Ret = WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr,
                                &stUpgradeHandle.stUpgradeCfg,
                                (Uint32)sizeof(ST_UPGRADE_CFG));
        if (i16Ret != 0)
        {
            (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
            return 2U;
        }

        /* 写入配置信息后，重启 */
        Hw_EnableWatchDog();
        Hw_FeedWatchDog();
        SysCtl_resetDevice();

        return 1U;
    }

    /* 中止类错误（两次 CAN、写 Flash 失败、文件头不合法、重试超限）：
     * 清掉本次会话的"进行中"配置后交给上层收尾；不再直接复位。 */
    if ((eXmodemError != XMODEM_OK) && (eXmodemError != XMODEM_TIMEOUT))
    {
        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        return 2U;
    }

    return 0U;
}

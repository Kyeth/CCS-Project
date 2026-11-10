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

void EraseBackUpBank(void)
{
    /* 芯片适配：备份区 = H/I/J（各 32K 字节）+ K（8K 字节），按真实扇区起点擦除 */
    (void)EraseSingleSector(SectorH_start);
    (void)EraseSingleSector(SectorI_start);
    (void)EraseSingleSector(SectorJ_start);
    (void)EraseSingleSector(SectorK_start);
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

void TaskUpgrade(void)
{
    Uint32 u32Sum = 0;
    E_XMODEM_ERROR eXmodemError = XMODEM_OK;
    if ((ReadUpgradeCfg(&stUpgradeHandle) == 1) && (GetUpgradeFirmwareFlag(&stUpgradeHandle) == 0))
    {
        if (s_u16UpgradeDone == 0U)
        {
            eXmodemError = XmodemReceive(&ObjXmodemCtrl);
            if ((eXmodemError == XMODEM_END) && (((GetFileWriteOver(&ObjXmodemCtrl) != 0) || (ObjXmodemCtrl.u32RecvLength >= ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize))))
            {
                s_u16UpgradeDone = 1U;
            }
        }
        else
        {
            eXmodemError = XMODEM_END;
        }

        // 只有当 Xmodem 状态机完整结束（收到EOT并回复ACK）后才处理
        if (eXmodemError == XMODEM_END)
        {
            // 确认文件接收完整
            if (((GetFileWriteOver(&ObjXmodemCtrl) != 0) || (ObjXmodemCtrl.u32RecvLength >= ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize)))
            {
                /* 端口适配：参考工程在这里阻塞等 SCI 移位寄存器排空。本工程的发送由
                 * 主循环的 PortTaskService 推进，这里阻塞会把自己等死，所以改成
                 * "没排空就下一拍再来"，最终一致。 */
                if (SciPortIsTxIdle(HW_SCI_ID_A) == 0U)
                {
                    return;
                }
                stUpgradeHandle.stUpgradeCfg.bChipType = ObjXmodemCtrl.stUpgradeFileHeader.bChipType;
                stUpgradeHandle.stUpgradeCfg.bFirmwareType = ObjXmodemCtrl.stUpgradeFileHeader.bFirmwareType;
                stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0xAAEE;
                stUpgradeHandle.stUpgradeCfg.u32FileSize = ObjXmodemCtrl.stUpgradeFileHeader.u32FileSize;
                stUpgradeHandle.stUpgradeCfg.u32FileCrc = ObjXmodemCtrl.stUpgradeFileHeader.u32FileCrc;

                /* 必要缺陷修复：参考工程在这里直接写配置并复位，把长度/CRC 校验留给
                 * 下次上电的 CheckUpgrade()。本工程的 APP 侧没有 CheckUpgrade，
                 * 交接约定是备份区校验通过才写配置，所以先校验再写。
                 * 参考位置：UpgradeUser.c TaskUpgrade() 的 XMODEM_END 分支。 */
                if (VerifyFireware(&stUpgradeHandle) != 1)
                {
                    EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                    Hw_EnableWatchDog();
                    Hw_FeedWatchDog();
                    SysCtl_resetDevice();
                    return;
                }

                u32Sum = CalcUpgradeCfgSum(&stUpgradeHandle);
                stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = u32Sum;
                EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
                WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr, &stUpgradeHandle.stUpgradeCfg, sizeof(ST_UPGRADE_CFG));

                // 写入配置信息后，重启
                Hw_EnableWatchDog();
                Hw_FeedWatchDog();
                SysCtl_resetDevice();
            }
        }
        // 如果到达了重试上限，则退出升级
        if (eXmodemError == XMODEM_RETRY_ERROR)
        {
            EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
            // 写入配置信息后，重启
            Hw_EnableWatchDog();
            Hw_FeedWatchDog();
            SysCtl_resetDevice();
        }
    }
}

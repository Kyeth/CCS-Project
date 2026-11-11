#include "UpgradeUser.h"
#include "Hw_WatchDog.h"
#include "Hw_SciDriver.h"
#include "SciPort.h"
#include "Hw_TimeBase.h"
#include "string.h"
#include "sysctl.h"
#include "XmodemCore.h"
#include "XmodemSlaveProcess.h"
#include "Dsp2837xSFlash.h"
#include "DspUpgradeCrc.h"
#include "XmodemTramsports.h"


ST_UPGRADE_HANDLE stUpgradeHandle;

/* 接收门控：只有"备份区擦除 + 接收配置"都准备好才允许 TaskUpgrade() 调 XmodemReceive()。
 * 首次接收、掉电重入、中止重入三条路径统一走这里，避免准备失败却发出 'C'。 */
static Uint16 s_u16ReceiveReady = 0U;

/* "接收中"记录里 u16UpgradeBaud = 0 表示默认档，必须与 SCI-A 的实际配置一致
 * （DrvCfg_EP15K.c 里 SCI-A = 19200），否则首次接收和掉电重入会用上不同的波特率。 */
#define UPGRADE_DEFAULT_BAUD (19200UL)
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
    s_u16ReceiveReady = 0U;
}

/* 自升级尾部在擦掉 A/B/C 之后还要清备份区，届时不能再回 Flash 取指，
 * 所以本函数与擦写链一起驻 RAM（.flashfunc）。 */
#pragma CODE_SECTION(EraseBackUpBank, ".flashfunc")
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

int16 EraseAppBank(void)
{
    int16 i16Ret = 0;

    /* APP 区 = D（8K 字节）+ E/F/G（各 32K 字节）。
     * 必要适配修正：参考实现丢弃四次擦除的返回值，回拷失败后仍清配置跳转。 */
    i16Ret = EraseSingleSector(SectorD_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }
    i16Ret = EraseSingleSector(SectorE_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }
    i16Ret = EraseSingleSector(SectorF_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }

    return EraseSingleSector(SectorG_start);
}


/* 必要修正（不可恢复缺陷）：BOOT 自升级时，被擦除的正是本文件所在的 A/B/C 扇区。
 * 参考实现里 EraseBootBank()/CopyBootFirmware() 链接在 Flash，擦完 A/B/C 之后
 * 函数返回要回到已被擦除的 Flash 取指，必然跑飞。这里把两者及自升级尾部一起放进
 * .flashfunc（LOAD = FLASH_BOOT，RUN = RAMGS3），整段在 RAM 里执行。 */
#pragma CODE_SECTION(EraseBootBank, ".flashfunc")
int16 EraseBootBank(void)
{
    int16 i16Ret = 0;

    /* BOOT 区 = A/B/C（各 8K 字节）。自升级时这里擦掉的正是本文件所在扇区，
     * 因此三个返回值都必须检查：擦失败后不能继续回拷。 */
    i16Ret = EraseSingleSector(SectorA_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }
    i16Ret = EraseSingleSector(SectorB_start);
    if (i16Ret != 0)
    {
        return i16Ret;
    }

    return EraseSingleSector(SectorC_start);
}

/* 前向声明：下面几个跳转辅助函数定义在 jump_to_app() 之前。 */
void jump_to_app(void);

/* 必要修正（APP 有效性）：参考工程在"无配置/校验失败/未收完但参数不合法"三个
 * 分支里都直接 jump_to_app()，不检查 APP 区是否有内容。APP 空白时等于跳进擦除态
 * Flash（首字 0xFFFF），整机失去响应。判据：APP 入口 0x086000 的首字（codestart
 * 跳转）不能是 0xFFFF，且次字（APP 入口向量）必须落在 APP 区内（0x086002..0x09FFFF）。 */
/* 配置自检：升级标志、芯片类型、镜像类型、非零偶数长度、按目标分区的长度上限。
 * 必要适配修正：参考实现在读备份区 CRC 之前不校验这些字段，配置损坏时 CRC 长度
 * 直接来自 u32FileSize，会读到备份区之外。 */
/* "接收中"记录只校验接收参数：此时 bChipType/bFirmwareType/u32FileSize/u32FileCrc
 * 都还是 0（APP 与 BOOT 写接收配置时就是这么填的），不能按完整镜像规则判无效。 */
static Uint16 IsReceiveCfgValid(void)
{
    Uint16 u16BaudCfg = stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud;

    if (stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag != 0xAAEEU)
    {
        return 0U;
    }

    if (stUpgradeHandle.stUpgradeCfg.u16UpgradeUart != (Uint16)HW_SCI_ID_A)
    {
        return 0U;
    }

    if ((u16BaudCfg != 0U) && (u16BaudCfg != 96U) && (u16BaudCfg != 192U) &&
        (u16BaudCfg != 384U) && (u16BaudCfg != 576U) && (u16BaudCfg != 1152U))
    {
        return 0U;
    }

    return 1U;
}


/* "已收完"记录按完整镜像校验：芯片类型、镜像类型、非零偶数长度、按目标分区限长。 */
static Uint16 IsImageCfgValid(void)
{
    Uint32 u32Size = stUpgradeHandle.stUpgradeCfg.u32FileSize;

    if (stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag != 0xAAEEU)
    {
        return 0U;
    }

    if (stUpgradeHandle.stUpgradeCfg.bChipType != XMODEM_CHIP_TYPE)
    {
        return 0U;
    }

    if ((stUpgradeHandle.stUpgradeCfg.bFirmwareType != XMODEM_FIRMWARE_APP) &&
        (stUpgradeHandle.stUpgradeCfg.bFirmwareType != XMODEM_FIRMWARE_BOOT))
    {
        return 0U;
    }

    if ((u32Size == 0UL) || ((u32Size & 1UL) != 0UL))
    {
        return 0U;
    }

    if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == XMODEM_FIRMWARE_APP)
    {
        if (u32Size > XMODEM_APP_MAX_BYTES)
        {
            return 0U;
        }
    }
    else
    {
        if (u32Size > XMODEM_BOOT_MAX_BYTES)
        {
            return 0U;
        }
    }

    return 1U;
}


/* 掉电重入：沿用已存在的"接收中"配置，重设波特率并擦净备份区。两步都成功才算准备好。 */
static Uint16 PrepareResumeReceive(void)
{
    Uint32 u32Baud = 0UL;
    Uint16 u16BaudCfg = stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud;

    if ((u16BaudCfg == 96U) || (u16BaudCfg == 192U) || (u16BaudCfg == 384U) ||
        (u16BaudCfg == 576U) || (u16BaudCfg == 1152U))
    {
        u32Baud = (Uint32)u16BaudCfg * 100U;
    }
    else if (u16BaudCfg == 0U)
    {
        u32Baud = UPGRADE_DEFAULT_BAUD;
    }
    else
    {
        return 0U;
    }

    if (stUpgradeHandle.stUpgradeCfg.u16UpgradeUart != (Uint16)HW_SCI_ID_A)
    {
        return 0U;
    }

    if (Hw_SciSetBaud(HW_SCI_ID_A, u32Baud) == 0U)
    {
        return 0U;
    }

    if (EraseBackUpBank() != 0)
    {
        return 0U;
    }

    return 1U;
}


static Uint16 IsAppValid(void)
{
    const volatile Uint16 *pu16App = (const volatile Uint16 *)0x086000U;
    Uint16 u16Word0;
    Uint16 u16Word1;
    Uint32 u32Target;

    u16Word0 = pu16App[0];
    u16Word1 = pu16App[1];

    if ((u16Word0 == 0xFFFFU) || (u16Word1 == 0xFFFFU))
    {
        return 0U;
    }

    /* C28x 的 LB 指令占两个字：word0 的高 10 位是操作码（(word0 & 0xFFC0) == 0x0040），
     * 低 6 位是目标地址的高 6 位，word1 是低 16 位。先前把 word1 直接当入口地址与
     * 0x086002 比较，Uint16 永远小于该下界，正常 APP 也会被判无效——必须解析完整目标。 */
    if ((u16Word0 & 0xFFC0U) != 0x0040U)
    {
        return 0U;
    }

    u32Target = ((Uint32)(u16Word0 & 0x003FU) << 16) | (Uint32)u16Word1;
    if ((u32Target < 0x086002UL) || (u32Target >= 0x0A0000UL))
    {
        return 0U;
    }

    return 1U;
}

/* APP 无效时留在 BOOT 收升级：写一份"接收中"配置，让主循环 TaskUpgrade() 的
 * 守卫（ReadUpgradeCfg == 1 && GetUpgradeFirmwareFlag == 0）通过。 */
static Uint16 StartReceiveFromBoot(Uint16 u16BaudCfg)
{
    /* 必要适配修正：参考工程进入接收前不擦备份区，中途掉电会与上一轮半截镜像混在一起；
     * 也不看配置擦写结果，写失败后仍然发 'C'，收下的数据无处安放。这里两步都成功
     * 才允许进入接收。返回 1 = 可以收包。 */
    if (EraseBackUpBank() != 0)
    {
        return 0U;
    }

    memset(&stUpgradeHandle.stUpgradeCfg, 0, sizeof(ST_UPGRADE_CFG));
    stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0xAAEEU;
    stUpgradeHandle.stUpgradeCfg.u16UpgradeUart = (Uint16)HW_SCI_ID_A;
    stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud = u16BaudCfg;  /* 115200 -> 1152 */
    stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0U;
    stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = CalcUpgradeCfgSum(&stUpgradeHandle);

    if (EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr) != 0)
    {
        return 0U;
    }

    if (WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr,
                       &stUpgradeHandle.stUpgradeCfg,
                       (Uint32)sizeof(ST_UPGRADE_CFG)) != 0)
    {
        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        return 0U;
    }

    /* 门控在此打开：只有备份区已擦净、接收配置已落盘，主循环才会调 XmodemReceive()。 */
    s_u16ReceiveReady = 1U;

    return 1U;
}

/* -------------------------------------------------------------------------
 * APP 无效时的救援握手（旧 ARM 协议，SCIA 19200 -> 115200）
 *   19200 ：F0 00 12 03 04    -> F0 00 12 05 06（ARM 升级请求）
 *           F0 00 12 07 08    -> F0 00 12 09 0A（ARM 自身升级通知，只应答）
 *   115200：01 10 04 38 00 02 -> 01 10 04 38 00 02 C1 35
 * 只认以上整帧形式，其余字节丢掉继续等：握手阶段没有可靠帧边界，把可疑字节
 * 当成升级流会毁掉传输。握手期间不擦写 Flash，动作全部留给 8 字节应答发完之后。
 * ---------------------------------------------------------------------- */
#define BOOT_HS_CMD_LEN       (5U)
#define BOOT_HS_REQ_LEN       (6U)
#define BOOT_HS_REPLY_LEN     (8U)
#define BOOT_HS_RX_MAX        (16U)
#define BOOT_HS_BAUD_NORMAL   (19200UL)
#define BOOT_HS_BAUD_UPGRADE  (115200UL)
#define BOOT_HS_WAIT_MS       (5000UL)
/* "接收中"配置里的速率编码是 x100，115200 即 1152 */
#define BOOT_HS_BAUD_CFG      (1152U)

#define BOOT_HS_STOPPED  (0xFFU)  /* 未启用：交给 TaskUpgrade() 或已跳 APP */
#define BOOT_HS_IDLE     (0U)     /* 19200 等 F0 */
#define BOOT_HS_F0_TX    (1U)     /* F0 应答在发，发完切 115200 */
#define BOOT_HS_WAIT_REQ (2U)     /* 115200 等传输请求 */
#define BOOT_HS_REQ_TX   (3U)     /* 8 字节应答在发，发完准备接收 */
#define BOOT_HS_RESTORE  (4U)     /* 收尾：控制字节发完再切回 19200 */

static const Uint16 s_au16HsUpCmd[BOOT_HS_CMD_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x03U, 0x04U
};
static const Uint16 s_au16HsUpReply[BOOT_HS_CMD_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x05U, 0x06U
};
static const Uint16 s_au16HsArmCmd[BOOT_HS_CMD_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x07U, 0x08U
};
static const Uint16 s_au16HsArmReply[BOOT_HS_CMD_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x09U, 0x0AU
};
static const Uint16 s_au16HsReq[BOOT_HS_REQ_LEN] =
{
    0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U
};
/* 前 6 字节的 Modbus CRC16 = 0x35C1，RTU 线上低字节先发 */
static const Uint16 s_au16HsReqReply[BOOT_HS_REPLY_LEN] =
{
    0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U, 0xC1U, 0x35U
};

static Uint16 s_u16HsState = BOOT_HS_STOPPED;
static Uint16 s_au16HsRx[BOOT_HS_RX_MAX];
static Uint16 s_u16HsRxLen = 0U;
static Uint32 s_u32HsTick = 0U;


/* 丢掉接收缓冲的前 u16Count 个字节 */
static void BootHsShift(Uint16 u16Count)
{
    Uint16 i;

    if (u16Count >= s_u16HsRxLen)
    {
        s_u16HsRxLen = 0U;
        return;
    }

    for (i = 0U; i < (Uint16)(s_u16HsRxLen - u16Count); i++)
    {
        s_au16HsRx[i] = s_au16HsRx[i + u16Count];
    }

    s_u16HsRxLen = (Uint16)(s_u16HsRxLen - u16Count);
}


/* 接收缓冲前部是否正好是给定整帧 */
static Uint16 BootHsMatch(const Uint16 *pu16Pat, Uint16 u16Len)
{
    Uint16 i;

    if (s_u16HsRxLen < u16Len)
    {
        return 0U;
    }

    for (i = 0U; i < u16Len; i++)
    {
        if (s_au16HsRx[i] != pu16Pat[i])
        {
            return 0U;
        }
    }

    return 1U;
}


/* 整理接收缓冲：丢掉首字节不是 u16Sync 的噪声；够一整帧却匹配不上时也往前滑一个字节 */
static void BootHsTrim(Uint16 u16Sync, Uint16 u16Need)
{
    Uint16 u16Guard = 0U;

    while ((s_u16HsRxLen > 0U) && (s_au16HsRx[0] != u16Sync))
    {
        BootHsShift(1U);
        u16Guard++;
        if (u16Guard >= BOOT_HS_RX_MAX)
        {
            s_u16HsRxLen = 0U;
            return;
        }
    }

    if (s_u16HsRxLen < u16Need)
    {
        return;                 /* 可能还没收完，留着等下一批 */
    }

    BootHsShift(1U);            /* 够长却不匹配：滑一个字节再看 */
}


/* 收空端口里已到的字节（缓冲满就留给下一轮） */
static void BootHsRead(void)
{
    ST_SCI_RX_SEG stSeg;
    Uint16 u16Got;

    do
    {
        u16Got = SciPortPopBatch(HW_SCI_ID_A,
                                 &s_au16HsRx[s_u16HsRxLen],
                                 (Uint16)(BOOT_HS_RX_MAX - s_u16HsRxLen),
                                 &stSeg);
        s_u16HsRxLen = (Uint16)(s_u16HsRxLen + u16Got);
    }
    while ((u16Got != 0U) && (s_u16HsRxLen < BOOT_HS_RX_MAX));
}


/* 把整帧挂到发送侧；返回 0 = 环满暂时发不下，下一轮重试 */
static Uint16 BootHsSend(const Uint16 *pu16Src, Uint16 u16Len)
{
    return (SciPortWriteTx(HW_SCI_ID_A, pu16Src, u16Len) == u16Len) ? 1U : 0U;
}


/* 进入收尾态：等控制字节按当前速率发完，再切回 19200 重新等 F0 */
static void BootHsEnterRestore(void)
{
    s_u16HsState = BOOT_HS_RESTORE;
    s_u16HsRxLen = 0U;
}


/* 开始救援握手：19200 重新等 F0（APP 无效、掉电重入、会话中止都走这里） */
static void BootHsStart(void)
{
    s_u16ReceiveReady = 0U;
    (void)Hw_SciSetBaud(HW_SCI_ID_A, BOOT_HS_BAUD_NORMAL);
    SciPortResync(HW_SCI_ID_A);
    s_u16HsRxLen = 0U;
    s_u16HsState = BOOT_HS_IDLE;
}


/**
 * @brief 救援握手服务（主循环逐轮调用）
 * @note  握手期间不擦写 Flash：备份区擦除与"接收中"配置都在 8 字节应答真正
 *        发完之后才做，避免准备动作挤掉应答、让主站等不到回复。
 */
void UpgradeUserHandshakeService(void)
{
    if (s_u16HsState == BOOT_HS_STOPPED)
    {
        return;
    }

    if (s_u16HsState == BOOT_HS_RESTORE)
    {
        /* 控制字节（NAK/CAN）必须按当前速率发完再切速，否则线上是半截帧 */
        if (SciPortIsTxSuspended(HW_SCI_ID_A) != 0U)
        {
            (void)SciPortTxResume(HW_SCI_ID_A);
            return;
        }

        if (SciPortIsTxIdle(HW_SCI_ID_A) == 0U)
        {
            return;
        }

        if (Hw_SciSetBaud(HW_SCI_ID_A, BOOT_HS_BAUD_NORMAL) == 0U)
        {
            return;             /* 切不回去就逐轮重试，绝不在 115200 上等 F0 */
        }

        SciPortResync(HW_SCI_ID_A);
        s_u16HsRxLen = 0U;
        s_u16HsState = BOOT_HS_IDLE;
        return;
    }

    if (s_u16HsState == BOOT_HS_F0_TX)
    {
        if (SciPortIsTxIdle(HW_SCI_ID_A) == 0U)
        {
            return;             /* 应答还在环/FIFO/移位寄存器里 */
        }

        if (Hw_SciSetBaud(HW_SCI_ID_A, BOOT_HS_BAUD_UPGRADE) == 0U)
        {
            BootHsEnterRestore();
            return;
        }

        SciPortResync(HW_SCI_ID_A);     /* 丢掉 19200 上的残留 */
        s_u16HsRxLen = 0U;
        s_u32HsTick = Hw_TimeBaseTicks();
        s_u16HsState = BOOT_HS_WAIT_REQ;
        return;
    }

    if (s_u16HsState == BOOT_HS_REQ_TX)
    {
        if (SciPortIsTxIdle(HW_SCI_ID_A) == 0U)
        {
            return;
        }

        if (StartReceiveFromBoot(BOOT_HS_BAUD_CFG) == 0U)
        {
            BootHsEnterRestore();       /* 准备失败：不回 'C'，切回 19200 等重来 */
            return;
        }

        s_u16HsState = BOOT_HS_STOPPED; /* 之后由 TaskUpgrade() 收包 */
        return;
    }

    BootHsRead();

    if (s_u16HsState == BOOT_HS_IDLE)
    {
        if (BootHsMatch(&s_au16HsUpCmd[0], BOOT_HS_CMD_LEN) != 0U)
        {
            if (BootHsSend(&s_au16HsUpReply[0], BOOT_HS_CMD_LEN) != 0U)
            {
                BootHsShift(BOOT_HS_CMD_LEN);
                s_u16HsState = BOOT_HS_F0_TX;
            }
            return;
        }

        /* ARM 自身升级通知：只应答，不切速、不擦 Flash、不写配置、不复位 */
        if (BootHsMatch(&s_au16HsArmCmd[0], BOOT_HS_CMD_LEN) != 0U)
        {
            if (BootHsSend(&s_au16HsArmReply[0], BOOT_HS_CMD_LEN) != 0U)
            {
                BootHsShift(BOOT_HS_CMD_LEN);
            }
            return;
        }

        BootHsTrim(0xF0U, BOOT_HS_CMD_LEN);
        return;
    }

    /* BOOT_HS_WAIT_REQ：只认恰好 6 字节的传输请求 */
    if (BootHsMatch(&s_au16HsReq[0], BOOT_HS_REQ_LEN) != 0U)
    {
        if (BootHsSend(&s_au16HsReqReply[0], BOOT_HS_REPLY_LEN) != 0U)
        {
            BootHsShift(BOOT_HS_REQ_LEN);
            s_u16HsState = BOOT_HS_REQ_TX;
        }
        return;
    }

    if (Hw_TimeBaseElapsed(s_u32HsTick) >=
        Hw_TimeBaseUsToTicks(BOOT_HS_WAIT_MS * 1000UL))
    {
        BootHsEnterRestore();       /* 第二次握手超时：回 19200 重新等 F0 */
        return;
    }

    BootHsTrim(0x01U, BOOT_HS_REQ_LEN);
}


/* 跳 APP 或留在 BOOT：APP 有效才跳，无效则先做救援握手。 */
static void BootToAppOrReceive(void)
{
    if (IsAppValid() != 0U)
    {
        jump_to_app();
    }

    /* APP 无效：先按旧 ARM 协议握手（19200 等 F0），握手成功才准备接收 */
    BootHsStart();
}

/* BOOT 自升级尾部：清配置 -> 擦 A/B/C -> 从备份区回拷 BOOT -> 复位。
 * 全程在 RAM 里跑（.flashfunc），执行期间不再回到 Flash 取指。先清配置，保证中途
 * 掉电也不会留下"已收完"的配置，否则下次上电会把半截镜像当成品再回拷一次。 */
/* 自擦后不能回 Flash：验证目标区与驻留都必须跑在 RAM 里。 */
#pragma CODE_SECTION(VerifyBootArea, ".flashfunc")
static Uint16 VerifyBootArea(void)
{
    Uint32 u32Crc;

    u32Crc = UpgradeCrc32((Uint16 *)0x080000U,
                          stUpgradeHandle.stUpgradeCfg.u32FileSize >> 1,
                          0xFFFFFFFFUL);
    if (u32Crc == stUpgradeHandle.stUpgradeCfg.u32FileCrc)
    {
        return 1U;
    }

    return 0U;
}

/* 已经擦掉 BOOT、但回拷失败：BOOT 镜像没了，绝不能复位回 Flash 取指。
 * 留在 RAM 里喂狗，等调试器重新烧写；本函数只在 .flashfunc 里调用。 */
#pragma CODE_SECTION(StallAfterBootErase, ".flashfunc")
static void StallAfterBootErase(void)
{
    while (1)
    {
        FlashFeedWatchDog();
    }
}

#pragma CODE_SECTION(BootSelfUpdateTail, ".flashfunc")
static void BootSelfUpdateTail(void)
{
    Uint16 u16Retry = 0U;

    /* 自擦前能验的都验一遍：配置标志、芯片类型、镜像类型、长度、备份区 CRC。
     * 任何一项不成立都不许动 BOOT 区，清掉配置退回正常流程。 */
    if ((GetUpgradeFlag(&stUpgradeHandle) != 1) ||
        (GetUpgradeFirmwareFlag(&stUpgradeHandle) != 1) ||
        (stUpgradeHandle.stUpgradeCfg.bChipType != XMODEM_CHIP_TYPE) ||
        (stUpgradeHandle.stUpgradeCfg.bFirmwareType != 2U) ||
        (stUpgradeHandle.stUpgradeCfg.u32FileSize == 0UL) ||
        ((stUpgradeHandle.stUpgradeCfg.u32FileSize & 1UL) != 0UL) ||
        (stUpgradeHandle.stUpgradeCfg.u32FileSize > XMODEM_BOOT_MAX_BYTES) ||
        (VerifyFireware(&stUpgradeHandle) != 1))
    {
        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        return;
    }

    /* 先清配置，并且必须清成功：清了才不会被下次上电当成"已收完"再装一遍；
     * 清不掉就停在 BOOT（下次上电还会走到这里重试），绝不动 A/B/C。 */
    if (EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr) != 0)
    {
        return;
    }

    /* 从这里开始 BOOT 区可能已被破坏：等待 FSM 超时也不许复位回 Flash。 */
    FlashSetStallOnTimeout(1U);

    /* 每轮重试都先擦目标扇区再编程：同一 64 位 ECC 单元不能被重复编程。 */
    for (u16Retry = 0U; u16Retry < 3U; u16Retry++)
    {
        if (EraseBootBank() != 0)
        {
            StallAfterBootErase();
        }

        if ((CopyBootFirmware(&stUpgradeHandle) == 0) && (VerifyBootArea() != 0U))
        {
            /* 回拷与目标校验都过了才清备份；之后复位，由新 BOOT 决定是否跳 APP */
            (void)EraseBackUpBank();
            FlashForceReset();
        }
    }

    StallAfterBootErase();

    while (1)
    {
    }
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
    int16 i16Temp = 0;

    i16Temp = ReadUpgradeCfg(&stUpgradeHandle);
    if (i16Temp == 0)
    {
        BootToAppOrReceive();
        return;
    }

    if (GetUpgradeFirmwareFlag(&stUpgradeHandle) == 0)
    {
        /* "接收中"：只校验接收参数，然后准备好门控继续收包。若用完整镜像的规则来判，
         * 芯片类型/镜像类型/长度都还是 0 的合法接收记录会被当成坏配置清掉。 */
        /* APP 无效时不能直接续收：旧 ARM 要先完成 19200/115200 两段握手，
         * 否则既等不到 'C'，也不会从首包重传。清掉旧记录，从 19200 重新等 F0。 */
        if (IsAppValid() == 0U)
        {
            (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
            BootHsStart();
            return;
        }

        if ((IsReceiveCfgValid() != 0U) && (PrepareResumeReceive() != 0U))
        {
            s_u16ReceiveReady = 1U;
            return;
        }

        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        BootToAppOrReceive();
        return;
    }

    /* "已收完"：按完整镜像校验，再做备份区 CRC（参考实现直接拿 u32FileSize 算 CRC）。 */
    if (IsImageCfgValid() == 0U)
    {
        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        BootToAppOrReceive();
        return;
    }

    if (VerifyFireware(&stUpgradeHandle) != 1)
    {
        (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
        BootToAppOrReceive();
        return;
    }

    if (stUpgradeHandle.stUpgradeCfg.bFirmwareType == XMODEM_FIRMWARE_APP)
    {
        /* 擦除 → 回拷 → 目标区 CRC 三步全过才清配置跳转；失败保留"已收完"配置
         * （下次上电重试安装）并停在 BOOT。 */
        if ((EraseAppBank() != 0) ||
            (CopyAppFirmware(&stUpgradeHandle) != 0) ||
            (VerifyAppArea(&stUpgradeHandle) != 1))
        {
            return;
        }

        /* 回拷后新 APP 的入口还必须有效：装进去却跳不进去等于变砖 */
        if (IsAppValid() == 0U)
        {
            return;
        }

        /* 清配置也要看结果：清不掉的话下次上电会重复安装一遍。 */
        if (EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr) != 0)
        {
            return;
        }

        /* 安装完成：备份区不再需要，擦掉可避免下次上电把同一镜像再装一次 */
        (void)EraseBackUpBank();

        BootToAppOrReceive();
        return;
    }

    /* FirmwareType == 2：自升级尾部在 RAM 里一次做完（成功复位 / 失败驻留） */
    BootSelfUpdateTail();
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

    /* 门控：接收准备（擦备份区 + 写接收配置）没成功就不碰 XmodemReceive()。 */
    if (s_u16ReceiveReady == 0U)
    {
        return 0U;
    }

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

/* 一次升级会话被中止（两次 CAN、CRC 不符、写 Flash 失败、重试超限）后的收尾：
 * 清会话与适配层残留，APP 有效就启动它，APP 空白则重新准备接收。 */
void UpgradeUserAbort(void)
{
    s_u16ReceiveReady = 0U;
    UpgradeUserSessionReset();
    XmodemProcessInit();
    XmodemRxReset();

    if (IsAppValid() != 0U)
    {
        jump_to_app();
    }

    /* APP 无效：会话中止后回到 19200 等 F0，旧 ARM 可以重新握手并从首包重传 */
    BootHsEnterRestore();
}

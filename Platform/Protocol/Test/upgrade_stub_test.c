/******************************************************************************
 * @file    upgrade_stub_test.c
 * @brief   05F 升级链路主机测试（XMODEM 接收 + 升级会话 + 备份区写入）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    本文件在主机上运行（gcc/clang），不需要 DSP。它把四个边界都换成替身：
 *          SciPort（收发）、Hw_TimeBase（时间）、Flash（RAM 数组）与
 *          AppModbusReleasePort / SysCtl_resetDevice（复位与端口交还）。
 *
 * @note    断言分 19 组，对应申请文档 §11 的 12 组 + 复核补充的 4 组，再加三组
 *          打包/配置边界。
 *
 * 编译（在仓库根目录）：
 *   cc -std=c99 -O2 -Wall -Wextra \
 *      -I Platform/Protocol/Test/shim -I Platform/Port/Test/shim \
 *      -I Platform/Port/Inc -I Platform/Protocol/Inc -I Platform/Common/Inc \
 *      -I Platform/Protocol/Xmodem/Inc -I Platform/Upgrade/Inc -I Drivers/inc \
 *      -I UserApp/Inc \
 *      Platform/Protocol/Test/upgrade_stub_test.c \
 *      Platform/Protocol/Xmodem/Src/XmodemCore.c \
 *      Platform/Upgrade/Src/UpgradeCore.c \
 *      UserApp/Src/AppUpgrade.c -o upgrade_stub_test
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "XmodemCore.h"
#include "UpgradeCore.h"
#include "AppUpgrade.h"
#include "SciPort.h"
#include "Hw_TimeBase.h"
#include "Hw_SciDriver.h"

/* ---------------- 检查框架 ---------------- */

static int s_iFail;
static int s_iCount;

static void Check(int iCond, const char *pszName)
{
    s_iCount++;

    if (iCond == 0)
    {
        s_iFail++;
        (void)printf("FAIL  %s\n", pszName);
    }
}

/* ---------------- 时间替身 ---------------- */

static Uint32 s_u32NowUs = 1000UL;

Uint32 Hw_TimeBaseTicks(void)
{
    return s_u32NowUs;
}

Uint16 Hw_TimeBaseIsReady(void)
{
    return 1U;
}

Uint32 Hw_TimeBaseUsToTicks(Uint32 u32Us)
{
    return u32Us;
}

Uint32 Hw_TimeBaseElapsed(Uint32 u32Start)
{
    return (Uint32)(u32Start - s_u32NowUs);
}

/* ---------------- 看门狗 / 复位替身 ---------------- */

static Uint32 s_u32FeedCount;

void Hw_EnableWatchDog(void)
{
}

void Hw_FeedWatchDog(void)
{
    s_u32FeedCount++;
}

static Uint32 s_u32ResetCount;

void SysCtl_resetDevice(void)
{
    s_u32ResetCount++;
}

/* ---------------- 端口交还替身 ---------------- */

static Uint32 s_u32ReleaseCount;
static Uint16 s_u16ReleasePort;

void AppModbusReleasePort(Uint16 u16Port)
{
    s_u32ReleaseCount++;
    s_u16ReleasePort = u16Port;
}

/* ---------------- SciPort 替身 ---------------- */

#define STUB_RX_DEPTH (128UL * 1024UL)
#define STUB_TX_DEPTH (512U)

static Uint16 s_au16Rx[STUB_RX_DEPTH];
static Uint32 s_u32RxLen;
static Uint32 s_u32RxRd;
static Uint16 s_u16RxChunk = 8U;          /* 每次 PopBatch 最多给出多少字节 */
static Uint16 s_au16Tx[STUB_TX_DEPTH];
static Uint16 s_u16TxLen;
static Uint16 s_u16TxBusy;                /* 1 = 端口暂时写不进去 */
static Uint32 s_u32ResyncCount;
static Uint16 s_u16TxSuspend;

Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max, ST_SCI_RX_SEG *pstSeg)
{
    Uint32 u32Left;
    Uint16 u16Got;
    Uint16 i;

    (void)u16PortId;

    if ((pu16Dst == 0) || (pstSeg == 0) || (u16Max == 0U))
    {
        return 0U;
    }

    u32Left = s_u32RxLen - s_u32RxRd;
    if (u32Left == 0UL)
    {
        return 0U;
    }

    u16Got = s_u16RxChunk;
    if ((Uint32)u16Got > u32Left)
    {
        u16Got = (Uint16)u32Left;
    }

    /* 一条记录装不进给的容量时，真实实现返回 0 且记录不丢 */
    if (u16Got > u16Max)
    {
        return 0U;
    }

    for (i = 0U; i < u16Got; i++)
    {
        pu16Dst[i] = s_au16Rx[s_u32RxRd + i];
    }

    s_u32RxRd = s_u32RxRd + (Uint32)u16Got;

    pstSeg->u32Tick = s_u32NowUs;
    pstSeg->u32TickEmptyLast = s_u32NowUs;
    pstSeg->u16SeqStart = (Uint16)s_u32RxRd;
    pstSeg->u16Count = u16Got;
    pstSeg->u16EmptyRounds = 0U;

    return u16Got;
}

Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 i;

    (void)u16PortId;

    if ((pu16Data == 0) || (u16Count == 0U) || (s_u16TxBusy != 0U))
    {
        return 0U;
    }

    if (((Uint32)s_u16TxLen + (Uint32)u16Count) > (Uint32)STUB_TX_DEPTH)
    {
        return 0U;
    }

    for (i = 0U; i < u16Count; i++)
    {
        s_au16Tx[s_u16TxLen] = pu16Data[i];
        s_u16TxLen++;
    }

    return u16Count;
}

Uint16 SciPortTxFree(Uint16 u16PortId)
{
    (void)u16PortId;
    return (Uint16)(STUB_TX_DEPTH - s_u16TxLen);
}

Uint16 SciPortIsTxIdle(Uint16 u16PortId)
{
    (void)u16PortId;
    return (s_u16TxLen == 0U) ? 1U : 0U;
}

Uint16 SciPortIsTxSuspended(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16TxSuspend;
}

Uint16 SciPortTxResume(Uint16 u16PortId)
{
    (void)u16PortId;
    s_u16TxSuspend = 0U;
    return 1U;
}

Uint16 SciPortRxState(Uint16 u16PortId)
{
    (void)u16PortId;
    return SCI_RX_IDLE;
}

void SciPortResync(Uint16 u16PortId)
{
    (void)u16PortId;
    s_u32ResyncCount++;
}

Uint16 SciPortIsStreamBreak(Uint16 u16PortId)
{
    (void)u16PortId;
    return 0U;
}

Uint16 SciPortFaultGeneration(Uint16 u16PortId)
{
    (void)u16PortId;
    return 0U;
}

Uint16 SciPortClearStreamBreak(Uint16 u16PortId, Uint16 u16Generation)
{
    (void)u16PortId;
    (void)u16Generation;
    return 1U;
}

/* ---------------- Flash 替身 ---------------- */

#define STUB_FLASH_BASE  (0x0A0000UL)
#define STUB_FLASH_WORDS (0x1A000UL)

static Uint16 s_au16Flash[STUB_FLASH_WORDS];
static Uint16 s_au16CfgFlash[0x1FF0U];

static Uint32 s_u32EraseCount;
static Uint32 s_u32ProgramCount;
static Uint32 s_u32Misaligned;
static Uint32 s_u32DoubleWrite;
static Uint32 s_u32LastEraseAddr = 0xFFFFFFFFUL;
static Uint16 s_u16FlashFail;            /* 1 = 下一次编程失败 */
static Uint16 s_au16UnitSeen[STUB_FLASH_WORDS / 8U];

static Uint16 StubErase(Uint32 u32SectorStart)
{
    Uint32 i;

    s_u32EraseCount++;
    s_u32LastEraseAddr = u32SectorStart;

    if ((u32SectorStart >= STUB_FLASH_BASE) &&
        (u32SectorStart < (STUB_FLASH_BASE + STUB_FLASH_WORDS)))
    {
        for (i = 0UL; i < STUB_FLASH_WORDS; i++)
        {
            if (((STUB_FLASH_BASE + i) >= u32SectorStart) &&
                ((STUB_FLASH_BASE + i) < (u32SectorStart + 0x8000UL)))
            {
                s_au16Flash[i] = 0xFFFFU;
            }
        }
    }
    else
    {
        for (i = 0UL; i < 0x1FF0UL; i++)
        {
            s_au16CfgFlash[i] = 0xFFFFU;
        }
    }

    return 1U;
}

static Uint16 StubProgram(Uint32 u32Addr, const Uint16 *pu16Data, Uint16 u16Words)
{
    Uint32 u32Index;
    Uint32 u32Unit;
    Uint16 i;

    s_u32ProgramCount++;

    if ((u16Words != 8U) || (((u32Addr - STUB_FLASH_BASE) % 8UL) != 0UL))
    {
        s_u32Misaligned++;
        return 0U;
    }

    if (s_u16FlashFail != 0U)
    {
        s_u16FlashFail = 0U;
        return 0U;
    }

    if (u32Addr >= UPGRADE_CFG_BASE)
    {
        u32Index = (u32Addr - UPGRADE_CFG_BASE) / 2UL;
        for (i = 0U; i < 8U; i++)
        {
            s_au16CfgFlash[u32Index + i] = pu16Data[i];
        }

        return 1U;
    }

    u32Index = (u32Addr - STUB_FLASH_BASE) / 2UL;
    u32Unit = u32Index / 8UL;

    if (s_au16UnitSeen[u32Unit] != 0U)
    {
        s_u32DoubleWrite++;
    }

    s_au16UnitSeen[u32Unit] = 1U;

    for (i = 0U; i < 8U; i++)
    {
        s_au16Flash[u32Index + i] = pu16Data[i];
    }

    return 1U;
}

static Uint16 StubRead(Uint32 u32Addr, Uint16 *pu16Dst, Uint16 u16Words)
{
    Uint32 u32Index;
    Uint16 i;

    if ((u32Addr >= STUB_FLASH_BASE) && (u32Addr < (STUB_FLASH_BASE + STUB_FLASH_WORDS)))
    {
        u32Index = (u32Addr - STUB_FLASH_BASE) / 2UL;
        for (i = 0U; i < u16Words; i++)
        {
            pu16Dst[i] = s_au16Flash[u32Index + i];
        }
    }
    else
    {
        u32Index = (u32Addr - UPGRADE_CFG_BASE) / 2UL;
        for (i = 0U; i < u16Words; i++)
        {
            pu16Dst[i] = s_au16CfgFlash[u32Index + i];
        }
    }

    return 1U;
}

static const ST_UPGRADE_FLASH_OPS s_stFlashOps =
{
    StubErase,
    StubProgram,
    StubRead
};

/* ---------------- CRC 参照实现 ---------------- */

static Uint16 RefCrc16(const Uint16 *pu16Buff, Uint16 u16Len)
{
    Uint16 u16Crc = 0U;
    Uint16 i;
    Uint16 j;

    for (i = 0U; i < u16Len; i++)
    {
        u16Crc = (Uint16)(u16Crc ^ ((Uint16)(pu16Buff[i] & 0xFFU) << 8));

        for (j = 0U; j < 8U; j++)
        {
            if ((u16Crc & 0x8000U) != 0U)
            {
                u16Crc = (Uint16)((Uint16)(u16Crc << 1) ^ 0x1021U);
            }
            else
            {
                u16Crc = (Uint16)(u16Crc << 1);
            }
        }
    }

    return u16Crc;
}

/* ---------------- 测试辅助 ---------------- */

static void StubReset(void)
{
    Uint32 i;

    s_u32RxLen = 0UL;
    s_u32RxRd = 0UL;
    s_u16RxChunk = 8U;
    s_u16TxLen = 0U;
    s_u16TxBusy = 0U;
    s_u16TxSuspend = 0U;
    s_u32ResyncCount = 0UL;
    s_u32ResetCount = 0UL;
    s_u32ReleaseCount = 0UL;
    s_u16ReleasePort = 0xFFFFU;
    s_u32FeedCount = 0UL;
    s_u32EraseCount = 0UL;
    s_u32ProgramCount = 0UL;
    s_u32Misaligned = 0UL;
    s_u32DoubleWrite = 0UL;
    s_u32LastEraseAddr = 0xFFFFFFFFUL;
    s_u16FlashFail = 0U;
    s_u32NowUs = 1000UL;

    for (i = 0UL; i < STUB_FLASH_WORDS; i++)
    {
        s_au16Flash[i] = 0xFFFFU;
        if (i < (STUB_FLASH_WORDS / 8UL))
        {
            s_au16UnitSeen[i] = 0U;
        }
    }

    for (i = 0UL; i < 0x1FF0UL; i++)
    {
        s_au16CfgFlash[i] = 0xFFFFU;
    }
}

static void PushBytes(const Uint16 *pu16Data, Uint16 u16Len)
{
    Uint16 i;

    for (i = 0U; i < u16Len; i++)
    {
        s_au16Rx[s_u32RxLen] = pu16Data[i];
        s_u32RxLen = s_u32RxLen + 1UL;
    }
}

static void TxClear(void)
{
    s_u16TxLen = 0U;
}

static void Advance(Uint32 u32Us)
{
    s_u32NowUs = s_u32NowUs - u32Us;
}

/**
 * @brief 组一个 SOH（128 字节）包
 * @return 包总长 133
 */
static Uint16 BuildSoh(Uint16 u16PackNo, const Uint16 *pu16Data128, Uint16 *pu16Frame)
{
    Uint16 u16Crc;
    Uint16 i;

    pu16Frame[0] = XMODEM_SOH;
    pu16Frame[1] = u16PackNo;
    pu16Frame[2] = (Uint16)(0xFFU - (u16PackNo & 0xFFU));

    for (i = 0U; i < XMODEM_PACK_SOH_DATA_LEN; i++)
    {
        pu16Frame[3 + i] = pu16Data128[i];
    }

    u16Crc = RefCrc16(&pu16Frame[1], (Uint16)(2U + XMODEM_PACK_SOH_DATA_LEN));

    pu16Frame[3 + XMODEM_PACK_SOH_DATA_LEN] = (Uint16)((u16Crc >> 8) & 0xFFU);
    pu16Frame[3 + XMODEM_PACK_SOH_DATA_LEN + 1] = (Uint16)(u16Crc & 0xFFU);

    return (Uint16)(3U + XMODEM_PACK_SOH_DATA_LEN + 2U);
}

/**
 * @brief 组一个 STX（1024 字节）包
 * @return 包总长 1029
 */
static Uint16 BuildStx(Uint16 u16PackNo, const Uint16 *pu16Data, Uint16 *pu16Frame)
{
    Uint16 u16Crc;
    Uint16 i;

    pu16Frame[0] = XMODEM_STX;
    pu16Frame[1] = u16PackNo;
    pu16Frame[2] = (Uint16)(0xFFU - (u16PackNo & 0xFFU));

    for (i = 0U; i < XMODEM_PACK_STX_DATA_LEN; i++)
    {
        pu16Frame[3 + i] = pu16Data[i];
    }

    u16Crc = RefCrc16(&pu16Frame[1], (Uint16)(2U + XMODEM_PACK_STX_DATA_LEN));

    pu16Frame[3 + XMODEM_PACK_STX_DATA_LEN] = (Uint16)((u16Crc >> 8) & 0xFFU);
    pu16Frame[3 + XMODEM_PACK_STX_DATA_LEN + 1] = (Uint16)(u16Crc & 0xFFU);

    return (Uint16)(3U + XMODEM_PACK_STX_DATA_LEN + 2U);
}

/** 32 字节文件头：ChipType / FirmwareType / 长度(小端) / CRC32(小端) */
static void BuildHead(Uint16 *pu16Head, Uint16 u16Chip, Uint16 u16Type, Uint32 u32Size, Uint32 u32Crc)
{
    Uint16 i;

    for (i = 0U; i < UPGRADE_FILE_HEAD_LEN; i++)
    {
        pu16Head[i] = 0U;
    }

    pu16Head[0] = u16Chip;
    pu16Head[1] = u16Type;
    pu16Head[2] = (Uint16)(u32Size & 0xFFUL);
    pu16Head[3] = (Uint16)((u32Size >> 8) & 0xFFUL);
    pu16Head[4] = (Uint16)((u32Size >> 16) & 0xFFUL);
    pu16Head[5] = (Uint16)((u32Size >> 24) & 0xFFUL);
    pu16Head[6] = (Uint16)(u32Crc & 0xFFUL);
    pu16Head[7] = (Uint16)((u32Crc >> 8) & 0xFFUL);
    pu16Head[8] = (Uint16)((u32Crc >> 16) & 0xFFUL);
    pu16Head[9] = (Uint16)((u32Crc >> 24) & 0xFFUL);
}

/** 起动一次升级会话并跑到 RECEIVE（4 个扇区各擦一次） */
static void StartSession(void)
{
    (void)AppUpgradeStart(HW_SCI_ID_A, 0, 0U);
    AppUpgradeTask();      /* HOLD → ERASE */
    AppUpgradeTask();      /* 扇区 H */
    AppUpgradeTask();      /* 扇区 I */
    AppUpgradeTask();      /* 扇区 J */
    AppUpgradeTask();      /* 扇区 K */
    AppUpgradeTask();      /* → RECEIVE */
    AppUpgradeTask();      /* 协议层第一轮，发出 'C' */
}

/** 反复跑升级任务，直到会话回到空闲 */
static void RunUntilIdle(Uint16 u16Times)
{
    Uint16 i;

    for (i = 0U; i < u16Times; i++)
    {
        AppUpgradeTask();
        if (AppUpgradeState() == APP_UP_STATE_IDLE)
        {
            break;
        }
    }
}

/** 反复跑升级任务，直到达到次数或状态不再是 RECEIVE */
static void RunReceive(Uint16 u16Times)
{
    Uint16 i;

    for (i = 0U; i < u16Times; i++)
    {
        AppUpgradeTask();
        if (AppUpgradeState() != APP_UP_STATE_RECEIVE)
        {
            break;
        }
    }
}

/**
 * @brief 成功收尾：先等最终 ACK 写进端口，再假装发完，等会话回到空闲
 * @note  最终 ACK 是"已落盘"的唯一凭据，必须在复位之前真正发出去；
 *        本函数把这两个时刻分开断言。
 */
static void FinishAndCheck(const char *pszAckName, const char *pszDoneName)
{
    RunUntilIdle(20U);

    Check(s_u16TxLen == 1U, pszAckName);
    Check(s_au16Tx[0] == XMODEM_ACK, "final: 最终 ACK = 0x06");

    TxClear();
    RunUntilIdle(10U);

    Check(AppUpgradeDoneCount() == 1UL, pszDoneName);
    Check(s_u32ResetCount == 1UL, "final: 写配置后复位一次");
    Check(s_u32ReleaseCount == 1UL, "final: 端口交还 Modbus");
    Check(AppUpgradeState() == APP_UP_STATE_IDLE, "final: 会话回到空闲");
}

/* ---------------- 1. 基础算法 ---------------- */

static void TestPrimitives(void)
{
    Uint16 au16Buff[9];
    Uint32 u32Crc;
    Uint16 au16Data[9];

    au16Buff[0] = 0x31U;
    au16Buff[1] = 0x32U;
    au16Buff[2] = 0x33U;
    au16Buff[3] = 0x34U;
    au16Buff[4] = 0x35U;
    au16Buff[5] = 0x36U;
    au16Buff[6] = 0x37U;
    au16Buff[7] = 0x38U;
    au16Buff[8] = 0x39U;

    /* CRC-16/XMODEM 的标准校验值：初值 0、poly 0x1021、不反射 */
    Check(RefCrc16(au16Buff, 9U) == 0x31C3U, "prim: CRC16(\"123456789\") = 0x31C3");

    au16Data[0] = 0x31U;
    au16Data[1] = 0x32U;
    au16Data[2] = 0x33U;
    au16Data[3] = 0x34U;
    au16Data[4] = 0x35U;
    au16Data[5] = 0x36U;
    au16Data[6] = 0x37U;
    au16Data[7] = 0x38U;
    au16Data[8] = 0x39U;

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, au16Data, 9U);
    Check(u32Crc == 0x0376E6E7UL, "prim: CRC32(\"123456789\") = 0x0376E6E7");
}

/* ---------------- 2. 文件头 ---------------- */

static void TestHeadChecks(void)
{
    Uint16 au16Head[32];
    ST_UPGRADE_FILE_HEAD stHead;

    (void)UpgradeCoreInit(&s_stFlashOps);

    BuildHead(au16Head, 0x37U, UPGRADE_FW_APP, 0x1000UL, 0x12345678UL);
    Check(UpgradeCoreParseHead(au16Head, &stHead) == UPGRADE_OK, "head: 解析 32 字节头成功");
    Check(stHead.u16ChipType == 0x37U, "head: ChipType = 0x37");
    Check(stHead.u16FirmwareType == UPGRADE_FW_APP, "head: FirmwareType = APP");
    Check(stHead.u32FileSize == 0x1000UL, "head: 长度小端解析 = 0x1000");
    Check(stHead.u32FileCrc == 0x12345678UL, "head: CRC 小端解析 = 0x12345678");
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_OK, "head: 合法头通过校验");

    BuildHead(au16Head, 0x38U, UPGRADE_FW_APP, 0x1000UL, 0x12345678UL);
    (void)UpgradeCoreParseHead(au16Head, &stHead);
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_ERR_HEAD, "head: 机型不符被拒");

    BuildHead(au16Head, 0x37U, 3U, 0x1000UL, 0x12345678UL);
    (void)UpgradeCoreParseHead(au16Head, &stHead);
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_ERR_HEAD, "head: 固件类型非 1/2 被拒");

    BuildHead(au16Head, 0x37U, UPGRADE_FW_APP, 0UL, 0x12345678UL);
    (void)UpgradeCoreParseHead(au16Head, &stHead);
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_ERR_SIZE, "head: 长度为 0 被拒");

    BuildHead(au16Head, 0x37U, UPGRADE_FW_APP, UPGRADE_APP_MAX_BYTES + 2UL, 0x12345678UL);
    (void)UpgradeCoreParseHead(au16Head, &stHead);
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_ERR_SIZE, "head: 超 APP 上限被拒");

    BuildHead(au16Head, 0x37U, UPGRADE_FW_BOOT, UPGRADE_BOOT_MAX_BYTES, 0x12345678UL);
    (void)UpgradeCoreParseHead(au16Head, &stHead);
    Check(UpgradeCoreCheckHead(&stHead, 0x37U) == UPGRADE_OK, "head: BOOT 上限本身可接受");
}

/* ---------------- 3. 正文字节→Flash 字 ---------------- */

static void TestBodyPacking(void)
{
    Uint16 au16Body[5];

    StubReset();
    (void)UpgradeCoreInit(&s_stFlashOps);
    (void)UpgradeCoreStartBody();

    au16Body[0] = 0x12U;
    au16Body[1] = 0x34U;
    au16Body[2] = 0x56U;
    au16Body[3] = 0x78U;
    au16Body[4] = 0x9AU;

    Check(UpgradeCoreWriteBody(au16Body, 5U) == UPGRADE_OK, "pack: 写入 5 字节成功");
    Check(UpgradeCoreBodyWords() == 0UL, "pack: 不满 8 字不下发");
    Check(s_u32ProgramCount == 0UL, "pack: 编程次数仍为 0");

    Check(UpgradeCoreEndBody() == UPGRADE_OK, "pack: 结束写入成功");
    Check(s_u32ProgramCount == 1UL, "pack: 补齐后只编程一次");

    Check(s_au16Flash[0] == 0x3412U, "pack: 前两字节拼成一个字（先到者在低字节）");
    Check(s_au16Flash[1] == 0x7856U, "pack: 第 3、4 字节拼成第二个字");
    Check(s_au16Flash[2] == 0x9AFFU, "pack: 奇数字节末字高字节补 0xFF");
    Check(s_au16Flash[3] == 0xFFFFU, "pack: 剩余补 0xFFFF");
    Check(s_au16Flash[7] == 0xFFFFU, "pack: 补齐到 8 个字");
    Check(UpgradeCoreBodyWords() == 3UL, "pack: 字数 = 3（5 字节 -> 3 个字，补齐不算正文）");
}

/* ---------------- 4. 配置区读写 ---------------- */

static void TestCfgRoundTrip(void)
{
    ST_UPGRADE_CFG stCfg;

    StubReset();
    (void)UpgradeCoreInit(&s_stFlashOps);

    Check(UpgradeCoreCommitCfg(UPGRADE_FW_APP, 0x1234UL, 0xABCDEF01UL) == UPGRADE_OK,
          "cfg: 写入配置成功");
    memset(&stCfg, 0, sizeof(stCfg));
    Check(UpgradeCoreReadCfg(&stCfg) == UPGRADE_OK, "cfg: 回读配置有效");
    Check(stCfg.u16UpgradeFlag == UPGRADE_CFG_FLAG, "cfg: 标志 = 0xAAEE");
    Check(stCfg.u16FirmwareType == UPGRADE_FW_APP, "cfg: 固件类型回读一致");
    Check(stCfg.u32FileSize == 0x1234UL, "cfg: 长度回读一致");
    Check(stCfg.u32FileCrc == 0xABCDEF01UL, "cfg: CRC 回读一致");

    Check(UpgradeCoreClearCfg() == UPGRADE_OK, "cfg: 清配置成功");
    memset(&stCfg, 0, sizeof(stCfg));
    (void)UpgradeCoreReadCfg(&stCfg);
    Check(stCfg.u16UpgradeFlag != UPGRADE_CFG_FLAG, "cfg: 清掉后标志不再是 0xAAEE");
}

/* ---------------- 5. 扇区擦除顺序 ---------------- */

static void TestEraseSectors(void)
{
    Uint16 i;

    StubReset();
    (void)UpgradeCoreInit(&s_stFlashOps);

    for (i = 0U; i < UPGRADE_BACKUP_SECTOR_NUM; i++)
    {
        Check(UpgradeCoreEraseBackupSector(i) == UPGRADE_OK, "erase: 单扇区擦除成功");
        Check(s_u32LastEraseAddr == g_astUpgradeBackupSector[i].u32Start,
              "erase: 擦除地址 = 扇区表起点");
    }

    Check(UpgradeCoreEraseBackupSector(UPGRADE_BACKUP_SECTOR_NUM) == UPGRADE_ERR_PARAM,
          "erase: 越界下标被拒");
    Check(s_u32EraseCount == UPGRADE_BACKUP_SECTOR_NUM, "erase: 共擦 4 次");
}

/* ---------------- 6. 正常流程（首包 SOH） ---------------- */

static Uint16 s_au16Packet[1100];

static void TestHappyPath(void)
{
    Uint16 au16Head[32];
    Uint16 au16Data[128];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);

    StartSession();

    Check(AppUpgradeState() == APP_UP_STATE_RECEIVE, "happy: 擦完四个扇区进入接收");
    Check(s_u32EraseCount == 4UL, "happy: 擦除 4 个扇区");
    Check(s_u16TxLen == 1U, "happy: 进入接收后先发 'C'");
    Check(s_au16Tx[0] == XMODEM_CTRLZ, "happy: 'C' = 0x1A");
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    /* 正文 100 字节 */
    for (i = 0U; i < 100U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x10U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 100U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 100UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(80U);

    Check(s_u16TxLen == 1U, "happy: 收完一包回一个 ACK");
    Check(s_au16Tx[0] == XMODEM_ACK, "happy: ACK = 0x06");
    TxClear();

    /* 两次 EOT */
    s_au16Packet[0] = XMODEM_EOT;
    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunReceive(20U);
    Check(s_u16TxLen == 1U, "happy: 首次 EOT 回 NAK");
    Check(s_au16Tx[0] == XMODEM_NAK, "happy: NAK = 0x15");
    TxClear();

    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunUntilIdle(40U);

    FinishAndCheck("happy: 校验通过才发最终 ACK", "happy: 会话以成功结束");

    Check(AppUpgradeBodyBytes() == 100UL, "happy: 正文长度 = 100");
    Check(s_u16ReleasePort == APP_UP_PORT_INDEX, "happy: 交还的是 ARM 口");
    Check(s_u32FeedCount >= 4UL, "happy: 擦除期间喂过狗");
    Check(s_au16Flash[0] == 0x1110U, "happy: 备份区第一个字 = 前两字节");
    Check(s_u32DoubleWrite == 0UL, "happy: 没有重复写同一个 64 位单元");
    Check(s_u32Misaligned == 0UL, "happy: 编程地址都对齐 8 字");
}

/* ---------------- 7. 首包 STX ---------------- */

static void TestFirstStx(void)
{
    Uint16 au16Head[32];
    Uint16 au16Data[1024];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 1024U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 600U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x40U + (i & 0x3FU));
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 600U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 600UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildStx(1U, au16Data, s_au16Packet);
    Check(u16FrameLen == 1029U, "stx: 首包总长 1029");
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(200U);

    Check(AppUpgradeBodyBytes() == 600UL, "stx: 首包 STX 的正文长度 = 600");

    s_au16Packet[0] = XMODEM_EOT;
    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunReceive(20U);
    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunUntilIdle(40U);

    FinishAndCheck("stx: 校验通过才发最终 ACK", "stx: STX 首包同样能完成");
}

/* ---------------- 8. 重复包 ---------------- */

static void TestDuplicate(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 32U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x20U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 32U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 32UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(80U);
    Check(s_u16TxLen == 1U, "dup: 首包回 ACK");
    TxClear();

    PushBytes(s_au16Packet, u16FrameLen);
    Advance(100U);
    RunReceive(80U);

    Check(s_u16TxLen == 1U, "dup: 重复包仍回 ACK");
    Check(s_au16Tx[0] == XMODEM_ACK, "dup: 重复包 ACK = 0x06");
    Check(AppUpgradeBodyBytes() == 32UL, "dup: 正文没有重复累加");
}

/* ---------------- 9. 序号错 ---------------- */

static void TestSequence(void)
{
    Uint16 au16Data[128];
    Uint16 u16FrameLen;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = (Uint16)(0x30U + (i & 0x0FU));
    }

    u16FrameLen = BuildSoh(2U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(80U);

    Check(s_u16TxLen == 1U, "seq: 首包不是 1 时不写正文");
    Check(s_au16Tx[0] == XMODEM_NAK, "seq: 序号错回 NAK");
    Check(AppUpgradeBodyBytes() == 0UL, "seq: 正文长度仍为 0");
}

/* ---------------- 10. CRC 错后重传 ---------------- */

static void TestCrcRetry(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;
    Uint16 u16Bad;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 16U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x50U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 16U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 16UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    u16Bad = s_au16Packet[10];
    s_au16Packet[10] = (Uint16)(u16Bad ^ 0x01U);

    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(80U);
    Check(s_u16TxLen == 1U, "crc: 坏包回一个控制字节");
    Check(s_au16Tx[0] == XMODEM_NAK, "crc: 坏包回 NAK");
    Check(AppUpgradeBodyBytes() == 0UL, "crc: 坏包不写正文");
    TxClear();

    s_au16Packet[10] = u16Bad;
    PushBytes(s_au16Packet, u16FrameLen);
    Advance(100U);
    RunReceive(80U);

    Check(s_u16TxLen == 1U, "crc: 重传好包回 ACK");
    Check(s_au16Tx[0] == XMODEM_ACK, "crc: 重传 OK");
    Check(AppUpgradeBodyBytes() == 16UL, "crc: 重传后正文 = 16");
}

/* ---------------- 11. 分批到达（每次 1 字节） ---------------- */

static void TestChunked(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    s_u16RxChunk = 1U;

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 8U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x60U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 8U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 8UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(400U);

    Check(AppUpgradeBodyBytes() == 8UL, "chunk: 逐字节到达也能拼成整包");
    Check(s_u16TxLen == 1U, "chunk: 只回一个 ACK");
}

/* ---------------- 12. 控制字节写不进去 ---------------- */

static void TestTxBusy(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);

    s_u16TxBusy = 1U;
    (void)AppUpgradeStart(HW_SCI_ID_A, 0, 0U);
    AppUpgradeTask();      /* HOLD → ERASE */
    AppUpgradeTask();      /* 扇区 H */
    AppUpgradeTask();      /* 扇区 I */
    AppUpgradeTask();      /* 扇区 J */
    AppUpgradeTask();      /* 扇区 K */
    AppUpgradeTask();      /* → RECEIVE */
    AppUpgradeTask();      /* 试发 'C'，端口忙，状态不推进 */

    Check(s_u16TxLen == 0U, "busy: 端口忙时 'C' 一个字节都没写进去");
    Check(AppUpgradeState() == APP_UP_STATE_RECEIVE, "busy: 状态仍停在接收（没当成已发送）");

    s_u16TxBusy = 0U;
    AppUpgradeTask();
    Check(s_u16TxLen == 1U, "busy: 端口空出来以后 'C' 才补发出去");
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 4U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x70U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 4U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 4UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);

    s_u16TxBusy = 1U;
    RunReceive(80U);
    Check(s_u16TxLen == 0U, "busy: ACK 发不出去时没被丢掉");
    s_u16TxBusy = 0U;
    AppUpgradeTask();
    Check(s_u16TxLen == 1U, "busy: 端口恢复后 ACK 补发");
    Check(s_au16Tx[0] == XMODEM_ACK, "busy: 补发的是 ACK");
}

/* ---------------- 13. Flash 失败不 ACK ---------------- */

static void TestFlashFail(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint32 u32Crc;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 32U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x80U + i);
    }

    u32Crc = UpgradeCoreCrc32(0xFFFFFFFFUL, &au16Data[32], 32U);
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 32UL, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);

    s_u16FlashFail = 1U;
    RunUntilIdle(40U);

    Check(s_u16TxLen >= 1U, "flashfail: 编程失败要有回应");
    Check(s_au16Tx[0] != XMODEM_ACK, "flashfail: 编程失败不能回 ACK");
}

/* ---------------- 14. 取消（两次 CAN） ---------------- */

static void TestCancel(void)
{
    Uint16 au16Can[2];

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    au16Can[0] = XMODEM_CAN;
    au16Can[1] = XMODEM_CAN;

    PushBytes(au16Can, 1U);
    RunReceive(20U);
    Check(s_u16TxLen == 1U, "cancel: 第一次 CAN 回 NAK");
    Check(s_au16Tx[0] == XMODEM_NAK, "cancel: NAK = 0x15");
    TxClear();

    PushBytes(&au16Can[1], 1U);
    Advance(100U);
    RunUntilIdle(40U);

    Check(AppUpgradeFailCount() == 1UL, "cancel: 记为一次失败");
    Check(AppUpgradeDoneCount() == 0UL, "cancel: 不计成功");
    Check(s_u16TxLen == 2U, "cancel: 取消要连发两个 CAN");
    Check(s_au16Tx[0] == XMODEM_CAN, "cancel: 第一个 CAN");
    Check(s_au16Tx[1] == XMODEM_CAN, "cancel: 第二个 CAN");
    Check(s_u32ReleaseCount == 1UL, "cancel: 端口交还 Modbus");
    Check(s_u32ResetCount == 0UL, "cancel: 取消后不复位");
    Check(AppUpgradeState() == APP_UP_STATE_IDLE, "cancel: 状态回到空闲");
}

/* ---------------- 15. 长度或 CRC 不符 ---------------- */

static void TestSizeMismatch(void)
{
    Uint16 au16Data[128];
    Uint16 au16Head[32];
    Uint16 u16FrameLen;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();
    TxClear();

    for (i = 0U; i < 128U; i++)
    {
        au16Data[i] = 0U;
    }

    for (i = 0U; i < 16U; i++)
    {
        au16Data[32 + i] = (Uint16)(0x90U + i);
    }

    /* 头里写 64 字节，实际只发 16 字节 */
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, 64UL, 0x11223344UL);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    u16FrameLen = BuildSoh(1U, au16Data, s_au16Packet);
    PushBytes(s_au16Packet, u16FrameLen);
    RunReceive(80U);
    TxClear();

    s_au16Packet[0] = XMODEM_EOT;
    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunReceive(20U);
    PushBytes(s_au16Packet, 1U);
    Advance(100U);
    RunUntilIdle(40U);

    Check(s_u16TxLen >= 1U, "mismatch: 结束阶段有回应");
    Check(s_au16Tx[0] != XMODEM_ACK, "mismatch: 长度/CRC 不符不能回 ACK");
    Check(AppUpgradeFailCount() == 1UL, "mismatch: 记一次失败");
    Check(s_u32ResetCount == 0UL, "mismatch: 不复位");
}

/* ---------------- 16. 等首包超时重发 'C' ---------------- */

static void TestStartTimeout(void)
{
    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);
    StartSession();

    Check(s_u16TxLen == 1U, "start: 先发第一个 'C'");
    TxClear();

    Advance(2100U);
    RunReceive(5U);

    Check(s_u16TxLen == 1U, "start: 超时后重发 'C'");
    Check(s_au16Tx[0] == XMODEM_CTRLZ, "start: 重发的仍是 'C'");
}

/* ---------------- 17. 超过 64 KiB 且序号回绕 ---------------- */

static Uint16 s_au16BigBody[0x18000];
static Uint16 s_au16BigPacket[1100];

static void TestBigFile(void)
{
    Uint32 u32Total = 0x18000UL;          /* 98,304 字节，跨 64 KiB */
    Uint32 u32Crc = 0xFFFFFFFFUL;
    Uint32 u32Off;
    Uint16 au16Head[32];
    Uint16 au16Data[1024];
    Uint16 u16PackNo = 2U;
    Uint16 u16FrameLen;
    Uint16 u16Take;
    Uint16 i;

    StubReset();
    AppUpgradeInit();
    AppUpgradeSetFlashOps(&s_stFlashOps);

    for (u32Off = 0UL; u32Off < u32Total; u32Off++)
    {
        s_au16BigBody[u32Off] = (Uint16)(u32Off & 0xFFUL);
    }

    /* CRC32 只有 16 位长度参数，按 4096 字节一段累加 */
    u32Off = 0UL;
    while (u32Off < u32Total)
    {
        u16Take = (Uint16)(((u32Total - u32Off) > 4096UL) ? 4096UL : (u32Total - u32Off));
        u32Crc = UpgradeCoreCrc32(u32Crc, &s_au16BigBody[u32Off], u16Take);
        u32Off = u32Off + (Uint32)u16Take;
    }

    StartSession();
    TxClear();

    /* 首包用 STX：头 32 字节 + 正文 992 字节 */
    BuildHead(au16Head, APP_UP_CHIP_TYPE, UPGRADE_FW_APP, u32Total, u32Crc);

    for (i = 0U; i < 32U; i++)
    {
        au16Data[i] = au16Head[i];
    }

    for (i = 0U; i < 992U; i++)
    {
        au16Data[32 + i] = s_au16BigBody[i];
    }

    u16FrameLen = BuildStx(1U, au16Data, s_au16BigPacket);
    PushBytes(s_au16BigPacket, u16FrameLen);
    RunReceive(400U);
    TxClear();

    u32Off = 992UL;
    while (u32Off < u32Total)
    {
        u16Take = (Uint16)(((u32Total - u32Off) > 128UL) ? 128UL : (u32Total - u32Off));

        for (i = 0U; i < 128U; i++)
        {
            au16Data[i] = 0U;
        }

        for (i = 0U; i < u16Take; i++)
        {
            au16Data[i] = s_au16BigBody[u32Off + (Uint32)i];
        }

        u16FrameLen = BuildSoh(u16PackNo, au16Data, s_au16BigPacket);
        PushBytes(s_au16BigPacket, u16FrameLen);
        RunReceive(200U);

        u32Off = u32Off + (Uint32)u16Take;
        u16PackNo = (Uint16)((u16PackNo + 1U) & 0xFFU);
    }

    Check(AppUpgradeBodyBytes() == u32Total, "big: 98,304 字节全部收下");
    Check(AppUpgradeState() == APP_UP_STATE_RECEIVE, "big: 仍在接收状态");

    s_au16BigPacket[0] = XMODEM_EOT;
    PushBytes(s_au16BigPacket, 1U);
    Advance(100U);
    RunReceive(20U);
    PushBytes(s_au16BigPacket, 1U);
    Advance(100U);
    RunUntilIdle(40U);

    FinishAndCheck("big: 校验通过才发最终 ACK", "big: 完成");
    Check(AppUpgradeBodyBytes() == u32Total, "big: 正文长度 = 98,304");
    Check(s_u32DoubleWrite == 0UL, "big: 大文件也没有重复写单元");
    Check(s_au16Flash[0] == (Uint16)((0x01U << 8) | 0x00U), "big: 备份区首字 = 正文前两字节");
}

/* ---------------- 主体 ---------------- */

int main(void)
{
    (void)printf("upgrade_stub_test\n");

    TestPrimitives();
    TestHeadChecks();
    TestBodyPacking();
    TestCfgRoundTrip();
    TestEraseSectors();
    TestHappyPath();
    TestFirstStx();
    TestDuplicate();
    TestSequence();
    TestCrcRetry();
    TestChunked();
    TestTxBusy();
    TestFlashFail();
    TestCancel();
    TestSizeMismatch();
    TestStartTimeout();
    TestBigFile();

    (void)printf("upgrade_stub_test: %d 项检查，%d 项失败\n", s_iCount, s_iFail);

    return (s_iFail == 0) ? 0 : 1;
}

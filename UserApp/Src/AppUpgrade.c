/******************************************************************************
 * @file    AppUpgrade.c
 * @brief   APP 侧升级会话实现
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    一条会话的时序：
 *          握手 6 字节 → Modbus 回 8 字节 → 本层接管端口 → 擦 H/I/J/K
 *          → 发 'C' → XMODEM 收包（每包写完再 ACK）→ EOT 两次
 *          → 校验长度/CRC → 写配置并回读 → 发最终 ACK → 等排空 → 复位。
 *          任何一步失败：发 CAN CAN、清配置、把端口交还 Modbus，不复位。
 ******************************************************************************/

#include "AppUpgrade.h"
#include "AppModbus.h"
#include "XmodemCore.h"
#include "UpgradeCore.h"
#include "UpgradeFlash.h"
#include "SciPort.h"
#include "Hw_TimeBase.h"
#include "Hw_WatchDog.h"
#include "Hw_SciDriver.h"
#include "sysctl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 常量 ---------------- */

/** 端口层一条批次最多 8 字节，与本层取批缓冲一致 */
#define APP_UP_BATCH_MAX            (8U)

/** 备份区扇区个数（H/I/J/K） */
#define APP_UP_SECTOR_NUM           (4U)

/* ---------------- 类型 ---------------- */

typedef struct
{
    Uint16                  u16State;       /* APP_UP_STATE_* */
    Uint16                  u16SciId;       /* 端口（硬件 SCI id） */
    Uint16                  u16EraseIndex;  /* 当前擦到第几个扇区 */
    Uint16                  bHeadOk;        /* 文件头已收且校验通过 */
    Uint16                  u16LastErr;     /* 结束原因 */
    Uint32                  u32BodyBytes;   /* 已写入备份区的正文字节数 */
    Uint16                  au16Tail[APP_UP_TAIL_MAX];
    Uint16                  u16TailLen;
    Uint16                  u16TailOff;
    Uint16                  au16Stage[APP_UP_BATCH_MAX];
    Uint16                  u16StageLen;
    Uint16                  u16StageOff;
    ST_UPGRADE_FILE_HEAD    stHead;
} ST_APP_UP;

/* ---------------- 数据 ---------------- */

static ST_APP_UP s_stUp;

/* 默认用 F021 实现；主机测试通过 AppUpgradeSetFlashOps() 换成替身 */
static const ST_UPGRADE_FLASH_OPS *s_pstFlashOps = &g_stUpgradeFlashOps;

#pragma DATA_SECTION(s_stXmodem, ".upybuf");
static ST_XMODEM_PROTOCOL s_stXmodem;

static Uint32 s_u32DoneCount;
static Uint32 s_u32FailCount;
static void (*s_pfPauseHook)(void);
static void (*s_pfResumeHook)(void);

/** 配置好的一组回调，Init 里一次性绑定 */
static const ST_XMODEM_CONFIG s_stXmodemCfg =
{
    1U,                             /* 用 CRC16 */
    XMODEM_DEFAULT_MAX_RETRY,
    XMODEM_DEFAULT_TIMEOUT_MS,
    XMODEM_DEFAULT_START_MS
};

/* ---------------- 传输层 ---------------- */

/**
 * @brief  从端口取字节（先交交接时留下的尾部字节）
 */
static Uint16 AppUpRead(Uint16 *pu16Buff, Uint16 u16Max)
{
    Uint16 u16Got = 0U;
    Uint16 i;
    Uint16 au16Batch[APP_UP_BATCH_MAX];
    ST_SCI_RX_SEG stSeg;

    if ((pu16Buff == 0) || (u16Max == 0U))
    {
        return 0U;
    }

    while ((s_stUp.u16TailOff < s_stUp.u16TailLen) && (u16Got < u16Max))
    {
        pu16Buff[u16Got] = s_stUp.au16Tail[s_stUp.u16TailOff];
        s_stUp.u16TailOff++;
        u16Got++;
    }

    if (u16Got != 0U)
    {
        return u16Got;
    }

    /* 一条批次可能比本次请求还长（协议层拼包时空间会越来越小），
     * 所以先放在暂存里，能取多少给多少。 */
    while ((s_stUp.u16StageOff < s_stUp.u16StageLen) && (u16Got < u16Max))
    {
        pu16Buff[u16Got] = s_stUp.au16Stage[s_stUp.u16StageOff];
        s_stUp.u16StageOff++;
        u16Got++;
    }

    if (u16Got != 0U)
    {
        return u16Got;
    }

    /* 暂存空了才取新批次：一条记录最多 8 字，装不进就不取，
     * 免得 PopBatch 返回 0 被当成"对端没有数据"。 */
    u16Got = SciPortPopBatch(s_stUp.u16SciId, au16Batch, APP_UP_BATCH_MAX, &stSeg);
    if (u16Got == 0U)
    {
        return 0U;
    }

    s_stUp.u16StageLen = u16Got;
    s_stUp.u16StageOff = 0U;
    for (i = 0U; i < u16Got; i++)
    {
        s_stUp.au16Stage[i] = au16Batch[i];
    }

    u16Got = 0U;
    while ((s_stUp.u16StageOff < s_stUp.u16StageLen) && (u16Got < u16Max))
    {
        pu16Buff[u16Got] = s_stUp.au16Stage[s_stUp.u16StageOff];
        s_stUp.u16StageOff++;
        u16Got++;
    }

    return u16Got;
}

/**
 * @brief  向端口写字节；返回 0 表示端口忙，必须下次重试
 */
static Uint16 AppUpWrite(const Uint16 *pu16Buff, Uint16 u16Count)
{
    if ((pu16Buff == 0) || (u16Count == 0U))
    {
        return 0U;
    }

    if (SciPortWriteTx(s_stUp.u16SciId, pu16Buff, u16Count) == 0U)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief  毫秒时基（用 Hw_TimeBase 的 SYSCLK 计数换算）
 */
static Uint32 AppUpNowMs(void)
{
    Uint32 u32PerMs = Hw_TimeBaseUsToTicks(1000U);

    if (u32PerMs == 0U)
    {
        return 0U;
    }

    return Hw_TimeBaseTicks() / u32PerMs;
}

/* ---------------- 数据层 ---------------- */

/**
 * @brief  一包正文落进备份区
 * @return >=0 实际接收的字节数，<0 落盘失败
 */
static int16 AppUpWriteBlock(Uint16 u16PackNo, Uint16 *pu16Buff, Uint16 u16Length)
{
    Uint16 u16Offset = 0U;
    Uint16 u16Len = u16Length;
    Uint32 u32Remain;
    Uint16 u16Ret;

    if ((pu16Buff == 0) || (u16Length == 0U))
    {
        return -1;
    }

    if (u16PackNo == 1U)
    {
        if (u16Length < XMODEM_FILE_HEAD_LEN)
        {
            s_stUp.u16LastErr = UPGRADE_ERR_HEAD;
            return -1;
        }

        (void)UpgradeCoreParseHead(&pu16Buff[0], &s_stUp.stHead);

        u16Ret = UpgradeCoreCheckHead(&s_stUp.stHead, APP_UP_CHIP_TYPE);
        if (u16Ret != UPGRADE_OK)
        {
            s_stUp.u16LastErr = u16Ret;
            return -1;
        }

        u16Offset = XMODEM_FILE_HEAD_LEN;
        u16Len = (Uint16)(u16Length - XMODEM_FILE_HEAD_LEN);
        s_stUp.bHeadOk = 1U;
    }
    else if (s_stUp.bHeadOk == 0U)
    {
        s_stUp.u16LastErr = UPGRADE_ERR_STATE;
        return -1;
    }

    /* 最后一包带填充：只写真实剩余长度 */
    u32Remain = s_stUp.stHead.u32FileSize - s_stUp.u32BodyBytes;
    if ((Uint32)u16Len > u32Remain)
    {
        u16Len = (Uint16)u32Remain;
    }

    if (u16Len == 0U)
    {
        return 0;
    }

    u16Ret = UpgradeCoreWriteBody(&pu16Buff[u16Offset], u16Len);
    if (u16Ret != UPGRADE_OK)
    {
        s_stUp.u16LastErr = u16Ret;
        return -1;
    }

    s_stUp.u32BodyBytes = s_stUp.u32BodyBytes + (Uint32)u16Len;

    return (int16)u16Len;
}

/* ---------------- 内部 ---------------- */

/**
 * @brief  发两个 CAN 让主机死心；端口忙就留到下一轮
 * @return 1 已入环，0 还要再来一次
 */
static Uint16 AppUpSendCancel(void)
{
    Uint16 au16Can[2];
    Uint16 u16Ret;

    au16Can[0] = XMODEM_CAN;
    au16Can[1] = XMODEM_CAN;

    u16Ret = AppUpWrite(&au16Can[0], 2U);
    if (u16Ret == 0U)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief  结束会话并把端口交还 Modbus
 */
static void AppUpEndSession(Uint16 bSuccess)
{
    if (s_pfResumeHook != 0)
    {
        s_pfResumeHook();
    }

    s_stUp.u16State = APP_UP_STATE_IDLE;

    /* 清掉本次协议残留，让 Modbus 从干净状态重新握手 */
    AppModbusReleasePort(APP_UP_PORT_INDEX);
    XmodemReset(&s_stXmodem);

    if (bSuccess != 0U)
    {
        s_u32DoneCount++;
    }
    else
    {
        s_u32FailCount++;
    }
}

/* ---------------- 对外接口 ---------------- */

void AppUpgradeInit(void)
{
    ST_XMODEM_TRANSPORT stTransport;
    ST_XMODEM_DATA_HANDLER stHandler;
    ST_XMODEM_TIMEOUT stTimeout;
    Uint16 i;

    s_stUp.u16State = APP_UP_STATE_IDLE;
    s_stUp.u16SciId = HW_SCI_ID_A;
    s_stUp.u16EraseIndex = 0U;
    s_stUp.bHeadOk = 0U;
    s_stUp.u16LastErr = 0U;
    s_stUp.u32BodyBytes = 0U;
    s_stUp.u16TailLen = 0U;
    s_stUp.u16TailOff = 0U;
    s_stUp.u16StageLen = 0U;
    s_stUp.u16StageOff = 0U;

    for (i = 0U; i < APP_UP_BATCH_MAX; i++)
    {
        s_stUp.au16Stage[i] = 0U;
    }

    for (i = 0U; i < APP_UP_TAIL_MAX; i++)
    {
        s_stUp.au16Tail[i] = 0U;
    }

    stTransport.pfRead = AppUpRead;
    stTransport.pfWrite = AppUpWrite;

    stHandler.pfWriteBlock = AppUpWriteBlock;

    stTimeout.pfNowMs = AppUpNowMs;

    (void)XmodemInit(&s_stXmodem, &s_stXmodemCfg, &stTransport, &stHandler, &stTimeout);

    (void)UpgradeCoreInit(s_pstFlashOps);
}

void AppUpgradeSetFlashOps(const ST_UPGRADE_FLASH_OPS *pstOps)
{
    s_pstFlashOps = (pstOps != 0) ? pstOps : &g_stUpgradeFlashOps;
    (void)UpgradeCoreInit(s_pstFlashOps);
}

Uint16 AppUpgradeStart(Uint16 u16SciId, const Uint16 *pu16Tail, Uint16 u16TailLen)
{
    Uint16 i;

    if (s_stUp.u16State != APP_UP_STATE_IDLE)
    {
        return 0U;
    }

    if (u16SciId != HW_SCI_ID_A)
    {
        return 0U;
    }

    if (u16TailLen > APP_UP_TAIL_MAX)
    {
        return 0U;
    }

    if ((pu16Tail == 0) && (u16TailLen != 0U))
    {
        return 0U;
    }

    s_stUp.u16SciId = u16SciId;
    s_stUp.u16TailLen = u16TailLen;
    s_stUp.u16TailOff = 0U;
    s_stUp.u16StageLen = 0U;
    s_stUp.u16StageOff = 0U;
    for (i = 0U; i < u16TailLen; i++)
    {
        s_stUp.au16Tail[i] = pu16Tail[i];
    }

    s_stUp.u16EraseIndex = 0U;
    s_stUp.bHeadOk = 0U;
    s_stUp.u16LastErr = 0U;
    s_stUp.u32BodyBytes = 0U;

    if (s_pfPauseHook != 0)
    {
        s_pfPauseHook();
    }

    (void)UpgradeCoreStartBody();

    XmodemReset(&s_stXmodem);

    s_stUp.u16State = APP_UP_STATE_HOLD;

    return 1U;
}

void AppUpgradeTask(void)
{
    Uint16 u16Ret;
    E_XMODEM_ERROR eErr;

    switch (s_stUp.u16State)
    {
    case APP_UP_STATE_HOLD:
        /* 等 Modbus 把 8 字节应答发完，端口才开始收包 */
        if (SciPortIsTxIdle(s_stUp.u16SciId) != 0U)
        {
            s_stUp.u16EraseIndex = 0U;
            s_stUp.u16State = APP_UP_STATE_ERASE;
        }
        break;

    case APP_UP_STATE_ERASE:
        if (s_stUp.u16EraseIndex < APP_UP_SECTOR_NUM)
        {
            u16Ret = UpgradeCoreEraseBackupSector(s_stUp.u16EraseIndex);
            if (u16Ret != UPGRADE_OK)
            {
                s_stUp.u16LastErr = u16Ret;
                s_stUp.u16State = APP_UP_STATE_FAIL;
                break;
            }

            s_stUp.u16EraseIndex++;
            Hw_FeedWatchDog();
        }
        else
        {
            /* 四个扇区擦完，'C' 由协议层第一轮自己发 */
            s_stUp.u16State = APP_UP_STATE_RECEIVE;
        }
        break;

    case APP_UP_STATE_RECEIVE:
        eErr = XmodemReceive(&s_stXmodem);
        if (XmodemIsComplete(&s_stXmodem) != 0U)
        {
            if (eErr == XMODEM_END)
            {
                s_stUp.u16State = APP_UP_STATE_FINISH;
            }
            else
            {
                s_stUp.u16LastErr = (Uint16)eErr;
                s_stUp.u16State = APP_UP_STATE_FAIL;
            }
        }
        break;

    case APP_UP_STATE_FINISH:
        /* 先补末块，再比长度与 CRC，最后才写配置 */
        u16Ret = UpgradeCoreEndBody();
        if (u16Ret != UPGRADE_OK)
        {
            s_stUp.u16LastErr = u16Ret;
            s_stUp.u16State = APP_UP_STATE_FAIL;
            break;
        }

        if ((s_stUp.u32BodyBytes != s_stUp.stHead.u32FileSize) ||
            (UpgradeCoreCrc() != s_stUp.stHead.u32FileCrc))
        {
            s_stUp.u16LastErr = UPGRADE_ERR_CRC;
            s_stUp.u16State = APP_UP_STATE_FAIL;
            break;
        }

        u16Ret = UpgradeCoreCommitCfg(s_stUp.stHead.u16FirmwareType,
                                      s_stUp.stHead.u32FileSize,
                                      s_stUp.stHead.u32FileCrc);
        if (u16Ret != UPGRADE_OK)
        {
            s_stUp.u16LastErr = u16Ret;
            s_stUp.u16State = APP_UP_STATE_FAIL;
            break;
        }

        /* 校验通过才给最终 ACK（协议层在第二次 EOT 时故意不发） */
        {
            Uint16 au16Ack = XMODEM_ACK;
            if (AppUpWrite(&au16Ack, 1U) == 0U)
            {
                break;      /* 端口忙，下一轮再发 */
            }
        }

        s_stUp.u16LastErr = 0U;
        s_stUp.u16State = APP_UP_STATE_DONE;
        break;

    case APP_UP_STATE_FAIL:
        /* 取消与"重试用尽"这两条路，协议层已经自己连发过两个 CAN 了，
         * 业务层再发一遍只会让主机多收四个字节；这里只补它没发过的那种
         * （例如文件头非法、写备份失败这类由回调返回错误造成的失败）。 */
        if ((s_stUp.u16LastErr != (Uint16)XMODEM_CANCEL) &&
            (s_stUp.u16LastErr != (Uint16)XMODEM_RETRY_ERROR))
        {
            if (AppUpSendCancel() == 0U)
            {
                break;          /* 端口忙，下一轮再发 */
            }
        }

        (void)UpgradeCoreClearCfg();
        AppUpEndSession(0U);
        break;

    case APP_UP_STATE_DONE:
        /* 最终 ACK 真正出环之后再复位，避免主机收不到确认 */
        if (SciPortIsTxIdle(s_stUp.u16SciId) != 0U)
        {
            AppUpEndSession(1U);
            Hw_FeedWatchDog();
            SysCtl_resetDevice();
        }
        break;

    default:
        break;
    }
}

Uint16 AppUpgradeIsActive(void)
{
    return (s_stUp.u16State == APP_UP_STATE_IDLE) ? 0U : 1U;
}

Uint16 AppUpgradeState(void)
{
    return s_stUp.u16State;
}

Uint32 AppUpgradeBodyBytes(void)
{
    return s_stUp.u32BodyBytes;
}

Uint16 AppUpgradeLastError(void)
{
    return s_stUp.u16LastErr;
}

Uint32 AppUpgradeDoneCount(void)
{
    return s_u32DoneCount;
}

Uint32 AppUpgradeFailCount(void)
{
    return s_u32FailCount;
}

void AppUpgradeSetPauseHook(void (*pfPause)(void), void (*pfResume)(void))
{
    s_pfPauseHook = pfPause;
    s_pfResumeHook = pfResume;
}

#ifdef __cplusplus
}
#endif

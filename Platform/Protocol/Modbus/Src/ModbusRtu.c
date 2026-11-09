/**
 * @file ModbusRtu.c
 * @brief Modbus RTU slave core. 见 ModbusRtu.h 的接口说明。
 *
 * 时间语义：u32Tick 是"这批字节被观察到的时刻"，不是每个字符的真实到达时间。
 * 3.5 字符静默只能用"预期长度 + 批次观察 + 保守等待"近似，不能宣称精确检测
 * 所有临界字符间隔（申请 §5 已写明该限制）。
 */

#include "ModbusRtu.h"

/* 接收缓冲放不下时的哨兵：与"长度未知（0）"区分开。 */
#define MB_LEN_TOO_LONG         (0xFFFFU)

static Uint16 MbBe16(const Uint16 *pu16Src)
{
    return (Uint16)(((pu16Src[0] & 0xFFU) << 8) | (pu16Src[1] & 0xFFU));
}

static void MbPutBe16(Uint16 *pu16Dst, Uint16 u16Value)
{
    pu16Dst[0] = (Uint16)((u16Value >> 8) & 0xFFU);
    pu16Dst[1] = (Uint16)(u16Value & 0xFFU);
}

static void MbAppendCrc(Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Crc = ModbusCrc16(pu16Buf, u16Len);

    pu16Buf[u16Len]      = (Uint16)(u16Crc & 0xFFU);   /* 低字节先发 */
    pu16Buf[u16Len + 1U] = (Uint16)((u16Crc >> 8) & 0xFFU);
}

static void MbDropCandidate(ST_MB_INSTANCE *pst, Uint16 bCountDrop)
{
    pst->u16State = MB_RX_HUNT;
    pst->u16RxLen = 0U;
    pst->u16Expected = 0U;
    if (bCountDrop != 0U)
    {
        pst->u32RxDropped = pst->u32RxDropped + 1UL;
    }
}

/**
 * @brief 依据已收到的字节推断整帧长度。
 * @return 0 = 还要更多字节；MB_LEN_TOO_LONG = 超出接收缓冲；其他 = 期望长度。
 */
static Uint16 MbExpectedLength(const ST_MB_INSTANCE *pst)
{
    Uint16 u16ByteCount;

    if (pst->u16RxLen < 2U)
    {
        return 0U;
    }
    if ((pst->au16RxBuf[1] & 0xFFU) != MB_FC_WRITE_MULTIPLE)
    {
        return 8U;   /* 0x03 / 0x06 / 未知功能码：按最小请求长度收 */
    }
    if (pst->u16RxLen < 7U)
    {
        return 0U;   /* 还没收到 byteCount 字段 */
    }
    u16ByteCount = (Uint16)(pst->au16RxBuf[6] & 0xFFU);
    if (u16ByteCount > (Uint16)(2U * MB_MAX_REGISTERS))
    {
        return MB_LEN_TOO_LONG;
    }
    return (Uint16)(9U + u16ByteCount);
}

static void MbBuildException(ST_MB_INSTANCE *pst, Uint16 u16Addr, Uint16 u16Func, Uint16 u16Exception)
{
    pst->au16TxBuf[0] = (Uint16)(u16Addr & 0xFFU);
    pst->au16TxBuf[1] = (Uint16)((u16Func | 0x80U) & 0xFFU);
    pst->au16TxBuf[2] = (Uint16)(u16Exception & 0xFFU);
    MbAppendCrc(pst->au16TxBuf, 3U);
    pst->u16TxLen = 5U;
    pst->u16TxOffset = 0U;
    pst->u16TxLost = 0U;
}

/**
 * @brief 执行一个已通过静默确认的候选帧。
 *
 * 校验顺序：长度 → CRC → 地址 → 功能码 → 字段 → 业务。任一步失败都不产生
 * "半个回复"：要么完整应答，要么丢弃。
 */
static void MbExecute(ST_MB_INSTANCE *pst)
{
    Uint16 u16Len = pst->u16RxLen;
    Uint16 u16Addr;
    Uint16 u16Func;
    Uint16 u16CrcRecv;
    Uint16 u16CrcCalc;
    Uint16 u16Start;
    Uint16 u16Qty;
    Uint16 u16ByteCount;
    Uint16 u16Exception;
    Uint16 u16Idx;
    Uint16 au16Values[MB_MAX_REGISTERS];

    if (u16Len < 4U)
    {
        MbDropCandidate(pst, 1U);
        return;
    }

    u16CrcRecv = (Uint16)((pst->au16RxBuf[u16Len - 2U] & 0xFFU) |
                          ((pst->au16RxBuf[u16Len - 1U] & 0xFFU) << 8));
    u16CrcCalc = ModbusCrc16(pst->au16RxBuf, (Uint16)(u16Len - 2U));
    if (u16CrcRecv != u16CrcCalc)
    {
        MbDropCandidate(pst, 1U);
        return;
    }

    u16Addr = (Uint16)(pst->au16RxBuf[0] & 0xFFU);
    u16Func = (Uint16)(pst->au16RxBuf[1] & 0xFFU);

    if ((u16Addr != pst->u16SlaveAddr) && (u16Addr != MB_ADDR_BROADCAST))
    {
        MbDropCandidate(pst, 1U);
        return;
    }

    pst->u32RxFrames = pst->u32RxFrames + 1UL;

    /* 广播读：忽略、不应答。 */
    if ((u16Addr == MB_ADDR_BROADCAST) && (u16Func == MB_FC_READ_HOLDING))
    {
        pst->u32RxIgnored = pst->u32RxIgnored + 1UL;
        MbDropCandidate(pst, 0U);
        return;
    }

    if ((u16Func != MB_FC_READ_HOLDING) &&
        (u16Func != MB_FC_WRITE_SINGLE) &&
        (u16Func != MB_FC_WRITE_MULTIPLE))
    {
        if (u16Addr != MB_ADDR_BROADCAST)
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_FUNC);
        }
        MbDropCandidate(pst, 0U);
        return;
    }

    if ((u16Func == MB_FC_READ_HOLDING) || (u16Func == MB_FC_WRITE_SINGLE))
    {
        if (u16Len != 8U)
        {
            MbDropCandidate(pst, 1U);
            return;
        }
    }

    u16Start = MbBe16(&pst->au16RxBuf[2]);

    /* ---- 0x03 读保持寄存器 ---- */
    if (u16Func == MB_FC_READ_HOLDING)
    {
        u16Qty = MbBe16(&pst->au16RxBuf[4]);
        if ((u16Qty == 0U) || (u16Qty > MB_MAX_REGISTERS))
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_VALUE);
        }
        else if (((Uint32)u16Start + (Uint32)u16Qty) > 0x10000UL)
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_ADDR);
        }
        else if (pst->pfRead == (MB_READ_HOLDING)0)
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_ADDR);
        }
        else
        {
            u16Exception = pst->pfRead(pst->pBizCtx, u16Start, u16Qty, au16Values);
            if (u16Exception != MB_EX_NONE)
            {
                MbBuildException(pst, u16Addr, u16Func, u16Exception);
            }
            else
            {
                pst->au16TxBuf[0] = (Uint16)(u16Addr & 0xFFU);
                pst->au16TxBuf[1] = (Uint16)MB_FC_READ_HOLDING;
                pst->au16TxBuf[2] = (Uint16)((u16Qty * 2U) & 0xFFU);
                for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
                {
                    MbPutBe16(&pst->au16TxBuf[3U + (u16Idx * 2U)], au16Values[u16Idx]);
                }
                MbAppendCrc(pst->au16TxBuf, (Uint16)(3U + (u16Qty * 2U)));
                pst->u16TxLen = (Uint16)(5U + (u16Qty * 2U));
                pst->u16TxOffset = 0U;
                pst->u16TxLost = 0U;
            }
        }
        MbDropCandidate(pst, 0U);
        return;
    }

    /* ---- 0x06 写单寄存器 ---- */
    if (u16Func == MB_FC_WRITE_SINGLE)
    {
        au16Values[0] = MbBe16(&pst->au16RxBuf[4]);
        u16Exception = MB_EX_NONE;
        if (pst->pfWrite != (MB_WRITE_HOLDING)0)
        {
            u16Exception = pst->pfWrite(pst->pBizCtx, u16Start, 1U, au16Values);
        }
        else
        {
            u16Exception = MB_EX_ILLEGAL_ADDR;
        }
        if (u16Addr != MB_ADDR_BROADCAST)
        {
            if (u16Exception != MB_EX_NONE)
            {
                MbBuildException(pst, u16Addr, u16Func, u16Exception);
            }
            else
            {
                for (u16Idx = 0U; u16Idx < 8U; u16Idx++)
                {
                    pst->au16TxBuf[u16Idx] = pst->au16RxBuf[u16Idx];
                }
                pst->u16TxLen = 8U;      /* 原样回显，CRC 已在请求里 */
                pst->u16TxOffset = 0U;
                pst->u16TxLost = 0U;
            }
        }
        MbDropCandidate(pst, 0U);
        return;
    }

    /* ---- 0x10 写多个寄存器 ---- */
    u16Qty = MbBe16(&pst->au16RxBuf[4]);
    u16ByteCount = (Uint16)(pst->au16RxBuf[6] & 0xFFU);
    if ((u16Qty == 0U) || (u16Qty > MB_MAX_REGISTERS) ||
        (u16ByteCount != (Uint16)(u16Qty * 2U)) ||
        (u16Len != (Uint16)(9U + u16ByteCount)))
    {
        if (u16Addr != MB_ADDR_BROADCAST)
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_VALUE);
        }
        MbDropCandidate(pst, 0U);
        return;
    }
    if (((Uint32)u16Start + (Uint32)u16Qty) > 0x10000UL)
    {
        if (u16Addr != MB_ADDR_BROADCAST)
        {
            MbBuildException(pst, u16Addr, u16Func, MB_EX_ILLEGAL_ADDR);
        }
        MbDropCandidate(pst, 0U);
        return;
    }

    for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
    {
        au16Values[u16Idx] = MbBe16(&pst->au16RxBuf[7U + (u16Idx * 2U)]);
    }
    u16Exception = MB_EX_NONE;
    if (pst->pfWrite != (MB_WRITE_HOLDING)0)
    {
        u16Exception = pst->pfWrite(pst->pBizCtx, u16Start, u16Qty, au16Values);
    }
    else
    {
        u16Exception = MB_EX_ILLEGAL_ADDR;
    }

    if (u16Addr != MB_ADDR_BROADCAST)
    {
        if (u16Exception != MB_EX_NONE)
        {
            MbBuildException(pst, u16Addr, u16Func, u16Exception);
        }
        else
        {
            pst->au16TxBuf[0] = (Uint16)(u16Addr & 0xFFU);
            pst->au16TxBuf[1] = (Uint16)MB_FC_WRITE_MULTIPLE;
            MbPutBe16(&pst->au16TxBuf[2], u16Start);
            MbPutBe16(&pst->au16TxBuf[4], u16Qty);
            MbAppendCrc(pst->au16TxBuf, 6U);
            pst->u16TxLen = 8U;
            pst->u16TxOffset = 0U;
            pst->u16TxLost = 0U;
        }
    }
    MbDropCandidate(pst, 0U);
}

Uint16 ModbusCrc16(const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Crc = 0xFFFFU;
    Uint16 u16Idx;
    Uint16 u16Bit;
    Uint16 u16Byte;

    if (pu16Data == (const Uint16 *)0)
    {
        return 0xFFFFU;
    }
    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        u16Byte = (Uint16)(pu16Data[u16Idx] & 0xFFU);
        u16Crc = (Uint16)(u16Crc ^ u16Byte);
        for (u16Bit = 0U; u16Bit < 8U; u16Bit++)
        {
            if ((u16Crc & 0x0001U) != 0U)
            {
                u16Crc = (Uint16)((u16Crc >> 1) ^ 0xA001U);
            }
            else
            {
                u16Crc = (Uint16)(u16Crc >> 1);
            }
        }
    }
    return u16Crc;
}

void ModbusRtuInit(ST_MB_INSTANCE *pst, Uint16 u16SlaveAddr, Uint32 u32T35Ticks,
                   Uint32 u32FrameTimeoutTicks, Uint16 u16Enabled)
{
    Uint16 u16Idx;

    if (pst == (ST_MB_INSTANCE *)0)
    {
        return;
    }
    for (u16Idx = 0U; u16Idx < MB_RX_BUF_SIZE; u16Idx++)
    {
        pst->au16RxBuf[u16Idx] = 0U;
    }
    for (u16Idx = 0U; u16Idx < MB_TX_BUF_SIZE; u16Idx++)
    {
        pst->au16TxBuf[u16Idx] = 0U;
    }

    pst->u16SlaveAddr = u16SlaveAddr;
    pst->u16Enabled = (u16Enabled != 0U) ? 1U : 0U;
    pst->u32T35Ticks = u32T35Ticks;
    pst->u32FrameTimeoutTicks = u32FrameTimeoutTicks;
    pst->pBizCtx = (void *)0;
    pst->pfRead = (MB_READ_HOLDING)0;
    pst->pfWrite = (MB_WRITE_HOLDING)0;

    pst->u16RxLen = 0U;
    pst->u16Expected = 0U;
    pst->u16State = MB_RX_HUNT;
    pst->u16TxLost = 0U;
    pst->u32LastByteTick = 0UL;
    pst->u32CompleteTick = 0UL;

    pst->u16TxLen = 0U;
    pst->u16TxOffset = 0U;

    pst->u32RxFrames = 0UL;
    pst->u32RxDropped = 0UL;
    pst->u32RxIgnored = 0UL;
    pst->u32TxSubmitted = 0UL;
    pst->u32TxBusyRounds = 0UL;
    pst->u32TxLostEvents = 0UL;
    pst->u32TimeInvalid = 0UL;
}

void ModbusRtuConfigure(ST_MB_INSTANCE *pst, void *pBizCtx,
                        MB_READ_HOLDING pfRead, MB_WRITE_HOLDING pfWrite)
{
    if (pst == (ST_MB_INSTANCE *)0)
    {
        return;
    }
    pst->pBizCtx = pBizCtx;
    pst->pfRead = pfRead;
    pst->pfWrite = pfWrite;
}

Uint16 ModbusRtuFeed(ST_MB_INSTANCE *pst, const Uint16 *pu16Data, Uint16 u16Count,
                     Uint32 u32Tick, Uint16 u16TickValid)
{
    Uint16 u16Idx;
    Uint16 u16Byte;
    Uint16 u16Expected;
    Uint16 bAbsorbed = 0U;

    if ((pst == (ST_MB_INSTANCE *)0) || (pu16Data == (const Uint16 *)0) ||
        (pst->u16Enabled == 0U) || (u16Count == 0U))
    {
        return 0U;
    }

    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        u16Byte = (Uint16)(pu16Data[u16Idx] & 0xFFU);

        if (pst->u16State == MB_RX_COMPLETE)
        {
            /* 以为结束了却又来字节：上一帧没结束或流已错位，丢弃后重新找帧头。 */
            MbDropCandidate(pst, 1U);
        }

        if (pst->u16State == MB_RX_HUNT)
        {
            if ((u16Byte == pst->u16SlaveAddr) || (u16Byte == MB_ADDR_BROADCAST))
            {
                pst->au16RxBuf[0] = u16Byte;
                pst->u16RxLen = 1U;
                pst->u16Expected = 0U;
                pst->u16State = MB_RX_FRAME;
                if (u16TickValid != 0U)
                {
                    pst->u32LastByteTick = u32Tick;
                }
                bAbsorbed = 1U;
            }
            else
            {
                pst->u32RxDropped = pst->u32RxDropped + 1UL;
            }
        }
        else /* MB_RX_FRAME */
        {
            if (pst->u16RxLen >= MB_RX_BUF_SIZE)
            {
                MbDropCandidate(pst, 1U);
                u16Idx = (Uint16)(u16Idx - 1U);   /* 用这个字节重新找帧头 */
                continue;
            }
            pst->au16RxBuf[pst->u16RxLen] = u16Byte;
            pst->u16RxLen = (Uint16)(pst->u16RxLen + 1U);
            if (u16TickValid != 0U)
            {
                pst->u32LastByteTick = u32Tick;
            }

            u16Expected = MbExpectedLength(pst);
            if (u16Expected == MB_LEN_TOO_LONG)
            {
                MbDropCandidate(pst, 1U);
                u16Idx = (Uint16)(u16Idx - 1U);
                continue;
            }
            if ((u16Expected != 0U) && (pst->u16RxLen >= u16Expected))
            {
                pst->u16Expected = u16Expected;
                pst->u16State = MB_RX_COMPLETE;
                pst->u32CompleteTick = u32Tick;
            }
            bAbsorbed = 1U;
        }
    }

    return bAbsorbed;
}

void ModbusRtuPoll(ST_MB_INSTANCE *pst, Uint32 u32Tick, Uint16 u16TickValid)
{
    Uint32 u32Elapsed;

    if ((pst == (ST_MB_INSTANCE *)0) || (pst->u16Enabled == 0U))
    {
        return;
    }

    if (pst->u16State == MB_RX_COMPLETE)
    {
        if (u16TickValid == 0U)
        {
            /* 时标不可信：不提交候选请求，等有效时标再判。 */
            pst->u32TimeInvalid = pst->u32TimeInvalid + 1UL;
            return;
        }
        u32Elapsed = u32Tick - pst->u32LastByteTick;
        if ((pst->u32T35Ticks == 0UL) || (u32Elapsed >= pst->u32T35Ticks))
        {
            MbExecute(pst);
        }
        return;
    }

    if ((pst->u16State == MB_RX_FRAME) && (pst->u32FrameTimeoutTicks != 0UL) && (u16TickValid != 0U))
    {
        u32Elapsed = u32Tick - pst->u32LastByteTick;
        if (u32Elapsed >= pst->u32FrameTimeoutTicks)
        {
            MbDropCandidate(pst, 1U);   /* 半帧超时：丢弃，不回复 */
        }
    }
}

void ModbusRtuOnStreamBreak(ST_MB_INSTANCE *pst)
{
    if (pst == (ST_MB_INSTANCE *)0)
    {
        return;
    }
    MbDropCandidate(pst, 1U);
}

Uint16 ModbusRtuHasReply(const ST_MB_INSTANCE *pst)
{
    if ((pst == (ST_MB_INSTANCE *)0) || (pst->u16TxLen == 0U))
    {
        return 0U;
    }
    return 1U;
}

Uint16 ModbusRtuConsumeReply(ST_MB_INSTANCE *pst, Uint16 u16Accepted)
{
    Uint16 u16Offset;

    if ((pst == (ST_MB_INSTANCE *)0) || (pst->u16TxLen == 0U))
    {
        return 0U;
    }

    u16Offset = (Uint16)(pst->u16TxOffset + u16Accepted);
    if (u16Offset >= pst->u16TxLen)
    {
        pst->u16TxLen = 0U;
        pst->u16TxOffset = 0U;
        pst->u32TxSubmitted = pst->u32TxSubmitted + 1UL;
        return 1U;
    }

    pst->u16TxOffset = u16Offset;
    return 0U;
}

Uint16 ModbusRtuDiscardReply(ST_MB_INSTANCE *pst)
{
    if ((pst == (ST_MB_INSTANCE *)0) || (pst->u16TxLen == 0U))
    {
        return 0U;
    }

    pst->u16TxLen = 0U;
    pst->u16TxOffset = 0U;
    pst->u16TxLost = 1U;
    pst->u32TxLostEvents = pst->u32TxLostEvents + 1UL;
    return 1U;
}

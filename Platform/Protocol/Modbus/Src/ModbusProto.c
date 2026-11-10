/**
 * @file ModbusProto.c
 * @brief Modbus RTU 从机适配层：SciPort 收发 + 点表访问 + 校验和应答。
 *
 * 关键约定：
 *   - 接收按"批次记录"消费，一条记录一次；短批次不代表端口取空；
 *   - 只有端口确实取空，才允许用静默判据确认帧；
 *   - 待发回复未提交完时不再执行会覆盖缓冲的新请求；
 *   - ARM 升级入口只在指定端口、候选命令起点匹配六字节，握手入环且硬件发完后交接。
 */

#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "SciPort.h"
#include "Hw_SciDriver.h"
#include "Hw_TimeBase.h"

#define MB_PROTO_SLOTS          (3U)     /* 与三路 SCI 对应 */
#define MB_RX_CHUNKS_PER_ROUND  (8U)     /* 每轮最多取 8 条批次记录 */
#define MB_RX_BYTES_PER_ROUND   (32U)    /* 但合计不超过 32 字节/轮 */
#define MB_RX_CHUNK_BYTES       (16U)    /* 单次取出的字节上限 */
#define MB_RX_SEG_MAX           (8U)     /* 真实接口单段上限（SciPort 每拍最多 8 次读取） */
#define MB_US_CHAR35_NUM        (38500000UL)  /* ≤19200：38.5 位 × 1e6，便于整数折算 */
#define MB_T35_FIXED_US         (1750UL) /* >19200：规范固定 3.5 字符时间 1750 µs */
#define MB_HIGH_BAUD_LIMIT      (19200UL)
#define MB_SYSCLK_FALLBACK      (200UL)  /* 取不到 ticks/µs 时的保守值 */

/* ARM 升级命令的固定起点：01 10 04 38 00 02（不校验后续数据与 CRC）。 */
static const Uint16 s_au16ArmPrefix[MB_ARM_PREFIX_LEN] = {
    0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U
};

typedef struct {
    ST_MB_INSTANCE stMb;
    ST_PROTO_INSTANCE *pstCfg;
    Uint32 u32ForceSeen;
    Uint16 u16GenSeen;
    Uint16 u16Frozen;      /* 1 = 强制复位后等待协议层确认损失 */
    Uint16 u16Bound;
    Uint16 u16ArmPending;  /* 1 = 握手回复还在提交途中 */
    Uint16 u16ArmSubmitted;/* 1 = 握手已全部入端口环，等硬件发完 */
    Uint16 u16Armed;       /* 1 = 端口已交给升级协议 */
    Uint16 u16ArmPeekLen;
    Uint32 u32LastBatchTick;   /* 最近一次已处理批次的观察时标 */
    Uint16 u16HaveBatchTick;   /* 1 = u32LastBatchTick 有效 */
    Uint16 u16ArmReplyLen;
    Uint16 au16ArmPeek[MB_ARM_PREFIX_LEN];
    Uint16 au16ArmReply[MB_ARM_REPLY_MAX];
    Uint32 u32RxBacklog;   /* 配额用完且可能仍有积压的轮数 */
} ST_MB_PROTO_STATE;

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_astMbProto, ".protobuf")
#endif
static ST_MB_PROTO_STATE s_astMbProto[MB_PROTO_SLOTS];
static Uint16 s_au16RxChunk[MB_RX_CHUNK_BYTES];
static Uint16 s_au16Replay[MB_ARM_PREFIX_LEN + MB_RX_CHUNK_BYTES];
static MB_ARM_HOOK s_pfArmHook = (MB_ARM_HOOK)0;
static void *s_pArmCtx = (void *)0;
static Uint16 s_u16ArmSciId = 0xFFFFU;   /* 只有这个端口检查升级起点 */

static Uint16 MbProtoArmFeed(ST_MB_PROTO_STATE *pstState, const Uint16 *pu16Data,
                            Uint16 u16Count, Uint32 u32Tick);
static void   MbProtoReplay(ST_MB_PROTO_STATE *pstState, Uint16 u16Keep,
                            const Uint16 *pu16Rest, Uint16 u16RestCount, Uint32 u32Tick);
static Uint16 MbProtoInit(ST_PROTO_INSTANCE *pstInst);
static Uint16 MbProtoService(ST_PROTO_INSTANCE *pstInst);
static void   MbProtoReset(ST_PROTO_INSTANCE *pstInst);
static Uint16 MbProtoReadCb(void *pCtx, Uint16 u16Start, Uint16 u16Count, Uint16 *pu16Values);
static Uint16 MbProtoWriteCb(void *pCtx, Uint16 u16Start, Uint16 u16Count, const Uint16 *pu16Values);
static Uint16 MbProtoTicksPerUs(void);
static Uint32 MbProtoUsRoundUp(Uint32 u32Numerator, Uint32 u32Denominator);
static Uint16 MbProtoHandleFaults(ST_MB_PROTO_STATE *pstState);

const ST_PROTO_OPS stModbusProtoOps = {
    PROTO_TYPE_MODBUS,
    MbProtoInit,
    MbProtoService,
    MbProtoReset
};

void ModbusProtoClear(void)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < MB_PROTO_SLOTS; u16Index++)
    {
        s_astMbProto[u16Index].pstCfg = 0;
        s_astMbProto[u16Index].u32ForceSeen = 0UL;
        s_astMbProto[u16Index].u16GenSeen = 0U;
        s_astMbProto[u16Index].u16Frozen = 0U;
        s_astMbProto[u16Index].u16Bound = 0U;
        s_astMbProto[u16Index].u16ArmPending = 0U;
        s_astMbProto[u16Index].u16ArmSubmitted = 0U;
        s_astMbProto[u16Index].u16Armed = 0U;
        s_astMbProto[u16Index].u16ArmPeekLen = 0U;
        s_astMbProto[u16Index].u16ArmReplyLen = 0U;
        s_astMbProto[u16Index].u32RxBacklog = 0UL;
        s_astMbProto[u16Index].stMb.u16TxLen = 0U;
        s_astMbProto[u16Index].stMb.u16TxOffset = 0U;
        s_astMbProto[u16Index].stMb.u16RxLen = 0U;
        s_astMbProto[u16Index].stMb.u16State = MB_RX_HUNT;
        s_astMbProto[u16Index].stMb.u16TxLost = 0U;
    }
}

void *ModbusProtoState(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0;
    }
    return &s_astMbProto[u16Index];
}

Uint32 ModbusProtoRxFrames(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0UL;
    }
    return s_astMbProto[u16Index].stMb.u32RxFrames;
}

Uint32 ModbusProtoTxSubmitted(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0UL;
    }
    return s_astMbProto[u16Index].stMb.u32TxSubmitted;
}

Uint32 ModbusProtoTxLostEvents(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0UL;
    }
    return s_astMbProto[u16Index].stMb.u32TxLostEvents;
}

Uint32 ModbusProtoRxDropped(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0UL;
    }
    return s_astMbProto[u16Index].stMb.u32RxDropped;
}

Uint32 ModbusProtoRxBacklog(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0UL;
    }
    return s_astMbProto[u16Index].u32RxBacklog;
}

Uint16 ModbusProtoArmPending(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0U;
    }
    return s_astMbProto[u16Index].u16ArmPending;
}

Uint16 ModbusProtoArmActive(Uint16 u16Index)
{
    if (u16Index >= MB_PROTO_SLOTS) {
        return 0U;
    }
    return s_astMbProto[u16Index].u16Armed;
}

static Uint16 MbProtoInit(ST_PROTO_INSTANCE *pstInst)
{
    ST_MB_PROTO_STATE *pstState;
    Uint32 u32Baud;
    Uint32 u32T35Ticks;
    Uint32 u32TimeoutTicks;
    Uint32 u32TicksPerUs;
    Uint32 u32Us;

    if ((pstInst == 0) || (pstInst->pCtx == 0) || (pstInst->pstTbl == 0) ||
        (pstInst->u16TblCount == 0U)) {
        return 0U;
    }
    if (ModbusTblValidate((const ST_MODBUS_POINT *)pstInst->pstTbl, pstInst->u16TblCount) == 0U) {
        return 0U;   /* 点表本身不合法：拒绝启用，不把错误留到收报文时 */
    }

    u32Baud = pstInst->u32Baud;
    if (u32Baud == 0UL) {
        return 0U;
    }

    pstState = (ST_MB_PROTO_STATE *)pstInst->pCtx;
    pstState->pstCfg = pstInst;

    u32TicksPerUs = (Uint32)MbProtoTicksPerUs();
    if (u32Baud > MB_HIGH_BAUD_LIMIT)
    {
        u32Us = MB_T35_FIXED_US;   /* 规范：>19200 用固定 1750 µs */
    }
    else
    {
        u32Us = MbProtoUsRoundUp(MB_US_CHAR35_NUM, u32Baud);
    }
    if (u32Us == 0UL) {
        u32Us = 1UL;
    }
    u32T35Ticks = u32Us * u32TicksPerUs;
    u32TimeoutTicks = 100000UL * u32TicksPerUs;   /* 候选帧停滞兜底 */

    ModbusRtuInit(&pstState->stMb, pstInst->u16SlaveAddr, u32T35Ticks, u32TimeoutTicks, 1U);
    ModbusRtuConfigure(&pstState->stMb, pstInst, MbProtoReadCb, MbProtoWriteCb);

    pstState->u32ForceSeen = SciPortForceResets(pstInst->u16SciId);
    pstState->u16GenSeen = SciPortFaultGeneration(pstInst->u16SciId);
    pstState->u16Frozen = 0U;
    pstState->u16Bound = 1U;
    pstState->u16ArmPending = 0U;
    pstState->u16ArmSubmitted = 0U;
    pstState->u16Armed = 0U;
    pstState->u16ArmPeekLen = 0U;
    pstState->u32LastBatchTick = 0UL;
    pstState->u16HaveBatchTick = 0U;
    pstState->u16ArmReplyLen = 0U;
    pstState->u32RxBacklog = 0UL;

    return 1U;
}

static Uint16 MbProtoService(ST_PROTO_INSTANCE *pstInst)
{
    ST_MB_PROTO_STATE *pstState;
    Uint32 u32PollTick;
    Uint16 u16PollValid;
    Uint16 u16State;
    Uint16 u16Got;
    Uint16 u16Total;
    Uint16 u16Chunk;
    Uint16 u16Drained;
    Uint16 u16Max;
    Uint16 u16ArmResult;

    if ((pstInst == 0) || (pstInst->pCtx == 0))
    {
        return 0U;
    }

    pstState = (ST_MB_PROTO_STATE *)pstInst->pCtx;
    if (pstState->u16Bound == 0U)
    {
        return 0U;
    }

    /* 端口已交给升级协议：本层不再碰 RX，也不再发 Modbus 回复。 */
    if (pstState->u16Armed != 0U)
    {
        return 0U;
    }

    if (MbProtoHandleFaults(pstState) == 0U)
    {
        return 0U;
    }

    /* ---- 握手提交与交接 ----
     * 只有"全部字节进了端口环"才算提交成功；随后还要等软件环与硬件都发完，
     * 才把端口交给升级协议。强制复位在 MbProtoHandleFaults() 里取消交接。 */
    if (pstState->u16ArmPending != 0U)
    {
        if (ModbusRtuHasReply(&pstState->stMb) != 0U)
        {
            if (SciPortIsTxSuspended(pstInst->u16SciId) == 0U)
            {
                u16Got = SciPortWriteTx(pstInst->u16SciId,
                                        &pstState->stMb.au16TxBuf[pstState->stMb.u16TxOffset],
                                        (Uint16)(pstState->stMb.u16TxLen - pstState->stMb.u16TxOffset));
                (void)ModbusRtuConsumeReply(&pstState->stMb, u16Got);
            }
        }
        if (ModbusRtuHasReply(&pstState->stMb) == 0U)
        {
            pstState->u16ArmPending = 0U;
            pstState->u16ArmSubmitted = 1U;
        }
        return 0U;   /* 交接期间不消费 RX */
    }

    if (pstState->u16ArmSubmitted != 0U)
    {
        if (SciPortIsTxIdle(pstInst->u16SciId) != 0U)
        {
            pstState->u16ArmSubmitted = 0U;
            pstState->u16Armed = 1U;
        }
        return 0U;
    }

    /* 有待发回复且端口未冻结：只按端口实际接受的字节数推进，不重发前缀。 */
    if (ModbusRtuHasReply(&pstState->stMb) != 0U) {
        if (SciPortIsTxSuspended(pstInst->u16SciId) != 0U) {
            pstState->stMb.u32TxBusyRounds++;
        } else {
            u16Got = SciPortWriteTx(pstInst->u16SciId,
                                    &pstState->stMb.au16TxBuf[pstState->stMb.u16TxOffset],
                                    (Uint16)(pstState->stMb.u16TxLen - pstState->stMb.u16TxOffset));
            (void)ModbusRtuConsumeReply(&pstState->stMb, u16Got);
            if ((u16Got == 0U) && (ModbusRtuHasReply(&pstState->stMb) != 0U)) {
                pstState->stMb.u32TxBusyRounds++;
            }
        }
    }

    /* ---- 接收：按批次记录消费，直到取空或用完配额 ---- */
    u16Drained = 0U;
    u16State = SciPortRxState(pstInst->u16SciId);
    if (u16State == SCI_RX_IDLE) {
        u16Total = 0U;

        for (u16Chunk = 0U; u16Chunk < MB_RX_CHUNKS_PER_ROUND; u16Chunk++)
        {
            ST_SCI_RX_SEG stSeg;

            u16Max = (Uint16)(MB_RX_BYTES_PER_ROUND - u16Total);
            if (u16Max > MB_RX_CHUNK_BYTES) {
                u16Max = MB_RX_CHUNK_BYTES;
            }
            if (u16Max < MB_RX_SEG_MAX) {
                /* 剩余额度装不下一条完整批次：无法判断是否取空，按未取空处理 */
                break;
            }

            u16Got = SciPortPopBatch(pstInst->u16SciId, s_au16RxChunk, u16Max, &stSeg);
            if (u16Got == 0U) {
                /* 取零：先确认这期间没有新的断流或代次变化，才算干净取空 */
                if ((SciPortForceResets(pstInst->u16SciId) != pstState->u32ForceSeen) ||
                    (SciPortFaultGeneration(pstInst->u16SciId) != pstState->u16GenSeen) ||
                    (SciPortIsStreamBreak(pstInst->u16SciId) != 0U)) {
                    (void)MbProtoHandleFaults(pstState);
                    u16Drained = 0U;
                    break;
                }
                u16Drained = 1U;
                break;
            }

            /* 新批次与上一批次间隔足够，才认为升级起点的匹配进度过期 */
            if (pstState->u16HaveBatchTick != 0U)
            {
                if ((Uint32)(pstState->u32LastBatchTick - stSeg.u32Tick) >= pstState->stMb.u32T35Ticks)
                {
                    pstState->u16ArmPeekLen = 0U;   /* 前缀匹配超时重新开始 */
                }
            }
            pstState->u32LastBatchTick = stSeg.u32Tick;
            pstState->u16HaveBatchTick = 1U;

            if (pstInst->u16SciId == s_u16ArmSciId) {
                u16ArmResult = MbProtoArmFeed(pstState, s_au16RxChunk, u16Got, stSeg.u32Tick);
                if (u16ArmResult != 0U) {
                    u16Total = (Uint16)(u16Total + u16Got);
                    if (u16ArmResult >= 2U) {
                        u16Drained = 0U;   /* 已交给升级协议 */
                        break;
                    }
                    continue;          /* 本批已暂存或已回放 */
                }
            }

            (void)ModbusRtuFeed(&pstState->stMb, s_au16RxChunk, u16Got,
                                stSeg.u32Tick, 1U);   /* 批次时标 0 也是有效值 */
            u16Total = (Uint16)(u16Total + u16Got);
        }
    }

    if (u16Drained != 0U) {
        /* 只有确认端口取空，才允许用"静默"确认帧结束。 */
        u32PollTick = Hw_TimeBaseTicks();
        u16PollValid = Hw_TimeBaseIsReady();
        if (u16PollValid != 0U) {
            ModbusRtuPoll(&pstState->stMb, u32PollTick, 1U);
        }
    } else {
        pstState->u32RxBacklog = pstState->u32RxBacklog + 1UL;
    }

    return 1U;
}

/**
 * @brief 升级入口：只在指定端口、候选命令起点累积六字节并调用回调。
 * @return 1 = 本批已被升级入口接管；0 = 继续按普通 Modbus 处理
 */
/* 升级起点匹配：返回 0 = 交回普通 Modbus，1 = 本批已消费（暂存或已回放），
 * 2 = 已接管（装载握手成功，本轮停止消费）。
 * 匹配中的字节只暂存、不交给普通 Modbus；失配或未接管时按原顺序回放，不丢不重。 */
static Uint16 MbProtoArmFeed(ST_MB_PROTO_STATE *pstState, const Uint16 *pu16Data,
                             Uint16 u16Count, Uint32 u32Tick)
{
    Uint16 u16Idx;
    Uint16 u16Byte;
    Uint16 u16ReplyLen;
    Uint16 u16Keep;

    if (s_pfArmHook == (MB_ARM_HOOK)0) {
        return 0U;
    }
    if ((pstState->u16Armed != 0U) || (pstState->u16ArmPending != 0U) ||
        (pstState->u16ArmSubmitted != 0U)) {
        return 1U;   /* 交接进行中：本层不再把字节交给普通 Modbus */
    }
    if (ModbusRtuIsHunting(&pstState->stMb) == 0U) {
        /* 普通 Modbus 已离开 HUNT：暂存字节不可信，直接丢弃、不回放。 */
        pstState->u16ArmPeekLen = 0U;
        return 0U;
    }

    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        u16Byte = (Uint16)(pu16Data[u16Idx] & 0xFFU);

        if (u16Byte == s_au16ArmPrefix[pstState->u16ArmPeekLen]) {
            pstState->au16ArmPeek[pstState->u16ArmPeekLen] = u16Byte;
            pstState->u16ArmPeekLen = (Uint16)(pstState->u16ArmPeekLen + 1U);
            if (pstState->u16ArmPeekLen < MB_ARM_PREFIX_LEN) {
                continue;   /* 匹配中：暂存，进度跨批次保留 */
            }

            u16ReplyLen = s_pfArmHook(s_pArmCtx, pstState->au16ArmPeek, MB_ARM_PREFIX_LEN,
                                      pstState->au16ArmReply, MB_ARM_REPLY_MAX);
            if ((u16ReplyLen != 0U) && (u16ReplyLen <= MB_ARM_REPLY_MAX)) {
                ModbusRtuOnStreamBreak(&pstState->stMb);
                if (ModbusRtuLoadReply(&pstState->stMb, pstState->au16ArmReply, u16ReplyLen) != 0U) {
                    pstState->u16ArmReplyLen = u16ReplyLen;
                    pstState->u16ArmPeekLen = 0U;
                    pstState->u16ArmPending = 1U;
                    return 2U;
                }
            }

            /* 未接管：暂存的前缀与本批剩余字节按原顺序交回普通 Modbus。
             * 第六字节已存入 au16ArmPeek，这里必须从下一个字节开始，否则会重复回放。 */
            u16Keep = pstState->u16ArmPeekLen;
            pstState->u16ArmPeekLen = 0U;
            MbProtoReplay(pstState, u16Keep, &pu16Data[u16Idx + 1U],
                          (Uint16)(u16Count - u16Idx - 1U), u32Tick);
            return 1U;
        }

        /* 失配：暂存的前缀 + 本字节及剩余字节按原顺序交回普通 Modbus */
        u16Keep = pstState->u16ArmPeekLen;
        pstState->u16ArmPeekLen = 0U;
        MbProtoReplay(pstState, u16Keep, &pu16Data[u16Idx],
                      (Uint16)(u16Count - u16Idx), u32Tick);
        return 1U;
    }

    return 1U;   /* 整批都用于前缀匹配，已暂存 */
}

/* 把暂存前缀与剩余字节合并后一次性交给普通 Modbus（保持原顺序）。 */
static void MbProtoReplay(ST_MB_PROTO_STATE *pstState, Uint16 u16Keep,
                          const Uint16 *pu16Rest, Uint16 u16RestCount, Uint32 u32Tick)
{
    Uint16 u16Idx;
    Uint16 u16Len = 0U;

    for (u16Idx = 0U; u16Idx < u16Keep; u16Idx++)
    {
        s_au16Replay[u16Len] = pstState->au16ArmPeek[u16Idx];
        u16Len = (Uint16)(u16Len + 1U);
    }
    for (u16Idx = 0U; u16Idx < u16RestCount; u16Idx++)
    {
        s_au16Replay[u16Len] = (Uint16)(pu16Rest[u16Idx] & 0xFFU);
        u16Len = (Uint16)(u16Len + 1U);
    }
    if (u16Len != 0U) {
        (void)ModbusRtuFeed(&pstState->stMb, s_au16Replay, u16Len, u32Tick, 1U);
    }
}

void ModbusProtoSetArmHook(Uint16 u16SciId, void *pCtx, MB_ARM_HOOK pfHook)
{
    s_u16ArmSciId = u16SciId;
    s_pArmCtx = pCtx;
    s_pfArmHook = pfHook;
}

void ModbusProtoArmRelease(Uint16 u16Index)
{
    ST_MB_PROTO_STATE *pstState;

    if (u16Index >= MB_PROTO_SLOTS) {
        return;
    }
    pstState = &s_astMbProto[u16Index];
    pstState->u16Armed = 0U;
    pstState->u16ArmPending = 0U;
    pstState->u16ArmSubmitted = 0U;
    pstState->u16ArmPeekLen = 0U;
    pstState->u16ArmReplyLen = 0U;
    ModbusRtuOnStreamBreak(&pstState->stMb);
}

static void MbProtoReset(ST_PROTO_INSTANCE *pstInst)
{
    ST_MB_PROTO_STATE *pstState;

    if ((pstInst == 0) || (pstInst->pCtx == 0)) {
        return;
    }
    pstState = (ST_MB_PROTO_STATE *)pstInst->pCtx;

    ModbusRtuOnStreamBreak(&pstState->stMb);
    (void)ModbusRtuDiscardReply(&pstState->stMb);
    pstState->u16ArmPending = 0U;
    pstState->u16ArmSubmitted = 0U;
    pstState->u16ArmPeekLen = 0U;
    pstState->u16GenSeen = SciPortFaultGeneration(pstInst->u16SciId);
    pstState->u32ForceSeen = SciPortForceResets(pstInst->u16SciId);
    pstState->u16Frozen = 0U;
}

/**
 * @brief 故障代次、强制复位、流中断与解冻。
 * @return 1 = 可以继续本轮服务；0 = 参数/绑定异常
 */
static Uint16 MbProtoHandleFaults(ST_MB_PROTO_STATE *pstState)
{
    ST_PROTO_INSTANCE *pstInst;
    Uint32 u32Force;
    Uint16 u16Gen;
    Uint16 u16State;

    pstInst = pstState->pstCfg;
    if (pstInst == 0) {
        return 0U;
    }

    /* 新的强制复位：端口软件环已被清空，本层尚未提交完的回复作废；
     * 正在进行的升级交接也要取消——握手的"丢弃"不等于"发送完成"。 */
    u32Force = SciPortForceResets(pstInst->u16SciId);
    if (u32Force != pstState->u32ForceSeen) {
        pstState->u32ForceSeen = u32Force;
        (void)ModbusRtuDiscardReply(&pstState->stMb);
        pstState->u16Frozen = 1U;
        pstState->u16ArmPending = 0U;
        pstState->u16ArmSubmitted = 0U;
        pstState->u16ArmPeekLen = 0U;
        pstState->u16ArmReplyLen = 0U;
    }

    /* 端口回 IDLE 后由协议层确认"接受这次损失"，再解冻发送。 */
    if ((pstState->u16Frozen != 0U) && (SciPortRxState(pstInst->u16SciId) == SCI_RX_IDLE)) {
        if (SciPortTxResume(pstInst->u16SciId) != 0U) {
            pstState->u16Frozen = 0U;
        }
    }

    /* 流中断：丢弃候选帧，并在端口允许时清掉断流标志。 */
    if (SciPortIsStreamBreak(pstInst->u16SciId) != 0U) {
        u16Gen = SciPortFaultGeneration(pstInst->u16SciId);
        if (u16Gen != pstState->u16GenSeen) {
            ModbusRtuOnStreamBreak(&pstState->stMb);
            pstState->u16GenSeen = u16Gen;
        }
        pstState->u16ArmPeekLen = 0U;
        (void)SciPortClearStreamBreak(pstInst->u16SciId, u16Gen);
    }

    u16State = SciPortRxState(pstInst->u16SciId);
    if ((u16State == SCI_RX_HW_RESET) || (u16State == SCI_RX_PAUSED)) {
        ModbusRtuOnStreamBreak(&pstState->stMb);
        pstState->u16ArmPeekLen = 0U;
    }

    return 1U;
}

static Uint16 MbProtoReadCb(void *pCtx, Uint16 u16Start, Uint16 u16Count, Uint16 *pu16Values)
{
    const ST_PROTO_INSTANCE *pstInst = (const ST_PROTO_INSTANCE *)pCtx;

    if (pstInst == 0) {
        return MB_EX_ILLEGAL_ADDR;
    }

    return ModbusTblRead((const ST_MODBUS_POINT *)pstInst->pstTbl, pstInst->u16TblCount,
                         u16Start, u16Count, pu16Values);
}

static Uint16 MbProtoWriteCb(void *pCtx, Uint16 u16Start, Uint16 u16Count, const Uint16 *pu16Values)
{
    const ST_PROTO_INSTANCE *pstInst = (const ST_PROTO_INSTANCE *)pCtx;

    if (pstInst == 0) {
        return MB_EX_ILLEGAL_ADDR;
    }

    return ModbusTblWrite((const ST_MODBUS_POINT *)pstInst->pstTbl, pstInst->u16TblCount,
                          u16Start, u16Count, pu16Values);
}

static Uint16 MbProtoTicksPerUs(void)
{
    Uint32 u32Ticks = Hw_TimeBaseUsToTicks(1UL);

    if ((u32Ticks == 0UL) || (u32Ticks > 0xFFFFUL)) {
        u32Ticks = MB_SYSCLK_FALLBACK;
    }

    return (Uint16)u32Ticks;
}

static Uint32 MbProtoUsRoundUp(Uint32 u32Numerator, Uint32 u32Denominator)
{
    if (u32Denominator == 0UL) {
        return 0UL;
    }

    return (u32Numerator + u32Denominator - 1UL) / u32Denominator;
}

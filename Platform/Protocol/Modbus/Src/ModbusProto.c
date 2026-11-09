/**
 * @file ModbusProto.c
 * @brief Modbus RTU 从机适配层：SciPort 收发 + 点表访问 + 故障恢复。
 */

#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "SciPort.h"
#include "Hw_SciDriver.h"
#include "Hw_TimeBase.h"

#define MB_PROTO_SLOTS          (3U)     /* 与三路 SCI 对应 */
#define MB_RX_CHUNKS_PER_ROUND  (2U)     /* 每个 1 ms 服务最多取两段 */
#define MB_RX_BYTES_PER_ROUND   (32U)    /* 但合计不超过 32 字节/次 */
#define MB_RX_CHUNK_BYTES       (16U)    /* 单次取出的字节上限 */
#define MB_US_CHAR35_NUM        (38500000UL)  /* 3.5 字符 = 38.5 位；×1e6 便于整数换算 */
#define MB_SYSCLK_FALLBACK      (200UL)  /* 取不到 ticks/µs 时的保守值 */

typedef struct {
    ST_MB_INSTANCE stMb;
    ST_PROTO_INSTANCE *pstCfg;
    Uint32 u32ForceSeen;
    Uint16 u16GenSeen;
    Uint16 u16Frozen;      /* 1 = 强制复位后等待协议层确认解冻 */
    Uint16 u16Bound;
    Uint16 u16Reserved;
} ST_MB_PROTO_STATE;

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_astMbProto, ".protobuf")
#endif
static ST_MB_PROTO_STATE s_astMbProto[MB_PROTO_SLOTS];
static Uint16 s_au16RxChunk[MB_RX_CHUNK_BYTES];

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

    for (u16Index = 0U; u16Index < MB_PROTO_SLOTS; u16Index++) {
        s_astMbProto[u16Index].pstCfg = 0;
        s_astMbProto[u16Index].u32ForceSeen = 0UL;
        s_astMbProto[u16Index].u16GenSeen = 0U;
        s_astMbProto[u16Index].u16Frozen = 0U;
        s_astMbProto[u16Index].u16Bound = 0U;
        s_astMbProto[u16Index].u16Reserved = 0U;
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
    u32Us = MbProtoUsRoundUp(MB_US_CHAR35_NUM, u32Baud);
    if (u32Us == 0UL) {
        u32Us = 1UL;
    }
    u32T35Ticks = u32Us * u32TicksPerUs;
    u32TimeoutTicks = 100000UL * u32TicksPerUs;   /* MB_FRAME_TIMEOUT_MS */

    ModbusRtuInit(&pstState->stMb, pstInst->u16SlaveAddr, u32T35Ticks, u32TimeoutTicks, 1U);
    ModbusRtuConfigure(&pstState->stMb, pstInst, MbProtoReadCb, MbProtoWriteCb);

    pstState->u32ForceSeen = SciPortForceResets(pstInst->u16SciId);
    pstState->u16GenSeen = SciPortFaultGeneration(pstInst->u16SciId);
    pstState->u16Frozen = 0U;
    pstState->u16Bound = 1U;

    return 1U;
}

static Uint16 MbProtoService(ST_PROTO_INSTANCE *pstInst)
{
    ST_MB_PROTO_STATE *pstState;
    Uint32 u32Now;
    Uint16 u16TickValid;
    Uint16 u16State;
    Uint16 u16Got;
    Uint16 u16Total;
    Uint16 u16Chunk;

    if ((pstInst == 0) || (pstInst->pCtx == 0)) {
        return 0U;
    }

    pstState = (ST_MB_PROTO_STATE *)pstInst->pCtx;
    if (pstState->u16Bound == 0U) {
        return 0U;
    }

    u32Now = Hw_TimeBaseTicks();
    u16TickValid = Hw_TimeBaseIsReady();

    if (MbProtoHandleFaults(pstState) == 0U) {
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

    u16State = SciPortRxState(pstInst->u16SciId);
    if (u16State == SCI_RX_IDLE) {
        u16Total = 0U;
        for (u16Chunk = 0U; u16Chunk < MB_RX_CHUNKS_PER_ROUND; u16Chunk++) {
            ST_SCI_RX_SEG stSeg;
            Uint16 u16Max;

            u16Max = (Uint16)(MB_RX_BYTES_PER_ROUND - u16Total);
            if (u16Max == 0U) {
                break;
            }
            if (u16Max > MB_RX_CHUNK_BYTES) {
                u16Max = MB_RX_CHUNK_BYTES;
            }

            u16Got = SciPortPopBatch(pstInst->u16SciId, s_au16RxChunk, u16Max, &stSeg);
            if (u16Got == 0U) {
                break;
            }
            (void)ModbusRtuFeed(&pstState->stMb, s_au16RxChunk, u16Got,
                                stSeg.u32Tick, (stSeg.u32Tick != 0UL) ? 1U : 0U);
            u16Total = (Uint16)(u16Total + u16Got);
        }
    }

    ModbusRtuPoll(&pstState->stMb, u32Now, u16TickValid);

    return 1U;
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

    /* 新的强制复位：端口软件环已被清空，本层尚未提交完的回复作废。 */
    u32Force = SciPortForceResets(pstInst->u16SciId);
    if (u32Force != pstState->u32ForceSeen) {
        pstState->u32ForceSeen = u32Force;
        (void)ModbusRtuDiscardReply(&pstState->stMb);
        pstState->u16Frozen = 1U;
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
        (void)SciPortClearStreamBreak(pstInst->u16SciId, u16Gen);
    }

    u16State = SciPortRxState(pstInst->u16SciId);
    if ((u16State == SCI_RX_HW_RESET) || (u16State == SCI_RX_PAUSED)) {
        ModbusRtuOnStreamBreak(&pstState->stMb);
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

/**
 * @file modbus_proto_stub_test.c
 * @brief 05D 主机测试：Modbus 适配层（ModbusProto）——静默确认、配额积压、回复忙、ARM 入口。
 *
 * 本文件自带 SciPort 与时间基准替身，链接 ModbusProto.c / ModbusRtu.c / ModbusTbl.c：
 *   cc -std=c99 -Wall -Wextra -I../../Port/Test/shim -I../../Port/Inc \
 *      -I../Inc -I../Modbus/Inc -I../../../Drivers/inc \
 *      modbus_proto_stub_test.c ../Modbus/Src/ModbusProto.c \
 *      ../Modbus/Src/ModbusRtu.c ../Modbus/Src/ModbusTbl.c -o modbus_proto_stub_test
 *
 * 替身按真实接口建模：
 *   - SciPortPopBatch() 一次返回"一条批次记录"，记录带自己的长度与观察时标；
 *     单条记录装不进 u16Max 时返回 0（真实接口同此语义）。
 *   - SciPortWriteTx() 内部是 RingBuffWriteExact()：整批装不下就一个字节都不收。
 *   - 时标为 Timer0 向下计数，"过去多久" = (Uint32)(last - now)。
 */

#include <stdio.h>

#include "Protocol.h"
#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "SciPort.h"

static int s_iChecks = 0;
static int s_iFails = 0;

static void Check(int iCond, const char *pszName)
{
    s_iChecks++;
    if (iCond == 0)
    {
        s_iFails++;
        printf("FAIL: %s\n", pszName);
    }
}

/* ================= 时间基准替身（向下计数） ================= */

static Uint32 s_u32Now = 1000000UL;

Uint32 Hw_TimeBaseTicks(void)
{
    return s_u32Now;
}

Uint16 Hw_TimeBaseIsReady(void)
{
    return 1U;
}

Uint32 Hw_TimeBaseUsToTicks(Uint32 u32Us)
{
    return u32Us;                /* 1 tick = 1 µs，便于直接算静默 */
}

static void Advance(Uint32 u32Ticks)
{
    s_u32Now = s_u32Now - u32Ticks;
}

/* ================= SciPort 替身（批次记录队列） ================= */

#define STUB_SEG_BYTES  (16U)
#define STUB_SEG_DEPTH  (96U)
#define STUB_TX_DEPTH   (512U)

typedef struct {
    Uint16 au16Data[STUB_SEG_BYTES];
    Uint16 u16Len;
    Uint32 u32Tick;          /* 该批次被观察到的时刻 */
} ST_STUB_SEG;

static ST_STUB_SEG s_astSeg[STUB_SEG_DEPTH];
static Uint16 s_u16SegHead = 0U;
static Uint16 s_u16SegTail = 0U;

static Uint16 s_u16RxState = SCI_RX_IDLE;
static Uint32 s_u32ForceResets = 0UL;
static Uint16 s_u16Gen = 1U;
static Uint16 s_u16StreamBreak = 0U;
static Uint16 s_u16TxSuspended = 0U;
static Uint16 s_u16TxFree = STUB_TX_DEPTH;
static Uint16 s_u16TxIdle = 1U;
static Uint16 s_au16Tx[STUB_TX_DEPTH];
static Uint16 s_u16TxLen = 0U;
static Uint16 s_u16PopCalls = 0U;
static Uint16 s_u16LateArmed = 0U;   /* 1 = 下一次 PopBatch 期间来新批次 */
static Uint16 s_u16LateByte = 0U;
static void PushSeg(const Uint16 *pu16Data, Uint16 u16Count);

Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max,
                       ST_SCI_RX_SEG *pstSeg)
{
    ST_STUB_SEG *pstRec;
    Uint16 u16Idx;

    (void)u16PortId;
    s_u16PopCalls++;

    if (s_u16LateArmed != 0U)
    {
        /* 入口取时之后、取空之前，ISR 发布了时标更新的新批次 */
        s_u16LateArmed = 0U;
        Advance(50UL);
        PushSeg(&s_u16LateByte, 1U);
    }

    if (pstSeg != (ST_SCI_RX_SEG *)0)
    {
        pstSeg->u32Tick = s_u32Now;
        pstSeg->u32TickEmptyLast = s_u32Now;
        pstSeg->u16SeqStart = 0U;
        pstSeg->u16Count = 0U;
        pstSeg->u16EmptyRounds = 1U;
    }
    if (s_u16SegHead >= s_u16SegTail)
    {
        return 0U;                   /* 端口确实取空 */
    }

    pstRec = &s_astSeg[s_u16SegHead];
    if (pstRec->u16Len > u16Max)
    {
        return 0U;                   /* 容量不足：真实接口也不拆段 */
    }
    for (u16Idx = 0U; u16Idx < pstRec->u16Len; u16Idx++)
    {
        pu16Dst[u16Idx] = pstRec->au16Data[u16Idx];
    }
    if (pstSeg != (ST_SCI_RX_SEG *)0)
    {
        pstSeg->u32Tick = pstRec->u32Tick;
        pstSeg->u32TickEmptyLast = pstRec->u32Tick;
        pstSeg->u16Count = pstRec->u16Len;
        pstSeg->u16EmptyRounds = 0U;
    }
    s_u16SegHead++;
    return pstRec->u16Len;
}

Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Idx;

    (void)u16PortId;
    if ((s_u16TxSuspended != 0U) || (u16Count > s_u16TxFree))
    {
        return 0U;                   /* RingBuffWriteExact：整批全有或全无 */
    }
    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        s_au16Tx[s_u16TxLen + u16Idx] = (Uint16)(pu16Data[u16Idx] & 0xFFU);
    }
    s_u16TxLen = (Uint16)(s_u16TxLen + u16Count);
    s_u16TxFree = (Uint16)(s_u16TxFree - u16Count);
    s_u16TxIdle = 0U;
    return u16Count;
}

Uint16 SciPortIsTxIdle(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16TxIdle;
}

Uint16 SciPortRxState(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16RxState;
}

Uint32 SciPortForceResets(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u32ForceResets;
}

Uint16 SciPortFaultGeneration(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16Gen;
}

Uint16 SciPortIsStreamBreak(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16StreamBreak;
}

Uint16 SciPortClearStreamBreak(Uint16 u16PortId, Uint16 u16Generation)
{
    (void)u16PortId;
    (void)u16Generation;
    s_u16StreamBreak = 0U;
    return 1U;
}

Uint16 SciPortIsTxSuspended(Uint16 u16PortId)
{
    (void)u16PortId;
    return s_u16TxSuspended;
}

Uint16 SciPortTxResume(Uint16 u16PortId)
{
    (void)u16PortId;
    s_u16TxSuspended = 0U;
    return 1U;
}

static void StubTxDrain(void)
{
    s_u16TxIdle = 1U;                /* 软件环与硬件都发完了 */
}

/* ================= 点表：5000..5015 共 16 个 Uint16 ================= */

#define PROTO_TBL_COUNT  (16U)

static Uint16 s_au16Regs[PROTO_TBL_COUNT];
static ST_MODBUS_POINT s_astProtoTbl[PROTO_TBL_COUNT];
static ST_PROTO_INSTANCE s_stInst;

static void TableSetUp(void)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < PROTO_TBL_COUNT; u16Idx++)
    {
        s_astProtoTbl[u16Idx].u16Addr = (Uint16)(5000U + u16Idx);
        s_astProtoTbl[u16Idx].u16Type = MB_PT_U16;
        s_astProtoTbl[u16Idx].u16Access = MB_PT_READWRITE;
        s_astProtoTbl[u16Idx].u16Reserved = 0U;
        s_astProtoTbl[u16Idx].pVar = &s_au16Regs[u16Idx];
        s_astProtoTbl[u16Idx].u32Min = 0UL;
        s_astProtoTbl[u16Idx].u32Max = 65535UL;
        s_astProtoTbl[u16Idx].fMin = 0.0f;
        s_astProtoTbl[u16Idx].fMax = 0.0f;
    }
}

/* 一段记录入队：模拟真实端口"一次观察到一个短批次"。 */
static void PushSeg(const Uint16 *pu16Data, Uint16 u16Count)
{
    ST_STUB_SEG *pstRec;
    Uint16 u16Idx;

    pstRec = &s_astSeg[s_u16SegTail];
    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        pstRec->au16Data[u16Idx] = (Uint16)(pu16Data[u16Idx] & 0xFFU);
    }
    pstRec->u16Len = u16Count;
    pstRec->u32Tick = s_u32Now;
    s_u16SegTail++;
}

/* 按 4 字节一段入队：接近真实 SCI 短批次的粒度。 */
static void PushBytes(const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Left = u16Count;
    Uint16 u16Chunk;

    while (u16Left != 0U)
    {
        u16Chunk = (u16Left > 4U) ? 4U : u16Left;
        PushSeg(pu16Data, u16Chunk);
        pu16Data = &pu16Data[u16Chunk];
        u16Left = (Uint16)(u16Left - u16Chunk);
    }
}

static Uint16 StubRxPending(void)
{
    Uint16 u16Idx;
    Uint16 u16Total = 0U;

    for (u16Idx = s_u16SegHead; u16Idx < s_u16SegTail; u16Idx++)
    {
        u16Total = (Uint16)(u16Total + s_astSeg[u16Idx].u16Len);
    }
    return u16Total;
}

static Uint16 BuildFrame(Uint16 *pu16Out, const Uint16 *pu16Body, Uint16 u16BodyLen)
{
    Uint16 u16Idx;
    Uint16 u16Crc = ModbusCrc16(pu16Body, u16BodyLen);

    for (u16Idx = 0U; u16Idx < u16BodyLen; u16Idx++)
    {
        pu16Out[u16Idx] = (Uint16)(pu16Body[u16Idx] & 0xFFU);
    }
    pu16Out[u16BodyLen] = (Uint16)(u16Crc & 0xFFU);
    pu16Out[u16BodyLen + 1U] = (Uint16)((u16Crc >> 8) & 0xFFU);
    return (Uint16)(u16BodyLen + 2U);
}

static Uint16 MakeRead(Uint16 u16Start, Uint16 u16Count, Uint16 *pu16Frame)
{
    Uint16 au16Body[8];

    au16Body[0] = 0x01U;
    au16Body[1] = 0x03U;
    au16Body[2] = (Uint16)((u16Start >> 8) & 0xFFU);
    au16Body[3] = (Uint16)(u16Start & 0xFFU);
    au16Body[4] = (Uint16)((u16Count >> 8) & 0xFFU);
    au16Body[5] = (Uint16)(u16Count & 0xFFU);
    return BuildFrame(pu16Frame, au16Body, 6U);
}

static Uint16 MakeWriteSingle(Uint16 u16Addr, Uint16 u16Value, Uint16 *pu16Frame)
{
    Uint16 au16Body[8];

    au16Body[0] = 0x01U;
    au16Body[1] = 0x06U;
    au16Body[2] = (Uint16)((u16Addr >> 8) & 0xFFU);
    au16Body[3] = (Uint16)(u16Addr & 0xFFU);
    au16Body[4] = (Uint16)((u16Value >> 8) & 0xFFU);
    au16Body[5] = (Uint16)(u16Value & 0xFFU);
    return BuildFrame(pu16Frame, au16Body, 6U);
}

/* ================= ARM 回调 ================= */

static Uint16 s_au16ArmReply[MB_ARM_REPLY_MAX];
static Uint16 s_u16ArmReplyLen = 0U;
static Uint16 s_u16ArmCalls = 0U;

static Uint16 ArmHook(void *pCtx, const Uint16 *pu16Prefix, Uint16 u16PrefixLen,
                      Uint16 *pu16Reply, Uint16 u16ReplyMax)
{
    Uint16 u16Idx;

    (void)pCtx;
    (void)pu16Prefix;
    s_u16ArmCalls++;
    if ((u16PrefixLen != MB_ARM_PREFIX_LEN) || (s_u16ArmReplyLen > u16ReplyMax))
    {
        return 0U;
    }
    for (u16Idx = 0U; u16Idx < s_u16ArmReplyLen; u16Idx++)
    {
        pu16Reply[u16Idx] = s_au16ArmReply[u16Idx];
    }
    return s_u16ArmReplyLen;
}

/* ================= 装配与流水线 ================= */

static const Uint16 s_au16ArmPrefix[MB_ARM_PREFIX_LEN] =
{
    0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U
};

static void ProtoSetUp(void)
{
    Uint16 u16Idx;

    TableSetUp();
    ModbusProtoClear();
    s_stInst.u16SciId = HW_SCI_ID_A;
    s_stInst.u16Type = (Uint16)PROTO_TYPE_MODBUS;
    s_stInst.u16SlaveAddr = 1U;
    s_stInst.u16Enabled = 1U;
    s_stInst.u16TblCount = PROTO_TBL_COUNT;
    s_stInst.u32Baud = 115200UL;          /* > 19200：t3.5 固定 1750 µs */
    s_stInst.pstTbl = (const void *)s_astProtoTbl;
    s_stInst.pCtx = ModbusProtoState(0U);

    s_u16SegHead = 0U;
    s_u16SegTail = 0U;
    s_u16RxState = SCI_RX_IDLE;
    s_u32ForceResets = 0UL;
    s_u16Gen = 1U;
    s_u16StreamBreak = 0U;
    s_u16TxSuspended = 0U;
    s_u16TxFree = STUB_TX_DEPTH;
    s_u16TxIdle = 1U;
    s_u16TxLen = 0U;
    s_u16PopCalls = 0U;
    s_u32Now = 1000000UL;
    s_u16ArmCalls = 0U;
    s_u16ArmReplyLen = 0U;
    for (u16Idx = 0U; u16Idx < PROTO_TBL_COUNT; u16Idx++)
    {
        s_au16Regs[u16Idx] = 0U;
    }

    Check(stModbusProtoOps.pfInit(&s_stInst) == 1U, "S-05D-09 实例初始化成功");
}

static void RunService(void)
{
    (void)stModbusProtoOps.pfService(&s_stInst);
}

/* 消费一批字节 → 越过 t3.5 → 静默确认并执行 → 提交回复 */
static void DeliverAndSettle(const Uint16 *pu16Frame, Uint16 u16Len)
{
    PushBytes(pu16Frame, u16Len);
    RunService();          /* 消费 */
    Advance(3000UL);       /* 越过 1750 µs 静默 */
    RunService();          /* 确认并执行，装载回复 */
    RunService();          /* 提交回复 */
    StubTxDrain();
}

/* ================= S-05D-09 静默确认与基本回复 ================= */

static void TestProtoBasics(void)
{
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    s_au16Regs[0] = 0x1234U;
    u16Len = MakeRead(5000U, 1U, au16Frame);
    PushBytes(au16Frame, u16Len);

    RunService();                    /* 收到 8 字节，静默不足 */
    Check(s_u16TxLen == 0U, "S-05D-09 静默不足不回复");

    Advance(3000UL);
    RunService();                    /* 端口已空 → 按静默确认并执行 */
    Check(s_u16TxLen == 0U, "S-05D-09 执行当轮还没提交");

    RunService();                    /* TX 阶段提交回复 */
    Check(s_u16TxLen == 7U, "S-05D-09 0x03 回复 7 字节");
    Check((s_au16Tx[0] == 0x01U) && (s_au16Tx[1] == 0x03U), "S-05D-09 回复头正确");
    Check((s_au16Tx[3] == 0x12U) && (s_au16Tx[4] == 0x34U), "S-05D-09 寄存器值正确");
    Check(ModbusProtoTxSubmitted(0U) == 1UL, "S-05D-09 已提交计数");
}

/* ================= S-05D-10 配额耗尽与积压 ================= */

static void TestProtoQuota(void)
{
    Uint16 au16Body[40];
    Uint16 au16Frame[48];
    Uint16 u16Len;
    Uint16 u16Idx;

    ProtoSetUp();
    /* 0x10 写 12 个寄存器（5000..5011）：7 字节头 + 24 字节数据 + 2 字节 CRC = 33 字节 */
    au16Body[0] = 0x01U;
    au16Body[1] = 0x10U;
    au16Body[2] = 0x13U;
    au16Body[3] = 0x88U;              /* 5000 */
    au16Body[4] = 0x00U;
    au16Body[5] = 0x0CU;              /* 12 个寄存器 */
    au16Body[6] = 0x18U;              /* 24 字节 */
    for (u16Idx = 0U; u16Idx < 24U; u16Idx++)
    {
        au16Body[7U + u16Idx] = 0x00U;
    }
    au16Body[7] = 0x00U;
    au16Body[8] = 0x55U;              /* 5000 = 0x0055 */
    u16Len = BuildFrame(au16Frame, au16Body, 31U);
    Check(u16Len == 33U, "S-05D-10 帧长 33 字节");
    PushBytes(au16Frame, u16Len);

    RunService();                     /* 七段 4 字节后剩余额度 4 < 8，本轮收手 */
    Check(StubRxPending() == 5U, "S-05D-10 本轮消费 28 字节，剩 5 字节");
    Check(ModbusProtoRxBacklog(0U) == 1UL, "S-05D-10 积压轮数被记录");
    Check(s_au16Regs[0] == 0U, "S-05D-10 积压未清不算静默，不执行");

    RunService();                     /* 取空剩余 5 字节 */
    Check(StubRxPending() == 0U, "S-05D-10 第二轮取空");
    Check(s_au16Regs[0] == 0U, "S-05D-10 取空当轮静默仍为 0，不执行");

    Advance(3000UL);
    RunService();                     /* 静默足够 → 执行 */
    Check(s_au16Regs[0] == 0x0055U, "S-05D-10 静默确认后执行写入");

    RunService();                     /* 提交 0x10 回复 */
    Check(s_u16TxLen == 8U, "S-05D-10 0x10 回复 8 字节");
}

/* ================= S-05D-11 新字节不证明上一帧结束 ================= */

static void TestProtoTwoFrames(void)
{
    Uint16 au16Frame[32];
    Uint16 au16Frame2[32];
    Uint16 u16Len;
    Uint16 u16Len2;

    ProtoSetUp();
    u16Len = MakeWriteSingle(5000U, 0x0011U, au16Frame);
    u16Len2 = MakeWriteSingle(5000U, 0x0022U, au16Frame2);

    PushBytes(au16Frame, u16Len);
    RunService();                     /* 8 字节收满，还没有边界信号 */
    Check(s_au16Regs[0] == 0U, "S-05D-11 第一帧未确认前不执行");

    /* 同一批次/同一时刻紧跟着来第二帧：间隔不足静默，不能当作第一帧结束。 */
    PushBytes(au16Frame2, u16Len2);
    RunService();
    Check(s_au16Regs[0] == 0U, "S-05D-11 无静默证据时不执行写命令");
    Check(s_u16TxLen == 0U, "S-05D-11 也没有生成回复");

    /* 只剩第二帧的候选在途：越过静默后按第二帧执行。 */
    Advance(3000UL);
    RunService();
    Check(s_au16Regs[0] == 0x0022U, "S-05D-11 静默后只执行第二帧");

    RunService();
    Check(ModbusProtoTxSubmitted(0U) == 1UL, "S-05D-11 只提交一条回复");
}

/* ================= S-05D-12 回复忙不覆盖 ================= */

static void TestProtoReplyBusy(void)
{
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    s_u16TxFree = 0U;                 /* 端口环满：回复提交不下去 */

    u16Len = MakeWriteSingle(5000U, 0x0011U, au16Frame);
    PushBytes(au16Frame, u16Len);
    RunService();                     /* 消费 */
    Advance(3000UL);
    RunService();                     /* 执行第一条，回复装载 */
    Check(s_au16Regs[0] == 0x0011U, "S-05D-12 第一条执行");

    RunService();                     /* 尝试提交，环满 */
    Check(ModbusProtoTxSubmitted(0U) == 0UL, "S-05D-12 回复仍未提交");

    u16Len = MakeWriteSingle(5000U, 0x0022U, au16Frame);
    PushBytes(au16Frame, u16Len);
    RunService();
    Advance(3000UL);
    RunService();                     /* 回复忙：第二条不得执行 */
    Check(s_au16Regs[0] == 0x0011U, "S-05D-12 回复忙时不执行新写");

    s_u16TxFree = STUB_TX_DEPTH;
    RunService();                     /* 端口恢复后补提交 */
    Check(ModbusProtoTxSubmitted(0U) == 1UL, "S-05D-12 端口恢复后补提交");
    Check((s_au16Tx[4] == 0x00U) && (s_au16Tx[5] == 0x11U),
          "S-05D-12 回复内容仍是第一条");
}

/* ================= S-05D-13 ARM 升级入口 ================= */

static void TestProtoArm(void)
{
    Uint16 u16PopBefore;
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_A, (void *)0, ArmHook);
    s_au16ArmReply[0] = 0x01U;
    s_au16ArmReply[1] = 0x10U;
    s_au16ArmReply[2] = 0x04U;
    s_au16ArmReply[3] = 0x38U;
    s_au16ArmReply[4] = 0x00U;
    s_au16ArmReply[5] = 0x02U;
    s_au16ArmReply[6] = 0xC1U;
    s_au16ArmReply[7] = 0x35U;
    s_u16ArmReplyLen = 8U;

    PushBytes(s_au16ArmPrefix, MB_ARM_PREFIX_LEN);
    RunService();                     /* 命中回调并装载握手回复 */
    Check(s_u16ArmCalls == 1U, "S-05D-13 六字节起点触发回调");
    Check(ModbusProtoArmPending(0U) == 1U, "S-05D-13 握手待提交");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-13 握手未发完不交接");

    RunService();                     /* 提交握手回复 */
    Check(s_u16TxLen == 8U, "S-05D-13 握手回复 8 字节入环");
    Check(ModbusProtoArmPending(0U) == 0U, "S-05D-13 提交完成后不再待提交");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-13 硬件未发完仍不交接");

    StubTxDrain();
    RunService();                     /* 软件环与硬件都空 → 交接 */
    Check(ModbusProtoArmActive(0U) == 1U, "S-05D-13 发送完成后交接端口");

    /* 交接后本层不再消费 RX */
    u16PopBefore = s_u16PopCalls;
    PushBytes(s_au16ArmPrefix, MB_ARM_PREFIX_LEN);
    RunService();
    Check((s_u16PopCalls == u16PopBefore) && (StubRxPending() == MB_ARM_PREFIX_LEN),
          "S-05D-13 交接后不再读端口");

    ModbusProtoArmRelease(0U);
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-13 释放后交还端口");

    /* 释放后普通请求继续可用 */
    s_u16SegHead = 0U;
    s_u16SegTail = 0U;
    s_au16Regs[0] = 0x7777U;
    u16Len = MakeRead(5000U, 1U, au16Frame);
    DeliverAndSettle(au16Frame, u16Len);
    Check(s_u16TxLen != 0U, "S-05D-13 释放后普通读恢复");
}

/* ================= S-05D-14 回调拒绝时不影响普通请求 ================= */

static void TestProtoArmReject(void)
{
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_A, (void *)0, ArmHook);
    s_u16ArmReplyLen = 0U;            /* 回调不接受：按普通 Modbus 继续 */

    u16Len = MakeRead(5000U, 1U, au16Frame);
    DeliverAndSettle(au16Frame, u16Len);
    Check(s_u16ArmCalls == 0U, "S-05D-14 普通帧不触发回调");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-14 普通帧不交接端口");
    Check(s_u16TxLen == 7U, "S-05D-14 普通读照常回复");
}

/* ================= S-05D-15 非 ARM 端口不识别升级起点 ================= */

static void TestProtoArmPortFilter(void)
{
    Uint16 au16Body[16];
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_C, (void *)0, ArmHook);   /* 只对 C 口生效 */
    s_u16ArmReplyLen = 8U;

    /* 完整的一帧 0x10：地址 0x0438、数量 2、4 字节数据 → 13 字节。 */
    au16Body[0] = 0x01U;
    au16Body[1] = 0x10U;
    au16Body[2] = 0x04U;
    au16Body[3] = 0x38U;
    au16Body[4] = 0x00U;
    au16Body[5] = 0x02U;
    au16Body[6] = 0x04U;
    au16Body[7] = 0x00U;
    au16Body[8] = 0x00U;
    au16Body[9] = 0x00U;
    au16Body[10] = 0x00U;
    u16Len = BuildFrame(au16Frame, au16Body, 11U);
    PushBytes(au16Frame, u16Len);
    RunService();
    Check(s_u16ArmCalls == 0U, "S-05D-15 非 ARM 端口不匹配");

    /* 按普通 0x10 处理：0x0438 不在点表内 → 异常应答 01 90 02 + CRC（5 字节）。 */
    Advance(3000UL);
    RunService();
    RunService();
    Check((s_u16TxLen == 5U) && (s_au16Tx[1] == 0x90U) && (s_au16Tx[2] == 0x02U),
          "S-05D-15 非 ARM 端口按普通请求处理");
}

/* ================= S-05D-16 强制复位取消交接 ================= */

static void TestProtoArmForceReset(void)
{
    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_A, (void *)0, ArmHook);
    s_u16ArmReplyLen = 8U;

    PushBytes(s_au16ArmPrefix, MB_ARM_PREFIX_LEN);
    RunService();
    Check(ModbusProtoArmPending(0U) == 1U, "S-05D-16 握手已装载");

    /* 握手还没提交完就发生强制复位：丢弃，不得把"丢弃"当成"已发完"。 */
    s_u32ForceResets++;
    RunService();
    Check(ModbusProtoArmPending(0U) == 0U, "S-05D-16 强制复位取消交接");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-16 未进入已交接状态");
}

/* ================= S-05D-17 ARM 起点跨批次识别 ================= */

static void TestProtoArmSplit(void)
{
    Uint16 u16Idx;
    Uint16 u16PopBefore;

    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_A, (void *)0, ArmHook);
    for (u16Idx = 0U; u16Idx < 8U; u16Idx++)
    {
        s_au16ArmReply[u16Idx] = 0U;
    }
    s_au16ArmReply[0] = 0x01U;
    s_au16ArmReply[1] = 0x10U;
    s_au16ArmReply[6] = 0xC1U;
    s_au16ArmReply[7] = 0x35U;
    s_u16ArmReplyLen = 8U;

    /* 第一批只有前 4 字节：不得触发回调，也不得漏进普通 Modbus */
    PushSeg(s_au16ArmPrefix, 4U);
    RunService();
    Check(s_u16ArmCalls == 0U, "S-05D-17 前缀未收全不触发回调");
    Check(StubRxPending() == 0U, "S-05D-17 已暂存的前缀不留在端口");
    Check(s_u16TxLen == 0U, "S-05D-17 匹配中不产生任何回复");
    Check(ModbusProtoArmPending(0U) == 0U, "S-05D-17 匹配中不算待提交");

    /* 第二批补齐后 2 字节：跨批次完成匹配 */
    PushSeg(&s_au16ArmPrefix[4], 2U);
    RunService();
    Check(s_u16ArmCalls == 1U, "S-05D-17 跨批次补齐后触发回调");
    Check(ModbusProtoArmPending(0U) == 1U, "S-05D-17 握手待提交");

    RunService();
    Check(s_u16TxLen == 8U, "S-05D-17 握手回复 8 字节");
    StubTxDrain();
    RunService();
    Check(ModbusProtoArmActive(0U) == 1U, "S-05D-17 跨批次识别后正常交接");

    u16PopBefore = s_u16PopCalls;
    PushBytes(s_au16ArmPrefix, MB_ARM_PREFIX_LEN);
    RunService();
    Check(s_u16PopCalls == u16PopBefore, "S-05D-17 交接后不再读端口");

    ModbusProtoArmRelease(0U);
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-17 释放后可交还端口");
}

/* ================= S-05D-18 探询前重新取时标 ================= */

static void TestProtoPollFresh(void)
{
    Uint16 au16Body[40];
    Uint16 au16Frame[48];
    Uint16 u16Len;
    Uint16 u16Idx;

    ProtoSetUp();
    au16Body[0] = 0x01U;
    au16Body[1] = 0x10U;
    au16Body[2] = 0x13U;
    au16Body[3] = 0x88U;              /* 5000 */
    au16Body[4] = 0x00U;
    au16Body[5] = 0x0CU;              /* 12 个寄存器 */
    au16Body[6] = 0x18U;
    for (u16Idx = 0U; u16Idx < 24U; u16Idx++)
    {
        au16Body[7U + u16Idx] = 0x00U;
    }
    au16Body[7] = 0x12U;
    au16Body[8] = 0x34U;              /* 5000 = 0x1234 */
    u16Len = BuildFrame(au16Frame, au16Body, 31U);
    Check(u16Len == 33U, "S-05D-18 帧长 33 字节");

    /* 先投 32 字节（帧未完成），最后一字节在探询期间到达 */
    PushBytes(au16Frame, 32U);
    RunService();
    Check(s_au16Regs[0] == 0U, "S-05D-18 帧未完成不执行");
    Check(StubRxPending() == 4U, "S-05D-18 本轮消费 28 字节，剩 4 字节");

    s_u16LateByte = au16Frame[32];
    s_u16LateArmed = 1U;
    RunService();                     /* 最后一字节比入口时标新 50 tick */
    Check(s_au16Regs[0] == 0U, "S-05D-18 实际静默为 0，不得执行");

    Advance(3000UL);
    RunService();                     /* 静默足够后执行 */
    Check(s_au16Regs[0] == 0x1234U, "S-05D-18 真正静默后才执行写入");

    RunService();
    Check(s_u16TxLen == 8U, "S-05D-18 0x10 回复 8 字节");
}

/* ================= S-05D-19 回调拒绝时回放下标 ================= */

static void TestProtoArmReplayIndex(void)
{
    Uint16 au16Body[16];
    Uint16 au16Frame[24];
    Uint16 u16Len;

    ProtoSetUp();
    ModbusProtoSetArmHook(HW_SCI_ID_A, (void *)0, ArmHook);
    s_u16ArmReplyLen = 0U;            /* 回调拒绝接管 */

    /* 合法 0x10 请求，但前六字节恰好等于升级起点：01 10 04 38 00 02 04 .... */
    au16Body[0] = 0x01U;
    au16Body[1] = 0x10U;
    au16Body[2] = 0x04U;
    au16Body[3] = 0x38U;              /* 地址 1080：点表里不存在 */
    au16Body[4] = 0x00U;
    au16Body[5] = 0x02U;
    au16Body[6] = 0x04U;              /* 4 字节数据 */
    au16Body[7] = 0x00U;
    au16Body[8] = 0x00U;
    au16Body[9] = 0x00U;
    au16Body[10] = 0x00U;
    u16Len = BuildFrame(au16Frame, au16Body, 11U);
    Check(u16Len == 13U, "S-05D-19 合法请求 13 字节");

    PushBytes(au16Frame, u16Len);
    RunService();
    Check(s_u16ArmCalls == 1U, "S-05D-19 前缀命中并调用回调");

    Advance(3000UL);
    RunService();                     /* 静默确认 → 生成异常应答 */
    RunService();                     /* 提交应答 */
    Check(s_u16TxLen == 5U, "S-05D-19 回放后按普通 Modbus 应答 5 字节");
    Check((s_au16Tx[0] == 0x01U) && (s_au16Tx[1] == 0x90U) && (s_au16Tx[2] == 0x02U),
          "S-05D-19 非法地址异常 0x02");
}

int main(void)
{
    TestProtoBasics();
    TestProtoQuota();
    TestProtoTwoFrames();
    TestProtoReplyBusy();
    TestProtoArm();
    TestProtoArmReject();
    TestProtoArmPortFilter();
    TestProtoArmForceReset();
    TestProtoArmSplit();
    TestProtoPollFresh();
    TestProtoArmReplayIndex();

    printf("checks=%d fails=%d\n", s_iChecks, s_iFails);

    return (s_iFails == 0) ? 0 : 1;
}

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
 * 时标语义：Timer0 向下计数，"过去多久" = (Uint32)(last - now)，替身同向。
 * 流水线：RX 阶段消费与执行（回复在此装载）→ 下一次调用的 TX 阶段才入端口环。
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

/* ================= SciPort 替身 ================= */

#define STUB_RX_DEPTH   (512U)
#define STUB_TX_DEPTH   (512U)

static Uint16 s_au16Rx[STUB_RX_DEPTH];
static Uint16 s_u16RxHead = 0U;
static Uint16 s_u16RxTail = 0U;
static Uint16 s_u16RxState = SCI_RX_IDLE;
static Uint32 s_u32ForceResets = 0UL;
static Uint16 s_u16Gen = 1U;
static Uint16 s_u16StreamBreak = 0U;
static Uint16 s_u16TxSuspended = 0U;
static Uint16 s_u16TxFree = STUB_TX_DEPTH;
static Uint16 s_au16Tx[STUB_TX_DEPTH];
static Uint16 s_u16TxLen = 0U;
static Uint16 s_u16PopCalls = 0U;

Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max,
                       ST_SCI_RX_SEG *pstSeg)
{
    Uint16 u16Got = 0U;

    (void)u16PortId;
    s_u16PopCalls++;
    while ((u16Got < u16Max) && (s_u16RxHead < s_u16RxTail))
    {
        pu16Dst[u16Got] = s_au16Rx[s_u16RxHead];
        s_u16RxHead++;
        u16Got++;
    }
    if (pstSeg != (ST_SCI_RX_SEG *)0)
    {
        pstSeg->u32Tick = s_u32Now;
        pstSeg->u32TickEmptyLast = s_u32Now;
        pstSeg->u16SeqStart = 0U;
        pstSeg->u16Count = u16Got;
        pstSeg->u16EmptyRounds = (u16Got == 0U) ? 1U : 0U;
    }
    return u16Got;
}

Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Accept = 0U;

    (void)u16PortId;
    if (s_u16TxSuspended != 0U)
    {
        return 0U;
    }
    while ((u16Accept < u16Count) && (s_u16TxFree != 0U) &&
           (s_u16TxLen < STUB_TX_DEPTH))
    {
        s_au16Tx[s_u16TxLen] = pu16Data[u16Accept];
        s_u16TxLen++;
        s_u16TxFree--;
        u16Accept++;
    }
    return u16Accept;
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

static void PushBytes(const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < u16Count; u16Idx++)
    {
        s_au16Rx[s_u16RxTail] = (Uint16)(pu16Data[u16Idx] & 0xFFU);
        s_u16RxTail++;
    }
}

static Uint16 StubRxPending(void)
{
    return (Uint16)(s_u16RxTail - s_u16RxHead);
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

    s_u16RxHead = 0U;
    s_u16RxTail = 0U;
    s_u16RxState = SCI_RX_IDLE;
    s_u32ForceResets = 0UL;
    s_u16Gen = 1U;
    s_u16StreamBreak = 0U;
    s_u16TxSuspended = 0U;
    s_u16TxFree = STUB_TX_DEPTH;
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
    /* 0x10 写 12 个寄存器（5000..5011）：帧长 9 + 24 + 2 = 35 字节，超过单轮 32 字节配额 */
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
    PushBytes(au16Frame, u16Len);

    RunService();                     /* 只取 32 字节，还剩 3 字节 */
    Check(StubRxPending() == 3U, "S-05D-10 单轮配额 32 字节");
    Check(ModbusProtoRxBacklog(0U) == 1UL, "S-05D-10 积压轮数被记录");
    Check(s_au16Regs[0] == 0U, "S-05D-10 积压未清不算静默，不执行");

    RunService();                     /* 取空剩余 3 字节 */
    Check(StubRxPending() == 0U, "S-05D-10 第二轮取空");
    Check(s_au16Regs[0] == 0U, "S-05D-10 取空当轮静默仍为 0，不执行");

    Advance(3000UL);
    RunService();                     /* 静默足够 → 执行 */
    Check(s_au16Regs[0] == 0x0055U, "S-05D-10 静默确认后执行写入");

    RunService();                     /* 提交 0x10 回复 */
    Check(s_u16TxLen == 8U, "S-05D-10 0x10 回复 8 字节");
}

/* ================= S-05D-11 两帧分批积压 ================= */

static void TestProtoTwoFrames(void)
{
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    u16Len = MakeWriteSingle(5000U, 0x0011U, au16Frame);
    PushBytes(au16Frame, u16Len);
    RunService();                     /* 8 字节收满，还没有边界信号 */
    Check(s_au16Regs[0] == 0U, "S-05D-11 第一帧未确认前不执行");

    u16Len = MakeWriteSingle(5000U, 0x0022U, au16Frame);
    PushBytes(au16Frame, u16Len);
    RunService();                     /* 新字节到了 → 先执行第一帧 */
    Check(s_au16Regs[0] == 0x0011U, "S-05D-11 第一帧按新帧起点确认并执行");

    RunService();                     /* 提交第一帧回复 */
    Check(s_u16TxLen == 8U, "S-05D-11 第一帧回复已入队");

    Advance(3000UL);
    RunService();                     /* 第二帧按静默执行 */
    RunService();                     /* 提交第二帧回复 */
    Check(s_au16Regs[0] == 0x0022U, "S-05D-11 第二帧也执行");
    Check(ModbusProtoTxSubmitted(0U) == 2UL, "S-05D-11 两条回复都提交");
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
    static const Uint16 au16Prefix[MB_ARM_PREFIX_LEN] =
    {
        0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U
    };
    Uint16 u16PopBefore;
    Uint16 au16Frame[32];
    Uint16 u16Len;

    ProtoSetUp();
    ModbusProtoSetArmHook((void *)0, ArmHook);
    s_au16ArmReply[0] = 0x01U;
    s_au16ArmReply[1] = 0x10U;
    s_au16ArmReply[2] = 0x00U;
    s_u16ArmReplyLen = 3U;

    PushBytes(au16Prefix, MB_ARM_PREFIX_LEN);
    RunService();                     /* 命中回调并装载握手回复 */
    Check(s_u16ArmCalls == 1U, "S-05D-13 六字节起点触发回调");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-13 握手未发完不交接");

    RunService();                     /* 提交握手回复 */
    Check(s_u16TxLen == 3U, "S-05D-13 握手回复已入队");

    RunService();                     /* 提交完成后交接 */
    Check((ModbusProtoTxSubmitted(0U) == 1UL) && (ModbusProtoArmActive(0U) == 1U),
          "S-05D-13 发送完成后交接端口");

    /* 交接后本层不再消费 RX */
    u16PopBefore = s_u16PopCalls;
    PushBytes(au16Prefix, MB_ARM_PREFIX_LEN);
    RunService();
    Check((s_u16PopCalls == u16PopBefore) && (StubRxPending() == MB_ARM_PREFIX_LEN),
          "S-05D-13 交接后不再读端口");

    ModbusProtoArmRelease(0U);
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-13 释放后交还端口");

    /* 释放后普通请求继续可用 */
    s_u16RxHead = 0U;
    s_u16RxTail = 0U;
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
    ModbusProtoSetArmHook((void *)0, ArmHook);
    s_u16ArmReplyLen = 0U;            /* 回调不接受：按普通 Modbus 继续 */

    u16Len = MakeRead(5000U, 1U, au16Frame);
    DeliverAndSettle(au16Frame, u16Len);
    Check(s_u16ArmCalls == 0U, "S-05D-14 普通帧不触发回调");
    Check(ModbusProtoArmActive(0U) == 0U, "S-05D-14 普通帧不交接端口");
    Check(s_u16TxLen == 7U, "S-05D-14 普通读照常回复");
}

int main(void)
{
    TestProtoBasics();
    TestProtoQuota();
    TestProtoTwoFrames();
    TestProtoReplyBusy();
    TestProtoArm();
    TestProtoArmReject();

    printf("checks=%d fails=%d\n", s_iChecks, s_iFails);

    return (s_iFails == 0) ? 0 : 1;
}

/**
 * @file port_stub_test.c
 * @brief 端口层主机桩测试（对应步骤 05 文档 §5.1，S-05A-03 ~ S-05A-25）
 * @version 0.5
 * @date 2026-11-05
 *
 * 编译（主机 cc，不进目标构建）：
 *   cc -DPORT_CRITICAL_HOOKS -std=c99 -Wall -Wextra -Ishim -I../Inc -I../../Common/Inc \
 *      port_stub_test.c ../Src/SciPort.c ../Src/CanPort.c ../Src/PortTask.c \
 *      ../Src/PortCritical.c \
 *      ../../Common/Src/RingBuff.c ../../Common/Src/CanQueue.c -o port_stub_test
 *
 * 桩只暴露真实驱动能给出的信息：读一字节成功/失败、接收错误标志、发送状态码、
 * 递减时标、模块是否仍在 pending。不提供到达时刻、帧边界等硬件给不出的量。
 *
 * v0.4 修正（对应 2026-11-05 复审）：
 *   1. 批次记录单元按 Uint16 个数计算；
 *   2. PortTask 用显式映射表把槽位换成硬件 ID，用例验证三路实际发送记录；
 *   3. CAN 发送前查 pfIsPending()，用例验证 pending 时不发起事务；
 *   4. SCI 接收错误每轮都查（含 FIFO 空），用例验证只报错也置流中断；
 *   5. 预算下沉到服务层，用例让单次接收消耗 30 µs，验证只发起一次；
 *   6. 重同步改为跨轮持续丢弃，另加故障代次确认竞态用例；
 *   7. CAN 入队校验帧类型与 RTR。
 */

#include <stdio.h>
#include "SciPort.h"
#include "CanPort.h"
#include "PortTask.h"
#include "PortBudget.h"

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

/* ================= 时间基准桩（递减计数器） ================= */

#define TICKS_PER_US (200UL)

static Uint32 s_u32Tick = 1000000UL;
static Uint16 s_bTickOk = 1U;
static Uint32 s_u32TickStepTicks = 0UL;   /* 每次读取后自动递减（模拟读取耗时） */
static Uint32 s_u32TickCalls = 0UL;       /* 本轮累计读取次数 */
static Uint32 s_u32TickFailAtCall = 0UL;  /* 1 起算：仅第 N 次读取失败，随后恢复 */

static Uint16 TestTick(Uint32 *pu32Tick)
{
    s_u32TickCalls++;

    if (s_bTickOk == 0U)
    {
        return 0U;
    }

    /* 一次性的中途失败：模拟时间源短暂不可用后自行恢复。 */
    if ((s_u32TickFailAtCall != 0UL) && (s_u32TickCalls == s_u32TickFailAtCall))
    {
        return 0U;
    }

    *pu32Tick = s_u32Tick;

    if (s_u32TickStepTicks != 0UL)
    {
        s_u32Tick -= s_u32TickStepTicks;
    }

    return 1U;
}

Uint16 Hw_TimeBaseIsReady(void) { return s_bTickOk; }
Uint32 Hw_TimeBaseTicks(void)   { return s_u32Tick; }
Uint16 Hw_TimeBaseInit(Uint32 u32SysclkHz) { (void)u32SysclkHz; return 1U; }

static void TicksAdvanceUs(Uint32 u32Us)
{
    s_u32Tick -= (u32Us * TICKS_PER_US);
}

static void TickReset(void)
{
    s_u32Tick = 1000000UL;
    s_bTickOk = 1U;
    s_u32TickStepTicks = 0UL;
    s_u32TickCalls = 0UL;
    s_u32TickFailAtCall = 0UL;
}

/* ================= SCI 桩 ================= */

#define STUB_FEED_MAX   (512U)
#define STUB_TX_LOG_MAX (64U)

static Uint16 s_au16Feed[4][STUB_FEED_MAX];
static Uint16 s_au16FeedLen[4];
static Uint16 s_au16FeedPos[4];
static Uint16 s_au16Attempts[4];          /* pfTryRead 调用次数（含失败探测） */
static Uint16 s_au16ErrFlag[4];
static Uint16 s_au16TxAccept[4];
static Uint16 s_au16TxLog[4][STUB_TX_LOG_MAX];
static Uint16 s_au16TxLogLen[4];
static Uint16 s_u16StatusOk[4];        /* Hw_SciGetRxStatus() 是否可读 */
static Uint16 s_u16ClearKind[4];       /* 0=正常 1=WAIT_TX 2=STILL_ERR 3=UNREADABLE */
static Uint16 s_u16TxIdle[4];          /* Hw_SciGetTxIdle() 报告的空闲值 */
static Uint16 s_u16TxIdleQueryOk[4];   /* Hw_SciGetTxIdle() 查询是否成功 */
static Uint32 s_u32ClearCalls;         /* Hw_SciClearRxError() 调用次数 */

Uint16 Hw_SciTryReadByte(Uint16 id, Uint16 *data)
{
    if (id > 3U)
    {
        return 0U;
    }

    s_au16Attempts[id]++;

    if (s_au16FeedPos[id] >= s_au16FeedLen[id])
    {
        return 0U;
    }

    *data = s_au16Feed[id][s_au16FeedPos[id]];
    s_au16FeedPos[id]++;
    return 1U;
}

Uint16 Hw_SciTryWriteByte(Uint16 id, Uint16 data)
{
    if ((id > 3U) || (s_au16TxAccept[id] == 0U))
    {
        return 0U;
    }

    s_au16TxAccept[id]--;

    if (s_au16TxLogLen[id] < STUB_TX_LOG_MAX)
    {
        s_au16TxLog[id][s_au16TxLogLen[id]] = data;
        s_au16TxLogLen[id]++;
    }

    return 1U;
}

Uint16 Hw_SciGetRxError(Uint16 id)
{
    return (id > 3U) ? 0U : s_au16ErrFlag[id];
}

/* 接收状态 + 分类错误位。bClear 只清状态位，FIFO 复位由 Hw_SciClearRxError() 负责。 */
Uint16 Hw_SciGetRxStatus(Uint16 id, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear)
{
    if ((id > 3U) || (pstStatus == (ST_SCI_RX_STATUS *)0))
    {
        return 0U;
    }

    if (s_u16StatusOk[id] == 0U)
    {
        return 0U;
    }

    pstStatus->u16Status = (s_au16ErrFlag[id] != 0U) ? HW_SCI_RXSTS_RXFFOVF : 0U;
    pstStatus->u16RxFifoLevel = (Uint16)(s_au16FeedLen[id] - s_au16FeedPos[id]);
    pstStatus->u16TxFifoLevel = 0U;

    if (bClear != 0U)
    {
        s_au16ErrFlag[id] = 0U;
    }

    return 1U;
}

/* 接收错误清除：可编程返回四种结果，模拟真实的"等发送 / 仍报错 / 读不到"。 */
Uint16 Hw_SciClearRxError(Uint16 id, Uint16 *pu16Remain)
{
    if ((id > 3U) || (pu16Remain == (Uint16 *)0))
    {
        return HW_SCI_CLEAR_UNREADABLE;
    }

    s_u32ClearCalls++;
    *pu16Remain = (Uint16)(s_au16FeedLen[id] - s_au16FeedPos[id]);

    if (s_u16ClearKind[id] == 1U)
    {
        return HW_SCI_CLEAR_WAIT_TX;
    }

    if (s_u16ClearKind[id] == 2U)
    {
        return HW_SCI_CLEAR_STILL_ERR;
    }

    if (s_u16ClearKind[id] == 3U)
    {
        return HW_SCI_CLEAR_UNREADABLE;
    }

    /* 正常路径：软复位后硬件错误消失、残留字节被丢弃。 */
    s_au16ErrFlag[id] = 0U;
    s_au16FeedLen[id] = 0U;
    s_au16FeedPos[id] = 0U;

    return HW_SCI_CLEAR_OK;
}

Uint16 Hw_SciGetTxIdle(Uint16 id, Uint16 *pu16Idle)
{
    if ((id > 3U) || (pu16Idle == (Uint16 *)0))
    {
        return 0U;
    }

    if (s_u16TxIdleQueryOk[id] == 0U)
    {
        return 0U;
    }

    *pu16Idle = s_u16TxIdle[id];

    return 1U;
}

Uint16 Hw_SciIsTxIdle(Uint16 id)
{
    Uint16 u16Idle = 0U;

    return (Hw_SciGetTxIdle(id, &u16Idle) != 0U) ? u16Idle : 0U;
}

static void SciStubReset(void)
{
    Uint16 i;

    for (i = 0U; i < 4U; i++)
    {
        s_au16FeedLen[i] = 0U;
        s_au16FeedPos[i] = 0U;
        s_au16Attempts[i] = 0U;
        s_au16ErrFlag[i] = 0U;
        s_au16TxAccept[i] = 0U;
        s_au16TxLogLen[i] = 0U;
        s_u16StatusOk[i] = 1U;
        s_u16ClearKind[i] = 0U;
        s_u16TxIdle[i] = 1U;
        s_u16TxIdleQueryOk[i] = 1U;
    }

    s_u32ClearCalls = 0UL;
}

static void SciFeed(Uint16 id, Uint16 u16Byte)
{
    if (s_au16FeedLen[id] < STUB_FEED_MAX)
    {
        s_au16Feed[id][s_au16FeedLen[id]] = u16Byte;
        s_au16FeedLen[id]++;
    }
}

static void SciFeedRun(Uint16 id, Uint16 u16Start, Uint16 u16Count)
{
    Uint16 i;

    for (i = 0U; i < u16Count; i++)
    {
        SciFeed(id, (Uint16)(u16Start + i));
    }
}

/* ================= 临界区钩子（延迟投递真正作用到端口状态） ================= */

static Uint16 s_u16Masked;
static Uint16 s_u16Depth;
static Uint16 s_u16PendingFault;
static Uint16 s_u16Delivered;
static Uint16 s_u16ExitCount;

/* 投递动作不只是记变量：让驱动报错并跑一轮服务，
 * 相当于"中断在恢复后立刻执行了一次接收服务"，端口必须真的变成流中断。 */
static void DeliverFaultToPort(void)
{
    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    (void)SciPortService100us();
    s_au16ErrFlag[HW_SCI_ID_A] = 0U;
}

ST_PORT_IRQ_STATE PortEnterCriticalHook(void)
{
    ST_PORT_IRQ_STATE u16Saved = s_u16Masked;

    s_u16Masked = 1U;
    s_u16Depth++;
    return u16Saved;
}

void PortExitCriticalHook(ST_PORT_IRQ_STATE u16State)
{
    s_u16Depth--;
    s_u16Masked = u16State;
    s_u16ExitCount++;

    if ((s_u16Masked == 0U) && (s_u16PendingFault != 0U))
    {
        s_u16PendingFault = 0U;
        s_u16Delivered = 1U;
        DeliverFaultToPort();
    }
}

static void IrqReset(void)
{
    s_u16Masked = 0U;
    s_u16Depth = 0U;
    s_u16PendingFault = 0U;
    s_u16Delivered = 0U;
    s_u16ExitCount = 0U;
}

/* ================= CAN 桩 ================= */

static Uint16 s_u16CanInited = 1U;
static Uint16 s_u16CanPending = 0U;
static Uint16 s_u16CanReadRet = HW_CAN_EMPTY;
static Uint16 s_u16CanSendRet = HW_CAN_OK;
static Uint16 s_u16CanMsgLst = 0U;
static Uint16 s_u16CanReadOkLeft = 0U;    /* 还能返回多少帧 OK，之后返回 EMPTY */
static Uint32 s_u32CanReadCount;
static Uint32 s_u32CanSendCount;
static Uint32 s_u32CanSendLastId;
static Uint16 s_u16CanSendLastData0;
static Uint32 s_u32CanReadCostTicks = 0UL; /* 每次接收消耗的 tick（模拟慢驱动） */

Uint16 Hw_CanIsInitialized(Uint16 u16CanId) { (void)u16CanId; return s_u16CanInited; }
Uint16 Hw_CanIsPending(Uint16 u16CanId)     { (void)u16CanId; return s_u16CanPending; }

Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len,
                    Uint16 *pu16FrameType, Uint16 *pu16MsgLst)
{
    Uint16 i;

    (void)u16CanId;
    (void)u16ObjIdx;

    s_u32CanReadCount++;

    if (s_u32CanReadCostTicks != 0UL)
    {
        s_u32Tick -= s_u32CanReadCostTicks;
    }

    if (s_u16CanReadRet != HW_CAN_OK)
    {
        return s_u16CanReadRet;
    }

    if (s_u16CanReadOkLeft == 0U)
    {
        return HW_CAN_EMPTY;
    }

    s_u16CanReadOkLeft--;

    *pu32MsgId = 0x123UL;
    *pu16Len = 2U;
    *pu16FrameType = HW_CAN_FRAME_STD;
    *pu16MsgLst = s_u16CanMsgLst;

    for (i = 0U; i < 8U; i++)
    {
        pu16Data[i] = (Uint16)(0x40U + i);
    }

    return HW_CAN_OK;
}

Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len)
{
    (void)u16CanId;
    (void)u16ObjIdx;
    (void)u16Len;

    s_u32CanSendCount++;
    s_u32CanSendLastId = u32MsgId;
    s_u16CanSendLastData0 = pu16Data[0];

    return s_u16CanSendRet;
}

static void CanStubReset(void)
{
    s_u16CanInited = 1U;
    s_u16CanPending = 0U;
    s_u16CanReadRet = HW_CAN_EMPTY;
    s_u16CanSendRet = HW_CAN_OK;
    s_u16CanMsgLst = 0U;
    s_u16CanReadOkLeft = 0U;
    s_u32CanReadCount = 0UL;
    s_u32CanSendCount = 0UL;
    s_u32CanSendLastId = 0UL;
    s_u16CanSendLastData0 = 0U;
    s_u32CanReadCostTicks = 0UL;
}

/* ================= 夹具 ================= */

static void Fixture(void)
{
    SciStubReset();
    CanStubReset();
    TickReset();
    IrqReset();
    SciPortInit();
    CanPortInit();
    PortTaskInit();
    PortTaskSetTimeSource(TestTick, 200000000UL);
}

static void RxRound(void)
{
    (void)SciPortService100us();
}

/* ================= 用例 ================= */

/* S-05A-01/02/19：初始化容量与单元到硬件 ID 的映射 */
static void TestInitAndMapping(void)
{
    Fixture();

    Check(SciPortTxFree(HW_SCI_ID_A) == 127U, "S-05A-01 tx free A");
    Check(SciPortTxFree(HW_SCI_ID_C) == 127U, "S-05A-01 tx free C");
    Check(CanPortTxFree(HW_CAN_ID_A) == 15U, "S-05A-02 tx free CAN-A");

    Check(PortTaskUnitPortId(0U) == HW_SCI_ID_A, "S-05A-19 unit0 = SCI-A");
    Check(PortTaskUnitPortId(1U) == HW_SCI_ID_C, "S-05A-19 unit1 = SCI-C");
    Check(PortTaskUnitPortId(2U) == HW_SCI_ID_D, "S-05A-19 unit2 = SCI-D");
    Check(PortTaskUnitPortId(3U) == HW_CAN_ID_A, "S-05A-19 unit3 = CAN-A");
    Check(PortTaskUnitPortId(4U) == HW_CAN_ID_B, "S-05A-19 unit4 = CAN-B");
    Check(PortTaskUnitPortId(5U) == 0xFFFFU, "S-05A-19 unit5 out of range");
}

/* S-05A-03：配额与探测计数（失败探测也计入 8 次上限） */
static void TestQuotaAndAttempts(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;
    Uint16 u16Total = 0U;
    Uint16 u16Got;

    Fixture();
    SciFeedRun(HW_SCI_ID_A, 1U, 20U);

    RxRound();   /* 首轮只建基准 */
    Check(s_au16Attempts[HW_SCI_ID_A] == 0U, "S-05A-03 baseline round does not read");

    RxRound();
    Check(s_au16Attempts[HW_SCI_ID_A] == 8U, "S-05A-03 first round 8 attempts");

    RxRound();
    Check(s_au16Attempts[HW_SCI_ID_A] == 16U, "S-05A-03 second round 8 attempts");

    RxRound();
    Check(s_au16Attempts[HW_SCI_ID_A] == 21U, "S-05A-03 failed probe counted (8+8+4+1)");

    while ((u16Got = SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg)) != 0U)
    {
        u16Total = (Uint16)(u16Total + u16Got);
    }

    Check(u16Total == 20U, "S-05A-03 all fed bytes published");
}

/* S-05A-04：两批之间保留空读观察与观察时刻 */
static void TestTwoBatchesWithEmpty(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg1;
    ST_SCI_RX_SEG stSeg2;
    Uint32 u32EmptyTick;

    Fixture();
    SciFeedRun(HW_SCI_ID_A, 10U, 3U);

    RxRound();              /* 建基准 */
    RxRound();              /* 批次 1 */
    RxRound();              /* 空读 1 */
    u32EmptyTick = s_u32Tick;
    RxRound();              /* 空读 2 */

    SciFeedRun(HW_SCI_ID_A, 20U, 5U);
    RxRound();              /* 批次 2，携带空读观察 */

    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg1) == 3U, "S-05A-04 batch1 count");
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg2) == 5U, "S-05A-04 batch2 count");
    Check(stSeg1.u16EmptyRounds == 0U, "S-05A-04 batch1 no empty rounds");
    Check(stSeg2.u16EmptyRounds == 2U, "S-05A-04 batch2 carries 2 empty rounds");
    Check(stSeg2.u32TickEmptyLast == u32EmptyTick, "S-05A-04 empty observation tick kept");
    Check(stSeg2.u16SeqStart == 3U, "S-05A-04 batch2 seq continues");
}

/* S-05A-05：服务间隔超限置 bTimingBad（只喂 1 字节也成立） */
static void TestGapBad(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;

    Fixture();
    RxRound();              /* 建基准 */
    SciFeed(HW_SCI_ID_A, 0xA5U);
    TicksAdvanceUs(1000UL);
    RxRound();

    Check(SciPortIsTimingBad(HW_SCI_ID_A) != 0U, "S-05A-05 gap 1000us sets timing bad");
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 1U, "S-05A-05 batch still published");
}

/* S-05A-06：部分发送后剩余字节与顺序 */
static void TestTxPartial(void)
{
    Uint16 au16Data[8];
    Uint16 u16FreeAfterEnqueue;
    Uint16 i;

    Fixture();
    for (i = 0U; i < 8U; i++)
    {
        au16Data[i] = (Uint16)(0xA1U + i);
    }

    Check(SciPortWriteTx(HW_SCI_ID_A, au16Data, 8U) == 8U, "S-05A-06 enqueue 8");
    u16FreeAfterEnqueue = SciPortTxFree(HW_SCI_ID_A);
    Check(u16FreeAfterEnqueue == 119U, "S-05A-06 free after enqueue is 119");

    s_au16TxAccept[HW_SCI_ID_A] = 3U;
    SciPortTaskOne(HW_SCI_ID_A);

    Check(SciPortTxFree(HW_SCI_ID_A) == 122U, "S-05A-06 free == enqueue free + 3");
    Check(s_au16TxLogLen[HW_SCI_ID_A] == 3U, "S-05A-06 accepted 3 bytes");
    Check(s_au16TxLog[HW_SCI_ID_A][0] == 0xA1U, "S-05A-06 byte0 order");
    Check(s_au16TxLog[HW_SCI_ID_A][2] == 0xA3U, "S-05A-06 byte2 order");
    Check(SciPortTxStall(HW_SCI_ID_A) == 0U, "S-05A-06 no stall on partial accept");

    s_au16TxAccept[HW_SCI_ID_A] = 8U;
    SciPortTaskOne(HW_SCI_ID_A);

    Check(s_au16TxLogLen[HW_SCI_ID_A] == 8U, "S-05A-06 remaining 5 sent");
    Check(s_au16TxLog[HW_SCI_ID_A][3] == 0xA4U, "S-05A-06 no duplicate/extra byte");
    Check(SciPortTxFree(HW_SCI_ID_A) == 127U, "S-05A-06 tx ring empty");
}

/* S-05A-07：硬件覆盖锁存（粘滞，只置一次；队列满也不丢证据） */
static void TestHwOverwriteLatch(void)
{
    ST_CAN_FRAME stFrame;
    Uint16 u16FreeBefore;
    Uint16 i;

    Fixture();

    /* TX 容量单独断言：16 槽、有效容量 15（不是 7）。 */
    Check(CanPortTxFree(HW_CAN_ID_A) == 15U, "S-05A-07 tx capacity is 15 slots");

    for (i = 0U; i < 8U; i++)
    {
        stFrame.id = 0x50UL + i;
        stFrame.flags = 0U;
        stFrame.dlc = 1U;
        stFrame.data[0] = i;
        (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);
    }

    u16FreeBefore = CanPortTxFree(HW_CAN_ID_A);

    /* 隔离 TX：CanPortServiceOne() 会依次服务 RX 与 TX，
       发送桩返回忙才能验证"接收不动 TX 队列、发送遇忙保留队头"。 */
    s_u16CanSendRet = HW_CAN_BUSY;

    s_u16CanReadRet = HW_CAN_OK;
    s_u16CanMsgLst = 1U;
    s_u16CanReadOkLeft = 3U;
    CanPortServiceOne(HW_CAN_ID_A);

    Check(CanPortHwOverwriteSeen(HW_CAN_ID_A) == 1U, "S-05A-07 latch after first overwrite");
    Check(CanPortTxFree(HW_CAN_ID_A) == u16FreeBefore, "S-05A-07 tx queue untouched by rx test");

    /* 软件帧环塞满后仍能锁存：4 轮各 4 帧 = 16 帧，第 16 帧被拒收 */
    Fixture();
    s_u16CanReadRet = HW_CAN_OK;
    s_u16CanMsgLst = 0U;
    s_u16CanReadOkLeft = 20U;
    CanPortServiceOne(HW_CAN_ID_A);
    CanPortServiceOne(HW_CAN_ID_A);
    CanPortServiceOne(HW_CAN_ID_A);

    s_u16CanMsgLst = 1U;
    CanPortServiceOne(HW_CAN_ID_A);   /* 15 帧入队后本帧被拒，但覆盖标志仍应锁存 */

    Check(CanPortSwDrop(HW_CAN_ID_A) > 0UL, "S-05A-07 sw drop counted");
    Check(CanPortHwOverwriteSeen(HW_CAN_ID_A) == 1U, "S-05A-07 latch kept when sw queue full");
}

/* S-05A-08 / S-05A-15：提交不确定不重发；RETRY 只解冻，下一次服务才提交 */
static void TestPendingHead(void)
{
    ST_CAN_FRAME stFrame;
    Uint32 u32CallsAfterUnknown;
    Uint32 u32CallsBeforeRetry;

    Fixture();
    stFrame.id = 0x111UL;
    stFrame.flags = 0U;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0x11U;
    (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);

    s_u16CanSendRet = HW_CAN_UNKNOWN;
    CanPortServiceOne(HW_CAN_ID_A);
    u32CallsAfterUnknown = s_u32CanSendCount;

    Check(u32CallsAfterUnknown == 1UL, "S-05A-08 one submit attempt");
    Check(CanPortIsTxFrozen(HW_CAN_ID_A) != 0U, "S-05A-08 tx frozen after UNKNOWN");

    CanPortServiceOne(HW_CAN_ID_A);
    CanPortServiceOne(HW_CAN_ID_A);
    Check(s_u32CanSendCount == u32CallsAfterUnknown, "S-05A-08 no resubmit while pending");

    Check(CanPortResolvePendingHead(HW_CAN_ID_A, 9U) == HW_CAN_ERR, "S-05A-15 illegal action rejected");
    Check(CanPortIsTxFrozen(HW_CAN_ID_A) != 0U, "S-05A-15 illegal action keeps frozen");

    s_u16CanPending = 1U;
    Check(CanPortResolvePendingHead(HW_CAN_ID_A, CAN_PORT_HEAD_RETRY) == HW_CAN_BUSY,
          "S-05A-15 busy while driver pending");
    s_u16CanPending = 0U;

    s_u16CanSendRet = HW_CAN_OK;
    u32CallsBeforeRetry = s_u32CanSendCount;
    Check(CanPortResolvePendingHead(HW_CAN_ID_A, CAN_PORT_HEAD_RETRY) == HW_CAN_OK,
          "S-05A-15 retry accepted");
    Check(s_u32CanSendCount == u32CallsBeforeRetry, "S-05A-15 retry does not submit immediately");

    CanPortServiceOne(HW_CAN_ID_A);
    Check(s_u32CanSendCount == (u32CallsBeforeRetry + 1UL), "S-05A-15 next service submits again");
    Check(s_u32CanSendLastId == 0x111UL, "S-05A-15 resubmitted head id");
    Check(s_u16CanSendLastData0 == 0x11U, "S-05A-15 resubmitted head payload");

    /* 重复裁定不得删除下一帧 */
    Fixture();
    stFrame.id = 0x222UL;
    stFrame.flags = 0U;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0x22U;
    (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);
    stFrame.id = 0x333UL;
    stFrame.data[0] = 0x33U;
    (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);

    s_u16CanSendRet = HW_CAN_UNKNOWN;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(CanPortResolvePendingHead(HW_CAN_ID_A, CAN_PORT_HEAD_DROP) == HW_CAN_OK, "S-05A-15 drop accepted");
    Check(CanPortResolvePendingHead(HW_CAN_ID_A, CAN_PORT_HEAD_DROP) == HW_CAN_ERR, "S-05A-15 repeat drop rejected");

    s_u16CanSendRet = HW_CAN_OK;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(s_u32CanSendLastId == 0x333UL, "S-05A-15 next frame survives repeat resolve");
}

/* S-05A-09：临界区嵌套与延迟投递（投递必须改变端口状态） */
static void TestCriticalSectionDeferredFault(void)
{
    ST_PORT_IRQ_STATE stOuter;
    ST_PORT_IRQ_STATE stInner;

    Fixture();

    /* (a) 进入前已屏蔽：退出后仍屏蔽，不投递 */
    s_u16Masked = 1U;
    s_u16PendingFault = 1U;
    stOuter = PortEnterCriticalHook();
    PortExitCriticalHook(stOuter);
    Check(s_u16Masked == 1U, "S-05A-09 stays masked when entered masked");
    Check(s_u16Delivered == 0U, "S-05A-09 no delivery while still masked");
    s_u16Masked = 0U;

    /* (b) 双层嵌套：内层退出只减深度 */
    IrqReset();
    stOuter = PortEnterCriticalHook();
    stInner = PortEnterCriticalHook();
    PortExitCriticalHook(stInner);
    Check(s_u16Depth == 1U, "S-05A-09 nested depth after inner exit");
    Check(s_u16Masked == 1U, "S-05A-09 still masked at depth 1");
    Check(s_u16Delivered == 0U, "S-05A-09 no delivery at depth 1");

    /* 屏蔽期间挂起故障，外层退出时投递并真正作用到端口状态 */
    s_u16PendingFault = 1U;
    PortExitCriticalHook(stOuter);
    Check(s_u16Depth == 0U, "S-05A-09 depth back to 0");
    Check(s_u16Masked == 0U, "S-05A-09 unmasked");
    Check(s_u16Delivered == 1U, "S-05A-09 fault delivered on unmask");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-09 delivered fault reaches port state");
}

/* S-05A-11：Bus-Off 恢复保留覆盖锁存，模块重建才清 */
static void TestFreezeSemantics(void)
{
    ST_CAN_FRAME stFrame;

    Fixture();
    stFrame.id = 0x10UL;
    stFrame.flags = 0U;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0U;
    (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);

    s_u16CanReadRet = HW_CAN_OK;
    s_u16CanMsgLst = 1U;
    s_u16CanReadOkLeft = 1U;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(CanPortHwOverwriteSeen(HW_CAN_ID_A) == 1U, "S-05A-11 latch before fault");

    s_u16CanReadRet = HW_CAN_UNKNOWN;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(CanPortIsRxFrozen(HW_CAN_ID_A) != 0U, "S-05A-11 rx frozen after unknown");

    CanPortOnBusOffCleared(HW_CAN_ID_A);
    Check(CanPortIsRxFrozen(HW_CAN_ID_A) == 0U, "S-05A-11 bus-off cleared unfreezes rx");
    Check(CanPortHwOverwriteSeen(HW_CAN_ID_A) == 1U, "S-05A-11 bus-off keeps overwrite latch");
    Check(CanPortIsTxFrozen(HW_CAN_ID_A) == 0U, "S-05A-11 bus-off does not unfreeze tx decisions");

    CanPortOnModuleRecovered(HW_CAN_ID_A);
    Check(CanPortHwOverwriteSeen(HW_CAN_ID_A) == 0U, "S-05A-11 module rebuild clears latch");
}

/* S-05A-12：时间源失败不搬运，恢复后重建基准 */
static void TestTimeSourceFailure(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;

    Fixture();
    RxRound();
    SciFeed(HW_SCI_ID_A, 0x11U);

    s_bTickOk = 0U;
    RxRound();
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 0U, "S-05A-12 no publish on tick failure");
    Check(SciPortIsTimingBad(HW_SCI_ID_A) != 0U, "S-05A-12 timing bad set");

    s_bTickOk = 1U;
    RxRound();
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 0U, "S-05A-12 first round after recovery rebaselines");

    RxRound();
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 1U, "S-05A-12 second round publishes");
}

/* S-05A-13：字节环整批拒收 */
static void TestByteRingReject(void)
{
    Uint16 i;

    Fixture();
    RxRound();

    SciFeedRun(HW_SCI_ID_A, 1U, 56U);   /* 7 批 x 8 */
    for (i = 0U; i < 7U; i++)
    {
        RxRound();
    }

    SciFeedRun(HW_SCI_ID_A, 100U, 7U);  /* 补到 63 字节整 */
    RxRound();

    SciFeed(HW_SCI_ID_A, 200U);         /* 第 64 字节无处可放 */
    RxRound();

    Check(SciPortRxRejected(HW_SCI_ID_A) == 1U, "S-05A-13 rejected byte counted");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-13 reject raises stream break");
}

/* S-05A-14：段环先满导致整批拒收 */
static void TestSegRingFull(void)
{
    Uint16 i;

    Fixture();
    RxRound();

    for (i = 0U; i < 32U; i++)
    {
        SciFeedRun(HW_SCI_ID_C, (Uint16)(i * 8U), 8U);
        RxRound();
    }

    Check(SciPortRxRejected(HW_SCI_ID_C) == 8U, "S-05A-14 seg ring full rejects whole batch");
    Check(SciPortIsStreamBreak(HW_SCI_ID_C) != 0U, "S-05A-14 seg ring full raises stream break");
}

/* S-05A-16：空读观察 tick = 0 是合法值 */
static void TestEmptyTickZero(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;

    Fixture();
    RxRound();
    s_u32Tick = 0UL;
    RxRound();

    SciFeed(HW_SCI_ID_A, 0x77U);
    RxRound();

    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 1U, "S-05A-16 batch published");
    Check(stSeg.u32TickEmptyLast == 0UL, "S-05A-16 empty tick 0 kept as value");
    Check(stSeg.u16EmptyRounds >= 1U, "S-05A-16 existence flagged by round count");
}

/* S-05A-21：接收错误合并成流中断，且不因 FIFO 空而跳过 */
static void TestSciRxError(void)
{
    Fixture();
    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    RxRound();

    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-21 error with empty fifo raises stream break");
    Check(SciPortRxErrorCount(HW_SCI_ID_A) == 1U, "S-05A-21 error counted");

    s_au16ErrFlag[HW_SCI_ID_A] = 0U;
    RxRound();
    Check(SciPortRxErrorCount(HW_SCI_ID_A) == 1U, "S-05A-21 no false count after clearing");
}

/* S-05A-24：故障代次确认（清除不能抹掉新故障） */
static void TestFaultGenerationRace(void)
{
    Uint16 u16Gen1;
    Uint16 u16Gen2;

    Fixture();

    /* 05B 起硬件错误会进入恢复态，确认只在 IDLE 被接受；
     * 这里用纯软件重同步制造两次故障，保持用例本意（代次竞态）。 */
    SciPortResync(HW_SCI_ID_A);
    u16Gen1 = SciPortFaultGeneration(HW_SCI_ID_A);
    Check(u16Gen1 != 0U, "S-05A-24 fault generation advanced");
    RxRound();
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05A-24 resync drain finished");

    SciPortResync(HW_SCI_ID_A);
    u16Gen2 = SciPortFaultGeneration(HW_SCI_ID_A);
    Check(u16Gen2 == (Uint16)(u16Gen1 + 1U), "S-05A-24 second fault bumps generation again");

    Check(SciPortClearStreamBreak(HW_SCI_ID_A, u16Gen1) == 0U, "S-05A-24 stale generation rejected");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-24 new fault survives stale clear");

    RxRound();
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05A-24 second resync drain finished");

    Check(SciPortClearStreamBreak(HW_SCI_ID_A, u16Gen2) == 1U, "S-05A-24 current generation clears");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) == 0U, "S-05A-24 flag cleared");

    SciPortResync(HW_SCI_ID_A);
    RxRound();
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-17 relatch after clear");
}

/* S-05A-23：重同步跨轮持续丢弃，直到真正读到空 */
static void TestResyncCrossRoundDrain(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;

    Fixture();
    SciFeedRun(HW_SCI_ID_A, 1U, 80U);
    RxRound();      /* 建基准 */
    RxRound();      /* 搬 8 */
    RxRound();      /* 搬 8 */

    Check(SciPortRxUsed(HW_SCI_ID_A) == 16U, "S-05A-23 batches published before resync");

    SciPortResync(HW_SCI_ID_A);

    Check(SciPortRxUsed(HW_SCI_ID_A) == 0U, "S-05A-23 software rings cleared");
    Check(SciPortIsResyncDraining(HW_SCI_ID_A) != 0U, "S-05A-23 still draining after one quota");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-23 stream break held during drain");
    Check(s_au16FeedPos[HW_SCI_ID_A] == 48U, "S-05A-23 first quota discarded");

    RxRound();      /* 再丢一轮配额，仍打满 -> 继续丢弃 */
    Check(SciPortIsResyncDraining(HW_SCI_ID_A) != 0U, "S-05A-23 keeps draining across rounds");
    Check(s_au16FeedPos[HW_SCI_ID_A] == 80U, "S-05A-23 second quota discarded");

    RxRound();      /* 读到空 -> 排空完成 */
    Check(SciPortIsResyncDraining(HW_SCI_ID_A) == 0U, "S-05A-23 drain completes on empty");
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 0U, "S-05A-23 no batch while draining");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-23 stream break still held for confirmation");
}

/* S-05A-27：丢弃未完成时不得确认流中断；旧代次永远不能清除新故障 */
static void TestClearDuringDrain(void)
{
    Uint16 u16Gen;

    Fixture();
    SciFeedRun(HW_SCI_ID_A, 1U, 80U);
    RxRound();      /* 建基准 */
    RxRound();
    RxRound();

    SciPortResync(HW_SCI_ID_A);
    u16Gen = SciPortFaultGeneration(HW_SCI_ID_A);

    Check(SciPortIsResyncDraining(HW_SCI_ID_A) != 0U, "S-05A-27 draining active");
    Check(SciPortClearStreamBreak(HW_SCI_ID_A, u16Gen) == 0U, "S-05A-27 confirm rejected while draining");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-27 stream break kept while draining");

    RxRound();      /* 再丢一轮配额，仍打满 */
    Check(SciPortClearStreamBreak(HW_SCI_ID_A, u16Gen) == 0U, "S-05A-27 confirm still rejected");

    RxRound();      /* 读到空，排空完成 */
    Check(SciPortIsResyncDraining(HW_SCI_ID_A) == 0U, "S-05A-27 drain finished");
    Check(SciPortClearStreamBreak(HW_SCI_ID_A, (Uint16)(u16Gen - 1U)) == 0U,
          "S-05A-27 stale generation cannot clear");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-27 break kept after stale confirm");
    Check(SciPortClearStreamBreak(HW_SCI_ID_A, u16Gen) == 1U, "S-05A-27 confirm accepted when idle");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) == 0U, "S-05A-27 break cleared");

    /* 清除后新故障仍能重新置位，并带来新的代次 */
    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    RxRound();
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05A-27 new fault re-arms break");
    Check(SciPortFaultGeneration(HW_SCI_ID_A) != u16Gen, "S-05A-27 generation advanced");
}

/* S-05A-28：帧类型设置只允许在初始化态调用 */
static void TestSetFrameTypeGuard(void)
{
    ST_CAN_FRAME stFrame;

    Fixture();
    stFrame.id = 0x21UL;
    stFrame.flags = 0U;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0U;

    /* 空队列、无未决：允许设置 */
    Check(CanPortSetTxFrameType(HW_CAN_ID_B, 1U) == 1U, "S-05A-28 set allowed when idle");

    /* 规则已改为扩展帧：标准帧被拒、扩展帧可入队 */
    Check(CanPortPushTx(HW_CAN_ID_B, &stFrame) == 0U, "S-05A-28 std frame blocked under ext rule");
    stFrame.flags = CAN_FRAME_EXT;
    Check(CanPortPushTx(HW_CAN_ID_B, &stFrame) == 1U, "S-05A-28 ext frame accepted");

    /* 队列非空：拒绝修改，且已有规则保持不变 */
    Check(CanPortSetTxFrameType(HW_CAN_ID_B, 0U) == 0U, "S-05A-28 reject when tx queue non-empty");
    Check(CanPortPushTx(HW_CAN_ID_B, &stFrame) == 1U, "S-05A-28 rule unchanged (ext still accepted)");

    /* 驱动有未决事务：拒绝 */
    Fixture();
    s_u16CanPending = 1U;
    Check(CanPortSetTxFrameType(HW_CAN_ID_A, 1U) == 0U, "S-05A-28 reject when driver pending");

    /* 未初始化 + 未决：同样拒绝（未决检查不以"已初始化"为前提） */
    Fixture();
    s_u16CanInited = 0U;
    s_u16CanPending = 1U;
    Check(CanPortSetTxFrameType(HW_CAN_ID_A, 1U) == 0U,
          "S-05A-28 reject pending even when uninitialized");

    /* 未初始化 + 无未决：允许初始化配置 */
    Fixture();
    s_u16CanInited = 0U;
    s_u16CanPending = 0U;
    Check(CanPortSetTxFrameType(HW_CAN_ID_A, 1U) == 1U,
          "S-05A-28 allow when uninitialized and idle");
}

/* S-05A-18：PortTask 预算截断与游标推进 */
static void TestPortTaskBudget(void)
{
    Uint16 u16First;
    Uint16 i;

    Fixture();
    PortTask();
    Check(PortTaskLastUnits() == 5U, "S-05A-18 all 5 units served");
    u16First = PortTaskLastFirstUnit();
    PortTask();
    Check(PortTaskLastFirstUnit() != u16First, "S-05A-18 cursor rotates");

    /* 每次读时标消耗 15 µs，20 µs 预算只能服务少量单元 */
    Fixture();
    s_u32TickStepTicks = 15UL * TICKS_PER_US;
    for (i = 0U; i < 3U; i++)
    {
        PortTask();
    }

    Check(PortTaskBudgetTruncations() > 0UL, "S-05A-18 budget truncation counted");
    Check(PortTaskLastUnits() < 5U, "S-05A-18 stops before all units");

    /* 时间源失败：整轮不服务 */
    Fixture();
    s_bTickOk = 0U;
    PortTask();
    Check(PortTaskLastUnits() == 0U, "S-05A-18 no unit on tick failure");
    Check(PortTaskTimeSourceFailures() > 0UL, "S-05A-18 time source failure counted");
}

/* S-05A-26：时间源中途失败后，本轮后续端口不得继续发起驱动调用 */
static void TestBudgetFaultLatch(void)
{
    Uint16 au16Data[3];

    Fixture();
    au16Data[0] = 0xB1U;
    au16Data[1] = 0xB2U;
    au16Data[2] = 0xB3U;

    (void)SciPortWriteTx(HW_SCI_ID_A, au16Data, 3U);
    (void)SciPortWriteTx(HW_SCI_ID_C, au16Data, 3U);
    (void)SciPortWriteTx(HW_SCI_ID_D, au16Data, 3U);

    s_au16TxAccept[HW_SCI_ID_A] = 8U;
    s_au16TxAccept[HW_SCI_ID_C] = 8U;
    s_au16TxAccept[HW_SCI_ID_D] = 8U;

    /* 第 1 次 = PortTask 取起点；第 2 次 = 单元 A 前；第 3 次 = A 的服务内部。
       失败后时间源自行恢复，但本轮必须锁存停止。 */
    s_u32TickFailAtCall = 3UL;
    PortTask();

    Check(PortTaskLastUnits() == 1U, "S-05A-26 only one unit before latch");
    Check(s_au16TxLogLen[HW_SCI_ID_C] == 0U, "S-05A-26 SCI-C not served after transient failure");
    Check(s_au16TxLogLen[HW_SCI_ID_D] == 0U, "S-05A-26 SCI-D not served after transient failure");
    Check(PortTaskTimeSourceFailures() > 0UL, "S-05A-26 failure counted");

    /* 下一轮 PortBudgetBegin() 复位锁存：时间源正常则服务全部 5 个单元 */
    PortTask();
    Check(PortTaskLastUnits() == 5U, "S-05A-26 latch cleared on next window");
    Check(s_au16TxLogLen[HW_SCI_ID_C] == 3U, "S-05A-26 SCI-C served in next round");
}

/* S-05A-19：PortTask 必须按硬件 ID 服务 A/C/D（不是槽位 0/1/2） */
static void TestPortTaskSciMapping(void)
{
    Uint16 au16Data[3];

    Fixture();
    au16Data[0] = 0xA1U;
    au16Data[1] = 0xA2U;
    au16Data[2] = 0xA3U;

    (void)SciPortWriteTx(HW_SCI_ID_A, au16Data, 3U);
    (void)SciPortWriteTx(HW_SCI_ID_C, au16Data, 3U);
    (void)SciPortWriteTx(HW_SCI_ID_D, au16Data, 3U);

    s_au16TxAccept[HW_SCI_ID_A] = 8U;
    s_au16TxAccept[HW_SCI_ID_C] = 8U;
    s_au16TxAccept[HW_SCI_ID_D] = 8U;

    PortTask();

    Check(s_au16TxLogLen[HW_SCI_ID_A] == 3U, "S-05A-19 SCI-A served");
    Check(s_au16TxLogLen[HW_SCI_ID_C] == 3U, "S-05A-19 SCI-C served");
    Check(s_au16TxLogLen[HW_SCI_ID_D] == 3U, "S-05A-19 SCI-D served");
    Check(s_au16TxLogLen[1] == 0U, "S-05A-19 no writes to unused SCI-B id");
    Check(SciPortTxFree(HW_SCI_ID_D) == 127U, "S-05A-19 SCI-D ring drained");
}

/* S-05A-20：模块仍有未结束事务时不发起发送 */
static void TestCanTxPendingBlock(void)
{
    ST_CAN_FRAME stFrame;

    Fixture();
    stFrame.id = 0x10UL;
    stFrame.flags = 0U;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0U;
    (void)CanPortPushTx(HW_CAN_ID_A, &stFrame);

    s_u16CanPending = 1U;
    CanPortServiceOne(HW_CAN_ID_A);

    Check(s_u32CanSendCount == 0UL, "S-05A-20 no submit while module pending");
    Check(CanPortTxPendingBlock(HW_CAN_ID_A) == 1UL, "S-05A-20 pending block counted");
    Check(CanPortTxFree(HW_CAN_ID_A) == 14U, "S-05A-20 frame kept in queue");
}

/* S-05A-22：单次驱动调用耗 30 µs 时，一轮内只发起一次接收且不再发送 */
static void TestBudgetMidService(void)
{
    Fixture();
    s_u16CanReadRet = HW_CAN_OK;
    s_u16CanReadOkLeft = 8U;
    s_u32CanReadCostTicks = 30UL * TICKS_PER_US;

    PortTask();

    Check(s_u32CanReadCount == 1UL, "S-05A-22 single read before budget stop");
    Check(s_u32CanSendCount == 0UL, "S-05A-22 no send after budget overrun");
    Check(PortTaskBudgetTruncations() > 0UL, "S-05A-22 truncation counted");
}

/* S-05A-25：CAN 入队校验帧类型与 RTR */
static void TestCanFrameTypeReject(void)
{
    ST_CAN_FRAME stFrame;

    Fixture();
    (void)CanPortSetTxFrameType(HW_CAN_ID_A, 0U);

    stFrame.id = 0x01UL;
    stFrame.flags = CAN_FRAME_EXT;
    stFrame.dlc = 1U;
    stFrame.data[0] = 0U;
    Check(CanPortPushTx(HW_CAN_ID_A, &stFrame) == 0U, "S-05A-25 ext frame rejected on std port");
    Check(CanPortTxTypeReject(HW_CAN_ID_A) == 1U, "S-05A-25 type reject counted");

    stFrame.flags = CAN_FRAME_RTR;
    Check(CanPortPushTx(HW_CAN_ID_A, &stFrame) == 0U, "S-05A-25 rtr frame rejected");
    Check(CanPortTxRtrReject(HW_CAN_ID_A) == 1U, "S-05A-25 rtr reject counted");

    stFrame.flags = 0U;
    Check(CanPortPushTx(HW_CAN_ID_A, &stFrame) == 1U, "S-05A-25 std frame accepted");

    (void)CanPortSetTxFrameType(HW_CAN_ID_B, 1U);
    stFrame.flags = CAN_FRAME_EXT;
    Check(CanPortPushTx(HW_CAN_ID_B, &stFrame) == 1U, "S-05A-25 ext frame accepted on ext port");
    stFrame.flags = 0U;
    Check(CanPortPushTx(HW_CAN_ID_B, &stFrame) == 0U, "S-05A-25 std frame rejected on ext port");
}

/* S-05A-10：CAN 连续接收、冻结前置与恢复 */
static void TestCanRxAndFreeze(void)
{
    ST_CAN_FRAME stFrame;

    Fixture();
    s_u16CanReadRet = HW_CAN_OK;
    s_u16CanReadOkLeft = 6U;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(CanPortPopRx(HW_CAN_ID_A, &stFrame) == 1U, "S-05A-10 rx frame available");
    Check(stFrame.id == 0x123UL, "S-05A-10 rx id");

    s_u16CanReadRet = HW_CAN_UNKNOWN;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(CanPortIsRxFrozen(HW_CAN_ID_A) != 0U, "S-05A-10 frozen after unknown");

    s_u32CanReadCount = 0UL;
    s_u16CanReadRet = HW_CAN_OK;
    CanPortServiceOne(HW_CAN_ID_A);
    Check(s_u32CanReadCount == 0UL, "S-05A-10 frozen port does not read");

    CanPortOnModuleRecovered(HW_CAN_ID_A);
    CanPortServiceOne(HW_CAN_ID_A);
    Check(s_u32CanReadCount > 0UL, "S-05A-10 recovers after module rebuild notice");
}


/* ================= 05B：接收恢复状态机 ================= */

/* 把端口从恢复中推进到 IDLE（默认清除路径）。 */
static void RecoverToIdle(Uint16 u16PortId)
{
    Uint16 u16Remain = 0U;
    Uint16 i;

    for (i = 0U; i < 12U; i++)
    {
        (void)SciPortRecover(u16PortId, &u16Remain);
        RxRound();

        if (SciPortRxState(u16PortId) == SCI_RX_IDLE)
        {
            return;
        }
    }
}

/* S-05B-01：错误 -> 分类 -> 等发送 -> 硬件复位 -> 排空 -> IDLE */
static void TestSciRecoverFullFlow(void)
{
    Uint16 u16Remain = 0U;
    Uint16 u16Attempts;

    Fixture();

    SciFeed(HW_SCI_ID_A, 0x51U);
    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    RxRound();

    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_RECOVER_REQ, "S-05B-01 error enters recover_req");
    Check(SciPortRxErrEvents(HW_SCI_ID_A) == 1UL, "S-05B-01 hardware event counted once");
    Check(SciPortIsStreamBreak(HW_SCI_ID_A) != 0U, "S-05B-01 stream break raised");
    Check(SciPortRxUsed(HW_SCI_ID_A) == 0U, "S-05B-01 rings invalidated on entry");

    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-01 classify busy");
    Check(SciPortRecPhase(HW_SCI_ID_A) == SCI_REC_PHASE_WAIT_TX, "S-05B-01 phase wait_tx");
    Check(SciPortIsTxSuspended(HW_SCI_ID_A) != 0U, "S-05B-01 tx suspended while waiting");

    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-01 wait base busy");

    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-01 commit reset busy");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_HW_RESET, "S-05B-01 state hw_reset");

    u16Attempts = s_au16Attempts[HW_SCI_ID_A];
    RxRound();
    Check(s_au16Attempts[HW_SCI_ID_A] == u16Attempts, "S-05B-01 heartbeat skips fifo in hw_reset");
    Check(s_au16FeedPos[HW_SCI_ID_A] == 0U, "S-05B-01 fifo untouched in hw_reset");

    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_OK, "S-05B-01 clear ok");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_DRAIN_ISR, "S-05B-01 draining after reset");

    RxRound();
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05B-01 back to idle after drain");
    Check(SciPortIsTxSuspended(HW_SCI_ID_A) == 0U, "S-05B-01 tx auto-resumes without loss");
}

/* S-05B-02：清除后仍报错 -> PAUSED；显式重试不计硬件错误事件 */
static void TestSciRecoverPausedRetry(void)
{
    Fixture();

    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    s_u16ClearKind[HW_SCI_ID_A] = 2U;   /* 软复位后仍报错 */

    RxRound();
    Check(SciPortRxErrEvents(HW_SCI_ID_A) == 1UL, "S-05B-02 event counted");

    RecoverToIdle(HW_SCI_ID_A);
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_PAUSED, "S-05B-02 paused on still error");

    Check(SciPortClearStreamBreak(HW_SCI_ID_A, SciPortFaultGeneration(HW_SCI_ID_A)) == 0U,
          "S-05B-02 confirm rejected while paused");

    Check(SciPortRecoverRetry(HW_SCI_ID_A) == SCI_RECOVER_OK, "S-05B-02 retry accepted");
    Check(SciPortRxErrEvents(HW_SCI_ID_A) == 1UL, "S-05B-02 retry does not count hardware event");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_RECOVER_REQ, "S-05B-02 retry restarts recovery");

    s_au16ErrFlag[HW_SCI_ID_A] = 0U;
    s_u16ClearKind[HW_SCI_ID_A] = 0U;
    RecoverToIdle(HW_SCI_ID_A);
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05B-02 retry reaches idle");
}

/* S-05B-03：发送持续忙 -> 8 ms 判定阈值后强制复位、清 TX 环并记损失 */
static void TestSciTxBusyForcedReset(void)
{
    Uint16 u16Remain = 0U;
    Uint16 au16Buf[8];
    Uint16 i;

    Fixture();

    for (i = 0U; i < 4U; i++)
    {
        au16Buf[i] = (Uint16)(0x60U + i);
    }

    Check(SciPortWriteTx(HW_SCI_ID_A, au16Buf, 4U) == 4U, "S-05B-03 queue four bytes");
    Check(SciPortTxFree(HW_SCI_ID_A) == 123U, "S-05B-03 four bytes queued");

    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    s_u16TxIdle[HW_SCI_ID_A] = 0U;
    RxRound();

    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* CLASSIFY -> WAIT_TX */
    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* 建立等待基准 */

    TicksAdvanceUs(2000UL);
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-03 still waiting");
    Check(SciPortTxFree(HW_SCI_ID_A) == 123U, "S-05B-03 tx ring kept before threshold");
    Check(SciPortTxResetDrop(HW_SCI_ID_A) == 0UL, "S-05B-03 no loss before threshold");

    TicksAdvanceUs(7000UL);
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-03 forced reset busy");
    Check(SciPortTxFree(HW_SCI_ID_A) == 127U, "S-05B-03 tx ring cleared");
    Check(SciPortTxResetDrop(HW_SCI_ID_A) == 4UL, "S-05B-03 loss counted once");
    Check(SciPortIsTxSuspended(HW_SCI_ID_A) != 0U, "S-05B-03 tx suspended after forced reset");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_HW_RESET, "S-05B-03 forced path hw_reset");

    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_OK, "S-05B-03 clear ok");
    Check(SciPortTxResume(HW_SCI_ID_A) == 0U, "S-05B-03 resume rejected while draining");
    RxRound();
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05B-03 idle after drain");
    Check(SciPortTxResume(HW_SCI_ID_A) == 1U, "S-05B-03 resume accepted when idle");
    Check(SciPortIsTxSuspended(HW_SCI_ID_A) == 0U, "S-05B-03 tx resumed");
}

/* S-05B-04：状态不可读 -> PAUSED（不能用"没读到"当"没错误"） */
static void TestSciStatusUnreadablePauses(void)
{
    Uint16 u16Remain = 0U;

    Fixture();

    s_u16StatusOk[HW_SCI_ID_A] = 0U;
    RxRound();

    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_RECOVER_REQ, "S-05B-04 unreadable status requests recovery");
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_UNKNOWN, "S-05B-04 classify unknown");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_PAUSED, "S-05B-04 paused on unreadable");
}

/* S-05B-05：发送空闲查询失败 -> PAUSED，不能最终变成忙超时 */
static void TestSciTxQueryFailurePauses(void)
{
    Uint16 u16Remain = 0U;

    Fixture();

    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    s_u16TxIdleQueryOk[HW_SCI_ID_A] = 0U;
    RxRound();

    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* CLASSIFY -> WAIT_TX */
    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* 建立等待基准 */
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_UNKNOWN,
          "S-05B-05 query failure unknown");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_PAUSED, "S-05B-05 paused on query failure");
}

/* S-05B-06：纯软件重同步不计硬件错误事件 */
static void TestSciSoftwareResyncNoEvent(void)
{
    Fixture();

    SciPortResync(HW_SCI_ID_A);

    Check(SciPortRxErrEvents(HW_SCI_ID_A) == 0UL, "S-05B-06 software resync is not hw event");
    Check(SciPortFaultGeneration(HW_SCI_ID_A) != 0U, "S-05B-06 generation still advances");
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_SW_RESYNC, "S-05B-06 state sw_resync");
}

/* S-05B-07：故障前的未消费批次在自动恢复后不与新数据混合 */
static void TestSciRecoverDropsOldBatch(void)
{
    Uint16 au16Buf[16];
    ST_SCI_RX_SEG stSeg;

    Fixture();

    SciFeedRun(HW_SCI_ID_A, 1U, 3U);
    RxRound();
    RxRound();
    Check(SciPortRxUsed(HW_SCI_ID_A) == 3U, "S-05B-07 old batch present");

    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    RxRound();
    Check(SciPortRxUsed(HW_SCI_ID_A) == 0U, "S-05B-07 old batch invalidated");
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 0U, "S-05B-07 nothing to pop");

    s_au16ErrFlag[HW_SCI_ID_A] = 0U;
    RecoverToIdle(HW_SCI_ID_A);
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_IDLE, "S-05B-07 idle before new data");

    SciFeedRun(HW_SCI_ID_A, 0x71U, 2U);
    RxRound();
    RxRound();
    Check(SciPortRxUsed(HW_SCI_ID_A) == 2U, "S-05B-07 only new batch published");
    Check(SciPortPopBatch(HW_SCI_ID_A, au16Buf, 16U, &stSeg) == 1U, "S-05B-07 new batch pops");
    Check((au16Buf[0] == 0x71U) && (stSeg.u16Count == 2U), "S-05B-07 new batch content");
}

/* S-05B-08：参数校验与空闲返回码 */
static void TestSciRecoverParam(void)
{
    Uint16 u16Remain = 0U;

    Fixture();

    Check(SciPortRecover(1U, &u16Remain) == SCI_RECOVER_PARAM, "S-05B-08 unknown port is param");
    Check(SciPortRecover(HW_SCI_ID_A, (Uint16 *)0) == SCI_RECOVER_PARAM, "S-05B-08 null remain is param");
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_IDLE_OK, "S-05B-08 idle returns idle_ok");
}

/* S-05B-09：预算不足时 RESET 阶段保持原状、下一轮继续且不重复记损失 */
static void TestSciResetBudgetCarryOver(void)
{
    Uint16 u16Remain = 0U;
    Uint16 u16Dropped;

    Fixture();

    s_au16ErrFlag[HW_SCI_ID_A] = 1U;
    s_u16TxIdle[HW_SCI_ID_A] = 0U;
    RxRound();

    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* CLASSIFY -> WAIT_TX */
    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* 建立等待基准 */

    TicksAdvanceUs(9000UL);
    (void)SciPortRecover(HW_SCI_ID_A, &u16Remain);   /* 强制复位 -> HW_RESET/RESET */
    Check(SciPortRxState(HW_SCI_ID_A) == SCI_RX_HW_RESET, "S-05B-09 forced path committed");

    /* 人为制造预算不足：RESET 阶段本轮不得执行清除 */
    PortBudgetBegin(s_u32Tick + 5000UL);
    Check(PortBudgetHasLeft() == 0U, "S-05B-09 budget exhausted");
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_BUSY, "S-05B-09 reset deferred");
    Check(SciPortRecPhase(HW_SCI_ID_A) == SCI_REC_PHASE_RESET, "S-05B-09 phase kept");
    Check(s_u32ClearCalls == 0UL, "S-05B-09 clear not executed");

    u16Dropped = (Uint16)SciPortTxResetDrop(HW_SCI_ID_A);
    PortBudgetEnd();

    /* 下一轮预算恢复后继续，不清空、不重复记损失 */
    Check(SciPortRecover(HW_SCI_ID_A, &u16Remain) == SCI_RECOVER_OK, "S-05B-09 reset resumes");
    Check((Uint16)SciPortTxResetDrop(HW_SCI_ID_A) == u16Dropped, "S-05B-09 loss not counted twice");
    Check(s_u32ClearCalls == 1UL, "S-05B-09 clear executed once");
}

int main(void)
{
    TestInitAndMapping();
    TestQuotaAndAttempts();
    TestTwoBatchesWithEmpty();
    TestGapBad();
    TestTxPartial();
    TestHwOverwriteLatch();
    TestPendingHead();
    TestCriticalSectionDeferredFault();
    TestFreezeSemantics();
    TestTimeSourceFailure();
    TestByteRingReject();
    TestSegRingFull();
    TestEmptyTickZero();
    TestSciRxError();
    TestFaultGenerationRace();
    TestResyncCrossRoundDrain();
    TestPortTaskBudget();
    TestBudgetFaultLatch();
    TestClearDuringDrain();
    TestSetFrameTypeGuard();
    TestPortTaskSciMapping();
    TestCanTxPendingBlock();
    TestBudgetMidService();
    TestCanFrameTypeReject();
    TestCanRxAndFreeze();
    TestSciRecoverFullFlow();
    TestSciRecoverPausedRetry();
    TestSciTxBusyForcedReset();
    TestSciStatusUnreadablePauses();
    TestSciTxQueryFailurePauses();
    TestSciSoftwareResyncNoEvent();
    TestSciRecoverDropsOldBatch();
    TestSciRecoverParam();
    TestSciResetBudgetCarryOver();

    printf("checks=%d fails=%d\n", s_iChecks, s_iFails);
    return (s_iFails == 0) ? 0 : 1;
}

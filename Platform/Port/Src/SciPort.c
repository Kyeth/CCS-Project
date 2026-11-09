/**
 * @file SciPort.c
 * @author yetingheng (004182)
 * @brief SCI 端口层实现：RX 批次搬运、接收恢复状态机、TX 前缀推进
 * @version 0.2
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 执行上下文（05A 定稿，05B 沿用）：
 *   - 生产者 SciPortService100us()：100 us 心跳，写两环与观察记录，也是
 *     唯一允许从 FIFO 取字节的上下文；
 *   - 消费者 SciPortPopBatch()：协议层，读两环；
 *   - SciPortTask()/SciPortTaskOne()：1 ms 前台，推进 TX；
 *   - SciPortRecover()：1 ms 前台，推进接收恢复状态机。
 * 生产者只写 head，消费者只写 tail；临界区保存并恢复整字 ST1。
 */

#include "SciPort.h"
#include "Hw_TimeBase.h"
#include "PortBudget.h"

#define SCI_PORT_TICKS_PER_US_DEF (200UL) /* 200 MHz，SetSysclkHz 可覆盖 */
#define SCI_PORT_RESYNC_DRAIN_MAX (32U)   /* 每轮丢弃上限（跨轮继续） */

/* 存储：容量为 05A 定稿值（A/C/D = 64/256/128 字节，段环 16/32/16 条）。
 * TX 三环由链接脚本放进 .portbuf，与 .bss 的可执行/初始化段分开核算。 */
static volatile Uint16 s_au16RxBufA[64];
static volatile Uint16 s_au16RxBufC[256];
static volatile Uint16 s_au16RxBufD[128];
static volatile Uint16 s_au16SegBufA[16U * SCI_SEG_ITEM_WORDS];
static volatile Uint16 s_au16SegBufC[32U * SCI_SEG_ITEM_WORDS];
static volatile Uint16 s_au16SegBufD[16U * SCI_SEG_ITEM_WORDS];

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_au16TxBufA, ".portbuf")
#pragma DATA_SECTION(s_au16TxBufC, ".portbuf")
#pragma DATA_SECTION(s_au16TxBufD, ".portbuf")


#endif
static volatile Uint16 s_au16TxBufA[128];
static volatile Uint16 s_au16TxBufC[128];
static volatile Uint16 s_au16TxBufD[128];

static ST_SCI_PORT s_astSciPort[SCI_PORT_NUM];
static Uint32 s_u32TicksPerUs = SCI_PORT_TICKS_PER_US_DEF;

/* O2：服务间隔阈值预置成 tick 数，热路径只做一次无符号比较。
   与原判据等价：floor(gapTicks / ticksPerUs) > SCI_PORT_SERVICE_GAP_LIMIT_US
   等价于 gapTicks >= (SCI_PORT_SERVICE_GAP_LIMIT_US + 1) * ticksPerUs。
   200 tick/µs 下阈值为 100200（即 501 µs 起算超限，500 µs 整仍通过）。 */
static Uint32 s_u32GapLimitTicks =
    ((SCI_PORT_SERVICE_GAP_LIMIT_US + 1UL) * SCI_PORT_TICKS_PER_US_DEF);
static Uint32 s_u32ServiceCalls = 0UL;   /* 心跳服务调用次数（观测/桩测试） */

/* ------------------------------------------------------------------ */
/* 驱动接口：默认绑真实驱动，桩测试可覆盖                              */
/* ------------------------------------------------------------------ */

static Uint16 SciPortTryGetTickDefault(Uint32 *pu32Tick)
{
    Uint32 u32Tick;

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    u32Tick = Hw_TimeBaseTicks();

    /* 读取期间校验失败会锁存故障，读后必须复查。 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    *pu32Tick = u32Tick;
    return 1U;
}

static const ST_SCI_PORT_DRV s_stSciPortDrvDefault =
{
    Hw_SciTryReadByte,
    Hw_SciTryWriteByte,
    SciPortTryGetTickDefault,
    Hw_SciGetRxStatus,
    Hw_SciClearRxError,
    Hw_SciGetTxIdle
};

/* ------------------------------------------------------------------ */
/* 内部工具                                                            */
/* ------------------------------------------------------------------ */

static ST_SCI_PORT *SciPortFind(Uint16 u16PortId)
{
    Uint16 i;

    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        if (s_astSciPort[i].u16PortId == u16PortId)
        {
            return &s_astSciPort[i];
        }
    }

    return (ST_SCI_PORT *)0;
}

static Uint16 SciPortGapToUs(Uint32 u32Ticks)
{
    if (s_u32TicksPerUs == 0U)
    {
        return 0xFFFFU;
    }

    u32Ticks = u32Ticks / s_u32TicksPerUs;

    if (u32Ticks > 0xFFFFUL)
    {
        return 0xFFFFU;
    }

    return (Uint16)u32Ticks;
}

/* 按当前 ticksPerUs 重算间隔阈值；初始化与更新时基时都必须调用。 */
static void SciPortUpdateGapLimit(void)
{
    Uint32 u32Limit = (SCI_PORT_SERVICE_GAP_LIMIT_US + 1UL);

    if (s_u32TicksPerUs > (0xFFFFFFFFUL / u32Limit))
    {
        s_u32GapLimitTicks = 0xFFFFFFFFUL;
        return;
    }

    s_u32GapLimitTicks = u32Limit * s_u32TicksPerUs;
}

/* 新的故障事件：代次递增并置流中断。所有会破坏字节流连续性的路径都走这里。 */
static void SciPortNoteFault(ST_SCI_PORT *p)
{
    p->u16FaultGen++;
    p->bStreamBreak = 1U;
}

/* 软件侧失效处理：两环、序号、观察基准一次清空。调用者必须已在临界区内。
 * 每次恢复只执行一次——恢复期间不再重复清空，避免把“恢复后新到达的数据”
 * 也删掉，也避免每轮清空导致旧批次与新批次被拼在一起。 */
static void SciPortSoftInvalidateInCritical(ST_SCI_PORT *p)
{
    (void)RingBuffClear(&p->stRxRing);
    (void)RingBuffClear(&p->stSegRing);
    p->u16RxSeq = 0U;
    p->u16RxSeqConsumed = 0U;
    p->u16EmptyRounds = 0U;
    p->u32TickEmptyLast = 0U;
    p->bTimeBaseValid = 0U;
    p->bTimingBad = 0U;
    p->bSwInvalidated = 1U;
}

/* 状态、阶段成组提交：心跳要么看到旧组合，要么看到新组合，不会看到半套。 */
static void SciPortCommit(ST_SCI_PORT *p, Uint16 u16State, Uint16 u16Phase)
{
    ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

    p->u16RxState = u16State;
    p->u16RecPhase = u16Phase;

    PORT_EXIT_CRITICAL(stIrq);
}

/* 进入恢复：软件侧失效 + 状态/阶段/门禁成组提交 + 硬件错误事件计数。
 * 只有“因硬件错误从 IDLE 进入”才算硬件事件；纯软件重同步与显式重试不计。 */
static void SciPortEnterRecover(ST_SCI_PORT *p, Uint16 u16Status, Uint16 bHardwareEvent)
{
    ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

    if (p->bSwInvalidated == 0U)
    {
        SciPortSoftInvalidateInCritical(p);
    }

    p->u16RxState = SCI_RX_RECOVER_REQ;
    p->u16RecPhase = SCI_REC_PHASE_CLASSIFY;
    p->u16RxErrSeen = (Uint16)(p->u16RxErrSeen | u16Status);
    p->bRxSuspend = 1U;
    p->bTxSuspended = 1U;
    p->bWaitBaseValid = 0U;
    p->u16ClearResult = HW_SCI_CLEAR_OK;
    p->u16RecRemain = 0U;
    SciPortNoteFault(p);

    if ((bHardwareEvent != 0U) && (p->u32RxErrEvents < 0xFFFFFFFFUL))
    {
        p->u32RxErrEvents++;
    }

    PORT_EXIT_CRITICAL(stIrq);
}

/* 恢复失败/结果不确定：停止硬件访问，等显式重试。 */
static void SciPortPause(ST_SCI_PORT *p)
{
    ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

    p->u16RxState = SCI_RX_PAUSED;
    p->u16RecPhase = SCI_REC_PHASE_NONE;
    p->bRxSuspend = 1U;
    p->bTxSuspended = 1U;
    p->bWaitBaseValid = 0U;
    SciPortNoteFault(p);

    PORT_EXIT_CRITICAL(stIrq);
}

/* 丢弃 FIFO 残留的一步：打满配额返回 1（可能仍有残留），读到空返回 0。 */
static Uint16 SciPortIsrDrop(ST_SCI_PORT *p)
{
    Uint16 u16Byte;
    Uint16 i;

    if (p->pstDrv == (const ST_SCI_PORT_DRV *)0)
    {
        return 0U;
    }

    for (i = 0U; i < SCI_PORT_RX_QUOTA; i++)
    {
        if (p->pstDrv->pfTryRead(p->u16PortId, &u16Byte) == 0U)
        {
            return 0U;
        }

        if (p->u32RecoverDrop < 0xFFFFFFFFUL)
        {
            p->u32RecoverDrop++;
        }
    }

    return 1U;
}

/* 单端口一轮搬运。返回 1 = 本轮有批次提交，0 = 无动作。 */
/* O1：每轮只做一次经过校验的取时，三路共享"本轮开始服务的观察时刻"。
 *
 * 契约：**固定采用槽位 0（SCI-A）驱动表的取时函数作为全局 RX 服务时间源**，
 * 不按"哪个端口先绑定"推断；其余端口必须处于同一时间域（本工程三路共用
 * Timer0，故该实现成立）。只给 SCI-C/D 绑定替代时间源不会影响心跳取时。
 * 槽位 0 尚未绑定驱动时（初始化前的防御路径）退回默认驱动表。
 * 返回值 0 表示本轮时间不可用——调用方不得据此发布任何时标。 */
static Uint16 SciPortReadRoundTick(Uint32 *pu32Tick)
{
    const ST_SCI_PORT_DRV *pstDrv = s_astSciPort[0].pstDrv;

    if (pstDrv == (const ST_SCI_PORT_DRV *)0)
    {
        pstDrv = &s_stSciPortDrvDefault;
    }

    if (pstDrv->pfTryGetTick == (Uint16 (*)(Uint32 *))0)
    {
        return 0U;
    }

    return pstDrv->pfTryGetTick(pu32Tick);
}

static Uint16 SciPortServiceRx(ST_SCI_PORT *p, Uint32 u32TickRound, Uint16 u16TickValid)
{
    Uint16 au16Buf[SCI_PORT_RX_QUOTA];
    ST_SCI_RX_SEG stSeg;
    ST_SCI_RX_STATUS stStatus;
    Uint32 u32Tick;
    Uint32 u32Gap;
    Uint16 u16Count = 0U;
    Uint16 u16Byte;
    Uint16 u16State;
    Uint16 i;

    if ((p->bFrozen != 0U) || (p->pstDrv == (const ST_SCI_PORT_DRV *)0))
    {
        return 0U;
    }

    u16State = p->u16RxState;

    /* 前台正在操作硬件或已暂停：本轮完全不碰寄存器。 */
    if ((u16State == SCI_RX_HW_RESET) || (u16State == SCI_RX_PAUSED))
    {
        return 0U;
    }

    /* 丢弃阶段：纯软件重同步、硬件恢复请求、硬件已清等待排空。
     * 每轮仍按配额丢，直到真正读到空；只有不需要前台继续推进的状态
     * （SW_RESYNC / DRAIN_ISR）才允许回到 IDLE——RECOVER_REQ 不能被读空吞掉。 */
    if ((u16State == SCI_RX_SW_RESYNC) || (u16State == SCI_RX_DRAIN_ISR) ||
        (u16State == SCI_RX_RECOVER_REQ))
    {
        if (SciPortIsrDrop(p) == 0U)
        {
            ST_PORT_IRQ_STATE stIrq;

            p->bTimeBaseValid = 0U;
            p->u16EmptyRounds = 0U;
            p->u32TickEmptyLast = 0U;

            if (u16State != SCI_RX_RECOVER_REQ)
            {
                /* 排空完成：IDLE + NONE 与暂停标志在同一临界区内提交。 */
                stIrq = PORT_ENTER_CRITICAL();
                p->u16RxState = SCI_RX_IDLE;
                p->u16RecPhase = SCI_REC_PHASE_NONE;
                p->bRxSuspend = 0U;
                p->bSwInvalidated = 0U;   /* 生命周期只到本次排空结束：下次故障重新失效 */
                if (p->bTxResetLoss == 0U)
                {
                    p->bTxSuspended = 0U;  /* 排空完成：解除临时挂起，保留强制复位后的冻结 */
                }
                p->bStreamBreak = 1U;
                PORT_EXIT_CRITICAL(stIrq);
            }

            return 0U;
        }

        p->u16EmptyRounds = 0U;
        p->u32TickEmptyLast = 0U;
        return 0U;
    }

    /* IDLE：每轮都查接收状态，不能因为本轮 FIFO 空就跳过。
     * 这是“硬件溢出/帧错误一定会被看见”的唯一入口。 */
    if (p->pstDrv->pfGetStatus(p->u16PortId, &stStatus, 0U) == 0U)
    {
        /* 状态不可读：按硬件未知处理，进入恢复（不能当成无错误）。 */
        if (p->u16RxErrorCount < 0xFFFFU)
        {
            p->u16RxErrorCount++;
        }

        SciPortEnterRecover(p, 0U, 1U);
        return 0U;
    }

    if (stStatus.u16Status != 0U)
    {
        if (p->u16RxErrorCount < 0xFFFFU)
        {
            p->u16RxErrorCount++;
        }

        SciPortEnterRecover(p, stStatus.u16Status, 1U);
        return 0U;
    }

    /* O1：本轮的观察时刻由 SciPortService100us() 统一取一次，三路共用。
     * 共享取时失败时，本口作废时间基准并停止发布；硬件错误检测已在上面完成，
     * 已进入排空阶段的端口在上面的丢弃分支里照常按配额推进。 */
    if (u16TickValid == 0U)
    {
        p->bTimingBad = 1U;
        p->bTimeBaseValid = 0U;
        return 0U;
    }

    u32Tick = u32TickRound;

    if (p->bTimeBaseValid == 0U)
    {
        /* 首轮只建立基准，不做判定。 */
        p->u32LastTick = u32Tick;
        p->bTimeBaseValid = 1U;
        return 0U;
    }

    /* 递减计数器：差值只能按无符号减法取。 */
    u32Gap = (Uint32)(p->u32LastTick - u32Tick);
    if (u32Gap > p->u32MaxGap)
    {
        p->u32MaxGap = u32Gap;
    }

    /* O2：tick 阈值比较，避免热路径做 32 位除法。 */
    if (u32Gap >= s_u32GapLimitTicks)
    {
        p->bTimingBad = 1U;
    }

    p->u32LastTick = u32Tick;

    /* 最多读出配额字节；配额到即结束本轮，不做额外探测。 */
    for (i = 0U; i < SCI_PORT_RX_QUOTA; i++)
    {
        if (p->pstDrv->pfTryRead(p->u16PortId, &u16Byte) == 0U)
        {
            break;
        }

        au16Buf[u16Count] = u16Byte;
        u16Count++;
    }

    if (u16Count == 0U)
    {
        if (p->u16EmptyRounds < 0xFFFFU)
        {
            p->u16EmptyRounds++;
        }

        p->u32TickEmptyLast = u32Tick;
        return 0U;
    }

    /* 双环都够放才提交；否则整批拒收，不留半批。 */
    if ((RingBuffFree(&p->stSegRing) == 0U) ||
        (RingBuffFree(&p->stRxRing) < u16Count))
    {
        p->u16RxRejected = (Uint16)(p->u16RxRejected + u16Count);
        SciPortNoteFault(p);
        p->u16EmptyRounds = 0U;
        p->u32TickEmptyLast = 0U;
        return 0U;
    }

    (void)RingBuffWrite(&p->stRxRing, au16Buf, u16Count);

    stSeg.u32Tick = u32Tick;
    stSeg.u32TickEmptyLast = p->u32TickEmptyLast;
    stSeg.u16SeqStart = p->u16RxSeq;
    stSeg.u16Count = u16Count;
    stSeg.u16EmptyRounds = p->u16EmptyRounds;
    (void)RingBuffWriteExact(&p->stSegRing, (const Uint16 *)&stSeg, 1U);

    p->u16RxSeq = (Uint16)(p->u16RxSeq + u16Count);
    p->u16EmptyRounds = 0U;
    p->u32TickEmptyLast = 0U;

    /* 读满配额说明本轮被截断，后到字节要等下一轮。 */
    if (u16Count >= SCI_PORT_RX_QUOTA)
    {
        p->u16RoundsFull++;
        p->bTimingBad = 1U;
    }

    return 1U;
}

/* TX：按硬件实际接纳的前缀逐字节 Drop；遇到忙停止本轮。
 * 恢复期间（含强制复位后的冻结）不发起任何新事务。 */
static void SciPortServiceTx(ST_SCI_PORT *p)
{
    Uint16 u16Byte;
    Uint16 k;
    Uint16 u16Accepted = 0U;

    if ((p->bFrozen != 0U) || (p->pstDrv == (const ST_SCI_PORT_DRV *)0))
    {
        return;
    }

    for (k = 0U; k < SCI_PORT_TX_QUOTA; k++)
    {
        ST_PORT_IRQ_STATE stIrq;

        /* 前台轮次预算：已开始的写入允许完成，下一个字节前先看预算。 */
        if (PortBudgetHasLeft() == 0U)
        {
            if (p->u16TxBudgetStop < 0xFFFFU)
            {
                p->u16TxBudgetStop++;
            }

            return;
        }

        /* 门禁与这一次提交必须相对心跳不可分割：否则心跳在“检查完门禁、还没写”
         * 之间把端口推进到恢复态，本字节仍会被送进 FIFO，等于绕过挂起。
         * 临界区只包一个字节，不包整轮。 */
        stIrq = PORT_ENTER_CRITICAL();

        if ((p->bTxSuspended != 0U) || (p->u16RxState == SCI_RX_HW_RESET) ||
            (p->u16RxState == SCI_RX_PAUSED))
        {
            PORT_EXIT_CRITICAL(stIrq);
            return;
        }

        if (RingBuffPeek(&p->stTxRing, 0U, &u16Byte) == 0U)
        {
            PORT_EXIT_CRITICAL(stIrq);
            return;
        }

        if (p->pstDrv->pfTryWrite(p->u16PortId, u16Byte) == 0U)
        {
            /* 头文件定义：统计“本轮一个字节都没送进去”的情形；
               已经接受过前缀再遇忙属于正常截断，不算停滞。 */
            if (u16Accepted == 0U)
            {
                p->u16TxStall++;
            }

            PORT_EXIT_CRITICAL(stIrq);
            return;
        }

        (void)RingBuffDrop(&p->stTxRing, 1U);
        PORT_EXIT_CRITICAL(stIrq);

        u16Accepted++;
    }
}

/* ------------------------------------------------------------------ */
/* 恢复状态机                                                          */
/* ------------------------------------------------------------------ */

/* 一轮恢复推进：每轮至多执行一个受限硬件动作，返回 SCI_RECOVER_*。 */
static Uint16 SciPortRecoverAdvance(ST_SCI_PORT *p, Uint16 *pu16Remain)
{
    Uint16 u16Phase = p->u16RecPhase;

    if (u16Phase == SCI_REC_PHASE_VERIFY_TX)
    {
        /* 上一次暂停是"复位后 TX FIFO 仍非空"：显式重试不能凭 RX 错误位为 0
         * 就解冻，必须先确认硬件 TX 通道真的空了。 */
        Uint16 u16Idle = 0U;

        if (PortBudgetHasLeft() == 0U)
        {
            return SCI_RECOVER_BUSY;
        }

        if (p->pstDrv->pfIsTxIdle(p->u16PortId, &u16Idle) == 0U)
        {
            SciPortPause(p);
            return SCI_RECOVER_UNKNOWN;
        }

        if (u16Idle == 0U)
        {
            /* 残留仍在：保持 PAUSED 与 TX 冻结，等下一次显式重试。 */
            SciPortPause(p);
            return SCI_RECOVER_FAIL;
        }

        {
            ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

            p->u16ClearResult = HW_SCI_CLEAR_OK;
            p->u16RecPhase = SCI_REC_PHASE_CLASSIFY;

            PORT_EXIT_CRITICAL(stIrq);
        }

        u16Phase = SCI_REC_PHASE_CLASSIFY;
    }

    if (u16Phase == SCI_REC_PHASE_CLASSIFY)
    {
        ST_SCI_RX_STATUS stStatus;

        if (PortBudgetHasLeft() == 0U)
        {
            return SCI_RECOVER_BUSY;
        }

        if (p->pstDrv->pfGetStatus(p->u16PortId, &stStatus, 0U) == 0U)
        {
            SciPortPause(p);
            return SCI_RECOVER_UNKNOWN;
        }

        p->u16RxErrSeen = (Uint16)(p->u16RxErrSeen | stStatus.u16Status);
        p->u16RecRemain = stStatus.u16Status;
        p->u16RecFifoLevel = stStatus.u16RxFifoLevel;
        *pu16Remain = p->u16RecRemain;

        if (stStatus.u16Status == 0U)
        {
            /* 无需复位：等心跳排空残余后回 IDLE。 */
            SciPortCommit(p, SCI_RX_DRAIN_ISR, SCI_REC_PHASE_NONE);
            return SCI_RECOVER_OK;
        }

        /* 仍有错误：先等发送空闲，期间不补发新字节。 */
        {
            ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

            p->u16RecPhase = SCI_REC_PHASE_WAIT_TX;
            p->bTxSuspended = 1U;
            p->bWaitBaseValid = 0U;

            PORT_EXIT_CRITICAL(stIrq);
        }

        return SCI_RECOVER_BUSY;
    }

    if (u16Phase == SCI_REC_PHASE_WAIT_TX)
    {
        Uint32 u32Tick;
        Uint32 u32Elapsed;
        Uint16 u16Idle;

        if (PortBudgetHasLeft() == 0U)
        {
            return SCI_RECOVER_BUSY;
        }

        if (p->pstDrv->pfTryGetTick(&u32Tick) == 0U)
        {
            /* 时间源失效：保持暂停，不判超时；同时作废旧基准。 */
            if (p->u32TimeSrcFail < 0xFFFFFFFFUL)
            {
                p->u32TimeSrcFail++;
            }

            p->bWaitBaseValid = 0U;
            return SCI_RECOVER_BUSY;
        }

        if (p->bWaitBaseValid == 0U)
        {
            /* 时间源重置或刚进入本阶段：只重建基准，本轮不判超时。 */
            p->u32WaitStartTick = u32Tick;
            p->bWaitBaseValid = 1U;
            return SCI_RECOVER_BUSY;
        }

        if (p->pstDrv->pfIsTxIdle(p->u16PortId, &u16Idle) == 0U)
        {
            /* 查询失败不能最终变成“忙超时”。 */
            SciPortPause(p);
            return SCI_RECOVER_UNKNOWN;
        }

        if (u16Idle != 0U)
        {
            /* 发送空闲：先提交 HW_RESET + RESET，再在下一轮访问硬件。 */
            SciPortCommit(p, SCI_RX_HW_RESET, SCI_REC_PHASE_RESET);
            return SCI_RECOVER_BUSY;
        }

        u32Elapsed = (Uint32)(p->u32WaitStartTick - u32Tick);

        if (u32Elapsed >= (s_u32TicksPerUs * (SCI_PORT_WAIT_TX_LIMIT_MS * 1000UL)))
        {
            /* 超过判定阈值只是“允许强制复位”：先保存意图并成组提交状态，
             * 清队列、锁存损失与计数都留到真正执行复位的那一轮。 */
            {
                ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

                p->bForceReset = 1U;
                p->u16RxState = SCI_RX_HW_RESET;
                p->u16RecPhase = SCI_REC_PHASE_RESET;

                PORT_EXIT_CRITICAL(stIrq);
            }

            return SCI_RECOVER_BUSY;
        }

        return SCI_RECOVER_BUSY;
    }

    if (u16Phase == SCI_REC_PHASE_RESET)
    {
        Uint16 u16Result;
        ST_PORT_IRQ_STATE stIrq;

        /* 进入本阶段时已提交过 HW_RESET；这里再确认一次状态。 */
        if (p->u16RxState != SCI_RX_HW_RESET)
        {
            SciPortCommit(p, SCI_RX_HW_RESET, SCI_REC_PHASE_RESET);
        }

        if (PortBudgetHasLeft() == 0U)
        {
            /* 未执行：阶段保持 RESET，下一轮从这里继续，且不重复记损失。 */
            return SCI_RECOVER_BUSY;
        }

        /* 本轮意图先取快照：只有这一次的强制请求能决定驱动实参、清环与计数。
         * bTxResetLoss 是"曾经损失过、保持冻结"的状态，绝不能再当成下一次
         * 强制复位的指令——否则第二次独立故障会凭空多一次强制复位。 */
        {
            Uint16 bForceThis = p->bForceReset;
            /* 驱动只在结果有效（OK / STILL_ERR）时写出参；这里先用上一次
             * 可靠值兜底，避免 WAIT_TX / UNREADABLE / TX_DIRTY 把已知的
             * 错误位冲成 0。 */
            Uint16 u16DrvRemain = p->u16RecRemain;

            if (bForceThis != 0U)
            {
                (void)SciPortTxResetQueue(p->u16PortId);
                p->bForceReset = 0U;

                if (p->u32ForceResets < 0xFFFFFFFFUL)
                {
                    p->u32ForceResets++;
                }
            }

            u16Result = p->pstDrv->pfClearRxError(p->u16PortId, bForceThis, &u16DrvRemain);

            stIrq = PORT_ENTER_CRITICAL();
            p->u16ClearResult = u16Result;
            p->u16RecRemain = u16DrvRemain;
            p->u16RecPhase = SCI_REC_PHASE_RESET_DONE;
            PORT_EXIT_CRITICAL(stIrq);
        }
    }

    /* SCI_REC_PHASE_RESET_DONE：评估清除结果。 */
    {
        Uint16 u16Result = p->u16ClearResult;
        ST_PORT_IRQ_STATE stIrq;

        /* 清除结果已经保存：预算不足就停在 RESET_DONE，下一轮只判结果、
         * 不再碰硬件，也不重复记损失。 */
        if (PortBudgetHasLeft() == 0U)
        {
            return SCI_RECOVER_BUSY;
        }

        if (u16Result == HW_SCI_CLEAR_OK)
        {
            stIrq = PORT_ENTER_CRITICAL();
            p->u16RxState = SCI_RX_DRAIN_ISR;
            p->u16RecPhase = SCI_REC_PHASE_NONE;
            p->bWaitBaseValid = 0U;

            /* 临时挂起不在这里解除：清完硬件还会排空一段时间，ISR 期间
             * 允许发送就会在残留数据之后立刻补发新字节。统一由 ISR 在
             * 提交 IDLE/NONE 时解除（强制复位留下的冻结另有 bTxResetLoss 管）。 */
            PORT_EXIT_CRITICAL(stIrq);
            return SCI_RECOVER_OK;
        }

        if (u16Result == HW_SCI_CLEAR_WAIT_TX)
        {
            stIrq = PORT_ENTER_CRITICAL();
            p->u16RxState = SCI_RX_RECOVER_REQ;
            p->u16RecPhase = SCI_REC_PHASE_WAIT_TX;
            p->bTxSuspended = 1U;
            p->bWaitBaseValid = 0U;
            PORT_EXIT_CRITICAL(stIrq);
            return SCI_RECOVER_BUSY;
        }

        if (u16Result == HW_SCI_CLEAR_TX_DIRTY)
        {
            /* 复位做了，但硬件没接受：残留字节仍可能上线。进 PAUSED 等
             * 显式重试，不能退回等待阶段自动再来一次强制复位。 */
            SciPortPause(p);
            return SCI_RECOVER_FAIL;
        }

        /* 剩余错误位在上一步已存进端口对象；这里不碰调用方出参。 */
        SciPortPause(p);
        return (u16Result == HW_SCI_CLEAR_UNREADABLE) ? SCI_RECOVER_UNKNOWN
                                                     : SCI_RECOVER_FAIL;
    }
}

/* ------------------------------------------------------------------ */
/* 对外接口                                                            */
/* ------------------------------------------------------------------ */

void SciPortInit(void)
{
    Uint16 i;

    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        ST_SCI_PORT *p = &s_astSciPort[i];
        Uint16 *pu16Raw = (Uint16 *)p;
        Uint16 u16Words = (Uint16)(sizeof(ST_SCI_PORT) / sizeof(Uint16));
        Uint16 j;

        for (j = 0U; j < u16Words; j++)
        {
            pu16Raw[j] = 0U;
        }
    }

    s_astSciPort[0].u16PortId = HW_SCI_ID_A;
    s_astSciPort[1].u16PortId = HW_SCI_ID_C;
    s_astSciPort[2].u16PortId = HW_SCI_ID_D;

    (void)RingBuffInitItems(&s_astSciPort[0].stRxRing, s_au16RxBufA,
                            (Uint32)(64UL * 1UL), 64U, 1U);
    (void)RingBuffInitItems(&s_astSciPort[0].stSegRing, s_au16SegBufA,
                            (Uint32)(16UL * SCI_SEG_ITEM_WORDS), 16U,
                            SCI_SEG_ITEM_WORDS);
    (void)RingBuffInitItems(&s_astSciPort[0].stTxRing, s_au16TxBufA,
                            (Uint32)(128UL * 1UL), 128U, 1U);

    (void)RingBuffInitItems(&s_astSciPort[1].stRxRing, s_au16RxBufC,
                            (Uint32)(256UL * 1UL), 256U, 1U);
    (void)RingBuffInitItems(&s_astSciPort[1].stSegRing, s_au16SegBufC,
                            (Uint32)(32UL * SCI_SEG_ITEM_WORDS), 32U,
                            SCI_SEG_ITEM_WORDS);
    (void)RingBuffInitItems(&s_astSciPort[1].stTxRing, s_au16TxBufC,
                            (Uint32)(128UL * 1UL), 128U, 1U);

    (void)RingBuffInitItems(&s_astSciPort[2].stRxRing, s_au16RxBufD,
                            (Uint32)(128UL * 1UL), 128U, 1U);
    (void)RingBuffInitItems(&s_astSciPort[2].stSegRing, s_au16SegBufD,
                            (Uint32)(16UL * SCI_SEG_ITEM_WORDS), 16U,
                            SCI_SEG_ITEM_WORDS);
    (void)RingBuffInitItems(&s_astSciPort[2].stTxRing, s_au16TxBufD,
                            (Uint32)(128UL * 1UL), 128U, 1U);

    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        s_astSciPort[i].pstDrv = &s_stSciPortDrvDefault;
        s_astSciPort[i].u16RxState = SCI_RX_IDLE;
        s_astSciPort[i].u16RecPhase = SCI_REC_PHASE_NONE;
    }

    /* O2：阈值与当前 ticksPerUs 对齐（时基变化时由 SciPortSetSysclkHz 再同步）。 */
    SciPortUpdateGapLimit();
}

Uint16 SciPortBindDriver(Uint16 u16PortId, const ST_SCI_PORT_DRV *pstDrv)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    if ((p == (ST_SCI_PORT *)0) || (pstDrv == (const ST_SCI_PORT_DRV *)0))
    {
        return 0U;
    }

    p->pstDrv = pstDrv;
    return 1U;
}

void SciPortSetSysclkHz(Uint32 u32SysclkHz)
{
    Uint32 u32TicksPerUs = u32SysclkHz / 1000000UL;

    s_u32TicksPerUs = (u32TicksPerUs == 0UL) ? 1UL : u32TicksPerUs;
    SciPortUpdateGapLimit();
}

Uint16 SciPortService100us(void)
{
    Uint16 i;
    Uint16 u16Active = 0U;
    Uint16 u16TickValid;
    Uint32 u32RoundTick = 0UL;

    if (s_u32ServiceCalls < 0xFFFFFFFFUL)
    {
        s_u32ServiceCalls++;
    }

    /* O1：本轮只取一次时标，三路共享同一个观察时刻。 */
    u16TickValid = SciPortReadRoundTick(&u32RoundTick);

    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        u16Active = (Uint16)(u16Active +
                             SciPortServiceRx(&s_astSciPort[i], u32RoundTick, u16TickValid));
    }

    return u16Active;
}

void SciPortTask(void)
{
    Uint16 i;

    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        SciPortServiceTx(&s_astSciPort[i]);
    }
}

void SciPortTaskOne(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    if (p == (ST_SCI_PORT *)0)
    {
        return;
    }

    SciPortServiceTx(p);
}

Uint16 SciPortRecover(Uint16 u16PortId, Uint16 *pu16Remain)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    /* 出参只在参数与驱动全部校验通过后才写：失败时保持调用方原值，
     * 便于用非零哨兵证明“失败不被写成 0”。 */
    if ((p == (ST_SCI_PORT *)0) || (pu16Remain == (Uint16 *)0) ||
        (p->pstDrv == (const ST_SCI_PORT_DRV *)0) ||
        (p->pstDrv->pfGetStatus == (Uint16 (*)(Uint16, ST_SCI_RX_STATUS *, Uint16))0) ||
        (p->pstDrv->pfClearRxError == (Uint16 (*)(Uint16, Uint16, Uint16 *))0) ||
        (p->pstDrv->pfIsTxIdle == (Uint16 (*)(Uint16, Uint16 *))0))
    {
        return SCI_RECOVER_PARAM;
    }

    switch (p->u16RxState)
    {
    case SCI_RX_IDLE:
        /* 没有待处理的错误：出参是有效报告（0 = 无剩余错误位）。 */
        *pu16Remain = 0U;
        return SCI_RECOVER_IDLE_OK;

    case SCI_RX_SW_RESYNC:
    case SCI_RX_DRAIN_ISR:
        /* 等心跳排空，前台无事可做。 */
        return SCI_RECOVER_BUSY;

    case SCI_RX_PAUSED:
        /* 暂停只能经 SciPortRecoverRetry() 退出；出参报最近一次可靠结果。 */
        *pu16Remain = p->u16RecRemain;
        return SCI_RECOVER_FAIL;

    default:
        break;
    }

    /* 出参一律由"本次已保存的结果"生成，不依赖本次调用内部的临时变量：
     * 一次调用可能同时走完 RESET 与 RESET_DONE，此时新结果已经写进端口
     * 对象，用进入函数时的旧值回写就会把上一次的错误位交给调用方。
     * BUSY / UNKNOWN 期间保持调用方原值，便于用哨兵证明契约成立。 */
    {
        Uint16 u16Ignore = 0U;
        Uint16 u16Ret = SciPortRecoverAdvance(p, &u16Ignore);

        if ((u16Ret == SCI_RECOVER_OK) || (u16Ret == SCI_RECOVER_IDLE_OK))
        {
            *pu16Remain = 0U;
        }
        else if (u16Ret == SCI_RECOVER_FAIL)
        {
            *pu16Remain = p->u16RecRemain;
        }

        return u16Ret;
    }
}

void SciPortInvalidateWaitBases(void)
{
    Uint16 i;
    ST_PORT_IRQ_STATE stIrq = PORT_ENTER_CRITICAL();

    /* 时间源刚刚失败过：所有正在等待发送空闲的端口都必须重新建立基准。
     * 否则"旧基准 + 新计数"会立刻越过 8 ms 判定阈值，强制复位在没有
     * 连续观察的情况下被触发。整轮预算失败期间由调用者统一调用本函数。 */
    for (i = 0U; i < SCI_PORT_NUM; i++)
    {
        s_astSciPort[i].bWaitBaseValid = 0U;
    }

    PORT_EXIT_CRITICAL(stIrq);
}

Uint16 SciPortRecoverRetry(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;

    if ((p == (ST_SCI_PORT *)0) || (p->pstDrv == (const ST_SCI_PORT_DRV *)0))
    {
        return SCI_RECOVER_PARAM;
    }

    if (p->u16RxState != SCI_RX_PAUSED)
    {
        return (p->u16RxState == SCI_RX_IDLE) ? SCI_RECOVER_IDLE_OK : SCI_RECOVER_BUSY;
    }

    /* 时间源不可用时不推进：保持在 PAUSED，等下一次显式重试。 */
    {
        Uint32 u32Tick;

        if (p->pstDrv->pfTryGetTick(&u32Tick) == 0U)
        {
            if (p->u32TimeSrcFail < 0xFFFFFFFFUL)
            {
                p->u32TimeSrcFail++;
            }

            return SCI_RECOVER_UNKNOWN;
        }
    }

    stIrq = PORT_ENTER_CRITICAL();

    /* 新的一次恢复尝试：重新做软件失效，但不再计一次硬件错误事件，
     * 也不清 u16RxErrSeen——累计错误证据要留给上一轮诊断。 */
    SciPortSoftInvalidateInCritical(p);
    p->u16RxState = SCI_RX_RECOVER_REQ;
    /* 保留进入 PAUSED 的原因：若是 TX FIFO 残留，重试必须重新核验；
     * 若一律改回 OK，CLASSIFY 会只看 RX 错误位就解冻。 */
    p->u16RecPhase = (p->u16ClearResult == HW_SCI_CLEAR_TX_DIRTY)
                         ? SCI_REC_PHASE_VERIFY_TX
                         : SCI_REC_PHASE_CLASSIFY;
    p->bRxSuspend = 1U;
    p->bTxSuspended = 1U;
    p->bWaitBaseValid = 0U;
    SciPortNoteFault(p);

    PORT_EXIT_CRITICAL(stIrq);

    return SCI_RECOVER_OK;
}

Uint16 SciPortRxState(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0xFFFFU : p->u16RxState;
}

Uint16 SciPortRecPhase(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0xFFFFU : p->u16RecPhase;
}

Uint16 SciPortClearResult(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0xFFFFU : p->u16ClearResult;
}

Uint16 SciPortRxErrSeen(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16RxErrSeen;
}

Uint32 SciPortRxErrEvents(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32RxErrEvents;
}

Uint32 SciPortRecoverDrop(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32RecoverDrop;
}

Uint32 SciPortTimeSrcFail(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32TimeSrcFail;
}

Uint16 SciPortIsTxSuspended(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 1U : p->bTxSuspended;
}

Uint16 SciPortTxResetQueue(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;
    Uint16 u16Dropped;

    if ((p == (ST_SCI_PORT *)0) || (p->pstDrv == (const ST_SCI_PORT_DRV *)0))
    {
        return 0U;
    }

    stIrq = PORT_ENTER_CRITICAL();

    u16Dropped = (Uint16)RingBuffUsed(&p->stTxRing);
    (void)RingBuffClear(&p->stTxRing);

    /* 只有“实际执行了复位”才记损失与冻结，预算不足时不会走到这里。 */
    p->u32TxResetDrop = p->u32TxResetDrop + (Uint32)u16Dropped;
    p->bTxResetLoss = 1U;
    p->bTxSuspended = 1U;

    PORT_EXIT_CRITICAL(stIrq);

    return 1U;
}

Uint16 SciPortTxResume(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;
    Uint16 u16Ret = 0U;

    if (p == (ST_SCI_PORT *)0)
    {
        return 0U;
    }

    stIrq = PORT_ENTER_CRITICAL();

    /* 只有“恢复已完成（IDLE）且确实发生过强制复位丢弃”才能解冻；
     * PAUSED / HW_RESET / 恢复中一律拒绝，避免绕过未完成的恢复流程。 */
    if ((p->u16RxState == SCI_RX_IDLE) && (p->bTxResetLoss != 0U))
    {
        p->bTxResetLoss = 0U;
        p->bTxSuspended = 0U;
        u16Ret = 1U;
    }

    PORT_EXIT_CRITICAL(stIrq);

    return u16Ret;
}

Uint32 SciPortTxResetDrop(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32TxResetDrop;
}

Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max,
                       ST_SCI_RX_SEG *pstSeg)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;
    ST_SCI_RX_SEG stSeg;
    Uint16 u16Read;

    if ((p == (ST_SCI_PORT *)0) || (pu16Dst == (Uint16 *)0) ||
        (pstSeg == (ST_SCI_RX_SEG *)0))
    {
        return 0U;
    }

    stIrq = PORT_ENTER_CRITICAL();

    if (RingBuffPeek(&p->stSegRing, 0U, (Uint16 *)&stSeg) == 0U)
    {
        PORT_EXIT_CRITICAL(stIrq);
        return 0U;
    }

    if (stSeg.u16Count == 0U)
    {
        (void)RingBuffDrop(&p->stSegRing, 1U);
        PORT_EXIT_CRITICAL(stIrq);
        return 0U;
    }

    if (stSeg.u16Count > u16Max)
    {
        PORT_EXIT_CRITICAL(stIrq);
        return 0U;
    }

    if (stSeg.u16SeqStart != p->u16RxSeqConsumed)
    {
        SciPortNoteFault(p);
        PORT_EXIT_CRITICAL(stIrq);
        return 0U;
    }

    u16Read = RingBuffRead(&p->stRxRing, pu16Dst, stSeg.u16Count);

    if (u16Read != stSeg.u16Count)
    {
        SciPortNoteFault(p);
        PORT_EXIT_CRITICAL(stIrq);
        return 0U;
    }

    (void)RingBuffDrop(&p->stSegRing, 1U);
    p->u16RxSeqConsumed = (Uint16)(p->u16RxSeqConsumed + stSeg.u16Count);
    *pstSeg = stSeg;

    PORT_EXIT_CRITICAL(stIrq);
    return stSeg.u16Count;
}

Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    Uint16 u16Written;

    if ((p == (ST_SCI_PORT *)0) || (pu16Data == (const Uint16 *)0) ||
        (u16Count == 0U))
    {
        return 0U;
    }

    u16Written = RingBuffWriteExact(&p->stTxRing, pu16Data, u16Count);

    if (u16Written == 0U)
    {
        p->u16TxRejected = (Uint16)(p->u16TxRejected + u16Count);
    }

    return u16Written;
}

Uint16 SciPortTxFree(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    if (p == (ST_SCI_PORT *)0)
    {
        return 0U;
    }

    return RingBuffFree(&p->stTxRing);
}

void SciPortResync(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;

    if (p == (ST_SCI_PORT *)0)
    {
        return;
    }

    /* 前台正在复位或端口已暂停：这条路径拥有该端口，重同步不得改写状态，
     * 也不得递增故障代次（否则会用一次软件请求冒充硬件故障）。 */
    if ((p->u16RxState == SCI_RX_HW_RESET) || (p->u16RxState == SCI_RX_PAUSED))
    {
        return;
    }

    stIrq = PORT_ENTER_CRITICAL();

    /* 任何状态下都先做软件侧失效：这是“把旧流切断”的唯一手段。 */
    SciPortSoftInvalidateInCritical(p);

    switch (p->u16RxState)
    {
    case SCI_RX_IDLE:
        /* 纯软件重同步：进入丢弃阶段，由心跳排空 FIFO 残留。
         * 纯软件事件不计入硬件错误事件数，但代次仍要递增。 */
        p->u16RxState = SCI_RX_SW_RESYNC;
        p->u16RecPhase = SCI_REC_PHASE_NONE;
        p->bRxSuspend = 1U;
        SciPortNoteFault(p);
        break;

    case SCI_RX_SW_RESYNC:
    case SCI_RX_DRAIN_ISR:
    case SCI_RX_RECOVER_REQ:
    case SCI_RX_HW_RESET:
    case SCI_RX_PAUSED:
    default:
        /* 恢复流程中再次请求：合并为“重做软件失效”，不改变状态机阶段，
         * 也不打断已经在进行的清除；调用方可用 SciPortRxState() 观察合并结果。 */
        SciPortNoteFault(p);
        break;
    }

    PORT_EXIT_CRITICAL(stIrq);
}

Uint16 SciPortIsStreamBreak(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->bStreamBreak;
}

Uint16 SciPortFaultGeneration(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16FaultGen;
}

Uint16 SciPortClearStreamBreak(Uint16 u16PortId, Uint16 u16Generation)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    ST_PORT_IRQ_STATE stIrq;
    Uint16 u16Cleared = 0U;

    if (p == (ST_SCI_PORT *)0)
    {
        return 0U;
    }

    stIrq = PORT_ENTER_CRITICAL();

    /* 丢弃/恢复仍在进行：此时确认等于谎报“已恢复”。 */
    if ((p->bRxSuspend != 0U) || (p->u16RxState != SCI_RX_IDLE))
    {
        u16Cleared = 0U;
    }
    /* 代次不一致说明确认期间又发生了新故障：保留故障，交给前台重新处理。 */
    else if (p->u16FaultGen == u16Generation)
    {
        p->bStreamBreak = 0U;
        u16Cleared = 1U;
    }

    PORT_EXIT_CRITICAL(stIrq);
    return u16Cleared;
}

Uint16 SciPortIsResyncDraining(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    Uint16 u16State;

    if (p == (ST_SCI_PORT *)0)
    {
        return 0U;
    }

    u16State = p->u16RxState;

    return ((u16State == SCI_RX_SW_RESYNC) || (u16State == SCI_RX_DRAIN_ISR))
               ? 1U
               : 0U;
}

Uint16 SciPortRxErrorCount(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16RxErrorCount;
}

Uint16 SciPortTxBudgetStop(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16TxBudgetStop;
}

Uint16 SciPortIsTimingBad(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->bTimingBad;
}

Uint16 SciPortRxRejected(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16RxRejected;
}

Uint16 SciPortTxStall(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16TxStall;
}

Uint16 SciPortRxUsed(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : RingBuffUsed(&p->stRxRing);
}

Uint32 SciPortMaxGap(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32MaxGap;
}

Uint16 SciPortTicksToUs(Uint32 u32Ticks)
{
    return SciPortGapToUs(u32Ticks);
}

Uint16 SciPortRecoveryFlags(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);
    Uint16 u16Flags = 0U;

    if (p == (ST_SCI_PORT *)0)
    {
        return 0U;
    }

    if ((p->u16RxState == SCI_RX_RECOVER_REQ) || (p->u16RxState == SCI_RX_HW_RESET) ||
        (p->u16RxState == SCI_RX_PAUSED))
    {
        u16Flags = (Uint16)(u16Flags | SCI_RECOVER_FLAG_VALID);
    }

    if (p->u16RecPhase == SCI_REC_PHASE_WAIT_TX)
    {
        u16Flags = (Uint16)(u16Flags | SCI_RECOVER_FLAG_WAIT_TX);
    }

    if (p->u16ClearResult == HW_SCI_CLEAR_STILL_ERR)
    {
        u16Flags = (Uint16)(u16Flags | SCI_RECOVER_FLAG_STILL_ERR);
    }

    if (p->u16ClearResult == HW_SCI_CLEAR_UNREADABLE)
    {
        u16Flags = (Uint16)(u16Flags | SCI_RECOVER_FLAG_UNREADABLE);
    }

    return u16Flags;
}

Uint16 SciPortRecFifoLevel(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0U : p->u16RecFifoLevel;
}

Uint32 SciPortForceResets(Uint16 u16PortId)
{
    ST_SCI_PORT *p = SciPortFind(u16PortId);

    return (p == (ST_SCI_PORT *)0) ? 0UL : p->u32ForceResets;
}

Uint32 SciPortServiceCallCount(void)
{
    return s_u32ServiceCalls;
}

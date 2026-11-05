/**
 * @file CanPort.c
 * @author yetingheng (004182)
 * @brief CAN 端口层实现：帧环收发、覆盖可见性、待裁定队头
 * @version 0.1
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 执行上下文：05A 中全部在 1 ms 前台（CanPortService 由 PortTask 调用），
 * 与 RX ISR 的并发留给 05B；因此本文件不含临界区，帧环只由前台单方推进。
 */

#include "CanPort.h"
#include "PortBudget.h"

/* 每模块 16 帧接收 + 16 帧发送，共 2 x 32 x 12 字。 */
static volatile Uint16 s_au16CanRxBufA[CAN_QUEUE_WORDS * 16U];
static volatile Uint16 s_au16CanTxBufA[CAN_QUEUE_WORDS * 16U];
static volatile Uint16 s_au16CanRxBufB[CAN_QUEUE_WORDS * 16U];
static volatile Uint16 s_au16CanTxBufB[CAN_QUEUE_WORDS * 16U];

static ST_CAN_PORT s_astCanPort[CAN_PORT_NUM];

static const ST_CAN_PORT_DRV s_stCanPortDrvDefault =
{
    Hw_CanIsInitialized,
    Hw_CanIsPending,
    Hw_CanReadRx,
    Hw_CanSend
};

static ST_CAN_PORT *CanPortFind(Uint16 u16CanId)
{
    Uint16 i;

    for (i = 0U; i < CAN_PORT_NUM; i++)
    {
        if (s_astCanPort[i].u16CanId == u16CanId)
        {
            return &s_astCanPort[i];
        }
    }

    return (ST_CAN_PORT *)0;
}

static void CanPortZeroPort(ST_CAN_PORT *p)
{
    Uint16 *pu16Raw = (Uint16 *)p;
    Uint16 u16Words = (Uint16)(sizeof(ST_CAN_PORT) / sizeof(Uint16));
    Uint16 i;

    for (i = 0U; i < u16Words; i++)
    {
        pu16Raw[i] = 0U;
    }
}

/* 一帧从驱动结果转成帧环元素。 */
static void CanPortBuildFrame(ST_CAN_FRAME *pstFrame, Uint32 u32MsgId,
                              const Uint16 *pu16Data, Uint16 u16Len,
                              Uint16 u16FrameType)
{
    Uint16 i;

    pstFrame->id = u32MsgId;
    pstFrame->flags = (u16FrameType == HW_CAN_FRAME_EXT) ? CAN_FRAME_EXT : 0U;
    pstFrame->dlc = (u16Len > HW_CAN_DATA_LEN_MAX) ? HW_CAN_DATA_LEN_MAX : u16Len;

    for (i = 0U; i < HW_CAN_DATA_LEN_MAX; i++)
    {
        pstFrame->data[i] = (i < u16Len) ? pu16Data[i] : 0U;
    }
}

static void CanPortServiceRx(ST_CAN_PORT *p)
{
    Uint16 u16Data[HW_CAN_DATA_LEN_MAX];
    Uint32 u32MsgId;
    Uint16 u16Len;
    Uint16 u16FrameType;
    Uint16 u16MsgLst;
    Uint16 u16Ret;
    Uint16 i;

    if ((p->bRxFrozen != 0U) || (p->pstDrv == (const ST_CAN_PORT_DRV *)0))
    {
        return;
    }

    if (p->pstDrv->pfIsInitialized(p->u16CanId) == 0U)
    {
        p->bNotReady = 1U;
        return;
    }

    if (p->pstDrv->pfIsPending(p->u16CanId) != 0U)
    {
        return;
    }

    for (i = 0U; i < CAN_PORT_RX_QUOTA; i++)
    {
        /* 前台轮次预算：每次驱动调用前检查，已开始的调用允许完成。 */
        if (PortBudgetHasLeft() == 0U)
        {
            return;
        }

        u16MsgLst = 0U;
        u16FrameType = HW_CAN_FRAME_STD;
        u16Len = 0U;
        u32MsgId = 0UL;

        u16Ret = p->pstDrv->pfReadRx(p->u16CanId, p->u16RxObjIdx, &u32MsgId,
                                     u16Data, &u16Len, &u16FrameType, &u16MsgLst);

        if (u16Ret == HW_CAN_EMPTY)
        {
            return;
        }

        if (u16Ret == HW_CAN_BUSY)
        {
            p->u32ReadBusy++;
            return;
        }

        if (u16Ret == HW_CAN_ERR)
        {
            p->u32ReadErr++;
            return;
        }

        if (u16Ret == HW_CAN_UNKNOWN)
        {
            /* 读结果不确定：冻结 RX，等恢复通知，不假设该帧已消费。 */
            p->u32ReadUnknown++;
            p->bRxFrozen = 1U;
            return;
        }

        p->u32ReadOk++;

        /* 粘滞覆盖标志：按对象锁存，同一轮只置一次。 */
        if (u16MsgLst != 0U)
        {
            p->bHwOverwriteSeen = 1U;
        }

        {
            ST_CAN_FRAME stFrame;

            CanPortBuildFrame(&stFrame, u32MsgId, u16Data, u16Len, u16FrameType);

            if (CanQueuePush(&p->stRxQueue, &stFrame) == 0U)
            {
                /* 软件帧满：与硬件覆盖分开计数。 */
                p->u32SwDrop++;
                p->bHwOverwriteSeen = (u16MsgLst != 0U) ? 1U : p->bHwOverwriteSeen;
            }
        }
    }
}

static void CanPortServiceTx(ST_CAN_PORT *p)
{
    ST_CAN_FRAME stFrame;
    Uint16 u16Ret;
    Uint16 i;

    if ((p->bTxFrozen != 0U) || (p->bHeadPending != 0U) ||
        (p->pstDrv == (const ST_CAN_PORT_DRV *)0))
    {
        return;
    }

    if (p->pstDrv->pfIsInitialized(p->u16CanId) == 0U)
    {
        p->bNotReady = 1U;
        return;
    }

    for (i = 0U; i < CAN_PORT_TX_QUOTA; i++)
    {
        /* 轮次预算：超预算不再发起新事务。 */
        if (PortBudgetHasLeft() == 0U)
        {
            p->u32TxBudgetStop++;
            return;
        }

        /* 模块里还有未结束的事务（例如本轮 RX 刚判 UNKNOWN）时，
         * 不得再发起新的发送事务，否则会把未决状态搅乱。 */
        if (p->pstDrv->pfIsPending(p->u16CanId) != 0U)
        {
            p->u32TxPendingBlock++;
            return;
        }

        if (CanQueuePeek(&p->stTxQueue, &stFrame) == 0U)
        {
            return;
        }

        u16Ret = p->pstDrv->pfSend(p->u16CanId, p->u16TxObjIdx, stFrame.id,
                                   stFrame.data, stFrame.dlc);

        if (u16Ret == HW_CAN_OK)
        {
            (void)CanQueueDrop(&p->stTxQueue);
            p->u32TxOk++;
            continue;
        }

        if (u16Ret == HW_CAN_BUSY)
        {
            p->u32TxBusy++;
            return;
        }

        if (u16Ret == HW_CAN_ERR)
        {
            p->u32TxErr++;
            return;
        }

        /* OK/EMPTY 以外只可能是 UNKNOWN：提交结果不确定。
         * 不 DROP、不重发，冻结队头等上层裁定。 */
        p->u32TxUnknown++;
        p->bHeadPending = 1U;
        p->bTxFrozen = 1U;
        return;
    }
}

void CanPortInit(void)
{
    Uint16 i;

    for (i = 0U; i < CAN_PORT_NUM; i++)
    {
        CanPortZeroPort(&s_astCanPort[i]);
    }

    s_astCanPort[0].u16CanId = HW_CAN_ID_A;
    s_astCanPort[0].u16TxObjIdx = 0U;
    s_astCanPort[0].u16RxObjIdx = 0U;
    s_astCanPort[0].bTxExtId = 0U;
    s_astCanPort[0].pstDrv = &s_stCanPortDrvDefault;

    s_astCanPort[1].u16CanId = HW_CAN_ID_B;
    s_astCanPort[1].u16TxObjIdx = 0U;
    s_astCanPort[1].u16RxObjIdx = 0U;
    s_astCanPort[1].bTxExtId = 0U;
    s_astCanPort[1].pstDrv = &s_stCanPortDrvDefault;

    (void)CanQueueInit(&s_astCanPort[0].stRxQueue, s_au16CanRxBufA,
                       (Uint32)(CAN_QUEUE_WORDS * 16U), 16U);
    (void)CanQueueInit(&s_astCanPort[0].stTxQueue, s_au16CanTxBufA,
                       (Uint32)(CAN_QUEUE_WORDS * 16U), 16U);
    (void)CanQueueInit(&s_astCanPort[1].stRxQueue, s_au16CanRxBufB,
                       (Uint32)(CAN_QUEUE_WORDS * 16U), 16U);
    (void)CanQueueInit(&s_astCanPort[1].stTxQueue, s_au16CanTxBufB,
                       (Uint32)(CAN_QUEUE_WORDS * 16U), 16U);
}

Uint16 CanPortBindDriver(Uint16 u16CanId, const ST_CAN_PORT_DRV *pstDrv)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if ((p == (ST_CAN_PORT *)0) || (pstDrv == (const ST_CAN_PORT_DRV *)0))
    {
        return 0U;
    }

    p->pstDrv = pstDrv;
    return 1U;
}

void CanPortService(void)
{
    Uint16 i;

    for (i = 0U; i < CAN_PORT_NUM; i++)
    {
        CanPortServiceRx(&s_astCanPort[i]);
        CanPortServiceTx(&s_astCanPort[i]);
    }
}

void CanPortServiceOne(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if (p == (ST_CAN_PORT *)0)
    {
        return;
    }

    CanPortServiceRx(p);
    CanPortServiceTx(p);
}

Uint16 CanPortPopRx(Uint16 u16CanId, ST_CAN_FRAME *pstFrame)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if ((p == (ST_CAN_PORT *)0) || (pstFrame == (ST_CAN_FRAME *)0))
    {
        return 0U;
    }

    return CanQueuePop(&p->stRxQueue, pstFrame);
}

Uint16 CanPortPushTx(Uint16 u16CanId, const ST_CAN_FRAME *pstFrame)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);
    Uint16 bFrameExt;

    if ((p == (ST_CAN_PORT *)0) || (pstFrame == (const ST_CAN_FRAME *)0))
    {
        return 0U;
    }

    /* 发送接口不支持远程帧：宁可在入队时拒绝，也不要发出一条与请求不同的报文。 */
    if ((pstFrame->flags & CAN_FRAME_RTR) != 0U)
    {
        p->u32TxRtrReject++;
        return 0U;
    }

    if (pstFrame->dlc > HW_CAN_DATA_LEN_MAX)
    {
        p->u32TxTypeReject++;
        return 0U;
    }

    /* 帧类型必须与本模块的固定发送类型一致。 */
    bFrameExt = ((pstFrame->flags & CAN_FRAME_EXT) != 0U) ? 1U : 0U;

    if (bFrameExt != ((p->bTxExtId != 0U) ? 1U : 0U))
    {
        p->u32TxTypeReject++;
        return 0U;
    }

    return CanQueuePush(&p->stTxQueue, pstFrame);
}

Uint16 CanPortSetTxFrameType(Uint16 u16CanId, Uint16 bExtId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if (p == (ST_CAN_PORT *)0)
    {
        return 0U;
    }

    /* 只允许当作初始化配置：
       队列里已有帧时改类型，会让已入队的帧与新的校验规则不一致；
       存在待裁定队头或驱动仍有未决事务时改类型，会让"已提交但结果未知"的
       那条报文与新规则混淆。 */
    if ((RingBuffUsed(&p->stTxQueue.ring) != 0U)
        || (p->bHeadPending != 0U)
        || (p->pstDrv->pfIsPending(p->u16CanId) != 0U))
    {
        return 0U;
    }

    /* 注意：未决检查**不以"已初始化"为前提**。模块重建失败后会保留未决标记，
       此时 IsInitialized 可能已经是 0，但那条"已提交、结果未知"的报文仍在，
       改校验规则会让它与新规则混淆。 */

    p->bTxExtId = (bExtId != 0U) ? 1U : 0U;
    return 1U;
}

Uint16 CanPortTxTypeReject(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : ((p->u32TxTypeReject > 0xFFFFUL) ? 0xFFFFU
                                                                          : (Uint16)p->u32TxTypeReject);
}

Uint16 CanPortTxRtrReject(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : ((p->u32TxRtrReject > 0xFFFFUL) ? 0xFFFFU
                                                                         : (Uint16)p->u32TxRtrReject);
}

Uint32 CanPortTxPendingBlock(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0UL : p->u32TxPendingBlock;
}

Uint32 CanPortTxBudgetStop(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0UL : p->u32TxBudgetStop;
}

Uint16 CanPortTxFree(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : RingBuffFree(&p->stTxQueue.ring);
}

Uint16 CanPortResolvePendingHead(Uint16 u16CanId, Uint16 u16Action)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);
    ST_CAN_FRAME stFrame;

    if (p == (ST_CAN_PORT *)0)
    {
        return HW_CAN_ERR;
    }

    /* 没有待裁定队头：重复裁定不得再动队列。 */
    if (p->bHeadPending == 0U)
    {
        return HW_CAN_ERR;
    }

    /* 只接受 DROP/RETRY。 */
    if ((u16Action != CAN_PORT_HEAD_DROP) && (u16Action != CAN_PORT_HEAD_RETRY))
    {
        return HW_CAN_ERR;
    }

    if (p->pstDrv == (const ST_CAN_PORT_DRV *)0)
    {
        return HW_CAN_ERR;
    }

    if (p->pstDrv->pfIsInitialized(p->u16CanId) == 0U)
    {
        return HW_CAN_BUSY;
    }

    /* 原事务必须已经明确结束（不再 pending）。 */
    if (p->pstDrv->pfIsPending(p->u16CanId) != 0U)
    {
        return HW_CAN_BUSY;
    }

    /* 队头必须真的存在。 */
    if (CanQueuePeek(&p->stTxQueue, &stFrame) == 0U)
    {
        return HW_CAN_ERR;
    }

    if (u16Action == CAN_PORT_HEAD_DROP)
    {
        (void)CanQueueDrop(&p->stTxQueue);
    }

    p->bHeadPending = 0U;
    p->bTxFrozen = 0U;
    return HW_CAN_OK;
}

void CanPortOnBusOffCleared(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if (p == (ST_CAN_PORT *)0)
    {
        return;
    }

    /* Bus-Off 恢复只恢复接收可用性；TX 队头的裁决仍由 ResolvePendingHead 负责。 */
    p->bRxFrozen = 0U;
    p->bNotReady = 0U;
}

void CanPortOnModuleRecovered(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    if (p == (ST_CAN_PORT *)0)
    {
        return;
    }

    p->bRxFrozen = 0U;
    p->bNotReady = 0U;
    p->bHwOverwriteSeen = 0U;
}

Uint16 CanPortIsRxFrozen(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : p->bRxFrozen;
}

Uint16 CanPortIsTxFrozen(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : p->bTxFrozen;
}

Uint16 CanPortIsReady(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : (Uint16)(p->bNotReady == 0U);
}

Uint16 CanPortHwOverwriteSeen(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0U : p->bHwOverwriteSeen;
}


Uint32 CanPortSwDrop(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0UL : p->u32SwDrop;
}

Uint32 CanPortReadErr(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0UL : p->u32ReadErr;
}

Uint32 CanPortTxErr(Uint16 u16CanId)
{
    ST_CAN_PORT *p = CanPortFind(u16CanId);

    return (p == (ST_CAN_PORT *)0) ? 0UL : p->u32TxErr;
}

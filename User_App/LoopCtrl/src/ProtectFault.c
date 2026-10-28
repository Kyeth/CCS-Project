/**
 * @file ProtectFault.c
 * @author yetingheng (004182)
 * @brief 保护故障框架实现：记录、去抖、聚合与动作等级
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 保护框架实现。所有判据由外部上报，本文件只负责状态机与聚合。
 */
#include "ProtectFault.h"

ST_PROTECT_STATE stProtect;

static const ST_PROTECT_ATTR *ProtectAttrOf(E_PROTECT_FAULT eFault)
{
    return &stProtectCfg.pstAttr[eFault];
}

void ProtectInit(void)
{
    Uint16 i;

    for (i = 0U; i < PROTECT_WORD_NUM; i++)
    {
        stProtect.u16Status[i]  = 0U;
        stProtect.u16Pending[i] = 0U;
        stProtect.u16Mask[i]    = 0U;
    }

    for (i = 0U; i < PROTECT_PROTO_WORD_NUM; i++)
    {
        stProtect.u16Proto[i] = 0U;
    }

    for (i = 0U; i < (Uint16)PROTECT_FLT_NUM; i++)
    {
        stProtect.u16DebounceCnt[i] = 0U;
    }

    stProtect.u16ActionLevel = (Uint16)PROTECT_SEV_INFO;
    stProtect.u16TotalSign   = 0U;
    stProtect.u16MaskAgeMs   = PROTECT_MASK_HEARTBEAT_MS;
}

void ProtectTrigger(E_PROTECT_FAULT eFault)
{
    const ST_PROTECT_ATTR *pstAttr = ProtectAttrOf(eFault);
    stProtect.u16Pending[pstAttr->u16Word] |= pstAttr->u16Mask;
}

void ProtectRelease(E_PROTECT_FAULT eFault)
{
    const ST_PROTECT_ATTR *pstAttr = ProtectAttrOf(eFault);
    stProtect.u16Pending[pstAttr->u16Word] &= (Uint16)(~pstAttr->u16Mask);
}

Uint16 ProtectIsSet(E_PROTECT_FAULT eFault)
{
    const ST_PROTECT_ATTR *pstAttr = ProtectAttrOf(eFault);
    return ((stProtect.u16Status[pstAttr->u16Word] & pstAttr->u16Mask) != 0U) ? 1U : 0U;
}

Uint16 ProtectGetWord(Uint16 u16Word)
{
    return (u16Word < PROTECT_WORD_NUM) ? stProtect.u16Status[u16Word] : 0U;
}

Uint16 ProtectGetProto(Uint16 u16ProtoWord)
{
    return (u16ProtoWord < PROTECT_PROTO_WORD_NUM) ? stProtect.u16Proto[u16ProtoWord] : 0U;
}

Uint16 ProtectGetActionLevel(void)
{
    return stProtect.u16ActionLevel;
}

Uint16 ProtectGetTotalSign(void)
{
    return stProtect.u16TotalSign;
}

void ProtectSetMask(Uint16 u16Word, Uint16 u16Mask)
{
    if (u16Word < PROTECT_WORD_NUM)
    {
        stProtect.u16Mask[u16Word] = u16Mask;
        stProtect.u16MaskAgeMs = 0U;
    }
}

Uint16 ProtectGetMask(Uint16 u16Word)
{
    return (u16Word < PROTECT_WORD_NUM) ? stProtect.u16Mask[u16Word] : 0U;
}

void ProtectMaskKeepAlive(void)
{
    stProtect.u16MaskAgeMs = 0U;
}

void ProtectReset(void)
{
    Uint16 i;
    const ST_PROTECT_ATTR *pstAttr;

    for (i = 0U; i < (Uint16)PROTECT_FLT_NUM; i++)
    {
        pstAttr = &stProtectCfg.pstAttr[i];
        if (pstAttr->u16Recover != PROTECT_REC_AUTO)
        {
            stProtect.u16Status[pstAttr->u16Word] &= (Uint16)(~pstAttr->u16Mask);
            stProtect.u16DebounceCnt[i] = 0U;
        }
    }
}

/*
 * 去抖与锁存：
 *   判据有效 -> 累计时间，超过去抖门限后置记录位；
 *   判据消失 -> 计数清零，AUTO 位立即恢复记录，其余保持锁存等复位。
 */
static void ProtectUpdateRecord(void)
{
    Uint16 i;
    Uint16 u16Bit;
    const ST_PROTECT_ATTR *pstAttr;

    for (i = 0U; i < (Uint16)PROTECT_FLT_NUM; i++)
    {
        pstAttr = &stProtectCfg.pstAttr[i];
        u16Bit  = (Uint16)(stProtect.u16Pending[pstAttr->u16Word] & pstAttr->u16Mask);

        if (u16Bit != 0U)
        {
            if (stProtect.u16DebounceCnt[i] < pstAttr->u16DebounceMs)
            {
                stProtect.u16DebounceCnt[i] =
                    (Uint16)(stProtect.u16DebounceCnt[i] + PROTECT_TICK_MS);
            }

            if (stProtect.u16DebounceCnt[i] >= pstAttr->u16DebounceMs)
            {
                stProtect.u16Status[pstAttr->u16Word] |= pstAttr->u16Mask;
            }
        }
        else
        {
            stProtect.u16DebounceCnt[i] = 0U;
            if (pstAttr->u16Recover == PROTECT_REC_AUTO)
            {
                stProtect.u16Status[pstAttr->u16Word] &= (Uint16)(~pstAttr->u16Mask);
            }
        }
    }
}

/* 屏蔽位只压动作：动作等级与总标志都按 记录位 & ~屏蔽字 计算 */
static void ProtectUpdateAction(void)
{
    Uint16 i;
    Uint16 u16Active;
    Uint16 u16Level = (Uint16)PROTECT_SEV_INFO;
    Uint16 u16Sign  = 0U;
    const ST_PROTECT_ATTR *pstAttr;

    for (i = 0U; i < (Uint16)PROTECT_FLT_NUM; i++)
    {
        pstAttr  = &stProtectCfg.pstAttr[i];
        u16Active = (Uint16)(stProtect.u16Status[pstAttr->u16Word]
                             & (Uint16)(~stProtect.u16Mask[pstAttr->u16Word])
                             & pstAttr->u16Mask);
        if (u16Active != 0U)
        {
            if (pstAttr->u16Severity > u16Level)
            {
                u16Level = pstAttr->u16Severity;
            }
            if (pstAttr->u16Severity >= (Uint16)PROTECT_SEV_STOP)
            {
                u16Sign = 1U;
            }
        }
    }

    stProtect.u16ActionLevel = u16Level;
    stProtect.u16TotalSign   = u16Sign;
}

/* 协议字是内部故障的下游产物，一处生成，点表只读 */
static void ProtectUpdateProto(void)
{
    Uint16 i;
    Uint16 j;
    Uint16 u16Value;
    const ST_PROTECT_AGG_ITEM *pstItem;

    for (i = 0U; i < PROTECT_PROTO_WORD_NUM; i++)
    {
        stProtect.u16Proto[i] = 0U;
    }

    for (i = 0U; i < stProtectCfg.u16AggNum; i++)
    {
        pstItem  = &stProtectCfg.pstAgg[i];
        u16Value = 0U;

        for (j = 0U; j < pstItem->u16SrcNum; j++)
        {
            if ((stProtect.u16Status[pstItem->u16SrcWord[j]] & pstItem->u16SrcMask[j]) != 0U)
            {
                u16Value = 1U;
                break;
            }
        }

        if (u16Value != 0U)
        {
            stProtect.u16Proto[pstItem->u16ProtoWord] |= (Uint16)(1U << pstItem->u16ProtoBit);
        }
    }
}

void ProtectRun10ms(void)
{
    Uint16 i;

    ProtectUpdateRecord();
    ProtectUpdateAction();
    ProtectUpdateProto();

    /* 上位机改写屏蔽字或翻转心跳，都视为一次续期 */
    for (i = 0U; i < PROTECT_WORD_NUM; i++)
    {
        if (stProtect.u16Mask[i] != stProtect.u16MaskShadow[i])
        {
            stProtect.u16MaskShadow[i]  = stProtect.u16Mask[i];
            stProtect.u16MaskAgeMs      = 0U;
        }
    }

    if (stProtect.u16MaskKeepAlive != stProtect.u16MaskKeepAliveLast)
    {
        stProtect.u16MaskKeepAliveLast = stProtect.u16MaskKeepAlive;
        stProtect.u16MaskAgeMs         = 0U;
    }

    if (stProtect.u16MaskAgeMs < PROTECT_MASK_HEARTBEAT_MS)
    {
        stProtect.u16MaskAgeMs = (Uint16)(stProtect.u16MaskAgeMs + PROTECT_TICK_MS);
    }
    else
    {
        for (i = 0U; i < PROTECT_WORD_NUM; i++)
        {
            stProtect.u16Mask[i]       = 0U;
            stProtect.u16MaskShadow[i] = 0U;
        }
    }
}

Uint16 ProtectQuery(Uint16 u16DomainMask, Uint16 u16MinSeverity)
{
    Uint16 i;
    Uint16 u16Active;
    const ST_PROTECT_ATTR *pstAttr;

    for (i = 0U; i < (Uint16)PROTECT_FLT_NUM; i++)
    {
        pstAttr = &stProtectCfg.pstAttr[i];
        if ((u16DomainMask & (Uint16)(1U << pstAttr->u16Domain)) == 0U)
        {
            continue;
        }

        u16Active = (Uint16)(stProtect.u16Status[pstAttr->u16Word]
                             & (Uint16)(~stProtect.u16Mask[pstAttr->u16Word])
                             & pstAttr->u16Mask);
        if ((u16Active != 0U) && (pstAttr->u16Severity >= u16MinSeverity))
        {
            return 1U;
        }
    }

    return 0U;
}

/**
 * @file RingBuff.c
 * @author yetingheng (004182)
 * @brief 通用环形缓存实现
 * @version 0.1
 * @date 2026-10-23
 * @copyright Copyright (c) 2026
 */

#include "RingBuff.h"

Uint16 RingBuffInit(ST_RING_BUFF *pstRingBuff, Uint16 *pu16Buf, Uint16 u16Size)
{
    Uint16 u16Tmp;

    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    /* 容量必须是 2 的幂且至少 2：判据是 size & (size-1) == 0 */
    if ((u16Size < 2U) || ((u16Size & (Uint16)(u16Size - 1U)) != 0U))
    {
        return 0U;
    }

    u16Tmp = u16Size - 1U;

    pstRingBuff->pu16Buf = pu16Buf;
    pstRingBuff->u16Mask = u16Tmp;
    pstRingBuff->u16Head = 0U;
    pstRingBuff->u16Tail = 0U;
    pstRingBuff->u16OverflowCnt = 0U;

    return 1U;
}

Uint16 RingBuffClear(ST_RING_BUFF *pstRingBuff)
{
    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    pstRingBuff->u16Tail = pstRingBuff->u16Head;

    return 1U;
}

Uint16 RingBuffUsed(const ST_RING_BUFF *pstRingBuff)
{
    if ((pstRingBuff == (const ST_RING_BUFF *)0) || (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    /* 无符号相减再掩码：head 回绕时也正确 */
    return (Uint16)((pstRingBuff->u16Head - pstRingBuff->u16Tail) & pstRingBuff->u16Mask);
}

Uint16 RingBuffFree(const ST_RING_BUFF *pstRingBuff)
{
    if ((pstRingBuff == (const ST_RING_BUFF *)0) || (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    return (Uint16)(pstRingBuff->u16Mask - RingBuffUsed(pstRingBuff));
}

Uint16 RingBuffWrite(ST_RING_BUFF *pstRingBuff, const Uint16 *pu16Src, Uint16 u16Len)
{
    Uint16 u16Free;
    Uint16 u16Idx;
    Uint16 u16Head;

    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pu16Src == (const Uint16 *)0) ||
        (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    u16Free = RingBuffFree(pstRingBuff);
    if (u16Len > u16Free)
    {
        /* 记下被丢掉的元素数，方便上层判断是不是溢出导致的丢包 */
        pstRingBuff->u16OverflowCnt =
            (Uint16)(pstRingBuff->u16OverflowCnt + (u16Len - u16Free));
        u16Len = u16Free;
    }

    u16Head = pstRingBuff->u16Head;
    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        pstRingBuff->pu16Buf[u16Head] = pu16Src[u16Idx];
        u16Head = (Uint16)((u16Head + 1U) & pstRingBuff->u16Mask);
    }

    /* 全部搬完再一次性提交写指针，消费者不会看到"半个元素" */
    pstRingBuff->u16Head = u16Head;

    return u16Len;
}

Uint16 RingBuffWriteOverwrite(ST_RING_BUFF *pstRingBuff, const Uint16 *pu16Src, Uint16 u16Len)
{
    Uint16 u16Idx;
    Uint16 u16Head;
    Uint16 u16Drop;
    Uint16 u16Free;

    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pu16Src == (const Uint16 *)0) ||
        (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    /* 写入超过容量时只保留最后 u16Mask 个元素 */
    if (u16Len > pstRingBuff->u16Mask)
    {
        pu16Src = &pu16Src[u16Len - pstRingBuff->u16Mask];
        u16Len = pstRingBuff->u16Mask;
    }

    /* 空间不够就把最旧的挤掉，腾出 u16Len 个位置 */
    u16Free = RingBuffFree(pstRingBuff);
    if (u16Free < u16Len)
    {
        u16Drop = (Uint16)(u16Len - u16Free);
        if (u16Drop > RingBuffUsed(pstRingBuff))
        {
            u16Drop = RingBuffUsed(pstRingBuff);
        }
        (void)RingBuffDrop(pstRingBuff, u16Drop);
        pstRingBuff->u16OverflowCnt = (Uint16)(pstRingBuff->u16OverflowCnt + u16Drop);
    }

    u16Head = pstRingBuff->u16Head;
    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        pstRingBuff->pu16Buf[u16Head] = pu16Src[u16Idx];
        u16Head = (Uint16)((u16Head + 1U) & pstRingBuff->u16Mask);
    }

    pstRingBuff->u16Head = u16Head;

    return u16Len;
}

Uint16 RingBuffRead(ST_RING_BUFF *pstRingBuff, Uint16 *pu16Dst, Uint16 u16Len)
{
    Uint16 u16Used;
    Uint16 u16Idx;
    Uint16 u16Tail;

    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pu16Dst == (Uint16 *)0) ||
        (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    u16Used = RingBuffUsed(pstRingBuff);
    if (u16Len > u16Used)
    {
        u16Len = u16Used;
    }

    u16Tail = pstRingBuff->u16Tail;
    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        pu16Dst[u16Idx] = pstRingBuff->pu16Buf[u16Tail];
        u16Tail = (Uint16)((u16Tail + 1U) & pstRingBuff->u16Mask);
    }

    pstRingBuff->u16Tail = u16Tail;

    return u16Len;
}

Uint16 RingBuffDrop(ST_RING_BUFF *pstRingBuff, Uint16 u16Len)
{
    Uint16 u16Used;

    if ((pstRingBuff == (ST_RING_BUFF *)0) || (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    u16Used = RingBuffUsed(pstRingBuff);
    if (u16Len > u16Used)
    {
        u16Len = u16Used;
    }

    pstRingBuff->u16Tail =
        (Uint16)((pstRingBuff->u16Tail + u16Len) & pstRingBuff->u16Mask);

    return u16Len;
}

Uint16 RingBuffPeek(const ST_RING_BUFF *pstRingBuff, Uint16 u16Offset)
{
    Uint16 u16Pos;

    if ((pstRingBuff == (const ST_RING_BUFF *)0) || (pstRingBuff->pu16Buf == (Uint16 *)0))
    {
        return 0U;
    }

    if (u16Offset >= RingBuffUsed(pstRingBuff))
    {
        return 0U;
    }

    u16Pos = (Uint16)((pstRingBuff->u16Tail + u16Offset) & pstRingBuff->u16Mask);

    return pstRingBuff->pu16Buf[u16Pos];
}

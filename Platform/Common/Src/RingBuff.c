#include "RingBuff.h"

static uint16_t Valid(const ST_RING_BUFF *q)
{
    return (uint16_t)((q != 0) && (q->pu16Buf != 0) &&
                      (q->u16ItemWords != 0U) && (q->u16Mask != 0U));
}
static void Reject(ST_RING_BUFF *q, uint16_t n)
{
    uint16_t old = q->u16Rejected;
    q->u16Rejected = (n > (uint16_t)(UINT16_MAX - old)) ?
                    UINT16_MAX : (uint16_t)(old + n);
}
uint16_t RingBuffInitItems(ST_RING_BUFF *q, volatile uint16_t *storage,
                         uint32_t storageWords, uint16_t slots,
                         uint16_t itemWords)
{
    uint32_t words;
    if (q == 0) { return 0U; }
    q->pu16Buf = 0;
    q->u16Mask = 0U;
    q->u16ItemWords = 0U;
    q->u16Head = 0U;
    q->u16Tail = 0U;
    q->u16Rejected = 0U;
    words = (uint32_t)slots * (uint32_t)itemWords;
    if ((storage == 0) || (slots < 2U) ||
        ((slots & (uint16_t)(slots - 1U)) != 0U) ||
        (itemWords == 0U) || (words > UINT16_MAX) ||
        (storageWords < words)) { return 0U; }
    q->pu16Buf = storage;
    q->u16Mask = (uint16_t)(slots - 1U);
    q->u16ItemWords = itemWords;
    return 1U;
}
uint16_t RingBuffInit(ST_RING_BUFF *q, volatile uint16_t *storage, uint16_t slots)
{
    return RingBuffInitItems(q, storage, slots, slots, 1U);
}
uint16_t RingBuffCapacity(const ST_RING_BUFF *q)
{
    return Valid(q) ? q->u16Mask : 0U;
}
uint16_t RingBuffUsed(const ST_RING_BUFF *q)
{
    uint16_t head, tail;
    if (!Valid(q)) { return 0U; }
    head = q->u16Head;
    tail = q->u16Tail;
    return (uint16_t)((head - tail) & q->u16Mask);
}
uint16_t RingBuffFree(const ST_RING_BUFF *q)
{
    return Valid(q) ? (uint16_t)(q->u16Mask - RingBuffUsed(q)) : 0U;
}
/* Split at array boundary: no mask or modulo in the payload copy loop. */
static void CopyIn(ST_RING_BUFF *q, uint16_t head,
                   const uint16_t *src, uint16_t count)
{
    uint16_t first, words, i, pos;
    first = (uint16_t)(q->u16Mask + 1U - head);
    if (first > count) { first = count; }
    pos = (uint16_t)(head * q->u16ItemWords);
    words = (uint16_t)(first * q->u16ItemWords);
    for (i = 0U; i < words; ++i) { q->pu16Buf[pos + i] = src[i]; }
    src += words;
    words = (uint16_t)((count - first) * q->u16ItemWords);
    for (i = 0U; i < words; ++i) { q->pu16Buf[i] = src[i]; }
}
static uint16_t Write(ST_RING_BUFF *q, const uint16_t *src,
                      uint16_t count, uint16_t exact)
{
    uint16_t head, tail, freeItems, accepted;
    if (!Valid(q) || (src == 0) || (count == 0U)) { return 0U; }
    head = q->u16Head;
    tail = q->u16Tail;
    freeItems = (uint16_t)(q->u16Mask - ((head - tail) & q->u16Mask));
    accepted = count;
    if (accepted > freeItems) {
        accepted = exact ? 0U : freeItems;
        Reject(q, (uint16_t)(count - accepted));
    }
    if (accepted == 0U) { return 0U; }
    CopyIn(q, head, src, accepted);
    /* Volatile payload stores are complete before this volatile store. */
    q->u16Head = (uint16_t)((head + accepted) & q->u16Mask);
    return accepted;
}
uint16_t RingBuffWrite(ST_RING_BUFF *q, const uint16_t *src, uint16_t count)
{
    return Write(q, src, count, 0U);
}
uint16_t RingBuffWriteExact(ST_RING_BUFF *q, const uint16_t *src, uint16_t count)
{
    return Write(q, src, count, 1U);
}
uint16_t RingBuffRead(ST_RING_BUFF *q, uint16_t *dst, uint16_t count)
{
    uint16_t head, tail, used, first, words, i, pos;
    if (!Valid(q) || (dst == 0) || (count == 0U)) { return 0U; }
    tail = q->u16Tail;
    head = q->u16Head;
    used = (uint16_t)((head - tail) & q->u16Mask);
    if (count > used) { count = used; }
    if (count == 0U) { return 0U; }
    first = (uint16_t)(q->u16Mask + 1U - tail);
    if (first > count) { first = count; }
    pos = (uint16_t)(tail * q->u16ItemWords);
    words = (uint16_t)(first * q->u16ItemWords);
    for (i = 0U; i < words; ++i) { dst[i] = q->pu16Buf[pos + i]; }
    dst += words;
    words = (uint16_t)((count - first) * q->u16ItemWords);
    for (i = 0U; i < words; ++i) { dst[i] = q->pu16Buf[i]; }
    /* All volatile payload reads precede releasing the slots. */
    q->u16Tail = (uint16_t)((tail + count) & q->u16Mask);
    return count;
}
uint16_t RingBuffDrop(ST_RING_BUFF *q, uint16_t count)
{
    uint16_t tail, head, used;
    if (!Valid(q)) { return 0U; }
    tail = q->u16Tail;
    head = q->u16Head;
    used = (uint16_t)((head - tail) & q->u16Mask);
    if (count > used) { count = used; }
    q->u16Tail = (uint16_t)((tail + count) & q->u16Mask);
    return count;
}
uint16_t RingBuffPeek(const ST_RING_BUFF *q, uint16_t offset, uint16_t *dst)
{
    uint16_t tail, head, used, pos, i;
    if (!Valid(q) || (dst == 0)) { return 0U; }
    tail = q->u16Tail;
    head = q->u16Head;
    used = (uint16_t)((head - tail) & q->u16Mask);
    if (offset >= used) { return 0U; }
    pos = (uint16_t)(((tail + offset) & q->u16Mask) * q->u16ItemWords);
    for (i = 0U; i < q->u16ItemWords; ++i) { dst[i] = q->pu16Buf[pos + i]; }
    return 1U;
}
uint16_t RingBuffClear(ST_RING_BUFF *q)
{
    uint16_t head;
    if (!Valid(q)) { return 0U; }
    head = q->u16Head;
    q->u16Tail = head;
    return 1U;
}

#include "CanQueue.h"
uint16_t CanQueueInit(ST_CAN_QUEUE *q, volatile uint16_t *storage,
                      uint32_t storageWords, uint16_t slots)
{
    if (q == 0) { return 0U; }
    return RingBuffInitItems(&q->ring, storage, storageWords, slots, CAN_QUEUE_WORDS);
}
uint16_t CanQueuePush(ST_CAN_QUEUE *q, const ST_CAN_FRAME *frame)
{
    uint16_t words[CAN_QUEUE_WORDS];
    uint16_t i;
    if ((q == 0) || (frame == 0)) { return 0U; }
    if ((frame->dlc > 8U) || ((frame->flags & (uint16_t)~3U) != 0U) ||
        (frame->id > ((frame->flags & CAN_FRAME_EXT) ?
                      UINT32_C(0x1FFFFFFF) : UINT32_C(0x7FF)))) { return 0U; }
    words[0] = (uint16_t)(frame->id & UINT32_C(0xFFFF));
    words[1] = (uint16_t)(frame->id >> 16U);
    words[2] = frame->flags;
    words[3] = frame->dlc;
    for (i = 0U; i < 8U; ++i) {
        words[4U + i] = 0U;
        if ((i < frame->dlc) && ((frame->flags & CAN_FRAME_RTR) == 0U)) {
            if (frame->data[i] > 0xFFU) { return 0U; }
            words[4U + i] = frame->data[i];
        }
    }
    return RingBuffWriteExact(&q->ring, words, 1U);
}
static void Decode(ST_CAN_FRAME *frame, const uint16_t *words)
{
    uint16_t i;
    frame->id = (uint32_t)words[0] | ((uint32_t)words[1] << 16U);
    frame->flags = words[2];
    frame->dlc = words[3];
    for (i = 0U; i < 8U; ++i) { frame->data[i] = words[4U + i]; }
}
uint16_t CanQueuePop(ST_CAN_QUEUE *q, ST_CAN_FRAME *frame)
{
    uint16_t words[CAN_QUEUE_WORDS];
    if ((q == 0) || (frame == 0)) { return 0U; }
    if (RingBuffRead(&q->ring, words, 1U) != 1U) { return 0U; }
    Decode(frame, words);
    return 1U;
}
uint16_t CanQueuePeek(const ST_CAN_QUEUE *q, ST_CAN_FRAME *frame)
{
    uint16_t words[CAN_QUEUE_WORDS];
    if ((q == 0) || (frame == 0)) { return 0U; }
    if (RingBuffPeek(&q->ring, 0U, words) != 1U) { return 0U; }
    Decode(frame, words);
    return 1U;
}
uint16_t CanQueueDrop(ST_CAN_QUEUE *q)
{
    return (q != 0) ? RingBuffDrop(&q->ring, 1U) : 0U;
}

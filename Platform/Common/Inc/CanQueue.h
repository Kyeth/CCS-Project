#ifndef CAN_QUEUE_H
#define CAN_QUEUE_H
#include "RingBuff.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CAN_QUEUE_WORDS 12U
#define CAN_FRAME_EXT  1U
#define CAN_FRAME_RTR  2U
/* Classical CAN only, not CAN FD. Each data[] entry holds ONE wire octet.
 * RTR: dlc is requested length, data[] is ignored and decoded as zero.
 */
typedef struct {
    uint32_t id;
    uint16_t flags;
    uint16_t dlc;
    uint16_t data[8];
} ST_CAN_FRAME;
typedef struct { ST_RING_BUFF ring; } ST_CAN_QUEUE;
uint16_t CanQueueInit(ST_CAN_QUEUE *q, volatile uint16_t *storage,
                      uint32_t storageWords, uint16_t slots);
uint16_t CanQueuePush(ST_CAN_QUEUE *q, const ST_CAN_FRAME *frame);
uint16_t CanQueuePop(ST_CAN_QUEUE *q, ST_CAN_FRAME *frame);
uint16_t CanQueuePeek(const ST_CAN_QUEUE *q, ST_CAN_FRAME *frame);
uint16_t CanQueueDrop(ST_CAN_QUEUE *q);
#ifdef __cplusplus
}
#endif
#endif

#ifndef RING_BUFF_H
#define RING_BUFF_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* TI C28x single CPU, single producer/single consumer only.
 * Storage AND indexes use volatile accesses: payload precedes publication.
 * No DMA, CLA, multicore or host threads. See README for ownership rules.
 * All lengths/counts are ITEMS; one item contains u16ItemWords 16-bit words.
 * A slots-sized backing array holds slots-1 usable items. No overwrite API.
 */
typedef struct {
    volatile uint16_t *pu16Buf;
    uint16_t u16Mask;
    uint16_t u16ItemWords;
    volatile uint16_t u16Head; /* producer only */
    volatile uint16_t u16Tail; /* consumer only */
    volatile uint16_t u16Rejected; /* producer only, saturates at 65535 */
} ST_RING_BUFF;

/* Initialization/reinitialization requires both endpoints stopped.
 * Actual storage must contain at least storageWords words.
 * slots: power of 2 in [2,32768]; slots*itemWords <= 65535.
 * On failure a non-null q becomes invalid; never init a running queue.
 */
uint16_t RingBuffInitItems(ST_RING_BUFF *q, volatile uint16_t *storage,
                         uint32_t storageWords, uint16_t slots,
                         uint16_t itemWords);
/* Compatibility initializer for one-word items; storage has slots words. */
uint16_t RingBuffInit(ST_RING_BUFF *q, volatile uint16_t *storage, uint16_t slots);
uint16_t RingBuffCapacity(const ST_RING_BUFF *q);
/* Used: consumer-side availability; Free: producer-side space.
 * A third-party observer gets an approximate value, not a coherent snapshot.
 */
uint16_t RingBuffUsed(const ST_RING_BUFF *q);
uint16_t RingBuffFree(const ST_RING_BUFF *q);
/* Producer only. Src must not alias queue storage; count is in items. */
uint16_t RingBuffWrite(ST_RING_BUFF *q, const uint16_t *src, uint16_t count);
/* All items or none; returns accepted item count (0 on insufficient space).
 * A failure counts all requested items as rejected, even if caller retries.
 */
uint16_t RingBuffWriteExact(ST_RING_BUFF *q, const uint16_t *src, uint16_t count);
/* Consumer only. Dst must not alias queue storage. */
uint16_t RingBuffRead(ST_RING_BUFF *q, uint16_t *dst, uint16_t count);
uint16_t RingBuffDrop(ST_RING_BUFF *q, uint16_t count);
/* Copy ONE item. 1=success, 0=invalid/absent; dst unchanged on failure. */
uint16_t RingBuffPeek(const ST_RING_BUFF *q, uint16_t offset, uint16_t *dst);
/* Consumer only: discard up to a head snapshot, not future arrivals.
 * Exclusive access with all other consumer calls is required.
 */
uint16_t RingBuffClear(ST_RING_BUFF *q);
#ifdef __cplusplus
}
#endif
#endif

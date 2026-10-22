/**
 * @file RingBuff.h
 * @author yetingheng (004182)
 * @brief 通用环形缓存：单生产者/单消费者，容量必须为 2 的幂
 * @version 0.1
 * @date 2026-10-23
 * @copyright Copyright (c) 2026
 *
 * @note 为什么容量必须是 2 的幂：
 *       C28x 没有硬件除法指令，`% u16Size` 会被编译成库函数调用（几十个周期）。
 *       容量取 2 的幂之后用 `& u16Mask` 代替取模，只有一条 AND 指令。
 *
 * @note 为什么索引要 volatile、但不需要关中断：
 *       单生产者（中断写 Head）与单消费者（任务读 Tail）各改一个索引，
 *       而 C28x 对 16 位对齐变量的读写在硬件上是原子的（一条 MOV），
 *       所以只要保证"一个索引只有一个写者"，就不需要 DINT/EDINT 临界区。
 *       头/尾指针不作为"满/空"的唯一判据（用已用数量算），避免歧义。
 *
 * @note 溢出策略分两种，由调用者选择：
 *       RingBuffWrite()          写不下就截断，丢新数据，返回实写数；
 *       RingBuffWriteOverwrite() 满了覆盖最旧数据，用于"只要最新的"场景。
 */
#ifndef RING_BUFF_H
#define RING_BUFF_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 环形缓存控制结构 */
typedef struct
{
    Uint16 *pu16Buf;                /* 数据区，由调用者提供 */
    Uint16  u16Mask;                /* 容量 - 1，容量必须是 2 的幂 */
    volatile Uint16 u16Head;        /* 写指针，只有生产者写 */
    volatile Uint16 u16Tail;        /* 读指针，只有消费者写 */
    volatile Uint16 u16OverflowCnt; /* 累计丢弃的元素数，便于定位溢出 */
} ST_RING_BUFF;

/* 初始化：u16Size 必须是 2 的幂，返回 1 成功、0 失败 */
Uint16 RingBuffInit(ST_RING_BUFF *pstRingBuff, Uint16 *pu16Buf, Uint16 u16Size);

/* 清空（不改变容量），返回 1 表示成功 */
Uint16 RingBuffClear(ST_RING_BUFF *pstRingBuff);

/* 已用元素数 / 剩余空间（元素数） */
Uint16 RingBuffUsed(const ST_RING_BUFF *pstRingBuff);
Uint16 RingBuffFree(const ST_RING_BUFF *pstRingBuff);

/* 写入：满了截断，返回实际写入的元素数 */
Uint16 RingBuffWrite(ST_RING_BUFF *pstRingBuff, const Uint16 *pu16Src, Uint16 u16Len);

/* 写入：满了覆盖最旧的，返回实际写入的元素数（等于 u16Len） */
Uint16 RingBuffWriteOverwrite(ST_RING_BUFF *pstRingBuff, const Uint16 *pu16Src, Uint16 u16Len);

/* 读出：返回实际读出的元素数 */
Uint16 RingBuffRead(ST_RING_BUFF *pstRingBuff, Uint16 *pu16Dst, Uint16 u16Len);

/* 丢弃头部 u16Len 个元素，返回实际丢弃数 */
Uint16 RingBuffDrop(ST_RING_BUFF *pstRingBuff, Uint16 u16Len);

/* 只看不取：读偏移 u16Offset 处的元素（0 = 队头），越界返回 0 */
Uint16 RingBuffPeek(const ST_RING_BUFF *pstRingBuff, Uint16 u16Offset);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFF_H */

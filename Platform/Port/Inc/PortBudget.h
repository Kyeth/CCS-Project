/**
 * @file PortBudget.h
 * @brief 端口服务轮次预算：由 PortTask 开窗，服务层每次驱动调用前查询
 * @version 0.1
 * @date 2026-11-05
 *
 * 为什么需要它：只在 PortTask 的单元之间查预算不够——一次 CanPortServiceOne()
 * 内部可以连续发起多次接收和发送，第一次调用就超预算时后面仍会继续。
 * 因此把窗口开放给服务层：每次驱动调用前查 PortBudgetHasLeft()，
 * 已经开始的调用允许完成，完成后超预算就停止发起下一次。
 *
 * 未开窗（例如 100 us 心跳里的 SCI 接收）时 HasLeft() 恒为 1，表示不受本预算约束。
 */

#ifndef PORT_BUDGET_H
#define PORT_BUDGET_H

#include "F2837xS_device.h"

/** 开窗：u32StartTick 为本轮起点（递减计数器读数） */
void PortBudgetBegin(Uint32 u32StartTick);

/** 关窗：恢复"不受预算约束"状态 */
void PortBudgetEnd(void);

/** 1 = 窗口未开或仍在预算内；0 = 已超预算，或时间源读取失败 */
Uint16 PortBudgetHasLeft(void);

/** 1 = 当前有活动窗口 */
Uint16 PortBudgetActive(void);
Uint16 PortBudgetFaulted(void);   /* 本轮内是否已发生过时间读取失败（锁存到轮末） */

#endif /* PORT_BUDGET_H */

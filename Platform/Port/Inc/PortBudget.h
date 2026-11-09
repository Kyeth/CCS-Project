/**
 * @file PortBudget.h
 * @brief 端口服务预算：一次前台服务机会只服务一个单元，单元窗口 100 us
 * @version 0.3
 * @date 2026-11-09
 *
 * 为什么需要这个窗口：一次 SciPortServiceTx() 或 CanPortServiceOne() 内部会连
 * 续发起多次驱动调用（逐字节提交、逐帧取队头）。只在单元之间查时间不够——
 * 第一次调用就超预算时后面仍会继续。因此窗口开放到服务层：每次驱动调用前查
 * PortBudgetHasLeft()，已经开始的调用允许完成，完成后超预算就不再发起下一次。
 *
 * 语义边界：
 *   - 预算只是"不再发起新的驱动调用"的门限，不是必须执行满的时长；
 *     空闲、硬件忙、配额用完都立即返回；
 *   - 时间源读取失败即锁存到本次调用结束：本单元后续不再发起驱动调用，
 *     切到下一个单元（下一次调用）才重新开窗，且开窗不清旧数据；
 *   - 预算是前台预算，不靠关中断保证；控制 ISR 随时可以抢占；
 *   - 未开窗（100 us 心跳里的 SCI 接收）时 HasLeft() 恒为 1，不受本预算约束。
 */

#ifndef PORT_BUDGET_H
#define PORT_BUDGET_H

#include "F2837xS_device.h"

/**
 * @brief 开一个单元窗口并清故障锁存
 * @param u32StartTick 本单元起点（递减计数器读数），调用方已经取过时标
 */
void PortBudgetBegin(Uint32 u32StartTick);

/** 关窗：恢复"不受预算约束"状态 */
void PortBudgetEnd(void);

/** 1 = 未开窗或单元还有余量；0 = 单元预算到期或时间源读取失败 */
Uint16 PortBudgetHasLeft(void);

/** 1 = 当前有活动窗口 */
Uint16 PortBudgetActive(void);

/** 1 = 本次调用内已发生过时间读取失败 */
Uint16 PortBudgetFaulted(void);

/** 当前单元的起点时标（未开窗时返回 0）；供诊断统计服务间隔 */
Uint32 PortBudgetUnitStartTick(void);

#endif /* PORT_BUDGET_H */

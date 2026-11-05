/**
 * @file PortTask.h
 * @brief 端口层汇总服务：公平轮转 + 轮次预算 + 时间源失败停止
 * @version 0.1
 * @date 2026-11-05
 *
 * 分工（对应步骤 05 文档 §3.3）：
 *   - SCI 接收：100 us 心跳里逐端口有界搬运（SciPortService100us）
 *   - SCI 发送、CAN 接收/发送：本文件的 PortTask，1 ms 前台调用
 *
 * PortTask 只决定"这一轮服务哪几个单元、什么时候提前收手"，
 * 具体收发逻辑仍留在 SciPort / CanPort。
 */

#ifndef PORTTASK_H
#define PORTTASK_H

#include "F2837xS_device.h"
#include "SciPort.h"
#include "CanPort.h"
#include "PortBudget.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 单元数：3 路 SCI 发送 + 2 路 CAN（每路含接收与发送） */
#define PORT_TASK_UNIT_NUM ((Uint16)(SCI_PORT_NUM + CAN_PORT_NUM))

/** 单次 PortTask 的轮次预算（微秒）：超出即停止发起后续单元 */
#define PORT_TASK_BUDGET_US (20UL)

/** 时间源读取回调：成功返回 1 并写 *pu32Tick，失败返回 0 */
typedef Uint16 (*PORT_TASK_TICK_FN)(Uint32 *pu32Tick);

/** 初始化：清游标与统计，时间源恢复为默认（Hw_TimeBase） */
void PortTaskInit(void);

/**
 * @brief 设置时间源与计数频率（默认实现用 Hw_TimeBase + 200 MHz）
 * @param pfTryGetTick 读取回调；传 0 恢复默认实现
 * @param u32SysclkHz  系统时钟，用于把 tick 差换算成微秒
 */
void PortTaskSetTimeSource(PORT_TASK_TICK_FN pfTryGetTick, Uint32 u32SysclkHz);

/** 1 ms 前台调用：按游标轮转服务 PORT_TASK_UNIT_NUM 个单元 */
void PortTask(void);

/** 单元号到服务入口参数的映射（单元号只是轮转槽位，不是硬件 ID）：
 *  0..SCI_PORT_NUM-1  -> 硬件 SCI ID（A/C/D）
 *  SCI_PORT_NUM..     -> 硬件 CAN ID（A/B）
 * 越界返回 0xFFFF。 */
Uint16 PortTaskUnitPortId(Uint16 u16Unit);

/* ---------------- 观测 ---------------- */

/** 上一轮第一个被服务的单元序号（0..UNIT_NUM-1）；未服务时为 0xFFFF */
Uint16 PortTaskLastFirstUnit(void);

/** 上一轮实际服务的单元数（0..UNIT_NUM） */
Uint16 PortTaskLastUnits(void);

/** 因轮次预算用尽而提前收手的次数 */
Uint32 PortTaskBudgetTruncations(void);

/** 因时间源读取失败而放弃本轮的次数 */
Uint32 PortTaskTimeSourceFailures(void);

#ifdef __cplusplus
}
#endif

#endif /* PORTTASK_H */

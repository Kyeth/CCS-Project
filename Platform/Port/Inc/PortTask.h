/**
 * @file PortTask.h
 * @brief 端口层前台服务：每 100 us 服务机会只服务一个单元，五单元轮转
 * @version 0.2
 * @date 2026-11-09
 *
 * 分工（对应步骤 05C 的实施裁定）：
 *   - SCI 接收：100 us 心跳里逐端口有界搬运（SciPortService100us），每拍一路
 *   - 前台单元：SCI-A TX、SCI-C TX、SCI-D TX，每个 100 us 服务机会只服务其中
 *     一路，每路标称 300 us
 *
 * @note BOOT 适配：本工程不包含 CAN（CanPort/Hw_CanDriver/CanQueue 都属于整机
 *       控制与通信功能，最小 BOOT 不需要），因此前台单元只保留 3 路 SCI 发送。
 *       这是与 APP 工程唯一的结构差异，其余逻辑逐字一致。
 *
 * 通知与执行分离：
 *   - 心跳只发"有一次服务机会"的通知（PortTaskRequest），重复通知合并，
 *     不补跑历史节拍；
 *   - 主循环取走通知后服务一个单元（PortTaskService）；
 *   - 时间源失败时本次调用不服务，故障锁存到本次调用结束。
 */

#ifndef PORTTASK_H
#define PORTTASK_H

#include "F2837xS_device.h"
#include "SciPort.h"
#include "PortBudget.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 单元数：3 路 SCI 发送（BOOT 不含 CAN） */
#define PORT_TASK_UNIT_NUM ((Uint16)(SCI_PORT_NUM))

/** 单个服务单元的预算（微秒）：一个单元从开窗起最多占用多久 */
#define PORT_TASK_UNIT_BUDGET_US (100UL)

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

/** 服务一个单元：按游标取单元、开 100 us 窗口、服务完把游标推进一格 */
void PortTask(void);

/**
 * @brief 主循环入口：取走一次服务机会并服务一个单元
 * @return 实际服务的单元数（0 或 1）；没有待服务通知时立即返回 0
 */
Uint16 PortTaskService(void);

/** 心跳入口：登记一次服务机会；已有未消费的通知时合并计数，不排队补跑 */
void PortTaskRequest(void);

/** 单元号到服务入口参数的映射（单元号只是轮转槽位，不是硬件 ID）：
 *  0..SCI_PORT_NUM-1  -> 硬件 SCI ID（A/C/D）
 * 越界返回 0xFFFF。 */
Uint16 PortTaskUnitPortId(Uint16 u16Unit);

/** 心跳内的端口服务入口（main.c 的 100 us 中断调用）。
 *  bPortInitError != 0 时完全不调用端口服务：初始化失败的端口不应被搬运，
 *  启停判定只在一处，主机桩也能直接验证门禁生效。 */
void PortTaskServiceFromHeartbeat(Uint16 bPortInitError);

/** 使能前台服务入口（替代原 1 ms 的 Task_Start 注册）。
 *  端口层初始化失败时 main.c 不会调用它。总是返回 1。 */
Uint16 PortTaskRegister(void);

/* ---------------- 观测 ---------------- */

/** 上一次服务机会实际服务的单元号（0..UNIT_NUM-1）；未服务时为 0xFFFF */
Uint16 PortTaskLastUnit(void);

/** 兼容旧观测名：等同 PortTaskLastUnit() */
Uint16 PortTaskLastFirstUnit(void);

/** 上一次服务机会实际服务的单元数（0 或 1） */
Uint16 PortTaskLastUnits(void);

/** 因单元预算用尽而提前收手的次数（累计） */
Uint32 PortTaskBudgetTruncations(void);

/** 因时间源读取失败而放弃本次服务的次数（累计） */
Uint32 PortTaskTimeSourceFailures(void);

/** 实际执行过的服务机会数（累计） */
Uint32 PortTaskServiceCalls(void);

/** 被合并掉的重复通知数（累计） */
Uint32 PortTaskRequestsMerged(void);

/** 某个单元相邻两次服务之间的最大间隔（tick）；尚未形成间隔时返回 0 */
Uint32 PortTaskUnitMaxGapTicks(Uint16 u16Unit);

#ifdef __cplusplus
}
#endif

#endif /* PORTTASK_H */

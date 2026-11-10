/**
 * @file Hw_TimeBase.h
 * @author yetingheng (004182)
 * @brief F2837xS 自由运行时间基准（CPU Timer0）
 * @version 0.1
 * @date 2026-11-03
 *
 * @copyright Copyright (c) 2026
 *
 * 说明：CPU Timer0 被独占用作"经过时间测量"的硬件时基，不产生中断、不参与调度。
 *      Timer1 仍负责 100 us 心跳（见步骤 03），两者互不影响。
 *      计数器为 32 位递减、200 MHz 下约 21.47 s 回绕一次，所有差值按无符号减法处理。
 */

#ifndef HW_TIME_BASE_H
#define HW_TIME_BASE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动 Timer0 自由运行
 *
 * 必须在系统时钟配置完成之后、任何按时间等待的模块初始化之前调用，且只调用一次。
 * 内部顺序：显式使能 PCLKCR0.CPUTIMER0 -> 停表 -> 配预分频/周期 -> 装载 -> 放行
 *          -> 回读配置 -> 自旋确认计数确实在递减。
 *
 * @param u32SysclkHz 系统时钟频率（Hz），用于换算微秒
 * @return 1 = 启动成功并就绪，0 = 失败（时钟位未使能、参数非法、配置未生效或计数未运行）
 */
Uint16 Hw_TimeBaseInit(Uint32 u32SysclkHz);

/**
 * @brief 读取当前计数
 *
 * @return TIMH:TIM 的原始递减计数（不做取反）；读取校验失败时返回 0
 *
 * 说明：32 位计数由两个 16 位寄存器拼成，读取期间可能跨低字回绕，
 *      内部用"高字-低字-高字"三读校验。连续校验失败时不返回拼接值，
 *      而是锁存故障并使 Hw_TimeBaseIsReady() 变为 0，调用方必须以就绪标志为准。
 */
Uint32 Hw_TimeBaseTicks(void);

/**
 * @brief 计算从 u32Start 到现在的经过时间
 *
 * @param u32Start 之前由 Hw_TimeBaseTicks() 取得的起点
 * @return (u32Start - now)，单位 SYSCLK 周期；递减计数下可安全跨越一次回绕
 *         读取失败时返回 0 并锁存故障，调用方必须查 Hw_TimeBaseIsReady()
 */
Uint32 Hw_TimeBaseElapsed(Uint32 u32Start);

/**
 * @brief 查询时间基准是否就绪
 *
 * @return 1 = 已成功启动，0 = 未启动或启动失败
 */
Uint16 Hw_TimeBaseIsReady(void);

/**
 * @brief 微秒换算为 SYSCLK 周期数
 *
 * @param u32Us 微秒数
 * @return 周期数；未就绪或系统时钟未知时返回 0
 */
Uint32 Hw_TimeBaseUsToTicks(Uint32 u32Us);

#ifdef __cplusplus
}
#endif

#endif /* HW_TIME_BASE_H */

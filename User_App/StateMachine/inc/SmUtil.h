/**
 * @file SmUtil.h
 * @author yetingheng (004182)
 * @brief 状态机公共时基与延时工具
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note
 *   - 时基由 10 kHz 任务（ProtectTask 尾部）调用 SmTick() 推进，1 拍 = 100us，
 *     与 EP5K 的 DELAY_*_100US 常量同量纲，便于直接搬运对方的时序数字。
 *   - 所有 API 都是 O(1)，可在中断里调用。
 */

#ifndef SM_UTIL_H
#define SM_UTIL_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 时基
 ******************************************************************************/
/** 10 kHz 一拍 = 100us，1ms = 10 拍 */
#define SM_TICK_PER_MS          (10UL)

/** 常用延时（单位：100us 拍数），与 EP5K InvStateMachine.h 对齐 */
#define SM_DELAY_10S            (100000UL)
#define SM_DELAY_5S             (50000UL)
#define SM_DELAY_3S             (30000UL)
#define SM_DELAY_2S             (20000UL)
#define SM_DELAY_500MS          (5000UL)
#define SM_DELAY_100MS          (1000UL)

/**
 * @brief 时基推进，必须由 10 kHz 任务调用（1 拍 = 100us）
 */
extern void SmTick(void);

/**
 * @brief 取当前 100us 拍数（32 位，约 119 小时回绕一次）
 */
extern Uint32 SmGetTick100us(void);

/**
 * @brief 取当前毫秒数
 */
extern Uint32 SmGetMs(void);

/******************************************************************************
 * 延时器
 ******************************************************************************/
typedef struct
{
    Uint32 u32Start;        /* 起始拍数 */
} ST_SM_DELAY;

/**
 * @brief 重新开始计时
 */
extern void SmDelayRestart(ST_SM_DELAY *pstDelay);

/**
 * @brief 判断是否已到时间（单位：100us 拍）。到时返回 1，否则 0
 */
extern Uint16 SmDelayElapsed(const ST_SM_DELAY *pstDelay, Uint32 u32Ticks);

/**
 * @brief 判断是否已到时间（单位：毫秒）
 */
extern Uint16 SmDelayElapsedMs(const ST_SM_DELAY *pstDelay, Uint32 u32Ms);

/******************************************************************************
 * 电平消抖
 ******************************************************************************/
typedef struct
{
    Uint16 u16State;        /* 消抖后的稳定电平 */
    Uint16 u16Cnt;          /* 与稳定电平不一致的连续拍数 */
    Uint16 u16Thr;          /* 判定门限（拍数） */
} ST_SM_DEBOUNCE;

/**
 * @brief 消抖初始化
 * @param u16Init 初始稳定电平
 * @param u16Thr  门限拍数（100us/拍）
 */
extern void SmDebounceInit(ST_SM_DEBOUNCE *pstDeb, Uint16 u16Init, Uint16 u16Thr);

/**
 * @brief 消抖处理，每个时基拍调用一次，返回消抖后的稳定电平
 */
extern Uint16 SmDebounceRun(ST_SM_DEBOUNCE *pstDeb, Uint16 u16Raw);

/**
 * @brief 电平是否发生过变化（上升沿/下降沿），用上一次返回值比较
 */
extern Uint16 SmRising(Uint16 u16Now, Uint16 u16Last);

#ifdef __cplusplus
}
#endif

#endif /* SM_UTIL_H */

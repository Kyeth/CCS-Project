/**
 * @file Hw_CpuTimerDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS CPU-Timer驱动头文件
 * @version 0.1
 * @date 2026-10-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef HW_CPUTIMER_DRIVER_H
#define HW_CPUTIMER_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CPU定时器编号
 *        0号定时器在TI的库函数中被占用，用户程序建议使用1/2号
 */
typedef enum
{
    HW_CPUTIMER_ID0 = 0U,
    HW_CPUTIMER_ID1,
    HW_CPUTIMER_ID2,
    HW_CPUTIMER_ID_MAX,
    HW_CPUTIMER_ID_NULL = 0xFFFFU
} E_HW_CPUTIMER_ID;

/**
 * @brief 系统定时器对象
 *        硬件侧只负责"溢出中断"，软件侧用分频系数再折算出一层更慢的心跳
 */
typedef struct
{
    Uint16 u16TimerId;      /* 定时器编号，见 E_HW_CPUTIMER_ID */
    Uint16 u16Prescaler;    /* 硬件预分频 TDDR(0~255)，实际分频系数 = TDDR + 1 */
    Uint16 bEnableInt;      /* 1: 使能定时器中断；0: 只计数不中断 */
    Uint16 bEmuStop;        /* 1: 仿真器挂起时停止计数；0: 自由运行 */
    Uint32 u32Period;       /* 硬件周期计数 = SYSCLK / 目标中断频率，写入 PRD */
    Uint32 u32Tick;         /* 软件心跳计数，由 Hw_CpuTimerIsrTick 累加 */
    Uint16 u16TickDivCnt;   /* 软件心跳分频计数器，中断中自增 */
    Uint16 u16TickDivCoef;  /* 软件心跳分频系数，每 N 次硬件中断算 1 个心跳 */
} ST_SYS_TIMER;

/**
 * @brief 初始化CPU定时器
 *        内部依次完成：停止定时器 -> 配置预分频 -> 配置周期 -> 配置仿真挂起方式
 *        -> 清中断标志并重载计数器 -> 配置中断使能 -> 清零软件心跳
 *        注意：初始化完成后定时器仍处于停止状态，需要调用 Hw_CpuTimerStart 启动
 *
 * @param pstSysTimer 定时器对象指针
 */
extern void Hw_CpuTimerInit(ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 启动CPU定时器（从当前计数器值继续）
 *
 * @param pstSysTimer 定时器对象指针
 */
extern void Hw_CpuTimerStart(ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 停止CPU定时器（计数器值保持不变）
 *
 * @param pstSysTimer 定时器对象指针
 */
extern void Hw_CpuTimerStop(ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 修改定时器周期
 *        运行中也安全：内部会先停止定时器，写入PRD后再恢复原来的运行状态
 *
 * @param pstSysTimer 定时器对象指针
 * @param u32Period   新的周期计数值 = SYSCLK / 目标中断频率
 */
extern void Hw_CpuTimerSetPeriod(ST_SYS_TIMER *pstSysTimer, Uint32 u32Period);

/**
 * @brief 重新装载计数器（计数器回到PRD并重新开始递减）
 *
 * @param pstSysTimer 定时器对象指针
 */
extern void Hw_CpuTimerReload(ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 读取计数器当前值
 *
 * @param pstSysTimer 定时器对象指针
 * @return 计数器当前值，对象非法时返回0
 */
extern Uint32 Hw_CpuTimerGetCount(const ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 清除定时器中断标志
 *        定时器中断服务函数在读取中断源后需调用此函数，否则中断会重复进入
 *
 * @param pstSysTimer 定时器对象指针
 */
extern void Hw_CpuTimerClearIntFlag(ST_SYS_TIMER *pstSysTimer);

/**
 * @brief 定时器中断节拍处理
 *        放在定时器中断服务函数中调用，内部按 u16TickDivCoef 做软件分频
 *
 * @param pstSysTimer 定时器对象指针
 * @return 1: 累计满一个软件心跳；0: 未满
 */
extern Uint16 Hw_CpuTimerIsrTick(ST_SYS_TIMER *pstSysTimer);

#ifdef __cplusplus
}
#endif

#endif /* HW_CPUTIMER_DRIVER_H */

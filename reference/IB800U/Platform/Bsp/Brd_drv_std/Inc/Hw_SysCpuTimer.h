/**
 * @file Hw_SysCpuTimer.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_SYSCPUTIMER_H
#define __HW_SYSCPUTIMER_H

#include "F28003x_device.h"


#ifdef __cplusplus
    extern "C" {
#endif

typedef struct
{
    Uint32 TimerBaseReg;        // 定时器寄存器基地址
    Uint32 TimerTick;           // 定时器心跳计数
    Uint16 TimerPeriod;         // 定时器周期
    Uint16 TimerDivCntPtd :8;   // 定时器心跳计数预分频计数
    Uint16 TimerFreqDivCoef :8; // 定时器心跳预分频系数
}ST_SYS_TIMER;



/**
 * @brief 初始化单个Timer
 * 
 * @param pstSysTimer ST_SYS_TIMER结构体对象的地址
 */
extern void Hw_CpuTimerInit(ST_SYS_TIMER *pstSysTimer);



/**
 * @brief 启动定时器
 * 
 * @param pstSysTimer :ST_SYS_TIMER结构体对象的地址
 */
extern void Hw_CpuTimerStart(ST_SYS_TIMER *pstSysTimer);



/**
 * @brief 停止定时器
 * 
 * @param pstSysTimer :ST_SYS_TIMER结构体对象的地址
 */
extern void Hw_CpuTimerStop(ST_SYS_TIMER *pstSysTimer);




extern volatile ST_SYS_TIMER stSysCpuTimer1;

extern volatile ST_SYS_TIMER stSysCpuTimer2;

#ifdef __cplusplus
    }
#endif

#endif

/**
 * @file Hw_SysCpuTimer.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_SysCpuTimer.h"
//#include "hw_memmap.h"
#include "Device.h"
#include "cputimer.h"

#ifdef __cplusplus
    extern "C" {
#endif


volatile ST_SYS_TIMER stSysCpuTimer1 =
{
 .TimerBaseReg = CPUTIMER1_BASE,
 .TimerPeriod = DEVICE_SYSCLK_FREQ / 20000,     // 除1000 = 1ms  除10000 = 0.1ms
 .TimerTick = 0,
 .TimerDivCntPtd = 0,
 .TimerFreqDivCoef = 20000/1000,                // 心跳计数周期配置成1ms，配套修改预分频系数
};


volatile ST_SYS_TIMER stSysCpuTimer2 =
{
 .TimerBaseReg = CPUTIMER2_BASE,
 .TimerPeriod = DEVICE_SYSCLK_FREQ / 10000,     // 除1000 = 1ms  除10000 = 0.1ms 除40000 = 25us
 .TimerTick = 0,
 .TimerDivCntPtd = 0,
 .TimerFreqDivCoef = 10000/1000,                // 心跳计数周期配置成1ms，配套修改预分频系数
};

/***暂时没有使用到
ST_SYS_TIMER stSysCpuTimer1 =
{
 .TimerBaseReg = CPUTIMER1_BASE,
 .TimerTick = 0,
};

ST_SYS_TIMER stSysCpuTimer2 =
{
 .TimerBaseReg = CPUTIMER2_BASE,
 .TimerTick = 0,
};
*/



/**
 * @brief 初始化单个Timer
 * 
 * @param pstSysTimer :ST_SYS_TIMER结构体对象的地址
 */
void Hw_CpuTimerInit(ST_SYS_TIMER *pstSysTimer)
{
    CPUTimer_stopTimer(pstSysTimer->TimerBaseReg);

    CPUTimer_setPeriod(pstSysTimer->TimerBaseReg, pstSysTimer->TimerPeriod);

    CPUTimer_setPreScaler(pstSysTimer->TimerBaseReg, 0);

    CPUTimer_setEmulationMode(pstSysTimer->TimerBaseReg,
                                  CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);

    CPUTimer_reloadTimerCounter(pstSysTimer->TimerBaseReg);

    CPUTimer_enableInterrupt(pstSysTimer->TimerBaseReg);
}



/**
 * @brief 启动定时器
 * 
 * @param pstSysTimer :ST_SYS_TIMER结构体对象的地址
 */
void Hw_CpuTimerStart(ST_SYS_TIMER *pstSysTimer)
{
    CPUTimer_resumeTimer(pstSysTimer->TimerBaseReg);
}



/**
 * @brief 停止定时器
 * 
 * @param pstSysTimer :ST_SYS_TIMER结构体对象的地址
 */
void Hw_CpuTimerStop(ST_SYS_TIMER *pstSysTimer)
{
    CPUTimer_stopTimer(pstSysTimer->TimerBaseReg);
}



#ifdef __cplusplus
    }
#endif

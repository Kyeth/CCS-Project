/**
 * @file Hw_CpuTimerDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS CPU-Timer驱动源文件
 * @version 0.1
 * @date 2026-10-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_CpuTimerDriver.h"

/* 预分频寄存器TDDR为8位，最大分频系数256 */
#define HW_CPUTIMER_PRESCALER_MAX            (255U)

/* CPU定时器寄存器基址表 */
static volatile struct CPUTIMER_REGS * const gCpuTimerRegs[HW_CPUTIMER_ID_MAX] =
{
    &CpuTimer0Regs,
    &CpuTimer1Regs,
    &CpuTimer2Regs
};

/**
 * @brief 根据定时器对象取寄存器基址，对象非法时返回空指针
 *
 * @param pstSysTimer 定时器对象指针
 * @return 寄存器基址
 */
static volatile struct CPUTIMER_REGS *Hw_CpuTimerGetRegs(const ST_SYS_TIMER *pstSysTimer)
{
    if ((const ST_SYS_TIMER *)0 == pstSysTimer)
    {
        return (volatile struct CPUTIMER_REGS *)0;
    }

    if ((Uint16)HW_CPUTIMER_ID_MAX <= pstSysTimer->u16TimerId)
    {
        return (volatile struct CPUTIMER_REGS *)0;
    }

    return gCpuTimerRegs[pstSysTimer->u16TimerId];
}

void Hw_CpuTimerInit(ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);
    Uint16 u16Tddr = 0U;

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    /* 1. 停止定时器，保证下面改写配置寄存器时计数器不动 */
    pstRegs->TCR.bit.TSS = 1U;

    /* 2. 预分频：TDDR超范围时按最大值截断，TPRH固定为0（只使用低8位） */
    u16Tddr = pstSysTimer->u16Prescaler;
    if (HW_CPUTIMER_PRESCALER_MAX < u16Tddr)
    {
        u16Tddr = HW_CPUTIMER_PRESCALER_MAX;
    }
    pstRegs->TPRH.all = 0U;
    pstRegs->TPR.all = u16Tddr;

    /* 3. 周期寄存器，计数器从PRD递减到0后溢出 */
    pstRegs->PRD.all = pstSysTimer->u32Period;

    /* 4. 仿真挂起方式：FREE=1时SOFT=1为"当前计数递减结束后停止"，SOFT=0为自由运行 */
    pstRegs->TCR.bit.FREE = 1U;
    pstRegs->TCR.bit.SOFT = (0U != pstSysTimer->bEmuStop) ? 1U : 0U;

    /* 5. 中断标志写1清除，TRB写1把计数器重载为PRD */
    pstRegs->TCR.bit.TIF = 1U;
    pstRegs->TCR.bit.TRB = 1U;

    /* 6. 中断使能 */
    pstRegs->TCR.bit.TIE = (0U != pstSysTimer->bEnableInt) ? 1U : 0U;

    /* 7. 软件心跳清零，分频系数为0时按1处理，防止中断里恒成立 */
    pstSysTimer->u32Tick = 0U;
    pstSysTimer->u16TickDivCnt = 0U;
    if (0U == pstSysTimer->u16TickDivCoef)
    {
        pstSysTimer->u16TickDivCoef = 1U;
    }
}

void Hw_CpuTimerStart(ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    pstRegs->TCR.bit.TSS = 0U;
}

void Hw_CpuTimerStop(ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    pstRegs->TCR.bit.TSS = 1U;
}

void Hw_CpuTimerSetPeriod(ST_SYS_TIMER *pstSysTimer, Uint32 u32Period)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);
    Uint16 u16TssBak = 0U;

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    /* 先停止定时器再改周期，改完恢复原来的运行状态 */
    u16TssBak = pstRegs->TCR.bit.TSS;
    pstRegs->TCR.bit.TSS = 1U;
    pstSysTimer->u32Period = u32Period;
    pstRegs->PRD.all = u32Period;
    pstRegs->TCR.bit.TRB = 1U;
    pstRegs->TCR.bit.TSS = u16TssBak;
}

void Hw_CpuTimerReload(ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    pstRegs->TCR.bit.TRB = 1U;
}

Uint32 Hw_CpuTimerGetCount(const ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return 0UL;
    }

    return pstRegs->TIM.all;
}

void Hw_CpuTimerClearIntFlag(ST_SYS_TIMER *pstSysTimer)
{
    volatile struct CPUTIMER_REGS *pstRegs = Hw_CpuTimerGetRegs(pstSysTimer);

    if ((volatile struct CPUTIMER_REGS *)0 == pstRegs)
    {
        return;
    }

    /* TIF为写1清除 */
    pstRegs->TCR.bit.TIF = 1U;
}

Uint16 Hw_CpuTimerIsrTick(ST_SYS_TIMER *pstSysTimer)
{
    if ((ST_SYS_TIMER *)0 == pstSysTimer)
    {
        return 0U;
    }

    pstSysTimer->u16TickDivCnt++;
    if (pstSysTimer->u16TickDivCnt < pstSysTimer->u16TickDivCoef)
    {
        return 0U;
    }

    pstSysTimer->u16TickDivCnt = 0U;
    pstSysTimer->u32Tick++;

    return 1U;
}

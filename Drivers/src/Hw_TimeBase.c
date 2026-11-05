/**
 * @file Hw_TimeBase.c
 * @author yetingheng (004182)
 * @brief F2837xS 自由运行时间基准（CPU Timer0）
 * @version 0.1
 * @date 2026-11-03
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_TimeBase.h"

/* 4096 次读仍不一致说明定时器被别处改写或时钟异常，退回未就绪 */
#define HW_TIMEBASE_READ_RETRY          (4U)

/* 启动后检查计数是否真的在动的自旋上限 */
#define HW_TIMEBASE_START_CHECK         (1000U)

/* 每微秒的 SYSCLK 周期数（200 MHz -> 200） */
#define HW_TIMEBASE_US_DIVISOR          (1000000UL)

/* Timer0 只写一次，之后只读；就绪标志供所有按时间等待的模块查询 */
static Uint16 s_bTimeBaseReady = 0U;
static Uint32 s_u32SysclkHz = 0UL;

static Uint16 Hw_TimeBaseReadRaw(Uint32 *pu32Value)
{
    Uint32 u32HiFirst;
    Uint32 u32Low;
    Uint32 u32HiSecond;
    Uint16 u16Retry;

    for (u16Retry = 0U; u16Retry < HW_TIMEBASE_READ_RETRY; u16Retry++)
    {
        u32HiFirst  = (Uint32)CpuTimer0Regs.TIM.bit.MSW;
        u32Low      = (Uint32)CpuTimer0Regs.TIM.bit.LSW;
        u32HiSecond = (Uint32)CpuTimer0Regs.TIM.bit.MSW;

        if (u32HiFirst == u32HiSecond)
        {
            *pu32Value = (Uint32)((u32HiFirst << 16U) | u32Low);
            return 1U;
        }
    }

    /* 连续不一致：显式上报时间源故障，绝不返回可能撕裂的拼接值 */
    s_bTimeBaseReady = 0U;

    return 0U;
}

Uint16 Hw_TimeBaseInit(Uint32 u32SysclkHz)
{
    Uint32 u32First;
    Uint16 u16Spin;
    Uint16 u16Running;

    s_bTimeBaseReady = 0U;
    s_u32SysclkHz = 0UL;

    if (u32SysclkHz == 0UL)
    {
        return 0U;
    }

    /* 1. 显式使能 Timer0 时钟：本工程没有调用 InitPeripheralClocks()，不能依赖默认值 */
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.CPUTIMER0 = 1U;
    EDIS;

    if (CpuSysRegs.PCLKCR0.bit.CPUTIMER0 == 0U)
    {
        return 0U;
    }

    /* 2. 先停表，保证下面改写配置时计数器不动 */
    CpuTimer0Regs.TCR.bit.TSS = 1U;

    /* 3. 永不产生中断：本定时器只用于测量 */
    CpuTimer0Regs.TCR.bit.TIE = 0U;

    /* 4. 预分频（TDDR:TDDRH）与周期（PRD:PRDH）全部显式写死，不继承复位或他人残留 */
    CpuTimer0Regs.TPR.all  = 0x0000U;
    CpuTimer0Regs.TPRH.all = 0x0000U;
    CpuTimer0Regs.PRD.all  = 0xFFFFFFFFUL;

    /* 5. 直接把计数器置为满量程，避免依赖 TRB 从 PRD 装载的时序 */
    CpuTimer0Regs.TIM.all = 0xFFFFFFFFUL;

    /* 6. 清可能残留的中断标志，然后放行 */
    CpuTimer0Regs.TCR.bit.TIF = 1U;
    CpuTimer0Regs.TCR.bit.TRB = 1U;
    CpuTimer0Regs.TCR.bit.TSS = 0U;

    /* 7. 回读配置：任何一项没写进去都按失败处理 */
    if ((CpuTimer0Regs.TPR.all != 0x0000U) || (CpuTimer0Regs.TPRH.all != 0x0000U) ||
        (CpuTimer0Regs.PRD.all != 0xFFFFFFFFUL) || (CpuTimer0Regs.TCR.bit.TSS != 0U) ||
        (CpuTimer0Regs.TCR.bit.TIE != 0U))
    {
        return 0U;
    }

    /* 8. 验证计数确实在递减：不运行则所有按时间的等待都不会到期 */
    u32First = CpuTimer0Regs.TIM.all;
    u16Running = 0U;

    for (u16Spin = 0U; u16Spin < HW_TIMEBASE_START_CHECK; u16Spin++)
    {
        if (CpuTimer0Regs.TIM.all != u32First)
        {
            u16Running = 1U;
            break;
        }
    }

    if (u16Running == 0U)
    {
        CpuTimer0Regs.TCR.bit.TSS = 1U;
        return 0U;
    }

    s_u32SysclkHz = u32SysclkHz;
    s_bTimeBaseReady = 1U;

    return 1U;
}

Uint32 Hw_TimeBaseTicks(void)
{
    Uint32 u32Value;

    if (Hw_TimeBaseReadRaw(&u32Value) == 0U)
    {
        /* 故障已在读取处锁存，Hw_TimeBaseIsReady() 随之为 0 */
        return 0UL;
    }

    return u32Value;
}

Uint32 Hw_TimeBaseElapsed(Uint32 u32Start)
{
    Uint32 u32Now;

    /* 递减计数器：经过的周期数 = 起点 - 当前值，模 2^32 自动吸收一次回绕 */
    if (Hw_TimeBaseReadRaw(&u32Now) == 0U)
    {
        return 0UL;
    }

    return (Uint32)(u32Start - u32Now);
}

Uint16 Hw_TimeBaseIsReady(void)
{
    return s_bTimeBaseReady;
}

Uint32 Hw_TimeBaseUsToTicks(Uint32 u32Us)
{
    Uint32 u32TicksPerUs;

    if ((s_bTimeBaseReady == 0U) || (s_u32SysclkHz == 0UL))
    {
        return 0UL;
    }

    u32TicksPerUs = s_u32SysclkHz / HW_TIMEBASE_US_DIVISOR;
    if (u32TicksPerUs == 0UL)
    {
        return 0UL;
    }

    return u32Us * u32TicksPerUs;
}

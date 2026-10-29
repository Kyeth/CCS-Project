/**
 * @file Hw_WatchDog.c
 * @author yetingheng(004182)
 * @brief
 * @version 0.1
 * @date 2026-10-19
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Hw_WatchDog.h"

#define WDOG_CLOCK_HZ   (10000000UL)  /* INTOSC1标称10MHz，WDCLK固定先除512 */
#define WDOG_CNT_MAX    (256UL)       /* 8 位计数器最大值 */

/**
 * @brief 初始化看门狗，配置超时复位时间（纯整型运算，无浮点依赖）
 *
 * @param TimeOut 期望超时时间ms；硬件8位计数不能实现小于约13.1ms
 */
void Hw_WatchDogInit(Uint16 TimeOut)
{
    Uint16 u16Prescaler = 0U;
    Uint16 i;
    Uint32 u32TargetTicks;
    Uint32 u32CurrentTicks = 0U;
    Uint32 u32Div;

    /* 计算目标计数值 (ms 转换为 10MHz 下的 tick 数) */
    u32TargetTicks = ((Uint32)TimeOut * (WDOG_CLOCK_HZ / 512UL)) / 1000UL;

    /* 遍历 0~7 级预分频，寻找最接近或大于目标 tick 的分频系数 */
    for (i = 0U; i < 8U; i++)
    {
        /* F2837xS 看门狗分频表: i=0为1倍，i=1为1倍，i=2为2倍，i=7为64倍 */
        u32Div = (i == 0U) ? 1UL : (1UL << (i - 1U));
        u32CurrentTicks = WDOG_CNT_MAX * u32Div;

        u16Prescaler = i;

        /* 如果当前分频的计数值已经覆盖了目标时间，直接跳出 */
        if (u32CurrentTicks >= u32TargetTicks)
        {
            break;
        }

        /* 如果循环到最大值（i=7）还不够，就锁定在最大值 */
        if (i == 7U)
        {
            u16Prescaler = 7U;
        }
    }

    /* 配置看门狗：WDDIS=1(禁用), WDCHK=101(必须), WDPS=u16Prescaler */
    EALLOW;
    WdRegs.WDCR.all = 0x0068U | (u16Prescaler & 0x0007U);
    EDIS;
}

/**
 * @brief 启动看门狗
 */
void Hw_EnableWatchDog(void)
{
    EALLOW;
    /* 清除 WDDIS 位 (bit 6)，保持 WDCHK 为 101，启动看门狗 */
    WdRegs.WDCR.all = (WdRegs.WDCR.all & ~0x0040U) | 0x0028U;
    EDIS;
}

/**
 * @brief 禁用看门狗
 */
void Hw_DisableWatchDog(void)
{
    EALLOW;
    /* 置位 WDDIS 位 (bit 6)，保持 WDCHK 为 101，禁用看门狗 */
    WdRegs.WDCR.all = (WdRegs.WDCR.all | 0x0068U);
    EDIS;
}

/**
 * @brief 喂狗操作
 */
void Hw_FeedWatchDog(void)
{
    EALLOW;
    WdRegs.WDKEY.all = 0x0055U;
    WdRegs.WDKEY.all = 0x00AAU;
    EDIS;
}

/**
 * @brief 看门狗复位芯片（写入错误的 WDCHK 校验值触发复位）
 */
void Hw_SysReset(void)
{
    EALLOW;
    /* 置位 WDDIS 不生效，写入 WDCHK = 000b 触发复位 */
    WdRegs.WDCR.all = WdRegs.WDCR.all & 0x00C7U;
    EDIS;
}

/**
 * @file SmUtil.c
 * @author yetingheng (004182)
 * @brief 状态机公共时基与延时工具实现
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 */

#include "SmUtil.h"

/******************************************************************************
 * 内部数据
 ******************************************************************************/
static volatile Uint32 sSmTick100us = 0UL;

/******************************************************************************
 * 时基
 ******************************************************************************/
void SmTick(void)
{
    sSmTick100us++;
}

Uint32 SmGetTick100us(void)
{
    return sSmTick100us;
}

Uint32 SmGetMs(void)
{
    return sSmTick100us / SM_TICK_PER_MS;
}

/******************************************************************************
 * 延时器
 ******************************************************************************/
void SmDelayRestart(ST_SM_DELAY *pstDelay)
{
    if (pstDelay != 0)
    {
        pstDelay->u32Start = sSmTick100us;
    }
}

Uint16 SmDelayElapsed(const ST_SM_DELAY *pstDelay, Uint32 u32Ticks)
{
    if (pstDelay == 0)
    {
        return 1U;
    }

    /* 无符号减法天然处理 32 位回绕 */
    return ((Uint32)(sSmTick100us - pstDelay->u32Start) >= u32Ticks) ? 1U : 0U;
}

Uint16 SmDelayElapsedMs(const ST_SM_DELAY *pstDelay, Uint32 u32Ms)
{
    return SmDelayElapsed(pstDelay, u32Ms * SM_TICK_PER_MS);
}

/******************************************************************************
 * 电平消抖
 ******************************************************************************/
void SmDebounceInit(ST_SM_DEBOUNCE *pstDeb, Uint16 u16Init, Uint16 u16Thr)
{
    if (pstDeb != 0)
    {
        pstDeb->u16State = (u16Init != 0U) ? 1U : 0U;
        pstDeb->u16Cnt   = 0U;
        pstDeb->u16Thr   = (u16Thr != 0U) ? u16Thr : 1U;
    }
}

Uint16 SmDebounceRun(ST_SM_DEBOUNCE *pstDeb, Uint16 u16Raw)
{
    Uint16 u16In;

    if (pstDeb == 0)
    {
        return (u16Raw != 0U) ? 1U : 0U;
    }

    u16In = (u16Raw != 0U) ? 1U : 0U;

    if (u16In == pstDeb->u16State)
    {
        pstDeb->u16Cnt = 0U;
    }
    else
    {
        if (pstDeb->u16Cnt < 0xFFFFU)
        {
            pstDeb->u16Cnt++;
        }

        if (pstDeb->u16Cnt >= pstDeb->u16Thr)
        {
            pstDeb->u16State = u16In;
            pstDeb->u16Cnt   = 0U;
        }
    }

    return pstDeb->u16State;
}

Uint16 SmRising(Uint16 u16Now, Uint16 u16Last)
{
    return ((u16Last == 0U) && (u16Now != 0U)) ? 1U : 0U;
}

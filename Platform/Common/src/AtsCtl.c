/**
 * @file AtsCtl.c
 * @author yetingheng (004182)
 * @brief ATS（断路器）干接点控制实现
 * @version 0.1
 * @date 2026-10-27
 * @copyright Copyright (c) 2026
 */
#include "AtsCtl.h"
#include "Hw_GpioDriver.h"

ST_ATS_CTL gAtsCtl;

/**
 * @brief 读 53 并换算成"是否闭合"
 */
static Uint16 AtsCtlReadClosed(void)
{
    Uint32 u32Level;

    u32Level = Hw_GpioReadPin(ATS_CTL_GPIO_STATE);

    return ((Uint16)u32Level == ATS_CTL_STATE_CLOSED_LEVEL) ? 1U : 0U;
}

void AtsCtlInit(void)
{
    gAtsCtl.u16State = AtsCtlReadClosed();
    gAtsCtl.u16RawState = gAtsCtl.u16State;
    gAtsCtl.u16Request = gAtsCtl.u16State;
    gAtsCtl.u16Level = 0U;          /* 52 上电保持 GPIO 表里的初值（低 = 申请断开） */
    gAtsCtl.u16DebounceCnt = 0U;
    gAtsCtl.u16TimeoutCnt = 0U;
    gAtsCtl.u16ActCnt = 0U;
    gAtsCtl.eActState = ATS_ACT_IDLE;

    Hw_GpioWritePin(ATS_CTL_GPIO_REQUEST, gAtsCtl.u16Level);
}

void AtsCtlTask(void *pArgV)
{
    Uint16 u16Now;

    (void)pArgV;

    /* ---------- 1. 状态采样与去抖 ---------- */
    u16Now = AtsCtlReadClosed();
    gAtsCtl.u16RawState = u16Now;

    if (u16Now == gAtsCtl.u16State)
    {
        gAtsCtl.u16DebounceCnt = 0U;
    }
    else
    {
        if (gAtsCtl.u16DebounceCnt < 0xFFFFU)
        {
            gAtsCtl.u16DebounceCnt++;
        }

        if (gAtsCtl.u16DebounceCnt >= ATS_CTL_DEBOUNCE_MS)
        {
            gAtsCtl.u16State = u16Now;
            gAtsCtl.u16DebounceCnt = 0U;
        }
    }

    /* ---------- 2. 动作完成/超时判定 ---------- */
    if (gAtsCtl.eActState == ATS_ACT_WAIT)
    {
        if (gAtsCtl.u16State == gAtsCtl.u16Request)
        {
            gAtsCtl.eActState = ATS_ACT_IDLE;
            gAtsCtl.u16TimeoutCnt = 0U;

            if (gAtsCtl.u16ActCnt < 0xFFFFU)
            {
                gAtsCtl.u16ActCnt++;
            }
        }
        else
        {
            if (gAtsCtl.u16TimeoutCnt < 0xFFFFU)
            {
                gAtsCtl.u16TimeoutCnt++;
            }

            if (gAtsCtl.u16TimeoutCnt >= ATS_CTL_TIMEOUT_MS)
            {
                gAtsCtl.eActState = ATS_ACT_TIMEOUT;
            }
        }
    }
}

void AtsRequestChange(Uint16 u16Target)
{
    Uint16 u16TargetLevel;

    u16Target = (u16Target != 0U) ? 1U : 0U;

    /* 已经一致：不用发命令（发了反而可能反向动作） */
    if (gAtsCtl.u16State == u16Target)
    {
        gAtsCtl.u16Request = u16Target;
        gAtsCtl.eActState = ATS_ACT_IDLE;
        return;
    }

    /* 边沿触发：翻转一次电平就是一次指令 */
    u16TargetLevel = (gAtsCtl.u16Level != 0U) ? 0U : 1U;

    gAtsCtl.u16Level = u16TargetLevel;
    gAtsCtl.u16Request = u16Target;
    gAtsCtl.u16TimeoutCnt = 0U;
    gAtsCtl.eActState = ATS_ACT_WAIT;

    Hw_GpioWritePin(ATS_CTL_GPIO_REQUEST, u16TargetLevel);
}

Uint16 AtsGetState(void)
{
    return gAtsCtl.u16State;
}

Uint16 AtsGetRequest(void)
{
    return gAtsCtl.u16Request;
}

Uint16 AtsIsBusy(void)
{
    return (gAtsCtl.eActState == ATS_ACT_WAIT) ? 1U : 0U;
}

Uint16 AtsIsTimeout(void)
{
    return (gAtsCtl.eActState == ATS_ACT_TIMEOUT) ? 1U : 0U;
}

/**
 * @file Hw_EcapDriver.c
 * @author yetingheng (004182)
 * @brief F28374S eCAP 捕获驱动实现
 * @version 0.1
 * @date 2026-10-22
 * @copyright Copyright (c) 2026
 */
#include "Hw_EcapDriver.h"

/* eCAP 模块寄存器基址表：下标即 E_HW_ECAP_ID */
static volatile struct ECAP_REGS * const gEcapRegs[HW_ECAP_ID_MAX] =
{
    &ECap1Regs,
    &ECap2Regs,
    &ECap3Regs,
    &ECap4Regs,
    &ECap5Regs,
    &ECap6Regs
};

/* eCAP 输入选择寄存器表：eCAP1~eCAP6 固定连接到 INPUT7~INPUT12 */
static volatile Uint16 * const gEcapInputXbar[HW_ECAP_ID_MAX] =
{
    &InputXbarRegs.INPUT7SELECT,
    &InputXbarRegs.INPUT8SELECT,
    &InputXbarRegs.INPUT9SELECT,
    &InputXbarRegs.INPUT10SELECT,
    &InputXbarRegs.INPUT11SELECT,
    &InputXbarRegs.INPUT12SELECT
};

/* 初始化标志，未初始化的模块不允许读写 */
static Uint16 gEcapInited[HW_ECAP_ID_MAX] = {0U};

/* 时钟使能后到模块可访问的延时，单位 SYSCLK 周期 */
#define HW_ECAP_CLOCK_DELAY_CNT     (16U)

/* 配置 eCAP 的 GPIO 合法范围（本板只用 GPIO0~GPIO63） */
#define HW_ECAP_GPIO_MAX            (63U)

/**
 * @brief 判断 eCAP 模块编号是否合法
 */
static Uint16 Hw_EcapIdIsValid(Uint16 u16EcapId)
{
    return (u16EcapId < (Uint16)HW_ECAP_ID_MAX) ? 1U : 0U;
}

/**
 * @brief 取模块寄存器基址
 */
static volatile struct ECAP_REGS *Hw_EcapGetRegs(Uint16 u16EcapId)
{
    if (Hw_EcapIdIsValid(u16EcapId) == 0U)
    {
        return (volatile struct ECAP_REGS *)0;
    }

    return gEcapRegs[u16EcapId];
}

/**
 * @brief 打开 eCAP 模块时钟
 *
 * PCLKCR3 的 bit0~bit5 分别对应 ECAP1~ECAP6，
 * PCLKCR 寄存器受 EALLOW 保护，由调用者负责。
 */
static void Hw_EcapEnableClock(Uint16 u16EcapId)
{
    Uint32 u32Mask;

    u32Mask = 1UL << u16EcapId;
    CpuSysRegs.PCLKCR3.all |= u32Mask;
}

/**
 * @brief 时钟使能后的短延时
 */
static void Hw_EcapClockDelay(void)
{
    volatile Uint16 u16Cnt;

    for (u16Cnt = 0U; u16Cnt < HW_ECAP_CLOCK_DELAY_CNT; u16Cnt++)
    {
    }
}

void Hw_EcapSingleInit(const ST_ECAP_CFG_ITEM *pstEcapCfgItem)
{
    volatile struct ECAP_REGS *pstRegs;
    Uint16 u16EcapId;

    if (pstEcapCfgItem == (const ST_ECAP_CFG_ITEM *)0)
    {
        return;
    }

    u16EcapId = pstEcapCfgItem->u16EcapId;

    if (Hw_EcapIdIsValid(u16EcapId) == 0U)
    {
        return;
    }

    if (pstEcapCfgItem->u16GpioNum > HW_ECAP_GPIO_MAX)
    {
        return;
    }

    pstRegs = gEcapRegs[u16EcapId];

    EALLOW;

    /* 1. 开时钟并等待模块可访问 */
    Hw_EcapEnableClock(u16EcapId);
    Hw_EcapClockDelay();

    /* 2. 把 GPIO 接到对应的 INPUTx，eCAP1~6 固定用 INPUT7~12 */
    *(gEcapInputXbar[u16EcapId]) = pstEcapCfgItem->u16GpioNum;

    /* 3. 配置期间停止时间戳计数器并清零 */
    pstRegs->ECCTL2.bit.TSCTRSTOP = 0U;
    pstRegs->TSCTR = 0UL;

    /* 4. 关中断、清所有事件标志 */
    pstRegs->ECEINT.all = 0x0000U;
    pstRegs->ECCLR.all = 0xFFFFU;

    /* 5. 捕获模式（不用 APWM），输入不预分频（事件滤波预分频为 1） */
    pstRegs->ECCTL2.bit.CAP_APWM = 0U;
    pstRegs->ECCTL1.bit.PRESCALE = 0U;

    /* 6. 连续模式，事件序列只到事件1就回绕（STOP_WRAP = 0 表示捕获 1 个事件后回绕） */
    pstRegs->ECCTL2.bit.CONT_ONESHT = 0U;
    pstRegs->ECCTL2.bit.STOP_WRAP = 0U;

    /* 7. 事件1：按配置的边沿捕获，捕获后清零计数器 → CAP1 即一个完整周期 */
    pstRegs->ECCTL1.bit.CAP1POL = pstEcapCfgItem->u16CaptureEdge;
    pstRegs->ECCTL1.bit.CTRRST1 = 1U;

    /* 8. 事件2~4 不使用，但允许装载 CAP 寄存器 */
    pstRegs->ECCTL1.bit.CAP2POL = 0U;
    pstRegs->ECCTL1.bit.CAP3POL = 0U;
    pstRegs->ECCTL1.bit.CAP4POL = 0U;
    pstRegs->ECCTL1.bit.CTRRST2 = 0U;
    pstRegs->ECCTL1.bit.CTRRST3 = 0U;
    pstRegs->ECCTL1.bit.CTRRST4 = 0U;
    pstRegs->ECCTL1.bit.CAPLDEN = 1U;

    /* 9. 不使用同步输入/输出 */
    pstRegs->ECCTL2.bit.SYNCI_EN = 0U;
    pstRegs->ECCTL2.bit.SYNCO_SEL = 2U;

    /* 10. 捕获事件1中断：默认不打开，需要时由配置表打开并在 PIE 里注册 */
    if (pstEcapCfgItem->bEnableInt != 0U)
    {
        pstRegs->ECEINT.bit.CEVT1 = 1U;
    }

    /* 11. 清掉配置期间产生的事件，启动计数器 */
    pstRegs->ECCLR.all = 0xFFFFU;
    pstRegs->ECCTL2.bit.TSCTRSTOP = 1U;

    EDIS;

    gEcapInited[u16EcapId] = 1U;
}

void Hw_EcapInit(const ST_ECAP_CFG *pstEcapCfg)
{
    Uint16 u16Idx;

    if ((pstEcapCfg == (const ST_ECAP_CFG *)0) ||
        (pstEcapCfg->pstEcapCfgItem == (const ST_ECAP_CFG_ITEM *)0))
    {
        return;
    }

    for (u16Idx = 0U; u16Idx < pstEcapCfg->u16ItemNum; u16Idx++)
    {
        Hw_EcapSingleInit(&pstEcapCfg->pstEcapCfgItem[u16Idx]);
    }
}

Uint16 Hw_EcapHasEvent(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return 0U;
    }

    return gEcapRegs[u16EcapId]->ECFLG.bit.CEVT1;
}

void Hw_EcapClearEvent(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return;
    }

    gEcapRegs[u16EcapId]->ECCLR.bit.CEVT1 = 1U;
    gEcapRegs[u16EcapId]->ECCLR.bit.INT = 1U;
}

Uint32 Hw_EcapReadEvent(Uint16 u16EcapId)
{
    Uint32 u32Ticks;

    if (Hw_EcapHasEvent(u16EcapId) == 0U)
    {
        return HW_ECAP_INVALID_TICKS;
    }

    u32Ticks = gEcapRegs[u16EcapId]->CAP1;
    Hw_EcapClearEvent(u16EcapId);

    return u32Ticks;
}

Uint32 Hw_EcapGetPeriodTicks(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return HW_ECAP_INVALID_TICKS;
    }

    return gEcapRegs[u16EcapId]->CAP1;
}

float32 Hw_EcapGetFrequencyHz(Uint16 u16EcapId)
{
    Uint32 u32Ticks;

    u32Ticks = Hw_EcapGetPeriodTicks(u16EcapId);

    if ((u32Ticks == 0UL) || (u32Ticks == HW_ECAP_INVALID_TICKS))
    {
        return HW_ECAP_INVALID_FREQ;
    }

    return (float32)HW_ECAP_SYSCLK_HZ / (float32)u32Ticks;
}

Uint32 Hw_EcapGetCounter(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return HW_ECAP_INVALID_TICKS;
    }

    return gEcapRegs[u16EcapId]->TSCTR;
}

void Hw_EcapStart(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return;
    }

    EALLOW;
    gEcapRegs[u16EcapId]->ECCTL2.bit.TSCTRSTOP = 1U;
    EDIS;
}

void Hw_EcapStop(Uint16 u16EcapId)
{
    if ((Hw_EcapIdIsValid(u16EcapId) == 0U) ||
        (gEcapInited[u16EcapId] == 0U))
    {
        return;
    }

    EALLOW;
    gEcapRegs[u16EcapId]->ECCTL2.bit.TSCTRSTOP = 0U;
    EDIS;
}

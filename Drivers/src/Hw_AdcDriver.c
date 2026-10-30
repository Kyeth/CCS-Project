/**
 * @file Hw_AdcDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS ADC驱动源文件
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_AdcDriver.h"
#include "F2837xS_GlobalPrototypes.h"

/* ADCSOCxCTL寄存器各字段的起始位
 * ACQPS 位于 bit8~bit0，CHSEL 位于 bit18~bit15，TRIGSEL 位于 bit24~bit20 */
#define HW_ADC_SOCCTL_ACQPS_SHIFT       (0U)
#define HW_ADC_SOCCTL_CHSEL_SHIFT       (15U)
#define HW_ADC_SOCCTL_TRIGSEL_SHIFT     (20U)

/* 配置参数上限，超出范围的项直接丢弃，避免写出非法寄存器值 */
#define HW_ADC_TRIGSEL_MAX              (31U)
#define HW_ADC_ACQPS_MAX                (511U)
#define HW_ADC_CHSEL_MAX                (15U)

/* 中断标志清除掩码，ADCINTFLGCLR为写1清除 */
#define HW_ADC_INTFLG_CLR_MASK          (0xFFFFU)

/* ADC上电稳定延时计数：200MHz下约1ms，只在初始化时执行一次
 * 注意：不要用device.h里的DEVICE_DELAY_US宏，它依赖的SysCtl_delay在本工程
 *      的device目录中既没有声明也没有实现，用了会链接失败 */
#define HW_ADC_POWERUP_DELAY_CNT        (40000U)

/* ADC寄存器基址表 */
static volatile struct ADC_REGS * const gAdcRegs[HW_ADC_ID_MAX] =
{
    &AdcaRegs,
    &AdcbRegs,
    &AdccRegs,
    &AdcdRegs
};

/* ADC结果寄存器基址表 */
static volatile struct ADC_RESULT_REGS * const gAdcResultRegs[HW_ADC_ID_MAX] =
{
    &AdcaResultRegs,
    &AdcbResultRegs,
    &AdccResultRegs,
    &AdcdResultRegs
};

/**
 * @brief 根据模块编号取ADC寄存器基址，编号非法时返回空指针
 *
 * @param u16AdcId ADC模块编号
 * @return 寄存器基址
 */
static volatile struct ADC_REGS *Hw_AdcGetRegs(Uint16 u16AdcId)
{
    if ((Uint16)HW_ADC_ID_MAX <= u16AdcId)
    {
        return (volatile struct ADC_REGS *)0;
    }

    return gAdcRegs[u16AdcId];
}

/**
 * @brief 根据模块编号取结果寄存器基址，编号非法时返回空指针
 *
 * @param u16AdcId ADC模块编号
 * @return 结果寄存器基址
 */
static volatile struct ADC_RESULT_REGS *Hw_AdcGetResultRegs(Uint16 u16AdcId)
{
    if ((Uint16)HW_ADC_ID_MAX <= u16AdcId)
    {
        return (volatile struct ADC_RESULT_REGS *)0;
    }

    return gAdcResultRegs[u16AdcId];
}

/**
 * @brief 判断某个SOC配置项是否为"未使用"，全0即视为未使用
 *
 * @param pstChCfg SOC配置项指针
 * @return 1: 未使用；0: 已使用
 */
static Uint16 Hw_AdcChCfgIsUnused(const ST_ADC_CH_CFG_ITEM *pstChCfg)
{
    if ((0U == pstChCfg->u16TrigSel) && (0U == pstChCfg->u16Acqps) && (0U == pstChCfg->u16ChSel))
    {
        return 1U;
    }

    return 0U;
}

/**
 * @brief 判断某个SOC配置项的参数是否在寄存器允许范围内
 *
 * @param pstChCfg SOC配置项指针
 * @return 1: 合法；0: 非法
 */
static Uint16 Hw_AdcChCfgIsValid(const ST_ADC_CH_CFG_ITEM *pstChCfg)
{
    if (HW_ADC_TRIGSEL_MAX < pstChCfg->u16TrigSel)
    {
        return 0U;
    }

    if (HW_ADC_ACQPS_MAX < pstChCfg->u16Acqps)
    {
        return 0U;
    }

    if (HW_ADC_CHSEL_MAX < pstChCfg->u16ChSel)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief ADC上电稳定延时，用软件空循环实现，仅初始化时调用一次
 */
static void Hw_AdcPowerUpDelay(void)
{
    volatile Uint32 u32Cnt = HW_ADC_POWERUP_DELAY_CNT;

    while (0UL < u32Cnt)
    {
        u32Cnt--;
    }
}

void Hw_AdcInit(const ST_ADC_CFG *pstCfg)
{
    Uint16 u16Idx = 0U;

    if ((const ST_ADC_CFG *)0 == pstCfg)
    {
        return;
    }

    if ((const ST_ADC_CFG_ITEM *)0 == pstCfg->pstAdcCfgItem)
    {
        return;
    }

    for (u16Idx = 0U; u16Idx < pstCfg->u16ItemNum; u16Idx++)
    {
        Hw_AdcSingleInit(&pstCfg->pstAdcCfgItem[u16Idx]);
    }
}

void Hw_AdcSingleInit(const ST_ADC_CFG_ITEM *pstAdcCfgItem)
{
    volatile struct ADC_REGS *pstRegs;
    volatile Uint32 *pu32SocCtl;
    const ST_ADC_CH_CFG_ITEM *pstChCfg;
    Uint16 u16Soc = 0U;
    Uint16 u16SocUsed = 0U;

    if ((const ST_ADC_CFG_ITEM *)0 == pstAdcCfgItem)
    {
        return;
    }

    if ((const ST_ADC_CH_CFG_ITEM *)0 == pstAdcCfgItem->pstAdcChCfgItem)
    {
        return;
    }

    pstRegs = Hw_AdcGetRegs(pstAdcCfgItem->u16AdcId);
    if ((volatile struct ADC_REGS *)0 == pstRegs)
    {
        return;
    }

    /* 1. 设置分辨率与信号模式：AdcSetMode内部会重装INL trim、从OTP取偏置trim并
     *    写ADCINLTRIMx，这些寄存器都需要EALLOW；该函数自身没有EALLOW/EDIS */
    EALLOW;
    /* 0. ADC时钟分频：ADCCLK = SYSCLK/4 = 50MHz（PRESCALE = 6）。
     *    复位值为0即SYSCLK不分频，超过器件ADCCLK上限，必须先设置。 */
    pstRegs->ADCCTL2.bit.PRESCALE = pstAdcCfgItem->u16Prescale;
    AdcSetMode(pstAdcCfgItem->u16AdcId, pstAdcCfgItem->u16Resolution, pstAdcCfgItem->u16SignalMode);
    pstRegs->ADCCTL1.bit.ADCPWDNZ = 1U;
    EDIS;

    /* 2. 上电后等待约1ms，等内部基准和模拟电路稳定 */
    Hw_AdcPowerUpDelay();

    EALLOW;

    /* 3. 在转换结束时产生中断脉冲，ADCINT1才能正确指示"转换完成" */
    pstRegs->ADCCTL1.bit.INTPULSEPOS = 1U;

    /* 4. 逐个配置SOC0~SOC15，未使用的SOC保持复位值
     *    ADCSOCxCTL在ADC_REGS中按32位连续排列，可整体数组化访问 */
    pu32SocCtl = (volatile Uint32 *)&pstRegs->ADCSOC0CTL;
    pstChCfg = pstAdcCfgItem->pstAdcChCfgItem;

    if (pstAdcCfgItem->u16SocStride == 0U)
    {
        EDIS;
        return;
    }

    /* Fixed priority keeps SOC15 after the other ZERO-triggered conversions. */
    pstRegs->ADCSOCPRICTL.bit.SOCPRIORITY = 16U;

    for (u16Soc = 0U; u16Soc < HW_ADC_SOC_MAX; u16Soc++)
    {
        const ST_ADC_CH_CFG_ITEM *pstSoc = &pstChCfg[(Uint32)u16Soc * pstAdcCfgItem->u16SocStride];
        if (0U != Hw_AdcChCfgIsUnused(pstSoc))
        {
            continue;
        }

        if (0U == Hw_AdcChCfgIsValid(pstSoc))
        {
            continue;
        }

        pu32SocCtl[u16Soc] = ((Uint32)pstSoc->u16TrigSel << HW_ADC_SOCCTL_TRIGSEL_SHIFT)
                           | ((Uint32)pstSoc->u16ChSel << HW_ADC_SOCCTL_CHSEL_SHIFT)
                           | ((Uint32)pstSoc->u16Acqps << HW_ADC_SOCCTL_ACQPS_SHIFT);
        u16SocUsed = 1U;
    }

    /* 5. 配置ADCINT1：指定由哪个SOC的转换完成触发。
     *    连续模式（INT1CONT=1）：标志在每个 EOC 自动更新，ADCINT1 才能
     *    每个 PWM 周期都产生一次脉冲去触发 CLA Task1；单次模式下标志置起后
     *    无人清除，CLA 只会被触发一次。 */
    pstRegs->ADCINTSEL1N2.bit.INT1SEL = (pstAdcCfgItem->u16IntSel & 0x000FU);
    pstRegs->ADCINTSEL1N2.bit.INT1CONT = 1U;

    if ((0U != pstAdcCfgItem->bAdcIntEnable) && (0U != u16SocUsed))
    {
        pstRegs->ADCINTSEL1N2.bit.INT1E = 1U;
    }
    else
    {
        pstRegs->ADCINTSEL1N2.bit.INT1E = 0U;
    }

    pstRegs->ADCINTSEL1N2.bit.INT2E = 0U;

    /* 6. 清中断标志，避免复位后残留标志误触发CLA或CPU中断 */
    pstRegs->ADCINTFLGCLR.all = HW_ADC_INTFLG_CLR_MASK;

    EDIS;
}

void Hw_AdcSoftTrigger(Uint16 u16AdcId, Uint16 u16SocMask)
{
    volatile struct ADC_REGS *pstRegs = Hw_AdcGetRegs(u16AdcId);

    if ((volatile struct ADC_REGS *)0 == pstRegs)
    {
        return;
    }

    pstRegs->ADCSOCFRC1.all = (Uint32)u16SocMask;
}

void Hw_AdcSetIntEnable(Uint16 u16AdcId, Uint16 bEnable)
{
    volatile struct ADC_REGS *pstRegs = Hw_AdcGetRegs(u16AdcId);

    if ((volatile struct ADC_REGS *)0 == pstRegs)
    {
        return;
    }

    EALLOW;

    if (0U != bEnable)
    {
        pstRegs->ADCINTSEL1N2.bit.INT1E = 1U;
    }
    else
    {
        pstRegs->ADCINTSEL1N2.bit.INT1E = 0U;
    }

    EDIS;
}

Uint16 Hw_AdcGetIntFlag(Uint16 u16AdcId)
{
    volatile struct ADC_REGS *pstRegs = Hw_AdcGetRegs(u16AdcId);

    if ((volatile struct ADC_REGS *)0 == pstRegs)
    {
        return 0U;
    }

    return (Uint16)pstRegs->ADCINTFLG.bit.ADCINT1;
}

void Hw_AdcClearIntFlag(Uint16 u16AdcId)
{
    volatile struct ADC_REGS *pstRegs = Hw_AdcGetRegs(u16AdcId);

    if ((volatile struct ADC_REGS *)0 == pstRegs)
    {
        return;
    }

    pstRegs->ADCINTFLGCLR.all = HW_ADC_INTFLG_CLR_MASK;
}

Uint16 Hw_AdcReadResult(Uint16 u16AdcId, Uint16 u16Soc)
{
    volatile struct ADC_RESULT_REGS *pstResultRegs = Hw_AdcGetResultRegs(u16AdcId);

    if ((volatile struct ADC_RESULT_REGS *)0 == pstResultRegs)
    {
        return 0U;
    }

    if (HW_ADC_SOC_MAX <= u16Soc)
    {
        return 0U;
    }

    return ((volatile Uint16 *)&pstResultRegs->ADCRESULT0)[u16Soc];
}

void Hw_AdcReadAll(Uint16 u16AdcId, Uint16 *pu16Buf)
{
    Uint16 u16Soc = 0U;

    if ((Uint16 *)0 == pu16Buf)
    {
        return;
    }

    for (u16Soc = 0U; u16Soc < HW_ADC_SOC_MAX; u16Soc++)
    {
        pu16Buf[u16Soc] = Hw_AdcReadResult(u16AdcId, u16Soc);
    }
}

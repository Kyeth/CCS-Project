/**
 * @file Hw_GpioDriver.c
 * @author yetingheng (004182)
 * @brief TMS320F2837xS GPIO底层驱动
 * @version 0.3
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Hw_GpioDriver.h"

/******************************************************************************
 * 私有函数
 ******************************************************************************/

/**
 * @brief 获取GPIO控制寄存器组地址
 */
static volatile Uint32 *Hw_GpioGetCtrlBase(Uint32 u32Pin)
{
    Uint32 u32GpioGroup;

    u32GpioGroup = u32Pin / 32U;

    return (volatile Uint32 *)HW_GPIO_CTRLREGS_BASE
           + (u32GpioGroup * HW_STEP_GPIOCTRL_REGS);
}

/**
 * @brief 获取GPIO数据寄存器组地址
 */
static volatile Uint32 *Hw_GpioGetDataBase(Uint32 u32Pin)
{
    Uint32 u32GpioGroup;

    u32GpioGroup = u32Pin / 32U;

    return (volatile Uint32 *)HW_GPIO_DATAREGS_BASE
           + (u32GpioGroup * HW_STEP_GPIODAT_REGS);
}

/**
 * @brief 在已经打开EALLOW的情况下配置单个GPIO
 */
static void Hw_GpioApplyConfig(const ST_GPIO_CFG_ITEM *pstGpioCfgItem)
{
    volatile Uint32 *pu32GpioCtrlAddr;
    volatile Uint32 *pu32GpioDataAddr;
    volatile Uint32 *pu32GpioQSelAddr;
    volatile Uint32 *pu32GpioMuxAddr;
    volatile Uint32 *pu32GpioGMuxAddr;

    Uint32 u32Pin;
    Uint32 u32PinBit;
    Uint32 u32PinMask;
    Uint32 u32MuxShift;
    Uint32 u32MuxMask;
    Uint32 u32MuxValue;
    Uint32 u32GMuxValue;
    Uint32 u32QualValue;

    if (pstGpioCfgItem == (const ST_GPIO_CFG_ITEM *)0)
    {
        return;
    }

    u32Pin = (Uint32)pstGpioCfgItem->u16Pin;

    if (u32Pin > 168U)
    {
        return;
    }

    u32PinBit = u32Pin % 32U;
    u32PinMask = (Uint32)1UL << u32PinBit;

    u32MuxShift = (u32Pin % 16U) * 2U;
    u32MuxMask = (Uint32)3UL << u32MuxShift;

    pu32GpioCtrlAddr = Hw_GpioGetCtrlBase(u32Pin);
    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);

    if (u32PinBit < 16U)
    {
        pu32GpioQSelAddr = pu32GpioCtrlAddr + HW_OFFSET_GPIOQSEL1;
        pu32GpioMuxAddr  = pu32GpioCtrlAddr + HW_OFFSET_GPIOMUX1;
        pu32GpioGMuxAddr = pu32GpioCtrlAddr + HW_OFFSET_GPIOGMUX1;
    }
    else
    {
        pu32GpioQSelAddr = pu32GpioCtrlAddr + HW_OFFSET_GPIOQSEL2;
        pu32GpioMuxAddr  = pu32GpioCtrlAddr + HW_OFFSET_GPIOMUX2;
        pu32GpioGMuxAddr = pu32GpioCtrlAddr + HW_OFFSET_GPIOGMUX2;
    }

    /* 1. 配置前暂时切换为输入状态 */
    pu32GpioCtrlAddr[HW_OFFSET_GPIODIR] &= ~u32PinMask;

    /* 2. 预装载GPIO输出锁存值 */
    if (pstGpioCfgItem->u16DefaultLevel == 0U)
    {
        pu32GpioDataAddr[HW_OFFSET_GPIOCLEAR] = u32PinMask;
    }
    else
    {
        pu32GpioDataAddr[HW_OFFSET_GPIOSET] = u32PinMask;
    }

    /* 3. 配置内部上拉（GPxPUD：0=使能，1=禁止） */
    if (pstGpioCfgItem->u16Pullup != 0U)
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIOPUD] &= ~u32PinMask;
    }
    else
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIOPUD] |= u32PinMask;
    }

    /* 4. 配置开漏输出 */
    if (pstGpioCfgItem->u16OpenDrain != 0U)
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIOODR] |= u32PinMask;
    }
    else
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIOODR] &= ~u32PinMask;
    }

    /* 5. 配置输入限定模式 */
    u32QualValue = (Uint32)pstGpioCfgItem->u16QualMode & 0x03UL;

    *pu32GpioQSelAddr &= ~u32MuxMask;
    *pu32GpioQSelAddr |= u32QualValue << u32MuxShift;

    /* 6. 配置复用功能（u16Mux为 (GMUX<<2)|MUX 编码） */
    u32GMuxValue = (((Uint32)pstGpioCfgItem->u16Mux >> 2U) & 0x03UL);
    u32MuxValue = ((Uint32)pstGpioCfgItem->u16Mux & 0x03UL);

    /* 先写GMUX，再写MUX，避免引脚产生毛刺 */
    *pu32GpioGMuxAddr &= ~u32MuxMask;
    *pu32GpioGMuxAddr |= u32GMuxValue << u32MuxShift;

    *pu32GpioMuxAddr &= ~u32MuxMask;
    *pu32GpioMuxAddr |= u32MuxValue << u32MuxShift;

    /* 7. 最后设置方向 */
    if (pstGpioCfgItem->u16Dir != 0U)
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIODIR] |= u32PinMask;
    }
    else
    {
        pu32GpioCtrlAddr[HW_OFFSET_GPIODIR] &= ~u32PinMask;
    }
}

/******************************************************************************
 * 公共函数
 ******************************************************************************/

/**
 * @brief 初始化单个GPIO
 */
void Hw_SingleGpioInit(const ST_GPIO_CFG_ITEM *pstGpioCfgItem)
{
    if (pstGpioCfgItem == (const ST_GPIO_CFG_ITEM *)0)
    {
        return;
    }

    EALLOW;
    Hw_GpioApplyConfig(pstGpioCfgItem);
    EDIS;
}

/**
 * @brief 根据配置表初始化一组GPIO
 */
void Hw_GpioInit(const ST_GPIO_CFG *pstGpioCfg)
{
    Uint16 u16Index;

    if (pstGpioCfg == (const ST_GPIO_CFG *)0)
    {
        return;
    }

    if (pstGpioCfg->pstGpioCfgItem == (ST_GPIO_CFG_ITEM *)0)
    {
        return;
    }

    EALLOW;

    for (u16Index = 0U; u16Index < pstGpioCfg->u16ItemNum; u16Index++)
    {
        Hw_GpioApplyConfig(&pstGpioCfg->pstGpioCfgItem[u16Index]);
    }

    EDIS;
}

/**
 * @brief 读取GPIO引脚状态
 */
Uint32 Hw_GpioReadPin(Uint32 u32Pin)
{
    volatile Uint32 *pu32GpioDataAddr;
    Uint32 u32PinBit;

    if (u32Pin > 168U)
    {
        return 0U;
    }

    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);
    u32PinBit = u32Pin % 32U;

    return ((pu32GpioDataAddr[HW_OFFSET_GPIODAT] >> u32PinBit) & 0x01UL);
}

/**
 * @brief 设置GPIO输出电平
 */
#pragma CODE_SECTION(Hw_GpioWritePin, ".TI.ramfunc")
void Hw_GpioWritePin(Uint32 u32Pin, Uint32 u32GpioValue)
{
    volatile Uint32 *pu32GpioDataAddr;
    Uint32 u32PinMask;

    if (u32Pin > 168U)
    {
        return;
    }

    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);
    u32PinMask = (Uint32)1UL << (u32Pin % 32U);

    if (u32GpioValue == 0U)
    {
        pu32GpioDataAddr[HW_OFFSET_GPIOCLEAR] = u32PinMask;
    }
    else
    {
        pu32GpioDataAddr[HW_OFFSET_GPIOSET] = u32PinMask;
    }
}

/**
 * @brief 将GPIO置为高电平
 */
void Hw_GpioSetPin(Uint32 u32Pin)
{
    volatile Uint32 *pu32GpioDataAddr;
    Uint32 u32PinMask;

    if (u32Pin > 168U)
    {
        return;
    }

    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);
    u32PinMask = (Uint32)1UL << (u32Pin % 32U);

    pu32GpioDataAddr[HW_OFFSET_GPIOSET] = u32PinMask;
}

/**
 * @brief 将GPIO置为低电平
 */
void Hw_GpioClearPin(Uint32 u32Pin)
{
    volatile Uint32 *pu32GpioDataAddr;
    Uint32 u32PinMask;

    if (u32Pin > 168U)
    {
        return;
    }

    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);
    u32PinMask = (Uint32)1UL << (u32Pin % 32U);

    pu32GpioDataAddr[HW_OFFSET_GPIOCLEAR] = u32PinMask;
}

/**
 * @brief 翻转GPIO输出电平
 */
void Hw_GpioTogglePin(Uint32 u32Pin)
{
    volatile Uint32 *pu32GpioDataAddr;
    Uint32 u32PinMask;

    if (u32Pin > 168U)
    {
        return;
    }

    pu32GpioDataAddr = Hw_GpioGetDataBase(u32Pin);
    u32PinMask = (Uint32)1UL << (u32Pin % 32U);

    pu32GpioDataAddr[HW_OFFSET_GPIOTOGGLE] = u32PinMask;
}

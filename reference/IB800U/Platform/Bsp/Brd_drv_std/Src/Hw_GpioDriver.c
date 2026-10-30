/**
 * @file Hw_GpioDriver.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <Hw_GpioDriver.h>


#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief 用于初始化单个GPIO
 * 
 * @param pstGpioCfgTable :入参是一个ST_GPIO_CFG_TABLE结构体对象
 */
void Hw_SingleGpioInit(ST_GPIO_CFG_ITEM *pstGpioCfgTable)
{
    Uint32 u32PinBit = 0;
    volatile Uint32 *pu32GpioAddr = NULL;
    volatile Uint32 *pu32GpioQSelAddr = NULL;
    volatile Uint32 *pu32GpioMuxAddr = NULL;
    volatile Uint32 *pu32GpioGMuxAddr = NULL;
    Uint32 u32RegSel = 0;
    Uint32 u32GpioIndex = 0;
    Uint32 u32GpioGroup = 0;
    Uint32 u32GpioFunc = 0;

    EALLOW;

    u32GpioIndex = pstGpioCfgTable->GPIONum;     // 获取引脚号
    u32GpioGroup = u32GpioIndex / 32U;                       // 获取GPIO组
    u32RegSel = (u32GpioIndex % 32U) / 16U;                   // 标识是同一组的前16个还是后16个
    u32PinBit = u32GpioIndex % 32U;                  // 计算引脚掩码

    pu32GpioAddr = (Uint32 *)GPIO_CTRLREGS_BASE + u32GpioGroup * STEP_GPIOCTRL_REGS;
    // 通过标识选择寄存器1还是寄存器2
    if (u32RegSel == 0)
    {
        pu32GpioQSelAddr = pu32GpioAddr + OFFSET_GPIOQSEL1;
        pu32GpioGMuxAddr = pu32GpioAddr + OFFSET_GPIOGMUX1;
        pu32GpioMuxAddr  = pu32GpioAddr + OFFSET_GPIOMUX1;

    }
    else
    {
        pu32GpioQSelAddr = pu32GpioAddr + OFFSET_GPIOQSEL2;
        pu32GpioGMuxAddr = pu32GpioAddr + OFFSET_GPIOGMUX2;
        pu32GpioMuxAddr  = pu32GpioAddr + OFFSET_GPIOMUX2;
    }

    // 如果配置成其他外设情况下，引脚滤波需要配置成异步模式,其他情况配置成同步即可
    if (pstGpioCfgTable->MuxBit != GPIO)
    {
        *pu32GpioQSelAddr = *pu32GpioQSelAddr | ((Uint32)3U << ((u32GpioIndex % 16) * 2U));
    }
    else
    {
        *pu32GpioQSelAddr  = *pu32GpioQSelAddr & (~(3U << ((u32GpioIndex % 16) * 2U)));
    }

    // 复用功能选择组GpioGMux
    u32GpioFunc = ((Uint16)pstGpioCfgTable->MuxBit >> 2) & 0x0003;
    *pu32GpioGMuxAddr = *pu32GpioGMuxAddr & (~((Uint32)3U << ((u32GpioIndex % 16) * 2U)));        // 先复位复用功能选择组
    *pu32GpioGMuxAddr = *pu32GpioGMuxAddr | (u32GpioFunc << ((u32GpioIndex % 16U) * 2U)); // 再设置复用功能选择组

    // 复用功能选择子GpioMux
    u32GpioFunc = ((Uint16)pstGpioCfgTable->MuxBit) & 0x0003;
    *pu32GpioMuxAddr = *pu32GpioMuxAddr & (~((Uint32)3U << ((u32GpioIndex % 16) * 2U)));          // 先复位复用功能选择子
    *pu32GpioMuxAddr = *pu32GpioMuxAddr | (u32GpioFunc << ((u32GpioIndex % 16U) * 2U));   // 再设置复用功能选择子

    // 使能上拉
    pu32GpioAddr[OFFSET_GPIOPUD] = pu32GpioAddr[OFFSET_GPIOPUD] & (~((Uint32)1 << u32PinBit));

    Hw_GpioWritePin(pstGpioCfgTable->GPIONum, pstGpioCfgTable->DefaultValue);

    // 配置引脚方向
    u32GpioFunc = ((Uint16)pstGpioCfgTable->DirBit) & 0x0001U;
    pu32GpioAddr[OFFSET_GPIODIR] = pu32GpioAddr[OFFSET_GPIODIR] & (~((Uint32)1 << u32PinBit));                               // 复位方向
    pu32GpioAddr[OFFSET_GPIODIR] = pu32GpioAddr[OFFSET_GPIODIR] | (u32GpioFunc << u32PinBit);                        // 设置方向

    // 配置开漏输出
    u32GpioFunc = ((Uint16)pstGpioCfgTable->EnableOD) & 0x0001U;
    pu32GpioAddr[OFFSET_GPIODR] = pu32GpioAddr[OFFSET_GPIODR] & (~((Uint32)1 << u32PinBit));
    pu32GpioAddr[OFFSET_GPIODR] = pu32GpioAddr[OFFSET_GPIODR] | (u32GpioFunc << u32PinBit);

    EDIS;
}


/**
 * @brief 用于初始化一组GPIO，通常用于参数初始化阶段
 * 
 * @param pstGpioCfg :入参是一组ST_GPIO_CFG_TABLE结构体对象的集合
 */
void Hw_GpioInit(ST_GPIO_CFG *pstGpioCfg) // 这里需要传入初始化IO的表
{
    Uint16 i = 0;

//    EALLOW

    for (i = 0; i < pstGpioCfg->u16ItemNum; i++)
    {
        Hw_SingleGpioInit(&pstGpioCfg->pstGpioCfgItem[i]);

//        // 配置中断
//        if (pstGpioCfg->stGpioCfgTable->EnableInt)
//        {
//
//        }
        // 把配置中断独立出去，先完成中断的相关驱动，再配置一张IO的中断配置表，然后在这里进行配置
    }

//    EDIS
}


/**
 * @brief 读取一个IO引脚的状态
 * 
 * @param u32Pin :引脚号
 * @return Uint32 :引脚状态
 */
Uint32 Hw_GpioReadPin(Uint32 u32Pin)
{
    Uint32 u32PinMask = 0;
    volatile Uint32 *pu32GpioAddr = NULL;
    Uint32 u32GpioGroup = 0;

    u32GpioGroup = u32Pin / 32U;                       // 获取GPIO组
    u32PinMask = (Uint32)1U << (u32Pin % 32U);                  // 计算引脚掩码
    u32PinMask = u32PinMask;

    pu32GpioAddr = (Uint32 *)GPIO_DATAREGS_BASE + u32GpioGroup * STEP_GPIODAT_REGS;

    return ((pu32GpioAddr[OFFSET_GPIODAT] >> (u32Pin % 32U)) & (Uint32) 0x1U);
}


/**
 * @brief 设置一个引脚的输出电平
 * 
 * @param u32Pin :引脚号
 * @param u32GpioValue :引脚的输出电平，参数取值 0 or 1
 */
#pragma CODE_SECTION(Hw_GpioWritePin, ".ISR.ramfunc");
void Hw_GpioWritePin(Uint32 u32Pin, Uint32 u32GpioValue)
{
    Uint32 u32PinMask = 0;
    volatile Uint32 *pu32GpioAddr = NULL;
    Uint32 u32GpioGroup = 0;

    u32GpioGroup = u32Pin / 32U;                       // 获取GPIO组
    u32PinMask = (Uint32)1U << (u32Pin % 32U);                  // 计算引脚掩码

    pu32GpioAddr = (Uint32 *)GPIO_DATAREGS_BASE + u32GpioGroup * STEP_GPIODAT_REGS;

    if (u32GpioValue == 0U)
    {
        pu32GpioAddr[OFFSET_GPIOCLEAR] = u32PinMask;
    }
    else
    {
        pu32GpioAddr[OFFSET_GPIOSET] = u32PinMask;
    }

}


/**
 * @brief 反转引脚的电平
 * 
 * @param u32Pin :引脚号
 */
void Hw_GpioTogglePin(Uint32 u32Pin)
{
    Uint32 u32PinMask = 0;
    volatile Uint32 *pu32GpioAddr = NULL;
    Uint32 u32GpioGroup = 0;

    u32GpioGroup = u32Pin / 32U;                        // 获取GPIO组
    u32PinMask = (Uint32)1U << (u32Pin % 32U);                  // 计算引脚掩码

    pu32GpioAddr = (Uint32 *)GPIO_DATAREGS_BASE + u32GpioGroup * STEP_GPIODAT_REGS;

    pu32GpioAddr[OFFSET_GPIOTOGGLE] = u32PinMask;
}

#ifdef __cplusplus
    }
#endif

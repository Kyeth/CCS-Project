/**
 * @file Hw_GpioDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_GPIODRIVER_H
#define __HW_GPIODRIVER_H

#include "f28003x_device.h"

// GPIO寄存器起始地址
#define GPIO_CTRLREGS_BASE          0x00007C00
#define GPIO_DATAREGS_BASE          0x00007F00
#define GPIO_DATA_READ_REGS_BASE    0x00007F80

// 寄存器偏移
#define STEP_GPIOCTRL_REGS      (0x40 / 2U)
#define OFFSET_GPIOCTRL         (0x00 / 2U)
#define OFFSET_GPIOQSEL1        (0x02 / 2U)
#define OFFSET_GPIOQSEL2        (0x04 / 2U)
#define OFFSET_GPIOMUX1         (0x06 / 2U)
#define OFFSET_GPIOMUX2         (0x08 / 2U)
#define OFFSET_GPIODIR          (0x0A / 2U)
#define OFFSET_GPIOPUD          (0x0C / 2U)
#define OFFSET_GPIOINV          (0x10 / 2U)
#define OFFSET_GPIODR           (0x12 / 2U)
#define OFFSET_GPIOAMSEL        (0x14 / 2U)
#define OFFSET_GPIOGMUX1        (0x20 / 2U)
#define OFFSET_GPIOGMUX2        (0x22 / 2U)
#define OFFSET_GPIOCSEL1        (0x28 / 2U)
#define OFFSET_GPIOCSEL2        (0x2A / 2U)
#define OFFSET_GPIOCSEL3        (0x2C / 2U)
#define OFFSET_GPIOCSEL4        (0x2E / 2U)
#define OFFSET_GPIOLOCK         (0x3C / 2U)
#define OFFSET_GPIOCR           (0x3E / 2U)

#define STEP_GPIODAT_REGS       (0x08 / 2U)
#define OFFSET_GPIODAT          (0x00 / 2U)
#define OFFSET_GPIOSET          (0x02 / 2U)
#define OFFSET_GPIOCLEAR        (0x04 / 2U)
#define OFFSET_GPIOTOGGLE       (0x06 / 2U)

#ifdef __cplusplus
    extern "C" {
#endif



// GPIO 功能定义这里改成宏会比较合适
enum GPIO_FUNC
{
    GPIO = 0,   // 通用IO
    PERIPH1 = 1,    // 外设1
    PERIPH2 = 2,    // 外设2
    PERIPH3 = 3,    // 外设3
    PERIPH4 = 5,    // 外设4
    PERIPH5 = 6,    // 外设5
    PERIPH6 = 7,    // 外设6
    PERIPH7 = 9,    // 外设7
    PERIPH8 = 10,    // 外设8
    PERIPH9 = 11,    // 外设9
    PERIPH10 = 13,    // 外设10
    PERIPH11 = 14,    // 外设11
    PERIPH12 = 15,    // 外设12
};


// GPIO 方向定义
enum GPIO_DIR
{
    DIR_DI = 0, // 输入
    DIR_DO      // 输出
};


// GPIO 电平
enum GPIO_LEVEL
{
    GPIO_LOW = 0,   // 低电平
    GPIO_HIGN       // 高电平
};


// GPIO 方向定义
enum GPIO_ABLE
{
    GPIO_OD_DISABEL = 0,   // 禁止
    GPIO_OD_ENABEL         // 使能
};

typedef struct
{
    Uint16 GPIONum          :8;     // IO号
    Uint16 MuxBit           :4;     // 复用功能选择
    Uint16 DirBit           :1;     // 方向，0：输入，1：输出
    Uint16 DefaultValue     :1;     // 默认电平，0：低电平，1：高电平
    Uint16 EnableOD         :1;     // 使能开漏输出，0：禁止，1：使能
//    Uint16 EnableInt        :1;     // 使能外部中断，0：禁止，1：使能   配置中断这个待定
}ST_GPIO_CFG_ITEM;

typedef struct
{
    ST_GPIO_CFG_ITEM *pstGpioCfgItem;
    Uint16 u16ItemNum;
}ST_GPIO_CFG;

//ST_GPIO_CFG stGpioCfg = {};

/**
 * @brief 用于初始化单个GPIO
 * 
 * @param pstGpioCfgTable :入参为一个ST_GPIO_CFG_TABLE结构体对象
 */
extern void Hw_SingleGpioInit(ST_GPIO_CFG_ITEM *pstGpioCfgTable);


/**
 * @brief 用于初始化一组GPIO，通常用于参数初始化阶段
 * 
 * @param pstGpioCfg :入参是一组ST_GPIO_CFG_TABLE结构体对象的集合
 */
extern void Hw_GpioInit(ST_GPIO_CFG *pstGpioCfg);


/**
 * @brief 读取一个IO引脚的状态
 * 
 * @param u32Pin :引脚号
 * @return Uint32 :引脚状态
 */
extern Uint32 Hw_GpioReadPin(Uint32 u32Pin);


/**
 * @brief 设置一个引脚的输出电平
 * 
 * @param u32Pin :引脚号
 * @param u32GpioValue :引脚的输出电平，参数取值 0 or 1
 */
extern void Hw_GpioWritePin(Uint32 u32Pin, Uint32 u32GpioValue);


/**
 * @brief 反转引脚的电平
 * 
 * @param u32Pin :引脚号
 */
extern void Hw_GpioTogglePin(Uint32 u32Pin);

#ifdef __cplusplus
    }
#endif

#endif

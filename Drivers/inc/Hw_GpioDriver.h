/**
 * @file Hw_GpioDriver.h
 * @author yetingheng (004182)
 * @brief TMS320F2837xS GPIO底层驱动
 * @version 0.3
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef HW_GPIO_DRIVER_H
#define HW_GPIO_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * GPIO寄存器基地址
 ******************************************************************************/
#define HW_GPIO_CTRLREGS_BASE              0x00007C00UL
#define HW_GPIO_DATAREGS_BASE              0x00007F00UL

/******************************************************************************
 * GPIO控制寄存器偏移（以16位字为单位）
 ******************************************************************************/
#define HW_STEP_GPIOCTRL_REGS              (0x40U / 2U)

#define HW_OFFSET_GPIOCTRL                 (0x00U / 2U)
#define HW_OFFSET_GPIOQSEL1                (0x02U / 2U)
#define HW_OFFSET_GPIOQSEL2                (0x04U / 2U)
#define HW_OFFSET_GPIOMUX1                 (0x06U / 2U)
#define HW_OFFSET_GPIOMUX2                 (0x08U / 2U)
#define HW_OFFSET_GPIODIR                  (0x0AU / 2U)
#define HW_OFFSET_GPIOPUD                  (0x0CU / 2U)
#define HW_OFFSET_GPIOINV                  (0x10U / 2U)
#define HW_OFFSET_GPIOODR                  (0x12U / 2U)
#define HW_OFFSET_GPIOAMSEL                (0x14U / 2U)
#define HW_OFFSET_GPIOGMUX1                (0x20U / 2U)
#define HW_OFFSET_GPIOGMUX2                (0x22U / 2U)
#define HW_OFFSET_GPIOCSEL1                (0x28U / 2U)
#define HW_OFFSET_GPIOCSEL2                (0x2AU / 2U)
#define HW_OFFSET_GPIOCSEL3                (0x2CU / 2U)
#define HW_OFFSET_GPIOCSEL4                (0x2EU / 2U)
#define HW_OFFSET_GPIOLOCK                 (0x3CU / 2U)
#define HW_OFFSET_GPIOCR                   (0x3EU / 2U)

/******************************************************************************
 * GPIO数据寄存器偏移
 ******************************************************************************/
#define HW_STEP_GPIODAT_REGS               (0x08U / 2U)

#define HW_OFFSET_GPIODAT                  (0x00U / 2U)
#define HW_OFFSET_GPIOSET                  (0x02U / 2U)
#define HW_OFFSET_GPIOCLEAR                (0x04U / 2U)
#define HW_OFFSET_GPIOTOGGLE               (0x06U / 2U)

/******************************************************************************
 * GPIO复用功能编码定义
 *
 * 编码规则：(GPyGMUXn << 2) | GPyMUXn
 *
 * F28374S数据手册GPIO Mux表共8种有效组合，命名与表格列标题一致：
 *
 *   列标题    GMUX   MUX    编码       宏名
 *   --------  -----  -----  ---------  ---------
 *   0/4/8/12  00     00     0x00       HW_GPIO_FUNC_GPIO
 *   1         00     01     0x01       HW_GPIO_FUNC_PERIPH1
 *   2         00     10     0x02       HW_GPIO_FUNC_PERIPH2
 *   3         00     11     0x03       HW_GPIO_FUNC_PERIPH3
 *   5         01     01     0x05       HW_GPIO_FUNC_PERIPH5
 *   6         01     10     0x06       HW_GPIO_FUNC_PERIPH6
 *   7         01     11     0x07       HW_GPIO_FUNC_PERIPH7
 *   15        11     11     0x0F       HW_GPIO_FUNC_PERIPH15
 *
 * 注意：其余组合为保留值，不可使用。
 ******************************************************************************/
enum HW_GPIO_FUNC
{
    HW_GPIO_FUNC_GPIO      = 0x00,   /* GMUX=00, MUX=00 — 通用IO */
    HW_GPIO_FUNC_PERIPH1   = 0x01,   /* GMUX=00, MUX=01 — 列1复用 */
    HW_GPIO_FUNC_PERIPH2   = 0x02,   /* GMUX=00, MUX=10 — 列2复用 */
    HW_GPIO_FUNC_PERIPH3   = 0x03,   /* GMUX=00, MUX=11 — 列3复用 */
    HW_GPIO_FUNC_PERIPH5   = 0x05,   /* GMUX=01, MUX=01 — 列5复用 */
    HW_GPIO_FUNC_PERIPH6   = 0x06,   /* GMUX=01, MUX=10 — 列6复用 */
    HW_GPIO_FUNC_PERIPH7   = 0x07,   /* GMUX=01, MUX=11 — 列7复用 */
    HW_GPIO_FUNC_PERIPH15  = 0x0F    /* GMUX=11, MUX=11 — 列15复用 */
};

/******************************************************************************
 * 复用功能宏定义（针对EP15K项目实际用到的引脚）
 *
 * 以下宏仅对EP15K项目实际使用的引脚有效，不是全芯片通用规则。
 * 若要将某功能迁移到其他引脚，必须先对照F28374S数据手册确认目标引脚
 * 在该功能下的列号，再决定是否仍使用该宏。
 *
 * 涉及引脚范围：
 *   HW_GPIO_MUX_GPIO  → 所有普通IO引脚
 *   HW_GPIO_MUX_PWM   → 所有EPWMxA/EPWMxB输出引脚
 *   HW_GPIO_MUX_CAN   → GPIO36 / GPIO37 / GPIO38 / GPIO39
 ******************************************************************************/

/* -------- 统一的功能宏 -------- */

#define HW_GPIO_MUX_GPIO         HW_GPIO_FUNC_GPIO        /* 普通GPIO */
#define HW_GPIO_MUX_PWM          HW_GPIO_FUNC_PERIPH1     /* ePWM输出（列1） */
#define HW_GPIO_MUX_CAN          HW_GPIO_FUNC_PERIPH6     /* CAN收发（列6，GPIO36~39） */
#define HW_GPIO_MUX_SCI          HW_GPIO_FUNC_PERIPH6     /* SCI收发（列6，GPIO48/49/89/90/93/94） */
#define HW_GPIO_MUX_SPI          HW_GPIO_FUNC_PERIPH1     /* SPI收发（列1，GPIO54~57） */


/******************************************************************************
 * GPIO方向定义
 ******************************************************************************/
enum HW_GPIO_DIR
{
    HW_DIR_DI = 0,         /* 输入 */
    HW_DIR_DO              /* 输出 */
};

/******************************************************************************
 * GPIO电平定义
 ******************************************************************************/
enum HW_GPIO_LEVEL
{
    HW_GPIO_LOW = 0,       /* 低电平 */
    HW_GPIO_HIGH           /* 高电平 */
};

/******************************************************************************
 * GPIO内部上拉定义
 ******************************************************************************/
enum HW_GPIO_PUD
{
    HW_GPIO_PUD_DISABLE = 0,    /* 禁止 */
    HW_GPIO_PUD_ENABLE          /* 使能 */
};

/******************************************************************************
 * GPIO开漏输出定义
 ******************************************************************************/
enum HW_GPIO_ABLE
{
    HW_GPIO_OD_DISABLE = 0,    /* 禁止 */
    HW_GPIO_OD_ENABLE          /* 使能 */
};

/******************************************************************************
 * GPIO输入限定模式
 ******************************************************************************/
enum HW_GPIO_QUAL
{
    HW_GPIO_QUAL_SYNC      = 0,    /* 与SYSCLKOUT同步 */
    HW_GPIO_QUAL_3SAMPLE   = 1,    /* 3次采样限定 */
    HW_GPIO_QUAL_6SAMPLE   = 2,    /* 6次采样限定 */
    HW_GPIO_QUAL_ASYNC     = 3     /* 异步输入 */
};

/******************************************************************************
 * 单个GPIO配置项
 *
 * 字段顺序：
 * {
 *     GPIO编号,
 *     复用功能,
 *     方向,
 *     默认输出电平,
 *     内部上拉,
 *     开漏输出,
 *     输入限定模式
 * }
 *
 * 说明：u16Mux 字段填写 HW_GPIO_FUNC 枚举值或对应的 GPIOxx_功能名 宏。
 ******************************************************************************/
typedef struct
{
    Uint16 u16Pin;
    Uint16 u16Mux;
    Uint16 u16Dir;
    Uint16 u16DefaultLevel;
    Uint16 u16Pullup;
    Uint16 u16OpenDrain;
    Uint16 u16QualMode;
} ST_GPIO_CFG_ITEM;

/******************************************************************************
 * GPIO配置表
 ******************************************************************************/
typedef struct
{
    ST_GPIO_CFG_ITEM *pstGpioCfgItem;
    Uint16 u16ItemNum;
} ST_GPIO_CFG;

/******************************************************************************
 * 函数声明
 ******************************************************************************/

/**
 * @brief 初始化单个GPIO
 */
extern void Hw_SingleGpioInit(const ST_GPIO_CFG_ITEM *pstGpioCfgItem);

/**
 * @brief 根据配置表初始化一组GPIO
 */
extern void Hw_GpioInit(const ST_GPIO_CFG *pstGpioCfg);

/**
 * @brief 读取GPIO引脚输入状态
 */
extern Uint32 Hw_GpioReadPin(Uint32 u32Pin);

/**
 * @brief 设置GPIO输出电平
 */
extern void Hw_GpioWritePin(Uint32 u32Pin, Uint32 u32GpioValue);

/**
 * @brief 将GPIO置为高电平
 */
extern void Hw_GpioSetPin(Uint32 u32Pin);

/**
 * @brief 将GPIO置为低电平
 */
extern void Hw_GpioClearPin(Uint32 u32Pin);

/**
 * @brief 翻转GPIO输出电平
 */
extern void Hw_GpioTogglePin(Uint32 u32Pin);

#ifdef __cplusplus
}
#endif

#endif /* HW_GPIO_DRIVER_H */

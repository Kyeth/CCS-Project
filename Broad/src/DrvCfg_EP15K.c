/**
 * @file DrvCfg_EP15K.c
 * @brief EP15K_BOOT 板级配置表（时钟 / GPIO / SCI / CPU Timer1）
 * @version 1.0
 * @date 2026-11-10
 *
 * @note 取值与 APP 工程 Broad/src/DrvCfg_EP15K.c 完全一致，只删掉 BOOT 不使用的
 *       PWM / ADC / CLA / CAN / Xbar 配置表。BOOT 只接管：驱动使能、SCI-A 收发、
 *       LED 与心跳定时器，其余引脚保持上电默认（不驱动功率级）。
 */
#include "DrvCfg_EP15K.h"
#include "F2837xS_Examples.h"   /* 时钟/定时器枚举：XTAL_OSC、IMULT_40 … */

/* 时钟：10 MHz 晶振，IMULT_40 / FMULT_0 / PLLCLK_BY_2 → 200 MHz */
const ST_CLOCK_CFG stClockCfg =
{
    XTAL_OSC, IMULT_40, FMULT_0, PLLCLK_BY_2,
    1U, 2U, 10000000UL, 200000000UL, 100000000UL, 50000000UL
};

/* stGpioCfgItem：只保留 BOOT 需要的引脚
 *   88  芯片驱动使能（上电即低，功率级不使能）
 *   75  LED
 *   48/49 SCI-A TX/RX（升级通道）
 */
ST_GPIO_CFG_ITEM stGpioCfgItem[] =
{
    /* 配置格式：{GPIO编号, 复用功能, 方向, 初始电平, 内部上拉, 开漏, 输入限定模式} */
    {88, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_PUD_DISABLE, HW_GPIO_OD_DISABLE, HW_GPIO_QUAL_SYNC},  // 芯片驱动使能（1:开 0:关）。
    {75, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_PUD_DISABLE, HW_GPIO_OD_DISABLE, HW_GPIO_QUAL_SYNC},  // LED灯（1:开 0:关）。
    {48, HW_GPIO_MUX_SCI,  HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_PUD_DISABLE, HW_GPIO_OD_DISABLE, HW_GPIO_QUAL_ASYNC}, // SCI-A发送信号SCITXDA。
    {49, HW_GPIO_MUX_SCI,  HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_PUD_ENABLE, HW_GPIO_OD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-A接收信号SCIRXDA。
};

/* stGpioCfg */
ST_GPIO_CFG stGpioCfg =
{
    stGpioCfgItem,
    sizeof(stGpioCfgItem) / sizeof(stGpioCfgItem[0])
};

/* stSciCfgItem：与 APP 一致的三路表（端口层按这张表建三路端口）。
 * BOOT 只用 SCI-A 收升级；C/D 只初始化不驱动引脚，保持端口层结构不变。 */
const ST_SCI_CFG_ITEM stSciCfgItem[] =
{
    {HW_SCI_ID_A,  19200UL, 0U, 0U,  8U},
    {HW_SCI_ID_C, 115200UL, 0U, 0U,  8U},
    {HW_SCI_ID_D,  38400UL, 0U, 0U,  8U},
};

/* stSciCfg */
ST_SCI_CFG stSciCfg =
{
    &stSciCfgItem[0],
    sizeof(stSciCfgItem) / sizeof(stSciCfgItem[0]),
    50000000UL
};

/* stSysCpuTimer1: 每 100 us 一次硬件中断 ((19999 + 1) / 200 MHz)，
   u16TickDivCoef = 10 再分频出 1 ms 软件心跳。 */
volatile ST_SYS_TIMER stSysCpuTimer1 =
{
    (Uint16)HW_CPUTIMER_ID1,        /* u16TimerId     */
    0U,                             /* u16Prescaler   */
    1U,                             /* bEnableInt     */
    1U,                             /* bEmuStop       */
    19999UL,                        /* u32Period      */
    0UL,                            /* u32Tick        */
    0U,                             /* u16TickDivCnt  */
    10U                             /* u16TickDivCoef */
};

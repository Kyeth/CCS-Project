/**
 * @file DrvCfg_EP15K.h
 * @brief EP15K_BOOT 板级配置对象声明（只保留 BOOT 需要的时钟/GPIO/SCI/定时器）
 * @version 1.0
 * @date 2026-11-10
 *
 * @note 与 APP 工程同名的 header 保持同一套类型，但只声明 BOOT 使用的四个对象。
 *       ADC/PWM/CLA/CAN/Xbar 的配置对象不在本工程内。
 */
#ifndef DRVCFG_EP15K_H
#define DRVCFG_EP15K_H

#include "Hw_GpioDriver.h"
#include "Hw_ClockDriver.h"
#include "Hw_CpuTimerDriver.h"
#include "Hw_SciDriver.h"

extern ST_GPIO_CFG stGpioCfg;
extern const ST_CLOCK_CFG stClockCfg;
extern ST_SCI_CFG stSciCfg;
extern volatile ST_SYS_TIMER stSysCpuTimer1;

#endif

/**
 * @file Schedule.h
 * @brief 主机桩测试用的调度器垫片（只声明 PortTaskRegister() 用到的入口）
 * @version 0.1
 * @date 2026-11-06
 */

#ifndef SCHEDULE_SHIM_H
#define SCHEDULE_SHIM_H

#include "F2837xS_device.h"

Uint16 Task_Start(void (*pfnCallback)(void), Uint16 u16PeriodMs, Uint16 u16OffsetMs);

#endif /* SCHEDULE_SHIM_H */

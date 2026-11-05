/**
 * @file Hw_TimeBase.h
 * @brief 主机桩测试用的时间基准垫片
 */

#ifndef HW_TIME_BASE_SHIM_H
#define HW_TIME_BASE_SHIM_H

#include "F2837xS_device.h"

Uint16 Hw_TimeBaseInit(Uint32 u32SysclkHz);
Uint32 Hw_TimeBaseTicks(void);
Uint16 Hw_TimeBaseIsReady(void);

#endif /* HW_TIME_BASE_SHIM_H */

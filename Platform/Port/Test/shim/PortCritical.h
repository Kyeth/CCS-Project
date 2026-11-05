/**
 * @file PortCritical.h
 * @brief 主机桩测试用的临界区垫片（钩子由测试文件自己提供）
 * @version 0.1
 * @date 2026-11-05
 */

#ifndef PORT_CRITICAL_SHIM_H
#define PORT_CRITICAL_SHIM_H

#include "F2837xS_device.h"

typedef Uint16 ST_PORT_IRQ_STATE;

ST_PORT_IRQ_STATE PortEnterCriticalReal(void);
void PortExitCriticalReal(ST_PORT_IRQ_STATE u16State);

#endif /* PORT_CRITICAL_SHIM_H */

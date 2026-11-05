/**
 * @file PortCritical.h
 * @author yetingheng (004182)
 * @brief 端口层真实临界区：保存完整 ST1 并在退出时整字恢复
 * @version 0.1
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 为什么不用深度计数器：
 *   计数器只能表达"我嵌套了几层"，无法表达"进入时中断本来就是关的"。
 *   在 100 us 心跳里（ISR 已经关中断）进入、退出时若按计数器开中断，
 *   会在 ISR 中途提前 EINT。保存/恢复整字 ST1（含 INTM 与 DBGM）才准确。
 */

#ifndef PORT_CRITICAL_H
#define PORT_CRITICAL_H

#include "F2837xS_device.h"

typedef Uint16 ST_PORT_IRQ_STATE;

/* 保存当前 ST1 并关中断（INTM/DBGM），返回值必须原样交给退出函数。 */
ST_PORT_IRQ_STATE PortEnterCriticalReal(void);

/* 按保存值整字恢复 ST1；不得用固定的 EINT/DINT 代替。 */
void PortExitCriticalReal(ST_PORT_IRQ_STATE u16State);

#endif

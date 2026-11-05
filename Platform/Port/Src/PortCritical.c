/**
 * @file PortCritical.c
 * @author yetingheng (004182)
 * @brief 端口层真实临界区实现（C28x）
 * @version 0.1
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 汇编语义（CGT 22.6.2.LTS 实测）：
 *   __disable_interrupts()  -> PUSH ST1 / SETC INTM, DBGM / MOV AL,*--SP
 *   __restore_interrupts(x) -> POP ST1
 * 即返回值是完整的 16 位 ST1，恢复时整字弹回，INTM 与 DBGM 同时回到原值。
 */

#ifndef PORT_CRITICAL_HOOKS

#include "PortCritical.h"
#include "interrupt.h"

ST_PORT_IRQ_STATE PortEnterCriticalReal(void)
{
    /* 不能只取低位当 INTM：DBGM 也要一起保存与恢复。 */
    return (ST_PORT_IRQ_STATE)__disable_interrupts();
}

void PortExitCriticalReal(ST_PORT_IRQ_STATE u16State)
{
    __restore_interrupts((uint16_t)u16State);
}

#endif /* !PORT_CRITICAL_HOOKS */

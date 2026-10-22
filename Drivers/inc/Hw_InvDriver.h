/**
 * @file Hw_InvDriver.h
 * @author yetingheng (004182)
 * @brief 三电平逆变桥驱动层（单极性控制，正/负半周由两个ePWM模块轮流斩波）
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 * @note 拓扑与开关定义（以R相为例）：
 *       竖管上管 S1 = EPWM1A，横管右管 S3 = EPWM1B（EPWM1 正半周斩波）
 *       竖管下管 S4 = EPWM4A，横管左管 S2 = EPWM4B（EPWM4 负半周斩波）
 *       正半周：EPWM1 斩波，EPWM4 静态（A=0 -> S4关，B=1 -> S2开）
 *       负半周：EPWM4 斩波，EPWM1 静态（A=0 -> S1关，B=1 -> S3开）
 *       静态态与"斩波占空比=0"的波形完全等价，故换相可在单拍内无缝完成。
 */

#ifndef HW_INV_DRIVER_H
#define HW_INV_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 占空比上限：为死区畸变与最小脉宽留余量 */
#define HW_INV_DUTY_MAX     (0.98f)

/* 换相安全阈值：低于该占空比认为斩波路已经等价于"中点钳位" */
#define HW_INV_DUTY_EPS     (0.002f)

typedef enum
{
    HW_INV_PHASE_R = 0U,
    HW_INV_PHASE_S,
    HW_INV_PHASE_T,
    HW_INV_PHASE_MAX
} E_HW_INV_PHASE;

/* 单相配置：正半周斩波模块 / 负半周斩波模块 / 载波周期计数 */
typedef struct
{
    Uint16 u16PosPwmId;     /* 正半周斩波：EPWMn，A=S1、B=S3 */
    Uint16 u16NegPwmId;     /* 负半周斩波：EPWMm，A=S4、B=S2 */
    Uint16 u16Period;       /* TBPRD，占空比换算用 */
} ST_INV_PHASE_CFG;

typedef struct
{
    const ST_INV_PHASE_CFG *pstPhaseCfg;
    Uint16 u16PhaseNum;
} ST_INV_CFG;

/* 初始化：三相全部压到中点钳位（A=0/B=1），占空比清零。 */
extern void Hw_InvInit(const ST_INV_CFG *pstInvCfg);

/* 下发单相占空比。
 * u16Positive != 0 表示正半周（用 u16PosPwmId 斩波）。
 * 内部完成：换相（单拍，异常时退化为两拍）、保持路钳位、占空比换算。 */
extern void Hw_InvSetDuty(Uint16 u16Phase, float fDuty, Uint16 u16Positive);

/* 单相封波：两路都压到中点钳位，占空比清零。 */
extern void Hw_InvStop(Uint16 u16Phase);

/* 三相封波。 */
extern void Hw_InvStopAll(void);

/* 读回当前斩波模块编号，供互锁与调试观察。 */
extern Uint16 Hw_InvGetChopPwm(Uint16 u16Phase);

/* 读回当前占空比。 */
extern float Hw_InvGetDuty(Uint16 u16Phase);

#ifdef __cplusplus
}
#endif

#endif /* HW_INV_DRIVER_H */

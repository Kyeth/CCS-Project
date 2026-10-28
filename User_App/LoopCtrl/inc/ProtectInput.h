/**
 * @file ProtectInput.h
 * @author yetingheng (004182)
 * @brief 保护判据源：把采样量翻译成 PROTECT_FLT_* 判据
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 判据源：把采样量翻译成保护框架的故障事件。
 *
 * 设计要点：
 * 1. 本模块只做判据，不做记录与动作，动作由 ProtectFault 统一处理；
 * 2. 判据按功能域分开，某个域出问题不牵连其它域；
 * 3. 阈值全部给宏，批 1.0 改为协议定值下发。
 */
#ifndef USER_APP_LOOPCTRL_INC_PROTECTINPUT_H_
#define USER_APP_LOOPCTRL_INC_PROTECTINPUT_H_

#include "ProtectFault.h"

/* 逆变输出判据 */
#define PROTECT_INV_VREF_VOLT       (230.0f)    /* 相电压参考值 [V] */
#define PROTECT_INV_OUT_DEV_RATIO   (0.10f)     /* 幅值不合规：偏离参考 10% */
#define PROTECT_INV_OUT_LOW_RATIO   (0.15f)     /* 有指令无输出：低于参考 15% */
#define PROTECT_INV_OUT_MOD_MIN     (0.10f)     /* 判定有指令的调制比下限 */
#define PROTECT_INV_OUT_NOOUT_MS    (50U)       /* 无输出确认时间 [ms] */

/* 逆变输出直流分量判据 */
#define PROTECT_INV_DC_OFS_MAX_A    (2.0f)      /* 直流分量上限 [A] */

void ProtectInputInit(void);

/**
 * @brief 判据扫描，10 ms 节拍调用，必须早于 ProtectRun10ms
 */
void ProtectInputRun10ms(void);

#endif /* USER_APP_LOOPCTRL_INC_PROTECTINPUT_H_ */

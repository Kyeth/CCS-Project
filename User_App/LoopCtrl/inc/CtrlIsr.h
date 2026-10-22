/**
 * @file CtrlIsr.h
 * @author yetingheng (004182)
 * @brief 20kHz 控制中断（CLA1_1 触发）：PLL 锁相、环路占空比下发、封波检测
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 * @note 触发链路：EPWM1 CTR=ZERO -> ADC SOCA -> ADCINT1 -> CLA Task1（滤波/归档）
 *                -> CLA1_1 中断（本文件）-> PLL + 环路 -> 写 CMPA -> 切 MUX 地址
 *       控制周期 Ts = 50us（20kHz），与 EPWM1 开关周期同源，抖动由硬件保证。
 */

#ifndef CTRL_ISR_H
#define CTRL_ISR_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 控制周期：20kHz */
#define CTRL_TS_SEC             (50.0e-6f)
#define CTRL_TWO_PI             (6.283185307f)
#define CTRL_PI                 (3.141592654f)
#define CTRL_TWO_PI_3           (2.094395102f)

/* SRF-PLL 参数（骨架值，实测后整定） */
/* KP/KI 基于归一化的相位误差（rad）：ζ=0.707、ωn=2π·20rad/s → KP=2ζωn≈180，KI=ωn²≈16000 */
#define CTRL_PLL_KP             (180.0f)
#define CTRL_PLL_KI             (16000.0f)

/* 电压幅值下限（V）：低于该值认为电网掉电，PLL 误差清零 */
#define CTRL_PLL_VMAG_MIN       (10.0f)
#define CTRL_PLL_FREQ_MIN       (40.0f)
#define CTRL_PLL_FREQ_MAX       (70.0f)

/* 采样标定：12位码值域 0~4095 对应 0~3V，零点 2047.5 */
#define CTRL_ADC_FULL           (4095.0f)
#define CTRL_ADC_MID            (2047.5f)
#define CTRL_ADC_16BIT_SCALE    (0.0625f)       /* ADCRESULT 左对齐，右移 4 位回到 12 位 */

#define CTRL_VOLT_RANGE         (462.69f)       /* 相电压/逆变电压：±462.69V <-> 0~3V */
#define CTRL_CUR_RANGE          (100.0f)        /* 逆变电流：±100A <-> 0~3V */
#define CTRL_CUR_RANGE_PV       (30.0f)         /* PV 电流：30A <-> 3V，零点 5/3V */
#define CTRL_CUR_OFFSET_PV      (1136.0f)       /* PV 电流零点码值 = 4095 * (5/3) / 3 */
#define CTRL_MUX2_VOLT_RANGE    (1033.0f)       /* MUX2（PV 电压）：1033V <-> 3V */
#define CTRL_MUX1_VOLT_RANGE    (563.64f)       /* MUX1（上/下半母线）：563.64V <-> 3V */
#define CTRL_VISO_VOLT_RANGE    (488.0f)        /* VISO：0~488V <-> 0~3V */

/* 保护阈值 */
#define CTRL_INV_CUR_LIMIT      (60.0f)         /* 逆变电流瞬时封波阈值 (A) */
#define CTRL_MOD_INDEX_MIN      (0.0f)
#define CTRL_MOD_INDEX_MAX      (0.90f)

/* CLA 心跳超时：连续 N 个控制周期 u16Seq 不变化即判定 CLA 停摆 */
#define CTRL_CLA_TIMEOUT_NUM    (20U)

typedef enum
{
    CTRL_FAULT_NONE = 0U,
    CTRL_FAULT_CLA_TIMEOUT = 1U,
    CTRL_FAULT_OVERCURRENT = 2U,
    CTRL_FAULT_MAX
} E_CTRL_FAULT;

/* 初始化：清状态、把 MUX 选到 PV1。需在 EINT 之前调用。 */
extern void CtrlIsrInit(void);

/* 控制中断主体：由 Isr_Cla1Task1 调用，必须保持定周期、无阻塞。 */
extern void CtrlIsrRun(void);

/* 调制比设定（0 ~ CTRL_MOD_INDEX_MAX），0 时三相占空比为 0、桥臂回到钳位。 */
extern void CtrlSetModIndex(float fModIndex);

/* 查询当前调制比 / 相位 / 电网频率 / 故障码 / 已运行拍数。 */
extern float  CtrlGetModIndex(void);
extern float  CtrlGetTheta(void);
extern float  CtrlGetGridFreq(void);
extern Uint16 CtrlGetFault(void);
extern Uint16 CtrlGetMuxAddr(void);
extern Uint16 CtrlGetCycleCnt(void);

/* 发波授权：0 = 只跑环路不发波（三相桥臂硬件钳位），1 = 允许调制输出。
 * 上电默认 0，必须显式调用 CtrlSetPwmEnable(1U) 才可能开通功率级。 */
extern void   CtrlSetPwmEnable(Uint16 u16Enable);
extern Uint16 CtrlGetPwmEnable(void);

/* 发波授权状态量：非 static，便于以后挂到 Modbus 点表里做软件互锁 */
extern volatile Uint16 gCtrlPwmEnable;

/* 观测影子量：控制中断每拍刷新，供软示波器（SoftOsc）按地址观测。
 * 控制环内部状态量本身是文件内 static，这里只做只读镜像。 */
extern volatile float  gCtrlObsThetaDeg;
extern volatile float  gCtrlObsGridFreq;
extern volatile float  gCtrlObsModIndex;
extern volatile Uint16 gCtrlObsFault;

#ifdef __cplusplus
}
#endif

#endif /* CTRL_ISR_H */

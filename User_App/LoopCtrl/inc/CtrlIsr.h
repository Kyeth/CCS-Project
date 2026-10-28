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

/* 采样标定：码值 -> 工程值的换算统一由 AdcCalib 模块负责（零点、系数、
 * 上电零漂检测都在那里）。控制环只调 AdcCalibInvCur() / AdcCalibGridVolt()
 * 这类取值接口，不再自己算系数；换机型/改量程只改 AdcCalib.c 的三张表。 */

/* 保护阈值 */
#define CTRL_INV_CUR_LIMIT      (60.0f)         /* 逆变电流瞬时封波阈值 (A) */
#define CTRL_MOD_INDEX_MIN      (0.0f)
#define CTRL_MOD_INDEX_MAX      (0.90f)

/* CLA 心跳超时：连续 N 个控制周期 u16Seq 不变化即判定 CLA 停摆 */
#define CTRL_CLA_TIMEOUT_NUM    (20U)

/* ---------------- 发波授权状态机（在 20kHz 控制中断内裁定） ----------------
 * CtrlSetPwmEnable() 只是一个"请求"，真正能否把占空比写进 ePWM 由这里决定：
 *   OFF      不发波，三相桥臂由 AQCSFRC 钳在中点
 *   WAIT     已收到请求，先等 CTRL_AUTH_WAIT_MAX 拍（10 ms）再放行
 *   ON       发波，实际调制比按 CTRL_AUTH_SLEW_UP 的斜率爬向请求值
 *   SOFT_OFF 请求已撤销，实际调制比按 CTRL_AUTH_SLEW_DN 降到 0 后再封波回 OFF
 * 出现故障（gCtrlFault != NONE）或零漂标定未完成时，无条件退回 OFF。 */
#define CTRL_AUTH_OFF               (0U)
#define CTRL_AUTH_WAIT              (1U)
#define CTRL_AUTH_ON                (2U)
#define CTRL_AUTH_SOFT_OFF          (3U)

#define CTRL_AUTH_WAIT_MAX          (200U)      /* 10 ms @ 20 kHz */
#define CTRL_AUTH_SLEW_UP           (0.0020f)   /* 调制比爬升：每秒 40 */
#define CTRL_AUTH_SLEW_DN           (0.0040f)   /* 调制比下降：每秒 80 */

/* CtrlGetAuthFlags() / gCtrlObsAuthFlags 的位定义 */
#define CTRL_AUTH_FLAG_FAULT        (0x0001U)   /* 控制故障封锁 */
#define CTRL_AUTH_FLAG_CALIB        (0x0002U)   /* 零漂标定未完成封锁 */


typedef enum
{
    CTRL_FAULT_NONE = 0U,
    CTRL_FAULT_CLA_TIMEOUT = 1U,
    CTRL_FAULT_OVERCURRENT = 2U,
    CTRL_FAULT_HW_OC = 3U,                  /* 硬件逆变过流（XINT1 漏桶判定） */
    CTRL_FAULT_SHORT = 4U,                  /* 硬件短路（XINT2 窗口判定） */
    CTRL_FAULT_TASK_STALL = 5U,             /* 后台任务停摆（200ms 活性窗口） */
    CTRL_FAULT_PROTECT = 6U,                /* 保护框架判定为停机级故障 */
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

/* 外部保护模块（10kHz 保护任务）置位/清除故障码：置位后控制中断下一拍立即封波。 */
extern void CtrlSetFault(Uint16 u16Fault);
extern void CtrlClearFault(void);
extern Uint16 CtrlGetCycleCnt(void);

/* 发波授权请求：0 = 撤销请求（实际调制比按斜率降到 0 后封波），1 = 请求发波。
 * 这只是"请求"：真正的放行由 20kHz 控制中断里的授权状态机裁定，它会额外要求
 * 无故障、零漂标定完成，并等 10 ms 后才按斜率把调制比爬上去。 */
extern void   CtrlSetPwmEnable(Uint16 u16Enable);
extern Uint16 CtrlGetPwmEnable(void);

/* 发波授权状态机的观测：状态 0=OFF 1=WAIT 2=ON 3=SOFT_OFF、实际调制比、封锁标志位。 */
extern Uint16 CtrlGetAuthState(void);
extern float  CtrlGetModActual(void);
extern Uint16 CtrlGetAuthFlags(void);

/* 发波授权请求量：非 static，便于挂到 Modbus 点表里做软件互锁 */
extern volatile Uint16 gCtrlPwmEnable;

/* 观测影子量：控制中断每拍刷新，供软示波器（SoftOsc）按地址观测。
 * 控制环内部状态量本身是文件内 static，这里只做只读镜像。 */
extern volatile float  gCtrlObsThetaDeg;
extern volatile float  gCtrlObsGridFreq;
extern volatile float  gCtrlObsModIndex;
extern volatile Uint16 gCtrlObsFault;
extern volatile Uint16 gCtrlObsAuthState;
extern volatile Uint16 gCtrlObsAuthFlags;
extern volatile float  gCtrlObsModActual;

#ifdef __cplusplus
}
#endif

#endif /* CTRL_ISR_H */

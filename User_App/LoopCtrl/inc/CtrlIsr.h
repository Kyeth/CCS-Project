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
#define CTRL_AUTH_FLAG_TEST         (0x0004U)   /* 台架发波模式 */

/* ---------------- 台架发波模式（只用来量驱动信号） ----------------
 * 1 = 上电即开环发波：不看零漂、不判故障、不做自检。
 *     INV 六路按 T 型三电平单极性发波（50 Hz，正负半周交替，正桥臂/负桥臂轮流斩波，
 *     另一桥臂钳中点），调制比 CTRL_TEST_MOD_INDEX；
 *     PV 四路固定 CTRL_TEST_PV_DUTY 占空比；半母线平衡桥释放。
 * 0 = 正常固件（发波授权 + 逆变/系统/PV 状态机）。
 * 置 1 时必须确认功率级已断开或已安全隔离。
 *
 * 【当前状态】台架驱动信号验证已完成，本宏 = 0，走正常固件路径：
 *   ProtectTask + InvSm/SysSm/PvSm 全部调度、TZ 硬件保护生效、
 *   StartSystem() 只置发波请求为 0，等状态机申请。
 *   台架代码全部保留，改回 1 即恢复上一版台架配置（PV 50% + INV 三电平单极性 50 Hz）。 */
#define CTRL_TEST_PWM_MODE          (0)     /* 台架验证完成，已还原为正常固件 */
#define CTRL_TEST_FREQ_HZ           (50.0f)
#define CTRL_TEST_MOD_INDEX         (0.30f)
#define CTRL_TEST_PV_DUTY           (0.50f)   /* 台架固定 50%：CMPA = 0.50×5000 = 2500 */
#define CTRL_TEST_TS_SEC            (100.0e-6f)  /* 台架模式控制周期：Timer1 10 kHz 驱动 */
#define CTRL_TEST_PV_PERIOD         (5000U)
/* 台架静态/动态选择：
 *   0 = 静态（仅 CtrlTestPwmStart 里一次性写死 CMPA，控制中断不再改逆变）
 *   1 = 动态（20 kHz 控制中断内 CtrlTestRun 开环正弦，依赖 ISR 链路）
 * 静态先保证"寄存器配得对、引脚一定出波"，排除 ISR 反复覆盖的干扰。 */
#define CTRL_TEST_DYNAMIC           (1)     /* 1=20kHz 中断出三电平单极性正弦（台架当前用这个） */
#define CTRL_TEST_MUX_SWEEP         (0)     /* 引脚复用位巡游：0=关（正常固件），1=台架查列号 */

typedef enum
{
    CTRL_FAULT_NONE = 0U,
    CTRL_FAULT_CLA_TIMEOUT = 1U,
    CTRL_FAULT_OVERCURRENT = 2U,
    CTRL_FAULT_HW_OC = 3U,                  /* 硬件逆变过流（XINT1 漏桶判定） */
    CTRL_FAULT_SHORT = 4U,                  /* 硬件短路（XINT2 窗口判定） */
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

/* 台架发波：把逆变六路、PV 四路、半母线平衡桥从软件钳位态释放。
 * 只在 CTRL_TEST_PWM_MODE = 1 时由 StartSystem() 调用。 */
extern void CtrlTestPwmStart(void);

/* 发波授权请求量：非 static，便于挂到 Modbus 点表里做软件互锁 */
extern volatile Uint16 gCtrlPwmEnable;

/* 台架调试计数：判断控制主体到底由谁在驱动 */
extern volatile Uint16 gCtrlTestClaCnt;   /* CLA1_1 中断次数，0 = ADC->CLA 触发链没通 */
extern volatile Uint16 gCtrlTestTmrCnt;   /* 台架模式下 Timer1 驱动控制主体的次数 */

/* CPU Timer1 中断不进时的现场快照：在 CLA1_1 中断里抓，等于 20 kHz 采一次样。
 * 判读：PIEIFR1 的 bit13=1 表示定时器中断请求已经到了 PIE；
 *       PIEACK 的 bit0=1  表示组 1 还没被应答（被同组中断堵住）。 */
extern volatile Uint16 gCtrlDiagPieIfr1;
extern volatile Uint16 gCtrlDiagPieAck;
extern volatile Uint16 gCtrlDiagT1Tcr;
extern volatile Uint32 gCtrlDiagT1Tim;
extern volatile Uint16 gCtrlDiagIer;
extern volatile Uint16 gCtrlDiagXint1Cnt;
extern volatile Uint16 gCtrlDiagXint2Cnt;

/* 台架静态自检快照：CtrlTestPwmStart() 末尾抓一次，
 * 用于判定逆变载波是否在跑、AQCSFRC 强制位是否真被清掉。 */
extern volatile Uint16 gCtrlDiagTbCtlInv;
extern volatile Uint16 gCtrlDiagAqcsfrcInv;
extern volatile Uint16 gCtrlDiagCmpaInv;
extern volatile Uint16 gCtrlDiagTzflgInv;
extern volatile Uint16 gCtrlDiagTzselInv;
extern volatile Uint16 gCtrlDiagTbCtrA;
extern volatile Uint16 gCtrlDiagTbCtrB;
extern volatile Uint16 gCtrlDiagTbCtrPvA;
extern volatile Uint16 gCtrlDiagTbCtrPvB;
extern volatile Uint16 gCtrlDiagTbCtrC;
extern volatile Uint16 gCtrlDiagTbCtrD;

/* 逐模块静态快照：EPWM4(负半周) / EPWM7(PV1) / EPWM8(BUS) / GPIO 复用 */
extern volatile Uint16 gCtrlDiagInv2Cmpa;
extern volatile Uint16 gCtrlDiagInv2Aqcsfrc;
extern volatile Uint16 gCtrlDiagInv2Tzctl;
extern volatile Uint16 gCtrlDiagPvCmpa;
extern volatile Uint16 gCtrlDiagPvAqcsfrc;
extern volatile Uint16 gCtrlDiagPvAqctla;
extern volatile Uint16 gCtrlDiagPvTzctl;
extern volatile Uint16 gCtrlDiagPvGldctl;
extern volatile Uint16 gCtrlDiagPvTbprd;
extern volatile Uint16 gCtrlDiagPvXlink;
extern volatile Uint16 gCtrlDiagBusDbctl;
extern volatile Uint16 gCtrlDiagBusAqctlb;
extern volatile Uint16 gCtrlDiagGpioMuxA1;
extern volatile Uint16 gCtrlDiagGpioGmuxA1;
extern volatile Uint16 gCtrlDiagGpioMuxA2;
extern volatile Uint16 gCtrlDiagGpioGmuxA2;
extern volatile Uint16 gCtrlDiagMuxCand;
extern volatile Uint16 gCtrlDiagInvDbctl;

/* PV 四路逐模块快照，下标顺序 [0]=EPWM7(PV1) [1]=EPWM9(PV2) [2]=EPWM5(PV3) [3]=EPWM6(PV4) */
extern volatile Uint16 gCtrlDiagPvAq[4];

/* 台架诊断：X-BAR 链路与 GPIO 输入通路（v10 新增） */
extern volatile Uint16 gCtrlDiagXbarIn[6];
extern volatile Uint16 gCtrlDiagTripCfg[4];
extern volatile Uint16 gCtrlDiagTripEn[4];
extern volatile Uint16 gCtrlDiagGpioADat;
extern volatile Uint16 gCtrlDiagGpioADir;
extern volatile Uint16 gCtrlDiagGpioBDat;
extern volatile Uint16 gCtrlDiagGpioBDir;
extern volatile Uint16 gCtrlDiagPvTzflg[4];
extern volatile Uint16 gCtrlDiagPvDcsel[4];
extern volatile Uint16 gCtrlDiagPvCmp[4];
extern volatile Uint16 gCtrlDiagPvCtl[4];
extern volatile Uint16 gCtrlDiagPvRun[4];

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

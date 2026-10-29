/**
 * @file InvStateMachine.h
 * @author yetingheng (004182)
 * @brief 逆变状态机：自检建压、软启、并网/离网切换、软停、故障锁存
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note 本文件只管逆变这一侧（发波授权、调制比、继电器时序、故障），
 *       不负责系统上电流程；由 SystemStateMachine 调用 InvSmStart/InvSmStop。
 *
 *       继电器时序按 ATS 状态分两种（硬件决定，不能混）：
 *       - ATS 闭合（电网在负载侧）：先建压，幅值接近后再闭合逆变继电器，避免冲击；
 *       - ATS 断开（负载侧无电网）：先闭合逆变继电器，再启 PWM，否则负载侧悬空。
 *
 *       自检（INV_ST_CHK）用桥臂自己建压来做：继电器保持断开，桥臂发一个小
 *       调制比，读桥臂侧电压（A0/C3/C4 -> stSampleRms.fInvVoltRms[SAMPLE_PH_R]）判断驱动与采样
 *       通路是否正常；同时读负载侧电压（A5/A1/A3 -> stSampleRms.fGridVoltRms[SAMPLE_PH_R]），
 *       ATS 断开时负载侧若跟着起压就说明继电器粘连。
 */

#ifndef INV_STATE_MACHINE_H
#define INV_STATE_MACHINE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 状态枚举
 ******************************************************************************/
typedef enum
{
    INV_ST_INIT = 0U,       /* 初始化：封锁、继电器断开 */
    INV_ST_SELF,            /* 待机：等开机命令 */
    INV_ST_CHK,             /* 建压自检 */
    INV_ST_SOFT,            /* 软启：调制比斜坡 + 幅值逼近 + 锁相 */
    INV_ST_GRIDON,          /* 闭合逆变继电器并确认 */
    INV_ST_RUN,             /* 运行 */
    INV_ST_STOP,            /* 软停：调制比回零后封锁 */
    INV_ST_ERROR,           /* 故障：封锁 + 断继电器，等复位 */
    INV_ST_MAX
} E_INV_STATE;

/******************************************************************************
 * 故障位
 ******************************************************************************/
#define INV_SM_FAULT_NONE       (0x0000U)
#define INV_SM_FAULT_SELFTEST   (0x0001U)   /* 自检失败：建压不起来 */
#define INV_SM_FAULT_STICK      (0x0002U)   /* 继电器粘连：负载侧跟随桥臂电压 */
#define INV_SM_FAULT_CTRL       (0x0004U)   /* 控制中断报故障（过流/CLA/硬件） */
#define INV_SM_FAULT_GRIDON     (0x0008U)   /* 闭合后电压跌落 */
#define INV_SM_FAULT_ATS        (0x0010U)   /* ATS 状态非法/超时 */

/******************************************************************************
 * 参数
 ******************************************************************************/
#define INV_SM_CHK_AMP_REACH    (200.0f)    /* 自检：桥臂建压到位门限（目标附近） */
#define INV_SM_CLOSE_DIFF_V     (10.0f)     /* 闭合确认：继电器两端电压差门限 */
#define INV_SM_MOD_MAX          (0.90f)     /* 调制比上限，与 CTRL_MOD_INDEX_MAX 一致 */
#define INV_SM_MOD_MIN          (0.02f)
#define INV_SM_RAMP_STEP        (0.0016f)   /* 每 100us 调制比增量，约 60ms 从 0 到 0.9 */
#define INV_SM_RAMP_STEP_DN     (0.0040f)   /* 停机斜坡，约 25ms 到 0 */
#define INV_SM_CHK_TICK         (3000U)     /* 自检建压 300ms（幅值闭环收口时间） */
#define INV_SM_CHK_VOUT_MIN     (5.0f)      /* 建压判据：桥臂侧 RMS 必须 > 5V */
#define INV_SM_CHK_STICK_VOLT   (50.0f)     /* 粘连判据：断开时两端压差应 >50V，否则判粘连 */
#define INV_SM_AMP_REF          (230.0f)    /* 离网目标相电压 RMS */
#define INV_SM_AMP_DIFF         (10.0f)     /* 并网前幅值差门限 */
#define INV_SM_AMP_KP           (0.020f)    /* 幅值闭环比例，每 100us */
#define INV_SM_GRIDON_TICK      (2000U)     /* 闭合后确认 200ms */
#define INV_SM_GRIDON_MIN       (0.70f)     /* 闭合后电压不得低于目标的 70% */

/******************************************************************************
 * 状态机结构
 ******************************************************************************/
typedef struct
{
    volatile Uint16 u16State;       /* 当前状态，见 E_INV_STATE */
    volatile Uint16 u16Fault;       /* 故障位，见 INV_SM_FAULT_* */
    volatile Uint16 u16StartReq;    /* 开机请求（由 SysSm 置位） */
    volatile Uint16 u16StopReq;     /* 停机请求 */
    volatile float  fModRef;        /* 调制比给定（斜坡/闭环输出） */
    volatile float  fVoutRms;       /* 桥臂侧电压 RMS 快照 */
    volatile float  fLoadRms;       /* 负载侧电压 RMS 快照 */
    volatile float  fAmpTarget;     /* 幅值目标 */
    Uint16 u16Cnt;                  /* 状态内计时（100us 拍） */
    Uint16 u16LastState;
} ST_INV_SM;

extern ST_INV_SM gInvSm;

/******************************************************************************
 * API
 ******************************************************************************/
void InvSmInit(void);
void RunInvStateMachine(void);      /* 20kHz 控制中断 ÷2 = 10kHz */
void InvSmStart(void);
void InvSmStop(void);
void InvSmReset(void);              /* 清故障，需故障源已消失 */
Uint16 InvSmGetState(void);
Uint16 InvSmGetFault(void);

#ifdef __cplusplus
}
#endif

#endif /* INV_STATE_MACHINE_H */

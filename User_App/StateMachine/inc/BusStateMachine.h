/**
 * @file BusStateMachine.h
 * @author yetingheng (004182)
 * @brief 半母线平衡状态机：上下母线电压差 -> 平衡桥占空比（框架）
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 结构与命名参照 EP5K《BUSStateMachine.c》：
 *   BUS_STATE_TYPE     三个状态（Init / Work / Error）
 *   BUS_SYSTEM_DATA    平衡控制量（电压差、滤波值、占空比、计数器）
 *   BUS_STATE_DATA     状态机自身状态（WorkState + 状态内计时）
 *   InitBusStateMachineData() / RunBusStateMachine()
 *
 * 本轮只搭框架：状态迁移与电压差测量已通，平衡 PI 与 EPWM8 下发留 TODO，
 * 等台架实测上下母线压差后再填增益，避免凭空给参数。
 */

#ifndef BUS_STATE_MACHINE_H
#define BUS_STATE_MACHINE_H

#include "F2837xS_device.h"

/* 状态内计时门限（20kHz ÷4 = 5kHz 调用，1 拍 = 200us） */
#define BUS_SM_INIT_TICK        (500U)      /* 100ms：等采样稳定 */
#define BUS_SM_DIFF_MIN_VOLT    (5.0f)      /* 压差小于该值认为已平衡 */
#define BUS_SM_DIFF_MAX_VOLT    (80.0f)     /* 压差超过该值判异常 */
#define BUS_SM_ERR_TICK         (1000U)     /* 200ms：异常确认后回 Init */

typedef enum
{
    BUS_INIT_STATE = 0U,        /* 初始化：等采样与母线建立 */
    BUS_WORK_STATE,             /* 平衡调节 */
    BUS_ERROR_STATE             /* 压差异常，退出调节 */
} BUS_STATE_TYPE;

typedef struct
{
    float  fBusPosVolt;         /* 正母线电压 (V) */
    float  fBusNegVolt;         /* 负母线电压 (V) */
    float  fBusDiff;            /* 正负压差 = Pos - Neg (V) */
    float  fBusDiffAvg;         /* 压差慢平均，抑制开关纹波 */
    float  fBalDuty;            /* 平衡桥占空比 0~1，0.5 = 不调节 */
    Uint16 u16StartCnt;         /* 起平衡计时 */
    Uint16 u16OcpCnt;           /* 平衡桥过流计数 */
} BUS_SYSTEM_DATA;

typedef struct
{
    volatile Uint16 u16WorkState;   /* BUS_STATE_TYPE */
    Uint16 u16StateCnt;             /* 状态内计时 */
    Uint16 u16LastState;            /* 上一状态，用于迁移沿动作 */
} BUS_STATE_DATA;

extern BUS_SYSTEM_DATA stBusSystem;
extern BUS_STATE_DATA  stBusState;

void InitBusStateMachineData(void);
void RunBusStateMachine(void);

#endif /* BUS_STATE_MACHINE_H */

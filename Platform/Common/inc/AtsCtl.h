/**
 * @file AtsCtl.h
 * @author yetingheng (004182)
 * @brief ATS（断路器）干接点控制：状态回读 + 分合闸申请
 * @version 0.1
 * @date 2026-10-27
 * @copyright Copyright (c) 2026
 *
 * 硬件（见 DrvCfg_EP15K.c 的 GPIO 表）：
 *   GPIO53 —— 输入，外置 ATS 分合标志
 *   GPIO52 —— 输出，外置 ATS 申请（1:闭合 0:断开）
 *
 * ATS 的 DI 是**边沿触发**：电平的每一次翻转才是一次有效指令。
 * 所以本模块不做"保持电平=持续请求"，而是把电平翻转当作一次动作：
 *   想闭合而当前是断开 -> 翻转一次 -> ATS 闭合
 *   想断开而当前是闭合 -> 翻转一次 -> ATS 断开
 * 动作后等 GPIO53 回读确认，超时就报 TIMEOUT，由上层决定重试。
 */
#ifndef ATS_CTL_H
#define ATS_CTL_H

#include "F28x_Project.h"

/* 干接点引脚 */
#define ATS_CTL_GPIO_STATE          (53U)   /* 输入：ATS 分合标志 */
#define ATS_CTL_GPIO_REQUEST        (52U)   /* 输出：ATS 申请 */

/* GPIO53 读到该电平表示 ATS 闭合（辅助触点常闭，默认低有效，需上板实测确认） */
#define ATS_CTL_STATE_CLOSED_LEVEL  (0U)

/* 去抖：连续多少个 1ms 采样与当前状态不同才认翻新 */
#define ATS_CTL_DEBOUNCE_MS         (20U)

/* 动作超时：申请后多久没等到状态翻转就报超时（ms） */
#define ATS_CTL_TIMEOUT_MS          (5000U)

/* 动作状态 */
typedef enum
{
    ATS_ACT_IDLE = 0U,      /* 空闲，已与目标一致 */
    ATS_ACT_WAIT = 1U,      /* 已翻转电平，等状态回读 */
    ATS_ACT_TIMEOUT = 2U    /* 等待超时 */
} E_ATS_ACT;

/* 运行时状态（点表与软示波器直接指向这里） */
typedef struct
{
    Uint16    u16State;      /* 1 = ATS 闭合，0 = ATS 断开（去抖后） */
    Uint16    u16RawState;   /* 1 = 53 读到"闭合电平" */
    Uint16    u16Request;    /* 期望状态：1 闭合 / 0 断开 */
    Uint16    u16Level;      /* 52 当前输出电平 */
    Uint16    u16DebounceCnt;
    Uint16    u16TimeoutCnt;
    Uint16    u16ActCnt;     /* 状态翻转成功次数 */
    E_ATS_ACT eActState;
} ST_ATS_CTL;

extern ST_ATS_CTL gAtsCtl;

void AtsCtlInit(void);

/* 1ms 周期任务：采样去抖 + 动作超时判定 */
void AtsCtlTask(void *pArgV);

/**
 * @brief 申请把 ATS 切到目标状态（已一致则立即返回）
 * @param u16Target 1 = 闭合，0 = 断开
 */
void AtsRequestChange(Uint16 u16Target);

Uint16 AtsGetState(void);       /* 1 = 闭合 */
Uint16 AtsGetRequest(void);     /* 期望状态 */
Uint16 AtsIsBusy(void);         /* 1 = 正在等待动作完成 */
Uint16 AtsIsTimeout(void);      /* 1 = 上一次动作超时 */

#endif

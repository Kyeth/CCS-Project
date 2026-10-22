/**
 * @file Schedule.h
 * @author yetingheng (004182)
 * @brief EP15K任务调度片
 * @version 0.1
 * @date 2026-09-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "F2837xS_device.h"

/* 定时器 100us 中断中调用此宏，触发任务运行标志 */
#define SYS_TICK()      (TaskCTL.On = 1U)

/* 回调函数指针类型 */
typedef void (*TaskFunc_t)(void* pArgV);

typedef struct
{
    TaskFunc_t Callback;    /* 任务函数指针 */
    void*      pArgV;       /* 任务参数 */
    uint16_t   Time;        /* 调度周期 */
    uint16_t   Cnt;         /* 当前计数 */
} TTask;

typedef struct
{
    volatile uint16_t On;   /* 运行标志，由 ISR 置位 */
    uint16_t Num;           /* 当前注册的任务总数 */
} TTaskCtrl;

extern TTaskCtrl TaskCTL;

void Task_Start(TaskFunc_t task, void* pArgV, uint16_t time, uint16_t point);
void Task_Run(void);

#endif /* SCHEDULE_H */

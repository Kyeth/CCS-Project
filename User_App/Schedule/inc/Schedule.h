/**
 * @file Schedule.h
 * @author yetingheng (004182)
 * @brief EP15K 后台任务调度与活性监视
 * @version 0.2
 * @date 2026-09-11
 *
 * @note 调度基准是 100us 定时器节拍：中断只递增自由运行计数，后台取差值推进，
 *       欠拍不会丢拍、也不会让周期漂移。
 * @note 活性监视与调度共用同一节拍：每个任务记录最近一次执行的节拍值，
 *       200ms 窗口内某个任务迟迟不执行即锁存后台停摆故障，由控制中断封锁发波。
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "F2837xS_device.h"

/* 100us 定时中断中调用：自由运行节拍计数 */
#define SYS_TICK()      (TaskCTL.Tick++)

/* 后台活性窗口：200ms，2000 个 100us 节拍 */
#define TASK_HEALTH_WINDOW_TICK     (2000U)

/* 任务迟到容限：周期 x 倍数与实际执行间隔取大者，单位 100us 节拍。
 * 800 拍 = 80ms，用于覆盖 EEPROM 分页落盘的同步阻塞（最坏 16 页 x 5ms）；
 * 落盘改为异步后应收紧到 100 拍。 */
#define TASK_HEALTH_TOL_NUM         (4U)
#define TASK_HEALTH_MIN_TICK        (800U)

/* 单次调度允许累积的最大节拍数：防止长时间阻塞后一次性补多拍 */
#define TASK_ELAPSED_CLAMP          (1000U)

/* 回调函数指针类型 */
typedef void (*TaskFunc_t)(void* pArgV);

typedef struct
{
    TaskFunc_t Callback;        /* 任务函数指针 */
    void*      pArgV;           /* 任务参数 */
    uint16_t   Time;            /* 调度周期，单位 100us 节拍 */
    uint16_t   Cnt;             /* 距下次到期的节拍余数 */
    uint16_t   u16RunTick;      /* 最近一次执行的节拍计数 */
    uint16_t   u16MissCnt;      /* 到期未能按时执行的次数，仅观测 */
} TTask;

typedef struct
{
    volatile uint16_t Tick;          /* 自由运行节拍计数，100us 中断递增 */
    uint16_t          Num;           /* 已注册任务数 */
    uint16_t          u16TickLast;   /* Task_Run 上次消费到的节拍 */
    uint16_t          u16WindowTick; /* 活性窗口起点 */
    volatile uint16_t u16StallTask;  /* 首个超时任务索引，0xFFFF 表示无 */
    volatile uint16_t u16HealthFault;/* 后台停摆锁存，1 = 故障 */
} TTaskCtrl;

extern TTaskCtrl TaskCTL;

void Task_Start(TaskFunc_t task, void* pArgV, uint16_t time, uint16_t point);
void Task_Run(void);

/* 100us 定时中断内调用：推进活性窗口并裁决后台是否停摆 */
extern void Task_HealthTick(void);

/* 控制中断查询：非 0 表示后台已停摆，应封锁发波 */
extern uint16_t Task_GetHealthFault(void);

#endif /* SCHEDULE_H */

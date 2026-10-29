/**
 * @file Schedule.c
 * @author yetingheng (004182)
 * @brief EP15K 后台任务调度与活性监视
 * @version 0.2
 * @date 2026-09-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Schedule.h"

/* 最大任务数 */
#define TASK_MAX_NUM                (17U)

/* 停摆任务索引的无效值 */
#define TASK_STALL_NONE             (0xFFFFU)

TTaskCtrl TaskCTL = {0U, 0U, 0U, 0U, TASK_STALL_NONE, 0U};

TTask Task[TASK_MAX_NUM];

/**
 * @brief 注册任务
 * @param task  任务回调函数
 * @param pArgV 传递给任务的参数
 * @param time  周期，单位 100us 节拍
 * @param point 首次执行的相位偏移
 */
void Task_Start(TaskFunc_t task, void* pArgV, uint16_t time, uint16_t point)
{
    if (task == (TaskFunc_t)0)
    {
        return;
    }

    if (time == 0U)
    {
        time = 1U;      /* 0 周期无意义，按 100us 最快档处理，避免任务被静默丢弃 */
    }

    if (TaskCTL.Num < TASK_MAX_NUM)
    {
        Task[TaskCTL.Num].Callback   = task;
        Task[TaskCTL.Num].pArgV      = pArgV;
        Task[TaskCTL.Num].Time       = time;
        Task[TaskCTL.Num].Cnt        = point;
        Task[TaskCTL.Num].u16RunTick = TaskCTL.Tick;
        Task[TaskCTL.Num].u16MissCnt = 0U;
        TaskCTL.Num++;
    }
}

/**
 * @brief 后台任务调度
 * @note  以节拍差推进：Task_Run 每被调用一次最多消耗自上次以来累积的节拍，
 *        到期扣减一个周期而非清零，因此后台快慢不影响各任务的平均周期。
 */
void Task_Run(void)
{
    uint16_t u16Scan;
    uint16_t u16Elapsed;

    /* 取走自上次调度以来累积的节拍数，未跨过节拍直接返回 */
    u16Elapsed = (uint16_t)(TaskCTL.Tick - TaskCTL.u16TickLast);

    if (u16Elapsed == 0U)
    {
        return;
    }

    TaskCTL.u16TickLast += u16Elapsed;

    /* 长时间阻塞后限制补拍数量，避免一次调度把多个周期堆在一起执行 */
    if (u16Elapsed > TASK_ELAPSED_CLAMP)
    {
        u16Elapsed = TASK_ELAPSED_CLAMP;
    }

    for (u16Scan = 0U; u16Scan < TaskCTL.Num; u16Scan++)
    {
        Task[u16Scan].Cnt += u16Elapsed;

        if (Task[u16Scan].Cnt >= Task[u16Scan].Time)
        {
            /* 扣减一个周期而不是归零，剩余节拍留给下一轮，周期不漂移 */
            Task[u16Scan].Cnt -= Task[u16Scan].Time;

            /* 扣减后仍欠一个以上周期，说明已经掉拍：丢弃积压，不追拍 */
            if (Task[u16Scan].Cnt >= Task[u16Scan].Time)
            {
                Task[u16Scan].u16MissCnt++;
                Task[u16Scan].Cnt = 0U;
            }

            Task[u16Scan].u16RunTick = TaskCTL.Tick;

            if (Task[u16Scan].Callback != NULL)
            {
                (*(Task[u16Scan].Callback))(Task[u16Scan].pArgV);
            }
        }
    }
}

/**
 * @brief 后台活性裁决，100us 定时中断内调用
 * @note  每 200ms 收口一次：任一任务的实际执行间隔超过
 *        "周期 x 容限倍数 + 固定余量"，即判定后台停摆并锁存，
 *        由控制中断读取后封锁发波；这里不做复位，复位由硬件看门狗负责。
 */
void Task_HealthTick(void)
{
    uint16_t u16Scan;
    uint16_t u16Age;
    uint16_t u16Limit;

    if ((uint16_t)(TaskCTL.Tick - TaskCTL.u16WindowTick) < TASK_HEALTH_WINDOW_TICK)
    {
        return;
    }

    TaskCTL.u16WindowTick = TaskCTL.Tick;

    if (TaskCTL.u16HealthFault != 0U)
    {
        return;     /* 已锁存，等待复位或显式清除 */
    }

    for (u16Scan = 0U; u16Scan < TaskCTL.Num; u16Scan++)
    {
        u16Age   = (uint16_t)(TaskCTL.Tick - Task[u16Scan].u16RunTick);
        u16Limit = (uint16_t)((uint32_t)Task[u16Scan].Time * TASK_HEALTH_TOL_NUM);

        if (u16Limit < TASK_HEALTH_MIN_TICK)
        {
            u16Limit = TASK_HEALTH_MIN_TICK;    /* 慢周期任务给固定下限，避免误判 */
        }

        if (u16Age > u16Limit)
        {
            TaskCTL.u16StallTask   = u16Scan;
            TaskCTL.u16HealthFault = 1U;
            return;
        }
    }
}

/**
 * @brief 查询后台活性故障
 * @return 0 = 正常，非 0 = 后台任务停摆
 */
uint16_t Task_GetHealthFault(void)
{
    return TaskCTL.u16HealthFault;
}

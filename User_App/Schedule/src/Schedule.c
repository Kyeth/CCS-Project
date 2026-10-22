/**
 * @file Schedule.c
 * @author yetingheng (004182)
 * @brief EP15K任务调度片
 * @version 0.1
 * @date 2026-09-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Schedule.h"

/* 最大任务数 */
#define TASK_MAX_NUM                (17U)

TTaskCtrl TaskCTL = {0U, 0U};
TTask Task[TASK_MAX_NUM];

/**
 * @brief 注册任务
 * @param task 任务回调函数
 * @param pArgV 传递给任务的参数
 * @param time 周期（单位：基准节拍，如100us）
 * @param point 初始偏移量
 */
void Task_Start(void (*task)(void*), void* pArgV, uint16_t time, uint16_t point)
{
    if ((task == NULL) || (time == 0U))
    {
        return;
    }

    if (TaskCTL.Num < TASK_MAX_NUM)
    {
        Task[TaskCTL.Num].Callback = task;
        Task[TaskCTL.Num].pArgV   = pArgV;
        Task[TaskCTL.Num].Time     = time;
        Task[TaskCTL.Num].Cnt      = point;
        TaskCTL.Num++;
    }
}

/**
 * @brief 主循环后台任务调度
 */
void Task_Run(void)
{
    uint16_t u16Scan;

    if (TaskCTL.On == 0U)
    {
        return;
    }

    /* 先清零标志，确保本轮执行期间发生的新中断能被下一轮处理 */
    TaskCTL.On = 0U;

    for (u16Scan = 0U; u16Scan < TaskCTL.Num; u16Scan++)
    {
        Task[u16Scan].Cnt++;
        if (Task[u16Scan].Cnt >= Task[u16Scan].Time)
        {
            Task[u16Scan].Cnt = 0U;
            if (Task[u16Scan].Callback != NULL)
            {
                (*(Task[u16Scan].Callback))(Task[u16Scan].pArgV);
            }
        }
    }
}




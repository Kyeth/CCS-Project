/**
 * @file Schedule.c
 * @brief Cooperative scheduler; callbacks run only from the main loop.
 */

#include "Schedule.h"

#define TASK_MAX_NUM 8U

typedef struct
{
    void (*Callback)(void);
    Uint16 PeriodMs;
    Uint16 CountMs;
} ST_TASK;

static ST_TASK stTasks[TASK_MAX_NUM];
static Uint16 u16TaskNum = 0U;
static volatile Uint16 u16PendingTicks = 0U;
volatile Uint32 gTaskTicksProcessed = 0UL;

void Task_Init(void)
{
    u16TaskNum = 0U;
    u16PendingTicks = 0U;
    gTaskTicksProcessed = 0UL;
}

Uint16 Task_Start(void (*callback)(void), Uint16 periodMs, Uint16 offsetMs)
{
    if (callback == 0 || periodMs == 0U || u16TaskNum >= TASK_MAX_NUM)
    {
        return 0U;
    }

    stTasks[u16TaskNum].Callback = callback;
    stTasks[u16TaskNum].PeriodMs = periodMs;
    stTasks[u16TaskNum].CountMs = offsetMs % periodMs;
    u16TaskNum++;
    return 1U;
}

void Task_OnTick(void)
{
    if (u16PendingTicks != 0xFFFFU)
    {
        u16PendingTicks++;
    }
}

void Task_Run(void)
{
    Uint16 i;
    Uint16 runTick = 0U;

    /* Claim one tick atomically; leave interrupts enabled during callbacks. */
    DINT;
    if (u16PendingTicks != 0U)
    {
        u16PendingTicks--;
        runTick = 1U;
    }
    EINT;

    if (runTick == 0U)
    {
        return;
    }

    gTaskTicksProcessed++;
    for (i = 0U; i < u16TaskNum; i++)
    {
        stTasks[i].CountMs++;
        if (stTasks[i].CountMs >= stTasks[i].PeriodMs)
        {
            stTasks[i].CountMs = 0U;
            stTasks[i].Callback();
        }
    }
}

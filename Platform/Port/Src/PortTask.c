/**
 * @file PortTask.c
 * @brief 端口层汇总服务实现
 * @version 0.1
 * @date 2026-11-05
 *
 * 三条约束：
 *   1. 公平：每轮从不同单元开始，避免 CAN 满载时饿死后面的单元；
 *   2. 有界：每个单元开始前检查轮次预算，超预算立即收手，不再发起新调用；
 *   3. 时间源失败即停：读不到可靠时标时整轮不服务（宁可延迟，不用坏时间做预算）。
 */

#include "PortTask.h"
#include "Hw_TimeBase.h"
#include "Schedule.h"

#define PORT_TASK_NO_UNIT (0xFFFFU)

static Uint16 s_u16Cursor;
static Uint16 s_u16LastFirst = PORT_TASK_NO_UNIT;
static Uint16 s_u16LastUnits;
static Uint32 s_u32Truncations;
static Uint32 s_u32TimeSrcFailures;
static Uint32 s_u32TicksPerUs = 200UL;
static Uint16 s_bBudgetTickFault;
static PORT_TASK_TICK_FN s_pfTryGetTick = (PORT_TASK_TICK_FN)0;

/* 轮次预算窗口（服务层通过 PortBudgetHasLeft() 查询）。 */
static Uint16 s_bBudgetActive;
static Uint32 s_u32BudgetStart;
static Uint32 s_u32BudgetLimit;

/* 单元 -> 硬件 ID。SCI A/C/D 的硬件 ID 是 0/2/3，不等于槽位 0/1/2。 */
static const Uint16 s_au16SciUnitPort[SCI_PORT_NUM] = { HW_SCI_ID_A, HW_SCI_ID_C, HW_SCI_ID_D };
static const Uint16 s_au16CanUnitPort[CAN_PORT_NUM] = { HW_CAN_ID_A, HW_CAN_ID_B };

/* ---------------- 时间源 ---------------- */

static Uint16 PortTaskDefaultTryGetTick(Uint32 *pu32Tick)
{
    Uint32 u32Ticks;

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    u32Ticks = Hw_TimeBaseTicks();

    /* 递减计数器跨高低字读取可能撕裂；读后复查就绪，失败就不用这个值。 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    *pu32Tick = u32Ticks;
    return 1U;
}

static Uint16 PortTaskTryGetTick(Uint32 *pu32Tick)
{
    if (s_pfTryGetTick != (PORT_TASK_TICK_FN)0)
    {
        return s_pfTryGetTick(pu32Tick);
    }

    return PortTaskDefaultTryGetTick(pu32Tick);
}

/* ---------------- 轮次预算窗口 ---------------- */

void PortBudgetBegin(Uint32 u32StartTick)
{
    s_u32BudgetStart = u32StartTick;
    s_u32BudgetLimit = PORT_TASK_BUDGET_US * s_u32TicksPerUs;
    s_bBudgetActive = 1U;

    /* 新一轮重新开放预算：上一轮的故障锁存只在本轮内有效。 */
    s_bBudgetTickFault = 0U;
}

void PortBudgetEnd(void)
{
    s_bBudgetActive = 0U;
}

Uint16 PortBudgetActive(void)
{
    return s_bBudgetActive;
}

Uint16 PortBudgetFaulted(void)
{
    return s_bBudgetTickFault;
}

Uint16 PortBudgetHasLeft(void)
{
    Uint32 u32Now;
    Uint32 u32Elapsed;

    /* 未开窗：不受本预算约束（100 us 心跳里的 SCI 接收就是这种情形）。 */
    if (s_bBudgetActive == 0U)
    {
        return 1U;
    }

    /* 本轮内只要出现过一次时间读取失败，就锁存到本轮结束：
       后续端口不得因为"下一次读取又成功"而继续发起驱动调用。
       时间源中途失效意味着已经量不准，整轮停止优于只停一个端口。 */
    if (s_bBudgetTickFault != 0U)
    {
        return 0U;
    }

    if (PortTaskTryGetTick(&u32Now) == 0U)
    {
        s_u32TimeSrcFailures++;
        s_bBudgetTickFault = 1U;
        SciPortInvalidateWaitBases();   /* 本轮时间不可信：等待基准一并作废 */
        return 0U;
    }
    u32Elapsed = (Uint32)(s_u32BudgetStart - u32Now);  /* Timer0 递减 */

    return (u32Elapsed < s_u32BudgetLimit) ? 1U : 0U;
}

/* ---------------- 对外接口 ---------------- */

void PortTaskInit(void)
{
    s_u16Cursor = 0U;
    s_u16LastFirst = PORT_TASK_NO_UNIT;
    s_u16LastUnits = 0U;
    s_u32Truncations = 0UL;
    s_u32TimeSrcFailures = 0UL;
    s_u32TicksPerUs = 200UL;
    s_pfTryGetTick = (PORT_TASK_TICK_FN)0;
    s_bBudgetActive = 0U;
    s_u32BudgetStart = 0UL;
    s_u32BudgetLimit = 0UL;
    s_bBudgetTickFault = 0U;
}

Uint16 PortTaskUnitPortId(Uint16 u16Unit)
{
    if (u16Unit < SCI_PORT_NUM)
    {
        return s_au16SciUnitPort[u16Unit];
    }

    if (u16Unit < PORT_TASK_UNIT_NUM)
    {
        return s_au16CanUnitPort[(Uint16)(u16Unit - SCI_PORT_NUM)];
    }

    return PORT_TASK_NO_UNIT;
}

void PortTaskSetTimeSource(PORT_TASK_TICK_FN pfTryGetTick, Uint32 u32SysclkHz)
{
    Uint32 u32PerUs = u32SysclkHz / 1000000UL;

    s_pfTryGetTick = pfTryGetTick;
    s_u32TicksPerUs = (u32PerUs == 0UL) ? 1UL : u32PerUs;
}

void PortTask(void)
{
    Uint32 u32Start;
    Uint16 u16Unit;
    Uint16 u16Slot;
    Uint16 u16Units = 0U;

    s_u16LastUnits = 0U;

    if (PortTaskTryGetTick(&u32Start) == 0U)
    {
        s_u32TimeSrcFailures++;
        s_u16LastFirst = PORT_TASK_NO_UNIT;
        SciPortInvalidateWaitBases();   /* 入口就没起来：同样不能留着旧基准 */
        return;
    }

    s_u16LastFirst = s_u16Cursor;

    /* 开窗：单元之间与服务层内部（每次驱动调用前）都查同一个 deadline。 */
    PortBudgetBegin(u32Start);

    for (u16Unit = 0U; u16Unit < PORT_TASK_UNIT_NUM; u16Unit++)
    {
        u16Slot = (Uint16)((s_u16Cursor + u16Unit) % PORT_TASK_UNIT_NUM);

        /* 预算检查放在每个单元之前：超出就不再发起新调用。 */
        if (PortBudgetHasLeft() == 0U)
        {
            if (s_bBudgetTickFault == 0U)
            {
                s_u32Truncations++;
            }
            break;
        }

        /* 槽位不是硬件 ID：经映射表换成真实端口号。 */
        if (u16Slot < SCI_PORT_NUM)
        {
            Uint16 u16PortId = s_au16SciUnitPort[u16Slot];
            Uint16 u16State = SciPortRxState(u16PortId);
            Uint16 u16Remain;

            /* 恢复优先：先推进一次受限动作，再在门禁允许时补发。
               顺序不能反——否则 bTxSuspended 会把自己挡在恢复门外。
               复位动作同样受同一个 20 us 轮次预算约束。 */
            if ((u16State == SCI_RX_RECOVER_REQ) || (u16State == SCI_RX_HW_RESET))
            {
                (void)SciPortRecover(u16PortId, &u16Remain);
            }

            SciPortTaskOne(u16PortId);
        }
        else
        {
            CanPortServiceOne(s_au16CanUnitPort[(Uint16)(u16Slot - SCI_PORT_NUM)]);
        }

        u16Units++;
    }

    PortBudgetEnd();

    s_u16LastUnits = u16Units;
    s_u16Cursor = (Uint16)((s_u16Cursor + 1U) % PORT_TASK_UNIT_NUM);
}

Uint16 PortTaskLastFirstUnit(void)
{
    return s_u16LastFirst;
}

Uint16 PortTaskLastUnits(void)
{
    return s_u16LastUnits;
}

Uint32 PortTaskBudgetTruncations(void)
{
    return s_u32Truncations;
}

Uint32 PortTaskTimeSourceFailures(void)
{
    return s_u32TimeSrcFailures;
}

void PortTaskServiceFromHeartbeat(Uint16 bPortInitError)
{
    if (bPortInitError != 0U)
    {
        return;
    }

    (void)SciPortService100us();
}

Uint16 PortTaskRegister(void)
{
    return Task_Start(PortTask, 1U, 0U);
}

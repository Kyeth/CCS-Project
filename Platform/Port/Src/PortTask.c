/**
 * @file PortTask.c
 * @brief 端口层前台服务实现：一次服务机会只服务一个单元
 * @version 0.2
 * @date 2026-11-09
 *
 * 三条约束：
 *   1. 有界：单个单元最多占用 PORT_TASK_UNIT_BUDGET_US，到期不再发起新的
 *      驱动调用，已开始的调用允许完成；
 *   2. 不积攒补跑：心跳与主循环之间用一条"待服务"通知连接，重复通知合并，
 *      主循环来不及时只丢机会、不补跑历史节拍，游标照常前进；
 *   3. 时间源失败即停：读不到可靠时标时本次服务不执行，故障锁存到本次结束。
 */

#include "PortTask.h"
#include "Hw_TimeBase.h"

#define PORT_TASK_NO_UNIT (0xFFFFU)

static Uint16 s_u16Cursor;
static Uint16 s_u16LastUnit = PORT_TASK_NO_UNIT;
static Uint16 s_u16LastUnits;
static Uint32 s_u32Truncations;
static Uint32 s_u32TimeSrcFailures;
static Uint32 s_u32ServiceCalls;
static Uint32 s_u32RequestsMerged;
static Uint32 s_u32TicksPerUs = 200UL;
static Uint16 s_bBudgetTickFault;
static Uint16 s_bTruncCounted;
static PORT_TASK_TICK_FN s_pfTryGetTick = (PORT_TASK_TICK_FN)0;

/* 服务预算窗口：一次调用只有一个单元，所以只有一个窗口。 */
static Uint16 s_bBudgetActive;
static Uint32 s_u32UnitStart;
static Uint32 s_u32UnitLimit;

/* 待服务通知：1 = 有一次服务机会尚未被主循环取走。 */
static volatile Uint16 s_bRequest;

/* 每个单元的实际服务间隔（递减计数器读数与最大间隔）。
   RAMGS0 余量只剩个位数，这 10 个字放进 .portstat（RAMGS1）。 */
static Uint32 s_au32LastTick[PORT_TASK_UNIT_NUM];
static Uint32 s_au32MaxGap[PORT_TASK_UNIT_NUM];
#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_au32LastTick, ".portstat")
#pragma DATA_SECTION(s_au32MaxGap, ".portstat")
#endif

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

/* ---------------- 服务预算窗口 ---------------- */

void PortBudgetBegin(Uint32 u32StartTick)
{
    s_u32UnitStart = u32StartTick;
    s_u32UnitLimit = PORT_TASK_UNIT_BUDGET_US * s_u32TicksPerUs;
    s_bBudgetActive = 1U;

    /* 新的一次服务机会重新开放预算：上一单元的故障锁存不跨调用累计。 */
    s_bBudgetTickFault = 0U;
    s_bTruncCounted = 0U;
}

void PortBudgetEnd(void)
{
    s_bBudgetActive = 0U;
    s_u32UnitStart = 0UL;
}

Uint16 PortBudgetActive(void)
{
    return s_bBudgetActive;
}

Uint16 PortBudgetFaulted(void)
{
    return s_bBudgetTickFault;
}

Uint32 PortBudgetUnitStartTick(void)
{
    return s_u32UnitStart;
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

    /* 本次调用内只要出现过一次时间读取失败，就锁存到本次结束：
       后续驱动调用不得因为"下一次读取又成功"而继续。 */
    if (s_bBudgetTickFault != 0U)
    {
        return 0U;
    }

    if (PortTaskTryGetTick(&u32Now) == 0U)
    {
        s_u32TimeSrcFailures++;
        s_bBudgetTickFault = 1U;
        SciPortInvalidateWaitBases();   /* 时间不可信：等待基准一并作废 */
        return 0U;
    }

    u32Elapsed = (Uint32)(s_u32UnitStart - u32Now);   /* Timer0 递减 */

    if (u32Elapsed >= s_u32UnitLimit)
    {
        /* 每次开窗最多记一次收手，避免同一单元内逐次查询把计数刷爆。 */
        if (s_bTruncCounted == 0U)
        {
            s_u32Truncations++;
            s_bTruncCounted = 1U;
        }
        return 0U;
    }

    return 1U;
}

/* ---------------- 对外接口 ---------------- */

void PortTaskInit(void)
{
    Uint16 u16Unit;

    s_u16Cursor = 0U;
    s_u16LastUnit = PORT_TASK_NO_UNIT;
    s_u16LastUnits = 0U;
    s_u32Truncations = 0UL;
    s_u32TimeSrcFailures = 0UL;
    s_u32ServiceCalls = 0UL;
    s_u32RequestsMerged = 0UL;
    s_u32TicksPerUs = 200UL;
    s_pfTryGetTick = (PORT_TASK_TICK_FN)0;
    s_bBudgetActive = 0U;
    s_u32UnitStart = 0UL;
    s_u32UnitLimit = 0UL;
    s_bBudgetTickFault = 0U;
    s_bTruncCounted = 0U;
    s_bRequest = 0U;

    for (u16Unit = 0U; u16Unit < PORT_TASK_UNIT_NUM; u16Unit++)
    {
        s_au32LastTick[u16Unit] = 0UL;
        s_au32MaxGap[u16Unit] = 0UL;
    }
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
    Uint32 u32Gap;
    Uint16 u16Slot;

    s_u16LastUnits = 0U;
    s_u16LastUnit = PORT_TASK_NO_UNIT;

    if (PortTaskTryGetTick(&u32Start) == 0U)
    {
        s_u32TimeSrcFailures++;
        SciPortInvalidateWaitBases();   /* 入口就没起来：同样不能留着旧基准 */
        return;
    }

    /* 游标只在取用单元时前进一次：被丢掉的服务机会不会让某一格饿死。 */
    u16Slot = s_u16Cursor;
    s_u16Cursor = (Uint16)((s_u16Cursor + 1U) % PORT_TASK_UNIT_NUM);
    s_u16LastUnit = u16Slot;

    /* 记录本单元与上一次服务之间的实际间隔（首次不记）。 */
    if (s_au32LastTick[u16Slot] != 0UL)
    {
        u32Gap = (Uint32)(s_au32LastTick[u16Slot] - u32Start);
        if (u32Gap > s_au32MaxGap[u16Slot])
        {
            s_au32MaxGap[u16Slot] = u32Gap;
        }
    }
    s_au32LastTick[u16Slot] = u32Start;

    PortBudgetBegin(u32Start);

    /* 槽位不是硬件 ID：经映射表换成真实端口号。 */
    if (u16Slot < SCI_PORT_NUM)
    {
        Uint16 u16PortId = s_au16SciUnitPort[u16Slot];
        Uint16 u16State = SciPortRxState(u16PortId);
        Uint16 u16Remain;

        /* 恢复优先：先推进一次受限动作，再在门禁允许时补发。
           顺序不能反——否则 bTxSuspended 会把自己挡在恢复门外。 */
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

    PortBudgetEnd();

    s_u16LastUnits = 1U;
    s_u32ServiceCalls++;
}

Uint16 PortTaskService(void)
{
    if (s_bRequest == 0U)
    {
        return 0U;
    }

    /* 先消费通知再服务：服务期间新到的通知留给下一次，不叠加。 */
    s_bRequest = 0U;
    PortTask();

    return s_u16LastUnits;
}

void PortTaskRequest(void)
{
    if (s_bRequest != 0U)
    {
        /* 上一次机会还没被取走：合并，不排队补跑。 */
        s_u32RequestsMerged++;
    }

    s_bRequest = 1U;
}

Uint16 PortTaskLastUnit(void)
{
    return s_u16LastUnit;
}

Uint16 PortTaskLastFirstUnit(void)
{
    return s_u16LastUnit;
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

Uint32 PortTaskServiceCalls(void)
{
    return s_u32ServiceCalls;
}

Uint32 PortTaskRequestsMerged(void)
{
    return s_u32RequestsMerged;
}

Uint32 PortTaskUnitMaxGapTicks(Uint16 u16Unit)
{
    if (u16Unit >= PORT_TASK_UNIT_NUM)
    {
        return 0UL;
    }

    return s_au32MaxGap[u16Unit];
}

void PortTaskServiceFromHeartbeat(Uint16 bPortInitError)
{
    if (bPortInitError != 0U)
    {
        return;
    }

    (void)SciPortService100us();

    /* 只登记"有一次前台服务机会"：重复通知合并，不补跑历史节拍。 */
    PortTaskRequest();
}

Uint16 PortTaskRegister(void)
{
    /* 不再注册 1 ms 任务：前台入口由主循环 + 心跳通知承担，
       避免两个入口重复消费 TX 队列或访问 CAN。 */
    return 1U;
}

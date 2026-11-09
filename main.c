/**
 * @file main.c
 * @author yetingheng (004182)
 * @brief Minimal F28374S peripheral bring-up and lifecycle entry points.
 * @brief 本工程主程序入口
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 */

#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "F2837xS_Pie_defines.h"
#include "DrvCfg_EP15K.h"
#include "Hw_TimeBase.h"
#include "Hw_ClaTask.h"
#include "Hw_WatchDog.h"
#include "Schedule.h"
#include "BringupStatus.h"
#include "SciPort.h"
#include "CanPort.h"
#include "PortTask.h"
#include "PortBench.h"
#include "SampleData.h"
#include "AdcSample.h"

/* ============================================================================
 * 上板测量开关（对应 05B 文档 §8.4）
 *
 * PORT_BENCH_TEST == 0U：不编译任何测量代码；与当前关闭态基线（非 ec59056）
 *   只允许存在 §2.5.4 记录的两处局部变量差异。
 * PORT_BENCH_TEST == 1U：编译回显/灌满任务与 ISR 打点，仅用于上板测量第 1~4 项。
 *
 * 测完改回 0U，重新构建并确认产物与基线一致后再提交；测量期间的打点差异需
 * 记录在 05B 文档 §8.3。
 * ==========================================================================*/

#if (PORT_BENCH_TEST != 0U)
#define BENCH_SCI_PORT   (HW_SCI_ID_D)   /* SCI-D：TX=GPIO93 / RX=GPIO94，38400 8N1 */
#define BENCH_BUF_WORDS  (128U)
/* Must stay below the TX ring capacity (128 slots -> 127 bytes):
 * SciPortWriteTx() uses RingBuffWriteExact(), which rejects the whole batch. */
#define BENCH_FLOOD_LEN  (64U)
#define BENCH_AMP        (4U)            /* 诊断模式 4/5/6 的重复次数 */
#define BENCH_GPIO_MARK  (33U)           /* 配置表里的"测试引脚"，独占不用 */

static Uint16 s_au16BenchBuf[BENCH_BUF_WORDS];
static ST_SCI_RX_SEG s_stBenchSeg;

/* 下面四个量从 CCS Expressions 里直接改/看（非 static，保证有符号） */
Uint16 g_u16BenchMode;            /* 0=回显 1=灌满 TX */
Uint32 g_u32BenchTxTotal;         /* 累计写入 TX 环的字节数 */
Uint32 g_u32BenchResumeTries;     /* 强制复位后解冻尝试次数 */
Uint32 g_u32BenchPopCalls;        /* 回显模式下成功取到的批次数 */

Uint32 g_u32IsrMarkTicks;         /* ISR 进入时刻的 Timer0 计数 */
Uint32 g_u32IsrMarkMaxTicks;      /* ISR 最长耗时（tick，200 tick = 1 us） */
Uint16 g_u16IsrMarkValid;

/* GPIO33 打点模式。**不支持运行中切换**：只在两个 ISR 都退出的前台断点处
 * 修改，确认 GPIO33 为低后再恢复运行，并丢弃暂停前后的过渡样本。
 *
 *   0 = 心跳 ISR 全程（含三路 SCI 服务，配合 g_u32IsrMarkMaxTicks 看软件计时）
 *   1 = Isr_Control 全程（20 kHz 控制中断，默认；示波器直接量脉宽与周期）
 *   2 = 心跳 ISR 里 SCI 服务那一段
 *   3 = 心跳 ISR 里定时器分频与毫秒任务段
 *   4 = 诊断：正常服务之后再执行 4 轮 SciPortService100us()（该心跳共 5 轮服务，
 *       会推进排空/发布/观察记录，改变端口状态），只作专项诊断，不进对照数据
 *   5 = 诊断：连续 4 次 Hw_TimeBaseTicks()
 *   6 = 诊断：三路各 4 次 Hw_SciGetRxStatus()
 *   7 = SciPortService100us() 内那一次共享取时（每轮 1 个脉冲）
 *   8 = 正常 RX 路径上的单路状态查询（仅在走到该查询时才有脉冲；恢复态可能跳过，
 *       故不能预设恒为 3 个）
 *   9 = 正常 RX 路径上的单次读取尝试，含返回空的尝试（成功读 n<8 字节后再探空 =
 *       n+1 次调用；连续读满 8 字节 = 8 次；**不含** SciPortIsrDrop() 的排空读取）
 *  10 = 调用点外侧的单路服务调用及活动计数累加（每轮三路各一个脉冲，但三路
 *       路径与脉宽未必相同，须分别记录）
 *  11 = 空窗口参考：一个 if 内连续两次 BenchMarkPin()，其调用结构不同于 7~10
 *       （7~10 为"模式判断 → PortBenchMark → BenchMarkPin"），只作自身读数
 * 同一时刻只有一种模式生效，GPIO33 只有一个脚。 */
volatile Uint16 g_u16BenchMarkMode = 1U;

/**
 * @brief 只翻转打点引脚，不做软件计时（示波器测量用）。
 */
static void BenchMarkPin(Uint16 u16Level)
{
    Hw_GpioWritePin(BENCH_GPIO_MARK, u16Level);
}

/* 供 SciPort.c 在真实调用点打点（05C §2.4）：只翻转引脚，不做软件计时。 */
void PortBenchMark(Uint16 u16Level)
{
    BenchMarkPin(u16Level);
}

/**
 * @brief 1 ms 回显/灌满任务。
 */
void BenchTask(void)
{
    Uint16 u16Got;
    Uint16 u16Put;

    /* 强制复位后 TX 冻结，这里模拟协议层：恢复回 IDLE 后主动解冻 */
    if (SciPortIsTxSuspended(BENCH_SCI_PORT) != 0U)
    {
        if (SciPortRxState(BENCH_SCI_PORT) == SCI_RX_IDLE)
        {
            (void)SciPortTxResume(BENCH_SCI_PORT);
            g_u32BenchResumeTries++;
        }
        return;
    }

    if (g_u16BenchMode == 0U)                    /* 回显：收到什么发回什么 */
    {
        u16Got = SciPortPopBatch(BENCH_SCI_PORT, s_au16BenchBuf,
                                 (Uint16)(BENCH_BUF_WORDS / 2U), &s_stBenchSeg);
        if (u16Got != 0U)
        {
            g_u32BenchPopCalls++;
            u16Put = SciPortWriteTx(BENCH_SCI_PORT, s_au16BenchBuf, u16Got);
            g_u32BenchTxTotal += (Uint32)u16Put;
        }
    }
    else                                         /* 灌满 TX：0x40..0x7F 循环，便于识别残留 */
    {
        Uint16 i;
        for (i = 0U; i < BENCH_FLOOD_LEN; ++i)
        {
            s_au16BenchBuf[i] = (Uint16)(0x40U + (i & 0x3FU));
        }
        u16Put = SciPortWriteTx(BENCH_SCI_PORT, s_au16BenchBuf, BENCH_FLOOD_LEN);
        g_u32BenchTxTotal += (Uint32)u16Put;
    }
}

/**
 * @brief ISR 入口打点：拉高 GPIO33 并记下 Timer0 计数。
 */
static void BenchIsrMarkBegin(void)
{
    Hw_GpioWritePin(BENCH_GPIO_MARK, 1U);
    if (Hw_TimeBaseIsReady() == 0U)
    {
        g_u16IsrMarkValid = 0U;
        return;
    }
    g_u16IsrMarkValid = 1U;
    g_u32IsrMarkTicks = Hw_TimeBaseTicks();
}

/**
 * @brief ISR 出口打点：拉低 GPIO33，更新最长耗时。
 */
static void BenchIsrMarkEnd(void)
{
    Uint32 u32Elapsed;

    Hw_GpioWritePin(BENCH_GPIO_MARK, 0U);
    if (g_u16IsrMarkValid == 0U)
    {
        return;
    }
    u32Elapsed = Hw_TimeBaseElapsed(g_u32IsrMarkTicks);
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return;
    }
    if (u32Elapsed > g_u32IsrMarkMaxTicks)
    {
        g_u32IsrMarkMaxTicks = u32Elapsed;
    }
}
#endif /* PORT_BENCH_TEST */

static __interrupt void Isr_Control(void);
static __interrupt void Isr_Heartbeat(void);

/**
 * @brief Configure interrupt vectors and enable masks before peripherals run.
 */
static void InterruptVectTableInit(void)
{
    DINT;
    InitPieCtrl();
    IER = 0U;
    IFR = 0U;
    InitPieVectTable();

    EALLOW;
    PieVectTable.CLA1_1_INT = &Isr_Control;
    PieVectTable.TIMER1_INT = &Isr_Heartbeat;
    EDIS;

    PieCtrlRegs.PIEIER11.bit.INTx1 = 1U;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
    IER |= (M_INT11 | M_INT13);
}

/**
 * @brief Initialize the chip and table-driven board peripherals.
 */
void PeripheralsInit(void)
{
    Uint16 i;

    if (Hw_ClockInit(&stClockCfg) == 0U ||
        stSciCfg.u32LspclkHz != stClockCfg.u32LspclkHz ||
        stCanCfg.u32SysclkHz != stClockCfg.u32SysclkHz)
    {
        stBringupStatus.unError.Bits.bClockInitError = 1U;
    }

    /* Free-running time base (CPU Timer0). Every time-bounded wait depends on it,
     * so it must run before CAN or any other module starts waiting on time. */
    if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
        (Hw_TimeBaseInit(stClockCfg.u32SysclkHz) == 0U))
    {
        stBringupStatus.unError.Bits.bCanInitError = 1U;
    }
    InterruptVectTableInit();

    /* The Broad table drives GPIO88 and all relay controls low at boot. */
    Hw_GpioInit(&stGpioCfg);
    Hw_GpioWritePin(88U, 0U);

    Hw_PwmInit(&stPwmCfg);
    Hw_StopAllPWM();
    Hw_XbarInit(&stXbarCfg);
    Hw_PwmInitTripZone(&stPwmTzCfg);
    Hw_AdcInit(&stAdcCfg);
    if (Hw_ClaInit(&stClaCfg) == 0U)
    {
        stBringupStatus.unError.Bits.bClaInitError = 1U;
    }

    AdcSampleInit();

    CpuToCla1Msg.u16MuxAddress = 0U;
    for (i = 0U; i < 8U; i++)
    {
        CpuToCla1Msg.u16NextMux[i] = stClaMuxNext[i];
    }
    Hw_GpioWritePin(46U, 0U);
    Hw_GpioWritePin(47U, 0U);
    Hw_GpioWritePin(42U, 0U);

    if (Hw_SciInit(&stSciCfg) == 0U)
    {
        stBringupStatus.unError.Bits.bSciInitError = 1U;
    }
    /* CAN waits are time-bounded: only start it when the clock is good and the
     * time base really runs. A failed time base blocks CAN instead of hanging. */
    if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
        (stBringupStatus.unError.Bits.bCanInitError == 0U) &&
        (Hw_TimeBaseIsReady() != 0U))
    {
        if (Hw_CanInit(&stCanCfg) == 0U)
        {
            stBringupStatus.unError.Bits.bCanInitError = 1U;
        }
    }
    else
    {
        stBringupStatus.unError.Bits.bCanInitError = 1U;
    }
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
}

/**
 * @brief Initialize platform components: scheduler, then the port layer.
 *
 * 05A only instantiates the ports (rings + driver bindings) so their storage is
 * real and visible to the linker. Neither SciPortService100us() nor
 * CanPortService() is called yet, and no SCI/CAN interrupt is enabled; the
 * wiring that drives them belongs to 05B.
 */
void PlatformInit(void)
{
    Task_Init();
    SciPortInit();
    CanPortInit();

    /* 每模块的发送帧类型固定：CAN-A 标准帧、CAN-B 扩展帧。
       失败只影响端口层可用性，所以单独记一位，不阻断 PWM 放行。 */
    if (CanPortSetTxFrameType(HW_CAN_ID_B, 1U) == 0U)
    {
        stBringupStatus.unError.Bits.bPortInitError = 1U;
    }
}

/**
 * @brief Initialize application data objects when an application is added.
 */
void ApplicationInit(void)
{
    /* No application objects in the peripheral bring-up image. */
}

/**
 * @brief Register background tasks when application services are added.
 */
void TaskCreate(void)
{
    /* 1 kHz display filter over the L3 / L4 sampling results. */
    if (Task_Start(AdcSampleTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }

    /* 1 ms port service: recovery first, then TX, inside one 20 us budget.
       A port layer that failed to initialize is not registered at all, so the
       heartbeat and this task cannot disagree about whether it is usable. */
    if (stBringupStatus.unError.Bits.bPortInitError == 0U)
    {
        if (PortTaskRegister() == 0U)
        {
            stBringupStatus.unError.Bits.bTaskCreateError = 1U;
        }
    }

#if (PORT_BENCH_TEST != 0U)
    /* 上板测量任务（§8.4）：回显或灌满 TX，1 ms 一次 */
    if (Task_Start(BenchTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }
#endif
}

/**
 * @brief Start time bases and interrupts without releasing power outputs.
 */
void StartSystem(void)
{
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);
    if (stBringupStatus.unError.all == 0U)
    {
        Hw_EnableAllPwmCLKSync();
        Hw_OpenAllPWM();
        stBringupStatus.bReady = 1U;
    }

    EINT;
    ERTM;
    Hw_EnableWatchDog();
    Hw_FeedWatchDog();
}

static __interrupt void Isr_Control(void)
{
    Uint16 next;
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 1U)
    {
        BenchMarkPin(1U);
    }
#endif
    next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;

    /* Sample the 20 result registers of this tick before advancing the MUX. */
    AdcSampleFast();

    stBringupStatus.u32ControlIsrCount++;
    stBringupStatus.u16CalState = stSampleZero.u16State;
    stBringupStatus.bCalReady = (stSampleZero.u16Valid != 0U) ? 1U : 0U;

    /* CLA selects the next MUX address; CPU owns the GPIO writes. */
    Hw_GpioWritePin(46U, next & 1U);
    Hw_GpioWritePin(47U, (next >> 1U) & 1U);
    Hw_GpioWritePin(42U, (next >> 2U) & 1U);
    CpuToCla1Msg.u16MuxAddress = next;

    Cla1Regs.MICLR.bit.INT1 = 1U;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 1U)
    {
        BenchMarkPin(0U);
    }
#endif
}

static __interrupt void Isr_Heartbeat(void)
{
    static Uint32 lastControlCount = 0UL;

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 0U)
    {
        BenchIsrMarkBegin();
    }
#endif

    /* TIF is cleared here only; Hw_CpuTimerIsrTick() does not touch it. */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    stBringupStatus.u32Tick100us++;

    /* 100 us SCI RX service runs before the 1 ms divider so it keeps its own
       cadence; its budget is bounded by SCI_PORT_RX_QUOTA x 8 reads.
       PortTaskServiceFromHeartbeat() honours bPortInitError: a port layer that
       failed to initialize is never serviced from here either. */
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 2U)
    {
        BenchMarkPin(1U);
    }
#endif
    PortTaskServiceFromHeartbeat(stBringupStatus.unError.Bits.bPortInitError);
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 2U)
    {
        BenchMarkPin(0U);
    }
#endif
#if (PORT_BENCH_TEST != 0U)
    /* 模式 11：空窗口参考——两次 BenchMarkPin() 之间不放任何代码。
       它不含 7~10 的模式判断与跨文件入口，结构不同，不能当作等价标定。 */
    if (g_u16BenchMarkMode == 11U)
    {
        BenchMarkPin(1U);
        BenchMarkPin(0U);
    }
#endif
#if (PORT_BENCH_TEST != 0U)
    /* 诊断模式 4/5/6：同一段重复 BENCH_AMP 次，用来拆出每轮、每次取时、
       每次状态查询各自的单价。注意模式 4 会让该心跳内共执行 5 轮正常服务，
       改变端口状态，只作专项诊断。 */
    if ((g_u16BenchMarkMode == 4U) || (g_u16BenchMarkMode == 5U) ||
        (g_u16BenchMarkMode == 6U))
    {
        ST_SCI_RX_STATUS stTmpStatus;
        Uint16 i2;

        BenchMarkPin(1U);
        for (i2 = 0U; i2 < BENCH_AMP; i2++)
        {
            if (g_u16BenchMarkMode == 4U)
            {
                (void)SciPortService100us();
            }
            else if (g_u16BenchMarkMode == 5U)
            {
                (void)Hw_TimeBaseTicks();
            }
            else
            {
                (void)Hw_SciGetRxStatus(HW_SCI_ID_A, &stTmpStatus, 0U);
                (void)Hw_SciGetRxStatus(HW_SCI_ID_C, &stTmpStatus, 0U);
                (void)Hw_SciGetRxStatus(HW_SCI_ID_D, &stTmpStatus, 0U);
            }
        }
        BenchMarkPin(0U);
    }
#endif

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 3U)
    {
        BenchMarkPin(1U);
    }
#endif
    /* u16TickDivCoef = 10: one return of 1 every 10 hardware interrupts, i.e. 1 ms. */
    if (Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer1) != 0U)
    {
        stBringupStatus.u32HeartbeatMs++;
        Task_OnTick();

        if (lastControlCount == stBringupStatus.u32ControlIsrCount)
        {
            if (stBringupStatus.u16ControlStallMs != 0xFFFFU)
            {
                stBringupStatus.u16ControlStallMs++;
            }
        }
        else
        {
            stBringupStatus.u16ControlStallMs = 0U;
            lastControlCount = stBringupStatus.u32ControlIsrCount;
        }
    }
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 3U)
    {
        BenchMarkPin(0U);
    }
#endif

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 0U)
    {
        BenchIsrMarkEnd();
    }
#endif
}

void main(void)
{
    PeripheralsInit(); // Initialize peripherals.
    PlatformInit();    // Initialize platform components.
    ApplicationInit(); // Initialize application data objects.
    TaskCreate();      // Register all background tasks.
    StartSystem();     // Start the system.

    while (1)
    {
        Hw_FeedWatchDog(); // Feed the watchdog.
        Task_Run();
    }
}

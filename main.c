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
#include "SampleData.h"
#include "AdcSample.h"

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
    Uint16 next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;

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
}

static __interrupt void Isr_Heartbeat(void)
{
    static Uint32 lastControlCount = 0UL;

    /* TIF is cleared here only; Hw_CpuTimerIsrTick() does not touch it. */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    stBringupStatus.u32Tick100us++;

    /* 100 us SCI RX service runs before the 1 ms divider so it keeps its own
       cadence; its budget is bounded by SCI_PORT_RX_QUOTA x 8 reads.
       PortTaskServiceFromHeartbeat() honours bPortInitError: a port layer that
       failed to initialize is never serviced from here either. */
    PortTaskServiceFromHeartbeat(stBringupStatus.unError.Bits.bPortInitError);

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

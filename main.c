/**
 * @file main.c
 * @brief Minimal F28374S peripheral bring-up and lifecycle entry points.
 */

#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "F2837xS_Pie_defines.h"
#include "DrvCfg_EP15K.h"
#include "Hw_ClaTask.h"
#include "Hw_WatchDog.h"
#include "Schedule.h"

volatile Uint32 gHeartbeatMs = 0UL;
volatile Uint32 gControlIsrCount = 0UL;
volatile Uint16 gControlStallMs = 0U;
volatile Uint16 gBringupReady = 0U;
volatile Uint16 gBringupError = 0U;

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
        gBringupError |= 8U;
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
        gBringupError |= 1U;
    }

    CpuToCla1Msg.u16MuxAddress = 0U;
    CpuToCla1Msg.fAlpha = fClaMuxAlpha;
    for (i = 0U; i < 8U; i++)
    {
        CpuToCla1Msg.u16NextMux[i] = stClaMuxNext[i];
    }
    Hw_GpioWritePin(46U, 0U);
    Hw_GpioWritePin(47U, 0U);
    Hw_GpioWritePin(42U, 0U);

    if (Hw_SciInit(&stSciCfg) == 0U)
    {
        gBringupError |= 2U;
    }
    Hw_CanInit(&stCanCfg);
    if ((Hw_CanIsInitialized(HW_CAN_ID_A) == 0U) ||
        (Hw_CanIsInitialized(HW_CAN_ID_B) == 0U))
    {
        gBringupError |= 4U;
    }
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
}

/**
 * @brief Initialize platform components. Only the scheduler exists today.
 */
void PlatformInit(void)
{
    Task_Init();
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
    /* The 1 ms heartbeat is available, but no application task is registered. */
}

/**
 * @brief Start time bases and interrupts without releasing power outputs.
 */
void StartSystem(void)
{
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);
    if (gBringupError == 0U)
    {
        Hw_EnableAllPwmCLKSync();
        gBringupReady = 1U;
    }

    EINT;
    ERTM;
    Hw_EnableWatchDog();
    Hw_FeedWatchDog();
}

static __interrupt void Isr_Control(void)
{
    Uint16 next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;
    gControlIsrCount++;

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

    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);
    gHeartbeatMs++;
    Task_OnTick();

    if (lastControlCount == gControlIsrCount)
    {
        if (gControlStallMs != 0xFFFFU)
        {
            gControlStallMs++;
        }
    }
    else
    {
        gControlStallMs = 0U;
        lastControlCount = gControlIsrCount;
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

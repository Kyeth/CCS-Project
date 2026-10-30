/* F28374S minimal peripheral bring-up. No application state machines. */
#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "F2837xS_Pie_defines.h"
#include "DrvCfg_EP15K.h"
#include "Hw_ClaTask.h"

volatile Uint32 gHeartbeatMs = 0UL;
volatile Uint32 gControlIsrCount = 0UL;
volatile Uint16 gControlStallMs = 0U;
volatile Uint16 gBringupReady = 0U;
volatile Uint16 gBringupError = 0U;

static __interrupt void Isr_Control(void);
static __interrupt void Isr_Heartbeat(void);

static void BoardConfigureInterrupts(void)
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

static void BoardConfigurePeripherals(void)
{
    Uint16 i;
    if (Hw_ClockInit(&stClockCfg) == 0U ||
        stSciCfg.u32LspclkHz != stClockCfg.u32LspclkHz ||
        stCanCfg.u32SysclkHz != stClockCfg.u32SysclkHz)
    {
        gBringupError |= 8U;
    }
    BoardConfigureInterrupts();

    /* GPIO88 defaults low and all relay outputs default open in the Broad table. */
    Hw_GpioInit(&stGpioCfg);
    Hw_GpioWritePin(88U, 0U);

    Hw_PwmInit(&stPwmCfg);
    Hw_StopAllPWM();            /* OST latches every gate output low. */
    Hw_XbarInit(&stXbarCfg);
    Hw_PwmInitTripZone(&stPwmTzCfg);
    Hw_AdcInit(&stAdcCfg);
    if (Hw_ClaInit(&stClaCfg) == 0U) { gBringupError |= 1U; }

    CpuToCla1Msg.u16MuxAddress = 0U;
    CpuToCla1Msg.fAlpha = fClaMuxAlpha;
    for (i = 0U; i < 8U; i++)
    {
        CpuToCla1Msg.u16NextMux[i] = stClaMuxNext[i];
    }
    Hw_GpioWritePin(46U, 0U);
    Hw_GpioWritePin(47U, 0U);
    Hw_GpioWritePin(42U, 0U);

    if (Hw_SciInit(&stSciCfg) == 0U) { gBringupError |= 2U; }
    Hw_CanInit(&stCanCfg);
    if ((Hw_CanIsInitialized(HW_CAN_ID_A) == 0U) ||
        (Hw_CanIsInitialized(HW_CAN_ID_B) == 0U)) { gBringupError |= 4U; }
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);

    /* SOC and CLA run while all gate PWM outputs remain hardware latched off. */
    if (gBringupError == 0U)
    {
        Hw_EnableAllPwmCLKSync();
        gBringupReady = 1U;
    }
    EINT;
    ERTM;
    Hw_WatchdogEnable();
}


static __interrupt void Isr_Control(void)
{
    Uint16 next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;
    gControlIsrCount++;

    /* CLA chooses the next address from the Broad table. CPU owns GPIO writes. */
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
    if (lastControlCount == gControlIsrCount)
    {
        if (gControlStallMs != 0xFFFFU) { gControlStallMs++; }
    }
    else
    {
        gControlStallMs = 0U;
        lastControlCount = gControlIsrCount;
    }
}

void main(void)
{
    BoardConfigurePeripherals();
    for (;;)
    {
        /* No application task runs in the minimal image. */
        ServiceDog();
    }
}

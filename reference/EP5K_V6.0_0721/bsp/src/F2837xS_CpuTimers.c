/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: F2837xS_CpuTimers.c
Author: tzz
Version : 0.0.2
Date:2022年02月11日
Description: 定时器 配置文件
Function List:
***********************************************************/
//
// Included Files
//
#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "bsp.h"

interrupt void cpu_timer0_isr(void);
//
// Globals
//
struct CPUTIMER_VARS CpuTimer0;
struct CPUTIMER_VARS CpuTimer1;
struct CPUTIMER_VARS CpuTimer2;

//
// InitCpuTimers - This function initializes all three CPU timers to a known
//                 state.
//
void InitCpuTimers(void)
{
    //
    // CPU Timer 0
    // Initialize address pointers to respective timer registers:
    //
    CpuTimer0.RegsAddr = &CpuTimer0Regs;

    //
    // Initialize timer period to maximum:
    //
    CpuTimer0Regs.PRD.all  = 0xFFFFFFFF;

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT):
    //
    CpuTimer0Regs.TPR.all  = 0;
    CpuTimer0Regs.TPRH.all = 0;

    //
    // Make sure timer is stopped:
    //
    CpuTimer0Regs.TCR.bit.TSS = 1;

    //
    // Reload all counter register with period value:
    //
    CpuTimer0Regs.TCR.bit.TRB = 1;

    //
    // Reset interrupt counters:
    //
    CpuTimer0.InterruptCount = 0;
    //
    // Initialize address pointers to respective timer registers:
    //
    CpuTimer1.RegsAddr = &CpuTimer1Regs;
    CpuTimer2.RegsAddr = &CpuTimer2Regs;

    //
    // Initialize timer period to maximum:
    //
    CpuTimer1Regs.PRD.all  = 0xFFFFFFFF;
    CpuTimer2Regs.PRD.all  = 0xFFFFFFFF;

    //
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT):
    //
    CpuTimer1Regs.TPR.all  = 0xc7c7;        // 1us
    CpuTimer1Regs.TPRH.all = 0;


    CpuTimer2Regs.TPR.all  = 0;
    CpuTimer2Regs.TPRH.all = 0;

    //
    // Make sure timers are stopped:
    //
    CpuTimer1Regs.TCR.bit.TSS = 1;
    CpuTimer2Regs.TCR.bit.TSS = 1;

    //
    // Reload all counter register with period value:
    //
    CpuTimer1Regs.TCR.bit.TRB = 1;
    CpuTimer2Regs.TCR.bit.TRB = 1;

    //
    // Reset interrupt counters:
    //
    CpuTimer1.InterruptCount = 0;
    CpuTimer2.InterruptCount = 0;
}

//
// ConfigCpuTimer - This function initializes the selected timer to the period
//                  specified by the "Freq" and "Period" parameters. The "Freq"
//                  is entered as "MHz" and the period in "uSeconds". The timer
//                  is held in the stopped state after configuration.
//
void ConfigCpuTimer(struct CPUTIMER_VARS *Timer, float Freq, float Period)
{
    Uint32 temp;

    //
    // Initialize timer period:
    //
    Timer->CPUFreqInMHz = Freq;
    Timer->PeriodInUSec = Period;
    temp = (long) (Freq * Period);

    //
    // Counter decrements PRD+1 times each period
    //
    Timer->RegsAddr->PRD.all = temp - 1;

    //
    // Set pre-scale counter to divide by 1 (SYSCLKOUT):
    //
    Timer->RegsAddr->TPR.all  = 0;
    Timer->RegsAddr->TPRH.all  = 0;

    //
    // Initialize timer control register:
    //
    Timer->RegsAddr->TCR.bit.TSS = 1;     // 1 = Stop timer, 0 = Start/Restart
                                          // Timer
    Timer->RegsAddr->TCR.bit.TRB = 1;     // 1 = reload timer
    Timer->RegsAddr->TCR.bit.SOFT = 0;
    Timer->RegsAddr->TCR.bit.FREE = 0;    // Timer Free Run Disabled
    Timer->RegsAddr->TCR.bit.TIE = 0;     // 0 = Disable/ 1 = Enable Timer

    //
    // Reset interrupt counter:
    //
    Timer->InterruptCount = 0;
}
/*************************************************
Function: User_usTimer
Description: us及tick定时器
Input: @User_Timer-计数器地址,@mSec定时时间设置值
Output: None
Frequency: main
*************************************************/
Uint16 User_usTimer(Uint32 *User_Timer, Uint32 mSec)     //1us定时
{
    Uint16 timeout = 0;

    if ((mSec == 0)||((*User_Timer) - (CpuTimer1.RegsAddr->TIM.all) >= mSec)) //定时器为减计数
    {
        *User_Timer = CpuTimer1.RegsAddr->TIM.all;
        timeout = 1;
    }

    return (timeout);
}
//***************************************************************************
Uint32 User_usCount(mSecCNT_struct *mSecCNT)    //us延时
{
    Uint32 msCount = 0;
    if (mSecCNT->reset == 0)
    {
        mSecCNT->reset = 1;
        mSecCNT->Timer = CpuTimer1.RegsAddr->TIM.all;
    }
    else
    {
        msCount = (mSecCNT->Timer) - (CpuTimer1.RegsAddr->TIM.all);
    }
    return(msCount); //返回计数值
}
void InitUserTimers(void)
{
    InitCpuTimers();

    ConfigCpuTimer(&CpuTimer0, 200, 100);       // 100us interrupt
    //ConfigCpuTimer(&CpuTimer1, 200, 1);         // 1us task
//    ConfigCpuTimer(&CpuTimer2, 200, 100);       // 1us measure

//    EALLOW;
//    PieVectTable.TIMER0_INT = &cpu_timer0_isr;
//    EDIS;

//    CpuTimer0Regs.TCR.bit.TSS = 0;
    CpuTimer1Regs.TCR.bit.TSS = 0;      // 非阻塞延时
    CpuTimer2Regs.TCR.bit.TSS = 0;      // 代码运行时长计算

#if 0
    // timer0 interrupt configuration
    CpuTimer0.RegsAddr->TCR.bit.TIE = 1;
    IER |= M_INT1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
#endif

}
//
// End of file
//

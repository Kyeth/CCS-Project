/**
 * @file
 * @author yetingheng  (004182)
 * @brief
 * @version 0.1
 * @date 2026-03-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <Hw_CpuTimer.h>
#include "InterruptCfg.h"
#include "interrupt.h"
#include "hw_ints.h"
#include "Hw_CanDriver.h"
#include "Hw_SciDriver.h"
#include "Hw_I2cDriver.h"
#include "Hw_AdcDriver.h"
#include "Hw_GpioDriver.h"
#include "Hw_SysCpuTimer.h"
#include "Hw_ClaDriver.h"
#include "Hw_MCanDriver.h"
#include "Schedule.h"
#include "f28003x_piectrl.h"
#include "f28003x_pie_defines.h"
#include "Hw_WatchDog.h"
#include "LLCControl.h"
#include "SysStateMachine.h"
#include "PlatformManager.h"
#include "Filter.h"
#include "Hw_PwmDriver.h"
#include "TaskCla.h"
#include "FaultProtect.h"
#include "ParallelSubFunc.h"
#include "Heating.h"
#include "Led.h"

extern void User_Can(void);
Uint16 bFanState = 0;

#ifdef __cplusplus
    extern "C" {
#endif

#pragma CODE_SECTION(CpuTimer1Isr, ".ISR.ramfunc");
__interrupt void CpuTimer1Isr(void)
{
//    Hw_GpioWritePin(34, 0);

    // 这里清除CPU0Timer中断标志位
    CpuTimer1Regs.TCR.bit.TIF=1;
    ++stSysCpuTimer1.TimerDivCntPtd;
    if (stSysCpuTimer1.TimerDivCntPtd >= stSysCpuTimer1.TimerFreqDivCoef)
    {
        stSysCpuTimer1.TimerTick++;
        stSysCpuTimer1.TimerDivCntPtd = 0;
    }

    // 获取ADC原始值
    Hw_AdcRead(ADC_ID0, (Uint16 **)&ObjPlatformCtrl.pAdcARawResult);
    Hw_AdcRead(ADC_ID1, (Uint16 **)&ObjPlatformCtrl.pAdcBRawResult);
    Hw_AdcRead(ADC_ID2, (Uint16 **)&ObjPlatformCtrl.pAdcCRawResult);

    ObjPlatformCtrl.u16AdcARawResult[4] = FilterAvgProcsee(&stAdcFilter1, ObjPlatformCtrl.pAdcARawResult[4]);   // A4: NTC2采样
//    ObjPlatformCtrl.u16AdcARawResult[5] = FilterAvgProcsee(&stAdcFilter2, ObjPlatformCtrl.pAdcARawResult[5]);   // A5: NTC1采样已拆除
    ObjPlatformCtrl.u16AdcARawResult[6] = FilterAvgProcsee(&stAdcFilter3, ObjPlatformCtrl.pAdcARawResult[6]);   // A6: 内部环境温度采样
    ObjPlatformCtrl.u16AdcCRawResult[5] = FilterAvgProcsee(&stAdcFilter10, ObjPlatformCtrl.pAdcCRawResult[5]);  // C5: 变压器2温度采样
    ObjPlatformCtrl.u16AdcBRawResult[5] = FilterAvgProcsee(&stAdcFilter8, ObjPlatformCtrl.pAdcBRawResult[5]);   // B5：低压管1温度采样

    ObjPlatformCtrl.u16AdcARawResult[10] = FilterAvgProcsee(&stAdcFilter13, ObjPlatformCtrl.pAdcARawResult[10]);    // A10:加热膜电流采样
//    ObjPlatformCtrl.u16AdcARawResult[11] = FilterProcsee(&stAdcFilter4, ObjPlatformCtrl.pAdcARawResult[11]);    // A11: 支路1电流采样
//    ObjPlatformCtrl.u16AdcARawResult[11] = FilterAvgProcsee(&stAdcFilter4, ObjPlatformCtrl.pAdcARawResult[11]);
    ObjPlatformCtrl.u16AdcARawResult[11] = ObjPlatformCtrl.pAdcARawResult[11];
    ObjPlatformCtrl.u16AdcBRawResult[0] = FilterProcsee(&stAdcFilter5, ObjPlatformCtrl.pAdcBRawResult[0]);    // B0: 输出电流采样
    ObjPlatformCtrl.u16AdcBRawResult[2] = FilterProcsee(&stAdcFilter6, ObjPlatformCtrl.pAdcBRawResult[2]);    // B2: VBat电压采样
    ObjPlatformCtrl.u16AdcBRawResult[3] = FilterProcsee(&stAdcFilter7, ObjPlatformCtrl.pAdcBRawResult[3]);    // B3: 空开前端电压采样
//    ObjPlatformCtrl.u16AdcCRawResult[2] = FilterProcsee(&stAdcFilter9, ObjPlatformCtrl.pAdcCRawResult[2]);    // C2: 支路2电流采样
//    ObjPlatformCtrl.u16AdcCRawResult[2] = FilterAvgProcsee(&stAdcFilter9, ObjPlatformCtrl.pAdcCRawResult[2]);
    ObjPlatformCtrl.u16AdcCRawResult[2] = ObjPlatformCtrl.pAdcCRawResult[2];

    ObjPlatformCtrl.u16AdcCRawResult[7] = FilterProcsee(&stAdcFilter11, ObjPlatformCtrl.pAdcCRawResult[7]);   // C7: 空开后端电压采样
//    ObjPlatformCtrl.u16AdcCRawResult[9] = FilterProcsee(&stAdcFilter12, ObjPlatformCtrl.pAdcCRawResult[9]);   // C9: VBus电压采样
    ObjPlatformCtrl.u16AdcCRawResult[9] = ObjPlatformCtrl.pAdcCRawResult[9];

    // 零飘校准
    ZeroDirftCheck(&ObjPlatformCtrl);

    // 转换成实际电压、电流值
    ObjSample.VBusRaw       = ObjPlatformCtrl.u16AdcCRawResult[9] * ObjSample.VBusSampCoefficient;
    ObjSample.VBus          = FilterProcsee(&stAdcFilter12, ObjSample.VBusRaw);
    ObjSample.VBusFilter    = GetFilterTrap(&ObjBusFilter, ObjSample.VBus, 50.0f);
    ObjSample.VBusFilter1   = FilterAvgProcsee(&stAdcFilter12, ObjSample.VBus);

    ObjSample.VAirFront  = ObjPlatformCtrl.u16AdcBRawResult[3] * ObjSample.VAirFrontSampCoefficient;
    ObjSample.VAirBack   = ObjPlatformCtrl.u16AdcCRawResult[7] * ObjSample.VAirBackSampCoefficient;

    ObjSample.VBatRaw    = ObjPlatformCtrl.u16AdcBRawResult[2] * ObjSample.VBatSampCoefficient;
    ObjSample.VBatSum    = ObjSample.VBatSum + ObjSample.VBatRaw - ObjSample.VBatSum * 0.001f;
    ObjSample.VBat       = ObjSample.VBatSum * 0.001f;

    ObjSample.IBusRaw    = ((int16)(ObjPlatformCtrl.u16AdcBRawResult[0] - ObjPlatformCtrl.u16IBusZeroDirft)) * ObjSample.IBusSampCoefficient;
    ObjSample.IBusSum    = ObjSample.IBusSum + ObjSample.IBusRaw - ObjSample.IBusSum * 0.0001f;
    ObjSample.IBus       = ObjSample.IBusSum * 0.0001f;

    ObjSample.IBranch_1  = ObjPlatformCtrl.u16AdcARawResult[11] * ObjSample.IBranch_1SampCoefficient;
    ObjSample.IBranch_2  = ObjPlatformCtrl.u16AdcCRawResult[2] * ObjSample.IBranch_2SampCoefficient;

    ObjSample.AvgPower = (int16)(ObjSample.VBusFilter1 * ObjSample.IBus);

    ObjSample.InstantPower = (int16)(ObjSample.VBusRaw * ObjSample.IBusRaw);
    ObjSample.PowerSum += ObjSample.InstantPower;
    ObjSample.PowerCnt++;

    if(ObjSample.DoubleSet == 0)
    {
        ObjSample.DoubleK = 1.0f;
    }
    else if(ObjSample.DoubleSet == 1)
    {
        ObjSample.DoubleK = 0.5f;
    }

    ObjSample.IErr = (ObjSample.IBranch_1 + ObjSample.IBranch_2) * ObjSample.DoubleK;


    ObjSample.Heating = ObjPlatformCtrl.u16AdcARawResult[10] * ObjSample.HeatingSampCoefficient;

    // 系统平台完成初始化后才运行执行其他逻辑
    if (GetPlatformIsInit(&ObjPlatformCtrl) == 1)
    {
        CheckRef(); // 设定目标电压

        // 数据交互
        CpuAdcSample.Vbat = ObjSample.VBat;
        CpuAdcSample.Vout = ObjSample.VBus;
        CpuAdcSample.Iout = fabsf(ObjSample.IBus);
        CpuAdcSample.IBus = fabsf(ObjSample.IBusRaw);
        CpuAdcSample.IErr = ObjSample.IErr;//ObjSample.IErr;
        CpuAdcSample.SetCurr = GetSetCurr(ObjSample.VBat);
        CpuAdcSample.TargetAvgCurr = fabsf(GetAvgCurr(&stBatteryHandle, &ObjSystemCtrl));
        CpuAdcSample.AgingISet = fabsf(LLCSystemCtrl.AgingISet);
        //---------
        CpuAdcSample.Vref = LLCSystemCtrl.Vref;

        CpuAdcSample.ParalelCtrlState = GetDeviceMode(&stBatteryHandle);
        if(stBatteryHandle.u16Soc >= 998)
        {
            LLCSystemCtrl.C2DVoltGap = 16.0f;
            LLCSystemCtrl.CBackVoltGap = 12.0f;
            LLCSystemCtrl.D2CVoltGap = 25.0f;
            LLCSystemCtrl.DBackVoltGap = 22.0f;
        }
        else
        {
            LLCSystemCtrl.C2DVoltGap = 16.0f;
            LLCSystemCtrl.CBackVoltGap = 12.0f;
            LLCSystemCtrl.D2CVoltGap = 15.0f;
            LLCSystemCtrl.DBackVoltGap = 12.0f;
        }
        CtrlPara.fPeriod = LLCSystemCtrl.Prd;

        // 触发充放电设置
        if ((ObjSystemCtrl.SysState == SYS_STATE_NORMAL_OPERATION) && (!stSysProtect.u16TotalFaultSign))
        {
            LoopCtrl();
        }
        else
        {
            LLCSystemCtrl.RelayFlag = 0;
            LLCSystemCtrl.SoftFinish = 0;
            // 断开主继电器
            Hw_GpioWritePin(23, 0);
            // 断开预充电阻
            Hw_GpioWritePin(41, 0);
            // 这里修改状态机以及关波是为了避免在系统状态机中关波异常的问题
            FreezePWMOutput();
            ObjSystemCtrl.LLCState = LLC_IDLE_STATE;
            ObjSystemCtrl.ParamFlag = 0;
        }
    }

    // FAN I/O Control
    if (LLCSystemCtrl.FanTestCmd == 0)
    {
        bFanState = 0;
        Hw_GpioWritePin(33, 0);
    }
    else if (LLCSystemCtrl.FanTestCmd == 1)
    {
        if (bFanState == 0)
        {
            bFanState = 1;
            Hw_GpioWritePin(33, 0);
        }
        else
        {
            bFanState = 0;
            Hw_GpioWritePin(33, 1);
        }
    }
    else if (LLCSystemCtrl.FanTestCmd == 2)
    {
        bFanState = 1;
        Hw_GpioWritePin(33, 1);
    }

    SetTaskRunStatus(ISR_TIME1_BIT);
}

#pragma CODE_SECTION(CpuTimer2Isr, ".TI.ramfunc");
__interrupt void CpuTimer2Isr(void)
{
// Hw_GpioWritePin(34, 0);
    // 这里清除CPU0Timer中断标志位
    CpuTimer2Regs.TCR.bit.TIF=1;

    ++stSysCpuTimer2.TimerDivCntPtd;
    if (stSysCpuTimer2.TimerDivCntPtd >= stSysCpuTimer2.TimerFreqDivCoef)
    {
        stSysCpuTimer2.TimerTick++;
        stSysCpuTimer2.TimerDivCntPtd = 0;
    }
    // 确认时基为100us
    TaskCTL.On = 1;
    // 为了减少中断执行时间，把SCI和CAN通信进行了相位偏移调度
    // 在不同的中断周期中执行不同的函数
    switch (stSysCpuTimer2.TimerDivCntPtd)
    {
    case 0:
//        Hw_GpioWritePin(34, 0);
        // SCI通信
        Hw_IsrSciReceive(SCI_ID0);
        Hw_IsrSciTransmit(SCI_ID0);
//        Hw_GpioWritePin(34, 1);
    break;

    case 1:
        // CAN通信
        Hw_IsrCanReceive(CAN_ID0);
        Hw_IsrCanTransmit(CAN_ID0);
    break;

    case 2:
        // SCI通信
        Hw_IsrSciReceive(SCI_ID1);
        Hw_IsrSciTransmit(SCI_ID1);
    break;

    case 3:
        Hw_IsrMCanTransmit(MCAN_ID0);
        Hw_IsrMCanReceive(MCAN_ID0);
        break;

    default:
        break;
    }

    // I2C主从状态机数据接收函数，100us调用一次
//    Hw_IsrI2cTransceiver(I2C_ID0);

    User_Can();

    // 系统平台完成初始化后才运行执行其他逻辑
    if (GetPlatformIsInit(&ObjPlatformCtrl) == 1)
    {

    }
    SetTaskRunStatus(ISR_TIME2_BIT);
// Hw_GpioWritePin(34, 1);
}

//-- AdcAIsr中断 --
// 该中断在内存中运行，所有采样均进行滤波，总耗时9.85us
#pragma CODE_SECTION(AdcAIsr, ".TI.ramfunc");
__interrupt void AdcAIsr(void)
{

// Hw_GpioWritePin(34, 0);
// Hw_GpioWritePin(6, 0);

    // 清除ADC中断标志
    Hw_AdcClearInitFlag(ADC_ID0);

    // 获取ADC原始值
    Hw_AdcRead(ADC_ID0, (Uint16 **)&ObjPlatformCtrl.pAdcARawResult);
    Hw_AdcRead(ADC_ID1, (Uint16 **)&ObjPlatformCtrl.pAdcBRawResult);
    Hw_AdcRead(ADC_ID2, (Uint16 **)&ObjPlatformCtrl.pAdcCRawResult);

    ObjPlatformCtrl.u16AdcARawResult[4] = ObjPlatformCtrl.pAdcARawResult[4];
    ObjPlatformCtrl.u16AdcARawResult[5] = ObjPlatformCtrl.pAdcARawResult[5];
    ObjPlatformCtrl.u16AdcARawResult[6] = ObjPlatformCtrl.pAdcARawResult[6];
    ObjPlatformCtrl.u16AdcCRawResult[5] = ObjPlatformCtrl.pAdcCRawResult[5];
    ObjPlatformCtrl.u16AdcBRawResult[5] = ObjPlatformCtrl.pAdcBRawResult[5];

    ObjPlatformCtrl.u16AdcARawResult[11] = FilterProcsee(&stAdcFilter4, ObjPlatformCtrl.pAdcARawResult[11]);    // A11: 支路1电流采样
    ObjPlatformCtrl.u16AdcBRawResult[0] = FilterProcsee(&stAdcFilter5, ObjPlatformCtrl.pAdcBRawResult[0]);    // B0: 输出电流采样
    ObjPlatformCtrl.u16AdcBRawResult[2] = FilterProcsee(&stAdcFilter6, ObjPlatformCtrl.pAdcBRawResult[2]);    // B2: VBat电压采样
    ObjPlatformCtrl.u16AdcBRawResult[3] = FilterProcsee(&stAdcFilter7, ObjPlatformCtrl.pAdcBRawResult[3]);    // B3: 空开前端电压采样
    ObjPlatformCtrl.u16AdcCRawResult[2] = FilterProcsee(&stAdcFilter9, ObjPlatformCtrl.pAdcCRawResult[2]);    // C2: 支路2电流采样
    ObjPlatformCtrl.u16AdcCRawResult[7] = FilterProcsee(&stAdcFilter11, ObjPlatformCtrl.pAdcCRawResult[7]);   // C7: 空开后端电压采样
    ObjPlatformCtrl.u16AdcCRawResult[9] = FilterProcsee(&stAdcFilter12, ObjPlatformCtrl.pAdcCRawResult[9]);   // C9: VBus电压采样

    // 零飘校准
    ZeroDirftCheck(&ObjPlatformCtrl);

    // 转换成实际电压、电流值
    ObjSample.VBus       = ObjPlatformCtrl.u16AdcARawResult[2] * ObjSample.VBusSampCoefficient;
    ObjSample.VAirFront  = ObjPlatformCtrl.u16AdcBRawResult[3] * ObjSample.VAirFrontSampCoefficient;
    ObjSample.VAirBack   = ObjPlatformCtrl.u16AdcARawResult[3] * ObjSample.VAirBackSampCoefficient;
    ObjSample.VBat       = ObjPlatformCtrl.u16AdcBRawResult[2] * ObjSample.VBatSampCoefficient;
    ObjSample.IBus       = ((int16)(ObjPlatformCtrl.u16AdcBRawResult[0] - ObjPlatformCtrl.u16IBusZeroDirft)) * ObjSample.IBusSampCoefficient;
    ObjSample.IBranch_1  = ObjPlatformCtrl.u16AdcARawResult[11] * ObjSample.IBranch_1SampCoefficient;
    ObjSample.IBranch_2  = ObjPlatformCtrl.u16AdcCRawResult[2] * ObjSample.IBranch_2SampCoefficient;


    ObjPlatformCtrl.u16AdcCnt++;
    // 清除PIE中断标志
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

// Hw_GpioWritePin(6, 1);
// Hw_GpioWritePin(34, 1);

}

// 该中断为CLA任务处理结束后调用(不启用)
__interrupt void TaskCla_1_Isr(void)
{
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP11);
}

#pragma CODE_SECTION(Gpio47_Isr, ".TI.ramfunc");
__interrupt void Gpio47_Isr(void)
{
    // 这里需要封波
    if (stSysProtect.unFaultMaskWord3.Bits.bit1 != 1)
    {
        FreezePWMOutput();
        stSysProtect.u16TotalFaultSign = 1;
        stSysProtect.unFaultStatusWord3.Bits.bit1 = 1;
        Hw_GpioWritePin(34, 0);
        stSysProtect.u16HardFPTakesEffect = 0;
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

#pragma CODE_SECTION(Gpio49_Isr, ".TI.ramfunc");
__interrupt void Gpio49_Isr(void)
{
    // 这里需要封波
    if (stSysProtect.unFaultMaskWord3.Bits.bit2 != 1)
    {
        FreezePWMOutput();
        stSysProtect.u16TotalFaultSign = 1;
        stSysProtect.unFaultStatusWord3.Bits.bit2 = 1;
        Hw_GpioWritePin(34, 0);
        stSysProtect.u16HardFPTakesEffect = 0;
    }

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}


#pragma CODE_SECTION(Gpio53_Isr, ".TI.ramfunc");
__interrupt void Gpio53_Isr(void)
{
    // 这里需要封波
    if (stSysProtect.unFaultMaskWord3.Bits.bit3 != 1)
    {
        FreezePWMOutput();

        stSysProtect.u16TotalFaultSign = 1;
        stSysProtect.unFaultStatusWord3.Bits.bit3 = 1;
        Hw_GpioWritePin(34, 0);
        stSysProtect.u16HardFPTakesEffect = 0;
    }
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP12);
}

#pragma CODE_SECTION(PWMIsr, ".ISR.ramfunc");
__interrupt void PWMIsr(void)
{
    Hw_GpioWritePin(6, 1);
    EPwm1Regs.ETCLR.bit.INT = 1;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP3);
    Hw_GpioWritePin(6, 0);
}

//-- 中断向量表初始化 --
void InterruptVectTableInit(void)
{
    // 清除中断标志，使能外设中断״̬
    Interrupt_initModule();

    // 复位中断向量表
    Interrupt_initVectorTable();

    // 注册Timer1的中断复位函数
    Interrupt_register(INT_TIMER1, CpuTimer1Isr);
    Interrupt_enable(INT_TIMER1);

    // 注册Timer2的中断复位函数
    Interrupt_register(INT_TIMER2, CpuTimer2Isr);
    Interrupt_enable(INT_TIMER2);

    // 注册GPIO47外部中断
    Interrupt_register(INT_XINT1, Gpio47_Isr);
    Interrupt_enable(INT_XINT1);

    // 注册GPIO49外部中断
    Interrupt_register(INT_XINT2, Gpio49_Isr);
    Interrupt_enable(INT_XINT2);

    // 注册GPIO53外部中断
    Interrupt_register(INT_XINT3, Gpio53_Isr);
    Interrupt_enable(INT_XINT3);
}

void NmiIsrHandler(void)
{
    // Close the PWM output
    FreezePWMOutput();
    stSysProtect.u16TotalFaultSign = 1;
    stSysProtect.unFaultStatusWord3.Bits.bit2 = 1;
    Hw_GpioWritePin(34, 0);
    stSysProtect.u16HardFPTakesEffect = 0;

    // Enable the Watch dog , Waiting for the Watch Dog reset DSP
    Hw_EnableWatchDog();
    Hw_FeedWatchDog();

    Hw_GpioWritePin(16, 0); // Fan off
    Hw_GpioWritePin(23, 0); // 关闭主继电器

    LedCloseFaultStatus();
}

#ifdef __cplusplus
    }
#endif

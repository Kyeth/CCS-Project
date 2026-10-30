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
#include <cpu.h>
#include <Hw_ClaDriver.h>
#include <Hw_GpioDriver.h>
#include <Hw_PwmDriver.h>
#include <LLCControl.h>
#include <SysStateMachine.h>
#include "string.h"
#include "ParallelSubFunc.h"
#include "EquipmentParallelCore.h"
#include "FaultProtect.h"
#include "TaskCla.h"

struct FREQ_MPPT FreqMppt;
LLC_SYSTEM_CONTROL_PARA LLCSystemCtrl;

#pragma DATA_SECTION(stClaOutputParam, "Cla1DataRam");
__attribute__((aligned(4))) ST_CLA_OUTPUT stClaOutputParam;     // CLA的运算输出结果
__attribute__((aligned(4))) ST_CLA_OUTPUT stReadClaOutputParam; // 存储CLA的结果以便用来读取

// CPU、CLA共享数据
#pragma DATA_SECTION(CtrlPara, "Cla1DataRam");
__attribute__((aligned(4))) CTRLPARA CtrlPara;

#pragma DATA_SECTION(CpuAdcSample, "CpuToCla1MsgRAM");
__attribute__((aligned(4))) CPUADCSAM CpuAdcSample;

#pragma CODE_SECTION(RmpCntlCal, ".ISR.ramfunc");
float RmpCntlCal(RAMP *v)
{
    if (v->TargetValue > v->SetpointValue + v->StepValue)
    {
        v->SetpointValue = v->SetpointValue + v->StepValue;
    }
    else if (v->TargetValue < v->SetpointValue - v->StepValue)
    {
        v->SetpointValue = v->SetpointValue - v->StepValue;
    }
    else
    {
        v->SetpointValue = v->TargetValue;
    }

    if (v->SetpointValue > v->RampHighLimit)
    {
        v->SetpointValue = v->RampHighLimit;
    }
    else if (v->SetpointValue < v->RampLowLimit)
    {
        v->SetpointValue = v->RampLowLimit;
    }
    return v->SetpointValue;
}

void InitTestData(void)
{
    LLCSystemCtrl.VrefRamp.TargetValue = 0.0f;
    LLCSystemCtrl.VrefRamp.RampLowLimit = 0.0f;
    LLCSystemCtrl.VrefRamp.RampHighLimit = 480.0f;
    LLCSystemCtrl.VrefRamp.StepValue = 0.005f;//每秒变化100V
    LLCSystemCtrl.VrefRamp.SetpointValue = 0.0f;

    LLCSystemCtrl.PowerRamp.TargetValue = 0.0f;
    LLCSystemCtrl.PowerRamp.RampLowLimit = -4000.0f;
    LLCSystemCtrl.PowerRamp.RampHighLimit = 6000.0f;
    LLCSystemCtrl.PowerRamp.StepValue = 30.0f;//每10毫秒变化6000W
    LLCSystemCtrl.PowerRamp.SetpointValue = 0.0f;

    LLCSystemCtrl.AgingRamp.TargetValue = 0.0f;
    LLCSystemCtrl.AgingRamp.RampLowLimit = -10.0f;
    LLCSystemCtrl.AgingRamp.RampHighLimit = 14.5f;
    LLCSystemCtrl.AgingRamp.StepValue = 0.00005f;
    LLCSystemCtrl.AgingRamp.SetpointValue = 0.0f;

    CtrlPara.ClaTestDuty.TargetValue = 500.0f;
    CtrlPara.ClaTestDuty.SetpointValue = 580.0f;
    CtrlPara.ClaTestDuty.StepValue = 0.005f;
    CtrlPara.ClaTestDuty.RampHighLimit = 580.0f;
    CtrlPara.ClaTestDuty.RampLowLimit = 500.0f;

    CtrlPara.ClaRoute1Duty.TargetValue = 500.0f;
    CtrlPara.ClaRoute1Duty.SetpointValue = 580.0f;
    CtrlPara.ClaRoute1Duty.StepValue = 0.005f;
    CtrlPara.ClaRoute1Duty.RampHighLimit = 580.0f;
    CtrlPara.ClaRoute1Duty.RampLowLimit = 500.0f;

    CtrlPara.ClaRoute2Duty.TargetValue = 500.0f;
    CtrlPara.ClaRoute2Duty.SetpointValue = 580.0f;
    CtrlPara.ClaRoute2Duty.StepValue = 0.005f;
    CtrlPara.ClaRoute2Duty.RampHighLimit = 580.0f;
    CtrlPara.ClaRoute2Duty.RampLowLimit = 500.0f;

    CtrlPara.ClaRefSoftFreq.TargetValue = 1644.0f;
    CtrlPara.ClaRefSoftFreq.SetpointValue = 1400.0f;
    CtrlPara.ClaRefSoftFreq.StepValue = 0.005f;
    CtrlPara.ClaRefSoftFreq.RampHighLimit = 1644.0f;
    CtrlPara.ClaRefSoftFreq.RampLowLimit = 1200.0f;

    CtrlPara.ClaZeroSoftFreq.TargetValue = 1644.0f;
    CtrlPara.ClaZeroSoftFreq.SetpointValue = 1400.0f;
    CtrlPara.ClaZeroSoftFreq.StepValue = 0.05f;
    CtrlPara.ClaZeroSoftFreq.RampHighLimit = 1644.0f;
    CtrlPara.ClaZeroSoftFreq.RampLowLimit = 1400.0f;

    CtrlPara.fSoftPrd = 1644.0f;
    LLCSystemCtrl.CurrTrigSet = 10.0f;
    // 上电默认参数
    ObjSystemCtrl.ProjectState = IB800JP_STATE;
    ObjSystemCtrl.Open = 0;     // 开关机
    ObjSystemCtrl.ClosedLoopMode = 1;           // 0:开环运行模式 1：闭环运行模式
    ObjSystemCtrl.u16TestMode = 0;              // 0:正常模式；  1:测试模式；
    ObjSystemCtrl.u16TestSoftStartMode = 0;     // 软启模式设定 0:充电软启    1:放电软启CHARGE_SWITCH
    ObjSystemCtrl.u16TestChargeCfgSwitch = 0;   // 0:充电调频闭环模式     1:充电调宽闭环模式
    ObjSystemCtrl.u16TestChargeZoom = 0;        // 用于设定充电切换条件（充电模式调试阶段用）
    CtrlPara.fs1 = 200.0f;
    CtrlPara.PhsOffset = 0.02;

    LLCSystemCtrl.VDROP_ENTRY1 = 25.0f;
    LLCSystemCtrl.VDROP_ENTRY2 = 25.0f;
    LLCSystemCtrl.VDROP_EXIT1 = 15.0f;
    LLCSystemCtrl.VDROP_EXIT2 = 15.0f;
    LLCSystemCtrl.Kdroop1 = 240.0f;
    LLCSystemCtrl.Kdroop2 = 160.0f;

    // 闭环参数
//    LLCSystemCtrl.DeadBranch = 72;//96;//168;//64;
    LLCSystemCtrl.DeadBranch = 42;//168;//96;//64;
    LLCSystemCtrl.DeadPrimary = 42;//108;//64;//48;
    LLCSystemCtrl.BaseFreq = 200.0f;

    CpuAdcSample.ChargeMaxFreq = 200.0f;
    CpuAdcSample.MaxFreq = 200.0f;//150.0f;
    CpuAdcSample.BaseFreq = 100.0f;
    CtrlPara.fClaFreq = 200.0f;
    CtrlPara.fMaxDuty = 44.0f;
    LLCSystemCtrl.C2DVoltGap = 13.0f;
    LLCSystemCtrl.CBackVoltGap = 10.0f;
    LLCSystemCtrl.D2CVoltGap = 15.0f;
    LLCSystemCtrl.DBackVoltGap = 12.0f;
    LLCSystemCtrl.Freq = 200.0f;

    // 软启动参数
    LLCSystemCtrl.ChargeMaxPrd = 428;

    stClaOutputParam.Prd = 0;
    stClaOutputParam.ClaCnt = 0;
    stClaOutputParam.Phase = 0;
    stClaOutputParam.PrimaryCMPA = 0;
    stClaOutputParam.PrimaryCMPB = 0;
    stClaOutputParam.PrimaryDuty = 0;

    stClaOutputParam.BranchCMPA = 0;
    stClaOutputParam.BranchCMPB = 0;
    stClaOutputParam.BranchDuty = 0;

    // 放电前馈给定步长斜坡
    CtrlPara.ClaRefFreq.TargetValue = 200.0f;
    CtrlPara.ClaRefFreq.SetpointValue = 200.0f;
    CtrlPara.ClaRefFreq.StepValue = 5.0f;      // 50Hz
    CtrlPara.ClaRefFreq.RampLowLimit = 75.0f;
    CtrlPara.ClaRefFreq.RampHighLimit = 200.0f;

    // 放电fs0给定斜坡
    CtrlPara.ClaRefFs0.TargetValue = 128.0f;
    CtrlPara.ClaRefFs0.SetpointValue = 128.0f;
    CtrlPara.ClaRefFs0.StepValue = 15.0f;
    CtrlPara.ClaRefFs0.RampHighLimit = 128.0f;
    CtrlPara.ClaRefFs0.RampLowLimit = 75.0f;

    // 放电母线电压环-变频输出
    CtrlPara.DisVoltPICtrl.Kp =  0.1f;      // A/V 每V误差调节的电流大小
    CtrlPara.DisVoltPICtrl.Ki =  0.0005f;     // A/V/s 每秒校正的误差比值
    CtrlPara.DisVoltPICtrl.Umax = 14.0f;      // 最大允许变频频率
    CtrlPara.DisVoltPICtrl.Umin = 0.0f;
    CtrlPara.DisVoltPICtrl.ErrLimit = 50.0f;
    // 放电母线电流环
    CtrlPara.DisCurrPICtrl.RealseEn = 1;
    CtrlPara.DisCurrPICtrl.Kp = 1.2f;       // KHz/A 每安培误差调节的频率大小
    CtrlPara.DisCurrPICtrl.Ki = 0.6f;      // KHz/A/s 每秒消除的KHz/A误差大小
    CtrlPara.DisCurrPICtrl.Umax = 25.0f;    // 最大工作频率
    CtrlPara.DisCurrPICtrl.Umin = -30.0f;
    // 充电电池电压环-频率输出
    CtrlPara.VoltPICtrl.Kp = 0.1f;
    CtrlPara.VoltPICtrl.Ki = 0.0001f;
    CtrlPara.VoltPICtrl.Umax = 10.9f;//6.0f
    CtrlPara.VoltPICtrl.Umin = 0.0f;
    // 充电电池电流环-频率输出
    CtrlPara.CurrPICtrl.Kp = 0.7f;
    CtrlPara.CurrPICtrl.Ki = 0.02f;
    CtrlPara.CurrPICtrl.Umax = 20.0f;
    CtrlPara.CurrPICtrl.Umin = -60.0f;
    // 功率电压环
    CtrlPara.VoltLLCPICtrl.Kp = 0.12f;
    CtrlPara.VoltLLCPICtrl.Ki = 0.0012f;
    CtrlPara.VoltLLCPICtrl.Umax = 6.0f;
    CtrlPara.VoltLLCPICtrl.Umin = 0.0f;
    // 功率环控制-频率输出
    CtrlPara.CurrLLCPICtrl.Kp = 0.004f;
    CtrlPara.CurrLLCPICtrl.Ki = 0.003f;
    CtrlPara.CurrLLCPICtrl.Umax = 0.304452f;
    CtrlPara.CurrLLCPICtrl.Umin = -0.17f;

    // 从机充电电流环
    CtrlPara.PowerCurrPICtrl.Kp = 0.3f;
    CtrlPara.PowerCurrPICtrl.Ki = 0.004f;
    CtrlPara.PowerCurrPICtrl.Umax = 20.0f;
    CtrlPara.PowerCurrPICtrl.Umin = -20.0f;
    // 从机充电电流环
    CtrlPara.PowerCurrLLCPICtrl.Kp = 0.004f;
    CtrlPara.PowerCurrLLCPICtrl.Ki = 0.003f;
    CtrlPara.PowerCurrLLCPICtrl.Umax = 0.304452f;
    CtrlPara.PowerCurrLLCPICtrl.Umin = -0.17f;
    // 从机放电电流环
    CtrlPara.PowerDisCurrPICtrl.Kp = 0.1f;
    CtrlPara.PowerDisCurrPICtrl.Ki = 0.001f;
    CtrlPara.PowerDisCurrPICtrl.Umax = 40.0f;
    CtrlPara.PowerDisCurrPICtrl.Umin = -100.0f;
    // 老化电流源充电
    CtrlPara.ChargeAgingICtrl.Kp = 2.0f;
    CtrlPara.ChargeAgingICtrl.Ki = 0.01f;
    CtrlPara.ChargeAgingICtrl.Umax = 125.0f;
    CtrlPara.ChargeAgingICtrl.Umin = -100.0f;
    // 老化电流源放电
    CtrlPara.DisChargeAgingICtrl.Kp = 2.0f;
    CtrlPara.DisChargeAgingICtrl.Ki = 0.01f;
    CtrlPara.DisChargeAgingICtrl.Umax = 125.0f;
    CtrlPara.DisChargeAgingICtrl.Umin = 0.0f;
    // 相角默认偏移
    CtrlPara.CPhsOffset = 0.0f;
    CtrlPara.DPhsOffset = 0.02f;

    // 默认开启
    CtrlPara.BuckSet = 1;
}
/*************************************************
Function: RelayStartCheck
Input: 继电器上电检测
Output:
*************************************************/
#define RELAY_CHK_FINISH            (0)
#define RELAY_CHK_STEP1             (1)
#define RELAY_CHK_STEP2             (2)
#define RELAY_CHK_SET_CURR_FALG     (3)
#define RELAY_CHK_ONE_BOOST         (4)
#define RELAY_CHK_TWO_BOOST         (5)
#define RELAY_CHK_TO_FINISH         (6)

static Uint16 u16OneBoostIChkCnt = 0;
static Uint16 u16TwoBoostIChkCnt = 0;

LLC_STATE RelayStartCheck(LLC_STATE State)
{
    LLC_STATE StateReturn = State;

    switch(LLCSystemCtrl.RelayData.CheckStep)
    {
    case RELAY_CHK_FINISH:
    {
        // 初始化以及继续判断
        if((LLCSystemCtrl.RelayData.FinishFlag == 1)&&(LLCSystemCtrl.RelayData.Flag1 == 1))
        {
            // 软启条件判断:充电软启还是放电软启
            if ((ObjSample.VAirFront >= LLCSystemCtrl.Vref - 6.0f) && (LLCSystemCtrl.VoltFlag == 0))
            {
                CpuState.Mode = SRDAB_IDLE;
                // 预充超时保护
                ++LLCSystemCtrl.u32PreChargeTimeOutCnt;
                if (LLCSystemCtrl.u32PreChargeTimeOutCnt > 60000)
                {
                    LLCSystemCtrl.u32PreChargeTimeOutCnt = 0;
                    stSysProtect.u16TotalFaultSign = 1;
                    stSysProtect.unFaultStatusWord2.Bits.bit0 = 1;
                    Hw_GpioWritePin(41, 0);
                }
                else
                {
                    // 闭合预充电阻
                    Hw_GpioWritePin(41, 1);
                }


                // 高压侧升压达到-超时处理（故障）
                if(ObjSample.VBusRaw > ObjSample.VAirFront - 5.0f)
                {
                    // 闭合主继电器
                    Hw_GpioWritePin(23, 1);
                    LLCSystemCtrl.StandDly++;
                    if(LLCSystemCtrl.StandDly >= 200)
                    {
                        LLCSystemCtrl.StandDly = 0;
                        // 断开预充电阻
                        Hw_GpioWritePin(41, 0);
                        LLCSystemCtrl.u32PreChargeTimeOutCnt = 0;
                        // 允许开放充电驱动缓启动
                        LLCSystemCtrl.SoftDir = 1;

                        StateReturn = LLC_SOFT_WAIT_STATE;

                    }
                }
            }
            else
            {
                LLCSystemCtrl.VoltFlag = 1;
                Hw_GpioWritePin(41, 0);
                // 如果外部电压不足以使得系统进入充电，则进入放电软启动
                // 进入后超时处理（也可以作为继电器粘连故障）
                if((ObjSample.VBusRaw > 4.0f)&&(LLCSystemCtrl.ClearVoltFlag == 0))
                {
                    ++LLCSystemCtrl.u32PowerAbsorptionTimeOutCnt;
                    if (LLCSystemCtrl.u32PowerAbsorptionTimeOutCnt >= 60000)
                    {
                        LLCSystemCtrl.u32PowerAbsorptionTimeOutCnt = 0;
                        if(ObjSample.VBusRaw > 10.0f)
                        {
                            FreezePWMOutput();
                            ObjSystemCtrl.ParamFlag = 0;
                            stSysProtect.u16TotalFaultSign = 1;
                            stSysProtect.unFaultStatusWord2.Bits.bit1 = 1;
                        }
                        else
                        {
                            LLCSystemCtrl.ClearVoltFlag = 1;
                        }
                    }
                    else
                    {
                        // 通知CLA对内部进行放电
                        CpuState.Mode = SRDAB_CHG_SOFT_START;
                        Cla1ForceTask1();
                        ObjSystemCtrl.ParamFlag = 1;
                    }

                }
                else
                {
                    LLCSystemCtrl.ClearVoltFlag = 1;
                    if((fabsf(ObjSample.VBusRaw - ObjSample.VAirFront) < 3.0f)&&(ObjSample.VAirFront < 3.0f))
                    {
                        // 闭合主继电器
                        Hw_GpioWritePin(23, 1);
                        LLCSystemCtrl.StandDly++;
                        if(LLCSystemCtrl.StandDly >= 200)
                        {
                            LLCSystemCtrl.RelayFlag = 1;
                            FreezePWMOutput();
                            ObjSystemCtrl.ParamFlag = 0;
                            LLCSystemCtrl.StandDly = 0;
                            // 进行放电缓启动
                            LLCSystemCtrl.SoftDir = 2;
                            StateReturn = LLC_SOFT_WAIT_STATE;
                        }
                    }
                    else
                    {
                        if(ObjSample.VAirFront < 200.0f)
                        {
                            CtrlPara.ClaRefSoftFreq.SetpointValue = 1300.0f;
                        }
                        else
                        {
                            CtrlPara.ClaRefSoftFreq.SetpointValue = 1400.0f;
                        }
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        LLCSystemCtrl.StandDly = 0;
                        // 进行放电缓启动
                        LLCSystemCtrl.SoftDir = 2;
                        StateReturn = LLC_SOFT_WAIT_STATE;
                    }
                }
            }
        }
        else
        {
            LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_STEP1;
        }
        break;
    }
    case RELAY_CHK_STEP1:
    {
        // 断开继电器，先检测内外部电压，确定判断逻辑
        Hw_GpioWritePin(23, 0);
        // 逻辑开始前先判断压差是否相等
        if(fabsf(ObjSample.VBus - ObjSample.VAirFront) < 10.0f)
        {
            // 需要主动扰压进行判断
            if(ObjSample.VBus < 200.0f)
            {
                LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_STEP2;
            }
            else
            {
                // 高压认为直接粘连，报故障继电器粘连
                stSysProtect.unFaultStatusWord2.Bits.bit5 = 1;
                stSysProtect.u16TotalFaultSign = 1;
                stSysProtect.u16SoftFPTakesEffect = 1;
            }
        }
        // 不相等则认为无粘连
        else
        {
            // 继电器检测没问题
            LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_SET_CURR_FALG;
        }

        break;
    }
    case RELAY_CHK_STEP2:
    {
        if(ObjSample.VBus > 200.0f)
        {
            // 停止升压，关波
            FreezePWMOutput();
            ObjSystemCtrl.ParamFlag = 0;
            if(fabsf(ObjSample.VBus - ObjSample.VAirFront) < 10.0f)
            {
                // 报故障继电器粘连故障
                stSysProtect.unFaultStatusWord2.Bits.bit5 = 1;
                stSysProtect.u16TotalFaultSign = 1;
                stSysProtect.u16SoftFPTakesEffect = 1;
            }
            else
            {
                // 继电器检测没问题
                LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_SET_CURR_FALG;
                CpuState.Mode = SRDAB_IDLE;
            }
        }
        else
        {
            // 通知CLA进入扰压模式
            CpuState.Mode = SRDAB_RELAY_CHK;
            Cla1ForceTask1();
            ObjSystemCtrl.ParamFlag = 1;
        }

        LLCSystemCtrl.RelayData.CheckCnt++;
        if(LLCSystemCtrl.RelayData.CheckCnt >= 40000)
        {
            LLCSystemCtrl.RelayData.CheckCnt = 0;
            // 超时报故障继电器粘连故障
            stSysProtect.unFaultStatusWord2.Bits.bit5 = 1;
            stSysProtect.u16TotalFaultSign = 1;
            stSysProtect.u16SoftFPTakesEffect = 1;
        }
        break;
    }
    case RELAY_CHK_SET_CURR_FALG:
    {
        // 继电器检测标志置位
        LLCSystemCtrl.RelayData.FinishFlag = 1;
        // 继电器检测结束，开始检测谐振电流采样
        LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_ONE_BOOST;
        u16OneBoostIChkCnt = 0;
        u16TwoBoostIChkCnt = 0;
        break;
    }
    case RELAY_CHK_ONE_BOOST:
    {
        //检测当前内部电压
        if(ObjSample.VBus < 320.0f)
        {
            // 开始单路升压
            CpuState.Mode = SRDAB_ONE_BOOST;
            Cla1ForceTask1();
            ObjSystemCtrl.ParamFlag = 1;
            // 第一路谐振电流检测
            LLCSystemCtrl.RelayData.CheckIbranch1Cnt++;
            if(LLCSystemCtrl.RelayData.CheckIbranch1Cnt > 20)
            {
                LLCSystemCtrl.RelayData.CheckIbranch1Cnt = 20;
                if(ObjPlatformCtrl.u16AdcARawResult[11] < 30) // 50
                {
                    if (++u16OneBoostIChkCnt >= 5)
                    {
                        u16OneBoostIChkCnt = 0;
                        stSysProtect.unFaultStatusWord2.Bits.bit11 = 1;
                        stSysProtect.u16TotalFaultSign = 1;
                        stSysProtect.u16SoftFPTakesEffect = 1;
                    }
                }
                else
                {
                    u16OneBoostIChkCnt = 0;
                }
            }
        }
        else
        {
            // 停止升压，关波
            FreezePWMOutput();
            ObjSystemCtrl.ParamFlag = 0;
            // 到达电压后进入单路降压
            LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_TWO_BOOST;
            CpuState.Mode = SRDAB_IDLE;
        }
        break;
    }
    case RELAY_CHK_TWO_BOOST:
    {
        if(ObjSample.VBus > 200.0f)
        {
            // 开始单路升压
            CpuState.Mode = SRDAB_TWO_BOOST;
            Cla1ForceTask1();
            ObjSystemCtrl.ParamFlag = 1;
            // 第二路谐振电流检测
            LLCSystemCtrl.RelayData.CheckIbranch2Cnt++;
            if(LLCSystemCtrl.RelayData.CheckIbranch2Cnt > 20)
            {
                LLCSystemCtrl.RelayData.CheckIbranch2Cnt = 20;
                if(ObjPlatformCtrl.u16AdcCRawResult[2] < 30)
                {
                    if (++u16TwoBoostIChkCnt >= 5)
                    {
                        u16TwoBoostIChkCnt = 0;
                        stSysProtect.unFaultStatusWord2.Bits.bit12 = 1;
                        stSysProtect.u16TotalFaultSign = 1;
                        stSysProtect.u16SoftFPTakesEffect = 1;
                    }
                }
                else
                {
                    u16TwoBoostIChkCnt = 0;
                }
            }
        }
        else
        {
            // 停止升压，关波
            FreezePWMOutput();
            ObjSystemCtrl.ParamFlag = 0;
            LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_TO_FINISH;
            CpuState.Mode = SRDAB_IDLE;
        }
        break;
    }
    case RELAY_CHK_TO_FINISH:
    {
        // 电流检测标志置位
        u16OneBoostIChkCnt = 0;
        u16TwoBoostIChkCnt = 0;
        LLCSystemCtrl.RelayData.Flag1 = 1;
        LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_FINISH;
        break;
    }
    default :
        u16OneBoostIChkCnt = 0;
        u16TwoBoostIChkCnt = 0;
        LLCSystemCtrl.RelayData.CheckStep = RELAY_CHK_FINISH;
        break;
    }
    return StateReturn;
}

/*************************************************
Function: CheckRef
Input: 母线稳压目标
Output:
*************************************************/
#define VREF_SET_MAX            (372.0f) // 372V
#define VREF_SET_MIN            (330.0f) // 330V

#pragma CODE_SECTION(CheckRef, ".ISR.ramfunc");
void CheckRef(void)
{
    float fTemp = 0.0f;

    // Get the VREF for the battery voltage
    if(ObjSample.VBat * 19 > VREF_SET_MAX)
    {
        LLCSystemCtrl.BaseVref = VREF_SET_MAX;
    }
    else
    {
        LLCSystemCtrl.BaseVref = ObjSample.VBat * 19;
    }

    if(LLCSystemCtrl.BaseVref > ObjSample.VBat * 15.5f)
    {
        LLCSystemCtrl.BaseVref = LLCSystemCtrl.BaseVref;
    }
    else
    {
        LLCSystemCtrl.BaseVref = ObjSample.VBat * 15.5f;
    }

    if (LLCSystemCtrl.BaseVref < VREF_SET_MIN)
    {
        LLCSystemCtrl.BaseVref = VREF_SET_MIN;
    }

    if (GetParallelMode(&stBatteryHandle))
    {
        VBusRegulation(&stBatteryHandle, &LLCSystemCtrl);
        // 跟随主机的Vref一定要高于自身的
        if(stBatteryHandle.fCtrlVoltage < LLCSystemCtrl.BaseVref)
        {
            fTemp = LLCSystemCtrl.BaseVref;
        }
        else
        {
            fTemp = stBatteryHandle.fCtrlVoltage;
        }
    }
    else
    {
        fTemp = LLCSystemCtrl.BaseVref;
    }

    if(ObjSystemCtrl.LLCState == LLC_CHARGE_STATE)
    {
        LLCSystemCtrl.PreVref = fTemp + 10.0f;
    }
    else if(ObjSystemCtrl.LLCState == LLC_DISCHARGE_STATE)
    {
        LLCSystemCtrl.PreVref = fTemp;
    }
    else
    {
        LLCSystemCtrl.PreVref = fTemp;
    }

    if(ObjSystemCtrl.ProjectState == IB800JP_STATE)
    {
        if(ObjSystemCtrl.LLCState == LLC_AGING_STATE)
        {
            LLCSystemCtrl.Vref = ObjSample.VBus - ObjSample.VBusFilter;
        }
        else
        {
            LLCSystemCtrl.Vref = LLCSystemCtrl.PreVref + ObjSample.VBusFilter;
        }
    }
    else
    {
        LLCSystemCtrl.Vref = LLCSystemCtrl.PreVref;
    }

}
/*************************************************
Function: ChargeModeCheck
Input: 实时监测充放电过程的变比，当超过SRDAB最大承受能力的变比时切换为LLC工作
Output:
*************************************************/
#pragma CODE_SECTION(ChargeModeCheck, ".ISR.ramfunc");
void ChargeModeCheck(void)
{

    LLCSystemCtrl.Ratio = (ObjSample.VBus - ObjSample.VBusFilter)/(ObjSample.VBat + 0.01f);
    if(LLCSystemCtrl.Ratio > 19.1f)
    {
        // 设置为降压充电
        CtrlPara.BuckSet = 1;
    }
    else if(LLCSystemCtrl.Ratio < 19.0f)
    {
        if((ObjSample.AvgPower < -1000)||(stBatteryHandle.i16CtrlPackPower < -1000))
        {
            CtrlPara.BuckSet = 0;
        }
        else if((ObjSample.AvgPower > -200)||(ObjPlatformCtrl.u16AllowChargePowerMax < 2000.0f))
        {
            // 设置为降压充电
            CtrlPara.BuckSet = 1;
        }
    }

    if ((ObjSystemCtrl.LLCState == LLC_DISCHARGE_STATE)&&(ObjSystemCtrl.ProjectState == IB800JP_STATE))
    {
        if (LLCSystemCtrl.Ratio < 13.5f)
        {
            LLCSystemCtrl.HighCount = 0;
            CtrlPara.BoostSet = 1;
        }
        else if ((LLCSystemCtrl.Ratio < 14.2f)&&(ObjSample.IBusRaw > LLCSystemCtrl.CurrTrigSet))
        {
            LLCSystemCtrl.HighCount = 0;
            CtrlPara.BoostSet = 1;
        }
        else if (LLCSystemCtrl.Ratio > 14.4f)
        {
            LLCSystemCtrl.HighCount++;
            if(LLCSystemCtrl.HighCount > 20)
            {
                CtrlPara.BoostSet = 0;
                LLCSystemCtrl.HighCount = 0;
            }
        }
        else
        {
            LLCSystemCtrl.HighCount = 0;
        }
    }
    else
    {
        LLCSystemCtrl.HighCount = 0;
        CtrlPara.BoostSet = 0;
    }

}
/*************************************************
Function: GetSetCurr
Input: 降压充电模式电流校准拟合-具体规则见IB800文件电流校准文档
Output:y=0.005763x^3−0.067612x^2+1.116233x+0.337859
*************************************************/
#pragma CODE_SECTION(GetSetCurr, ".ISR.ramfunc");
float GetSetCurr(float Vbat)
{
    float y = 0.0f;

    if((Vbat < 28.9f)&&(Vbat > 19.9f))
    {
        y = LINE_K2 * Vbat + LINE_B2;
        UPDNLMT(y,1.629f,1.132f);
    }
    else
    {
        y = 0;
    }

    return y;
}
/*************************************************
Function: InitLoopData
Input: 初始化环路参数
Output:
*************************************************/
void InitLoopData(void)
{

}

void AgingCurrLimit(SYS_PLATFORM_CTRL *pObjPlatformCtrl, LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl)
{
    pstLLCSystemCtrl->AgingRamp.RampLowLimit = -4200.0f/420.0f;
    pstLLCSystemCtrl->AgingRamp.RampHighLimit = 6050.0f/420.0f;
}


#pragma CODE_SECTION(CalcAllowCurrent, ".ISR.ramfunc");
void CalcAllowCurrent(LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl, CTRLPARA *pstCtrlPara, ST_SAMPLE *ObjSample)
{
    float fTemp1 = 0.0f;
    float fTemp2 = 0.0f;

    fTemp1 = (float)ObjPlatformCtrl.u16AllowChargePowerMax;
    fTemp2 = LLCSystemCtrl.Ratio;

    pstCtrlPara->VoltPICtrl.Umax = (float)fTemp1 / ObjSample->VBusFilter1;
    UPLMT(pstCtrlPara->VoltPICtrl.Umax, 4200.f/ObjSample->VBusFilter1);

    pstCtrlPara->CurrPICtrl.Umin = (fTemp2 - 15.5f) * -30.0f - 60.0f;
    UPDNLMT(pstCtrlPara->CurrPICtrl.Umin, -20.0f, -100.0f);

    pstCtrlPara->PowerCurrPICtrl.Umin = (fTemp2 - 15.5f) * -30.0f - 60.0f;
    UPDNLMT(pstCtrlPara->PowerCurrPICtrl.Umin, -20.0f, -100.0f);

    pstCtrlPara->VoltLLCPICtrl.Umax = (float)fTemp1 / ObjSample->VBusFilter1;
    UPLMT(pstCtrlPara->VoltLLCPICtrl.Umax, 4200.f/ObjSample->VBusFilter1);
}

/*************************************************
Function: ResetChargeData
Input: 重置充电参数
Output:
*************************************************/
#pragma CODE_SECTION(ResetChargeData, ".ISR.ramfunc");
void ResetChargeData(void)
{
    // 重置积分量
    CtrlPara.VoltPICtrl.i1 = 1.0f;      // 初始1A历史量
    CtrlPara.CurrPICtrl.i1 = 0;
    CtrlPara.PowerCurrPICtrl.i1 = 0;

    CtrlPara.fClaFreq = 200.0f;
    CtrlPara.ClaRefFreq.TargetValue = 200.0f;
    CtrlPara.ClaRefFreq.SetpointValue = 200.0f;

    CtrlPara.fs0 = 128.0f;
    CtrlPara.fs1 = 128.0f;
    CtrlPara.ClaRefFs0.TargetValue = 128.0f;
    CtrlPara.ClaRefFs0.SetpointValue = 128.0f;
    CtrlPara.ClaRefFs0.StepValue = 15.0f;
    CtrlPara.ClaZeroSoftFreq.SetpointValue = 1400;
    CtrlPara.LoopStart = 0;
    CtrlPara.fslate = 0;
    CtrlPara.LoopCnt = 0;
    CtrlPara.DisCurrPICtrl.Out = 0;
    CtrlPara.PhsOffset = CtrlPara.CPhsOffset;

    CtrlPara.ChargeStep.Last = 0;
    CtrlPara.ChargeStep.Current = 0;
    CtrlPara.ChargeStep.Delta = 0;
    CtrlPara.ChargeStep.Target = 0;
    CtrlPara.ChargeStep.Sign = 0;
}
/*************************************************
Function: RestDisChargeData
Input: 重置放电参数
Output:
*************************************************/
#pragma CODE_SECTION(RestDisChargeData, ".ISR.ramfunc");
void RestDisChargeData(void)
{
    CtrlPara.DisVoltPICtrl.i1 = 0;
    CtrlPara.DisVoltPICtrl.ui = 0;
    CtrlPara.DisVoltPICtrl.Out = 0;
    CtrlPara.DisCurrPICtrl.i1 = 0;
    CtrlPara.DisCurrPICtrl.ui = 0;
    CtrlPara.DisCurrPICtrl.Out = 0;
    CtrlPara.PowerDisCurrPICtrl.i1 = 0;
    CtrlPara.PowerDisCurrPICtrl.ui = 0;
    CtrlPara.PowerDisCurrPICtrl.Out = 0;

    CtrlPara.fClaFreq = 200.0f;//200

    CtrlPara.fs0 = 128.0f;
    CtrlPara.fs1 = 128.0f;
    CtrlPara.ClaRefFreq.TargetValue = 200.0f;//200
    CtrlPara.ClaRefFreq.SetpointValue = 200.0f;//200
    CtrlPara.ClaRefFs0.TargetValue = 128.0f;
    CtrlPara.ClaRefFs0.SetpointValue = 128.0f;
    CtrlPara.ClaRefFs0.StepValue = 15.0f;
    CtrlPara.ClaRefSoftFreq.SetpointValue = 1400;       // 300hz软启
    CtrlPara.LoopStart = 0;
    CtrlPara.fslate = 0;
    CtrlPara.LoopCnt = 0;
    CtrlPara.DisCurrPICtrl.Out = 0;
    CtrlPara.PhsOffset = CtrlPara.DPhsOffset;

    CtrlPara.DisChargeStep.Last = 0;
    CtrlPara.DisChargeStep.Current = 0;
    CtrlPara.DisChargeStep.Delta = 0;
    CtrlPara.DisChargeStep.Target = 0;
    CtrlPara.DisChargeStep.Sign = 0;
}

void RestDisChargeAgingData(void)
{
    CtrlPara.DisChargeAgingICtrl.i1 = 0;
    CtrlPara.DisChargeAgingICtrl.ui = 0;
    CtrlPara.DisChargeAgingICtrl.Out = 0;
}
void RestChargeAgingData(void)
{
    CtrlPara.ChargeAgingICtrl.i1 = 0;
    CtrlPara.ChargeAgingICtrl.ui = 0;
    CtrlPara.ChargeAgingICtrl.Out = 0;
}
/*************************************************
Function: RestPowerData
Input: 重置放电参数
Output:
*************************************************/
void RestPowerData(void)
{
    CtrlPara.CurrLLCPICtrl.i1 = 0;
    CtrlPara.VoltLLCPICtrl.i1 = 0;
    CtrlPara.PowerCurrLLCPICtrl.i1 = 0;
}

void ResetStateFiveData(void)
{
    LLCSystemCtrl.CanTStable = 0;
    LLCSystemCtrl.StateDir = 0;
}

#pragma CODE_SECTION(SetPwmParam, ".ISR.ramfunc");
void SetPwmParam(ST_CLA_OUTPUT *Param)
{
    // 这里需要设置PWM参数
    Hw_SetPWM1_PRD(Param->Prd);
    Hw_SetPWM2_PRD(Param->Prd);
    Hw_SetPWM4_PRD(Param->Prd);
    Hw_SetPWM5_PRD(Param->Prd);
    Hw_SetPWM7_PRD(Param->Prd);

    Hw_SetPWM1_CMPA(Param->BranchCMPA);
    Hw_SetPWM4_CMPA(Param->BranchCMPA);

    Hw_SetPWM2_CMPA(Param->PrimaryCMPA);
    Hw_SetPWM5_CMPA(Param->PrimaryCMPA);

    Hw_SetPWM1_CMPB(Param->BranchCMPB);
    Hw_SetPWM4_CMPB(Param->BranchCMPB);

    Hw_SetPWM2_CMPB(Param->PrimaryCMPB);
    Hw_SetPWM5_CMPB(Param->PrimaryCMPB);


    Hw_SetPWM1_CMPC(Param->BranchCMPC);

    Hw_SetPWM2_CMPC(Param->PrimaryCMPC);

    Hw_SetPWM1_CMPD(Param->BranchCMPD);

    Hw_SetPWM1_DealTime(Param->BranchDeadRed,Param->BranchDeadFed);
    Hw_SetPWM2_DealTime(Param->PrimaryDeadRed,Param->PrimaryDeadFed);

    Hw_SetPWM4_DealTime(Param->BranchDeadRed,Param->BranchDeadFed);
    Hw_SetPWM5_DealTime(Param->PrimaryDeadRed,Param->PrimaryDeadFed);
}

void ResetPwmCfg(void)
{

    // 这里需要设置PWM参数
    Hw_SetPWM1_PRD(600);
    Hw_SetPWM2_PRD(600);
    Hw_SetPWM4_PRD(600);
    Hw_SetPWM5_PRD(600);
    Hw_SetPWM7_PRD(600);

    Hw_SetPWM1_CMPA(0);
    Hw_SetPWM4_CMPA(0);

    Hw_SetPWM2_CMPA(0);
    Hw_SetPWM5_CMPA(0);

    Hw_SetPWM1_CMPB(0);
    Hw_SetPWM4_CMPB(0);

    Hw_SetPWM2_CMPB(0);
    Hw_SetPWM5_CMPB(0);

    Hw_SetPWM1_CMPC(0);

    Hw_SetPWM2_CMPC(0);
}


void ResetPwmParam(void)
{
    Hw_SetPWM1_PRD(1644);
    Hw_SetPWM2_PRD(1644);
    Hw_SetPWM4_PRD(1644);
    Hw_SetPWM5_PRD(1644);
    Hw_SetPWM7_PRD(1644);

    Hw_SetPWM1_CMPA(11);
    Hw_SetPWM4_CMPA(11);
    Hw_SetPWM2_CMPA(0);
    Hw_SetPWM5_CMPA(0);

    Hw_SetPWM1_CMPB(833);
    Hw_SetPWM4_CMPB(833);
    Hw_SetPWM2_CMPB(1644);
    Hw_SetPWM5_CMPB(1644);


//    Hw_SetPWM1_CMPC(162);
//    Hw_SetPWM2_CMPC(162);
//    Hw_SetPWM3_CMPC(162);

    Hw_SetPWM1_CMPC(0);
    Hw_SetPWM2_CMPC(0);
}

void ResetChargePwmParam(void)
{
    Hw_SetPWM1_PRD(648);
    Hw_SetPWM2_PRD(648);
    Hw_SetPWM4_PRD(648);
    Hw_SetPWM5_PRD(648);
    Hw_SetPWM7_PRD(648);

    Hw_SetPWM1_CMPA(56);
    Hw_SetPWM4_CMPA(56);
    Hw_SetPWM2_CMPA(11);
    Hw_SetPWM5_CMPA(11);

    Hw_SetPWM1_CMPB(379);
    Hw_SetPWM4_CMPB(379);
    Hw_SetPWM2_CMPB(334);
    Hw_SetPWM5_CMPB(334);

//    Hw_SetPWM1_CMPC(162);
//    Hw_SetPWM2_CMPC(162);
//    Hw_SetPWM3_CMPC(155);
//
    Hw_SetPWM1_CMPC(0);
    Hw_SetPWM2_CMPC(0);
}


void ResetDisChargePwmParam(void)
{
    Hw_SetPWM1_PRD(648);
    Hw_SetPWM2_PRD(648);
    Hw_SetPWM4_PRD(648);
    Hw_SetPWM5_PRD(648);
    Hw_SetPWM7_PRD(648);

    Hw_SetPWM1_CMPA(11);
    Hw_SetPWM4_CMPA(11);
    Hw_SetPWM2_CMPA(56);
    Hw_SetPWM5_CMPA(56);

    Hw_SetPWM1_CMPB(334);
    Hw_SetPWM4_CMPB(334);
    Hw_SetPWM2_CMPB(379);
    Hw_SetPWM5_CMPB(379);


//    Hw_SetPWM1_CMPC(162);
//    Hw_SetPWM2_CMPC(162);
//    Hw_SetPWM3_CMPC(162);

    Hw_SetPWM1_CMPC(0);
    Hw_SetPWM2_CMPC(0);
}
/*************************************************
Function: CheckIoscBurst
Input: 启动从机电流检测
Output:
*************************************************/
Uint16 CheckIoscBurst(void)
{
    return 1;
}
/*************************************************
Function: MasterSlaveCheckModule
Input: 主从控制模块
Output:
*************************************************/
void MasterSlaveCheckModule(ST_BATTERY_HANDLE *pstBatteryHandle)
{

}
/*************************************************
Function: LoopCtrl
Input: 环路控制, Timer2 ISR : 20khz tick
Output:
*************************************************/
//#pragma CODE_SECTION(LoopCtrl, ".ISR.ramfunc");
void LoopCtrl(void)
{
    // 这里需要设置PWM参数
    memcpy(&stReadClaOutputParam, &stClaOutputParam, sizeof(ST_CLA_OUTPUT));

    // 这里还需要做锁控制，要等CLA计算完之后再设置
    if (ObjSystemCtrl.ParamFlag == 1)
    {
        SetPwmParam(&stReadClaOutputParam);

        if(ObjSystemCtrl.LLCState == LLC_SOFT_STATE)
        {
            if(CpuState.Mode == SRDAB_CHG_SOFT_START)
            {
                EnablePWMOutput2_5();
            }
            else if(CpuState.Mode == SRDAB_RELAY_CHK)
            {
                EnablePWMOutput1_4();
            }
            else if(CpuState.Mode == SRDAB_ONE_BOOST)
            {
                EnablePWMOutput1();
            }
            else if(CpuState.Mode == SRDAB_TWO_BOOST)
            {
                EnablePWMOutput5();
            }
        }
        else if (ObjSystemCtrl.LLCState == LLC_SOFT_WAIT_STATE)
        {
            EnablePWMOutput1_4();
        }
        else
        {
            if(CtrlPara.BoostSet == 1)
            {
                EnablePWMOutput1_4();
            }
            else
            {
                EnablePWMOutput();
            }
        }
        // 然后执行复位
        ObjSystemCtrl.ParamFlag = 0;
    }

    CalcAllowCurrent(&LLCSystemCtrl, &CtrlPara, &ObjSample);
    ChargeModeCheck();

    switch(ObjSystemCtrl.LLCState)
    {
        // 空闲态，等待LLC启动前置条件准备就绪(系统状态机准备就绪)
        case LLC_IDLE_STATE:
        {
            CpuState.Mode = SRDAB_IDLE;
            LLCSystemCtrl.StatePrompt = 0;
            ResetStateFiveData();
            // 在空闲态执行关波，同时会触发全局加载
            Hw_StopAllPWM();
            // 这里还需要触发全局加载
            Hw_TrigGlobalLoad();
            ObjSystemCtrl.ParamFlag = 0;

            // 复位参数
            CtrlPara.ClaTestDuty.SetpointValue = 580.0f;
            // 缓启动完成标志
            LLCSystemCtrl.SoftFinish = 0;
            CtrlPara.DisChargeStep.Last = 0;
            CtrlPara.ChargeStep.Last = 0;
            CtrlPara.fPhase = 0;
            ResetChargeData();
            RestDisChargeData();
            LLCSystemCtrl.StandDly = 0;
            CtrlPara.LoopStart = 0;
            CtrlPara.fslate = 0;
            CtrlPara.DisCurrPICtrl.Out = 0;
            LLCSystemCtrl.ClearVoltFlag = 0;
            LLCSystemCtrl.VoltFlag = 0;
            memset(&stReadClaOutputParam, 0, sizeof(ST_CLA_OUTPUT));
            memset(&stClaOutputParam, 0, sizeof(ST_CLA_OUTPUT));

            // 复位预充超时计数器
            LLCSystemCtrl.u32PreChargeTimeOutCnt = 0;
            // 复位电容抽电超时计数器
            LLCSystemCtrl.u32PowerAbsorptionTimeOutCnt = 0;
            ObjSystemCtrl.LLCState = LLC_SOFT_STATE;

            break;
        }

        case LLC_SOFT_STATE:
        {
            ObjSystemCtrl.LLCState = RelayStartCheck(LLC_SOFT_STATE);
            break;
        }

        case LLC_SOFT_WAIT_STATE:
        {
            // 通知CLA进入软启模式
            CpuState.Mode = SRDAB_DISCHG_SOFT_START;
            if (LLCSystemCtrl.SoftDir == 1)
            {
                // 切换到充电配置切换
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
                // 缓启动完成标志
                LLCSystemCtrl.SoftFinish = 1;
            }
            else if (LLCSystemCtrl.SoftDir == 2)
            {
                if(LLCSystemCtrl.RelayFlag == 0)
                {
                    // 两边压差一样时吸合
                    if(fabsf(ObjSample.VBusRaw - ObjSample.VAirFront) < 5.0f)
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        LLCSystemCtrl.SoftBoostFlag = 1;
                        // 闭合主继电器
                        Hw_GpioWritePin(23, 1);
                        LLCSystemCtrl.StandDly++;
                        if(LLCSystemCtrl.StandDly >= 200)
                        {
                            LLCSystemCtrl.StandDly = 0;
                            LLCSystemCtrl.RelayFlag = 1;
                            LLCSystemCtrl.SoftBoostFlag = 0;
                        }
                    }
                }

                // 这里需要判断是否软启结束，如果软启结束，需要切换成放电闭环
                // 电池电压<21V VBUS电压达不到设定值，修改判断条件
                if((ObjSample.VBus > (LLCSystemCtrl.Vref - 3.0f)) || \
                   ((ObjSample.VBat < 21.0f) && (ObjSample.VBus > ObjSample.VBat * 20.0f)))
                {
                    // 软启动后进入放电闭环
                    // 切换到充电配置切换
                    FreezePWMOutput();
                    ObjSystemCtrl.ParamFlag = 0;
                    if(LLCSystemCtrl.RelayFlag == 1)
                    {
                        ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                        // 缓启动完成标志
                        LLCSystemCtrl.SoftFinish = 1;
                    }
                }
                else
                {
                    if(LLCSystemCtrl.SoftBoostFlag == 0)
                    {
                        Cla1ForceTask1();
                        ObjSystemCtrl.ParamFlag = 1;
                    }
                }
            }
            break;
        }

        case LLC_CHARGE_STATE:
        {
            LLCSystemCtrl.StatePrompt = 1;
            // 关闭驱动的过渡态
            if(LLCSystemCtrl.ChargeFirst == 0)
            {
                // 状态切换时，需要改变工作模式，以及参数复位
                // 在切换进该状态前需要先关波，避免波形未关闭完成，然后修改了发波参数
                ResetChargePwmParam();
                WorkModificate();   // 切换至正常工作模式
                ResetChargeData();  // 参数重置
                RestPowerData();
                Hw_TrigGlobalLoad();
                // 这里作打嗝条件判断
                if (ObjSystemCtrl.BurstFlag)
                {
                    // 电压回升退出打嗝
                    if((ObjSample.VBus > LLCSystemCtrl.Vref)||((CpuAdcSample.ParalelCtrlState == 2)&&(LLCSystemCtrl.fPowerRef < -200.0f)))
                    {
                        // 进入发波
                        LLCSystemCtrl.ChargeFirst = 1;
                        // 正常充电工作模式
                        LLCSystemCtrl.ChargeState = 1;
                        ObjSystemCtrl.BurstFlag = 0;
                    }
                    // 电压过低退出打嗝并跳转为放电配置
                    else if(ObjSample.VBus < LLCSystemCtrl.Vref - LLCSystemCtrl.C2DVoltGap)//13.0f
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        ObjSystemCtrl.BurstFlag = 0;
                        ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                    }
                    Cla1ForceTask1();
                }
                else
                {
                    // 进入发波
                    LLCSystemCtrl.ChargeFirst = 1;
                    // 充电工作模式区分
                    if(CtrlPara.BuckSet == 1)
                    {
                        // 特殊充电工作模式
                        LLCSystemCtrl.ChargeState = 2;
                    }
                    else
                    {
                        // 正常充电工作模式
                        LLCSystemCtrl.ChargeState = 1;
                    }
                }
            }
            else
            {
                // 正常充电工作模式
                if(LLCSystemCtrl.ChargeState == 1)
                {
                    CpuState.Mode = SRDAB_CHARGE;
                    Cla1ForceTask1();
                    ObjSystemCtrl.ParamFlag = 1;

                    // LLCSystemCtrl.IStagnation = IoscCheck();
                    if(CtrlPara.BuckSet == 1)
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        // 回退关波过渡判断
                        LLCSystemCtrl.ChargeFirst = 0;
                    }
                }
                // 特殊充电工作模式
                else if(LLCSystemCtrl.ChargeState == 2)
                {
                    CpuState.Mode = SRDAB_ADJUST_DUTY;
                    Cla1ForceTask1();
                    ObjSystemCtrl.ParamFlag = 1;

                    if(CtrlPara.BuckSet == 0)
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        // 回退关波过渡判断
                        LLCSystemCtrl.ChargeFirst = 0;
                    }
                }
                // 回退关波过渡判断
                if ((ObjSample.VBus < LLCSystemCtrl.Vref - LLCSystemCtrl.CBackVoltGap)||((CpuAdcSample.ParalelCtrlState == 2)&&(LLCSystemCtrl.fPowerRef > -50.0f)))
                {
                    FreezePWMOutput();
                    ObjSystemCtrl.ParamFlag = 0;
                    LLCSystemCtrl.ChargeFirst = 0;
                    ObjSystemCtrl.BurstFlag = 1;
                }
            }

            if (LLCSystemCtrl.u16AllowCharge == 0)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_STANDBY_STATE;
            }

            if(LLCSystemCtrl.SlaveStateChange == 4)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
            }

            if(LLCSystemCtrl.AgingFlag == 1)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_AGING_STATE;
            }
            break;
        }

        case LLC_DISCHARGE_STATE:
        {
            LLCSystemCtrl.StatePrompt = 2;
            // 关闭驱动的过渡态
            if(LLCSystemCtrl.DisChargeFirst == 0)
            {
                if(CtrlPara.BoostSet == 0)
                {
                    ResetDisChargePwmParam();   // 先主动切换成将要发波的参数
                }
                else
                {
                    ResetPwmParam();   // 先主动切换成将要发波的参数
                }
                WorkModificate();   // 切换至正常工作模式
                RestDisChargeData();
                Hw_TrigGlobalLoad();

                // 这里作打嗝条件判断
                if(ObjSystemCtrl.BurstFlag == 1)
                {
                    // 电压回落退出打嗝
                    if((ObjSample.VBus < LLCSystemCtrl.Vref - 1.0f)||((CpuAdcSample.ParalelCtrlState == 2)&&(LLCSystemCtrl.fPowerRef > 500.0f)))
                    {
                        LLCSystemCtrl.DisChargeFirst = 1;
                        // 正常放电工作模式
                        LLCSystemCtrl.DisChargeState = 1;
                        ObjSystemCtrl.BurstFlag = 0;
                    }
                    // 电压过高退出打嗝并跳转到充电配置
                    else if(ObjSample.VBus > LLCSystemCtrl.Vref + LLCSystemCtrl.D2CVoltGap)//15.0f
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        ObjSystemCtrl.BurstFlag = 0;
                        ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
                    }
                    Cla1ForceTask1();
                }
                else
                {
                    // 进入发波
                    LLCSystemCtrl.DisChargeFirst = 1;
                    // 放电工作模式区分
                    if(CtrlPara.BoostSet == 1)
                    {
                        // 特殊放电工作模式
                        LLCSystemCtrl.DisChargeState = 2;
                    }
                    else
                    {
                        // 正常放电工作模式
                        LLCSystemCtrl.DisChargeState = 1;
                    }
                }
            }
            else
            {
                // 正常放电模式
                if(LLCSystemCtrl.DisChargeState == 1)
                {
                    CpuState.Mode = SRDAB_DIS_CHARGE;
                    Cla1ForceTask1();
                    ObjSystemCtrl.ParamFlag = 1;

                    if(CtrlPara.BoostSet == 1)
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        // 回退关波过渡判断
                        LLCSystemCtrl.DisChargeFirst = 0;
                    }
                }
                // 特殊放电模式
                else if(LLCSystemCtrl.DisChargeState == 2)
                {
                    CpuState.Mode = SRDAB_LLC_MODE;
                    Cla1ForceTask1();
                    ObjSystemCtrl.ParamFlag = 1;

                    if(CtrlPara.BoostSet == 0)
                    {
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                        // 回退关波过渡判断
                        LLCSystemCtrl.DisChargeFirst = 0;
                    }
                }

                if ((ObjSample.VBus > LLCSystemCtrl.Vref + LLCSystemCtrl.DBackVoltGap)||((CpuAdcSample.ParalelCtrlState == 2)&&(LLCSystemCtrl.fPowerRef < 50.0f)))
                {
                    ObjSystemCtrl.ParamFlag = 0;
                    ObjSystemCtrl.BurstFlag = 1;
                    // 切换状态需要关波
                    FreezePWMOutput();
                    LLCSystemCtrl.DisChargeFirst = 0;
                }
            }

            if ((LLCSystemCtrl.u16AllowDischarge == 0)&&(CpuAdcSample.ParalelCtrlState == 2))
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_STANDBY_STATE;
            }

            if(LLCSystemCtrl.SlaveStateChange == 3)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
            }

            if(LLCSystemCtrl.AgingFlag == 1)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_AGING_STATE;
            }
            break;
        }

        case LLC_STANDBY_STATE:
        {
            CpuState.Mode = SRDAB_IDLE;
            LLCSystemCtrl.ChargeFirst = 0;
            LLCSystemCtrl.DisChargeFirst = 0;
            // 从充电状态而来,即原处于充电工作但因禁止充电而进入待机
            if(LLCSystemCtrl.StatePrompt == 1)
            {
                LLCSystemCtrl.StatePrompt = 0;
                LLCSystemCtrl.StateDir = 1;
            }
            // 从放电状态而来，即原处于放电工作但因禁止放电而进入待机
            else if(LLCSystemCtrl.StatePrompt == 2)
            {
                LLCSystemCtrl.StatePrompt = 0;
                LLCSystemCtrl.StateDir = 2;
            }

            if(GetParallelMode(&stBatteryHandle))
            {
                if(CpuAdcSample.ParalelCtrlState == 2)
                {
                    if(LLCSystemCtrl.StateDir == 1)
                    {
                        if(LLCSystemCtrl.u16AllowCharge == 1)
                        {
                            ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
                            ResetStateFiveData();
                        }
                        if ((ObjSample.VBus < (LLCSystemCtrl.Vref - LLCSystemCtrl.C2DVoltGap))||(LLCSystemCtrl.fPowerRef > 500.0f))
                        {
                            ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                            ResetStateFiveData();
                        }
                    }
                    else if(LLCSystemCtrl.StateDir == 2)
                    {
                        if(LLCSystemCtrl.u16AllowDischarge == 1)
                        {
                            ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                            ResetStateFiveData();
                        }
                        if ((ObjSample.VBus > (LLCSystemCtrl.Vref + LLCSystemCtrl.D2CVoltGap))||(LLCSystemCtrl.fPowerRef < -500.0f))
                        {
                            ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
                            ResetStateFiveData();
                        }
                    }
                }
                else if(CpuAdcSample.ParalelCtrlState == 1)
                {
                    // 此时作为稳压包的机器需要更换，告知总线主机该设备当前无法承担稳压责任
                    LLCSystemCtrl.CanTStable = 1;
                    if (ObjSample.VBus < (LLCSystemCtrl.Vref - LLCSystemCtrl.C2DVoltGap))
                    {
                        ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                        ResetStateFiveData();
                    }
                }
            }
            else
            {
                if(LLCSystemCtrl.u16AllowCharge == 1)
                {
                    ObjSystemCtrl.LLCState = LLC_CHARGE_STATE;
                    ResetStateFiveData();
                }
                // 单包进入该状态只能是因禁止充电
                if (ObjSample.VBus < (LLCSystemCtrl.Vref - LLCSystemCtrl.C2DVoltGap))
                {
                    ObjSystemCtrl.LLCState = LLC_DISCHARGE_STATE;
                    ResetStateFiveData();
                }
            }

            if(LLCSystemCtrl.AgingFlag == 1)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                LLCSystemCtrl.ChargeFirst = 0;
                LLCSystemCtrl.DisChargeFirst = 0;
                ObjSystemCtrl.LLCState = LLC_AGING_STATE;
            }
            break;
        }

        case LLC_AGING_STATE:
        {
            CpuState.Mode = AGING_MODE;

            if(ObjSystemCtrl.unStateSign == 0)
            {
                // 静置
                LLCSystemCtrl.AgingIDirCom = 0;
            }
            else if(ObjSystemCtrl.unStateSign == 1)
            {
                // 恒流充电
                if(LLCSystemCtrl.u16AllowCharge == 0)
                {
                    LLCSystemCtrl.AgingIDirCom = 0;
                }
                else
                {
                    LLCSystemCtrl.AgingIDirCom = -1;
                }
            }
            else if(ObjSystemCtrl.unStateSign == 2)
            {
                // 恒流放电
                if(LLCSystemCtrl.u16AllowDischarge == 0)
                {
                    LLCSystemCtrl.AgingIDirCom = 0;
                }
                else
                {
                    LLCSystemCtrl.AgingIDirCom = 1;
                }
            }

            if ((ObjSample.VBus > 300.0f)&&(ObjSample.VBus < 480.0f))
            {
                LLCSystemCtrl.AgingCnt++;
                if(LLCSystemCtrl.AgingCnt > 20000)
                {
                    LLCSystemCtrl.AgingCnt = 20000;
                    AgingCurrLimit(&ObjPlatformCtrl, &LLCSystemCtrl);

                    if(ObjSystemCtrl.u16RunSet == 1)
                    {
                        LLCSystemCtrl.AgingISetCom = (((float)ObjSystemCtrl.u16MaintainCurrent) * LLCSystemCtrl.AgingIDirCom) * 0.1;
                    }
                    else
                    {
                        LLCSystemCtrl.AgingISetCom = 0;
                    }
                    LLCSystemCtrl.AgingRamp.TargetValue = LLCSystemCtrl.AgingISetCom;
                    LLCSystemCtrl.AgingISet = RmpCntlCal(&LLCSystemCtrl.AgingRamp);

                    if(LLCSystemCtrl.AgingISetLast * LLCSystemCtrl.AgingISet <= 0)
                    {
                        LLCSystemCtrl.TrigAging = 1;
                    }
                    else
                    {
                        LLCSystemCtrl.TrigAging = 0;
                    }

                    if((LLCSystemCtrl.AgingISet > 0.1f)&&(LLCSystemCtrl.TrigAging == 0))
                    {
                        CpuAdcSample.AgingIDir = 1;
                        Cla1ForceTask1();
                        ObjSystemCtrl.ParamFlag = 1;
                    }
                    else if((LLCSystemCtrl.AgingISet < 0.1f)&&(LLCSystemCtrl.TrigAging == 0))
                    {
                        CpuAdcSample.AgingIDir = -1;
                        Cla1ForceTask1();
                        ObjSystemCtrl.ParamFlag = 1;
                    }
                    else
                    {
                        LLCSystemCtrl.TrigAging = 0;
                        RestDisChargeAgingData();
                        RestChargeAgingData();
                        CpuAdcSample.AgingIDir = 0;
                        FreezePWMOutput();
                        ObjSystemCtrl.ParamFlag = 0;
                    }

                    LLCSystemCtrl.AgingISetLast = LLCSystemCtrl.AgingISet;
                }
            }
            else
            {
                LLCSystemCtrl.AgingCnt = 0;
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                RestDisChargeAgingData();
                RestChargeAgingData();
                CpuAdcSample.AgingIDir = 0;
                LLCSystemCtrl.AgingRamp.SetpointValue = 0;
                LLCSystemCtrl.AgingISetLast = 0;
            }

            if(LLCSystemCtrl.AgingFlag == 0)
            {
                FreezePWMOutput();
                ObjSystemCtrl.ParamFlag = 0;
                ObjSystemCtrl.LLCState = LLC_IDLE_STATE;
            }



            break;
        }
        default:
            break;
    }

}
/*************************************************
Function: FreezePWMOutput
Input: 冻结PWM输出
Output:
*************************************************/
void FreezePWMOutput(void)
{
    Hw_StopAllPWM();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();

}
/*************************************************
Function: EnablePWMOutput
Input: 使能PWM输出
Output:
*************************************************/
void EnablePWMOutput(void)
{
    Hw_StartAllPWM();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}

void EnablePWMOutput1_4(void)
{
    Hw_StartPWM1();
    Hw_StartPWM4();
    Hw_StopPWM2();
    Hw_StopPWM5();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}

void EnablePWMOutput2_5(void)
{

    Hw_StopPWM1();
    Hw_StopPWM4();
    Hw_StartPWM2();
//    Hw_StartPWM3();
    Hw_StartPWM5();
//    Hw_StartPWM6();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}

void EnablePWMOutput1(void)
{

    Hw_StartPWM1();
    Hw_StopPWM4();
    Hw_StopPWM2();
    Hw_StopPWM5();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}

void EnablePWMOutput5(void)
{

    Hw_StopPWM1();
    Hw_StopPWM4();
    Hw_StopPWM2();
    Hw_StartPWM5();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}
/*************************************************
Function: SoftStartModificate
Input: 软启动配置切换
Output:
*************************************************/
void SoftStartModificate(void)
{
    Hw_SetPWM1Mode_Soft();
    Hw_SetPWM2Mode_Soft();
    Hw_SetPWM4Mode_Soft();
    Hw_SetPWM5Mode_Soft();
}
/*************************************************
Function: WorkModificate
Input: 正常工作配置切换
Output:
*************************************************/
#pragma CODE_SECTION(WorkModificate, ".ISR.ramfunc");
void WorkModificate(void)
{
    Hw_SetPWM1Mode_Normal();
    Hw_SetPWM2Mode_Normal();
//    Hw_SetPWM3Mode_Normal();
    Hw_SetPWM4Mode_Normal();
    Hw_SetPWM5Mode_Normal();
//    Hw_SetPWM6Mode_Normal();

}

void EnablePWMOutput123(void)
{
    Hw_StartPWM1();
    Hw_StartPWM2();
//    Hw_StartPWM3();

    Hw_StopPWM4();
    Hw_StopPWM5();
//    Hw_StopPWM6();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}

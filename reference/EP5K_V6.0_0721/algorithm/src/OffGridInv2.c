/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: OffGridInv2.c
Author: tzz
Date:2022年3月2日
Description: 离网控制模块,采用单电压环控制
Version:0.0.2
Function List:
***********************************************************/
#include <string.h>
#include <bsp.h>
#include "Sample.h"
#include "ConfigSystemPara.h"
#include "CalculateAngle.h"
#include "OffGridInv2.h"
#include "InvStateMachine.h"
#include "Parallel.h"
#include "OnGridInv2.h"
#include "Function.h"
#include "Protect.h"

#define KPR  30

OFF_GRID_CONTROL_COEFF UrCtrl;
OFF_GRID_CONTROL_COEFF UsCtrl;

void LowpowerCntl(OFF_GRID_CONTROL_COEFF *v);
void PeakVoltCntl(OFF_GRID_CONTROL_COEFF *v);
static void ControlSingleOffGridInv11(OFF_GRID_CONTROL_COEFF *v);
/*************************************************
Function: InitOffGridInvControlData
Description: 初始化离网控制模块变量
Input: None
Output: None
Frequency: Once
*************************************************/
void InitOffGridInvControlData(void)
{
    memset(&UrCtrl, 0, sizeof(OFF_GRID_CONTROL_COEFF));
    memset(&UsCtrl, 0, sizeof(OFF_GRID_CONTROL_COEFF));

    // PWM 参数

    UrCtrl.Period = INV_EPWM_PERIOD;
//    UsCtrl.Period = INV_EPWM_PERIOD;
    // 目标电压
    UrCtrl.RefTarget = (SystemPara.InvRateVolt*1.414159f);
//    UsCtrl.RefTarget = UrCtrl.RefTarget;

    // 前馈系数
    UrCtrl.FwCoeff = 1.0f;
//    UsCtrl.FwCoeff = 1.0f;
    // 前馈缓起动
    UrCtrl.FwRamp.TargetValue = UrCtrl.RefTarget;
    UrCtrl.FwRamp.StepValue = 100.0f/(INV_PWM_FREQ_KHZ*1000.0f*0.5f);    //
    UrCtrl.FwRamp.RampHighLimit = SystemPara.InvRateVolt*1.414f;

    // 参考缓起
    UrCtrl.RefRamp.TargetValue = UrCtrl.RefTarget;
    UrCtrl.RefRamp.StepValue = UrCtrl.FwRamp.StepValue;
    UrCtrl.RefRamp.RampHighLimit = SystemPara.InvRateVolt*1.414f;

    // PI参数需要重新调
    UrCtrl.PICtrl.Kp = 0.005f;
    UrCtrl.PICtrl.Ki = 0.0001f;
    UrCtrl.PICtrl.Umax = 0.35f;     //
    UrCtrl.PICtrl.Umin = -0.35f;    //
//    UsCtrl.PICtrl = UrCtrl.PICtrl;

    UrCtrl.KpSet = UrCtrl.PICtrl.Kp;
//    UsCtrl.KpSet = UsCtrl.PICtrl.Kp;
    // 重复控制系数
//    UrCtrl.ReEnable = 1;
    UrCtrl.RepCtrl.kp = 0.00005f;
    UrCtrl.RepCtrl.ThetaShift = 4;
    UrCtrl.RepCtrl.OutMax = 0.2f;
    UrCtrl.RepCtrl.OutMin = -0.2f;
    // S 相
//    UsCtrl.ReEnable = 1;
//    UsCtrl.RepCtrl = UrCtrl.RepCtrl;

    // 可能是 50 或者 60
    UrCtrl.LastFreq = SystemPara.InvRateFreq;
    // 当频率发生大的变化时(离网不需要),该函数要重新被调用更新参数
    SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UrCtrl.Sogi);
//    SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UsCtrl.Sogi);

    // 最大输出占空比
    UrCtrl.OutMax = 0.99;
    UrCtrl.OutMin = -0.99;
//    UsCtrl.OutMax = 0.99;
//    UsCtrl.OutMin = -UsCtrl.OutMax;

//    UrCtrl.LowPoWCtrl.Kp =  0.0002f;
//    UrCtrl.LowPoWCtrl.Ki =  0.00002f;
//    UrCtrl.LowPoWCtrl.Umax = 0.55f;     //
//    UrCtrl.LowPoWCtrl.Umin = -0.55f;    //
//    UsCtrl.LowPoWCtrl = UrCtrl.LowPoWCtrl;

    UrCtrl.BusErrCtrl.Ki =  0.00002f;

    Init_PRCoeff(&UrCtrl.LowPoWCoeffs, 0, 30.0f ,__mpy2pif32(SystemPara.InvRateFreq)  ,(PWM_ISR_FREQUENCY),__mpy2pif32(0.2));   //
    Init_PRCoeff(&UrCtrl.PRCoeffs, 0, (30.0f*7) ,__mpy2pif32(SystemPara.InvRateFreq)  ,(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));   //
    Init_PRCoeff(&UrCtrl.R3Coeffs, 0, (15.0f*5) ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));
    Init_PRCoeff(&UrCtrl.R5Coeffs, 0, (10.0f*5) ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));
    Init_PRCoeff(&UrCtrl.R7Coeffs, 0, (6.0f*3)  ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));//0.07
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.LowPoWVars, 40 , -40);
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.PRVars, KPR , -KPR);
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R3Vars, KPR, -KPR); //@sjj20260202
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R5Vars, KPR, -KPR); //@sjj20260202
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R7Vars, KPR, -KPR); //@sjj20260202
//    UsCtrl.PRCoeffs = UrCtrl.PRCoeffs;
//    UsCtrl.R3Coeffs = UrCtrl.R3Coeffs;
//    UsCtrl.R5Coeffs = UrCtrl.R5Coeffs;
//    UsCtrl.R7Coeffs = UrCtrl.R7Coeffs;
//    UsCtrl.PRVars = UrCtrl.PRVars;
//    UsCtrl.R3Vars = UrCtrl.R3Vars;
//    UsCtrl.R5Vars = UrCtrl.R5Vars;
//    UsCtrl.R7Vars = UrCtrl.R7Vars;
//    UsCtrl.LowPoWCoeffs = UrCtrl.LowPoWCoeffs;
//    UsCtrl.LowPoWVars = UrCtrl.LowPoWVars;

    UsCtrl = UrCtrl;
    UrCtrl.Id = 1;
    UsCtrl.Id = 2;
}
/*************************************************
Function: ClearOffGridRepeatData
Description: 清除离网重复控制缓存
Input: None
Output: None
Frequency: Once
*************************************************/
void ClearOffGridRepeatData(uint16_t Cnt)
{
    if (Cnt >= 210)
    {
        Cnt = 210;
    }

    UrCtrl.RepCtrl.Data[Cnt] = 0;
    UrCtrl.RepCtrl.Data[Cnt+1] = 0;
    UsCtrl.RepCtrl.Data[Cnt] = 0;
    UsCtrl.RepCtrl.Data[Cnt+1] = 0;
}
/*************************************************
Function: SetOffGridStepValueHigh
Description: 设置离网缓起步长-快速
Input: None
Output: None
Frequency: Once
*************************************************/
void SetOffGridStepValueHigh(void)
{
    UrCtrl.FwRamp.StepValue = 33334.0f/(INV_PWM_FREQ_KHZ*1000.0f*0.5f); //100V/3ms

    UsCtrl.FwRamp.StepValue = UrCtrl.FwRamp.StepValue;
}
/*************************************************
Function: SetOffGridStepValueLow
Description: 设置离网缓起步长-慢速
Input: None
Output: None
Frequency: Once
*************************************************/
void SetOffGridStepValueLow(void)
{
    UrCtrl.FwRamp.StepValue = 200.0f/(INV_PWM_FREQ_KHZ*1000.0f*0.5f);   // 0.3V/ms

    UsCtrl.FwRamp.StepValue = UrCtrl.FwRamp.StepValue;
}
/*************************************************
Function: UpdateOffGridRef
Description: 更新目标电压
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(UpdateOffGridRef, ".TI.ramfunc");
void UpdateOffGridRef(float refSet)
{
    // 目标电压
    UrCtrl.RefTarget = refSet*1.414f;
    UsCtrl.RefTarget = refSet*1.414f;

    // 前馈缓起动
    UrCtrl.FwRamp.TargetValue = UrCtrl.RefTarget;
    UsCtrl.FwRamp.TargetValue = UrCtrl.FwRamp.TargetValue;
    UrCtrl.FwRamp.RampHighLimit = refSet*1.414f;
    UsCtrl.FwRamp.RampHighLimit = refSet*1.414f;

    // 参考缓起
    UrCtrl.RefRamp.TargetValue = UrCtrl.RefTarget;
    UrCtrl.RefRamp.RampHighLimit = refSet*1.414f;
    UsCtrl.RefRamp.TargetValue = UrCtrl.RefTarget;
    UsCtrl.RefRamp.RampHighLimit = refSet*1.414f;
}
/*************************************************
Function: ClearOffGridPidData
Description: 清除离网PI积分量和当前缓起给定初始值
Input: None
Output: None
Frequency: Once
*************************************************/
void ClearOffGridPidData(void)
{
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.PRVars, KPR , -KPR);
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R3Vars, KPR, -KPR); //@sjj20260202
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R5Vars, KPR, -KPR); //@sjj20260202
    Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R7Vars, KPR, -KPR); //@sjj20260202
    UsCtrl.PRVars = UrCtrl.PRVars;
    UsCtrl.R3Vars = UrCtrl.R3Vars;
    UsCtrl.R5Vars = UrCtrl.R5Vars;
    UsCtrl.R7Vars = UrCtrl.R7Vars;

    UpdateOffGridRef(SystemPara.InvRateVolt);
}
/*************************************************
Function: SetOffGridInitVolt
Description: 设置当前缓起给定初始值
Input: None
Output: None
Frequency: Once
*************************************************/
void SetOffGridInitVolt(float initVolt)
{
    float volt = 0;

    volt = __fsat(initVolt, 170.0f, 0);

//    // 前馈斜坡初值清零
    UrCtrl.FwRamp.SetpointValue = volt; // 0->150V
    UsCtrl.FwRamp.SetpointValue = volt; // 0->150V
    UrCtrl.RefRamp.SetpointValue = volt; // 0->150V
    UsCtrl.RefRamp.SetpointValue = volt; // 0->150V
}
/*************************************************
Function: OpenLoopOffGridInv11
Description: 开环输出-调制比0.75
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(OpenLoopOffGridInv11, ".TI.ramfunc");
static void OpenLoopOffGridInv11(OFF_GRID_CONTROL_COEFF *v)
{
    v->Fw = RmpCntlCalc(&v->FwRamp);
    // 调制比0.8
    v->Out = __divf32(v->Fw, v->FwRamp.TargetValue) * 0.75f;
    v->Out = __fsat(v->Out, 0.75f, 0.0f);
    v->Out = v->Out * v->Park.Sin;

    //-------------------------------双极性控制-----------------------------------
    v->CAMP = (v->Out * v->Period);

    UpdateINVPwmComp3(v->Id, v->CAMP, v->Period,1);
}
/*************************************************
Function: ControlSingleOffGridInv11
Description: 闭环控制,给定量做前馈+PI+重复控制
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(ControlSingleOffGridInv11, ".TI.ramfunc");
static void ControlSingleOffGridInv11(OFF_GRID_CONTROL_COEFF *v)
{
//    if(v->FwStartOver)
//    {
//        v->ReEnable = 1;
      v->PICtrl.Ref = RmpCntlCalc(&v->RefRamp);
//    }
//    else
//    {
//        v->ReEnable = 0;
//        v->lastSin = 0;
//    }

    // 反变换
//    v->iPark.Sin = v->Park.Sin;
//    v->iPark.Cos = v->Park.Cos;
//    v->iPark.Ds = v->PICtrl.Out;
//    v->iPark.Alph = v->iPark.Ds * v->iPark.Sin;  // 22

    // 重复控制-交流控制
    if (v->ReEnable)
    {
        v->RepCtrl.Ref = v->PICtrl.Ref*v->Park.Sin;
        v->RepCtrl.Fdb = v->Sogi.u[0];
        RepeatControlFunc(&v->RepCtrl);
        v->PRVars.Errn = (v->RepCtrl.Ref - v->RepCtrl.Fdb);
        CNTL_2P2Z_F_FUNC(&v->PRCoeffs, &v->PRVars);
        v->R3Vars.Errn = v->PRVars.Errn;
        CNTL_2P2Z_F_FUNC(&v->R3Coeffs, &v->R3Vars);
        v->R5Vars.Errn = v->PRVars.Errn;
        CNTL_2P2Z_F_FUNC(&v->R5Coeffs, &v->R5Vars);
        v->R7Vars.Errn = v->PRVars.Errn;
        CNTL_2P2Z_F_FUNC(&v->R7Coeffs, &v->R7Vars);
        PeakVoltCntl(v);
    }
    else
    {
//        v->RepCtrl.Data[v->RepCtrl.OSTheta] = 0;
        v->RepCtrl.CfOut = 0;
        v->RepCtrl.Out = 0;
    }

    LowpowerCntl(v);


//    v->Out = (v->Fw * v->FwCoeff + v->PRVars.Out + v->R3Vars.Out + v->R5Vars.Out + v->R7Vars.Out + v->LowPoWVars.Out
//             + 200 * v->RepCtrl.CfOut + v->KBus );

//    v->Fw = __fsat(v->Fw, 145 + v->KBus, -145 + v->KBus) - DroopCntl.Ro * v->IL;

    v->Out0 = (v->Fw * v->FwCoeff + v->KBus + v->PRVars.Out + v->R3Vars.Out + v->R5Vars.Out + v->R7Vars.Out + v->LowPoWVars.Out
                + 200 * v->RepCtrl.CfOut + v->RcdLimit);

    v->Out = __fsat(v->Out0 - DroopCntl.Ro * v->IL2 - v->Rc * v->IL,156,-156);

    v->Out1 = v->Out;

    v->Out = __fsat(v->Out1*v->BusVolt, v->OutMax, v->OutMin);

    //-------------------------------双极性控制-----------------------------------
    v->CAMP = (v->Out * v->Period);

    UpdateINVPwmComp3(v->Id, v->CAMP, v->Period, (v->Park.Sin > v->lastSin));

    v->Pwmload = 1;

    v->lastSin = v->Park.Sin;
}
/*************************************************
Function: ControlOffGridInv
Description: 闭环控制,控制参数赋值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
float level = 0.002;
#pragma CODE_SECTION(ControlOffGridInv, ".TI.ramfunc");
void ControlOffGridInv(void)
{
    float tempOutMax = 0;

    if ((InvStateData.WorkState == UpsState) && (InvPhase.SyncBeginFlag == 1))
    {
        if (fabs(UrCtrl.LastFreq - CaptureData.FreqR) >= 0.7f)
        {
            UrCtrl.LastFreq = CaptureData.FreqR;
            // 当频率发生大的变化时(离网不需要),该函数要重新被调用更新参数
//            SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UrCtrl.Sogi);
//            SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UsCtrl.Sogi);
        }
    }
    else
    {
        if (UrCtrl.LastFreq != SystemPara.InvRateFreq)
        {
            UrCtrl.LastFreq = SystemPara.InvRateFreq;

            // 当频率发生大的变化时(离网不需要),该函数要重新被调用更新参数
//            SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UrCtrl.Sogi);
//            SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) (2 * PI_S * UrCtrl.LastFreq), &UsCtrl.Sogi);

            Init_PRCoeff(&UrCtrl.LowPoWCoeffs, 0, 30.0f ,__mpy2pif32(UrCtrl.LastFreq)   ,(PWM_ISR_FREQUENCY),__mpy2pif32(0.2));   //
            Init_PRCoeff(&UrCtrl.PRCoeffs, 0, (30.0f * 7) ,__mpy2pif32(UrCtrl.LastFreq) ,(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));   //
            Init_PRCoeff(&UrCtrl.R3Coeffs, 0, (15.0f * 5),__mpy2pif32(UrCtrl.LastFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));
            Init_PRCoeff(&UrCtrl.R5Coeffs, 0, (10.0f * 5),__mpy2pif32(UrCtrl.LastFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));
            Init_PRCoeff(&UrCtrl.R7Coeffs, 0, (6.0f * 3) ,__mpy2pif32(UrCtrl.LastFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.02));//0.07


        }
    }

    UsCtrl.PRCoeffs = UrCtrl.PRCoeffs;
    UsCtrl.R3Coeffs = UrCtrl.R3Coeffs;
    UsCtrl.R5Coeffs = UrCtrl.R5Coeffs;
    UsCtrl.R7Coeffs = UrCtrl.R7Coeffs;
    UsCtrl.LowPoWCoeffs = UrCtrl.LowPoWCoeffs;

    OffGridVolt();
    //--------------------------------R相--------------------------------------
    // 母线电压赋值
//    UrCtrl.BusVolt = SampleIns.Bus1VoltIns;
    // 输出占空比限幅
//    tempOutMax = 200.0f/SampleRms.Bus1VoltFilter200;
//    UrCtrl.OutMax = 200.0f/SampleIns.Bus1VoltIns;
//    UrCtrl.OutMax = __fmin(UrCtrl.OutMax, tempOutMax);
//    UrCtrl.OutMax = __fsat(UrCtrl.OutMax, 0.87f, 0.72f);
//    UrCtrl.OutMax = 0.99;
//    UrCtrl.OutMin = -UrCtrl.OutMax;
    // SOGI 正交分解
    UrCtrl.Sogi.u[0] = SampleIns.Inv1VoltIns;
//    SPLL_1ph_SOGI_F_FUNC(&UrCtrl.Sogi);
    // Park 变换
    UrCtrl.Park.Sin = InvPhase.SinR;
    UrCtrl.Park.Cos = InvPhase.CosR;
//    UrCtrl.sin3 = __sin(3*InvPhase.ThetaR);   // 计算R相角度的正弦值
//    UrCtrl.cos3 = __cos(3*InvPhase.ThetaR);   // 计算R相角度的正弦值
//    UrCtrl.Park.Alph = UrCtrl.Sogi.osg_u[0];
//    UrCtrl.Park.Beta = UrCtrl.Sogi.osg_qu[0];
//    UrCtrl.Park.Ds = UrCtrl.Park.Alph * UrCtrl.Park.Sin - UrCtrl.Park.Beta * UrCtrl.Park.Cos;   // 9cycle
//    UrCtrl.Park.Qs = UrCtrl.Park.Alph * UrCtrl.Park.Cos + UrCtrl.Park.Beta * UrCtrl.Park.Sin;   // 9cycle
//    ParkCalc(&UrCtrl.Park);
    // 在此处添加直流量的滤波
    // 重复控制
    UrCtrl.RepCtrl.OSTheta = InvPhase.ThetaPointR;
    UrCtrl.RepCtrl.PeriodPoint = InvPhase.PeriodPoint;  // 当前一个周期点数

    //--------------------------------S相--------------------------------------
    // 母线电压赋值
//    UsCtrl.BusVolt = SampleIns.Bus2VoltIns;
    // 输出占空比限幅
//    tempOutMax = 200.0f/SampleRms.Bus2VoltFilter200;
//    UsCtrl.OutMax = 200.0f/SampleIns.Bus2VoltIns;
//    UsCtrl.OutMax = __fmin(UsCtrl.OutMax, tempOutMax);
//    UsCtrl.OutMax = __fsat(UsCtrl.OutMax, 0.9f, 0.72f);
//    UsCtrl.OutMax = 0.99;
//    UsCtrl.OutMin = -UsCtrl.OutMax;
    // SOGI 正交分解
    UsCtrl.Sogi.u[0] = SampleIns.Inv2VoltIns;
//    SPLL_1ph_SOGI_F_FUNC(&UsCtrl.Sogi);
    // Park 变换
//    UsCtrl.cos3 = __cos(3*InvPhase.ThetaS);   // 计算R相角度的正弦值
    UsCtrl.Park.Sin = InvPhase.SinS;
    UsCtrl.Park.Cos = InvPhase.CosS;
//    UsCtrl.Park.Alph = UsCtrl.Sogi.osg_u[0];
//    UsCtrl.Park.Beta = UsCtrl.Sogi.osg_qu[0];
 //   UsCtrl.Park.Ds = UsCtrl.Park.Alph * UsCtrl.Park.Sin - UsCtrl.Park.Beta * UsCtrl.Park.Cos;   // 9cycle
//    UsCtrl.Park.Qs = UsCtrl.Park.Alph * UsCtrl.Park.Cos + UsCtrl.Park.Beta * UsCtrl.Park.Sin;   // 9cycle
//    ParkCalc(&UsCtrl.Park);
    // 重复控制
    UsCtrl.RepCtrl.OSTheta = InvPhase.ThetaPointS;
    UsCtrl.RepCtrl.PeriodPoint = InvPhase.PeriodPoint;  // 当前一个周期点数

    UrCtrl.Io = SampleRms.Inv1CurrRms;
    UsCtrl.Io = SampleRms.Inv2CurrRms;

    SampleADRaw.Inv1CurrRaw = AdcdResultRegs.ADCRESULT6;
    SampleADRaw.Inv2CurrRaw = AdcdResultRegs.ADCRESULT2;
    SampleIns.Inv1CurrIns = ((int32_t)SampleCalibCoef.Inv1CurrK*(SampleADRaw.Inv1CurrRaw - SampleZeroCalibrate.Inv1CurrZero)>>12)*0.1f;
    SampleIns.Inv2CurrIns = ((int32_t)SampleCalibCoef.Inv2CurrK*(SampleADRaw.Inv2CurrRaw - SampleZeroCalibrate.Inv2CurrZero)>>12)*0.1f;

    UrCtrl.IL = SampleIns.Inv1CurrIns - SampleIns.Load1CurrIns;
    UsCtrl.IL = SampleIns.Inv2CurrIns - SampleIns.Load2CurrIns;
//
    UrCtrl.IL2 = SampleIns.Inv1CurrIns;
    UsCtrl.IL2 = SampleIns.Inv2CurrIns;

//    OffGridPara();
    if (SystemPara.OpenLoopEn == 1)
    {
        OpenLoopOffGridInv11(&UrCtrl);
        OpenLoopOffGridInv11(&UsCtrl);
    }
    else if((InvStateData.WorkState == 3)||(InvStateData.WorkState == 4))
    {
        ControlSingleOffGridInv11(&UrCtrl);
        ControlSingleOffGridInv11(&UsCtrl);
        Flag_Local.Bits.invPwmOn = 1;
//        UpdateINVPwmComp3(UrCtrl.Id, UrCtrl.CAMP, UrCtrl.Period, 0);
//        UpdateINVPwmComp3(UsCtrl.Id, UsCtrl.CAMP, UsCtrl.Period, 0);
    }
    OffGridPowerCal();
}

//=================================================================================
// 函数名称:
// 函数功能:离网电压给定调整
// 输入参数: 无
// 输出参数: 无
// 其它说明:
//=================================================================================
void OffGridVolt(void)
{
 //   static float LastBusData[230] = {0};
 //   int theta = 0;

    mRefRamp(UrCtrl.BusErrCtrl.Out , SampleRms.Bus2VoltFilter200 - SampleRms.Bus1VoltFilter200, 1e-4f);

//    UrCtrl.BusErrCtrl.Out += UrCtrl.BusErrCtrl.Ki * (SampleRms.Bus2VoltFilter200 - SampleRms.Bus1VoltFilter200);
    UrCtrl.BusErrCtrl.Out = __fsat(UrCtrl.BusErrCtrl.Out,5,-5);

    UrCtrl.KBus = (SampleIns.Bus2VoltIns - SampleIns.Bus1VoltIns)*0.5- UrCtrl.BusErrCtrl.Out;   //抑制单相瞬间波动  //0.5
    UsCtrl.KBus = UrCtrl.KBus;


//    if(InvPhase.ThetaPointR >= (InvPhase.PeriodPoint>>1))
//        theta = InvPhase.ThetaPointR - (InvPhase.PeriodPoint>>1);
//    else
//        theta = InvPhase.ThetaPointR;

    if(UrCtrl.PWMONflagCnt > 110 && UrCtrl.ReEnable)
    {
        float bustmp= __fsat(SampleIns.BUSVoltInsCom,SampleIns.BUSVoltIns + 15,SampleIns.BUSVoltIns - 15);
        UrCtrl.BusVolt = 1/(SampleIns.BUSVoltInsCom*0.5);
    }
    else
    {
        UrCtrl.BusVolt = 1/(SampleIns.BUSVoltIns *0.5);
    }
//    LastBusData[theta] = SampleIns.BUSVoltIns;

//    if(PWMONflag)
//        UrCtrl.BusVolt = SampleIns.BUSVoltIns*0.5;
//    else
////        UrCtrl.BusVolt = (SampleRms.BUSVoltFilter200)*0.5;
////        UrCtrl.BusVolt = (SampleRms.BUSVoltFilter200 + SampleIns.BUSVoltIns)*0.25;
//    UrCtrl.BusVolt = SampleIns.BUSVoltIns*0.5;

//    UrCtrl.BusVolt =(SampleIns.Bus2VoltIns + SampleIns.Bus1VoltIns)*0.5;
    UsCtrl.BusVolt = UrCtrl.BusVolt;

    //母线低压时降低目标电压，但是现在已经会BUS欠压先保护
//    if(__fmin(SampleRms.Bus1VoltFilter200,SampleRms.Bus2VoltFilter200) <= 155)
//    {
//        UrCtrl.RefTarget = SampleRms.Bus1VoltFilter200 * 0.9215f;
//        UsCtrl.RefTarget = UrCtrl.RefTarget;
//    }
//    else
//    {
//        UrCtrl.RefTarget = 142.83f + VaRmsCntl.yp[0];
//        UsCtrl.RefTarget = 142.83f + VbRmsCntl.yp[0];
////    }

    UrCtrl.FwRamp.TargetValue = (SystemPara.InvRateVolt*1.414159f) + VaRmsCntl.yp[0];
    UsCtrl.FwRamp.TargetValue = (SystemPara.InvRateVolt*1.414159f) + VbRmsCntl.yp[0];

    UrCtrl.RefRamp.TargetValue = UrCtrl.FwRamp.TargetValue;
    UsCtrl.RefRamp.TargetValue = UsCtrl.FwRamp.TargetValue;

}

 float krms1 = 0.36;  //1.1f-5k
 float krms2 = 0.46;  //1.1f-5k

#pragma CODE_SECTION(LowpowerCntl, ".TI.ramfunc");
void LowpowerCntl(OFF_GRID_CONTROL_COEFF *v)
{
    if(PWMONflag)
    {
        float M = v->FwRamp.TargetValue*v->Park.Sin*v->BusVolt;
//        float sin = 0.9* v->Park.Cos + 0.32* v->Park.Sin;
        float sin =  v->Park.Cos;

        if(sin>0)
        {
            float tmp =(1 - M)/(1 + M);
            v->Fw = 2.6* __sqrt((fabs(sin)/v->BusVolt)/tmp);
        }
        else
        {
            float tmp =(1 + M)/(1 - M);
            v->Fw =-2.6* __sqrt((fabs(sin)/v->BusVolt)/tmp);
        }

//        v->Fw = v->FwRamp.TargetValue *(0.9* v->Park.Cos - 0.2*v->cos3 + 0.32* v->Park.Sin) * v->LowPoWKrms;

        v->RepCtrl.kp = 5e-4f;

        Init_CNTL_2P2Z_F_VARS_(&UrCtrl.PRVars, KPR , -KPR);
        Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R3Vars, KPR, -KPR); //@sjj20260202
        Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R5Vars, KPR, -KPR); //@sjj20260202
        Init_CNTL_2P2Z_F_VARS_(&UrCtrl.R7Vars, KPR, -KPR); //@sjj20260202
        UsCtrl.PRVars = UrCtrl.PRVars;
        UsCtrl.R3Vars = UrCtrl.R3Vars;
        UsCtrl.R5Vars = UrCtrl.R5Vars;
        UsCtrl.R7Vars = UrCtrl.R7Vars;

        v->LowPoWVars.Errn = (v->RepCtrl.Ref - v->RepCtrl.Fdb);
        CNTL_2P2Z_F_FUNC(&v->LowPoWCoeffs, &v->LowPoWVars);

//        if(v->LowPoWKrms < krms2)
//            v->LowPoWKrms+=0.0001;

//        v->iPark.Alph = 0;

        v->PWMONflagCnt = 0;
        v->Rc = 0;
    }
    else
    {
        //切换 过零
        if (++v->PWMONflagCnt < 110)
        {
            if(v->PWMONflagCnt == 1)
            {
                v->Fw = v->Sogi.u[0];
            }
//            else
//            {
            float temp = v->FwRamp.TargetValue * (0.99*v->Park.Sin + 0.05*v->Park.Cos); //*0.95// - DroopCntl.Ro*v->IL

            float step = __fsat(fabsf(v->Fw - temp)*0.3, 40, 10); //0.7->0.3

            if(v->Fw < temp - 10)
            {
                v->Fw += step;
            }
            else if(v->Fw > temp + 10)
            {
                v->Fw -= step;
            }
            else
            {
                v->Fw = temp;
            }
//            }
//            v->Fw = v->FwRamp.TargetValue * (0.99*v->Park.Sin + 0.05*v->Park.Cos) + 0.2f*v->Io* v->Park.Sin - DroopCntl.Ro*v->IL;

            v->RepCtrl.Data[v->PWMONflagCnt + 100] = 0;
            v->RepCtrl.Data[v->PWMONflagCnt] = 0;
            v->RepCtrl.CfOut = 0;
            v->RepCtrl.kp = 5e-5f; // * __fsat(1.2 - fabsf(v->Park.Sin) ,1 ,0.1)
            v->ReEnable = 0;
        }
        else
        {
            if(InvPhase.ThetaPointR == 1)
            {
                v->ReEnable = 1;
            }

            v->Fw = v->FwRamp.TargetValue * (0.99*v->Park.Sin + 0.05*v->Park.Cos);//- DroopCntl.Ro*v->IL
            v->PWMONflagCnt = 111;
        }

        Init_CNTL_2P2Z_F_VARS_(&v->LowPoWVars, 40 , -40);

//        v->LowPoWKrms = (SystemPara.InvRateFreq * 0.003f) + 0.26f;    //krms1; 0.24+0.15 = 0.4
//        v->LowPoWCtrl.Out = 0;
//        v->LowPoWCtrl.i1 = 0;
        v->Rc = (111 - v->PWMONflagCnt)*0.02f;
//        v->Rc = 0;
//        v->Rc = (211)*0.01f;
//        v->PICtrl.Kp = 0.005f;      //
//        v->KpSet = 0.005f;      //
//        PWMONflagFirstFlg = 1;
    }

}

#pragma CODE_SECTION(PeakVoltCntl, ".TI.ramfunc");
void PeakVoltCntl(OFF_GRID_CONTROL_COEFF *v)
{
    float Krcd = __fsat( fabsf(v->IL)*0.05, 1, 0.1);

    if(v->RepCtrl.Fdb > (v->PICtrl.Ref + 2))
    {
        v->RcdLimit = Krcd*((v->PICtrl.Ref + 2) - v->RepCtrl.Fdb);
    }
    else if (v->RepCtrl.Fdb < (-v->PICtrl.Ref - 2))
    {
        v->RcdLimit = Krcd*((-v->PICtrl.Ref - 2) - v->RepCtrl.Fdb);
    }
    else
    {
        v->RcdLimit = 0;
    }
}


#pragma CODE_SECTION(InvPwmReload, ".TI.ramfunc");
void InvPwmReload(void)
{
    if( UrCtrl.Pwmload)
    {
        UrCtrl.Pwmload = 0;

        SampleADRaw.Inv1CurrRaw = AdcdResultRegs.ADCRESULT6;
        SampleADRaw.Inv2CurrRaw = AdcdResultRegs.ADCRESULT2;
        SampleIns.Inv1CurrIns = ((int32_t)SampleCalibCoef.Inv1CurrK*(SampleADRaw.Inv1CurrRaw - SampleZeroCalibrate.Inv1CurrZero)>>12)*0.1f;
        SampleIns.Inv2CurrIns = ((int32_t)SampleCalibCoef.Inv2CurrK*(SampleADRaw.Inv2CurrRaw - SampleZeroCalibrate.Inv2CurrZero)>>12)*0.1f;

        UrCtrl.IL = SampleIns.Inv1CurrIns - SampleIns.Load1CurrIns;
        UsCtrl.IL = SampleIns.Inv2CurrIns - SampleIns.Load2CurrIns;

        UrCtrl.IL2 = SampleIns.Inv1CurrIns;
        UsCtrl.IL2 = SampleIns.Inv2CurrIns;

        UrCtrl.Out = __fsat(UrCtrl.Out0 - DroopCntl.Ro * UrCtrl.IL2 - UrCtrl.Rc * UrCtrl.IL,156,-156);
        UsCtrl.Out = __fsat(UsCtrl.Out0 - DroopCntl.Ro * UsCtrl.IL2 - UsCtrl.Rc * UsCtrl.IL,156,-156);

        UrCtrl.Out2 = UrCtrl.Out;
        UsCtrl.Out2 = UsCtrl.Out;

        UrCtrl.Out = __fsat(UrCtrl.Out2*UrCtrl.BusVolt, UrCtrl.OutMax, UrCtrl.OutMin);
        UsCtrl.Out = __fsat(UsCtrl.Out2*UsCtrl.BusVolt, UsCtrl.OutMax, UsCtrl.OutMin);

        //-------------------------------双极性控制-----------------------------------
        UrCtrl.PWMDuty = (UrCtrl.Out * UrCtrl.Period);
        UsCtrl.PWMDuty = (UsCtrl.Out * UsCtrl.Period);

        UpdateINVPwmComp3(UrCtrl.Id, UrCtrl.PWMDuty, UrCtrl.Period, 0);
        UpdateINVPwmComp3(UsCtrl.Id, UsCtrl.PWMDuty, UsCtrl.Period, 0);

    }
}

/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: OnGridInv2.c
Author: tzz
Date:2022年3月1日
Description: 并网控制
Version:0.0.2
Function List:
***********************************************************/
#include <bsp.h>
#include <string.h>
#include "Sample.h"
#include "Protect.h"
#include "RepCtrl.h"
#include "OnGridInv2.h"
#include "ModbusTable.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "ConfigSystemPara.h"
#include "PowerCalculator.h"
#include "Energy_Manage.h"
#include "Function.h"

char GridPWMflg =0;
//ON_GRID_CONTROL_COEFF IrCtrl, IsCtrl;
//#pragma DATA_SECTION(IrCtrl, "user_var_data3");
ON_GRID_CONTROL_COEFF IrCtrl;
//#pragma DATA_SECTION(IsCtrl, "user_var_data3");
ON_GRID_CONTROL_COEFF IsCtrl;

void InitAcdcMode(void);
void AcdcMode(void);
char UpsVoltChk(ON_GRID_CONTROL_COEFF *v);
void GridEnhanceCHK(void);
void DCIAdjustR(void);
void DCIAdjustS(void);
I_REAL_REF RealRefR,RealRefS;
int16_t PwmOld = 0;
int16_t Grid1EnhanceDrop=0;
int16_t Grid2EnhanceDrop=0;
#define ACTIVE_CORRECTION_FACTOR     (1.086f)       // 有功目标修正系数1.065f改为1.086
#define INACTIVE_CORRECTION_FACTOR   (1.0f)         // 无功目标修正系数
/*************************************************
Function: InitOnGridInvControlData
Description: 初始化并网控制模块变量
Input: None
Output: None
Frequency: Once
*************************************************/
void InitOnGridInvControlData(void)
{
    memset(&IrCtrl, 0, sizeof(ON_GRID_CONTROL_COEFF));
    memset(&IsCtrl, 0, sizeof(ON_GRID_CONTROL_COEFF));
    memset(&RealRefR, 0, sizeof(I_REAL_REF));
    memset(&RealRefS, 0, sizeof(I_REAL_REF));

    IrCtrl.Id = 1;
    // 参考设置
    IrCtrl.RmpCtrl.RampHighLimit = 37.5f*1.414f*ACTIVE_CORRECTION_FACTOR;     // 放电
    IrCtrl.RmpCtrl.RampLowLimit = -37.5f*1.414f*INACTIVE_CORRECTION_FACTOR;     // 充电
    IrCtrl.RmpCtrl.StepValue = 10.0f/(INV_PWM_FREQ_KHZ*1000.0*0.5f);    // 每s增加10A
    IrCtrl.RmpCtrl.TargetValue = 10.0f; // 目标电流 10A 测试用
    // PWM参数
    IrCtrl.Period = INV_EPWM_PERIOD;
    // 前馈系数
    IrCtrl.FwCoeff = 1.04f;//1.08f
    // TODO 重复控制参数
    IrCtrl.ReEnable = 1;
    IrCtrl.RepCtrl.OutMax = 0.15f;
    IrCtrl.RepCtrl.OutMin = -0.15f;
    IrCtrl.RepCtrl.ThetaShift = 2;      // 1或者3都会发散
    IrCtrl.RepCtrl.kp = 0;
    IrCtrl.RepCtrl.kpSet =  0.004f;     // 0.004f
    // PI 参数设置
    IrCtrl.IKpSet = 0.01f;//0.01f
    IrCtrl.PICtrl.Ki = 0;
    IrCtrl.PICtrl.Umax = 0.1f;
    IrCtrl.PICtrl.Umin = -0.1f;

    IrCtrl.ZeroPoint = SystemPara.InvRateFreq * 0.001f;
    IsCtrl.ZeroPoint = IrCtrl.ZeroPoint;

    Init_VffFilter(&IrCtrl.Vgff);

//    Init_PRCoeff(&IrCtrl.PRCoeffs,  0, 12.0f ,__mpy2pif32(SystemPara.InvRateFreq),(PWM_ISR_FREQUENCY),__mpy2pif32(0.2));   //15&0.2振荡
//    Init_PRCoeff(&IrCtrl.PR3Coeffs, 0, 4.0f ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.05));
//    Init_PRCoeff(&IrCtrl.PR5Coeffs, 0, 4.0f ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.05));
//    Init_PRCoeff(&IrCtrl.PR7Coeffs, 0, 4.0f ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.05));//0.07

//    Init_PRCoeff(&IrCtrl.PRCoeffs,  0, 44.0f ,__mpy2pif32(SystemPara.InvRateFreq),(PWM_ISR_FREQUENCY),__mpy2pif32(0.04));   //
//    Init_PRCoeff(&IrCtrl.PR3Coeffs, 0, 20.0f ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
//    Init_PRCoeff(&IrCtrl.PR5Coeffs, 0, 16.0f ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
//    Init_PRCoeff(&IrCtrl.PR7Coeffs, 0, 12.0f ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));//0.07
      Init_PRCoeff(&IrCtrl.PRCoeffs,  0, 22.0f ,__mpy2pif32(SystemPara.InvRateFreq),(PWM_ISR_FREQUENCY),__mpy2pif32(0.04));   //
      Init_PRCoeff(&IrCtrl.PR3Coeffs, 0, 10.0f ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
      Init_PRCoeff(&IrCtrl.PR5Coeffs, 0, 6.0f ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
      Init_PRCoeff(&IrCtrl.PR7Coeffs, 0, 6.0f ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));//0.07
    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PRVars , 0.2 , -0.2);
    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR3Vars, 0.3 , -0.3); //@sjj20250911
    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR5Vars, 0.3 , -0.3); //@sjj20250911
    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR7Vars, 0.2 , -0.2); //@sjj20250911

    IsCtrl = IrCtrl;
    IsCtrl.Id = 2;

    RealRefR.DeltaTheta.RampHighLimit = PI_S*0.5f;
    RealRefR.DeltaTheta.RampLowLimit = -PI_S*0.5f;
    RealRefR.DeltaTheta.StepValue = PI_S/9.0f/(INV_PWM_FREQ_KHZ*1000.0f*0.5f); // 每秒增加30度
    RealRefS = RealRefR;

    // 无功补偿值渐变
    RealRefR.QRef.RampHighLimit = 1750.0f;
    RealRefR.QRef.RampLowLimit = -1750.0f;
    RealRefR.QRef.StepValue = 5000.0f/(PWM_ISR_FREQUENCY*1.0f);    // 5000var/s
    RealRefS.QRef = RealRefR.QRef;
    ArmWriteData.DCI1=0.3;
    ArmWriteData.DCI2=0.3;

    InitAcdcMode();
}
/*************************************************
Function: CleanOnGridCtrlData
Description: 清除并网控制参数,包括:重复控制缓存
Input: @cnt-重复控制缓存数组索引
Output: None
Frequency: 离网转并网时执行一次
*************************************************/
void CleanOnGridCtrlData(uint16_t cnt)
{
    if (cnt >= 110)
    {
        cnt = 110;
    }
    IrCtrl.RepCtrl.Data[2*cnt] = 0;
    IrCtrl.RepCtrl.Data[2*cnt+1] = 0;
    IsCtrl.RepCtrl.Data[2*cnt] = 0;
    IsCtrl.RepCtrl.Data[2*cnt+1] = 0;
}

void CleanRPhaseOnGridCtrlData(uint16_t cnt)
{
    if (cnt >= 210)
    {
        cnt = 210;
    }
    IrCtrl.RepCtrl.Data[cnt] = 0;
}
void CleanSPhaseOnGridCtrlData(uint16_t cnt)
{
    if (cnt >= 210)
    {
        cnt = 210;
    }
    IsCtrl.RepCtrl.Data[cnt] = 0;
}
/*************************************************
Function: ClearOnGridPidData
Description: 清除并网控制参数,PI积分,缓起初始值
Input: None
Output: None
Frequency: 离网转并网时执行一次
*************************************************/
void ClearOnGridPidDataR(void)
{
    IrCtrl.PICtrl.i1 = 0;

    IrCtrl.RmpCtrl.SetpointValue = 0;
    IrCtrl.RmpCtrl.TargetValue =0;

    RealRefR.DeltaTheta.SetpointValue = 0;

    IrCtrl.IdcSum = 0;

    IrCtrl.IdcCurr = 0;

    IrCtrl.ZeroPoint = SystemPara.InvRateFreq * 0.001f;
    IrCtrl.DCIErr = 0;
    IrCtrl.DCIErrOld = 0;
    IrCtrl.DCIAdjOut = 0;
}
void ClearOnGridPidDataS(void)
{
    IsCtrl.PICtrl.i1 = 0;

    IsCtrl.RmpCtrl.SetpointValue = 0;
    IsCtrl.RmpCtrl.TargetValue =0;

    RealRefS.DeltaTheta.SetpointValue = 0;

    IsCtrl.IdcSum = 0;

    IsCtrl.IdcCurr = 0;

    IsCtrl.ZeroPoint = IrCtrl.ZeroPoint;
    IsCtrl.DCIErr = 0;
    IsCtrl.DCIErrOld = 0;
    IsCtrl.DCIAdjOut = 0;
}
void ClearOnGridSinglePidData(uint16_t ch)
{
    if (ch == 1)
    {
        IrCtrl.PICtrl.i1 = 0;
        IrCtrl.RmpCtrl.SetpointValue = 0;//v->PRVars.Out + v->PR3Vars.Out + v->PR7Vars.Out
        IrCtrl.IdcSum = 0;
        IrCtrl.IdcCurr = 0;
        IrCtrl.PRVars.Out=0;
        IrCtrl.PR3Vars.Out=0;
        IrCtrl.PR7Vars.Out=0;
    }
    else if (ch == 2)
    {
        IsCtrl.PICtrl.i1 = 0;
        IsCtrl.RmpCtrl.SetpointValue = 0;
        IsCtrl.IdcSum = 0;
        IsCtrl.IdcCurr = 0;
        IsCtrl.PRVars.Out=0;
		IsCtrl.PR3Vars.Out=0;
		IsCtrl.PR7Vars.Out=0;
    }
}
/*************************************************
Function: SetCurrStepValue
Description: 设置电流步长
Input: @set-电流步长设定值单位A/s
Output: None
Frequency: 10KHz
*************************************************/
void SetCurrStepValue(float set)
{
    IrCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A
    IsCtrl.RmpCtrl.StepValue = IrCtrl.RmpCtrl.StepValue;
}
/*************************************************
Function: ControlSingleOnGridInv
Description: 单相逆变并网控制
Input: @v-并网逆变器控制结构体变量
Output: None
Frequency: 离网转并网时执行一次
*************************************************/
#pragma CODE_SECTION(ControlSingleOnGridInv, ".TI.ramfunc");
void ControlSingleOnGridInv(ON_GRID_CONTROL_COEFF *v)
{
    int16_t ZeroFlag = 0;
    //-------------------------------PI----------------------------------------
    v->RmpCtrl.finalCurr = RmpCntlCalc(&v->RmpCtrl);
    v->PICtrl.Ref = v->RmpCtrl.finalCurr * v->SinTheta;
    CNTL_PI_F_FUNC(&v->PICtrl);

    v->PRVars.Errn = (v->PICtrl.Ref - v->PICtrl.Fbk);
    CNTL_2P2Z_F_FUNC(&v->PRCoeffs, &v->PRVars);
    v->PR3Vars.Errn = v->PRVars.Errn;
    CNTL_2P2Z_F_FUNC(&v->PR3Coeffs, &v->PR3Vars);
    v->PR5Vars.Errn = v->PRVars.Errn;
    CNTL_2P2Z_F_FUNC(&v->PR5Coeffs, &v->PR5Vars);
    v->PR7Vars.Errn = v->PRVars.Errn;
    CNTL_2P2Z_F_FUNC(&v->PR7Coeffs, &v->PR7Vars);
    //-------------------------------重复控制-------------------------------------
    if (v->ReEnable)
    {
        // 重复控制
        v->RepCtrl.Ref = v->PICtrl.Ref;
        v->RepCtrl.Fdb = v->PICtrl.Fbk;
        RepeatControlFunc2(&v->RepCtrl);
    }
    else
    {
        v->RepCtrl.Out = 0;
        v->RepCtrl.CfOut = 0;
    }
    //----------------------------------前馈----------------------------------
    v->Fw = __divf32(v->Fw, v->BusVolt) * v->FwCoeff;    // 电网/BUS

//    v->Out = v->PICtrl.Out + v->Fw + v->RepCtrl.CfOut;

    v->gain = __fmax(1.0f - fabsf(v->PICtrl.Ref) * 0.016f ,0.3f);

    v->Out =  v->Fw + (v->PICtrl.Out + v->RepCtrl.CfOut + v->PRVars.Out + v->PR3Vars.Out + v->PR5Vars.Out + v->PR7Vars.Out)*v->gain; //

    v->Out = __fsat(v->Out, 1.0f, -1.0f);             // 限制占空比

    v->PWMDuty = (int16_t)(v->Out * v->Period);

//    if ((v->PWMDuty >= 0) && (PwmOld < 0))
//    {
//        ZeroFlag = 0;
//    }
//    if((v->PWMDuty <= 0) && (PwmOld > 0))
//    {
//        ZeroFlag = 1;
//    }
//    else
//    {
//        ZeroFlag = 2;
//    }
//    PwmOld = v->PWMDuty;
    // 单边
    UpdateINVPwmComp3(v->Id, v->PWMDuty, v->Period, ZeroFlag);

    v->PWMOn = 1;
}
/*************************************************
Function: GridEnhanceMode
Description: 计算电网增强模式下电流参考值
Input: @channel-1:R相;2-S相
Output: None
Frequency: 10KHz
*************************************************/
void GridEnhanceMode(uint16_t channel)
{
    float fGridLow = 101.f * 0.9f;//120
    float fGridHigh = 101.f * 1.05f;
    float set = 0;

    fGridLow = 85.0f;

    if (channel == 1)
    {
        // 电网电压高，如果并网馈电状态下开始降功率如果充电不降功率
        if(SampleRms.Grid1VoltRms > fGridHigh)
        {
            set = 10 + 3*(SampleRms.Grid1VoltRms - fGridHigh);
            IrCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A

            if (IrCtrl.RmpCtrl.SetpointValue > 0)
            {
                IrCtrl.RmpCtrl.SetpointValue -= IrCtrl.RmpCtrl.StepValue;
                IrCtrl.RmpCtrl.SetpointValue = __fmax(IrCtrl.RmpCtrl.SetpointValue, 0);
                RealRefR.IRef = IrCtrl.RmpCtrl.SetpointValue;
            }
        }
        // 电网电压低，并网充电状态下开始降功率如果馈电不降功率
        else if (SampleRms.Grid1VoltRms < fGridLow)
        {
            set = (10 + 3*(fGridLow - SampleRms.Grid1VoltRms));
            IrCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A

            if (IrCtrl.RmpCtrl.SetpointValue < 0)
            {
                IrCtrl.RmpCtrl.SetpointValue += IrCtrl.RmpCtrl.StepValue;
                IrCtrl.RmpCtrl.SetpointValue = __fmin(IrCtrl.RmpCtrl.SetpointValue, 0);
                RealRefR.IRef = IrCtrl.RmpCtrl.SetpointValue;
            }
            // 馈电时
            else
            {
                IrCtrl.RmpCtrl.SetpointValue -= IrCtrl.RmpCtrl.StepValue;
                IrCtrl.RmpCtrl.SetpointValue = __fmax(IrCtrl.RmpCtrl.SetpointValue, 0);
                RealRefR.IRef = IrCtrl.RmpCtrl.SetpointValue;
            }
        }
        else if ((SampleDisplayData.Grid1Volt <= (fGridHigh - 2.0f)) && (SampleDisplayData.Grid1Volt >= (fGridLow+2.0f)))
        {
            set = (10);
            IrCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A
        }
        else
        {
            set = (10);
            IrCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A
//            IrCtrl.RmpCtrl.StepValue = 0;    // 每s增加1A
        }
    }
    else if (channel == 2)
    {
        if (SampleRms.Grid2VoltRms > fGridHigh)
        {
            set = (10 + 3*(SampleRms.Grid2VoltRms - fGridHigh));
            IsCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A

            if (IsCtrl.RmpCtrl.SetpointValue > 0)
            {
                IsCtrl.RmpCtrl.SetpointValue -= IsCtrl.RmpCtrl.StepValue;
                IsCtrl.RmpCtrl.SetpointValue = __fmax(IsCtrl.RmpCtrl.SetpointValue, 0);

                RealRefS.IRef = IsCtrl.RmpCtrl.SetpointValue;
            }
        }
        else if (SampleRms.Grid2VoltRms < fGridLow)
        {
            set = (10 + 3*(fGridLow - SampleRms.Grid2VoltRms));
            IsCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A

            if (IsCtrl.RmpCtrl.SetpointValue < 0)
            {
                IsCtrl.RmpCtrl.SetpointValue += IsCtrl.RmpCtrl.StepValue;
                IsCtrl.RmpCtrl.SetpointValue = __fmin(IsCtrl.RmpCtrl.SetpointValue, 0);

                RealRefS.IRef = IsCtrl.RmpCtrl.SetpointValue;
            }
            // 馈电时
            else
            {
                IsCtrl.RmpCtrl.SetpointValue -= IsCtrl.RmpCtrl.StepValue;
                IsCtrl.RmpCtrl.SetpointValue = __fmax(IsCtrl.RmpCtrl.SetpointValue, 0);

                RealRefS.IRef = IsCtrl.RmpCtrl.SetpointValue;
            }
        }
        else if ((SampleDisplayData.Grid2Volt <= (fGridHigh - 2.0f)) && (SampleDisplayData.Grid2Volt >= (fGridLow+2.0f)))
        {
            set = (10);
            IsCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A
        }
        else
        {
            set = (10);
            IsCtrl.RmpCtrl.StepValue = set/(PWM_ISR_FREQUENCY*1.0f);    // 每s增加1A
//            IsCtrl.RmpCtrl.StepValue = 0;    // 每s增加1A
        }
    }
}


void GridEnhanceCHK(void)
{
    // 电网低压阈值根据电压保护值取最大值
    // 电网高压阈值根据电压保护值取最小值
    static uint16_t GridLossCntR=0;
    static uint16_t GridLossCntS=0;
    static uint32_t GridLossRecoverCntR=0;
    static uint32_t GridLossRecoverCntS=0;

    if(fabs(SampleIns.Grid1VoltIns) < 30 && SystemPara.GridEnhanceEn)
    {
        if (++GridLossCntR >= 50)//5ms
        {
            GridLossCntR = 0;
            //GridError.Grid1Err.bit.GridEnhanceDrop = 1;
            Grid1EnhanceDrop=1;
            GridLossRecoverCntR=0;
        }
    }
    else
    {
        //GridError.Grid1Err.bit.GridEnhanceDrop = 0;
        Grid1EnhanceDrop=0;
        GridLossRecoverCntR=0;
        GridLossCntR = 0;
    }

    if(fabs(SampleIns.Grid2VoltIns) < 30 && SystemPara.GridEnhanceEn)
    {
        if (++GridLossCntS >= 50)//5ms
        {
            GridLossCntS = 0;
            //GridError.Grid2Err.bit.GridEnhanceDrop = 1;
            Grid2EnhanceDrop=1;
            GridLossRecoverCntS=0;
        }
    }
    else
    {
        //GridError.Grid2Err.bit.GridEnhanceDrop = 0;
        Grid2EnhanceDrop=0;
        GridLossRecoverCntS=0;
        GridLossCntS = 0;
    }

}
/*************************************************
Function:
Description:整流与逆变的参数区分，整流模式实际逆变电压比逆变要低
Input:
Output: None
Frequency: 10KHz
*************************************************/
void OnGridParaR(void)
{
	if(IrCtrl.RmpCtrl.SetpointValue >= 0)
	{
		IrCtrl.FwCoeff = 1.04f;

		if(IrCtrl.RmpCtrl.SetpointValue<10)
		{
	        IrCtrl.IKpSet = 0.009f;
			IrCtrl.RepCtrl.kpSet =  0.03f;//0.6
		}
		else
		{
	        IrCtrl.IKpSet = 0.005f;
 			IrCtrl.RepCtrl.kpSet =  0.01f;//0.6   //0.015@sjj
		}
	}
	else
	{
		IrCtrl.FwCoeff = 1.04f;

		if(IrCtrl.RmpCtrl.SetpointValue> -16)
		{
		    IrCtrl.IKpSet = 0.008f;
			IrCtrl.RepCtrl.kpSet =  0.03f;//0.6
//            IrCtrl.IKpSet = 0.005f;
//            IrCtrl.RepCtrl.kpSet =  0.01f;//0.6
		}
		else
		{
		    IrCtrl.IKpSet = 0.005f;
			IrCtrl.RepCtrl.kpSet =  0.01f;//0.6

//            IrCtrl.IKpSet = 0.001f;
		}

	}

//	if( fabsf(IrCtrl.RmpCtrl.SetpointValue) > 16)
//	{
//	    IrCtrl.IKrSet = 0.5;
//	}
//	else
//	{
//        IrCtrl.IKrSet = 1;
//	}


	if(SystemPara.InvRateFreqLast!=SystemPara.InvRateFreq)
	{
	    Init_PRCoeff(&IrCtrl.PRCoeffs,  0, 22.0f ,__mpy2pif32(SystemPara.InvRateFreq),(PWM_ISR_FREQUENCY),__mpy2pif32(0.04));   //
	    Init_PRCoeff(&IrCtrl.PR3Coeffs, 0, 10.0f ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
	    Init_PRCoeff(&IrCtrl.PR5Coeffs, 0, 8.0f ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
	    Init_PRCoeff(&IrCtrl.PR7Coeffs, 0, 6.0f ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));//0.07
	    Init_PRCoeff(&IsCtrl.PRCoeffs,  0, 22.0f ,__mpy2pif32(SystemPara.InvRateFreq),(PWM_ISR_FREQUENCY),__mpy2pif32(0.04));   //
	    Init_PRCoeff(&IsCtrl.PR3Coeffs, 0, 10.0f ,__mpy2pif32(SystemPara.InvRateFreq*3),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
	    Init_PRCoeff(&IsCtrl.PR5Coeffs, 0, 8.0f ,__mpy2pif32(SystemPara.InvRateFreq*5),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));
	    Init_PRCoeff(&IsCtrl.PR7Coeffs, 0, 6.0f ,__mpy2pif32(SystemPara.InvRateFreq*7),(PWM_ISR_FREQUENCY),__mpy2pif32(0.03));//0.07
	}
	SystemPara.InvRateFreqLast=SystemPara.InvRateFreq;

}

void OnGridParaS(void)
{
	if(IsCtrl.RmpCtrl.SetpointValue >= 0)
	{
		 IsCtrl.FwCoeff = 1.04f;

		if(IsCtrl.RmpCtrl.SetpointValue < 10)
		{
	        IsCtrl.IKpSet = 0.009f;
			IsCtrl.RepCtrl.kpSet =  0.03f;
		}
		else
		{
	        IsCtrl.IKpSet = 0.005f;
			IsCtrl.RepCtrl.kpSet =  0.01f; //0.015@sjj


		}
	}
	else
	{
		IsCtrl.FwCoeff = 1.04f;

		if(IsCtrl.RmpCtrl.SetpointValue > -16)
		{
	        IsCtrl.IKpSet = 0.008f;
			IsCtrl.RepCtrl.kpSet =  0.03f;
//            IsCtrl.IKpSet = 0.005f;
//            IsCtrl.RepCtrl.kpSet =  0.01f;
		}
		else
		{
		    IsCtrl.IKpSet = 0.005f;
			IsCtrl.RepCtrl.kpSet =  0.01f;

//            IsCtrl.IKpSet = 0.001f;
		}
	}


//    if( fabsf(IsCtrl.RmpCtrl.SetpointValue) > 16)
//    {
//        IsCtrl.IKrSet = 0.5;
//    }
//    else
//    {
//        IsCtrl.IKrSet = 1;
//    }
}
/*************************************************
Function: CorrectInActivePowerAim
Description: 修正无功目标值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
float CorrectInActivePowerAim(float raw)
{
    float ret = 0;

//    if ((raw < 1500.0f) && (raw > 0))
    if ((raw < 5500.0f) && (raw > 0))
    {
        ret = 110.0f;
    }

    return ret;
}

/*************************************************
Function: CaculateCurrentRef
Description: 计算电流参考给定值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(CaculateCurrentRef, ".TI.ramfunc");
void CaculateCurrentRef(void)
{
    float theta = 0;
    float temp = 0;
    float phaseCmp = 0;

    // 不允许充电,电流目标值不能为负
    if (InvError.NoCharge == 1)
    {
        EnergyManage.P1out = __fsat(EnergyManage.P1out, 2750.0f, 0);
        EnergyManage.P2out = __fsat(EnergyManage.P2out, 2750.0f, 0);
    }
    // 电池低压限功率
    if (InvError.BATLowLimit == 1)
    {
        if (InvError.NoCharge == 1)
        {
            EnergyManage.P1out = __fsat(EnergyManage.P1out, 1000.0f, 0);
            EnergyManage.P2out = __fsat(EnergyManage.P2out, 1000.0f, 0);
        }
        else
        {
            EnergyManage.P1out = __fsat(EnergyManage.P1out, 1000.0f, -1000);
            EnergyManage.P2out = __fsat(EnergyManage.P2out, 1000.0f, -1000);
        }
    }
    // 电网端口电流过流将目标功率为0
    if (GridError.Grid1Err.bit.GridOC == 1)
    {
        EnergyManage.P1out = __fsat(EnergyManage.P1out, 2750.0f, 0);
    }
    if (GridError.Grid2Err.bit.GridOC == 1)
    {
        EnergyManage.P2out = __fsat(EnergyManage.P2out, 2750.0f, 0);
    }

    // 发电机接入标志,只能充电不能馈电
    if (ArmWriteData.Model.bit.DieselMode == 1)
    {
        if ((GridError.Grid1Err.bit.GridOC == 1) ||
                (GridError.Grid2Err.bit.GridOC == 1) ||
                (InvError.NoCharge == 1))
        {
            EnergyManage.P1out = 0;
            EnergyManage.P2out = 0;
        }
        else
        {
            if (InvError.BATLowLimit == 1)
            {
                EnergyManage.P1out = __fsat(EnergyManage.P1out, 0.0f, -1000);
                EnergyManage.P2out = __fsat(EnergyManage.P2out, 0.0f, -1000);
            }
            else
            {
                EnergyManage.P1out = __fsat(EnergyManage.P1out, 0.0f, -2750);
                EnergyManage.P2out = __fsat(EnergyManage.P2out, 0.0f, -2750);
            }
        }
    }

    // -------------------------功率目标值-----------------------------
    // R 相 功率目标值
//    CorrectActivePowerAim(EnergyManage.P1out,EnergyManage.P2out);
    RealRefR.P = EnergyManage.P1out;//FuncPara.L1PAimUse
    RealRefR.QRef.TargetValue =EnergyManage.Q1out;// CorrectInActivePowerAim(SystemPara.ActivePowerSetR);  // 此处无功需要缓启防止小功率馈电时功率过冲
    RealRefR.Q = RmpCntlCalc(&RealRefR.QRef);
    // S 相功率目标值
    RealRefS.P = EnergyManage.P2out;//FuncPara.L2PAimUse
    RealRefS.QRef.TargetValue =EnergyManage.Q2out;//CorrectInActivePowerAim(SystemPara.ActivePowerSetS);  // 此处无功需要缓启防止小功率馈电时功率过冲
    RealRefS.Q = RmpCntlCalc(&RealRefS.QRef);

    AcdcMode();   //pwm rect

    // R 相计算视在功率限值
    RealRefR.S = __sqrt(RealRefR.P * RealRefR.P + RealRefR.Q * RealRefR.Q);
    RealRefR.S = __fmin(RealRefR.S, 2750.0f*ACTIVE_CORRECTION_FACTOR);   //

    // S 相视在功率限值
    RealRefS.S = __sqrt(RealRefS.P * RealRefS.P + RealRefS.Q * RealRefS.Q);
    RealRefS.S = __fmin(RealRefS.S, 2750.0f*ACTIVE_CORRECTION_FACTOR);   //

    // --------------------------计算 R 相电流参考-----------------------
    // 计算给定电流幅值
    temp = RealRefR.S/SampleRms.Inv1VoltRms*1.414f;
    temp = __fsat(temp, 39.0f, -39.0f);
    if (RealRefR.P >= 0)
    {
        RealRefR.IRef = temp;

//        if (RealRefR.P >= 1500.0f)
//        {
//            phaseCmp = 0.0628f;         // 补偿2个点角度2/167*2*PI=0.0628=11/180
//        }
//        else
//        {
//            phaseCmp = 0;
//        }
    }
    else
    {
        RealRefR.IRef = -temp;
        //phaseCmp =0.0314f;//-0.0314f;
    }

    // 正常电流变化30A/s优先级最低;电网增强10A/s优先级低
    // 柴油发电机模式下1A/s,优先级中
    // 电网增强模式使能
    if (SystemPara.GridEnhanceEn)
    {
        GridEnhanceMode(1);         // R 相电网增强
    }
    else if((fabs(IrCtrl.RmpCtrl.TargetValue) < fabs(IrCtrl.RmpCtrl.SetpointValue))
            ||(fabs(IsCtrl.RmpCtrl.TargetValue) < fabs(IsCtrl.RmpCtrl.SetpointValue)))
    {
        SetCurrStepValue(200.0f);
        if((fabs(IrCtrl.RmpCtrl.TargetValue) < (fabs(IrCtrl.RmpCtrl.SetpointValue)-3))
                ||(fabs(IsCtrl.RmpCtrl.TargetValue) < (fabs(IsCtrl.RmpCtrl.SetpointValue)-3)))
        {
            SystemPara.DynamicStageCnt = 210;
        }
    }
    else
    {
        if((IrCtrl.RmpCtrl.TargetValue + IsCtrl.RmpCtrl.TargetValue) < 0
        && (IrCtrl.RmpCtrl.SetpointValue + IsCtrl.RmpCtrl.SetpointValue) < 0)
        {
            SetCurrStepValue(3.0f); // 正常模式下电流变化 30A/s
        }
        else
        {
            SetCurrStepValue(30.0f); // 正常模式下电流变化 30A/s
        }
    }

    IrCtrl.RmpCtrl.TargetValue = RealRefR.IRef;

    // 电流的缓起已经在电流环中处理,此处只需要将角度做缓慢变化
    RealRefR.DeltaTheta.TargetValue = __atan2(RealRefR.Q/RealRefR.P, 1.0f); // 取值范围[-0.5*pi,0.5*pi]
    if (RealRefR.QRef.TargetValue == 0)
    {
        RealRefR.DeltaTheta.TargetValue = 0;                        // 如果无功给定为 0,不需要角度偏移,角度偏移量为0
    }
    RmpCntlCalc(&RealRefR.DeltaTheta);                              // 角度缓起

//    // 不做无功补偿
    // 小功率馈电无功补偿
    theta = IrCtrl.AFDTheta  + RealRefR.DeltaTheta.SetpointValue;//+ phaseCmp 
    RealRefR.Sin = __sin(theta);
    RealRefR.ThetaPoint = InvPhase.ThetaPointR + RealRefR.DeltaTheta.SetpointValue/InvPhase.DeltR;

    if (RealRefR.ThetaPoint >= ((int16_t)InvPhase.PeriodPoint))
    {
        RealRefR.ThetaPoint -= InvPhase.PeriodPoint;
    }
    else if (RealRefR.ThetaPoint < 0)
    {
        RealRefR.ThetaPoint += InvPhase.PeriodPoint;
    }

    // --------------------------计算 S 相电流参考-----------------------------
    // 给定电流幅值
    temp = RealRefS.S/SampleRms.Inv2VoltRms*1.414f;
    temp = __fsat(temp, 39.0f, -39.0f);
    if (RealRefS.P >= 0)
    {
        RealRefS.IRef = temp;

//        if (RealRefS.P >= 1500.0f)
//        {
//            phaseCmp = 0.0628f;         // 补偿2个点角度2/167*2*PI=0.0628=11/180
//        }
//        else
//        {
//            phaseCmp = 0;
//        }
    }
    else
    {
        RealRefS.IRef = -temp;
      //  phaseCmp = 0.0314f;//-0.0314f;
    }

    // 电网增强模式使能
    if (SystemPara.GridEnhanceEn)
    {
        GridEnhanceMode(2);
    }
    IsCtrl.RmpCtrl.TargetValue = RealRefS.IRef;

    // 电流的缓起已经在电流环中处理,此处只需要将角度做缓慢变化
    RealRefS.DeltaTheta.TargetValue = __atan2(RealRefS.Q/RealRefS.P, 1.0f); // 取值范围[-0.5*pi,0.5*pi]
    if (RealRefS.QRef.TargetValue == 0)
    {
        RealRefS.DeltaTheta.TargetValue = 0;
    }
    RmpCntlCalc(&RealRefS.DeltaTheta);

    // 馈电小功率无功补偿
    theta = IsCtrl.AFDTheta+ RealRefS.DeltaTheta.SetpointValue ;//+ phaseCmp
    RealRefS.Sin = __sin(theta);
    RealRefS.ThetaPoint = InvPhase.ThetaPointS+ RealRefS.DeltaTheta.SetpointValue/InvPhase.DeltS;

    if (RealRefS.ThetaPoint > ((int16_t)InvPhase.PeriodPoint))
    {
        RealRefS.ThetaPoint -= InvPhase.PeriodPoint;
    }
    else if (RealRefS.ThetaPoint < 0)
    {
        RealRefS.ThetaPoint += InvPhase.PeriodPoint;
    }

    //
    GridEnhanceCHK();

    if(SystemPara.DynamicStageCnt != 0)
    {
        SystemPara.DynamicStageCnt--;
        IrCtrl.ReEnable = 0;
        IsCtrl.ReEnable = 0;
        IrCtrl.RepCtrl.Data[SystemPara.DynamicStageCnt] = 0;
        IsCtrl.RepCtrl.Data[SystemPara.DynamicStageCnt] = 0;
    }
    else
    {
        IrCtrl.ReEnable = 1;
        IsCtrl.ReEnable = 1;
    }

    if (ArmWriteData.Model.bit.DieselMode || ArmWriteData.AgingLable)
    {
        SetCurrStepValue(1.0f);
    }
    // 电池低压快速降功率,优先级高
    if (InvError.BATLowLimit == 1)
    {
        SetCurrStepValue(53.0f);
    }
    // 电网端口过流,快速降功率优先级最高
    if ((GridError.Grid1Err.bit.GridOC == 1) || (GridError.Grid2Err.bit.GridOC == 1))
    {
        SetCurrStepValue(30.0f);
    }



}
/*************************************************
Function: GetReaptePara
Description: 计算电流环重复控制系数,当电流大的时候减小重复控制系数
// 根据并网电流大小以及当前角度值,调整重复控制 kp 系数
Input: @theta-锁相角度,@curr-当前电流目标大小,@-set-kp设定值;
@zKp-零点附近kp参数系数
Output: None
Frequency: 10KHz
*************************************************/
float GetReaptePara(float curr, float set)
{
    float kp = 0;

    if (curr > 45.0f)
    {
        kp = set*0.35f;
    }
    else if (curr > 30.0f)
    {
        kp = set*0.6f;
    }
    else if (curr > 20.0f)
    {
        kp = -0.0002f*curr + 0.009f;
    }
    else
    {
        kp = set;
    }

    return kp;
}
/*************************************************
Function: GetPIKpPara
Description: 计算电流环PI控制系数,当电流大的时候减小KP控制系数
Input: @curr-当前电流目标大小,@-set-kp设定值;
Output: None
Frequency: 10KHz
*************************************************/
float GetPIKpPara(float curr, float set)
{
    float kp = 0;

    if (curr > 45.0f)
    {
        kp = set*0.20f;
    }
    else if (curr > 30.0f)
    {
        kp = set*0.3f;
    }
    else if (curr > 20.0f)
    {
        kp = -0.0007f*curr + 0.024f;
    }
    else
    {
        kp = set;
    }

    return kp;
}
/*************************************************
Function: ControlOnGridInv
Description: 双路逆变并网控制主函数
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(ControlOnGridInvR, ".TI.ramfunc");
int ControlOnGridInvR(uint16_t enFlg)
{
	 // 计算电流目标值
	    CaculateCurrentRef();

	if (GridError.Grid1Err.bit.GridOCP == 1||GridError.Grid1Err.bit.Grid1Drop1 == 1
		||GridError.Grid2Err.bit.Grid2Drop1 == 1||Grid1EnhanceDrop == 1
		||Grid2EnhanceDrop == 1)
		{
	         GridPWMflg= 3;
		     DISABLE_INV1PWM;
		     return 1;
		}
    // 直流分量发生改变
    if (ArmWriteData.DCChangeFlg == 1)
    {
        ArmWriteData.DCChangeFlg = 0;
    }
    else
    {
        IrCtrl.IdcCurr = 0;
        IsCtrl.IdcCurr = 0;
    }

    if ((SystemPara.DieselEngineMode == 0) && (fabs(SampleIns.Inv1CurrIns) >= 66.0f))
    {
       // GridError.Grid1Err.bit.GridDrop = 1;
       // InvStateData.GridErrTZCnt = 0;
       // DISABLE_INV1PWM;
    }

    // R相---------------如果当前功率目标为0则关闭逆变驱动
    if ((SystemPara.ActivePowerSetR == 0) && (fabs(IrCtrl.RmpCtrl.SetpointValue) <= 4.3f))
    {
        //整流模式下不关闭
        return 1;
    }
    else
    {
        IrCtrl.BusVolt =  SampleIns.BUSVoltIns*0.5f;
        IrCtrl.OutLimit =  __fsat((__fmax(SampleRms.Grid1VoltRms,95) + 20)*(1.414f),IrCtrl.BusVolt-1,IrCtrl.BusVolt-100);
        if(UpsVoltChk(&IrCtrl))
        {
            return 1;
        }
        if (enFlg == 0)
        {
            return 1;
        }

        if (EPwm1Regs.AQCSFRC.bit.CSFA == 0)
		{
		DCIAdjustR();
		OnGridParaR();
		// --------------------------R��------------------------------------
//		IrCtrl.IdcSum += IrCtrl.IdcCurr;
//		IrCtrl.IdcSum = __fsat(IrCtrl.IdcSum, 2.0f, -2.0f);
//		IrCtrl.DCIAdj  = SampleRms.DCCurr1Rms*0.003f;
//		IrCtrl.DCIAdj =__fsat(IrCtrl.DCIAdj, 2.0f, -2.0f);
		IrCtrl.SinTheta = RealRefR.Sin;
		IrCtrl.RepCtrl.PeriodPoint = (SystemPara.FrequencySet == 1) ? 167:200;
		IrCtrl.RepCtrl.OSTheta = RealRefR.ThetaPoint;
		IrCtrl.RepCtrl.OSTheta = (int16_t)__fsat((float)IrCtrl.RepCtrl.OSTheta, 225.0f, 0);
		IrCtrl.PICtrl.Fbk = SampleIns.Inv1CurrIns+ IrCtrl.DCIAdj;

//        IrCtrl.Fw = IrCtrl.Vgff.y[0];//SampleIns.Grid1VoltIns;//

        IrCtrl.Fw = IrCtrl.Usogi.osg_u[0] - IrCtrl.Usogi.osg_qu[0]*0.06f;  //
        IrCtrl.Vgrid = SampleIns.Grid1VoltIns;
        if(fabsf(SampleIns.Inv1CurrIns) > 50 && fabsf(SampleIns.Grid1VoltIns)<fabsf(IrCtrl.Fw))
            IrCtrl.Fw = SampleIns.Grid1VoltIns;

//		IrCtrl.BusVolt =  SampleIns.Bus1VoltIns;

		IrCtrl.RepCtrl.kp = IrCtrl.RepCtrl.kpSet;//GetReaptePara(fabs(IrCtrl.RmpCtrl.SetpointValue),IrCtrl.RepCtrl.kpSet);
		IrCtrl.PICtrl.Kp = IrCtrl.IKpSet;

		// R �ಢ������
		ControlSingleOnGridInv(&IrCtrl);
		}
    }

    return 0;
}
/*
 * 如果 enFlg 为 0 此时驱动是关闭,闭环控制不需要执行
 *
 * */
#pragma CODE_SECTION(ControlOnGridInvS, ".TI.ramfunc");
int ControlOnGridInvS(uint16_t enFlg)
{
	if (GridError.Grid1Err.bit.GridOCP == 1||GridError.Grid1Err.bit.Grid1Drop1 == 1
		||GridError.Grid2Err.bit.Grid2Drop1 == 1||Grid1EnhanceDrop == 1
		||Grid2EnhanceDrop == 1)
		{
	        GridPWMflg= 3;
		    DISABLE_INV2PWM;
		    return 1;
		}
    if ((SystemPara.DieselEngineMode == 0) && (fabs(SampleIns.Inv2CurrIns) >= 66.0f))
    {
        //GridError.Grid2Err.bit.GridDrop = 1;
        //InvStateData.GridErrTZCnt = 0;
        //DISABLE_INV2PWM;
    }
    // S相---------------如果当前功率目标为0则关闭逆变驱动
    if ((SystemPara.ActivePowerSetS == 0) && (fabs(IsCtrl.RmpCtrl.SetpointValue) <= 4.3f))
    {
//        if(ArmWriteData.EmgChagEn == 0)  //紧急充电不封波？
        {
            GridPWMflg= 4;
            return 1;
        }
    }
    else
    {
        IsCtrl.BusVolt =  SampleIns.BUSVoltIns*0.5f;
        IsCtrl.OutLimit =  __fsat((__fmax(SampleRms.Grid2VoltRms,95) + 20)*1.414f,IsCtrl.BusVolt-1,IsCtrl.BusVolt-100);
        if(UpsVoltChk(&IsCtrl))
        {
            GridPWMflg= 1;
            return 1;
        }
        if (enFlg == 0)
        {
//            GridPWMflg= 2;
            return 1;
        }

        if(EPwm2Regs.AQCSFRC.bit.CSFA == 0)
	   {
	  	   DCIAdjustS();
		   OnGridParaS();
		   // -------------------------S��-------------------------------------
//		   IsCtrl.IdcSum += IsCtrl.IdcCurr;
//		   IsCtrl.IdcSum = __fsat(IsCtrl.IdcSum, 2.0f, -2.0f);
//		   IsCtrl.DCIAdj  = SampleRms.DCCurr2Rms*0.003f;//-DCIAdjOut *0.001f;
//		   IsCtrl.DCIAdj = __fsat(IsCtrl.DCIAdj, 2.0f, -2.0f);
		   IsCtrl.SinTheta = RealRefS.Sin;
		   IsCtrl.RepCtrl.PeriodPoint = (SystemPara.FrequencySet == 1) ? 167:200;
		   IsCtrl.RepCtrl.OSTheta = RealRefR.ThetaPoint;
		   IsCtrl.RepCtrl.OSTheta = (int16_t)__fsat((float)IsCtrl.RepCtrl.OSTheta, 225.0f, 0);
		   IsCtrl.PICtrl.Fbk = SampleIns.Inv2CurrIns + IsCtrl.DCIAdj;

   //        IsCtrl.Fw = IsCtrl.Vgff.y[0];//SampleIns.Grid2VoltIns;//

           IsCtrl.Fw = IsCtrl.Usogi.osg_u[0] - IsCtrl.Usogi.osg_qu[0]*0.06f; //
           IsCtrl.Vgrid = SampleIns.Grid2VoltIns;
	        if(fabsf(SampleIns.Inv2CurrIns) > 50 && fabsf(SampleIns.Grid2VoltIns)<fabsf(IsCtrl.Fw))
	            IsCtrl.Fw = SampleIns.Grid2VoltIns;

//		   IsCtrl.BusVolt = SampleIns.Bus1VoltIns;

		   IsCtrl.RepCtrl.kp = IsCtrl.RepCtrl.kpSet;//GetReaptePara(fabs(IsCtrl.RmpCtrl.SetpointValue), IsCtrl.RepCtrl.kpSet);
		   IsCtrl.PICtrl.Kp = IsCtrl.IKpSet;//GetPIKpPara(fabs(IsCtrl.RmpCtrl.SetpointValue), IsCtrl.IKpSet);

		   // S �ಢ������
		   ControlSingleOnGridInv(&IsCtrl);

	   }
    }

    return 0;
}


char UpsVoltChk(ON_GRID_CONTROL_COEFF *v)
{
    if(ArmWriteData.AgingLable)
    {
        return 0;
    }

    if((fabsf(v->Out) * v->BusVolt > v->OutLimit && v->RmpCtrl.SetpointValue > 0 && fabsf(v->Out) > fabsf(v->Vgrid) && fabsf(v->PICtrl.Fbk) < 5) //
        ||(fabsf(v->PRVars.Out + v->PICtrl.Out) > 0.25f && fabsf(v->PICtrl.Fbk) < 5))
//     ||(fabsf(v->PRVars.Out) > 0.25f && (v->RmpCtrl.TargetValue == v->RmpCtrl.SetpointValue)) )    //+ v->PICtrl.Out
//     ||CaptureData.FreqR > (Protect.Value.GridOF1 + 5)
    {
//        v->RmpCtrl.SetpointValue = 0;
//        v->Out = 1;
//////        PWMRectifyData.PWMRectifyEnable = 0;
////
//        if( ++v->OutOVcnt > 1000)
//        {
//            v->Out = 0;
//            v->OutOVcnt = 1000;
//        }

        v->Out = 0;
        return 1;
    }
    else
    {
        v->OutOVcnt = 0;
    }
    return 0;
}
void DCIAdjustR(void)
{
    //float DCIErr;
   // static float DCIErrOld;
   // static float DCIAdjOut = 0;

    IrCtrl.DCIErr = 0 - SampleRms.DCCurr1Rms;
    IrCtrl.DCIAdjOut = (IrCtrl.DCIErr - IrCtrl.DCIErrOld) *0.005   + IrCtrl.DCIErr * 0.001  + IrCtrl.DCIAdjOut;
    IrCtrl.DCIErrOld = IrCtrl.DCIErr;
    IrCtrl.DCIAdj  = -IrCtrl.DCIAdjOut *0.001f;
    IrCtrl.DCIAdj =__fsat(IrCtrl.DCIAdj, 3.0f, -3.0f);

 }
void DCIAdjustS(void)
{
   // float DCIErr;
  //  static float DCIErrOld;
  //  static float DCIAdjOut = 0;

    IsCtrl.DCIErr = 0 - SampleRms.DCCurr2Rms;
    IsCtrl.DCIAdjOut = (IsCtrl.DCIErr - IsCtrl.DCIErrOld) *0.005   + IsCtrl.DCIErr * 0.001  + IsCtrl.DCIAdjOut;
    IsCtrl.DCIErrOld = IsCtrl.DCIErr;
    IsCtrl.DCIAdj  = -IsCtrl.DCIAdjOut *0.001f;
    IsCtrl.DCIAdj = __fsat(IsCtrl.DCIAdj, 3.0f, -3.0f);
}


typedef struct
{
    PIDCntl_F BusVoltCntl;

    float Vbus_ref;
    float Vbus_max;
    float P1_hat;
    float P2_hat;
    float PPV_max;
} ACDC_STRUCT;
ACDC_STRUCT Acdc;

void InitAcdcMode(void)
{
    Acdc.Vbus_ref = 400;
    Acdc.Vbus_max = 430;
    Init_PI_Controller(&Acdc.BusVoltCntl, 20 ,0.1, 5500, -5500);
}

void AcdcMode(void)
{

    /*  暂不需要
    //优化THd,过零点更新
//    if(InvPhase.ThetaPointR == 1)
//    {
//
//    }
    if(ArmWriteData.EmgChagEn) //整流模式//加热直流电源    ,SOC = 0
    {
        mRefRamp(Acdc.BusVoltCntl.Ref, Acdc.Vbus_ref, (100.0f/PWM_ISR_FREQUENCY));    //100V/s ramp
        if(Acdc.BusVoltCntl.Ref < __fmin(SampleIns.BUSVoltIns - 10, Acdc.Vbus_ref))   //init min ref
            Acdc.BusVoltCntl.Ref = __fmin(SampleIns.BUSVoltIns - 10, Acdc.Vbus_ref);
        Acdc.BusVoltCntl.xp[0] = -(Acdc.BusVoltCntl.Ref - SampleIns.BUSVoltIns);
        Acdc.BusVoltCntl.max = __fmax(EnergyManage.P1out + EnergyManage.P2out,0);     //P discharge limit
        Acdc.BusVoltCntl.min = __fmin(EnergyManage.P1out + EnergyManage.P2out,0);     //
        PI_Controller(&Acdc.BusVoltCntl);

        //Vpv > Vbus 限放电功率
//        if(SampleRms.PV1VoltFilter8 >  Acdc.Vbus_ref
//         ||SampleRms.PV2VoltFilter8 >  Acdc.Vbus_ref
//         ||SampleRms.PV3VoltFilter8 >  Acdc.Vbus_ref
//         ||SampleRms.PV4VoltFilter8 >  Acdc.Vbus_ref)
//        {
//            Acdc.P1_hat = __fmin(Acdc.BusVoltCntl.yp[0]*0.5,EnergyManage.P1out);
//            Acdc.P2_hat = __fmin(Acdc.BusVoltCntl.yp[0]*0.5,EnergyManage.P2out);
//        }
//        else
//        {
//            Acdc.P1_hat = __fmin(Acdc.BusVoltCntl.yp[0]*0.5,EnergyManage.P1out);
//            Acdc.P2_hat = __fmin(Acdc.BusVoltCntl.yp[0]*0.5,EnergyManage.P2out);
//        }

        //limit Ac Power
        if(Acdc.BusVoltCntl.yp[0] > 0)
        {
            Acdc.P1_hat = __fmin(Acdc.BusVoltCntl.yp[0],__fmax(EnergyManage.P1out,0));  //
            Acdc.P2_hat = __fmin(Acdc.BusVoltCntl.yp[0] - Acdc.P1_hat,__fmax(EnergyManage.P2out,0)); //
        }
        else
        {
            Acdc.P1_hat = __fmax(Acdc.BusVoltCntl.yp[0],__fmin(EnergyManage.P1out,0));
            Acdc.P2_hat = __fmax(Acdc.BusVoltCntl.yp[0] - Acdc.P1_hat,__fmin(EnergyManage.P2out,0));
        }
        //Pv Power limited for Vbus self used mode
//        if(ArmWriteData.BMSChgMaxCurrSet == 0)
//        {
//            Acdc.PPV_max = __fmax(Acdc.BusVoltCntl.yp[0],0);
//        }

    }
//    else if(ArmWriteData.BMSChgMaxCurrSet == 0)//SOC >= 99,非充非放时,输出功率限值
//    {
//        //多路PV最大功率点不一致，如果输出限流会导致无法每路都追到最大功率点
//          //，动态检测每一路PV的功率 ，按实际功率分配限流值后，都能追到最大功率点。
//        Acdc.BusVoltCntl.Ref = Acdc.Vbus_ref;
//        Acdc.BusVoltCntl.xp[0] = -(Acdc.BusVoltCntl.Ref - SampleIns.BUSVoltIns);
//        PI_Controller(&Acdc.BusVoltCntl);
//        Acdc.P1_hat = EnergyManage.P1out + 0.5*Acdc.BusVoltCntl.yp[0];
//        Acdc.P2_hat = EnergyManage.P2out + 0.5*Acdc.BusVoltCntl.yp[0];
//    }
    else  //待机
    {
        Acdc.BusVoltCntl.Ref = SampleIns.BUSVoltIns;
        Acdc.BusVoltCntl.yp[1] = 0;
        Acdc.BusVoltCntl.xp[1] = 0;
        Acdc.P1_hat = 0;
        Acdc.P2_hat = 0;
    }

    if(ArmWriteData.EmgChagEn)  //|| (ArmWriteData.BMSChgMaxPowerSet == 0)
    {
        RealRefR.P = Acdc.P1_hat;//
        RealRefS.P = Acdc.P2_hat;//
    }
*/
    if(SampleIns.BUSVoltIns>465)
    {
        if(RealRefR.P < 0)
            RealRefR.P = 0;
        if(RealRefS.P < 0)
            RealRefS.P = 0;
    }
}

#pragma CODE_SECTION(InvPwmReloadOnGrid, ".TI.ramfunc");
void InvPwmReloadOnGrid(void)
{
    if(IrCtrl.PWMOn)
    {
        IrCtrl.PWMOn = 0;
        SampleADRaw.Inv1CurrRaw = AdcdResultRegs.ADCRESULT6;
        SampleIns.Inv1CurrIns = ((int32_t)SampleCalibCoef.Inv1CurrK*(SampleADRaw.Inv1CurrRaw - SampleZeroCalibrate.Inv1CurrZero)>>12)*0.1f;
        IrCtrl.PICtrl.Fbk = SampleIns.Inv1CurrIns+ IrCtrl.DCIAdj;
        CNTL_PI_F_FUNC(&IrCtrl.PICtrl);
        IrCtrl.Out =  IrCtrl.Fw + (IrCtrl.PICtrl.Out + IrCtrl.RepCtrl.CfOut + IrCtrl.PRVars.Out + IrCtrl.PR3Vars.Out + IrCtrl.PR5Vars.Out + IrCtrl.PR7Vars.Out)* IrCtrl.gain; //
        IrCtrl.Out = __fsat(IrCtrl.Out, 1.0f, -1.0f);             // 限制占空比
        IrCtrl.PWMDuty = (int16_t)(IrCtrl.Out * IrCtrl.Period);
        UpdateINVPwmComp3( IrCtrl.Id,  IrCtrl.PWMDuty,  IrCtrl.Period, 0);
    }

    if(IsCtrl.PWMOn)
    {
        IsCtrl.PWMOn = 0;
        SampleADRaw.Inv2CurrRaw = AdcdResultRegs.ADCRESULT2;
        SampleIns.Inv2CurrIns = ((int32_t)SampleCalibCoef.Inv2CurrK*(SampleADRaw.Inv2CurrRaw - SampleZeroCalibrate.Inv2CurrZero)>>12)*0.1f;
        IsCtrl.PICtrl.Fbk = SampleIns.Inv2CurrIns+ IsCtrl.DCIAdj;
        CNTL_PI_F_FUNC(&IsCtrl.PICtrl);
        IsCtrl.Out =  IsCtrl.Fw + (IsCtrl.PICtrl.Out + IsCtrl.RepCtrl.CfOut + IsCtrl.PRVars.Out + IsCtrl.PR3Vars.Out + IsCtrl.PR5Vars.Out + IsCtrl.PR7Vars.Out)* IsCtrl.gain; //
        IsCtrl.Out = __fsat(IsCtrl.Out, 1.0f, -1.0f);             // 限制占空比
        IsCtrl.PWMDuty = (int16_t)(IsCtrl.Out * IsCtrl.Period);
        UpdateINVPwmComp3( IsCtrl.Id,  IsCtrl.PWMDuty,  IsCtrl.Period, 0);
    }
}

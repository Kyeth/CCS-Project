#include <string.h>
#include "bsp.h"
#include "rmp.h"
#include "MPPTFunc.h"
#include "Sample.h"
#include "Protect.h"
#include "PVStateMachine.h"
#include "ConfigSystemPara.h"
#include "ModbusTable.h"
#include "Function.h"
#include "PowerCalculator.h"
#define ARM_FAULT_BITS      (0x1221)        // ARM故障使能位0X3A1->0X221->0x1221 for PV
#define ARM_FAULT_INV_BITS  (0x221)         // ARM故障使能位0X3A1->0X221 for inv

#define VPVmax  (450)
#define IPVmax  (16.2f)
//#define IPVmax  (2520.0f/156.25f)

void EnablePVPwm(uint16_t channel);
void DisablePVPwm(uint16_t channel);

PV_STATE_DATA PV1WorkState, PV2WorkState, PV3WorkState, PV4WorkState;
PV_SYSTEM_DATA PVSystemData;

void InitPVStateMachineData(void)
{
    memset(&PV1WorkState, 0, sizeof(PV_STATE_DATA));
    // 最大扰动步长
    PV1WorkState.Mppt.cMpptStepMax = 5;     // 7V->3V
    // 最小扰动步长
    PV1WorkState.Mppt.cMpptStepMin = 1;     // 2V
    // 初始扰动步长
    PV1WorkState.Mppt.DisturbStep = PV1WorkState.Mppt.cMpptStepMax;
    // 扰动周期
    PV1WorkState.Mppt.DisturbPeriod = (30000/4);  // 1s 10000->30000
    PV1WorkState.Mppt.DisturbHalfPeriod = PV1WorkState.Mppt.DisturbPeriod/2; //实测500ms可内扰动完成
    // MPPT电压范围80V~VPVmaxV,最大扰动输出电压
    PV1WorkState.Mppt.MpptOutMax = VPVmax;
    // 最小扰动输出电压
    PV1WorkState.Mppt.MpptOutMin = 79;
    // 初始扰动步长
    PV1WorkState.Mppt.StepVolt = 0.6;     // 4->2
//    PV1WorkState.Mppt.BusVolt = &SampleIns.BUSVoltIns;
    PV1WorkState.Mppt.BusVolt = &SampleRms.BUSVoltFilter200;

    // 快速扰动电压步长 暂定10V
//    PV1WorkState.Mppt.FastStep = 3;
    // 电流环参数设定   ep6k 0.0009, 0.1
    PV1WorkState.CurrGain = 0.0009;
    PV1WorkState.CurrPIF.Kp = 0.0009;     //0.003 ->
    PV1WorkState.CurrPIF.Ki = 0.04;       //0.009 ->
    PV1WorkState.CurrPIF.Umax = 0.1;  //限最大占空比 1 - (PV1WorkState.Mppt.MpptOutMin/PV1WorkState.Mppt.MpptOutMax)
    PV1WorkState.CurrPIF.Umin = -0.1;
    // 电压环参数设定
    PV1WorkState.Vff = 0.1;
//    PV1WorkState.VoltGain = 1;
    PV1WorkState.VoltPIF.Kp = 0.05;       //@sjj 0.05->1.5 ep6k电压环   1.7, (0.009)
    PV1WorkState.VoltPIF.Ki = 0.00005;      //
    PV1WorkState.VoltPIF.Umax = IPVmax;       // 最大电流16A
    PV1WorkState.VoltPIF.Umin = 0;
    // MPPT 电压参考设置(80V~VPVmaxV)
    PV1WorkState.VoltSet.TargetValue = 0;
    PV1WorkState.VoltSet.RampHighLimit = VPVmax;
    PV1WorkState.VoltSet.RampLowLimit = 79;
    PV1WorkState.VoltSet.StepValue = 600.0f/(PV_PWM_FREQ_KHZ*1000.0); //200
    // MPPT 电流参考设置
    PV1WorkState.CurrSet.TargetValue = 0;
//    PV1WorkState.CurrSet.StepValue = 0;
    PV1WorkState.CurrSet.RampHighLimit = 30;//16.1f;
    PV1WorkState.CurrSet.RampLowLimit = -3;
    PV1WorkState.CurrSet.StepValue = 5.0f/(PV_PWM_FREQ_KHZ*1000.0*0.5); //100A/S?

    PV1WorkState.Period = 15000;       // 1.5s clear fault
    PV2WorkState = PV1WorkState;
    PV3WorkState = PV1WorkState;
    PV4WorkState = PV1WorkState;

    PV1WorkState.PVId = 1;
    PV2WorkState.PVId = 2;
    PV3WorkState.PVId = 3;
    PV4WorkState.PVId = 4;

    memset(&PVSystemData, 0, sizeof(PV_SYSTEM_DATA));

//    pv1fault.bit.UV = 1;
//    pv2fault.bit.UV = 1;
//    pv3fault.bit.UV = 1;
//    pv4fault.bit.UV = 1;

    PVSystemData.PV1PowerAim = 2500;
    PVSystemData.PV2PowerAim = 2500;
    PVSystemData.PV3PowerAim = 2500;
    PVSystemData.PV4PowerAim = 2500;
//    PVSystemData.PVParallel = 15;        // 机壳丝印的PV2对应软件的PV1~PV2,丝印的PV1对应软件的PV3
    PVSystemData.PVSourceType = PVBoard;

    PV1WorkState.PVCurr = &SampleIns.PV1CurrIns;
    PV2WorkState.PVCurr = &SampleIns.PV2CurrIns;
    PV3WorkState.PVCurr = &SampleIns.PV3CurrIns;
    PV4WorkState.PVCurr = &SampleIns.PV4CurrIns;
    PV1WorkState.PVVolt = &SampleIns.PV1VoltIns;
    PV2WorkState.PVVolt = &SampleIns.PV2VoltIns;
    PV3WorkState.PVVolt = &SampleIns.PV3VoltIns;
    PV4WorkState.PVVolt = &SampleIns.PV4VoltIns;
    PV1WorkState.DCVolt = &SampleDisplayData.PV1Volt;
    PV2WorkState.DCVolt = &SampleDisplayData.PV2Volt;
    PV3WorkState.DCVolt = &SampleDisplayData.PV3Volt;
    PV4WorkState.DCVolt = &SampleDisplayData.PV4Volt;

    PV1WorkState.Mppt.PVVoltAvg = &SampleRms.PV1VoltFilter8;
    PV2WorkState.Mppt.PVVoltAvg = &SampleRms.PV2VoltFilter8;
    PV3WorkState.Mppt.PVVoltAvg = &SampleRms.PV3VoltFilter8;
    PV4WorkState.Mppt.PVVoltAvg = &SampleRms.PV4VoltFilter8;
    PV1WorkState.Mppt.PVCurrentAVg = &SampleRms.PV1CurrFilter8;
    PV2WorkState.Mppt.PVCurrentAVg = &SampleRms.PV2CurrFilter8;
    PV3WorkState.Mppt.PVCurrentAVg = &SampleRms.PV3CurrFilter8;
    PV4WorkState.Mppt.PVCurrentAVg = &SampleRms.PV4CurrFilter8;

    PV1WorkState.Mppt.ID = PV1WorkState.PVId;
    PV2WorkState.Mppt.ID = PV2WorkState.PVId;
    PV3WorkState.Mppt.ID = PV3WorkState.PVId;
    PV4WorkState.Mppt.ID = PV4WorkState.PVId;

}

/*************************************************
Function: UpdateMPPTVoltLimit
Description: 更新MPPT上限值,日规机器450V,美规机器500V
Input: None
Output: None
Frequency:
*************************************************/
//void UpdateMPPTVoltLimit(uint16_t id)
//{
//    // 日规机器
//    if (id == 1)
//    {
//        PV1WorkState.Mppt.MpptOutMax = VPVmax;
//        PV2WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//        PV3WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//        PV4WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//
//        PV1WorkState.VoltSet.RampHighLimit = VPVmax;
//        PV2WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//        PV3WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//        PV4WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//    }
//    else if (id == 0)
//    {
//        PV1WorkState.Mppt.MpptOutMax = VPVmax;
//        PV2WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//        PV3WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//        PV4WorkState.Mppt.MpptOutMax = PV1WorkState.Mppt.MpptOutMax;
//
//        PV1WorkState.VoltSet.RampHighLimit = VPVmax;
//        PV2WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//        PV3WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//        PV4WorkState.VoltSet.RampHighLimit = PV1WorkState.VoltSet.RampHighLimit;
//    }
//}
//#include "Energy_Manage.h"
#include "OnGridInv2.h"

#define VpvRef 460
/******************************************************************************
******************************************************************************/
#pragma CODE_SECTION(PVControlModular, ".TI.ramfunc");
void PVControlModular(PV_STATE_DATA* pv)
{
    float Mtmp;
    // ---- 电压环
    if(PVSystemData.PVSourceType == PVBoard)
    {
        if(SampleIns.BUSVoltIns > VpvRef)
        {
            //fast bus OV1
            pv->Mppt.BusOVFlag = 1;
            //fast bus OV2
            pv->VoltPIF.Ki = 0.00005;      //
            pv->VoltPIF.Ref = (VpvRef*10);
            pv->VoltPIF.Fbk = SampleIns.BUSVoltIns*10;
//            pv->PVVoltLimit =*pv->Mppt.PVVoltAvg;
            pv->PVVoltLimit =pv->VoltSet.SetpointValue;

        }
        else
        {
//            pv->VoltPIF.Ref = *pv->PVVolt;
            pv->VoltPIF.Ki = 0.005;      //
            pv->VoltPIF.Ref = *pv->Mppt.PVVoltAvg;
            pv->VoltPIF.Fbk = RmpCntlCalc(&pv->VoltSet);
            pv->PVVoltLimit = pv->VoltPIF.Fbk;
        }

        pv->M1 = __fmax(1 - pv->PVVoltLimit * PVSystemData.BusAdptCoeff,0);
        pv->M0 = pv->M1;
    }
    else
    {
        if(*pv->PVCurr < pv->VoltPIF.Umin)
        {
            pv->VoltPIF.Umin = *pv->PVCurr;
        }

        if(SampleIns.BUSVoltIns > 440)
        {
            pv->VoltPIF.Ref = (436*10);
            pv->VoltPIF.Fbk = SampleIns.BUSVoltIns*10;
        }
        else
        {
            pv->VoltPIF.Ref = RmpCntlCalc(&pv->VoltSet);
            pv->VoltPIF.Fbk = SampleIns.BUSVoltIns;
        }

//            pv->VoltPIF.Kp = PV1WorkState.VoltPIF.Kp;
//            pv->VoltPIF.Ki = PV1WorkState.VoltPIF.Ki;
//            pv->CurrPIF.Ki = PV1WorkState.CurrPIF.Ki;
        //        pv->PVVoltLimit =*pv->PVVolt;
        pv->PVVoltLimit =*pv->Mppt.PVVoltAvg;

        Mtmp = __fsat(PVSystemData.BusAdptCoeff, (1/380.0f), (1/420.0f));
        pv->M0 = __fmax(1 - pv->PVVoltLimit * 0.0025,0);

        if(ArmWriteData.Fault.bit.ATEMode)
        {
            pv->VoltPIF.Ki = 0.001;      //
            pv->M1 = pv->M0;
        }
        else
        {
            pv->VoltPIF.Ki = 0.00005;      //
            pv->M1 = __fmax(1 - pv->PVVoltLimit * Mtmp,0);
        }
    }

    // 并联状态下主机 限制电压环的输出-限制 Umax 的值,即限制电流目标值
    if (pv->Parallel == PV_MASTER)
    {
        pv->VoltPIF.Umax = pv->PVCurrLimit*PVSystemData.PVParallelWorkNum;
    }
    else
    {
        pv->VoltPIF.Umax =  pv->PVCurrLimit;
    }

    CNTL_PI_F_MACRO(pv->VoltPIF);

    // ---- 电流环
    if (PVSystemData.PVStart == 0)  // 关机
    {
        pv->CurrSet.SetpointValue = 0;
        pv->CurrSet.TargetValue = 0;
        pv->Duty -= 15;

        if(pv->Duty <= 1)
        {
            DisablePVPwm(pv->PVId);
            pv->Duty = 1;
            pv->State = SELF_STATE;
            pv->Count = 0;

        }
        return;
    }
    else
    {
        EnablePVPwm(pv->PVId);

        if (pv->Parallel == PV_FREE)
        {
            pv->CurrSet.TargetValue = pv->VoltPIF.Out;
        }
        // 并联主机
        else if (pv->Parallel == PV_MASTER)
        {
            PVSystemData.PVMasterCurrRef = pv->VoltPIF.Out;
            pv->CurrSet.TargetValue = PVSystemData.PVMasterCurrRef* PVSystemData.IPVParallelWorkNum;
        }
        // 并联从机
        else
        {
            pv->CurrSet.TargetValue = PVSystemData.PVMasterCurrRef* PVSystemData.IPVParallelWorkNum;
        }
    }

    if(pv->Run == 0)
    {
        pv->Run = 1;
        pv->CurrPIF.i1 = - pv->M0;
        pv->CurrPIF.Out = - pv->M0;
        pv->M = pv->M0;
    }

    //变参数 5(单路) -> 3.5(4路并联)
    if(pv->CurrPIF.Out < pv->M * -0.1 && pv->M > 0.1 && pv->CurrPIF.Ref < 3.5)
    {
        pv->CurrPIF.Kp = 0.003;
        pv->VoltPIF.Kp = 0.15;
        pv->M = __fsat(pv->M0,pv->M+0.01,pv->M-0.01);
    }
    else
    {
        pv->M = __fsat(pv->M1,pv->M +0.01,pv->M-0.01);
        pv->CurrPIF.Kp = (PV1WorkState.CurrGain* PVSystemData.IPVParallelWorkNum) * __fsat(1.5f - pv->CurrPIF.Ref * (1.2/IPVmax),1,0.5);
        pv->VoltPIF.Kp = 0.05;
    }
//    if(SampleIns.BUSVoltIns > VpvRef) //fast bus OV2
//    {
//        pv->CurrPIF.Kp = 0.002;
//    }

//    pv->CurrPIF.Ref = RmpCntlCalc(&pv->CurrSet); //慢增快降
//    if(pv->CurrPIF.Ref > pv->CurrSet.TargetValue || PVSystemData.PVSourceType == Vsource)   //
//    float tmp = __fsat(pv->CurrPIF.Ref*PV1WorkState.Vff,1,0); //源不稳定时取0.1

//    pv->M = tmp *__fmax(1 - pv->PVVoltLimit * PVSystemData.BusAdptCoeff,0);
    pv->CurrPIF.Ref = pv->CurrSet.TargetValue;
    pv->CurrPIF.Fbk = *pv->PVCurr;
    pv->CurrPIF.Umin = -0.1 - pv->M;

    //电流基准偏低无法到达0占空比，偏高时扰动失效。
//    pv->CurrPIF.Fbk = *pv->Mppt.PVCurrentAVg;
//    pv->CurrPIF.Fbk = __fmax(*pv->PVCurr,0);

    CNTL_PI_F_MACRO(pv->CurrPIF);

//    if (pv->CurrPIF.Out == pv->CurrPIF.Umin)
//        pv->Duty = 0;
//    else
//    if(PVSystemData.PVParallel != 0)
//        pv->Duty = pv->CurrPIF.Out*PV_PWM_PERIOD;
//    else

    pv->Duty = (pv->CurrPIF.Out + pv->M) * PV_PWM_PERIOD;//* 400 * PVSystemData.BusAdptCoeff

    pv->Duty = __fsat(pv->Duty,PVSystemData.BusDutyLimit,0);//bus OV3

//    pv->M = __fsat(pv->M,PVSystemData.BusDutyLimit,0);//bus OV3
}
/*************************************************
Function: MpptControlModular
Description: MPPT控制模块
Input: None
Output: None
Frequency: 10KHz
*************************************************/
//#pragma CODE_SECTION(MpptControlModular, ".TI.ramfunc");
inline void MpptControlModular(PV_STATE_DATA* pv)
{
    //PV功率接近限值时降低
///    if(SystemPara.SinglePVMaxPowerLimit )

    if(ArmWriteData.BMSSOC == 100)
    {
        pv->Mppt.cMpptStepMax = 1;
    }
    else if ( *pv->Mppt.PVVoltAvg <= 175.0f)
    {
        pv->Mppt.cMpptStepMax = 2;
//            pv->Mppt.StepVolt = 0.6f;
    }
    else if ( *pv->Mppt.PVVoltAvg <= 200.0f)
    {
        pv->Mppt.cMpptStepMax = 3;
//            pv->Mppt.StepVolt = 0.8f;
    }
    else
    {
        pv->Mppt.cMpptStepMax = 5;
//            pv->Mppt.StepVolt = 1.0f;
    }
    pv->Mppt.FastStep = pv->Mppt.cMpptStepMax;
    if (pv->Parallel != PV_SLAVE)
    {
        MpptFunc(&pv->Mppt);

        pv->VoltSet.TargetValue = pv->Mppt.Output;
    }
    if(pv->Parallel == PV_MASTER)
    {
        PVSystemData.PVMasterMPPTout = pv->Mppt.Output;
    }
    if(pv->Parallel == PV_SLAVE)
    {
        pv->VoltSet.TargetValue = PVSystemData.PVMasterMPPTout;
        pv->Mppt.Output = PVSystemData.PVMasterMPPTout;
    }
}
/******************************************************************************
******************************************************************************/
#pragma CODE_SECTION(PVState, ".TI.ramfunc");
void PVState(PV_STATE_DATA* pv)
{
    switch(pv->State)
    {
        // 初始化状态
        case INIT_STATE:
        {
            DisablePVPwm(pv->PVId);
            // 检查零飘检测是否结束
            if (SampleIns.CalSampleZeroFlag == 1 && FuncPara.ISOFinishFlag == 1)
            {
//                PV_TZ_Clear();
                pv->State = SELF_STATE;
            }
            break;
        }
        // 自检状态
        case SELF_STATE:
        {
            pv->Run = 0;
            pv->Mppt.MPPTEnable = 0;                              // MPPT 复位
            MpptControlModular(pv);

            // 添加禁止发波
            DisablePVPwm(pv->PVId);
            // 开机指令&&无故障&&开机电压满足要求&&电流限值大于 0
            if ((PVSystemData.PVStart == 1) && (pv->Error == 0) && (pv->Online  == 1))
            {
                if (++pv->Count >= pv->Period)
                {
                    pv->Count = 0;
                    // 清除电压环、电流环
                    pv->Duty = 1;                              // 清除电流环积分量
                    pv->VoltPIF.i1 = 0;                              // 清除电压环积分量
                    // 初始化电压环、电流环当前输出值
                    pv->VoltSet.SetpointValue = *pv->PVVolt - pv->Mppt.FastStep; // 当前 MPPT 电压缓起初值
                    pv->CurrSet.SetpointValue = 0;                   // 电流环限幅初值
                    pv->State = MPPT_STATE;                // 切入到 MPPT 状态
                }
            }
            else
            {
                pv->Count = 0;
            }

            // 有故障 进入到故障状态,屏蔽PV2/PV3
            if ((pv->Error != 0))
            {
                pv->Count = 0;
                pv->State = ERROR_STATE;
            }
            break;
        }
        // MPPT状态
        case MPPT_STATE:
        {
            pv->Mppt.MPPTEnable = 1;                // MPPT 禁用通过手动方式写入电流给定
            if (PVSystemData.PVSourceType == PVBoard)
            {
                MpptControlModular(pv);
            }

            PVControlModular(pv);

            // 有故障 进入到故障状态,屏蔽PV2/PV3
            if ((pv->Error != 0))
            {
                pv->Count = 0;

                DisablePVPwm(pv->PVId);

                pv->State = ERROR_STATE;
            }

            break;
        }
        // 故障状态
        case ERROR_STATE:
        {
            pv->Run = 0;
            DisablePVPwm(pv->PVId);

            // 有故障 进入到故障状态,屏蔽PV2/PV3
            if (pv->Error == 0)     // 无故障进入自检状态
            {
                if (++pv->Count >= pv->Period)    // 无故障3s后切入到自检状态
                {
                    pv->Count = 0;
                    pv->State = SELF_STATE;
                }
            }
            else
            {
                pv->Count = 0;
            }

            break;
        }
        default:
        {
            DISABLE_PV1PWM;
            DISABLE_PV2PWM;
            DISABLE_PV3PWM;
            DISABLE_PV4PWM;

            PV1WorkState.State = SELF_STATE;
            PV2WorkState.State = SELF_STATE;
            PV3WorkState.State = SELF_STATE;
            PV4WorkState.State = SELF_STATE;
            break;
        }
    }
}

/*************************************************
Function: CalculatePVCurrLimite
Description: 计算PV当前限流值-min(额定电流,BMS限流值,PV目标功率限流值)
Input: None
Output: None
Frequency: 10KHz
*************************************************/
//#pragma CODE_SECTION(CalculatePVCurrLimite, ".TI.ramfunc");

void CalculatePVCurrLimite(void)
{
    float PowerLimitTmp = 0;
    float PVSum = 0;
    float BMSLimitPVMaxPower = 0;

    int PVnumber = 0;
    float PVpower = 0;
    float PVpowerRes = 0;
    //PV最大功率限值
    SystemPara.SinglePVMaxPowerLimit = ArmWriteData.PVMaxPowerSet>>2;
    //额定功率
    PowerLimitTmp = __fmin(SystemPara.PVRatedPower,SystemPara.SinglePVMaxPowerLimit);
    //温度限功率
    PowerLimitTmp = __fmin(PowerLimitTmp,ArmWriteData.PVTempLimitPower);
    // PV功率设置
    if( PVSystemData.PVSourceType == Vsource)
    {
        ArmWriteData.PV1PowerAim = SystemPara.SinglePVMaxPowerLimit;
        ArmWriteData.PV2PowerAim = SystemPara.SinglePVMaxPowerLimit;
        ArmWriteData.PV3PowerAim = SystemPara.SinglePVMaxPowerLimit;
        ArmWriteData.PV4PowerAim = SystemPara.SinglePVMaxPowerLimit;
    }
    else
    {
        //BMS限值
        BMSLimitPVMaxPower = SystemPara.BMSChgMaxPowerSet + 50
                            + __fsat(RealRefR.P,100+SystemPower.Inv1.Average,SystemPower.Inv1.Average-100)
                            + __fsat(RealRefS.P,100+SystemPower.Inv2.Average,SystemPower.Inv2.Average-100);

        if(BMSLimitPVMaxPower<0)BMSLimitPVMaxPower =0;

        if(PVSystemData.PVParallel == 0)
        {
            PVSum = SystemPower.PV1.Average + SystemPower.PV2.Average +SystemPower.PV3.Average+ SystemPower.PV4.Average;

    //            if((PVSum > BMSLimitPVMaxPower+100)&&(PVSum!=0))
    //            {
            ArmWriteData.PV1PowerAim = BMSLimitPVMaxPower*(SystemPower.PV1.Average/PVSum); //
            ArmWriteData.PV2PowerAim = BMSLimitPVMaxPower*(SystemPower.PV2.Average/PVSum);
            ArmWriteData.PV3PowerAim = BMSLimitPVMaxPower*(SystemPower.PV3.Average/PVSum);
            ArmWriteData.PV4PowerAim = BMSLimitPVMaxPower*(SystemPower.PV4.Average/PVSum);
    //            }
    //            else if(PVSum < BMSLimitPVMaxPower-100)
    //            {
    //                ArmWriteData.PV1PowerAim = SystemPara.PVRatedPower;
    //                ArmWriteData.PV2PowerAim = SystemPara.PVRatedPower;
    //                ArmWriteData.PV3PowerAim = SystemPara.PVRatedPower;
    //                ArmWriteData.PV4PowerAim = SystemPara.PVRatedPower;
    //            }

            PVpowerRes = (BMSLimitPVMaxPower+200) - PVSum;

            if(PVpowerRes > 0)
            {
                ArmWriteData.PV1PowerAim += PVpowerRes*0.25;
                ArmWriteData.PV2PowerAim += PVpowerRes*0.25;
                ArmWriteData.PV3PowerAim += PVpowerRes*0.25;
                ArmWriteData.PV4PowerAim += PVpowerRes*0.25;

                //无法达到目标功率:1电压高灌进  2.限流  3.限功率
                if( ArmWriteData.PV1PowerAim >  PowerLimitTmp || PV1WorkState.State != MPPT_STATE)
                {
                    PVnumber++;
                }
                if( ArmWriteData.PV2PowerAim >  PowerLimitTmp || PV2WorkState.State != MPPT_STATE)
                {
                    PVnumber++;
                }
                if( ArmWriteData.PV3PowerAim >  PowerLimitTmp || PV3WorkState.State != MPPT_STATE)
                {
                    PVnumber++;
                }
                if( ArmWriteData.PV4PowerAim >  PowerLimitTmp || PV4WorkState.State != MPPT_STATE)
                {
                    PVnumber++;
                }

                if(PVnumber < 4)
                {
                    PVpower = (50*PVnumber)/(4-PVnumber);
                }

                ArmWriteData.PV1PowerAim += PVpower;
                ArmWriteData.PV2PowerAim += PVpower;
                ArmWriteData.PV3PowerAim += PVpower;
                ArmWriteData.PV4PowerAim += PVpower;
            }
        }
        else
        {
            ArmWriteData.PV1PowerAim  = BMSLimitPVMaxPower*PVSystemData.IPVParallelWorkNum+50;
            ArmWriteData.PV2PowerAim  = ArmWriteData.PV1PowerAim;
            ArmWriteData.PV3PowerAim  = ArmWriteData.PV1PowerAim;
            ArmWriteData.PV4PowerAim  = ArmWriteData.PV1PowerAim;
        }
    }

    PVSystemData.PV1PowerAim = __fmin(ArmWriteData.PV1PowerAim,PowerLimitTmp);//下发目标功率
    PVSystemData.PV2PowerAim = __fmin(ArmWriteData.PV2PowerAim,PowerLimitTmp);
    PVSystemData.PV3PowerAim = __fmin(ArmWriteData.PV3PowerAim,PowerLimitTmp);
    PVSystemData.PV4PowerAim = __fmin(ArmWriteData.PV4PowerAim,PowerLimitTmp);

    PV1WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV1PowerAim/SampleRms.PV1VoltFilter8);
    PV2WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV2PowerAim/SampleRms.PV2VoltFilter8);
    PV3WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV3PowerAim/SampleRms.PV3VoltFilter8);
    PV4WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV4PowerAim/SampleRms.PV4VoltFilter8);

//    PV1WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV1PowerAim/PV1WorkState.Mppt.Output);
//    PV2WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV2PowerAim/PV2WorkState.Mppt.Output);
//    PV3WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV3PowerAim/PV3WorkState.Mppt.Output);
//    PV4WorkState.PVCurrLimit = __fmin(IPVmax,PVSystemData.PV4PowerAim/PV4WorkState.Mppt.Output);


    //    if(InvStateData.WorkState != OnGridState)
    //    {
    //        RealRefR.P = 0;
    //        RealRefS.P = 0;
    //    }
    //    pv->PVCurrLimit = __fmin(pv->PVCurrLimit,__fmax((RealRefR.P +  RealRefS.P + 200)/pv->PVVolt ,0));

    if(SampleRms.PV1VoltFilter8 > 79)
    {
        PV1WorkState.Online = 1;
    }
    else if(PV1WorkState.State != MPPT_STATE)
    {
        PV1WorkState.Online = 0;
    }
    if(SampleRms.PV2VoltFilter8 > 79)
    {
        PV2WorkState.Online = 1;
    }
    else if(PV2WorkState.State != MPPT_STATE)
    {
        PV2WorkState.Online = 0;
    }
    if(SampleRms.PV3VoltFilter8 > 79)
    {
        PV3WorkState.Online = 1;
    }
    else if(PV3WorkState.State != MPPT_STATE)
    {
        PV3WorkState.Online = 0;
    }
    if(SampleRms.PV4VoltFilter8 > 79)
    {
        PV4WorkState.Online = 1;
    }
    else if(PV4WorkState.State != MPPT_STATE)
    {
        PV4WorkState.Online = 0;
    }
}
/*************************************************
Function: JudgeParallelMode
Description: 判断并联模式分配主机和从机,需要开机之前设置
Input: None
Output: None
Frequency: 10KHz
*************************************************/
inline void JudgeParallelMode(void)
{
    if (PVSystemData.PVParallel != 0)
    {
 //       PVSystemData.PVParallelWorkNum = PVSystemData.PVParallelWorkNum;

        if(PV1WorkState.Online)
        {
            PV1WorkState.Parallel = PV_MASTER;      // PV1 为主机运行 MPPT
            PV2WorkState.Parallel = PV_SLAVE;   // PV2 为从机,电压参考从 PV1 获取
            PV3WorkState.Parallel = PV_SLAVE;   // PV3 为从机,电压参考从 PV1 获取
            PV4WorkState.Parallel = PV_SLAVE;   // PV4 为从机,电压参考从 PV1 获取

            PVSystemData.PVParallelWorkNum = 1 + PV2WorkState.Run + PV3WorkState.Run + PV4WorkState.Run;

            if (PV1WorkState.State != MPPT_STATE)
            {
                if(PV2WorkState.State == MPPT_STATE)
                {
                    PV2WorkState.Count = 0;
                    PV2WorkState.State = SELF_STATE;
                }
                if(PV3WorkState.State == MPPT_STATE)
                {
                    PV3WorkState.Count = 0;
                    PV3WorkState.State = SELF_STATE;
                }
                if(PV4WorkState.State == MPPT_STATE)
                {
                    PV4WorkState.Count = 0;
                    PV4WorkState.State = SELF_STATE;
                }
            }

        }
        else if(PV2WorkState.Online)
        {
            PV1WorkState.Parallel = PV_FREE;      // PV1 无
            PV2WorkState.Parallel = PV_MASTER;   // PV2 为主机
            PV3WorkState.Parallel = PV_SLAVE;   // PV3 为从机,电压参考从 PV2 获取
            PV4WorkState.Parallel = PV_SLAVE;   // PV4 为从机,电压参考从 PV2 获取

            PVSystemData.PVParallelWorkNum = 1 + PV3WorkState.Run + PV4WorkState.Run;

            if (PV2WorkState.State != MPPT_STATE)
            {
                if(PV3WorkState.State == MPPT_STATE)
                {
                    PV3WorkState.Count = 0;
                    PV3WorkState.State = SELF_STATE;
                }
                if(PV4WorkState.State == MPPT_STATE)
                {
                    PV4WorkState.Count = 0;
                    PV4WorkState.State = SELF_STATE;
                }
            }
        }
        else if(PV3WorkState.Online)
        {
            PV1WorkState.Parallel = PV_FREE;      // PV1 无
            PV2WorkState.Parallel = PV_FREE;   // PV2 无
            PV3WorkState.Parallel = PV_MASTER;   // PV3 为主机
            PV4WorkState.Parallel = PV_SLAVE;   // PV4 为从机,电压参考从 PV3 获取

            PVSystemData.PVParallelWorkNum = 1 + PV4WorkState.Run;

            if (PV3WorkState.State != MPPT_STATE)
            {
                if(PV4WorkState.State == MPPT_STATE)
                {
                    PV4WorkState.Count = 0;
                    PV4WorkState.State = SELF_STATE;
                }
            }
        }
        else
        {
            PV1WorkState.Parallel = PV_FREE;
            PV2WorkState.Parallel = PV_FREE;
            PV3WorkState.Parallel = PV_FREE;
            PV4WorkState.Parallel = PV_FREE;
            PVSystemData.PVParallelWorkNum = 1;
        }
    }
    else
    {
        PV1WorkState.Parallel = PV_FREE;
        PV2WorkState.Parallel = PV_FREE;
        PV3WorkState.Parallel = PV_FREE;
        PV4WorkState.Parallel = PV_FREE;
        PVSystemData.PVParallelWorkNum = 1;
    }

    //台数变化时突变
    float tmp= 1.0f/PVSystemData.PVParallelWorkNum;
    mRefRamp(PVSystemData.IPVParallelWorkNum , tmp, 0.1f);
}

inline void DCSourceModeConfig(void)
{
         // 单电流环,直接根据目标功率计算出目标电流
//        PVSystemData.PV1PowerAim = SystemPara.PVRatedPower;
//        PVSystemData.PV2PowerAim = SystemPara.PVRatedPower;
//        PVSystemData.PV3PowerAim = SystemPara.PVRatedPower;
//        PVSystemData.PV4PowerAim = SystemPara.PVRatedPower;
//
//        PV1WorkState.Parallel = PV_S_LOOP;
//        PV2WorkState.Parallel = PV_S_LOOP;
//        PV3WorkState.Parallel = PV_S_LOOP;
//        PV4WorkState.Parallel = PV_S_LOOP;


        PV1WorkState.VoltSet.TargetValue = 400;
        PV2WorkState.VoltSet.TargetValue = PV1WorkState.VoltSet.TargetValue;
        PV3WorkState.VoltSet.TargetValue = PV1WorkState.VoltSet.TargetValue;
        PV4WorkState.VoltSet.TargetValue = PV1WorkState.VoltSet.TargetValue;

        PV1WorkState.Parallel = PV_FREE;
        PV2WorkState.Parallel = PV_FREE;
        PV3WorkState.Parallel = PV_FREE;
        PV4WorkState.Parallel = PV_FREE;
        PVSystemData.PVParallelWorkNum = 1;

        //台数变化时突变
        float tmp= 1.0f/PVSystemData.PVParallelWorkNum;
        mRefRamp(PVSystemData.IPVParallelWorkNum , tmp, 0.1f);


        PV1WorkState.CurrSet.StepValue = (1.0f/(PV_PWM_FREQ_KHZ*1000.0*0.5));
        PV2WorkState.CurrSet.StepValue = PV1WorkState.CurrSet.StepValue;
        PV3WorkState.CurrSet.StepValue = PV1WorkState.CurrSet.StepValue;
        PV4WorkState.CurrSet.StepValue = PV1WorkState.CurrSet.StepValue;
}
/*************************************************
Function: CheckPVParallelErr
Description: 检查PV并联接线是否正确,如果不正确将报PV并联接线告警
Input: None
Output: None
Frequency: 0.1ms
*************************************************/
void CheckPVParallelErr(void)
{
    static int PvCnt = 0;
    if((SampleIns.PV1CurrIns < -2)
        ||(SampleIns.PV2CurrIns < -2)
        ||(SampleIns.PV3CurrIns < -2)
        ||(SampleIns.PV4CurrIns < -2))
    {
        if( ++PvCnt >  PWM_ISR_FREQUENCY)
        SystemError.Fault1.bit.Parallel = 1;
    }
    else
    {
        PvCnt = 0;
    }

}
/******************************************************************************
// Export Function-Prototype: GeneralMoudlarPVState
// Description  : Handle PVState
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/08/09
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(RunPVStateMachine, ".TI.ramfunc");
void RunPVStateMachine(void)
{
    PVSystemData.PVStart = SystemPara.SystemPowerOn && (FuncPara.BatState == 0x22);
 //   PV2WorkState.Online = 0;

    PV1WorkState.Error = SystemError.StateErr.bit.PV1Fault || (SystemError.ArmFault&ARM_FAULT_BITS);
    PV2WorkState.Error = SystemError.StateErr.bit.PV2Fault || (SystemError.ArmFault&ARM_FAULT_BITS);
    PV3WorkState.Error = SystemError.StateErr.bit.PV3Fault || (SystemError.ArmFault&ARM_FAULT_BITS);
    PV4WorkState.Error = SystemError.StateErr.bit.PV4Fault || (SystemError.ArmFault&ARM_FAULT_BITS);

//    PV1WorkState.Error = pv1fault.all;
//    PV2WorkState.Error = pv2fault.all;
//    PV3WorkState.Error = pv3fault.all;
//    PV4WorkState.Error = pv4fault.all;
//    PV1WorkState.Online = (pv1fault.all == 0);
//    PV2WorkState.Online = (pv2fault.all == 0);
//    PV3WorkState.Online = (pv3fault.all == 0);
//    PV4WorkState.Online = (pv4fault.all == 0);
//    PV1WorkState.Online = (SampleIns.PV1VoltIns > 80);//(SystemAlarm.bit.PV1Low == 0);
//    PV2WorkState.Online = (SampleIns.PV2VoltIns > 80);//(SystemAlarm.bit.PV2Low == 0);
//    PV3WorkState.Online = (SampleIns.PV3VoltIns > 80);//(SystemAlarm.bit.PV3Low == 0);
//    PV4WorkState.Online = (SampleIns.PV4VoltIns > 80);//(SystemAlarm.bit.PV4Low == 0);

    //电池或电网存在时启动pv

    if(++PV1WorkState.Mppt.turn > 4)
        PV1WorkState.Mppt.turn = 1;
    PV2WorkState.Mppt.turn = PV1WorkState.Mppt.turn;
    PV3WorkState.Mppt.turn = PV1WorkState.Mppt.turn;
    PV4WorkState.Mppt.turn = PV1WorkState.Mppt.turn;

    if (PVSystemData.PVSourceType == PVBoard)
    {
        JudgeParallelMode();        // 并机模式判断
    }
    else
    {
//        JudgeParallelMode();        // 并机模式判断
        DCSourceModeConfig();
    }

//    PVSystemData.BusAdptCoeff = 1/__fsat(SampleIns.BUSVoltInsCom,VpvRef,300);
    PVSystemData.BusAdptCoeff = 1/__fsat(SampleIns.BUSVoltIns,VpvRef,300);
//    PVSystemData.BusAdptCoeff = 1/__fsat(SampleRms.BUSVoltFilter200,VpvRef,300);
    PVSystemData.BusDutyLimit = (PV_PWM_PERIOD - 1) * __fsat(1 - 0.1*((SampleIns.BUSVoltIns - (VpvRef+3))), 1,0);  //电池充电电压最高453
//    PVSystemData.BusAdptCoeff = __fsat(PVSystemData.BusAdptCoeff,1.2f,0.8f);
    // -------------------并联方式---------------------------------
    // PV1 为主机
    if (PV1WorkState.Parallel == PV_MASTER)
    {
        // 主机先运行
        PVState(&PV1WorkState);
        PVState(&PV2WorkState);
        PVState(&PV3WorkState);
        PVState(&PV4WorkState);
    }

    // PV2 为主机
    else if (PV2WorkState.Parallel == PV_MASTER)
    {
        // 主机先运行
        PVState(&PV2WorkState);
        PVState(&PV1WorkState);
        PVState(&PV3WorkState);
        PVState(&PV4WorkState);
    }

    // PV3 为主机
    else if(PV3WorkState.Parallel == PV_MASTER)
    {
        // 主机先运行
        PVState(&PV3WorkState);
        PVState(&PV1WorkState);
        PVState(&PV2WorkState);
        PVState(&PV4WorkState);
    }
    // ----------------------没有并联,独立方式------------------------
    else
    {
        PVState(&PV1WorkState);
        PVState(&PV2WorkState);
        PVState(&PV3WorkState);
        PVState(&PV4WorkState);
    }
}


/*************************************************
Function: DisablePVPwm
Description: 禁止 PV PWM
Input: @channel-1禁止PV1;2-禁止PV2;3-禁止PV3
Output: None
Frequency:
*************************************************/
#pragma CODE_SECTION(DisablePVPwm, ".TI.ramfunc");
void DisablePVPwm(uint16_t channel)
{
    if (channel == 1)
    {
        DISABLE_PV1PWM;
    }
    else if (channel == 2)
    {
        DISABLE_PV2PWM;
    }
    else if (channel == 3)
    {
        DISABLE_PV3PWM;
    }
    else if (channel == 4)
    {
        DISABLE_PV4PWM;
    }

}

/*************************************************
Function: EnablePVPwm
Description: 使能 PV PWM
Input: @channel-1使能PV1;2-使能PV2;3-使能PV3
Output: None
Frequency:
*************************************************/
#pragma CODE_SECTION(EnablePVPwm, ".TI.ramfunc");
void EnablePVPwm(uint16_t channel)
{
    if (channel == 1)
    {
        EPwm6Regs.CMPA.bit.CMPA = (int)PV1WorkState.Duty;
        ENABLE_PV1PWM;
    }
    else if (channel == 2)
    {
        EPwm7Regs.CMPA.bit.CMPA = (int)PV2WorkState.Duty;
        ENABLE_PV2PWM;
    }
    else if (channel == 3)
    {
        EPwm10Regs.CMPA.bit.CMPA = (int)PV3WorkState.Duty;
        ENABLE_PV3PWM;
    }
    else if (channel == 4)
    {
        EPwm5Regs.CMPA.bit.CMPA = (int)PV4WorkState.Duty;
        ENABLE_PV4PWM;
    }
}





#include <Function.h>
#include <Sample.h>
#include <F2837xS_device.h>
#include "ConfigSystemPara.h"
#include <bsp.h>
#include <math.h>
#include <ModbusTable.h>
#include <Protect.h>
#include "OffGridInv2.h"
#include "OnGridInv2.h"
#include <string.h>
#include <CalculateAngle.h>
#include <InvStateMachine.h>
#include <PowerCalculator.h>
#include "PVStateMachine.h"
struct FUNC_PARA FuncPara;

void InitFuncData(void)
{
    memset(&FuncPara, 0, sizeof(struct FUNC_PARA));
}


//=================================================================================
// 函数名称:
// 函数功能:
// 输入参数: 无
// 输出参数: 无
// 其它说明: 无
//=================================================================================
void InitAddrMap(void)
{
//    address[0] =  &ArmReadData.ARMEnable;//5000
//    address[1] =  &ArmReadData.RInvState;
//    address[2] =  &ArmReadData.UrINV;
//    address[3] =  &ArmReadData.IrINV;
//    address[4] =  &ArmReadData.PrINV;
//    address[5] =  &ArmReadData.IrLoad;
//    address[6] =  &ArmReadData.PrLoad;
//    address[7] =  &ArmReadData.UrGrid;
//    address[8] =  &ArmReadData.IrGrid;
//    address[9] =  &ArmReadData.PrGrid;
//    address[10] = &ArmReadData.SInvState;//5010
//    address[11] = &ArmReadData.UsINV;
//    address[12] = &ArmReadData.IsINV;
//    address[13] = &ArmReadData.PsINV;
//    address[14] = &ArmReadData.IsLoad;
//    address[15] = &ArmReadData.PsLoad;
//    address[16] = &ArmReadData.UsGrid;
//    address[17] = &ArmReadData.IsGrid;
//    address[18] = &ArmReadData.PsGrid;  //5018
//    address[19] = &ArmReadData.InvFreq; //5028
//    address[20] = &ArmReadData.GridFreq;
//    address[21] = &ArmReadData.PCSErr1;
//    address[22] = &ArmReadData.PCSErr2.all;
//    address[23] = &ArmReadData.PCSErr3;
//    address[24] = &ArmReadData.PCSErr4;
//    address[25] = &ArmReadData.PCSAlarm1;


}

//=================================================================================
// 函数名称:
// 函数功能: ISO检测
// 输入参数: 上继电器控制IO,下继电器控制IO,继电器切换时间,检测结果AD值,母线电压,PV在线标志,检测电路阻值,上电等待时间
// 输出参数: Rx,Ry,R保护点,iso检测结束标志,iso检测结果
// 其它说明: 无
//=================================================================================
void ISOCheck(void)
{

    if((SampleIns.CalSampleZeroFlag != 1)||(FuncPara.ISOFinishFlag != 0))
    return;

    if((PVSystemData.PV1Available)
            ||(PVSystemData.PV2Available)
            ||(PVSystemData.PV3Available)
            ||(PVSystemData.PV4Available))//包含并联模式与独立模式
    {
        FuncPara.ISOCount++;
        FuncPara.ISOVoltTemp = Cla1ToCpuMsg.adresult1[5];
//        FuncPara.ISOVoltTemp = AdcaResultRegs.ADCRESULT0;

        if(FuncPara.ISOCount <= 2000)
        {
            PV1_ISO_TEST_RLY_ON;
            PV2_ISO_TEST_RLY_OFF;

            if(FuncPara.ISOCount >= 1600)
            {
                FuncPara.ISOVoltSum1 += FuncPara.ISOVoltTemp;
                if(FuncPara.ISOCount == 2000)
                {
                    FuncPara.ISOVoltTemp1 = FuncPara.ISOVoltSum1 * 0.0025f;
                    FuncPara.ISOVoltSum1  = 0;
                }
            }
        }
        else if(FuncPara.ISOCount <= 4000)
        {
            PV1_ISO_TEST_RLY_OFF;
            PV2_ISO_TEST_RLY_ON;

            if(FuncPara.ISOCount >= 3600)
            {
                FuncPara.ISOVoltSum2 += FuncPara.ISOVoltTemp;
                if(FuncPara.ISOCount == 4000)
                {
                    FuncPara.ISOVoltTemp2 = FuncPara.ISOVoltSum2 * 0.0025f;
                    FuncPara.ISOVoltSum2  = 0;
                }
            }
        }
        else
        {

            FuncPara.ISOBusVolt = SampleIns.BUSVoltIns;
            if(FuncPara.ISOVoltTemp1 > 67)
            {
                FuncPara.ISOVoltFinal1=FuncPara.ISOVoltTemp1*(12200/75.0f*3/4096.0f)* 0.9999-0.104;
                FuncPara.ISOVoltFinal2=FuncPara.ISOVoltTemp2*(12200/75.0f*3/4096.0f)* 0.9999-0.104;
            }
            else
            {
                FuncPara.ISOVoltFinal1=FuncPara.ISOVoltTemp1*(12200/75.0f*3/4096.0f)* 1.0719-0.231;
                FuncPara.ISOVoltFinal2=FuncPara.ISOVoltTemp2*(12200/75.0f*3/4096.0f)* 1.0719-0.231;
            }
            FuncPara.ISORx = ((FuncPara.ISOVoltFinal2-FuncPara.ISOVoltFinal1)*63600.0)/(FuncPara.ISOVoltFinal1*3.0-FuncPara.ISOVoltFinal2*53.0);                //rx阻值
            FuncPara.ISORy = ((FuncPara.ISOVoltFinal1-FuncPara.ISOVoltFinal2)*63600.0)/(FuncPara.ISOVoltFinal2*3.0-53.0*FuncPara.ISOVoltFinal1+50.0*FuncPara.ISOBusVolt);     //ry阻值
            FuncPara.ISORx = fabsf(FuncPara.ISORx);
            FuncPara.ISORy = fabsf(FuncPara.ISORy);
            FuncPara.ISOMin = __fmin(FuncPara.ISORx,FuncPara.ISORy);

            if((FuncPara.ISORx <= 410)||(FuncPara.ISORy <= 410)||((FuncPara.ISOVoltTemp2 == 0)&&(FuncPara.ISOVoltFinal1 >= 25)))
            {//故障
                SystemError.Fault2.bit.PVISO = 1;
            }

            PV1_ISO_TEST_RLY_OFF;
            PV2_ISO_TEST_RLY_OFF;

            FuncPara.ISOCount = 0;
            FuncPara.ISOFinishFlag = 1;
        }
    }
    else
    {
        if(FuncPara.ISOJumpDelay++ >= 5000)
        {
            FuncPara.ISOFinishFlag = 1;
        }
    }


}
//=================================================================================
// 函数名称:
// 函数功能:继电器检测
// 输入参数: 无
// 输出参数: 无
// 其它说明: 无
//=================================================================================
void InvRlyCheck(void)
{
    float CheckVolt1,CheckVolt2;

    if((FuncPara.RlyInvOver)||(FuncPara.RlyError))
        return;

    if(InvStateData.WorkState == OnGridState)
    {
        CheckVolt1 = SampleRms.Inv1VoltRms;
        CheckVolt2 = SampleRms.Inv2VoltRms;
    }
    else
    {
        CheckVolt1 = SampleRms.Load1VoltRms;
        CheckVolt2 = SampleRms.Load2VoltRms;
    }

    if(FuncPara.RlyCheckSt0 == 0)
    {
        LOAD_RLY1_ON;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if((CheckVolt1 > 60)||(CheckVolt2 > 60))
            {
                FuncPara.RlyError = 14;//负载L2或N粘连
            }
            else
            {
                FuncPara.RlyCheckSt0 = 1;
            }
        }
    }

    if(FuncPara.RlyCheckSt0 == 1)
    {
        LOAD_RLY1_OFF;
        LOAD_RLY2_ON;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(CheckVolt2 > 60)
            {
                FuncPara.RlyError = 15;//负载L1粘连
            }
            else
            {
                FuncPara.RlyCheckSt0 = 2;
            }
        }
    }

    if(FuncPara.RlyCheckSt0 == 2)
    {
        if((SampleIns.PeriodPoint - InvPhase.ThetaPointR) == 90)
        {
            N_LOAD_RLY_ON;
        }
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(CheckVolt2 < 60)
            {
                FuncPara.RlyError = 16;//负载N或负载L2无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt0 = 3;
            }
        }
    }

    if(FuncPara.RlyCheckSt0 == 3)
    {
        if((SampleIns.PeriodPoint - InvPhase.ThetaPointR) == 90)
        {
            LOAD_RLY1_ON;
        }
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(CheckVolt1 < 60)
            {
                FuncPara.RlyError = 17;//负载L1无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt0 = 4;
                FuncPara.RlyInvOver = 1;
            }
        }
    }

}

//=================================================================================
// 函数名称:
// 函数功能:继电器检测
// 输入参数: 无
// 输出参数: 无
// 其它说明: 无
//=================================================================================
void GridRlyCheck(void)
{
    if((FuncPara.RlyGridOver)||(FuncPara.RlyError))
        return;

    if(FuncPara.RlyCheckSt1 == 0)
    {
        if(SystemAlarm.all & 0xF)
        {
            FuncPara.RlyGridOver = 1;
            FuncPara.RlyMidOver = 1;
        }
        else
        {
            GRID_L1_RLY_ON;
            GRID_L2_RLY_ON;
            if(FuncPara.RlyDelay++ >= 5000)
            {
                FuncPara.RlyDelay = 0;
                if((SampleRms.Mid1VoltRms < 60)||(SampleRms.Mid2VoltRms < 60))
                {
                    FuncPara.RlyError = 1;//并网L1或L2无法吸合
                }
                else
                {
                    FuncPara.RlyCheckSt1 = 1;
                }
            }
        }
    }

    if(FuncPara.RlyCheckSt1 == 1)
    {
        GRID_L2_RLY_OFF;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms > 60)
            {
                FuncPara.RlyError = 2;//并网N无法断开
            }
            if(SampleRms.Mid2VoltRms > 60)
            {
                FuncPara.RlyError = 3;//并网L2无法断开
            }
            else
            {
                FuncPara.RlyCheckSt1 = 2;
            }
        }
    }

    if(FuncPara.RlyCheckSt1 == 2)
    {
        N_GRID_RLY_ON;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms < 60)
            {
                FuncPara.RlyError = 4;//并网N无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt1 = 3;
            }
        }
    }

    if(FuncPara.RlyCheckSt1 == 3)
    {
        GRID_L1_RLY_OFF;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms > 60)
            {
                FuncPara.RlyError = 5;//并网L1无法断开
            }
            else
            {
                FuncPara.RlyCheckSt1 = 4;
                FuncPara.RlyGridOver = 1;
                N_GRID_RLY_OFF;
            }
        }
    }
}

void MidRlyCheck(void)
{
    if((FuncPara.RlyMidOver)||(FuncPara.RlyError))
        return;

    if(FuncPara.RlyCheckSt2 == 0)
    {
        if(FuncPara.RlyDelay++ >= 10000)
        {
            MID_L1_RLY_ON;
            MID_L2_RLY_ON;
            if(FuncPara.RlyDelay++ >= 20000)
            {
                FuncPara.RlyDelay = 0;
                if((SampleRms.Mid1VoltRms < 60)||(SampleRms.Mid2VoltRms < 60))
                {
                    FuncPara.RlyError = 6;//中间L1或L2无法吸合
                }
                else
                {
                    FuncPara.RlyCheckSt2 = 1;
                }
            }
        }
    }

    if(FuncPara.RlyCheckSt2 == 1)
    {
        MID_L2_RLY_OFF;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms > 60)
            {
                FuncPara.RlyError = 7;//中间N无法断开
            }
            if(SampleRms.Mid2VoltRms > 60)
            {
                FuncPara.RlyError = 8;//中间L2无法断开
            }
            else
            {
                FuncPara.RlyCheckSt2 = 2;
            }
        }
    }

    if(FuncPara.RlyCheckSt2 == 2)
    {
        N_MID_RLY_ON;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms < 60)
            {
                FuncPara.RlyError = 9;//中间N无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt2 = 3;
            }
        }
    }

    if(FuncPara.RlyCheckSt2 == 3)
    {
        MID_L1_RLY_OFF;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid1VoltRms > 60)
            {
                FuncPara.RlyError = 10;//中间L1无法断开
            }
            else
            {
                FuncPara.RlyCheckSt2 = 4;
                FuncPara.RlyMidOver = 1;
                N_MID_RLY_OFF;
            }
        }
    }
}

void BpRlyCheck(void)
{
    if((FuncPara.RlyBpOver)||(FuncPara.RlyError))
        return;

    if(FuncPara.RlyMidOver)
    {
        FuncPara.RlyBpOver = 1;
    }
    else
    {
        if(FuncPara.RlyCheckSt3 == 0)
        {
            GRID_L1_RLY_ON;
            N_GRID_RLY_ON;
            if(FuncPara.RlyDelay++ >= 5000)
            {
                FuncPara.RlyDelay = 0;
                if(SampleRms.Mid1VoltRms < 60)
                {
                    FuncPara.RlyError = 11;//并网L1、N无法吸合
                }
                else
                {
                    FuncPara.RlyCheckSt3 = 1;
                }
            }
        }
    }

    if(FuncPara.RlyCheckSt3 == 1)
    {
        GRID_L2_RLY_ON;
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Mid2VoltRms < 60)
            {
                FuncPara.RlyError = 12;//并网L2无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt3 = 2;
            }
        }
    }

    if(FuncPara.RlyCheckSt3 == 2)
    {
        if(fabs((PI_S - IrCtrl.ThetaSpll)- (43 - 30 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR)
         {
             MID_L1_RLY_ON;
             N_MID_RLY_ON ;
         }
        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Load1VoltRms < 60)
            {
                FuncPara.RlyError = 13;//中间L1、N无法吸合
            }
            else
            {
                FuncPara.RlyCheckSt3 = 3;
            }
        }

    }

    if(FuncPara.RlyCheckSt3 == 3)
    {
        if(fabs((PI_S - IrCtrl.ThetaSpll)-(43 - 30 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR)
        {
            MID_L2_RLY_ON;
        }

        if(FuncPara.RlyDelay++ >= 5000)
        {
            FuncPara.RlyDelay = 0;
            if(SampleRms.Load2VoltRms < 60)
            {
                FuncPara.RlyError = 14;//并网L2无法吸合
            }
            else
            {
                FuncPara.RlyBpOver = 1;
                FuncPara.RlyCheckSt3 = 4;
            }
        }
    }

}


typedef struct{
    Uint16 time;     //
    Uint16 cnt;      //
}FLAG_CHECK_STRUCT;
extern char mFlagCounter(Uint16 flag,FLAG_CHECK_STRUCT * Counter);

void RlyOpenCheck(void)
{
    static FLAG_CHECK_STRUCT Rly1fault = {150,0};    //1500ms/10ms
    static FLAG_CHECK_STRUCT Rly2fault = {150,0};    //1500ms/10ms

    //并网开路检测
    if(mFlagCounter(fabsf(SampleRms.Grid1VoltRms - SampleRms.Inv1VoltRms) >30
                    && (InvStateData.WorkState == OnGridState)
                    && (InvStateData.OnGridCnt >= 100000L)
                    && (ArmWriteData.AgingLable ==0 )
                    && (SystemPara.GridEnhanceEn ==0 )
                  ,&Rly1fault))
    {
        FuncPara.RlyError = 15;//并网L1无法吸合
    }

    if(mFlagCounter(fabsf(SampleRms.Grid2VoltRms - SampleRms.Inv2VoltRms) >30
                    && (InvStateData.WorkState == OnGridState)
                    && (InvStateData.OnGridCnt >= 100000L)
                    && (ArmWriteData.AgingLable ==0 )
                    && (SystemPara.GridEnhanceEn ==0 )
                  ,&Rly2fault))
    {
        FuncPara.RlyError = 16;//并网L2无法吸合
    }

    //离网开路检测


}


//=================================================================================
// 函数名称:
// 函数功能:过流斩波事件处理
// 输入参数: 无
// 输出参数: 无
// 其它说明: 无
//=================================================================================
void InvOcpHandle(void)
{
    //半波判断
    if((!SystemError.Fault1.bit.L1OverCurrH)&&(!SystemError.Fault1.bit.L2OverCurrH))
    {
        if((fabs(FuncPara.INVIAVG1) >= 5)||(fabs(FuncPara.INVIAVG2) >= 5))//RCD载前两个周期会呈现出半波负载的不对称性
        {
            if(FuncPara.HalfWaveJudge++ > 400)
            {
                FuncPara.ProtectCount = 150;
                FuncPara.RecoverCount = 150;
            }
        }
        else
        {
            FuncPara.ProtectCount = 400;
            FuncPara.RecoverCount = 50;
            FuncPara.HalfWaveJudge = 0;
        }
    }
    //保护判断
    if(FuncPara.OCL1)
    {
        FuncPara.OCL1 = 0;
        FuncPara.OCL1RecCount = 0;
        FuncPara.OCL1Count ++;
        if(FuncPara.OCL1Count >= FuncPara.ProtectCount)
        {
            FuncPara.OCL1Count = FuncPara.ProtectCount;
            SystemError.Fault1.bit.L1OverCurrH = 1;
        }
    }
    else
    {
        if(!SystemError.Fault1.bit.L1OverCurrH)
        {
            if(FuncPara.OCL1RecCount++ >= FuncPara.RecoverCount)
            {
                FuncPara.OCL1RecCount = 0;
                FuncPara.OCL1Count = 0;
            }
        }
    }
    if(FuncPara.OCL2)
    {
        FuncPara.OCL2 = 0;
        FuncPara.OCL2Count ++;
        FuncPara.OCL2RecCount = 0;
        if(FuncPara.OCL2Count >= FuncPara.ProtectCount)
        {
            FuncPara.OCL2Count = FuncPara.ProtectCount;
            SystemError.Fault1.bit.L2OverCurrH = 1;
        }
    }
    else
    {
        if(!SystemError.Fault1.bit.L2OverCurrH)
        {
            if(FuncPara.OCL2RecCount++ >= FuncPara.RecoverCount)
            {
                FuncPara.OCL2RecCount = 0;
                FuncPara.OCL2Count = 0;
            }
        }
    }
    if(FuncPara.OCL3)
    {
        FuncPara.OCL3 = 0;
        FuncPara.OCL3Count ++;
        if(FuncPara.OCL3Count >= 3)
        {
            FuncPara.OCL3Count = 3;
            SystemError.Fault1.bit.L1OverCurrH = 1;
            SystemError.Fault1.bit.L2OverCurrH = 1;
        }
    }
    if(FuncPara.OCL3Count > 0)
    {
        FuncPara.OCL3RecCount ++;
        if(FuncPara.OCL3RecCount >= 100)
        {
            FuncPara.OCL3Count = 0;
            FuncPara.OCL3RecCount = 0;
        }
    }

}
//=================================================================================
// 函数名称:
// 函数功能:跳转自检状态
// 输入参数: 无
// 输出参数: 无
// 其它说明:最好是离网过零关驱动，并网过零断继电器
//=================================================================================
void SwitchToSelfCheckState(void)
{
    if(InvPhase.ThetaPointR >= InvPhase.PeriodPoint - 1)//旁路时若电网掉电计数值不会清0，离网运行到这里时不会是InvPhase.PeriodPoint，在前面已经清0
    {
        InvStateData.SelfCheckCnt = 0;
        ControlRelay(SelfCheckState);
        InvStateData.WorkState = SelfCheckState;
    }
}

//=================================================================================
// 函数名称:
// 函数功能:BUS高压关PV驱动
// 输入参数: 无
// 输出参数: 无
// 其它说明:
//=================================================================================
//void PVHVCloseDrive(void)
//{
//    if((SampleIns.BUSVoltIns >= 460)&&(FuncPara.PVHVFlag == 0))
//    {
//        FuncPara.PVHVFlag = 1;
//    }
//    if(FuncPara.PVHVFlag == 1)
//    {
//        DISABLE_PV1PWM;
//        DISABLE_PV2PWM;
//        DISABLE_PV3PWM;
//        DISABLE_PV4PWM;
//        if(SampleIns.BUSVoltIns <= 440)
//        {
//            FuncPara.PVHVFlag = 0;
//        }
//    }
//
//}

//=================================================================================
// 函数名称:
// 函数功能:采样系数校准
// 输入参数: 无
// 输出参数: 无
// 其它说明:按功率划分区间,但是ARM不会一直下发，故仿真器烧录后所有下发系数为0
//=================================================================================
void ADCoeffModify(void)
{
//    if(ArmWriteData.WAdKCoefFlag == 2)
//    {
//        if(IrCtrl.RmpCtrl.SetpointValue >= 0)//INVL1
//        {
//            if(SystemPower.ApInv1.Average > 1020)
//            {
//                SampleCalibCoef.Inv1CurrK = ArmWriteData.ADCoeff.Iinv1K2;
//            }
//            else if(SystemPower.ApInv1.Average < 980)
//            {
//                SampleCalibCoef.Inv1CurrK = ArmWriteData.ADCoeff.Iinv1K1;
//            }
//        }
//        else
//        {
//            if(SystemPower.ApInv1.Average > 1520)
//            {
//                SampleCalibCoef.Inv1CurrK = ArmWriteData.ADCoeff.Iinv1K4;
//            }
//            else if(SystemPower.ApInv1.Average < 1480)
//            {
//                SampleCalibCoef.Inv1CurrK = ArmWriteData.ADCoeff.Iinv1K3;
//            }
//        }
//
//        if(IsCtrl.RmpCtrl.SetpointValue >= 0)//INVL2
//        {
//             if(SystemPower.ApInv2.Average > 1020)
//             {
//                 SampleCalibCoef.Inv2CurrK = ArmWriteData.ADCoeff.Iinv2K2;
//             }
//             else if(SystemPower.ApInv2.Average < 980)
//             {
//                 SampleCalibCoef.Inv2CurrK = ArmWriteData.ADCoeff.Iinv2K1;
//             }
//
//        }
//        else
//        {
//            if(SystemPower.ApInv2.Average > 1520)
//            {
//                SampleCalibCoef.Inv2CurrK = ArmWriteData.ADCoeff.Iinv2K4;
//            }
//            else if(SystemPower.ApInv2.Average < 1480)
//            {
//                SampleCalibCoef.Inv2CurrK = ArmWriteData.ADCoeff.Iinv2K3;
//            }
//        }
//    }
//    SampleCalibCoef.Inv1CurrK = __fsat(SampleCalibCoef.Inv1CurrK, 3000.0f, 1000.0f);
//    SampleCalibCoef.Inv2CurrK = __fsat(SampleCalibCoef.Inv2CurrK, 3000.0f, 1000.0f);

}

//=================================================================================
// 函数名称:
// 函数功能:电池包激活任务
// 输入参数: 无
// 输出参数: 无
// 其它说明:
//=================================================================================
void BatActiveTask(void)
{
    uint16_t PVThreshold = 95;//100-5

    if(!SystemPara.SystemPowerOn)
     {
         POWER_OFF;
         FuncPara.BatPowering = 0;
         FuncPara.BatPowerCnt = 0;
         FuncPara.BatPowerDly = 0;
         FuncPara.BatPowered = 0;
         FuncPara.BatPowerTry = 0;
         return;
     }
    else if(FuncPara.BatPowered && (FuncPara.BatState != 0x22))//ArmWriteData.BMSSOC==0 &&
    {
        FuncPara.BatPowering = 0;
        FuncPara.BatPowerCnt = 0;
        FuncPara.BatPowerDly = (6000L*90);    //90MIN
        FuncPara.BatPowered = 0;
        FuncPara.BatPowerTry = 0;
    }

    if(PVSystemData.PVSourceType == Vsource)
        FuncPara.BatPowerDly = 10;

    if(FuncPara.BatPowering) //本次激活结束
    {
        if((FuncPara.BatState == 0x22)&&(ArmWriteData.Fault.bit.ATEMode==0))//激活过程中电池包开机认为激活成功
        {
            POWER_OFF;
            FuncPara.BatPowered = 1;
            FuncPara.BatPowering = 0;
            FuncPara.BatPowerTry = 0;
            return;
        }
        if(FuncPara.BatPowerCnt++ >= 3000)//激活信号持续30s，防止电池包状态信息传过来太慢
        {
            POWER_OFF;
            FuncPara.BatPowering = 0;
            FuncPara.BatPowerCnt = 0;
            FuncPara.BatPowerTry++;
            if(FuncPara.BatPowerTry <= 3)
            {
                FuncPara.BatPowerDly = (6000L);//30s没激活准备60s进行下一次激活
            }
        }
        return;
    }

    if(InvStateData.WorkState == ByPass && FuncPara.BatPowerDly > 6000L)    //电网来了立即激活
    {
        FuncPara.BatPowerDly = 6000L;
    }

    if(FuncPara.BatPowerDly > 0) //
    {
        FuncPara.BatPowerDly--;
        return;
    }

    if(((FuncPara.BatState == 0x22)&&(ArmWriteData.Fault.bit.ATEMode == 0))
            ||((FuncPara.BatPowered && FuncPara.BatState != 0x22))//认为手动关机
            ||(FuncPara.BatPowerTry > 3))
    {
        return;
    }
    bool canActivate = (ArmWriteData.Fault.bit.ATEMode == 1)||(InvStateData.WorkState == ByPass ) ||
                           (SampleRms.PV1VoltFilter8 >= PVThreshold && !SystemError.StateErr.bit.PV1Fault) ||
                           (SampleRms.PV2VoltFilter8 >= PVThreshold && !SystemError.StateErr.bit.PV2Fault) ||
                           (SampleRms.PV3VoltFilter8 >= PVThreshold && !SystemError.StateErr.bit.PV3Fault) ||
                           (SampleRms.PV4VoltFilter8 >= PVThreshold && !SystemError.StateErr.bit.PV4Fault);
    if(canActivate)
    {
        POWER_ON;
        FuncPara.BatPowering = 1;
        FuncPara.BatPowerCnt = 0;
    }
}

/*************************************************
Function: CorrectActivePowerAim
Description: 修正有功目标值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
void CorrectActivePowerAim(float L1Target, float L2Target)
{
//    if(fabs(FuncPara.L1PAimOld -L1Target)> 1)
//    {
//        FuncPara.L1AdjustCnt = 0;//下发变化后等待新功率计算出来
//        FuncPara.L1PAimOld = L1Target;
//        FuncPara.L1PAimUse = L1Target;
//    }
//    else
//    {
//        if(FuncPara.L1AdjustCnt < 200)//功率更新了再选择调整方向
//        {
//            FuncPara.L1AdjustCnt++;
//        }
//        else
//        {
//            if(L1Target - SystemPower.Inv1.Average >= 20.0f)
//            {
//                FuncPara.L1PAimUse ++;
//            }
//            else if(L1Target - SystemPower.Inv1.Average <= -20.0f)
//            {
//                FuncPara.L1PAimUse --;
//            }
//            FuncPara.L1AdjustCnt = 0;
//        }
//    }
//
//    if(fabs(FuncPara.L2PAimOld -L2Target)> 1)
//    {
//        FuncPara.L2AdjustCnt = 0;
//        FuncPara.L2PAimOld = L2Target;
//        FuncPara.L2PAimUse = L2Target;
//    }
//    else
//    {
//        if(FuncPara.L2AdjustCnt < 200)
//        {
//            FuncPara.L2AdjustCnt++;
//        }
//        else
//        {
//            if(L2Target - SystemPower.Inv2.Average >= 20.0f)
//            {
//                FuncPara.L2PAimUse ++;
//            }
//            else if(L2Target - SystemPower.Inv2.Average <= -20.0f)
//            {
//                FuncPara.L2PAimUse --;
//            }
//            FuncPara.L2AdjustCnt = 0;
//        }
//    }
//
//    if((FuncPara.L1PAimUse - L1Target)>= 250)
//    {
//        FuncPara.L1PAimUse = L1Target +250;
//    }
//    if((FuncPara.L1PAimUse - L1Target)<= -250)
//    {
//        FuncPara.L1PAimUse = L1Target -250;
//    }
//    if((FuncPara.L2PAimUse - L2Target)>= 250)
//    {
//        FuncPara.L2PAimUse = L2Target +250;
//    }
//    if((FuncPara.L2PAimUse - L2Target)<= -250)
//    {
//        FuncPara.L2PAimUse = L2Target -250;
//    }
//    if(fabs(L1Target)<500)
//	{
//		FuncPara.L1PAimUse = L1Target;
//	}
//	if(fabs(L2Target)<500)
//	{
//		FuncPara.L2PAimUse = L2Target;
//	}
//    FuncPara.L1PAimUse = __fsat(FuncPara.L1PAimUse,3000.0f,-3000.0f);
//    FuncPara.L2PAimUse = __fsat(FuncPara.L2PAimUse,3000.0f,-3000.0f);
}

/*************************************************
Function:主动并离网切换处理
Description:
Input: None
Output: None
Frequency: 10KHz
*************************************************/
void SelfOnToOffHandle(void)
{
    if(InvPhase.ThetaPointR == (SampleIns.PeriodPoint - 32))//继电器动作3ms左右
    {
        ControlRelay(OffGridState);
    }

    if(InvPhase.ThetaPointR == 1 && (InvStateData.MidRlyFlg == 0))//继电器动作3ms左右
    {
        FuncPara.BeOnToOff = 1;
    }

//    if(SystemPara.FrequencySet == 0)//50HZ
//    {
//        if(InvPhase.ThetaPointR == 168)//继电器动作3ms左右
//        {
//            FuncPara.BeOnToOff = 1;
//        }
//    }
//    else//60HZ
//    {
//        if(InvPhase.ThetaPointR == 135)
//        {
//            FuncPara.BeOnToOff = 1;
//        }
//    }
}

/*************************************************
Function:被动并离网切换处理
Description:切离网时电压过高进行放电
Input: None
Output: None
Frequency: 10KHz
*************************************************/
//void ForceOnToOffHandle(void)
//{
////    if(FuncPara.ForceToOff)
////    {
//        FuncPara.ForceToOff = 0;//只进一次
//            DISABLE_INV1PWM;
//            DISABLE_INV2PWM;
//            DISABLE_INV3PWM;
//            DISABLE_INV4PWM;
//
////    }
//
//}

/*************************************************
Function:离网软起处理
Description:
Input: None
Output: None
Frequency: 10KHz
*************************************************/
//void OffGridSoftStart(void)
//{
//    if(InvStateData.OffGridStartCnt <= 100) //过零切换 //|| (InvPhase.ThetaPointR != 1 )
//    {
//        InvStateData.OffGridStartCnt++;
//        UrCtrl.FwStartOver = 0;
//        UsCtrl.FwStartOver = 0;
////        FuncPara.InvOffset = 6.0f ;
////        FuncPara.InvLimit = 5.0f ;
//    }
//    else if(InvPhase.ThetaPointR == 1)
//    {
////        FuncPara.InvOffset -= 0.06f;
////        FuncPara.InvOffset = __fmax(FuncPara.InvOffset,0);
////        FuncPara.InvLimit = __fmin(FuncPara.InvLimit+0.1f,20.0f);
//        UrCtrl.FwStartOver = 1;
//        UsCtrl.FwStartOver = 1;
//    }
//}

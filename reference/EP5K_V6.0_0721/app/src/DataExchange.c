/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: DataExchange.c
Author: tzz
Version : 0.0.2
Date:2022年3月12日
Description: 对外显示数据更新
Function List: 更新对外显示数据,对外数据不会直接给而是通过赋值给 *
* ArmReadData结构体变量统一赋值
***********************************************************/
#include "bsp.h"
#include "Sample.h"
#include "Protect.h"
#include "OnGridInv2.h"
#include "ModbusTable.h"
#include "PowerCalculator.h"
#include "ConfigSystemPara.h"
#include "CalculateAngle.h"
#include "PVStateMachine.h"
#include "LLCStateMachine.h"
#include "InvStateMachine.h"
#include "OffGridInv2.h"
#include "MicroInvmode.h"
#include "Function.h"
extern float InvPowerCalibrate(float val);
void  PVfaultDisplay(void);

/*************************************************
Function: DataExchange
Description: 用于更新 AMR 或者对外调试串口访问的数据,主要包含
1.常规数据的读写 2.AD比例系数读写 3.AD零飘系数修改 4.认证参数的设置
Input: None
Output: None
Frequency: 10ms
*************************************************/
void DataExchange(void)
{
    float pvTotalPower = 0;
    float chgBmsTotalPower = 0;
    float temp = 0;

    CalculateDisplayData();

    // ------------------- 更新外部访问数据值 -----------------------------------
    //0-停机；1-离网运行;2-旁路；3-并网
    if (InvStateData.WorkState == OnGridState)
    {
        ArmReadData.RInvState = 3;
        ArmReadData.InvFreq = CaptureData.AvgFreqR*100;    // 离网时等于系统频率 50 or 60,并网时等于电网频率
        ArmReadData.PrLoad = __fmax(SystemPower.Load1.Average, 0);
        ArmReadData.PsLoad = __fmax(SystemPower.Load2.Average, 0);

    }
    else if (InvStateData.WorkState == ByPass)
    {
        ArmReadData.RInvState = 2;
        ArmReadData.InvFreq = 0;
        ArmReadData.PrLoad = __fmax(SystemPower.Load1.Average, 0);
        ArmReadData.PsLoad = __fmax(SystemPower.Load2.Average, 0);
    }
    else if ((InvStateData.WorkState == OffGridState)||(InvStateData.WorkState == UpsState))
    {
        ArmReadData.RInvState = 1;
        ArmReadData.InvFreq = SystemPara.InvRateFreq*100;
        ArmReadData.PrLoad = SystemPower.ApLoad1.Average;
        ArmReadData.PsLoad = SystemPower.ApLoad2.Average;
    }
    else
    {
        ArmReadData.RInvState = 0;
        ArmReadData.InvFreq = 0;
    }
    ArmReadData.SInvState = ArmReadData.RInvState;
    ArmReadData.GridFreq = CaptureData.AvgFreqR*100;

    // 逆变器输出数据
    ArmReadData.UrINV = SampleDisplayData.Inv1Volt * 10.0f;    // 精度 0.1
    ArmReadData.IrINV = SampleDisplayData.Inv1Curr * 10.0f;    // 精度 0.1
    ArmReadData.UsINV = SampleDisplayData.Inv2Volt * 10.0f;    // 精度 0.1
    ArmReadData.IsINV = SampleDisplayData.Inv2Curr * 10.0f;    // 精度 0.1
    // 逆变显示功率需要修正

    ArmReadData.PrINV = InvPowerCalibrate(SystemPower.Inv1.Average);
    ArmReadData.PsINV = InvPowerCalibrate(SystemPower.Inv2.Average);
    // 电网端数据
    ArmReadData.UrGrid = SampleDisplayData.Grid1Volt * 10.0f;  // 精度 0.1
    ArmReadData.IrGrid = 0;
    ArmReadData.PrGrid = 0;
    ArmReadData.UsGrid = SampleDisplayData.Grid2Volt * 10.0f;  // 精度 0.1
    ArmReadData.IsGrid = 0;
    ArmReadData.PsGrid = 0;
    // 负载端数据
    ArmReadData.UrLoad = SampleDisplayData.Load1Volt *10.0f;
    ArmReadData.IrLoad = SampleDisplayData.Load1Curr * 10.0f;
//    ArmReadData.PrLoad = __fmax(SystemPower.Load1.Average, 0); //离网跟并网需要区分
    ArmReadData.UsLoad = SampleDisplayData.Load2Volt *10.0f;
    ArmReadData.IsLoad = SampleDisplayData.Load2Curr * 10.0f;
//    ArmReadData.PsLoad = __fmax(SystemPower.Load2.Average, 0);

    // 故障告警
    ArmReadData.PCSErr1 = SystemError.Fault1.all;
    ArmReadData.PCSErr2.all = SystemError.Fault2.all;
    ArmReadData.PCSErr2.bit.GFCIHardError=SystemError.Fault2.bit.GFCIHardError;
	ArmReadData.PCSErr2.bit.GFCI=SystemError.Fault2.bit.GFCI;
	
    ArmReadData.PCSErr3 = SystemError.Fault3.all;
    ArmReadData.PCSErr4 = SystemError.Fault4.all;
    if(SystemAlarm.bit.GridVoltL ==1 &&SystemAlarm.bit.LossGrid == 1)
     {
        ArmReadData.PCSAlarm1 = SystemAlarm.all & 0xFFDF;//SystemAlarm.bit.LossGrid=0;
     }
     else
     {
         ArmReadData.PCSAlarm1 = SystemAlarm.all;
     }
   // ArmReadData.PCSAlarm1 = SystemAlarm.all;
    //PV欠压故障频繁上报，只允许PV在正常运行30分钟后异常后5s内上报故障，然后清除。
    PVfaultDisplay();

    ArmReadData.BusVolt =  SampleRms.BUSVoltFilter200 * 10;
    ArmReadData.Bus1Volt = SampleRms.Bus1VoltFilter200 * 10;
    ArmReadData.Bus2Volt = SampleRms.Bus2VoltFilter200 * 10;

    // 逆变允许最大输出功率,默认额定
    ArmReadData.MaxPowerSetR = INV_RATE_POWER;
    ArmReadData.MaxPowerSetS = INV_RATE_POWER;

    // PV采样数据
    ArmReadData.UPV1 = SampleDisplayData.PV1Volt*10.0f;
    ArmReadData.UPV2 = SampleDisplayData.PV2Volt*10.0f;
    ArmReadData.UPV3 = SampleDisplayData.PV3Volt*10.0f;
    ArmReadData.UPV4 = SampleDisplayData.PV4Volt*10.0f;

    ArmReadData.DCCurr1Rms = 0.9*ArmReadData.DCCurr1Rms + 0.1*SampleRms.DCCurr1Rms;
    ArmReadData.DCCurr2Rms = 0.9*ArmReadData.DCCurr2Rms + 0.1*SampleRms.DCCurr2Rms;
    // PV1 功率
    if (SystemPower.PV1.Average != 0)
    {
        ArmReadData.PPV1 = fabs(SystemPower.PV1.Average);   // 限制功率为正,0.97传输效率
        ArmReadData.IPV1 = __fmax(10.0f*SystemPower.PV1.Average/SampleDisplayData.PV1Volt,0); //0.97f;     // 需要校准
    }
    else
    {
        ArmReadData.PPV1 = 0;
        ArmReadData.IPV1 = 0;
    }
    // PV2 功率
    if (SystemPower.PV2.Average != 0)
    {
        ArmReadData.PPV2 = fabs(SystemPower.PV2.Average);// 0.97f;   // 限制功率为正
        ArmReadData.IPV2 = __fmax(10.0f*SystemPower.PV2.Average/SampleDisplayData.PV2Volt,0); //0.97f;     // 需要校准
    }
    else
    {
        ArmReadData.PPV2 = 0;
        ArmReadData.IPV2 = 0;
    }
    // PV3 功率
    if (SystemPower.PV3.Average != 0)
    {
        ArmReadData.PPV3 = fabs(SystemPower.PV3.Average);// 0.97f;                               // 限制功率为正
        ArmReadData.IPV3 = __fmax(10.0f*SystemPower.PV3.Average/SampleDisplayData.PV3Volt,0); //0.97f;     // 需要校准
    }
    else
    {
        ArmReadData.PPV3 = 0;
        ArmReadData.IPV3 = 0;
    }
    // PV4 功率
    if (SystemPower.PV4.Average != 0)
    {
        ArmReadData.PPV4 = fabs(SystemPower.PV4.Average);// 0.97f;                               // 限制功率为正
        ArmReadData.IPV4 = __fmax(10.0f*SystemPower.PV4.Average/SampleDisplayData.PV4Volt,0); //0.97f;     // 需要校准
    }
    else
    {
        ArmReadData.PPV4 = 0;
        ArmReadData.IPV4 = 0;
    }

    // 电网最大充放电电流
    SystemPara.GridChgMaxCurrSet = ArmWriteData.GridChgMaxCurrSet;
    SystemPara.GridDischgMaxCurrSet = ArmWriteData.GridDischgMaxCurrSet;

    // 电池包数量
    SystemPara.BATPackNum = ArmWriteData.BATPackNum;

    // 电网充电功率限值
    chgBmsTotalPower = SystemPara.BMSChgMaxPowerSet;
    pvTotalPower = ArmReadData.PPV1 + ArmReadData.PPV2 + ArmReadData.PPV3 + ArmReadData.PPV4;


    if(ArmWriteData.Fault.bit.ATEMode)
    {
        ArmWriteData.ReConnectDly = 10L*100L;
        Protect.ClrDly.GridReConnectDly = ArmWriteData.ReConnectDly;        // [20,300]  1s
        // 电网过/欠压 告警复位
        Protect.ClrDly.GridOVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        Protect.ClrDly.GridUVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        // 电网过/欠频 告警复位
        Protect.ClrDly.GridOFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        Protect.ClrDly.GridUFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
    }


    // 写数据处理, 默认写使能禁止
    if (SystemPara.ModbusWEnable)
    {
        // 如果老化标签置1需要将电网掉电保护参数放宽
        ModifyGridLossPara();

        // 写常规标志位置1
        if (ArmWriteData.WFlag)
        {
            // 并网状态下写入有效
            if (InvStateData.WorkState == OnGridState)
            {
                if (SystemPara.EnDCAdjust == 1)
                {
#if 1
                    // 逆变1电流直流分量
                    temp = ArmWriteData.DCComponent1*ArmWriteData.DCI1;//1.0f;
	               /* if(fabs(temp)>600)
	                {
	                    temp = __fsat(temp, 250.0f, -250.0f);         // 限值50mA
	                }
					else if(fabs(temp)>300)
	                {
	                    temp = __fsat(temp, 150.0f, -150.0f);         // 限值50mA
	                }
					else
					{
					     temp = __fsat(temp, 100.0f, -100.0f);         // 限值50mA
					}
                    */
                    temp = __fsat(temp, 250.0f, -250.0f);         // 限值50mA
                    IrCtrl.IdcCurr = temp*0.001f;
                    ArmWriteData.DCComponent1 = 0;              // 清零直流分量
                    // 逆变2电流直流分量
                    temp = ArmWriteData.DCComponent2*ArmWriteData.DCI2;//1.0f;
                   /* if(fabs(temp)>600)
	                {
	                    temp = __fsat(temp, 250.0f, -250.0f);         // 限值50mA
	                }
					else if(fabs(temp)>300)
	                {
	                    temp = __fsat(temp, 100.0f, -100.0f);         // 限值50mA
	                }
					else
					{
					     temp = __fsat(temp, 50.0f, -50.0f);         // 限值50mA
					}*/
                    temp = __fsat(temp, 250.0f, -250.0f);         // 限值50mA
                    IsCtrl.IdcCurr = temp*0.001f;               // 转为A
                    ArmWriteData.DCComponent2 = 0;              // 清零直流分量

                    ArmWriteData.DCChangeFlg = 1;
    #endif
                }
                else
                {
                    IrCtrl.IdcCurr = 0;
                    IsCtrl.IdcCurr = 0;
                }
            }

            // 停机时允许修改电压和频率以及并网认证国家
            if (InvStateData.WorkState < 3)
            {
                SystemPara.ParallelEn = ArmWriteData.ParallelEn;  // 停机状态下允许设置并联模式

                SystemPara.CountryID = ArmWriteData.CountryID;    // 0-美国;1-日本

                // 频率设置 0-50Hz;1-60Hz
                if ((ArmWriteData.FrequencySet == 1) || (ArmWriteData.FrequencySet == 0))
                {
                    if(ArmWriteData.FrequencySet != SystemPara.FrequencySet)
                    {
                        ArmWriteData.FreqEnFlag =1;
                    }
                    SystemPara.FrequencySet = ArmWriteData.FrequencySet;//;
                    SystemPara.InvRateFreq = (SystemPara.FrequencySet == 0) ? 50 :60;
                }
            }

            // 有功设置值

            SystemPara.ActivePowerSetRTemp = __fsat((float)ArmWriteData.ActivePowerSetR, 2750.0f, -2750.0f);//SystemPara.ActivePowerSetR
            SystemPara.ActivePowerSetSTemp = __fsat((float)ArmWriteData.ActivePowerSetS, 2750.0f, -2750.0f);
            SystemPara.GridMaxChgPowerLimit = SystemPara.ActivePowerSetR + SystemPara.ActivePowerSetS;
            if (SystemPara.GridMaxChgPowerLimit < 0)
            {
                if (fabs(SystemPara.GridMaxChgPowerLimit) >= (chgBmsTotalPower - pvTotalPower + 1000.0f))
                {
                    ArmWriteData.ActivePowerSetR = 0;
                    ArmWriteData.ActivePowerSetS = 0;
                    SystemPara.ActivePowerSetR = 0;
                    SystemPara.ActivePowerSetS = 0;
                }
            }

            // 无功设置
            SystemPara.InActivePowerSetR = __fsat((float)ArmWriteData.InActivePowerSetR, 4500.0f, -4500.0f);             // 无功功率限制[-4500.0f, 4500.0f]
            SystemPara.InActivePowerSetS = __fsat((float)ArmWriteData.InActivePowerSetS, 4500.0f, -4500.0f);

           // BMS 充放电电压电流设置
//            SystemPara.BMSChgMaxVoltSet = ArmWriteData.BMSChgMaxVoltSet*0.1f;           // BMS 最大充电电压(暂不使用)
            SystemPara.BMSChgMaxPowerSet = ArmWriteData.BMSChgMaxPowerSet;           // BMS 最大充电功率
            SystemPara.BMSDischgMaxPowerSet = ArmWriteData.BMSDischgMaxPowerSet;     // BMS 最大放电功率
            // 开机
            SystemPara.SystemPowerOn = ArmWriteData.SystemPowerOn;

            // ARM故障位
            SystemError.ArmFault = ArmWriteData.Fault.all;

            // 开环发波使能
            SystemPara.OpenLoopEn = ArmWriteData.OpenLoopEn;

            //电池包运行状态
            FuncPara.BatState = ArmWriteData.BatState;

            // 电网增强模式使能
            SystemPara.GridEnhanceEn = ArmWriteData.GridEnhanceEn && (ArmWriteData.AgingLable == 0);

            // 微逆模式
//            SystemPara.MicroInverterMode = ArmWriteData.Model.bit.MicroInvterMode;
            if (SystemPara.MicroInverterMode)
            {
                UrCtrl.ReEnable = 0;
                UsCtrl.ReEnable = 0;
                MicroInvmode.Pmax = ArmWriteData.MicroInvterPowerMax;
                MicroInvmode.PcLimit = chgBmsTotalPower - pvTotalPower - 100.0f;
                MicroInvmode.kOF_P = (MicroInvmode.FH - MicroInvmode.FN - MicroInvmode.OFDeadband) /MicroInvmode.Pmax;
            }
            // 柴油发电机模式
            SystemPara.DieselEngineMode = ArmWriteData.Model.bit.DieselMode;
            // 正常模式下恢复重复控制
            if (SystemPara.MicroInverterMode == 0)
            {
                UrCtrl.ReEnable = 1;
                UsCtrl.ReEnable = 1;
            }

            ArmWriteData.WFlag = 0;

            SystemPara.FanSpeed = ArmWriteData.FanSpeed;
        }

        // PV输入源设置
        if ((PV1WorkState.State != MPPT_STATE) &&
                (PV2WorkState.State != MPPT_STATE) &&
                (PV3WorkState.State != MPPT_STATE)&&
                (PV4WorkState.State != MPPT_STATE))
        {
            PVSystemData.PVSourceType = ArmWriteData.PVSourceType || ArmWriteData.AgingLable;
            // PV 并联模式
            PVSystemData.PVParallel = ArmWriteData.PVParallel;
        }

        // 修改 AD 比例系数
        if (ArmWriteData.WAdKCoefFlag == 1)
        {
           ArmWriteData.WAdKCoefFlag = 2;
           // 采样校准系数,需要在此处做限值处理
           // PV1 电压
           temp = ArmWriteData.ADCoeff.UPV1K;
           temp = __fsat(temp, 7500.0f, 5500.0f);   // 703*0.92~703*1.08
           SampleCalibCoef.PV1VoltK = temp;
           // PV2 电压
           temp = ArmWriteData.ADCoeff.UPV2K;
           temp = __fsat(temp, 7500.0f, 5500.0f);
           SampleCalibCoef.PV2VoltK = temp;
           // PV3 电压
           temp = ArmWriteData.ADCoeff.UPV3K;
           temp = __fsat(temp, 7500.0f, 5500.0f);
           SampleCalibCoef.PV3VoltK = temp;
           // PV4 电压
           temp = ArmWriteData.ADCoeff.UPV4K;
           temp = __fsat(temp, 7500.0f, 5500.0f);
           SampleCalibCoef.PV4VoltK = temp;
//           //PV1 电流
//           temp = ArmWriteData.ADCoeff.IPV1K;
//           temp = __fsat(temp, 5500.0f, 3500.0f);     // 620*0.92~620*1.08
//           SampleCalibCoef.PV1CurrK = temp;
//           // PV2 电流
//           temp = ArmWriteData.ADCoeff.IPV2K;
//           temp = __fsat(temp, 5500.0f, 3500.0f);
//           SampleCalibCoef.PV2CurrK = temp;
//           // PV3 电流
//           temp = ArmWriteData.ADCoeff.IPV3K;
//           temp = __fsat(temp, 5500.0f, 3500.0f);
//           SampleCalibCoef.PV3CurrK = temp;
//           // PV4 电流
//           temp = ArmWriteData.ADCoeff.IPV4K;
//           temp = __fsat(temp, 5500.0f, 3500.0f);
//           SampleCalibCoef.PV4CurrK = temp;

//           // 负载电流1
//           temp = ArmWriteData.ADCoeff.ILoad1K;
//           temp = __fsat(temp, 3000.0f, 1000.0f);
//           SampleCalibCoef.Load1CurrK =  temp;
//           // 负载电流2
//           temp = ArmWriteData.ADCoeff.ILoad2K;
//           temp = __fsat(temp, 3000.0f, 1000.0f);
//           SampleCalibCoef.Load2CurrK =  temp;
        }

        //if(SystemPara.CountryID == 1)
             {
//                 ArmWriteData.WAuthenFlag = 0;
//                 if (SystemPara.GridEnhanceEn)
//				 {
//                	 // 并网过欠压
//				  Protect.Value.GridOV1 = SystemPara.InvRateVolt*1.138f;   // 115V
//				  Protect.Delay.GridOV1Dly = 93L;                         // 930ms
//				  Protect.Value.GridOV2 = Protect.Value.GridOV1;
//				  Protect.Delay.GridOV2Dly = Protect.Delay.GridOV1Dly;
//				  Protect.Value.RecoveryGridOV1 = SystemPara.InvRateVolt*1.109f;   // 116V
//
//				  Protect.Value.GridUV1 = SystemPara.InvRateVolt*0.65f;//80v
//				  Protect.Delay.GridUV1Dly = 200L;                        // 200ms
//				  Protect.Value.GridUV2 = Protect.Value.GridUV1;
//				  Protect.Delay.GridUV2Dly = Protect.Delay.GridUV1Dly;
//				  Protect.Value.RecoveryGridUV1 = SystemPara.InvRateVolt*0.822f;
//				 }
//                 else
//                 if(SystemPara.CountryID == 0)
//                 {
//                     SystemPara.CountryID = 1;
//                	 // 并网过欠压
//                      Protect.Value.GridOV1 = SystemPara.InvRateVolt*1.138f;   // 115V
//                      Protect.Delay.GridOV1Dly = 93L;                         // 930ms
//                      Protect.Value.GridOV2 = Protect.Value.GridOV1;
//                      Protect.Delay.GridOV2Dly = Protect.Delay.GridOV1Dly;
//                      Protect.Value.RecoveryGridOV1 = SystemPara.InvRateVolt*1.109f;   // 116V
//
//                      Protect.Value.GridUV1 = SystemPara.InvRateVolt*0.792f;//80v
//                      Protect.Delay.GridUV1Dly = 93L;                        // 200ms
//                      Protect.Value.GridUV2 = Protect.Value.GridUV1;
//                      Protect.Delay.GridUV2Dly = Protect.Delay.GridUV1Dly;
//                      Protect.Value.RecoveryGridUV1 = SystemPara.InvRateVolt*0.822f;
//                 }

            if(ArmWriteData.FreqEnFlag)
            {
                ArmWriteData.FreqEnFlag = 0;

                // 并网过欠频
                if(SystemPara.FrequencySet == 0)
               {
                   Protect.Value.GridOF1 = SystemPara.InvRateFreq + 1.0f;
                   Protect.Delay.GridOF1Dly = 93L;                       // 200ms
                   // 等级2
                   Protect.Value.GridOF2 = Protect.Value.GridOF1;
                   Protect.Delay.GridOF2Dly = Protect.Delay.GridOF1Dly;
                   Protect.Value.RecoveryGridOF1 = SystemPara.InvRateFreq + 0.8f;
                   // 等级1
                   Protect.Value.GridUF1 = SystemPara.InvRateFreq - 1.5f;
                   Protect.Delay.GridUF1Dly = 93L;                       // 200ms
                   // 等级2
                   Protect.Value.GridUF2 = Protect.Value.GridUF1;
                   Protect.Delay.GridUF2Dly = Protect.Delay.GridUF1Dly;
                   Protect.Value.RecoveryGridUF1 = SystemPara.InvRateFreq - 1.3f;
               }
               else
               {
                   Protect.Value.GridOF1 = SystemPara.InvRateFreq + 1.2f;
                   Protect.Delay.GridOF1Dly = 93L;                       // 200ms
                   // 等级2
                   Protect.Value.GridOF2 = Protect.Value.GridOF1;
                   Protect.Delay.GridOF2Dly = Protect.Delay.GridOF1Dly;
                   Protect.Value.RecoveryGridOF1 = SystemPara.InvRateFreq + 1.0f;
                   // 等级1
                   Protect.Value.GridUF1 = SystemPara.InvRateFreq - 1.8f;
                   Protect.Delay.GridUF1Dly = 93L;                       // 200ms
                   // 等级2
                   Protect.Value.GridUF2 = Protect.Value.GridUF1;
                   Protect.Delay.GridUF2Dly = Protect.Delay.GridUF1Dly;
                   Protect.Value.RecoveryGridUF1 = SystemPara.InvRateFreq - 1.6f;
               }
            }
         }
    }
}


typedef struct
{
    long   CheckStrat;
    long   CheckStratCnt;
    long   LowFlagCnt;
    long   CheckfaultFlag;
    long   flag;
}PV_FAULT_DISP;

void  PVfaultDisplay(void)
{
    #define PvStartCntTime  1800
    #define PvFaultTime   5//1800
    static Uint32 msTimerPvCheckStrat = 0;
    //pv上报滤波，防止弱光频繁上报   30分 = 1800s = 1800 * 1000ms
    static PV_FAULT_DISP PVDisplay1,PVDisplay2,PVDisplay3,PVDisplay4;

    if(User_usTimer(&msTimerPvCheckStrat, 1E6L))//1s定时
    {
        //
        PVDisplay1.flag = SystemAlarm.bit.PV1Low;
        PVDisplay2.flag = SystemAlarm.bit.PV2Low;
        PVDisplay3.flag = SystemAlarm.bit.PV3Low;
        PVDisplay4.flag = SystemAlarm.bit.PV4Low;

        if(PV1WorkState.State == MPPT_STATE)       //100u
        {
            PVDisplay1.CheckStratCnt++;
            if(PVDisplay1.LowFlagCnt)
            {
                PVDisplay1.LowFlagCnt = 0;
                PVDisplay1.CheckStratCnt = 0;
            }
        }
        else if(PVDisplay1.flag && PVDisplay1.LowFlagCnt++ >= PvFaultTime)    //故障上报维持20s
        {
            PVDisplay1.LowFlagCnt = PvFaultTime;
            PVDisplay1.CheckStratCnt = 0;
        }

        if(PV2WorkState.State == MPPT_STATE)
        {
            PVDisplay2.CheckStratCnt++;
            if(PVDisplay2.LowFlagCnt)
            {
                PVDisplay2.LowFlagCnt = 0;
                PVDisplay2.CheckStratCnt = 0;
            }
        }
        else if(PVDisplay2.flag && PVDisplay2.LowFlagCnt++ >= PvFaultTime)    //故障上报维持20s
        {
            PVDisplay2.LowFlagCnt = PvFaultTime;
            PVDisplay2.CheckStratCnt = 0;
        }

        if(PV3WorkState.State == MPPT_STATE)
        {
            PVDisplay3.CheckStratCnt++;
            if(PVDisplay3.LowFlagCnt)
            {
                PVDisplay3.LowFlagCnt = 0;
                PVDisplay3.CheckStratCnt = 0;
            }
        }
        else if(PVDisplay3.flag && PVDisplay3.LowFlagCnt++ >= PvFaultTime)    //故障上报维持20s
        {
            PVDisplay3.LowFlagCnt = PvFaultTime;
            PVDisplay3.CheckStratCnt = 0;
        }

        if(PV4WorkState.State == MPPT_STATE)
        {
            PVDisplay4.CheckStratCnt++;
            if(PVDisplay4.LowFlagCnt)
            {
                PVDisplay4.LowFlagCnt = 0;
                PVDisplay4.CheckStratCnt = 0;
            }
        }
        else if(PVDisplay4.flag && PVDisplay4.LowFlagCnt++ >= PvFaultTime)    //故障上报维持20s
        {
            PVDisplay4.LowFlagCnt = PvFaultTime;
            PVDisplay4.CheckStratCnt = 0;
        }
    }

    if(PVDisplay1.CheckStratCnt >= PvStartCntTime)
    {
        PVDisplay1.CheckStratCnt = PvStartCntTime;
    }
    else
    {
        ArmReadData.PCSAlarm1 &= 0xffbf;
    }
    if(PVDisplay2.CheckStratCnt >= PvStartCntTime)
    {
        PVDisplay2.CheckStratCnt = PvStartCntTime;
    }
    else
    {
        ArmReadData.PCSAlarm1 &= 0xff7f; //
    }
    if(PVDisplay3.CheckStratCnt >= PvStartCntTime)
    {
        PVDisplay3.CheckStratCnt = PvStartCntTime;
    }
    else
    {
        ArmReadData.PCSAlarm1 &= 0xfeff; //
    }
    if(PVDisplay4.CheckStratCnt >= PvStartCntTime)
    {
        PVDisplay4.CheckStratCnt = PvStartCntTime;
    }
    else
    {
        ArmReadData.PCSAlarm1 &= 0xdfff; //
    }
}

/*
 * BUSStateMachine.c
 *
 *  Created on: 2025年5月21日
 *      Author: 86130
 */
#include <string.h>
#include "bsp.h"
#include "rmp.h"
#include "stdlib.h"
#include "MPPTFunc.h"
#include "Sample.h"
#include "Protect.h"
#include "InvStateMachine.h"
#include "BUSStateMachine.h"
#include "ConfigSystemPara.h"
#include "ModbusTable.h"
#include "CNTL_PI_F.h"
#include "Function.h"

BUS_SYSTEM_DATA BusSystemData;
BUS_STATE_DATA BusStateData;
CNTL_PI_F BusVoltPIF;
RMPCNTL BusDataSet;

void InitBusBalanceData(void)
{
    BusVoltPIF.Kp = 0.001;
    BusVoltPIF.Ki = 0.0001;
    BusVoltPIF.Umax = 0.1f;//40
    BusVoltPIF.Umin = -0.1f;//40
	memset(&BusStateData, 0, sizeof(BUS_STATE_DATA));
	memset(&BusSystemData, 0, sizeof(BUS_SYSTEM_DATA));

	BusSystemData.KDP = 0.0005;
}

void CheckBUSBalanceState(void)
{
    BusSystemData.DeltaBusIns = SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns;
    BusSystemData.DeltaBusAvg8 = fabsf(SampleRms.Bus1VoltFilter8 - SampleRms.Bus2VoltFilter8);
    BusSystemData.DeltaBusAvg200 = SampleRms.Bus1VoltFilter200 - SampleRms.Bus2VoltFilter200;

    if(InvStateData.WorkState != OnGridState)
    {
        static int Pcnt = 0;

//         //中点平衡使能
//         if(BusSystemData.DeltaBusIns > 2.0f)  //一个周期中点不平
//         {
//            Pcnt++;
//            if(Pcnt > (10e3/50.0f))
//               Pcnt = (10e3/50.0f + 5.0f);
//         }
//         else
//         {
//            Pcnt--;
//            if(Pcnt < (10e3/50.0f))
//               Pcnt = 0;
//         }
//
//        if(BusSystemData.DeltaBusAvg8 >= 7.0f)
//        {
//            BusSystemData.BusStartFlag = 1; //启动平衡电路
//        }
//        else if(Pcnt > (10e3/50.0f))
//        {
//            BusSystemData.BusStartFlag = 1; //启动平衡电路
//        }
////        else if(BusSystemData.DeltaBusIns >= 8.0f)
////        {
////            BusSystemData.BusStartFlag = 1; //启动平衡电路
////        }
//        else if((BusSystemData.DeltaBusIns <= 3.0f)
//                &&(BusSystemData.DeltaBusAvg8 <= 1.0f))
//        {
//            BusSystemData.BusStartFlag = 0; //调节结束
//        }

        BusSystemData.BusStartFlag = 1; //启动平衡电路
    }
    else
    {
        BusSystemData.BusStartFlag = 1; //启动平衡电路

//        if((fabsf(SystemPara.ActivePowerSetR) < 300.0f)&&(fabsf(SystemPara.ActivePowerSetS) < 300.0f))
//        {
//        	if(BusSystemData.DeltaBusAvg8 >= 7.0f)
//			{
//				BusSystemData.BusStartFlag = 1; //启动平衡电路
//			}
//			else if(BusSystemData.DeltaBusAvg8 <= 3.0f)
//			{
//				BusSystemData.BusStartFlag = 0; //调节结束
//			}
//        }
//        else
//        {
//            if(BusSystemData.DeltaBusAvg8 >= 4.0f)
//            {
//                BusSystemData.BusStartFlag = 1; //启动平衡电路
//            }
//            else if(BusSystemData.DeltaBusIns >= 5.0f)
//            {
//                BusSystemData.BusStartFlag = 1; //启动平衡电路
//            }
//            else if((BusSystemData.DeltaBusAvg8 <= 2.0f)
//                    &&(BusSystemData.DeltaBusIns <= 3.0f))
//            {
//                BusSystemData.BusStartFlag = 0; //调节结束
//            }
//        }
    }

    if((SampleRms.Bus1VoltFilter8 < 20.0f)||(SampleRms.Bus2VoltFilter8 < 20.0f))
    {
        if(BusSystemData.StartBusCnt >= 5*1000*10L)
        {
            BusSystemData.StartBusCnt = 0;
            BusSystemData.BusStartFlag = 0; //调节结束
        }
    }
    if(SystemError.StateErr.bit.SystemFault)
    {  
        BusSystemData.BusStartFlag = 0; //调节结束
        BusStateData.StartState = Error_State;
    }
}


/*************************************************
Function: BusOCP
Description: 平衡电路驱动
Input: None
Output: None
Frequency: 10K
*************************************************/
inline void SwapBUSPWM(char polar)
{
    if(polar)
    {
        EALLOW;
        if(EPwm8Regs.DBCTL.bit.OUTSWAP == 3 && EPwm8Regs.AQCSFRC.bit.CSFA == 0)
        {
            EPwm8Regs.CMPA.bit.CMPA = 1;
            EPwm8Regs.AQCSFRC.bit.CSFA = 1;
        }
        else
        {
            EPwm8Regs.AQCSFRC.bit.CSFA = 0;
            EPwm8Regs.DBCTL.bit.OUTSWAP = 0;
        }
        EDIS;
    }
    else
    {
        EALLOW;
        if(EPwm8Regs.DBCTL.bit.OUTSWAP == 0 && EPwm8Regs.AQCSFRC.bit.CSFA == 0)
        {
            EPwm8Regs.CMPA.bit.CMPA = 1;
            EPwm8Regs.AQCSFRC.bit.CSFA = 1;
        }
        else
        {
            EPwm8Regs.AQCSFRC.bit.CSFA = 0;
            EPwm8Regs.DBCTL.bit.OUTSWAP = 3;
        }
        EDIS;
    }
}

void RunBusState3(void)
{
    EPwm8Regs.AQCSFRC.bit.CSFB = 2;
    switch(BusStateData.StartState)
    {
        case Init_State:
        {
           // DISABLE_BUSPWM;
			EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;
            EDIS;
            // 检查零飘检测是否结束
            if (SampleIns.CalSampleZeroFlag == 1)
            {
                BusStateData.StartState = Work_State;
            }
            break;
        }
        case Work_State:
        {
            CheckBUSBalanceState();
          // GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1 ; //过零信号来一次跳一次
            if(BusSystemData.BusStartFlag == 1)
            {
              //  ENABLE_BUSPWM;
				EALLOW;
			    EPwm8Regs.TZCLR.bit.OST = 1;
			    EDIS;
                //if(InvStateData.WorkState != OnGridState)
                {
                    BusVoltPIF.Ref = SampleRms.Bus1VoltFilter200;
                    BusVoltPIF.Fbk = SampleRms.Bus2VoltFilter200;

//                    BusVoltPIF.Ref = SampleIns.Bus1VoltIns;
//                    BusVoltPIF.Fbk = SampleIns.Bus2VoltIns;

                    CNTL_PI_F_MACRO(BusVoltPIF);

                    //快速响应
//                    if(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns > 60)  //开启上管
//                    {
//                        if(BusVoltPIF.Out < BusVoltPIF.Umax)
//                        {
//                            BusVoltPIF.i1 = BusVoltPIF.Umax;
//                            BusVoltPIF.Out = BusVoltPIF.Umax;
//                        }
//                    }
//                    else if(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns < -60)  //开启上管
//                    {
//                        if(BusVoltPIF.Out > -BusVoltPIF.Umax)
//                        {
//                            BusVoltPIF.Out = -BusVoltPIF.Umax;
//                            BusVoltPIF.i1 = -BusVoltPIF.Umax;
//                        }
//                    }

                    BusSystemData.KIN = __fsat(1 + (fabs(SampleIns.LnCurrIns) - 15)*0.4,8,1);
                    BusSystemData.KIAVE = 0.003 + fabsf(SampleRms.LnDCCurrFilter200)*0.003;
//                    BusSystemData.DutyData = (BusVoltPIF.Out - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN) * BUS_PWM_PERIOD;
//                    BusSystemData.DutyData =  BusVoltPIF.Out * BUS_PWM_PERIOD;


//                    if(BusVoltPIF.Out >  0.045)
//                    {
//                        BusSystemData.DutyData =  ((BusVoltPIF.Out - 0.045) + 0.45) * BUS_PWM_PERIOD;
//                    }
//                    else if(BusVoltPIF.Out < -0.045)
//                    {
//                        BusSystemData.DutyData =  ((BusVoltPIF.Out + 0.045) - 0.45) * BUS_PWM_PERIOD;
//                    }
//                    else
//                    {
//                        BusSystemData.DutyData =  BusVoltPIF.Out *(10 * BUS_PWM_PERIOD);
//                    }
                    static int BusVoltInsFlag = 0;

//                   if(fabs(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns - SampleIns.Inv1CurrIns - SampleIns.Inv2CurrIns) > 60)
                    if(fabs(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) > 60)
                    {
                        BusVoltInsFlag = 1;
                    }
                    else if(fabs(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) < 20)
                    {
                        BusVoltInsFlag = 0;
                    }

                    if(BusVoltInsFlag)
                    {
                        if(SampleIns.Bus1VoltIns > SampleIns.Bus2VoltIns)  //开启上管
                        {
                            SwapBUSPWM(1);

                            BusSystemData.DutyData = __fmin((SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) * BusSystemData.KIAVE + 0.45,0.55);

                            EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fsat((BusSystemData.DutyData - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.6,0.02);
                        }
                        else if(SampleIns.Bus1VoltIns < SampleIns.Bus2VoltIns)  //开启上管
                        {
                            SwapBUSPWM(0);

                            BusSystemData.DutyData = __fmin((SampleIns.Bus2VoltIns - SampleIns.Bus1VoltIns) * BusSystemData.KIAVE + 0.45,0.55);

                            EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fsat((BusSystemData.DutyData +  SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.6,0.02);
                        }
                    }
                    else if(((SampleRms.Bus1VoltFilter200 > (SampleRms.Bus2VoltFilter200 + 2)) && (SampleIns.Bus1VoltIns > (SampleIns.Bus2VoltIns + 0.1))))
//                    if((BusVoltPIF.Out > 0.02 && (SampleIns.Bus1VoltIns > (SampleIns.Bus2VoltIns + 0.1))))
                    {
                        SwapBUSPWM(1);

                        BusSystemData.DutyData = __fmin((SampleRms.Bus1VoltFilter200 - SampleRms.Bus2VoltFilter200) * BusSystemData.KIAVE + __fsat((SampleRms.Bus1VoltFilter200 - SampleRms.Bus2VoltFilter200) * 0.1 ,0.45,0.00),0.55);
//                        BusSystemData.DutyData = __fmin((SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) * 0.002 + 0.45,0.55);

                        EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fmax((BusSystemData.DutyData - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.02);
                    }
                    else if((((SampleRms.Bus1VoltFilter200 + 2) < SampleRms.Bus2VoltFilter200) && ((SampleIns.Bus1VoltIns + 0.1) < SampleIns.Bus2VoltIns)))
//                    else if((BusVoltPIF.Out < -0.02 && ((SampleIns.Bus1VoltIns + 0.1) < SampleIns.Bus2VoltIns)))//开启下管
                    {
                        SwapBUSPWM(0);
                        //TODO 半波加入积分
                        BusSystemData.DutyData = __fmin((SampleRms.Bus2VoltFilter200 - SampleRms.Bus1VoltFilter200) * BusSystemData.KIAVE +  __fsat((SampleRms.Bus2VoltFilter200 - SampleRms.Bus1VoltFilter200) * 0.1 ,0.45,0.00),0.55);

//                        BusSystemData.DutyData = __fmin((SampleIns.Bus2VoltIns - SampleIns.Bus1VoltIns) * 0.002 + 0.45,0.55);

                        EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD * __fmax((BusSystemData.DutyData + SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.02);
                    }
                    else   //开启上管
                    {
                        BusSystemData.DutyData = 0;
                        EPwm8Regs.CMPA.bit.CMPA = 1;
                        EPwm8Regs.AQCSFRC.bit.CSFA = 1;
                    }

//                    EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD_HALF + BusSystemData.DutyData;
                }
            }
            else
            {   EALLOW;
                EPwm8Regs.TZFRC.bit.OST = 1;
                EDIS;
                BusVoltPIF.i1 = 0;
                BusSystemData.DutyData = 0;
                BusDataSet.TargetValue = 0;
            }
            break;
        }
        case Error_State:
        {
            //DISABLE_BUSPWM;
			EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;         // 初始化时立即关闭PWM输出
            EPwm8Regs.CMPA.bit.CMPA = 0;
            EDIS;
            if(SystemError.Fault1.bit.BusHigh == 0)
            {
                BusSystemData.Count++;
                if(BusSystemData.Count >= 10000)
                {
                    BusSystemData.Count = 0;
                    BusStateData.StartState = Work_State;
                }
            }
            break;
        }
    }
}



#define Bipolar   0
//双极性
#if(Bipolar)
void RunBusState(void)
{   
    switch(BusStateData.StartState)
    {
        case Init_State:
        {
           // DISABLE_BUSPWM;
            EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;         
            EDIS;
            // 检查零飘检测是否结束
            if (SampleIns.CalSampleZeroFlag == 1)
            {
                BusStateData.StartState = Work_State;
            }
            break;
        }
        case Work_State:
        {
            CheckBUSBalanceState();
            if(BusSystemData.BusStartFlag == 1)
            {
              //  ENABLE_BUSPWM;
                EALLOW;
                EPwm8Regs.TZCLR.bit.OST = 1;
                EDIS;

                BusSystemData.FoldCurr= fabsf(FuncPara.INVIAVG1 + FuncPara.INVIAVG2);
                BusSystemData.KIN = __fsat((fabs(SampleIns.LnCurrIns) - 20)*0.4,8,0);
                BusSystemData.KIAVE = 0.003 + fabsf(SampleRms.LnDCCurrFilter200)*0.003;
//                    BusSystemData.DutyData = (BusVoltPIF.Out - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN) * BUS_PWM_PERIOD;
//                    BusSystemData.DutyData =  BusVoltPIF.Out * BUS_PWM_PERIOD;
                BusSystemData.DutyData = 0.5;
                EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fsat((BusSystemData.DutyData -  SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.6,0.4);
            }
            else
            {
                EALLOW;
                EPwm8Regs.TZFRC.bit.OST = 1;        
                EDIS;
                BusVoltPIF.i1 = 0;
                BusSystemData.DutyData = 0;
                BusDataSet.TargetValue = 0;
            }
            break;
        }
        case Error_State:
        {
            //DISABLE_BUSPWM;
            EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;         // 初始化时立即关闭PWM输出
            EPwm8Regs.CMPA.bit.CMPA = 0;
            EDIS;
            if(SystemError.Fault1.bit.BusHigh == 0)
            {
                BusSystemData.Count++;
                if(BusSystemData.Count >= 10000)
                {
                    BusSystemData.Count = 0;
                    BusStateData.StartState = Work_State;
                }
            }
            break;
        }
    }
}
#else
void RunBusState(void)
{
    EPwm8Regs.AQCSFRC.bit.CSFB = 2;
    switch(BusStateData.StartState)
    {
        case Init_State:
        {
           // DISABLE_BUSPWM;
          EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;
            EDIS;
            // 检查零飘检测是否结束
            if (SampleIns.CalSampleZeroFlag == 1)
            {
                BusStateData.StartState = Work_State;
            }
            break;
        }
        case Work_State:
        {
            CheckBUSBalanceState();
          // GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1 ; //过零信号来一次跳一次
            if(BusSystemData.BusStartFlag == 1)
            {
              //  ENABLE_BUSPWM;
              EALLOW;
              EPwm8Regs.TZCLR.bit.OST = 1;
              EDIS;
                //if(InvStateData.WorkState != OnGridState)
                {
                    BusVoltPIF.Ref = SampleRms.Bus1VoltFilter200;
                    BusVoltPIF.Fbk = SampleRms.Bus2VoltFilter200;

//                    BusVoltPIF.Ref = SampleIns.Bus1VoltIns;
//                    BusVoltPIF.Fbk = SampleIns.Bus2VoltIns;

                    CNTL_PI_F_MACRO(BusVoltPIF);

                    BusSystemData.KIN = __fsat(1 + (fabs(SampleIns.LnCurrIns) - 15)*0.4,8,1);
                    BusSystemData.KIAVE = 0.002; // + fabsf(SampleRms.LnDCCurrFilter200)*0.003;
                    BusSystemData.FoldCurr= fabsf(FuncPara.INVIAVG1 + FuncPara.INVIAVG2);
//                    BusSystemData.DutyData = (BusVoltPIF.Out - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN) * BUS_PWM_PERIOD;
//                    BusSystemData.DutyData =  BusVoltPIF.Out * BUS_PWM_PERIOD;

                    static int BusVoltInsFlag = 0;

//                    if(fabsf(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns - SampleIns.Inv1CurrIns - SampleIns.Inv2CurrIns) > 80)
//                    {
//                        BusVoltInsFlag = 1;
//                    }
//                    else if(fabsf(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns - SampleIns.Inv1CurrIns - SampleIns.Inv2CurrIns) < 60)
//                    {
//                        BusVoltInsFlag = 0;
//                    }
                    if(fabs(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) > 65 && InvStateData.WorkState != OnGridState)
                    {
                        BusVoltInsFlag = 1;
                    }
                    else if(fabs(SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) < 20)
                    {
                        if(BusSystemData.FoldCurr < 5)
                        {
                            BusVoltInsFlag = 0;
                        }
                    }

                    if(BusVoltInsFlag)
                    {
                        if(SampleIns.Bus1VoltIns > SampleIns.Bus2VoltIns)  //开启上管
                        {
                            SwapBUSPWM(1);

                            BusSystemData.DutyData = __fmin((SampleIns.Bus1VoltIns - SampleIns.Bus2VoltIns) * BusSystemData.KIAVE + 0.5,0.55);

                            EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fsat((BusSystemData.DutyData - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.55,0.02);
                        }
                        else if(SampleIns.Bus1VoltIns < SampleIns.Bus2VoltIns)  //开启上管
                        {
                            SwapBUSPWM(0);

                            BusSystemData.DutyData = __fmin((SampleIns.Bus2VoltIns - SampleIns.Bus1VoltIns) * BusSystemData.KIAVE + 0.5,0.55);

                            EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fsat((BusSystemData.DutyData +  SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.55,0.02);
                        }
                    }
                    else if(((BusSystemData.DeltaBusAvg200 > 1) && (SampleIns.Bus1VoltIns > SampleIns.Bus2VoltIns)))
                    {
                        SwapBUSPWM(1);

                        BusSystemData.DutyData = __fsat(BusSystemData.DeltaBusAvg200 * 0.05,0.45,0.02); //0.06 -> 0.03

                        if(BusSystemData.DeltaBusAvg200 > 15 && BusSystemData.DeltaBusIns > 15 && InvStateData.WorkState != OnGridState)
                        {
                            BusSystemData.DutyData += __fmin((BusSystemData.DeltaBusAvg200-15)*0.004, 0.05)
                                                   +  __fmin(BusSystemData.FoldCurr*0.05,0.05);
                        }

                        EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD *__fmax((BusSystemData.DutyData - SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.02);
                    }
                    else if(((BusSystemData.DeltaBusAvg200 < -1) && (SampleIns.Bus1VoltIns < SampleIns.Bus2VoltIns)))
                    {
                        SwapBUSPWM(0);   //TODO 半波加入积分

                        BusSystemData.DutyData = __fsat(-BusSystemData.DeltaBusAvg200 * 0.05,0.45,0.02);
//
                        if(BusSystemData.DeltaBusAvg200 < -15 && BusSystemData.DeltaBusIns < -15 && InvStateData.WorkState != OnGridState)
                        {
                            BusSystemData.DutyData +=  __fmin((-15 - BusSystemData.DeltaBusAvg200)*0.004, 0.05)
                                                   +   __fmin(BusSystemData.FoldCurr*0.05,0.05);
                        }
                        EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD * __fmax((BusSystemData.DutyData + SampleIns.LnCurrIns* BusSystemData.KDP * BusSystemData.KIN),0.02);
                    }
                    else   //开启上管
                    {
                        BusSystemData.DutyData = 0;
                        EPwm8Regs.CMPA.bit.CMPA = 1;
                        EPwm8Regs.AQCSFRC.bit.CSFA = 1;
                    }
                }
            }
            else
            {   EALLOW;
                EPwm8Regs.TZFRC.bit.OST = 1;
                EDIS;
                BusVoltPIF.i1 = 0;
                BusSystemData.DutyData = 0;
                BusDataSet.TargetValue = 0;
            }
            break;
        }
        case Error_State:
        {
            //DISABLE_BUSPWM;
          EALLOW;
            EPwm8Regs.TZFRC.bit.OST = 1;         // 初始化时立即关闭PWM输出
            EPwm8Regs.CMPA.bit.CMPA = 0;
            EDIS;
            if(SystemError.Fault1.bit.BusHigh == 0)
            {
                BusSystemData.Count++;
                if(BusSystemData.Count >= 10000)
                {
                    BusSystemData.Count = 0;
                    BusStateData.StartState = Work_State;
                }
            }
            break;
        }
    }
}
#endif
/*************************************************
Function: BusOCP
Description: 平衡电路过流检测，该故障不恢复
Input: None
Output: None
Frequency: 10K
*************************************************/
void BusOCP(void)
{
    if(++BusSystemData.OCPWindow <= 400)
    {
        if (BUS_OC_DATA == 0)
        {
            if(++BusSystemData.OCPCnt >= 200)
            {
                if(BusSystemData.L1OCPCnt > 75)
                {
                    SystemError.Fault1.bit.L1OverCurrH = 1;
                }
                else if(BusSystemData.L2OCPCnt > 75)
                {
                    SystemError.Fault1.bit.L2OverCurrH = 1;
                }
                else
                {
                    SystemError.Fault1.bit.BalanceOC = 1;
                }
            }
        }

        if (FuncPara.OCL1 == 1)
        {
            BusSystemData.L1OCPCnt++;
        }

        if (FuncPara.OCL2 == 1)
        {
            BusSystemData.L2OCPCnt++;
        }
    }
    else
    {
        BusSystemData.OCPWindow = 0;
        BusSystemData.OCPCnt = 0;
        BusSystemData.L1OCPCnt = 0;
        BusSystemData.L2OCPCnt = 0;
    }

    //TODO SystemError.Fault1.bit.BalanceOC = 0;

    //TODO 增加平衡电流有效值过流保护

}




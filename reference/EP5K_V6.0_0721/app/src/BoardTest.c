/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: BoardTest.c
Author: tzz
Date:2022年2月15日
Description: CAN上位机通信:数据读写
 * 注意：测试时序要求比较严时，不要把对比的两个变量分别放到第一组1~6和第二组7~12当中，
 * 否则时间上有100us的延时
 * 要放到同一个组中进行对比
Version:0.0.2
Function List:1.通过BoardTest()将数据实时上传;2.通过ParseCanBusData()
 * 或者 * ParseCanBusData2()函数实时响应CAN上位机指令
***********************************************************/
#include "bsp.h"
#include "User_can.h"
#include "BoardTest.h"
#include "Sample.h"
#include "CalculateAngle.h"
#include "LLCStateMachine.h"
#include "OffGridInv2.h"
#include "OnGridInv2.h"
#include "PVStateMachine.h"
#include "MPPTFunc.h"
#include "InvStateMachine.h"
#include "ConfigSystemPara.h"
#include "PowerCalculator.h"
#include "Protect.h"
#include "ModbusTable.h"

extern MPPT_DATA Mppt1,Mppt2,Mppt3;
extern RMPCNTL PV1CurrSet,PV2CurrSet,PV3CurrSet;
extern RMPCNTL PV1VoltSet,PV2VoltSet,PV3VoltSet;

Uint16 BoardTestType = 3;
extern Uint32 RunPeriod;

extern uint16_t DeadBandComp;

extern float SetThetaRCmpent;
extern float SetThetaSCmpent;

extern CBCEVT_DATA INV1CBCData;

TEMP_DEBUG_DATA DebugData;

/*************************************************
Function: BoardTest
Description: 用于CAN上位机录波数据选择,通过设置BoardTestType变量
观察不同变量
Input: None
Output: None
Frequency: 10KHz
*************************************************/
/*
void BoardTest(void)
{
    // 测试逆变
    if (BoardTestType == 1)
    {
        // CAN 调试通道赋值,通道 1、7 数据长度12位不能表示有符号数据,其他数据长度16位  PV1CurrPIF.Out
        Cantx.CH1 = (int16)(PV1CurrPIF.Out*100);
        Cantx.CH2 = (int16)(PV1VoltPIF.Umax*10);

        Cantx.CH3 = (int16)(PV1VoltPIF.Fbk);
        Cantx.CH4 = (int16)(SampleIns.PV1VoltIns);

        Cantx.CH5 = (int16)(PV1CurrPIF.Ref*10.0f);
        Cantx.CH6 = (int16)(SampleIns.PV1CurrIns*10.0f);

        // CAN
        Cantx.CH7 = (int16)(PV2CurrPIF.Out*100);
        Cantx.CH8 = (int16)(SampleIns.PV3CurrIns*10.0f);//PV2VoltPIF.Umax*10);

        Cantx.CH9 = (int16)(PV2VoltPIF.Fbk);
        Cantx.CH10 = (int16)(SampleIns.PV2VoltIns);

        Cantx.CH11 = (int16)(PV2CurrPIF.Ref*10.0f);
        Cantx.CH12 = (int16)(SampleIns.PV2CurrIns*10.0f);
    }
    else if (BoardTestType == 2)
    {
        // CAN 调试通道赋值,通道 1、7 数据长度12位不能表示有符号数据,其他数据长度16位
        Cantx.CH1 = (int16)(PV3CurrPIF.Out*100);
        Cantx.CH2 = (int16)(PV3VoltPIF.Umax*10);

        Cantx.CH3 = (int16)(PV3VoltPIF.Fbk);
        Cantx.CH4 = (int16)(SampleIns.PV3VoltIns);

        Cantx.CH5 = (int16)(PV3CurrPIF.Ref*10.0f);
        Cantx.CH6 = (int16)(SampleIns.PV3CurrIns*10.0f);

        // CAN
        Cantx.CH7 = InvStateData.WorkState;
        Cantx.CH8 = (int16)(UrCtrl.Out*100.0f);

        Cantx.CH9 = (int16)(InvPhase.SinR*100.0f);
        Cantx.CH10 = (int16)(InvPhase.SinS*100.0f);

        Cantx.CH11 = (int16)(SampleIns.Inv1VoltIns);
        Cantx.CH12 = RunPeriod;
    }
    else if (BoardTestType == 3)
    {
        // 整机运行
        Cantx.CH1 = (int16)(SampleIns.Bus1VoltIns);
        Cantx.CH2 = (int16)(SampleIns.Bus2VoltIns);

        Cantx.CH3 = (int16)(SystemError.Fault1.all);
        Cantx.CH4 = (int16)(SystemError.Fault2.all);

        Cantx.CH5 = (int16)(SystemError.Fault3.all);
        Cantx.CH6 = (int16)(SystemAlarm.all);

        // CAN
        Cantx.CH7 = (InvStateData.WorkState*10);
        Cantx.CH8 = (int16)(SampleIns.BUSVoltIns);

        Cantx.CH9 = (int16)(SampleIns.Inv1VoltIns);
        Cantx.CH10 = (int16)(SampleIns.Inv1CurrIns*10);

        Cantx.CH11 = (int16)(SampleIns.Inv2VoltIns);
        Cantx.CH12 = (int16)(SampleIns.Inv2CurrIns*10);
    }
    // 采样AD原始值
    else if (BoardTestType == 4)
    {
//        // 整机运行
//        Cantx.CH1 = (int16)(SampleRms.Bus1VoltFilter8);
//        Cantx.CH2 = (int16)(SampleADRaw.Load1CurrRaw);
//
//        Cantx.CH3 = (int16)(SampleADRaw.Inv1VoltRaw);
//        Cantx.CH4 = (int16)(SampleADRaw.Inv1CurrRaw);
//
//        Cantx.CH5 = (int16)(SampleADRaw.Inv2VoltRaw);
//        Cantx.CH6 = (int16)(SampleADRaw.Inv2CurrRaw);
//
//        // CAN
//        Cantx.CH7 = SampleADRaw.Bus1VoltRaw;
//        Cantx.CH8 = (int16)(SampleADRaw.Bus2VoltRaw);
//
//        Cantx.CH9 = (int16)(SampleADRaw.Grid1VoltRaw);
//        Cantx.CH10 = (int16)(SampleADRaw.Grid2VoltRaw);
//
//        Cantx.CH11 = (int16)(SampleADRaw.Load1CurrRaw);
//        Cantx.CH12 = (int16)(SampleADRaw.Load2CurrRaw);
        Cantx.CH1 = (int16)(SampleIns.Bus1VoltIns);
        Cantx.CH2 = (int16)(SampleIns.Bus2VoltIns);
        Cantx.CH3 = (int16)(SampleIns.Grid1VoltIns);
        Cantx.CH4 = (int16)(SampleIns.Grid2VoltIns);
        Cantx.CH5 = (int16)IrCtrl.Vgff.y[0];
        Cantx.CH6 = (int16)IsCtrl.Vgff.y[0];

        Cantx.CH7 = (InvStateData.WorkState*10);
        Cantx.CH8 = (int16)(SampleIns.BUSVoltIns);

        Cantx.CH9 = (int16)(IrCtrl.Sin*100.0f);
        Cantx.CH10 = (int16)(IsCtrl.Sin*100.0f);
        Cantx.CH11 = (int16)(IrCtrl.ThetaSpll*10.0f);
        Cantx.CH12 = (int16)(IsCtrl.ThetaSpll*10.0f);
    }
    else if (BoardTestType == 5)
    {
        // 整机运行
        Cantx.CH1 = (int16)(SampleRms.Bus1VoltFilter8);
        Cantx.CH2 = (int16)(SampleADRaw.BUSVoltRaw);

        Cantx.CH3 = (int16)(SampleADRaw.PV1VoltRaw);
        Cantx.CH4 = (int16)(SampleADRaw.PV1CurrRaw);

        Cantx.CH5 = (int16)(SampleADRaw.PV2VoltRaw);
        Cantx.CH6 = (int16)(SampleADRaw.PV2CurrRaw);

        // CAN
        Cantx.CH7 = SampleADRaw.PV3VoltRaw;
        Cantx.CH8 = (int16)(SampleADRaw.PV3CurrRaw);

        Cantx.CH9 = (int16)(SampleADRaw.Inv1CurrRaw);
        Cantx.CH10 = (int16)(SampleADRaw.Inv2CurrRaw);
        Cantx.CH11 = (int16)(0);
        Cantx.CH12 = (int16)(0);
    }
    // 并网参数 逆变2
    else if (BoardTestType == 6)
    {
        Cantx.CH1 = (int16)(InvPhase.ThetaPointR);
        Cantx.CH2 = (int16)(IrCtrl.Fw*100.0f);
        Cantx.CH3 = (int16)(IrCtrl.PICtrl.Fbk*10.0f);
        Cantx.CH4 = (int16)(IrCtrl.PICtrl.Ref*10.0f);
        Cantx.CH5 = (int16)(IrCtrl.PICtrl.Out*100.0f);
        Cantx.CH6 = (int16)(IrCtrl.RepCtrl.CfOut*100.0f);

        // CAN
        Cantx.CH7 = InvStateData.WorkState*10;
        Cantx.CH8 = (int16)(InvPhase.SinR*100.0f);
        Cantx.CH9 = (int16)(InvPhase.SinS*100.0f);
        Cantx.CH10 = (int16)(IrCtrl.PWMDuty);
        Cantx.CH11 = (int16)(InvPhase.ThetaR*10.0f);
        Cantx.CH12 = (int16)(IrCtrl.SinTheta*100.0f);
    }

    // 并网参数 逆变1
    else if (BoardTestType == 7)
    {
        Cantx.CH1 = (int16)(SampleIns.Bus2VoltIns);
        Cantx.CH2 = (int16)(SampleIns.Inv2CurrIns*10.0f);

        Cantx.CH3 = (int16)(IsCtrl.PICtrl.Ref*10.0f);
        Cantx.CH4 = (int16)(IsCtrl.Out*100.0f);

        Cantx.CH5 = (int16)(IsCtrl.PICtrl.Out*100.0f);
        Cantx.CH6 = (int16)(IsCtrl.RepCtrl.CfOut*100.0f);

        // CAN
        Cantx.CH7 = (int16)(InvPhase.ThetaPointS);
        Cantx.CH8 = (int16)((IsCtrl.PICtrl.Ref - IsCtrl.PICtrl.Fbk)*100.0f);

        Cantx.CH9 = (int16)(IsCtrl.Fw*100.0f);
        Cantx.CH10 = (int16)(IsCtrl.PWMDuty);

        Cantx.CH11 = (int16)(InvPhase.SinR*100.0f);
        Cantx.CH12 = (int16)(InvPhase.SinS*100.0f);
    }
    else if (BoardTestType == 8)
    {
        Cantx.CH1 = (int16)(SampleRms.Inv1VoltRms);
        Cantx.CH2 = (int16)(SampleRms.Inv1CurrRms*10.0f);

        Cantx.CH3 = (int16)(SampleRms.Inv2VoltRms);
        Cantx.CH4 = (int16)(SampleRms.Inv2CurrRms*10.0f);

        Cantx.CH5 = (int16)(SampleIns.Load1CurrIns*10.0f);
        Cantx.CH6 = (int16)(SampleIns.Load2CurrIns*10.0f);

        // CAN
        Cantx.CH7 = (int16)(SampleRms.Load1VoltRms);
        Cantx.CH8 = (int16)(SampleRms.Load1VoltRms);

        Cantx.CH9 = (int16)(SampleRms.Grid1CurrRms*10.0f);
        Cantx.CH10 = (int16)(SampleRms.Grid2CurrRms*10.0f);

        Cantx.CH11 = (int16)(SampleIns.Inv1CurrIns*10.0f);
        Cantx.CH12 = (int16)(SampleIns.Inv2CurrIns*10.0f);
    }

    else if (BoardTestType == 9)
    {
        Cantx.CH1 = (int16)(InvPhase.ThetaPointR);
        Cantx.CH2 = (int16)(SampleRms.Bus1VoltFilter200);
        Cantx.CH3 = (int16)(UrCtrl.PICtrl.Fbk*10.0f);
        Cantx.CH4 = (int16)(UrCtrl.PICtrl.Ref*10.0f);
        Cantx.CH5 = (int16)(UrCtrl.PICtrl.Out*100.0f);
        Cantx.CH6 = (int16)(UrCtrl.RepCtrl.CfOut*100.0f);

        // CAN
        Cantx.CH7 = InvStateData.WorkState*10;
        Cantx.CH8 = (int16)(SampleIns.Inv1VoltIns);
        Cantx.CH9 = (int16)(InvPhase.SinR*100.0f);
        Cantx.CH10 = (int16)(UrCtrl.PWMDuty);
        Cantx.CH11 = (int16)(UrCtrl.Fw);
        Cantx.CH12 = (int16)(SampleIns.Inv1CurrIns*10.0f);
    }

    else if (BoardTestType == 10)
    {
        Cantx.CH1 = (int16)(SampleIns.Bus2VoltIns);
        Cantx.CH2 = (int16)(UsCtrl.Park.Ds*10.0f);
        Cantx.CH3 = (int16)(SampleIns.Inv2VoltIns*10.0f);
        Cantx.CH4 = (int16)(UsCtrl.PWMDuty*10.0f);
        Cantx.CH5 = (int16)(UsCtrl.PICtrl.Out*100.0f);
        Cantx.CH6 = (int16)(UsCtrl.RepCtrl.CfOut*100.0f);

        // CAN
        Cantx.CH7 = InvStateData.WorkState*10;
        Cantx.CH8 = (int16)(SampleIns.Inv2VoltIns);
        Cantx.CH9 = (int16)(InvPhase.SinS*100.0f);
        Cantx.CH10 = (int16)(UsCtrl.PWMDuty);
        Cantx.CH11 = (int16)(SampleIns.Inv2VoltIns);
        Cantx.CH12 = (int16)(SampleRms.Bus2VoltFilter200);
    }
    else if (BoardTestType == 11)
    {
        Cantx.CH1 = InvPhase.SyncBeginFlag;//(int16)(SampleRms.Bus1VoltFilter8);
        Cantx.CH2 = (int16)(SampleIns.Grid1VoltIns);

        Cantx.CH3 = (int16)(InvPhase.SinR*100.0f);
        Cantx.CH4 = (int16)(CaptureData.RawValueR[1]*0.0001f);

        Cantx.CH5 = (int16)(InvPhase.SinS*100.0f);
        Cantx.CH6 = (int16)(CaptureData.RawValueS[1]*0.0001f);

        // CAN
        Cantx.CH7 = (int16)(InvStateData.WorkState*10);
        Cantx.CH8 = (int16)(SampleIns.Grid2VoltIns);

        Cantx.CH9 = InvPhase.ThetaPointR;//(int16)(IrCtrl.Out*100.0f);
        Cantx.CH10 = InvPhase.ThetaPointS;//(int16)(IsCtrl.Out*100.0f);

        Cantx.CH11 = (int16)(SampleIns.Inv1CurrIns*10.0f);
        Cantx.CH12 = (int16)(SampleIns.Inv2CurrIns*10.0f);
    }
    // 零飘 AD 值
    else if (BoardTestType == 12)
    {
        Cantx.CH1 = SampleZeroCalibrate.PV1CurrZero;
        Cantx.CH2 = SampleZeroCalibrate.PV2CurrZero;

        Cantx.CH3 = SampleZeroCalibrate.PV3CurrZero;
        Cantx.CH4 = 0;//SampleZeroCalibrate.BUSVoltZero;

        Cantx.CH5 = SampleZeroCalibrate.Load1CurrZero;
        Cantx.CH6 = SampleZeroCalibrate.Load2CurrZero;

        // CAN
        Cantx.CH7 = SampleZeroCalibrate.Inv1VoltZero;
        Cantx.CH8 = SampleZeroCalibrate.Inv2VoltZero;

        Cantx.CH9 = SampleZeroCalibrate.Inv1CurrZero;
        Cantx.CH10 = SampleZeroCalibrate.Inv2CurrZero;

        Cantx.CH11 = SampleZeroCalibrate.Grid1VoltZero;
        Cantx.CH12 = SampleZeroCalibrate.Grid2VoltZero;
    }

    // 锁相
    else if (BoardTestType == 13)
    {
        // 整机运行
        Cantx.CH1 = InvStateData.WorkState*10;
        Cantx.CH2 = (int16)(CaptureData.FreqR*100.0f);

        Cantx.CH3 = GridError.Grid1Err.all;//(int16)(InvPhase.SinR*100.0f);//(SystemError.Fault1.all);
        Cantx.CH4 = GridError.Grid2Err.all;//(int16)(InvPhase.SinS*100.0f);//(SystemError.Fault2.all);

        Cantx.CH5 = (int16)(IslandDataR.AvgFreq*100.0f);// SystemError.Fault3.all);
        Cantx.CH6 = (int16)(CaptureData.CmpDataR*0.001f);

        // CAN
        Cantx.CH7 = (int16)(SampleRms.Grid1VoltRms);
        Cantx.CH8 = (int16)(SampleRms.Grid2VoltRms);

        Cantx.CH9 = (int16)(SampleIns.Grid1VoltIns);
        Cantx.CH10 = (int16)(SampleIns.Grid2VoltIns);

        Cantx.CH11 = (int16)(CaptureData.FreqS*100.0f);
        Cantx.CH12 = (int16)(CaptureData.CmpDataS*0.001f);
    }
}*/
/*************************************************
Function: ParseCanBusData
Description: 用于执行CAN上位机指令,id是指令编号data是写入的值
Input: @id-指令编号;@data-写入的值
Output: None
Frequency: 10KHz
*************************************************/
/*
void ParseCanBusData(uint32_t id, float data)
{
    ClearCANCommunicationFaultCount();

    switch(id)
    {
        case 0:
        {
            data = __fsat(data, 1.0f, 0.0f);
            SystemPara.SystemPowerOn = data;
            break;
        }
        case 1:
        {
            BoardTestType = data;
            break;
        }
        case 2:
        {
            data = __fsat(data, 20.0f, 1.0f);
            Mppt3.StepVolt = data;
            break;
        }
        // 风机转速百分比
        case 4:
        {
            data = __fsat(data, 4.0f, 0.0f);
            SystemPara.FanSpeed = data;
            break;
        }
        // ARM 写使能控制
        case 6:
        {
            data = __fsat(data, 1.0f, 0.0f);
            SystemPara.ModbusWEnable = data;
            break;
        }
        case 7:
        {
            data = __fsat(data, 2.0f, 0.0f);
            IslandDataR.AllEnable = data;
            break;
        }
        case 8:
        {
            data = __fsat(data, 2.25f, 0.0f);
            IslandDataR.DeltaF = data;
            IslandDataS.DeltaF = data;
            break;
        }
        case 9:
        {
            data = __fsat(data, 10.0f, 0.0f);
            IslandDataR.TimeCheck = data;
            IslandDataS.TimeCheck = data;
            break;
        }


        // TODO R相 离网控制参数设置
        case 11:        // R-KP
        {
            data = __fsat(data, 1.0f, 0.0f);
            UrCtrl.PICtrl.Kp = data;

            break;
        }
        case 12:        // K-KI
        {
            data = __fsat(data, 1.0f, 0.0f);
            UrCtrl.PICtrl.Ki = data;

            break;
        }
        case 13:        // 前馈系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            UrCtrl.FwCoeff = data;

            break;
        }
        // TODO 重复控制系数
        case 14:            // 使能
        {
            if (data == 1)
            {
                UrCtrl.ReEnable = 1;
            }

            break;
        }
        case 15:            // REP-KP
        {
            data = __fsat(data, 1.0f, 0.0f);
            UrCtrl.RepCtrl.kp = data;

            break;
        }
        case 16:            // REP-ThetaShift
        {
            data = __fsat(data, 10.0f, -10.0f);
            UrCtrl.RepCtrl.ThetaShift = data;

            break;
        }

        // 可选接地继电器开关
        case 17:
        {
            if (data == 1)
            {
                PE_RLY_ON
            }
            else if (data == 2)
            {
                PE_RLY_OFF
            }

            break;
        }

        // TODO S相 离网控制参数设置
        case 21:        // R-KP
        {
            data = __fsat(data, 1.0f, 0.0f);
            UsCtrl.PICtrl.Kp = data;

            break;
        }
        case 22:        // K-KI
        {
            data = __fsat(data, 1.0f, 0.0f);
            UsCtrl.PICtrl.Ki = data;

            break;
        }
        case 23:        // 前馈系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            UsCtrl.FwCoeff = data;

            break;
        }
        // TODO 重复控制系数
        case 24:            // 使能
        {
            if (data <= 1)
            {
                UsCtrl.ReEnable = data;
            }

            break;
        }
        case 25:            // REP-KP
        {
            data = __fsat(data, 1.0f, 0.0f);
            UsCtrl.RepCtrl.kp = data;

            break;
        }
        case 26:            // REP-ThetaShift
        {
            data = __fsat(data, 10.0f, 0.0f);
            UsCtrl.RepCtrl.ThetaShift = data;

            break;
        }

        case 29:            // 目标电流
        {
            data = __fsat(data, 50.0f, -50.0f);
            IsCtrl.RmpCtrl.TargetValue = data;

            break;
        }
        // TODO -R相并网控制
        case 30:            // 目标电流
        {
            data = __fsat(data, 60.0f, -60.0f);
            IrCtrl.RmpCtrl.TargetValue = data;

            break;
        }
        case 31:            // 前馈系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IrCtrl.FwCoeff = data;

            break;
        }
        case 32:            // 重复控制使能
        {
            if (data <= 1)
            {
                IrCtrl.ReEnable = data;
            }
            break;
        }
        case 33:            // 重复控制系数
        {
            data = __fsat(data, 10.0f, -10.0f);
            IrCtrl.RepCtrl.ThetaShift = data;

            break;
        }
        case 34:            // 重复控制系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IrCtrl.RepCtrl.kpSet = data;
            break;
        }
        case 35:            // PI参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IrCtrl.IKpSet = data;

            break;
        }
        case 36:            // PI参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IrCtrl.PICtrl.Ki = data;

            break;
        }
        case 38:            // 有功功率设置
        {
            data = __fsat(data, 4500.0f, -4500.0f);
            SystemPara.ActivePowerSetR = data;
            break;
        }
        case 39:            // 有功功率设置
        {
            data = __fsat(data, 2.0f, -2.0f);
            IrCtrl.IdcSum = data;
            break;
        }

        // TODO -S相并网控制
        case 40:            // 目标电流
        {
            data = __fsat(data, 60.0f, -60.0f);
            IsCtrl.RmpCtrl.TargetValue = data;

            break;
        }
        case 41:            // 前馈系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IsCtrl.FwCoeff = data;

            break;
        }
        case 42:            // 重复控制系数
        {
            if (data == 1)
            {
                IsCtrl.ReEnable = 1;
            }

            break;
        }
        case 43:            // 重复控制系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IsCtrl.RepCtrl.ThetaShift = data;

            break;
        }
        case 44:            // 重复控制系数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IsCtrl.RepCtrl.kpSet = data;
            break;
        }
        case 45:            // PI参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IsCtrl.IKpSet = data;

            break;
        }
        case 46:            // PI参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            IsCtrl.PICtrl.Ki = data;

            break;
        }
        case 48:            // 有功功率设置
        {
            data = __fsat(data, 4500.0f, -4500.0f);
            SystemPara.ActivePowerSetS = data;
            break;
        }
        case 49:            // 有功功率设置
        {
            data = __fsat(data, 2.0f, -2.0f);
            IsCtrl.IdcSum = data;
            break;
        }

        // TODO PV参数设置
        case 50:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV1CurrPIF.Kp = data;
            break;
        }
        case 51:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV1CurrPIF.Ki = data;
            break;
        }
        // 电压环
        case 52:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV1VoltPIF.Kp = data;
            break;
        }
        case 53:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV1VoltPIF.Ki = data;
            break;
        }
        case 54:            // PV 电压参考
        {
            data = __fsat(data, 480.0f, 160.0f);
            PV1VoltSet.TargetValue = data;
            break;
        }
        case 55:            // PV电流参考
        {
            data = __fsat(data, 10.0f, -10.0f);
            PV1CurrSet.TargetValue = data;
            break;
        }
        case 56:            // PV-调试
        {
            data = __fsat(data, 1.0f, 0);
            PV1CurrPIF.Out = data;
            break;
        }
        case 59:
        {
            SystemPara.EnDCAdjust = data;
            break;
        }

        // TODO PV2参数设置
        case 60:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV2CurrPIF.Kp = data;
            break;
        }
        case 61:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV2CurrPIF.Ki = data;
            break;
        }
        // 电压环
        case 62:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV2VoltPIF.Kp = data;
            break;
        }
        case 63:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV2VoltPIF.Ki = data;
            break;
        }
        case 64:            // PV 电压参考
        {
            data = __fsat(data, 450.0f, 120.0f);
            PV2VoltSet.TargetValue = data;
            break;
        }
        case 65:            // PV电流参考
        {
            data = __fsat(data, 10.0f, -10.0f);
            PV2CurrSet.TargetValue = data;
            break;
        }
        case 66:            // PV-调试
        {
            data = __fsat(data, 1.0f, 0);
            PV2CurrPIF.Out = data;
            break;
        }

        // TODO PV2参数设置
        case 70:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV3CurrPIF.Kp = data;
            break;
        }
        case 71:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV3CurrPIF.Ki = data;
            break;
        }
        // 电压环
        case 72:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV3VoltPIF.Kp = data;
            break;
        }
        case 73:            // PI 参数
        {
            data = __fsat(data, 1.0f, 0.0f);
            PV3VoltPIF.Ki = data;
            break;
        }
        case 74:            // PV 电压参考
        {
            data = __fsat(data, 450.0f, 120.0f);
            PV3VoltSet.TargetValue = data;
            break;
        }
        case 75:            // PV电流参考
        {
            data = __fsat(data, 10.0f, -10.0f);
            PV3CurrSet.TargetValue = data;
            break;
        }
        case 76:            // PV-调试
        {
            data = __fsat(data, 1.0f, 0);
            PV3CurrPIF.Out = data;
            break;
        }

        case 81:            // 低压侧死区
        {
            data = __fsat(data, 2.0f, 0.1f);
//            LLCWorkState.LDeathTime = data;
            break;
        }
        case 82:            // 高压侧
        {
            data = __fsat(data, 2.0f, 0.1f);
//            LLCWorkState.HDeathTime = data;
            break;
        }
        case 83:            // 频率
        {
            data = __fsat(data, 97.0f, 60.0f);
//            LLCWorkState.LLCFreq = data;
            break;
        }
        case 84:            // 使能
        {
//            LLCWorkState.LLCParaFlg = data;
            break;
        }

//        case 85:            // 有功功率设置
//        {
//            data = __fsat(data, 0.99f, 0.7f);
//            break;
//        }

        // TODO AD 校准系数----------------------------------------------
        case 100:
        {
            SampleCalibCoef.Bus1VoltK = data;
            break;
        }
        case 101:
        {
            SampleCalibCoef.Bus2VoltK = data;
            break;
        }
        case 102:
        {
            SampleCalibCoef.PV1VoltK = data;
            break;
        }
        case 103:
        {
            SampleCalibCoef.PV2VoltK = data;
            break;
        }
        case 104:
        {
            SampleCalibCoef.PV3VoltK = data;
             break;
        }
        case 105:
        {
            SampleCalibCoef.PV1CurrK = data;
            break;
        }
        case 106:
        {
            SampleCalibCoef.PV2CurrK = data;
            break;
        }
        case 107:
        {
            SampleCalibCoef.PV3CurrK = data;
            break;
        }
        case 108:
        {
            SampleCalibCoef.BUSVoltK = data;
            break;
        }
        case 110:
        {
            SampleCalibCoef.Inv1VoltK = data;
            break;
        }
        case 111:
        {
            SampleCalibCoef.Inv2VoltK = data;
            break;
        }
        case 112:
        {
            SampleCalibCoef.Inv1CurrK = data;
            break;
        }
        case 113:
        {
            SampleCalibCoef.Inv2CurrK = data;
            break;
        }
        case 114:
        {
            SampleCalibCoef.Grid1VoltK = data;
            break;
        }
        case 115:
        {
            SampleCalibCoef.Grid2VoltK = data;
            break;
        }
        case 116:
        {
            SampleCalibCoef.Load1CurrK = data;
            break;
        }
        case 117:
        {
            SampleCalibCoef.Load2CurrK = data;
            break;
        }

        default:
            break;
    }
}*/

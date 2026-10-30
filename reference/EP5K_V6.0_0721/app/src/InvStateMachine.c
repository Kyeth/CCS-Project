/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: InvStateMachine.c
Author: tzz
Version : 0.0.2
Date:2021年10月12日
Description: 逆变状态机控制
Function List: 逆变状态机控制，离网，并网切换
***********************************************************/
#include "bsp.h"
#include "string.h"
#include "Sample.h"
#include "Protect.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "OffGridInv2.h"
#include "OnGridInv2.h"
#include "ConfigSystemPara.h"
#include "ModbusTable.h"
#include "Energy_Manage.h"
#include "Parallel.h"
#include "BoardTest.h"
#include <Function.h>
#include "BUSStateMachine.h"
INV_STATE_STRUCT InvStateData;

/*************************************************
Function: InitProtectData
Description: 保护模块数据初始化
Input: None
Output: None
Frequency: Once
*************************************************/
void InitInvStateMachineData(void)
{
    memset(&InvStateData, 0, sizeof(INV_STATE_STRUCT));
}
/*************************************************
Function: ControlRelay
Description: 根据当前逆变器状态控制继电器
Input: None
Output: None
Frequency:
*************************************************/
void ControlRelay(INV_STATE_TYPE state)
{
    FuncPara.RlyDelay =  0;
    FuncPara.RlyCheckSt1 = 0;
    FuncPara.RlyCheckSt2 = 0;
    FuncPara.RlyCheckSt3 = 0;

    switch(state)
    {
        case InitState:
        case SelfCheckState:
        {
            AUS_RLY_ON;
            BUS_RLY_OFF;
            LOAD_RLY1_OFF
            LOAD_RLY2_OFF
            N_LOAD_RLY_OFF

            MID_L1_RLY_OFF
            MID_L2_RLY_OFF
            N_MID_RLY_OFF

            GRID_L1_RLY_OFF
            GRID_L2_RLY_OFF
            N_GRID_RLY_OFF

            PE_RLY_OFF

            break;
        }
        case ErrorState:
        {
            AUS_RLY_ON;
            BUS_RLY_OFF
            LOAD_RLY1_OFF
            LOAD_RLY2_OFF
            N_LOAD_RLY_OFF

            //断开输出即可
            MID_L1_RLY_OFF
            MID_L2_RLY_OFF
            N_MID_RLY_OFF

            GRID_L1_RLY_OFF
            GRID_L2_RLY_OFF
            N_GRID_RLY_OFF

            PE_RLY_OFF

            break;
        }
        case OffGridState:
        {
            AUS_RLY_OFF;
            BUS_RLY_OFF;
            LOAD_RLY1_ON
            LOAD_RLY2_ON
            N_LOAD_RLY_ON

            MID_L1_RLY_OFF
            MID_L2_RLY_OFF
            N_MID_RLY_OFF

            GRID_L1_RLY_OFF
            GRID_L2_RLY_OFF
            N_GRID_RLY_OFF

            PE_RLY_ON;

            break;
        }
        case UpsState:
        {
            AUS_RLY_OFF;
            BUS_RLY_OFF;
            LOAD_RLY1_ON
            LOAD_RLY2_ON
            N_LOAD_RLY_ON

            MID_L1_RLY_OFF
            MID_L2_RLY_OFF
            N_MID_RLY_OFF

            GRID_L1_RLY_OFF
            GRID_L2_RLY_OFF
            N_GRID_RLY_OFF

            PE_RLY_OFF

            break;
        }
        case OnGridState:
        {
//            AUS_RLY_OFF;  //不能马上断开，wait LOAD_RLY1_ON
            BUS_RLY_OFF;    //不能马上断开，wait LOAD_RLY1_ON
            LOAD_RLY1_ON
            LOAD_RLY2_ON
            N_LOAD_RLY_ON

            MID_L1_RLY_ON
            MID_L2_RLY_ON
            N_MID_RLY_ON

            GRID_L1_RLY_ON
            GRID_L2_RLY_ON
            N_GRID_RLY_ON

            PE_RLY_OFF

            break;
        }
        case BusCharge:
        {
            AUS_RLY_ON;
            BUS_RLY_ON;
            LOAD_RLY1_OFF
            LOAD_RLY2_OFF
            N_LOAD_RLY_OFF

            MID_L1_RLY_ON
            MID_L2_RLY_ON
            N_MID_RLY_ON

            GRID_L1_RLY_ON
            GRID_L2_RLY_ON
            N_GRID_RLY_ON

            PE_RLY_OFF

            break;
        }
        case ByPass:
        {
            AUS_RLY_ON;
            BUS_RLY_OFF;
            LOAD_RLY1_OFF
            LOAD_RLY2_OFF
            N_LOAD_RLY_OFF

            MID_L1_RLY_ON
            MID_L2_RLY_ON
            N_MID_RLY_ON

            GRID_L1_RLY_ON
            GRID_L2_RLY_ON
            N_GRID_RLY_ON

            PE_RLY_OFF

            break;
        }
        default:
            break;
    }
}

void ReconnectChk(void)
{
    if (SampleRms.Grid1VoltRms > Protect.Value.GridOV1)
    {
        GridError.Grid1Err.bit.OV1 = 1;
        Protect.ClrCnt.Grid1OV1RstCnt = 0;
        return;
    }

    if (SampleRms.Grid1VoltRms < Protect.Value.GridUV1)
    {
        GridError.Grid1Err.bit.UV1 = 1;
        Protect.ClrCnt.Grid1UV1RstCnt = 0;
        return;
    }

    if (SampleRms.Grid2VoltRms < Protect.Value.GridUV1)
    {
        GridError.Grid2Err.bit.UV1 = 1;
        Protect.ClrCnt.Grid2UV1RstCnt = 0;
        return;
    }

    if (SampleRms.Grid2VoltRms > Protect.Value.GridOV1)
    {
        GridError.Grid2Err.bit.OV1 = 1;
        Protect.ClrCnt.Grid2OV1RstCnt = 0;
        return;
    }

    if (CaptureData.FreqR > Protect.Value.GridOF1)
    {
        GridError.Grid1Err.bit.OF1 = 1;
        Protect.ClrCnt.Grid1OF1RstCnt = 0;
        return;
    }

    if (CaptureData.FreqR < Protect.Value.GridUF1)
    {
        GridError.Grid1Err.bit.UF1 = 1;
        Protect.ClrCnt.Grid1UF1RstCnt = 0;
        return;
    }
}

void PreChargeChk(void)
{
    static int BUS_JDQOffCnt = 0;

    if(InvStateData.WorkState != ByPass || SampleIns.BUSVoltIns < 185)
    {
        FuncPara.PreChgOver = 0;
        BUS_JDQOffCnt = 0;
    }
    else
    {
//        //通过此处调整LOAD过零开启
//        if(StateDataStruct.LoadCnt== LOAD27P3MS )
//        {
//            LOAD_JDQ_ON //打开负载继电器
//            //闭合LOAD继电器N相
//            N_LOAD_JDQ_ON
//        }
        if(++BUS_JDQOffCnt >= 5000)  //500ms
        {
            BUS_JDQOffCnt = 5000;
            FuncPara.PreChgOver = 1;
        }
    }
}

//电网过载时过零断开
void SwitchToErrorState(void)
{
    if(InvPhase.ThetaPointR == (SampleIns.PeriodPoint - 32))//继电器动作3ms左右
    {
        InvStateData.ErrorStateCnt = 0;
        InvStateData.WorkState = ErrorState;
    }
}
/*************************************************
Function: RunInvStateMachine
Description: 逆变工作状态控制-InitState、SelfCheckState
** ErrorState、OffGridState、UpsState、OnGridState
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(RunInvStateMachine, ".TI.ramfunc");
void RunInvStateMachine(void)
{
    int ret = 0;
    Uint16 canRevState = 0;

    switch (InvStateData.WorkState)
    {
       // 自检状态:上电后进入
       case InitState:
       {
           // 逆变驱动禁止
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;
           // 如果电池已经启动直接断开预充继电器&&零飘检测结束&&PV ISO检测结束
           if ((SampleIns.CalSampleZeroFlag == 1)
                   && (FuncPara.ISOFinishFlag == 1))
           {
               InvStateData.InitStateCnt++;
               // 断开预充继电器
               BUS_RLY_OFF
               // 5s后进入下一状态
               if (InvStateData.InitStateCnt >= DELAY_5S_100US)
               {
                   // 关闭继电器
                   ControlRelay(InitState);
                   InvStateData.EmgyChgFlg = 0;
                   InvStateData.SelfCheckCnt = 0;
                   InvStateData.WorkState = SelfCheckState;
               }
           }
           break;
       }
       // 停机状态:自检结束无故障进入改状态
       case SelfCheckState:
       {
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;
           if((SystemPara.SystemPowerOn == 1)&&(SystemError.ArmFault&0x02)&&InvStateData.OnGridPermit==0) //无bms
           {
               //继电器自检
               //GridRlyCheck();
               // 满足离网运行条件:开机、BMS放电电流大于0;如果BMS允许放电电流为0停止逆变器输出,如果电网OK直接进行并网
//               if(FuncPara.RlyGridOver)//并网检完即可离网输出
               {
                   if (SystemPara.BMSDischgMaxPowerSet > 0.0001f)
                   {
                       // 条件满足持续 5s-LLC缓起结束 并且并机设备不是故障状态
                       if (++InvStateData.SelfCheckCnt >= DELAY_5S_100US)
//                               && CheckPowerONStatus()                                      // 并机设备是否在故障状态
//                               || (SystemPara.ParallelEn == 0)
//                               || Para_CheckWorkState(OffGridState, &canRevState))          // 并机设备已经进入到离网运行状态
                       {
                           InvStateData.OffGridCnt = 0;             // 清零离网状态计数器
                           InvStateData.OffGridFirst = 0;           // 清除离网首次标志位

                           InvStateData.OnGridToOffFlag = 0;        // 清除并网转离网标志位

                           InvStateData.EmgyChgFlg = 0;

                           InvStateData.WorkState = OffGridState;
                       }
                   }
                   // 如果电池严重低压,需要紧急充电,电网OK的情况下直接并网
                   else
                   {
                       InvStateData.SelfCheckCnt = 0;
                       // 电网OK直接并网,确认并机设备电网也正常才能进入并网流程
                       if (
//                               (ParaAlarm[1]&(1<<14)) == 0)         // 从机相序 OK
                                ((SystemAlarm.all & 0x2f) == 0)       // 本机电压频率 OK
//                               && ((ParaAlarm[3] & 0x2f) == 0)          // 从机电压频率 OK
                               && (ArmWriteData.GridOnEn == 1)
                               && InvStateData.BypassPermit)          // 允许并网
                       {
                           if ((++InvStateData.UpsStateCnt >= DELAY_5S_100US)
                                   || (SystemPara.ParallelEn == 0))                   // 单机模式
//                                   || Para_CheckWorkState(UpsState, &canRevState))     // 如果并机设备已经进入追电网状态,本机直接跳转到和并机设备相同的状态
                           {
                               InvStateData.UpsStateCnt = 0;             // 清除待机状态下的标志计数器
                               InvStateData.UpsGridFlg = 0;              // 清除标志

                               InvStateData.EmgyChgFlg = 1;              // 设置紧急充电标志位

                               ResetOffGridAngle();                      // 复位离网角度初值

                               InvStateData.WorkState = UpsState;        // 跳转到待机状态 进行相位同步
                           }
                       }
                   }

                   ControlRelay(SelfCheckState);
               }

           }
           else
           {
               if((ArmWriteData.GridOnEn)
                       &&InvStateData.BypassPermit
                       &&(SystemError.StateErr.bit.SystemFault == 0)
                       &&((SystemAlarm.all & 0xF) == 0)) //
               {
                   ReconnectChk();
                   if((FuncPara.RlyMidOver)||(FuncPara.RlyBpOver))
                   {
                       //@sjj 未使能锁相，应使用电网相位 锁相反
//                       if(fabs(-IrCtrl.ThetaSpll-(3 + 48 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR)
//                       {
//                           MID_L1_RLY_ON
//                           MID_L2_RLY_ON
//                           N_MID_RLY_ON
//                       }
                       if((fabs(-IrCtrl.ThetaSpll-(28 - 32 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR)) //43 -> 28 @sjj可能是继电器批次导致
                  //     if((fabs(-IrCtrl.ThetaSpll-(48 - 32 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR) && InvStateData.MidRlyFlg)
                       {
                           ControlRelay(ByPass);//否则继电器动作和状态机同时切换
                           InvStateData.WorkState = ByPass;
                       }
                   }
                   else
                   {
                       InvStateData.WorkState = ByPass;//初次进通过继电器检测闭合继电器
                   }
               }
           }


           // 如果异常进入故障状态
           if (SystemError.StateErr.bit.INVFault != 0)
           {
               InvStateData.ErrorStateCnt = 0;
               InvStateData.WorkState = ErrorState;
           }

           break;
       }
       case OffGridState:
       {
           // 首次进入时要清除重复控制buffer
           if (InvStateData.OffGridFirst == 0)
           {
               // 如果是并网转离网需要快速启动
               if (InvStateData.OnGridToOffFlag == 1)
               {
                   InvStateData.OffGridCnt++;
                   if (InvStateData.OffGridCnt >= 30
                       ||(InvStateData.MidRlyFlg == 0 && FuncPara.ForceToOff == 0))//主动并离网切换在继电器断开后零点处开始发波
                   {
                       FuncPara.ForceToOff = 0;
                       InvStateData.OffGridStartCnt = 0;
                       InvStateData.OnGridToOffFlag = 0;
                       InvStateData.OffGridCnt = 0;
                       InvStateData.OffGridFirst = 1;
                       UrCtrl.PWMONflagCnt = 0;
                       UsCtrl.PWMONflagCnt = 0;
                       ClearOffGridPidData();           // 离网电压环  PI 积分量以及前馈软启计数,初始电压为0
//                       InvPhase.ThetaR = (PI_S) - IrCtrl.ThetaSpll + 2*InvPhase.DeltR;//
                       ResetOffGridAngle();             // 复位离网角度初值
                       SetOffGridInitVolt(142.83f);
                       UrCtrl.Fw = SampleIns.Inv1VoltIns;
                       UsCtrl.Fw = SampleIns.Inv2VoltIns;
                       ControlOffGridInv();//提前1计算占空比
                   }
                   else
                   {
                       DISABLE_INV1PWM;
                       DISABLE_INV2PWM;
                       DISABLE_INV3PWM;
                       DISABLE_INV4PWM;
                   }
                   ControlRelay(OffGridState);
               }
               // 正常启动(非并网转离网)
               else
               {
//                   if (InvStateData.OffGridCnt < 206)
//                   {
//                       ClearOffGridRepeatData(InvStateData.OffGridCnt);    // 清除重复控制缓存
//                   }
                   // 零点开始发波
                   if ((InvStateData.OffGridCnt >= 402)
                       &&(InvPhase.ThetaPointR == 0))
                   {
                       InvStateData.OffGridStartCnt = 0;
                       InvStateData.OffGridCnt = 0;
                       InvStateData.OffGridFirst = 1;   // 标记首次启动完成
                       UrCtrl.PWMONflagCnt = 0;
                       UsCtrl.PWMONflagCnt = 0;
                       ClearOffGridPidData();           // 离网电压环  PI 积分量以及前馈软启计数,初始电压为0
                       ResetOffGridAngle();
                       SetOffGridInitVolt(142.83);       // 设置逆变初始缓起电压
                   }

                   InvStateData.OffGridCnt+=2;
               }
           }
           else
           {
               InvRlyCheck();

               if(FuncPara.RlyMidOver == 0 && GpioDataRegs.GPCDAT.bit.GPIO70 == 1)
               {
                   MidRlyCheck();
               }
               else
               {
                   if(FuncPara.RlyInvOver && (SampleIns.PeriodPoint - InvPhase.ThetaPointR) == 90)
                   {
                       ControlRelay(OffGridState);
                   }
               }

               // 开环到闭环切换时间
               //OffGridSoftStart();
               // 离网输出控制
               ControlOffGridInv();
               if( PWMONflag==0 )
                 {
                     //滞后1周期使能
                     ENABLE_INV1PWM;
                     ENABLE_INV2PWM;
                     ENABLE_INV3PWM;
                     ENABLE_INV4PWM;
                 }
           }

           if (SystemPara.MicroInverterMode == 1)
           {
               ClearOffGridRepeatData(InvStateData.ClearCnt);

               InvStateData.ClearCnt += 2;
               if (InvStateData.ClearCnt > 201)
               {
                   InvStateData.ClearCnt = 0;
               }
           }
           // 如果有故障进入到故障状态, 若果并机设备故障关机,当前设备也要进入停机状态
           if ((SystemError.StateErr.bit.INVFault != 0)
                   || ((SystemError.ArmFault&0x02) == 0))
//                   || Para_CheckWorkState(ErrorState, &canRevState)
           {
               InvStateData.ErrorStateCnt = 0;

               DISABLE_INV1PWM;
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
               InvStateData.WorkState = ErrorState;
               PWMONflag=0;
           }
           // 如果有停机指令进入到自检状态
           else if ((SystemPara.SystemPowerOn == 0)
                   || (SystemPara.BMSDischgMaxPowerSet < 0.0001f))
//                   || Para_CheckWorkStateInSameState(SelfCheckState, canRevState))
           {
               SwitchToSelfCheckState();
           }
           // 工作模式允许并网时进行并网, 此处的转并网条件是:电网电压/频率正常&&输出继电器自检结束
           else if (((SystemAlarm.all & 0x2f) == 0))         // 本机电网电压频率 OK
//                   && ((ParaAlarm[3] & 0x2f) == 0)          // 并机设备电网电压频率 OK
//                   && ((ParaAlarm[1]&(1<<14)) == 0))        // 并机设备电压相序 OK
           {
               InvStateData.UpsStateCnt = 0;                // 清除待机状态下的标志计数器
               InvStateData.UpsGridFlg = 0;                 // 清除标志
               InvStateData.EmgyChgFlg = 0;                 // 清除紧急充电标志位

//               if (ArmWriteData.AgingLable == 1 &&
//                    ArmWriteData.EnableGridChg == 0)
//               {
//                   InvStateData.WorkState = OffGridState;
//                   ESmode.ES_DELAY_CNT = 0;
//               }
//               else
               if ((InvStateData.OnGridPermit)
                       && (FuncPara.RlyMidOver))
               {
                   if (((ESmode.ES_DELAY_CNT++) > ESmode.ES_DELAY))
//                           || Para_CheckWorkStateInSameState(UpsState, canRevState))    // 如果并机设备已经进入追电网状态,本机直接跳转到和并机设备相同的状态
                   {
                       ESmode.ES_DELAY_CNT = 0;
                       ESmode.ES_STATUS = ES_DELAY;
                       InvStateData.WorkState = UpsState;
                   }
               }
               else
               {
                   ESmode.ES_DELAY_CNT = 0;
               }
           }
           else
           {
               ESmode.ES_DELAY_CNT = 0;
           }

           break;
       }
       case UpsState:
       {
           ReconnectChk();

           if(sGetModuleType() == cSlaveModule)
           {
               InvPhase.SyncReadyFlag = CheckGridRly();  //
           }
           // 相位同步成功,吸合继电器等待继电器完全吸合后进入并网控制 && 保证从机也处于UpsState状态
           if (InvPhase.SyncReadyFlag == 1)
           {
               //60hz取1.5周期 = 25ms
//                   InvStateData.RelayOnDelay = 0 + 48*(int)SystemPara.FrequencySet;//目前测试结果为18.3ms闭合(50HZ)
//
//                   if(InvPhase.ThetaPointR == InvStateData.RelayOnDelay&&(InvStateData.UpsGridFlg == 0))
//                   {
//                       MID_L1_RLY_ON
//                       MID_L2_RLY_ON
//                       N_MID_RLY_ON
//                       InvStateData.UpsGridFlg = 2;
//                   }

               InvStateData.RelayOnDelay = 20 + 48*(int)SystemPara.FrequencySet;//目前测试结果为15.3ms闭合(50HZ)

               if ((InvPhase.ThetaPointR == InvStateData.RelayOnDelay))
//                   if ((InvPhase.ThetaPointR == InvStateData.RelayOnDelay)&&(InvStateData.UpsGridFlg == 2))
               {
                   // 此刻主机发送继电器闭合指令到从机,当指令发送完成后开始闭合继电器
//                       SetRlyCloseCmd();
                   InvStateData.UpsGridFlg = 1;
                   ControlRelay(OnGridState);
               }
               // 最多需要 4ms 时间闭合指令发送完成
//                   if (ParaEvent.Bits.WaitForCloseRly
//                           || (Flag_Local.Bits.ParaMode == cNewModule))
//                   {
//                       ParaEvent.Bits.WaitForCloseRly = 0;
//
//                       InvStateData.UpsGridFlg = 1;
//                       ControlRelay(OnGridState);
//                   }
           }

           if (InvStateData.UpsGridFlg == 1)
           {
               // TODO 离网转并网 让继电器闭合时间落在电压零点位置
               if (InvStateData.UpsStateCnt >= 182)     //50HZ/60HZ
               {
                   InvStateData.OnGridCnt = 0;              // 复位并网计数器
                   InvStateData.GridErrTZCnt = 0;

                   ClearOnGridPidDataR();                    // 清除积分量
                   ClearOnGridPidDataS();
                   ESmode.ES_STATUS = ES_RAMP;
                   InvStateData.WorkState = OnGridState;    // 进入并网运行状态
               }
               else
               {
                   CleanOnGridCtrlData(InvStateData.UpsStateCnt);
                   InvStateData.UpsStateCnt++;
               }
           }
           else     // 如果没有同步成功,等待同步成功
           {
               if (InvStateData.EmgyChgFlg == 0)
               {
                   ControlRelay(UpsState);
               }
           }

           // 如果只是电网异常 保持当前状态,否则进入到故障状态
           if ((SystemError.StateErr.bit.INVFault != 0)
                   || ((SystemError.ArmFault & 0x02) == 0))
//                   || Para_CheckWorkState(ErrorState, &canRevState))
           {
                   InvStateData.ErrorStateCnt = 0;

                   DISABLE_INV1PWM;
                   DISABLE_INV2PWM;
                   DISABLE_INV3PWM;
                   DISABLE_INV4PWM;
                   InvStateData.WorkState = ErrorState;
           }
           else if ((SystemPara.SystemPowerOn == 0))//并网使能没关先切旁路
//                   || Para_CheckWorkStateInSameState(SelfCheckState, canRevState))
           {
               SwitchToSelfCheckState();
           }
           // 电网异常或者相序错误或切离网的故障
           else if ((InvStateData.OnGridPermit == 0)
//                   || (ParaAlarm[3] & 0x2f)
//                   || (ParaAlarm[1] & (1<<14))
                   )
           {
               if (InvStateData.EmgyChgFlg == 0)
//                       || Para_CheckWorkStateInSameState(OffGridState, canRevState))
               {
                   InvStateData.OffGridCnt = 0;
                   InvStateData.OffGridFirst = 1;
                   sESmodeinit();
                   InvStateData.WorkState = OffGridState;
               }
               else if ((InvStateData.EmgyChgFlg == 1))
//                       || Para_CheckWorkStateInSameState(SelfCheckState, canRevState))
               {
                   SwitchToSelfCheckState();
               }
           }
           // 紧急充电时直接进入并网状态
           if ((InvStateData.EmgyChgFlg == 1) || (InvStateData.UpsStateCnt >= 182))
           {
               DISABLE_INV1PWM;
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
           }
           else if (InvStateData.EmgyChgFlg == 0)
           {
               // 离网输出控制
               ControlOffGridInv();

               ENABLE_INV1PWM;
               ENABLE_INV2PWM;
               ENABLE_INV3PWM
               ENABLE_INV4PWM
           }

           if ((InvStateData.WorkState == OnGridState))
//                   || Para_CheckWorkStateInSameState(OnGridState, canRevState))
           {
               InvStateData.OnGridCloseFlg = 0;
               InvStateData.OnGridCloseFlg2 = 0;
               InvStateData.OnGridCloseCnt = 0;
               InvStateData.OnGridCloseCnt2 = 0;
               InvStateData.EmgyChgFlg = 0;

               DISABLE_INV1PWM;         // 进入并网后逆变停止发波
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
           }

           break;
       }
       case OnGridState:
       {
           // 并网状态下将离网控制量清零
//           if (InvStateData.OnGridCnt < 206)
//           {
//               ClearOffGridRepeatData(InvStateData.OnGridCnt);    // 清除重复控制缓存
//           }

           InvStateData.OnGridCnt+=2;

           // 5s后启动并网电流调节
           if (InvStateData.OnGridCnt >= 100000L)
           {
               // BUS高压锁波5s
               if ((GridError.Grid1Err.all&0x1A00) || (GridError.Grid2Err.all&0x1A00))
               {
                    // 电网跌落开始封波,持续100ms后
                    DISABLE_INV1PWM;
                    DISABLE_INV2PWM;
                    DISABLE_INV3PWM;
                    DISABLE_INV4PWM;

//                    CleanSPhaseOnGridCtrlData(InvStateData.OnGridCloseCnt2);   // 清除重复控制量
//                    InvStateData.OnGridCloseCnt2++;
                    IrCtrl.RmpCtrl.SetpointValue = 0;
                    IsCtrl.RmpCtrl.SetpointValue = 0;
                    ClearOnGridSinglePidData(1);                         // 清除R相积分量
					ClearOnGridPidDataR();                            // 清除积分量
					ClearOnGridSinglePidData(2);
					ClearOnGridPidDataS();                            // 清除积分量
					Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PRVars , 0.2 , -0.2);
				    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR3Vars, 0.3 , -0.3); //@sjj20250911
				    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR5Vars, 0.3 , -0.3); //@sjj20250911
				    Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR7Vars, 0.2 , -0.2); //@sjj20250911
                    Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PRVars , 0.2 , -0.2);
                    Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR3Vars, 0.3 , -0.3); //@sjj20250911
                    Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR5Vars, 0.3 , -0.3); //@sjj20250911
                    Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR7Vars, 0.2 , -0.2); //@sjj20250911
                    if ((InvStateData.GridErrTZCnt >= (10*100))
                            && (InvPhase.ThetaPointR == 1))
                    {
                        InvStateData.GridErrTZCnt = 0;
                        GridError.Grid1Err.bit.GridDrop = 0;
                        GridError.Grid2Err.bit.GridDrop = 0;
                        GridError.Grid1Err.bit.Island = 0;
                        GridError.Grid2Err.bit.Island = 0;

                        ClearOnGridPidDataR();                            // 清除积分量
                        ClearOnGridPidDataS();
                       // ClearINVPwmComp3();                              // 复位并网PWM标记
                    }
                    else
                    {
                        CleanOnGridCtrlData(InvStateData.GridErrTZCnt);  // 清除重复控制量
                        InvStateData.GridErrTZCnt++;
                    }
               }
               else
               {
                   // 并网输出控制:等待微逆退出低功耗模式
                   ret = ControlOnGridInvR(InvStateData.OnGridCloseFlg);//&&(!LLCAdjustData1.WaitFlg));

                   // R相恢复驱动
                   if ((ret == 0) && (InvStateData.OnGridCloseFlg == 1))
//                           && (LLCAdjustData1.WaitFlg == 0))
                   {
                       if(InvPhase.ThetaPointR == 1)
                       {
                           InvStateData.OnGridCloseCnt = 0;
                           ENABLE_INV1PWM;
                           ENABLE_INV4PWM;
                           IrCtrl.PWMOn = 0;
                       }
                   }
                   // R 相目标功率为0 封波
                   else
                   {
                       if ((InvStateData.OnGridCloseCnt >= (10*100))
                               && (InvPhase.ThetaPointR == 1))
                       {
                           InvStateData.OnGridCloseFlg = 1;
                           InvStateData.OnGridCloseCnt = 10*300;

                           ClearOnGridSinglePidData(1);                         // 清除R相积分量
                          // ClearINV1PwmComp3();
                       }
                       else
                       {
                           InvStateData.OnGridCloseFlg = 0;
                           CleanRPhaseOnGridCtrlData(InvStateData.OnGridCloseCnt);   // 清除重复控制量
                           InvStateData.OnGridCloseCnt++;
                       }
                      ClearOnGridPidDataR();                            // 清除积分量
					  Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PRVars , 0.2 , -0.2);
					  Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR3Vars, 0.3 , -0.3); //@sjj20250911
					  Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR5Vars, 0.3 , -0.3); //@sjj20250911
					  Init_CNTL_2P2Z_F_VARS_(&IrCtrl.PR7Vars, 0.2 , -0.2); //@sjj20250911

                       DISABLE_INV1PWM;
                       DISABLE_INV4PWM;
                       IrCtrl.PWMOn = 0;
                   }

                   ret = ControlOnGridInvS(InvStateData.OnGridCloseFlg2);//&&(!LLCAdjustData2.WaitFlg));
                   // S相恢复驱动等待LLC2退出低功耗模式
                   if ((ret == 0) && (InvStateData.OnGridCloseFlg2 == 1))
//                           && (LLCAdjustData2.WaitFlg == 0))
                   {
                	   if(InvPhase.ThetaPointS == 1)
					   {
						 InvStateData.OnGridCloseCnt2 = 0;
						 ENABLE_INV2PWM;
						 ENABLE_INV3PWM;
					   }
                   }

                   // S 相目标功率为0 封波
                   else
                   {
                       if ((InvStateData.OnGridCloseCnt2 >= (10*100))
                               && (InvPhase.ThetaPointS == 1))
                       {
                           InvStateData.OnGridCloseFlg2 = 1;
                           InvStateData.OnGridCloseCnt2 = 10*300;

                           ClearOnGridSinglePidData(2);                         // 清除S相积分量
                           //ClearINV2PwmComp3();
                       }
                       else
                       {
                           InvStateData.OnGridCloseFlg2 = 0;
                           CleanSPhaseOnGridCtrlData(InvStateData.OnGridCloseCnt2);   // 清除重复控制量
                           InvStateData.OnGridCloseCnt2++;
                       }
                       ClearOnGridPidDataS();                            // 清除积分量
					   Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PRVars , 0.2 , -0.2);
					   Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR3Vars, 0.3 , -0.3); //@sjj20250911
					   Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR5Vars, 0.3 , -0.3); //@sjj20250911
					   Init_CNTL_2P2Z_F_VARS_(&IsCtrl.PR7Vars, 0.2 , -0.2); //@sjj20250911
                       DISABLE_INV2PWM;
                       DISABLE_INV3PWM;
                   }
               }

               InvStateData.OnGridCnt = 100000L;
           }

           if(sGetModuleType() == cSlaveModule && !CheckGridRly())
           {
               SystemAlarm.bit.LossGrid = 1;
           }
           // 快速并离网切换判断
           if (SystemAlarm.bit.LossGrid
                   ||((SystemAlarm.all & 0xF)&&(SystemPara.GridEnhanceEn)))
           {
               FuncPara.BeOnToOff = 1;
               FuncPara.ForceToOff = 1;
               ReconnectChk();
               SystemAlarm.bit.GridOsc =0;
           }
           else if((InvStateData.OnGridPermit == 0))
//                   || Para_CheckWorkStateInSameState(OffGridState, canRevState)
//                   || ((ArmWriteData.AgingLable == 1)&&(ArmWriteData.EnableGridChg == 0)))
           {
               SystemAlarm.bit.GridOsc =0;
               SelfOnToOffHandle();
           }
           else if(FuncPara.RlyInvOver == 0)
           {
               if(ArmWriteData.EmgChagEn)
               {
                   FuncPara.RlyInvOver = 1;
               }
               InvRlyCheck();
           }
           else if((SampleIns.PeriodPoint - InvPhase.ThetaPointR) == 90)   //80 -> 90 @sjj 20260320
           {
               if(InvStateData.OnGridCnt >= 1600L)
               {
                   ControlRelay(OnGridState);
               }
           }

           // 不允许旁路的故障切故障状态
           if (((SystemError.StateErr.bit.INVFault != 0)&&(InvStateData.BypassPermit == 0)))
//                   || Para_CheckWorkState(ErrorState, &canRevState))
           {
               DISABLE_INV1PWM;
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
               IrCtrl.PWMOn = 0;
               SwitchToErrorState();
           }
           // 允许旁路的故障或关机切旁路状态
           else if ((SystemPara.SystemPowerOn == 0)
                   ||((SystemError.StateErr.bit.INVFault != 0)&&(InvStateData.BypassPermit))
                   ||((SystemError.ArmFault & 0x02) == 0))
           {
               DISABLE_INV1PWM;
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
               IrCtrl.PWMOn = 0;
               if(InvPhase.ThetaPointR == (95 - 16*(int)SystemPara.FrequencySet))
               {
                   ControlRelay(ByPass);//否则继电器动作和状态机同时切换
                   InvStateData.WorkState = ByPass;
               }

           }
//           else if (Para_CheckWorkStateInSameState(SelfCheckState, canRevState))
//           {
//               SwitchToSelfCheckState();
//           }
           //切离网状态
           else if(FuncPara.BeOnToOff == 1)
           {
               InvStateData.OffGridCnt = 0;
               InvStateData.OffGridFirst = 0;
               InvStateData.OnGridToOffFlag = 1;        // 置位并网转离网标志位
               FuncPara.BeOnToOff = 0;
               FuncPara.RlyInvOver = 1;
               sESmodeinit();
               // 封锁 INV PWM
               DISABLE_INV1PWM;
               DISABLE_INV2PWM;
               DISABLE_INV3PWM;
               DISABLE_INV4PWM;
               IrCtrl.PWMOn = 0;
               // 并网切离网过程中,断开电网时逆变有直流高压
               InvStateData.WorkState = OffGridState;
           }
           break;
       }
       case ByPass:
       {
           // 封锁 INV PWM
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;

           BpRlyCheck();

           if(sGetModuleType() == cSlaveModule && !CheckGridRly())
           {
               ArmWriteData.GridOnEn = 0;
           }

           if(InvStateData.BypassPermit == 0)
           {
               SwitchToErrorState();
           }
           else if((ArmWriteData.GridOnEn == 0)||((SystemAlarm.all & 0xF) != 0))
           {
               SwitchToSelfCheckState();
           }
           else if(ArmWriteData.EmgChagEn
                   && SystemPara.SystemPowerOn)
           {
               if(FuncPara.RlyBpOver)
               {
                   ControlRelay(BusCharge);
//                   InvStateData.WorkState = BusCharge;
               }
           }
           else
           {
//               FuncPara.PreChgOver = 0;
               BUS_RLY_OFF;
           }

           if((SystemError.StateErr.bit.INVFault == 0)
               &&(FuncPara.PreChgOver || (ArmWriteData.EmgChagEn == 0)) //急救时需保证预充完成
               &&(FuncPara.RlyBpOver)//防止自检过程中切换状态
               &&(SystemPara.SystemPowerOn == 1)//开机指令在才切离网，否则会切到离网然后关机再切回旁路
               &&(SystemError.ArmFault&0x02)//电池包异常不退出旁路，无法恢复正常工作
               &&(ArmWriteData.PVSourceType == 0 || SampleIns.BUSVoltIns > 385))
           {
               //@sjj开机旁路直接并网，启动锁相过零闭合
               if (InvStateData.BpsStateCnt >= 205)
               {
                   InvStateData.BpsStateCnt = 205;
                   if((fabs((PI_S - IrCtrl.ThetaSpll)-50*InvPhase.DeltR) <= InvPhase.DeltR))
                   {
                       InvStateData.OnGridCnt = 0;              // 复位并网计数器
                       InvStateData.GridErrTZCnt = 0;
                       ClearOnGridPidDataR();                    // 清除积分量
                       ClearOnGridPidDataS();
                       ESmode.ES_STATUS = ES_RAMP;
                       InvStateData.WorkState = OnGridState;    // 进入并网运行状态
                   }
               }
               else
               {
                   CleanOnGridCtrlData(InvStateData.BpsStateCnt);
                   InvStateData.BpsStateCnt++;
               }
           }
           else
           {
               InvStateData.BpsStateCnt= 0;
           }

           PreChargeChk();

           break;
       }
       case BusCharge:
       {
           // 封锁 INV PWM
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;

           if(InvStateData.PreChagCnt++ >= 25000)//2.5s
           {
               InvStateData.PreChagCnt = 25000;
               if(fabs((PI_S - IrCtrl.ThetaSpll)- 77 <= InvPhase.DeltR))//只有在电网电压瞬时值绝对值高于母线时才会有电流
               {
                   LOAD_RLY1_ON;
                   LOAD_RLY2_ON;
                   N_LOAD_RLY_ON;
//                   BUS_RLY_OFF;//测试关断时间？
               }
           }

           break;
       }
       case ErrorState:
       {
           // 封锁 INV PWM
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;

           if(InvStateData.BypassPermit //(SystemError.StateErr.bit.SystemFault == 0)
                   &&((SystemAlarm.all & 0xF) == 0)
//                   &&(!SystemPara.ByPassDisable)
                   &&(ArmWriteData.GridOnEn))//不是关系统的故障才进行旁路判断,电网没问题且并网使能就旁路
           {
               ReconnectChk();
               if((FuncPara.RlyMidOver)||(FuncPara.RlyBpOver))
               {
                   //@sjj 未使能锁相，应使用电网相位 锁相反
//                   IrCtrl.ThetaTmp = fabs(-IrCtrl.ThetaSpll-(48 * (int)SystemPara.FrequencySet)*InvPhase.DeltR);
//                   if(IrCtrl.ThetaTmp <= InvPhase.DeltR)
//                   if(fabs(-IrCtrl.ThetaSpll-(3 + 48 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR)
//                   {
//                       MID_L1_RLY_ON
//                       MID_L2_RLY_ON
//                       N_MID_RLY_ON
//                   }
                   if((fabs(-IrCtrl.ThetaSpll-(28 - 32 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR))
//                   if((fabs(-IrCtrl.ThetaSpll-(43 - 32 * (int)SystemPara.FrequencySet)*InvPhase.DeltR) <= InvPhase.DeltR) && InvStateData.MidRlyFlg)
                   {
                       ControlRelay(ByPass);//否则继电器动作和状态机同时切换
                       InvStateData.WorkState = ByPass;
                   }
               }
               else
               {
                   InvStateData.WorkState = ByPass;//初次进通过继电器检测闭合继电器
               }
           }
           else
           {
               ControlRelay(ErrorState);
           }

           if (((InvError.Inv1Err.all&0x07) == 0)
                   && ((InvError.Inv2Err.all&0x07) == 0)
                   && (SystemError.StateErr.bit.INVFault == 0))
           {
               // 连续2s无故障进入到停机状态
               if (++InvStateData.ErrorStateCnt >= DELAY_2S_100US)
               {
                   InvStateData.SelfCheckCnt = 0;
                   InvStateData.WorkState = SelfCheckState;
               }
           }
           else
           {
               InvStateData.SelfCheckCnt = 0;
           }

           break;
       }
       default:
       {
           // 封锁 INV PWM
           DISABLE_INV1PWM;
           DISABLE_INV2PWM;
           DISABLE_INV3PWM;
           DISABLE_INV4PWM;

           SystemPara.SystemPowerOn = 0;            // 关机

           ControlRelay(ErrorState);                // 断开继电器

           if (++InvStateData.SelfCheckCnt > DELAY_2S_100US)
           {
               InvStateData.SelfCheckCnt = 0;
               InvStateData.WorkState = SelfCheckState; // 进入待机

           }
           break;
       }
    }

    Para_ResetWorkState();

    if(Flag_Local.Bits.enParallel)
    {
        FuncPara.RlyInvOver = 1;
    }
}



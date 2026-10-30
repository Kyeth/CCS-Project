/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: Protect.c
Author: tzz
Version : 0.0.2
Date:2021年10月12日
Description: 软件保护模块文件
Function List: 软件保护功能文件:故障、告警、孤岛、过载
***********************************************************/
#include <math.h>
#include <string.h>
#include <bsp.h>
#include "Sample.h"
#include "Protect.h"
#include "ModbusTable.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "PowerCalculator.h"
#include "PVStateMachine.h"
#include "ConfigSystemPara.h"
#include "LLCStateMachine.h"
#include "PVStateMachine.h"
#include "User_I2C.h"
#include "OnGridInv2.h"
#include <Function.h>
#include "BUSStateMachine.h"
#include "Energy_Manage.h"
#include "Parallel.h"
PROTECT Protect;
INV_ERROR InvError;
GRID_ERROR GridError;
SYSTEM_ERROR SystemError;
SYSTEM_ALARM SystemAlarm;
// 孤岛
ISLAND_PROTECT IslandDataR,IslandDataS;

// 故障自动复位
FATULT_AUTO_RST FaultAutoRst;
GFCI_DATA  GFCIData ;
//测试
struct FAULT_CNT FaultCnt;

/*************************************************
Function: InitProtectData
Description: 保护模块数据初始化
Input: None
Output: None
Frequency: Once
*************************************************/
void InitProtectData(void)
{
    memset(&FaultCnt, 0, sizeof(struct FAULT_CNT));

    memset(&FaultAutoRst, 0, sizeof(FATULT_AUTO_RST));
    memset(&Protect, 0, sizeof(PROTECT));
    memset(&SystemError, 0, sizeof(SYSTEM_ERROR));
    memset(&SystemAlarm, 0, sizeof(SYSTEM_ALARM));
    memset(&InvError, 0, sizeof(INV_ERROR));
    memset(&GridError, 0, sizeof(GRID_ERROR));
    memset(&IslandDataR, 0, sizeof(ISLAND_PROTECT));
    memset(&IslandDataS, 0, sizeof(ISLAND_PROTECT));
	memset(&GFCIData, 0, sizeof(GFCI_DATA));
	GFCIData.DelayCnt = 1000;
    Protect.Enable.all = 0x0;               // 调试禁止
    Protect.Enable.bit.BusVoltOver = 1;     // 母线过压故障保护使能
    Protect.Enable.bit.BusVoltLow  = 1;     // 母线欠压故障保护使能
    Protect.Enable.bit.BATVoltLow  = 1;     // 直流输入低压保护使能
                                            // 硬件逆变过流默认开启
    Protect.Enable.bit.PVVoltOver  = 1;     // PV过压使能
    Protect.Enable.bit.PVVoltLow   = 1;     // PV欠压使能
    Protect.Enable.bit.PVCurrOver  = 1;     // PV过流使能
    Protect.Enable.bit.GFCI        = 0;     // GFCI保护使能

    Protect.Enable.bit.INVVoltFault = 1;     // INV输出故障使能
    Protect.Enable.bit.Commu = 0;
    Protect.Enable.bit.GridVoltLow = 1;     //电网故障使能暂时没用
    Protect.Enable.bit.Freq = 1;            //同上

    // -----------------通信故障,禁止,500ms 检测一次,改为3s触发,有一次通信立即复位
    Protect.Delay.ArmCommuDly = 6;            // 15s->5s->3s
    Protect.ClrDly.ArmCommuRstDly = 4;        // 10s 故障复位->1s

    Protect.Delay.CANCommuDly = 6;
    Protect.Delay.RS485CommuDly = 6;
    Protect.ClrDly.CANCommuRstDly = 20;
    Protect.ClrDly.RS485CommuRstDly = 4;

    // ----------------- 电网过压保护-------10ms检测频率

    Protect.Value.GridOV1 = SystemPara.InvRateVolt*1.138f;   // 115V
    Protect.Delay.GridOV1Dly = 93L;                         // 930ms
    Protect.Value.GridOV2 = Protect.Value.GridOV1;
    Protect.Delay.GridOV2Dly = Protect.Delay.GridOV1Dly;
    Protect.Value.RecoveryGridOV1 = SystemPara.InvRateVolt*1.109f;   // 116V

    Protect.Value.GridUV1 = SystemPara.InvRateVolt*0.792f;//80v
    Protect.Delay.GridUV1Dly = 93L;                        // 200ms
    Protect.Value.GridUV2 = Protect.Value.GridUV1;
    Protect.Delay.GridUV2Dly = Protect.Delay.GridUV1Dly;
    Protect.Value.RecoveryGridUV1 = SystemPara.InvRateVolt*0.822f;

//    Protect.Value.GridOV1 = SystemPara.InvRateVolt*1.15f;   // 116V
//    Protect.Delay.GridOV1Dly = 90L;                         // 900ms
//    Protect.Value.GridOV2 = Protect.Value.GridOV1;
//    Protect.Delay.GridOV2Dly = Protect.Delay.GridOV1Dly;
//    Protect.Value.RecoveryGridOV1 = SystemPara.InvRateVolt*1.1f;   // 116V
//
//    Protect.Value.GridUV1 = SystemPara.InvRateVolt*0.8f;
//    Protect.Delay.GridUV1Dly = 90L;                        // 200ms
//    Protect.Value.GridUV2 = Protect.Value.GridUV1;
//    Protect.Delay.GridUV2Dly = Protect.Delay.GridUV1Dly;
//    Protect.Value.RecoveryGridUV1 = SystemPara.InvRateVolt*0.85f;
    // 并网过欠频
    Protect.Value.GridOF1 = SystemPara.InvRateFreq + 1.0f;
    Protect.Delay.GridOF1Dly = 90L;                       // 200ms
    // 等级2
    Protect.Value.GridOF2 = Protect.Value.GridOF1;
    Protect.Delay.GridOF2Dly = Protect.Delay.GridOF1Dly;
    Protect.Value.RecoveryGridOF1 = SystemPara.InvRateFreq + 0.5f;
    // 等级1
    Protect.Value.GridUF1 = SystemPara.InvRateFreq - 1.5f;
    Protect.Delay.GridUF1Dly = 90L;                       // 200ms
    // 等级2
    Protect.Value.GridUF2 = Protect.Value.GridUF1;
    Protect.Delay.GridUF2Dly = Protect.Delay.GridUF1Dly;
	Protect.Value.RecoveryGridUF1 = SystemPara.InvRateFreq - 1.0f;
	// 电网过/欠压 告警复位
    Protect.ClrDly.GridOVRstDly = 10L*100L;      // 300s
    Protect.ClrDly.GridUVRstDly = 10L*100L;      // 300s
    // 电网过/欠频 告警复位
    Protect.ClrDly.GridOFRstDly = 10L*100L;      // 300s
    Protect.ClrDly.GridUFRstDly = 10L*100L;      // 300s
    
    // 故障复位
    // ---------- 电网掉电快速保护 -----------------
    Protect.Delay.GridLossDly = 30;
    Protect.Delay.GridLoss2Dly = 35;//20;
    Protect.Delay.GridLoss3Dly = 3;
    // 电网掉电告警复位
    Protect.Value.GridLoss1 = 30.0f;
    Protect.Value.GridLoss2 = 60.0f;
    Protect.Value.GridLoss3 = (101.0f*1.2f*1.414f);
    Protect.ClrDly.GridLossRstDly = 100000;         //
    // 孤岛保护
    IslandDataR.AllEnable = 0;
    IslandDataR.IslandEnable = 0;
   // IslandDataR.DeltaF = 0.04f;             // 起始扰动频率   0.04Hz + 60

    IslandDataR.MaxDeltaF = 0.7f;           // 最大扰动频率   0.7 + 60

    IslandDataR.TimeCheck = 20;             // 检测时间20个周波--4
    IslandDataR.CheckInterval = 10*1000;    // 检测一次1s扰动一次

    IslandDataS = IslandDataR;
    InitAutoRstErrCode();
}

/*************************************************
** Function: TestProtectDetect
** Description: 一般性故障检测，只根据return值判断故障发生
** Input: 输入 保护计数 阈值 保护延时
** Output: None
** Frequency:Once
*************************************************/
#pragma CODE_SECTION(TestProtectDetect, ".TI.ramfunc");
uint16_t TestProtectDetect(float Sample,uint16_t *ProtectCnt,float Protectlimit,uint16_t ProtectDly)
{
    if(Sample > Protectlimit)
    {
        if((*ProtectCnt)++ > ProtectDly)
        {
            *ProtectCnt = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        *ProtectCnt = 0;
        return 0;
    }
}

/*************************************************
Function: ReadUpdateState
Description: 读取当前是否可以升级,如果当前电网有电则不允许升级
Input: None
Output: None
Frequency: 2ms
*************************************************/
Uint16 ReadUpdateState(void)
{
    if (((SampleRms.Inv1VoltRms < 15.0f) && (SampleRms.Inv2VoltRms < 15.0f))
            || (SystemError.ArmFault&0x02))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*************************************************
Function: ModifyGridLossPara
Description: 修改电网掉电保护参数,放宽保护参数,老化时需要修改
Input: None
Output: None
Frequency:
*************************************************/
void ModifyGridLossPara(void)
{
    if (ArmWriteData.AgingLable)
    {
        Protect.Value.GridOV1 = 135;
        Protect.Value.RecoveryGridOV1 = 130;    //
        ArmWriteData.ReConnectDly = (10L*100L);
        Protect.ClrDly.GridReConnectDly = 10L*100L;
        // 电网过/欠压 告警复位
        Protect.ClrDly.GridOVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        Protect.ClrDly.GridUVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        // 电网过/欠频 告警复位
        Protect.ClrDly.GridOFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
        Protect.ClrDly.GridUFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s

        Protect.Delay.GridLossDly = 60;         // 50Hz-50;60Hz-42;
        Protect.Delay.GridLoss2Dly = 30;
        Protect.Value.GridLoss1 = 60.0f;
        Protect.Value.GridLoss2 = 100.0f;
    }
    else if (SystemPara.DieselEngineMode || SystemPara.GridEnhanceEn)
    {
        Protect.Delay.GridLossDly = 70;
        Protect.Delay.GridLoss2Dly = 50;
        Protect.Value.GridLoss1 = 30.0f;       //连续70点小于30v
        Protect.Value.GridLoss2 = 100.0f;      //连续50点与正弦相差100V
    }
    else
    {
        Protect.Delay.GridLossDly =  42;
        Protect.Delay.GridLoss2Dly = 27;
        Protect.Value.GridLoss1 = 60.0f;
        Protect.Value.GridLoss2 = 40.0f;
    }
}

void SetParallelErrBit(void)
{
    SystemError.Fault4.bit.ParaCommu = 1;
}
void CleanParallelErrBit(void)
{
    SystemError.Fault4.bit.ParaCommu = 0;
}
/*************************************************
** Function: AutoRstErrCode
** Description: 硬件故障自动复位,每3min清除一次故障,
** 如果三分钟累计触发超过3次，之后不再清除故障
** Input: @err-需要自动复位的硬件故障结构体指针
** Output: None
** Frequency:1ms调用一次
*************************************************/
void AutoRstErrCode(AUTO_RST_T *err)
{
    // 从第一次故障开始30min内如果故障触发次数大于3,故障将不再复位
    if ((*err->ErrCode&(1<<err->ErrBit)) || (err->TrigCnt > 0))
    {
        if ((++err->ThirtyMinCnt <= (1000L*60L*30L)) && (err->TrigCnt < 3))     // 30分钟3次
        {
            // 如果故障码为1
            if (*err->ErrCode&(1<<err->ErrBit))
            {
                // 3min清除故障位
                if (++err->RstCnt >= err->RstDly)
                {
                    err->RstCnt = 0;
                    *err->ErrCode &= ~(1<<err->ErrBit);      // 故障码清零
                    err->TrigCnt++;                          // 触发次数加1
                }
            }
            else
            {
                err->RstCnt = 0;
            }
        }
        else
        {
            // 如果30min内超过3次故障不再复位,否则重新开始累计
            if (err->TrigCnt < 3)
            {
                err->TrigCnt = 0;
                err->ThirtyMinCnt = 0;
            }
            else
            {
                err->ThirtyMinCnt = (1000L*60L*31L);
            }
        }
    }
}

/*************************************************
** Function: InitAutoRstErrCode
** Description: 初始化硬件故障自动复位结构体变量
** Input: None
** Output: None
** Frequency:Once
*************************************************/
void InitAutoRstErrCode(void)
{
    // 逆变1硬件过流 ------------考虑不做自动复位
    FaultAutoRst.L1OC.ErrCode = &SystemError.Fault1.all;
    FaultAutoRst.L1OC.ErrBit = 7;
    FaultAutoRst.L1OC.RstDly = 1000L*150l;
    // 逆变2硬件过流
    FaultAutoRst.L2OC.ErrCode = &SystemError.Fault1.all;
    FaultAutoRst.L2OC.ErrBit = 8;
    FaultAutoRst.L2OC.RstDly = 1000L*150l;
    // PV1-PV4硬件过流
    FaultAutoRst.PV1OC.ErrCode = &SystemError.Fault2.all;
    FaultAutoRst.PV1OC.ErrBit = 0;
    FaultAutoRst.PV1OC.RstDly = 1000L*150l;

    FaultAutoRst.PV2OC.ErrCode = &SystemError.Fault2.all;
    FaultAutoRst.PV2OC.ErrBit = 1;
    FaultAutoRst.PV2OC.RstDly = 1000L*150l;

    FaultAutoRst.PV3OC.ErrCode = &SystemError.Fault2.all;
    FaultAutoRst.PV3OC.ErrBit = 3;
    FaultAutoRst.PV3OC.RstDly = 1000L*150l;

    FaultAutoRst.PV4OC.ErrCode = &SystemError.Fault4.all;
    FaultAutoRst.PV4OC.ErrBit = 5;
    FaultAutoRst.PV4OC.RstDly = 1000L*150l;
    // 过载
    FaultAutoRst.OverLoad1.ErrCode = &SystemError.Fault3.all;
    FaultAutoRst.OverLoad1.ErrBit = 7;
    FaultAutoRst.OverLoad1.RstDly = 1000L*120l;

    FaultAutoRst.OverLoad2.ErrCode = &SystemError.Fault3.all;
    FaultAutoRst.OverLoad2.ErrBit = 8;
    FaultAutoRst.OverLoad2.RstDly = 1000L*120l;
    // 逆变输出故障
    FaultAutoRst.Inv1Volt.ErrCode = &SystemError.Fault3.all;
    FaultAutoRst.Inv1Volt.ErrBit = 10;
    FaultAutoRst.Inv1Volt.RstDly = 1000L*120l;

    FaultAutoRst.Inv2Volt.ErrCode = &SystemError.Fault3.all;
    FaultAutoRst.Inv2Volt.ErrBit = 11;
    FaultAutoRst.Inv2Volt.RstDly = 1000L*120l;
    //平衡电路过流
    FaultAutoRst.BusBalance.ErrCode = &SystemError.Fault1.all;
    FaultAutoRst.BusBalance.ErrBit = 11;
    FaultAutoRst.BusBalance.RstDly = 1000L*120l;
    //BUS硬件过压
    FaultAutoRst.BusHHV.ErrCode = &SystemError.Fault1.all;
    FaultAutoRst.BusHHV.ErrBit = 4;
    FaultAutoRst.BusHHV.RstDly = 1000L*120l;
}
/*************************************************
** Function: CheckAutoRstErrCode
** Description: 硬件故障自动复位主函数体
** Input: None
** Output: None
** Frequency: 1ms
*************************************************/
void CheckAutoRstErrCode(void)
{
    AutoRstErrCode(&FaultAutoRst.L1OC);
    AutoRstErrCode(&FaultAutoRst.L2OC);
    // PV过流
    AutoRstErrCode(&FaultAutoRst.PV1OC);
    AutoRstErrCode(&FaultAutoRst.PV2OC);
    AutoRstErrCode(&FaultAutoRst.PV3OC);
    AutoRstErrCode(&FaultAutoRst.PV4OC);
    // 过载
    AutoRstErrCode(&FaultAutoRst.OverLoad1);
    AutoRstErrCode(&FaultAutoRst.OverLoad2);
    // 逆变输出故障
    AutoRstErrCode(&FaultAutoRst.Inv1Volt);
    AutoRstErrCode(&FaultAutoRst.Inv2Volt);
    //平衡电路过流
    AutoRstErrCode(&FaultAutoRst.BusBalance);
}

/*************************************************
Function: ClearCommunicationFaultCount
Description: 通信异常计数器清零, 在 modbus 通信模块中被调用
Input: None
Output: None
Frequency:
*************************************************/
void ClearCommunicationFaultCount(void)
{
    Protect.Count.ArmCommuCnt = 0;

    SystemError.Fault3.bit.CommuErr = 0;
}
void ClearRS485CommunicationFaultCount(void)
{
    Protect.Count.Rs485CommuCnt = 0;

    SystemAlarm.bit.rs485 = 0;
}
void ClearCANCommunicationFaultCount(void)
{
    Protect.Count.CANCommuCnt = 0;

    SystemAlarm.bit.CAN = 0;
}
/*************************************************
Function: CheckCommunicationFault
Description: ARM通信异常监测,连续15s内通信失败触发通信故障,连续10s通信正常清除故障
Input: None
Output: None
Frequency:
*************************************************/
void CheckCommunicationFault(void)
{
    // ARM 通信保护
    if (Protect.Enable.bit.Commu)
    {
        if (++Protect.Count.ArmCommuCnt >= Protect.Delay.ArmCommuDly)
        {
            Protect.Count.ArmCommuCnt = 0;
            SystemError.Fault3.bit.CommuErr = 1;
        }
    }
    else
    {
        Protect.Count.ArmCommuCnt = 0;
    }

#if 0
    // ARM 通信故障复位:连续 2s 有通信数据(5s内有接收到arm数据)清除故障位
    if (SystemError.Fault3.bit.CommuErr == 1)
    {
        if (Protect.Count.ArmCommuCnt < 3)        // 5s
        {
            if (++Protect.ClrCnt.ArmCommuRstCnt > Protect.ClrDly.ArmCommuRstDly)        // 2s
            {
                Protect.ClrCnt.ArmCommuRstCnt = 0;
                SystemError.Fault3.bit.CommuErr = 0;
            }
        }
        else
        {
            Protect.ClrCnt.ArmCommuRstCnt = 0;
        }
    }
    else
    {
        Protect.ClrCnt.ArmCommuRstCnt = 0;
    }
#endif

#if 0
    // 485 通信故障复位:连续 1min 有通信数据(5s内有接收到arm数据)清除故障位
    if (SystemAlarm.bit.rs485 == 1)
    {
        if (Protect.Count.Rs485CommuCnt < 3)        // 5s
        {
            if (++Protect.ClrCnt.RS485CommuRstCnt >= Protect.ClrDly.RS485CommuRstDly)        // 1s
            {
                Protect.ClrCnt.RS485CommuRstCnt = 0;
                SystemAlarm.bit.rs485 = 0;
            }
        }
        else
        {
            Protect.ClrCnt.RS485CommuRstCnt = 0;
        }
    }
    else
    {
        Protect.ClrCnt.RS485CommuRstCnt = 0;
    }
#endif

#if 0
    // CAN 通信故障复位:连续 1min 有通信数据(5s内有接收到arm数据)清除故障位
    if (SystemAlarm.bit.CAN == 1)
    {
        if (Protect.Count.CANCommuCnt < 10)        // 5s
        {
            if (++Protect.ClrCnt.CANCommuRstCnt > Protect.ClrDly.CANCommuRstDly)        // 2分钟
            {
                Protect.ClrCnt.CANCommuRstCnt = 0;
                SystemAlarm.bit.CAN = 0;
            }
        }
        else
        {
            Protect.ClrCnt.CANCommuRstCnt = 0;
        }
    }
    else
    {
        Protect.ClrCnt.CANCommuRstCnt = 0;
    }
#endif
}
/*************************************************
Function: CheckGridVoltLoss
Description: 电网掉电快速保护,并网状态下开始检测
Input: None
Output: None
Frequency:
*************************************************/
char GridDropflg =0;
char GridLossflg =0;
unsigned int Timer_200ms =0;
unsigned int Timer_30s =0;
unsigned int Chk_Ampl_Delay_Cnt =0;
float Er_alpha_Z0=0.0f;
float Er_alpha_Z1=0.0f;
float Er_beta_Z0=0.0f;
float Er_ampl_sqr=0.0f;
#pragma CODE_SECTION(CheckGridVoltLoss, ".TI.ramfunc");
void CheckGridVoltLoss(void)
{
    float grid1Stand, grid2Stand;
    grid1Stand = SampleRms.Grid1VoltRms * 1.414f;
    grid2Stand = SampleRms.Grid2VoltRms * 1.414f;
	float Delta_Theta;

    //&&SystemPara.GridEnhanceEn==0
    if (InvStateData.WorkState == OnGridState)
    {
        // 1.电网振荡
         if (fabs(grid1Stand* InvPhase.SinR) - fabs(SampleIns.Grid1VoltIns) >= 40.0f &&(InvStateData.OnGridCnt >= 300L))
         {
             if (++Protect.Count.Grid1Loss5Cnt >= 6)
             {
                 GridError.Grid1Err.bit.Grid1Drop1 = 1;
                 GridDropflg =1;
                 InvStateData.GridErrTZCnt = 0;
                 Protect.Count.Grid1Loss7Cnt=0;
             }
         }
         else
         {
//             if(Protect.Count.Grid1Loss5Cnt> 6)
//             {
//                 Protect.Count.Grid1Loss5Cnt = 5;
//             }
             if(Protect.Count.Grid1Loss5Cnt> 0)
             {
                 Protect.Count.Grid1Loss5Cnt--;
             }
             if(++Protect.Count.Grid1Loss7Cnt>100000)
             {
                 Protect.Count.Grid1Loss7Cnt=100000;
                 GridError.Grid1Err.bit.Grid1Drop1 = 0;
             }
         }
         // 2.电网振荡
         if((fabs(InvPhase.SinR)>0.7) && (InvStateData.OnGridCnt >= 300L))
 //        if(fabs(IrCtrl.Sin)>0.7)
         {
             if(fabs(SampleIns.Grid1VoltIns) <40.0)
             {
                 if (++Protect.Count.Grid1Loss4Cnt >= 6)//600us
                 {
                     Protect.Count.Grid1Loss4Cnt = 0;
                     GridError.Grid1Err.bit.Grid1Drop1 = 1;
                     GridDropflg =3;
                     Protect.ClrCnt.Grid1LossRstCnt1 = 0;     // 清除电网掉电复位计数
                 }
             }
             else
             {
                 Protect.Count.Grid1Loss4Cnt = 0;
             }
         }
         else
         {
 //            GridError.Grid1Err.bit.Grid1Drop1 = 0;
             Protect.Count.Grid1Loss4Cnt =0;
         }
         if((fabs(InvPhase.SinR)>0.2) && (InvStateData.OnGridCnt >= 300L))
        {
             if(fabs(SampleIns.Grid1VoltIns) <20.0)//20v
             {
                 if (++Protect.Count.Grid1Loss10Cnt >= 6)//800us
                 {
                     Protect.Count.Grid1Loss10Cnt = 0;
                     Protect.ClrCnt.Grid1LossRstCnt1 = 0;     // 清除电网掉电复位计数
                     GridError.Grid1Err.bit.Grid1Drop1 = 1;
                     GridDropflg =5;
                 }
             }
             else
             {
                 Protect.Count.Grid1Loss10Cnt = 0;
             }
         }
         else
         {
             Protect.Count.Grid1Loss10Cnt =0;
         }
         if(++Protect.ClrCnt.Grid1LossRstCnt1>200000)//20s
         {
             GridError.Grid1Err.bit.Grid1Drop1 = 0;
             Protect.ClrCnt.Grid1LossRstCnt1 = 0;     // 清除电网掉电复位计数
         }
         if((fabs(InvPhase.SinS)>0.7) && (InvStateData.OnGridCnt >= 300L))
 //        if(fabs(InvPhase.SinS)>0.7)
         {
             if(fabs(SampleIns.Grid2VoltIns) <40.0)
             {
                 if (++Protect.Count.Grid2Loss4Cnt >= 8)//600us
                 {
                     Protect.Count.Grid2Loss4Cnt = 0;
                     GridError.Grid2Err.bit.Grid2Drop1 = 1;
                     GridDropflg =4;
                     Protect.ClrCnt.Grid2LossRstCnt1 = 0;     // 清除电网掉电复位计数
                 }
             }
             else
             {
                 Protect.Count.Grid2Loss4Cnt = 0;            //@sjj容易误报电网振荡
             }
         }
         else
         {
 //            GridError.Grid2Err.bit.Grid2Drop1 = 0;
             Protect.Count.Grid2Loss4Cnt = 0;            //@sjj容易误报电网振荡
         }
         if((fabs(InvPhase.SinS)>0.2) && (InvStateData.OnGridCnt >= 300L))
          {
              if(fabs(SampleIns.Grid2VoltIns) <20.0)
              {
                  if (++Protect.Count.Grid2Loss10Cnt >= 8)//600us
                  {
                      Protect.Count.Grid2Loss10Cnt = 0;
                      Protect.ClrCnt.Grid2LossRstCnt1 = 0;     // 清除电网掉电复位计数
                      GridError.Grid2Err.bit.Grid2Drop1 = 1;
                      GridDropflg =6;
                  }
              }
              else
              {
                  Protect.Count.Grid2Loss10Cnt = 0;            //@sjj容易误报电网振荡
              }
          }
          else
          {
              Protect.Count.Grid2Loss10Cnt = 0;            //@sjj容易误报电网振荡
          }
         if(++Protect.ClrCnt.Grid2LossRstCnt1>200000)//20s
         {
             GridError.Grid2Err.bit.Grid2Drop1 = 0;
             Protect.ClrCnt.Grid2LossRstCnt1 = 0;     // 清除电网掉电复位计数
         }

         // 电网2
         if (fabs(grid2Stand* InvPhase.SinS) - fabs(SampleIns.Grid2VoltIns) >= 40.0f&& (InvStateData.OnGridCnt >= 300L))//
         {
             if (++Protect.Count.Grid2Loss5Cnt >= 6)
             {
                 GridError.Grid2Err.bit.Grid2Drop1 = 1;
                 Protect.Count.Grid2Loss7Cnt =0;
                 GridDropflg =2;
                 InvStateData.GridErrTZCnt = 0;
             }
         }
         else
         {
//             if(Protect.Count.Grid2Loss5Cnt > 6)
//             {
//                 Protect.Count.Grid2Loss5Cnt = 5;
//             }
             if(Protect.Count.Grid2Loss5Cnt> 0)
             {
                 Protect.Count.Grid2Loss5Cnt--;
             }
             if(++Protect.Count.Grid2Loss7Cnt>100000)
             {
                 Protect.Count.Grid2Loss7Cnt=100000;
                 GridError.Grid2Err.bit.Grid2Drop1 = 0;
             }

         }
         if(GridError.Grid1Err.bit.Grid1Drop1==1|| GridError.Grid2Err.bit.Grid2Drop1==1)//发生电网震荡持续30s后再上报告警
          {
              if(++Timer_30s>300000)//30s
              {
                  Timer_30s=0;
                  SystemAlarm.bit.GridOsc = (GridError.Grid1Err.bit.Grid1Drop1|| GridError.Grid2Err.bit.Grid2Drop1) ? 1 : 0;
              }

          }
          else
          {
              Timer_30s=0;
              SystemAlarm.bit.GridOsc =0;
          }
         //1.掉电---------------------------------------------------
//        if(SystemPara.GridEnhanceEn==0)
//        {
            if ((ArmWriteData.AgingLable == 0) && (SystemPara.DieselEngineMode == 0)&&(SystemPara.GridEnhanceEn==0))
            {
               // 主要针对电网掉电50%以上立马封波
                if (fabs(SampleIns.Grid1VoltIns - grid1Stand* InvPhase.SinR) >= 70.0f)
                {
                    GridError.Grid1Err.bit.GridDrop = 1;
                    InvStateData.GridErrTZCnt = 0;
                }
                if (fabs(SampleIns.Grid2VoltIns - grid2Stand* InvPhase.SinS) >= 70.0f)
                {
                    GridError.Grid2Err.bit.GridDrop = 1;
                    InvStateData.GridErrTZCnt = 0;
                }
            }
            else
            {
                GridError.Grid1Err.bit.GridDrop = 0;
                GridError.Grid2Err.bit.GridDrop = 0;
            }

            // 电网掉电检测,连续 3ms瞬时电压小于30V,欠压点小于30V的时间为2.5ms
            if (fabs(SampleIns.Grid1VoltIns) < Protect.Value.GridLoss1)
            {
                if (++Protect.Count.Grid1LossCnt >= Protect.Delay.GridLossDly)
                {
                    Protect.Count.Grid1LossCnt = 0;
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 1;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid1LossCnt = 0;
            }

            // 连续3ms电压和锁相理想电压相差大于过欠压保护值
            if (fabs(SampleIns.Grid1VoltIns - 142.83f* InvPhase.SinR) > Protect.Value.GridLoss2)//*InvPhase.SinR
            {
                if(fabs(SampleIns.Grid1VoltIns - 142.83f* InvPhase.SinR) > 1.5*Protect.Value.GridLoss2)
                {
                    Protect.Count.Grid1Loss2Cnt++;
                }

                if (++Protect.Count.Grid1Loss2Cnt >= Protect.Delay.GridLoss2Dly)
                {
                    Protect.Count.Grid1Loss2Cnt = 0;
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 2;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid1Loss2Cnt = 0;
            }


            if (fabs(SampleIns.Grid2VoltIns) < Protect.Value.GridLoss1)
            {
                if (++Protect.Count.Grid2LossCnt >= Protect.Delay.GridLossDly)
                {
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 3;
                    Protect.Count.Grid2LossCnt = 0;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid2LossCnt = 0;
            }

            if (fabs(SampleIns.Grid2VoltIns - 142.83f* InvPhase.SinS) > Protect.Value.GridLoss2) //*InvPhase.SinS
            {
                if(fabs(SampleIns.Grid2VoltIns - 142.83f* InvPhase.SinS) > 1.5*Protect.Value.GridLoss2)
                {
                    Protect.Count.Grid2Loss2Cnt++;
                }

                if (++Protect.Count.Grid2Loss2Cnt >= Protect.Delay.GridLoss2Dly)
                {
                    Protect.Count.Grid2Loss2Cnt = 0;
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 4;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid2Loss2Cnt = 0;
            }

            //3、检测空载电网掉电
            if(++Protect.Count.GridLossWindow <=3)
            {
                FuncPara.DletaV1 += fabs(SampleIns.Grid1VoltIns - FuncPara.Grid1VOld);
                FuncPara.DletaV2 += fabs(SampleIns.Grid2VoltIns - FuncPara.Grid2VOld);
            }
            else
            {
                if(FuncPara.DletaV1 < 2)
                {
                    Protect.Count.Grid1LossRiskCnt++;
                }
                else
                {
                    Protect.Count.Grid1LossRiskCnt = 0;
                }
                if(FuncPara.DletaV2 < 2)
                {
                    Protect.Count.Grid2LossRiskCnt++;
                }
                else
                {
                    Protect.Count.Grid2LossRiskCnt = 0;
                }
                Protect.Count.GridLossWindow = 0;
                FuncPara.DletaV1 = 0;
                FuncPara.DletaV2 = 0;
                FuncPara.Grid1VOld = SampleIns.Grid1VoltIns;
                FuncPara.Grid2VOld = SampleIns.Grid2VoltIns;
            }
            if((Protect.Count.Grid1LossRiskCnt>=5)||(Protect.Count.Grid2LossRiskCnt>=5))
            {
                SystemAlarm.bit.LossGrid = 1;//电网注入7次谐波，这里触发并网切离网，先屏蔽
                GridLossflg = 5;
            }
 /*
            // 并网情况下掉电切离网
                        // R相

                        // S相
                        if((InvStateData.WorkState==OnGridState) && (SampleIns.Grid2CurrIns<0.5) && (IsCtrl.RmpCtrl.TargetValue>1.0))
                        {
                            if (++Protect.Count.Grid2Loss11Cnt >= 20)   //2ms
                            {
                                SystemAlarm.bit.LossGrid = 1;
                                GridError.Grid1Err.bit.LossGrid = 1;
                                GridLossflg = 9;
                                Protect.Count.Grid2Loss11Cnt = 0;
                                Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                            }
                        }
                        else
                        {
                            Protect.Count.Grid2Loss11Cnt = 0;
                        }
*/
    //        // 3.瞬时电压是额定的1.2倍，防止电压过冲
            // 电网1
            if (fabs(SampleIns.Grid1VoltIns) > Protect.Value.GridLoss3)
            {
                if (++Protect.Count.Grid1Loss3Cnt >= Protect.Delay.GridLoss3Dly)
                {
                    GridError.Grid1Err.bit.LossGrid = 1;
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 6;
                    Protect.Count.Grid1Loss3Cnt = 0;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid1Loss3Cnt = 0;
            }
            // 电网2
            if (fabs(SampleIns.Grid2VoltIns) > Protect.Value.GridLoss3)
            {
                if (++Protect.Count.Grid2Loss3Cnt >= Protect.Delay.GridLoss3Dly)
                {
                    GridError.Grid2Err.bit.LossGrid = 1;
                    SystemAlarm.bit.LossGrid = 1;
                    GridLossflg = 7;
                    Protect.Count.Grid2Loss3Cnt = 0;
                    Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                }
            }
            else
            {
                Protect.Count.Grid2Loss3Cnt = 0;
            }
//        }
            //充电模式掉电判断电网异常处理

            if(IrCtrl.RmpCtrl.SetpointValue <0)
            {
                  if(fabs(SampleIns.Grid1VoltIns - 142.83f* InvPhase.SinR) > 40)//40v
                  {
                      if (++Protect.Count.Grid1Loss8Cnt >= 30)//3ms
                      {
                          Protect.Count.Grid1Loss8Cnt = 0;
                          SystemAlarm.bit.LossGrid = 1;
                          Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                      }
                  }
                  else
                  {
                      if(Protect.Count.Grid1Loss8Cnt>0)
                      {
                          Protect.Count.Grid1Loss8Cnt--;
                      }
                  }
            }
            if(IsCtrl.RmpCtrl.SetpointValue <0)
            {
                 if(fabs(SampleIns.Grid2VoltIns - 142.83f* InvPhase.SinR) > 40)//40v
                  {
                      if (++Protect.Count.Grid2Loss8Cnt >= 30)//3ms
                      {
                          Protect.Count.Grid2Loss8Cnt = 0;
                          SystemAlarm.bit.LossGrid = 1;
                          Protect.ClrCnt.GridLossRstCnt = 0;     // 清除电网掉电复位计数
                      }
                  }
                  else
                  {
                      if(Protect.Count.Grid2Loss8Cnt>0)
                      {
                          Protect.Count.Grid2Loss8Cnt--;
                      }
                  }
            }



              if((IrCtrl.RmpCtrl.SetpointValue <0)&&(fabs(InvPhase.SinR)>0.5))
              {
                    if(fabs(SampleIns.Inv1CurrIns)<fabs(IrCtrl.RmpCtrl.finalCurr)*0.8)
                    {
                       if(++Protect.Count.Grid1Loss8Cnt>2)
                       {
                          Protect.Count.Grid1Loss8Cnt=0;
                          GridError.Grid1Err.bit.LossGrid = 1;
                       }

                    }
                    else
                    {
                      Protect.Count.Grid1Loss8Cnt=0;
                    }
              }
              else
              {
                  Protect.Count.Grid1Loss8Cnt=0;
              }
              if((IsCtrl.RmpCtrl.SetpointValue <0)&&(fabs(InvPhase.SinS)>0.5))
              {
                    if(fabs(SampleIns.Inv2CurrIns)<fabs(IsCtrl.RmpCtrl.finalCurr)*0.8)
                    {
                       if(++Protect.Count.Grid2Loss8Cnt>2)
                       {
                          Protect.Count.Grid2Loss8Cnt=0;
                          GridError.Grid2Err.bit.LossGrid = 1;
                       }

                    }
                    else
                    {
                      Protect.Count.Grid2Loss8Cnt=0;
                    }
               }
              else
              {
                  Protect.Count.Grid2Loss8Cnt=0;
              }
    }
    else
    {
        GridError.Grid1Err.bit.Grid1Drop1 = 0;
        GridError.Grid2Err.bit.Grid2Drop1 = 0;
        GridError.Grid1Err.bit.GridDrop = 0;
        GridError.Grid2Err.bit.GridDrop = 0;
        SystemAlarm.bit.GridOsc= 0;

        Protect.Count.Grid2LossCnt = 0;
        Protect.Count.Grid1LossCnt = 0;

        Protect.Count.Grid2Loss2Cnt = 0;
        Protect.Count.Grid1Loss2Cnt = 0;

        Protect.Count.Grid1Loss3Cnt = 0;
        Protect.Count.Grid2Loss3Cnt = 0;

        Protect.Count.Grid1Loss4Cnt = 0;
        Protect.Count.Grid2Loss4Cnt = 0;

        Protect.Count.Grid1Loss5Cnt = 0;
        Protect.Count.Grid2Loss5Cnt = 0;

        Protect.Count.Grid1Loss6Cnt = 0;
        Protect.Count.Grid2Loss6Cnt = 0;

        Protect.Count.Grid1LossRiskCnt = 0;
        Protect.Count.Grid2LossRiskCnt = 0;
    }

    // 10s清除告警
    if (SystemAlarm.bit.LossGrid)
    {
//        if((SystemAlarm.all & 0xF) == 0)
//        {
        if (++Protect.ClrCnt.GridLossRstCnt >= Protect.ClrDly.GridLossRstDly)
        {
            SystemAlarm.bit.LossGrid = 0;
        }
//        }
    }
    else
    {
        Protect.ClrCnt.GridLossRstCnt = 0;
    }

    // 并网状态下BUS高压 逆变锁波持续 5s
//    if (InvStateData.WorkState == OnGridState)
//    {
//        if ((SampleIns.Bus1VoltIns > (ProtectGroup.BUS1H.Protectlimit-5.0f))
//                || (SampleIns.Bus2VoltIns > (ProtectGroup.BUS2H.Protectlimit-5.0f)))
//        {
//            GridError.Grid1Err.bit.BusOV = 1;
//            InvStateData.GridErrTZCnt = 0;
//        }
//    }
//    if (GridError.Grid1Err.bit.BusOV == 1)
//    {
//        if (++Protect.ClrCnt.BusOVTZRstCnt >= (10L*1000L*5L))
//        {
//            GridError.Grid1Err.bit.BusOV = 0;
//            Protect.ClrCnt.BusOVTZRstCnt = 0;
//        }
//    }
//    else
//    {
//        Protect.ClrCnt.BusOVTZRstCnt = 0;
//    }

}
/*************************************************
Function: CheckInvLoad
Description: 逆变输出过载保护,离网状态下触发
100%~110%不超过10min,110%~120%不超过10s,大于120%不超过500ms,2min后自动清除故障
Input: None
Output: None
Frequency:10ms
*************************************************/
void CheckInvLoad(void)
{
    //离网过载
    if ((InvStateData.WorkState == OffGridState) || (InvStateData.WorkState == UpsState))
    {
        // 逆变 1
        if((TestProtectDetect(SystemPower.ApLoad1.PowerTemp,&FaultCnt.Inv1PowerOLCnt1,2750 ,60000))
         ||(TestProtectDetect(SystemPower.ApLoad1.PowerTemp,&FaultCnt.Inv1PowerOLCnt2,3025 ,1000))
         ||(TestProtectDetect(SystemPower.ApLoad1.PowerTemp,&FaultCnt.Inv1PowerOLCnt3,3300 ,50))
         ||TestProtectDetect(SampleRms.Load1CurrRms      ,&FaultCnt.Inv1CurrOLCnt1 ,27.5 ,60000)
         ||(TestProtectDetect(SampleRms.Load1CurrRms     ,&FaultCnt.Inv1CurrOLCnt2 ,30.25,1000))
         ||(TestProtectDetect(SampleRms.Inv1CurrRms     ,&FaultCnt.Inv1CurrOLCnt3 ,33   ,50)))
        {
            SystemError.Fault3.bit.L1OverLoad = 1;
        }

        // 逆变 2
        if((TestProtectDetect(SystemPower.ApLoad2.PowerTemp,&FaultCnt.Inv2PowerOLCnt1,2750 ,60000))
         ||(TestProtectDetect(SystemPower.ApLoad2.PowerTemp,&FaultCnt.Inv2PowerOLCnt2,3025 ,1000))
         ||(TestProtectDetect(SystemPower.ApLoad2.PowerTemp,&FaultCnt.Inv2PowerOLCnt3,3300 ,50))
         ||(TestProtectDetect(SampleRms.Load2CurrRms     ,&FaultCnt.Inv2CurrOLCnt1 ,27.5 ,60000))
         ||(TestProtectDetect(SampleRms.Load2CurrRms     ,&FaultCnt.Inv2CurrOLCnt2 ,30.25,1000))
         ||(TestProtectDetect(SampleRms.Inv2CurrRms     ,&FaultCnt.Inv2CurrOLCnt3 ,33   ,50)))
        {
            SystemError.Fault3.bit.L2OverLoad = 1;
        }
    }
    else
    {
        memset(&FaultCnt.Inv1PowerOLCnt1, 0, 12*sizeof(uint16_t));//清0 12个用于该模式下的保护计数
    }

    // 旁路负载电流过载
    if ((InvStateData.WorkState == OnGridState)||(InvStateData.WorkState == ByPass))
    {
        //L1
        if((TestProtectDetect(SystemPower.ApLoad1.PowerTemp,&FaultCnt.Bp1PowerOLCnt1  ,3200,1000))
         ||(TestProtectDetect(SystemPower.ApLoad1.PowerTemp,&FaultCnt.Bp1PowerOLCnt2  ,4000,100))
         ||(TestProtectDetect(SystemPower.Grid1.PowerTemp,&FaultCnt.Grid1PowerOLCnt1,4800,1000))
         ||(TestProtectDetect(SystemPower.Grid1.PowerTemp,&FaultCnt.Grid1PowerOLCnt2,5500,100))
         ||(TestProtectDetect(SampleRms.Load1CurrRms     ,&FaultCnt.Bp1CurrOLCnt1   ,32  ,1000))
         ||(TestProtectDetect(SampleRms.Load1CurrRms     ,&FaultCnt.Bp1CurrOLCnt2   ,40  ,100))
         ||(TestProtectDetect(SampleRms.Grid1CurrRms     ,&FaultCnt.Grid1CurrOLCnt1 ,48  ,1000))
         ||(TestProtectDetect(SampleRms.Grid1CurrRms     ,&FaultCnt.Grid1CurrOLCnt2 ,55  ,100)))
        {
            SystemError.Fault3.bit.L1OverLoad = 1;
        }
        //L2
        if((TestProtectDetect(SystemPower.ApLoad2.PowerTemp,&FaultCnt.Bp2PowerOLCnt1  ,3200,1000))
         ||(TestProtectDetect(SystemPower.ApLoad2.PowerTemp,&FaultCnt.Bp2PowerOLCnt2  ,4000,100))
         ||(TestProtectDetect(SystemPower.Grid2.PowerTemp,&FaultCnt.Grid2PowerOLCnt1,4800,1000))
         ||(TestProtectDetect(SystemPower.Grid2.PowerTemp,&FaultCnt.Grid2PowerOLCnt2,5500,100))
         ||(TestProtectDetect(SampleRms.Load2CurrRms     ,&FaultCnt.Bp2CurrOLCnt1   ,32  ,1000))
         ||(TestProtectDetect(SampleRms.Load2CurrRms     ,&FaultCnt.Bp2CurrOLCnt2   ,40  ,100))
         ||(TestProtectDetect(SampleRms.Grid2CurrRms     ,&FaultCnt.Grid2CurrOLCnt1 ,48  ,1000))
         ||(TestProtectDetect(SampleRms.Grid2CurrRms     ,&FaultCnt.Grid2CurrOLCnt2 ,55  ,100)))
        {
            SystemError.Fault3.bit.L2OverLoad = 1;
        }
    }
    else
    {
        memset(&FaultCnt.Grid1PowerOLCnt1, 0, 16*sizeof(uint16_t));//清0 16个用于该模式下的保护计数
    }

    //故障恢复，放在3次恢复逻辑中
}
/*************************************************
Function: CheckInvTotalLoad
Description: 逆变总功率过载保护,当电池包数量为2包时限制离网负载两相最大7600W
100%~110%不超过10min,110%~150%不超过10s,大于150%不超过100ms,2min后自动清除故障
Input: None
Output: None
Frequency:1ms
*************************************************/
void CheckInvTotalLoad(void)
{
//    float totalPower = 0;
//    float totalTempPower = 0;
//
//    totalPower = SystemPower.ApInv1.Average + SystemPower.ApInv2.Average;
//    totalTempPower = SystemPower.ApInv1.PowerTemp + SystemPower.ApInv2.PowerTemp;

//    // 如果电池包数量为2,触发保护检查
//    if ((InvStateData.WorkState == OffGridState) && (SystemPara.BATPackNum == 2))
//    {
//        // 逆变 1 过载保护
//        if (totalPower >= TotalOverLoad.Value.INVLoadOver1)
//        {
//            // 过载100%~110%,10min
//            if (++TotalOverLoad.Count.INV1LoadOver1Cnt >= TotalOverLoad.Delay.INVLoadOver1Dly)
//            {
//                TotalOverLoad.Count.INV1LoadOver1Cnt = 0;
//
//                SystemError.Fault3.bit.L1OverLoad = 1;
//            }
//        }
//        else
//        {
//            TotalOverLoad.Count.INV1LoadOver1Cnt = 0;
//        }
//        // 过载110%~150%
//        if (totalPower >= TotalOverLoad.Value.INVLoadOver2)
//        {
//            if (++TotalOverLoad.Count.INV1LoadOver2Cnt >= TotalOverLoad.Delay.INVLoadOver2Dly)
//            {
//                TotalOverLoad.Count.INV1LoadOver2Cnt = 0;
//
//                SystemError.Fault3.bit.L1OverLoad = 1;
//            }
//        }
//        else
//        {
//            TotalOverLoad.Count.INV1LoadOver2Cnt = 0;
//        }
//        // 大于150% 100ms
//        if (totalTempPower >= TotalOverLoad.Value.INVLoadOver3)
//        {
//            if (++TotalOverLoad.Count.INV1LoadOver3Cnt >= TotalOverLoad.Delay.INVLoadOver3Dly)
//            {
//                TotalOverLoad.Count.INV1LoadOver3Cnt = 0;
//
//                SystemError.Fault3.bit.L1OverLoad = 1;
//            }
//        }
//        else
//        {
//            TotalOverLoad.Count.INV1LoadOver3Cnt = 0;
//        }
//    }
//    else
//    {
//        TotalOverLoad.Count.INV1LoadOver1Cnt = 0;
//        TotalOverLoad.Count.INV1LoadOver2Cnt = 0;
//        TotalOverLoad.Count.INV1LoadOver3Cnt = 0;
//    }
}

/*************************************************
Function: CheckBusVolt
Description: 母线过欠压检测与故障复位
 * 母线过压恢复策略:2分钟后如果母线都小于 20V 则重重新启动系统-清除故障码
 * 母线欠压恢复策略:2分钟后如果母线都小于 20V 则重重新启动系统-清除故障码
Input: None
Output: None
Frequency:10K
*************************************************/
#pragma CODE_SECTION(CheckBusVolt, ".TI.ramfunc");
void CheckBusVolt(void)
{
    uint32_t BusHRstDly  = 10l*1000l*150l;//2min30s
    uint32_t BusLRstDly  = 10l*1000l*150l;
    uint32_t BatLRstDly  = 100000;//10s
    // ---------------------软件BUS过压检测----------------------------------
    if(Protect.Enable.bit.BusVoltOver)
    {
        if((TestProtectDetect(SampleIns.Bus1VoltIns,&FaultCnt.Bus1HVCnt,275,100))    //@20260331半母线容量不足，投载时易冲高
         ||(TestProtectDetect(SampleIns.Bus2VoltIns,&FaultCnt.Bus2HVCnt,275,100))
         ||(TestProtectDetect(SampleIns.BUSVoltIns ,&FaultCnt.BusHVCnt ,480,100)))
        {
            SystemError.Fault1.bit.BusHigh = 1;
        }
    }

    // ---------------------硬件件BUS过压检测----------------------------------
    if (BUS_OV_DATA == 0)
    {
        SystemError.Fault1.bit.BusHighH = 1;
    }

    // ---------------------软件BUS欠压检测----------------------------------
    if(PVSystemData.PVSourceType == PVBoard)
    {
        if((Protect.Enable.bit.BusVoltLow)&&((SystemError.ArmFault&0x02)||(FuncPara.BatState == 0x22)))//无BMS或电池包开机
        {
            if((TestProtectDetect(-SampleIns.Bus1VoltIns,&FaultCnt.Bus1LVCnt,-100,100))
             ||(TestProtectDetect(-SampleIns.Bus2VoltIns,&FaultCnt.Bus2LVCnt,-100,100))
             ||(TestProtectDetect(-SampleIns.BUSVoltIns ,&FaultCnt.BusLVCnt ,-315,100)))
            {
                SystemError.Fault1.bit.BusLow = 1;
            }

        }

        // ------------------------欠压故障恢复-------------------------
        if (SystemError.Fault1.bit.BusLow)
        {
    //        AUS_RLY_ON;
            SystemError.Fault1.bit.BusHigh = 0;
            Protect.ClrCnt.BusVoltOverRstCnt = 0;//互斥处理

            if((SampleIns.Bus1VoltIns > 155)
                    &&(SampleIns.Bus2VoltIns > 155)
                    &&(SampleRms.BUSVoltFilter200 > 320))
            {
                if (++Protect.ClrCnt.BusVoltLowRstCnt >= BusLRstDly)
                {
                    Protect.ClrCnt.BusVoltLowRstCnt = 0;
                    SystemError.Fault1.bit.BusLow = 0;
                }
            }
            else
            {
                Protect.ClrCnt.BusVoltLowRstCnt = 0;
            }
        }
        else
        {
    //        AUS_RLY_OFF;//不欠压就不吸合，降低功耗
        }
    }
    else
    {
//        SystemError.Fault3.bit.BATVoltLow = SystemAlarm.bit.PV1Low && SystemAlarm.bit.PV2Low
//                                            && SystemAlarm.bit.PV3Low && SystemAlarm.bit.PV4Low;
        SystemError.Fault1.bit.BusLow = 0;
    }

    // ---------------------直流输入低压（电池低压）
//    if ((Protect.Enable.bit.BATVoltLow)&&(!SystemError.Fault3.bit.BATVoltLow))
//    {

//    }

    // ----------------------------直流输入低压恢复-----------------------------
    if(SystemError.Fault3.bit.BATVoltLow)
    {
        if((SampleRms.BUSVoltFilter200 > 320)
                &&(SampleIns.Bus2VoltIns > 155)
                &&(SampleIns.Bus1VoltIns > 155))
        {
            if(++Protect.ClrCnt.BATVoltLowRstCnt > BatLRstDly)
            {
                Protect.ClrCnt.BATVoltLowRstCnt = 0;
                SystemError.Fault3.bit.BATVoltLow = 0;
            }
        }
        else
        {
            Protect.ClrCnt.BATVoltLowRstCnt = 0;
        }
    }

    // ----------------------------过压故障恢复-----------------------------
    if(SystemError.Fault1.bit.BusHigh)
    {
        SystemError.Fault1.bit.BusLow = 0;
        Protect.ClrCnt.BusVoltLowRstCnt = 0;//互斥处理

        if((SampleIns.Bus1VoltIns < 230)
                &&(SampleIns.Bus2VoltIns < 230)
                &&(SampleIns.BUSVoltIns < 450))
        {
            if(++Protect.ClrCnt.BusVoltOverRstCnt > BusHRstDly)
            {
                Protect.ClrCnt.BusVoltOverRstCnt = 0;
                SystemError.Fault1.bit.BusHigh = 0;
            }
        }
        else
        {
            Protect.ClrCnt.BusVoltOverRstCnt = 0;
        }
    }

    //硬件过压不恢复


}
/*************************************************
Function: CheckPVVolt
Description: PV过欠压保护
Input: None
Output: None
Frequency: 10K
*************************************************/
#pragma CODE_SECTION(CheckPVVolt, ".TI.ramfunc");
void CheckPVVolt(void)
{
    uint16_t PVVoltUpLimit = 454;//上限450V
    uint16_t PVVoltDownLimit = 79;//下限80V
    uint16_t PVOVRstDly = 20000;//2s
    uint16_t PVUVRstDly = 20000;

    uint32_t PVUVAutoRstDly = 10l*1000l*120l;//2min
    // PV1 开机电压
    PVSystemData.PV1Available = (SampleRms.PV1VoltFilter8 > PVVoltDownLimit && SampleRms.PV1VoltFilter8 <= PVVoltUpLimit);
    // PV2
    PVSystemData.PV2Available = (SampleRms.PV2VoltFilter8 > PVVoltDownLimit && SampleRms.PV2VoltFilter8 <= PVVoltUpLimit);
    // PV3
    PVSystemData.PV3Available = (SampleRms.PV3VoltFilter8 > PVVoltDownLimit && SampleRms.PV3VoltFilter8 <= PVVoltUpLimit);
    // PV4
    PVSystemData.PV4Available = (SampleRms.PV4VoltFilter8 > PVVoltDownLimit && SampleRms.PV4VoltFilter8 <= PVVoltUpLimit);

    // 严重高压故障不复位
    if((TestProtectDetect(SampleIns.PV1VoltIns,&FaultCnt.PV1OVBCnt,465,10))
     ||(TestProtectDetect(SampleIns.PV2VoltIns,&FaultCnt.PV2OVBCnt,465,10))
     ||(TestProtectDetect(SampleIns.PV3VoltIns,&FaultCnt.PV3OVBCnt,465,10))
     ||(TestProtectDetect(SampleIns.PV4VoltIns,&FaultCnt.PV4OVBCnt,465,10)))
    {
        SystemError.Fault4.bit.PVOVBad = 1;
    }

    //----------------------------PV过压-------------------------------------
    if(Protect.Enable.bit.PVVoltOver)
    {
        // PV软件长时过压保护455V
        if(TestProtectDetect(SampleIns.PV1VoltIns ,&FaultCnt.PV1HVCnt,455,100))
        SystemError.Fault2.bit.PV1OverVolt = 1;
        // PV2长时过压保护
        if(TestProtectDetect(SampleIns.PV2VoltIns ,&FaultCnt.PV2HVCnt,455,100))
        SystemError.Fault2.bit.PV2OverVolt = 1;
        // PV3长时过压保护
        if(TestProtectDetect(SampleIns.PV3VoltIns ,&FaultCnt.PV3HVCnt,455,100))
        SystemError.Fault2.bit.PV3OverVolt = 1;
        // PV4长时过压保护
        if(TestProtectDetect(SampleIns.PV4VoltIns ,&FaultCnt.PV4HVCnt,455,100))
        SystemError.Fault4.bit.PV4OverVolt = 1;
    }

    // --------------------------PV欠压----------------------------
    if(Protect.Enable.bit.PVVoltLow)
    {
        if (PV1WorkState.State == MPPT_STATE)
        {
            if(TestProtectDetect(-SampleIns.PV1VoltIns ,&FaultCnt.PV1LVCnt,-64,100))
            SystemAlarm.bit.PV1Low = 1;
        }
        else
        {
            FaultCnt.PV1LVCnt = 0;
        }
        //------------------------PV2--------------------------------
        if (PV2WorkState.State == MPPT_STATE)
        {
            if(TestProtectDetect(-SampleIns.PV2VoltIns ,&FaultCnt.PV2LVCnt,-64,100))
            SystemAlarm.bit.PV2Low = 1;
        }
        else
        {
            FaultCnt.PV2LVCnt = 0;
        }
        //------------------------PV3--------------------------------
        if (PV3WorkState.State == MPPT_STATE)
        {
            if(TestProtectDetect(-SampleIns.PV3VoltIns ,&FaultCnt.PV3LVCnt,-64,100))
            SystemAlarm.bit.PV3Low = 1;
        }
        else
        {
            FaultCnt.PV3LVCnt = 0;
        }
        //------------------------PV4--------------------------------
        if (PV4WorkState.State == MPPT_STATE)
        {
            if(TestProtectDetect(-SampleIns.PV4VoltIns ,&FaultCnt.PV4LVCnt,-64,100))
            SystemAlarm.bit.PV4Low = 1;
        }
        else
        {
            FaultCnt.PV4LVCnt = 0;
        }
    }

    //长时过压恢复
    if (SystemError.Fault2.bit.PV1OverVolt)
    {
        if(TestProtectDetect(-SampleIns.PV1VoltIns ,&Protect.ClrCnt.PV1VoltOverRstCnt,-450,PVOVRstDly))
        SystemError.Fault2.bit.PV1OverVolt = 0;
    }

    if (SystemError.Fault2.bit.PV2OverVolt)
    {
        if(TestProtectDetect(-SampleIns.PV2VoltIns ,&Protect.ClrCnt.PV2VoltOverRstCnt,-450,PVOVRstDly))
        SystemError.Fault2.bit.PV2OverVolt = 0;
    }

    if (SystemError.Fault2.bit.PV3OverVolt)
    {
        if(TestProtectDetect(-SampleIns.PV3VoltIns ,&Protect.ClrCnt.PV3VoltOverRstCnt,-450,PVOVRstDly))
        SystemError.Fault2.bit.PV3OverVolt = 0;
    }

    if (SystemError.Fault4.bit.PV4OverVolt)
    {
        if(TestProtectDetect(-SampleIns.PV4VoltIns ,&Protect.ClrCnt.PV4VoltOverRstCnt,-450,PVOVRstDly))
        SystemError.Fault4.bit.PV4OverVolt = 0;
    }
    //PV欠压恢复
    if(SystemAlarm.bit.PV1Low)
    {
        if(TestProtectDetect(SampleIns.PV1VoltIns ,&Protect.ClrCnt.PV1VoltLowRstCnt1,80,PVUVRstDly))
        SystemAlarm.bit.PV1Low = 0;

        if(Protect.ClrCnt.PV1VoltLowRstCnt2++ >= PVUVAutoRstDly)
        {
            Protect.ClrCnt.PV1VoltLowRstCnt2 = 0;
            SystemAlarm.bit.PV1Low = 0;
        }
    }

    if(SystemAlarm.bit.PV2Low)
    {
        if(TestProtectDetect(SampleIns.PV2VoltIns ,&Protect.ClrCnt.PV2VoltLowRstCnt1,80,PVUVRstDly))
        SystemAlarm.bit.PV2Low = 0;

        if(Protect.ClrCnt.PV2VoltLowRstCnt2++ >= PVUVAutoRstDly)
        {
            Protect.ClrCnt.PV2VoltLowRstCnt2 = 0;
            SystemAlarm.bit.PV2Low = 0;
        }
    }

    if(SystemAlarm.bit.PV3Low)
    {
        if(TestProtectDetect(SampleIns.PV3VoltIns ,&Protect.ClrCnt.PV3VoltLowRstCnt1,80,PVUVRstDly))
        SystemAlarm.bit.PV3Low = 0;

        if(Protect.ClrCnt.PV3VoltLowRstCnt2++ >= PVUVAutoRstDly)
        {
            Protect.ClrCnt.PV3VoltLowRstCnt2 = 0;
            SystemAlarm.bit.PV3Low = 0;
        }
    }

    if(SystemAlarm.bit.PV4Low)
    {
        if(TestProtectDetect(SampleIns.PV4VoltIns ,&Protect.ClrCnt.PV4VoltLowRstCnt1,80,PVUVRstDly))
        SystemAlarm.bit.PV4Low = 0;

        if(Protect.ClrCnt.PV4VoltLowRstCnt2++ >= PVUVAutoRstDly)
        {
            Protect.ClrCnt.PV4VoltLowRstCnt2 = 0;
            SystemAlarm.bit.PV4Low = 0;
        }
    }
}
/*************************************************
Function: CheckPVCurr
Description: PV过流保护
Input: None
Output: None
Frequency: 10K
*************************************************/
typedef struct
{
    uint16_t level;             // 过流检测窗口
    uint16_t  Cnt;
}HardWareIOlowS;

char HardWareIOlow(Uint16 flag,HardWareIOlowS* v)
{
    if(++v->level <= 100)
    {
        if (flag == 1)
        {
            if(++v->Cnt >= 30)
            {
                return 1;
            }
        }
    }
    else
    {
        v->level = 0;
        v->Cnt = 0;
    }
    return 0;
}
//#pragma CODE_SECTION(CheckPVCurr, ".TI.ramfunc");
inline void CheckPVCurr(void)
{
    static HardWareIOlowS pv1HardWare,pv2HardWare,pv3HardWare,pv4HardWare;
    if(Protect.Enable.bit.PVCurrOver)
    {
        // -------------------------PV1软件过流保护--------------------------------------
        if(TestProtectDetect(SampleIns.PV1CurrIns,&FaultCnt.PV1OCCnt,18,10000))
        SystemError.Fault2.bit.PV1OverCurr = 1;
        if(TestProtectDetect(SampleIns.PV2CurrIns,&FaultCnt.PV2OCCnt,18,10000))
        SystemError.Fault2.bit.PV2OverCurr = 1;
        if(TestProtectDetect(SampleIns.PV3CurrIns,&FaultCnt.PV3OCCnt,18,10000))
        SystemError.Fault2.bit.PV3OverCurr = 1;
        if(TestProtectDetect(SampleIns.PV4CurrIns,&FaultCnt.PV4OCCnt,18,10000))
        SystemError.Fault4.bit.PV4OverCurr = 1;
    }

    if(HardWareIOlow(EPwm6Regs.TZCBCFLG.bit.DCBEVT2,&pv1HardWare))
    {
        SystemError.Fault2.bit.PV1HardOC = 1;
        SystemError.Fault2.bit.PV1OverCurr = 1;
    }
    if(HardWareIOlow(EPwm7Regs.TZCBCFLG.bit.DCBEVT2,&pv2HardWare))
    {
        SystemError.Fault2.bit.PV2HardOC = 1;
        SystemError.Fault2.bit.PV2OverCurr = 1;
    }
    if(HardWareIOlow(EPwm10Regs.TZCBCFLG.bit.DCBEVT2,&pv3HardWare))
    {
        SystemError.Fault2.bit.PV3HardOC = 1;
        SystemError.Fault2.bit.PV3OverCurr = 1;
    }
    if(HardWareIOlow(EPwm5Regs.TZCBCFLG.bit.DCBEVT2,&pv4HardWare))
    {
        SystemError.Fault4.bit.PV4HardOC = 1;
        SystemError.Fault4.bit.PV4OverCurr = 1;
    }

    if(SystemError.Fault2.bit.PV1OverCurr == 0)
        SystemError.Fault2.bit.PV1HardOC = 0;
    if(SystemError.Fault2.bit.PV2OverCurr == 0)
        SystemError.Fault2.bit.PV2HardOC = 0;
    if(SystemError.Fault2.bit.PV3OverCurr == 0)
        SystemError.Fault2.bit.PV3HardOC = 0;
    if(SystemError.Fault4.bit.PV4OverCurr == 0)
        SystemError.Fault4.bit.PV4HardOC = 0;

//    if(HardWareIOlow(EPwm6Regs.TZCBCFLG.bit.DCBEVT2,&pv1HardWare))
//    SystemError.Fault2.bit.PV1HardOC = 1;
//    if(HardWareIOlow(EPwm7Regs.TZCBCFLG.bit.DCBEVT2,&pv2HardWare))
//    SystemError.Fault2.bit.PV2HardOC = 1;
//    if(HardWareIOlow(EPwm10Regs.TZCBCFLG.bit.DCBEVT2,&pv3HardWare))
//    SystemError.Fault2.bit.PV3HardOC = 1;
//    if(HardWareIOlow(EPwm5Regs.TZCBCFLG.bit.DCBEVT2,&pv4HardWare))
//    SystemError.Fault4.bit.PV4HardOC = 1;

    // PV硬件过流
    if(EPwm6Regs.TZCBCFLG.bit.DCBEVT2)
    {
        EALLOW;
        EPwm6Regs.TZCBCCLR.bit.DCBEVT2 = 1;
        EPwm6Regs.TZCLR.bit.DCBEVT2 = 1;
        EDIS;
    }
    // PV2硬件故障
    if(EPwm7Regs.TZCBCFLG.bit.DCBEVT2)
    {
        EALLOW;
        EPwm7Regs.TZCBCCLR.bit.DCBEVT2 = 1;
        EPwm7Regs.TZCLR.bit.DCBEVT2 = 1;
        EDIS;
    }
    // PV3硬件故障
    if(EPwm10Regs.TZCBCFLG.bit.DCBEVT2)
    {
        EALLOW;
        EPwm10Regs.TZCBCCLR.bit.DCBEVT2 = 1;
        EPwm10Regs.TZCLR.bit.DCBEVT2 = 1;
        EDIS;
    }
    // PV4硬件故障
    if (EPwm5Regs.TZCBCFLG.bit.DCBEVT2)
    {
        EALLOW;
        EPwm5Regs.TZCBCCLR.bit.DCBEVT2 = 1;
        EPwm5Regs.TZCLR.bit.DCBEVT2 = 1;
        EDIS;
    }

    //PV过流恢复放在3次恢复逻辑中
}

/*************************************************
Function: CheckBATVolt
Description: BAT过压保护
Input: None
Output: None
Frequency: 10K
*************************************************/
#if 0
#pragma CODE_SECTION(CheckBATVolt, ".TI.ramfunc");
void CheckBATVolt(void)
{
    if (Protect.Enable.bit.BATVoltOver)
    {
        // 瞬时过压
        if (SampleIns.BUSVoltIns > Protect.Value.BATVoltOverIns)
        {
            if (++Protect.Count.BATVoltOverInsCnt >= Protect.Delay.BATVoltOverInsDly)
            {
                Protect.Count.BATVoltOverInsCnt = 0;

                SystemError.Fault3.bit.BATVoltOver = 1;
            }
        }
        else
        {
            Protect.Count.BATVoltOverInsCnt = 0;
        }
        // 慢保护
        if (SampleIns.BUSVoltIns > Protect.Value.BATVoltOver)
        {
            if (++Protect.Count.BATVoltOverCnt >= Protect.Delay.BATVoltOverDly)
            {
                Protect.Count.BATVoltOverCnt = 0;

                SystemError.Fault3.bit.BATVoltOver = 1;
            }
        }
        else
        {
            Protect.Count.BATVoltOverCnt = 0;
        }

        // 高压故障自动复位
        if (SampleIns.BUSVoltIns < Protect.Value.BATVoltOverRst)        // 114V
        {
            if (++Protect.ClrCnt.BATVoltOverRstCnt > Protect.ClrDly.BATVoltOverRstDly)
            {
                Protect.ClrCnt.BATVoltOverRstCnt = 0;
                SystemError.Fault3.bit.BATVoltOver = 0;
            }
        }
        else
        {
            Protect.ClrCnt.BATVoltOverRstCnt = 0;
        }
    }
    // 电池低压 保护
    if (Protect.Enable.bit.BATVoltLow)
    {
        // 开机后开始检测电池低压
        if ((SampleIns.BUSVoltIns < Protect.Value.BATVoltLow) &&
                (SystemPara.SystemPowerOn == 1))
        {
            if (++Protect.Count.BATVoltLowCnt >= Protect.Delay.BATVoltLowDly)
            {
                Protect.Count.BATVoltLowCnt = 0;

                SystemError.Fault3.bit.BATVoltLow = 1;
            }

            Protect.ClrCnt.BATVoltLowRstCnt = 0;
        }
        else
        {
            Protect.Count.BATVoltLowCnt = 0;

            // 70V
            if ((SampleIns.BUSVoltIns > Protect.Value.BATVoltLowRst) ||
                    ((SystemPara.SystemPowerOn == 0) && (SystemError.Fault3.bit.BATVoltLow)))
            {
                if (++Protect.ClrCnt.BATVoltLowRstCnt > Protect.ClrDly.BATVoltLowRstDly)  // 10s
                {
                    Protect.ClrCnt.BATVoltLowRstCnt = 0;

                    SystemError.Fault3.bit.BATVoltLow = 0;
                }
            }
            else
            {
                Protect.ClrCnt.BATVoltLowRstCnt = 0;
            }
        }

        // 电池严重低压
        if ((SampleIns.BUSVoltIns <= Protect.Value.BATVoltLowB) &&
                (SystemPara.SystemPowerOn == 1))
        {
            if (++Protect.Count.BATVoltLowBCnt >= Protect.Delay.BATVoltLowBDly)
            {
                Protect.Count.BATVoltLowBCnt = 0;

                SystemError.Fault3.bit.BATVoltLow = 1;
            }
        }
        else
        {
            Protect.Count.BATVoltLowBCnt = 0;
        }
    }

    // -------------------------------电池硬件高压故障复位-------------------------------------------------------
    if (SystemError.Fault1.bit.BATHighH == 1)
    {
        if (SampleIns.BUSVoltIns < Protect.Value.BATVoltOverRst)         // 连续 10s 电压低于 114V
        {
            if (++Protect.ClrCnt.BATHVoltOverRstCnt > Protect.ClrDly.BATHVoltOverRstDly)
            {
                Protect.ClrCnt.BATHVoltOverRstCnt = 0;

                SystemError.Fault1.bit.BATHighH = 0;
            }
        }
        else
        {
            Protect.ClrCnt.BATHVoltOverRstCnt = 0;
        }
    }
    else
    {
        Protect.ClrCnt.BATHVoltOverRstCnt = 0;
    }
}
#endif

/*************************************************
Function: CheckInvterVolt
Description: 逆变输出电压检测
Input: None
Output: None
Frequency: 10ms
*************************************************/
void CheckInvterVolt(void)
{
    if(!FuncPara.RlyInvOver)
        return;

    if (Protect.Enable.bit.INVVoltFault)
    {
        if((InvStateData.WorkState == OffGridState) || (InvStateData.WorkState == UpsState))
        {
            if((TestProtectDetect(SampleRms.Inv1VoltRms  ,&FaultCnt.Inv1HVCnt    ,125 ,50))
             ||(TestProtectDetect(SampleRms.Load1VoltRms ,&FaultCnt.Inv1LoadHVCnt,125 ,50))
             ||(TestProtectDetect(-SampleRms.Inv1VoltRms ,&FaultCnt.Inv1LVCnt    ,-75 ,50))
             ||(TestProtectDetect(-SampleRms.Load1VoltRms,&FaultCnt.Inv1LoadLVCnt,-75 ,50)))
            {
                SystemError.Fault3.bit.L1InvError = 1;
            }

            if((TestProtectDetect(SampleRms.Inv2VoltRms  ,&FaultCnt.Inv2HVCnt    ,125 ,50))
             ||(TestProtectDetect(SampleRms.Load2VoltRms ,&FaultCnt.Inv2LoadHVCnt,125 ,50))
             ||(TestProtectDetect(-SampleRms.Inv2VoltRms ,&FaultCnt.Inv2LVCnt    ,-75 ,50))
             ||(TestProtectDetect(-SampleRms.Load2VoltRms,&FaultCnt.Inv2LoadLVCnt,-75 ,50)))
            {
                SystemError.Fault3.bit.L2InvError = 1;
            }
        }
        else
        {
            memset(&FaultCnt.Inv1HVCnt, 0, 8*sizeof(uint16_t));//清0 8个用于该模式下的保护计数
        }

        if(InvStateData.WorkState == OnGridState)
        {
            if(SystemPara.GridEnhanceEn==1)
            {
                if((TestProtectDetect(fabs(SampleRms.Inv1VoltRms -SampleRms.Grid1VoltRms) ,&FaultCnt.Inv1VBiasCnt  ,10 ,100))
                 ||(TestProtectDetect(fabs(SampleRms.Load1VoltRms -SampleRms.Grid1VoltRms),&FaultCnt.Load1VBiasCnt ,10 ,100)))
                {
                    SystemError.Fault3.bit.L1InvError = 1;
                }
                if((TestProtectDetect(fabs(SampleRms.Inv2VoltRms -SampleRms.Grid2VoltRms) ,&FaultCnt.Inv2VBiasCnt  ,10 ,100))
                 ||(TestProtectDetect(fabs(SampleRms.Load2VoltRms -SampleRms.Grid2VoltRms),&FaultCnt.Load2VBiasCnt ,10 ,100)))
                {
                    SystemError.Fault3.bit.L2InvError = 1;
                }
            }
            else
            {
                if((TestProtectDetect(fabs(SampleRms.Inv1VoltRms -SampleRms.Grid1VoltRms) ,&FaultCnt.Inv1VBiasCnt  ,10 ,50))
                 ||(TestProtectDetect(fabs(SampleRms.Load1VoltRms -SampleRms.Grid1VoltRms),&FaultCnt.Load1VBiasCnt ,10 ,50)))
                {
                    SystemError.Fault3.bit.L1InvError = 1;
                }
                if((TestProtectDetect(fabs(SampleRms.Inv2VoltRms -SampleRms.Grid2VoltRms) ,&FaultCnt.Inv2VBiasCnt  ,10 ,50))
                 ||(TestProtectDetect(fabs(SampleRms.Load2VoltRms -SampleRms.Grid2VoltRms),&FaultCnt.Load2VBiasCnt ,10 ,50)))
                {
                    SystemError.Fault3.bit.L2InvError = 1;
                }
            }

//
//            if((TestProtectDetect(fabs(SampleRms.Inv1VoltRms -SampleRms.Grid1VoltRms) ,&FaultCnt.Inv1VBiasCnt  ,10 ,50))
//             ||(TestProtectDetect(fabs(SampleRms.Load1VoltRms -SampleRms.Grid1VoltRms),&FaultCnt.Load1VBiasCnt ,10 ,50)))
//            {
//                SystemError.Fault3.bit.L1InvError = 1;
//            }
//            if((TestProtectDetect(fabs(SampleRms.Inv2VoltRms -SampleRms.Grid2VoltRms) ,&FaultCnt.Inv2VBiasCnt  ,10 ,50))
//             ||(TestProtectDetect(fabs(SampleRms.Load2VoltRms -SampleRms.Grid2VoltRms),&FaultCnt.Load2VBiasCnt ,10 ,50)))
//            {
//                SystemError.Fault3.bit.L2InvError = 1;
//            }
        }
        else
        {
            memset(&FaultCnt.Inv1VBiasCnt, 0, 4*sizeof(uint16_t));//清0 4个用于该模式下的保护计数
        }

    }

    //30分钟3次恢复
}

/*************************************************
Function: CheckGridVolt
Description: 电网电压异常保护
Input: None
Output: None
Frequency: 10ms
*************************************************/
void CheckGridVolt(void)
{
    if(SampleIns.CalSampleZeroFlag != 1)
        return;

    if (SystemPara.AuthenFlag == 0)
    {
        // 电网电压过压
        // -----------------------电网1过压---------------------------------------
        // 等级1
        if (SampleRms.Grid1VoltRms > Protect.Value.GridOV1)
        {
            if (++Protect.Count.Grid1OV1Cnt >= Protect.Delay.GridOV1Dly)
            {
                Protect.Count.Grid1OV1Cnt = 0;
                GridError.Grid1Err.bit.OV1 = 1;
                
            }

            Protect.ClrCnt.Grid1OV1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1OV1Cnt = 0;
            
        }

        // 等级2
        /*
        if (SampleRms.Grid1VoltRms > Protect.Value.GridOV2)
        {
            if (++Protect.Count.Grid1OV2Cnt >= Protect.Delay.GridOV2Dly)
            {
                Protect.Count.Grid1OV2Cnt = 0;
                GridError.Grid1Err.bit.OV2 = 1;
            }

            Protect.ClrCnt.Grid1OV2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1OV2Cnt = 0;
        }
         */
        // -----------------------电网2过压---------------------------------------
        // 电网2
        if (SampleRms.Grid2VoltRms > Protect.Value.GridOV1)
        {
            if (++Protect.Count.Grid2OV1Cnt >= Protect.Delay.GridOV1Dly)
            {
                Protect.Count.Grid2OV1Cnt = 0;
                GridError.Grid2Err.bit.OV1 = 1;
            }

            Protect.ClrCnt.Grid2OV1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2OV1Cnt = 0;
        }

        // 等级2
        /*
        if (SampleRms.Grid2VoltRms > Protect.Value.GridOV2)
        {
            if (++Protect.Count.Grid2OV2Cnt >= Protect.Delay.GridOV2Dly)
            {
                Protect.Count.Grid2OV2Cnt = 0;
                GridError.Grid2Err.bit.OV2 = 1;
            }

            Protect.ClrCnt.Grid2OV2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2OV2Cnt = 0;
        }
         */
        // --------------------------低压欠压保护-----------------------------------------------------
        // ---------------------------------Grid1-----------------------
        // 等级1
        if (SampleRms.Grid1VoltRms < Protect.Value.GridUV1)
        {
            if (++Protect.Count.Grid1UV1Cnt >= Protect.Delay.GridUV1Dly)
            {
                GridError.Grid1Err.bit.UV1 = 1;

                Protect.Count.Grid1UV1Cnt = 0;
            }

            Protect.ClrCnt.Grid1UV1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1UV1Cnt = 0;
        }
        // 等级2
        /*
        if (SampleRms.Grid1VoltRms < Protect.Value.GridUV2)
        {
            if (++Protect.Count.Grid1UV2Cnt >= Protect.Delay.GridUV2Dly)
            {
                GridError.Grid1Err.bit.UV2 = 1;

                Protect.Count.Grid1UV2Cnt = 0;
            }

            Protect.ClrCnt.Grid1UV2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1UV2Cnt = 0;
        }
        */
        // ---------------------------------Grid2-----------------------
        // 等级1
        if (SampleRms.Grid2VoltRms < Protect.Value.GridUV1)
        {
            if (++Protect.Count.Grid2UV1Cnt >= Protect.Delay.GridUV1Dly)
            {
                GridError.Grid2Err.bit.UV1 = 1;

                Protect.Count.Grid2UV1Cnt = 0;
            }

            Protect.ClrCnt.Grid2UV1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2UV1Cnt = 0;
        }
        // 等级2
        /*
        if (SampleRms.Grid2VoltRms < Protect.Value.GridUV2)
        {
            if (++Protect.Count.Grid2UV2Cnt >= Protect.Delay.GridUV2Dly)
            {
                GridError.Grid2Err.bit.UV2 = 1;

                Protect.Count.Grid2UV2Cnt = 0;
            }

            Protect.ClrCnt.Grid2UV2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2UV2Cnt = 0;
        }*/
    }
    else
    {
        Protect.Count.Grid1OV1Cnt = 0;
        Protect.Count.Grid1OV2Cnt = 0;
        Protect.Count.Grid1UV1Cnt = 0;
        Protect.Count.Grid1UV1Cnt = 0;

        Protect.Count.Grid2OV1Cnt = 0;
        Protect.Count.Grid2OV2Cnt = 0;
        Protect.Count.Grid2UV1Cnt = 0;
        Protect.Count.Grid2UV1Cnt = 0;
    }
    // -----------------------------过欠压复位-----------------------------
    // 电网1 OV1
    if (GridError.Grid1Err.bit.OV1 == 1 && SampleRms.Grid1VoltRms < Protect.Value.RecoveryGridOV1)
    {
        if (++Protect.ClrCnt.Grid1OV1RstCnt > Protect.ClrDly.GridOVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid1OV1RstCnt = 0;

            GridError.Grid1Err.bit.OV1 = 0;                      // 10s后故障码自动清除
        }
        GridError.Grid1Err.bit.UV1 = 0;
       
    }
    else
    {
        Protect.ClrCnt.Grid1OV1RstCnt = 0;
    }
    // 过压复位
    /*
    if (GridError.Grid1Err.bit.OV2 == 1)
    {
        if (++Protect.ClrCnt.Grid1OV2RstCnt > Protect.ClrDly.GridOVRstDly)  // 10s时间可设置
        {
            Protect.ClrCnt.Grid1OV2RstCnt = 0;

            GridError.Grid1Err.bit.OV2 = 0;                     // 10s后故障码自动清除
        }

        GridError.Grid1Err.bit.UV1 = 0;
        GridError.Grid1Err.bit.UV2 = 0;
        GridError.Grid2Err.bit.UV1 = 0;
        GridError.Grid2Err.bit.UV2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid1OV2RstCnt = 0;
    }*/
    // 电网2 OV1
    if (GridError.Grid2Err.bit.OV1 && SampleRms.Grid2VoltRms < Protect.Value.RecoveryGridOV1)
    {
        if (++Protect.ClrCnt.Grid2OV1RstCnt > Protect.ClrDly.GridOVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2OV1RstCnt = 0;

            GridError.Grid2Err.bit.OV1 = 0;                     // 10s后故障码自动清除
        }
        
        GridError.Grid2Err.bit.UV1 = 0;
       
    }
    else
    {
        Protect.ClrCnt.Grid2OV1RstCnt = 0;
    }
    // 电网2 OV2
    /*
    if (GridError.Grid2Err.bit.OV2)
    {
        if (++Protect.ClrCnt.Grid2OV2RstCnt > Protect.ClrDly.GridOVRstDly)  // 10s时间可设置
        {
            Protect.ClrCnt.Grid2OV2RstCnt = 0;

            GridError.Grid2Err.bit.OV2 = 0;                     // 10s后故障码自动清除
        }
        GridError.Grid2Err.bit.UV1 = 0;
        GridError.Grid2Err.bit.UV2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid2OV2RstCnt = 0;
    }*/
    // 电网1 UV1
    if (GridError.Grid1Err.bit.UV1 && SampleRms.Grid1VoltRms > Protect.Value.RecoveryGridUV1)
    {
        if (++Protect.ClrCnt.Grid1UV1RstCnt > Protect.ClrDly.GridUVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid1UV1RstCnt = 0;

            GridError.Grid1Err.bit.UV1 = 0;                     // 10s后故障码自动清除
        }
    }
    else
    {
        Protect.ClrCnt.Grid1UV1RstCnt = 0;
    }
    // 电网1 UV2
    /*
    if (GridError.Grid1Err.bit.UV2)
    {
        if (++Protect.ClrCnt.Grid1UV2RstCnt > Protect.ClrDly.GridUVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid1UV2RstCnt = 0;

            GridError.Grid1Err.bit.UV2 = 0;                     // 10s后故障码自动清除
        }
    }
    else
    {
        Protect.ClrCnt.Grid1UV2RstCnt = 0;
    }
     */
    // Grid2Err UV1
    if (GridError.Grid2Err.bit.UV1 && SampleRms.Grid2VoltRms > Protect.Value.RecoveryGridUV1)
    {
        if (++Protect.ClrCnt.Grid2UV1RstCnt > Protect.ClrDly.GridUVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2UV1RstCnt = 0;

            GridError.Grid2Err.bit.UV1 = 0;                     // 10s后故障码自动清除
        }
    }
    else
    {
        Protect.ClrCnt.Grid2UV1RstCnt = 0;
    }
    // Grid2Err UV2
    /*
    if (GridError.Grid2Err.bit.UV2)
    {
        if (++Protect.ClrCnt.Grid2UV2RstCnt > Protect.ClrDly.GridUVRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2UV2RstCnt = 0;

            GridError.Grid2Err.bit.UV2 = 0;                     // 10s后故障码自动清除
        }
    }
    else
    {
        Protect.ClrCnt.Grid2UV2RstCnt = 0;
    }
     */
    // 并网转离网时不报电网高压告警
    if (Protect.Enable.bit.INVInsVoltOver == 0)
    {
        //GridError.Grid1Err.bit.OV1 = 0;
       // GridError.Grid1Err.bit.OV2 = 0;
       // GridError.Grid2Err.bit.OV1 = 0;
       // GridError.Grid2Err.bit.OV2 = 0;
    }
}
/*************************************************
Function: CheckGridFreq
Description: 电网频率异常保护
Input: None
Output: None
Frequency: 10ms
*************************************************/
void CheckGridFreq(void)
{
    if (InvStateData.WorkState >= 3)
    {
        // -------------------------------电网1过频保护频率--------------------------------------
        // 等级1
        if (CaptureData.FreqR > Protect.Value.GridOF1)
        {
            // 累计时间超出设定值,设置故障码
            if (++Protect.Count.Grid1OF1Cnt >= Protect.Delay.GridOF1Dly)
            {
                GridError.Grid1Err.bit.OF1 = 1;

                Protect.Count.Grid1OF1Cnt = 0;
            }

            Protect.ClrCnt.Grid1OF1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1OF1Cnt = 0;
        }

        // 等级2
        /*
        if (CaptureData.FreqR > Protect.Value.GridOF2)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid1OF2Cnt >= Protect.Delay.GridOF2Dly)
            {
                GridError.Grid1Err.bit.OF2 = 1;

                Protect.Count.Grid1OF2Cnt = 0;
            }

            Protect.ClrCnt.Grid1OF2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1OF2Cnt = 0;
        }*/
        // --------------------------电网2过频保护----------------------------------------
        // 等级1
        if (CaptureData.FreqS > Protect.Value.GridOF1)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid2OF1Cnt >= Protect.Delay.GridOF1Dly)
            {
                GridError.Grid2Err.bit.OF1 = 1;

                Protect.Count.Grid2OF1Cnt = 0;
            }

            Protect.ClrCnt.Grid2OF1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2OF1Cnt = 0;
        }

        // 等级2
        /*
        if (CaptureData.FreqS > Protect.Value.GridOF2)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid2OF2Cnt >= Protect.Delay.GridOF2Dly)
            {
                GridError.Grid2Err.bit.OF2 = 1;

                Protect.Count.Grid2OF2Cnt = 0;
            }

            Protect.ClrCnt.Grid2OF2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2OF2Cnt = 0;
        }*/

        // -------------------------------电网1欠频保护--------------------------------------
        // 等级1
        if (CaptureData.FreqR < Protect.Value.GridUF1)
        {
            // 累计时间超出设定值,设置故障码
            if (++Protect.Count.Grid1UF1Cnt >= Protect.Delay.GridUF1Dly)
            {
                GridError.Grid1Err.bit.UF1 = 1;

                Protect.Count.Grid1UF1Cnt = 0;
            }

            Protect.ClrCnt.Grid1UF1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1UF1Cnt = 0;
        }

        // 等级2
        /*
        if (CaptureData.FreqR < Protect.Value.GridUF2)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid1UF2Cnt >= Protect.Delay.GridUF2Dly)
            {
                GridError.Grid1Err.bit.UF2 = 1;

                Protect.Count.Grid1UF2Cnt = 0;
            }

            Protect.ClrCnt.Grid1UF2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid1UF2Cnt = 0;
        }*/
        // -------------------------------电网2欠频保护--------------------------------------
        // 等级1
        if (CaptureData.FreqS < Protect.Value.GridUF1)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid2UF1Cnt >= Protect.Delay.GridUF1Dly)
            {
                GridError.Grid2Err.bit.UF1 = 1;

                Protect.Count.Grid2UF1Cnt = 0;
            }

            Protect.ClrCnt.Grid2UF1RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2UF1Cnt = 0;
        }

        // 等级2
        /*
        if (CaptureData.FreqS < Protect.Value.GridUF2)
        {
            // 累计时间超出设定值设置故障码
            if (++Protect.Count.Grid2UF2Cnt >= Protect.Delay.GridUF2Dly)
            {
                GridError.Grid2Err.bit.UF2 = 1;

                Protect.Count.Grid2UF2Cnt = 0;
            }

            Protect.ClrCnt.Grid2UF2RstCnt = 0;
        }
        else
        {
            Protect.Count.Grid2UF2Cnt = 0;
        }*/
    }
    else
    {
        Protect.Count.Grid1OF1Cnt = 0;
        Protect.Count.Grid1OF2Cnt = 0;
        Protect.Count.Grid1UF1Cnt = 0;
        Protect.Count.Grid1UF2Cnt = 0;

        Protect.Count.Grid2OF1Cnt = 0;
        Protect.Count.Grid2OF2Cnt = 0;
        Protect.Count.Grid2UF1Cnt = 0;
        Protect.Count.Grid2UF2Cnt = 0;
    }

    // 电网1过频复位
    if (GridError.Grid1Err.bit.OF1 && CaptureData.FreqR < Protect.Value.RecoveryGridOF1)
    {
        if (++Protect.ClrCnt.Grid1OF1RstCnt > Protect.ClrDly.GridOFRstDly)
        {
            Protect.ClrCnt.Grid1OF1RstCnt = 0;

            GridError.Grid1Err.bit.OF1 = 0;
        }

        // 过频时清除欠频故障位
        GridError.Grid1Err.bit.UF1 = 0;
        GridError.Grid1Err.bit.UF2 = 0;
        GridError.Grid2Err.bit.UF1 = 0;
        GridError.Grid2Err.bit.UF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid1OF1RstCnt = 0;
    }
    /*
    if (GridError.Grid1Err.bit.OF2)
    {
        if (++Protect.ClrCnt.Grid1OF2RstCnt > Protect.ClrDly.GridOFRstDly)
        {
            Protect.ClrCnt.Grid1OF2RstCnt = 0;

            GridError.Grid1Err.bit.OF2 = 0;
        }

        // 过频时清除欠频故障位
        GridError.Grid1Err.bit.UF1 = 0;
        GridError.Grid1Err.bit.UF2 = 0;
        GridError.Grid2Err.bit.UF1 = 0;
        GridError.Grid2Err.bit.UF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid1OF2RstCnt = 0;
    }*/
    // 电网2过频复位
    if (GridError.Grid2Err.bit.OF1 && CaptureData.FreqS < Protect.Value.RecoveryGridOF1)
    {
        if (++Protect.ClrCnt.Grid2OF1RstCnt > Protect.ClrDly.GridOFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2OF1RstCnt = 0;

            GridError.Grid2Err.bit.OF1 = 0;
        }

        // 过频时清除欠频故障位
        GridError.Grid2Err.bit.UF1 = 0;
        GridError.Grid2Err.bit.UF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid2OF1RstCnt = 0;
    }
   /*
    if (GridError.Grid2Err.bit.OF2)
    {
        if (++Protect.ClrCnt.Grid2OF2RstCnt > Protect.ClrDly.GridOFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2OF2RstCnt = 0;

            GridError.Grid2Err.bit.OF2 = 0;
        }

        // 过频时清除欠频故障位
        GridError.Grid2Err.bit.UF1 = 0;
        GridError.Grid2Err.bit.UF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid2OF2RstCnt = 0;
    }*/
    // 电网1欠频复位
    if (GridError.Grid1Err.bit.UF1 && CaptureData.FreqR > Protect.Value.RecoveryGridUF1)
    {
        if (++Protect.ClrCnt.Grid1UF1RstCnt > Protect.ClrDly.GridUFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid1UF1RstCnt = 0;

            GridError.Grid1Err.bit.UF1 = 0;
        }

        // 欠频时清除过频故障位
        GridError.Grid1Err.bit.OF1 = 0;
        GridError.Grid1Err.bit.OF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid1UF1RstCnt = 0;
    }
	/*
    if (GridError.Grid1Err.bit.UF2)
    {
        if (++Protect.ClrCnt.Grid1UF2RstCnt > Protect.ClrDly.GridUFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid1UF2RstCnt = 0;

            GridError.Grid1Err.bit.UF2 = 0;
        }

        // 欠频时清除过频故障位
        GridError.Grid1Err.bit.OF1 = 0;
        GridError.Grid1Err.bit.OF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid1UF2RstCnt = 0;
    }*/
    // 电网2欠频复位
    if (GridError.Grid2Err.bit.UF1 && CaptureData.FreqS > Protect.Value.RecoveryGridUF1)
    {
        if (++Protect.ClrCnt.Grid2UF1RstCnt > Protect.ClrDly.GridUFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2UF1RstCnt = 0;

            GridError.Grid2Err.bit.UF1 = 0;
        }

        // 欠频时清除过频故障位
        GridError.Grid2Err.bit.OF1 = 0;
        GridError.Grid2Err.bit.OF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid2UF1RstCnt = 0;
    }
	/*
    if (GridError.Grid2Err.bit.UF2)
    {
        if (++Protect.ClrCnt.Grid2UF2RstCnt > Protect.ClrDly.GridUFRstDly)  // 10s
        {
            Protect.ClrCnt.Grid2UF2RstCnt = 0;

            GridError.Grid2Err.bit.UF2 = 0;
        }

        // 欠频时清除过频故障位
        GridError.Grid2Err.bit.OF1 = 0;
        GridError.Grid2Err.bit.OF2 = 0;
    }
    else
    {
        Protect.ClrCnt.Grid2UF2RstCnt = 0;
    }*/
}
/*************************************************
Function: CheckFanState
Description: 风机状态读取,风机故障 ,三组风机有一组未安装将触发故障
Input: None
Output: None
Frequency: 500ms
*************************************************/
void CheckFanState(void)
{
    uint16_t ErrorDly = 10;
    uint16_t RstDly   = 20;
//    if(SystemPara.FanSpeed == 0)
//    {
//        FAN2_OFF;
//    }
//    else if(SystemPara.FanSpeed == 1)
//    {
//        //20%
//    }
//    else if(SystemPara.FanSpeed == 2)
//    {
//        //50%
//    }
//    else
//    {
//        FAN2_ON;
//    }

    if (FAN2_FAULT_DATA == 0)
    {
        Protect.ClrCnt.FanErrRstCnt = 0;

        if (++Protect.Count.FanErrCnt >= ErrorDly)
        {
            SystemError.Fault2.bit.FanFault = 1;
        }
    }
    else
    {
        Protect.Count.FanErrCnt = 0;

        if (++Protect.ClrCnt.FanErrRstCnt > RstDly)
        {
            Protect.ClrCnt.FanErrRstCnt = 0;
            SystemError.Fault2.bit.FanFault = 0;
        }
    }
}

/*************************************************
Function: ForcePowerOff
Description: 强制停机,封锁PV、INV、LLC驱动
Input: None
Output: None
Frequency:
*************************************************/
void ForcePowerOff(void)
{
    // 封锁 INV PWM
    DISABLE_INV1PWM;
    DISABLE_INV2PWM;
    DISABLE_INV3PWM;
    DISABLE_INV4PWM;

    InvStateData.WorkState = ErrorState;     // 进入待机

    SystemPara.SystemPowerOn = 0;            // 关机

    // 封锁 PV PWM
    DISABLE_PV1PWM;
    DISABLE_PV2PWM;
    DISABLE_PV3PWM;
    DISABLE_PV4PWM;

    PV1WorkState.State = SELF_STATE;
    PV2WorkState.State = SELF_STATE;
    PV3WorkState.State = SELF_STATE;
    PV4WorkState.State = SELF_STATE;
}
/*************************************************
Function: CheckBATLimitChargeState
Description: 并网状态下电池电压小于82V时并网功率限值在1000W内,
电池电压大于87V
Input: None
Output: None
Frequency: 1ms
*************************************************/
void CheckBATLimitChargeState(void)
{
    static uint16_t rstCountBATLimit = 0;

    if (InvStateData.WorkState == OnGridState && ArmWriteData.AgingLable == 0)
    {
        if (SampleRms.BUSVoltFilter8 <= 77.0f)     // 或者77V两个点
        {
            InvError.BATLowLimit = 1;              // 电池低压限功率
            rstCountBATLimit = 0;
        }
        // 连续1s电池电压大于83V清除限功率标志
        else if (SampleRms.BUSVoltFilter8 >= 83.0f)
        {
            if (++rstCountBATLimit >= (1000))
            {
                InvError.BATLowLimit = 0;
            }
        }
    }
    else
    {
        rstCountBATLimit = 0;
        InvError.BATLowLimit = 0;
    }
}

/*************************************************
Function: CheckGridChargeState
Description: 检查当前状态下是否允许并网充电
如果 Vgrid*1.414/2.45 - Vbat >= 3.0  则不允许电网充电
Input: None
Output: None
Frequency: 10ms
*************************************************/
void CheckGridChargeState(void)
{
    static uint16_t rstCount = 0;
    float gridMax = 0,result = 0;

    if (InvStateData.WorkState == OnGridState)
    {
        gridMax = __fmax(SampleRms.Grid1VoltRms, SampleRms.Grid2VoltRms);

        result = (1.414f/2.6f)*gridMax;
        result -= SampleRms.BUSVoltFilter8;

        if (result >= 3.0f)
        {
            rstCount = 0;
            InvError.NoCharge = 1;     // 不允许并网充电
        }
        else
        {
            // 30s后清除标志位
            if (++rstCount >= (100*30))
            {
                rstCount = 0;
                InvError.NoCharge = 0;
            }
        }
    }
    else
    {
        rstCount = 0;
        InvError.NoCharge = 0;
    }
}

/*************************************************
Function: MainProtect
Description: 该函数在主程序中被调用
Input: None
Output: None
Frequency: always
*************************************************/
void MainProtect(void)
{
    // BUS1 过压

#if 0
    else if (SampleRms.Bus1VoltFilter8 > (Protect.Value.BusVoltOverIns + 25))
    {
        SystemError.Fault1.bit.Bus1High = 1;

        ForcePowerOff();
    }
    // BUS2 过压
    else if (SampleRms.Bus2VoltFilter8 > (Protect.Value.BusVoltOverIns + 25))
    {
        SystemError.Fault1.bit.Bus2High = 1;

        ForcePowerOff();
    }
#endif
}

/*************************************************
Function: CheckAuxPowerState
Description: 辅助电源掉电保护检测
Input: None
Output: None
Frequency: 10K
*************************************************/
inline void CheckAuxPowerState(void)
{
    if (POWER_LOW_DATA == 0)
    {
            SystemError.Fault1.bit.AuxPowerFault = 1;
    }
}
/*************************************************
Function: CheckAntiIslandingProtect
Description: 检查孤岛负载是否存在,当孤岛扰动状态下,如果扰频导致电网频率发生同步变化
// 则认为存在孤岛负载;设计孤岛扰动步长0.05Hz,总共扰10次,如果连续出现6次扰频和电压频率同步
Input: None
Output: None
Frequency: 20K
*************************************************/
#pragma CODE_SECTION(CheckAntiIslandingProtect, ".TI.ramfunc");
void CheckAntiIslandingProtect(void)
{
    if (IslandDataR.IslandEnable || (IslandDataR.SumDeltaF >= 0.5f))
    {
        // 每个周期判断一次
        if (InvPhase.ThetaPointR == 50)
        {
            // 频率滤波
            if (IslandDataR.CntFreq >= 3)
            {
                IslandDataR.AvgFreq = IslandDataR.SumFreq / 3.0f;
                IslandDataR.SumFreq = 0;
                IslandDataR.CntFreq = 0;
                IslandDataR.FilterIndex++;

                if (IslandDataR.FilterIndex == 1)
                {
                    IslandDataR.FreqFilter[0] = IslandDataR.AvgFreq;
                }
                if (IslandDataR.FilterIndex == 5)
                {
                    IslandDataR.FreqFilter[1] = IslandDataR.AvgFreq;
                }

                // 正向扰频:频率变大
                if (IslandDataR.Dir == 0)
                {
                    // 或者差值大于0.02Hz
                    if ((IslandDataR.AvgFreq - IslandDataR.LastAvgFreq) > 0.006f)
                    {
                        if (++IslandDataR.ErrCnt >= 5)     // 累计6次触发孤岛保护
                        {
                            IslandDataR.ErrCnt = 0;

                            if ((IslandDataR.FreqFilter[1] - IslandDataR.FreqFilter[0]) >= 0.2f)
                            {
                                InvStateData.GridErrTZCnt = 0;
                                GridError.Grid1Err.bit.Island = 1;
                            }
                        }
                    }
                    else
                    {
                        IslandDataR.ErrCnt = 0;
                    }
                }
                // 反向扰频:周期变大频率减小
                else if (IslandDataR.Dir == 1)
                {
                    if ((IslandDataR.LastAvgFreq - IslandDataR.AvgFreq) > 0.006f)
                    {
                        if (++IslandDataR.ErrCnt >= 5)     // 累计6次触发孤岛保护
                        {
                            IslandDataR.ErrCnt = 0;

                            if ((IslandDataR.FreqFilter[0] - IslandDataR.FreqFilter[1]) >= 0.2f)
                            {
                                InvStateData.GridErrTZCnt = 0;
                                GridError.Grid1Err.bit.Island = 1;
                            }
                        }
                    }
                    else
                    {
                        IslandDataR.ErrCnt = 0;
                    }
                }
                IslandDataR.LastAvgFreq = IslandDataR.AvgFreq;
            }

            IslandDataR.CntFreq++;
            IslandDataR.SumFreq += CaptureData.FreqR;
        }
    }
    else
    {
        IslandDataR.ErrCnt = 0;
        IslandDataR.FilterIndex = 0;
        IslandDataR.CntFreq = 0;
        IslandDataR.SumFreq = 0;
        IslandDataR.AvgFreq = CaptureData.FreqR;
        IslandDataR.LastAvgFreq = IslandDataR.AvgFreq;
    }

    // S相
    if (IslandDataS.IslandEnable || (IslandDataS.SumDeltaF >= 0.5f))
    {
        // 每个周期判断一次
        if (InvPhase.ThetaPointS == 50)
        {
            if (IslandDataS.CntFreq >= 3)
            {
                IslandDataS.AvgFreq = IslandDataS.SumFreq / 3.0f;
                IslandDataS.SumFreq = 0;
                IslandDataS.CntFreq = 0;
                IslandDataS.FilterIndex++;

                if (IslandDataS.FilterIndex == 1)
                {
                    IslandDataS.FreqFilter[0] = IslandDataS.AvgFreq;
                }
                if (IslandDataS.FilterIndex == 5)
                {
                    IslandDataS.FreqFilter[1] = IslandDataS.AvgFreq;
                }

                // 正向扰频:频率变大
                if (IslandDataS.Dir == 0)
                {
                    // 或者差值大于0.02Hz
                    if ((IslandDataS.AvgFreq - IslandDataS.LastAvgFreq) > 0.006f)
                    {
                        if (++IslandDataS.ErrCnt >= 5)     // 累计6次触发孤岛保护
                        {
                            IslandDataS.ErrCnt = 0;
                            if ((IslandDataS.FreqFilter[1] - IslandDataS.FreqFilter[0]) >= 0.2f)
                            {
                                InvStateData.GridErrTZCnt = 0;
                                GridError.Grid2Err.bit.Island = 1;
                            }
                        }
                    }
                    else
                    {
                        IslandDataS.ErrCnt = 0;
                    }
                }
                // 反向扰频:周期变大频率减小
                else if (IslandDataS.Dir == 1)
                {
                    if ((IslandDataS.LastAvgFreq - IslandDataS.AvgFreq) > 0.006f)
                    {
                        if (++IslandDataS.ErrCnt >= 5)     // 累计6次触发孤岛保护
                        {
                            IslandDataS.ErrCnt = 0;
                            if ((IslandDataS.FreqFilter[0] - IslandDataS.FreqFilter[1]) >= 0.2f)
                            {
                                InvStateData.GridErrTZCnt = 0;
                                GridError.Grid2Err.bit.Island = 1;
                            }
                        }
                    }
                    else
                    {
                        IslandDataS.ErrCnt = 0;
                    }
                }
                IslandDataS.LastAvgFreq = IslandDataS.AvgFreq;
            }

            IslandDataS.CntFreq++;
            IslandDataS.SumFreq += CaptureData.FreqS;
        }
    }
    else
    {
        IslandDataS.ErrCnt = 0;
        IslandDataS.FilterIndex = 0;
        IslandDataS.CntFreq = 0;
        IslandDataS.SumFreq = 0;
        IslandDataS.AvgFreq = CaptureData.FreqS;
        IslandDataS.LastAvgFreq = IslandDataS.AvgFreq;
    }
}
/*************************************************
Function: xint2_isr
Description: 逆变过流封波信号处理
Input: None
Output: None
Frequency:
*************************************************/
__interrupt void xint2_isr(void)
{
//    ForcePowerOff();
//    SystemError.Fault1.bit.L1OverCurrH = 1;
//    SystemError.Fault1.bit.L2OverCurrH = 1;
    FuncPara.OCL3 = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
/*************************************************
Function: epwm5_tzint_isr
Description: LLC1 过流 TZ 事件中断入口,复位将在LLC状态机中执行
Input: None
Output: None
Frequency:
*************************************************/
__interrupt void epwm5_tzint_isr(void)
{
    SystemError.Fault1.bit.LLC1OverCurrH = 1;

    // Acknowledge this interrupt to receive more interrupts from group 2
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;
}
/*************************************************
Function: epwm7_tzint_isr
Description: LLC2 过流 TZ 事件中断入口,复位将在LLC状态机中执行
Input: None
Output: None
Frequency:
*************************************************/
__interrupt void epwm7_tzint_isr(void)
{
    SystemError.Fault1.bit.LLC2OverCurrH = 1;

    // Acknowledge this interrupt to receive more interrupts from group 2
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP2;
}
//=================================================================================
// 函数名称: GFCICheckTest
// 函数功能:  GFCI自检测
// 输入参数: 无
// 输出参数: 无
// 其它说明: 主动注入50ma漏电流 20220918
//=================================================================================
void GFCICheckTest(void)
{
    if(GFCIData.GfciTestOver == 0)
    {
        if(InvPhase.ThetaPointS == 10)	//一个电网周期
        {  
            GFCIData.DelayCnt--;
            if((GFCIData.DelayCnt >= 450)&&(GFCIData.DelayCnt <= 500))
            {
                GFCI_50mA_OFF ;
                if(GFCIData.GFCIRms > 20)//200
                {
                    if(++GFCIData.HardErrorCnt > 5)
                    {
                        GFCIData.HardErrorCnt = 0 ;
                        SystemError.Fault2.bit.GFCIHardError=1;
                    }
                }
            }
            else if((GFCIData.DelayCnt >= 390)&&(GFCIData.DelayCnt <= 400))
            {
                GFCI_50mA_ON ;
            }
            else if((GFCIData.DelayCnt >= 350)&&(GFCIData.DelayCnt < 390))
            {
                if((GFCIData.GFCIRms > 80)||(GFCIData.GFCIRms < 15))
                {
                    if(++GFCIData.HardErrorCnt2 > 5)
                    {
                        GFCIData.HardErrorCnt2 = 0 ;
                        SystemError.Fault2.bit.GFCIHardError=1;
                    }
                }
            }
            else if((GFCIData.DelayCnt >= 300)&&(GFCIData.DelayCnt < 350))
            {
                GFCI_50mA_OFF ;
            }
            else if(GFCIData.DelayCnt <= 200)
            {
                GFCIData.GfciTestOver = 1 ;
            }
        }
    }
}
//=================================================================================
// 函数名称:Check_Gfci_Vde0126_4105
// 函数功能: 漏电流检测
// 输入参数: 无
// 输出参数: 无
// 其它说明: 1、连续漏电流大于300mA     0.3s内断网；    2、电流突变30mA  0.3s内断网；
//         3、电流突变60mA       0.15s内断网；         4、电流突变150mA   0.04s内断网
//=================================================================================
uint16_t levelGfci = 1;
uint16_t GfciCheckState=0;
uint16_t GFCI_Temp[8]={0,0,0,0,0,0,0,0};
void Check_Gfci_Vde0126_4105(void)
{
    static uint16_t delay=0;
    static uint16_t DelayFaultJump110mA=0;
    static uint16_t DelayFaultContinue=0;
    static uint16_t DelayFaultJump150mA=0;
    static uint16_t DelayFaultJump60mA=0;
    static uint16_t DelayFaultJump30mA=0;
    static int32_t DelayFaultClear=0;
    static int16_t GfciBaseValue=0;
    int16_t  DeltaGfci=0;

    GFCIData.GFCIAD = AdcdResultRegs.ADCRESULT1;
	GFCIData.GetRFealValue = (GFCIData.GFCIAD -2132)*0.255 ;//0.238正样校准后更改为0.255  SciData.GFCIRmsZero
    GFCIData.GFCISUM += GFCIData.GetRFealValue * GFCIData.GetRFealValue;
    GFCIData.CalCnt++;

    if(fabsf(GFCIData.GetRFealValue)- GFCI_Temp[7] > 110)  //10ms快速
    {
        if(++DelayFaultJump110mA > 100)
        {
            GFCIData.GFCIError = 10;
            SystemError.Fault2.bit.GFCI=1;
            GfciCheckState = 5;
        }
    }
    else
    {
        DelayFaultJump110mA = 0;
    }

    if(InvPhase.ThetaPointR == 1 || InvPhase.ThetaPointR == (SampleIns.PeriodPoint>>1)) //一个电网周期InvPhase.ThetaPointS == 1 ++Cnt_10ms>100
    {   
        GFCIData.GFCIRms = sqrtf(GFCIData.GFCISUM / GFCIData.CalCnt) ; //仅电网频率 = 50HZ
        GFCIData.CalCnt = 0 ;
        GFCIData.GFCISUM = 0 ;
//        GFCIData.GFCIDisplay = ;
    }
    else
    {
        return;
    }
	if(GFCIData.GfciTestOver==0)//
    {
        return;
    }

//	if(++Cnt_10ms>100)
//	{
//	    Cnt_10ms = 0;
//	}
//	else
//	{
//	    return;
//	}

    //瞬态
    GFCI_Temp[7] = GFCI_Temp[6];
    GFCI_Temp[6] = GFCI_Temp[5];
    GFCI_Temp[5] = GFCI_Temp[4];
    GFCI_Temp[4] = GFCI_Temp[3];
    GFCI_Temp[3] = GFCI_Temp[2];
    GFCI_Temp[2] = GFCI_Temp[1];
    GFCI_Temp[1] = GFCI_Temp[0];
    GFCI_Temp[0] = GFCIData.GFCIRms;

    switch(GfciCheckState)
    {
    case 0:
        if(InvStateData.WorkState != 0)//系统运行
        {
            if(++delay >= 150)
            {
                delay=0;
                GfciCheckState=1;
            }
        }
        break;
    case 1:
        //检测跳变
        GFCIData.GFCItemp1=DeltaGfci = GFCI_Temp[0]-GFCI_Temp[7];//GFCIData.InitValue
        GFCIData.InitValue=GFCI_Temp[7];
        GFCIData.GFCIDelta = GFCI_Temp[0];
        GfciBaseValue=GFCI_Temp[7];

        if(DeltaGfci>=110)//&&(GfciBaseValue >= 30)//150
        {
            GFCIData.GFCIError = 1 ;
			SystemError.Fault2.bit.GFCI=1;
            GfciCheckState = 5;
        }
        else if(DeltaGfci >= 45)//&&(GfciBaseValue >= 160)  //60
        {
            GfciCheckState = 3; //60mA跳变延时确认
        }
        else if(DeltaGfci >= 20)//&&(GfciBaseValue >= 200)//24
        {
            GfciCheckState=4; //30mA跳变延时确认
        }
        else
        {
            if(GFCI_Temp[0] >= 250)
            {
                if(++DelayFaultContinue >= 22)//11
                {
                    DelayFaultContinue = 0;
                    SystemError.Fault2.bit.GFCI=1;
                    GfciCheckState = 5; //故障，等待恢复
                    GFCIData.GFCIError = 2;
                }
            }
            else
            {
                if(DelayFaultContinue>0)
                    DelayFaultContinue--;
            }
        }
        /*
		 if(GFCI_Temp[0] >= 250)
            {
                if(++DelayFaultContinue >= 11)
                {
                    DelayFaultContinue = 0;
                    SystemError.Fault2.bit.GFCI=1;
                    GfciCheckState = 5; //故障，等待恢复
                    GFCIData.GFCIError = 2;
                }
            }
            else
            {
                if(DelayFaultContinue>0)
                    DelayFaultContinue--;
            }*/
        break;
    case 2:
        DeltaGfci = GFCI_Temp[0]-GfciBaseValue;
        if(DeltaGfci >= 110)
        {
            if(++DelayFaultJump150mA >= 1)//10mA
            {
                DelayFaultJump150mA = 0;
                SystemError.Fault2.bit.GFCI=1;
                GfciCheckState=5;
				GFCIData.GFCIError = 3 ;
            }
        }
        else
        {
            if(DelayFaultJump150mA>0)
                DelayFaultJump150mA--;
            else
                GfciCheckState=1;
        }
        break;
    case 3:
        GFCIData.GFCItemp1=DeltaGfci = GFCI_Temp[0] - GfciBaseValue;
        if(DeltaGfci >= 45)//60
        {
            if(++DelayFaultJump60mA >= ((0.09 * 10e3 * 2)/SampleIns.PeriodPoint - 1))//(90ms * 2)/Prd_grid - 1    //8
            {
                DelayFaultJump60mA=0;
                SystemError.Fault2.bit.GFCI=1;
                GfciCheckState=5;
				GFCIData.GFCIError = 4 ;
            }
        }
        else
        {
            if(DelayFaultJump60mA>0)
                DelayFaultJump60mA--;
            else
                GfciCheckState=1;
        }
		if(DeltaGfci>=110)//&&(GfciBaseValue >= 30)
        {
            GFCIData.GFCIError = 7 ;
			SystemError.Fault2.bit.GFCI=1;
            GfciCheckState = 5;
        }
        break;
    case 4:
        GFCIData.GFCItemp1=DeltaGfci = GFCI_Temp[0]-GfciBaseValue;
        if(DeltaGfci >= 20)//24
        {
            if(++DelayFaultJump30mA >= ((0.19 * 10e3 * 2)/SampleIns.PeriodPoint - 1))//190ms
            {
                DelayFaultJump30mA=0;
                SystemError.Fault2.bit.GFCI=1;
                GfciCheckState=5;
				GFCIData.GFCIError = 5 ;
            }
        }
        else
        {
            if(DelayFaultJump30mA>0)
                DelayFaultJump30mA--;
            else
                GfciCheckState=1;
        }
		if(DeltaGfci >= 45)//60
        {
            if(++DelayFaultJump60mA >= ((0.09 * 10e3 * 2)/SampleIns.PeriodPoint - 1))//90ms
            {
                DelayFaultJump60mA=0;
                SystemError.Fault2.bit.GFCI=1;
                GfciCheckState=5;
				GFCIData.GFCIError = 8 ;
            }
        }
        else
        {
            if(DelayFaultJump60mA>0)
                DelayFaultJump60mA--;
        }
		if(DeltaGfci>=110)//&&(GfciBaseValue >= 30)
        {
            GFCIData.GFCIError = 9 ;
			SystemError.Fault2.bit.GFCI=1;
            GfciCheckState = 5;
        }
        break;
    case 5:
        if(GFCI_Temp[0] <=17)
        {
            if(++DelayFaultClear >= 2000)//15000
            {
                DelayFaultClear=0;
//               SystemError.Fault2.bit.GFCI=0;
//               GfciCheckState=0;
				GFCIData.GFCIError = 6 ;
            }
        }
        else
        {
            if(DelayFaultClear>1)
               DelayFaultClear--;
        }
		DelayFaultJump30mA=0;
		DelayFaultJump60mA=0;
		DelayFaultJump150mA = 0;
		DelayFaultContinue = 0;
        break;
    default:
        GfciCheckState=0;
        break;
    }
}
/*************************************************
Function: Check_AcDc_Vde0126
Description: 直流分量异常保护 275mA立刻保护，100mA 2min30恢复
Input: None
Output: None
Frequency: 10ms
*************************************************/
void Check_AcDc_Vde0126(void)
{
    static uint32_t RecoverCntlimt = 0;//100*60*30
    static uint32_t RecoverCnt  = 0;
    static uint16_t RecoverTime = 0;
    static Uint32   DelayCnt = 0;
//  static Uint32 FirstCheckDelay = 0;
    if(SystemError.Fault1.bit.DCCommentErr == 0&&InvStateData.WorkState==OnGridState&&ArmWriteData.AgingLable == 0)
       {
           if(IrCtrl.RmpCtrl.finalCurr>(IrCtrl.RmpCtrl.TargetValue-0.05)&&IrCtrl.RmpCtrl.finalCurr<(IrCtrl.RmpCtrl.TargetValue+0.05)
              &&IsCtrl.RmpCtrl.finalCurr>(IsCtrl.RmpCtrl.TargetValue-0.05)&&IsCtrl.RmpCtrl.finalCurr<(IsCtrl.RmpCtrl.TargetValue+0.05))
           {
                   if(abs(SampleRms.DCCurr1Rms)>230||abs(SampleRms.DCCurr2Rms)>230)//270
                   {
                       if(++DelayCnt>40)//400ms
                       {
                           SystemError.Fault1.bit.DCCommentErr = 1 ;
                           DelayCnt=0;
                       }

                   }
                   else
                   {
                       DelayCnt=0;
                   }
           }

       }
       else
       {
           DelayCnt=0;
       }
       if( GridError.Grid1Err.bit.Grid1Drop1 ==1|| GridError.Grid1Err.bit.Grid1Drop1==1)//电网掉电1ms 1次以上暂时关闭直流分量故障
       {
           SystemError.Fault1.bit.DCCommentErr = 0 ;
           DelayCnt=0;
       }
    //3次不恢复故障，但是有条件恢复
    //if(RecoverTime < 3)//故障未超3次
    {
        if(RecoverCntlimt <= 180000L)
        {
            if((SystemError.Fault1.bit.DCCommentErr)||(RecoverTime > 0))//出现过故障开始计时
            {
                RecoverCntlimt ++;
                if(abs(SampleRms.DCCurr1Rms)<100 && abs(SampleRms.DCCurr2Rms)<100 && SystemError.Fault1.bit.DCCommentErr)//有故障才进行恢复次数计数
                {
                    if(RecoverCnt++ >= 15000)//2min30s
                    {
                        RecoverCnt = 0;
                        SystemError.Fault1.bit.DCCommentErr = 0;
                        RecoverTime++;//恢复次数+1
                    }
                }
                else
                {
                    RecoverCnt = 0;
                }
            }
        }
        else//时间超过30min且故障未超3次
        {
            RecoverCntlimt = 0;
            RecoverTime = 0;
        }
    }//30min超过3次不再进入此段

}

#pragma CODE_SECTION(OCPSoft, ".TI.ramfunc");
void OCPSoft(void)
{
    if (InvStateData.WorkState == OnGridState)
    {
        if(SampleIns.Inv1CurrIns>65||SampleIns.Inv2CurrIns>65)//
        {
            if (++Protect.Count.GridOCPCnt >= 3)//600us
            {
                Protect.Count.GridOCPCnt = 0;
                GridError.Grid1Err.bit.GridOCP = 1;
                Protect.ClrCnt.GridOCPCnt = 0;     // 清除电网掉电复位计数
                Protect.Count.Grid2Loss8Cnt++;
            }

        }
        else
        {
            Protect.Count.GridOCPCnt = 0;     // 清除电网掉电复位计数
        }
        if(GridError.Grid1Err.bit.GridOCP==1)//2s
        {
            if(++Protect.ClrCnt.GridOCPCnt>20000)//2s
            {
                Protect.Count.GridOCPCnt = 0;
                GridError.Grid1Err.bit.GridOCP = 0;
                Protect.ClrCnt.GridOCPCnt = 0;     // 清除电网掉电复位计数
            }
        }

    }
}


#pragma CODE_SECTION(ECO, ".TI.ramfunc");
void ECO(void)
{
    static Uint32 Inv1ProtectCnt = 0;
    static Uint16 StartCnt=0;
    float dltVolt = 45;
    float StandbyI = 0;
    StandbyI = (SystemPara.FrequencySet == 1) ? 1.2 :1;

    if (InvStateData.WorkState == OffGridState && SystemPara.OpenLoopEn == 0
            && Flag_Local.Bits.enParallel == 0)
    {
        if(SampleRms.Inv1VoltRms<90||SampleRms.Inv2VoltRms<90
            ||SampleRms.Inv1CurrRms>2.0||SampleRms.Inv2CurrRms>2.0
            ||fabsf(SampleIns.Load1CurrIns)>1.1 ||fabsf(SampleIns.Load2CurrIns)>1.1)
        {
            PWMONflag=0;
            Inv1ProtectCnt=0;
        }
        else if((SampleRms.Inv1CurrRms< StandbyI &&SampleRms.Inv2CurrRms< StandbyI)  //空载电流最大1A,60HZ最大1.2A
                &&(SampleRms.Load1CurrRms<0.3&&SampleRms.Load2CurrRms<0.3)
                &&(fabsf(SampleIns.Load1CurrIns)<1.3&&fabsf(SampleIns.Load2CurrIns)<1.3)
                &&(SampleRms.Inv1VoltRms>97&&SampleRms.Inv2VoltRms>97))
        {
            if((++Inv1ProtectCnt>30000)&&(InvPhase.ThetaPointR == ((SampleIns.PeriodPoint>>2)-10)))//30s
            {
                PWMONflag=1;    //TODO @sjj
                Inv1ProtectCnt=0;
                StartCnt=0;
            }
        }

        if(++StartCnt>400)//第一次进入低功耗模式，延时600ms进行电压异常判断
        {
            StartCnt=400;
            dltVolt = 45;
        }
        else
        {
            dltVolt = 65;
        }
        if(fabs(SampleIns.Inv1VoltIns - 143* InvPhase.SinR) >=dltVolt)
        {
//            if(++DelayCnt1>7)//
//            {
                PWMONflag=0;
                Inv1ProtectCnt = 0;
//                DelayCnt1=0;
//                TEST_ON
//            }
        }
        else
        {
//            DelayCnt1=0;
//            TEST_OFF
        }
        if (fabs(SampleIns.Inv2VoltIns - 143* InvPhase.SinS) >=dltVolt)
        {
            PWMONflag=0;
            Inv1ProtectCnt = 0;
        }
        else
        {
//            DelayCnt2=0;
//            TEST_OFF
        }

    }
    else
    {
        PWMONflag=0;
        Inv1ProtectCnt=0;
    }

}
/*************************************************
Function: HandleProtect
Description: 快速保护主函数入口
Input: None
Output: None
Frequency: 10K
*************************************************/
#pragma CODE_SECTION(HandleProtect, ".TI.ramfunc");
void HandleProtect(void)
{
    CURR_REF_OFF;

    //平衡电路过流
    BusOCP();

    //硬件逆变过流
    InvOcpHandle();

//    PVHVCloseDrive();

    // 辅源欠压状态检查--如果辅源异常可能会误报其他异常要做处理
    CheckAuxPowerState();

    // 零飘结束
    if (SampleIns.CalSampleZeroFlag == 1)
    {
        CheckBusVolt();
        CheckPVVolt();
        CheckPVParallelErr();
        CheckPVCurr();

        if(Protect.Enable.bit.GFCI)
        {
            GFCICheckTest();
            Check_Gfci_Vde0126_4105();  //GFCI检测
        }
    }
    else
    {
        GFCI_50mA_OFF ;//不影响ARM采零点
    }

    // --------------------------继电器故障----------------------------------
    if(FuncPara.RlyError!=0)
    {
        SystemError.Fault1.bit.RelayFault = 1;
    }
    // 电网故障
    // 过欠压
    SystemAlarm.bit.GridVoltH = ((GridError.Grid1Err.all&0x03) > 0 || (GridError.Grid2Err.all&0x03) > 0) ? 1 : 0;
    SystemAlarm.bit.GridVoltL = (GridError.Grid1Err.bit.UV1 || GridError.Grid2Err.bit.UV1 );
    // 过欠频
    SystemAlarm.bit.GridFreqH = ((GridError.Grid1Err.all&0x30) > 0 || (GridError.Grid2Err.all&0x30) > 0) ? 1 : 0;
    SystemAlarm.bit.GridFreqL = ((GridError.Grid1Err.all&0xC0) > 0 || (GridError.Grid2Err.all&0xC0) > 0) ? 1 : 0;

    // PV 故障位,只影响 PV 的开关机
    //
    if ((SystemError.Fault1.all & 0xD995)
            || (SystemError.Fault2.all & 0x3049)
            || (SystemError.Fault3.all & 0x0C01)
            || (SystemError.Fault4.all & 0x0001)
            || SystemAlarm.bit.PV1Low)
    {
        SystemError.StateErr.bit.PV1Fault = 1;
    }
    else
    {
        SystemError.StateErr.bit.PV1Fault = 0;
    }
    if ((SystemError.Fault1.all & 0xD995)
            || (SystemError.Fault2.all & 0x3052)
            || (SystemError.Fault3.all & 0x0C01)
            || (SystemError.Fault4.all & 0x0001)
            || SystemAlarm.bit.PV2Low)
    {
        SystemError.StateErr.bit.PV2Fault = 1;
    }
    else
    {
        SystemError.StateErr.bit.PV2Fault = 0;
    }
    if ((SystemError.Fault1.all & 0xD995)
            || (SystemError.Fault2.all & 0x3064)
            || (SystemError.Fault3.all & 0x0C01)
            || (SystemError.Fault4.all & 0x0001)
            || SystemAlarm.bit.PV3Low)
    {
        SystemError.StateErr.bit.PV3Fault = 1;
    }
    else
    {
        SystemError.StateErr.bit.PV3Fault = 0;
    }
    if ((SystemError.Fault1.all & 0xD995)
            || (SystemError.Fault2.all & 0x3040)
            || (SystemError.Fault3.all & 0x0C01)
            || (SystemError.Fault4.all & 0x0061)
            || SystemAlarm.bit.PV4Low)
    {
        SystemError.StateErr.bit.PV4Fault = 1;
    }
    else
    {
        SystemError.StateErr.bit.PV4Fault = 0;
    }
#if 0
    // LLC 故障位,只影响 LLC 的开关机;BAT,LLC过流,BUS高低压
    if ((SystemError.Fault1.all & 0x5673)
            || (SystemError.Fault2.all & 0x1C0)
            || (SystemError.Fault3.all & 0x0D))
    {
        SystemError.StateErr.bit.LLCFault = 1;
    }
    else
    {
        SystemError.StateErr.bit.LLCFault = 0;
    }
#endif
    // INV 故障位,只影响  INV 的开关机 逆变故障+通信故障
//    if (SystemPara.ParallelEn == 0)
//    {
//        SystemError.Fault4.bit.ParaCommu = 0;
//    }
    if ((SystemError.Fault1.all & 0xD995)
            || (SystemError.Fault2.all & 0x3040)
            || (SystemError.Fault3.all & 0x0D85)
            || (SystemError.Fault4.bit.PVOVBad)
            || (SystemError.Fault4.bit.ParaCommu == 1))
    {
        SystemError.StateErr.bit.INVFault = 1;
    }
    else
    {
        SystemError.StateErr.bit.INVFault = 0;
    }
    // 关系统故障，同时关闭电池包
    if((SystemError.Fault1.all & 0xD810)
            ||(SystemError.Fault2.all & 0x3040)
            ||(SystemError.Fault3.all & 0x1)
            ||(SystemError.Fault4.all & 0x1))
    {
        SystemError.StateErr.bit.SystemFault = 1;
    }
    else
    {
        SystemError.StateErr.bit.SystemFault = 0;
    }
    // 允许并网
    if((ESmode.ES_EN)
            &&(ArmWriteData.GridOnEn)
            &&(ArmWriteData.Model.bit.CTCheck == 0)
            &&((SystemError.Fault1.all & 0x2008) == 0)
            &&((SystemAlarm.all & 0x2f) == 0))
    {
        InvStateData.OnGridPermit = 1;
    }
    else
    {
        InvStateData.OnGridPermit = 0;
    }
    // 允许旁路
    if((SystemError.Fault1.bit.RelayFault == 0)
            &&(SystemError.Fault2.bit.GFCI ==0)
//            &&(SystemError.Fault2.bit.GFCIHardError ==0)
            &&((SystemError.Fault3.all & 0x180) == 0)
            &&(Para_CheckInvRly() == 0))    //TODO 直流分量异常、零漂异常需断开？待商榷
    {
        InvStateData.BypassPermit = 1;
    }
    else
    {
        InvStateData.BypassPermit = 0;
    }

    // 老化时该值是1,ATE时该值是2,将电池低压保护值改为82V
    if (ArmWriteData.AgingLable == 1 || ArmWriteData.ATETest == 1)
    {
        IslandDataR.AllEnable = 0;              // 孤岛功能禁止
    }
    else
    {
        if ((InvStateData.WorkState == OnGridState)
                && (SystemPara.MicroInverterMode == 0)
                && (SystemPara.DieselEngineMode == 0)
                && (IrCtrl.RmpCtrl.SetpointValue >= 1.5f || IsCtrl.RmpCtrl.SetpointValue >= 1.5f))
        {
            IslandDataR.AllEnable = 1;       // 孤岛使能设置-并网馈电电流目标大于等于0.5A使能-充电禁止-ATE及老化时禁止
        }
        else
        {
            IslandDataR.AllEnable = 0;
        }
    }
}
void AntiFlow(void)
{
        static float GridActivePowerRDir=0;
        static float GridActivePowerSDir=0;

        SystemPara.GridActivePowerR=SampleRms.Grid1VoltRms*SampleRms.Grid1CurrRms;
        if(fabs(InvPhase.SinR)>0.5)
         {
           GridActivePowerRDir=SampleIns.Grid1VoltIns*SampleIns.Grid1CurrIns;
         }
        if(GridActivePowerRDir>0)
         {
              SystemPara.ActivePowerSetR--;
         }
        else
         {
              SystemPara.ActivePowerSetR++;
         }
         SystemPara.ActivePowerSetR=__fsat(SystemPara.ActivePowerSetR,SystemPara.ActivePowerSetRTemp, 0);

        SystemPara.GridActivePowerS=SampleRms.Grid2VoltRms*SampleRms.Grid2CurrRms;
        if(fabs(InvPhase.SinS)>0.5)
        {
            GridActivePowerSDir=SampleIns.Grid2VoltIns*SampleIns.Grid2CurrIns;
        }
        if(GridActivePowerSDir>0)
        {
          SystemPara.ActivePowerSetS--;
        }
        else
        {
          SystemPara.ActivePowerSetS++;
        }
        SystemPara.ActivePowerSetS=__fsat(SystemPara.ActivePowerSetS,SystemPara.ActivePowerSetSTemp, 0);

}

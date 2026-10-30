/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: Protect.h
Author: tzz
Version: 0.0.2
Date: 2021年10月12日
Description: 定义保护相关变量的数据类型
*************************************************/
#ifndef _PROTECT_H_
#define _PROTECT_H_

#include "stdint.h"

// -----------------------保护使能---------------------------
typedef struct
{
    uint16_t BusVoltOver:1;     // 0 BUS 过压保护使能
    uint16_t BusVoltLow:1;      // 1 BUS 欠压保护使能
    uint16_t INVVoltFault:1;     // 2 逆变输出故障保护使能
    uint16_t Res:1;             // 3

    uint16_t PVVoltOver:1;      // 4 PV 输入过压保护使能
    uint16_t PVVoltLow:1;       // 5 PV 输入欠压保护使能
    uint16_t PVCurrOver:1;      // 6 PV 输入过流保护使能
    uint16_t GFCI:1;            // 7 GFCI保护使能

    uint16_t INVCurrOver:1;     // 8 逆变输出过流保护使能
    uint16_t LoadOver:1;        // 9 过载保护使能
    uint16_t GridVoltOver:1;    // 10 电网电压过压保护使能
    uint16_t GridVoltLow:1;     // 11 电网电压欠压保护使能

    uint16_t Freq:1;            // 12 电网频率保护
    uint16_t Commu:1;           // 13 通信故障 :DSP/ARM
    uint16_t INVInsVoltOver:1;  // 14 瞬时过压保护使能
    uint16_t BATVoltLow:1;      // 15 BAT 电压欠压保护使能
}PROTECT_ENABLE_T;
typedef union
{
    uint16_t all;
    PROTECT_ENABLE_T bit;
}PROTECT_ENABLE_U;
// ----------------------保护阈值设定值------------------------
typedef struct
{
    // 电网
    float GridOV1;             // 电网过压保护阈值1
    float GridOV2;             // 电网过压保护阈值2
    float GridUV1;             // 电网欠压保护阈值1
    float GridUV2;             // 电网欠压保护阈值2
    float RecoveryGridOV1;
	float RecoveryGridUV1;
    float GridOF1;               // 电网过压保护阈值1
    float GridOF2;               // 电网过压保护阈值2
    float GridUF1;               // 电网欠压保护阈值1
    float GridUF2;               // 电网欠压保护阈值2
    float RecoveryGridOF1;
	float RecoveryGridUF1;
    float GridLoss1;             // 电网掉电阈值1
    float GridLoss2;             // 电网掉电阈值2
    float GridLoss3;             // 电网掉电阈值3

    uint16_t ArmCommu;           // ARM 通信延时设定值
}PROTECT_VALUE_T;
// ----------------------保护延时设定值----------------------------
typedef struct
{
    // 电网
    uint32_t GridOV1Dly;             // 电网过压保护时间1
    uint32_t GridOV2Dly;             // 电网过压保护时间2
    uint32_t GridUV1Dly;             // 电网欠压保护时间1
    uint32_t GridUV2Dly;             // 电网欠压保护时间2

    uint32_t GridOF1Dly;             // 电网过压保护时间1
    uint32_t GridOF2Dly;             // 电网过压保护时间2
    uint32_t GridUF1Dly;             // 电网欠压保护时间1
    uint32_t GridUF2Dly;             // 电网欠压保护时间2
    uint16_t GridLoss3Dly;            // 电网掉电延时时间
    uint16_t GridLoss2Dly;            // 电网掉电延时时间
    uint16_t GridLossDly;             // 电网掉电延时时间

    uint16_t ArmCommuDly;           // ARM 通信延时设定时间

    uint16_t RS485CommuDly;           // ARM 通信延时设定时间
    uint16_t CANCommuDly;             // 风机故障计数
}PROTECT_DELAY_T;
// ----------------------保护延时计数----------------------------
typedef struct
{
    // 电网
    uint16_t GridOCPCnt;              // 电网过流
	uint16_t Grid1Loss4Cnt;           // 电网1掉电计数4
	uint16_t Grid2Loss4Cnt;           // 电网2掉电计数4
	uint16_t Grid1Loss3Cnt;           // 电网1掉电计数3
	uint16_t Grid2Loss3Cnt;           // 电网2掉电计数3
	uint16_t Grid1Loss5Cnt;           // 电网1掉电计数
	uint16_t Grid2Loss5Cnt;           // 电网2掉电计数
	uint16_t Grid1Loss6Cnt;           // 电网1掉电计数
	uint16_t Grid2Loss6Cnt;           // 电网2掉电计数
	uint16_t Grid1Loss7Cnt;           // 电网1掉电计数
	uint16_t Grid2Loss7Cnt;           // 电网2掉电计数
    uint16_t Grid1Loss8Cnt;           // 电网2掉电计
    uint16_t Grid2Loss8Cnt;           // 电网2掉电计
    uint16_t Grid1Loss10Cnt;           // 电网2掉电计
    uint16_t Grid2Loss10Cnt;           // 电网2掉电计
    uint16_t Grid1Loss11Cnt;           // 电网2掉电计
    uint16_t Grid2Loss11Cnt;           // 电网2掉电计
    uint16_t Grid1_1ms_Drop;
    uint16_t Grid2_1ms_Drop;
    uint16_t GridLossWindow;          //电网掉电检测窗口
    uint16_t Grid1LossRiskCnt;         //
    uint16_t Grid2LossRiskCnt;         //
    uint16_t Grid1Loss2Cnt;           // 电网1掉电计数
    uint16_t Grid2Loss2Cnt;           // 电网2掉电计数
    uint16_t Grid1LossCnt;            // 电网1掉电计数
    uint16_t Grid2LossCnt;            // 电网2掉电计数
    uint16_t Grid1OVInsCnt;           // 电网1瞬时过压计数
    uint16_t Grid2OVInsCnt;           // 电网2瞬时过压计数
    uint16_t Grid1OV1Cnt;             // 电网过压保护计数1
    uint16_t Grid1OV2Cnt;             // 电网过压保护计数2
    uint16_t Grid1UV1Cnt;             // 电网欠压保护计数1
    uint16_t Grid1UV2Cnt;             // 电网欠压保护计数2

    uint16_t Grid2OV1Cnt;             // 电网过压保护计数1
    uint16_t Grid2OV2Cnt;             // 电网过压保护计数2
    uint16_t Grid2UV1Cnt;             // 电网欠压保护计数1
    uint16_t Grid2UV2Cnt;             // 电网欠压保护计数2

    uint32_t Grid1OF1Cnt;               // 电网过压保护计数1
    uint32_t Grid1OF2Cnt;               // 电网过压保护计数2
    uint32_t Grid1UF1Cnt;               // 电网欠压保护计数1
    uint32_t Grid1UF2Cnt;               // 电网欠压保护计数2

    uint32_t Grid2OF1Cnt;               // 电网过压保护计数1
    uint32_t Grid2OF2Cnt;               // 电网过压保护计数2
    uint32_t Grid2UF1Cnt;               // 电网欠压保护计数1
    uint32_t Grid2UF2Cnt;               // 电网欠压保护计数2

    uint16_t ArmCommuCnt;              // ARM 通信延时设定计数
    uint16_t FanErrCnt;                // 风机故障计数

    uint16_t Rs485CommuCnt;            // ARM 通信异常
    uint16_t CANCommuCnt;              // CAN通信异常
}PROTECT_COUNT_T;
// ----------------------故障自动复位时间设定值---------------------
typedef struct
{
    // 电网
    uint32_t GridReConnectDly;         // 并网重连时间单位s
    uint32_t GridLossRstDly;           // 电网掉电警告清除延时
    uint32_t GridOVRstDly;             // 电网过压保护计数1
    uint32_t GridUVRstDly;             // 电网过压保护计数1
    uint32_t GridOFRstDly;             // 电网过压保护计数1
    uint32_t GridUFRstDly;             // 电网过压保护计数1

    uint16_t ArmCommuRstDly;           // ARM 通信延时设定计数
    uint16_t RS485CommuRstDly;         // ARM 通信延时设定计数
    uint16_t CANCommuRstDly;           // 风机故障保护延时计数
}PROTECT_AUTO_RST_T;
// ----------------------保护恢复计数----------------------------
typedef struct
{
    //电网故障
    uint16_t Grid1OV1RstCnt;             // 电网过压保护计数1
    uint16_t Grid1OV2RstCnt;             // 电网过压保护计数2
    uint16_t Grid1UV1RstCnt;             // 电网欠压保护计数1
    uint16_t Grid1UV2RstCnt;             // 电网欠压保护计数2

    uint16_t Grid2OV1RstCnt;             // 电网过压保护计数1
    uint16_t Grid2OV2RstCnt;             // 电网过压保护计数2
    uint16_t Grid2UV1RstCnt;             // 电网欠压保护计数1
    uint16_t Grid2UV2RstCnt;             // 电网欠压保护计数2

    uint16_t ArmCommuRstCnt;             // ARM 通信延时设定计数
    uint16_t FanErrRstCnt;               // 风机故障复位计数

    uint16_t RS485CommuRstCnt;           // ARM 通信延时设定计数
    uint16_t CANCommuRstCnt;             // 风机故障复位计数
    //PV故障
    uint16_t PV1VoltOverRstCnt;          //PV过压恢复
    uint16_t PV2VoltOverRstCnt;
    uint16_t PV3VoltOverRstCnt;
    uint16_t PV4VoltOverRstCnt;

    uint16_t PV1VoltLowRstCnt1;
    uint16_t PV2VoltLowRstCnt1;
    uint16_t PV3VoltLowRstCnt1;
    uint16_t PV4VoltLowRstCnt1;

    uint32_t PV1VoltLowRstCnt2;
    uint32_t PV2VoltLowRstCnt2;
    uint32_t PV3VoltLowRstCnt2;
    uint32_t PV4VoltLowRstCnt2;
    //母线故障
    uint32_t BusVoltOverRstCnt;           // BUS过压恢复计数
    uint32_t BusVoltLowRstCnt;            // BUS欠压恢复计数
    uint32_t BATVoltLowRstCnt;             // 直流输入低压
    // 电网
    uint32_t BusOVTZRstCnt;                // BUS过压锁波复位计数
    uint32_t GridLossRstCnt;               // 电网1瞬时过压恢复计数
    uint32_t Grid1OF1RstCnt;               // 电网过压保护计数1
    uint32_t Grid1OF2RstCnt;               // 电网过压保护计数2
    uint32_t Grid1UF1RstCnt;               // 电网欠压保护计数1
    uint32_t Grid1UF2RstCnt;               // 电网欠压保护计数2
    uint32_t Grid2OF1RstCnt;               // 电网过压保护计数1
    uint32_t Grid2OF2RstCnt;               // 电网过压保护计数2
    uint32_t Grid2UF1RstCnt;               // 电网欠压保护计数1
    uint32_t Grid2UF2RstCnt;               // 电网欠压保护计数2
    uint32_t Grid1LossRstCnt1;              // 电网1瞬时过压恢复计数
	uint32_t Grid2LossRstCnt1;              // 电网2瞬时过压恢复计数
	uint32_t GridOCPCnt;
}PROTECT_RST_CNT_T;

typedef struct
{
    PROTECT_VALUE_T    Value;      // 保护值
    PROTECT_DELAY_T    Delay;      // 保护延时(滤波)
    PROTECT_COUNT_T    Count;      // 保护延时计数
    PROTECT_AUTO_RST_T ClrDly;     // 复位清零设置
    PROTECT_RST_CNT_T  ClrCnt;     // 复位清零计数
    PROTECT_ENABLE_U   Enable;     // 保护使能位
}PROTECT;
extern PROTECT Protect;
// ----------------------故障信息1----------------------------
typedef struct
{
    //--------------------------------------------------
    uint16_t BusHigh:1;           // 0    BUS过压
    uint16_t Reserve1:1;          // 1    预留
    uint16_t BusLow:1;            // 2    BUS低压
    uint16_t UnCtrlBackFlow:1;    // 3    并网BUS低压(防电网不控整流倒灌)

    uint16_t BusHighH:1;         // 4    BUS 硬件过压
    uint16_t Reserve3:1;          // 5    BUS 硬件过压
    uint16_t BATHighH:1;          // 6    电池硬件高压
    uint16_t L1OverCurrH:1;       // 7    逆变1输出硬件过流

    uint16_t L2OverCurrH:1;       // 8    逆变2输出硬件过流
    uint16_t LLC1OverCurrH:1;     // 9    LLC1 硬件过流
    uint16_t LLC2OverCurrH:1;     // 10   LLC2 硬件过流
    uint16_t BalanceOC:1;         // 11   平衡电路过流输入

    uint16_t AuxPowerFault:1;     // 12   辅助电源异常
    uint16_t DCCommentErr:1;      // 13   直流分量异常
    uint16_t RelayFault:1;        // 14   继电器故障
    uint16_t Parallel:1;          // 15   PV 并联错误
    //--------------------------------------------------
}FAULT1_T;
typedef union
{
    uint16_t all;
    FAULT1_T bit;
}FAULT1_U;
// ------------------------------故障信息2----------------------------
typedef struct
{
    //--------------------------------------------------------------
    uint16_t PV1OverCurr:1;    // 0     PV1 输入过流
    uint16_t PV2OverCurr:1;    // 1     PV2 输入过流
    uint16_t PV3OverCurr:1;    // 2     PV3 输入过流
    uint16_t PV1OverVolt:1;    // 3     PV1 输入过压

    uint16_t PV2OverVolt:1;    // 4     PV2 输入过压
    uint16_t PV3OverVolt:1;    // 5     PV3 输入过压
    uint16_t PVISO:1;          // 6     PV ISO 故障
    uint16_t Reserve1:1;       // 7     PV2 ISO 故障

    uint16_t Reserve2:1;       // 8     PV3 ISO 故障
    uint16_t PV1HardOC:1;      // 9     PV1 硬件故障
    uint16_t PV2HardOC:1;      // 10    PV2 硬件故障
    uint16_t PV3HardOC:1;      // 11    PV3 硬件故障

    uint16_t GFCIHardError:1;           // 12    GFCI 硬件电路故障
    uint16_t GFCI:1;           // 13    GFCI 故障
    uint16_t PhaseErr:1;       // 14    相序错误
    uint16_t FanFault:1;       // 15    风机故障
    //---------------------------------------------------------------
}FAULT2_T;
typedef union
{
    uint16_t all;
    FAULT2_T bit;
}FAULT2_U;
// ------------------------------故障信息3----------------------------
typedef struct
{
    //---------------------------------------------
    uint16_t ADZeroErr:1;      // 0 零飘异常
    uint16_t Res1:1;           // 1
    uint16_t BATVoltLow:1;     // 2 BAT 电压低压故障
    uint16_t BATVoltOver:1;    // 3 BAT 电压过压故障

    uint16_t Res2:3;           // 4~6
    uint16_t L1OverLoad:1;     // 7    L1 过载

    uint16_t L2OverLoad:1;     // 8    L2 过载
    uint16_t Reserve1:1;       // 9

    uint16_t L1InvError:1;     // 10   L1 逆变输出故障
    uint16_t L2InvError:1;     // 11   L2 逆变输出故障
    uint16_t Reserve2:1;       // 12
    uint16_t TempOver:1;       // 13   温度4过温
    uint16_t CommuErr:1;       // 14   通信故障
    uint16_t Res3:1;           // 15   预留
    //---------------------------------------------
}FAULT3_T;
typedef union
{
    uint16_t all;
    FAULT3_T bit;
}FAULT3_U;
// ------------------------------故障信息4----------------------------
typedef struct
{
    //---------------------------------------------
    uint16_t PVOVBad:1;      // 0 PV严重高压
    uint16_t res1:2;         // 1-2
    uint16_t ParaSet:1;      // 3 并联设置异常

    uint16_t ParaCommu:1;    // 4 并联通信异常
    uint16_t PV4OverCurr:1;  // 5 PV4输入过流
    uint16_t PV4OverVolt:1;  // 6 PV4输入过压
    uint16_t PV4ISO:1;       // 7 PV4 ISO故障
    uint16_t PV4HardOC:1;    // 8 PV4 硬件故障
    uint16_t Res3:7;         // 9
    //---------------------------------------------
}FAULT4_T;
typedef union
{
    uint16_t all;
    FAULT4_T bit;
}FAULT4_U;

// 影响状态机开关机
typedef struct
{
    uint16_t PV1Fault:1;       // PV1 故障标志
    uint16_t PV2Fault:1;       // PV2 故障标志
    uint16_t PV3Fault:1;       // PV3 故障标志
    uint16_t PV4Fault:1;       // PV4 故障标志
    uint16_t LLCFault:1;       // LLC 故障位

    uint16_t INVFault:1;       // INV 故障标志
    uint16_t SystemFault:1;       // 系统 故障标志
    uint16_t res1:2;           // 预留

    uint16_t res2:8;           // 预留
}STATE_FAULT_T;
typedef union
{
    uint16_t      all;
    STATE_FAULT_T bit;
}STATE_FAULT_U;

typedef struct
{
    FAULT1_U Fault1;        // 系统故障 1
    FAULT2_U Fault2;        // 系统故障 2
    FAULT3_U Fault3;        // 系统故障 3
    FAULT4_U Fault4;        // 系统故障 4
    STATE_FAULT_U StateErr; // 状态错误
    uint16_t ArmFault;      // ARM故障位
    uint16_t CTFaultFlag;   //CT自检错误
}SYSTEM_ERROR;
extern SYSTEM_ERROR SystemError;
//---------------------------告警----------------------------
typedef struct
{
    uint16_t GridVoltH:1;     // 0  电网过压
    uint16_t GridVoltL:1;     // 1  电网欠压
    uint16_t GridFreqH:1;     // 2  电网频率高
    uint16_t GridFreqL:1;     // 3  电网频率低

    uint16_t GridOsc:1;          // 4 预留 电网震荡
    uint16_t LossGrid:1;      // 5 电网丢失
    uint16_t PV1Low:1;        // 6 PV1 低压
    uint16_t PV2Low:1;        // 7 PV2 低压

    uint16_t PV3Low:1;        // 8 PV3 低压
    uint16_t Res2:1;          // 9 发电机
    uint16_t CAN:1;           // 10 调试CAN无通信
    uint16_t rs485:1;         // 11 调试485无通信

    uint16_t ESOR:1;          // 12 允许并网条件（电压频率）超范围
    uint16_t PV4Low:1;        // 13 PV4 低压
    uint16_t InvOV:1;         // 14 逆变输出高压
    uint16_t Arm:1;           // 15 Arm 故障位
}INVERTER_ALARM;
typedef union
{
   uint16_t         all;
   INVERTER_ALARM   bit;
}SYSTEM_ALARM;
extern SYSTEM_ALARM SystemAlarm;
// ------------------------逆变器故障----------------------------
typedef struct
{
    uint16_t OVIns:1;   // 0瞬时过压
    uint16_t OV1:1;     // 1-过压1
    uint16_t OV2:1;     // 2
    uint16_t UV1:1;     // 3-欠压1

    uint16_t UV2:1;     // 4-
    uint16_t OC1:1;     // 5-瞬时过流1
    uint16_t OC2:1;     // 6-长时过流1
    uint16_t ADFault:1; // bit7 电压采样失效
}INV_ERROR_T;
typedef union
{
    uint16_t    all;
    INV_ERROR_T bit;
}INV_ERROR_U;
typedef struct
{
    INV_ERROR_U Inv1Err;
    INV_ERROR_U Inv2Err;
    uint16_t NoCharge;          // 不允许并网充电标志
    uint16_t BATLowLimit;       // 电池低压限功率标志,低于83V生效,高于87V解除
}INV_ERROR;
extern INV_ERROR InvError;
// ------------------------电网异常----------------------------
typedef struct
{
	uint16_t OV1:1;    // BIT0 过压
	uint16_t OV2:1;    // BIT1 过压
	uint16_t UV1:1;    // BIT2 欠压
	uint16_t GridDrop:1;   //BIT3 电网跌落电网跌落uint16_t UV2:1;

	uint16_t OF1:1;    // BIT4 过频
	uint16_t OF2:1;    // BIT5 过频
	uint16_t UF1:1;    // BIT6 欠频
	uint16_t UF2:1;    // BIT7 欠频

	uint16_t LossGrid:1;    // 8 电网丢失
	uint16_t GridOC:1;      // 9 电网端口过流
	uint16_t Island:1;      // 10 孤岛
	uint16_t BusOV:1;       // 11 母线电压高
	uint16_t GridOCP:1;     //12
	uint16_t Grid1Drop1:1;  // 13 电网震荡
	uint16_t Grid2Drop1:1;  // 14 电网跌落
	uint16_t GridEnhanceDrop:1; // 15 电网跌落
}GRID_ERROR_T;
typedef union
{
    uint16_t all;
    GRID_ERROR_T bit;
}GRID_ERROR_U;
typedef struct
{
    GRID_ERROR_U Grid1Err;
    GRID_ERROR_U Grid2Err;
    uint16_t Grid1ErrOCCnt;
    uint16_t Grid2ErrOCCnt;
}GRID_ERROR;
extern GRID_ERROR GridError;

// 故障自动复位
typedef struct
{
    uint16_t TrigCnt;                 // 三分钟内触发故障的次数
    uint16_t *ErrCode;                // 故障标记
    uint16_t ErrBit;                  // 故障位bit位
    uint32_t ThirtyMinCnt;            // 3min计数器
    uint32_t RstCnt;                  // 单次复位延时计数
    uint32_t RstDly;                  // 单次复位时间
}AUTO_RST_T;

// 故障自动复位
typedef struct
{
    AUTO_RST_T L1OC;                // 逆变1硬件过流
    AUTO_RST_T L2OC;                // 逆变2硬件过流

    AUTO_RST_T PV1OC;               // PV1过流
    AUTO_RST_T PV2OC;               // PV2过流
    AUTO_RST_T PV3OC;               // PV3过流
    AUTO_RST_T PV4OC;               // PV4过流

    AUTO_RST_T OverLoad1;
    AUTO_RST_T OverLoad2;

    AUTO_RST_T Inv1Volt;
    AUTO_RST_T Inv2Volt;

    AUTO_RST_T BusBalance;
    AUTO_RST_T BusHHV;

}FATULT_AUTO_RST;
extern FATULT_AUTO_RST FaultAutoRst;    // 故障自动复位
// 孤岛检测
typedef struct
{
    uint16_t AllEnable;        // 两相孤岛检测自动使能

    uint16_t IslandEnable;     // 孤岛检测使能

    uint16_t TimeCheck;        // 单次扰动持续时间
    uint16_t TimeCheckCnt;     // 单次扰动持续时间计数器
    uint16_t CheckInterval;    // 检测间隔
    uint16_t CheckIntervalCnt; // 检测间隔计数器
    uint16_t CapFlag;          // R相捕获标志,用于判断相位起点
    uint16_t Dir;              // 频率扰动方向,0-频率大的方向扰动;1-频率小的方向扰动
    uint16_t ErrCnt;           // 计数值
    uint16_t CntFreq;
    uint16_t FilterIndex;
    float    FreqFilter[2];

    float MaxDeltaF;           // △f 最大值
    float DeltaF;              // △f
    float SumDeltaF;           // △f累加值
    float DeltaWt;             // △wt
    float AvgFreq;
    float LastAvgFreq;
    float SumFreq;
}ISLAND_PROTECT;
extern ISLAND_PROTECT IslandDataR;
extern ISLAND_PROTECT IslandDataS;

typedef struct
{
    uint16_t CBCEVTPeriodCnt;     // 单个周期内砍波事件计数器
    uint16_t CBCEVTTotalCnt;
    uint16_t CBCEVTCntArry[4];    // 砍波事件计数器
    uint16_t CBCEVTCntArryIndex;  // 砍波事件计数器

    uint16_t ShortCnt;            // 短路电压持续时间计数
}CBCEVT_DATA;

typedef struct
{
	int32_t GFCISUM ;
	int16_t InitValue ;
	int16_t DelayCnt  ;	//20ms基准     10S
	int16_t DelayCnt1 ;
	int16_t CalCnt ;
	int32_t GetRFealValue  ;
	int16_t CalAvgCnt ;
    int16_t GFCIHardError;
	int16_t HardErrorCnt  ;
	int16_t HardErrorCnt2  ;
	int32_t SumInitValue ;
    int16_t GFCIError;
	char CheckHardFinish  ;
	int16_t  GFCIAD ;
	int16_t GFCIRms ;
	int16_t GFCIRmsZero ;
	int16_t GFCItemp1;
	int16_t GFCItemp2;
	int16_t GFCIZeroFlag;
	int16_t GfciTestOver ;
	int16_t GFCIDelta ;
    int16_t GFCIDisplay;
}GFCI_DATA;

extern GFCI_DATA  GFCIData ;

struct  FAULT_CNT
{
    uint16_t BusHVCnt;
    uint16_t Bus1HVCnt;
    uint16_t Bus2HVCnt;   //BUS过压保护计数

    uint16_t BusLVCnt;
    uint16_t Bus1LVCnt;
    uint16_t Bus2LVCnt;   //BUS欠压保护计数

    uint16_t PV1OCCnt;
    uint16_t PV2OCCnt;
    uint16_t PV3OCCnt;
    uint16_t PV4OCCnt;    //PV过流保护计数

    uint16_t PV1HVCnt;
    uint16_t PV2HVCnt;
    uint16_t PV3HVCnt;
    uint16_t PV4HVCnt;    //PV过压保护计数

    uint16_t Inv1PowerOLCnt1;
    uint16_t Inv1PowerOLCnt2;
    uint16_t Inv1PowerOLCnt3; //离网L1功率过载
    uint16_t Inv2PowerOLCnt1;
    uint16_t Inv2PowerOLCnt2;
    uint16_t Inv2PowerOLCnt3; //离网L2功率过载
    uint16_t Inv1CurrOLCnt1;
    uint16_t Inv1CurrOLCnt2;
    uint16_t Inv1CurrOLCnt3;  //离网L1电流过载
    uint16_t Inv2CurrOLCnt1;
    uint16_t Inv2CurrOLCnt2;
    uint16_t Inv2CurrOLCnt3;  //离网L2电流过载

    uint16_t Grid1PowerOLCnt1;
    uint16_t Grid1PowerOLCnt2;//并网L1功率过载
    uint16_t Grid2PowerOLCnt1;
    uint16_t Grid2PowerOLCnt2;//并网L2功率过载
    uint16_t Grid1CurrOLCnt1;
    uint16_t Grid1CurrOLCnt2; //并网L1电流过载
    uint16_t Grid2CurrOLCnt1;
    uint16_t Grid2CurrOLCnt2; //并网L2电流过载

    uint16_t Bp1PowerOLCnt1;
    uint16_t Bp1PowerOLCnt2;//旁路L1功率过载
    uint16_t Bp2PowerOLCnt1;
    uint16_t Bp2PowerOLCnt2;//旁路L2功率过载
    uint16_t Bp1CurrOLCnt1;
    uint16_t Bp1CurrOLCnt2; //旁路L1电流过载
    uint16_t Bp2CurrOLCnt1;
    uint16_t Bp2CurrOLCnt2; //旁路L2电流过载

    uint16_t Inv1HVCnt;
    uint16_t Inv1LoadHVCnt;
    uint16_t Inv1LVCnt;
    uint16_t Inv1LoadLVCnt;
    uint16_t Inv2HVCnt;
    uint16_t Inv2LoadHVCnt;
    uint16_t Inv2LVCnt;
    uint16_t Inv2LoadLVCnt; //逆变输出故障（过欠压）

    uint16_t Inv1VBiasCnt;
    uint16_t Load1VBiasCnt;
    uint16_t Inv2VBiasCnt;
    uint16_t Load2VBiasCnt;   //逆变输出故障（电压不一致）

    uint16_t PV1OVBCnt;
    uint16_t PV2OVBCnt;
    uint16_t PV3OVBCnt;
    uint16_t PV4OVBCnt;   //PV严重高压

    uint16_t PV1LVCnt;
    uint16_t PV2LVCnt;
    uint16_t PV3LVCnt;
    uint16_t PV4LVCnt;   //PV欠压
};
extern struct FAULT_CNT FaultCnt;

void InitProtectData(void);
void ClearCommunicationFaultCount(void);
void ClearRS485CommunicationFaultCount(void);
void ClearCANCommunicationFaultCount(void);
void CheckCommunicationFault(void);
void AntiFlow(void);
void CheckAntiIslandingProtect(void);
void CheckGridVoltLoss(void);
void HandleProtect(void);
//void CheckPVCurr(void);
void CheckInvLoad(void);
void CheckInvTotalLoad(void);
void CheckGridLoad(void);
void CheckFanState(void);
void CheckGridVolt(void);
//void CheckInvterCurr(void);
void OCPSoft(void);
void CheckGridFreq(void);
void CheckInvterVolt(void);
void SetPhaseErrFlag(void);
void ECO(void);
void CheckOnInvterVoltADFault(void);
void CheckOffInvterVoltADFault(void);

void ModifyGridLossPara(void);

void CheckGridChargeState(void);
void CheckBATLimitChargeState(void);
void MainProtect(void);
uint16_t CheckErrState(void);
// 继电器自检
void ResetCheckInvRelay(void);
void ResetCheckGridRelayOffData(void);
void ResetCheckGridRelayOnData(void);
void CheckInvRelay(void);

uint16_t ReadUpdateState(void);

void CheckAutoRstErrCode(void);
void InitAutoRstErrCode(void);


void SetParallelErrBit(void);
void CleanParallelErrBit(void);

void GFCICheckTest(void);
void Check_Gfci_Vde0126_4105(void);

void Check_AcDc_Vde0126(void);

#endif /* APP_INC_HANDLEPROTECT_H_ */

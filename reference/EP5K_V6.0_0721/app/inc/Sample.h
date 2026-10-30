/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: Sample.h
Author: tzz
Version: 0.0.2
Date: 2021年10月12日
Description: 定义采样相关变量的数据类型
*************************************************/
#ifndef _SAMPLE_H_
#define _SAMPLE_H_
#include "stdint.h"

#define     UPLMT16(Var,Max)  {(Var)=((Var)>=(Max))?(Max):(Var);}
#define     DNLMT16(Var,Min)  {(Var)=((Var)<=(Min))?(Min):(Var);}
//--------------Mux
#define L1_LOAD_CUR    0
#define L2_LOAD_CUR    1
#define L1_MID_VOL     2
#define L2_MID_VOL     3
#define L1_LOAD_VOL    4
#define L2_LOAD_VOL    5

#ifdef  _GLOBALS
#define _EXT
#else
#define _EXT extern
#endif

//负载电流12、负载电压12、Vbus、VISO、Vpre-cL12
//M.DCI-L12、M.BUSHL、M.I-ACPV_L1、NC、Temp7、Temp8
//负载电压331.95Vdc->3V；负载电流：100Adc->3V；总母线电压626.5->3V；ISO：488V->3V；粘连电压：331.95Vdc->3V；
struct Cla1ToCpuMsg_Str{
    long cnt;
    long adresult1[8],adresult2[8];
    long adresultV1Sum,adresultV2Sum;
    long adresultV1,adresultV2;
    uint32_t mux;
    uint32_t muxV;
    uint32_t turn;
};

struct CpuToCla1Msg_Str{
    float theta;

};


_EXT struct Cla1ToCpuMsg_Str Cla1ToCpuMsg;
_EXT struct CpuToCla1Msg_Str CpuToCla1Msg;
/******************************************************************************
                         标定系数,所有系数放大10倍
******************************************************************************/
typedef struct
{
    int16_t BUSVoltK;      // BUS电压系数
    int16_t Bus1VoltK;     // Bus1电压系数
    int16_t Bus2VoltK;     // Bus2电压系数
    int16_t PV1VoltK;      // PV1电压系数
    int16_t PV1CurrK;      // PV1电流系数
    int16_t PV2VoltK;      // PV2电压系数
    int16_t PV2CurrK;      // PV2电流系数
    int16_t PV3VoltK;      // PV3电压系数
    int16_t PV3CurrK;      // PV3电流系数
    int16_t PV4VoltK;      // PV4电压系数
    int16_t PV4CurrK;      // PV4电流系数
    float PV1Effc;       // 效率
    float PV2Effc;       // 效率
    float PV3Effc;       // 效率
    float PV4Effc;       // 效率

    float Inv1VoltR;     // 逆变输出离网精度系数
    float Inv2VoltR;     // 逆变输出离网精度系数
    int16_t Inv1VoltK;     // 逆变输出R相电压系数
    int16_t Inv2VoltK;     // 逆变输出S相电压系数
    int16_t Inv1CurrK;     // 逆变输出R相电流系数
    int16_t Inv2CurrK;     // 逆变输出S相电流系数
    int16_t ACPVCurrK;     // 逆变输出S相电流系数
    int16_t Load1CurrK;    // 负载R相电流系数
    int16_t Load2CurrK;    // 负载S相电流系数
    int16_t Load1VoltK;    // 负载R相电流系数
    int16_t Load2VoltK;    // 负载S相电流系数
    int16_t Mid1VoltK;     // 负载R相电流系数
    int16_t Mid2VoltK;     // 负载S相电流系数
    int32_t Grid1VoltK;    // 电网R相电压系数
    int32_t Grid2VoltK;    // 电网S相电压系数
    int16_t DCCurr1K;      // 负载R相电流系数
    int16_t DCCurr2K;      // 负载S相电流系数
    int16_t Grid1CurrK;    // 负载R相电流系数
    int16_t Grid2CurrK;    // 负载S相电流系数
    int16_t LnCurrK;       // 负载S相电流系数
}ADC_SAMPLE_COEF;
extern ADC_SAMPLE_COEF SampleCalibCoef;

typedef struct
{
    int16_t LnCurrRaw;     // 负载R相电流系数
    int16_t DCCurr1Raw;    // 负载R相电流系数
    int16_t DCCurr2Raw;    // 负载S相电流系数
    int16_t Load1CurrRaw;    // 负载R相电流系数
    int16_t Load2CurrRaw;    // 负载S相电流系数
    int16_t Load1VoltRaw;    // 负载R相电流系数
    int16_t Load2VoltRaw;    // 负载S相电流系数
    int16_t Mid1VoltRaw;    // 负载R相电流系数
    int16_t Mid2VoltRaw;    // 负载S相电流系数
    int16_t BUSVoltRaw;      // 总母线电压AD值
    int16_t Bus1VoltRaw;     // Bus1电压AD值
    int16_t Bus2VoltRaw;     // Bus2电压AD值
    int16_t PV1VoltRaw;      // PV1电压AD值
    int16_t PV1CurrRaw;      // PV1电压AD值
    int16_t PV2VoltRaw;      // PV2电压AD值
    int16_t PV2CurrRaw;      // PV2电压AD值
    int16_t PV3VoltRaw;      // PV3电压AD值
    int16_t PV3CurrRaw;      // PV3电压AD值
    int16_t PV4VoltRaw;      // PV4电压AD值
    int16_t PV4CurrRaw;      // PV4电压AD值
    int16_t GFCICurrRaw;     // GFCI漏电流AD值
    int16_t Inv1VoltRaw;     // 逆变输出R相电压AD值
    int16_t Inv2VoltRaw;     // 逆变输出S相电压AD值
    int16_t Inv1CurrRaw;     // 逆变输出R相电流AD值
    int16_t Inv2CurrRaw;     // 逆变输出S相电流AD值
    int16_t AlterSelectRaw;  // 轮换采样AD值
    int16_t Grid1VoltRaw;    // 电网R相电压AD值
    int16_t Grid2VoltRaw;    // 电网S相电压AD值
    int16_t Grid1CurrRaw;    // 电网R相电压AD值
    int16_t Grid2CurrRaw;    // 电网S相电压AD值
    int16_t ACPVCurrRaw;    // 电网S相电压AD值
}ADC_SAMPLE_RAW;
extern ADC_SAMPLE_RAW SampleADRaw;

typedef struct
{
    int16_t DCCurr1Zero;    // 负载R相电流系数
    int16_t DCCurr2Zero;    // 负载S相电流系数
//    int16_t BUSVoltZero;      // 总母线电压零飘值
//    int16_t Bus1VoltZero;     // Bus1电压零飘值
//    int16_t Bus2VoltZero;     // Bus2电压零飘值
//    int16_t PV1VoltZero;      // PV1电压零飘值
    int16_t PV1CurrZero;      // PV1电流零飘值
//    int16_t PV2VoltZero;      // PV2电压零飘值
    int16_t PV2CurrZero;      // PV2电流零飘值
//    int16_t PV3VoltZero;      // PV3电压零飘值
    int16_t PV3CurrZero;      // PV3电流零飘值
//    int16_t PV4VoltZero;      // PV4电压零飘值
    int16_t PV4CurrZero;      // PV4电流零飘值

    int16_t PV1CurrZero2;      // PV1电流零飘值
    int16_t PV2CurrZero2;      // PV1电流零飘值
    int16_t PV3CurrZero2;      // PV1电流零飘值
    int16_t PV4CurrZero2;      // PV1电流零飘值

    int16_t GFCICurrZero;     // GFCI漏电流零飘值
    int16_t Inv1VoltZero;     // 逆变输出R相电压零飘值
    int16_t Inv2VoltZero;     // 逆变输出S相电压零飘值
    int16_t Inv1CurrZero;     // 逆变输出R相电流零飘值
    int16_t Inv2CurrZero;     // 逆变输出S相电流零飘值
    int16_t ACPVCurrZero;     // 逆变输出S相电流零飘值

    int16_t Mid1VoltZero;     // 中间R相电压零飘值
    int16_t Mid2VoltZero;     // 中间S相电压零飘值
    int16_t Load1VoltZero;    // 负载R相电压零飘值
    int16_t Load2VoltZero;    // 负载S相电压零飘值
    int16_t Load1CurrZero;    // 负载R相电流零飘值
    int16_t Load2CurrZero;    // 负载S相电流零飘值
    int16_t LnCurrZero;    // 负载S相电流零飘值
    int16_t Grid1VoltZero;    // 电网R相电压零飘值
    int16_t Grid2VoltZero;    // 电网S相电压零飘值
    int16_t Grid1CurrZero;    // 电网R相电压零飘值
    int16_t Grid2CurrZero;    // 电网S相电压零飘值
}AD_SAMPLE_ZERO_CALIBRATE;
extern AD_SAMPLE_ZERO_CALIBRATE SampleZeroCalibrate;

typedef struct
{
    float BusLastData[130];
    float BUSVoltInsCom;    //
    float BUSVoltIns;      // BUS电压瞬时值
    float Bus1VoltIns;     // Bus1电压瞬时值
    float Bus2VoltIns;     // Bus2电压瞬时值
    float PV1VoltIns;      // PV1电压瞬时值
    float PV1CurrIns;      // PV1电压瞬时值
    float PV2VoltIns;      // PV2电压瞬时值
    float PV2CurrIns;      // PV2电压瞬时值
    float PV3VoltIns;      // PV3电压瞬时值
    float PV3CurrIns;      // PV3电压瞬时值
    float PV4VoltIns;      // PV4电压瞬时值
    float PV4CurrIns;      // PV4电流瞬时值

    float Inv1VoltIns;     // 逆变输出R相电压瞬时值
    float Inv2VoltIns;     // 逆变输出S相电压瞬时值
    float Inv1CurrIns;     // 逆变输出R相电流瞬时值
    float Inv2CurrIns;     // 逆变输出S相电流瞬时值
    float ACPVCurrIns;     // 逆变输出S相电流瞬时值

    float DCCurr1Ins;     // 逆变输出R相电流瞬时值
    float DCCurr2Ins;     // 逆变输出S相电流瞬时值

    float LnCurrIns;
    float Load1CurrIns;    // 负载R相电流瞬时值
    float Load2CurrIns;    // 负载S相电流瞬时值
    float Load1VoltIns;    // 负载R相电压瞬时值
    float Load2VoltIns;    // 负载S相电压瞬时值
    float Mid1VoltIns;     // 负载R相电压瞬时值
    float Mid2VoltIns;     // 负载S相电压瞬时值
    float Grid1VoltIns;    // 电网R相电压瞬时值
    float Grid2VoltIns;    // 电网S相电压瞬时值
    float Grid1CurrIns;    // 电网R相电流瞬时值
    float Grid2CurrIns;    // 电网S相电流瞬时值

    float BUSVoltSum8;        // BUS 电压瞬时累加值,8点
    float BUSVoltSum200;      // BUS 电压瞬时累加值,200点
    float Bus1VoltSum8;       // Bus1电压瞬时累加值,8点
    float Bus2VoltSum8;       // Bus2电压瞬时累加值,8点
    float Bus1VoltSum200;     // Bus1电压瞬时累加值,200点
    float Bus2VoltSum200;     // Bus2电压瞬时累加值,200点
    float LnDCCurrSum200;

    float PV1VoltSum8;        // PV1电压瞬时累加值
    float PV1CurrSum8;        // PV1电压瞬时累加值
    float PV2VoltSum8;        // PV2电压瞬时累加值
    float PV2CurrSum8;        // PV2电压瞬时累加值
    float PV3VoltSum8;        // PV3电压瞬时累加值
    float PV3CurrSum8;        // PV3电压瞬时累加值
    float PV4VoltSum8;        // PV4电压瞬时累加值
    float PV4CurrSum8;        // PV4电流瞬时累加值

    float Inv1VoltSum;      // 逆变输出R相电压瞬时平方累加值
    float Inv2VoltSum;      // 逆变输出S相电压瞬时平方累加值
    float Inv1CurrSum;      // 逆变输出R相电流瞬时平方累加值
    float Inv2CurrSum;      // 逆变输出S相电流瞬时平方累加值
    float ACPVCurrSum;      // 逆变输出S相电流瞬时平方累加值

    float LnCurrSum;        // 负载R相电流瞬时平方累加值
    float Load1CurrSum;     // 负载R相电流瞬时平方累加值
    float Load2CurrSum;     // 负载S相电流瞬时平方累加值
    float Load1VoltSum;     // 负载R相电压瞬时平方累加值
    float Load2VoltSum;     // 负载S相电压瞬时平方累加值
    float Mid1VoltSum;      // 负载R相电压瞬时平方累加值
    float Mid2VoltSum;      // 负载S相电压瞬时平方累加值
    float Grid1VoltSum;     // 电网R相电压瞬时平方累加值
    float Grid2VoltSum;     // 电网S相电压瞬时平方累加值
    float Grid1CurrSum;     // 电网R相电流瞬时平方累加值
    float Grid2CurrSum;     // 电网S相电流瞬时平方累加值

    float DCCurr1Sum;     // 电网R相电流瞬时平方累加值
    float DCCurr2Sum;     // 电网S相电流瞬时平方累加值


    float BusBuffer[240];
    //        uint16_t SwitchCnt;       // 选通计数器
    uint16_t SwitchStep;
    uint16_t Count1;          // 采样计数器1
    uint16_t Count2;
    uint16_t Count3;
    uint16_t Count4;          // 采样计数器4
    uint16_t Count5;
    //        uint16_t Count6;
    //        uint16_t Count7;          // 采样计数器7
    //        uint16_t Count8;
    //        uint16_t Count9;

    uint16_t PVAutoZeroFlag;  // 用于PV自动校准零飘
    uint16_t CntGridRMS;      // 用于电网电压有效值计算
    uint16_t GridPeriodPoint; // 用于电网电压有效值计算
    uint16_t AcCalOverFlag;   // AC 有效值计算完成标志
    uint16_t ACSampleZeroFlag;
    uint16_t DCSampleZeroFlag;
    uint16_t SelectSampleZeroFlag;
    uint16_t CalSampleZeroFlag;
    uint16_t LoadZeroCalCnt;
    uint16_t DCSampleK;
    uint16_t PeriodPoint;     // 交流信号周期采样点数

}ADC_SAMPLE_INS;
extern ADC_SAMPLE_INS SampleIns;    // 瞬时值

typedef struct
{

    float LnDCCurrFilter200;

    float BUSVoltFilter8;    // BAT 电压滤波后的值
    float BUSVoltFilter200;  // BAT 电压滤波后的值

    float PV1VoltFilter8;    // PV1电压滤波后的值,用于PV控制&功率计算
    float PV2VoltFilter8;    // PV2电压滤波后的值
    float PV3VoltFilter8;    // PV3电压滤波后的值
    float PV4VoltFilter8;    // PV3电压滤波后的值

    float PV1CurrFilter8;    // PV1电流滤波后的值,用于计算PV功率
    float PV2CurrFilter8;    // PV2电流滤波后的值
    float PV3CurrFilter8;    // PV3电流滤波后的值
    float PV4CurrFilter8;    // PV3电流滤波后的值

    float Bus1VoltFilter8;   // BUS1滤波后的电压值,用于逆变控制
    float Bus2VoltFilter8;   // BUS2滤波后的电压值
    float Bus1VoltFilter200; // BUS1滤波后的电压值,用于逆变控制
    float Bus2VoltFilter200; // BUS2滤波后的电压值,用于逆变控制
    float Inv1VoltRms;       // 逆变输出R相电压有效值
    float Inv2VoltRms;       // 逆变输出S相电压有效值
    float Inv1CurrRms;       // 逆变输出R相电流有效值
    float Inv2CurrRms;       // 逆变输出S相电流有效值
    float LnCurrRms;       // 逆变输出S相电流有效值
    float Load1CurrRms;      // 负载R相电流有效值
    float Load2CurrRms;      // 负载S相电流有效值
    float Load1VoltRms;      // 负载1电压
    float Load2VoltRms;      // 负载2电压
    float Mid1VoltRms;       // 中间1电压
    float Mid2VoltRms;       // 中间2电压
    float Grid1VoltRms;      // 电网R相电压有效值
    float Grid2VoltRms;      // 电网S相电压有效值
    float Grid1CurrRms;      // 电网R相电压有效值
    float Grid2CurrRms;      // 电网S相电压有效值

    float DCCurr1Rms;       // 逆变输出R相电流有效值
    float DCCurr2Rms;       // 逆变输出S相电流有效值

    float ACPVCurrRms;      // 逆变输出S相电流有效值
}ADC_SAMPLE_RMS;
extern ADC_SAMPLE_RMS SampleRms;

typedef struct
{
    float PV1Volt;      // PV1电压显示
    float PV2Volt;      // PV2电压显示
    float PV3Volt;      // PV3电压显示
    float PV4Volt;      // PV4电压显示
    float Inv1Volt;     // 逆变输出R相电压显示
    float Inv2Volt;     // 逆变输出S相电压显示
    float Inv1Curr;     // 逆变输出R相电流显示
    float Inv2Curr;     // 逆变输出S相电流显示
    float Load1Curr;    // 负载R相电流显示
    float Load2Curr;    // 负载S相电流显示
    float Load1Volt;    // 负载R相电压显示
    float Load2Volt;    // 负载S相电压显示
    float Grid1Volt;    // 电网R相电压显示
    float Grid2Volt;    // 电网S相电压显示
//    float VISOVolt;     // VISO电压显示

    float PV1VoltSum;      // PV1电压显示
    float PV2VoltSum;      // PV2电压显示
    float PV3VoltSum;      // PV3电压显示
    float PV4VoltSum;      // PV4电压显示
    float Inv1VoltSum;     // 逆变输出R相电压显示
    float Inv2VoltSum;     // 逆变输出S相电压显示
    float Inv1CurrSum;     // 逆变输出R相电流显示
    float Inv2CurrSum;     // 逆变输出S相电流显示
    float Load1CurrSum;    // 负载R相电流显示
    float Load2CurrSum;    // 负载S相电流显示
    float Load1VoltSum;    // 负载R相电压显示
    float Load2VoltSum;    // 负载S相电压显示
    float Grid1VoltSum;    // 电网R相电压显示
    float Grid2VoltSum;    // 电网S相电压显示

    uint16_t Count1;
    uint16_t Count2;
}SAMPLE_DISPLAY;
extern SAMPLE_DISPLAY SampleDisplayData;

void InitSampleData(void);
void HandleSample(uint16_t type);
void PVAutoCheck(void);
void Gpio_Multiplexer(int16_t senum);
void MulTiSecWait(uint16_t Delay);
void CalculateDisplayData(void);

#endif /* MODULE_SRC_HANDLESAMPLE_H_ */

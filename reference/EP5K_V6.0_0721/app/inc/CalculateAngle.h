/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: CalculateAngle.h
Author: tzz
Version: 0.0.2
Date: 2021年10月12日
Description: 定义锁相相关变量的数据类型;声明外部调用变量及函数
*************************************************/
#include "stdint.h"

#ifndef _CALCULATEANGLEFLOAT_H_
#define _CALCULATEANGLEFLOAT_H_

#define  PI_S  3.141592f
#define  PI_K  0.031415926f

// 电网电压捕获频率值上下限
#define CAP_PERIOD_HI   68.5f   // 频率上限 66.5
#define CAP_PERIOD_LO   41.5f   // 频率下限 43.5

typedef struct
{
    uint16_t  SyncBeginFlag;     // 相位同步开始标志
    uint16_t  SyncReadyFlag;     // 相位同步完成标志
    uint16_t  SyncDelayCount;    // 相位同步延时计数
    int16_t   ThetaPointR;       // R相当前角度点位置号
    int16_t   ThetaPointS;       // S相当前角度点位置号

    uint16_t  GridPeriodPoint;   // 单个周期内采样点数
    uint16_t  PeriodPoint;       // 单个周期内采样点数
    uint16_t  PeriodPointS;      // S相单个周期内采样点数
    uint16_t  PeriodPointMax;    // 单个周期内采样点数最大值
    uint16_t  PeriodPointMin;    // 单个周期内采样点数最小值

    float DeltR;                 // delta;
    float DeltS;                 // delta;
    float ThetaR;                // R相角度值
    float ThetaS;                // S相角度值
    float SinR;                  // R相角度正弦
    float CosR;                  // R相角度余弦
    float SinS;                  // S相角度正弦
    float CosS;                  // S相角度余弦
    float HrThetaR;              // R相角度值
}INV_PHASE;
extern INV_PHASE InvPhase;

typedef struct
{
    uint32_t CmpDataR;          // R相输入捕获运算值,用于比较捕获周期是否在合法范围内
    uint32_t RawValueR[2];      // R相输入捕获原始值

    uint32_t CmpDataS;
    uint32_t RawValueS[2];      // S相输入捕获原始值

    uint32_t FacHighCycle;      // 频率捕获值上限
    uint32_t FacLowCycle;       // 频率捕获值下限

    uint16_t CntFreqR;          // 频率计数用于计算平均值
    uint16_t CntFreqS;          // 频率计数用于计算平均值

    uint16_t RZeroFlag;         // 用于R相过零触发
    uint16_t SZeroFlag;         // 用于S相过零触发

    float LastFreqR;            // R相上次计算的捕获频率值
    float LastFreqS;            // S相上次计算的捕获频率值
    float FreqR;                // R相当前捕获频率值
    float FreqS;                // S相当前捕获频率值

    float SumFreqR;
    float SumFreqS;
    float AvgFreqR;
    float AvgFreqS;
}CAPTURE_DATA;

extern CAPTURE_DATA CaptureData;       // 输入捕获

void InitCalculateAngleData(void);
void CalculateAngle(uint16_t state, uint16_t capFlag, uint32_t capValue, int16_t gridVolt);
void HandleInvterPhase(void);
void UpdateAngle(void);
void ResetOffGridAngle(void);
void SoftPLL(void);
void MicroInvterModeFreqChange(void);
//int16_t ReadThetaPointR(void);
//uint16_t ReadSyncReadyFlag(void);
#endif

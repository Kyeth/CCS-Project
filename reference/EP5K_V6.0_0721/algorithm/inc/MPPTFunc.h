/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name:MPPTFunc.h
** Latest modified Date:2021-08-27
** Latest Version: V1.0
** Description:MPPT模块数据结构定义;对外函数声明
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2021年10月6日
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/
#ifndef _MPPT_H_
#define _MPPT_H_

#include "stdint.h"

typedef struct
{
    uint16_t  ID;                // 4分频
    uint16_t  turn;              // 4分频
    uint16_t  DisturbPeriod;     // 扰动周期1s
    uint16_t  DisturbHalfPeriod; // 扰动周期的一半
    uint16_t  MpptPeriodCnt;     // 扰动计数
    int16_t   DisturbDir;        // 扰动方向
    int16_t   MPPTEnable;    // 使能标志
    uint16_t  MPPtErrorcnt;  // 计算错误计数
    int16_t   FastMpptFlag;  // 快速扰动标志位
    uint16_t  BusOVFlag;      // 4分频

    uint16_t  DelayCnt;      // 快速扰动切换到正常扰动后延时计数器
    float*  PVVoltAvg;       // 暂定 8 次滤波
    float*  BusVolt;         // 输出电压
    float*  PVCurrentAVg;    // 暂定 8 次滤波
    float   Powerdelta;

    float   DisturbStep;   // 扰动步长
    float   cMpptStepMax;  // 扰动步长最大值
    float   cMpptStepMin;  // 扰动步长最小值
    float   StepVolt;
    float   FastStep;      // 快速扰动步长

    float   Output;        // MPPT输出
    float   MpptOutMax;    // MPPT输出最大值
    float   MpptOutMin;    // MPPT输出最小值
    long double  PowerNew;   // 本次功率(累加)
    long double  PowerOld;   // 上次功率(累加)
}MPPT_DATA;

void MpptFunc(MPPT_DATA *mppt);

#endif /* ending */

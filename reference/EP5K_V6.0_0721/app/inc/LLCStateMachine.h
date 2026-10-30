/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: LLCStateMachine.h
Author: tzz
Version: 0.0.2
Date: 2022年2月15日
Description: LLC状态机结构体数据类型定义
*************************************************/
#ifndef _LLCSTATEMACHINE_H_
#define _LLCSTATEMACHINE_H_

#include "stdint.h"

// llc软启动允许最大电压
#define LLC_START_VOLT_MAX      (235)

typedef enum
{
    LLC_INIT_STATE = 0,
    LLC_SELF_STATE,
    LLC_RUN_STATE
}LLC_STATE_TYPE;

typedef struct
{
    LLC_STATE_TYPE State;      // LLC 状态值
    uint16_t Count;            // 计数器
    uint16_t DelayCnt;         // 关机延时计数器
    uint16_t Period;           // 周期

    uint16_t LLCParaFlg;       // LLC参数修改标志
    float LDeathTime;          // 低压侧死区
    float HDeathTime;          // 高压侧死区
    float LLCFreq;             // 频率
}LLC_STATE_DATA;

extern LLC_STATE_DATA LLCWorkState;

typedef struct
{
    uint16_t LLCAdjustEnable;       // LLC 软启使能标志位
    uint16_t LLCID;                 // LLC 通道号
    uint16_t LLCSoftStarEnable;     // LLC 软启使能标志位
    uint16_t LLCSoftStartFinish;    // LLC 软启动标志位
    uint16_t PwmDuty;               // 软启占空比
    uint16_t SoftStartCnt;          // 软启执行计数
    uint16_t SoftStartPeriod;       // 软启动执行周期，10ms/次
    uint16_t SoftFinishDelayCnt;    // 软启结束延时计数，
    uint16_t SoftFinishIinterval;   // 软启结束延时时间间隔
    uint16_t LLCPwmPeriod;          // LLCPWM 周期值
    uint16_t LLCPwmHalfPeriod;      // LLCPWM 周期值一半
    uint16_t LLCDeathTimeCount;     // LLCPWM 周期值一半
    uint16_t WaitFlg;               // 等待标志
    float LLCDeathTime;             // 死区时间单位 us
    float VbusRated;                // LLC理论BUS电压
    float VbusSamp;                 // LLC采样BUS电压
}LLC_ADJUST_DATA;

extern LLC_ADJUST_DATA LLCAdjustData1, LLCAdjustData2;

void InitLLCAdjustData(void);

void LLCOpenLoopSoftStart(LLC_ADJUST_DATA *llc);

void RunLLCStateMachine(void);
uint16_t GetLLCSoftStartSate(void);

#endif /* ALGORITHM_INC_LLCADJUST_H_ */

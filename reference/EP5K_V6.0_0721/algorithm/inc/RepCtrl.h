/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: RepCtrl.h
Author: tzz
Version: 1.0.2
Date: 2022年2月15日
Description: 申明函数
*************************************************/
#ifndef _REPCTRL_H_
#define _REPCTRL_H_

#include "stdint.h"
//=========================宏定义=================================================
typedef struct
{
    int16_t OSTheta;      // 系统角度
    int16_t ThetaShift;   // 角度偏移
    int16_t Theta;        // 偏移后的角度
    uint16_t PeriodPoint; // 周期点数

    float kp;           // 当前点的 kp 会根据当前点所在的位置在kpSet的基础上除以某个系数
    float kpSet;        // 设定的重复 kp 值
    float Data[230];    // 记录的所有点的上一次的补偿值
    float Ref;          // 当前相位点的标准正弦值
    float Fdb;          // 当前相位点的反馈值

    float OutLast;      // 当前相位点上次的补偿值
    float ProporOut;    // 当前相位点比例输出
    float Out;          // 当前相位点输出补偿值
    float CfOut;        // 当前相位点输出补偿值
    float OutMax;       // 输出最大值
    float OutMin;       // 输出最小值
}REPEAT_CTRL_F;

void RepeatControlFunc(REPEAT_CTRL_F *repeat);
void RepeatControlFunc2(REPEAT_CTRL_F *repeat);
#endif /* ALGORITHM_INC_REPCTRL_H_ */

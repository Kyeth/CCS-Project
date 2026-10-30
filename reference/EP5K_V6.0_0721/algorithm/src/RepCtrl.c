/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: RepCtrl.c
Author: tzz
Date:2022年2月15日
Description: 重复控制算法模块
Version:1.0.2
Function List:
***********************************************************/
#include "RepCtrl.h"

/*************************************************
Function: RepeatControlFunc
Description: 重复控制函数体
Input: @repeat-重复控制结构体变量指针
Output: None
Frequency: 10KHz
*************************************************/
void RepeatControlFunc(REPEAT_CTRL_F *repeat)
{
    repeat->Theta = repeat->OSTheta + repeat->ThetaShift; // 当前实际补偿的相位点

    if (repeat->Theta < 0)
    {
        repeat->Theta += repeat->PeriodPoint;
    }

    else if (repeat->Theta >= repeat->PeriodPoint)
    {
        repeat->Theta -= repeat->PeriodPoint;
    }
    // 上一次的补偿值
    repeat->OutLast = repeat->Data[repeat->OSTheta]*0.95f;
    // 比例
    repeat->ProporOut = (repeat->Ref - repeat->Fdb) * repeat->kp;
    // 上次和当前补偿值为实际当前补偿值
    repeat->Out = repeat->ProporOut + repeat->OutLast;
    // 限制幅值
    repeat->Out = __fsat(repeat->Out, repeat->OutMax, repeat->OutMin);
    // 更新上一次的补偿值
    repeat->Data[repeat->OSTheta] = repeat->Out;
    // 取偏后的输出值
    repeat->CfOut = repeat->Data[repeat->Theta];
}



void RepeatControlFunc2(REPEAT_CTRL_F *repeat)
{
    repeat->Theta = repeat->OSTheta + repeat->ThetaShift; // 当前实际补偿的相位点

    if (repeat->Theta < 0)
    {
        repeat->Theta += repeat->PeriodPoint;
    }

    else if (repeat->Theta >= repeat->PeriodPoint)
    {
        repeat->Theta -= repeat->PeriodPoint;
    }
    // 上一次的补偿值
    repeat->OutLast = repeat->Data[repeat->OSTheta]*0.997f;

    // 上次和当前补偿值为实际当前补偿值
    repeat->Out = (repeat->Ref - repeat->Fdb)*0.003f + repeat->OutLast;

    // 更新上一次的补偿值
    repeat->Data[repeat->OSTheta] = repeat->Out;

    // 取偏后的输出值
    repeat->CfOut = repeat->Data[repeat->Theta] * repeat->kp ;

    // 限制幅值
    repeat->CfOut = __fsat(repeat->CfOut, repeat->OutMax, repeat->OutMin);
}


/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: rmp.c
Author: tzz
Date:2022年3月2日
Description: 斜坡函数实现
Version:0.0.2
Function List:
***********************************************************/
#include "rmp.h"
/*************************************************
Function: RmpCntlCalc
Description: Park变换
Input: @v-斜坡函数当前值
Output:斜坡当前值
Frequency:
*************************************************/
float RmpCntlCalc(RMPCNTL *v)
{
    if (v->TargetValue > v->SetpointValue + v->StepValue)
    {
        v->SetpointValue = v->SetpointValue + v->StepValue;
    }
    else if (v->TargetValue < v->SetpointValue - v->StepValue)
    {
        v->SetpointValue = v->SetpointValue - v->StepValue;
    }
    else
    {
        v->SetpointValue = v->TargetValue;
    }


//    // 检查是否越界
//    if (v->SetpointValue > v->RampHighLimit)
//    {
//        v->SetpointValue = v->RampHighLimit;
//    }
//    else if (v->SetpointValue < v->RampLowLimit)
//    {
//        v->SetpointValue = v->RampLowLimit;
//    }

    // 返回当前设定值
    return v->SetpointValue;
}

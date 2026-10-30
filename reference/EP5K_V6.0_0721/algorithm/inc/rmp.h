/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: rmp.h
Author: tzz
Version: 0.0.2
Date: 2022年1月7日
Description: 申明函数,定义变量类型
*************************************************/
#ifndef _RMP_H_
#define _RMP_H_

typedef struct
{
    float TargetValue;    // Input: Target input
    float StepValue;      // Input: one step
    float RampLowLimit;   // Parameter: Minimum limit
    float RampHighLimit;  // Parameter: Maximum limit
    float SetpointValue;  // Output: Target output
    float finalCurr;
}RMPCNTL;

float RmpCntlCalc(RMPCNTL *v);

#endif /* _RMP_H_ */

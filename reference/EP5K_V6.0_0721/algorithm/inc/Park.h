/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: Park.h
Author: tzz
Version: 0.0.2
Date: 2022年2月15日
Description: 申明函数,定义变量类型
*************************************************/
#ifndef _PARK_H_
#define _PARK_H_

typedef struct
{
    float Alph;
    float Beta;
    float Sin;
    float Cos;
    float Ds;
    float Qs;
}PARK_STRUCT;

void ParkCalc(PARK_STRUCT *v);   //ab-dq
void IparkCalc(PARK_STRUCT *v);  //dq-ab

#endif /* ALGORITHM_INC_PARK_H_ */

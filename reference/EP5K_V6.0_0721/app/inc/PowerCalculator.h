/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: PowerCalculator.h
Author: tzz
Version: 0.0.2
Date: 2022年1月15日
Description: 申明函数
*************************************************/
#ifndef _POWERCALCULATOR_H_
#define _POWERCALCULATOR_H_

#include "stdint.h"

typedef struct
{
   unsigned int IndexP;    // 功率索引
   unsigned int IndexIns;  // 瞬时功率索引
   float InsPowerSum;      // 瞬时功率累加值
   float PowerSum;         // 功率累加值
   float Buffer[17];       // 滤波缓存数组
   float Average;          // 功率平均值(有功功率)
   float PowerTemp;        // 当前功率
}POWER;
typedef struct
{
    POWER PV1;
    POWER PV2;
    POWER PV3;
    POWER PV4;

    POWER Inv1;
    POWER Inv2;

    POWER Load1;
    POWER Load2;

    POWER ApLoad1;
    POWER ApLoad2;

    POWER ApInv1;   // 逆变视在功率
    POWER ApInv2;   // 逆变视在功率

    POWER Grid1;   // 逆变视在功率
    POWER Grid2;   // 逆变视在功率

}SYSTEM_POWER;

extern SYSTEM_POWER SystemPower;

void ResetPowerCalculator(POWER *Power);
void ApparentPowerCalculator(POWER *Power, float Volt, float Curr);
void PowerCalculator(POWER *Power, float Volt, float Curr, float periodPoint);
void RepowerCalculator(float *Re, float P ,float P_Set,float Q_Set);

#endif /* APP_INC_POWERCALCULATOR_H_ */

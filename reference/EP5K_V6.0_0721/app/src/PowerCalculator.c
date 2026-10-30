/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: PowerCalculator.c
Author: tzz
Date:2022年1月15日
Description:计算有功功率和视在功率,并做16次均值滤波处理
Version:0.0.2
Function List:
***********************************************************/
#include "string.h"
#include "PowerCalculator.h"

SYSTEM_POWER SystemPower;

/*************************************************
Function: PowerCalculator
Description: 计算有功功率
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(PowerCalculator, ".TI.ramfunc");
void PowerCalculator(POWER *Power, float Volt, float Curr, float periodPoint)
{
    Power->InsPowerSum += Volt*Curr;    // 单位 1W

    // 计算有功功率 P = 单个周波内瞬时电压电流乘积累加值的平均值
    if (++Power->IndexIns >= periodPoint)
    {
        Power->IndexIns = 0;

        Power->Buffer[Power->IndexP] = Power->InsPowerSum/periodPoint;
        Power->PowerTemp = Power->Buffer[Power->IndexP];
        Power->PowerSum += Power->Buffer[Power->IndexP];

        Power->IndexP++;
        Power->InsPowerSum = 0;
    }
    // 计算连续 16 次的有功功率的平均值
    if (Power->IndexP >= 16)
    {
        Power->IndexP = 0;

        Power->Average = Power->PowerSum*(1.0f/16.0f);

        Power->PowerSum = 0;
    }
}
/*************************************************
Function: PowerCalculator
Description: 计算有功功率,16次平均滤波
Input: None
Output: None
Frequency:
*************************************************/
#pragma CODE_SECTION(ApparentPowerCalculator, ".TI.ramfunc");
void ApparentPowerCalculator(POWER *Power, float Volt, float Curr)
{
    Power->Buffer[Power->IndexP] = Volt*Curr;           // 单位 1W 8cycle
    Power->PowerSum += Power->Buffer[Power->IndexP];    // 10 cycle
    Power->PowerTemp = Power->Buffer[Power->IndexP];

    if (++Power->IndexP >= 16)
    {
        Power->IndexP = 0;

        Power->Average = Power->PowerSum*(1.0f/16.0f);

        Power->PowerSum = 0;
    }
}
/*************************************************
Function: ResetPowerCalculator
Description: 清复位功率值
Input: None
Output: None
Frequency:
*************************************************/
#pragma CODE_SECTION(ResetPowerCalculator, ".TI.ramfunc");
void ResetPowerCalculator(POWER *Power)
{
//    memset(Power, 0, sizeof(POWER));    // 41 cycle

    Power->InsPowerSum = 0;
    Power->PowerSum = 0;
    Power->IndexIns = 0;
    Power->IndexP = 0;
    Power->Average = 0;
    Power->PowerTemp = 0;
}


/*******************************************************
 *
 *认证计算无功功率
 *RepowerCalculator
 ********************************************************/
void RepowerCalculator(float *Re, float P ,float P_Set,float Q_Set)
{
    if(P_Set==0.0)
    {
        *Re=Q_Set+20.0;
    }
    else
    {
        *Re=(Q_Set/P_Set)*P;
    }

}


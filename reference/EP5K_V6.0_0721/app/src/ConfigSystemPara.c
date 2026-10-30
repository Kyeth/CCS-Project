/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: ConfigSystemPara.c
Author: tzz
Version : 0.0.2
Date:2022年2月14日
Description: 系统参数配置模块初始化
Function List: 额定功率、额定电流
***********************************************************/
#include "string.h"
#include "bsp.h"
#include "ConfigSystemPara.h"

SYSTEM_PARA SystemPara;
/*************************************************
Function: InitSystemControlData
Description: 初始化系统参数配置模块变量
Input: None
Output: None
Frequency: Once
*************************************************/
void InitSystemControlData(void)
{
    memset(&SystemPara, 0, sizeof(SYSTEM_PARA));

    SystemPara.ModbusWEnable = 1;           // 默认 Modbus 写无效

    SystemPara.EnDCAdjust = 1;

    // 1.PV 最大目标功率默认设置值

    // 2.BMS 最大充电电流默认值
    SystemPara.BMSChgMaxPowerSet = 10000.0f;
//    SystemPara.SinglePVMaxPowerSet = SystemPara.BMSChgMaxPowerSet/4.0f;
    SystemPara.BMSDischgMaxPowerSet = 0;

    // 3.单路 PV 额定额定功率 2.5KW
    SystemPara.PVRatedPower = 2530.0f;//2500允许20W向上误差
    SystemPara.PVRatedCurr = SystemPara.PVRatedPower/155.0f;
    // 系统频率,逆变运行状态下不可以修改
    SystemPara.FrequencySet = 0;            // 0-50Hz; 1-60Hz
    SystemPara.InvRateFreq = (SystemPara.FrequencySet == 1) ? 60 :50;
    SystemPara.InvRateVolt = 101;
//    SystemPara.InvRateVolt = 110;
    // 功率初始值
    SystemPara.ActivePowerSetR = 0;
    SystemPara.ActivePowerSetS = 0;
    SystemPara.GridChgMaxCurrSet = 50;      // 电网给电池充电电流最大值
    SystemPara.GridDischgMaxCurrSet = 75;   // 电池放电到电网

    SystemPara.FanCtrEn = 1;                // 风机调速默认使能

    SystemPara.GridEnhanceIStep = 500.0f;     // 电流变化率
}
/*************************************************
Function: ControlFanSpeed
Description: 风机速度控制
Input: @speed-0-0%，1-25%，2-50%，3-75%，4-100%
Output: None
Frequency: 500ms
*************************************************/
void ControlFanSpeed(uint16_t speed)
{
    switch (speed)
    {
        case 0:
        {
            SetFanSpeed(0.0f);
            break;
        }
        case 1:
        {
            SetFanSpeed(0.2f);       //      0.25->0.15
            break;
        }
        case 2:
        {
            SetFanSpeed(0.4f);       //      0.5->0.32
            break;
        }
        case 3:
        {
            SetFanSpeed(0.6f);       //
            break;
        }
        case 4:
        {
            SetFanSpeed(1.0f);       //
            break;
        }

        default:
            break;
    }
}

/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: ConfigSystemPara.h
Author: tzz
Version: 0.0.2
Date: 2022年2月14日
Description: 系统配置相关变量数据类型定义,变量声明,函数声明
*************************************************/
#ifndef _CONFIGSYSTEMPARA_H_
#define _CONFIGSYSTEMPARA_H_

#include "stdint.h"

#define ARM_FAULT_BITS      (0x1221)        // ARM故障使能位0X3A1->0X221->0x1221 for PV
#define ARM_FAULT_INV_BITS  (0x221)         // ARM故障使能位0X3A1->0X221 for inv
#define INV_RATE_POWER      (2750)          // 逆变器单相额定功率
/*****************************************************************************/
typedef struct
{
    uint16_t ModbusWEnable;          // Modbus 写使能
    uint16_t FanCtrEn;               // 风扇调速使能
    uint16_t FanSpeed;               // 风机档位
    uint16_t PVWorkNum;              // PV 当前工作数量
    uint16_t Lookwindows;

    float GridMaxChgPowerLimit;      // 电网最大充电功率限值
    float ActivePowerSetR;           // R 相有功功率设定值,正发电,负充电
    float ActivePowerSetS;           // S 相有功功率设定值
    float InActivePowerSetR;         // R 相无功功率设定值,仅认证需要无功控制
    float InActivePowerSetS;         // S 相无功功率设定值
    float BMSChgMaxVoltSet;          // BMS 允许充电电压
    float BMSChgMaxPowerSet;          // BMS 允许充电功率
    float BMSDischgMaxPowerSet;       // BMS 允许放电功率
    float ActivePowerSetRTemp;           //
    float ActivePowerSetSTemp;           //
    float GridActivePowerR;           //
    float GridActivePowerS;           //
    uint16_t EnDCAdjust;             // 使能直流分量调节
    uint16_t Restore;                // 恢复出厂设置
    uint16_t SystemPowerOn;          // 系统开关机设置
    uint16_t FrequencySet;           // 频率设置   0-50hz; 1-60hz
    uint16_t GridEnhanceEn;          // 电网增强模式
    uint16_t OpenLoopEn;             // 逆变开环使能
    uint16_t LLCOpenLoopEn;          // LLC
    uint16_t SysTimerCount;          // 定时器计数值
    uint16_t BATPackNum;             // 电池包数量
    uint16_t CountryID;              // 并网认证国家
    uint16_t AuthenFlag;             // 认证信息下发标志
    uint16_t ParallelEn;             // 离网并机使能

    float InvRateVolt;               // 逆变额定电压
    float InvRateCurr;               // 逆变额定电流
    float InvRateFreq;               // 逆变额定频率值
    float InvRateFreqLast;

    float GridEnhanceIStep;          // 电网增强模式下电流变化率A/s
    float GridChgMaxCurrSet;         // 电网允许充电电流
    float GridDischgMaxCurrSet;      // 电网允许放电电流

//    float SinglePVMaxPowerSet;       // BMS允许单路 PV 最大功率
    float SinglePVMaxPowerLimit;     // 设置允许单路 PV 最大功率
    float PVRatedPower;              // 单路 PV 额定功率
    float PVRatedCurr;               // PV 额定最大电流限值,系统决定3KW/110V
    uint16_t MicroInverterMode;      // 微逆模式
    uint16_t DieselEngineMode;       // 柴油机模式
//    uint16_t PCSBrustMode;           // 逆变器降损模式
    uint16_t ByPassDisable;          // 关闭旁路
    uint16_t DynamicStageCnt;        //动态过程计数
}SYSTEM_PARA;

extern SYSTEM_PARA SystemPara;

void InitSystemControlData(void);
void ControlFanSpeed(uint16_t speed);
void InitMicroInverterPara(void);

#endif /* APP_INC_CONFIGSYSTEMPARA_H_ */

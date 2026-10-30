/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: OffGridInv2.h
Author: tzz
Version: 0.0.2
Date: 2022年2月15日
Description: 申明函数、控制结构体变量类型定义
*************************************************/
#ifndef ALGORITHM_INC_OFFGRIDINV2_H_
#define ALGORITHM_INC_OFFGRIDINV2_H_

#include "RepCtrl.h"
#include "CNTL_PI_F.h"
#include "rmp.h"
#include "sogi.h"
#include "Park.h"
#include "nPnZ_Cntl.h"

// 离网控制变量
typedef struct
{
    uint16_t  Id;          // 相序号1(R相)/2(S相)
    int16_t  PWMONflagCnt;
    int16_t  PWMDuty;      // PWM占空比
    float Period;        // PWM限制周期值
    float CAMP;

    uint16_t ReEnable;     // 重复控制使能
    uint16_t FwStartOver;    // 软起

    float lastSin;           // 软起
    float Pwmload;           // 软起

    float FwCnt;           // 前馈时间
    float LastFreq;        // 上一次的电网频率
    float BusVolt;         // 母线电压
    float KBus;            // 母线电压
    float RcdLimit;        // rcd电感限值

    float Fw;              // 前馈值
    float FwCoeff;         // 前馈系数

    float OutMax;          // 输出最大[0.99]
    float OutMin;          // 输出最小[-0.99]
    float Out;             // 输出
    float Out0;            // 输出
    float Out1;             // 输出
    float Out2;             // 输出
    float RefTarget;       // 参考目标值
    float KpSet;           // Kp 设置值

    float Rc;              //
    float Io;              // 输出电流
    float IL;              // 电感电流
    float IL2;             // 电感电流

    RMPCNTL FwRamp;        // 电压前馈缓起,用于软启动控制
    RMPCNTL RefRamp;       // 电压给定缓起控制
    SPLL_1ph_SOGI_F Sogi;  // 正交分解
    PARK_STRUCT Park;      // Park变换
//    PARK_STRUCT iPark;     // Park反变换
    CNTL_PI_F PICtrl;      // PI控制器数据结构
    REPEAT_CTRL_F RepCtrl; // 重复控制数据结构
    CNTL_PI_F BusErrCtrl;  // 母线

//    CNTL_PI_F LowPoWCtrl;  // 低功耗
//    float  LowPoWKrms;     // 低功耗
//    float  sin3;         // 低功耗
//    float  cos3;         // 低功耗

    CNTL_2P2Z_F_VARS      LowPoWVars;     //pr
    CNTL_2P2Z_F_COEFFS    LowPoWCoeffs;
    CNTL_2P2Z_F_VARS      PRVars;     //pr
    CNTL_2P2Z_F_COEFFS    PRCoeffs;
    CNTL_2P2Z_F_VARS      R3Vars;     //pr
    CNTL_2P2Z_F_COEFFS    R3Coeffs;
    CNTL_2P2Z_F_VARS      R5Vars;     //pr
    CNTL_2P2Z_F_COEFFS    R5Coeffs;
    CNTL_2P2Z_F_VARS      R7Vars;     //pr
    CNTL_2P2Z_F_COEFFS    R7Coeffs;
}OFF_GRID_CONTROL_COEFF;

extern OFF_GRID_CONTROL_COEFF UrCtrl, UsCtrl;

void InitOffGridInvControlData(void);
void ClearOffGridRepeatData(uint16_t Cnt);
void ClearOffGridPidData(void);
void SetOffGridInitVolt(float initVolt);
void ControlOffGridInv(void);
void InvPwmReload(void);
void SetOffGridStepValueLow(void);
void SetOffGridStepValueHigh(void);
void OffGridPara(void);
void OffGridVolt(void);
#endif /* ALGORITHM_INC_OFFGRIDINV2_H_ */

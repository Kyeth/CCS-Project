/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: OnGridInv2.h
Author: tzz
Version: 0.0.2
Date: 2022年3月1日
Description: 定义并网控制结数据构体类型
*************************************************/
#ifndef _ONGRIDINV2_H_
#define _ONGRIDINV2_H_

#include "RepCtrl.h"
#include "CNTL_PI_F.h"
#include "nPnZ_Cntl.h"
#include "rmp.h"
#include "sogi.h"

// 并网控制系数
typedef struct
{
    int16_t  Id;           // 相序号1(R相)/2(S相)
    uint16_t Period;       // PWM限制周期值
    int16_t PWMDuty;       // PWM占空比
    uint16_t ReEnable;     // 重复控制使能
    int16_t PWMOn;         // PWM占空比

    float BusVolt;         // 母线电压
    float SinTheta;        // 角度正弦

    float Vgrid;              // 前馈值
    float Fw;              // 前馈值
    float FwCoeff;         // 前馈系数

    float Out;             // 输出
    float Out1;             // 输出
    float OutOVcnt;
    float OutLimit;

    float IKpSet;          // 电流环 Kp 设定值
    float IKrSet;          // 电流环 PR 设定值

    float IdcSum;          // 直流分量累加值
    float IdcCurr;         // 直流分量当前值
    float DCIErr;
    float DCIErrOld;
    float DCIAdjOut;
    float DCIAdj;
    float ZeroPoint;       // 过零
    float gain;

    RMPCNTL RmpCtrl;       // 给定缓起控制
    CNTL_PI_F PICtrl;      // PI控制器
    REPEAT_CTRL_F RepCtrl; // 重复控制系数
    Filter_STRUCT Vgff;    // 电网电压滤波器
    float LastFreq;        // 上一次的电网频率

    CNTL_2P2Z_F_VARS    PRVars;      //pr
    CNTL_2P2Z_F_COEFFS  PRCoeffs;
    CNTL_2P2Z_F_VARS    PR3Vars;     //3pr
    CNTL_2P2Z_F_COEFFS  PR3Coeffs;
    CNTL_2P2Z_F_VARS    PR5Vars;     //7pr
    CNTL_2P2Z_F_COEFFS  PR5Coeffs;
    CNTL_2P2Z_F_VARS    PR7Vars;     //7pr
    CNTL_2P2Z_F_COEFFS  PR7Coeffs;

    float Theta;
    float ThetaSpll;
    float AFDTheta;
    SPLL_1ph_SOGI_F Usogi;
    float ThetaSpllLast;
    float DeltaTheta;
    float Sin;
    float FreqCal;
//    float ThetaTmp;
}ON_GRID_CONTROL_COEFF;

extern ON_GRID_CONTROL_COEFF IrCtrl, IsCtrl;

// 并网电流参考
typedef struct
{
    int16_t  ThetaPoint; // 相当前角度点位置号,用于重复控制
    float P;
    float Q;
    float S;
    float Sin;
    float IRef;
    RMPCNTL DeltaTheta;
    RMPCNTL QRef;
}I_REAL_REF;
extern I_REAL_REF RealRefR,RealRefS;

void InitOnGridInvControlData(void);
void CleanOnGridCtrlData(uint16_t cnt);
void CleanRPhaseOnGridCtrlData(uint16_t cnt);
void CleanSPhaseOnGridCtrlData(uint16_t cnt);
void ClearOnGridPidDataR(void);
void ClearOnGridPidDataS(void);
void ClearOnGridSinglePidData(uint16_t ch);
int ControlOnGridInvS(uint16_t enFlg);
int ControlOnGridInvR(uint16_t enFlg);
void InvPwmReloadOnGrid(void);
#endif /* ALGORITHM_INC_ONGRIDINV2_H_ */

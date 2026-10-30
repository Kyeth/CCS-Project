#ifndef __TASKCLA_H
#define __TASKCLA_H

#include "cla.h"

#define LLCSYSTEMCLOCKHZ            120000L
#define CHARGESOFTPRD               857
#define DISCHARGESOFTPRD            600

#define SRDAB_IDLE                   (0)
#define SRDAB_CHARGE                 (1)
#define SRDAB_DIS_CHARGE             (2)
#define SRDAB_DISCHG_SOFT_START      (3)
#define SRDAB_ADJUST_DUTY            (4)
#define SRDAB_CHG_SOFT_START         (5)
#define SRDAB_RELAY_CHK              (6)
#define SRDAB_LLC_MODE               (7)
#define SRDAB_ONE_BOOST              (8)
#define SRDAB_TWO_BOOST              (9)
#define AGING_MODE                   (10)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct {
    float  Ref;             // Input: reference set-point
    float  Fbk;             // Input: feedback
    float  Out;             // Output: controller output
    float  Kp;              // Parameter: proportional loop gain
    float  Ki;              // Parameter: integral gain
    float  Umax;            // Parameter: upper saturation limit
    float  Umin;            // Parameter: lower saturation limit
    float  up;              // Data: proportional term
    float  ui;              // Data: integral term
    float  v1;              // Data: pre-saturated controller output
    float  i1;              // Data: integrator storage: ui(k-1)
    float  ui_temp;
    float  Err;
    float  out_ui;
    float  ErrLimit;
    unsigned short  RealseEn;
} CNTL_PI_F;

typedef struct
{
    float Target;
    float Current;
    float Delta;
    float Last;
    float Step;
    float Out;
    float Sign;
}LIMSTEP;

typedef struct
{
    float TargetValue;    // Input: Target input
    float StepValue;      // Input: one step
    float RampLowLimit;   // Parameter: Minimum limit
    float RampHighLimit;  // Parameter: Maximum limit
    float SetpointValue;  // Output: Target output
}RMPCNTL;

typedef struct
{
    LIMSTEP ChargeStep;
    LIMSTEP DisChargeStep;
    CNTL_PI_F VoltSoftPI;              // 缓启动PI控制器
    CNTL_PI_F DisVoltPICtrl,VoltPICtrl,VoltLLCPICtrl;           // 电压环PI控制器数据
    CNTL_PI_F DisCurrPICtrl,CurrPICtrl,CurrLLCPICtrl,PowerCurrPICtrl,PowerDisCurrPICtrl,PowerCurrLLCPICtrl;           // 电流环PI控制器数据
    CNTL_PI_F PowerPICtrl;                        // 功率环PI控制器数据
    CNTL_PI_F VoltPowerPICtrl;
    CNTL_PI_F DisChargeAgingICtrl,ChargeAgingICtrl;
    RMPCNTL ClaRefSoftFreq;
    RMPCNTL ClaZeroSoftFreq;
    RMPCNTL ClaRefFreq;                             // 频率给定斜坡
    RMPCNTL ClaRefFs0;                              // fs0斜坡
    RMPCNTL ClaTestDuty;
    RMPCNTL ClaRoute1Duty;
    RMPCNTL ClaRoute2Duty;
    RMPCNTL AgingStep;
    float LateCtrlData;                             // 占空比前馈
    float fClaDuty;                                 // 给定占空比
    float fClaFreq;                                 // 给定频率
    float fClaFreqSet;                              // 实际频率
    float fPwmFlag;                                 // 打嗝标志
    float fPeriod;                                  // 给定的PRD值
    float fMaxDuty;                                 // 给定最大的占空比
    float fPhase;                                   // 相位
    float fPhaseSet;                                // 实际相位
    short i16Interval;                              // 0:空载无前馈      1:加载前馈
    short LoopCnt;                                  // 环路工作计数
    short SetCnt;                                   // 打嗝发波计数
    short BuckSet;                                  // 变比降压模式启动设置
    short BoostSet;                                 // 变比稳压模式启动设置
    float fSoftPrd;                                 // 切换发波

    float G;
    float Gdiv;
    float Kr;
    float Krdiv;
    float SinKr;
    float PhsOffset;
    float CPhsOffset;
    float DPhsOffset;
    float T0;
    float fs0;
    float fs1;
    float fd1;
    float A;
    float B;
    float C;
    float D;
    float Ro;
    float Tperiod;
    float CurrKp;
    float CurrKi;
    short LoopStart;
    float fslate;
    float fsdelta;
    float fsFreq;
    float dfreq;
    float Compensate;//补偿量
    float fDeadTime;
    float fCurrRef; //电流给定
    float PIOutStep;
    float PIOutDelta;
    float PIOutCurrent;
    float PIOutLast;
}CTRLPARA;

typedef struct
{
    float Vout;
    float Vbat;
    float IBus;
    float Iout;
    float Iref;
    float SetCurr;
    float IErr;
    float IErrF;
    float Imax;
    float Vref;
    float PBus;
    float DisChargeSoft_Volt;
    float BaseFreq;
    float ChargeMaxFreq;
    float MaxFreq;
    float TargetAvgCurr;            // 目标平均电流
    float AgingISetRef;
    float AgingISet;
    short AgingIDir;
    unsigned short ParallelMode;    // 并机模式
    unsigned short ParalelCtrlState;  // 并机时设备模式  1:稳压  2:跟随
}CPUADCSAM;

typedef struct
{
    unsigned short Mode;
    unsigned short ReSet;
}CPUSTATE;

typedef struct
{
    float Prd;
    float Phase;
    float PrimaryPhase;
    float BranchPhase;
    float PrimaryCMPA;
    float PrimaryCMPB;
    float PrimaryCMPC;
    float PrimaryDuty;
    float BranchCMPA;
    float BranchCMPB;
    float BranchCMPC;
    float BranchCMPD;
    float BranchDuty;
    float ClaCnt;
    float BranchDeadRed;
    float BranchDeadFed;
    float PrimaryDeadRed;
    float PrimaryDeadFed;
    float DeadTime;
}ST_CLA_OUTPUT;

extern CTRLPARA CtrlPara;
extern float ClaFreq;
extern CPUADCSAM CpuAdcSample;
extern CPUSTATE CpuState;
extern ST_CLA_OUTPUT stClaOutputParam;                     // CLA的运算输出结果
extern volatile unsigned int ClaCnt;
extern __interrupt void TaskCla_1(void);

void RunSoftClosedLoop(void);
void RunDutyChargeLoop(void);
void RunChargeClosedLoop(void);
void RunDischargeClosedLoop(void);
void DisChargeLLCLoop(void);
void RouteOneVolt(void);
void RouteTwoVolt(void);
void RunPowerClosedLoop(void);
void AgingLLCLoop(void);
void CNTL_PI_F_FUNC(CNTL_PI_F *v);
float RmpCntlCalc(RMPCNTL *v);
float LIMITSTEP_FUNC(LIMSTEP *v);
extern void CalcSoftStartVolt(void);
extern void ChargeSoftStart(void);
extern void DisChargeSoftStart(void);
extern void DisturbRelayVolt(void);

#ifdef __cplusplus
    }
#endif

#endif

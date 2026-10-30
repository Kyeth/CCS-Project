#include "F28003x_device.h"
#include "TaskCla.h"
#include "PlatformManager.h"
#include "LowPowerMode.h"
#include "SysStateMachine.h"
#include "string.h"
#include "ParallelProcess.h"
#include "EquipmentParallelCore.h"

#ifndef USER_LOOPCTRL_INC_LLCCONTROL_H_
#define USER_LOOPCTRL_INC_LLCCONTROL_H_

#define LLCSYSTEMCLOCK          120000000L
#define MAXFREQ                 300000L
#define MINFREQ                 60000L
#define UPDNLMT(Var,Max,Min)    {(Var)=((Var)>=(Max))?(Max):(Var);(Var)=((Var)<=(Min))?(Min):(Var);}
#define UPLMT(Var,Max)          {(Var)=((Var)>=(Max))?(Max):(Var);}
#define DNLMT(Var,Min)          {(Var)=((Var)<=(Min))?(Min):(Var);}
#define MAXDUTY                 43
#define MINDUTY                 0
#define MAXPRD                  (LLCSYSTEMCLOCK/MINFREQ/2)
#define MINPRD                  (LLCSYSTEMCLOCK/MAXFREQ/2)
#define MAXPHS                  180
#define MINPHS                  0
#define LINE_K1                 -0.2855f
#define LINE_B1                 8.21f
#define LINE_K2                 0.056458333f
#define LINE_B2                 0.003f

#define TRANSFORMER_TURNS_RATIO 15.0f

typedef enum
{
    STATE_STEADY,
    STATE_TRANSIENT_FOLLOW,
    STATE_FORCED_BOOST,
    STATE_WAIT_FOR_RECOVERY
}TARGET_STATE;


typedef struct
{
    float TargetValue;    // Input: Target input
    float StepValue;      // Input: one step
    float RampLowLimit;   // Parameter: Minimum limit
    float RampHighLimit;  // Parameter: Maximum limit
    float SetpointValue;  // Output: Target output
}RAMP;


typedef struct
{
    Uint16 FinishFlag;
    Uint16 Flag1;
    Uint16 CheckStep;
    Uint16 CheckCnt;
    Uint16 CheckIbranch1Cnt;
    Uint16 CheckIbranch2Cnt;
}RELAYDATA;

typedef enum
{
    DROOP_IDLE,
    DROOP_DISCHARGE,
    DROOP_CHARGE,
}DROOPDATA;

typedef struct
{
    RAMP VrefRamp;
    RAMP PowerRamp;
    RAMP AgingRamp;
    TARGET_STATE TargetState;   // 稳压状态机
    RELAYDATA RelayData;
    DROOPDATA DroopState;
    Uint16 SoftDir;             // 软启动模式：1：充电缓启动 2：放电缓启动 0：无
    Uint16 SoftFinish;          // 软启动完成标志；

    Uint16 StandDly;            // 继电器稳定计数
    Uint16 ClearVoltFlag;       // 清空电压标志
    Uint16 VoltFlag;

    Uint16 DeadBranch;          // 副边死区
    Uint16 DeadPrimary;         // 原边死区

    Uint32 DeadBranchRise;          // 副边死区
    Uint32 DeadBranchFull;          // 副边死区
    Uint32 DeadPrimaryRise;         // 原边死区
    Uint32 DeadPrimaryFull;         // 原边死区

    Uint16 AgingFlag;           // 老化标志
    Uint16 TrigAging;           // 切换触发
    Uint16 AgingCnt;            // 老化工作计时器

    float MaxDischargeCurr;     // 最大放电电流
    float MaxChargeCurr;        // 最大充电电流

    float Iset;                 // 用于设定原边最大允许电流（充电模式调试阶段用）
    float VBat_Set;             // 用于设定电池目标电压（充电模式调试阶段用）
    int16 AgingIDirCom;         // 用于老化通信方向
    float AgingISetCom;        // 用于老化模式通信设定目标电流
    float AgingISet;            // 用于老化模式设定目标电流
    float AgingISetLast;        // 用于存放老化设定电流上次值

    float Ratio;                // 变比（指此时高压侧对低压侧电压的比值）
    float CliffVolt1;           // 触发打嗝的电压差（断崖电压）
    float CliffVolt2;           // 触发充放切换的电压差（断崖电压）

    float BaseVref;             // 稳压电压目标
    float Vref;                 // 稳压电压目标
    float PreVref;              // 斜坡电压目标
    float VrefSet;              // 稳压电压目标外部设置
    Uint16 Duty;                // 缓启动占空比设置
    float Freq;                 // 工作频率
    Uint32 Prd;                 // 工作频率对应的时基周期

    Uint32 ChargeMaxPrd;         // 充电闭环最大设定时基
    float BaseFreq;             // 基准频率

    float C2DVoltGap;           // 充转放差值
    float D2CVoltGap;           // 放转充差值
    float CBackVoltGap;         // 充电回退点
    float DBackVoltGap;         // 放电回退点

    Uint16 ChargeCnt;
    Uint16 DisChargeCnt;
    float CurrTrigSet;
    Uint16 HighCount;

    int16 PowerTotal;                           // 总功率
    float fPowerRef;                            // 包功率实际
    float fPowerTar;                            // 包功率目标
    int16 LastPackPower;                        // 上次设定的功率
    Uint16 u16PackPower;                        // 包的功率
    Uint16 u16DischargeCurrentAllowMax;         // 电池侧最大放电电流，已放大10倍，即100 == 10A电流
    Uint16 u16ChargeCurrentAllowMax;            // 电池侧最大充电电流，已放大10倍
    Uint16 u16DischargeCoefficient;             // BMS设定的放电系数，已放大10倍
    Uint16 u16ChargeCoefficient;                // BMS设定的充电系数，已放大10倍
    Uint16 u16AllowCharge;                      // 充电模式允许
    Uint16 u16AllowDischarge;                   // 放电模式允许

    Uint16 StandByFlag;
    Uint16 CanTStable;                          // 无法稳压
    Uint16 StateDir;                            // 锁存状态方向
    Uint16 DisChargeFirst;                      // 放电过态
    Uint16 ChargeFirst;                         // 充电过渡
    Uint16 DisChargeState;                      // 放电模式
    Uint16 ChargeState;                         // 充电模式
    Uint16 MasterFormCnt;                       // 主机检测
    Uint16 SetState;                            // 受控状态
    Uint16 StatePrompt;                         // 指示状态 1：充电状态 2：放电状态
    Uint16 SlaveStateChange;                    // 跟随包切换工作状态
    Uint16 SoftBoostFlag;                       // 放电软启动升压标志
    Uint16 RelayFlag;                           // 主继电器完成闭合标志

    Uint32 u32PreChargeTimeOutCnt;              // 预充超时计数
    Uint32 u32PowerAbsorptionTimeOutCnt;        // 抽电超时计数

    Uint16 TestCnt;
    Uint16 TestFlag;
    Uint16 TestDir;
    Uint16 TestSetTime;
    Uint16 Temp;

    Uint16 FanTestEnable;
    Uint16 FanTestGear;
    Uint16 FanTestCmd;

    float VDROP_ENTRY1;
    float VDROP_ENTRY2;
    float VDROP_EXIT1;
    float VDROP_EXIT2;
    float fDelta;
    float fDroopPower;
    float Kdroop1;
    float Kdroop2;

}LLC_SYSTEM_CONTROL_PARA;

extern LLC_SYSTEM_CONTROL_PARA LLCSystemCtrl;


struct FREQ_MPPT
{
    Uint16 counter10;
    float PowerPv1_sum;
    float Vout;
    float Vbat;
    float Power;
    float DCP2_mppt1_old;
    float mppt1dir;
    float Freq;
    float DC_Vref_1;
    float mpptstep;

};
extern struct FREQ_MPPT FreqMppt;

extern RMPCNTL RefDuty;
extern RMPCNTL RefRamp;

void RunMpptFreq(struct FREQ_MPPT *Private);
void InitTestData(void);
void ResetTestData(void);
void TestCtrlFunc(void);
void SetPWMSelect(void);
void RunTestLoop(void);
void InitLoopData(void);
void ResetChargeData(void);
void RestDisChargeData(void);
void LoopCtrl(void);
void RunChargeClosedLoop(void);
void RunDischargeClosedLoop(void);
void FreezePWMOutput(void);
float RmpCntlCal(RAMP *v);
void EnablePWMOutput123(void);
void FreezePWM2Output(void);
void EnablePWMOutput(void);
void EnablePWM1Output(void);
void EnablePWM2Output(void);
void SoftStartModificate(void);
void WorkModificate(void);
void ChargeModificate(void);
void DisChargeModificate(void);
void SetDutyPWM(float Duty, float Prd, float Phase);
void SetChargeDutyPWM(float Duty, float Prd, float Phase);
void SetChargePWM(float Freq);
void SetCharge1PWM(float Freq);
void SetCharge2PWM(float Freq);
void SetDisChargePWM(float Freq,float Phase);
void SetDisCharge1PWM(float Freq,float Phase);
void SetDisCharge2PWM(float Freq,float Phase);
void EnablePWMOutput1_4(void);
void EnablePWMOutput2_5(void);
void EnablePWMOutput1(void);
void EnablePWMOutput5(void);
void CheckRef(void);
float GetSetCurr(float Vbat);
void MasterSlaveCheckModule(ST_BATTERY_HANDLE *pstBatteryHandle);

extern void CheckMaxAllowCurrent(void);

static inline Uint16 GetAllowDischargeCurrent(LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl)
{
    return pstLLCSystemCtrl->u16DischargeCurrentAllowMax;
}

static inline Uint16 GetAllowChargeCurrent(LLC_SYSTEM_CONTROL_PARA *pstLLCSystemCtrl)
{
    return pstLLCSystemCtrl->u16ChargeCurrentAllowMax;
}

#endif /* USER_LOOPCTRL_INC_LLCCONTROL_H_ */

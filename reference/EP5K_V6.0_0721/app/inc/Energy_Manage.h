/*
 * Energy_Manage.h
 *
 *  Created on: 2022年5月9日
 *      Author: DLadmin
 */

#ifndef ENERGY_MANAGE_H_
#define ENERGY_MANAGE_H_

#include"InvStateMachine.h"

#define EMDataNum 69

typedef enum
{
    ES_INIT,        //禁止
    ES_DELAY,       //延时
    ES_RAMP,        //斜坡
    ES_FINISH,      //完成
}ES_STATUS_TYPE;

typedef union{
    unsigned int   all;
    struct {
        unsigned int OV:1;
        unsigned int UV:1;
        unsigned int OF:1;
        unsigned int UF:1;
        unsigned int ESdelay:1;
        unsigned int Conncected:1;
        unsigned int bit6:1;
        unsigned int bit7:1;
        unsigned int bit8:1;
        unsigned int bit9:1;
        unsigned int bit10:1;
        unsigned int bit11:1;
        unsigned int bit12:1;
        unsigned int bit13:1;
        unsigned int bit14:1;
        unsigned int bit15:1;
    }bit;
}FAULTMSG;

typedef union{
    unsigned int   all;
    struct {
        unsigned int OV2:1;
        unsigned int OV1:1;
        unsigned int UV1:1;
        unsigned int UV2:1;
        unsigned int OF2:1;
        unsigned int OF1:1;
        unsigned int UF1:1;
        unsigned int UF2:1;
        unsigned int Connect:1;
        unsigned int Recover:1;
        unsigned int :1;
        unsigned int :1;
        unsigned int :1;
        unsigned int :1;
        unsigned int :1;
        unsigned int :1;
    }bit;
}FAULTCHECK_ENABLE;

typedef struct
{
    FAULTMSG  FaultMsg;  //状态
    FAULTCHECK_ENABLE Enable;
    float OF2;// 二级过频保护点  ，默认62Hz ，可设61.8~66Hz
    float OF1;//一级过频保护点  ，默认61.2Hz， 可设61~66Hz
    float UF1;//一级欠频保护点，  默认58.5Hz，可设50~59Hz
    float UF2;//二级欠频保护点，  默认56.5Hz，可设50~57Hz
    float OF2LossTime;//二级过频保护时间，默认0.16s，可设0.16~1000s
    float OF1LossTime;//一级过频保护时间，默认300s ，可设180~1000s
    float UF1LossTime;//一级欠频保护时间，默认300s ，可设180~1000s
    float UF2LossTime;//二级欠频保护时间，默认0.16s，  可设0.16~1000s
    float OV2;//二级过压保护点  ，默认1.2  ，可设
    float OV1;//一级过压保护点  ，默认1.1 , 可设 1.1~1.2
    float UV1;//一级欠压保护点，  默认0.88，可设0~0.88
    float UV2;//二级欠压保护点，  默认0.58，可设0~0.5
    float OV2LossTime;//二级过压保护时间，默认0.16s，可设
    float OV1LossTime;//一级过压保护时间，默认13s，可设1~13s
    float UV1LossTime;//一级欠压保护时间，默认21s，可设2~50s
    float UV2LossTime;//二级欠压保护时间，默认2s，可设0.16~21s
    float UVConnect;//允许并网的最低电压，默认0.917，可设0.88~0.95
    float OVConnect;//允许并网的最高电压，默认1.05 ，可设1.05~1.06
    float UFConnect;//允许并网的最低频率，默认59.5 Hz，可设59~59.9Hz
    float OFConnect;//允许并网的最高频率，默认60.1Hz，可设60.1~61Hz
    float ConnectTime;//2s,并网时间
    float V;  //电网电压
    float f;  //电网频率
}GRIDCHECK_STRUCT;
extern GRIDCHECK_STRUCT GridCheck;

typedef struct
{
    float P;//输出
    float P_SET;//目标功率
    unsigned int ES_PERMIT_SERVICE_EN;
    float ES_UV;
    float ES_OV;
    float ES_UF;
    float ES_OF;
    unsigned long ES_RANDOMIZED_DELAY;
    unsigned long ES_DELAY;
    unsigned long ES_RAMP_RATE;
    unsigned long ES_DELAY_CNT;
    unsigned long ES_CNT;
    char ES_EN;
    ES_STATUS_TYPE ES_STATUS;
    float SlopStep;//爬坡步长
}ESMODE_STRUCT;
extern ESMODE_STRUCT ESmode;

typedef struct
{
    float Q1;
    float Q2;
}CQMODE_STRUCT;
extern CQMODE_STRUCT CQmode;


typedef struct
{
    float P;
}CPMODE_STRUCT;
extern CPMODE_STRUCT CPmode;


typedef struct
{
    float STemperature;//视在功率温度限值
    float PCommand;//限功率指令
    float Smax;//最大视在功率
    float Pmax;//最大有功功率
    float Qmax;//最大有功功率
}PLIMIT_STRUCT;
extern PLIMIT_STRUCT Plimit;

typedef struct
{
    float P1;//模块的输入功率
    float P2;//默认0.2 activePowerRated和activePowerMin中大者
    float V1;//P1 相对应电压  1.06Vn ,可设（1.05~1.09 Vn）
    float V2;//P2 相对应电压  1.1Vn ,可设（V1+0.01 Vn~1.1Vn）
    float K12;//V1V2的斜率
    float P;//输出
    float P1out;//输出
    float P2out;//输出
    char cnt1;
    char cnt2;
    float V1_Avg_Pre;
    float V2_Avg_Pre;
    float V;//输入
    float Tr;   //开环响应时间
}PUMODE_STRUCT;
extern PUMODE_STRUCT PUmode;

typedef struct
{
    float Vref;//中心参考电压
    float V1;//Q1 相对应电压 ，Vref-0.08Vn
    float V2;//Q2 相对应电压 ，Vref-0.02Vn
    float V3;//Q3 相对应电压 ，Vref+0.02Vn
    float V4;//Q4 相对应电压 ，Vref+0.08Vn
    float V5;//
    float V6;//
    float V1_delta;//V1 - Vref
    float V2_delta;//V2 - Vref
    float V3_delta;//V3 - Vref
    float V4_delta;//V4 - Vref
    float V_delta;//V - Vref
    float Q1;//最大过激无功设定，默认44%视在功率，可设 0~1可用无功
    float Q2;//在下限死区电压的无功设定，默认0
    float Q3;//在上限死区电压的无功设定，默认0
    float Q4;//最大欠激无功设定，默认44%视在功率，可设 0~1可用无功
    float K12;//Q1、Q2间的斜率
    float K23;//Q2、Q3间的斜率
    float K34;//Q3、Q4间的斜率
    float Lpf_Tc;//低通滤波时间常数,1000,可设300 s 至 5000 秒
    float Lpf_Ts;//低通滤波采样时间
    float Lpf_A0;//低通滤波系数
    float Lpf_A1;//低通滤波系数
    float Tr;   //开环响应时间
    float V;    //输入电压
    float Q;    //输出
    float V1_Pre;
    float V2_Pre;
    char cnt1;
    char cnt2;
    float Q1_out;//上次无功输出
    float Q2_out;//上次无功输出
    float limitQ1;
    float limitQ2;
    float limitP1;
    float limitP2;
    unsigned int Vref_AdjEn;        //vref 自動調節使能
    unsigned int Q1_MsCnt;
    unsigned int Q2_MsCnt;

}QUMODE_STRUCT;
extern QUMODE_STRUCT QUmode;

typedef struct
{
    float P3_P;//有功功率点3，  默认1 额定有功功率
    float P2_P;//有功功率点2，  默认0.5 额定有功功率
    float P1_P;//有功功率点1，  默认0.2 activePowerRated和activePowerMin中大者
    float Q1_P;//无功响应点1，默认0，可设 0~1 可用无功功率
    float Q2_P;//无功响应点2，默认0，可设 0~1 可用无功功率
    float Q3_P;//无功响应点3，默认44%视在功率，可设 0~1可用无功功率
    float K01_P;//无功响应区间01折线斜率
    float K12_P;//无功响应区间12折线斜率
    float K23_P;//无功响应区间23折线斜率
    float P3_N;//有功功率点3，  默认1 额定有功功率
    float P2_N;//有功功率点2，  默认0.5 额定有功功率
    float P1_N;//有功功率点1，  默认0.2 activePowerRated和activePowerMin中大者
    float Q1_N;//无功响应点1，默认0，可设 0~1 可用无功功率
    float Q2_N;//无功响应点2，默认0，可设 0~1 可用无功功率
    float Q3_N;//无功响应点3，默认44%视在功率，可设 0~1可用无功功率
    float K01_N;//无功响应区间01折线斜率
    float K12_N;//无功响应区间12折线斜率
    float K23_N;//无功响应区间23折线斜率
    float P ;//输入
    float Q ;//输出
}QPMODE_STRUCT;
extern QPMODE_STRUCT QPmode;

typedef struct
{
    float OFDeadband;//死区，有功功率不响应频率变化，0.036Hz，可设0.017~1
    float UFDeadband;//死区，有功功率不响应频率变化，0.036Hz，可设0.017~1
    float kOF_F;//斜率，表示单位频率引起的单位有功功率，0.05,可设0.02~0.05
    float kUF_F;//斜率，表示单位频率引起的单位有功功率，0.05,可设0.02~0.05
    float kOF_P;//斜率，表示单位功率引起的单位频率，
    float kUF_P;//斜率，表示单位频率引起的单位频率，
    float P1pre;//功率基准值
    float P2pre;//功率基准值
    float F1pre;
    float F2pre;
    float Fdelta;//频差
    float Pref;
    float FH;//最大输入频率
    float FL;//最小输入频率
    float F;//输入
    float P;//功率输出值
    float P1out;
    float P2out;
    char cnt1;
    char cnt2;

    float Tr;
}PFMODE_STRUCT;
extern PFMODE_STRUCT PFmode;

typedef struct
{
    float Cosphi;//指令功率因数
    float PFmin_inj;//最小注入功率因数，对于 A 类和 B 类设备均为 0.90
    float PFmin_ab; //最小吸收功率因数，对于 A 类为 0.97，对于 B 类为 0.90
    unsigned int Cosphi_exc;
    float P;//有功功率输入
    float Q;//无功功率输出
}COSPHIMODE_STRUCT;
extern COSPHIMODE_STRUCT Cosphimode;

typedef union{
    unsigned int   all;
    struct {
        unsigned int CosphiMode:1;
        unsigned int QUmode:1;
        unsigned int QPmode:1;
        unsigned int CQmode:1;
        unsigned int PUmode:1;
        unsigned int ActivePowerLimit:1;
        unsigned int PFmode:1;
        unsigned int CPmode:1;
        unsigned int reserve:1;
        unsigned int PermitServ:1;
        unsigned int VrefAdj:1;
        unsigned int bit11:1;
        unsigned int bit12:1;
        unsigned int bit13:1;
        unsigned int bit14:1;
        unsigned int bit15:1;
    }bit;
}POWERMODE_EN;

typedef struct
{
    POWERMODE_EN Enable;//上位机使能控制
    float P1Command;//上位机有功指令
    float P2Command;//上位机有功指令
    float Q1Command;//上位机有功指令
    float Q2Command;//上位机有功指令
    float PLimitCmd;//上位机限功率指令
    float Srated;//额定视在功率
    float Prated;//额定有功功率
    float Vrated;//额定电压
    float Frated;//额定频率
    float V1;//L1当前电压
    float V2;//L2当前电压
    float f1;//当前频率
    float f2;//当前频率
    float P1;//当前有功
    float P2;//当前有功
    float Q1;//当前无功
    float Q2;//当前无功
    float P_Tr;//有功响应时间
    float Q_Tr;//无功响应时间
    float P1out;//当前有功给定
    float P2out;//当前有功给定
    float Q1out;//当前无功给定
    float Q2out;//当前无功给定
    float IslandQR;//当前无功给定
    float IslandQS;//当前无功给定
	float Temp1;//当前无功给定
	float Temp2;//当前无功给定
	float Temp3;//当前无功给定

}ENERGYMANAGE_STRUCT;
extern ENERGYMANAGE_STRUCT EnergyManage;


struct  Alarm_State
{
    unsigned int OVER_CURRENT:1;     // 0 逆变器过流
    unsigned int OVER_VOLTAGE:1;      // 1 电网过压
    unsigned int UNDER_VOLTAGE:1;     // 2 电网欠压
    unsigned int OVER_FREQUENCY:1;     // 3 电网过频

    unsigned int UNDER_FREQUENCY:1;     // 4电网欠频
    unsigned int VOLTAGE_IMBALANCE:1;   // 5
    unsigned int CURRENT_IMBALANCE:1;   // 6
    unsigned int EMERGENCY_LOCAL:1;     // 7

    unsigned int EMERGENCY_REMOTE:1;     // 8
    unsigned int LOW_POWER_INPUT:1;      // 9
    unsigned int PHASE_ROTATION:1;       // 10
    unsigned int Reserve :5;             // 11
};

union Alarm_REG
{
    unsigned int          all;
    struct Alarm_State    bit;
};
extern union Alarm_REG INV_Status;
extern INV_STATE_STRUCT InvStateData;

extern void InitEnergyManage(void);
extern void EsmodeENStatusChk(void);
void sESmodeinit(void);
extern void EMmodeENcheck(void);
extern char GetInvOnOffStation(void);
extern char GetInvConnectStation(void);
extern char GetInvStation(void);
extern char GetControlmode(void);
extern char GetInvStorageStation(void);


extern float SOC;
extern float Q1offset,Q2offset,P1offset,P2offset,PoffsetTMP;

void sEnergyManage(void);
void InitGridCheck(void);
void sGridCheck(void);

ES_STATUS_TYPE GetsESmodeStatus(void);

extern void User_EMData_Check(void);
extern void User_EME2ROMData_Read(void);
void EMdatainit(void);
void EMData_shadowinit(void);

#endif /* ENERGY_MANAGE_H_ */

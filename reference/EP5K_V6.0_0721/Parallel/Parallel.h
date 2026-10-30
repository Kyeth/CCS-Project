/*
 * Parallel.h
 *
 *  Created on: 2022年8月24日
 *      Author: DLadmin
 */

#include "F28x_Project.h"
#include "CanDrive.h"
#include "ParaCntl.h"
#include "nPnZ_Cntl.h"
#include "ParaProtocol.h"
//#include "Control.h"

#ifndef MODULE_PARALLEL_H_
#define MODULE_PARALLEL_H_

//define 通信时基
#define cTimer1ms               (1000)
#define cTimer5ms               (5*cTimer1ms)
#define cTimer20ms              (20*cTimer1ms)
#define cTimer500us             (0.5*cTimer1ms)
//#define cTimer200us             (0.2*cTimer1ms)

#define cParaMaxNum              0x02   //最大并联机器数
#define cParaRealNum             0x01   //当前并联机器数

#define cSingleMode              0x00
#define cParaMode                0x01

//define inverter's role for parallel
#define cMasterModule            0x01
#define cSlaveModule             0x02
#define cNewModule               0x03

typedef struct
{
    float a;
    float b;
    float c;
}Struct_Abc_F;

typedef struct
{
    long a;
    long b;
    long c;
}Struct_Abc_Int32;

typedef struct
{
    float* a;
    float* b;
    float* c;
}Struct_Abc_PT;

typedef struct
{
    int a;
    int b;
    int c;
}Struct_Abc_Int;

typedef union
{
    Uint16  Word;
    struct
    {
        Uint16 CAN_Sync:1;//载波同步
        Uint16 CAN_Spll:1;//相位同步
        Uint16 CAN_MasterInNet:1;      //收到主机信号

        Uint16 MasterInNetDetectOver:1;//检测主机
        Uint16 MasterCompetitionOver:1;//主从竞争
        Uint16 InquiryNewModule:1;     //查新模块
        Uint16 InquirySlaveModule:1;   //查从模块
        Uint16 MasterSendInfo:1;       //

        Uint16 CheckSystemStatus:1;
        Uint16 ClassifyCommand:1;
        Uint16 SystemSetSend:1;       //发送主机设置
        Uint16 InquirySynAction:1;    //请求同步
        Uint16 SendCrossZero:1;       //过零发送
        Uint16 ReceiveCrossZero:1;    //过零生效
        Uint16 CrossZeroUpdate:1;     //过零更新
        Uint16 rev3:1;//
    }Bits;
}PARA_CYCLE_UNION;


typedef union
{
    Uint32  Word;
    struct
    {
        Uint16 FwTurnOffComm:1;        //开始放开继电器
        Uint16 FwSynCloseRly:1;        //开始吸合继电器
        Uint16 WaitForTurnOff:1;       //并机继电器等待一起放开
        Uint16 WaitForCloseRly:1;      //并机继电器等待一起吸合

        Uint16 eParaSet:1;             //设置系统共同参数,电压、频率保护点等
        Uint16 eAverageOverLoad:1;     //平均过载
        Uint16 eModuleOverLoad:1;      //模块过载
        Uint16 eModuleFault:1;         //模块故障

        Uint16 ModuleNumFault:1;       //
        Uint16 MasterToBeSlave:1;      //主机转从机
        Uint16 SetParallelNum:1;       //设置并机台数
        Uint16 enParallel:1;           //并机使能

        Uint16 eClearParaCntl:1;
        Uint16 eTurnOffSystem:1;
        Uint16 eSendWorkState:1;
        Uint16 eStateReceive:1;

        //16~32
        Uint16 FwIslanded:1;           //一起离网
        Uint16 FwConnectGrid:1;        //一起并网
        Uint16 WaitForIslanded:1;      //一起离网
        Uint16 WaitForConnectGrid:1;   //一起并网

        Uint16 eStartAssignLocation:1;
        Uint16 MasterFault;
        Uint16 rev:10;                //
    }Bits;
}PARA_EVENT_UNION;

//for parallel status
typedef union
{
    Uint32  Word;
    struct
    {
        //1~8
        Uint16 ParaMode:2;          //并机模式
        Uint16 OfflineCheck:3;      //模块离线检测
        Uint16 TotalNumFault:1;     //台数错误，有模块未连接
        Uint16 AddressingOk:1;      //编址成功  1
        Uint16 CommCanFail:1;       //通信线故障
        //9~16
        Uint16 MasterExist:1;       //主机已连接  0
        Uint16 ParaRunning:1;       //主从模块之间已建立通信   1
        Uint16 PhaseLocked:1;       //锁相ok     1
        Uint16 PwmSyncOk:1;         //载波同步ok   1
        Uint16 PowerOnOk:1;         //本机上电ok   1
        Uint16 SystemRunOk:1;       //并机系统正常，允许开机
        Uint16 LoadRlyClosed:1;         //继电器状态
        Uint16 invPwmOn:1;          //启动逆变PWM
        //17~24
        Uint16 OffGridWorking:1;    //本机是否处于离网并联状态
        Uint16 LoadRedundancy:1;    //并机冗余      说明可以关掉一台 unused
        Uint16 OnGridWorking:1;     //相连是否处于离网并联状态
        Uint16 MoudleFault:1;       //模块故障
        Uint16 SrcAddr:3;           //并机地址
        Uint16 SelfCheck:1;         //上电自检
        //25~32
        Uint16 enParallel:1;        //检测到并机系统
        Uint16 AppCfgFault:1;       //EMS配置错误
        Uint16 freq:1;              //统一系统频率
        Uint16 volt:1;              //统一系统电压
        Uint16 redundancyMode:1;      //允许部分机器保持工作
        Uint16 Open:1;                //输出开路
        Uint16 ready:1;               //
        Uint16 ComTurnOff:1;                 //
    }Bits;
}PARA_STATUS_UNION;

typedef struct{
    Struct_Abc_F P;      //有功
    Struct_Abc_F Q;      //无功
    Struct_Abc_F sigma;      //功角
    Struct_Abc_F P_delta;    //
    Struct_Abc_F Q_delta;    //
    Struct_Abc_F Idc_delta;    //
    Struct_Abc_F Pave_delta;    //
    Struct_Abc_F Iave_delta;    //
    Struct_Abc_F Psoc_delta;    //

    Struct_Abc_F Vadj;       //电压偏差
    Struct_Abc_F V0adj;      //电压偏差
    Struct_Abc_F VNadj;      //电压偏差
    Struct_Abc_F Iadj;       //电刘偏差
    float Pdc_delta;
    float Ro;
    float Rc;
    float Kp;
    float Kq;
    float Kp2;
    float Kq2;
    float Kdc;
    PIDCntl_F   QaCntl,QbCntl;
}DROOPCntl_F;
extern DROOPCntl_F DroopCntl;

typedef struct
{
    Struct_Abc_F Il;
    Struct_Abc_F VGrid;
    Struct_Abc_F VCap;
    Struct_Abc_F Q;
    Struct_Abc_F P;

    float VbusP;
    float VbusN;
    float Vbus;
    float Vbuck;
    float Vbat;
    float Ibat;

    float DcWatt;
    float AcWatt;
    float AcVar;
    float Fgrid;
}AD_SAMPLE_Float32;

extern AD_SAMPLE_Float32 ParaSample;


typedef struct{

    Struct_Abc_F  Vavg;  //Variable 本机电压平均值
    Struct_Abc_F  Vbus;  //Variable 本机母线平均值
    Struct_Abc_F  Pavg;  //Variable 本机有功平均值
    Struct_Abc_F  Iavg;  //Variable 本机电流平均值
    Struct_Abc_F  V0;    //Variable 本机电压0漂值
    Struct_Abc_F  RmsCntl;
    Struct_Abc_F  droopTheta;
    Struct_Abc_F  droopTheta_delta;

}PARA_ELECT_STRUCT;



typedef struct{

    float theta_Adj;           //output  预同步相位调节
    float theta_gain;          //调节增益
    float P_limit;             //input
    float Q_limit;             //input
    float Idc_limit;           //input
    float* theta_Pt;           //input  本机角度

    float*   Pdc_local;                 //input  直流功率
    float    Pdc_para;                  //input  直流功率
    float    P_Soc;                     //input  soc均衡功率
    Uint16*  SOP_local;                 //input  SOP
    float    SOP_para[cParaMaxNum];     //input  SOP
    float    SOP_min;                   //input  最小SOP
    Uint16*  SOC_local;                 //input  SOC
    Uint16   SOC_para[cParaMaxNum];     //input  SOC

    Struct_Abc_PT V0;                       //input  电压直流分量
    Struct_Abc_PT VCap;                     //input  电容电压
    Struct_Abc_PT Idc_local;                //input  直流电流
    Struct_Abc_F  Idc_para;                 //input  直流电流

    Struct_Abc_Int  P_local;                  //input  本机有功
    Struct_Abc_Int  P_para[cParaMaxNum];      //input  并机有功
    Struct_Abc_Int  Q_local;                  //input  本机无功
    Struct_Abc_Int  Q_para[cParaMaxNum];      //input  并机无功
    Struct_Abc_Int32  P_Sum;                  //input  本机有功
    Struct_Abc_Int32  Q_Sum;                  //input  本机有功

    PARA_ELECT_STRUCT local;
    PARA_ELECT_STRUCT para;

    int16  TBCTR_local;        //本机载波
    int16  TBCTR_master;       //主机载波
    int16  TBCTR_rxts;         //
    int16  TBCTR_delay;        //can载波延迟
    int16  TBPRD;              //

    int16  theta_local;        //本机相位
    int16  theta_master;       //主机相位
    int16  theta_delay;        //can相位延迟
    int16  theta_rxts;
    float  theta_Start;        //本机相位细调开始
    float  theta_End;          //本机相位细调结束

    Uint16 P_local_SendCnt;
    Uint16 Q_local_SendCnt;
    Uint16 SendCnt_local;
    Uint16 SendCnt_para;
    Uint16 SendCnt_delta;

    int16  RealNumber;
    int16  SetNumber;
    int16  ReadyNumber;

    float*  Systemfreq;
    float*  SystemVolt;
    Uint32  CommAddr;
    Uint32  UID;
    //Uint16  CTRDIR_Local;    //本机载波计数方向
    //Uint16  CTRDIR_Receive;  //并机系统载波计数方向
}PARA_ShareData_STRUCT;



extern PARA_STATUS_UNION Flag_Local,Flag_para[cParaMaxNum];
extern PARA_CYCLE_UNION  ParaCycles;
extern PARA_EVENT_UNION  ParaEvent;
extern PARA_ShareData_STRUCT  ShareData,MasterData;
extern PIDCntl_F  VaRmsCntl ,VbRmsCntl;
extern PIDCntl_F  VaIdcCntl ,VbIdcCntl;

extern void    InitParallel(void);
extern void    InitParallelCan(void);
extern void    ParallelTask(void);
extern void    Para_GetPwmStep(void);
extern void    Para_PowerOff(void);
//extern void    Para_SendWorkState(Uint16 mode);
extern Uint16  Para_CheckWorkState(Uint16 mode);
extern Uint16  Para_CheckPowerOn(void);
extern void    Para_SendPowerOff(void);
extern void    Para_StartSystem(void);
extern Uint16  sGetModuleType(void);
interrupt void Para_CanIsr(void);


//extern void    ResistanceMatching (Uint32 uid);


#endif /* MODULE_PARALLEL_H_ */

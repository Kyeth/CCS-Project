/*
 * PVStateMachine.h
 *
 *  Created on: 2021年10月7日
 *      Author: tzz
 */

#ifndef _PVSTATEMACHINE_H_
#define _PVSTATEMACHINE_H_

#include "CNTL_PI_F.h"
#include "stdint.h"
#include "MPPTFunc.h"
#include "rmp.h"

#define PV_INPUT_LIMIT      (16.0f*0.99f)       // 效率0.99
#define PVBoard  (0)
#define Vsource  (1)


typedef enum
{
    INIT_STATE = 0,
    SELF_STATE,
    MPPT_STATE,
    ERROR_STATE
}PV_STATE_TYPE;
// PV 并机类型
typedef enum
{
    PV_FREE = 0,
    PV_MASTER,
    PV_SLAVE,
    PV_S_LOOP       // 单电流环模式
}PV_PARALLEL_TYPE;

//typedef struct
//{
//    PV_PARALLEL_TYPE Parallel;  // 并机类型
//    PV_STATE_TYPE State;        // PV 状态上面4种
//    uint16_t Run;               // run-1 or not-0
//    uint16_t Error;             // PV error状态
//    uint16_t PVId;              // PV 通道号
//    uint16_t SelfCheckFinish;   // 自检完成标志
//
//    uint16_t Count;             // 计数器1
//    uint16_t Period;            // 周期1
//
//    float PVCurrLimit;          // PV 限流值
//}PV_STATE_DATA;

typedef struct
{
    PV_PARALLEL_TYPE Parallel;  // 并机类型
    PV_STATE_TYPE State;        // PV 状态上面4种
     int16_t Run;               // run-1 or not-0
    uint16_t Error;             // PV error状态
    uint16_t PVId;              // PV 通道号
    uint16_t SelfCheckFinish;   // 自检完成标志

    uint16_t Count;             // 计数器1
    uint16_t Period;            // 周期1


    float M0;                 //调制比
    float M1;                 //调制比
    float M;                 //调制比
    float Vff;              // qk
    float CurrGain;             // PV 限流值
    float Duty;             // PV 限流值
    float PVVoltLimit;      //
    float PVCurrLimit;      // PV 限流值
    float* DCVolt;            //
    float* PVVolt;            // PV 限流值
    float* PVCurr;            // PV 限流值
    uint16_t Online;           // PV 限流值
    uint16_t Start;           // PV 限流值

    // MPPT
    MPPT_DATA Mppt;
    // 电压环电流环
    CNTL_PI_F VoltPIF;
    CNTL_PI_F CurrPIF;

    // 电压参考给定
    RMPCNTL VoltSet;
    RMPCNTL CurrSet;
}PV_STATE_DATA;

// PV 系统变量
typedef struct
{
    uint16_t PVStart;

    float PV1PowerAim;              // PV1目标功率
    float PV2PowerAim;              // PV2目标功率
    float PV3PowerAim;              // PV3目标功率
    float PV4PowerAim;              // PV4目标功率
    uint16_t PVSourceType;             // PV源类型0-PV;1-直流源

    uint16_t PV1Available;             // PV1电压满足
    uint16_t PV2Available;             // PV2电压满足
    uint16_t PV3Available;             // PV3电压满足
    uint16_t PV4Available;             // PV4电压满足

    uint16_t PVParallel;        // PV 并联状态
    uint16_t PVWorkNum;         // 当前 PV 工作的路数
    uint16_t PVParallelNum;     // 当前 PV 并机数量
    int16_t PVParallelWorkNum; // 当前 PV 并机工作数量
    float PVMasterMPPTout;      // 并联主机MPPT输出
    float PVMasterCurrRef;      // PV 并机模式下主机电流参考值
    float PVMaxChgCurrSet;      // PV 最大限流值
    float BusAdptCoeff;         //BUS电压自适应系数
    float BusDutyLimit;         //BUS电压自适应系数
    float IPVParallelWorkNum;

    uint16_t MpptTurn;
}PV_SYSTEM_DATA;

extern PV_SYSTEM_DATA PVSystemData;
extern PV_STATE_DATA PV1WorkState, PV2WorkState, PV3WorkState, PV4WorkState;
extern CNTL_PI_F PV1VoltPIF,PV2VoltPIF,PV3VoltPIF,PV4VoltPIF;
extern CNTL_PI_F PV1CurrPIF,PV2CurrPIF,PV3CurrPIF,PV4CurrPIF;
extern MPPT_DATA Mppt1,Mppt2,Mppt3,Mppt4;
extern RMPCNTL PV1VoltSet,PV2VoltSet,PV3VoltSet,PV4VoltSet;
extern RMPCNTL PV1CurrSet,PV2CurrSet,PV3CurrSet,PV4CurrSet;

void InitPVControlData(void);
void InitPVStateMachineData(void);
//void UpdateMPPTVoltLimit(uint16_t id);
void InitMpptData(void);
void RunPVStateMachine(void);
void CheckPVParallelErr(void);
void CalculatePVCurrLimite(void);

#endif /* APP_INC_HANDLEPVSTATE_H_ */

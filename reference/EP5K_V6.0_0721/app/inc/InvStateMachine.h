/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: InvStateMachine.h
Author: tzz
Version: 0.0.2
Date: 2021年11月10日
Description: 外部函数及变量声明
*************************************************/
#ifndef _INVSTATEMACHINE_H_
#define _INVSTATEMACHINE_H_

#include "stdint.h"

#define DELAY_10S_100US  100000L
#define DELAY_5S_100US   50000
#define DELAY_3S_100US   30000
#define DELAY_2S_100US   20000

typedef enum
{
    InitState = 0,       // 初始化状态
    SelfCheckState,      // 停机/自检状态
    ErrorState,          // 故障状态
    OffGridState,        // 离网运行状态
    UpsState,            // 待机状态
    OnGridState,         // 离网状态
    BusCharge,           // BUS预充
    ByPass               // 旁路状态
}INV_STATE_TYPE;

typedef struct
{
    INV_STATE_TYPE WorkState;
//    uint32_t chgRelayCheckCount;// 预充继电器闭合计数
    uint16_t InitStateCnt;      // 初始化状态计数器
    uint32_t SelfCheckCnt;      // 自检状态计数器
    uint16_t ErrorStateCnt;     // 故障状态计数器
    uint16_t EmgyChgFlg;        // 紧急充电标志
    uint16_t UpsGridFlg;        // 电网相位同步成功标志
    uint16_t MidRlyFlg;         // 中间jdq先闭合标志
    uint16_t UpsStateCnt;       // 旁路状态计数器
    uint16_t BpsStateCnt;       // 旁路状态计数器
    uint16_t OffGridCnt;        // 离网状态计数器
    uint16_t OffGridStartCnt;
    uint16_t OffGridFirst;
    uint16_t ClearCnt;          // buffer 清除计数器
    uint32_t OnGridCnt;         // 并网状态计数器
    uint32_t GridErrTZCnt;      // 锁波计数
    uint32_t OnGridCloseCnt;    // R相目标功率为零封驱动计数
    uint32_t OnGridCloseCnt2;   // S相目标功率为零封驱动计数
    uint32_t OnGridCloseFlg;    // 目标功率为零封驱动计数
    uint32_t OnGridCloseFlg2;   // 目标功率为零封驱动计数
    uint16_t OnGridPermit;      // 允许并网
    uint16_t BypassPermit;      // 允许旁路
    uint16_t OnGridToOffFlag;   // 并网转离网标志
    uint16_t RelayOnDelay;      // 继电器闭合时间延时
    uint16_t OffBypassFlag;
    uint16_t PreChagCnt;         //预充时间
    uint16_t GridZCD;            //过零点
}INV_STATE_STRUCT;

extern INV_STATE_STRUCT InvStateData;

void InitInvStateMachineData(void);
void RunInvStateMachine(void);
void ControlRelay(INV_STATE_TYPE state);

#endif /* APP_INC_HANDLESTATEMACHINE_H_ */

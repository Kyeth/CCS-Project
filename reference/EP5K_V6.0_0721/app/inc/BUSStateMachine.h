/*
 * BUSStateMachine.h
 *
 *  Created on: 2025年5月21日
 *      Author: 86130
 */

#ifndef APP_INC_BUSSTATEMACHINE_H_
#define APP_INC_BUSSTATEMACHINE_H_

typedef enum
{
    Init_State = 0,
    Work_State,
    Error_State
}BUS_STATE_TYPE;
// BUS 系统变量
typedef struct
{   
    float KIN ;                     // 上下电容瞬时电压差
    float KDP ;                     //
    float KIAVE ;                   //
    float FoldCurr;                 //
    float DeltaBusIns ;             // 上下电容瞬时电压差
    float DeltaBusAvg8 ;            // 上下电容8点滤波电压差
    float DeltaBusAvg200 ;          // 上下电容200点滤波电压差
    float DutyData;
    uint16_t StartBusCnt;           // BUS启动平衡计时
    uint16_t BusStartFlag ;         // BUS平衡调节触发
    uint16_t OCPWindow;             // 过流检测窗口
	int16  OCPCnt;
    int16  L1OCPCnt;
    int16  L2OCPCnt;
    uint16_t Count;                 // BUS恢复平衡计时

}BUS_SYSTEM_DATA;

extern BUS_SYSTEM_DATA BusSystemData;

typedef struct
{
    BUS_STATE_TYPE StartState;        // BUS 状态上面4种

}BUS_STATE_DATA;
extern BUS_STATE_DATA BusStateData;

void InitBusBalanceData(void);
void CheckBUSBalanceState(void);
void RunBusState(void);
void BusOCP(void);

#endif /* APP_INC_BUSSTATEMACHINE_H_ */

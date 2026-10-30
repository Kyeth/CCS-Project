/*
 * McanDrive.h
 *
 *  Created on: 2024年3月16日
 *      Author: DLadmin
 */

#ifndef USER_PARALLEL_CANDRIVE_H_
#define USER_PARALLEL_CANDRIVE_H_

#include "Types.h"

typedef union {
    struct {  //该结构体用于收发
    uint32 ID;
    uint32 DATA_32;
    uint32 DATB_32;
    };
    //兼容标准帧11bit格式
    struct {              // 该结构体用于解析
    uint32 src : 8;       // 扩展 源地址
    uint32 dst : 8;       // 扩展 目标地址
    uint32 page :2;       // 扩展 页
    uint32 Cmd : 8;       // 标准 指令
    uint32 SrcAddr :3;    // 标准 源地址
    uint32 rtr :1;        // Remote
    uint32 xtd :1;        // Extended Identifier
    uint32 esi :1;        // Error State Indicator
    float DATA_F ;
    float DATB_F ;
    };
    struct {  //该结构体用于解析(整型)
    uint16 ID_L;
    uint16 ID_H;
    uint16 Data_0 ;                     // 15:0 Data Byte 0
    uint16 Data_1 ;                     // 31:16 Data Byte 1
    uint16 Data_2 ;
    uint16 Data_3 ;
    };
}User_CAN_DATA_UNION;

extern char sCheckCanRx(User_CAN_DATA_UNION *CanMsg);
extern void sUserDebugData(User_CAN_DATA_UNION *CanMsg);
extern void sSendFloatFrame(uint16 Command , float dataA , float dataB);
extern void sSendUint32Frame(uint16 Command , uint32 dataA, uint32 dataB);
extern void sSendInt16Frame(uint16 Command, uint16 data0, uint16 data1, uint16 data2, uint16 data3);
extern void sSendGraph(uint16 data1, uint16 data2, uint16 data3, uint16 data4, uint16 data5, uint16 data6);

extern void sSendPwmTBCTR(void);
extern void sSendPhaseData(void);
extern void sSendRlyAction( uint16 rlyAction);
extern void Para_SendWorkState( uint16 WorkState);

#endif /* USER_PARALLEL_MCANDRIVE_H_ */

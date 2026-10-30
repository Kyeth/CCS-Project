/**
 * @file Hw_CanDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS CAN驱动头文件
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * 说明：F2837xS的CAN模块是32位寄存器的DCAN，报文对象通过IF1/IF2接口寄存器读写，
 *      本驱动只使用IF1写、IF2读，不使用IF3。
 */

#ifndef HW_CAN_DRIVER_H
#define HW_CAN_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CAN模块编号：F2837xS共2个CAN模块，本板两个都用 */
typedef enum
{
    HW_CAN_ID_A = 0U,           /* CANA：GPIO36(RX)/GPIO37(TX) */
    HW_CAN_ID_B,                /* CANB：GPIO39(RX)/GPIO38(TX) */
    HW_CAN_ID_MAX,
    HW_CAN_ID_NULL = 0xFFU
} E_HW_CAN_ID;

/* 报文对象数量，F2837xS固定32个，编号1~32 */
#define HW_CAN_OBJ_MAX                  (32U)

/* 一帧最多8字节 */
#define HW_CAN_DATA_LEN_MAX             (8U)

/* 一个CAN模块的配置项
 * u16TxObjNum/u16RxObjNum 是报文对象起始编号（1~32），发送对象在前、接收对象在后
 * u32RxAcceptId/u32RxAcceptMask 只在接收对象上用，屏蔽位为1表示必须与ID一致
 * u16RxDlc 接收对象的初始DLC，一般填8 */
typedef struct
{
    Uint16 u16CanId;                /* 模块编号，E_HW_CAN_ID */
    Uint32 u32Baudrate;             /* 波特率，如500000 */
    Uint16 u16TxObjNum;             /* 发送报文对象起始编号 */
    Uint16 u16TxObjCnt;             /* 发送报文对象个数 */
    Uint16 u16RxObjNum;             /* 接收报文对象起始编号 */
    Uint16 u16RxObjCnt;             /* 接收报文对象个数 */
    Uint32 u32RxAcceptId;           /* 接收验收ID */
    Uint32 u32RxAcceptMask;         /* 接收验收屏蔽，1=必须匹配 */
    Uint16 bExtId;                  /* 1:29位扩展帧 0:11位标准帧 */
    Uint16 bLoopback;               /* 1:内部回环自测 0:正常 */
    Uint16 bAutoBusOn;              /* 1:使能自动总线恢复(ABO) */
} ST_CAN_CFG_ITEM;

/* 全部CAN模块的配置表 */
typedef struct
{
    const ST_CAN_CFG_ITEM *pstCanCfgItem;
    Uint16 u16ItemNum;
    Uint32 u32SysclkHz;
} ST_CAN_CFG;

/* 初始化配置表中的全部CAN模块 */
void Hw_CanInit(const ST_CAN_CFG *pstCanCfg);
Uint16 Hw_CanIsInitialized(Uint16 u16CanId);

/* 发送一帧，u16ObjIdx是发送对象序号（0~u16TxObjCnt-1），
 * pu16Data按"一个Uint16放一个字节"排列，u16Len为0~8，返回1表示已装载并启动发送 */
Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len);

/* 查询发送是否完成，返回1表示该对象已发送完毕（TxRqst已清零） */
Uint16 Hw_CanIsTxDone(Uint16 u16CanId, Uint16 u16ObjIdx);

/* 读取一帧接收数据，返回1表示读到了新数据
 * u16ObjIdx是接收对象序号（0~u16RxObjCnt-1），pu32MsgId可为空，pu16Len可为空 */
Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len);

/* 查询是否有待读的接收数据，返回1表示有新数据 */
Uint16 Hw_CanIsRxReady(Uint16 u16CanId, Uint16 u16ObjIdx);

/* 读取错误状态寄存器CAN_ES的原始值 */
Uint16 Hw_CanGetStatus(Uint16 u16CanId);

/* 查询是否处于总线关闭状态，返回1表示Bus-Off */
Uint16 Hw_CanIsBusOff(Uint16 u16CanId);

/* 总线关闭后的恢复：重新进入初始化再退出，返回1表示成功 */
Uint16 Hw_CanRecoverBusOff(Uint16 u16CanId);

#ifdef __cplusplus
}
#endif

#endif /* HW_CAN_DRIVER_H */

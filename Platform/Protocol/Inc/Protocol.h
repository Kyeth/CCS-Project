/**
 * @file Protocol.h
 * @brief 协议层框架：协议实例配置表 + 协议操作表 + 1 ms 任务分发。
 *
 * 三层分工：
 *   - 应用层维护 Modbus 点表（地址、权限、类型、变量绑定、上下限）；
 *   - 工程配置维护协议实例配置表（SCI 端口、协议类型、从机地址、点表、实例上下文）；
 *   - 通用协议模块维护协议操作表（Init / Service / Reset）。
 *
 * 调度：ProtocolInit() 在 ApplicationInit() 里建表，ProtocolTask() 由 1 ms 任务
 * 节拍调用，每次对每个已启用实例做一次有界服务。1 ms 是任务运行周期，不是 RTU
 * 的帧间隔精度；帧间隔由协议自己用"批次观察时刻"判断。
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "F2837xS_device.h"

#define PROTO_TYPE_MODBUS       (1U)   /* 本版只实现 Modbus RTU 从机 */
#define PROTO_MAX_OPS           (2U)   /* 以后加 Xmodem 再占一格 */
#define PROTO_MAX_INSTANCES     (3U)   /* 对应三路 SCI */

typedef struct ST_PROTO_INSTANCE ST_PROTO_INSTANCE;

/** 协议操作表：一个协议一行，通常只在加新协议时改动。 */
typedef struct {
    Uint16 u16Type;
    Uint16 (*pfInit)(ST_PROTO_INSTANCE *pstInst);
    Uint16 (*pfService)(ST_PROTO_INSTANCE *pstInst);
    void   (*pfReset)(ST_PROTO_INSTANCE *pstInst);
} ST_PROTO_OPS;

/** 协议实例配置表：一行就是"某端口上跑某协议、用某张点表"。 */
struct ST_PROTO_INSTANCE {
    Uint16 u16SciId;         /* HW_SCI_ID_A / _C / _D */
    Uint16 u16Type;          /* PROTO_TYPE_* */
    Uint16 u16SlaveAddr;     /* 从机地址 */
    Uint16 u16Enabled;       /* 1 = 参与 1 ms 服务 */
    Uint16 u16TblCount;      /* 点表项数 */
    Uint32 u32Baud;          /* 端口波特率（帧间隔换算用，来自工程配置） */
    const void *pstTbl;      /* 指向该协议的点表 */
    void *pCtx;              /* 协议私有实例上下文（缓冲、状态） */
};

/** 清表。 */
void ProtocolInit(void);

/** 登记一张协议操作表；重复类型返回 0。 */
Uint16 ProtocolRegisterOps(const ST_PROTO_OPS *pstOps);

/** 追加一个实例并调用该协议的 pfInit；失败返回 0（实例不参与服务）。 */
Uint16 ProtocolAddInstance(const ST_PROTO_INSTANCE *pstInst);

/** 1 ms 任务入口：依次服务所有启用实例，每个实例一次有界服务。 */
void ProtocolTask(void);

/** 改变端口所有权（如切到 Xmodem）时，复位该端口上的协议实例。 */
Uint16 ProtocolResetPort(Uint16 u16SciId);

Uint16 ProtocolInstanceCount(void);
const ST_PROTO_INSTANCE *ProtocolInstance(Uint16 u16Index);
Uint32 ProtocolServiceCalls(void);
Uint32 ProtocolServiceErrors(void);

#endif /* PROTOCOL_H */

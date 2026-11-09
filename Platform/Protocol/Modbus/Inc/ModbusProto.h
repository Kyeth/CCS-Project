/**
 * @file ModbusProto.h
 * @brief Modbus RTU 从机适配层：把协议核心接到 SciPort 与点表上。
 *
 * 这是 PROTO_TYPE_MODBUS 的协议操作表实现。它自己持有每实例的接收/回复缓冲
 * （ModbusRtu 的实例结构），并把点表查询绑定成 ModbusRtu 的读写回调。
 */

#ifndef MODBUS_PROTO_H
#define MODBUS_PROTO_H

#include "Protocol.h"

/** 协议操作表：由 AppModbus 在初始化时登记到 Protocol 层。 */
extern const ST_PROTO_OPS stModbusProtoOps;

/** 取第 u16Index 个私有状态（供工程配置表填写 pCtx）。 */
void *ModbusProtoState(Uint16 u16Index);

/** 清空全部私有状态（初始化前调用）。 */
void ModbusProtoClear(void);

/* 观测口径：请求执行数、回复全部入环数、强制复位作废数、丢弃候选帧数。 */
Uint32 ModbusProtoRxFrames(Uint16 u16Index);
Uint32 ModbusProtoTxSubmitted(Uint16 u16Index);
Uint32 ModbusProtoTxLostEvents(Uint16 u16Index);
Uint32 ModbusProtoRxDropped(Uint16 u16Index);

#endif /* MODBUS_PROTO_H */

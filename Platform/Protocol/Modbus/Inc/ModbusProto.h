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
Uint32 ModbusProtoRxBacklog(Uint16 u16Index);

/*
 * 协议切换入口（ARM 升级）：候选帧起点匹配前 6 字节后回调，命中时本层把端口
 * 交给 Xmodem。这个特例不校验后续数据与 CRC，普通 Modbus 请求仍按原规则校验。
 */
#define MB_ARM_PREFIX_LEN       (6U)
#define MB_ARM_REPLY_MAX        (16U)

/**
 * @brief ARM 升级入口回调。
 * @param pu16Prefix 匹配到的 6 个起始字节
 * @param pu16Reply  命中时由回调填写握手回复
 * @return >0 = 握手回复字节数（发送完成后交接端口）；0 = 不是升级命令
 */
typedef Uint16 (*MB_ARM_HOOK)(void *pCtx, const Uint16 *pu16Prefix, Uint16 u16PrefixLen,
                              Uint16 *pu16Reply, Uint16 u16ReplyMax);

/** 交接完成（ACK 与 'C' 都已发完）时的通知，用于切到升级波特率。 */
typedef void (*MB_ARM_DONE_HOOK)(void);

void ModbusProtoSetArmHook(Uint16 u16SciId, void *pCtx, MB_ARM_HOOK pfHook);

/** 注册交接完成回调（ACK 与 'C' 发完、硬件空闲后调用一次）。 */
void ModbusProtoSetArmDoneHook(void *pCtx, MB_ARM_DONE_HOOK pfHook);
/** 1 = 该端口已交给升级协议，本层不再收发。 */
Uint16 ModbusProtoArmActive(Uint16 u16Index);

/** 1 = 握手回复仍在提交途中（尚未全部进入端口环）。 */
Uint16 ModbusProtoArmPending(Uint16 u16Index);
/** 升级结束后交还端口，本层从找帧头重新开始。 */
void ModbusProtoArmRelease(Uint16 u16Index);

#endif /* MODBUS_PROTO_H */

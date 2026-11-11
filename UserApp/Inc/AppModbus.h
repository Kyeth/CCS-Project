/**
 * @file AppModbus.h
 * @brief Modbus RTU 从机应用层对外接口（05E：参考协议层 + 端口层桥接）。
 */

#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include "Protocol.h"

/** 三路 SCI 从站：A / C / D。 */
#define APP_MB_PORTS (3U)

/** 建立三张从站表并登记协议实例；必须在 ProtocolTask 开始运行前调用一次。 */
void AppModbusInit(void);

/** 已登记进 1 ms 任务的从站实例数（0..APP_MB_PORTS）。 */
Uint16 AppModbusEnabledCount(void);

/** 初始化阶段拿到错误返回的次数；0 = 三个实例全部登记成功。 */
Uint32 AppModbusInitErrors(void);

/** 该端口的升级标志（参考层 stMdsUpdateParam.u16MdsIsUpdate）。 */
Uint16 AppModbusUpgradeFlag(Uint16 u16Port);

/** 升级结束（成功或取消）后把端口交还协议层 */
void AppModbusReleasePort(Uint16 u16Port);

/* 同时改硬件速率与协议层记录：只改硬件会让 t3.5 与端口层缓存对不上。 */
void AppModbusRestoreBaud(Uint16 u16SciId, Uint32 u32Baud);

/** 诊断：该端口已交给参考层的完整请求帧数。 */
Uint32 AppModbusRxFrames(Uint16 u16Port);

/** 诊断：该端口已整帧交给端口层的回复帧数。 */
Uint32 AppModbusTxFrames(Uint16 u16Port);

/** 诊断：该端口因缓冲装不下或端口不在接收态而丢弃的字节数。 */
Uint32 AppModbusRxDropped(Uint16 u16Port);
Uint32 AppModbusTxDropped(Uint16 u16Port);
Uint32 AppModbusCallBacks(Uint16 u16Port);

/** 诊断：协议层实例（只读）。 */
const ST_PROTO_INSTANCE *AppModbusInstance(Uint16 u16Index);

#endif /* APP_MODBUS_H */

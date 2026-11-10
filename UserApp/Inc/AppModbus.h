/**
 * @file AppModbus.h
 * @brief 应用层 Modbus 点表与协议实例配置表。
 *
 * 应用层只维护两件事：
 *   - Modbus 点表：地址、权限、类型、变量绑定、上下限；
 *   - 协议实例配置表：哪个 SCI 口跑 Modbus、从机地址多少、用哪张点表。
 * 收发、校验、类型转换、帧间隔都由平台协议层完成。
 */

#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include "F2837xS_device.h"
#include "Protocol.h"

#define APP_MB_PORTS            (3U)

/** 建立点表与实例表；必须在 ProtocolTask 开始运行前调用一次。 */
void AppModbusInit(void);

Uint16 AppModbusEnabledCount(void);
Uint32 AppModbusTableErrors(void);

/** 该端口是否已收到升级请求（对应参考实现的 u16MdsIsUpdate）。 */
Uint16 AppModbusUpgradeFlag(Uint16 u16Port);
const ST_PROTO_INSTANCE *AppModbusInstance(Uint16 u16Index);

#endif /* APP_MODBUS_H */

/*
 * AppModbusProtoTbl.h
 *
 * SCI-A 的 Modbus 点表（按 ARM/DSP 协议表生成，共 440 项）。
 * 由 _tmp/gen_proto_tbl.py 生成，不要手工修改；改协议表后重新生成。
 *
 * 约定（所有者 m07729）：所有点只有 int16 与 Uint16 两种类型；
 * 缩放由业务层单独处理，点表只负责类型与上下限；
 * 当前没有业务变量，全部绑定到占位数组 s_au16ProtoRegs[]。
 */

#ifndef APP_MODBUS_PROTO_TBL_H
#define APP_MODBUS_PROTO_TBL_H

#include "F2837xS_device.h"
#include "ModbusTbl.h"

#define APP_PROTO_REG_COUNT (440U)

/** 协议点表（Flash 常量，按地址升序）。 */
extern const ST_MODBUS_POINT g_astAppProtoPointTbl[APP_PROTO_REG_COUNT];

/** 占位寄存器存储的首地址与项数（调试观察用）。 */
Uint16 *AppProtoRegBase(void);
Uint16 AppProtoRegCount(void);

#endif

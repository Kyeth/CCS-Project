/*
 * AppModbusRegCfg.h
 *
 * Modbus 寄存器表（按 ARM/DSP 协议表生成，共 440 项）。
 * 由 _tmp/gen_proto_tbl.py 生成，不要手工修改；改协议表后重新生成。
 *
 * 行格式与参考工程 Platform/Protocol/Modbus 保持逐字一致：
 *   {addr, READ_WRITE/READ_ONLY, TYPE_UINT16/TYPE_INT16, NOT_SAVE,
 *    (U_REG_VAL *)&变量, i32DownLimitVal, u32UpLimitVal}
 *
 * 约定（所有者 m07729）：所有点只有 int16 与 Uint16 两种类型；
 * 缩放由业务层单独处理，寄存器表只负责类型与上下限；
 * 地址必须严格升序——ReadModbusRegisterData() 用二分查找，乱序会查不到；
 * 当前没有业务变量，全部绑定到占位数组 s_auRegVal[]（段 .mbregs）。
 */

#ifndef APP_MODBUS_REG_CFG_H
#define APP_MODBUS_REG_CFG_H

#include "F2837xS_device.h"
#include "ModbusRegProcess.h"

/* 寄存器表项数。 */
#define APP_MODBUS_REG_COUNT (440U)

/* 寄存器表（Flash 常量，按地址严格升序）。 */
extern const ST_REG_SLAVE_ITEM g_astAppRegSlaveItem[APP_MODBUS_REG_COUNT];

/* 从站寄存器配置：表头 + 项数，直接交给 ModbusSlaveTableRegister()。 */
extern const ST_REG_SLAVE_CFG stRegSlaveCfg_App;

/* 把占位寄存器清零。.mbregs 不参与 C 运行时清零，上电必须显式调一次。 */
void AppRegCfgReset(void);

#endif /* APP_MODBUS_REG_CFG_H */

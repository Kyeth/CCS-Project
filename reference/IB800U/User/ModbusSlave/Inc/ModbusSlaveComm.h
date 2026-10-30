#ifndef __MODBUSSLAVECOMM_H
#define __MODBUSSLAVECOMM_H

#include "ModbusSlave.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void ModbusSlaveInit(void);
extern void TaskModbusSlave_1(void);
extern void TaskModbusSlave_2(void);

extern Uint16 ModbusTimeOutCnt_1;        // Modbus超时统计计数器
extern Uint16 ModbusTimeOutCnt_2;        // Modbus超时统计计数器

extern ST_MODBUS_SLAVE_TABLE_CRTL ObjModbusSlaveCtrl_1;    // Modbus从站对象
extern ST_MODBUS_SLAVE_TABLE_CRTL ObjModbusSlaveCtrl_2;    // Modbus从站对象

#ifdef __cplusplus
}
#endif

#endif

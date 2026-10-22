/**
 * @file Crc16.h
 * @author yetingheng (004182)
 * @brief Modbus RTU 用的 CRC16（多项式 0xA001 反射，初值 0xFFFF）
 * @version 0.1
 * @date 2026-10-23
 * @copyright Copyright (c) 2026
 *
 * @note 查表法实现：256 项表占 512 字节 flash，换掉每字节 8 次移位循环。
 *       数据按"一个 Uint16 存一个字节（低 8 位有效）"的方式传，与 SCI 驱动一致。
 */
#ifndef CRC16_H
#define CRC16_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 计算一段数据的 CRC16，pu16Data 每个元素只取低 8 位 */
Uint16 Crc16Modbus(const Uint16 *pu16Data, Uint16 u16Len);

/* 把 CRC 追加到帧尾：低字节在前（Modbus RTU 规定），返回新的帧长 */
Uint16 Crc16ModbusAppend(Uint16 *pu16Frame, Uint16 u16Len);

/* 校验整帧（含末尾 2 字节 CRC），返回 1 表示正确 */
Uint16 Crc16ModbusCheck(const Uint16 *pu16Frame, Uint16 u16Len);

#ifdef __cplusplus
}
#endif

#endif /* CRC16_H */

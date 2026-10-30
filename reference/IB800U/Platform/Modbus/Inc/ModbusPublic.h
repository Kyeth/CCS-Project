/**
 * @file ModbusPublic.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __MODBUSPUBLIC_H
#define __MODBUSPUBLIC_H

#include "F28003x_device.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef TRUE
   #define TRUE              1
#endif

#ifndef FALSE
   #define FALSE             0
#endif



#define HI_UINT16(u16Num)                       (((u16Num) >> 8)  & 0xFF)     // ȡһ��16λ�������ݵĸ߰�λ
#define LO_UINT16(u16Num)                       ((u16Num) & 0xFF)       // ȡһ��16λ�������ݵĵͰ�λ
#define BUILD_UINT16(u8Low, u8High)    (((Uint16)u8Low & 0xFF) | ((Uint16)u8High << 8))


/**
 * @brief ModbusCRC16
 *        用于计算Modbus帧的CRC值
 * 
 * @param pu16Buff - 16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 *                  待计算的数据
 * @param u16DataLen - 需要计算CRC的长度
 * @return Uint16 - 0：入参异常
 *                  其他值：16位的CRC校验
 */
extern Uint16 ModbusCRC16(Uint16 *pu16Buff, Uint16 u16DataLen);


/**
 * @brief   PackModbusCrc
 *          计算CRC值并将封装CRC值进指定索引位置
 * 
 * @param pu16Buff - 16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 *                  待计算的数据
 * @param u16Index - 索引位置，同时也是需要计算的数据个数
 * @return Uint16 -  0：入参异常
 *                  其他：CRC校验值
 */
extern Uint16 PackModbusCrc(Uint16 *pu16Buff, Uint16 u16Index);


/**
 * @brief   CheckModbusCrc
 *          检测接收的CRC值与计算的CRC值是否一致
 * 
 * @param pu16Buff - 16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 *                  待计算的数据
 * @param u16DataLen - 接收的数据长度
 * @return int16 -  -1：入参异常
 *                  -2：数据长度小于3
 *                  -3：CRC值异常
 *                  1：CRC值相等
 */
extern int16 CheckModbusCrc(Uint16 *pu16Buff, Uint16 u16DataLen);

#ifdef __cplusplus
}
#endif

#endif

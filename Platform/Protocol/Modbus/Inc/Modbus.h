/**
 * @file Modbus.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __MODBUS_H
#define __MODBUS_H


#include "ModbusRegProcess.h"
#include "ModbusPublic.h"
#include "ModbusSlave.h"

#ifdef __cplusplus
extern "C" {
#endif

//typedef void (*ModbusCallBack)(void);       // Modbus回调函数接口
//
//
//typedef enum
//{
//    MODBUS_SLAVE_DEFAULT = 0x00,
//    MODBUS_SLAVE_TABLE_MAX,
//    MODBUS_SLAVE_TABLE_NULL,
//}E_MODBUS_SLAVE_TABLE_TYPE;
//
//typedef enum
//{
//    MODBUS_MASTER_DEFAULT = 0x00,
//    MODBUS_MASTER_TABLE_MAX,
//    MODBUS_MASTER_TABLE_NULL,
//}E_MODBUS_MASTER_TABLE_TYPE;
//
//
//typedef struct
//{
//    E_MODBUS_SLAVE_TABLE_TYPE eProtocolType;
//    ST_REG_SLAVE_CFG *pstProtocolTable;
//    ModbusCallBack SlaveModbusCallBack;     // 从机回调函数接口，当接收到正确的modbus帧时调用
//}ST_MODBUS_SLAVE_TABLE_ARRY;
//
//
////typedef struct
////{
////    E_MODBUS_MASTER_TABLE_TYPE eProtocolType;
////    ST_REG_MASTER_CFG *pstProtocolTable;
////    ModbusCallBack MasterModbusCallBack;    // 主机回调函数接口，当接收到正确的modbus帧时调用
////}ST_MODBUS_MASTER_TABLE_ARRY;
//
//
//
//typedef struct
//{
//    Uint16 bModbusSlaveAddr     :8;     // 处于从机模式时，自身的地址
////    Uint16 bModbusMasterAddr    :8;     // 处于主机模式时，与从设备通信的地址
////    Uint16 bMode                :1;     // 0:Slave  1:Master
//    Uint16 bChangeState         :1;     // 0:通信点表正等待切换  1:通信点表已完成切换
//    Uint16 bSlaveTableCount     :4;     // 已注册从机通信点表计数
////    Uint16 bMasterTableCount    :4;     // 已注册主机通信点表计数
//    Uint16 bRsv                 :3;     // 保留
//    E_MODBUS_SLAVE_TABLE_TYPE eCurrentSlaveTableIndex;           // 当前从机协议
//    E_MODBUS_SLAVE_TABLE_TYPE eCurrentSlaveTableIndexCache;      // 当前从机协议备份
////    E_MODBUS_MASTER_TABLE_TYPE eCurrentMasterTableIndex;         // 当前主机协议
////    E_MODBUS_MASTER_TABLE_TYPE eCurrentMasterTableIndexCache;    // 当前主机协议备份
//    ST_MODBUS_SLAVE_TABLE_ARRY stSlaveTableArry[MODBUS_SLAVE_TABLE_MAX];    // 从机点表的注册池
////    ST_MODBUS_MASTER_TABLE_ARRY stMasterTableArry[MODBUS_MASTER_TABLE_MAX];    // 主机点表的注册池
//
//
//}ST_MODBUS_TABLE_CRTL;




/**
 * @brief ModbusSlaveHandle
 *        Modbus从站业务调用函数
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param pstMdsData - Modbus从站原始数据接收的缓存结构体
 * @return ST_MDS_FRAME* 应答帧回复的结构体指针，返回的结构体中的ST_MDS_DATA参数指向的是函数入参pstMdsData，
 *                      回复数据存放在其中；
 *                      如果出现广播帧CRC校验错误、帧长度小于5、地址不匹配、入参异常时，会返回NULL
 *  若在接收到数据后且数据解析前切换了通信点表或者通信地址，则会使用新的通信地址或者通信点表进行解析
 *  会出现地址不匹配时忽略该Modbus帧，或者使用新的通信点表的寄存器地址数据进行回复
 */
extern ST_MDS_FRAME* ModbusSlaveHandle(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, ST_MDS_DATA *pstMdsData);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif

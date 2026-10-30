/**
 * @file ModbusSlave.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __MODBUSSLAVE_H
#define __MODBUSSLAVE_H

#include "ModbusRegProcess.h"
#include "F28003x_device.h"

#ifdef __cplusplus
extern "C" {
#endif


// 功能码
typedef enum
{
    MODBUS_READ_CMD           = 0x03,  // 读命令 -- 支持单个和多个寄存器连续读取
    MODBUS_WRITE_SINGLE_CMD   = 0x06,  // 写单个寄存器命令 -- 支持多寄存器连续写动作
    MODBUS_WRITE_MULTI_CMD    = 0x10,  // 写多个寄存器命令 -- 支持单寄存器连续写动作
    MODBUS_MANAGE_CMD         = 0x40,  // 管理命令
}E_NORTH_MODBUS_CMD;

// 错误码类别
typedef enum
{
    DATA_OK             = 0x00,     // 数据处理正常
    ERROR_FUNC          = 0x01,     // 非法功能码
    ERROR_ADDRESS       = 0x02,     // 非法数据地址
    ERROR_DATA          = 0x03,     // 非法数据值
    ERROR_SLAVE_FAULT   = 0x04,     // 从节点设备故障
    MASTER_CONFIRM      = 0x05,     // 主节点接收服务调用，但是需要很长时间完成服务调用，先回复一个确认
    SLAVE_BUSY          = 0x06,     // 从设备忙
    ERROR_DATA_LEN      = 0x07,     // 数据帧长度错误
    ERROR_CRC           = 0x08,     // 数据帧校验错误
    ERROR_FRAME         = 0x09,     // 数据帧错误
}E_MDS_ERROR_TYPE;

typedef struct
{
    Uint16 u16DataLenth;
    Uint16 *pu16DataBuff;
}ST_MDS_DATA;

typedef struct
{
    Uint16 bAddr      :8;         // 地址
    Uint16 bFuncCmd   :8;         // 功能码
    Uint16 bParamSaveFlag   :1;     // 参数是否保存   0:不保存   1:保存
    Uint16 bRsv             :15;    // 保留
    Uint16 u16RegStartAddr;         // 本次读/写的寄存器起始地址
    Uint16 u16RegNums;              // 本次读/写的寄存器个数
    E_MDS_ERROR_TYPE eErrorType;    // 错误码
    ST_MDS_DATA *pstMdsData;        // 回复数据
}ST_MDS_FRAME;

typedef struct
{
    Uint16 u16MdsIsUpdate;              // 0:不升级  1:升级
    Uint16 u16UpdateBaud;               // 当前升级波特率
    Uint16 u16FirmwareType;             // 当前升级固件类型
}ST_MDS_UPDATE_PARAM;

typedef enum
{
    MODBUS_SLAVE_DEFAULT = 0x00,
    MODBUS_SLAVE_TABLE_MAX,
    MODBUS_SLAVE_TABLE_NULL,
}E_MODBUS_SLAVE_TABLE_TYPE;

//typedef enum
//{
//    MODBUS_MASTER_DEFAULT = 0x00,
//    MODBUS_MASTER_TABLE_MAX,
//    MODBUS_MASTER_TABLE_NULL,
//}E_MODBUS_MASTER_TABLE_TYPE;


typedef struct
{
    E_MODBUS_SLAVE_TABLE_TYPE eProtocolType;
    ST_REG_SLAVE_CFG *pstProtocolTable;
    ModbusCallBack SlaveModbusCallBack;     // 从机回调函数接口，当接收到正确的modbus帧时调用
}ST_MODBUS_SLAVE_TABLE_ARRY;


//typedef struct
//{
//    E_MODBUS_MASTER_TABLE_TYPE eProtocolType;
//    ST_REG_MASTER_CFG *pstProtocolTable;
//    ModbusCallBack MasterModbusCallBack;    // 主机回调函数接口，当接收到正确的modbus帧时调用
//}ST_MODBUS_MASTER_TABLE_ARRY;



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
//}ST_MODBUS_SLAVE_TABLE_CRTL;



typedef struct
{
    Uint16 bModbusSlaveAddr         :8;     // 处于从机模式时，自身的地址
    Uint16 bModbusSlaveAddrCache    :8;     // 从机地址备份
    Uint16 bUpdateState             :1;     // 0:参数更新中  1:参数更新完成
    Uint16 bSlaveTableCount         :4;     // 已注册从机通信点表计数
    Uint16 bRsv                     :11;     // 保留
    E_MODBUS_SLAVE_TABLE_TYPE eCurrentSlaveTableIndex;           // 当前从机协议
    E_MODBUS_SLAVE_TABLE_TYPE eCurrentSlaveTableIndexCache;      // 当前从机协议备份
    ST_MODBUS_SLAVE_TABLE_ARRY stSlaveTableArry[MODBUS_SLAVE_TABLE_MAX];    // 从机点表的注册池
    ST_MDS_FRAME stMdsFrame;            // 当前处理帧及回复数据
    ST_MDS_UPDATE_PARAM stMdsUpdateParam;       // 升级参数
    Uint32 u32ReSetTimeCountDown;      // 重启倒计时

}ST_MODBUS_SLAVE_TABLE_CRTL;


typedef struct
{
    Uint16 u16Cmd;
    E_MDS_ERROR_TYPE (*pFunc)(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl);
}ST_CMD_ITEM;




/**
 * @brief ModbusSlaveTableRegister
 *          初始化Modbus从站控制对象，将配置表注册、回调函数、表索引注册进该对象(pstModbusTableCtrl)
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param eTableType - Modbus表索引(表类型、协议类型)
 * @param pstTable  - Modbus寄存器配置表
 * @param pFunc     - Modbus寄存器配置表的回调函数，在处理完对应的Modbus帧后执行
 * @return int32 -  -1：协议数量超限，大于MODBUS_SLAVE_TABLE_MAX
 *                  -2：协议类型eTableType异常
 *                  0：初始化结束
 * 
 */
extern int32 ModbusSlaveTableRegister(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, E_MODBUS_SLAVE_TABLE_TYPE eTableType,
                                      ST_REG_SLAVE_CFG *pstTable, ModbusCallBack pFunc);


/**
 * @brief SetModbusSlaveTableType
 *          切换当前处理的Modbus点表
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param eTableType - Modbus表索引(表类型、协议类型)
 * @return int32 -  -1：入参异常
 *                  -2：该协议点表未注册
 *                  0：函数执行完成，可调用GetResultForSetModbusSlaveTableType接口查看切换结果
 */
extern int32 SetModbusSlaveTableType(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, E_MODBUS_SLAVE_TABLE_TYPE eTableType);


/**
 * @brief GetModbusSlaveTableType
 *          获取Modbus从站当前使用的协议点表
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return E_MODBUS_SLAVE_TABLE_TYPE -  其他值：返回当前使用协议
 *                                      MODBUS_SLAVE_TABLE_NULL：入参错误或者协议点表未注册
 */
extern E_MODBUS_SLAVE_TABLE_TYPE GetModbusSlaveTableType(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);


/**
 * @brief SetModbusSlaveAddr
 *          设置从站Modbus通信地址
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param u16Addr - 等待设置的地址
 */
extern void SetModbusSlaveAddr(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, Uint16 u16Addr);


/**
 * @brief GetModbusSlaveAddr
 *          获取从站Modbus通信地址
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  -1：入参错误
 *                  0-255：通信地址
 */
extern int16 GetModbusSlaveAddr(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);


/**
 * @brief ModbusSlaveParamUpdate
 *          生效SetModbusSlaveAddr()和SetModbusSlaveTableType()的操作
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  -1：入参错误
 *                  0：参数无须更新
 *                  1：参数已更新
 */
extern int16 ModbusSlaveParamUpdate(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);


/**
 * @brief GetResultForParamUpdate
 *          获取SetModbusSlaveAddr()和SetModbusSlaveTableType()是否生效
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  0：参数等待更新
 *                  1：参数已更新
 */
extern int16 GetResultForParamUpdate(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);


/**
 * @brief ModbusSlaveDataHandle
 *          Modbus从机帧解析函数
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param pstMdsData - Modbus从站原始数据接收的缓存结构体
 * @return ST_MDS_FRAME* 应答帧回复的结构体指针，返回的结构体中的ST_MDS_DATA参数指向的是函数入参pstMdsData，
 *                      回复数据存放在其中；
 *                      如果出现广播帧CRC校验错误、帧长度小于5、地址不匹配、入参异常时，会返回NULL
 */
extern ST_MDS_FRAME* ModbusSlaveDataHandle(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, ST_MDS_DATA *pstMdsData);


/**
 * @brief GetParamSaveFlag
 *          获取参数保存的标志，应用层获取该标志来触发参数保存的相关逻辑
 * 
 * @param pstModbusTableCtrl 
 * @return int16 -  1：需要执行参数保存动作
 *                  0：不需要执行参数保存
 */
extern int16 GetParamSaveFlag(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);


/**
 * @brief ClearParamSaveFlag
 *          清除参数保存的标志
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 */
extern void ClearParamSaveFlag(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl);

#ifdef __cplusplus
}
#endif



#endif

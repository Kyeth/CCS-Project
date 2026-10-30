/**
 * @file Hw_I2cDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef __HW_I2CDRIVER_H
 #define __HW_I2CDRIVER_H


#include "RingBuff.h"

 #ifdef __cplusplus
    extern "C" {
#endif




#define MAX_RECV_MSGS   8       // 最大接收消息个数
#define MAX_DATA_LEN    64      // IIC最大传输长度



// I2cId与芯片ID增长顺序相关
typedef enum eSI2cId
{
    I2C_ID0     = 0x00, // 在DSP中对应I2cA，
    I2C_ID1     = 0x01, // 在DSP中对应I2cB
    I2C_ID_MAX  = 0x02,
    I2C_ID_NULL = 0xFF,
}E_I2CID;



typedef enum
{
    I2C_IDLE,
    I2C_READ_BUSY1,
    I2C_READ_BUSY2,
}E_I2C_TRANSCEIVER_STATE;

typedef enum
{
    I2C_PARAM_ERROR,
    I2C_STP_NOP_READY_ERROR,
    I2C_BUS_BUSY_ERROR,
    I2C_READ_BUSY1_ERROR,
    I2C_READ_BUSY2_ERROR,
    I2C_SUCCESS,
}E_I2C_TRANSCEIVER_ERROR;

typedef struct 
{
    Uint16 u16I2cId;
    Uint16 u16TargetAddr;
    Uint32 u32Speed;

}ST_I2C_CFG_ITEM;

typedef struct
{
    ST_I2C_CFG_ITEM *pstI2cCfgItem;
    Uint16 u16ItemNum;
}ST_I2C_CFG;


typedef struct
{
    Uint16 u16State;    // IIC状态
    Uint16 u16SlaveAddr;    // 通信设备地址
    Uint16 u16MemAddr;      // 内存地址
    Uint16 u16DataLen;      // 读写长度
    Uint16 u16DataBuff[14];
}ST_I2C_MSG;




/**
 * @brief 用于初始化单个I2C外设
 * 
 * @param pstI2cCfgItem 入参是一个ST_I2C_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleI2cInit(ST_I2C_CFG_ITEM *pstI2cCfgItem);



/**
 * @brief 用于初始化一组I2C，通常用于参数初始化阶段
 * 
 * @param pstI2cCfg :入参是一组ST_I2C_CFG结构体对象的集合
 */
extern void Hw_I2cInit(ST_I2C_CFG *pstI2cCfg);



/**
 * @brief 提交读/写命令请求，将请求提交到CMD缓存中
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，里面需要填写读取的从机设备地址、目标内存地址、读取方向、BUFF空间等
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际写入请求命令的个数
 */
extern int16 Hw_I2cSubmitRWReuest(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg);



/**
 * @brief 从接收FIFO中取出读出来的I2C消息
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，该消息需要指定对应的消息ID
 * @param u16Size   :指定读取消息的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际读取的个数
 */
extern int16 Hw_I2cRead(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg);



/**
 * @brief 将指定的I2C消息对象写入到发送FIFO中
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，该消息需要填充完整属性
 * @param u16Size   :指定写入消息的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际写入的个数
 */
extern int16 Hw_I2cWrite(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg);



/**
 * @brief I2C外设的控制接口(待实现)
 * 
 * @param u16I2cId :I2C的ID号
 * @param u16Cmd :I2C命令枚举
 * @param vParam :参数
 * @return int16 
 */
extern int16 Hw_I2cCtrl(Uint16 u16I2cId, Uint16 Cmd, void *pvParam);



/**
 * @brief I2C数据接收和发送的状态机
 * 
 * @param u16I2cId :I2C的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :0 结束
 */
extern int16 Hw_IsrI2cTransceiver(Uint16 u16I2cId);


#ifdef __cplusplus
    }
#endif



 #endif

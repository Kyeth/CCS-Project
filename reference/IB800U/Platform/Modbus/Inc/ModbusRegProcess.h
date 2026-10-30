/**
 * @file ModbusRegProcess.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __MODBUSREGPROCESS_H
#define __MODBUSREGPROCESS_H

#include "F28003x_device.h"

#ifdef __cplusplus
extern "C" {
#endif

// 命令帧上限值
#define MAX_REG_NUM     (120u)      // 最大寄存器个数
#define MIN_REG_NUM     (1u)        // 最小寄存器个数
#define CRC_LENGTH      (2u)        // CRC长度

// 读命令格式偏移
#define MDS_RD_ADDR_INDEX           0
#define MDS_RD_CMD_INDEX            1
#define MDS_RD_REG_ST_ADDR_INDEX    2       // MSB  LSB
#define MDS_RD_REGS_NUM_INDEX       4       // MSB  LSB
#define MDS_RD_CRC_INDEX            6       // LSB   MSB

// 读命令回复格式各个长度
#define MDS_RDREPLY_ADDR_LEN            1
#define MDS_RDREPLY_CMD_LEN             1
#define MDS_RDREPLY_BYTE_NUMS_LEN       1
#define MDS_RDREPLY_VAL_ST_ADDR_LEN     2
#define MDS_RDREPLY_CRC_LEN             2

// 读命令回复格式各个偏移
#define MDS_RDREPLY_ADDR_INDEX          0
#define MDS_RDREPLY_CMD_INDEX           1
#define MDS_RDREPLY_BYTE_NUMS_INDEX     2
#define MDS_RDREPLY_VAL_ST_ADDR_INDEX   3   // 回复数据的第一个寄存器地址

// 单寄存器写命令回复格式各个长度
#define MDS_SINGLE_WD_ADDR_LEN          1
#define MDS_SINGLE_WD_CMD_LEN           1
#define MDS_SINGLE_WD_REG_ADDR_LEN      2
#define MDS_SINGLE_WD_REG_VAL_LEN       2
#define MDS_SINGLE_WD_CRC_LEN           2

// 单寄存器写命令回复格式各个偏移
#define MDS_SINGLE_WD_ADDR_INDEX        0
#define MDS_SINGLE_WD_CMD_INDEX         1
#define MDS_SINGLE_WD_REG_ADDR_INDEX    2
#define MDS_SINGLE_WD_REG_VAL_INDEX     4
#define MDS_SINGLE_WD_CRC_INDEX         6

// 多个寄存器写命令格式各个长度
#define MDS_MULTI_WD_ADDR_LEN          1
#define MDS_MULTI_WD_CMD_LEN           1
#define MDS_MULTI_WD_REG_ADDR_LEN      2
#define MDS_MULTI_WD_REG_VAL_LEN       2
#define MDS_MULTI_WD_CRC_LEN           2

// 多个寄存器写命令格式各个偏移
#define MDS_MULTI_WD_ADDR_INDEX        0
#define MDS_MULTI_WD_CMD_INDEX         1
#define MDS_MULTI_WD_REG_ADDR_INDEX    2
#define MDS_MULTI_WD_REGS_NUM_INDEX    4
#define MDS_MULTI_WD_REG_VAL_INDEX     7



typedef void (*ModbusCallBack)(void);       // Modbus回调函数接口


// 寄存器值的读写属性
typedef enum
{
    READ_ONLY   = 0x00,         // 只读
    READ_WRITE,                 // 可读可写
}E_RW_TYPE;


// 保存和恢复类型
typedef enum
{
    NOT_SAVE = 0,       //应用层使用 - 参数不保存到文件
    SAVE_TO_FILE,       //应用层使用
} E_SAVE_RECOVER_TYPE;

// 寄存器对应的值的类型
typedef enum
{
    TYPE_INT8,
    TYPE_UINT8,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32,
    TYPE_UINT32,
    TYPE_FLOAT
}E_VAL_TYPE;


//  联合体：寄存器对应的值
typedef union
{
    unsigned char   u8Val;
    char            i8Val;
    Uint16  u16Val;
    int16   i16Val;
    Uint32  u32Val;
    int32   i32Val;
    float   fVal;
}U_REG_VAL;

// 寄存器结构体定义
typedef struct
{
    Uint16 u16RegAddr;          // 寄存器地址
    Uint16 bRWType          :4; // 寄存器操作属性
    Uint16 bValType         :4; // 寄存器对应值的数据类型
    Uint16 bParamSaveType   :8; // 参数保存属性
    U_REG_VAL *puRegVal;        // 寄存器对应的值的指针
    int32 i32DownLimitVal;      // 寄存器对应的下限值
    Uint32 u32UpLimitVal;       // 寄存器对应的上限值
}ST_REG_SLAVE_ITEM;

// 寄存器配置表结构体定义
typedef struct
{
    ST_REG_SLAVE_ITEM *pstReg;
    Uint16 u16Num;
}ST_REG_SLAVE_CFG;





/**
 * @brief ReadModbusRegisterData
 *          从Modbus寄存器表中读取单个寄存器子项，寄存器表的地址需要保持增序，否则会出现查找不到的情况
 * 
 * @param pstProtocolTable - Modbus寄存器表
 * @param u16RegAddr - 寄存器地址
 * @param pstGetReg - 获取到的单个寄存器子项
 * @return int16 -  -1：入参异常
 *                  -2：超出寄存器地址范围
 *                  -3：未找到有效地址
 *                  0：寄存器地址存在
 */
extern int16 ReadModbusRegisterData(ST_REG_SLAVE_CFG *pstProtocolTable, Uint16 u16RegAddr, ST_REG_SLAVE_ITEM *pstGetReg);


/**
 * @brief SetRegValToMdsMsg
 *          将寄存器数据封装进待发送的消息缓存区
 * 
 * @param pu16DataBuff - 消息缓存区：16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 * @param pstSetRegVal - 寄存器数据
 * @param u16Index - 消息缓存区的地址偏移
 * @return int16 -  -1：入参异常
 *                  1：操作了一个寄存器
 *                  2：操作了两个寄存器
 */
extern int16 SetRegValToMdsMsg(Uint16 *pu16DataBuff, ST_REG_SLAVE_ITEM *pstSetRegVal, Uint16 u16Index);


/**
 * @brief LoadRegValFromMdsMsg
 *          根据值类型(u16ValType)，将原始数据解析存放进临时的寄存器(pstSrcRegVal)
 * 
 * @param pu16DataBuff - 消息缓存区：16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 * @param pstSrcRegVal - 临时的寄存器
 * @param u16ValType - 值类型
 * @param u16RegNo - 获取第u16RegNo个的寄存器
 * @param u16Index - 起始位置偏移
 * @return int16 -  -1：入参异常
 *                  1：操作了一个寄存器
 *                  2：操作了两个寄存器
 */
extern int16 LoadRegValFromMdsMsg(Uint16 *pu16DataBuff, ST_REG_SLAVE_ITEM *pstSrcRegVal, Uint16 u16ValType, Uint16 u16RegNo, Uint16 u16Index);


/**
 * @brief SetRegValByType
 *          寄存器值赋值
 * 
 * @param pstRegDrc - 目标寄存器
 * @param pstRegSrc - 源寄存器
 * @return int16 -  -1：入参异常
 *                  -2：目标寄存器上下限属性配置异常
 *                  -3：目标寄存器值类型配置异常
 *                  0：值超限
 *                  1：成功
 */
extern int16 SetRegValByType(ST_REG_SLAVE_ITEM *pstRegDrc, ST_REG_SLAVE_ITEM *pstRegSrc);

#ifdef __cplusplus
}
#endif /* extern "C" */


#endif

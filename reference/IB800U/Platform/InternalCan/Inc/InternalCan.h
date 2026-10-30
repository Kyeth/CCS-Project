/**
 * @file InternalCan.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __INTERNALCAN_H
#define __INTERNALCAN_H


#include "F28003x_device.h"
#include "RingBuff.h"

#ifdef __cplusplus
    extern "C" {
#endif


#define INTERNAL_CAN_FRAME_MAX  20
#define SINGLE_PROCESS_RX_QUANTITY 5           // 单次接收CAN帧的最大数




// 寄存器对应的值的类型
typedef enum
{
    CAN_TYPE_INT8,
    CAN_TYPE_UINT8,
    CAN_TYPE_INT16,
    CAN_TYPE_UINT16,
    CAN_TYPE_INT32,
    CAN_TYPE_UINT32,
    CAN_TYPE_FLOAT
}E_CAN_VAL_TYPE;


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
}U_CAN_VAL;



typedef union
{
    Uint32 u32All;

    struct
    {
        Uint16 LowWord;
        Uint16 HighWord;
    }Words;

    struct
    {
        Uint16 u8Byte0  :8;
        Uint16 u8Byte1  :8;
        Uint16 u8Byte2  :8;
        Uint16 u8Byte3  :8;
    }Bytes;
}U_INTERNAL_CAN_DATA;


// 应用层CAN数据帧
typedef struct
{
    Uint16 bDLC     :4;         // 数据长度
    Uint16 bIDE     :1;         // 0：标准帧    1：拓展帧
    Uint16 bRTR     :1;         // 0：数据帧    1：远程帧
    Uint16 bRsv1    :10;
    Uint32 bID      :29;        // 帧ID
    Uint32 bRsv2    :3;
    U_INTERNAL_CAN_DATA unDataA;
    U_INTERNAL_CAN_DATA unDataB;
}ST_INTERNAL_CAN_FRAME;


// 对象字典数据结构
typedef struct
{
    Uint16 u16Index;
    Uint16 u16Type;
    U_CAN_VAL *puValue;
}ST_OBJ_DICTIONARY;


// 周期表和触发表数据结构
typedef struct
{
    Uint32 u32FrameID;          // 帧ID
    Uint16 u16DataDlc;          // 待发送数据长度，单位为字节
    Uint16 u16ObjNum;           // 待发送对象索引个数
    Uint16 u16ObjIndex_1;       // 对象索引1
    Uint16 u16ObjIndex_2;
    Uint16 u16ObjIndex_3;
    Uint16 u16ObjIndex_4;
    Uint16 u16ObjIndex_5;
    Uint16 u16ObjIndex_6;
    Uint16 u16ObjIndex_7;
    Uint16 u16ObjIndex_8;
}ST_ITEM_TABLE;

typedef ST_ITEM_TABLE ST_PERIOD_TABLE;
typedef ST_ITEM_TABLE ST_TRIG_TABLE;


typedef struct
{
    ST_OBJ_DICTIONARY *pstObjDictionary;        // 对象字典指针
    ST_PERIOD_TABLE *pstPeriodTable;            // 周期表指针   
    ST_TRIG_TABLE *pstTrigTable;                // 触发表指针
    Uint16 u16ObjDictionaryItemNum;             // 对象字典成员个数
    Uint16 u16PeriodTableItemNum;               // 周期表成员个数
    Uint16 u16TrigTableItemNum;                 // 触发表成员个数
}ST_INTERNAL_CAN_TABLE;


// 内部CAN通信接口回调函数
typedef struct
{
    int16 (*CanRead)(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize);
    int16 (*CanWrite)(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize);
}ST_INTERNAL_CAN_TRANSPORT_INTF;


// 内部CAN通信对象结构体
typedef struct
{
    ST_INTERNAL_CAN_TABLE *pstInterCanTable;        // 内部CAN通信配置表
    ST_INTERNAL_CAN_TRANSPORT_INTF stInterCanTranIntf;      // 通信接口回调函数
    Uint16 u16CanNodeAddr;          // Can节点地址，暂时未使用
    Uint16 u16CanNodeBaud;          // Can通信波特率，暂时未使用

    ST_INTERNAL_CAN_FRAME stInternalCanTxBuff[INTERNAL_CAN_FRAME_MAX];          // TxFrameFifo的空间
    ST_INTERNAL_CAN_FRAME stInternalCanRxBuff[INTERNAL_CAN_FRAME_MAX];          // RxFrameFifo的空间
    Uint32 stTrigRequestBuff[INTERNAL_CAN_FRAME_MAX / 2];                       // TrigRequestFifo的空间


    ST_RING_BUFF stTxFrameFifo;
    ST_RING_BUFF stRxFrameFifo;
    ST_RING_BUFF stTrigRequestFifo;
    Uint16 IsInit;                  // 是否完成初始化
    Uint16 u16PeriodSendIndex;      // 周期发送索引
    Uint16 u16TrigFlag;             // 0:没有触发请求     1:存在触发请求

}ST_INTERNALCAN_HANDLE;



/**
 * @brief 根据帧ID查找周期表中是否存在该ID，存在则返回索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :Can帧，内部存在要查找的帧ID
 * @return int16        : -1 参数非法
 *                      : >=0 周期表索引下标
 */
extern int16 InternalCanRegister(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_TABLE *pstInterCanTable, ST_INTERNAL_CAN_TRANSPORT_INTF *pstTranSportIntf);



/**
 * @brief 调用Can驱动接口将数据存放到接收FIFO中
 *          每次调用最多处理宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16    :-1 参数非法或者未初始化
 *                  :>=0 接收到数据帧的个数
 */
extern int16 InternalCanReceive(ST_INTERNALCAN_HANDLE *pstInternalCanHandle);



/**
 * @brief 从发送FIFO中将CAN数据帧取出，然后调用CAN驱动接口将数据发送到CAN总线上
 *          每次调用最多处理宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16    : -1 参数非法或者未初始化
 *                  : >=0 实际发送数据的个数
 */
extern int16 InternalCanTransmit(ST_INTERNALCAN_HANDLE *pstInternalCanHandle);



/**
 * @brief 对接收的数据帧进行解析
 *          每次调用最多解析宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16        : -1 参数非法或者未初始化
 *                      : >=0 解析数据帧的个数
 */
extern int16 InternalReceiveFrameProcess(ST_INTERNALCAN_HANDLE *pstInternalCanHandle);



/**
 * @brief 封装待发送的数据帧，写入发送FIFO中，每次调用只封装一帧
 * 
 * @param pstInternalCanHandle 
 * @return int 
 */
extern int InternalTransmitFrameProcess(ST_INTERNALCAN_HANDLE *pstInternalCanHandle);

#ifdef __cplusplus
    }
#endif

#endif

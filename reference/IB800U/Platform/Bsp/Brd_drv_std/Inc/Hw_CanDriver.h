/**
 * @file Hw_CanDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_CANDRIVER_H
#define __HW_CANDRIVER_H


#include "RingBuff.h"

#define CAN_BUFF_SIZE   20         // 串口的发送和接收buff大小
#define CAN_HW_FIFO     32         // SCI模块的硬件FIFO大小

#ifdef __cplusplus
extern "C"{
#endif

enum
{
    CANBAUD_1000K = 0x00,
    CANBAUD_800K = 0x01,
    CANBAUD_500K,
    CANBAUD_250K,
    CANBAUD_125K,
};

typedef enum CanId
{
    CAN_ID0     = 0x00,
    CAN_ID_MAX  = 0x01,
    CAN_ID_NULL = 0xFF,
}E_CANID;



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
}U_CAN_DATA;




typedef struct
{
    Uint16 bDLC     :4;         // 数据长度
    Uint16 bIDE     :1;         // 0：标准帧    1：拓展帧
    Uint16 bRTR     :1;         // 0：数据帧    1：远程帧
    Uint16 bRsv1    :10;
    Uint32 bID      :29;        // 帧ID
    Uint32 bRsv2    :3;
    U_CAN_DATA unDataA;
    U_CAN_DATA unDataB;
}ST_CAN_FRAME;



typedef struct
{
    volatile struct CAN_REGS *pstCanRegs;
    Uint32 u32RxFifoValidMask;      // 将fifo邮箱定义为接收时，对应的Bit位需要置1，该变量用于判断是否接收到新数据

    ST_CAN_FRAME stTxBuff[CAN_BUFF_SIZE];
    ST_CAN_FRAME stRxBuff[CAN_BUFF_SIZE];

    ST_RING_BUFF stTxRingBuff;
    ST_RING_BUFF stRxRingBuff;


    Uint16 u16Baud;
    Uint16 u16DeviceBusy;           // 1:繁忙; 0:空闲
    Uint16 bTransmitState   :1;     // 1:发送完成; 0:正在发送
    Uint16 bFrameOver       :1;     // 1:标识接收到一个完整帧; 0:正在接收
    Uint16 bIsInit          :1;     // 1:表示已经初始化完成; 0:未初始化
    Uint16 Rsv              :13;    // 保留

}ST_CAN_HANDLE;


typedef struct
{
    Uint16 bFifoId      :8;             // 指定使用邮箱的编号
    Uint16 bXtd         :1;             // 0:标准帧    1:拓展帧
    Uint16 bDir         :1;             // 0:读        1:写
    Uint16 bDlc         :4;
    Uint16 bRxIE        :1;             // 接收中断使能
    Uint16 bMaskEnable  :1;             // 接收掩码使能
    Uint32 bMask        :29;            // 接收掩码
    Uint32 bEob         :1;             // 0:邮箱组的子邮箱 1:邮箱组的最后一个邮箱
    Uint32 bRsv1        :2;
    Uint32 bARBMsgId    :29;            // 邮箱的接收ID
    Uint32 bRsv2        :3;
}ST_CANFIFO_CFG_ITEM;

typedef struct
{
    ST_CANFIFO_CFG_ITEM *pstCanFifoCfgItem;
    Uint16 u16ItemNum;
}ST_CANFIFO_CFG;


typedef struct
{
    Uint16 u16CanId;
    Uint16 u16Baud;
    Uint32 u32RxFifoValidMask;       // 将fifo邮箱定义为接收时，对应的Bit位需要置1，该变量用于判断是否接收到新数据
    ST_CANFIFO_CFG *pstCanFifoCfg;

}ST_CAN_CFG_ITEM;


typedef struct
{
    ST_CAN_CFG_ITEM *pstCanCfgItem;
    Uint16 u16ItemNum;

}ST_CAN_CFG;



/**
 * @brief 用于初始化单个CAN外设
 * 
 * @param pstCanCfgItem 入参是一个ST_CAN_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleCanInit(ST_CAN_CFG_ITEM *pstCanCfgItem);



/**
 * @brief 用于初始化一组CAN，通常用于参数初始化阶段
 * 
 * @param pstCanCfg :入参是一组ST_CAN_CFG结构体对象的集合
 */
extern void Hw_CanInit(ST_CAN_CFG *pstCanCfg);



/**
 * @brief 从接收FIFO中取出指定数据个数的CAN帧
 * 
 * @param u16CanId :CAN的ID号
 * @param pstCanFrame :ST_CAN_FRAME结构体指针，可以是一个结构体数组指针，也可以是单个结构体指针
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstCanFrame参数非法
 *                  :-4 CAN设备处于繁忙状态，正被其他线程使用
 */
extern int16 Hw_CanRead(Uint16 u16CanId, ST_CAN_FRAME *pstCanFrame, Uint16 u16Size);



/**
 * @brief 将指定个数的CAN帧写入发送FIFO中
 * 
 * @param u16CanId :CAN的ID号
 * @param pstCanFrame :ST_CAN_FRAME结构体指针，可以是一个结构体数组指针，也可以是单个结构体指针
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstCanFrame参数非法
 *                  :-4 CAN设备处于繁忙状态，正被其他线程使用
 */
extern int16 Hw_CanWrite(Uint16 u16CanId, ST_CAN_FRAME *pstCanFrame, Uint16 u16Size);



/**
 * @brief CAN外设的控制接口(待实现)
 * 
 * @param u16CanId :CAN的ID号
 * @param u16Cmd  :CAN命令枚举
 * @param vParam :参数
 * @return int 
 */
extern Hw_CanCtrl(Uint16 u16CanId, Uint16 u16Cmd, void *vParam);



/**
 * @brief 将CAN总线上的数据接收到FIFO中，通常在中断中调用
 * 
 * @param u16CanId :CAN的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际接收到的数据
 */
extern int16 Hw_IsrCanReceive(Uint16 u16CanId);



/**
 * @brief 将FIFO中的数据发送到CAN总线上，通常在中断中调用
 * 
 * @param u16CanId :CAN的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际发送的数据
 */
extern int16 Hw_IsrCanTransmit(Uint16 u16CanId);


#ifdef __cplusplus
}
#endif


#endif

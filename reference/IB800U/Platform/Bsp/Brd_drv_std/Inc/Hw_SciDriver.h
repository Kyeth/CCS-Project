/**
 * @file Hw_SciDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_SCIDRIVER_H
#define __HW_SCIDRIVER_H

#include "F28003x_device.h"
#include "RingBuff.h"
//#include "F2837sS_sci.h"

#ifdef __cplusplus
    extern "C" {
#endif


#define FRAME_TIMEOUT   4           // 这个为断帧超时时间，总时间 = FRAME_TIMEOUT * 控制中断的频率，超时时间不得小于4bit的时间
#define SCI_BUFF_SIZE   1030        // 串口的发送和接收buff大小
#define SCI_HW_FIFO     16          // SCI模块的硬件FIFO大小


// 和芯片相关的枚举类型会在设备抽象层将其提炼出去，统一对应用曾做归一化枚举
// 目前暂未对驱动做抽象

// Bit0:DISABLE/ENABLE
// Bit1:Odd Parity/Even Parity
typedef enum eParity
{
    DISABLE_PARITY = 0x00,
    ODD_PARITY = 0x01,
    EVEN_PARITY = 0x03,
}E_PARITY;

typedef enum eStopBits
{
    STOPBITS_1 = 0x00,
    STOPBITS_2 = 0x01,
}E_STOPBITS;

typedef enum eDataBits
{
    DATABITS_1 = 0x00,
    DATABITS_2 = 0x01,
    DATABITS_3 ,
    DATABITS_4 ,
    DATABITS_5 ,
    DATABITS_6 ,
    DATABITS_7 ,
    DATABITS_8 ,
}E_DATABITS;

// SciId与芯片ID增长顺序相关
typedef enum eSciId
{
    SCI_ID0     = 0x00, // 在DSP中对应SciA，GD中对应Uart0，ST中对应Uart1
    SCI_ID1     = 0x01, // 在DSP中对应SciB
    SCI_ID_MAX  = 0x02,
    SCI_ID_NULL = 0xFF,
}E_SCIID;

typedef struct
{
    Uint32 u32BaudRate;
    Uint16 b4DataBits   :4;
    Uint16 b2StopBits   :2;
    Uint16 b2Parity     :2;
    Uint16 bRsv         :8;
}ST_SCI_COMM_CFG;


typedef struct
{
    Uint16 u16SciId;
    Uint32 u32BaudRate;
    Uint16 b4DataBits   :4;
    Uint16 bStopBits    :2;
    Uint16 bParity      :2;
    Uint16 bTxMode      :4;
    Uint16 bRxMode      :4;
    Uint16 bRs485Pin    :8;
    Uint16 bRsv         :8;
}ST_SCI_CFG_ITEM;


typedef enum
{
    SCI_COMM_CFG,
}E_SCI_PARAM;

typedef struct
{
    ST_SCI_CFG_ITEM *pstSciCfgItem;
    Uint16 u16ItemNum;
}ST_SCI_CFG;

typedef struct
{
    volatile struct SCI_REGS *pstSciRegs;
    ST_SCI_COMM_CFG stSciCommCfg;
    // 嵌入式平台不推荐使用malloc分配内存，定义一个FIFO数组给到下面的RingBuff来管理
    // 用户不可直接操作该Buff
    Uint16 u16TxBuff[SCI_BUFF_SIZE];
    Uint16 u16RxBuff[SCI_BUFF_SIZE];
    // 设置FIFO的大小
    ST_RING_BUFF stTxRingBuff;
    ST_RING_BUFF stRxRingBuff;
    Uint16 u16FrameTimeOutCount;    // 断帧时间计数
    Uint16 u16DeviceBusy;           // 1:繁忙; 0:空闲
    Uint16 bTransmitState   :1;     // 1:发送完成; 0:正在发送
    Uint16 bFrameOver       :1;     // 1:标识接收到一个完整帧; 0:正在接收
    Uint16 bIsInit          :1;     // 1:表示已经初始化完成; 0:未初始化
    Uint16 bFrameStart      :1;     // 检测到帧开始
    Uint16 Rsv              :12;    // 保留

}ST_SCI_HANDLE;



/**
 * @brief 用于初始化单个SCI外设
 * 
 * @param pstSciCfgItem 入参是一个ST_SCI_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleSciInit(ST_SCI_CFG_ITEM* pstSciCfgItem);



/**
 * @brief 用于初始化一组SCI，通常用于参数初始化阶段
 * 
 * @param pstSciCfg :入参是一组ST_SCI_CFG结构体对象的集合
 */
extern void  Hw_SciInit(ST_SCI_CFG *pstSciCfg);



/**
 * @brief 从接收FIFO中取出指定数据个数的字节
 * 
 * @param u16SciId :SCI的ID号
 * @param u16RxBuff :数组的接收缓存
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 u16TxBuff参数为NULL
 *                  :-4 SCI设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际读取的个数
 */
extern int16 Hw_SciRead(Uint16 u16SciId, Uint16 *u16RxBuff, Uint16 u16Size);



/**
 * @brief 将指定个数的字节写入发送FIFO中
 * 
 * @param u16SciId :SCI的ID号
 * @param u16TxBuff :存放待写入数据的数组
 * @param u16Size :指定写入数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 u16TxBuff参数为NULL
 *                  :-4 SCI设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实现写入的个数
 */
extern int16 Hw_SciWrite(Uint16 u16SciId, Uint16 *u16TxBuff, Uint16 u16Size);



/**
 * @brief SCI外设的控制接口(待实现)
 * 
 * @param u16SciId :SCI的ID号
 * @param u16Cmd :SCI命令枚举
 * @param vParam :参数
 * @return int16 
 */
extern int16 Hw_SciCtrl(Uint16 u16SciId, Uint16 u16Cmd, void *vParam);



/**
 * @brief 将SCI总线上的数据接收到FIFO中，通常在中断中调用
 * 
 * @param u16SciId :SCI的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际接收到的数据
 */
extern int16 Hw_IsrSciReceive(Uint16 u16SciId);



/**
 * @brief 将FIFO中的数据发送到SCI总线上，通常在中断中调用
 * 
 * @param u16SciId :SCI的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  : >=0 本次发送数据
 */
extern int16 Hw_IsrSciTransmit(Uint16 u16SciId);


#ifdef __cplusplus
    }
#endif

#endif

/**
 * @file Hw_SciDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS SCI驱动头文件
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef HW_SCI_DRIVER_H
#define HW_SCI_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SCI模块编号：F2837xS共4个SCI模块，本板使用A/C/D三个 */
typedef enum
{
    HW_SCI_ID_A = 0U,           /* SCIA：GPIO48(TX)/GPIO49(RX) */
    HW_SCI_ID_B,                /* SCIB：本板未使用 */
    HW_SCI_ID_C,                /* SCIC：GPIO90(TX)/GPIO89(RX)，与外部TVS通信 */
    HW_SCI_ID_D,                /* SCID：GPIO93(TX)/GPIO94(RX) */
    HW_SCI_ID_MAX,
    HW_SCI_ID_NULL = 0xFFU
} E_HW_SCI_ID;

/* 字符长度，取值即为SCICCR.SCICHAR的值，实际位数为该值加1 */
#define HW_SCI_CHAR_LEN_1BIT            (0U)
#define HW_SCI_CHAR_LEN_2BIT            (1U)
#define HW_SCI_CHAR_LEN_3BIT            (2U)
#define HW_SCI_CHAR_LEN_4BIT            (3U)
#define HW_SCI_CHAR_LEN_5BIT            (4U)
#define HW_SCI_CHAR_LEN_6BIT            (5U)
#define HW_SCI_CHAR_LEN_7BIT            (6U)
#define HW_SCI_CHAR_LEN_8BIT            (7U)

/* 停止位，取值即为SCICCR.STOPBITS的值 */
#define HW_SCI_STOP_BITS_1              (0U)
#define HW_SCI_STOP_BITS_2              (1U)

/* 校验方式，不是寄存器原值，驱动内部会拆成PARITYENA与PARITY两个位 */
#define HW_SCI_PARITY_NONE              (0U)
#define HW_SCI_PARITY_ODD               (1U)
#define HW_SCI_PARITY_EVEN              (2U)

/* FIFO深度，F2837xS的SCI收发FIFO各16级 */
#define HW_SCI_FIFO_DEPTH               (16U)

/* 一个SCI模块的配置项
 * u32Baudrate  目标波特率，驱动按LSPCLK自动算BRR，超出误差范围会返回失败
 * u16RxFifoLevel 接收FIFO触发中断的电平（1~16），只在bRxIntEnable=1时有效 */
typedef struct
{
    Uint16 u16SciId;                /* 模块编号，E_HW_SCI_ID */
    Uint32 u32Baudrate;             /* 波特率，如115200 */
    Uint16 u16CharLen;              /* 字符长度，HW_SCI_CHAR_LEN_xxx */
    Uint16 u16StopBits;             /* 停止位，HW_SCI_STOP_BITS_xxx */
    Uint16 u16Parity;               /* 校验方式，HW_SCI_PARITY_xxx */
    Uint16 bLoopback;               /* 1:内部回环（自测用） 0:正常 */
    Uint16 bFifoEnable;             /* 1:使能16级FIFO 0:单字节缓冲模式 */
    Uint16 u16RxFifoLevel;          /* 接收FIFO中断电平 */
    Uint16 bRxIntEnable;            /* 1:使能接收FIFO中断 */
} ST_SCI_CFG_ITEM;

/* 全部SCI模块的配置表 */
typedef struct
{
    const ST_SCI_CFG_ITEM *pstSciCfgItem;
    Uint16 u16ItemNum;
} ST_SCI_CFG;

/* 初始化配置表中的全部SCI模块 */
void Hw_SciInit(const ST_SCI_CFG *pstSciCfg);

/* 初始化单个SCI模块，Hw_SciInit内部也调用它 */
void Hw_SciSingleInit(const ST_SCI_CFG_ITEM *pstSciCfgItem);

/* 发送一个字节（阻塞等待发送缓冲空），返回1表示成功 */
Uint16 Hw_SciSendByte(Uint16 u16SciId, Uint16 u16Data);

/* 发送一段数据（阻塞），返回实际发送的字节数 */
Uint16 Hw_SciSendBuffer(Uint16 u16SciId, const Uint16 *pu16Buf, Uint16 u16Len);

/* 查询接收FIFO中已有的字节数 */
Uint16 Hw_SciGetRxCount(Uint16 u16SciId);

/* 读取一个字节，返回1表示读到数据，0表示当前无数据 */
Uint16 Hw_SciReadByte(Uint16 u16SciId, Uint16 *pu16Data);

/* 读取一段数据，返回实际读到的字节数 */
Uint16 Hw_SciReadBuffer(Uint16 u16SciId, Uint16 *pu16Buf, Uint16 u16Len);

/* 清空接收FIFO，返回1表示成功 */
Uint16 Hw_SciClearRxFifo(Uint16 u16SciId);

/* 查询接收FIFO溢出标志，返回1表示发生过溢出（查到后驱动自动清除标志） */
Uint16 Hw_SciGetRxOverflow(Uint16 u16SciId);

/* 查询接收错误标志（PE/OE/FE/BRKDT任一置位即返回1），读SCIRXST会自动清标志 */
Uint16 Hw_SciGetRxError(Uint16 u16SciId);

#ifdef __cplusplus
}
#endif

#endif /* HW_SCI_DRIVER_H */

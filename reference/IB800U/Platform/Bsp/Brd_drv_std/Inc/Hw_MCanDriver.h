#ifndef __HW_MCANDRIVER_H
#define __HW_MCANDRIVER_H

#include "F28003x_device.h"
#include "RingBuff.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define MCAN_BUFF_SIZE   20         // 串口的发送和接收buff大小
#define MCAN_HW_FIFO     32         // SCI模块的硬件FIFO大小


enum
{
    MCANBAUD_1000K = 0x00,
    MCANBAUD_800K = 0x01,
    MCANBAUD_500K,
    MCANBAUD_250K,
    MCANBAUD_125K,
};

typedef enum MCanId
{
    MCAN_ID0     = 0x00,
    MCAN_ID_MAX  = 0x01,
    MCAN_ID_NULL = 0xFF,
}E_MCANID;


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
}U_MCAN_DATA;


typedef struct
{
    Uint16 bDLC     :4;         // 数据长度
    Uint16 bIDE     :1;         // 0：标准帧    1：拓展帧
    Uint16 bRTR     :1;         // 0：数据帧    1：远程帧
    Uint16 bRsv1    :10;
    Uint32 bID      :29;        // 帧ID
    Uint32 bRsv2    :3;
    U_MCAN_DATA unDataA;
    U_MCAN_DATA unDataB;
}ST_MCAN_FRAME;


typedef struct
{
    volatile struct MCANA_REGS *pstMCanRegs;
    Uint32 u32RxFifoValidMask;      // 将fifo邮箱定义为接收时，对应的Bit位需要置1，该变量用于判断是否接收到新数据

    ST_MCAN_FRAME stTxBuff[MCAN_BUFF_SIZE];
    ST_MCAN_FRAME stRxBuff[MCAN_BUFF_SIZE];

    ST_RING_BUFF stTxRingBuff;
    ST_RING_BUFF stRxRingBuff;


    Uint16 u16Baud;
    Uint16 u16DeviceBusy;           // 1:繁忙; 0:空闲
    Uint16 bTransmitState   :1;     // 1:发送完成; 0:正在发送
    Uint16 bFrameOver       :1;     // 1:标识接收到一个完整帧; 0:正在接收
    Uint16 bIsInit          :1;     // 1:表示已经初始化完成; 0:未初始化
    Uint16 Rsv              :13;    // 保留

}ST_MCAN_HANDLE;


typedef struct
{
    Uint32 bMask        :29;            // 接收掩码
    Uint32 bRsv1        :3;
    Uint32 bARBMsgId    :29;            // 邮箱的接收ID
    Uint32 bRsv2        :3;
}ST_MCAN_EXT_FILTER;


typedef struct
{
    Uint32 bMask        :29;            // 接收掩码
    Uint32 bRsv1        :3;
    Uint32 bARBMsgId    :29;            // 邮箱的接收ID
    Uint32 bRsv2        :3;
}ST_MCAN_STD_FILTER;


typedef struct
{
    Uint16 u16MCanId;
    Uint16 u16Baud;
    ST_MCAN_EXT_FILTER *pstMCanExtFilter;
    Uint16 u16ExtFilterNum;
    ST_MCAN_STD_FILTER *pstMCanStdFilter;
    Uint16 u16StdFilterNum;


}ST_MCAN_CFG_ITEM;


typedef struct
{
    ST_MCAN_CFG_ITEM *pstMCanCfgItem;
    Uint16 u16ItemNum;

}ST_MCAN_CFG;


extern void Hw_SingleMCanInit(ST_MCAN_CFG_ITEM *pstMCanCfgItem);

extern void Hw_MCanInit(ST_MCAN_CFG *pstMCanCfg);

extern int16 Hw_MCanRead(Uint16 u16MCanId, ST_MCAN_FRAME *pstMCanFrame, Uint16 u16Size);

extern int16 Hw_MCanWrite(Uint16 u16MCanId, ST_MCAN_FRAME *pstMCanFrame, Uint16 u16Size);

extern int16 Hw_IsrMCanReceive(Uint16 u16MCanId);

extern int16 Hw_IsrMCanTransmit(Uint16 u16MCanId);

extern void Hw_ClearMCanBusError(Uint16 u16MCanId);

extern void test(void);

extern ST_MCAN_HANDLE g_stMCanHandle[MCAN_ID_MAX] ;

#ifdef __cplusplus
    }
#endif


#endif

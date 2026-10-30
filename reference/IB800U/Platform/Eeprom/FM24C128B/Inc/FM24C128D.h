#ifndef __FM24C128D_H
#define __FM24C128D_H

#include "F28003x_device.h"

#ifdef __cplusplus
    extern "C" {
#endif


#define FM24C128D_SIZE          16384   // 16K字节
#define FM24C128D_PAGE_SIZE     64      // 页大小(字节)
#define FM24C128D_WRITE_DELAY   5       // 写周期延迟(5ms)

#define FM24C128D_DATA_ADDR         0x50    // bit6~bit3默认为1010,bit3~bit1默认为0
#define FM24C128D_UID_ADDR          0x58

typedef struct
{
    Uint16 bDir     :1;     // 0:写操作    1:读操作
    Uint16 bAddr    :10;    // 从机的设备地址
    Uint16 bMsgId   :5;     // 消息唯一标识符
    Uint16 u16DataLength;   // 读写目标长度
    Uint32 u32WrAddr;       // 存储介质的读写地址
    Uint16 u16TargetBuff[FM24C128D_PAGE_SIZE];     // 数据空间,必须是全局变量
}ST_FM24_I2C_MSG;


typedef struct st_FM24128D_Handle ST_FM24C128D_HANDLE;

typedef struct
{
    int16 (*EepromRead)(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen);
    int16 (*EepromWrite)(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen);
    int16 (*EepromGetTick)(void);
}ST_FM24C128D_INTF;


typedef struct
{
    Uint16 u16TargetAddr;
    Uint16 *pu16Buff;
    Uint16 u16Len;
    Uint16 u16Dir;          // 0:write 1:read
}ST_FMC24128D_CMD;


struct st_FM24128D_Handle
{
    // I2c读写接口
    ST_FM24C128D_INTF stFM24C128DIntf;     // 读写接口
    Uint16 u16Addr;                 // 地址配置
    Uint16 u16WRPort;               // 读写使能引脚   1:读     0:写
    Uint16 IsInit;                  // 是否初始化
};

extern ST_FM24C128D_HANDLE stFM24C128DHandle;



extern void FM24C128DEnableWriteProtection(ST_FM24C128D_HANDLE *pstFM24C128DHandle);


extern void FM24C128DDisableWriteProtection(ST_FM24C128D_HANDLE *pstFM24C128DHandle);


extern int16 FM24C128DRegister(ST_FM24C128D_HANDLE *pstFM24C128DHandle, ST_FM24C128D_INTF *pstFM24C128DRwIntf, Uint16 u16Port, Uint16 u16Addr);

//extern int16 FM24C128DWriteByte(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 u16Data);

//extern int16 FM24C128DReadByte(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *pu16Data);

extern int16 FM24C128DReadUID(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 *pu16Data);

//extern int16 FM24C128DBlockRead(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen);
//
//extern int16 FM24C128DBlockWrite(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen);

extern int16 FM24C128DWrite(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen);

extern int16 FM24C128DRead(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen);

#ifdef __cplusplus
    }
#endif

#endif

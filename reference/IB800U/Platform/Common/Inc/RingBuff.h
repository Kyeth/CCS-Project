#ifndef __RINGBUFF_H
#define __RINGBUFF_H

// 这里包含了对变量类型的自定义，后续可以对其进行修改，改成通用命名类型
#include "F28003x_device.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    void *pvBuff;         // 环形缓存区
    Uint16 u16Size;         // 环形缓存区大小
    Uint16 u16In;           // 输入位置指针
    Uint16 u16Out;          // 输出位置指针
    Uint16 u16ElementSize;  // 单个元素大小
}ST_RING_BUFF;

extern int32 InitRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16ElementSize, Uint16 u16MaxLen);


extern int32 ClearRingBuff(ST_RING_BUFF *pstRingBuff);


extern int32 GetRingBuffDataLen(ST_RING_BUFF *pstRingBuff);


extern int32 WriteRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16Len);


extern int32 ReadRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16Len);

#ifdef __cplusplus
}
#endif


#endif

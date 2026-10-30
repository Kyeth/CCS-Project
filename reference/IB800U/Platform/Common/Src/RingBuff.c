#include "RingBuff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define min(a,b) (( (a) < (b) )? (a):(b))

int32 InitRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16ElementSize, Uint16 u16MaxLen)
{
    if (NULL == pstRingBuff || NULL == pvBuff)
    {
        return -1;
    }

    pstRingBuff->pvBuff = pvBuff;
    pstRingBuff->u16Size = u16MaxLen;
    pstRingBuff->u16In = pstRingBuff->u16Out = 0;
    pstRingBuff->u16ElementSize = u16ElementSize;

    return 1;
}

int32 ClearRingBuff(ST_RING_BUFF *pstRingBuff)
{
    if (NULL == pstRingBuff)
    {
        return -1;
    }

    pstRingBuff->u16In = 0;
    pstRingBuff->u16Out = 0;

    return 1;
}

int32 GetRingBuffDataLen(ST_RING_BUFF *pstRingBuff)
{
    if (NULL == pstRingBuff)
    {
        return -1;
    }

    return ((pstRingBuff->u16Size - pstRingBuff->u16Out + pstRingBuff->u16In) % pstRingBuff->u16Size);
}

#pragma CODE_SECTION(WriteRingBuff, ".ISR.ramfunc");
int32 WriteRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16Len)
{
    unsigned char *pu8Target = NULL;
    unsigned char *pu8Src = NULL;
    Uint16 u16Remainder = 0;
    Uint16 u16SpaceUntilEnd = 0;
    Uint16 u16FirstWriteLen  = 0;
    Uint16 u16SecondWriteLen = 0;

//    Uint16 u16In = pstRingBuff->u16In;
//    Uint16 u16Out = pstRingBuff->u16Out;
//    Uint16 u16Size = pstRingBuff->u16Size;
//    Uint16 u16ElementSize = pstRingBuff->u16ElementSize;


    if (NULL == pstRingBuff || NULL == pvBuff)
    {
        return -1;
    }

    // 计算当前缓存区剩余空间（缓存区最大空间为u32Size - 1）
    u16Remainder = (pstRingBuff->u16Size + pstRingBuff->u16Out - pstRingBuff->u16In -1) % pstRingBuff->u16Size;

    // 如果剩余长度为0直接返回
    if ((u16Remainder == 0) || (u16Len == 0))
    {
        return 0;
    }
    // 得到实际写入长度，避免循环覆盖
    u16Len = min(u16Len , u16Remainder);

    // In < Out的情况，直接写入即可
    if (pstRingBuff->u16In < pstRingBuff->u16Out)
    {
        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (pstRingBuff->u16In * pstRingBuff->u16ElementSize);
        memcpy(pu8Target, pvBuff, u16Len * pstRingBuff->u16ElementSize);
    }
    else    // In >= Out
    {
        // 计算第一次写入和第二次写入的长度
        u16SpaceUntilEnd = pstRingBuff->u16Size - pstRingBuff->u16In;
        u16FirstWriteLen = (u16Len <= u16SpaceUntilEnd) ? u16Len : u16SpaceUntilEnd;
        u16SecondWriteLen = u16Len - u16FirstWriteLen;

        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (pstRingBuff->u16In * pstRingBuff->u16ElementSize);
        memcpy(pu8Target, pvBuff, u16FirstWriteLen * pstRingBuff->u16ElementSize);

        pu8Src = (unsigned char *)pvBuff + (u16FirstWriteLen * pstRingBuff->u16ElementSize);

        memcpy(pstRingBuff->pvBuff, pu8Src, u16SecondWriteLen * pstRingBuff->u16ElementSize);
    }

    // 更新索引
    pstRingBuff->u16In = (pstRingBuff->u16In + u16Len) % pstRingBuff->u16Size;

//    // 计算当前缓存区剩余空间（缓存区最大空间为u32Size - 1）
//    u16Remainder = (u16Size + u16Out - u16In -1) % u16Size;
//
//    // 如果剩余长度为0直接返回
//    if ((u16Remainder == 0) || (u16Len == 0))
//    {
//        return 0;
//    }
//    // 得到实际写入长度，避免循环覆盖
//    u16Len = min(u16Len , u16Remainder);
//
//    // In < Out的情况，直接写入即可
//    if (u16In < u16Out)
//    {
//        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (u16In * u16ElementSize);
//        memcpy(pu8Target, pvBuff, u16Len * u16ElementSize);
//    }
//    else    // In >= Out
//    {
//        // 计算第一次写入和第二次写入的长度
//        u16SpaceUntilEnd = u16Size - u16In;
//        u16FirstWriteLen = (u16Len <= u16SpaceUntilEnd) ? u16Len : u16SpaceUntilEnd;
//        u16SecondWriteLen = u16Len - u16FirstWriteLen;
//
//        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (u16In * u16ElementSize);
//        memcpy(pu8Target, pvBuff, u16FirstWriteLen * u16ElementSize);
//
//        pu8Src = (unsigned char *)pvBuff + (u16FirstWriteLen * u16ElementSize);
//
//        memcpy(pstRingBuff->pvBuff, pu8Src, u16SecondWriteLen * u16ElementSize);
//    }
//
//    // 更新索引
//    pstRingBuff->u16In = (u16In + u16Len) % u16Size;


    return u16Len;
}

#pragma CODE_SECTION(ReadRingBuff, ".ISR.ramfunc");
int32 ReadRingBuff(ST_RING_BUFF *pstRingBuff, void *pvBuff, Uint16 u16Len)
{
    unsigned char *pu8Target = NULL;
    unsigned char *pu8Src = NULL;
    Uint16 u16Remainder = 0;
    Uint16 u16SpaceUntilEnd = 0;
    Uint16 u16FirstWriteLen  = 0;
    Uint16 u16SecondWriteLen = 0;

    if (NULL == pstRingBuff || NULL == pvBuff)
    {
        return -1;
    }

    // 计算数据长度
    u16Remainder = (pstRingBuff->u16Size - pstRingBuff->u16Out + pstRingBuff->u16In) % pstRingBuff->u16Size;

    // 如果剩余长度为0直接返回
    if ((u16Remainder == 0) || (u16Len == 0))
    {
        return 0;
    }

    u16Len = min(u16Len , u16Remainder);

    // Out < In的情况，直接写入即可
    if (pstRingBuff->u16Out < pstRingBuff->u16In)
    {
        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (pstRingBuff->u16Out * pstRingBuff->u16ElementSize);
        memcpy(pvBuff, pu8Target, u16Len * pstRingBuff->u16ElementSize);
    }
    else    // Out >= In
    {
        // 计算第一次写入和第二次写入的长度
        u16SpaceUntilEnd = pstRingBuff->u16Size - pstRingBuff->u16Out;
        u16FirstWriteLen = (u16Len <= u16SpaceUntilEnd) ? u16Len : u16SpaceUntilEnd;
        u16SecondWriteLen = u16Len - u16FirstWriteLen;

        pu8Target = (unsigned char *)pstRingBuff->pvBuff + (pstRingBuff->u16Out * pstRingBuff->u16ElementSize);
        memcpy(pvBuff, pu8Target, u16FirstWriteLen * pstRingBuff->u16ElementSize);

        pu8Src = (unsigned char *)pvBuff + (u16FirstWriteLen * pstRingBuff->u16ElementSize);
        memcpy(pu8Src, pstRingBuff->pvBuff, u16SecondWriteLen * pstRingBuff->u16ElementSize);
    }

    // 更新索引
    pstRingBuff->u16Out = (pstRingBuff->u16Out + u16Len) % pstRingBuff->u16Size;

    return u16Len;
}

#ifdef __cplusplus
}
#endif

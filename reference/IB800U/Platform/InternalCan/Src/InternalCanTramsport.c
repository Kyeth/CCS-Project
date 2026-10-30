/**
 * @file InternalCanTramsport.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "InternalCanTramsport.h"
#include "Hw_CanDriver.h"


#ifdef __cplusplus
    extern "C" {
#endif



/**
 * @brief 内部CAN通信读取回调接口实现
 * 
 * @param pstCanFrame   :应用层CAN数据帧对象指针
 * @param u16ReadSize   :读取的最大长度
 * @return int16    :>=0 已读取的长度
 */
int16 InternalCanReadTramsport(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize)
{
    ST_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    Uint16 i = 0;
    Uint16 i16ReadNum = 0;


    if (u16ReadSize > SINGLE_PROCESS_RX_QUANTITY)
    {
        u16ReadSize = SINGLE_PROCESS_RX_QUANTITY;
    }
    i16ReadNum = Hw_CanRead(CAN_ID0, &stCanFrame[0], u16ReadSize);

    for (i = 0; i < i16ReadNum; i++)
    {
        pstCanFrame[i].bDLC = stCanFrame[i].bDLC;
        pstCanFrame[i].bID = stCanFrame[i].bID;
        pstCanFrame[i].bIDE = stCanFrame[i].bIDE;
        pstCanFrame[i].bRTR = stCanFrame[i].bRTR;
        pstCanFrame[i].unDataA.u32All = stCanFrame[i].unDataA.u32All;
        pstCanFrame[i].unDataB.u32All = stCanFrame[i].unDataB.u32All;
    }

    return i16ReadNum;
}



/**
 * @brief 内部CAN通信写入回调接口实现
 * 
 * @param pstCanFrame   :应用层CAN数据帧对象指针
 * @param u16WriteSize  :写入的最大长度
 * @return int16    :>=0 已写入的数据长度
 */
int16 InternalCanWriteTramsport(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16WriteSize)
{

    ST_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    Uint16 i = 0;
    Uint16 i16Ret = 0;

    if (u16WriteSize > SINGLE_PROCESS_RX_QUANTITY)
    {
        u16WriteSize = SINGLE_PROCESS_RX_QUANTITY;
    }

    for (i = 0; i < u16WriteSize; i++)
    {
        stCanFrame[i].bDLC = pstCanFrame[i].bDLC;
        stCanFrame[i].bID = pstCanFrame[i].bID;
        stCanFrame[i].bIDE = pstCanFrame[i].bIDE;
        stCanFrame[i].bRTR = pstCanFrame[i].bRTR;
        stCanFrame[i].unDataA.u32All = pstCanFrame[i].unDataA.u32All;
        stCanFrame[i].unDataB.u32All = pstCanFrame[i].unDataB.u32All;
    }
    i16Ret = Hw_CanWrite(CAN_ID0, &stCanFrame[0], u16WriteSize);
    return i16Ret;
}

#ifdef __cplusplus
}
#endif

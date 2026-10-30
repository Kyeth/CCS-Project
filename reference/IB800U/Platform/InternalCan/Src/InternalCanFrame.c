/**
 * @file InternalCanFrame.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "InternalCanFrame.h"


#ifdef __cplusplus
    extern "C" {
#endif



/**
 * @brief 根据帧ID查找到在周期表中的索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :应用层CAN帧，内部的ID需要有效
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未找到下标，帧ID不存在周期表中
 *                  : >=0 数组下标
 */
int16 SearchPeriodTableIndex(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_FRAME *pstCanFrame)
{
    int16 i = 0;

    int16 iRet = -2;
    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0) || (pstCanFrame == NULL))
    {
        return -1;
    }

    iRet = -2;
    for (i = 0; i < pstInternalCanHandle->pstInterCanTable->u16PeriodTableItemNum; i++)
    {
        if (pstCanFrame->bID == pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i].u32FrameID)
        {
            iRet = i;
            break;
        }
    }

    return iRet;
}



/**
 * @brief 根据帧ID查找到在触发表中的索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :应用层CAN帧，内部的ID需要有效
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未找到下标，帧ID不存在周期表中
 *                  : >=0 数组下标
 */
int16 SearchTrigTableIndex(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_FRAME *pstCanFrame)
{
    int16 i = 0;

    int16 iRet = -2;
    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0) || (pstCanFrame == NULL))
    {
        return -1;
    }

    iRet = -2;
    for (i = 0; i < pstInternalCanHandle->pstInterCanTable->u16TrigTableItemNum; i++)
    {
        if (pstCanFrame->bID == pstInternalCanHandle->pstInterCanTable->pstTrigTable[i].u32FrameID)
        {
            iRet = i;
            break;
        }
    }

    return iRet;
}



/**
 * @brief 根据数组索引下标从周期表中取出对应的子项
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param i16Index              :数组的索引下标
 * @param pstGetItem            :获取的子项
 * @return int16        :-1 参数非法或者未初始化
 *                      :-2 下标不存在
 *                      :0 查找成功
 */
int16 GetIndexItemValuefromPeriodTable(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, int16 i16Index, ST_TRIG_TABLE *pstGetItem)
{
    if((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0) || (pstGetItem == NULL))
    {
        return -1;
    }

    if ((i16Index < 0) || (i16Index >= pstInternalCanHandle->pstInterCanTable->u16PeriodTableItemNum))
    {
        return -2;
    }

    pstGetItem->u32FrameID = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u32FrameID;
    pstGetItem->u16DataDlc = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16DataDlc;
    pstGetItem->u16ObjNum = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjNum;
    pstGetItem->u16ObjIndex_1 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_1;
    pstGetItem->u16ObjIndex_2 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_2;
    pstGetItem->u16ObjIndex_3 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_3;
    pstGetItem->u16ObjIndex_4 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_4;
    pstGetItem->u16ObjIndex_5 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_5;
    pstGetItem->u16ObjIndex_6 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_6;
    pstGetItem->u16ObjIndex_7 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_7;
    pstGetItem->u16ObjIndex_8 = pstInternalCanHandle->pstInterCanTable->pstPeriodTable[i16Index].u16ObjIndex_8;

    return 0;
}



/**
 * @brief 根据数组索引下标从触发表中取出对应的子项
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param i16Index              :数组的索引下标
 * @param pstGetItem            :获取的子项
 * @return int16        :-1 参数非法或者未初始化
 *                      :-2 下标不存在
 *                      :0 查找成功
 */
int16 GetIndexItemValuefromTrigTable(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, int16 i16Index, ST_ITEM_TABLE *pstGetItem)
{
    if((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0) || (pstGetItem == NULL))
    {
        return -1;
    }

    if ((i16Index < 0) || (i16Index >= pstInternalCanHandle->pstInterCanTable->u16TrigTableItemNum))
    {
        return -2;
    }

    pstGetItem->u32FrameID = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u32FrameID;
    pstGetItem->u16DataDlc = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16DataDlc;
    pstGetItem->u16ObjIndex_1 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_1;
    pstGetItem->u16ObjIndex_2 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_2;
    pstGetItem->u16ObjIndex_3 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_3;
    pstGetItem->u16ObjIndex_4 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_4;
    pstGetItem->u16ObjIndex_5 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_5;
    pstGetItem->u16ObjIndex_6 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_6;
    pstGetItem->u16ObjIndex_7 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_7;
    pstGetItem->u16ObjIndex_8 = pstInternalCanHandle->pstInterCanTable->pstTrigTable[i16Index].u16ObjIndex_8;

    return 0;
}



/**
 * @brief 将值写入对象字典对应的变量中
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstItem               :从周期表或者触发表中获取的子项
 * @param pstCanFrame           :从CAN总线中接收到的数据帧
 * @return int16        :-1 参数非法或者未初始化
 *                      :0 设置成功
 */
int16 SetValueToObjDictionary(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_ITEM_TABLE *pstItem, ST_INTERNAL_CAN_FRAME *pstCanFrame)
{
    Uint16 i = 0;
    Uint16 u16Offset = 0;
    Uint16 Value[8];
    Uint16 u16FrameData[8];

    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;

    if((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0)
       || (pstItem == NULL) || (pstCanFrame == NULL))
    {
        return -1;
    }

    // 对象索引
    Value[0] = pstItem->u16ObjIndex_1;
    Value[1] = pstItem->u16ObjIndex_2;
    Value[2] = pstItem->u16ObjIndex_3;
    Value[3] = pstItem->u16ObjIndex_4;
    Value[4] = pstItem->u16ObjIndex_5;
    Value[5] = pstItem->u16ObjIndex_6;
    Value[6] = pstItem->u16ObjIndex_7;
    Value[7] = pstItem->u16ObjIndex_8;

    // 接收的CAN帧数据
    u16FrameData[0] = pstCanFrame->unDataA.Bytes.u8Byte0;
    u16FrameData[1] = pstCanFrame->unDataA.Bytes.u8Byte1;
    u16FrameData[2] = pstCanFrame->unDataA.Bytes.u8Byte2;
    u16FrameData[3] = pstCanFrame->unDataA.Bytes.u8Byte3;
    u16FrameData[4] = pstCanFrame->unDataB.Bytes.u8Byte0;
    u16FrameData[5] = pstCanFrame->unDataB.Bytes.u8Byte1;
    u16FrameData[6] = pstCanFrame->unDataB.Bytes.u8Byte2;
    u16FrameData[7] = pstCanFrame->unDataB.Bytes.u8Byte3;

    for (i = 0; i < pstItem->u16ObjNum; i++)
    {
//        *pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]]->puValue = pstCanFrame->unDataA.Words.LowWord;
        switch (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].u16Type)
        {

        case CAN_TYPE_INT8:
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i8Val = (int16)u16FrameData[u16Offset];

            u16Offset += 1;
            break;

        case CAN_TYPE_UINT8:
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u8Val = (Uint16)u16FrameData[u16Offset];
            u16Offset += 1;
            break;

        case CAN_TYPE_INT16:
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i16Val = (int16)(u16FrameData[u16Offset] | u16FrameData[u16Offset + 1] << 8);
            u16Offset += 2;
            break;

        case CAN_TYPE_UINT16:
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u16Val = (Uint16)(u16FrameData[u16Offset] | u16FrameData[u16Offset + 1] << 8);
            u16Offset += 2;
            break;

        case CAN_TYPE_INT32:
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i32Val = (int32)(u16FrameData[u16Offset] | (((Uint32)u16FrameData[u16Offset + 1]) << 8) |
                                                                                                    (((Uint32)u16FrameData[u16Offset + 2]) << 16) | (((Uint32)u16FrameData[u16Offset + 3]) << 24));
            u16Offset += 4;
            break;

        case CAN_TYPE_UINT32:
            *(&pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u32Val) = (Uint32)(u16FrameData[u16Offset] | (((Uint32)u16FrameData[u16Offset + 1]) << 8) |
                                                                                                    (((Uint32)u16FrameData[u16Offset + 2]) << 16) | (((Uint32)u16FrameData[u16Offset + 3]) << 24));
            u16Offset += 4;
            break;

        case CAN_TYPE_FLOAT:
            uFloatByte.u16Val[0] = (u16FrameData[u16Offset] | u16FrameData[u16Offset + 1] << 8);
            uFloatByte.u16Val[1] = (u16FrameData[u16Offset + 2] | u16FrameData[u16Offset + 3] << 8);
            pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->fVal = uFloatByte.fVal;
            u16Offset += 4;
            break;
        }
    }

    return 0;

}



/**
 * @brief 根据子项从对象字典中获取变量的值写入CAN数据帧中
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstItem               :从周期表或者触发表中获取的子项
 * @param pstCanFrame           :需要填充的数据帧
 * @return int16        :-1 参数非法或者未初始化
 *                      :0 获取成功
 */
int16 GetValueFormObjDictionary(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_ITEM_TABLE *pstItem, ST_INTERNAL_CAN_FRAME *pstCanFrame)
{
    Uint16 i = 0;
    Uint16 u16Offset = 0;
    Uint16 Value[8];
    Uint16 u16FrameData[8];

    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;

    if((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0)
       || (pstItem == NULL) || (pstCanFrame == NULL))
    {
        return -1;
    }

    // 对象索引
    Value[0] = pstItem->u16ObjIndex_1;
    Value[1] = pstItem->u16ObjIndex_2;
    Value[2] = pstItem->u16ObjIndex_3;
    Value[3] = pstItem->u16ObjIndex_4;
    Value[4] = pstItem->u16ObjIndex_5;
    Value[5] = pstItem->u16ObjIndex_6;
    Value[6] = pstItem->u16ObjIndex_7;
    Value[7] = pstItem->u16ObjIndex_8;

    for (i = 0; i < pstItem->u16ObjNum; i++)
    {
//        *pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]]->puValue = pstCanFrame->unDataA.Words.LowWord;
        switch (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].u16Type)
        {

        case CAN_TYPE_INT8:
            u16FrameData[u16Offset] = pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i8Val;
            u16Offset += 1;
            break;

        case CAN_TYPE_UINT8:
            u16FrameData[u16Offset] = pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u8Val;
            u16Offset += 1;
            break;

        case CAN_TYPE_INT16:
            u16FrameData[u16Offset] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i16Val) & 0xFF;
            u16FrameData[u16Offset + 1] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i16Val >> 8) & 0xFF;
            u16Offset += 2;
            break;

        case CAN_TYPE_UINT16:
            u16FrameData[u16Offset] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u16Val) & 0xFF;
            u16FrameData[u16Offset + 1] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u16Val >> 8) & 0xFF;
            u16Offset += 2;
            break;

        case CAN_TYPE_INT32:
            u16FrameData[u16Offset] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i32Val) & 0xFF;
            u16FrameData[u16Offset + 1] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i32Val >> 8) & 0xFF;
            u16FrameData[u16Offset + 2] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i32Val >> 16) & 0xFF;
            u16FrameData[u16Offset + 3] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->i32Val >> 24) & 0xFF;
            u16Offset += 4;
            break;

        case CAN_TYPE_UINT32:
            u16FrameData[u16Offset] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u32Val) & 0xFF;
            u16FrameData[u16Offset + 1] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u32Val >> 8) & 0xFF;
            u16FrameData[u16Offset + 2] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u32Val >> 16) & 0xFF;
            u16FrameData[u16Offset + 3] = (pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->u32Val >> 24) & 0xFF;
            u16Offset += 4;
            break;

        case CAN_TYPE_FLOAT:
            uFloatByte.fVal = pstInternalCanHandle->pstInterCanTable->pstObjDictionary[Value[i]-1].puValue->fVal;
            u16FrameData[u16Offset] = uFloatByte.u16Val[0] & 0xFF;
            u16FrameData[u16Offset + 1] = (uFloatByte.u16Val[0] >> 8) & 0xFF;
            u16FrameData[u16Offset + 2] = (uFloatByte.u16Val[1]) & 0xFF;
            u16FrameData[u16Offset + 3] = (uFloatByte.u16Val[1] >> 8) & 0xFF;
            u16Offset += 4;
            break;
        }
    }

    pstCanFrame->unDataA.Bytes.u8Byte0 = u16FrameData[0];
    pstCanFrame->unDataA.Bytes.u8Byte1 = u16FrameData[1];
    pstCanFrame->unDataA.Bytes.u8Byte2 = u16FrameData[2];
    pstCanFrame->unDataA.Bytes.u8Byte3 = u16FrameData[3];
    pstCanFrame->unDataB.Bytes.u8Byte0 = u16FrameData[4];
    pstCanFrame->unDataB.Bytes.u8Byte1 = u16FrameData[5];
    pstCanFrame->unDataB.Bytes.u8Byte2 = u16FrameData[6];
    pstCanFrame->unDataB.Bytes.u8Byte3 = u16FrameData[7];

    return 0;
}



/**
 * @brief 设置触发帧请求
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param u32CanFrameID         :需要触发的帧ID，需要存在在触发表中
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未在触发表中找到该帧
 *                  :0 触发帧请求队列已满
 *                  :1 已下发触发请求
 */
int16 SetTrigRequest(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, Uint32 u32CanFrameID)
{
    Uint16 i = 0;
    int16 iRet = 0;

    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0))
    {
        return -1;
    }

    iRet = -2;
    for (i = 0; i < pstInternalCanHandle->pstInterCanTable->u16TrigTableItemNum; i++)
    {
        if (u32CanFrameID == pstInternalCanHandle->pstInterCanTable->pstTrigTable[i].u32FrameID)
        {
            iRet = WriteRingBuff(&pstInternalCanHandle->stTrigRequestFifo, &u32CanFrameID, 1);
            if (iRet)
            {
                pstInternalCanHandle->u16TrigFlag = 1;
            }
            break;
        }
    }
    return iRet;
}


#ifdef __cplusplus
    }
#endif


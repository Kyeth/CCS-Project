/**
 * @file InternalCan.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "InternalCan.h"
#include "InternalCanFrame.h"

#ifdef __cplusplus
    extern "C" {
#endif




/**
 * @brief 根据帧ID查找周期表中是否存在该ID，存在则返回索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :Can帧，内部存在要查找的帧ID
 * @return int16        : -1 参数非法
 *                      : >=0 周期表索引下标
 */
int16 InternalCanRegister(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_TABLE *pstInterCanTable, ST_INTERNAL_CAN_TRANSPORT_INTF *pstTranSportIntf)
{
    if ((pstInternalCanHandle == NULL) || (pstInterCanTable == NULL)
            || (pstInterCanTable->pstPeriodTable == NULL) || (pstInterCanTable->pstTrigTable == NULL)
            || (pstInterCanTable->pstObjDictionary == NULL))
    {
        return -1;      // 参数非法
    }

    pstInternalCanHandle->pstInterCanTable = pstInterCanTable;
    pstInternalCanHandle->stInterCanTranIntf.CanRead = pstTranSportIntf->CanRead;
    pstInternalCanHandle->stInterCanTranIntf.CanWrite = pstTranSportIntf->CanWrite;
    pstInternalCanHandle->u16CanNodeAddr = 0;
    pstInternalCanHandle->u16CanNodeBaud = 0;
    pstInternalCanHandle->u16PeriodSendIndex = 0;

    // 初始化FIFO队列
    InitRingBuff(&pstInternalCanHandle->stTxFrameFifo, &pstInternalCanHandle->stInternalCanTxBuff[0], sizeof(ST_INTERNAL_CAN_FRAME), INTERNAL_CAN_FRAME_MAX);
    InitRingBuff(&pstInternalCanHandle->stRxFrameFifo, &pstInternalCanHandle->stInternalCanRxBuff[0], sizeof(ST_INTERNAL_CAN_FRAME), INTERNAL_CAN_FRAME_MAX);
    InitRingBuff(&pstInternalCanHandle->stTrigRequestFifo, &pstInternalCanHandle->stTrigRequestBuff[0], sizeof(Uint32), INTERNAL_CAN_FRAME_MAX / 2);
    pstInternalCanHandle->IsInit = 1;       // 完成初始化

    return 0;
}



/**
 * @brief 调用Can驱动接口将数据存放到接收FIFO中
 *          每次调用最多处理宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16    :-1 参数非法或者未初始化
 *                  :>=0 接收到数据帧的个数
 */
int16 InternalCanReceive(ST_INTERNALCAN_HANDLE *pstInternalCanHandle)
{
    ST_INTERNAL_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    int16 i16ReadNum = 0;
    int16 i16Ret = 0;

    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0))
    {
        return -1;
    }
    stCanFrame[0].bDLC = 0;
    stCanFrame[0].bID = 0;
    stCanFrame[0].bIDE = 0;
    stCanFrame[0].bRTR = 0;
    stCanFrame[0].unDataA.u32All = 0;
    stCanFrame[0].unDataB.u32All = 0;

    // 回调函数，读取CAN总线数据
    i16ReadNum = pstInternalCanHandle->stInterCanTranIntf.CanRead(&stCanFrame[0], SINGLE_PROCESS_RX_QUANTITY);

    // 如果接收到数据，将数据写入FIFO中
    if (i16ReadNum > 0)
    {
        i16Ret = WriteRingBuff(&pstInternalCanHandle->stRxFrameFifo, &stCanFrame[0], i16ReadNum);
    }

    return i16Ret;
}



/**
 * @brief 从发送FIFO中将CAN数据帧取出，然后调用CAN驱动接口将数据发送到CAN总线上
 *          每次调用最多处理宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16    : -1 参数非法或者未初始化
 *                  : >=0 实际发送数据的个数
 */
int16 InternalCanTransmit(ST_INTERNALCAN_HANDLE *pstInternalCanHandle)
{
    ST_INTERNAL_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    ST_INTERNAL_CAN_FRAME stCanFrameTemp[SINGLE_PROCESS_RX_QUANTITY];
    int16 i16ReadNum = 0;
    int16 i = 0;
    int16 i16Ret = 0;

    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0))
    {
        return -1;
    }

    i16ReadNum = ReadRingBuff(&pstInternalCanHandle->stTxFrameFifo, stCanFrameTemp, SINGLE_PROCESS_RX_QUANTITY);

    for (i = 0; i < i16ReadNum; i++)
    {
        stCanFrame[i].bDLC = stCanFrameTemp[i].bDLC;
        stCanFrame[i].bID = stCanFrameTemp[i].bID;
        stCanFrame[i].bIDE = stCanFrameTemp[i].bIDE;
        stCanFrame[i].bRTR = stCanFrameTemp[i].bRTR;
        stCanFrame[i].unDataA.u32All = stCanFrameTemp[i].unDataA.u32All;
        stCanFrame[i].unDataB.u32All = stCanFrameTemp[i].unDataB.u32All;
    }

    if (i16ReadNum)
    {
        i16Ret = pstInternalCanHandle->stInterCanTranIntf.CanWrite(&stCanFrame[0], i16ReadNum);
    }

    return i16Ret;
}



/**
 * @brief 对接收的数据帧进行解析
 *          每次调用最多解析宏SINGLE_PROCESS_RX_QUANTITY个数的帧
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @return int16        : -1 参数非法或者未初始化
 *                      : >=0 解析数据帧的个数
 */
int16 InternalReceiveFrameProcess(ST_INTERNALCAN_HANDLE *pstInternalCanHandle)
{
    //ST_INTERNAL_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    ST_INTERNAL_CAN_FRAME stCanFrameTemp[SINGLE_PROCESS_RX_QUANTITY];
    ST_ITEM_TABLE stGetItem[SINGLE_PROCESS_RX_QUANTITY];
    Uint16 i = 0;
    int16 i16Index = 0;
    int16 i16ReadNum = 0;
    int16 iRet = 0;
    
    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0))
    {
        return -1;
    }

    i16ReadNum = ReadRingBuff(&pstInternalCanHandle->stRxFrameFifo, stCanFrameTemp, SINGLE_PROCESS_RX_QUANTITY);

    for (i = 0; i < i16ReadNum; i++)
    {
        // 查找出在ID在周期表还是在触发表，返回在表中的索引下标地址

        // 执行一个回调函数
        // 用于监听广播帧来设置通信表
        // 建立通信表

        // 先查触发表
        i16Index = SearchTrigTableIndex(pstInternalCanHandle, &stCanFrameTemp[i]);
        if (i16Index >= 0)
        {
            GetIndexItemValuefromTrigTable(pstInternalCanHandle, i16Index, &stGetItem[i]);
            SetValueToObjDictionary(pstInternalCanHandle, &stGetItem[i], &stCanFrameTemp[i]);
            ++iRet;
            continue;
        }

        // 再查周期表
        i16Index = SearchPeriodTableIndex(pstInternalCanHandle, &stCanFrameTemp[i]);
        if (i16Index >= 0)
        {
            GetIndexItemValuefromPeriodTable(pstInternalCanHandle, i16Index, &stGetItem[i]);
            SetValueToObjDictionary(pstInternalCanHandle, &stGetItem[i], &stCanFrameTemp[i]);
            ++iRet;
            continue;
        }
    }
    return iRet;
}



/**
 * @brief 封装待发送的数据帧，写入发送FIFO中，每次调用只封装一帧
 * 
 * @param pstInternalCanHandle 
 * @return int 
 */
int InternalTransmitFrameProcess(ST_INTERNALCAN_HANDLE *pstInternalCanHandle)
{
    ST_INTERNAL_CAN_FRAME stCanFrame[SINGLE_PROCESS_RX_QUANTITY];
    ST_ITEM_TABLE stGetItem[SINGLE_PROCESS_RX_QUANTITY];
    Uint16 i16Ret = 0;
    Uint32 u32FrameID = 0;
    int16 i16Index = 0;
    if ((pstInternalCanHandle == NULL) || (pstInternalCanHandle->IsInit == 0))
    {
        return -1;
    }

    // 检测是否存在触发帧的发送请求
    if (pstInternalCanHandle->u16TrigFlag == 0)
    {
        // 获取周期表中的对象ITEM
        GetIndexItemValuefromPeriodTable(pstInternalCanHandle, pstInternalCanHandle->u16PeriodSendIndex, &stGetItem[0]);

        // 从对象字典取出值填充到CAN帧中
        GetValueFormObjDictionary(pstInternalCanHandle, &stGetItem[0], &stCanFrame[0]);
        stCanFrame[0].bDLC = stGetItem[0].u16DataDlc;
        stCanFrame[0].bID = stGetItem[0].u32FrameID;
        stCanFrame[0].bIDE = 1;
        stCanFrame[0].bRTR = 0;

        ++pstInternalCanHandle->u16PeriodSendIndex;
        if (pstInternalCanHandle->u16PeriodSendIndex == pstInternalCanHandle->pstInterCanTable->u16PeriodTableItemNum)
        {
            pstInternalCanHandle->u16PeriodSendIndex = 0;
        }
        i16Ret = WriteRingBuff(&pstInternalCanHandle->stTxFrameFifo, stCanFrame, 1);

    }
    else
    {
        pstInternalCanHandle->u16TrigFlag = 0;
        // 从触发队列中,取出触发帧的ID
        i16Ret = ReadRingBuff(&pstInternalCanHandle->stTrigRequestFifo, &u32FrameID, 1);
        if (i16Ret)
        {
            stCanFrame[0].bID = u32FrameID;
            i16Index = SearchTrigTableIndex(pstInternalCanHandle, &stCanFrame[0]);
            if (i16Index >= 0)
            {
                // 获取周期表中的对象ITEM
                GetIndexItemValuefromTrigTable(pstInternalCanHandle, i16Index, &stGetItem[0]);

                // 从对象字典取出值填充到CAN帧中
                GetValueFormObjDictionary(pstInternalCanHandle, &stGetItem[0], &stCanFrame[0]);
                stCanFrame[0].bDLC = stGetItem[0].u16DataDlc;
                stCanFrame[0].bID = stGetItem[0].u32FrameID;
                stCanFrame[0].bIDE = 1;
                stCanFrame[0].bRTR = 0;
                i16Ret = WriteRingBuff(&pstInternalCanHandle->stTxFrameFifo, stCanFrame, 1);
            }
        }
    }
    return i16Ret;
}


#ifdef __cplusplus
}
#endif

#include "EquipmentParallelCore.h"
#include "string.h"


void EquipmentParallelCoreInit(ST_BATTERY_HANDLE *pstBatteryHandle, ST_BATTERY_CFG *pstInterCanTable, ST_PARALLEL_TRANSPORT_INTF *pstTranSportIntf)
{
    memset(pstBatteryHandle, 0, sizeof(ST_BATTERY_HANDLE));

    pstBatteryHandle->stParallelTranIntf.CanRead = pstTranSportIntf->CanRead;
    pstBatteryHandle->stParallelTranIntf.CanWrite = pstTranSportIntf->CanWrite;
    pstBatteryHandle->stParallelTranIntf.ClearBusError = pstTranSportIntf->ClearBusError;
    pstBatteryHandle->pstBatteryCfg = pstInterCanTable;

    // 初始化FIFO队列
    InitRingBuff(&pstBatteryHandle->stTxFrameFifo, &pstBatteryHandle->stParallelTxBuff[0], sizeof(ST_PARALLEL_CAN_FRAME), PARALLEL_FRAME_MAX);
    InitRingBuff(&pstBatteryHandle->stRxFrameFifo, &pstBatteryHandle->stParallelRxBuff[0], sizeof(ST_PARALLEL_CAN_FRAME), PARALLEL_FRAME_MAX);

}


int32 CheckAddrVaild(ST_BATTERY_HANDLE *pstBatteryHandle, Uint32 u32FrameId)
{
    Uint32 i = 0;
    Uint16 Addr = 0;
    int32 i32Ret = -1;
    Uint32 u32FrameType = 0;

    u32FrameType = (u32FrameId & 0xFFF00000) >> 20;
    Addr = (Uint16)(u32FrameId & 0x00FF);

     if ((u32FrameType == HEARTBEAT_HEAD) || (u32FrameType == DATA_HEAD))
    {
        for (i = 1; i < MAX_PACK_NUM; i++)
        {
            if ((pstBatteryHandle->stBatteryPack[i].u16PackAddr == Addr)
                    && (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1))
            {
                i32Ret = i;
                break;
            }

        }

    }


    return i32Ret;
}



int32 AddFrameId(ST_BATTERY_HANDLE *pstBatteryHandle, Uint32 u32FrameId)
{
    Uint32 i = 0;
    Uint16 Addr = 0;
    int32 i32Ret = -1;
    Uint32 u32FrameType = 0;

    u32FrameType = (u32FrameId & 0xFFF00000) >> 20;
    Addr = (Uint16)(u32FrameId & 0x00FF);

    if (u32FrameType == HEARTBEAT_HEAD)
    {
        for (i = 1; i < MAX_PACK_NUM; i++)
        {
            // 如果已经存在这个地址，则直接退出
            if ((pstBatteryHandle->stBatteryPack[i].u16PackAddr == Addr) && (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1))
            {
                break;
            }

            // 如果存在空的数组，且地址不存在，则添加进去
            if ((pstBatteryHandle->stBatteryPack[i].u16IsInit != 1) && (pstBatteryHandle->stBatteryPack[i].u16PackAddr != Addr)
                    && (pstBatteryHandle->u16LocalAddr != Addr))
            {
                pstBatteryHandle->stBatteryPack[i].u16PackAddr = Addr;
                pstBatteryHandle->stBatteryPack[i].u16IsInit = 1;
                i32Ret = i;
                break;
            }
        }
    }

    return i32Ret;
}



int16 ParallelReceive(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    ST_PARALLEL_CAN_FRAME stCanFrame[SINGLE_PROCESS_MAX_NUM];
    int16 i16ReadNum = 0;
    int16 i16Ret = 0;

    stCanFrame[0].bDLC = 0;
    stCanFrame[0].bID = 0;
    stCanFrame[0].bIDE = 0;
    stCanFrame[0].bRTR = 0;
    stCanFrame[0].unDataA.u32All = 0;
    stCanFrame[0].unDataB.u32All = 0;

    // 回调函数，读取CAN总线数据
    i16ReadNum = pstBatteryHandle->stParallelTranIntf.CanRead(stCanFrame, SINGLE_PROCESS_MAX_NUM);

    // 如果接收到数据，将数据写入FIFO中
    if (i16ReadNum > 0)
    {
        i16Ret = WriteRingBuff(&pstBatteryHandle->stRxFrameFifo, &stCanFrame[0], i16ReadNum);
    }

    return i16Ret;
}




int16 ParallelTransmit(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    ST_PARALLEL_CAN_FRAME stCanFrame[SINGLE_PROCESS_MAX_NUM];
    ST_PARALLEL_CAN_FRAME stCanFrameTemp[SINGLE_PROCESS_MAX_NUM];
    int16 i16ReadNum = 0;
    int16 i = 0;
    int16 i16Ret = 0;

    i16ReadNum = ReadRingBuff(&pstBatteryHandle->stTxFrameFifo, stCanFrameTemp, SINGLE_PROCESS_MAX_NUM);

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
        i16Ret = pstBatteryHandle->stParallelTranIntf.CanWrite(stCanFrame, i16ReadNum);
    }

    return i16Ret;
}



void SetValueToTable(ST_BATTERY_HANDLE *pstBatteryHandle, int16 i16Index, ST_PARALLEL_CAN_FRAME *pstCanFrame)
{
    Uint32 u32FrameFunc = 0;
    Uint32 u32FrameId = 0;
    Uint16 u16FrameLen = 0;
    Uint16 i = 0, j = 0;

    Uint16 u16Offset = 0;
    Uint16 u16FrameData[8];
    int16 *pi16Ponit;
    int32 *pi32Ponit;
    Uint16 *pu16Ponit;
    Uint32 *pu32Ponit;
    float *pfPonit;
    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;


    if ((i16Index < 0) || (i16Index >= MAX_PACK_NUM))
    {
        return;
    }

    u32FrameId = pstCanFrame->bID;
    u32FrameFunc = (u32FrameId & 0xFFF00)>>8;
    u16FrameLen = pstCanFrame->bDLC;

    ST_OBJ_ATTRIBUTE stObjAttribute[8];


    // 接收的CAN帧数据
    u16FrameData[0] = pstCanFrame->unDataA.Bytes.u8Byte3;
    u16FrameData[1] = pstCanFrame->unDataA.Bytes.u8Byte2;
    u16FrameData[2] = pstCanFrame->unDataA.Bytes.u8Byte1;
    u16FrameData[3] = pstCanFrame->unDataA.Bytes.u8Byte0;
    u16FrameData[4] = pstCanFrame->unDataB.Bytes.u8Byte3;
    u16FrameData[5] = pstCanFrame->unDataB.Bytes.u8Byte2;
    u16FrameData[6] = pstCanFrame->unDataB.Bytes.u8Byte1;
    u16FrameData[7] = pstCanFrame->unDataB.Bytes.u8Byte0;


    for (i = 0; i < pstBatteryHandle->pstBatteryCfg->u16ItemNum; i++)
    {

        if ((u32FrameFunc == pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16FrameNo)
                && (u16FrameLen == pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16DataDlc))
        {
            stObjAttribute[0] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_1;
            stObjAttribute[1] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_2;
            stObjAttribute[2] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_3;
            stObjAttribute[3] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_4;
            stObjAttribute[4] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_5;
            stObjAttribute[5] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_6;
            stObjAttribute[6] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_7;
            stObjAttribute[7] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjAttribute_8;

            for (j = 0; j < pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i].u16ObjNum; j++)
            {
                switch(stObjAttribute[j].eValType)
                {
                case PARALLEL_TYPE_INT8:
                    pi16Ponit = (int16*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pi16Ponit = (int16)u16FrameData[u16Offset];
                    u16Offset += 1;
                    break;

                case PARALLEL_TYPE_UINT8:
                    pu16Ponit = (Uint16*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pu16Ponit = (Uint16)u16FrameData[u16Offset];
                    u16Offset += 1;
                    break;

                case PARALLEL_TYPE_INT16:
                    pi16Ponit = (int16*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pi16Ponit = (int16)((Uint16)u16FrameData[u16Offset] | (Uint16)u16FrameData[u16Offset + 1] << 8);
                    u16Offset += 2;
                                break;


                case PARALLEL_TYPE_UINT16:
                    pu16Ponit = (Uint16*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pu16Ponit = ((Uint16)u16FrameData[u16Offset] | (Uint16)u16FrameData[u16Offset + 1] << 8);
                    u16Offset += 2;
                                break;

                case PARALLEL_TYPE_INT32:
                    pi32Ponit = (int32*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pi32Ponit = (int32)((Uint32)u16FrameData[u16Offset] | ((Uint32)u16FrameData[u16Offset + 1] << 8)
                                | ((Uint32)u16FrameData[u16Offset + 2] << 16)
                                | ((Uint32)u16FrameData[u16Offset + 3] << 24));
                    u16Offset += 4;
                                break;

                case PARALLEL_TYPE_UINT32:
                    pu32Ponit = (Uint32*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    *pu32Ponit = ((Uint32)u16FrameData[u16Offset] | ((Uint32)u16FrameData[u16Offset + 1] << 8)
                                | ((Uint32)u16FrameData[u16Offset + 2] << 16)
                                | ((Uint32)u16FrameData[u16Offset + 3] << 24));
                    u16Offset += 4;
                                break;

                case PARALLEL_TYPE_FLOAT:
                    pfPonit = (float*)(((Uint16*)&pstBatteryHandle->stBatteryPack[i16Index].stBatteryData) + stObjAttribute[j].u16ObjOffset);
                    uFloatByte.u16Val[0] = (u16FrameData[u16Offset] | u16FrameData[u16Offset + 1] << 8);
                    uFloatByte.u16Val[1] = (u16FrameData[u16Offset + 2] | u16FrameData[u16Offset + 3] << 8);
                    *pfPonit = uFloatByte.fVal;
                    u16Offset += 4;
                                break;
                }
            }
            u16Offset = 0;
            // 如果找到了符合Func的帧，则跳出循环，避免处理相同Func的值
            break;
        }
    }
}


int16 ParallelReceiveFrameProcess(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    ST_PARALLEL_CAN_FRAME stCanFrame[SINGLE_PROCESS_MAX_NUM];
    Uint16 i = 0;
    int16 i16Index = 0;
    int16 i16ReadNum = 0;
    int16 iRet = 0;
    Uint32 u32FrameId = 0;
    Uint16 Addr = 0;

    i16ReadNum = ReadRingBuff(&pstBatteryHandle->stRxFrameFifo, stCanFrame, SINGLE_PROCESS_MAX_NUM);

    for (i = 0; i < i16ReadNum; i++)
    {
        u32FrameId = stCanFrame[i].bID;

        Addr = (Uint16)(u32FrameId & 0x00000FF);

        if (pstBatteryHandle->u16LocalAddr == Addr)
        {
            continue;
        }

        AddFrameId(pstBatteryHandle, u32FrameId);

        i16Index = CheckAddrVaild(pstBatteryHandle, u32FrameId);

        if (i16Index > 0)
        {
            pstBatteryHandle->stBatteryPack[i16Index].u16HeartCount++;  // 更新包心跳，用于进行断线检测
            SetValueToTable(pstBatteryHandle, i16Index, &stCanFrame[i]);
        }


    }
    return iRet;
}






void GetValueFromTable(ST_BATTERY_HANDLE *pstBatteryHandle, int16 i16Index, ST_PARALLEL_CAN_FRAME *pstCanFrame)
{
    Uint32 i = 0;
    ST_OBJ_ATTRIBUTE stObjAttribute[8];
    Uint16 u16Value[8];
    Uint16 u16Offset = 0;;
    Uint16 *pu16Point;
    Uint32 *pu32Point;
    float *pfPonit;
    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;

    // 如果当前帧无效，则直接跳过
    if (pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16Vaild == INVAILD)
    {
        return;
    }

    stObjAttribute[0] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_1;
    stObjAttribute[1] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_2;
    stObjAttribute[2] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_3;
    stObjAttribute[3] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_4;
    stObjAttribute[4] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_5;
    stObjAttribute[5] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_6;
    stObjAttribute[6] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_7;
    stObjAttribute[7] = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjAttribute_8;

    for (i = 0; i < pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16ObjNum; i++)
    {

        switch(stObjAttribute[i].eValType)
        {
        case PARALLEL_TYPE_INT8:
            pu16Point = (Uint16*)((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset;
            u16Value[u16Offset] = *pu16Point;
            u16Offset += 1;

            break;

        case PARALLEL_TYPE_UINT8:
            pu16Point = (Uint16*)((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset;
            u16Value[u16Offset] = *pu16Point;
            u16Offset += 1;

            break;

        case PARALLEL_TYPE_INT16:
            pu16Point = (Uint16*)((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset;
            u16Value[u16Offset] = (*pu16Point) & 0x00FF;
            u16Value[u16Offset + 1] = ((*pu16Point) >> 8) & 0x00FF;
            u16Offset += 2;
                    break;

        case PARALLEL_TYPE_UINT16:
            pu16Point = (Uint16*)((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset;
            u16Value[u16Offset] = (*pu16Point) & 0x00FF;
            u16Value[u16Offset + 1] = ((*pu16Point) >> 8) & 0x00FF;
            u16Offset += 2;
                    break;

        case PARALLEL_TYPE_INT32:
            pu32Point = (Uint32*)(((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset);
            u16Value[u16Offset] = (*pu32Point) & 0x00FF;
            u16Value[u16Offset + 1] = (((Uint32)(*pu32Point)) >> 8) & 0x00FF;
            u16Value[u16Offset + 2] = (((Uint32)(*pu32Point)) >> 16) & 0x00FF;
            u16Value[u16Offset + 3] = (((Uint32)(*pu32Point)) >> 24) & 0x00FF;
            u16Offset += 4;
                    break;

        case PARALLEL_TYPE_UINT32:
            pu32Point = (Uint32*)(((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset);
            u16Value[u16Offset] = (*pu32Point) & 0x00FF;
            u16Value[u16Offset + 1] = (((Uint32)(*pu32Point)) >> 8) & 0x00FF;
            u16Value[u16Offset + 2] = (((Uint32)(*pu32Point)) >> 16) & 0x00FF;
            u16Value[u16Offset + 3] = (((Uint32)(*pu32Point)) >> 24) & 0x00FF;
            u16Offset += 4;
                    break;

        case PARALLEL_TYPE_FLOAT:
            pfPonit = (float*)(((Uint16*)&pstBatteryHandle->stBatteryPack[0].stBatteryData) + stObjAttribute[i].u16ObjOffset);
            uFloatByte.fVal = *pfPonit;
            u16Value[u16Offset] = uFloatByte.u16Val[0] & 0x00FF;
            u16Value[u16Offset + 1] = (((Uint32)(uFloatByte.u16Val[0])) >> 8) & 0x00FF;
            u16Value[u16Offset + 2] = uFloatByte.u16Val[1] & 0x00FF;
            u16Value[u16Offset + 3] = (((Uint32)(uFloatByte.u16Val[1])) >> 8) & 0x00FF;
            u16Offset += 4;
                    break;

        }
    }

    pstCanFrame->bDLC = pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16DataDlc;
    pstCanFrame->bID = ((Uint32)pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16FrameType << 20)
            | ((Uint32)pstBatteryHandle->pstBatteryCfg->pstBatteryItem[i16Index].u16FrameNo << 8)
            | pstBatteryHandle->u16LocalAddr;
    pstCanFrame->bIDE = 1;
    pstCanFrame->bRTR = 0;
    pstCanFrame->unDataA.Bytes.u8Byte3 = u16Value[0];
    pstCanFrame->unDataA.Bytes.u8Byte2 = u16Value[1];
    pstCanFrame->unDataA.Bytes.u8Byte1 = u16Value[2];
    pstCanFrame->unDataA.Bytes.u8Byte0 = u16Value[3];
    pstCanFrame->unDataB.Bytes.u8Byte3 = u16Value[4];
    pstCanFrame->unDataB.Bytes.u8Byte2 = u16Value[5];
    pstCanFrame->unDataB.Bytes.u8Byte1 = u16Value[6];
    pstCanFrame->unDataB.Bytes.u8Byte0 = u16Value[7];

}





int ParallelTransmitFrameProcess(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    ST_PARALLEL_CAN_FRAME stCanFrame[SINGLE_PROCESS_MAX_NUM];

    Uint16 i16Ret = 0;
    GetValueFromTable(pstBatteryHandle, pstBatteryHandle->u16PeriodSendIndex, stCanFrame);
    ++pstBatteryHandle->u16PeriodSendIndex;
    if (pstBatteryHandle->u16PeriodSendIndex >= pstBatteryHandle->pstBatteryCfg->u16ItemNum)
    {
        pstBatteryHandle->u16PeriodSendIndex = 0;
    }
    i16Ret = WriteRingBuff(&pstBatteryHandle->stTxFrameFifo, stCanFrame, 1);

    return i16Ret;
}



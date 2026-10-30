#include "EquipmentparallelTramsport.h"
#include "Hw_MCanDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif


int16 ParallelReadTramsport(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize)
{
    ST_MCAN_FRAME stMCanFrame[SINGLE_PROCESS_MAX_NUM];
    Uint16 i = 0;
    Uint16 i16ReadNum = 0;


    if (u16ReadSize > SINGLE_PROCESS_MAX_NUM)
    {
        u16ReadSize = SINGLE_PROCESS_MAX_NUM;
    }
    i16ReadNum = Hw_MCanRead(MCAN_ID0, &stMCanFrame[0], u16ReadSize);

    for (i = 0; i < i16ReadNum; i++)
    {
        pstCanFrame[i].bDLC = stMCanFrame[i].bDLC;
        pstCanFrame[i].bID = stMCanFrame[i].bID;
        pstCanFrame[i].bIDE = stMCanFrame[i].bIDE;
        pstCanFrame[i].bRTR = stMCanFrame[i].bRTR;
        pstCanFrame[i].unDataA.u32All = stMCanFrame[i].unDataA.u32All;
        pstCanFrame[i].unDataB.u32All = stMCanFrame[i].unDataB.u32All;
    }

    return i16ReadNum;
}




int16 ParallelWriteTramsport(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16WriteSize)
{

    ST_MCAN_FRAME stMCanFrame[SINGLE_PROCESS_MAX_NUM];
    Uint16 i = 0;
    Uint16 i16Ret = 0;
    Uint16 i16ReadNum = 0;

    if (u16WriteSize > SINGLE_PROCESS_MAX_NUM)
    {
        u16WriteSize = SINGLE_PROCESS_MAX_NUM;
    }

    for (i = 0; i < u16WriteSize; i++)
    {
        stMCanFrame[i].bDLC = pstCanFrame[i].bDLC;
        stMCanFrame[i].bID = pstCanFrame[i].bID;
        stMCanFrame[i].bIDE = pstCanFrame[i].bIDE;
        stMCanFrame[i].bRTR = pstCanFrame[i].bRTR;
        stMCanFrame[i].unDataA.u32All = pstCanFrame[i].unDataA.u32All;
        stMCanFrame[i].unDataB.u32All = pstCanFrame[i].unDataB.u32All;
    }
    i16Ret = Hw_MCanWrite(MCAN_ID0, &stMCanFrame[0], u16WriteSize);
    return i16Ret;
}



int16 ClearBusError(void)
{
    Hw_ClearMCanBusError(MCAN_ID0);
}


#ifdef __cplusplus
    }
#endif

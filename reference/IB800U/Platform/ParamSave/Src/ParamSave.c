#include "ParamSave.h"

#ifdef __cplusplus
extern "C" {
#endif

static int16 BinarySearch(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index)
{
    Uint16 u16Len = 0, u16Low = 0, u16High = 0, u16Mid = 0;

    ST_PARAM_ITEM *pstParamStart = NULL;
    ST_PARAM_ITEM *pstParamTmp = NULL;
    ST_PARAM_ITEM *pstParamMix = NULL;
    ST_PARAM_ITEM *pstParamMax = NULL;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    pstParamMix = (ST_PARAM_ITEM*)&pstParamSaveHandle->pstParamCfg->pstParamItem[0];
    pstParamMax = (ST_PARAM_ITEM*)&pstParamSaveHandle->pstParamCfg->pstParamItem[pstParamSaveHandle->pstParamCfg->u16ItemNum-1];

    if ((u16Index < pstParamMix->u16Index) || (u16Index > pstParamMax->u16Index))
    {
        return -2;
    }

    pstParamStart = pstParamSaveHandle->pstParamCfg->pstParamItem;
    u16Len = pstParamSaveHandle->pstParamCfg->u16ItemNum;

    u16High = u16Len - 1;

    while (u16Low <= u16High)
    {
        u16Mid = (u16High + u16Low) >> 1;
        pstParamTmp = pstParamStart + u16Mid;

        if (u16Index < pstParamTmp->u16Index)
        {
            u16High = u16Mid - 1;
        }
        else if (u16Index > pstParamTmp->u16Index)
        {
            u16Low = u16Mid + 1;
        }
        else
        {
            return u16Mid;
        }
    }
    return 0;
}

static int16 SetParamValue(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, void* vBuff, Uint16 u16Index)
{
    if ((pstParamSaveHandle == NULL) || (vBuff == NULL))
    {
        return -1;
    }

    switch (pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].bParamType)
    {
    case PARAM_TYPE_INT8:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i8Val = *(int16 *)vBuff;
        break;

    case PARAM_TYPE_UINT8:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u8Val = *(Uint16 *)vBuff;
        break;

    case PARAM_TYPE_INT16:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i16Val = *(int16 *)vBuff;
        break;

    case PARAM_TYPE_UINT16:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u16Val = *(Uint16 *)vBuff;
        break;

    case PARAM_TYPE_INT32:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i32Val = *(int32 *)vBuff;
        break;

    case PARAM_TYPE_UINT32:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u32Val = *(Uint32 *)vBuff;
        break;

    case PARAM_TYPE_FLOAT:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->fVal = *(float *)vBuff;
        break;

    default:
        pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u8Val = *(Uint16 *)vBuff;
        break;
    }
    return 1;
}



static int16 GetParamValue(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, void* vBuff, Uint16 u16Index)
{
    if ((pstParamSaveHandle == NULL) || (vBuff == NULL))
    {
        return -1;
    }

    switch (pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].bParamType)
    {
    case PARAM_TYPE_INT8:
        *(int16 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i8Val;
        break;

    case PARAM_TYPE_UINT8:
        *(Uint16 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u8Val;
        break;

    case PARAM_TYPE_INT16:
        *(int16 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i16Val;
        break;

    case PARAM_TYPE_UINT16:
        *(Uint16 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u16Val;
        break;

    case PARAM_TYPE_INT32:
        *(int32 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->i32Val;
        break;

    case PARAM_TYPE_UINT32:
        *(Uint32 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u32Val;
        break;

    case PARAM_TYPE_FLOAT:
        *(float *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->fVal;
        break;

    default:
        *(Uint16 *)vBuff = pstParamSaveHandle->pstParamCfg->pstParamItem[u16Index].puParamVal->u16Val;
        break;
    }
    return 1;
}


int16 ParamSaveInit(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, ST_PARAM_CFG *pstParamCfg, ST_PARAM_WR_INTF *pstParamWrIntf)
{
    Uint16 u16Value[4];

    if ((pstParamSaveHandle == NULL) || (pstParamCfg == NULL))
    {
        return -1;
    }


    pstParamSaveHandle->pstParamCfg = pstParamCfg;

    InitRingBuff(&pstParamSaveHandle->stParamCMD, pstParamSaveHandle->stParamCmdBuff, sizeof(ST_PARAM_CMD), CMD_BUFF_SIZE);

    pstParamSaveHandle->stParamWRIntf.ParamWrite = pstParamWrIntf->ParamWrite;
    pstParamSaveHandle->stParamWRIntf.ParamRead = pstParamWrIntf->ParamRead;

    // 读取根目录
    pstParamSaveHandle->stParamWRIntf.ParamRead(PARAM_MANAGER_ADDR, &u16Value[0], PARAM_BANK_SIZE);

    // 如果未初始化，那么进行初始化
    if ((u16Value[0] != PARAM_ROOT_BYTE0) || (u16Value[1] != PARAM_ROOT_BYTE1)
            || (u16Value[2] != PARAM_ROOT_BYTE2) || (u16Value[3] != PARAM_ROOT_BYTE3))
    {
        SaveAllParamProc(pstParamSaveHandle);
        u16Value[0] = PARAM_ROOT_BYTE0;
        u16Value[1] = PARAM_ROOT_BYTE1;
        u16Value[2] = PARAM_ROOT_BYTE2;
        u16Value[3] = PARAM_ROOT_BYTE3;
        pstParamSaveHandle->stParamWRIntf.ParamWrite(PARAM_MANAGER_ADDR, &u16Value[0], PARAM_BANK_SIZE);
    }
    else
    {
        LoadAllParamProc(pstParamSaveHandle);
    }

    return 1;
}



int16 LoadAllParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle)
{
    ST_PARAM_CMD stParamCmd;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    stParamCmd.u16Cmd = CMD_LOAR_ALL_PARAM;
    stParamCmd.u16Index = 0;    // 加载所有参数时,u16Index不关注

    return WriteRingBuff(&pstParamSaveHandle->stParamCMD, &stParamCmd, 1);
}


int16 SaveAllParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle)
{
    ST_PARAM_CMD stParamCmd;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    stParamCmd.u16Cmd = CMD_SAVE_ALL_PARAM;
    stParamCmd.u16Index = 0;    // 保存所有参数时,u16Index不关注

    return WriteRingBuff(&pstParamSaveHandle->stParamCMD, &stParamCmd, 1);
}


int16 SaveSingleParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index)
{
    ST_PARAM_CMD stParamCmd;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    stParamCmd.u16Cmd = CMD_SAVE_SING_PARAM;
    stParamCmd.u16Index = u16Index;

    return WriteRingBuff(&pstParamSaveHandle->stParamCMD, &stParamCmd, 1);
}


int16 LoadSingleParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index)
{
    ST_PARAM_CMD stParamCmd;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    stParamCmd.u16Cmd = CMD_LOAD_SING_PARAM;
    stParamCmd.u16Index = u16Index;

    return WriteRingBuff(&pstParamSaveHandle->stParamCMD, &stParamCmd, 1);

}


int16 LoadAllParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle)
{
    Uint16 i = 0;
    Uint16 u16Buff[4];
    Uint32 u32Data = 0;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    for (i = 0; i < pstParamSaveHandle->pstParamCfg->u16ItemNum; i++)
    {
        pstParamSaveHandle->stParamWRIntf.ParamRead(PARAM_START_ADDR + (i * PARAM_BANK_SIZE),
                                                    (Uint16 *)u16Buff, 4);

        u32Data = u32Data | u16Buff[0] & 0xFF;
        u32Data = u32Data | ((u16Buff[1] & 0xFF) << 8);
        u32Data = u32Data | (((Uint32)u16Buff[2] & 0xFF) << 16);
        u32Data = u32Data | (((Uint32)u16Buff[3] & 0xFF) << 24);
        SetParamValue(pstParamSaveHandle, &u32Data, i);

    }
    return 1;
}

int16 SaveAllParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle)
{
    Uint16 i = 0;
    Uint32 u32Buff;
    Uint16 u16Data[4];

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    for (i = 0; i < pstParamSaveHandle->pstParamCfg->u16ItemNum; i++)
    {

        GetParamValue(pstParamSaveHandle, &u32Buff, i);
        u16Data[0] = u32Buff & 0xFF;
        u16Data[1] = (u32Buff >> 8) & 0xFF;
        u16Data[2] = (u32Buff >> 16) & 0xFF;
        u16Data[3] = (u32Buff >> 24) & 0xFF;

        pstParamSaveHandle->stParamWRIntf.ParamWrite(PARAM_START_ADDR + (i * PARAM_BANK_SIZE),
                                                     (Uint16 *)u16Data, 4);
    }
    return 1;
}

int16 LoadSingleParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle,  Uint16 u16Index)
{
    int32 i32Result = 0;
    Uint16 u16Buff[4];
    Uint32 u32Data = 0;
    int16 i16Ret = 0;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    i32Result = BinarySearch(pstParamSaveHandle, u16Index);

    if (i32Result)
    {
        pstParamSaveHandle->stParamWRIntf.ParamRead(PARAM_START_ADDR + (i32Result * PARAM_BANK_SIZE),
                                                    (Uint16 *)u16Buff, 4);

        u32Data = u32Data | u16Buff[0] & 0xFF;
        u32Data = u32Data | ((u16Buff[1] & 0xFF) << 8);
        u32Data = u32Data | (((Uint32)u16Buff[2] & 0xFF) << 16);
        u32Data = u32Data | (((Uint32)u16Buff[3] & 0xFF) << 24);
        SetParamValue(pstParamSaveHandle, &u32Data, i32Result);
        i16Ret = 1;
    }

    return i16Ret;

}




int16 SaveSingleParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index)
{
    int32 i32Result = 0;
    Uint32 u32Buff;
    Uint16 u16Data[4];
    int16 i16Ret = 0;

    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }

    i32Result = BinarySearch(pstParamSaveHandle, u16Index);

    if (i32Result)
    {
        GetParamValue(pstParamSaveHandle, &u32Buff, u16Index);
        u16Data[0] = u32Buff & 0xFF;
        u16Data[1] = (u32Buff >> 8) & 0xFF;
        u16Data[2] = (u32Buff >> 16) & 0xFF;
        u16Data[3] = (u32Buff >> 24) & 0xFF;

        pstParamSaveHandle->stParamWRIntf.ParamWrite(PARAM_START_ADDR + (i32Result * PARAM_BANK_SIZE),
                                                     (Uint16 *)u16Data, 4);
        i16Ret = 1;
    }

    return i16Ret;


}

int16 ParamSaveHandle(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle)
{
    ST_PARAM_CMD stParamCmd;
    if (pstParamSaveHandle == NULL)
    {
        return -1;
    }
    stParamCmd.u16Cmd = 0;
    ReadRingBuff(&pstParamSaveHandle->stParamCMD, &stParamCmd, 1);

    switch(stParamCmd.u16Cmd)
    {
    case CMD_SAVE_SING_PARAM:
        SaveSingleParamProc(pstParamSaveHandle, stParamCmd.u16Index);
        break;

    case CMD_SAVE_ALL_PARAM:
        SaveAllParamProc(pstParamSaveHandle);
        break;

    case CMD_LOAD_SING_PARAM:
        LoadSingleParamProc(pstParamSaveHandle, stParamCmd.u16Index);
        break;

    case CMD_LOAR_ALL_PARAM:
        LoadAllParamProc(pstParamSaveHandle);
        break;

    default:
        break;
    }
    return 1;
}


#ifdef __cplusplus
}
#endif

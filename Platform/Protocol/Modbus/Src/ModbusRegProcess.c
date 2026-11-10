/**
 * @file ModbusRegProcess.c
 * @author hanyuhang(008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "ModbusRegProcess.h"
#include "ModbusPublic.h"

#ifdef __cplusplus
extern "C" {
#endif





/**
 * @brief ReadModbusRegisterData
 *          从Modbus寄存器表中读取单个寄存器子项，寄存器表的地址需要保持增序，否则会出现查找不到的情况
 * 
 * @param pstProtocolTable - Modbus寄存器表
 * @param u16RegAddr - 寄存器地址
 * @param pstGetReg - 获取到的单个寄存器子项
 * @return int16 -  -1：入参异常
 *                  -2：超出寄存器地址范围
 *                  -3：未找到有效地址
 *                  0：寄存器地址存在
 */
int16 ReadModbusRegisterData(ST_REG_SLAVE_CFG *pstProtocolTable, Uint16 u16RegAddr, ST_REG_SLAVE_ITEM *pstGetReg)
{
    Uint16 u16Len = 0, u16Low = 0, u16High = 0, u16Mid = 0;
    ST_REG_SLAVE_ITEM *pstRegStart = NULL;
    ST_REG_SLAVE_ITEM *pstRegTmp = NULL;
    ST_REG_SLAVE_ITEM *pstRegTmpMix = NULL;
    ST_REG_SLAVE_ITEM *pstRegTmpMax = NULL;

    if ((pstProtocolTable == NULL) || (pstGetReg == NULL))
    {
        return -1;
    }

    // 获取寄存器表的上下限
    pstRegTmpMix = (ST_REG_SLAVE_ITEM *)&pstProtocolTable->pstReg[0];
    pstRegTmpMax = (ST_REG_SLAVE_ITEM *)&pstProtocolTable->pstReg[pstProtocolTable->u16Num - 1];

    // 判断是否超出寄存器地址范围
    if ((u16RegAddr < pstRegTmpMix->u16RegAddr) || (u16RegAddr > pstRegTmpMax->u16RegAddr))
    {
        return -2;
    }

    pstRegStart = pstProtocolTable->pstReg;
    u16Len = pstProtocolTable->u16Num;

    u16High = u16Len - 1;

    // 2分查找法
    while (u16Low <= u16High)
    {
        u16Mid = (u16High + u16Low) >> 1;
        pstRegTmp = pstRegStart + u16Mid;

        if (u16RegAddr < pstRegTmp->u16RegAddr)
        {
            u16High = u16Mid - 1;
        }
        else if (u16RegAddr > pstRegTmp->u16RegAddr)
        {
            u16Low = u16Mid + 1;
        }
        else
        {
            pstGetReg->u16RegAddr = pstRegTmp->u16RegAddr;
            pstGetReg->bRWType = pstRegTmp->bRWType;
            pstGetReg->bValType = pstRegTmp->bValType;
            pstGetReg->bParamSaveType = pstRegTmp->bParamSaveType;
            pstGetReg->puRegVal = pstRegTmp->puRegVal;
            pstGetReg->u32UpLimitVal = pstRegTmp->u32UpLimitVal;
            pstGetReg->i32DownLimitVal = pstRegTmp->i32DownLimitVal;

            // 正常返回
            return 0;
        }
    }

    // 未找到有效地址
    return -3;

}


/**
 * @brief SetRegValToMdsMsg
 *          将寄存器数据封装进待发送的消息缓存区
 * 
 * @param pu16DataBuff - 消息缓存区：16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 * @param pstSetRegVal - 寄存器数据
 * @param u16Index - 消息缓存区的地址偏移
 * @return int16 -  -1：入参异常
 *                  1：操作了一个寄存器
 *                  2：操作了两个寄存器
 */
int16 SetRegValToMdsMsg(Uint16 *pu16DataBuff, ST_REG_SLAVE_ITEM *pstSetRegVal, Uint16 u16Index)
{
    Uint16 u16RegNum = 0;
    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;

    if (NULL == pu16DataBuff || NULL == pstSetRegVal)
    {
        return -1;
    }

    switch(pstSetRegVal->bValType)
    {
        case TYPE_UINT8:
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]      = HI_UINT16(0x00);
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = LO_UINT16(pstSetRegVal->puRegVal->u8Val);
            u16RegNum = 1;
            break;

        case TYPE_INT8:
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]      = HI_UINT16(0x00);
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = LO_UINT16(pstSetRegVal->puRegVal->i8Val);
            u16RegNum = 1;
            break;

        case TYPE_UINT16:
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]      = HI_UINT16(pstSetRegVal->puRegVal->u16Val);
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = LO_UINT16(pstSetRegVal->puRegVal->u16Val);
            u16RegNum = 1;
            break;

        case TYPE_INT16:
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]      = HI_UINT16(pstSetRegVal->puRegVal->i16Val);
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = LO_UINT16(pstSetRegVal->puRegVal->i16Val);
            u16RegNum = 1;
            break;

        case TYPE_UINT32:
            // 高字在前，低字在后；高字节在前，低字节在后（即：ABCD字节序、大端字节序+大端字序）
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]  = (pstSetRegVal->puRegVal->u32Val >> 24) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = (pstSetRegVal->puRegVal->u32Val >> 16) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 2]      = (pstSetRegVal->puRegVal->u32Val >> 8) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 3]  = (pstSetRegVal->puRegVal->u32Val & 0xFF);
            u16RegNum = 2;
            break;

        case TYPE_INT32:
            // 高字在前，低字在后；高字节在前，低字节在后（即：ABCD字节序、大端字节序+大端字序）
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]  = (pstSetRegVal->puRegVal->i32Val >> 24) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = (pstSetRegVal->puRegVal->i32Val >> 16) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 2]      = (pstSetRegVal->puRegVal->i32Val >> 8) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 3]  = (pstSetRegVal->puRegVal->i32Val) & 0xFF;
            u16RegNum = 2;
            break;

        case TYPE_FLOAT:
            // 高字在前，低字在后；高字节在前，低字节在后
            uFloatByte.fVal = pstSetRegVal->puRegVal->fVal;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX]      = (uFloatByte.u16Val[1] >> 8) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 1]  = (uFloatByte.u16Val[1]) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 2]  = (uFloatByte.u16Val[0] >> 8) & 0xFF;
            pu16DataBuff[2 * u16Index + MDS_RDREPLY_VAL_ST_ADDR_INDEX + 3]  = (uFloatByte.u16Val[0]) & 0xFF;
            u16RegNum = 2;
            break;

        default:
            u16RegNum = 1;
            break;
    }
    return u16RegNum;
}


/**
 * @brief LoadRegValFromMdsMsg
 *          根据值类型(u16ValType)，将原始数据解析存放进临时的寄存器(pstSrcRegVal)
 * 
 * @param pu16DataBuff - 消息缓存区：16位数组指针，实际上只用到了低8位，在DSP平台操作16位数据比操作8位数据快
 * @param pstSrcRegVal - 临时的寄存器
 * @param u16ValType - 值类型
 * @param u16RegNo - 获取第u16RegNo个的寄存器
 * @param u16Index - 起始位置偏移
 * @return int16 -  -1：入参异常
 *                  1：操作了一个寄存器
 *                  2：操作了两个寄存器
 */
int16 LoadRegValFromMdsMsg(Uint16 *pu16DataBuff, ST_REG_SLAVE_ITEM *pstSrcRegVal, Uint16 u16ValType, Uint16 u16RegNo, Uint16 u16Index)
{
    Uint16 u16RegNum = 0;
    Uint16 u16ValHigh = 0, u16ValLow = 0;
    Uint32 u32Val = 0;
    union unValue
    {
        float fVal;
        Uint16 u16Val[2];
    } uFloatByte;

    if ((NULL == pu16DataBuff) || (NULL == pstSrcRegVal))
    {
        return -1;
    }

    u16ValHigh = pu16DataBuff[u16Index + 2 * u16RegNo];
    u16ValLow = pu16DataBuff[u16Index + 2 * u16RegNo + 1];

    switch (u16ValType)
    {
        case TYPE_UINT8:
            pstSrcRegVal->puRegVal->u8Val = u16ValLow;
            u16RegNum = 1;
            break;

        case TYPE_INT8:
            pstSrcRegVal->puRegVal->i8Val = u16ValLow;
            u16RegNum = 1;
            break;

        case TYPE_UINT16:
            pstSrcRegVal->puRegVal->u16Val = BUILD_UINT16(u16ValLow, u16ValHigh);
            u16RegNum = 1;
            break;

        case TYPE_INT16:
            pstSrcRegVal->puRegVal->i16Val = BUILD_UINT16(u16ValLow, u16ValHigh);
            u16RegNum = 1;
            break;

        case TYPE_UINT32:
            // 高字在前，低字在后；高字节在前，低字节在后（即：ABCD字节序、大端字节序+大端字序）
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo] << 24);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 1] << 16);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 2] << 8);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 3]);
            pstSrcRegVal->puRegVal->u32Val = u32Val;
            u16RegNum = 2;
            break;

        case TYPE_INT32:
            // 高字在前，低字在后；高字节在前，低字节在后（即：ABCD字节序、大端字节序+大端字序）
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo] << 24);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 1] << 16);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 2] << 8);
            u32Val = u32Val | ((Uint32)pu16DataBuff[u16Index + 2 * u16RegNo + 3]);
            pstSrcRegVal->puRegVal->i32Val = u32Val;
            u16RegNum = 2;
            break;

        case TYPE_FLOAT:
            uFloatByte.u16Val[1] = (pu16DataBuff[u16Index + 2 * u16RegNo] << 8) | (pu16DataBuff[u16Index + 2 * u16RegNo + 1]);
            uFloatByte.u16Val[0] = (pu16DataBuff[u16Index + 2 * u16RegNo + 2]  << 8) | (pu16DataBuff[u16Index + 2 * u16RegNo + 3]);
//            uFloatByte.u16Val[0] = (pu16DataBuff[u16Index + 2 * u16RegNo]) | (pu16DataBuff[u16Index + 2 * u16RegNo + 1] << 8);
//            uFloatByte.u16Val[1] = (pu16DataBuff[u16Index + 2 * u16RegNo + 2]) | (pu16DataBuff[u16Index + 2 * u16RegNo + 3] << 8);
            pstSrcRegVal->puRegVal->fVal = uFloatByte.fVal;
            u16RegNum = 2;
            break;
        default :
            u16RegNum = 1;
            break;
    }

    return u16RegNum;
}


/**
 * @brief SetRegValByType
 *          寄存器值赋值
 * 
 * @param pstRegDrc - 目标寄存器
 * @param pstRegSrc - 源寄存器
 * @return int16 -  -1：入参异常
 *                  -2：目标寄存器上下限属性配置异常
 *                  -3：目标寄存器值类型配置异常
 *                  0：值超限
 *                  1：成功
 */
int16 SetRegValByType(ST_REG_SLAVE_ITEM *pstRegDrc, ST_REG_SLAVE_ITEM *pstRegSrc)
{
    int16 i16Ret = 0;
    if ((pstRegDrc == NULL) || (pstRegSrc == NULL))
    {
        return -1;
    }

    if ((int64)pstRegDrc->i32DownLimitVal > pstRegDrc->u32UpLimitVal)
    {
        return -2;
    }

    switch (pstRegDrc->bValType)
    {
        case TYPE_UINT8:
            if ((pstRegSrc->puRegVal->u8Val <= pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->u8Val >= pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->u8Val = pstRegSrc->puRegVal->u8Val;
                i16Ret = 1;
            }
            break;

        case TYPE_INT8:
            if ((pstRegSrc->puRegVal->i8Val <= (int32)pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->i8Val >= pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->i8Val = pstRegSrc->puRegVal->i8Val;
                i16Ret = 1;
            }
            break;

        case TYPE_UINT16:
            if ((pstRegSrc->puRegVal->u16Val <= pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->u16Val >= pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->u16Val = pstRegSrc->puRegVal->u16Val;
                i16Ret = 1;
            }
            break;

        case TYPE_INT16:
            if ((pstRegSrc->puRegVal->i16Val <= (int32)pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->i16Val >= pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->i16Val = pstRegSrc->puRegVal->i16Val;
                i16Ret = 1;
            }
            break;

        case TYPE_UINT32:
            if ((pstRegSrc->puRegVal->u32Val <= pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->u32Val >= (int64)pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->u32Val = pstRegSrc->puRegVal->u32Val;
                i16Ret = 1;
            }
            break;

        case TYPE_INT32:
            if (((int64)pstRegSrc->puRegVal->i32Val <= pstRegDrc->u32UpLimitVal) && (pstRegSrc->puRegVal->i32Val >= (int32)pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->i32Val = pstRegSrc->puRegVal->i32Val;
                i16Ret = 1;
            }
            break;

        case TYPE_FLOAT:
            // 浮点数不能直接比较需要转化成整数
            if (((int64)pstRegSrc->puRegVal->fVal <= pstRegDrc->u32UpLimitVal) && ((int64)pstRegSrc->puRegVal->fVal >= pstRegDrc->i32DownLimitVal))
            {
                pstRegDrc->puRegVal->fVal = pstRegSrc->puRegVal->fVal;
                i16Ret = 1;
            }
            break;
        default :
            i16Ret = -3;
            break;
    }

    return i16Ret;
}

#ifdef __cplusplus
}
#endif



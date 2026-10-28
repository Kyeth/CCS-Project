/**
 * @file ModbusSlave.c
 * @author yetingheng (004182)
 * @brief Modbus RTU 从机实现（03 / 06 / 10 三个功能码）
 * @version 0.1
 * @date 2026-10-23
 * @copyright Copyright (c) 2026
 */

#include "ModbusSlave.h"
#include "EepromParam.h"   /* 写属性带 SAVE 位时，写入成功后标脏 */
#include "Hw_SciDriver.h"
#include "Crc16.h"
#include "AtsProto.h"   /* ATS 旁听：抓 ARM<->ATS 的 Modbus 应答 */

/* 从机数量上限 */
#define MB_SLAVE_MAX                (4U)

/* 32 位量与浮点的转换 */
typedef union
{
    float32 f32Val;
    Uint32  u32Val;
} U_MB_F32;

/* 运行时状态 */
static const ST_MB_CFG *gMbCfg = (const ST_MB_CFG *)0;
static Uint16 gMbFrame[MB_FRAME_MAX];
static Uint16 gMbResp[MB_FRAME_MAX];
static Uint16 gMbFrameCnt[MB_SLAVE_MAX];
static Uint16 gMbErrCnt[MB_SLAVE_MAX];
static Uint16 gMbLastFunc[MB_SLAVE_MAX];

/* =1 表示当前帧是广播帧：照常执行写操作，但一律不应答 */
static Uint16 gMbNoResp = 0U;

/* 一个表项占用几个寄存器 */
static Uint16 MbRegCount(Uint16 u16ValType)
{
    if ((u16ValType == MB_TYPE_U32) || (u16ValType == MB_TYPE_I32) ||
        (u16ValType == MB_TYPE_F32))
    {
        return 2U;
    }

    return 1U;
}

/**
 * @brief 在表里找覆盖地址 u16Addr 的表项
 * @param pu16Offset 返回地址在该表项内的偏移（0 或 1）
 * @return 1 表示找到
 */
static Uint16 MbFindReg(const ST_MB_SLAVE_CFG *pstSlave, Uint16 u16Addr,
                        const ST_MB_REG_ITEM **ppstItem, Uint16 *pu16Offset)
{
    Uint16 u16Idx;
    Uint16 u16Cnt;

    for (u16Idx = 0U; u16Idx < pstSlave->u16RegNum; u16Idx++)
    {
        u16Cnt = MbRegCount(pstSlave->pstRegItem[u16Idx].u16ValType);

        if ((u16Addr >= pstSlave->pstRegItem[u16Idx].u16RegAddr) &&
            (u16Addr < (Uint16)(pstSlave->pstRegItem[u16Idx].u16RegAddr + u16Cnt)))
        {
            *ppstItem = &pstSlave->pstRegItem[u16Idx];
            *pu16Offset = (Uint16)(u16Addr - pstSlave->pstRegItem[u16Idx].u16RegAddr);
            return 1U;
        }
    }

    return 0U;
}

/* 读表项的第 u16Offset 个寄存器（16 位切片） */
static Uint16 MbReadWord(const ST_MB_REG_ITEM *pstItem, Uint16 u16Offset)
{
    U_MB_F32 unVal;
    volatile Uint16 *pu16Ptr;
    volatile Uint32 *pu32Ptr;

    if (pstItem->pvData == (void *)0)
    {
        return 0U;
    }

    pu16Ptr = (volatile Uint16 *)pstItem->pvData;
    pu32Ptr = (volatile Uint32 *)pstItem->pvData;

    switch (pstItem->u16ValType)
    {
        case MB_TYPE_U16:
            return *pu16Ptr;

        case MB_TYPE_I16:
            return (Uint16)(*((volatile int16 *)pstItem->pvData));

        case MB_TYPE_U32:
            unVal.u32Val = *pu32Ptr;
            break;

        case MB_TYPE_I32:
            unVal.u32Val = (Uint32)(*((volatile int32 *)pstItem->pvData));
            break;

        case MB_TYPE_F32:
            unVal.f32Val = *((volatile float32 *)pstItem->pvData);
            break;

        default:
            return 0U;
    }

    /* 32 位量按 MB_WORD_HIGH_FIRST 决定高低字顺序 */
#if (MB_WORD_HIGH_FIRST == 0)
    if (u16Offset == 0U)
    {
        return (Uint16)(unVal.u32Val & 0x0000FFFFUL);
    }
    return (Uint16)((unVal.u32Val >> 16U) & 0x0000FFFFUL);
#else
    if (u16Offset == 0U)
    {
        return (Uint16)((unVal.u32Val >> 16U) & 0x0000FFFFUL);
    }
    return (Uint16)(unVal.u32Val & 0x0000FFFFUL);
#endif
}

/* 把 int32 限幅到表项范围内 */
static int32 MbClampI32(const ST_MB_REG_ITEM *pstItem, int32 i32Val)
{
    Uint32 u32Max;

    u32Max = pstItem->u32Max;

    if (i32Val < pstItem->i32Min)
    {
        return pstItem->i32Min;
    }

    /* 上限按无符号解释，兼容 0~65535 这类范围 */
    if ((i32Val >= 0) && ((Uint32)i32Val > u32Max))
    {
        return (int32)u32Max;
    }

    return i32Val;
}

/**
 * @brief 写表项的第 u16Offset 个寄存器
 * @return 0 表示成功，否则为异常码
 */
static Uint16 MbWriteWordRaw(const ST_MB_REG_ITEM *pstItem, Uint16 u16Offset, Uint16 u16Val)
{
    U_MB_F32 unVal;
    volatile Uint32 *pu32Ptr;
    int32 i32Val;
    float32 f32Val;

    if (pstItem->pvData == (void *)0)
    {
        return MB_EX_SLAVE_FAIL;
    }

    if ((pstItem->u16Attr & MB_ATTR_RW) == 0U)
    {
        /* 只读寄存器按"非法数据地址"处理，与多数从机实现一致 */
        return MB_EX_ILLEGAL_ADDR;
    }

    pu32Ptr = (volatile Uint32 *)pstItem->pvData;

    switch (pstItem->u16ValType)
    {
        case MB_TYPE_U16:
            i32Val = MbClampI32(pstItem, (int32)u16Val);
            *((volatile Uint16 *)pstItem->pvData) = (Uint16)i32Val;
            return MB_EX_NONE;

        case MB_TYPE_I16:
            i32Val = MbClampI32(pstItem, (int32)((int16)u16Val));
            *((volatile int16 *)pstItem->pvData) = (int16)i32Val;
            return MB_EX_NONE;

        case MB_TYPE_U32:
        case MB_TYPE_I32:
            unVal.u32Val = *pu32Ptr;
#if (MB_WORD_HIGH_FIRST == 0)
            if (u16Offset == 0U)
            {
                unVal.u32Val = (unVal.u32Val & 0xFFFF0000UL) | (Uint32)u16Val;
            }
            else
            {
                unVal.u32Val = (unVal.u32Val & 0x0000FFFFUL) | ((Uint32)u16Val << 16U);
            }
#else
            if (u16Offset == 0U)
            {
                unVal.u32Val = (unVal.u32Val & 0x0000FFFFUL) | ((Uint32)u16Val << 16U);
            }
            else
            {
                unVal.u32Val = (unVal.u32Val & 0xFFFF0000UL) | (Uint32)u16Val;
            }
#endif
            i32Val = (int32)unVal.u32Val;
            i32Val = MbClampI32(pstItem, i32Val);
            if (pstItem->u16ValType == MB_TYPE_U32)
            {
                *pu32Ptr = (Uint32)i32Val;
            }
            else
            {
                *((volatile int32 *)pstItem->pvData) = i32Val;
            }
            return MB_EX_NONE;

        case MB_TYPE_F32:
            unVal.f32Val = *((volatile float32 *)pstItem->pvData);
#if (MB_WORD_HIGH_FIRST == 0)
            if (u16Offset == 0U)
            {
                unVal.u32Val = (unVal.u32Val & 0xFFFF0000UL) | (Uint32)u16Val;
            }
            else
            {
                unVal.u32Val = (unVal.u32Val & 0x0000FFFFUL) | ((Uint32)u16Val << 16U);
            }
#else
            if (u16Offset == 0U)
            {
                unVal.u32Val = (unVal.u32Val & 0x0000FFFFUL) | ((Uint32)u16Val << 16U);
            }
            else
            {
                unVal.u32Val = (unVal.u32Val & 0xFFFF0000UL) | (Uint32)u16Val;
            }
#endif
            f32Val = unVal.f32Val;
            if (f32Val < (float32)pstItem->i32Min)
            {
                f32Val = (float32)pstItem->i32Min;
            }
            if (f32Val > (float32)pstItem->u32Max)
            {
                f32Val = (float32)pstItem->u32Max;
            }
            *((volatile float32 *)pstItem->pvData) = f32Val;
            return MB_EX_NONE;

        default:
            return MB_EX_SLAVE_FAIL;
    }
}


/**
 * @brief 写表项的第 u16Offset 个寄存器（带掉电保存挂钩）
 *
 * @note 真正落地的是 MbWriteWordRaw()，这一层只负责"写成功了就把这个变量标脏"，
 *       这样协议层不需要认识 EepromParam，点表属性是唯一的开关。
 */
static Uint16 MbWriteWord(const ST_MB_REG_ITEM *pstItem, Uint16 u16Offset, Uint16 u16Val)
{
    Uint16 u16Ex;

    u16Ex = MbWriteWordRaw(pstItem, u16Offset, u16Val);

    if ((u16Ex == MB_EX_NONE) && ((pstItem->u16Attr & MB_ATTR_SAVE) != 0U))
    {
        (void)EepromParamMarkDirty(pstItem->pvData);
    }

    return u16Ex;
}

/* 发送异常应答：[addr][func|0x80][exCode][crc] */
static void MbSendException(Uint16 u16SlaveIdx, Uint16 u16Func, Uint16 u16ExCode)
{
    const ST_MB_SLAVE_CFG *pstSlave;
    Uint16 u16Len;

    pstSlave = &gMbCfg->pstSlaveCfg[u16SlaveIdx];

    /* 广播帧不出应答，只记一次错误 */
    if (gMbNoResp != 0U)
    {
        gMbErrCnt[u16SlaveIdx]++;
        return;
    }

    gMbResp[0] = pstSlave->u16SlaveAddr;
    gMbResp[1] = (Uint16)((u16Func & 0x007FU) | 0x0080U);
    gMbResp[2] = u16ExCode;

    u16Len = Crc16ModbusAppend(gMbResp, 3U);
    (void)Hw_SciTxWrite(pstSlave->u16SciId, gMbResp, u16Len);

    gMbErrCnt[u16SlaveIdx]++;
}

/* 03：读保持寄存器 */
static void MbReadHolding(Uint16 u16SlaveIdx, Uint16 u16Len)
{
    const ST_MB_SLAVE_CFG *pstSlave;
    const ST_MB_REG_ITEM *pstItem;
    Uint16 u16Start;
    Uint16 u16Qty;
    Uint16 u16Idx;
    Uint16 u16Offset;
    Uint16 u16ByteCnt;
    Uint16 u16RespLen;

    pstSlave = &gMbCfg->pstSlaveCfg[u16SlaveIdx];

    /* 广播不能读，直接忽略 */
    if (gMbNoResp != 0U)
    {
        return;
    }

    if (u16Len != 8U)
    {
        MbSendException(u16SlaveIdx, MB_FUNC_READ_HOLDING, MB_EX_ILLEGAL_VALUE);
        return;
    }

    u16Start = (Uint16)((gMbFrame[2] << 8U) | (gMbFrame[3] & 0x00FFU));
    u16Qty = (Uint16)((gMbFrame[4] << 8U) | (gMbFrame[5] & 0x00FFU));

    if ((u16Qty == 0U) || (u16Qty > MB_READ_REG_MAX))
    {
        MbSendException(u16SlaveIdx, MB_FUNC_READ_HOLDING, MB_EX_ILLEGAL_VALUE);
        return;
    }

    /* 先全量校验地址，避免应答到一半才发现越界 */
    for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
    {
        if (MbFindReg(pstSlave, (Uint16)(u16Start + u16Idx), &pstItem, &u16Offset) == 0U)
        {
            MbSendException(u16SlaveIdx, MB_FUNC_READ_HOLDING, MB_EX_ILLEGAL_ADDR);
            return;
        }
    }

    u16ByteCnt = (Uint16)(u16Qty * 2U);
    gMbResp[0] = pstSlave->u16SlaveAddr;
    gMbResp[1] = MB_FUNC_READ_HOLDING;
    gMbResp[2] = (Uint16)(u16ByteCnt & 0x00FFU);

    for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
    {
        (void)MbFindReg(pstSlave, (Uint16)(u16Start + u16Idx), &pstItem, &u16Offset);
        u16RespLen = MbReadWord(pstItem, u16Offset);
        gMbResp[3U + (u16Idx * 2U)] = (Uint16)((u16RespLen >> 8U) & 0x00FFU);
        gMbResp[4U + (u16Idx * 2U)] = (Uint16)(u16RespLen & 0x00FFU);
    }

    u16RespLen = Crc16ModbusAppend(gMbResp, (Uint16)(3U + u16ByteCnt));
    (void)Hw_SciTxWrite(pstSlave->u16SciId, gMbResp, u16RespLen);
}

/* 06：写单个寄存器 */
static void MbWriteSingle(Uint16 u16SlaveIdx, Uint16 u16Len)
{
    const ST_MB_SLAVE_CFG *pstSlave;
    const ST_MB_REG_ITEM *pstItem;
    Uint16 u16Addr;
    Uint16 u16Val;
    Uint16 u16Offset;
    Uint16 u16Ex;
    Uint16 u16RespLen;

    pstSlave = &gMbCfg->pstSlaveCfg[u16SlaveIdx];

    if (u16Len != 8U)
    {
        MbSendException(u16SlaveIdx, MB_FUNC_WRITE_SINGLE, MB_EX_ILLEGAL_VALUE);
        return;
    }

    u16Addr = (Uint16)((gMbFrame[2] << 8U) | (gMbFrame[3] & 0x00FFU));
    u16Val = (Uint16)((gMbFrame[4] << 8U) | (gMbFrame[5] & 0x00FFU));

    if (MbFindReg(pstSlave, u16Addr, &pstItem, &u16Offset) == 0U)
    {
        MbSendException(u16SlaveIdx, MB_FUNC_WRITE_SINGLE, MB_EX_ILLEGAL_ADDR);
        return;
    }

    u16Ex = MbWriteWord(pstItem, u16Offset, u16Val);
    if (u16Ex != MB_EX_NONE)
    {
        MbSendException(u16SlaveIdx, MB_FUNC_WRITE_SINGLE, u16Ex);
        return;
    }

    /* 广播写不出应答 */
    if (gMbNoResp != 0U)
    {
        return;
    }

    /* 06 的正常应答就是把请求原样回送 */
    for (u16RespLen = 0U; u16RespLen < 6U; u16RespLen++)
    {
        gMbResp[u16RespLen] = gMbFrame[u16RespLen];
    }
    gMbResp[0] = pstSlave->u16SlaveAddr;

    u16RespLen = Crc16ModbusAppend(gMbResp, 6U);
    (void)Hw_SciTxWrite(pstSlave->u16SciId, gMbResp, u16RespLen);
}

/* 10：写多个寄存器 */
static void MbWriteMulti(Uint16 u16SlaveIdx, Uint16 u16Len)
{
    const ST_MB_SLAVE_CFG *pstSlave;
    const ST_MB_REG_ITEM *pstItem;
    Uint16 u16Start;
    Uint16 u16Qty;
    Uint16 u16ByteCnt;
    Uint16 u16Idx;
    Uint16 u16Offset;
    Uint16 u16Ex;
    Uint16 u16Val;
    Uint16 u16RespLen;

    pstSlave = &gMbCfg->pstSlaveCfg[u16SlaveIdx];

    if (u16Len < 9U)
    {
        MbSendException(u16SlaveIdx, MB_FUNC_WRITE_MULTI, MB_EX_ILLEGAL_VALUE);
        return;
    }

    u16Start = (Uint16)((gMbFrame[2] << 8U) | (gMbFrame[3] & 0x00FFU));
    u16Qty = (Uint16)((gMbFrame[4] << 8U) | (gMbFrame[5] & 0x00FFU));
    u16ByteCnt = (Uint16)(gMbFrame[6] & 0x00FFU);

    if ((u16Qty == 0U) || (u16Qty > MB_WRITE_REG_MAX) ||
        (u16ByteCnt != (Uint16)(u16Qty * 2U)) ||
        (u16Len != (Uint16)(9U + u16ByteCnt)))
    {
        MbSendException(u16SlaveIdx, MB_FUNC_WRITE_MULTI, MB_EX_ILLEGAL_VALUE);
        return;
    }

    /* 先校验全部地址与写权限，避免写一半失败 */
    for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
    {
        if (MbFindReg(pstSlave, (Uint16)(u16Start + u16Idx), &pstItem, &u16Offset) == 0U)
        {
            MbSendException(u16SlaveIdx, MB_FUNC_WRITE_MULTI, MB_EX_ILLEGAL_ADDR);
            return;
        }

        if ((pstItem->u16Attr & MB_ATTR_RW) == 0U)
        {
            MbSendException(u16SlaveIdx, MB_FUNC_WRITE_MULTI, MB_EX_ILLEGAL_ADDR);
            return;
        }
    }

    for (u16Idx = 0U; u16Idx < u16Qty; u16Idx++)
    {
        (void)MbFindReg(pstSlave, (Uint16)(u16Start + u16Idx), &pstItem, &u16Offset);
        u16Val = (Uint16)((gMbFrame[7U + (u16Idx * 2U)] << 8U) |
                          (gMbFrame[8U + (u16Idx * 2U)] & 0x00FFU));
        u16Ex = MbWriteWord(pstItem, u16Offset, u16Val);
        if (u16Ex != MB_EX_NONE)
        {
            MbSendException(u16SlaveIdx, MB_FUNC_WRITE_MULTI, u16Ex);
            return;
        }
    }

    /* 广播写不出应答 */
    if (gMbNoResp != 0U)
    {
        return;
    }

    /* 应答：[addr][10][startHi][startLo][qtyHi][qtyLo][crc] */
    gMbResp[0] = pstSlave->u16SlaveAddr;
    gMbResp[1] = MB_FUNC_WRITE_MULTI;
    gMbResp[2] = gMbFrame[2];
    gMbResp[3] = gMbFrame[3];
    gMbResp[4] = gMbFrame[4];
    gMbResp[5] = gMbFrame[5];

    u16RespLen = Crc16ModbusAppend(gMbResp, 6U);
    (void)Hw_SciTxWrite(pstSlave->u16SciId, gMbResp, u16RespLen);
}

/* 处理一帧 */
static void MbHandleFrame(Uint16 u16SlaveIdx, Uint16 u16Len)
{
    const ST_MB_SLAVE_CFG *pstSlave;
    Uint16 u16Addr;
    Uint16 u16Func;

    pstSlave = &gMbCfg->pstSlaveCfg[u16SlaveIdx];

    if (u16Len < 4U)
    {
        return;
    }

    /* ATS 旁听：本串口若与 ARM 共用，凡 ATS 地址的报文都要过一遍解析器；
     * 必须在下面的“不是本机地址就丢弃”之前调用，否则永远看不到 */
    AtsProtoOnFrame(pstSlave->u16SciId, gMbFrame, u16Len);

    u16Addr = (Uint16)(gMbFrame[0] & 0x00FFU);

    /* 既不是本机地址也不是广播地址，直接不理 */
    if ((u16Addr != pstSlave->u16SlaveAddr) && (u16Addr != 0U))
    {
        return;
    }

    if (Crc16ModbusCheck(gMbFrame, u16Len) == 0U)
    {
        /* CRC 错一律不应答，避免总线上出现无意义的错误帧 */
        gMbErrCnt[u16SlaveIdx]++;
        return;
    }

    u16Func = (Uint16)(gMbFrame[1] & 0x00FFU);
    gMbLastFunc[u16SlaveIdx] = u16Func;

    /* 广播帧（地址 0）：照常执行写操作，但读和应答都不做 */
    /* 广播帧、或配成"只听不应答"的从机（TVS）：执行写但不回复 */
    gMbNoResp = ((u16Addr == 0U) || (pstSlave->bNoReply != 0U)) ? 1U : 0U;

    switch (u16Func)
    {
        case MB_FUNC_READ_HOLDING:
            MbReadHolding(u16SlaveIdx, u16Len);
            break;

        case MB_FUNC_WRITE_SINGLE:
            MbWriteSingle(u16SlaveIdx, u16Len);
            break;

        case MB_FUNC_WRITE_MULTI:
            MbWriteMulti(u16SlaveIdx, u16Len);
            break;

        default:
            MbSendException(u16SlaveIdx, u16Func, MB_EX_ILLEGAL_FUNC);
            break;
    }
}

void ModbusInit(const ST_MB_CFG *pstMbCfg)
{
    Uint16 u16Idx;

    gMbCfg = pstMbCfg;

    for (u16Idx = 0U; u16Idx < MB_SLAVE_MAX; u16Idx++)
    {
        gMbFrameCnt[u16Idx] = 0U;
        gMbErrCnt[u16Idx] = 0U;
        gMbLastFunc[u16Idx] = 0U;
    }
}

void ModbusTask(void *pArgV)
{
    Uint16 u16Idx;
    Uint16 u16Len;

    (void)pArgV;

    if ((gMbCfg == (const ST_MB_CFG *)0) ||
        (gMbCfg->pstSlaveCfg == (const ST_MB_SLAVE_CFG *)0))
    {
        return;
    }

    for (u16Idx = 0U; u16Idx < gMbCfg->u16SlaveNum; u16Idx++)
    {
        if (Hw_SciFrameIsReady(gMbCfg->pstSlaveCfg[u16Idx].u16SciId) == 0U)
        {
            continue;
        }

        u16Len = Hw_SciFrameRead(gMbCfg->pstSlaveCfg[u16Idx].u16SciId,
                                 gMbFrame, MB_FRAME_MAX);
        if (u16Len == 0U)
        {
            continue;
        }

        gMbFrameCnt[u16Idx]++;
        MbHandleFrame(u16Idx, u16Len);
    }
}

Uint16 ModbusGetFrameCnt(Uint16 u16SlaveIdx)
{
    if (u16SlaveIdx >= MB_SLAVE_MAX)
    {
        return 0U;
    }

    return gMbFrameCnt[u16SlaveIdx];
}

Uint16 ModbusGetErrCnt(Uint16 u16SlaveIdx)
{
    if (u16SlaveIdx >= MB_SLAVE_MAX)
    {
        return 0U;
    }

    return gMbErrCnt[u16SlaveIdx];
}

Uint16 ModbusGetLastFunc(Uint16 u16SlaveIdx)
{
    if (u16SlaveIdx >= MB_SLAVE_MAX)
    {
        return 0U;
    }

    return gMbLastFunc[u16SlaveIdx];
}

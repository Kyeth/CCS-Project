/**
 * @file ModbusTbl.c
 * @brief Modbus 点表：校验、类型转换、读与"先校验后提交"的写。
 */

#include "ModbusTbl.h"

typedef union {
    Uint16 u16;
    int16 i16;
    Uint32 u32;
    int32 i32;
    float  f32;
} MB_VALUE;

static Uint16 MbFindPoint(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                          Uint16 u16Addr, const ST_MODBUS_POINT **ppstOut);
static void   MbLoadValue(const ST_MODBUS_POINT *pstPoint, MB_VALUE *pstValue);
static void   MbStoreValue(const ST_MODBUS_POINT *pstPoint, const MB_VALUE *pstValue);
static void   MbEncodeRegs(const MB_VALUE *pstValue, Uint16 u16Type, Uint16 *pu16Out);
static void   MbDecodeRegs(const Uint16 *pu16In, Uint16 u16Type, MB_VALUE *pstValue);
static Uint16 MbInLimits(const ST_MODBUS_POINT *pstPoint, const MB_VALUE *pstValue);
static Uint16 MbLimitsDeclared(const ST_MODBUS_POINT *pstPoint, Uint16 u16Regs);

Uint16 ModbusTblRegs(Uint16 u16Type)
{
    Uint16 u16Regs;

    switch (u16Type) {
    case MB_PT_U16:
    case MB_PT_I16:
        u16Regs = 1U;
        break;
    case MB_PT_U32:
    case MB_PT_I32:
    case MB_PT_F32:
        u16Regs = 2U;
        break;
    default:
        u16Regs = 0U;
        break;
    }

    return u16Regs;
}

Uint16 ModbusTblValidate(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count)
{
    Uint32 u32NextEnd = 0UL;
    Uint16 u16Regs;
    Uint16 u16Index;

    if ((pstTbl == 0) || (u16Count == 0U)) {
        return 0U;
    }

    for (u16Index = 0U; u16Index < u16Count; u16Index++) {
        const ST_MODBUS_POINT *pstPoint = &pstTbl[u16Index];

        u16Regs = ModbusTblRegs(pstPoint->u16Type);
        if (u16Regs == 0U) {
            return 0U;   /* 未定义类型 */
        }
        if ((pstPoint->u16Access < MB_PT_READ) || (pstPoint->u16Access > MB_PT_READWRITE)) {
            return 0U;   /* 权限非法 */
        }
        if (pstPoint->pVar == 0) {
            return 0U;   /* 未绑定变量 */
        }
        if (u16Index > 0U) {
            if ((Uint32)pstPoint->u16Addr < u32NextEnd) {
                return 0U;   /* 地址未递增或与前一点重叠 */
            }
        }
        u32NextEnd = (Uint32)pstPoint->u16Addr + (Uint32)u16Regs;
        if (u32NextEnd > 65536UL) {
            return 0U;   /* 越过 16 位地址空间 */
        }
        if (MbLimitsDeclared(pstPoint, u16Regs) == 0U) {
            return 0U;   /* 限值与类型不符 */
        }
    }

    return 1U;
}

Uint16 ModbusTblRead(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                     Uint16 u16Start, Uint16 u16Qty, Uint16 *pu16Out)
{
    Uint32 u32Pos = (Uint32)u16Start;
    Uint32 u32End;
    Uint16 u16Out = 0U;

    if ((pstTbl == 0) || (pu16Out == 0) || (u16Qty == 0U) || (u16Qty > MB_MAX_REGISTERS)) {
        return MB_EX_ILLEGAL_VALUE;
    }

    u32End = u32Pos + (Uint32)u16Qty;
    if (u32End > 65536UL) {
        return MB_EX_ILLEGAL_ADDR;
    }

    while (u32Pos < u32End) {
        const ST_MODBUS_POINT *pstPoint = 0;
        MB_VALUE stValue;
        Uint16 u16Regs;

        if (MbFindPoint(pstTbl, u16Count, (Uint16)u32Pos, &pstPoint) == 0U) {
            return MB_EX_ILLEGAL_ADDR;   /* 地址不存在，或落在多寄存器点的中间 */
        }
        if ((pstPoint->u16Access & MB_PT_READ) == 0U) {
            return MB_EX_ILLEGAL_ADDR;   /* 只写点不可读 */
        }

        u16Regs = ModbusTblRegs(pstPoint->u16Type);
        if (((Uint32)pstPoint->u16Addr + (Uint32)u16Regs) > u32End) {
            return MB_EX_ILLEGAL_ADDR;   /* 32 位点只取了一个寄存器：不完整访问 */
        }

        MbLoadValue(pstPoint, &stValue);
        MbEncodeRegs(&stValue, pstPoint->u16Type, &pu16Out[u16Out]);

        u16Out = (Uint16)(u16Out + u16Regs);
        u32Pos += (Uint32)u16Regs;
    }

    return MB_EX_NONE;
}

Uint16 ModbusTblWrite(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                      Uint16 u16Start, Uint16 u16Qty, const Uint16 *pu16In)
{
    Uint32 u32Pos;
    Uint32 u32End;
    Uint16 u16In;
    Uint16 u16Pass;

    if ((pstTbl == 0) || (pu16In == 0) || (u16Qty == 0U) || (u16Qty > MB_MAX_REGISTERS)) {
        return MB_EX_ILLEGAL_VALUE;
    }

    u32End = (Uint32)u16Start + (Uint32)u16Qty;
    if (u32End > 65536UL) {
        return MB_EX_ILLEGAL_ADDR;
    }

    /* 第一遍只校验：任何一项不合法都不写，避免半段生效。 */
    for (u16Pass = 0U; u16Pass < 2U; u16Pass++) {
        u32Pos = (Uint32)u16Start;
        u16In = 0U;

        while (u32Pos < u32End) {
            const ST_MODBUS_POINT *pstPoint = 0;
            MB_VALUE stValue;
            Uint16 u16Regs;

            if (MbFindPoint(pstTbl, u16Count, (Uint16)u32Pos, &pstPoint) == 0U) {
                return MB_EX_ILLEGAL_ADDR;
            }
            if ((pstPoint->u16Access & MB_PT_WRITE) == 0U) {
                return MB_EX_ILLEGAL_ADDR;   /* 只读点被写 */
            }

            u16Regs = ModbusTblRegs(pstPoint->u16Type);
            if (((Uint32)pstPoint->u16Addr + (Uint32)u16Regs) > u32End) {
                return MB_EX_ILLEGAL_ADDR;   /* 32 位点只写了一个寄存器 */
            }

            MbDecodeRegs(&pu16In[u16In], pstPoint->u16Type, &stValue);
            if (MbInLimits(pstPoint, &stValue) == 0U) {
                return MB_EX_ILLEGAL_VALUE;  /* 超出限值 */
            }

            if (u16Pass != 0U) {
                MbStoreValue(pstPoint, &stValue);
            }

            u16In = (Uint16)(u16In + u16Regs);
            u32Pos += (Uint32)u16Regs;
        }
    }

    return MB_EX_NONE;
}

static Uint16 MbFindPoint(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                          Uint16 u16Addr, const ST_MODBUS_POINT **ppstOut)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < u16Count; u16Index++) {
        if (pstTbl[u16Index].u16Addr == u16Addr) {
            *ppstOut = &pstTbl[u16Index];
            return 1U;
        }
        if (pstTbl[u16Index].u16Addr > u16Addr) {
            break;   /* 地址递增，后面不会再有 */
        }
    }

    return 0U;
}

static void MbLoadValue(const ST_MODBUS_POINT *pstPoint, MB_VALUE *pstValue)
{
    switch (pstPoint->u16Type) {
    case MB_PT_U16:
        pstValue->u16 = *(const volatile Uint16 *)pstPoint->pVar;
        break;
    case MB_PT_I16:
        pstValue->i16 = *(const volatile int16 *)pstPoint->pVar;
        break;
    case MB_PT_U32:
        pstValue->u32 = *(const volatile Uint32 *)pstPoint->pVar;
        break;
    case MB_PT_I32:
        pstValue->i32 = *(const volatile int32 *)pstPoint->pVar;
        break;
    default:
        pstValue->f32 = *(const volatile float *)pstPoint->pVar;
        break;
    }
}

static void MbStoreValue(const ST_MODBUS_POINT *pstPoint, const MB_VALUE *pstValue)
{
    switch (pstPoint->u16Type) {
    case MB_PT_U16:
        *(volatile Uint16 *)pstPoint->pVar = pstValue->u16;
        break;
    case MB_PT_I16:
        *(volatile int16 *)pstPoint->pVar = pstValue->i16;
        break;
    case MB_PT_U32:
        *(volatile Uint32 *)pstPoint->pVar = pstValue->u32;
        break;
    case MB_PT_I32:
        *(volatile int32 *)pstPoint->pVar = pstValue->i32;
        break;
    default:
        *(volatile float *)pstPoint->pVar = pstValue->f32;
        break;
    }
}

static void MbEncodeRegs(const MB_VALUE *pstValue, Uint16 u16Type, Uint16 *pu16Out)
{
    Uint32 u32Bits;

    switch (u16Type) {
    case MB_PT_U16:
        pu16Out[0] = pstValue->u16;
        break;
    case MB_PT_I16:
        pu16Out[0] = (Uint16)pstValue->i16;
        break;
    case MB_PT_F32:
        u32Bits = (Uint32)pstValue->f32;
        pu16Out[0] = (Uint16)(u32Bits >> 16);
        pu16Out[1] = (Uint16)(u32Bits & 0xFFFFUL);
        break;
    case MB_PT_I32:
        u32Bits = (Uint32)pstValue->i32;
        pu16Out[0] = (Uint16)(u32Bits >> 16);
        pu16Out[1] = (Uint16)(u32Bits & 0xFFFFUL);
        break;
    default:   /* MB_PT_U32 */
        u32Bits = pstValue->u32;
        pu16Out[0] = (Uint16)(u32Bits >> 16);
        pu16Out[1] = (Uint16)(u32Bits & 0xFFFFUL);
        break;
    }
}

static void MbDecodeRegs(const Uint16 *pu16In, Uint16 u16Type, MB_VALUE *pstValue)
{
    Uint32 u32Bits;

    switch (u16Type) {
    case MB_PT_U16:
        pstValue->u16 = pu16In[0];
        break;
    case MB_PT_I16:
        pstValue->i16 = (int16)pu16In[0];
        break;
    case MB_PT_F32:
        u32Bits = ((Uint32)pu16In[0] << 16) | (Uint32)pu16In[1];
        pstValue->f32 = (float)u32Bits;
        break;
    case MB_PT_I32:
        u32Bits = ((Uint32)pu16In[0] << 16) | (Uint32)pu16In[1];
        pstValue->i32 = (int32)u32Bits;
        break;
    default:   /* MB_PT_U32 */
        pstValue->u32 = ((Uint32)pu16In[0] << 16) | (Uint32)pu16In[1];
        break;
    }
}

static Uint16 MbInLimits(const ST_MODBUS_POINT *pstPoint, const MB_VALUE *pstValue)
{
    Uint16 u16Ok = 1U;

    switch (pstPoint->u16Type) {
    case MB_PT_U16:
        if ((Uint32)pstValue->u16 < pstPoint->u32Min) {
            u16Ok = 0U;
        } else if ((Uint32)pstValue->u16 > pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    case MB_PT_I16:
        if ((int32)pstValue->i16 < (int32)pstPoint->u32Min) {
            u16Ok = 0U;
        } else if ((int32)pstValue->i16 > (int32)pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    case MB_PT_U32:
        if (pstValue->u32 < pstPoint->u32Min) {
            u16Ok = 0U;
        } else if (pstValue->u32 > pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    case MB_PT_I32:
        if (pstValue->i32 < (int32)pstPoint->u32Min) {
            u16Ok = 0U;
        } else if (pstValue->i32 > (int32)pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    default:
        if ((pstValue->f32 < pstPoint->fMin) || (pstValue->f32 > pstPoint->fMax)) {
            u16Ok = 0U;
        }
        break;
    }

    return u16Ok;
}

static Uint16 MbLimitsDeclared(const ST_MODBUS_POINT *pstPoint, Uint16 u16Regs)
{
    Uint16 u16Ok = 1U;

    switch (pstPoint->u16Type) {
    case MB_PT_U16:
        if ((pstPoint->u32Max > 0xFFFFUL) || (pstPoint->u32Min > pstPoint->u32Max)) {
            u16Ok = 0U;   /* 例：Uint16 配 66530 必须在这里被拒绝 */
        }
        break;
    case MB_PT_I16:
        if ((int32)pstPoint->u32Min < -32768L) {
            u16Ok = 0U;
        } else if ((int32)pstPoint->u32Max > 32767L) {
            u16Ok = 0U;
        } else if ((int32)pstPoint->u32Min > (int32)pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    case MB_PT_U32:
        if (pstPoint->u32Min > pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    case MB_PT_I32:
        if ((int32)pstPoint->u32Min > (int32)pstPoint->u32Max) {
            u16Ok = 0U;
        }
        break;
    default:
        if (!(pstPoint->fMin <= pstPoint->fMax)) {
            u16Ok = 0U;   /* 浮点上下限：用 float 比较，NaN 也会被挡下 */
        }
        break;
    }

    (void)u16Regs;
    return u16Ok;
}

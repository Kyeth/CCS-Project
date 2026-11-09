/**
 * @file ModbusTbl.h
 * @brief Modbus 点表：带类型的寄存器映射、校验与访问。
 *
 * 线上地址仍是 16 位寄存器；点表可以直接描述 Uint16 / int16 / Uint32 / int32 /
 * float，由本模块负责类型转换。32 位量占两个寄存器，统一"高字在前"。
 *
 * 访问约束：
 *   - 32 位点必须整段访问，只碰其中一个寄存器返回地址异常（0x02）；
 *   - 多寄存器写"先全部校验、再提交"，不会出现前半段已写、后半段未写；
 *   - 读写通过类型化指针访问绑定变量，保留 volatile 语义（控制 ISR 正在更新的
 *     变量不会被编译器缓存）。
 */

#ifndef MODBUS_TBL_H
#define MODBUS_TBL_H

#include "F2837xS_device.h"
#include "ModbusRtu.h"

/* 点类型 */
#define MB_PT_U16               (1U)   /* 占 1 个寄存器 */
#define MB_PT_I16               (2U)   /* 占 1 个寄存器 */
#define MB_PT_U32               (3U)   /* 占 2 个寄存器，高字在前 */
#define MB_PT_I32               (4U)   /* 占 2 个寄存器，高字在前 */
#define MB_PT_F32               (5U)   /* 占 2 个寄存器，高字在前 */

/* 访问权限 */
#define MB_PT_READ              (1U)
#define MB_PT_WRITE             (2U)
#define MB_PT_READWRITE         (3U)

typedef struct {
    Uint16 u16Addr;      /* 起始寄存器地址 */
    Uint16 u16Type;      /* MB_PT_* */
    Uint16 u16Access;    /* MB_PT_READ / WRITE / READWRITE */
    Uint16 u16Reserved;  /* 保留（保存属性等后续使用） */
    void  *pVar;         /* 绑定的变量（必须非空） */
    Uint32 u32Min;       /* 整数型下限（按类型宽度解释） */
    Uint32 u32Max;       /* 整数型上限 */
    float  fMin;         /* 浮点型下限 */
    float  fMax;         /* 浮点型上限 */
} ST_MODBUS_POINT;

/** 点占用的寄存器数；未知类型返回 0。 */
Uint16 ModbusTblRegs(Uint16 u16Type);

/**
 * @brief 初始化期检查：类型/权限合法、地址严格递增不重叠、限值合法、绑定非空。
 * @return 1 = 通过，0 = 有错（应拒绝启用该点表）
 */
Uint16 ModbusTblValidate(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count);

/**
 * @brief 读一段寄存器到 16 位寄存器数组（Modbus 数据域顺序）。
 * @return MB_EX_NONE 或 MB_EX_ILLEGAL_ADDR / MB_EX_ILLEGAL_VALUE
 */
Uint16 ModbusTblRead(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                     Uint16 u16Start, Uint16 u16Qty, Uint16 *pu16Out);

/**
 * @brief 写一段寄存器：先整段校验（存在、可写、完整、限值），全部通过才提交。
 * @return MB_EX_NONE / MB_EX_ILLEGAL_ADDR / MB_EX_ILLEGAL_VALUE
 */
Uint16 ModbusTblWrite(const ST_MODBUS_POINT *pstTbl, Uint16 u16Count,
                      Uint16 u16Start, Uint16 u16Qty, const Uint16 *pu16In);

#endif /* MODBUS_TBL_H */

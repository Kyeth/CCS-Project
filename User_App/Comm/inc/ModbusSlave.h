/**
 * @file ModbusSlave.h
 * @author yetingheng (004182)
 * @brief Modbus RTU 从机：表驱动 + 指针直映业务变量
 * @version 0.1
 * @date 2026-10-23
 * @copyright Copyright (c) 2026
 *
 * @note 设计要点（照 IB800 的 ST_REG_SLAVE_ITEM 思路，补了两个坑）：
 *       1. 表项带上下限，写操作自动限幅，业务侧不用再判一次；
 *       2. 表项按"基地址 + 类型"占若干寄存器（32 位类型占 2 个），
 *          读请求可以跨表项，不会像 IB800 那样必须手工保持地址增序。
 *
 * @note 从机不缓存业务数据：读的时候直接从 pvData 取，写的时候直接写回，
 *       所以任何变量改一行表就能被上位机看到/改写。
 */
#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 功能码 */
#define MB_FUNC_READ_HOLDING        (0x03U)
#define MB_FUNC_WRITE_SINGLE        (0x06U)
#define MB_FUNC_WRITE_MULTI         (0x10U)

/* 异常码 */
#define MB_EX_NONE                  (0x00U)
#define MB_EX_ILLEGAL_FUNC          (0x01U)
#define MB_EX_ILLEGAL_ADDR          (0x02U)
#define MB_EX_ILLEGAL_VALUE         (0x03U)
#define MB_EX_SLAVE_FAIL            (0x04U)

/* 数据长度与限制 */
#define MB_FRAME_MAX                (260U)
#define MB_READ_REG_MAX             (125U)
#define MB_WRITE_REG_MAX            (123U)

/* 寄存器数据类型 */
#define MB_TYPE_U16                 (0U)
#define MB_TYPE_I16                 (1U)
#define MB_TYPE_U32                 (2U)
#define MB_TYPE_I32                 (3U)
#define MB_TYPE_F32                 (4U)

/* 读写属性 */
#define MB_ATTR_RO                  (0U)
#define MB_ATTR_RW                  (1U)

/* 32 位量的高低字顺序：0 = 低字在前（Modbus 惯例），1 = 高字在前 */
/* 32 位量（U32/I32/F32）占两个保持寄存器，字序选择：
 *   1 = 高字在前（ABCD），与 IB800 上位机/Modbus Poll 默认浮点解码一致；
 *   0 = 低字在前（CDAB）。
 * 每个寄存器内部字节序固定为高字节在前（Modbus RTU 规定），不受本宏影响。 */
#define MB_WORD_HIGH_FIRST          (1U)

/* 一个寄存器表项 */
typedef struct
{
    Uint16 u16RegAddr;              /* 起始寄存器地址 */
    Uint16 u16ValType;              /* MB_TYPE_xxx */
    Uint16 u16Attr;                 /* MB_ATTR_RO / MB_ATTR_RW */
    void  *pvData;                  /* 直接指向业务变量 */
    int32  i32Min;                  /* 写下限（浮点类型按取整后比较） */
    Uint32 u32Max;                  /* 写上限 */
} ST_MB_REG_ITEM;

/* 一个从机实例 */
typedef struct
{
    Uint16 u16SciId;                    /* 挂在哪路 SCI 上 */
    Uint16 u16SlaveAddr;                /* 从机地址，0 表示广播（只写不答） */
    const ST_MB_REG_ITEM *pstRegItem;   /* 寄存器表 */
    Uint16 u16RegNum;                   /* 表长 */
    Uint16 bNoReply;                    /* 1 = 只听不应答（TVS 这类单向数据源用）*/
} ST_MB_SLAVE_CFG;

/* 从机配置表（一块板子上可以有多路 Modbus 从机） */
typedef struct
{
    const ST_MB_SLAVE_CFG *pstSlaveCfg;
    Uint16 u16SlaveNum;
} ST_MB_CFG;

/* 初始化全部从机 */
void ModbusInit(const ST_MB_CFG *pstMbCfg);

/* 调度任务：轮询各从机的"帧就绪"标志并处理，建议 200us 一次 */
void ModbusTask(void *pArgV);

/* 统计量：收到并处理的帧数 / 异常帧数 / 最近一次功能码 */
Uint16 ModbusGetFrameCnt(Uint16 u16SlaveIdx);
Uint16 ModbusGetErrCnt(Uint16 u16SlaveIdx);
Uint16 ModbusGetLastFunc(Uint16 u16SlaveIdx);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_SLAVE_H */

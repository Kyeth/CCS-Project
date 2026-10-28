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
typedef enum
{
    MB_FUNC_READ_HOLDING = 0x03,            /* 读保持寄存器 */
    MB_FUNC_WRITE_SINGLE = 0x06,            /* 写单个寄存器 */
    MB_FUNC_WRITE_MULTI  = 0x10             /* 写多个寄存器 */
} E_MB_FUNC_CODE;

/* 异常码 */
typedef enum
{
    MB_EXC_NONE          = 0x00,
    MB_EXC_ILLEGAL_FUNC  = 0x01,            /* 功能码不支持 */
    MB_EXC_ILLEGAL_ADDR  = 0x02,            /* 地址不在表内或只读 */
    MB_EXC_ILLEGAL_VALUE = 0x03,            /* 数量或取值越界 */
    MB_EXC_SLAVE_FAIL    = 0x04             /* 从机内部失败 */
} E_MB_EXCEPTION;

/* 帧长与单帧寄存器数量上限 */
typedef enum
{
    MB_LIMIT_FRAME_MAX   = 260,             /* RTU 帧最大字节数 */
    MB_LIMIT_READ_MAX    = 125,             /* 03 每帧最多读 125 个 */
    MB_LIMIT_WRITE_MAX   = 123              /* 10 每帧最多写 123 个 */
} E_MB_LIMIT;

/* 寄存器值类型 */
typedef enum
{
    MB_VAL_U16 = 0,                         /* 无符号 16 位，占 1 个寄存器 */
    MB_VAL_I16 = 1,                         /* 有符号 16 位，占 1 个寄存器 */
    MB_VAL_U32 = 2,                         /* 无符号 32 位，占 2 个寄存器 */
    MB_VAL_I32 = 3,                         /* 有符号 32 位，占 2 个寄存器 */
    MB_VAL_F32 = 4                          /* 单精度浮点，占 2 个寄存器 */
} E_MB_VAL_TYPE;

/* 读写属性：低两位按位组合，SAVE 与 WRITE 正交 */
typedef enum
{
    MB_RW_READ_ONLY  = 0x00,                /* 只读 */
    MB_RW_WRITE      = 0x01,                /* 可写 */
    MB_RW_SAVE       = 0x02,                /* 掉电保存标志 */
    MB_RW_WRITE_SAVE = 0x03                 /* 可写且保存 */
} E_MB_RW_ATTR;

/* 32 位量的字序 */
typedef enum
{
    MB_WORD_LOW_FIRST  = 0,                 /* 低字在前（CDAB） */
    MB_WORD_HIGH_FIRST = 1                  /* 高字在前（ABCD） */
} E_MB_WORD_ORDER;

/* 字序选择：与上位机默认浮点解码保持一致 */
#define MB_WORD_ORDER_SEL           MB_WORD_HIGH_FIRST

/* 一个寄存器表项 */
typedef struct
{
    Uint16 u16RegAddr;              /* 起始寄存器地址 */
    Uint16 u16ValType;              // E_MB_VAL_TYPE
    Uint16 u16RwAttr;               // E_MB_RW_ATTR，按位组合
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

/* 按从机序号与寄存器地址反查表项，供命令触发与调试读取 */
extern const ST_MB_REG_ITEM *MbResolveByAddr(Uint16 u16SlaveIdx, Uint16 u16Addr);

/* 统计量：收到并处理的帧数 / 异常帧数 / 最近一次功能码 */
Uint16 ModbusGetFrameCnt(Uint16 u16SlaveIdx);
Uint16 ModbusGetErrCnt(Uint16 u16SlaveIdx);
Uint16 ModbusGetLastFunc(Uint16 u16SlaveIdx);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_SLAVE_H */

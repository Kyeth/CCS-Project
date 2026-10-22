/**
 * @file SoftOsc.h
 * @author yetingheng (004182)
 * @brief CAN-B 软件示波器：把任意 12 路变量按 400us 节拍发到上位机
 * @version 0.1
 * @date 2026-10-22
 * @copyright Copyright (c) 2026
 *
 * @note 思路照搬 IB800 的 User/SoftOsc：8 字节数据装不下 12 路，
 *       于是把 2 路塞进 29 位扩展帧 ID，4 路塞进 8 字节数据，一帧 6 路，
 *       两组帧交替发送（400us 一帧 → 每组 800us 刷新一次）。
 *
 * @note 一帧的字节布局（扩展帧，DLC = 8）：
 *
 *       位域                              内容
 *       --------------------------------  ------------------------------
 *       ID[28]                            组别：0 = 第 1~6 路，1 = 第 7~12 路
 *       ID[15:0]                          第 3、第 4 路（低 16 位 = 第 3 路）
 *       ID[27:16] + ID[15:0]            第 1、第 2 路（ID = 第1路<<16 | 第2路）
 *       Data_0 / Data_1                  第 5 路高字节 / 低字节
 *       Data_2 / Data_3                  第 6 路高字节 / 低字节
 *       Data_4 / Data_5                  第 3 路高字节 / 低字节
 *       Data_6 / Data_7                  第 4 路高字节 / 低字节
 *
 *       即：DATA_32 = (ch5 << 16) | ch6，DATB_32 = (ch3 << 16) | ch4，
 *       每个 16 位值先发高字节，与 IB800 的 U16tobp32 完全一致。
 */
#ifndef SOFT_OSC_H
#define SOFT_OSC_H

#include "F2837xS_device.h"
#include "Hw_CanDriver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 通道数与数据长度 */
#define SOFT_OSC_CH_NUM             (12U)
#define SOFT_OSC_DATA_LEN           (8U)

/* 使用 CAN-B（GPIO38 = CANTXB / GPIO39 = CANRXB）发送 */
#define SOFT_OSC_CAN_ID             (HW_CAN_ID_B)
#define SOFT_OSC_TX_OBJ_IDX         (0U)

/* 扩展帧 ID 第 28 位：区分两组通道 */
#define SOFT_OSC_FRAME_FLAG         (0x10000000UL)
#define SOFT_OSC_ID_MASK            (0x0FFFFFFFUL)

/* 通道数据类型，与 Modbus 点表的类型保持同序，便于以后直接从点表绑定 */
#define SOFT_OSC_TYPE_INT8          (0U)
#define SOFT_OSC_TYPE_UINT8         (1U)
#define SOFT_OSC_TYPE_INT16         (2U)
#define SOFT_OSC_TYPE_UINT16        (3U)
#define SOFT_OSC_TYPE_INT32         (4U)
#define SOFT_OSC_TYPE_UINT32        (5U)
#define SOFT_OSC_TYPE_FLOAT         (6U)

/* 发送模式：由上位机通过 SCID 的 Modbus 点表改写 gSoftOscMode */
#define SOFT_OSC_MODE_DUAL          (0U)   /* 双边：12 路分两组交替，每路 800us */
#define SOFT_OSC_MODE_SINGLE        (1U)   /* 单边：只发一组 6 路，每路 400us */

/* 模式与组别选择：非 static volatile，可直接挂到 Modbus 点表里读写 */
extern volatile Uint16 gSoftOscMode;       /* SOFT_OSC_MODE_DUAL / SINGLE */
extern volatile Uint16 gSoftOscGroupSel;   /* 单边模式下发哪一组：0 = 1~6 路，1 = 7~12 路 */

/* ============================ 观测源编号 ============================
 * 上位机通过 SCID 的 Modbus 点表写的就是这个编号，写 0 表示该通道不观测。
 * 新增可观测变量时只改 SoftOsc.c 里的 gSoftOscSrcTab[] 和这里的宏。
 */
#define SOFT_OSC_SRC_NONE           (0U)
#define SOFT_OSC_SRC_THETA          (1U)    /* 锁相角，度 */
#define SOFT_OSC_SRC_GRID_FREQ      (2U)    /* 电网频率，Hz */
#define SOFT_OSC_SRC_MOD_INDEX      (3U)    /* 调制比 */
#define SOFT_OSC_SRC_FAULT          (4U)    /* 故障字 */
#define SOFT_OSC_SRC_SEQ            (5U)    /* CLA 采样序号 */
#define SOFT_OSC_SRC_MUX_ADDR       (6U)    /* 当前 MUX 选通地址 */
#define SOFT_OSC_SRC_INV_CUR_R      (7U)    /* 逆变电流 R（两点平均） */
#define SOFT_OSC_SRC_INV_CUR_S      (8U)
#define SOFT_OSC_SRC_INV_CUR_T      (9U)
#define SOFT_OSC_SRC_INV_CUR_PK_R   (10U)   /* 逆变电流 R（PRD 第二采样） */
#define SOFT_OSC_SRC_INV_CUR_PK_S   (11U)
#define SOFT_OSC_SRC_INV_CUR_PK_T   (12U)
#define SOFT_OSC_SRC_INV_VOLT_R     (13U)
#define SOFT_OSC_SRC_INV_VOLT_S     (14U)
#define SOFT_OSC_SRC_INV_VOLT_T     (15U)
#define SOFT_OSC_SRC_GRID_VOLT_R    (16U)
#define SOFT_OSC_SRC_GRID_VOLT_S    (17U)
#define SOFT_OSC_SRC_GRID_VOLT_T    (18U)
#define SOFT_OSC_SRC_PV_CUR1        (19U)
#define SOFT_OSC_SRC_PV_CUR2        (20U)
#define SOFT_OSC_SRC_PV_CUR3        (21U)
#define SOFT_OSC_SRC_PV_CUR4        (22U)
#define SOFT_OSC_SRC_PV_VOLT1       (23U)
#define SOFT_OSC_SRC_PV_VOLT2       (24U)
#define SOFT_OSC_SRC_PV_VOLT3       (25U)
#define SOFT_OSC_SRC_PV_VOLT4       (26U)
#define SOFT_OSC_SRC_BUS_VOLT_L     (27U)
#define SOFT_OSC_SRC_BUS_VOLT_H     (28U)
#define SOFT_OSC_SRC_MAX            (29U)

/* ---- 按"Modbus 寄存器地址"选源------------------------
 * 通道配置寄存器里写的不是编号，而是**点表里的寄存器地址**，示波器
 * 按地址在 ARM 表 / 上位机表里反查出"变量地址 + 数据类型"：
 *
 * 解析顺序：源编号 < SOFT_OSC_SRC_ADDR_MIN 时先查内置源表（1~28，
 * 给那些不在点表里的内部量用）；否则按寄存器地址在两张点表里查。
 */
#define SOFT_OSC_SRC_ADDR_MIN       (100U)

/* 点表里写"源编号/寄存器地址"时允许的最大值 */
#define SOFT_OSC_SRC_ID_MAX         (65535U)

/* 一个通道的可写配置：观测源编号 + 定点放大倍数 */
typedef struct
{
    Uint16 u16SrcId;            /* SOFT_OSC_SRC_xxx，0 = 不观测 */
    Uint16 u16Multiple;         /* 放大倍数，如 100 表示 x100 */
} ST_SOFT_OSC_CH_CFG;

/* 12 路的运行期配置：非 static，直接挂 Modbus 点表 */
extern ST_SOFT_OSC_CH_CFG gSoftOscChCfg[SOFT_OSC_CH_NUM];

/* 已发送帧计数（只读观测点） */
extern volatile Uint16 gSoftOscFrameCnt;

/* 把 gSoftOscChCfg 重新解析成"变量地址 + 类型 + 倍数"，改绑后调用一次即可 */
extern void SoftOscApplyCfg(void);

/* 查某个观测源的数据类型；编号非法返回 0xFFFF */
extern Uint16 SoftOscGetSrcType(Uint16 u16SrcId);

/* 按源编号改绑（代码里用；上位机走点表，不需要调这个） */
extern void SoftOscBindSrc(Uint16 u16Ch, Uint16 u16SrcId, Uint16 u16Multiple);

/* 单个观测通道 */
typedef struct
{
    Uint16 u16Type;             /* 数据类型，SOFT_OSC_TYPE_xxx */
    void  *pvAddr;              /* 被观测变量的地址 */
    Uint16 u16Multiple;         /* 定点放大倍数，如 100 表示发出去的值 = 实际值 * 100 */
} ST_SOFT_OSC_CH;

/* 绑定默认的 12 路观测通道（控制量与 CLA 输出） */
extern void SoftOscInit(void);

/* 重新指定某一路观测的变量，供以后由 Modbus 点表动态绑定使用 */
extern void SoftOscSetChannel(Uint16 u16Ch, Uint16 u16Type,
                              void *pvAddr, Uint16 u16Multiple);

/* 400us 调度任务：按 gSoftOscMode 决定发单组还是两组交替 */
extern void SoftOscTask(void *pArgV);

/* 切换发送模式 / 单边模式的组别（也可直接写上面两个 volatile 变量） */
extern void SoftOscSetMode(Uint16 u16Mode);
extern void SoftOscSetGroup(Uint16 u16Group);

#ifdef __cplusplus
}
#endif

#endif

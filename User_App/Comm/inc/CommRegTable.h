/**
 * @file CommRegTable.h
 * @author yetingheng (004182)
 * @brief 通信点表：ARM（SCIA）/ 上位机（SCID）/ TVS（SCIC，只听不应答）
 * @version 0.2
 * @date 2026-10-24
 * @copyright Copyright (c) 2026
 *
 * @note 每张表先放少量参考行。加正式点表时：
 *       1. 在 CommRegTable.c 里照格式往下加行；
 *       2. 把对应的 xxx_NUM 宏改成实际行数（数组按 MAX 声明，不需要改）。
 *
 * @note ======================= 地址规划 =======================
 *       0     ~ 26     软示波器控制块（固定，明细见下）
 *       27    ~ 99     预留
 *       100   ~ 499    上位机调试变量区（随便维护，会被 ModbusPoll 读）
 *       500   ~ 10000  ARM 点表（等 ARM 侧通信内容定了再排地址）
 *       10001 ~ 65535  预留 / TVS（等 TVS 侧通信内容定了再排地址）
 *
 *       上位机控制块明细（SCID，从机地址 2）：
 *         0  ~ 11   12 路示波器选源：写"要观察的变量所在的寄存器地址"
 *                    例如写 10522，第 1 路就去看地址 10522 那个变量；
 *                    写 0 = 不观察；写 1~28 = 内置源（见 SoftOsc.h）
 *         12 ~ 23   12 路放大倍数：1~10000，与 0~11 一一对应
 *         24        发送模式：0 = 双边（两组交替，每路 800us 刷新）
 *         25        单边模式下固定发哪组：0 = 第 1~6 路，1 = 第 7~12 路
 *         26        已发帧数（只读）
 *
 *       注意：32 位类型（U32/I32/F32）占两个寄存器地址，下一行要空开。
 * ============================================================
 */
#ifndef COMM_REG_TABLE_H
#define COMM_REG_TABLE_H

#include "F2837xS_device.h"
#include "ModbusSlave.h"
#include "ArmPointTable.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- ARM 通信点表（SCI-A，GPIO48/49）：表体与镜像见 ArmPointTable.c ---- */
#define MB_ARM_REG_MAX              (ARM_POINT_NUM)
#define MB_ARM_REG_NUM              (ARM_POINT_NUM)

/* ---- 上位机点表（SCI-D，GPIO93/94）：0~26 控制块 + 100 以后调试变量 ---- */
#define MB_PC_REG_MAX               (96U)
#define MB_PC_REG_NUM               (84U)
extern Uint16  gMbPcDemoWord;
/* 掉电保存示范变量：属性带 MB_RW_SAVE，上电自动回填 */
extern Uint16  gMbSavedDemoWord;
extern float32 gMbSavedDemoFloat;
extern float32 gMbPcDemoFloat;
extern const ST_MB_REG_ITEM stMbRegItemPc[MB_PC_REG_MAX];

/* ---- TVS 点表（SCI-C，GPIO89 收、GPIO90 不发；只听不应答）---- */
#define MB_TVS_REG_MAX              (8U)
#define MB_TVS_REG_NUM              (1U)
extern Uint16 gTvsRefSlotData[4];
extern const ST_MB_REG_ITEM stMbRegItemTvs[MB_TVS_REG_MAX];

#ifdef __cplusplus
}
#endif

#endif /* COMM_REG_TABLE_H */

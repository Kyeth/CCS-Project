/**
 * @file CommRegTable.c
 * @author yetingheng (004182)
 * @brief 通信点表实现：ARM / 上位机 / TVS 三张 Modbus 点表
 * @version 0.2
 * @date 2026-10-24
 * @copyright Copyright (c) 2026
 *
 * @note 表项字段：{寄存器地址, 数据类型, 读写属性, 变量地址, 下限, 上限}
 *       地址可以不用连续、也不用排序，驱动按"基地址 + 类型宽度"匹配。
 *
 * @note 数组按 XXX_MAX 声明、按 XXX_NUM 使用，所以加行时不需要改数组大小，
 *       只把 CommRegTable.h 里对应的 NUM 宏改成实际行数。
 *
 * @note ======================= 地址规划 =======================
 *       0     ~ 26     软示波器控制块（上位机表）
 *       27    ~ 99     预留
 *       100   ~ 499    上位机调试变量区
 *       500   ~ 10000  ARM 点表（地址待 ARM 侧通信内容确定）
 *       10001 ~ 65535  预留 / TVS（地址待 TVS 侧通信内容确定）
 *
 *       同一个从机（同一条串口）内地址不能重复，否则 MbFindReg() 命中先出现
 *       的那一行；不同串口之间（SCIA / SCID / SCIC）地址可以随便重叠。
 * ================================================================
 */

#include "CommRegTable.h"
#include "SoftOsc.h"
#include "EepromParam.h"   /* 保存层诊断量的 Modbus 镜像 */
#include "ProtectTask.h"   /* 硬件保护任务状态量 */

/* ===================== 上位机调试用的演示变量 ===================== */

/* 一行 16 位参考：地址 100 */
Uint16 gMbPcDemoWord = 0U;

/* 一行 32 位浮点参考：地址 102（占 102~103） */
float32 gMbPcDemoFloat = 0.0F;

/* 掉电保存示范变量：地址 31 / 32，写属性带 SAVE 位。
 * 这两个变量只是示范，正式点表里任何一个变量都可以照此标记。 */
Uint16  gMbSavedDemoWord  = 1234U;
float32 gMbSavedDemoFloat = 1.5F;

/* ============================ ARM 点表 ============================ */

/* 参考用的演示变量，正式点表进来后可以删掉 */
Uint16 gMbArmDemoWord = 0U;

/* 一行参考：地址 1000，16 位可读写，范围 0~1000
 *
 * ARM 点表地址区间等 ARM 侧通信内容定了再排；现在只放一行占位。
 * 加行时把 CommRegTable.h 里的 MB_ARM_REG_NUM 改成实际行数。
 */
const ST_MB_REG_ITEM stMbRegItemArm[MB_ARM_REG_MAX] =
{
    {1000U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gMbArmDemoWord, 0, 1000U},
};

/* ============================ TVS 点表 ============================ */
/*
 * TVS 走标准 Modbus，但SCIC 只收不发。
 *   - 收到 06 / 10 写寄存器：照常落到变量上
 *   - 读请求、异常应答：一律不回复
 *
 * 地址等 TVS 侧通信内容确定后再排，现在放一行占位。
 */
Uint16 gTvsRefSlotData[4] = {0U, 0U, 0U, 0U};

const ST_MB_REG_ITEM stMbRegItemTvs[MB_TVS_REG_MAX] =
{
    {20000U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gTvsRefSlotData[0], 0, 65535U},
};

/* ============================ 上位机点表 ============================ */
const ST_MB_REG_ITEM stMbRegItemPc[MB_PC_REG_MAX] =
{
    /* ---------- 0~11：12 路选源（写变量的寄存器地址）---------- */
    {0U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[0].u16SrcId,  0, 65535U},
    {1U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[1].u16SrcId,  0, 65535U},
    {2U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[2].u16SrcId,  0, 65535U},
    {3U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[3].u16SrcId,  0, 65535U},
    {4U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[4].u16SrcId,  0, 65535U},
    {5U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[5].u16SrcId,  0, 65535U},
    {6U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[6].u16SrcId,  0, 65535U},
    {7U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[7].u16SrcId,  0, 65535U},
    {8U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[8].u16SrcId,  0, 65535U},
    {9U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[9].u16SrcId,  0, 65535U},
    {10U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[10].u16SrcId,  0, 65535U},
    {11U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[11].u16SrcId,  0, 65535U},

    /* ---------- 12~23：对应通道的放大倍数 ---------- */
    {12U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[0].u16Multiple, 1, 10000U},
    {13U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[1].u16Multiple, 1, 10000U},
    {14U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[2].u16Multiple, 1, 10000U},
    {15U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[3].u16Multiple, 1, 10000U},
    {16U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[4].u16Multiple, 1, 10000U},
    {17U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[5].u16Multiple, 1, 10000U},
    {18U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[6].u16Multiple, 1, 10000U},
    {19U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[7].u16Multiple, 1, 10000U},
    {20U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[8].u16Multiple, 1, 10000U},
    {21U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[9].u16Multiple, 1, 10000U},
    {22U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[10].u16Multiple, 1, 10000U},
    {23U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscChCfg[11].u16Multiple, 1, 10000U},

    /* ---------- 24~26：发送模式 / 单边组别 / 已发帧数 ---------- */
    {24U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscMode,       0, 1U},
    {25U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gSoftOscGroupSel,   0, 1U},
    {26U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gSoftOscFrameCnt,   0, 65535U},

    /* ---------- 27~30：掉电保存层诊断（只读镜像）---------- */
    {27U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gEepromParamStatus,     0, 65535U},
    {28U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gEepromParamItemNum,    0, 65535U},
    {29U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gEepromParamWriteCntLo, 0, 65535U},
    {30U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gEepromParamErrCnt,     0, 65535U},

    /* ---------- 31~33：掉电保存示范变量（MB_ATTR_RW_SAVE）---------- */
    {31U, MB_TYPE_U16, MB_ATTR_RW_SAVE, (void *)&gMbSavedDemoWord,   0, 65535U},
    {32U, MB_TYPE_F32, MB_ATTR_RW_SAVE, (void *)&gMbSavedDemoFloat, -10000, 10000U},

    /* ---------- 33~36：硬件保护任务状态（10kHz 漏桶判定）---------- */
    {33U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gProtectInvOcCnt,    0, 65535U},
    {34U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gProtectShortCnt,    0, 65535U},
    {35U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gProtectFaultFlags,  0, 65535U},
    {36U, MB_TYPE_U16, MB_ATTR_RO, (void *)&gProtectTick,        0, 65535U},

    /* ---------- 100 以后：调试变量区，按需往下加 ---------- */
    {100U, MB_TYPE_U16, MB_ATTR_RW, (void *)&gMbPcDemoWord,   0, 65535U},
    {102U, MB_TYPE_F32, MB_ATTR_RW, (void *)&gMbPcDemoFloat, -10000, 10000U},
};

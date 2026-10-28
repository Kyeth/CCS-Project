/**
 * @file DrvCfg_Protect.c
 * @author yetingheng (004182)
 * @brief 保护配置表定义：故障属性表与协议聚合表
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 保护配置数据：故障位属性表 + 协议故障字聚合表。
 * 属性表由 PROTECT_FAULT_LIST 展开，新增故障只需改头文件一行。
 */
#include "DrvCfg_Protect.h"

const ST_PROTECT_ATTR stProtectAttr[] =
{
#define PROTECT_FAULT_ATTR_ROW(pId, pWord, pMask, pDomain, pSev, pRec, pDeb) \
    { (Uint16)(pWord), (Uint16)(pMask), (Uint16)(pDeb), (Uint16)(pDomain), (Uint16)(pSev), (Uint16)(pRec), 0U },
    PROTECT_FAULT_LIST(PROTECT_FAULT_ATTR_ROW)
#undef PROTECT_FAULT_ATTR_ROW
};

/*
 * 一个协议位 = 若干内部位的逻辑或。
 * 例：5032.b0 零漂异常 = 系统字零漂总位 或 分相零漂任一位，
 *     内部知道是哪一相，协议只看到一位。
 */
#define AGG(protoWord, protoBit, srcWord, srcMask) \
    { (Uint16)(protoWord), (Uint16)(protoBit), 1U, 0U, { (Uint16)(srcWord), 0U, 0U }, { (Uint16)(srcMask), 0U, 0U } }
#define AGG2(protoWord, protoBit, w0, m0, w1, m1) \
    { (Uint16)(protoWord), (Uint16)(protoBit), 2U, 0U, { (Uint16)(w0), (Uint16)(w1), 0U }, { (Uint16)(m0), (Uint16)(m1), 0U } }

const ST_PROTECT_AGG_ITEM stProtectAgg[] =
{
    /* 5030 PCS 故障信息 1 */
    AGG(PROTECT_PW_5030,  0U, PROTECT_W_DC,   0x0001U),   /* BUS1 过压 */
    AGG(PROTECT_PW_5030,  1U, PROTECT_W_DC,   0x0002U),   /* BUS2 过压 */
    AGG(PROTECT_PW_5030,  2U, PROTECT_W_DC,   0x0004U),   /* BUS1 欠压 */
    AGG(PROTECT_PW_5030,  3U, PROTECT_W_DC,   0x0008U),   /* BUS2 欠压 */
    AGG(PROTECT_PW_5030,  4U, PROTECT_W_DC,   0x0010U),   /* 硬件 BUS1 过压 */
    AGG(PROTECT_PW_5030,  5U, PROTECT_W_DC,   0x0020U),   /* 硬件 BUS2 过压 */
    AGG(PROTECT_PW_5030,  6U, PROTECT_W_DC,   0x0040U),   /* 硬件电池过压 */
    AGG(PROTECT_PW_5030,  7U, PROTECT_W_INV,  0x0001U),   /* 硬件逆变过流 GPIO81 */
    AGG(PROTECT_PW_5030,  8U, PROTECT_W_INV,  0x0002U),   /* 短路过流 GPIO82 */
    AGG(PROTECT_PW_5030,  9U, PROTECT_W_DC,   0x0080U),   /* 硬件 LLC1 过流 */
    AGG(PROTECT_PW_5030, 10U, PROTECT_W_DC,   0x0080U),   /* 硬件 LLC2 过流 */
    AGG(PROTECT_PW_5030, 11U, PROTECT_W_INV,  0x0200U),   /* 平衡电路过流 */
    AGG(PROTECT_PW_5030, 12U, PROTECT_W_DC,   0x0100U),   /* 辅助电源欠压 */
    AGG(PROTECT_PW_5030, 13U, PROTECT_W_INV,  0x0100U),   /* 直流分量异常 */
    AGG(PROTECT_PW_5030, 14U, PROTECT_W_INV,  0x4000U),   /* 继电器故障 */
    AGG(PROTECT_PW_5030, 15U, PROTECT_W_PV,   0x4000U),   /* PV 连接方式错误 */
    /* 5031 PCS 故障信息 2 */
    AGG(PROTECT_PW_5031,  0U, PROTECT_W_PV,   0x0001U),   /* PV1 过流 */
    AGG(PROTECT_PW_5031,  1U, PROTECT_W_PV,   0x0002U),   /* PV2 过流 */
    AGG(PROTECT_PW_5031,  2U, PROTECT_W_PV,   0x0004U),   /* PV3 过流 */
    AGG(PROTECT_PW_5031,  3U, PROTECT_W_PV,   0x0010U),   /* PV1 高压 */
    AGG(PROTECT_PW_5031,  4U, PROTECT_W_PV,   0x0020U),   /* PV2 高压 */
    AGG(PROTECT_PW_5031,  5U, PROTECT_W_PV,   0x0040U),   /* PV3 高压 */
    AGG(PROTECT_PW_5031,  6U, PROTECT_W_PV,   0x0100U),   /* PV1 ISO */
    AGG(PROTECT_PW_5031,  7U, PROTECT_W_PV,   0x0200U),   /* PV2 ISO */
    AGG(PROTECT_PW_5031,  8U, PROTECT_W_PV,   0x0400U),   /* PV3 ISO */
    AGG(PROTECT_PW_5031,  9U, PROTECT_W_PV,   0x2000U),   /* 硬件 PV1 */
    AGG(PROTECT_PW_5031, 10U, PROTECT_W_PV,   0x2000U),   /* 硬件 PV2 */
    AGG(PROTECT_PW_5031, 11U, PROTECT_W_PV,   0x2000U),   /* 硬件 PV3 */
    AGG(PROTECT_PW_5031, 12U, PROTECT_W_SYS,  0x0020U),   /* GFCI 硬件 */
    AGG(PROTECT_PW_5031, 13U, PROTECT_W_SYS,  0x0040U),   /* GFCI 漏电 */
    AGG(PROTECT_PW_5031, 14U, PROTECT_W_GRID, 0x0400U),   /* 相序错误 */
    AGG(PROTECT_PW_5031, 15U, PROTECT_W_SYS,  0x0002U),   /* 风扇故障 */
    /* 5032 PCS 故障信息 3 */
    AGG2(PROTECT_PW_5032,  0U, PROTECT_W_SYS, 0x0008U, PROTECT_W_DETAIL, 0x0007U), /* 零漂异常 */
    AGG(PROTECT_PW_5032,  1U, PROTECT_W_DC,   0x8000U),   /* 硬件电流过流输入 */
    AGG(PROTECT_PW_5032,  2U, PROTECT_W_DC,   0x0400U),   /* 直流输入低压 */
    AGG(PROTECT_PW_5032,  3U, PROTECT_W_DC,   0x0800U),   /* 直流输入高压 */
    AGG(PROTECT_PW_5032,  4U, PROTECT_W_DC,   0x1000U),   /* 直流输入过流 */
    AGG(PROTECT_PW_5032,  5U, PROTECT_W_DC,   0x2000U),   /* LLC1 输出过压 */
    AGG(PROTECT_PW_5032,  6U, PROTECT_W_DC,   0x2000U),   /* LLC2 输出过压 */
    AGG2(PROTECT_PW_5032,  7U, PROTECT_W_INV, 0x2000U, PROTECT_W_DETAIL, 0x0008U), /* L1 过载 */
    AGG2(PROTECT_PW_5032,  8U, PROTECT_W_INV, 0x2000U, PROTECT_W_DETAIL, 0x0010U), /* L2 过载 */
    AGG2(PROTECT_PW_5032,  9U, PROTECT_W_INV, 0x2000U, PROTECT_W_DETAIL, 0x0020U), /* L3 过载 */
    AGG(PROTECT_PW_5032, 10U, PROTECT_W_INV,  0x0020U),   /* L1 逆变输出故障 */
    AGG(PROTECT_PW_5032, 11U, PROTECT_W_INV,  0x0040U),   /* L2 逆变输出故障 */
    AGG(PROTECT_PW_5032, 12U, PROTECT_W_INV,  0x0080U),   /* L3 逆变输出故障 */
    AGG(PROTECT_PW_5032, 13U, PROTECT_W_SYS,  0x0001U),   /* 过温保护 */
    AGG(PROTECT_PW_5032, 14U, PROTECT_W_SYS,  0x0080U),   /* 通讯故障 */
    AGG(PROTECT_PW_5032, 15U, PROTECT_W_DETAIL, 0x2000U), /* 电池放电欠功率 */
    /* 5033 PCS 故障信息 4 */
    AGG(PROTECT_PW_5033,  0U, PROTECT_W_PV,   0x1000U),   /* PV 严重高压 */
    AGG(PROTECT_PW_5033,  1U, PROTECT_W_DC,   0x4000U),   /* LLC 输出低压 */
    AGG(PROTECT_PW_5033,  2U, PROTECT_W_DC,   0x0200U),   /* BUS 软启动异常 */
    AGG(PROTECT_PW_5033,  3U, PROTECT_W_SYS,  0x0100U),   /* 并机地址重复 */
    AGG(PROTECT_PW_5033,  4U, PROTECT_W_SYS,  0x0200U),   /* 并机通讯丢失 */
    AGG(PROTECT_PW_5033,  5U, PROTECT_W_PV,   0x0008U),   /* PV4 过流 */
    AGG(PROTECT_PW_5033,  6U, PROTECT_W_PV,   0x0080U),   /* PV4 高压 */
    AGG(PROTECT_PW_5033,  7U, PROTECT_W_PV,   0x0800U),   /* PV4 ISO */
    AGG(PROTECT_PW_5033,  8U, PROTECT_W_PV,   0x2000U),   /* 硬件 PV4 */
    /* 5034 PCS 告警信息 1 */
    AGG2(PROTECT_PW_5034,  0U, PROTECT_W_GRID, 0x0001U, PROTECT_W_GRID, 0x0010U), /* 电网过压 */
    AGG2(PROTECT_PW_5034,  1U, PROTECT_W_GRID, 0x0002U, PROTECT_W_GRID, 0x0020U), /* 电网欠压 */
    AGG2(PROTECT_PW_5034,  2U, PROTECT_W_GRID, 0x0004U, PROTECT_W_GRID, 0x0040U), /* 电网过频 */
    AGG2(PROTECT_PW_5034,  3U, PROTECT_W_GRID, 0x0008U, PROTECT_W_GRID, 0x0080U), /* 电网欠频 */
    AGG(PROTECT_PW_5034,  4U, PROTECT_W_GRID, 0x0100U),   /* 电网振荡 */
    AGG(PROTECT_PW_5034,  5U, PROTECT_W_GRID, 0x0200U),   /* 电网掉电 */
    AGG(PROTECT_PW_5034,  6U, PROTECT_W_DETAIL, 0x0040U), /* PV1 低压 */
    AGG(PROTECT_PW_5034,  7U, PROTECT_W_DETAIL, 0x0080U), /* PV2 低压 */
    AGG(PROTECT_PW_5034,  8U, PROTECT_W_DETAIL, 0x0100U), /* PV3 低压 */
    AGG(PROTECT_PW_5034,  9U, PROTECT_W_DETAIL, 0x0400U), /* 发电机电压异常 */
    AGG(PROTECT_PW_5034, 10U, PROTECT_W_DETAIL, 0x0800U), /* 调试 CAN 无通讯 */
    AGG(PROTECT_PW_5034, 11U, PROTECT_W_DETAIL, 0x1000U), /* 调试 485 无通讯 */
    AGG(PROTECT_PW_5034, 12U, PROTECT_W_GRID, 0x0800U),   /* 并网重连异常 */
    AGG(PROTECT_PW_5034, 13U, PROTECT_W_DETAIL, 0x0200U), /* PV4 低压 */
    AGG(PROTECT_PW_5034, 14U, PROTECT_W_GRID, 0x1000U),   /* 当天继电器切换频繁 */
    AGG(PROTECT_PW_5034, 15U, PROTECT_W_GRID, 0x2000U),   /* 此刻继电器切换频繁 */
    /* 5054 PCS 故障信息 5 */
    AGG(PROTECT_PW_5054,  0U, PROTECT_W_SYS,  0x0800U),   /* 并机状态异常 */
    /* 5060 DSP 控制位 */
    AGG(PROTECT_PW_5060,  2U, PROTECT_W_INV,  0x8000U),   /* 半母线短路 */
    AGG(PROTECT_PW_5060,  3U, PROTECT_W_PV,   0x8000U),   /* PV 持续过流 */
    AGG(PROTECT_PW_5060,  4U, PROTECT_W_SYS,  0x8000U),   /* 硬件故障不可恢复 */
    AGG(PROTECT_PW_5060,  6U, PROTECT_W_SYS,  0x0400U)    /* 并机接线错误 */
};

const ST_PROTECT_CFG stProtectCfg =
{
    stProtectAttr,
    (Uint16)(sizeof(stProtectAttr) / sizeof(stProtectAttr[0])),
    stProtectAgg,
    (Uint16)(sizeof(stProtectAgg) / sizeof(stProtectAgg[0]))
};

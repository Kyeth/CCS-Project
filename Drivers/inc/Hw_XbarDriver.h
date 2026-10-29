/**
 * @file Hw_XbarDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS X-BAR 驱动头文件：InputXbar 引脚通路 + ePWM X-BAR 故障汇聚
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note 两段 X-BAR 各管一段，缺一不可：
 *       - InputXbar：把 GPIO 接到 INPUT1~INPUT14，是 XINT1~5、eCAP、ePWM X-BAR
 *         共用的信号源，输出到 INPUTx 之后各用各的，互不影响（一个 INPUT 可以
 *         同时被 XINT、eCAP、ePWM X-BAR 三方取用）；
 *       - ePWM X-BAR：INPUT1~INPUT14 经 MUX 选进 TRIP4~TRIP12，一个 TRIP 可以把
 *         多路 MUX 做“或”汇聚，再送进 ePWM 的 DCAH/DCBH 做数字比较触发。
 *
 *       本板用法（逆变/光伏过流保护）：
 *         PV1(GPIO13)->INPUT1->MUX0->TRIP4 -> EPWM7 DCAH
 *         PV2(GPIO17)->INPUT2->MUX1->TRIP5 -> EPWM9 DCAH
 *         PV3(GPIO41)->INPUT3->MUX2->TRIP7 -> EPWM5 DCAH
 *         PV4(GPIO40)->INPUT6->MUX5->TRIP8 -> EPWM6 DCAH
 *         逆变过流(GPIO81)->INPUT4->MUX3-┐
 *         硬件短路(GPIO82)->INPUT5->MUX4-┴->TRIP9 -> 6 个逆变模块 DCAH+DCBH
 *       INPUT4/INPUT5 同时是 XINT1/XINT2 的信号源，给软件做 10kHz 计数判定。
 */

#ifndef HW_XBAR_DRIVER_H
#define HW_XBAR_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* InputXbar 的输入通道数：INPUT1~INPUT14 */
#define HW_XBAR_INPUT_NUM       (14U)

/* ePWM X-BAR 每个 TRIP 的 MUX 数：MUX0~MUX31 */
#define HW_XBAR_MUX_NUM         (32U)

/* ePWM X-BAR 的 TRIP 数：TRIP4/5/7/8/9/10/11/12，没有 TRIP6 */
#define HW_XBAR_TRIP_NUM        (8U)

/* 输入通道编号（不是 GPIO 号，写表的顺序就是这里） */
typedef enum
{
    HW_XBAR_INPUT1 = 0U,
    HW_XBAR_INPUT2,
    HW_XBAR_INPUT3,
    HW_XBAR_INPUT4,
    HW_XBAR_INPUT5,
    HW_XBAR_INPUT6,
    HW_XBAR_INPUT7,
    HW_XBAR_INPUT8,
    HW_XBAR_INPUT9,
    HW_XBAR_INPUT10,
    HW_XBAR_INPUT11,
    HW_XBAR_INPUT12,
    HW_XBAR_INPUT13,
    HW_XBAR_INPUT14,
    HW_XBAR_INPUT_MAX
} E_HW_XBAR_INPUT;

/* TRIP 编号：与硬件 TRIPn 一一对应（跳过 TRIP6） */
typedef enum
{
    HW_XBAR_TRIP4 = 0U,
    HW_XBAR_TRIP5,
    HW_XBAR_TRIP7,
    HW_XBAR_TRIP8,
    HW_XBAR_TRIP9,
    HW_XBAR_TRIP10,
    HW_XBAR_TRIP11,
    HW_XBAR_TRIP12,
    HW_XBAR_TRIP_MAX
} E_HW_XBAR_TRIP;

/* MUX 的 2 位配置值（TRIPnMUX0TO15CFG 每个 MUX 占 2 位） */
#define HW_XBAR_MUX_DISABLE     (0U)    /* 该 MUX 不参与本 TRIP */
#define HW_XBAR_MUX_ENABLE      (1U)    /* 选择该MUX的INPUTXBAR源（G1/G3/...），并使能 */
#define HW_XBAR_MUX_INVERT      (2U)    /* 已废弃：反相应由独立TRIPOUTINV设置 */

/* 引脚映射项：把某个 GPIO 接到某一路 INPUTx */
typedef struct
{
    Uint16 u16InputId;      /* E_HW_XBAR_INPUT，目的 INPUTx */
    Uint16 u16GpioNum;      /* 信号源 GPIO 编号 0~255 */
} ST_XBAR_INPUT_CFG_ITEM;

/* TRIP 汇聚项：某一路 MUX 以什么方式进 TRIPn，一个 TRIP 可以写多行做“或” */
typedef struct
{
    Uint16 u16TripId;       /* E_HW_XBAR_TRIP，目的 TRIPn */
    Uint16 u16MuxNum;       /* INPUT1~6对应MUX1/3/5/7/9/11 */
    Uint16 u16MuxMode;      /* 当前板卡为1：选择INPUTXBAR源并使能 */
} ST_XBAR_TRIP_MUX_CFG_ITEM;

/* TRIP 输出极性项：TRIP 整体的输出是否取反 */
typedef struct
{
    Uint16 u16TripId;       /* E_HW_XBAR_TRIP */
    Uint16 u16Invert;       /* 1: 该 TRIP 输出取反 */
} ST_XBAR_TRIP_OUT_CFG_ITEM;

/* X-BAR 总配置 */
typedef struct
{
    const ST_XBAR_INPUT_CFG_ITEM *pstInputCfgItem;
    Uint16 u16InputItemNum;
    const ST_XBAR_TRIP_MUX_CFG_ITEM *pstTripMuxCfgItem;
    Uint16 u16TripMuxItemNum;
    const ST_XBAR_TRIP_OUT_CFG_ITEM *pstTripOutCfgItem;
    Uint16 u16TripOutItemNum;
} ST_XBAR_CFG;

/* 按配置表初始化两段 X-BAR（先 InputXbar，再 ePWM X-BAR） */
extern void Hw_XbarInit(const ST_XBAR_CFG *pstXbarCfg);

/* 单独改一路 InputXbar 的引脚映射 */
extern void Hw_XbarSetInput(Uint16 u16InputId, Uint16 u16GpioNum);

/* 单独改一路 TRIP 的 MUX 配置（含使能位） */
extern void Hw_XbarSetTripMux(Uint16 u16TripId, Uint16 u16MuxNum, Uint16 u16MuxMode);

#ifdef __cplusplus
}
#endif

#endif /* HW_XBAR_DRIVER_H */

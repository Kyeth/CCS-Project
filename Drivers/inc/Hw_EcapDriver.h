/**
 * @file Hw_EcapDriver.h
 * @author yetingheng (004182)
 * @brief F28374S eCAP 捕获驱动（电网过零 / 电网频率信号捕获）
 * @version 0.1
 * @date 2026-10-22
 * @copyright Copyright (c) 2026
 *
 * @note eCAP1~eCAP6 的输入是固定的，不与 GPIO 直接对应：
 *       eCAP1 ← INPUT7   eCAP2 ← INPUT8   eCAP3 ← INPUT9
 *       eCAP4 ← INPUT10  eCAP5 ← INPUT11  eCAP6 ← INPUT12
 *       GPIO 到 INPUTx 的连线由 InputXbar 的 INPUTxSELECT 寄存器决定，
 *       本驱动在初始化时按配置表的 GPIO 号写入。
 * @note 引脚本身必须先在 GPIO 配置表里配成数字输入（方向、上拉、限定），
 *       本驱动不重复配置 GPIO。
 */
#ifndef HW_ECAP_DRIVER_H
#define HW_ECAP_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* eCAP 时钟：PCLKCR3 打开后 eCAP 计数时钟就是 SYSCLK = 200MHz，1 个计数 5ns */
#define HW_ECAP_SYSCLK_HZ           (200000000UL)

/* 读取失败时的返回值 */
#define HW_ECAP_INVALID_TICKS       (0xFFFFFFFFUL)
#define HW_ECAP_INVALID_FREQ        (0.0F)

/* 捕获边沿：对应 ECCTL1.CAP1POL */
#define HW_ECAP_EDGE_RISING         (0U)    /* 上升沿捕获 */
#define HW_ECAP_EDGE_FALLING        (1U)    /* 下降沿捕获 */

/* eCAP 模块编号（与硬件 eCAP1~eCAP6 一一对应） */
typedef enum
{
    HW_ECAP_ID0 = 0U,       /* eCAP1，输入固定为 INPUT7  */
    HW_ECAP_ID1,            /* eCAP2，输入固定为 INPUT8  */
    HW_ECAP_ID2,            /* eCAP3，输入固定为 INPUT9  */
    HW_ECAP_ID3,            /* eCAP4，输入固定为 INPUT10 */
    HW_ECAP_ID4,            /* eCAP5，输入固定为 INPUT11 */
    HW_ECAP_ID5,            /* eCAP6，输入固定为 INPUT12 */
    HW_ECAP_ID_MAX,
    HW_ECAP_ID_NULL = 0xFFFFU
} E_HW_ECAP_ID;

/**
 * @brief eCAP 单模块配置项
 *
 * 捕获波形：连续模式，只捕获事件1（CAP1），每次捕获后 TSCTR 清零，
 * 因此 CAP1 就是相邻两个同向边沿之间的 SYSCLK 计数 = 一个信号周期。
 */
typedef struct
{
    Uint16 u16EcapId;           /* 模块编号，见 E_HW_ECAP_ID */
    Uint16 u16GpioNum;          /* 信号源 GPIO 号，写入 InputXbar 的 INPUTxSELECT */
    Uint16 u16CaptureEdge;      /* 捕获边沿，HW_ECAP_EDGE_RISING / FALLING */
    Uint16 bEnableInt;          /* 1: 使能捕获事件1中断，0: 只轮询（需在 InterruptCfg 注册） */
} ST_ECAP_CFG_ITEM;

/**
 * @brief eCAP 配置汇总
 */
typedef struct
{
    const ST_ECAP_CFG_ITEM *pstEcapCfgItem;
    Uint16 u16ItemNum;
} ST_ECAP_CFG;

/* 按配置表初始化所有 eCAP 模块（含时钟使能、InputXbar、模块寄存器） */
extern void Hw_EcapInit(const ST_ECAP_CFG *pstEcapCfg);

/* 初始化单个 eCAP 模块 */
extern void Hw_EcapSingleInit(const ST_ECAP_CFG_ITEM *pstEcapCfgItem);

/* 是否有未处理的捕获事件（ECFLG.CEVT1） */
extern Uint16 Hw_EcapHasEvent(Uint16 u16EcapId);

/* 清除捕获事件标志 */
extern void Hw_EcapClearEvent(Uint16 u16EcapId);

/* 读取捕获值并清除事件；无事件时返回 HW_ECAP_INVALID_TICKS */
extern Uint32 Hw_EcapReadEvent(Uint16 u16EcapId);

/* 只读取周期计数（CAP1），不清除事件 */
extern Uint32 Hw_EcapGetPeriodTicks(Uint16 u16EcapId);

/* 读 CAP1 并换算成频率（Hz）；无有效捕获时返回 HW_ECAP_INVALID_FREQ */
extern float32 Hw_EcapGetFrequencyHz(Uint16 u16EcapId);

/* 读取当前时间戳计数器 TSCTR（用于两次捕获之间做相位插值） */
extern Uint32 Hw_EcapGetCounter(Uint16 u16EcapId);

/* 启动 / 停止时间戳计数器 */
extern void Hw_EcapStart(Uint16 u16EcapId);
extern void Hw_EcapStop(Uint16 u16EcapId);

#ifdef __cplusplus
}
#endif

#endif

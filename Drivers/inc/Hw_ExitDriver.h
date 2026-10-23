/**
 * @file Hw_ExitDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS 外部中断（XINT1~XINT5）驱动头文件
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note XINT 与 GPIO 之间没有直连，信号路径是：
 *         GPIO -> InputXbar INPUTx -> XINTn
 *       其中 XINT 的输入通道是硬件固定的：
 *         XINT1 <- INPUT4   XINT2 <- INPUT5   XINT3 <- INPUT6
 *         XINT4 <- INPUT13  XINT5 <- INPUT14
 *       所以本驱动只管 XINTnCR 的极性和使能，GPIO 到 INPUTx 的连线由 Hw_XbarDriver
 *       负责（DrvCfg 里 stXbarCfg 的输入项）。
 *
 *       中断里只做一件事：把事件计数加一，不判故障。故障判定放到 10kHz 保护任务里
 *       做漏桶计数，避免中断里出现分支很多的比较逻辑（EP5K 的做法，已验证）。
 *
 *       极性只有两档可选（POLARITY 是 2 位，但硬件只实现了下降/上升两种）：
 *         HW_XINT_FALLING = 0，HW_XINT_RISING = 1
 *       比较器输出低有效，所以本板一律用 FALLING。
 */

#ifndef HW_EXIT_DRIVER_H
#define HW_EXIT_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* XINT 的模块数 */
#define HW_XINT_NUM             (5U)

typedef enum
{
    HW_XINT_ID1 = 0U,
    HW_XINT_ID2,
    HW_XINT_ID3,
    HW_XINT_ID4,
    HW_XINT_ID5,
    HW_XINT_ID_MAX
} E_HW_XINT_ID;

/* 触发极性（写 XINTnCR.POLARITY） */
#define HW_XINT_FALLING         (0U)    /* 下降沿触发：比较器低有效时用这个 */
#define HW_XINT_RISING          (1U)    /* 上升沿触发 */

/* 单路外部中断配置 */
typedef struct
{
    Uint16 u16XintId;       /* E_HW_XINT_ID */
    Uint16 u16Edge;         /* HW_XINT_FALLING / HW_XINT_RISING */
    Uint16 bEnable;         /* 1: 使能 XINTn */
} ST_EXIT_CFG_ITEM;

typedef struct
{
    const ST_EXIT_CFG_ITEM *pstExitCfgItem;
    Uint16 u16ItemNum;
} ST_EXIT_CFG;

/* 按配置表初始化 XINT1~XINT5（只动 XINTnCR，不动 PIE） */
extern void Hw_ExitInit(const ST_EXIT_CFG *pstExitCfg);

/* 单独使能/关闭一路 XINT */
extern void Hw_ExitEnable(Uint16 u16XintId, Uint16 u16Enable);

/* 中断服务里调用：清 PIE 由调用方（InterruptCfg.c）处理，这里只累加事件计数 */
extern void Hw_ExitIsr(Uint16 u16XintId);

/* 取走自上次调用以来新增的事件数（自由计数差值，16 位回绕安全，无需关中断） */
extern Uint16 Hw_ExitTakeCount(Uint16 u16XintId);

/* 查询累计事件总数（32 位，诊断用） */
extern Uint32 Hw_ExitGetTotal(Uint16 u16XintId);

/* 清零累计计数（上电初始化用） */
extern void Hw_ExitClearAll(void);

#ifdef __cplusplus
}
#endif

#endif /* HW_EXIT_DRIVER_H */

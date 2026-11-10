/******************************************************************************
 * @file    UpgradeFlash.h
 * @brief   F021 Flash API 适配层（RAM 执行）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    本层的每个函数都放在 .flashfunc 段，运行时在 RAMGS3 里执行。
 *          F021 下发擦写命令后不会等待完成：FSM 忙时任何从 Flash 取指的
 *          执行流（含中断服务程序）都会被拖住，所以"发命令 → 在 RAM 里等
 *          FSM 就绪 → 检查状态 → 返回"这一整段必须留在 RAM 内。
 *
 *          调用前提：处于调度器上下文、中断开启；本层内部用 DINT/EINT
 *          屏蔽可屏蔽中断，只保护擦写的那一小段，不改变外设配置。
 ******************************************************************************/

#ifndef UPGRADE_FLASH_H
#define UPGRADE_FLASH_H

#include "UpgradeCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 常量 ---------------- */

/** 本板 CPU 频率（MHz），F021 用它算 RWAIT/EWAIT */
#define UPGRADE_FLASH_HCLK_MHZ      (200UL)

/** 单次编程的最大字数（F021 一次最多 8 字 = 一个 128 位 ECC 单元） */
#define UPGRADE_FLASH_BLOCK_WORDS   (8U)

/** 等 FSM 的循环上限：按 200MHz 下最长擦除时间留足余量 */
#define UPGRADE_FLASH_WAIT_GUARD    (2000000UL)

/* ---------------- 接口 ---------------- */

/**
 * @brief  F021 初始化（一次性）：初始化 API 并选中 Flash Bank0
 * @return 1 成功，0 失败
 */
Uint16 UpgradeFlashInit(void);

/**
 * @brief  擦除一个扇区
 * @param  u32Addr 扇区起始地址（必须是扇区首地址）
 * @return 1 成功，0 失败
 */
Uint16 UpgradeFlashEraseSector(Uint32 u32Addr);

/**
 * @brief  编程一个 8 字块（自动生成 ECC），下完命令等到 FSM 就绪并回读比对
 * @param  u32Addr   块起始地址（必须 8 字对齐）
 * @param  pu16Data  数据缓冲（不足 8 字时由调用方补齐）
 * @param  u16Words  字数，必须为 8
 * @return 1 成功，0 失败
 */
Uint16 UpgradeFlashProgramBlock(Uint32 u32Addr, const Uint16 *pu16Data, Uint16 u16Words);

/**
 * @brief  从 Flash 读若干字（FSM 空闲时才允许调用）
 * @return 1 成功，0 入参错误
 */
Uint16 UpgradeFlashReadWords(Uint32 u32Addr, Uint16 *pu16Dst, Uint16 u16Words);

/** 供 UpgradeCore 使用的操作表 */
extern const ST_UPGRADE_FLASH_OPS g_stUpgradeFlashOps;

#ifdef __cplusplus
}
#endif

#endif /* UPGRADE_FLASH_H */

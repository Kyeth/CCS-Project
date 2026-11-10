/******************************************************************************
 * @file    AppUpgrade.h
 * @brief   APP 侧升级会话（XMODEM 接收 → 备份区 → 写配置 → 复位）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    端口所有权：Modbus 识别到六字节握手并回完 8 字节应答之后，把端口
 *          连同"这一批里多出来的字节"一起交给本层；本层发 'C'、收包、写备份、
 *          写配置，成功则复位，取消或失败则清掉本次残留并把端口交还 Modbus。
 *
 * @note    升级期间按复核裁定：暂停功率输出、控制 ISR、ADC/CLA 触发；保留时基、
 *          端口服务与看门狗。返回 Modbus 时不要自动重新开启功率输出——这由上层
 *          的恢复流程决定。
 ******************************************************************************/

#ifndef APPUPGRADE_H
#define APPUPGRADE_H

#include "F2837xS_device.h"
#include "UpgradeCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 常量 ---------------- */

/** 升级端口：ARM 口 = SCI-A = 端口下标 0 */
#define APP_UP_PORT_INDEX           (0U)

/** 芯片类型字节：主机打包工具必须用同一个值 */
#define APP_UP_CHIP_TYPE            (0x37U)

/** 交接时最多保存多少个"握手之后多出来的字节" */
#define APP_UP_TAIL_MAX             (64U)

/** 会话状态 */
#define APP_UP_STATE_IDLE           (0U)    /* 未升级，端口归 Modbus */
#define APP_UP_STATE_HOLD           (1U)    /* 已接管端口，等 8 字节应答发完 */
#define APP_UP_STATE_ERASE          (2U)    /* 逐扇区擦除备份区 */
#define APP_UP_STATE_RECEIVE        (3U)    /* XMODEM 收包中（含发 'C' 等首包） */
#define APP_UP_STATE_FINISH         (4U)    /* 校验长度/CRC，写配置 */
#define APP_UP_STATE_FAIL           (5U)    /* 失败：清配置、回 CAN、交还端口 */
#define APP_UP_STATE_DONE           (6U)    /* 成功：等发送排空后复位 */

/* ---------------- 接口 ---------------- */

/**
 * @brief  初始化升级层（只绑定回调，不碰端口）
 */
void AppUpgradeInit(void);

/**
 * @brief  替换底层 Flash 操作（主机测试注入替身用）
 * @param  pstOps 操作表，传 0 恢复默认的 F021 实现
 */
void AppUpgradeSetFlashOps(const ST_UPGRADE_FLASH_OPS *pstOps);

/**
 * @brief  接管端口，开始一次升级
 * @param  u16SciId   端口（硬件 SCI id）
 * @param  pu16Tail   握手批次里多出来的字节（可为 0）
 * @param  u16TailLen 字节数
 * @return 1 已接管，0 拒绝（已有会话在跑或入参非法）
 */
Uint16 AppUpgradeStart(Uint16 u16SciId, const Uint16 *pu16Tail, Uint16 u16TailLen);

/**
 * @brief  升级任务体：每轮推进一小步，不阻塞
 */
void AppUpgradeTask(void);

/**
 * @brief  是否正在升级（Modbus 据此让出端口）
 */
Uint16 AppUpgradeIsActive(void);

/**
 * @brief  当前会话状态（APP_UP_STATE_*）
 */
Uint16 AppUpgradeState(void);

/**
 * @brief  已写入备份区的正文字节数
 */
Uint32 AppUpgradeBodyBytes(void);

/**
 * @brief  最近一次会话的结束原因（XMODEM_* 或 UPGRADE_* 错误码）
 */
Uint16 AppUpgradeLastError(void);

/**
 * @brief  会话计数（成功 / 失败）
 */
Uint32 AppUpgradeDoneCount(void);
Uint32 AppUpgradeFailCount(void);

/**
 * @brief  注册升级期间的暂停/恢复钩子（功率输出、ADC/CLA 触发等）
 * @note   钩子可以为空；本层只在进入 RECEIVE 前调一次暂停，在离开升级时
 *         调一次恢复。恢复钩子是否重新开功率由上层决定。
 */
void AppUpgradeSetPauseHook(void (*pfPause)(void), void (*pfResume)(void));

#ifdef __cplusplus
}
#endif

#endif /* APPUPGRADE_H */

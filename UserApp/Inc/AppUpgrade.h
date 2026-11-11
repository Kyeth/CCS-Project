#ifndef __APPUPGRADE_H
#define __APPUPGRADE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 升级占用 ARM 端口（SCI-A），与 Modbus 握手同一路 */
#define APP_UP_PORT_INDEX   (0U)

/**
 * @brief 平台初始化：Xmodem 协议实例、Flash API、升级句柄（顺序同参考工程 PlatformInit）
 */
extern void AppUpgradeInit(void);

/**
 * @brief 握手应答发完后交接端口，之后由本模块独占该端口收升级流
 * @param u16SciId    端口（协议层传入的硬件 SCI id）
 * @param pu16Tail    握手同批取上来的升级流字节（可为 NULL）
 * @param u16TailLen  字节数
 * @return 1 = 已接管
 */
/* u32Baud = 本次会话实际使用的 SCI-A 速率，会按 x100 编码写进"接收中"配置，
 * BOOT 掉电重入时据此恢复串口（115200 → 1152）。 */
extern Uint16 AppUpgradeStart(Uint16 u16SciId, Uint32 u32Baud,
                              const Uint16 *pu16Tail, Uint16 u16TailLen);

/**
 * @brief 升级任务（周期调用）：等发送排空 -> 擦备份区 -> 收包
 */
extern void AppUpgradeTask(void);

extern Uint16 AppUpgradeIsActive(void);

/* 中止次数（供诊断与上板确认） */
extern Uint32 AppUpgradeAbortCount(void);

/**
 * @brief 挂升级开始时的暂停钩子（停功率输出、压制采样/控制动作）
 * @note  升级以复位收尾，因此不设恢复钩子；按 05F 裁定不自动重开功率输出
 */
extern void AppUpgradeSetPauseHook(void (*pfPause)(void));

#ifdef __cplusplus
}
#endif

#endif

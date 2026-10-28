/**
 * @file RelayCtl.h
 * @author yetingheng (004182)
 * @brief 继电器统一控制层（状态机公共层）
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note
 *   - 本层只负责"把某个继电器打到某个状态"，不判断该不该打；
 *     什么时候打由各状态机的继电器矩阵决定（对应 EP5K 的 ControlRelay(state)）。
 *   - 所有继电器都是高电平闭合（1:闭合 0:断开），与 DrvCfg_EP15K.c 的 GPIO 表一致。
 *   - ATS 干接点（GPIO52/53）不在这里，见 AtsCtl.h。
 */

#ifndef RELAY_CTL_H
#define RELAY_CTL_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 继电器 ID
 ******************************************************************************/
typedef enum
{
    E_RELAY_INV_R = 0,      /* GPIO58  逆变端 R 相继电器 */
    E_RELAY_INV_S,          /* GPIO61  逆变端 S 相继电器 */
    E_RELAY_INV_N,          /* GPIO62  逆变端 N 相继电器 */
    E_RELAY_INV_T,          /* GPIO63  逆变端 T 相继电器 */
    E_RELAY_PRECHG,         /* GPIO73  预充继电器 */
    E_RELAY_PE,             /* GPIO66  接地继电器 */
    E_RELAY_AUX,            /* GPIO74  市电辅源继电器 */
    E_RELAY_PV_ISO1,        /* GPIO50  PV 绝缘检测继电器 1 */
    E_RELAY_PV_ISO2,        /* GPIO51  PV 绝缘检测继电器 2 */
    E_RELAY_NUM
} E_RELAY_ID;

/* 逆变端四相继电器位掩码（E_RELAY_INV_R..E_RELAY_INV_T） */
#define RELAY_CTL_INV_MASK      (0x000FU)
/* PV 绝缘检测继电器位掩码 */
#define RELAY_CTL_ISO_MASK      (0x0180U)

/******************************************************************************
 * API
 ******************************************************************************/
/**
 * @brief 初始化：全部继电器置断开，清内部缓存
 */
extern void RelayCtlInit(void);

/**
 * @brief 设置单个继电器
 * @param eId   继电器 ID
 * @param u16On 0=断开 1=闭合
 */
extern void RelayCtlSet(E_RELAY_ID eId, Uint16 u16On);

/**
 * @brief 读回上次下发的命令状态（不是回读 GPIO，粘连检测请用两侧电压）
 */
extern Uint16 RelayCtlGet(E_RELAY_ID eId);

/**
 * @brief 逆变端四相一起动作（R/S/N/T）
 */
extern void RelayCtlInvAll(Uint16 u16On);

/**
 * @brief 取命令状态位图，bit n 对应 E_RELAY_ID = n
 */
extern Uint32 RelayCtlGetMask(void);

#ifdef __cplusplus
}
#endif

#endif /* RELAY_CTL_H */

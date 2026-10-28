/**
 * @file DataExchange.h
 * @author yetingheng (004182)
 * @brief 点表镜像与内部变量交换的接口
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 协议镜像与内部变量之间的搬运层。
 * Rx：ARM 下发的命令/参数 -> 内部变量（1 kHz）
 * Tx：内部采样、状态、故障 -> 发布镜像（1 kHz）
 * 好处是协议字段与业务变量解耦，两边各自改名互不影响。
 */
#ifndef DATA_EXCHANGE_H
#define DATA_EXCHANGE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DATA_EXCHANGE_TASK_TICK     (10U)   // 10 x 100us = 1 kHz

extern void DataExchangeInit(void);
extern void DataExchangeRx(void);
extern void DataExchangeTx(void);
extern void DataExchangeTask(void *pArgV);

#ifdef __cplusplus
}
#endif

#endif /* DATA_EXCHANGE_H */

/**
 * @file InternalCanProcess.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __INTERNALCANPROCESS_H
#define __INTERNALCANPROCESS_H

#include "InternalCan.h"

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief 内部CAN协议初始化
 * 
 */
extern void InternalCanInit(void);

/**
 * @brief 内部CAN协议任务
 * 
 */
extern void TaskInternalCan_1(void);
extern ST_INTERNALCAN_HANDLE g_stInternalCanHandle;

#ifdef __cplusplus
    }
#endif

#endif

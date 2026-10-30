/**
 * @file InternalCanProcess.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "InternalCanProcess.h"
#include "InternalCanTramsport.h"
#include "InternalCanCfg.h"
#include "Hw_GpioDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif

ST_INTERNALCAN_HANDLE g_stInternalCanHandle;

/**
 * @brief 内部CAN协议初始化
 * 
 */
void InternalCanInit(void)
{
    ST_INTERNAL_CAN_TRANSPORT_INTF stTranSportIntf;

    // 传输接口初始化，具体实现来源于InternalCanTramsport.h
    stTranSportIntf.CanRead = InternalCanReadTramsport;
    stTranSportIntf.CanWrite = InternalCanWriteTramsport;

    InternalCanRegister(&g_stInternalCanHandle, &g_stInernalCanTable, &stTranSportIntf);
}

/**
 * @brief 内部CAN协议任务
 * 
 */
void TaskInternalCan_1(void)
{
    // 帧协议处理
    InternalCanReceive(&g_stInternalCanHandle);
    InternalCanTransmit(&g_stInternalCanHandle);

    // 数据帧的封装与解析
    InternalReceiveFrameProcess(&g_stInternalCanHandle);
    InternalTransmitFrameProcess(&g_stInternalCanHandle);
}

#ifdef __cplusplus
    }
#endif

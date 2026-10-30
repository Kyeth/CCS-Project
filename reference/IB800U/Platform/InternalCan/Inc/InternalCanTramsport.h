/**
 * @file InternalCanTramsport.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __INTERNALCANTRAMSPORT_H
#define __INTERNALCANTRAMSPORT_H

#include "InternalCan.h"

#ifdef __cplusplus
    extern "C" {
#endif



/**
 * @brief 内部CAN通信读取回调接口实现
 * 
 * @param pstCanFrame   :应用层CAN数据帧对象指针
 * @param u16ReadSize   :读取的最大长度
 * @return int16    :>=0 已读取的长度
 */
extern int16 InternalCanReadTramsport(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize);



/**
 * @brief 内部CAN通信写入回调接口实现
 * 
 * @param pstCanFrame   :应用层CAN数据帧对象指针
 * @param u16WriteSize  :写入的最大长度
 * @return int16    :>=0 已写入的数据长度
 */
extern int16 InternalCanWriteTramsport(ST_INTERNAL_CAN_FRAME *pstCanFrame, Uint16 u16WriteSize);


#ifdef __cplusplus
    }
#endif

#endif

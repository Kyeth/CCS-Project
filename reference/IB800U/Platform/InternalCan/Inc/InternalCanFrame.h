/**
 * @file InternalCanFrame.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __INTERNALCANFRAME_H
#define __INTERNALCANFRAME_H


#include "InternalCan.h"


#ifdef __cplusplus
    extern "C" {
#endif



/**
 * @brief 根据帧ID查找到在周期表中的索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :应用层CAN帧，内部的ID需要有效
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未找到下标，帧ID不存在周期表中
 *                  : >=0 数组下标
 */
extern int16 SearchPeriodTableIndex(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_FRAME *pstCanFrame);



/**
 * @brief 根据帧ID查找到在触发表中的索引下标
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstCanFrame           :应用层CAN帧，内部的ID需要有效
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未找到下标，帧ID不存在周期表中
 *                  : >=0 数组下标
 */
extern int16 SearchTrigTableIndex(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_INTERNAL_CAN_FRAME *pstCanFrame);



/**
 * @brief 根据数组索引下标从周期表中取出对应的子项
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param i16Index              :数组的索引下标
 * @param pstGetItem            :获取的子项
 * @return int16        :-1 参数非法或者未初始化
 *                      :-2 下标不存在
 *                      :0 查找成功
 */
extern int16 GetIndexItemValuefromPeriodTable(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, int16 i16Index, ST_TRIG_TABLE *pstGetItem);



/**
 * @brief 根据数组索引下标从触发表中取出对应的子项
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param i16Index              :数组的索引下标
 * @param pstGetItem            :获取的子项
 * @return int16        :-1 参数非法或者未初始化
 *                      :-2 下标不存在
 *                      :0 查找成功
 */
extern int16 GetIndexItemValuefromTrigTable(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, int16 i16Index, ST_ITEM_TABLE *pstGetItem);



/**
 * @brief 将值写入对象字典对应的变量中
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstItem               :从周期表或者触发表中获取的子项
 * @param pstCanFrame           :从CAN总线中接收到的数据帧
 * @return int16        :-1 参数非法或者未初始化
 *                      :0 设置成功
 */
extern int16 SetValueToObjDictionary(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_ITEM_TABLE *pstItem, ST_INTERNAL_CAN_FRAME *pstCanFrame);



/**
 * @brief 根据子项从对象字典中获取变量的值写入CAN数据帧中
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param pstItem               :从周期表或者触发表中获取的子项
 * @param pstCanFrame           :需要填充的数据帧
 * @return int16        :-1 参数非法或者未初始化
 *                      :0 获取成功
 */
extern int16 GetValueFormObjDictionary(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, ST_ITEM_TABLE *pstItem, ST_INTERNAL_CAN_FRAME *pstCanFrame);



/**
 * @brief 设置触发帧请求
 * 
 * @param pstInternalCanHandle  :InternalCan控制对象
 * @param u32CanFrameID         :需要触发的帧ID，需要存在在触发表中
 * @return int16    :-1 参数非法或者未初始化
 *                  :-2 未在触发表中找到该帧
 *                  :0 触发帧请求队列已满
 *                  :1 已下发触发请求
 */
extern int16 SetTrigRequest(ST_INTERNALCAN_HANDLE *pstInternalCanHandle, Uint32 u32CanFrameID);


#ifdef __cplusplus
    }
#endif

#endif

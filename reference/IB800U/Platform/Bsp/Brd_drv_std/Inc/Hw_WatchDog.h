/**
 * @file Hw_WatchDog.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_WATCHDOG_H
#define __HW_WATCHDOG_H

#include "F28003x_device.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @brief 初始化看门狗，配置超时复位时间，
 *          初始化之后需要调用Hw_EnableWatchDog启动看门狗
 * 
 * @param TimeOut :超时时间，单位ms
 */
extern void Hw_WatchDogInit(Uint16 TimeOut);



/**
 * @brief 启动看门狗
 * 
 */
extern void Hw_EnableWatchDog(void);



/**
 * @brief 禁用看门狗
 * 
 */
extern void Hw_DisableWatchDog(void);



/**
 * @brief 喂狗操作
 * 
 */
extern void Hw_FeedWatchDog(void);



/**
 * @brief 看门狗复位芯片
 *
 */
extern void Hw_SysReset(void);

#ifdef __cplusplus
}
#endif


#endif

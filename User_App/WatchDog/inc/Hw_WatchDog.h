#ifndef HW_WATCHDOG_H
#define HW_WATCHDOG_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化看门狗，配置超时复位时间
 *        初始化之后需要调用 Hw_EnableWatchDog 启动看门狗
 *        注意：F28374S 硬件最大超时约 1.6384ms，若入参大于此值，将自动配置为最大分频。
 *
 * @param TimeOut 超时时间，单位 ms
 */
extern void Hw_WatchDogInit(Uint16 TimeOut);

/**
 * @brief 启动看门狗
 */
extern void Hw_EnableWatchDog(void);

/**
 * @brief 禁用看门狗
 */
extern void Hw_DisableWatchDog(void);

/**
 * @brief 喂狗操作
 */
extern void Hw_FeedWatchDog(void);

/**
 * @brief 看门狗复位芯片
 */
extern void Hw_SysReset(void);

#ifdef __cplusplus
}
#endif

#endif /* HW_WATCHDOG_H */

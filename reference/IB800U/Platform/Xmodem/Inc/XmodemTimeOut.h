#ifndef __XMODEMTIMEOUT_H
#define __XMODEMTIMEOUT_H



#include "F28003x_device.h"


#ifdef __cplusplus
extern "C" {
#endif




typedef struct TimeOutManager ST_TIMEOUT_MANAGER;

typedef struct TimerDriver
{
    void (*TimeInit)(void);         // 初始化定时器
    void (*TimeStart)(ST_TIMEOUT_MANAGER *stTimeOutManager, Uint32 u32Ms);   // 启动定时器，设置超时时间
    int16 (*IsTimeOut)(ST_TIMEOUT_MANAGER *stTimeOutManager);       // 检测是否超时
    void (*TimeStop)(ST_TIMEOUT_MANAGER *stTimeOutManager);         // 停止定时器
} ST_TIMER_DRIVER;

struct TimeOutManager
{
    ST_TIMER_DRIVER stTimeDriver;   // 定时器驱动
    Uint32 u32StartTick;            // 启动定时器时Tick值
    Uint32 u32TimeOut;              // 预计超时时间
    Uint16 u16IsActive;             // 定时器是否激活
};


extern void TimeInit(void);
extern void TimeStart(ST_TIMEOUT_MANAGER *pstTimeOutManager, Uint32 u32Ms);
extern int16 IsTimeOut(ST_TIMEOUT_MANAGER *pstTimeOutManager);
extern void TimeStop(ST_TIMEOUT_MANAGER *pstTimeOutManager);

#ifdef __cplusplus
}
#endif


#endif

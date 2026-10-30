//#############################################################################
//
// FILE:   Schedule.h
//
// TITLE:  閺夊灝鐡欓惃鍕�栨禒鑸靛▕鐠炩槄绱欓崢鐔烘倞閸ュ墽娈戞潪顖欐閹诲繗鍫弬鍥︽閿涳拷
//
//#############################################################################


#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#ifdef __cplusplus
extern "C"
{
#endif

//
// Included Files
//
#include "driverlib.h"

//
// Check for invalid compile flags
//
#if defined(__TMS320C28XX_FPU64__)
#error "Invalid FPU Configuration"
#endif

#define TASK_SYS_MACHINE_BIT        ((uint16_t)1 << 0)
#define TASK_PLATFORM_BIT           ((uint16_t)1 << 1)
#define TASK_BAT_PARAL_BIT          ((uint16_t)1 << 2)
#define TASK_FAULT_DETECT_BIT       ((uint16_t)1 << 3)

#define TASK_MOBUS_SLAVE1_BIT       ((uint16_t)1 << 4)
#define TASK_MOBUS_SLAVE2_BIT       ((uint16_t)1 << 5)
#define TASK_CHECK_UPGRADE_BIT      ((uint16_t)1 << 6)
#define TASK_BAT_PARAL_COMM_BIT     ((uint16_t)1 << 7)

#define TASK_LED_BIT                ((uint16_t)1 << 8)
#define TASK_HEATING_BIT            ((uint16_t)1 << 9)
#define TASK_FAN_BIT                ((uint16_t)1 << 10)
#define TASK_TEMP_BIT               ((uint16_t)1 << 11)

#define TASK_OSC_PARAM_BIT          ((uint16_t)1 << 12)

#define ISR_TIME1_BIT               ((uint16_t)1 << 13)
#define ISR_TIME2_BIT               ((uint16_t)1 << 14)

#define TASK_ALL_SYS_BIT            (0x7FFF)

// 任务控制对象
typedef struct tagTaskCtrl
{
    uint8_t On;         // 周期达到
    uint8_t Num;        // 已注册的任务数
}TTaskCtrl;


typedef struct tagTask
{
    uint16_t Time;           // 周期值
    uint16_t Cnt;            // 计数值
    void (*Callback)(void);  // 回调函数
}TTask;
//----------------------------------------------------
extern TTaskCtrl TaskCTL;
extern TTask Task[];

extern void Task_Start(void (*task)(void), void* pArgV, uint16_t time, uint16_t point);

extern void Task_Run();
extern inline void SetTaskRunStatus(uint16_t wData);
extern void SetTaskFaultBit(void);

#ifdef __cplusplus
}
#endif

#endif // __DEVICE_H__

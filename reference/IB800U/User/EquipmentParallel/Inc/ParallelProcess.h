#ifndef __PARALLELPROCESS_H
#define __PARALLELPROCESS_H

#include "EquipmentParallelCore.h"
#include "LLCControl.h"

#define PARALLEL_LOCAL      0
#define PARALLEL_MASTER     1
#define PARALLEL_SLAVE      2

#define PARALLEL_STATE_1    1
#define PARALLEL_STATE_2    2
#define PARALLEL_STATE_3    3

#ifdef __cplusplus
    extern "C" {
#endif

extern void BatteryParallelInit(void);
extern void TaskBatteryParallelComm(void);

extern void TaskBatteryParallelProcess(void);
extern float GetAvgCurr(ST_BATTERY_HANDLE *pstBatteryHandle, ST_SYS_CTRL *pObjSystemCtrl);
extern ST_BATTERY_HANDLE stBatteryHandle;

#ifdef __cplusplus
    }
#endif

#endif

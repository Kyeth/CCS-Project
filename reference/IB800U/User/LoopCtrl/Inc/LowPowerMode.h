#include "F28003x_device.h"

#ifndef USER_LOOPCTRL_INC_LOWPOWERMODE_H_
#define USER_LOOPCTRL_INC_LOWPOWERMODE_H_

typedef struct
{
    Uint16 Status;              // 低功耗状态
    Uint16 Rise;
    Uint16 EPWM_Start;
    Uint16 WaitTime;
    Uint16 WaitTimeSet;
    Uint16 FltCnt;
    Uint16 FltCntSet;
    Uint16 RiseCounter;
    Uint16 FallCounter;

    float BusVH;
    float BusVL;

}ST_POWERMODE;

extern void TaskLowPowerMode(void);
extern void LowPower_Exit(void);
extern void LowPower_Reset(void);
extern void SetBusV(float Vset);
extern void LowPowerModeFunc(float VBus, float IBus, ST_POWERMODE *pstData, Uint16 u16TaskCyc);
extern void InitLowPowerModeData(void);

extern ST_POWERMODE LowPowerMode;

#endif /* USER_LOOPCTRL_INC_LOWPOWERMODE_H_ */

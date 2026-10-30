#ifndef DRVCFG_EP15K_H
#define DRVCFG_EP15K_H

#include "Hw_GpioDriver.h"
#include "Hw_ClockDriver.h"
#include "Hw_PwmDriver.h"
#include "Hw_CpuTimerDriver.h"
#include "Hw_AdcDriver.h"
#include "Hw_ClaDriver.h"
#include "Hw_SciDriver.h"
#include "Hw_CanDriver.h"
#include "Hw_XbarDriver.h"

/* Board requirements are declared here and implemented as tables in Broad/src. */
extern ST_GPIO_CFG stGpioCfg;
extern const ST_CLOCK_CFG stClockCfg;
extern ST_PWM_CFG stPwmCfg;
extern ST_PWM_TZ_CFG stPwmTzCfg;
extern ST_XBAR_CFG stXbarCfg;
extern ST_ADC_CFG stAdcCfg;
extern ST_CLA_CFG stClaCfg;
extern ST_SCI_CFG stSciCfg;
extern ST_CAN_CFG stCanCfg;
extern volatile ST_SYS_TIMER stSysCpuTimer1;
extern const Uint16 stClaMuxNext[8];
extern const float fClaMuxAlpha;

#endif

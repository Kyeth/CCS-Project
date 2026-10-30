#ifndef __DRVCFG_IB800_H
#define __DRVCFG_IB800_H

#include "Hw_GpioDriver.h"
#include "Hw_SciDriver.h"
#include "Hw_EcapDriver.h"
#include "Hw_PwmDriver.h"
#include "Hw_AdcDriver.h"
#include "Hw_ExitDriver.h"
#include "Hw_CanDriver.h"
#include "Hw_I2cDriver.h"
#include "Hw_ClaDriver.h"
#include "Hw_MCanDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif

extern ST_GPIO_CFG stGpioCfg;
extern ST_SCI_CFG stSciCfg;
extern ST_PWM_CFG stPwmCfg;
extern ST_ADC_CFG stAdcCfg;
extern ST_EXIT_CFG stExitCfg;
extern ST_CAN_CFG stCanCfg;
extern ST_I2C_CFG stI2cCfg;
extern ST_CLA_CFG stClaCfg;
extern ST_MCAN_CFG stMCanCfg;

#ifdef __cplusplus
    }
#endif

#endif

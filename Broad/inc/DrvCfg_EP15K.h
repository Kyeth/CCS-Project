#ifndef __DRVCFG_EP15K_H
#define __DRVCFG_EP15K_H

#include "Hw_GpioDriver.h"
#include "Hw_PwmDriver.h"
#include "Hw_CpuTimerDriver.h"
#include "Hw_AdcDriver.h"
#include "Hw_ClaDriver.h"
#include "Hw_InvDriver.h"
#include "Hw_SciDriver.h"
#include "Hw_CanDriver.h"
#include "Hw_EcapDriver.h"
#include "ModbusSlave.h"


#ifdef __cplusplus
    extern "C" {
#endif

extern ST_GPIO_CFG stGpioCfg;
extern ST_PWM_CFG stPwmCfg;

/* 系统节拍定时器：硬件中断周期100us，每100us产生一个软件心跳，中断中调用SYS_TICK() */
extern volatile ST_SYS_TIMER stSysCpuTimer1;

/* 辅助定时器：硬件中断周期1ms，每1ms产生一个软件心跳，可用于喂狗、毫秒级计时 */
extern volatile ST_SYS_TIMER stSysCpuTimer2;

/* ADC模块配置：四路ADC的SOC通道表与中断配置，详见 stAdcCfg */
extern ST_ADC_CFG stAdcCfg;

/* CLA任务配置：Task1 由 ADC-A INT1 硬件触发，详见 stClaCfg */
extern ST_CLA_CFG stClaCfg;

/* 逆变驱动配置：三电平单极性，R/S/T 相与正负半周模块映射，详见 stInvCfg */
extern ST_INV_CFG stInvCfg;

/* SCI串口配置：本板三个SCI模块A/C/D，详见 stSciCfg */
extern ST_SCI_CFG stSciCfg;

/* CAN配置：本板两个CAN模块A/B，详见 stCanCfg */
extern ST_CAN_CFG stCanCfg;

/* Modbus 从机配置：SCIA 给 ARM、SCID 给上位机，详见 stMbCfg */
extern ST_MB_CFG stMbCfg;

/* TVS 走 Modbus（SCIC 只听不应答），其从机配置在 stMbCfg 里，点表见 stMbRegItemTvs */

/* eCAP捕获配置：电网过零(R/S/T)与电网频率信号(R/S/T)，详见 stEcapCfg */
extern ST_ECAP_CFG stEcapCfg;

#ifdef __cplusplus
    }
#endif

#endif

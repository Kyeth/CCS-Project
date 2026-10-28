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
#include "Hw_I2cDriver.h"
#include "Hw_SpiDriver.h"
#include "Hw_XbarDriver.h"
#include "AtsProto.h"
#include "Hw_ExitDriver.h"
#include "Fm24c128d.h"
#include "EepromParam.h"
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

/* ATS（断路器）Modbus 通道监听配置：与 ARM 共用同一条 RS485 */
extern ST_ATS_PROTO_CFG stAtsProtoCfg;

/* TVS 走 Modbus（SCIC 只听不应答），其从机配置在 stMbCfg 里，点表见 stMbRegItemTvs */

/* eCAP捕获配置：电网过零(R/S/T)与电网频率信号(R/S/T)，详见 stEcapCfg */
extern ST_ECAP_CFG stEcapCfg;

/* 软件I2C配置：GPIO91 SDA / GPIO92 SCL，本板挂 FM24C128D，详见 stI2cCfg */
extern ST_I2C_CFG stI2cCfg;

/* FM24C128D 器件配置：写保护脚 GPIO99、自检保留区，详见 stFm24c128dCfg */
extern ST_FM24C128D_CFG stFm24c128dCfg;

/* 掉电保存参数配置：扫描哪几张点表、合并窗口与落盘限速，详见 stEepromParamCfg */
extern ST_EEPROM_PARAM_CFG stEepromParamCfg;

/* SPI主机配置：SPI-A 接 ARM 芯片 STM32F103VET6，详见 stSpiCfg */
extern ST_SPI_CFG stSpiCfg;

/* ePWM TZ 故障源：把 ePWM X-BAR 的 TRIP 接进各模块 DCAH/DCBH 做逐波限流，详见 stPwmTzCfg */
extern ST_PWM_TZ_CFG stPwmTzCfg;

/* X-BAR 配置：GPIO -> INPUTx -> TRIPn，故障信号汇聚路径，详见 stXbarCfg */
extern ST_XBAR_CFG stXbarCfg;

/* 外部中断配置：XINT1 <- INPUT4(逆变过流) / XINT2 <- INPUT5(硬件短路) */
extern ST_EXIT_CFG stExitCfg;

#ifdef __cplusplus
    }
#endif

#endif

/**
 * @file DrvCfg_EP15K.h
 * @brief 05E 主机测试用的板级配置垫片（只声明 AppModbus.c 用到的 stSciCfg）
 *
 * 真实头文件在 Broad/inc，会拉进 ADC/CAN/CLA 等一大串驱动头；主机测试只需要
 * "从哪儿查波特率"这一件事，所以在测试目录里放同名垫片，并把它排在包含路径首位。
 */

#ifndef DRVCFG_EP15K_SHIM_H
#define DRVCFG_EP15K_SHIM_H

#include "Hw_SciDriver.h"

extern ST_SCI_CFG stSciCfg;

#endif /* DRVCFG_EP15K_SHIM_H */

/**
 * @file Hw_AdcDriver.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_ADCDRIVER_H
#define __HW_ADCDRIVER_H

#include "F28003x_device.h"


#ifdef __cplusplus
    extern "C" {
#endif


#define ADC_RESOLUTION_BIT_12 0
#define ADC_RESOLUTION_BIT_16 1


#define ADC_SIGNALMODE_SINGLE       0
#define ADC_SIGNALMODE_DIFFERENTIAL 1

typedef enum
{
    ADC_ID0 = 0x00,     // 对应AdcA
    ADC_ID1 = 0x01,     // 对应AdcB
    ADC_ID2,
    ADC_ID_MAX = 0x03,
    ADC_ID_NULL = 0xFF,
}E_ADCID;

typedef struct
{
    Uint16 bTrigSel :5; // 配置触发源
    Uint16 bAcqps   :11; // 采样窗口时间
    Uint16 bChSel   :4; // 采样通道选择
    Uint16 Rsv      :12; // 保留

}ST_ADC_CH_CFG_ITEM;

typedef struct
{
    Uint16 u16AdcId;
    const ST_ADC_CH_CFG_ITEM *pstAdcChCfgItem;
    Uint16 bResolution  :1;
    Uint16 bSignalMode  :1;
    Uint16 bAdcIntEnable:1;
    Uint16 bIntSel      :4;

}ST_ADC_CFG_ITEM;

typedef struct
{
    ST_ADC_CFG_ITEM *pstAdcCfgItem;
    Uint16 u16ItemNum;
}ST_ADC_CFG;



/**
 * @brief 用于初始化单个ADC外设
 * 
 * @param pstAdcCfgItem 入参是一个ST_ADC_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleAdcInit(ST_ADC_CFG_ITEM* pstAdcCfgItem);



/**
 * @brief 用于初始化一组ADC，通常用于参数初始化阶段
 * 
 * @param pstAdcCfg :入参是一组ST_ADC_CFG结构体对象的集合
 */
extern void Hw_AdcInit(ST_ADC_CFG* pstAdcCfg);



/**
 * @brief 从ADC外设采样结果寄存器中读取采样值
 * 
 * @param u16AdcId  : ADC的ID号
 * @param pu16AdcResult : volatile Uint16* 指针类型的地址
 * @return int16    :-1 参数非法
 *                  : 0 读取成功
 */
extern int16 Hw_AdcRead(Uint16 u16AdcId, Uint16 **pu16AdcResult);




extern void Hw_AdcClearInitFlag(Uint16 u16AdcId);
#ifdef __cplusplus
    }
#endif

#endif

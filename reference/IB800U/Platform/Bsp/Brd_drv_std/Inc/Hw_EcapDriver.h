/**
 * @file Hw_EcapDriver.h
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_ECAPDRIVER_H
#define __HW_ECAPDRIVER_H

#include "F28003x_device.h"


#ifdef __cplusplus
extern "C" {
#endif


// EcapId与芯片ID增长顺序相关
typedef enum ECapId
{
    ECAP_ID0     = 0x00, // 在DSP中对应Ecap1
    ECAP_ID1     = 0x01, // 在DSP中对应Ecap2
    ECAP_ID2     = 0x02, // 在DSP中对应Ecap3
    ECAP_ID3     = 0x03, // 在DSP中对应Ecap4
    ECAP_ID4     = 0x04, // 在DSP中对应Ecap4
    ECAP_ID5     = 0x05, // 在DSP中对应Ecap4
    ECAP_ID_MAX  = 0x06,
    ECAP_ID_NULL = 0xFF,
}E_ECAPID;



typedef enum
{
    DISABLE_EDGE = 0x00,
    RISING_EDGE = 0x2,
    FALLING_EDGE = 0x3,
}E_CAP_POLARITY;



typedef enum
{
    EC_CONTINUOUS = 0x00,
    EC_ONESHOT,
}E_CAP_ONESHT_MODE;

typedef struct
{
    Uint16 u16EcapId;
    Uint16 bGPIONum         :8;     // IO号
    Uint16 bCap1OL          :2;     // Cap1触发极性
    Uint16 bCap2OL          :2;     // Cap2触发极性
    Uint16 bCap3OL          :2;     // Cap3触发极性
    Uint16 bCap4OL          :2;     // Cap4触发极性
    Uint16 bCapOneShtMode         :1;     // 工作模式：1-单次模式；0-连续模式
}ST_ECAP_CFG_ITEM;

typedef struct
{
    ST_ECAP_CFG_ITEM *pstEcapCfgItem;
    Uint16 u16ItemNum;
}ST_CAP_CFG;



typedef struct
{
    volatile struct ECAP_REGS *pstEcapRegs;
    // 对以下变量不可直接使用，请通过相应的接口函数来获取
    Uint32 u32EcapValue_1;
    Uint32 u32EcapValue_2;
    Uint32 u32EcapValue_3;
    Uint32 u32EcapValue_4;
    Uint16 bEcapCEVT1   :1;
    Uint16 bEcapCEVT2   :1;
    Uint16 bEcapCEVT3   :1;
    Uint16 bEcapCEVT4   :1;
    Uint16 bIsInit      :1;
}ST_ECAP_HANDLE;



/**
 * @brief 用于初始化单个eCap外设
 * 
 * @param pstCapCfgItem 入参是一个ST_ECAP_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleEcapInit(ST_ECAP_CFG_ITEM *pstCapCfgItem);



/**
 * @brief 用于初始化一组eCap，通常用于参数初始化阶段
 * 
 * @param pstCapCfg :入参是一组ST_CAP_CFG结构体对象的集合
 */
extern void Hw_EcapInit(ST_CAP_CFG *pstCapCfg);



/**
 * @brief 获取对应Cap的计数值
 * 
 * @param u16EcapId :eCap的ID
 * @param Index     :eCap的索引(可选1、2、3、4)
 * @return Uint32   :返回对应eCap寄存器的值
 *                  :0xFFFFFFF 索引非法
 */
extern Uint32 Hw_GetEcapValue(Uint16 u16EcapId, Uint16 Index);



/**
 * @brief 返回eCap捕获事件
 * 
 * @param u16EcapId :eCap的ID
 * @param Index     :eCap的索引(可选1、2、3、4)
 * @return int16    :1 发生捕获事件
 *                  :0 未发生事件
 *                  :-1 ID非法
 *                  :-2 索引非法
 */
extern int16 Hw_GetEcapCEVT(Uint16 u16EcapId, Uint16 Index);



/**
 * @brief 检测eCap事件是否发生，通常在中断中调用
 * 
 */
extern void Hw_IsrEcapData(void);



#ifdef __cplusplus
}
#endif

#endif

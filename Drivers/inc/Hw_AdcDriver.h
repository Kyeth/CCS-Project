/**
 * @file Hw_AdcDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS ADC驱动头文件
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef HW_ADC_DRIVER_H
#define HW_ADC_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 每个ADC模块的SOC数量与结果寄存器数量，F2837xS固定为16 */
#define HW_ADC_SOC_MAX                  (16U)

/* ADC分辨率，取值即为ADCCTL2.RESOLUTION的值 */
#define HW_ADC_RESOLUTION_12BIT         (0U)
#define HW_ADC_RESOLUTION_16BIT         (1U)

/* 信号模式，取值即为ADCCTL2.SIGNALMODE的值 */
#define HW_ADC_SIGNALMODE_SINGLE        (0U)
#define HW_ADC_SIGNALMODE_DIFFERENTIAL  (1U)

/* SOC触发源，取值即为ADCSOCxCTL.TRIGSEL的值
 * 规律：EPWMn_SOCA = 5 + 2*(n-1)，EPWMn_SOCB = 6 + 2*(n-1) */
#define HW_ADC_TRIG_SOFTWARE            (0U)    /* 软件强制(ADCSOCFRC1) */
#define HW_ADC_TRIG_CPU1_TINT0          (1U)    /* CPU定时器0中断 */
#define HW_ADC_TRIG_CPU1_TINT1          (2U)    /* CPU定时器1中断 */
#define HW_ADC_TRIG_CPU1_TINT2          (3U)    /* CPU定时器2中断 */
#define HW_ADC_TRIG_GPIO                (4U)    /* GPIO(经输入X-BAR) */
#define HW_ADC_TRIG_EPWM1_SOCA          (5U)
#define HW_ADC_TRIG_EPWM1_SOCB          (6U)
#define HW_ADC_TRIG_EPWM2_SOCA          (7U)
#define HW_ADC_TRIG_EPWM2_SOCB          (8U)
#define HW_ADC_TRIG_EPWM3_SOCA          (9U)
#define HW_ADC_TRIG_EPWM3_SOCB          (10U)
#define HW_ADC_TRIG_EPWM4_SOCA          (11U)
#define HW_ADC_TRIG_EPWM4_SOCB          (12U)
#define HW_ADC_TRIG_EPWM5_SOCA          (13U)
#define HW_ADC_TRIG_EPWM5_SOCB          (14U)
#define HW_ADC_TRIG_EPWM6_SOCA          (15U)
#define HW_ADC_TRIG_EPWM6_SOCB          (16U)
#define HW_ADC_TRIG_EPWM7_SOCA          (17U)
#define HW_ADC_TRIG_EPWM7_SOCB          (18U)
#define HW_ADC_TRIG_EPWM8_SOCA          (19U)
#define HW_ADC_TRIG_EPWM8_SOCB          (20U)
#define HW_ADC_TRIG_EPWM9_SOCA          (21U)
#define HW_ADC_TRIG_EPWM9_SOCB          (22U)
#define HW_ADC_TRIG_EPWM10_SOCA         (23U)
#define HW_ADC_TRIG_EPWM10_SOCB         (24U)
#define HW_ADC_TRIG_EPWM11_SOCA         (25U)
#define HW_ADC_TRIG_EPWM11_SOCB         (26U)
#define HW_ADC_TRIG_EPWM12_SOCA         (27U)
#define HW_ADC_TRIG_EPWM12_SOCB         (28U)

/**
 * @brief ADC模块编号
 */
typedef enum
{
    HW_ADC_ID0 = 0U,            /* ADC-A */
    HW_ADC_ID1,                 /* ADC-B */
    HW_ADC_ID2,                 /* ADC-C */
    HW_ADC_ID3,                 /* ADC-D */
    HW_ADC_ID_MAX,
    HW_ADC_ID_NULL = 0xFFU
} E_HW_ADC_ID;

/**
 * @brief 单个SOC的通道配置
 *        成员顺序与配置表中的书写顺序一致，刻意不使用位域，避免跨16位字的位域
 *        打包方式依赖编译器实现
 */
typedef struct
{
    Uint16 u16TrigSel;      /* SOC触发源，见HW_ADC_TRIG_xxx */
    Uint16 u16Acqps;        /* 采样保持窗口，实际窗口 = (u16Acqps + 1) 个SYSCLK周期 */
    Uint16 u16ChSel;        /* 模拟输入通道号，0~15 */
} ST_ADC_CH_CFG_ITEM;

/**
 * @brief 单个ADC模块的配置
 */
typedef struct
{
    Uint16 u16AdcId;        /* ADC模块编号，见E_HW_ADC_ID */
    const ST_ADC_CH_CFG_ITEM *pstAdcChCfgItem;  /* 该模块SOC0~SOC15的通道配置表 */
    Uint16 u16Resolution;   /* 分辨率，见HW_ADC_RESOLUTION_xxx */
    Uint16 u16SignalMode;   /* 信号模式，见HW_ADC_SIGNALMODE_xxx */
    Uint16 bAdcIntEnable;   /* 1: 使能ADCINT1；0: 不使能 */
    Uint16 u16IntSel;       /* ADCINT1的触发SOC编号，bAdcIntEnable为1时有效 */
    Uint16 bAdcInt2Enable;  /* 1: 使能ADCINT2（第二采样批次用）；0: 不使能 */
    Uint16 u16Int2Sel;      /* ADCINT2的触发SOC编号 */
    Uint16 u16SocStride;    /* SOC表项跨度；交织的A/B/C/D表使用4 */
} ST_ADC_CFG_ITEM;

/**
 * @brief ADC初始化配置汇总，与各驱动统一的"表+项数"结构
 */
typedef struct
{
    const ST_ADC_CFG_ITEM *pstAdcCfgItem;   /* 模块配置表首地址 */
    Uint16 u16ItemNum;                      /* 模块配置项数 */
} ST_ADC_CFG;

/**
 * @brief 初始化全部ADC模块
 *        内部按配置表逐项调用 Hw_AdcSingleInit，顺序与表中顺序一致
 *
 * @param pstCfg ADC配置汇总表指针
 */
extern void Hw_AdcInit(const ST_ADC_CFG *pstCfg);

/**
 * @brief 初始化单个ADC模块
 *        内部依次完成：设置分辨率与信号模式(含OTP偏置校准) -> ADC上电并等待稳定
 *        -> 配置SOC0~SOC15的触发源/采样窗口/通道 -> 配置ADCINT1 -> 清中断标志
 *        注意：调用前必须先完成GPIO复用配置，且触发源所依赖的ePWM需要先初始化
 *
 * @param pstAdcCfgItem 模块配置项指针
 */
extern void Hw_AdcSingleInit(const ST_ADC_CFG_ITEM *pstAdcCfgItem);

/**
 * @brief 软件强制触发指定SOC的转换，用于调试或非ePWM触发的软件采样
 *
 * @param u16AdcId ADC模块编号
 * @param u16SocMask SOC位掩码，bit0对应SOC0，置1的位被强制触发
 */
extern void Hw_AdcSoftTrigger(Uint16 u16AdcId, Uint16 u16SocMask);

/**
 * @brief 使能/禁止ADCINT1中断
 *
 * @param u16AdcId ADC模块编号
 * @param bEnable 1: 使能；0: 禁止
 */
extern void Hw_AdcSetIntEnable(Uint16 u16AdcId, Uint16 bEnable);

/**
 * @brief 读取ADCINT1标志
 *
 * @param u16AdcId ADC模块编号
 * @return 1: 已有转换结束且未清除；0: 无标志
 */
extern Uint16 Hw_AdcGetIntFlag(Uint16 u16AdcId);

/**
 * @brief 清除ADCINT1标志，中断服务函数中必须调用
 *
 * @param u16AdcId ADC模块编号
 */
extern void Hw_AdcClearIntFlag(Uint16 u16AdcId);

/**
 * @brief 读取单个SOC的转换结果
 *        结果为12位左对齐(有效数据在bit15~bit4)，16位模式下为全16位有效
 *
 * @param u16AdcId ADC模块编号
 * @param u16Soc SOC编号，0~15
 * @return 结果寄存器原始值，参数非法时返回0
 */
extern Uint16 Hw_AdcReadResult(Uint16 u16AdcId, Uint16 u16Soc);

/**
 * @brief 一次性读取某模块SOC0~SOC15的全部结果
 *
 * @param u16AdcId ADC模块编号
 * @param pu16Buf 目标缓冲区，长度不得小于HW_ADC_SOC_MAX
 */
extern void Hw_AdcReadAll(Uint16 u16AdcId, Uint16 *pu16Buf);

#ifdef __cplusplus
}
#endif

#endif

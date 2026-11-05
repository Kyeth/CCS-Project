/**
 * @file AdcCalib.h
 * @author yetingheng (004182)
 * @brief 上电零漂与码值→工程值换算
 * @version 0.1
 * @date 2026-10-22
 * 数据流：ADC 结果寄存器 → SampleData.L1（码值）→ 本模块（减零点、乘系数）
 *         → SampleData.L2（V / A）→ 控制环 / 保护 / 显示。
 *
 * 零点只记录一次：上电 ADC_CAL_SKIP_TICKS（3 s）后，连续累加
 * ADC_CAL_TRACK_TICKS（4000 拍 = 200 ms = 50 Hz 的 10 个整周期）取平均。
 * 200 ms 内正弦净和为零，所以电网电压这类上电就带电的通道也能记录零点。
 *
 * L2 在零漂完成（READY）之前不更新，调用方通过 AdcCalibIsReady() 判断。
 */

#ifndef ADC_CALIB_H
#define ADC_CALIB_H

#include "F2837xS_device.h"
#include "SampleData.h"

/* 零漂状态机状态值（ST_SAMPLE_ZERO.u16State）。
 * 注意：FAULT 的数值比 READY 大，判“是否就绪”必须用 AdcCalibIsReady()，
 * 不能用 `u16State >= ADC_CAL_STATE_READY`。 */
typedef enum
{
    ADC_CAL_STATE_IDLE = 0U,/* 未启动 */
    ADC_CAL_STATE_SKIP,     /* 上电 3 s 内，不做零点 */
    ADC_CAL_STATE_TRACK,    /* 正在累加 4000 拍 */
    ADC_CAL_STATE_READY,    /* 零点有效，L2 已生效 */
    ADC_CAL_STATE_FAULT     /* 零点超差，相关通道回退标称值 */
} E_ADC_CAL;

#define ADC_CAL_SKIP_TICKS      (60000U)    /* 3 s @ 20 kHz */
#define ADC_CAL_TRACK_TICKS     (4000U)     /* 200 ms = 10 × 50 Hz = 12 × 60 Hz 整周期 */
#define ADC_CAL_ZERO_TOL        (307.0f)    /* ±15% of 2048 码，超差判 FAULT */

/*! 单通道标定项（表实例在 Broad/src/DrvCfg_Sample.c）。 */
typedef struct
{
    float  fK;          /* 码值 → 工程值，单位 V/码 或 A/码 */
    float  fZeroNom;    /* 标称零点码值，零漂超差时回退到它 */
    Uint16 bTrackZero;  /* 1 = 上电记录零漂；0 = 直接用 fZeroNom（多路选择器） */
} ST_SAMPLE_CAL_ITEM;

/*! 全部测点的标定表，按 L1/L2 的字段顺序组织。 */
typedef struct
{
    ST_SAMPLE_CAL_ITEM aInvCurr[SAMPLE_PH_NUM];     /* R / S / T */
    ST_SAMPLE_CAL_ITEM aInvVolt[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aGridVolt[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aInvDc[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aPvCurr[SAMPLE_PV_NUM];
    ST_SAMPLE_CAL_ITEM aPvVolt[SAMPLE_PV_NUM];      /* MUX2 地址 0~3 */
    ST_SAMPLE_CAL_ITEM aVisoVolt;                   /* MUX2 地址 4 */
    ST_SAMPLE_CAL_ITEM aBusLVolt;                   /* MUX1 地址 6，下半母线 */
    ST_SAMPLE_CAL_ITEM aBusHVolt;                   /* MUX1 地址 7，上半母线 */
    ST_SAMPLE_CAL_ITEM aBalCurr;
    ST_SAMPLE_CAL_ITEM aGfciCurr;
} ST_SAMPLE_CAL_CFG;

/**
 * @brief 用标定表的标称零点初始化 ST_SAMPLE_ZERO，状态置 IDLE。
 */
void AdcCalibInit(void);

/**
 * @brief 每个 20 kHz 采样拍推进一次零漂状态机。
 * @return 当前状态值（E_ADC_CAL_STATE）
 */
Uint16 AdcCalibStep(void);

/**
 * @brief 零点是否已经生效。
 * @return 1 = READY，0 = 未就绪或已判故障
 */
Uint16 AdcCalibIsReady(void);

/**
 * @brief 当前零漂状态（给观测变量用）。
 */
Uint16 AdcCalibGetState(void);

/**
 * @brief L1 → L2：逐通道 (code - zero) * K。
 *        只在 AdcCalibIsReady() 为 1 时调用，否则 L2 保持全零。
 */
void AdcCalibConvert(void);

#endif /* ADC_CALIB_H */

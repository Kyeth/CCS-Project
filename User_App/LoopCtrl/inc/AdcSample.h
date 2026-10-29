/**
 * @file AdcSample.h
 * @author yetingheng (004182)
 * @brief 采样统计层接口：刷新频率、统计窗口与对外函数
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 数据结构在 SampleData.h，本文件只管"多久刷新一次、窗口多长、滞后多快"。
 * stSampleIns   20 kHz 原样刷新，控制环专用，不做任何滤波；
 * stSampleAvg   块平均，8 拍（2.5 kHz）与 200 拍（100 Hz）两档；
 * stSampleRms   工频周期 RMS，再叠一阶滞后，供上位机 / ARM / 软示波器；
 * stSampleRaw   CLA 直出的码值，供标定调试与点表回读。
 */
#ifndef ADC_SAMPLE_H
#define ADC_SAMPLE_H

#include "F2837xS_device.h"
#include "SampleData.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 刷新频率与滤波参数
 ******************************************************************************/

/* 瞬时值刷新频率（CtrlIsr，20 kHz）与对外刷新频率（调度任务，1 kHz） */
#define ADC_SAMPLE_FAST_HZ          (20000U)
#define ADC_SAMPLE_TASK_HZ          (1000U)

/* 交流 RMS 的窗口长度由锁相角 theta 过零收口（50 Hz 下 400 拍 = 20 ms）；
 * 离网或 theta 停住时用最大样本数兜底，避免窗口无限长 */
#define ADC_SAMPLE_RMS_WIN_MIN      (10U)       /* 少于 10 拍不采信（约 0.5 ms） */
#define ADC_SAMPLE_RMS_WIN_MAX      (600U)      /* 30 ms 强制收口 */

/* RMS 一阶滞后系数：alpha = 1 / (tau * fs)，fs 取对外刷新率 1 kHz，
 * tau = 150 ms -> 到 99% 约 0.69 s。块平均本身已经足够平滑，只留这一处滞后。 */
#define ADC_SAMPLE_ALPHA_RMS        (0.00667f)

/* ==================== 统计层内部状态 ====================
 * RMS 平方和、块平均累加器、有功累加器与窗口计数全部收在这里，
 * 函数内一律经 stAdcSampleAcc 访问。 */
typedef struct
{
    float  fRmsAcc[11];         /* 平方和累加，下标见 AdcSample.c 的 RMS_xxx */
    float  fRmsRaw[11];         /* 本工频周期原始 RMS */
    float  fRmsPubPtr[11];      /* 一阶滞后后的对外值 */
    float  fPkAcc[3];           /* 本周期峰值累加 R/S/T */
    float  fAvg8Acc[10];        /* 8 拍块平均累加，下标见 AVG8_xxx */
    float  fAvg200Acc[6];       /* 200 拍块平均累加，下标见 AVG200_xxx */
    float  fPwrAcc8[3];         /* 有功 8 拍累加 */
    float  fPwrAcc200[3];       /* 有功 200 拍累加 */
    Uint16 u16WinCnt;           /* RMS 窗口样本数 */
    Uint16 u16Avg8Cnt;          /* 8 拍块计数 */
    Uint16 u16Avg200Cnt;        /* 200 拍块计数 */
    Uint16 u16RmsFresh;         /* 1 = 有周期 RMS 待滤波 */
    float  fThetaPrev;          /* 上一拍锁相角，用于找过零点 */
} ST_ADC_SAMPLE_ACC;

/* 实例定义在 AdcSample.c */
extern ST_ADC_SAMPLE_ACC stAdcSampleAcc;

/******************************************************************************
 * 接口
 ******************************************************************************/
/* 初始化：清零全部采样层与内部累加器，放在 CtrlIsrInit 之前 */
extern void AdcSampleInit(void);

/* 20 kHz：在 CtrlIsrRun 末尾调用（此时本拍的 AdcCalib 与 theta 都已更新） */
extern void AdcSampleFast(void);

/* 1 kHz 任务：RMS 一阶滞后 + 状态位，由调度器按 10 个节拍调用 */
extern void AdcSampleTask(void *pArgV);

#ifdef __cplusplus
}
#endif

#endif /* ADC_SAMPLE_H */

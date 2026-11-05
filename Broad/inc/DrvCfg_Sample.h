/**
 * @file DrvCfg_Sample.h
 * @author yetingheng (004182)
 * @brief EP15K 采样标定表：每路量程系数、标称零点、是否上电零漂。
 * @version 0.2
 * @date 2026-11-04
 * 量程数值全部取自既有 EP15K 量程表,具体见15KW软件管脚整理文档。
 * 不在这里重新定义硬件事实：
 *   A0/A1/A3/A5 交流电压 ±462.69 V ↔ 0~3 V，0 V ↔ 1.5 V（2048 码）
 *   A2/D2/D14 交流电流 ±100 A     ↔ 0~3 V，0 A ↔ 1.5 V
 *   B0/B1/B2    直流分量 ±2.95 A  ↔ 0~3 V
 *   A4/C2/D0/D15 PV 电流 30 A ↔ 3 V，0 A ↔ 5/3 V（2275.6 码）
 *   D4          平衡电流 ±36.4 A  ↔ 0~3 V
 *   D1          GFCI ±0.5 A       ↔ 0~3 V
 *   B3 地址 0~3 PV 电压 1033 V ↔ 3 V（单极性，无零点）
 *   B3 地址 4   VISO 488 V ↔ 3 V（单极性，无零点）
 *   D3 地址 6/7 上下半母线 563.64 V ↔ 3 V（单极性，无零点）
 */

#ifndef DRVCFG_SAMPLE_H
#define DRVCFG_SAMPLE_H

#include "AdcCalib.h"

/* 12 位 ADC 码值参考点
 * 码值约定按 TRM SPRUHX5G Table 10-2 / 10-3：
 *   ADCRESULT = 4096 * (Vin - VREFLO) / (VREFHI - VREFLO)，小数截断；
 *   Vin      = (VREFHI - VREFLO) * (ADCRESULT + 0.5) / 4096。
 * 所以满量程是 4096 码、1.5 V 是 2048 码。寄存器在 Vin >= VREFHI 时饱和为 4095，
 * 4095 代表的是 [4095/4096 * 3 V, 3 V] 这一格，不是“3 V 的码值”，所以这里不用 4095
 * 做分母。每格宽度 = 3 V / 4096 = 732.4 uV。 */
#define ADC_CAL_CODE_FULL       (4096.0f)   /* 满量程码值（寄存器饱和码为 4095） */
#define ADC_CAL_CODE_MID        (2048.0f)   /* 1.5 V 对应码值，双极性通道零点 */
#define ADC_CAL_CODE_PV_MID     (2275.56f)  /* 5/3 V 对应码值 = 4096*5/9，PV 电流零点 */

/* 码值 → 工程值系数 */
#define ADC_CAL_K_INV_CUR       (100.0f / ADC_CAL_CODE_MID)     /* A/码 */
#define ADC_CAL_K_AC_VOLT       (462.69f / ADC_CAL_CODE_MID)    /* V/码 */
#define ADC_CAL_K_DC_CUR        (2.95f / ADC_CAL_CODE_MID)      /* A/码 */
#define ADC_CAL_K_BAL_CUR       (36.4f / ADC_CAL_CODE_MID)      /* A/码 */
#define ADC_CAL_K_GFCI          (0.5f / ADC_CAL_CODE_MID)       /* A/码 */
#define ADC_CAL_K_PV_CUR        (30.0f / (ADC_CAL_CODE_FULL - ADC_CAL_CODE_PV_MID))
#define ADC_CAL_K_PV_VOLT       (1033.0f / ADC_CAL_CODE_FULL)   /* V/码 */
#define ADC_CAL_K_VISO_VOLT     (488.0f / ADC_CAL_CODE_FULL)    /* V/码 */
#define ADC_CAL_K_BUS_VOLT      (563.64f / ADC_CAL_CODE_FULL)   /* V/码，上/下半母线 */

/* 频率与窗口 */
#define ADC_CAL_F_NOMINAL       (50.0f)     /* 默认额定频率，Hz */
#define ADC_CAL_F_FREQ_MIN      (45.0f)     /* 允许设置的额定频率下限 */
#define ADC_CAL_F_FREQ_MAX      (65.0f)     /* 允许设置的额定频率上限 */

/* 显示层一级 IIR 系数：1 kHz 上 τ ≈ 200 ms（α = 1 - exp(-1/(τ·fs))） */
#define ADC_CAL_ALPHA_DISPLAY   (0.005f)

extern const ST_SAMPLE_CAL_CFG stSampleCalCfg;

#endif /* DRVCFG_SAMPLE_H */

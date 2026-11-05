/**
 * @file AdcSample.h
 * @author yetingheng (004182)
 * @brief 20 kHz 采样入口：读结果寄存器 → 推进零漂 → 瞬时值 → 块平均 → 周期值。
 * @version 0.2
 * @date 2026-11-04
 *
 * 调用位置：
 *   AdcSampleFast() —— Isr_Control 第一件事（CLA Task1 结束、ADCINT1 之后），20 kHz。
 *   AdcSampleTask() —— Task_Run() 里的 1 kHz 任务，只做显示层强滤波。
 * CLA 侧不再读 ADC，所有结果寄存器由本模块读，因此“哪个寄存器是哪个测点”
 * 只有一处定义（AdcSample.c）。
 */

#ifndef ADC_SAMPLE_H
#define ADC_SAMPLE_H

#include "F2837xS_device.h"

#define ADC_SAMPLE_FAST_HZ      (20000U)    /* ADCA1 触发频率，来自 EPWM1 SOCA */

/* 交流窗口的样本数上下限：正常窗口由角度收口给出（50 Hz 400 拍），
 * 这两个值只用于强制收口上限的钳制与“窗口样本数够不够”的判断。 */
#define ADC_SAMPLE_RMS_WIN_MIN  (10U)
#define ADC_SAMPLE_RMS_WIN_MAX  (600U)

/**
 * @brief 初始化采样层：清零 L1~L5，用标定表的标称零点初始化零漂结构。
 *        必须在 ADC 中断使能之前调用。
 */
void AdcSampleInit(void);

/**
 * @brief 20 kHz 一拍：读 20 个结果寄存器、归档多路选择器、推进零漂、
 *        换算 L2，并累加 L3 块平均与 L4 交流窗口。
 */
void AdcSampleFast(void);

/**
 * @brief 1 kHz 任务：把 L3 / L4 的结果强滤波成 L5 显示值。
 */
void AdcSampleTask(void);

#endif /* ADC_SAMPLE_H */

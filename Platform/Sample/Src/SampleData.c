/**
 * @file SampleData.c
 * @author yetingheng (004182)
 * @brief 五层采样数据实例、频率模式与初始化。
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 *
 * 实例集中在本文件定义，任何模块只能通过 SampleData.h 的 extern 访问，
 * 不在各自模块里另开副本，避免出现两份当前采样值。
 */

#include "SampleData.h"
#include "AdcSample.h"
#include "DrvCfg_Sample.h"
#include "PllData.h"

#include <string.h>

ST_SAMPLE_RAW     stSampleRaw;
ST_SAMPLE_INS     stSampleIns;
ST_SAMPLE_ZERO    stSampleZero;
ST_SAMPLE_FLT     stSampleFlt;
ST_SAMPLE_AC      stSampleAc;
ST_SAMPLE_DISPLAY stSampleDisplay;

volatile ST_SAMPLE_CFG stSampleCfg;

/**
 * @brief 写入额定频率并刷新两个派生长度。
 *
 * 半周期块长度 u16HalfSteps：半块的意义是“把一个工频半周的纹波积掉”，
 * 固定 200 拍只在 50 Hz 成立，60 Hz 下要 167 拍（m00721 裁定按频率自适应）。
 * 交流窗口上限 u16WinMaxSamples：θ 未运行时的安全网，留 25% 余量。
 */
static void SampleCfgApply(float fNominalFreq)
{
    float  fFreq = fNominalFreq;
    float  fSteps;
    Uint16 u16Steps;

    if (fFreq < ADC_CAL_F_FREQ_MIN)
    {
        fFreq = ADC_CAL_F_FREQ_MIN;
    }
    else if (fFreq > ADC_CAL_F_FREQ_MAX)
    {
        fFreq = ADC_CAL_F_FREQ_MAX;
    }
    else
    {
        /* 在范围内，原样使用。 */
    }

    stSampleCfg.fNominalFreq = fFreq;

    /* 半周期块长度 */
    fSteps = ((float)ADC_SAMPLE_FAST_HZ / fFreq) * 0.5f;
    u16Steps = (Uint16)(fSteps + 0.5f);
    if (u16Steps < SAMPLE_AVG8_STEPS)
    {
        u16Steps = SAMPLE_AVG8_STEPS;
    }
    stSampleCfg.u16HalfSteps = u16Steps;

    /* 交流窗口强制收口上限 */
    fSteps = ((float)ADC_SAMPLE_FAST_HZ / fFreq) * 1.25f;
    u16Steps = (Uint16)(fSteps + 0.5f);
    if (u16Steps > ADC_SAMPLE_RMS_WIN_MAX)
    {
        u16Steps = ADC_SAMPLE_RMS_WIN_MAX;
    }
    else if (u16Steps < ADC_SAMPLE_RMS_WIN_MIN)
    {
        u16Steps = ADC_SAMPLE_RMS_WIN_MIN;
    }
    else
    {
        /* 在范围内。 */
    }
    stSampleAc.u16WinMaxSamples = u16Steps;
}

void SampleDataInit(void)
{
    (void)memset(&stSampleRaw, 0, sizeof(stSampleRaw));
    (void)memset(&stSampleIns, 0, sizeof(stSampleIns));
    (void)memset(&stSampleZero, 0, sizeof(stSampleZero));
    (void)memset(&stSampleFlt, 0, sizeof(stSampleFlt));
    (void)memset(&stSampleAc, 0, sizeof(stSampleAc));
    (void)memset(&stSampleDisplay, 0, sizeof(stSampleDisplay));

    /* 采样层唯一一次写角度结构：清零。之后由 PLL 模块负责。 */
    stPLLData.ThetaR = 0.0f;
    stPLLData.ThetaS = 0.0f;
    stPLLData.ThetaT = 0.0f;

    stSampleCfg.fNominalFreq = ADC_CAL_F_NOMINAL;
    stSampleCfg.u16FreqMode  = SAMPLE_FREQ_MODE_50HZ;
    stSampleCfg.u16HalfSteps = 0U;
    SampleCfgApply(ADC_CAL_F_NOMINAL);
}

void SampleSetFreqMode(Uint16 u16Mode)
{
    if (u16Mode == SAMPLE_FREQ_MODE_60HZ)
    {
        stSampleCfg.u16FreqMode = SAMPLE_FREQ_MODE_60HZ;
        SampleCfgApply(SAMPLE_FREQ_60HZ);
    }
    else
    {
        stSampleCfg.u16FreqMode = SAMPLE_FREQ_MODE_50HZ;
        SampleCfgApply(SAMPLE_FREQ_50HZ);
    }
}

Uint16 SampleSetNominalFreq(float fNominalFreq)
{
    Uint16 u16InRange = 1U;

    if ((fNominalFreq < ADC_CAL_F_FREQ_MIN) || (fNominalFreq > ADC_CAL_F_FREQ_MAX))
    {
        u16InRange = 0U;
    }

    SampleCfgApply(fNominalFreq);
    stSampleCfg.u16FreqMode = (stSampleCfg.fNominalFreq > 55.0f)
                              ? SAMPLE_FREQ_MODE_60HZ : SAMPLE_FREQ_MODE_50HZ;

    return u16InRange;
}

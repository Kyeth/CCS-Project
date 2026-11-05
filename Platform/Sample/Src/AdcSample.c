/**
 * @file AdcSample.c
 * @author yetingheng (004182)
 * @brief 20 kHz 采样入口实现（L1 ~ L4）与 1 kHz 显示层（L5）
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 *
 * 结果寄存器编号 = SOC 行号，与通道号无关（见 DrvCfg_EP15K.c 的 SOC 表）：
 *   逆变电流  D14→ADCD.RESULT1  D2→RESULT0  A2→ADCA.RESULT0
 *   逆变电压  C4 →ADCC.RESULT4  C3→RESULT3  A0→RESULT6
 *   电网电压  A5 →ADCA.RESULT5  A1→RESULT7  A3→RESULT3
 *   直流分量  B0 →ADCB.RESULT1  B1→RESULT2  B2→RESULT0
 *   PV 电流   A4 →ADCA.RESULT4  C2→RESULT2  D0→RESULT4  D15→RESULT6
 *   平衡电流  D4 →ADCD.RESULT5
 *   GFCI   D1 →ADCD.RESULT2
 *   多路选择器 B3→ADCB.RESULT3  D3→ADCD.RESULT3
 *
 * 节拍（裁定）：
 *   L1 原始值     AD采样转换结果
 *   L2 瞬时值     20 kHz，一拍一次 —— 控制环可直接用
 *   L3 Avg8       8 拍 = 400 us          —— 控制环“当前值”
 *   L3 AvgHalf    半周期，50 Hz 200 拍   —— 有功、直流分量
 *   L3 AvgMux     16 轮 ≈ 5.6 ms         —— 母线 / PV / VISO 电压，监测用
 *   L4 RMS/峰值   一个工频周期，由 stPLLData.ThetaR 回绕收口
 *   L5 Display    1 kHz 一级强滤波
 */

#include "AdcSample.h"
#include "AdcCalib.h"
#include "SampleData.h"
#include "DrvCfg_Sample.h"
#include "PllData.h"
#include "Hw_ClaTask.h"

#include <math.h>

#define ADC_SAMPLE_THETA_MAX        (6.7831853f)    /* 2π + 0.5 */
#define ADC_SAMPLE_THETA_WRAP_HI    (5.0f)
#define ADC_SAMPLE_THETA_WRAP_LO    (1.0f)

/* 上一拍角度，用于判回绕。只在 20 kHz 路径里读写。 */
static float s_fThetaPrev;

static void AdcSamplePublishMux(void);

static void AdcSampleRead(void)
{
    ST_SAMPLE_RAW *pRaw = &stSampleRaw;

    /* 逆变电流：R / S / T */
    pRaw->u16InvCurrCode[0] = AdcdResultRegs.ADCRESULT1;
    pRaw->u16InvCurrCode[1] = AdcdResultRegs.ADCRESULT0;
    pRaw->u16InvCurrCode[2] = AdcaResultRegs.ADCRESULT0;

    /* 逆变电压：R / S / T */
    pRaw->u16InvVoltCode[0] = AdccResultRegs.ADCRESULT4;
    pRaw->u16InvVoltCode[1] = AdccResultRegs.ADCRESULT3;
    pRaw->u16InvVoltCode[2] = AdcaResultRegs.ADCRESULT6;

    /* 电网电压：R / S / T */
    pRaw->u16GridVoltCode[0] = AdcaResultRegs.ADCRESULT5;
    pRaw->u16GridVoltCode[1] = AdcaResultRegs.ADCRESULT7;
    pRaw->u16GridVoltCode[2] = AdcaResultRegs.ADCRESULT3;

    /* 逆变直流分量：R / S / T */
    pRaw->u16InvDcCode[0] = AdcbResultRegs.ADCRESULT1;
    pRaw->u16InvDcCode[1] = AdcbResultRegs.ADCRESULT2;
    pRaw->u16InvDcCode[2] = AdcbResultRegs.ADCRESULT0;

    /* PV 电流：PV1 / PV2 / PV3 / PV4 */
    pRaw->u16PvCurrCode[0] = AdcaResultRegs.ADCRESULT4;
    pRaw->u16PvCurrCode[1] = AdccResultRegs.ADCRESULT2;
    pRaw->u16PvCurrCode[2] = AdcdResultRegs.ADCRESULT4;
    pRaw->u16PvCurrCode[3] = AdcdResultRegs.ADCRESULT6;

    pRaw->u16BalCurrCode  = AdcdResultRegs.ADCRESULT5;
    pRaw->u16GfciCurrCode = AdcdResultRegs.ADCRESULT2;
}

/**
 * @brief 按本拍地址线值把两个多路选择器的读数放进对应槽位。
 *
 * 地址在上一拍 CPU ISR 里写进 GPIO，本拍 ADC 采样用的就是它；
 * CpuToCla1Msg.u16MuxAddress 与 GPIO 同步更新，所以它就是本拍的地址。
 * 地址 5 或越界值不该出现，出现则丢弃本拍、不写任何槽位。
 */
static void AdcSampleDemuxMux(void)
{
    Uint16 u16Addr = CpuToCla1Msg.u16MuxAddress;
    Uint16 u16CodeB3 = AdcbResultRegs.ADCRESULT3;
    Uint16 u16CodeD3 = AdcdResultRegs.ADCRESULT3;

    stSampleRaw.u16MuxAddr = u16Addr;

    if (u16Addr < SAMPLE_PV_NUM)
    {
        stSampleRaw.u16PvVoltCode[u16Addr] = u16CodeB3;    /* 地址 0~3：PV1~PV4 电压 */
    }
    else if (u16Addr == SAMPLE_MUX2_ADDR_VISO)
    {
        stSampleRaw.u16VisoCode = u16CodeB3;               /* 地址 4：VISO */
    }
    else if (u16Addr == SAMPLE_MUX1_ADDR_BUSL)
    {
        stSampleRaw.u16BusLCode = u16CodeD3;               /* 地址 6：下半母线 */
    }
    else if (u16Addr == SAMPLE_MUX1_ADDR_BUSH)
    {
        stSampleRaw.u16BusHCode = u16CodeD3;               /* 地址 7：上半母线 */
    }
    else
    {
        /* 地址 5 或非法值：本拍的两个结果丢弃。 */
    }
}

/* ---------------- L3：块平均 ---------------- */

static void AdcSamplePublish8(void)
{
    const float fInv = 1.0f / (float)SAMPLE_AVG8_STEPS;
    ST_SAMPLE_AVG8_ACC *pAcc = &stSampleFlt.stAcc8;
    Uint16 i;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        stSampleFlt.fInvCurrAvg8[i]  = pAcc->fInvCurr[i]  * fInv;
        stSampleFlt.fInvVoltAvg8[i]  = pAcc->fInvVolt[i]  * fInv;
        stSampleFlt.fGridVoltAvg8[i] = pAcc->fGridVolt[i] * fInv;
        stSampleFlt.fInvPwrAvg8[i]   = pAcc->fInvPwr[i]   * fInv;

        pAcc->fInvCurr[i]  = 0.0f;
        pAcc->fInvVolt[i]  = 0.0f;
        pAcc->fGridVolt[i] = 0.0f;
        pAcc->fInvPwr[i]   = 0.0f;
    }

    stSampleFlt.fInvPwrTotalAvg8 = pAcc->fInvPwrTotal * fInv;
    pAcc->fInvPwrTotal = 0.0f;
}

static void AdcSamplePublishHalf(void)
{
    ST_SAMPLE_HALF_ACC *pAcc = &stSampleFlt.stAccHalf;
    Uint16 u16N = stSampleFlt.u16CntHalf;
    float  fInv;
    Uint16 i;

    if (u16N == 0U)
    {
        return;
    }

    fInv = 1.0f / (float)u16N;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        stSampleFlt.fInvCurrAvgHalf[i]  = pAcc->fInvCurr[i]  * fInv;
        stSampleFlt.fInvVoltAvgHalf[i]  = pAcc->fInvVolt[i]  * fInv;
        stSampleFlt.fGridVoltAvgHalf[i] = pAcc->fGridVolt[i] * fInv;
        stSampleFlt.fInvDcAvgHalf[i]    = pAcc->fInvDc[i]    * fInv;
        stSampleFlt.fInvPwrAvgHalf[i]   = pAcc->fInvPwr[i]   * fInv;

        pAcc->fInvCurr[i]  = 0.0f;
        pAcc->fInvVolt[i]  = 0.0f;
        pAcc->fGridVolt[i] = 0.0f;
        pAcc->fInvDc[i]    = 0.0f;
        pAcc->fInvPwr[i]   = 0.0f;
    }

    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        stSampleFlt.fPvCurrAvgHalf[i] = pAcc->fPvCurr[i] * fInv;
        pAcc->fPvCurr[i] = 0.0f;
    }

    stSampleFlt.fBalCurrAvgHalf  = pAcc->fBalCurr * fInv;
    stSampleFlt.fGfciCurrAvgHalf = pAcc->fGfciCurr * fInv;
    stSampleFlt.fInvPwrTotalAvgHalf = pAcc->fInvPwrTotal * fInv;

    pAcc->fBalCurr  = 0.0f;
    pAcc->fGfciCurr = 0.0f;
    pAcc->fInvPwrTotal = 0.0f;
}

static void AdcSampleMuxAccumulate(void)
{
    Uint16 u16Addr = stSampleRaw.u16MuxAddr;
    Uint16 u16OK = 1U;

    if (u16Addr < SAMPLE_PV_NUM)
    {
        stSampleFlt.stMuxAcc.fPvVolt[u16Addr] += stSampleIns.fPvVolt[u16Addr];
    }
    else if (u16Addr == SAMPLE_MUX2_ADDR_VISO)
    {
        stSampleFlt.stMuxAcc.fVisoVolt += stSampleIns.fVisoVolt;
    }
    else if (u16Addr == SAMPLE_MUX1_ADDR_BUSL)
    {
        stSampleFlt.stMuxAcc.fBusLVolt += stSampleIns.fBusLVolt;
    }
    else if (u16Addr == SAMPLE_MUX1_ADDR_BUSH)
    {
        stSampleFlt.stMuxAcc.fBusHVolt += stSampleIns.fBusHVolt;
    }
    else
    {
        u16OK = 0U;     /* 非法地址，本拍不计入轮次 */
    }

    /* 一轮 = 地址 0→…→7 走完；在最后一个地址 7 之后记数。 */
    if ((u16OK != 0U) && (u16Addr == SAMPLE_MUX1_ADDR_BUSH))
    {
        stSampleFlt.u16CntMux++;
        if (stSampleFlt.u16CntMux >= SAMPLE_AVG_MUX_STEPS)
        {
            AdcSamplePublishMux();
        }
    }
}

static void AdcSamplePublishMux(void)
{
    const float fInv = 1.0f / (float)SAMPLE_AVG_MUX_STEPS;
    ST_SAMPLE_MUX_ACC *pAcc = &stSampleFlt.stMuxAcc;
    Uint16 i;

    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        stSampleFlt.fPvVoltAvgMux[i] = pAcc->fPvVolt[i] * fInv;
        pAcc->fPvVolt[i] = 0.0f;
    }

    stSampleFlt.fVisoVoltAvgMux = pAcc->fVisoVolt * fInv;
    stSampleFlt.fBusLAvgMux     = pAcc->fBusLVolt * fInv;
    stSampleFlt.fBusHAvgMux     = pAcc->fBusHVolt * fInv;

    pAcc->fVisoVolt = 0.0f;
    pAcc->fBusLVolt = 0.0f;
    pAcc->fBusHVolt = 0.0f;
    stSampleFlt.u16CntMux = 0U;
}

static void AdcSampleFltAccumulate(void)
{
    ST_SAMPLE_AVG8_ACC *pAcc8 = &stSampleFlt.stAcc8;
    ST_SAMPLE_HALF_ACC *pAccH = &stSampleFlt.stAccHalf;
    float fPwrR = stSampleIns.fInvVolt[0] * stSampleIns.fInvCurr[0];
    float fPwrS = stSampleIns.fInvVolt[1] * stSampleIns.fInvCurr[1];
    float fPwrT = stSampleIns.fInvVolt[2] * stSampleIns.fInvCurr[2];
    float fPwrSum = fPwrR + fPwrS + fPwrT;
    Uint16 i;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        pAcc8->fInvCurr[i]  += stSampleIns.fInvCurr[i];
        pAcc8->fInvVolt[i]  += stSampleIns.fInvVolt[i];
        pAcc8->fGridVolt[i] += stSampleIns.fGridVolt[i];

        pAccH->fInvCurr[i]  += stSampleIns.fInvCurr[i];
        pAccH->fInvVolt[i]  += stSampleIns.fInvVolt[i];
        pAccH->fGridVolt[i] += stSampleIns.fGridVolt[i];
        pAccH->fInvDc[i]    += stSampleIns.fInvDc[i];
    }

    pAcc8->fInvPwr[0] += fPwrR;
    pAcc8->fInvPwr[1] += fPwrS;
    pAcc8->fInvPwr[2] += fPwrT;
    pAcc8->fInvPwrTotal += fPwrSum;

    pAccH->fInvPwr[0] += fPwrR;
    pAccH->fInvPwr[1] += fPwrS;
    pAccH->fInvPwr[2] += fPwrT;
    pAccH->fInvPwrTotal += fPwrSum;

    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        pAccH->fPvCurr[i] += stSampleIns.fPvCurr[i];
    }

    pAccH->fBalCurr  += stSampleIns.fBalCurr;
    pAccH->fGfciCurr += stSampleIns.fGfciCurr;

    stSampleFlt.u16Cnt8++;
    if (stSampleFlt.u16Cnt8 >= SAMPLE_AVG8_STEPS)
    {
        AdcSamplePublish8();
        stSampleFlt.u16Cnt8 = 0U;
    }

    stSampleFlt.u16CntHalf++;
    if (stSampleFlt.u16CntHalf >= stSampleCfg.u16HalfSteps)
    {
        AdcSamplePublishHalf();
        stSampleFlt.u16CntHalf = 0U;
    }
}

/* ---------------- L4：交流周期值 ---------------- */

static void AdcSampleAcReset(void)
{
    ST_SAMPLE_AC_ACC *pAcc = &stSampleAc.stAcc;
    Uint16 i;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        pAcc->fInvCurrSq[i]  = 0.0f;
        pAcc->fInvVoltSq[i]  = 0.0f;
        pAcc->fGridVoltSq[i] = 0.0f;
        pAcc->fInvCurrPk[i]  = 0.0f;
        pAcc->fInvVoltPk[i]  = 0.0f;
        pAcc->fGridVoltPk[i] = 0.0f;
    }

    stSampleAc.u16WinCnt = 0U;
}

/**
 * @brief 收口当前窗口并发布，然后从零开始下一个窗口。
 * @param u16Flags 追加的有效性位（THETA / FORCED）
 */
static void AdcSampleAcClose(Uint16 u16Flags)
{
    ST_SAMPLE_AC_ACC *pAcc = &stSampleAc.stAcc;
    Uint16 u16N = stSampleAc.u16WinCnt;
    float  fInv;
    Uint16 i;

    stSampleAc.u16WinSamples = u16N;

    if (u16N < ADC_SAMPLE_RMS_WIN_MIN)
    {
        stSampleAc.u16Valid = 0U;      /* 样本太少，本窗口不发布 */
    }
    else
    {
        fInv = 1.0f / (float)u16N;
        for (i = 0U; i < SAMPLE_PH_NUM; i++)
        {
            stSampleAc.fInvCurrRms[i]  = sqrtf(pAcc->fInvCurrSq[i]  * fInv);
            stSampleAc.fInvVoltRms[i]  = sqrtf(pAcc->fInvVoltSq[i]  * fInv);
            stSampleAc.fGridVoltRms[i] = sqrtf(pAcc->fGridVoltSq[i] * fInv);

            stSampleAc.fInvCurrPk[i]  = pAcc->fInvCurrPk[i];
            stSampleAc.fInvVoltPk[i]  = pAcc->fInvVoltPk[i];
            stSampleAc.fGridVoltPk[i] = pAcc->fGridVoltPk[i];
        }

        stSampleAc.u16Valid = (Uint16)(SAMPLE_AC_VALID_RMS | SAMPLE_AC_VALID_PEAK | u16Flags);
    }

    AdcSampleAcReset();
}

static void AdcSampleAcStep(void)
{
    ST_SAMPLE_AC_ACC *pAcc = &stSampleAc.stAcc;
    float  fTheta = stPLLData.ThetaR;
    Uint16 bThetaOK;
    Uint16 bWrap = 0U;
    float  fVal;
    float  fMag;
    Uint16 i;

    bThetaOK = ((fTheta >= 0.0f) && (fTheta <= ADC_SAMPLE_THETA_MAX)) ? 1U : 0U;

    if ((bThetaOK != 0U) &&
        (s_fThetaPrev > ADC_SAMPLE_THETA_WRAP_HI) &&
        (fTheta < ADC_SAMPLE_THETA_WRAP_LO))
    {
        bWrap = 1U;     /* 角度越过 2π，一个工频周期结束 */
    }

    if (stSampleAc.u16WinCnt >= stSampleAc.u16WinMaxSamples)
    {
        /* θ 不动或有效位异常：靠样本上限收口，值仅供参考。 */
        AdcSampleAcClose(SAMPLE_AC_VALID_FORCED);
    }
    else if (bWrap != 0U)
    {
        AdcSampleAcClose(SAMPLE_AC_VALID_THETA);
    }
    else
    {
        /* 窗口未结束。 */
    }

    /* 本拍样本计入新窗口 */
    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        fVal = stSampleIns.fInvCurr[i];
        pAcc->fInvCurrSq[i] += fVal * fVal;
        fMag = fabsf(fVal);
        if (fMag > pAcc->fInvCurrPk[i])
        {
            pAcc->fInvCurrPk[i] = fMag;
        }

        fVal = stSampleIns.fInvVolt[i];
        pAcc->fInvVoltSq[i] += fVal * fVal;
        fMag = fabsf(fVal);
        if (fMag > pAcc->fInvVoltPk[i])
        {
            pAcc->fInvVoltPk[i] = fMag;
        }

        fVal = stSampleIns.fGridVolt[i];
        pAcc->fGridVoltSq[i] += fVal * fVal;
        fMag = fabsf(fVal);
        if (fMag > pAcc->fGridVoltPk[i])
        {
            pAcc->fGridVoltPk[i] = fMag;
        }
    }

    stSampleAc.u16WinCnt++;
    s_fThetaPrev = (bThetaOK != 0U) ? fTheta : 0.0f;
}

/* ---------------- 1 kHz 显示层 ---------------- */

#define SAMPLE_DISPLAY_IIR(pDst, fSrc)     \
    do {                                   \
        (pDst) += (fAlphaDisplay) * ((fSrc) - (pDst)); \
    } while (0)

void AdcSampleTask(void)
{
    static const float fAlphaDisplay = ADC_CAL_ALPHA_DISPLAY;
    ST_SAMPLE_DISPLAY *pDisp = &stSampleDisplay;
    Uint16 i;
    Uint16 bFirst = pDisp->bFirst;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        if (bFirst == 0U)
        {
            pDisp->fInvCurrRmsDisplay[i]  = stSampleAc.fInvCurrRms[i];
            pDisp->fInvVoltRmsDisplay[i]  = stSampleAc.fInvVoltRms[i];
            pDisp->fGridVoltRmsDisplay[i] = stSampleAc.fGridVoltRms[i];
        }
        else
        {
            SAMPLE_DISPLAY_IIR(pDisp->fInvCurrRmsDisplay[i],  stSampleAc.fInvCurrRms[i]);
            SAMPLE_DISPLAY_IIR(pDisp->fInvVoltRmsDisplay[i],  stSampleAc.fInvVoltRms[i]);
            SAMPLE_DISPLAY_IIR(pDisp->fGridVoltRmsDisplay[i], stSampleAc.fGridVoltRms[i]);
        }
    }

    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        if (bFirst == 0U)
        {
            pDisp->fPvVoltDisplay[i] = stSampleFlt.fPvVoltAvgMux[i];
            pDisp->fPvCurrDisplay[i] = stSampleFlt.fPvCurrAvgHalf[i];
        }
        else
        {
            SAMPLE_DISPLAY_IIR(pDisp->fPvVoltDisplay[i], stSampleFlt.fPvVoltAvgMux[i]);
            SAMPLE_DISPLAY_IIR(pDisp->fPvCurrDisplay[i], stSampleFlt.fPvCurrAvgHalf[i]);
        }
    }

    if (bFirst == 0U)
    {
        pDisp->fVisoVoltDisplay = stSampleFlt.fVisoVoltAvgMux;
        pDisp->fBusLDisplay     = stSampleFlt.fBusLAvgMux;
        pDisp->fBusHDisplay     = stSampleFlt.fBusHAvgMux;
        pDisp->fBalCurrDisplay  = stSampleFlt.fBalCurrAvgHalf;
        pDisp->fGfciCurrDisplay = stSampleFlt.fGfciCurrAvgHalf;
        pDisp->bFirst = 1U;
    }
    else
    {
        SAMPLE_DISPLAY_IIR(pDisp->fVisoVoltDisplay, stSampleFlt.fVisoVoltAvgMux);
        SAMPLE_DISPLAY_IIR(pDisp->fBusLDisplay,     stSampleFlt.fBusLAvgMux);
        SAMPLE_DISPLAY_IIR(pDisp->fBusHDisplay,     stSampleFlt.fBusHAvgMux);
        SAMPLE_DISPLAY_IIR(pDisp->fBalCurrDisplay,  stSampleFlt.fBalCurrAvgHalf);
        SAMPLE_DISPLAY_IIR(pDisp->fGfciCurrDisplay, stSampleFlt.fGfciCurrAvgHalf);
    }

    pDisp->u16Cnt++;
}

/* ---------------- 入口 ---------------- */

void AdcSampleInit(void)
{
    SampleDataInit();
    AdcCalibInit();
    s_fThetaPrev = 0.0f;
}

void AdcSampleFast(void)
{
    AdcSampleRead();
    AdcSampleDemuxMux();
    stSampleRaw.u32Seq++;

    (void)AdcCalibStep();

    if (AdcCalibIsReady() != 0U)
    {
        AdcCalibConvert();
        AdcSampleFltAccumulate();
        AdcSampleMuxAccumulate();
        AdcSampleAcStep();
    }
}

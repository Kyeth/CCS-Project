/**
 * @file AdcSample.c
 * @brief 采样统计层：瞬时值（20 kHz）+ 块平均 / 工频 RMS / 峰值 / 有功
 * @author yetingheng (004182)
 * @version 0.2
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 数据流：
 *   ADC -> CLA（20 kHz）-> Cla1ToCpuMsg -> stSampleRaw（码值）
 *        -> AdcCalib（码值转工程值）-> stSampleIns（瞬时值）
 *        -> AdcSampleFast 累加 -> stSampleAvg / stSampleRms
 *        -> AdcSampleTask（1 kHz）一阶滞后 -> 对外可读
 */
#include "AdcSample.h"
#include "AdcCalib.h"
#include "CtrlIsr.h"
#include "SampleData.h"
#include "User_ClaTask.h"

#include <math.h>
#include <string.h>

/* RMS 内部索引：需要做工频周期有效值的交流量 */
enum
{
    RMS_GRID_R = 0,
    RMS_GRID_S,
    RMS_GRID_T,
    RMS_INV_VOLT_R,
    RMS_INV_VOLT_S,
    RMS_INV_VOLT_T,
    RMS_INV_CUR_R,
    RMS_INV_CUR_S,
    RMS_INV_CUR_T,
    RMS_BAL_CUR,
    RMS_GFCI,
    RMS_NUM
};

/* Avg8（400us）块平均内部索引 */
enum
{
    AVG8_BUS_POS = 0,
    AVG8_BUS_NEG,
    AVG8_PV_VOLT1,
    AVG8_PV_VOLT2,
    AVG8_PV_VOLT3,
    AVG8_PV_VOLT4,
    AVG8_PV_CUR1,
    AVG8_PV_CUR2,
    AVG8_PV_CUR3,
    AVG8_PV_CUR4,
    AVG8_NUM
};

/* Avg200（10ms）块平均内部索引 */
enum
{
    AVG200_BUS_POS = 0,
    AVG200_BUS_NEG,
    AVG200_DC_R,
    AVG200_DC_S,
    AVG200_DC_T,
    AVG200_BAL,
    AVG200_NUM
};

static float sRmsAcc[RMS_NUM];          /* 平方和累加 */
static float sRmsRaw[RMS_NUM];          /* 本周期原始 RMS */
static float sRmsPubPtr[RMS_NUM];       /* 一阶滞后后的对外值 */
static float sPkAcc[SAMPLE_PH_NUM];     /* 本周期峰值累加 */

static float sAvg8Acc[AVG8_NUM];
static float sAvg200Acc[AVG200_NUM];
static float sPwrAcc8[SAMPLE_PH_NUM];   /* 有功 8 拍累加 */
static float sPwrAcc200[SAMPLE_PH_NUM]; /* 有功 200 拍累加 */

static Uint16 sWinCnt;                  /* RMS 窗口样本数 */
static Uint16 sAvg8Cnt;
static Uint16 sAvg200Cnt;
static Uint16 sRmsFresh;                /* 有周期 RMS 待滤波 */
static float  sThetaPrev;               /* 上一拍锁相角，用于找过零点 */

/**
 * @brief 初始化统计层，清零全部累加器与实例
 */
void AdcSampleInit(void)
{
    (void)memset(sRmsAcc, 0, sizeof(sRmsAcc));
    (void)memset(sRmsRaw, 0, sizeof(sRmsRaw));
    (void)memset(sRmsPubPtr, 0, sizeof(sRmsPubPtr));
    (void)memset(sPkAcc, 0, sizeof(sPkAcc));
    (void)memset(sAvg8Acc, 0, sizeof(sAvg8Acc));
    (void)memset(sAvg200Acc, 0, sizeof(sAvg200Acc));
    (void)memset(sPwrAcc8, 0, sizeof(sPwrAcc8));
    (void)memset(sPwrAcc200, 0, sizeof(sPwrAcc200));

    sWinCnt    = 0U;
    sAvg8Cnt   = 0U;
    sAvg200Cnt = 0U;
    sRmsFresh  = 0U;
    sThetaPrev = 0.0f;

    SampleDataInit();
}

/**
 * @brief 把 CLA 输出的码值原样搬进 stSampleRaw，供标定调试与点表回读
 */
static void AdcSampleRawSnapshot(void)
{
    stSampleRaw.fInvCurrRaw[SAMPLE_PH_R]   = Cla1ToCpuMsg.fInvCur[CLA_IDX_R];
    stSampleRaw.fInvCurrRaw[SAMPLE_PH_S]   = Cla1ToCpuMsg.fInvCur[CLA_IDX_S];
    stSampleRaw.fInvCurrRaw[SAMPLE_PH_T]   = Cla1ToCpuMsg.fInvCur[CLA_IDX_T];
    stSampleRaw.fInvCurrPkRaw[SAMPLE_PH_R] = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_R];
    stSampleRaw.fInvCurrPkRaw[SAMPLE_PH_S] = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_S];
    stSampleRaw.fInvCurrPkRaw[SAMPLE_PH_T] = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_T];

    stSampleRaw.fInvVoltRaw[SAMPLE_PH_R]   = Cla1ToCpuMsg.fInvVolt[CLA_IDX_R];
    stSampleRaw.fInvVoltRaw[SAMPLE_PH_S]   = Cla1ToCpuMsg.fInvVolt[CLA_IDX_S];
    stSampleRaw.fInvVoltRaw[SAMPLE_PH_T]   = Cla1ToCpuMsg.fInvVolt[CLA_IDX_T];

    stSampleRaw.fGridVoltRaw[SAMPLE_PH_R]  = Cla1ToCpuMsg.fGridVolt[CLA_IDX_R];
    stSampleRaw.fGridVoltRaw[SAMPLE_PH_S]  = Cla1ToCpuMsg.fGridVolt[CLA_IDX_S];
    stSampleRaw.fGridVoltRaw[SAMPLE_PH_T]  = Cla1ToCpuMsg.fGridVolt[CLA_IDX_T];

    stSampleRaw.fInvDcOfsRaw[SAMPLE_PH_R]  = Cla1ToCpuMsg.fDc[CLA_IDX_R];
    stSampleRaw.fInvDcOfsRaw[SAMPLE_PH_S]  = Cla1ToCpuMsg.fDc[CLA_IDX_S];
    stSampleRaw.fInvDcOfsRaw[SAMPLE_PH_T]  = Cla1ToCpuMsg.fDc[CLA_IDX_T];

    stSampleRaw.fBalCurrRaw  = Cla1ToCpuMsg.fBalCur;
    stSampleRaw.fGfciCurrRaw = Cla1ToCpuMsg.fGfciCur;

    stSampleRaw.fPvCurrRaw[SAMPLE_PV1] = Cla1ToCpuMsg.fPvCur[0];
    stSampleRaw.fPvCurrRaw[SAMPLE_PV2] = Cla1ToCpuMsg.fPvCur[1];
    stSampleRaw.fPvCurrRaw[SAMPLE_PV3] = Cla1ToCpuMsg.fPvCur[2];
    stSampleRaw.fPvCurrRaw[SAMPLE_PV4] = Cla1ToCpuMsg.fPvCur[3];

    stSampleRaw.fMuxB3Raw  = Cla1ToCpuMsg.fMuxB3;
    stSampleRaw.fMuxD3Raw  = Cla1ToCpuMsg.fMuxD3;
    stSampleRaw.u16MuxAddr = Cla1ToCpuMsg.u16MuxAddr;
    stSampleRaw.u16Seq     = Cla1ToCpuMsg.u16Seq;
}

/**
 * @brief 20 kHz 采样统计：瞬时值快照 + 各类累加（CtrlIsrRun 末尾调用）
 */
void AdcSampleFast(void)
{
    float fTheta;
    float fPk;
    Uint16 i;

    AdcSampleRawSnapshot();

    /* ---------- 1. 瞬时值快照：一次读 gAdcCalValue，保证同拍一致 ---------- */
    stSampleIns.fGridVoltIns[SAMPLE_PH_R] = gAdcCalValue[ADC_CAL_GRID_VOLT_R];
    stSampleIns.fGridVoltIns[SAMPLE_PH_S] = gAdcCalValue[ADC_CAL_GRID_VOLT_S];
    stSampleIns.fGridVoltIns[SAMPLE_PH_T] = gAdcCalValue[ADC_CAL_GRID_VOLT_T];

    stSampleIns.fInvVoltIns[SAMPLE_PH_R] = gAdcCalValue[ADC_CAL_INV_VOLT_R];
    stSampleIns.fInvVoltIns[SAMPLE_PH_S] = gAdcCalValue[ADC_CAL_INV_VOLT_S];
    stSampleIns.fInvVoltIns[SAMPLE_PH_T] = gAdcCalValue[ADC_CAL_INV_VOLT_T];

    stSampleIns.fInvCurrIns[SAMPLE_PH_R] = gAdcCalValue[ADC_CAL_INV_CUR_R];
    stSampleIns.fInvCurrIns[SAMPLE_PH_S] = gAdcCalValue[ADC_CAL_INV_CUR_S];
    stSampleIns.fInvCurrIns[SAMPLE_PH_T] = gAdcCalValue[ADC_CAL_INV_CUR_T];

    stSampleIns.fInvDcOfsIns[SAMPLE_PH_R] = gAdcCalValue[ADC_CAL_DC_R];
    stSampleIns.fInvDcOfsIns[SAMPLE_PH_S] = gAdcCalValue[ADC_CAL_DC_S];
    stSampleIns.fInvDcOfsIns[SAMPLE_PH_T] = gAdcCalValue[ADC_CAL_DC_T];

    stSampleIns.fBalCurrIns  = gAdcCalValue[ADC_CAL_BAL_CUR];
    stSampleIns.fGfciCurrIns = gAdcCalValue[ADC_CAL_GFCI];

    stSampleIns.fBusPosVoltIns = gAdcCalValue[ADC_CAL_BUS_VOLT_H];
    stSampleIns.fBusNegVoltIns = gAdcCalValue[ADC_CAL_BUS_VOLT_L];
    stSampleIns.fBusVoltIns = stSampleIns.fBusPosVoltIns + stSampleIns.fBusNegVoltIns;

    stSampleIns.fPvCurrIns[SAMPLE_PV1] = gAdcCalValue[ADC_CAL_PV_CUR1];
    stSampleIns.fPvCurrIns[SAMPLE_PV2] = gAdcCalValue[ADC_CAL_PV_CUR2];
    stSampleIns.fPvCurrIns[SAMPLE_PV3] = gAdcCalValue[ADC_CAL_PV_CUR3];
    stSampleIns.fPvCurrIns[SAMPLE_PV4] = gAdcCalValue[ADC_CAL_PV_CUR4];

    stSampleIns.fPvVoltIns[SAMPLE_PV1] = gAdcCalValue[ADC_CAL_PV_VOLT1];
    stSampleIns.fPvVoltIns[SAMPLE_PV2] = gAdcCalValue[ADC_CAL_PV_VOLT2];
    stSampleIns.fPvVoltIns[SAMPLE_PV3] = gAdcCalValue[ADC_CAL_PV_VOLT3];
    stSampleIns.fPvVoltIns[SAMPLE_PV4] = gAdcCalValue[ADC_CAL_PV_VOLT4];

    /* ---------- 2. 交流平方和 + 峰值 ---------- */
    sRmsAcc[RMS_GRID_R] += stSampleIns.fGridVoltIns[SAMPLE_PH_R] * stSampleIns.fGridVoltIns[SAMPLE_PH_R];
    sRmsAcc[RMS_GRID_S] += stSampleIns.fGridVoltIns[SAMPLE_PH_S] * stSampleIns.fGridVoltIns[SAMPLE_PH_S];
    sRmsAcc[RMS_GRID_T] += stSampleIns.fGridVoltIns[SAMPLE_PH_T] * stSampleIns.fGridVoltIns[SAMPLE_PH_T];

    sRmsAcc[RMS_INV_VOLT_R] += stSampleIns.fInvVoltIns[SAMPLE_PH_R] * stSampleIns.fInvVoltIns[SAMPLE_PH_R];
    sRmsAcc[RMS_INV_VOLT_S] += stSampleIns.fInvVoltIns[SAMPLE_PH_S] * stSampleIns.fInvVoltIns[SAMPLE_PH_S];
    sRmsAcc[RMS_INV_VOLT_T] += stSampleIns.fInvVoltIns[SAMPLE_PH_T] * stSampleIns.fInvVoltIns[SAMPLE_PH_T];

    sRmsAcc[RMS_INV_CUR_R] += stSampleIns.fInvCurrIns[SAMPLE_PH_R] * stSampleIns.fInvCurrIns[SAMPLE_PH_R];
    sRmsAcc[RMS_INV_CUR_S] += stSampleIns.fInvCurrIns[SAMPLE_PH_S] * stSampleIns.fInvCurrIns[SAMPLE_PH_S];
    sRmsAcc[RMS_INV_CUR_T] += stSampleIns.fInvCurrIns[SAMPLE_PH_T] * stSampleIns.fInvCurrIns[SAMPLE_PH_T];

    sRmsAcc[RMS_BAL_CUR] += stSampleIns.fBalCurrIns * stSampleIns.fBalCurrIns;
    sRmsAcc[RMS_GFCI]    += stSampleIns.fGfciCurrIns * stSampleIns.fGfciCurrIns;

    for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
    {
        fPk = fabsf(stSampleIns.fInvCurrIns[i]);

        if (fPk > sPkAcc[i])
        {
            sPkAcc[i] = fPk;
        }
    }

    /* ---------- 3. 有功：逆变电压乘逆变电流，8 拍与 200 拍两档 ---------- */
    for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
    {
        fPk = stSampleIns.fInvVoltIns[i] * stSampleIns.fInvCurrIns[i];
        sPwrAcc8[i]   += fPk;
        sPwrAcc200[i] += fPk;
    }

    /* ---------- 4. Avg8 块累加，400us 收口 ---------- */
    sAvg8Acc[AVG8_BUS_POS] += stSampleIns.fBusPosVoltIns;
    sAvg8Acc[AVG8_BUS_NEG] += stSampleIns.fBusNegVoltIns;
    sAvg8Acc[AVG8_PV_VOLT1] += stSampleIns.fPvVoltIns[SAMPLE_PV1];
    sAvg8Acc[AVG8_PV_VOLT2] += stSampleIns.fPvVoltIns[SAMPLE_PV2];
    sAvg8Acc[AVG8_PV_VOLT3] += stSampleIns.fPvVoltIns[SAMPLE_PV3];
    sAvg8Acc[AVG8_PV_VOLT4] += stSampleIns.fPvVoltIns[SAMPLE_PV4];
    sAvg8Acc[AVG8_PV_CUR1] += stSampleIns.fPvCurrIns[SAMPLE_PV1];
    sAvg8Acc[AVG8_PV_CUR2] += stSampleIns.fPvCurrIns[SAMPLE_PV2];
    sAvg8Acc[AVG8_PV_CUR3] += stSampleIns.fPvCurrIns[SAMPLE_PV3];
    sAvg8Acc[AVG8_PV_CUR4] += stSampleIns.fPvCurrIns[SAMPLE_PV4];

    sAvg8Cnt++;
    if (sAvg8Cnt >= (Uint16)SAMPLE_AVG8_DIV)
    {
        sAvg8Cnt = 0U;

        stSampleAvg.fBusPosVoltAvg8 = sAvg8Acc[AVG8_BUS_POS] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fBusNegVoltAvg8 = sAvg8Acc[AVG8_BUS_NEG] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fBusVoltAvg8 = stSampleAvg.fBusPosVoltAvg8 + stSampleAvg.fBusNegVoltAvg8;

        stSampleAvg.fPvVoltAvg8[SAMPLE_PV1] = sAvg8Acc[AVG8_PV_VOLT1] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvVoltAvg8[SAMPLE_PV2] = sAvg8Acc[AVG8_PV_VOLT2] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvVoltAvg8[SAMPLE_PV3] = sAvg8Acc[AVG8_PV_VOLT3] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvVoltAvg8[SAMPLE_PV4] = sAvg8Acc[AVG8_PV_VOLT4] * (1.0f / (float)SAMPLE_AVG8_DIV);

        stSampleAvg.fPvCurrAvg8[SAMPLE_PV1] = sAvg8Acc[AVG8_PV_CUR1] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvCurrAvg8[SAMPLE_PV2] = sAvg8Acc[AVG8_PV_CUR2] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvCurrAvg8[SAMPLE_PV3] = sAvg8Acc[AVG8_PV_CUR3] * (1.0f / (float)SAMPLE_AVG8_DIV);
        stSampleAvg.fPvCurrAvg8[SAMPLE_PV4] = sAvg8Acc[AVG8_PV_CUR4] * (1.0f / (float)SAMPLE_AVG8_DIV);

        for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
        {
            stSampleAvg.fInvPwrAvg8[i] = sPwrAcc8[i] * (1.0f / (float)SAMPLE_AVG8_DIV);
            sPwrAcc8[i] = 0.0f;
        }

        (void)memset(sAvg8Acc, 0, sizeof(sAvg8Acc));
    }

    /* ---------- 5. Avg200 块累加，10ms 收口 ---------- */
    sAvg200Acc[AVG200_BUS_POS] += stSampleIns.fBusPosVoltIns;
    sAvg200Acc[AVG200_BUS_NEG] += stSampleIns.fBusNegVoltIns;
    sAvg200Acc[AVG200_DC_R] += stSampleIns.fInvDcOfsIns[SAMPLE_PH_R];
    sAvg200Acc[AVG200_DC_S] += stSampleIns.fInvDcOfsIns[SAMPLE_PH_S];
    sAvg200Acc[AVG200_DC_T] += stSampleIns.fInvDcOfsIns[SAMPLE_PH_T];
    sAvg200Acc[AVG200_BAL]  += stSampleIns.fBalCurrIns;

    sAvg200Cnt++;
    if (sAvg200Cnt >= (Uint16)SAMPLE_AVG200_DIV)
    {
        sAvg200Cnt = 0U;

        stSampleAvg.fBusPosVoltAvg200 = sAvg200Acc[AVG200_BUS_POS] * (1.0f / (float)SAMPLE_AVG200_DIV);
        stSampleAvg.fBusNegVoltAvg200 = sAvg200Acc[AVG200_BUS_NEG] * (1.0f / (float)SAMPLE_AVG200_DIV);
        stSampleAvg.fBusVoltAvg200 = stSampleAvg.fBusPosVoltAvg200 + stSampleAvg.fBusNegVoltAvg200;

        stSampleAvg.fInvDcOfsAvg200[SAMPLE_PH_R] = sAvg200Acc[AVG200_DC_R] * (1.0f / (float)SAMPLE_AVG200_DIV);
        stSampleAvg.fInvDcOfsAvg200[SAMPLE_PH_S] = sAvg200Acc[AVG200_DC_S] * (1.0f / (float)SAMPLE_AVG200_DIV);
        stSampleAvg.fInvDcOfsAvg200[SAMPLE_PH_T] = sAvg200Acc[AVG200_DC_T] * (1.0f / (float)SAMPLE_AVG200_DIV);

        stSampleAvg.fBalCurrAvg200 = sAvg200Acc[AVG200_BAL] * (1.0f / (float)SAMPLE_AVG200_DIV);

        for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
        {
            stSampleAvg.fInvPwrAvg200[i] = sPwrAcc200[i] * (1.0f / (float)SAMPLE_AVG200_DIV);
            sPwrAcc200[i] = 0.0f;
        }

        stSampleAvg.fInvPwrTotal = stSampleAvg.fInvPwrAvg200[SAMPLE_PH_R]
                                 + stSampleAvg.fInvPwrAvg200[SAMPLE_PH_S]
                                 + stSampleAvg.fInvPwrAvg200[SAMPLE_PH_T];

        (void)memset(sAvg200Acc, 0, sizeof(sAvg200Acc));
    }

    /* ---------- 6. 交流 RMS 窗口收口：锁相角过零 ----------
     * theta 由软件积分产生，离网时按程序自身的 50Hz 自由运行，并网后跟随电网，
     * 两种情况过零点都对得上一个工频周期，不需要额外的过零捕获。 */
    sWinCnt++;
    fTheta = CtrlGetTheta();

    if ((fTheta < sThetaPrev) || (sWinCnt >= (Uint16)ADC_SAMPLE_RMS_WIN_MAX))
    {
        if (sWinCnt >= (Uint16)ADC_SAMPLE_RMS_WIN_MIN)
        {
            for (i = 0U; i < (Uint16)RMS_NUM; i++)
            {
                sRmsRaw[i] = sqrtf(sRmsAcc[i] / (float)sWinCnt);
                sRmsAcc[i] = 0.0f;
            }

            sRmsFresh = 1U;
        }
        else
        {
            for (i = 0U; i < (Uint16)RMS_NUM; i++)
            {
                sRmsAcc[i] = 0.0f;
            }
        }

        /* 峰值逐周期收口：本周期最大值写进结构体后累加器归零。
         * 峰值不参与保护（保护走 RMS + 时间去抖），只供显示与软示波器。 */
        for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
        {
            stSampleRms.fInvCurrPk[i] = sPkAcc[i];
            sPkAcc[i] = 0.0f;
        }

        sWinCnt = 0U;
    }

    sThetaPrev = fTheta;
}

/**
 * @brief 1 kHz 对外刷新：RMS 一阶滞后 + 状态位
 */
void AdcSampleTask(void *pArgV)
{
    Uint16 i;
    float fVal;

    (void)pArgV;

    if (sRmsFresh != 0U)
    {
        sRmsFresh = 0U;

        for (i = 0U; i < (Uint16)RMS_NUM; i++)
        {
            fVal = sRmsPubPtr[i];
            sRmsPubPtr[i] = fVal + (ADC_SAMPLE_ALPHA_RMS * (sRmsRaw[i] - fVal));
        }

        stSampleRms.fGridVoltRms[SAMPLE_PH_R] = sRmsPubPtr[RMS_GRID_R];
        stSampleRms.fGridVoltRms[SAMPLE_PH_S] = sRmsPubPtr[RMS_GRID_S];
        stSampleRms.fGridVoltRms[SAMPLE_PH_T] = sRmsPubPtr[RMS_GRID_T];

        stSampleRms.fInvVoltRms[SAMPLE_PH_R] = sRmsPubPtr[RMS_INV_VOLT_R];
        stSampleRms.fInvVoltRms[SAMPLE_PH_S] = sRmsPubPtr[RMS_INV_VOLT_S];
        stSampleRms.fInvVoltRms[SAMPLE_PH_T] = sRmsPubPtr[RMS_INV_VOLT_T];

        stSampleRms.fInvCurrRms[SAMPLE_PH_R] = sRmsPubPtr[RMS_INV_CUR_R];
        stSampleRms.fInvCurrRms[SAMPLE_PH_S] = sRmsPubPtr[RMS_INV_CUR_S];
        stSampleRms.fInvCurrRms[SAMPLE_PH_T] = sRmsPubPtr[RMS_INV_CUR_T];

        stSampleRms.fBalCurrRms  = sRmsPubPtr[RMS_BAL_CUR];
        stSampleRms.fGfciCurrRms = sRmsPubPtr[RMS_GFCI];

        stSampleStatus.u16Valid |= (Uint16)SAMPLE_VALID_RMS;
    }

    stSampleStatus.fGridFreq = CtrlGetGridFreq();

    if (AdcCalibGetZeroState() == (Uint16)ADC_CAL_ZERO_DONE)
    {
        stSampleStatus.u16Valid |= (Uint16)SAMPLE_VALID_ZERO;
    }
    else
    {
        stSampleStatus.u16Valid &= (Uint16)(~SAMPLE_VALID_ZERO);
    }
}

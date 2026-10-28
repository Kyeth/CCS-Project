/**
 * @file AdcSample.c
 * @brief ADC 采样两层封装实现：瞬时值（20 kHz）+ 对外值（RMS / 慢速滤波，1 kHz）
 * @author yetingheng (004182)
 * @version 0.1
 * @date 2026-10-26
 *
 * @copyright Copyright (c) 2026
 *
 * 数据流：
 *   ADC -> CLA（20 kHz）-> Cla1ToCpuMsg -> AdcCalib（码值转工程值）
 *        -> AdcSampleFast（20 kHz，CtrlIsr 末尾）-> gAdcIns + 累加器
 *        -> AdcSampleTask（1 kHz，调度任务）-> gAdcPub（Modbus / 软示波器 / ARM）
 */
#include "AdcSample.h"
#include "AdcCalib.h"
#include "CtrlIsr.h"
#include "User_ClaTask.h"

#include <math.h>
#include <string.h>

/******************************************************************************
 * 全局实例
 ******************************************************************************/
volatile ST_ADC_SAMPLE_INS gAdcIns;   /* 20kHz 控制中断写，后台状态机读 */
ST_ADC_SAMPLE_PUB gAdcPub;

/******************************************************************************
 * 内部索引：需要做工频周期 RMS 的交流量
 ******************************************************************************/
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

/******************************************************************************
 * 内部索引：需要块平均 + 一阶滞后的直流量
 ******************************************************************************/
enum
{
    SLOW_BUS_L = 0,
    SLOW_BUS_H,
    SLOW_DC_R,
    SLOW_DC_S,
    SLOW_DC_T,
    SLOW_BAL_CUR,
    SLOW_PV_CUR1,
    SLOW_PV_CUR2,
    SLOW_PV_CUR3,
    SLOW_PV_CUR4,
    SLOW_PV_VOLT1,
    SLOW_PV_VOLT2,
    SLOW_PV_VOLT3,
    SLOW_PV_VOLT4,
    SLOW_NUM
};

/******************************************************************************
 * 状态
 ******************************************************************************/
/* 交流 RMS 的平方和 / 本周期的原始 RMS / 峰值 */
static float sRmsAcc[RMS_NUM];
static float sRmsRaw[RMS_NUM];
static float sPkAcc[3];
static float sPkRaw[3];

/* 直流块累加与块平均值 */
static float sSlowAcc[SLOW_NUM];
static float sSlowAvg[SLOW_NUM];

/* 计数与标志 */
static Uint16 sWinCnt;          /* 当前 RMS 窗口内的样本数 */
static Uint16 sSlowCnt;         /* 当前块平均窗口内的样本数 */
static Uint16 sRmsFresh;        /* 有一个周期 RMS 待刷新（Fast 置位，Task 消费） */
static Uint16 sSlowFresh;       /* 有一块平均值待刷新 */
static float  sThetaPrev;       /* 上一拍的锁相角，用于找过零点 */

/* 对外字段的地址表：滤波状态直接存在 gAdcPub 里，省一层镜像 */
static float * const sRmsPubPtr[RMS_NUM] =
{
    &gAdcPub.fGridVoltRmsR,
    &gAdcPub.fGridVoltRmsS,
    &gAdcPub.fGridVoltRmsT,
    &gAdcPub.fInvVoltRmsR,
    &gAdcPub.fInvVoltRmsS,
    &gAdcPub.fInvVoltRmsT,
    &gAdcPub.fInvCurRmsR,
    &gAdcPub.fInvCurRmsS,
    &gAdcPub.fInvCurRmsT,
    &gAdcPub.fBalCurRms,
    &gAdcPub.fGfciRms
};

static float * const sPkPubPtr[3] =
{
    &gAdcPub.fInvCurPkR,
    &gAdcPub.fInvCurPkS,
    &gAdcPub.fInvCurPkT
};

static float * const sSlowPubPtr[SLOW_NUM] =
{
    &gAdcPub.fBusVoltLSlow,
    &gAdcPub.fBusVoltHSlow,
    &gAdcPub.fDcCurSlowR,
    &gAdcPub.fDcCurSlowS,
    &gAdcPub.fDcCurSlowT,
    &gAdcPub.fBalCurSlow,
    &gAdcPub.fPvCur1Slow,
    &gAdcPub.fPvCur2Slow,
    &gAdcPub.fPvCur3Slow,
    &gAdcPub.fPvCur4Slow,
    &gAdcPub.fPvVolt1Slow,
    &gAdcPub.fPvVolt2Slow,
    &gAdcPub.fPvVolt3Slow,
    &gAdcPub.fPvVolt4Slow
};

/* 每路直流量的滞后系数：母线要快（中点平衡），PV / 直流分量可以慢 */
static const float sSlowAlpha[SLOW_NUM] =
{
    ADC_SAMPLE_ALPHA_BUS,       /* BUS_L */
    ADC_SAMPLE_ALPHA_BUS,       /* BUS_H */
    ADC_SAMPLE_ALPHA_SLOW,      /* DC_R */
    ADC_SAMPLE_ALPHA_SLOW,      /* DC_S */
    ADC_SAMPLE_ALPHA_SLOW,      /* DC_T */
    ADC_SAMPLE_ALPHA_SLOW,      /* BAL_CUR */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_CUR1 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_CUR2 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_CUR3 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_CUR4 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_VOLT1 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_VOLT2 */
    ADC_SAMPLE_ALPHA_SLOW,      /* PV_VOLT3 */
    ADC_SAMPLE_ALPHA_SLOW       /* PV_VOLT4 */
};

/******************************************************************************
 * 初始化
 ******************************************************************************/
void AdcSampleInit(void)
{
    (void)memset((void *)&gAdcIns, 0, sizeof(gAdcIns));
    (void)memset(&gAdcPub, 0, sizeof(gAdcPub));
    (void)memset(sRmsAcc, 0, sizeof(sRmsAcc));
    (void)memset(sRmsRaw, 0, sizeof(sRmsRaw));
    (void)memset(sPkAcc, 0, sizeof(sPkAcc));
    (void)memset(sPkRaw, 0, sizeof(sPkRaw));
    (void)memset(sSlowAcc, 0, sizeof(sSlowAcc));
    (void)memset(sSlowAvg, 0, sizeof(sSlowAvg));

    sWinCnt   = 0U;
    sSlowCnt  = 0U;
    sRmsFresh = 0U;
    sSlowFresh = 0U;
    sThetaPrev = 0.0f;
}

/******************************************************************************
 * 20 kHz：瞬时值快照 + 累加（CtrlIsrRun 末尾调用）
 ******************************************************************************/
void AdcSampleFast(void)
{
    float fTheta;
    float fPk;
    Uint16 i;

    /* ---------- 1. 瞬时值快照：一次读 gAdcCalValue，保证同拍一致 ---------- */
    gAdcIns.fGridVoltR = gAdcCalValue[ADC_CAL_GRID_VOLT_R];
    gAdcIns.fGridVoltS = gAdcCalValue[ADC_CAL_GRID_VOLT_S];
    gAdcIns.fGridVoltT = gAdcCalValue[ADC_CAL_GRID_VOLT_T];
    gAdcIns.fInvVoltR  = gAdcCalValue[ADC_CAL_INV_VOLT_R];
    gAdcIns.fInvVoltS  = gAdcCalValue[ADC_CAL_INV_VOLT_S];
    gAdcIns.fInvVoltT  = gAdcCalValue[ADC_CAL_INV_VOLT_T];
    gAdcIns.fInvCurR   = gAdcCalValue[ADC_CAL_INV_CUR_R];
    gAdcIns.fInvCurS   = gAdcCalValue[ADC_CAL_INV_CUR_S];
    gAdcIns.fInvCurT   = gAdcCalValue[ADC_CAL_INV_CUR_T];

    gAdcIns.fDcCurR    = gAdcCalValue[ADC_CAL_DC_R];
    gAdcIns.fDcCurS    = gAdcCalValue[ADC_CAL_DC_S];
    gAdcIns.fDcCurT    = gAdcCalValue[ADC_CAL_DC_T];
    gAdcIns.fBalCur    = gAdcCalValue[ADC_CAL_BAL_CUR];
    gAdcIns.fGfciCur   = gAdcCalValue[ADC_CAL_GFCI];

    gAdcIns.fBusVoltL  = gAdcCalValue[ADC_CAL_BUS_VOLT_L];
    gAdcIns.fBusVoltH  = gAdcCalValue[ADC_CAL_BUS_VOLT_H];
    gAdcIns.fBusVolt   = gAdcIns.fBusVoltL + gAdcIns.fBusVoltH;

    gAdcIns.fPvCur1    = gAdcCalValue[ADC_CAL_PV_CUR1];
    gAdcIns.fPvCur2    = gAdcCalValue[ADC_CAL_PV_CUR2];
    gAdcIns.fPvCur3    = gAdcCalValue[ADC_CAL_PV_CUR3];
    gAdcIns.fPvCur4    = gAdcCalValue[ADC_CAL_PV_CUR4];
    gAdcIns.fPvVolt1   = gAdcCalValue[ADC_CAL_PV_VOLT1];
    gAdcIns.fPvVolt2   = gAdcCalValue[ADC_CAL_PV_VOLT2];
    gAdcIns.fPvVolt3   = gAdcCalValue[ADC_CAL_PV_VOLT3];
    gAdcIns.fPvVolt4   = gAdcCalValue[ADC_CAL_PV_VOLT4];

    /* MUX 上的瞬时电压：具体是哪一路由 u16MuxAddr 解释 */
    gAdcIns.fMuxVoltB3 = Cla1ToCpuMsg.fMuxB3;
    gAdcIns.fMuxVoltD3 = Cla1ToCpuMsg.fMuxD3;

    /* ---------- 2. 交流平方和 + 峰值 ---------- */
    sRmsAcc[RMS_GRID_R] += gAdcIns.fGridVoltR * gAdcIns.fGridVoltR;
    sRmsAcc[RMS_GRID_S] += gAdcIns.fGridVoltS * gAdcIns.fGridVoltS;
    sRmsAcc[RMS_GRID_T] += gAdcIns.fGridVoltT * gAdcIns.fGridVoltT;
    sRmsAcc[RMS_INV_VOLT_R] += gAdcIns.fInvVoltR * gAdcIns.fInvVoltR;
    sRmsAcc[RMS_INV_VOLT_S] += gAdcIns.fInvVoltS * gAdcIns.fInvVoltS;
    sRmsAcc[RMS_INV_VOLT_T] += gAdcIns.fInvVoltT * gAdcIns.fInvVoltT;
    sRmsAcc[RMS_INV_CUR_R] += gAdcIns.fInvCurR * gAdcIns.fInvCurR;
    sRmsAcc[RMS_INV_CUR_S] += gAdcIns.fInvCurS * gAdcIns.fInvCurS;
    sRmsAcc[RMS_INV_CUR_T] += gAdcIns.fInvCurT * gAdcIns.fInvCurT;
    sRmsAcc[RMS_BAL_CUR] += gAdcIns.fBalCur * gAdcIns.fBalCur;
    sRmsAcc[RMS_GFCI] += gAdcIns.fGfciCur * gAdcIns.fGfciCur;

    fPk = fabsf(gAdcIns.fInvCurR);
    if (fPk > sPkAcc[0])
    {
        sPkAcc[0] = fPk;
    }

    fPk = fabsf(gAdcIns.fInvCurS);
    if (fPk > sPkAcc[1])
    {
        sPkAcc[1] = fPk;
    }

    fPk = fabsf(gAdcIns.fInvCurT);
    if (fPk > sPkAcc[2])
    {
        sPkAcc[2] = fPk;
    }

    /* ---------- 3. 直流块累加 ---------- */
    sSlowAcc[SLOW_BUS_L] += gAdcIns.fBusVoltL;
    sSlowAcc[SLOW_BUS_H] += gAdcIns.fBusVoltH;
    sSlowAcc[SLOW_DC_R] += gAdcIns.fDcCurR;
    sSlowAcc[SLOW_DC_S] += gAdcIns.fDcCurS;
    sSlowAcc[SLOW_DC_T] += gAdcIns.fDcCurT;
    sSlowAcc[SLOW_BAL_CUR] += gAdcIns.fBalCur;
    sSlowAcc[SLOW_PV_CUR1] += gAdcIns.fPvCur1;
    sSlowAcc[SLOW_PV_CUR2] += gAdcIns.fPvCur2;
    sSlowAcc[SLOW_PV_CUR3] += gAdcIns.fPvCur3;
    sSlowAcc[SLOW_PV_CUR4] += gAdcIns.fPvCur4;
    sSlowAcc[SLOW_PV_VOLT1] += gAdcIns.fPvVolt1;
    sSlowAcc[SLOW_PV_VOLT2] += gAdcIns.fPvVolt2;
    sSlowAcc[SLOW_PV_VOLT3] += gAdcIns.fPvVolt3;
    sSlowAcc[SLOW_PV_VOLT4] += gAdcIns.fPvVolt4;

    sSlowCnt++;
    if (sSlowCnt >= (Uint16)ADC_SAMPLE_SLOW_DIV)
    {
        sSlowCnt = 0U;

        for (i = 0U; i < (Uint16)SLOW_NUM; i++)
        {
            sSlowAvg[i] = sSlowAcc[i] * (1.0f / (float)ADC_SAMPLE_SLOW_DIV);
            sSlowAcc[i] = 0.0f;
        }

        sSlowFresh = 1U;
    }

    /* ---------- 4. 交流 RMS 窗口收口：锁相角过零 ----------
     * theta 由软件积分产生，离网时按程序自身的 50Hz 自由运行，并网后跟随电网，
     * 两种情况过零点都对得上一个工频周期，所以不需要额外的过零捕获。 */
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

            sPkRaw[0] = sPkAcc[0];
            sPkRaw[1] = sPkAcc[1];
            sPkRaw[2] = sPkAcc[2];

            sRmsFresh = 1U;
        }
        else
        {
            for (i = 0U; i < (Uint16)RMS_NUM; i++)
            {
                sRmsAcc[i] = 0.0f;
            }
        }

        sPkAcc[0] = 0.0f;
        sPkAcc[1] = 0.0f;
        sPkAcc[2] = 0.0f;
        sWinCnt = 0U;
    }

    sThetaPrev = fTheta;
}

/******************************************************************************
 * 1 kHz：对外值刷新（块平均 + 一阶滞后 + 峰值保持）
 ******************************************************************************/
void AdcSampleTask(void *pArgV)
{
    Uint16 i;
    float fVal;

    (void)pArgV;

    /* ---------- 1. 交流：周期 RMS 的一阶滞后 + 峰值保持 ---------- */
    if (sRmsFresh != 0U)
    {
        sRmsFresh = 0U;

        for (i = 0U; i < (Uint16)RMS_NUM; i++)
        {
            fVal = *sRmsPubPtr[i];
            *sRmsPubPtr[i] = fVal + (ADC_SAMPLE_ALPHA_RMS * (sRmsRaw[i] - fVal));
        }

        for (i = 0U; i < 3U; i++)
        {
            /* 上一周期的保持值先按系数衰减，再被本周期最大值顶上去 */
            fVal = (*sPkPubPtr[i]) * ADC_SAMPLE_PK_DECAY;

            if (sPkRaw[i] > fVal)
            {
                fVal = sPkRaw[i];
            }

            *sPkPubPtr[i] = fVal;
        }

        gAdcPub.u16Valid |= (Uint16)ADC_SAMPLE_VALID_RMS;
    }

    /* ---------- 2. 直流：块平均 + 一阶滞后 ---------- */
    if (sSlowFresh != 0U)
    {
        sSlowFresh = 0U;

        for (i = 0U; i < (Uint16)SLOW_NUM; i++)
        {
            fVal = *sSlowPubPtr[i];
            *sSlowPubPtr[i] = fVal + (sSlowAlpha[i] * (sSlowAvg[i] - fVal));
        }
    }

    /* ---------- 3. 派生量 / 状态位 ---------- */
    gAdcPub.fBusVoltSlow = gAdcPub.fBusVoltLSlow + gAdcPub.fBusVoltHSlow;
    gAdcPub.fGridFreq = CtrlGetGridFreq();

    if (AdcCalibGetZeroState() == (Uint16)ADC_CAL_ZERO_DONE)
    {
        gAdcPub.u16Valid |= (Uint16)ADC_SAMPLE_VALID_ZERO;
    }
    else
    {
        gAdcPub.u16Valid &= (Uint16)(~ADC_SAMPLE_VALID_ZERO);
    }
}

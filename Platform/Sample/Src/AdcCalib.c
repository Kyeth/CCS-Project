/**
 * @file AdcCalib.c
 * @author yetingheng (004182)
 * @brief 上电零漂状态机与码值换算
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "AdcCalib.h"
#include "DrvCfg_Sample.h"

/* 零漂累加器：18 路带偏置通道。4000 × 4095 = 1.638e7，Uint32 足够且无截断。 */
typedef struct
{
    Uint32 au32InvCurr[SAMPLE_PH_NUM];
    Uint32 au32InvVolt[SAMPLE_PH_NUM];
    Uint32 au32GridVolt[SAMPLE_PH_NUM];
    Uint32 au32InvDc[SAMPLE_PH_NUM];
    Uint32 au32PvCurr[SAMPLE_PV_NUM];
    Uint32 au32BalCurr;
    Uint32 au32GfciCurr;
} ST_ADC_CAL_ACC;

static ST_ADC_CAL_ACC s_stAcc;
static Uint16  s_u16Tick;

static void AdcCalibResetAcc(void)
{
    Uint16 i;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        s_stAcc.au32InvCurr[i]  = 0UL;
        s_stAcc.au32InvVolt[i]  = 0UL;
        s_stAcc.au32GridVolt[i] = 0UL;
        s_stAcc.au32InvDc[i]    = 0UL;
    }
    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        s_stAcc.au32PvCurr[i] = 0UL;
    }
    s_stAcc.au32BalCurr  = 0UL;
    s_stAcc.au32GfciCurr = 0UL;
}

static void AdcCalibAccumulate(void)
{
    Uint16 i;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        s_stAcc.au32InvCurr[i]  += (Uint32)stSampleRaw.u16InvCurrCode[i];
        s_stAcc.au32InvVolt[i]  += (Uint32)stSampleRaw.u16InvVoltCode[i];
        s_stAcc.au32GridVolt[i] += (Uint32)stSampleRaw.u16GridVoltCode[i];
        s_stAcc.au32InvDc[i]    += (Uint32)stSampleRaw.u16InvDcCode[i];
    }
    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        s_stAcc.au32PvCurr[i] += (Uint32)stSampleRaw.u16PvCurrCode[i];
    }
    s_stAcc.au32BalCurr  += (Uint32)stSampleRaw.u16BalCurrCode;
    s_stAcc.au32GfciCurr += (Uint32)stSampleRaw.u16GfciCurrCode;
}

/**
 * @brief 把累加值折算成零点。超差则该通道保留标称值并返回 1。
 */
static Uint16 AdcCalibTakeZero(Uint32 u32Sum, float fZeroNom, float *pZero)
{
    float fAvg = (float)u32Sum / (float)ADC_CAL_TRACK_TICKS;

    if ((fAvg > (fZeroNom + ADC_CAL_ZERO_TOL)) || (fAvg < (fZeroNom - ADC_CAL_ZERO_TOL)))
    {
        *pZero = fZeroNom;
        return 1U;
    }

    *pZero = fAvg;
    return 0U;
}

static void AdcCalibFinish(void)
{
    Uint16 u16Fault = 0U;
    Uint16 i;
    const ST_SAMPLE_CAL_CFG *pCfg = &stSampleCalCfg;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        u16Fault |= AdcCalibTakeZero(s_stAcc.au32InvCurr[i], pCfg->aInvCurr[i].fZeroNom,
                                     &stSampleZero.fInvCurr[i]);
        u16Fault |= AdcCalibTakeZero(s_stAcc.au32InvVolt[i], pCfg->aInvVolt[i].fZeroNom,
                                     &stSampleZero.fInvVolt[i]);
        u16Fault |= AdcCalibTakeZero(s_stAcc.au32GridVolt[i], pCfg->aGridVolt[i].fZeroNom,
                                     &stSampleZero.fGridVolt[i]);
        u16Fault |= AdcCalibTakeZero(s_stAcc.au32InvDc[i], pCfg->aInvDc[i].fZeroNom,
                                     &stSampleZero.fInvDc[i]);
    }
    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        u16Fault |= AdcCalibTakeZero(s_stAcc.au32PvCurr[i], pCfg->aPvCurr[i].fZeroNom,
                                     &stSampleZero.fPvCurr[i]);
    }
    u16Fault |= AdcCalibTakeZero(s_stAcc.au32BalCurr, pCfg->aBalCurr.fZeroNom,
                                 &stSampleZero.fBalCurr);
    u16Fault |= AdcCalibTakeZero(s_stAcc.au32GfciCurr, pCfg->aGfciCurr.fZeroNom,
                                 &stSampleZero.fGfciCurr);

    /* 多路选择器不做零漂，零点恒为标称值（工程值为 0）。 */
    stSampleZero.u16Valid = (u16Fault == 0U) ? 1U : 0U;
    stSampleZero.u16State = (u16Fault == 0U) ? ADC_CAL_STATE_READY : ADC_CAL_STATE_FAULT;
    s_u16Tick = 0U;
}

void AdcCalibInit(void)
{
    Uint16 i;
    const ST_SAMPLE_CAL_CFG *pCfg = &stSampleCalCfg;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        stSampleZero.fInvCurr[i]  = pCfg->aInvCurr[i].fZeroNom;
        stSampleZero.fInvVolt[i]  = pCfg->aInvVolt[i].fZeroNom;
        stSampleZero.fGridVolt[i] = pCfg->aGridVolt[i].fZeroNom;
        stSampleZero.fInvDc[i]    = pCfg->aInvDc[i].fZeroNom;
    }
    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        stSampleZero.fPvCurr[i] = pCfg->aPvCurr[i].fZeroNom;
        stSampleZero.fPvVolt[i] = pCfg->aPvVolt[i].fZeroNom;
    }
    stSampleZero.fBalCurr  = pCfg->aBalCurr.fZeroNom;
    stSampleZero.fGfciCurr = pCfg->aGfciCurr.fZeroNom;
    stSampleZero.fVisoVolt = pCfg->aVisoVolt.fZeroNom;
    stSampleZero.fBusLVolt = pCfg->aBusLVolt.fZeroNom;
    stSampleZero.fBusHVolt = pCfg->aBusHVolt.fZeroNom;

    stSampleZero.u16State = ADC_CAL_STATE_IDLE;
    stSampleZero.u16Valid = 0U;

    AdcCalibResetAcc();
    s_u16Tick = 0U;
}

Uint16 AdcCalibStep(void)
{
    switch (stSampleZero.u16State)
    {
    case ADC_CAL_STATE_IDLE:
        AdcCalibResetAcc();
        s_u16Tick = 0U;
        stSampleZero.u16State = ADC_CAL_STATE_SKIP;
        break;

    case ADC_CAL_STATE_SKIP:
        if (s_u16Tick < ADC_CAL_SKIP_TICKS)
        {
            s_u16Tick++;
        }
        else
        {
            AdcCalibResetAcc();
            s_u16Tick = 0U;
            stSampleZero.u16State = ADC_CAL_STATE_TRACK;
        }
        break;

    case ADC_CAL_STATE_TRACK:
        AdcCalibAccumulate();
        if (s_u16Tick < ADC_CAL_TRACK_TICKS)
        {
            s_u16Tick++;
        }
        else
        {
            AdcCalibFinish();
        }
        break;

    default:
        /* READY / FAULT：状态机停在这里，不自动重试。 */
        break;
    }

    return stSampleZero.u16State;
}

Uint16 AdcCalibIsReady(void)
{
    return (stSampleZero.u16State == ADC_CAL_STATE_READY) ? 1U : 0U;
}

Uint16 AdcCalibGetState(void)
{
    return stSampleZero.u16State;
}

void AdcCalibConvert(void)
{
    Uint16 i;
    const ST_SAMPLE_CAL_CFG *pCfg = &stSampleCalCfg;

    for (i = 0U; i < SAMPLE_PH_NUM; i++)
    {
        stSampleIns.fInvCurr[i]  = ((float)stSampleRaw.u16InvCurrCode[i]  - stSampleZero.fInvCurr[i])
                                   * pCfg->aInvCurr[i].fK;
        stSampleIns.fInvVolt[i]  = ((float)stSampleRaw.u16InvVoltCode[i]  - stSampleZero.fInvVolt[i])
                                   * pCfg->aInvVolt[i].fK;
        stSampleIns.fGridVolt[i] = ((float)stSampleRaw.u16GridVoltCode[i] - stSampleZero.fGridVolt[i])
                                   * pCfg->aGridVolt[i].fK;
        stSampleIns.fInvDc[i]    = ((float)stSampleRaw.u16InvDcCode[i]    - stSampleZero.fInvDc[i])
                                   * pCfg->aInvDc[i].fK;
    }

    for (i = 0U; i < SAMPLE_PV_NUM; i++)
    {
        stSampleIns.fPvCurr[i] = ((float)stSampleRaw.u16PvCurrCode[i] - stSampleZero.fPvCurr[i])
                                 * pCfg->aPvCurr[i].fK;
        stSampleIns.fPvVolt[i] = ((float)stSampleRaw.u16PvVoltCode[i] - stSampleZero.fPvVolt[i])
                                 * pCfg->aPvVolt[i].fK;
    }

    stSampleIns.fBalCurr  = ((float)stSampleRaw.u16BalCurrCode  - stSampleZero.fBalCurr)
                            * pCfg->aBalCurr.fK;
    stSampleIns.fGfciCurr = ((float)stSampleRaw.u16GfciCurrCode - stSampleZero.fGfciCurr)
                            * pCfg->aGfciCurr.fK;
    stSampleIns.fVisoVolt = ((float)stSampleRaw.u16VisoCode - stSampleZero.fVisoVolt)
                            * pCfg->aVisoVolt.fK;
    stSampleIns.fBusLVolt = ((float)stSampleRaw.u16BusLCode - stSampleZero.fBusLVolt)
                            * pCfg->aBusLVolt.fK;
    stSampleIns.fBusHVolt = ((float)stSampleRaw.u16BusHCode - stSampleZero.fBusHVolt)
                            * pCfg->aBusHVolt.fK;
}

/******************************************************************************
 * @file    AdcCalib.c
 * @author  yetingheng (004182)
 * @brief   ADC 采样标定层实现：码值 -> 工程值 + 上电零漂检测
 * @version 1.0
 * @date    2026-11-05
 *
 * @note    设计要点：
 *          1. 系数/零点全部走表，替换硬件版本时只改 AdcCalib.c 顶部三张表
 *             与 AdcCalib.h 的系数宏，控制环代码不动；
 *          2. 换算公式统一为 value = (code12 - zero) * scale，
 *             其中 scale 单位是"工程值/码值"，直接乘即可；
 *          3. 上电零漂检测：功率级未发波时各通道电流应为 0、电压应为偏置值，
 *             用 200ms 平均得到真实零点；超差（>±15%）保留标称值并报 FAILED，
 *             避免明显的坏零点灌进控制环；
 *          4. GFCI 默认不参与零漂检测，理由见 ADC_CAL_GFCI_TRACK_EN 注释。
 *
 * @note    与 EP5K 的对应关系：
 *          EP5K《Sample.c》用 int16 系数 K，换算 (K*raw>>12)*0.1；
 *          本工程用 float 系数，换算 (raw-zero)*scale，两者数学等价，
 *          但本工程把零点与比例解耦，现场标定时可以分别修。
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#include "AdcCalib.h"
#include "User_ClaTask.h"      /* Cla1ToCpuMsg：CLA 侧发布的滤波后码值 */

/******************************************************************************
 * 三段常量表：标称零点 / 标称系数 / 零漂检测掩码
 *   下标顺序必须与 AdcCalib.h 的 E_ADC_CAL_CH 一致
 ******************************************************************************/

/* 标称零点（12 位码值）：双极性 1.5V 偏置 = 2047.5，PV 电流 5/3V 偏置 = 2275，
 * 单极性电压（PV 电压、母线电压）零点为 0 */
static const float sNominalZero[ADC_CAL_CH_NUM] =
{
    /* 逆变电流 R/S/T */   ADC_CAL_CODE_MID, ADC_CAL_CODE_MID, ADC_CAL_CODE_MID,
    /* 逆变电流峰值 R/S/T*/ ADC_CAL_CODE_MID, ADC_CAL_CODE_MID, ADC_CAL_CODE_MID,
    /* 逆变电压 R/S/T */   ADC_CAL_CODE_MID, ADC_CAL_CODE_MID, ADC_CAL_CODE_MID,
    /* 电网电压 R/S/T */   ADC_CAL_CODE_MID, ADC_CAL_CODE_MID, ADC_CAL_CODE_MID,
    /* 相直流分量 R/S/T */ ADC_CAL_CODE_MID, ADC_CAL_CODE_MID, ADC_CAL_CODE_MID,
    /* 平衡电路电流 */      ADC_CAL_CODE_MID,
    /* GFCI 漏电流 */       ADC_CAL_CODE_MID,
    /* PV1~PV4 电流 */      ADC_CAL_CODE_PV_MID, ADC_CAL_CODE_PV_MID,
                            ADC_CAL_CODE_PV_MID, ADC_CAL_CODE_PV_MID,
    /* PV1~PV4 电压 */      0.0f, 0.0f, 0.0f, 0.0f,
    /* 上/下半母线电压 */     0.0f, 0.0f
};

/* 标称系数（工程值 / 码值） */
static const float sNominalScale[ADC_CAL_CH_NUM] =
{
    /* 逆变电流 R/S/T */   ADC_CAL_K_INV_CUR, ADC_CAL_K_INV_CUR, ADC_CAL_K_INV_CUR,
    /* 逆变电流峰值 R/S/T*/ ADC_CAL_K_INV_CUR, ADC_CAL_K_INV_CUR, ADC_CAL_K_INV_CUR,
    /* 逆变电压 R/S/T */   ADC_CAL_K_AC_VOLT, ADC_CAL_K_AC_VOLT, ADC_CAL_K_AC_VOLT,
    /* 电网电压 R/S/T */   ADC_CAL_K_AC_VOLT, ADC_CAL_K_AC_VOLT, ADC_CAL_K_AC_VOLT,
    /* 相直流分量 R/S/T */ ADC_CAL_K_DC_CUR,  ADC_CAL_K_DC_CUR,  ADC_CAL_K_DC_CUR,
    /* 平衡电路电流 */      ADC_CAL_K_BAL_CUR,
    /* GFCI 漏电流 */       ADC_CAL_K_GFCI,
    /* PV1~PV4 电流 */      ADC_CAL_K_PV_CUR, ADC_CAL_K_PV_CUR,
                            ADC_CAL_K_PV_CUR, ADC_CAL_K_PV_CUR,
    /* PV1~PV4 电压 */      ADC_CAL_K_PV_VOLT, ADC_CAL_K_PV_VOLT,
                            ADC_CAL_K_PV_VOLT, ADC_CAL_K_PV_VOLT,
    /* 上/下半母线电压 */     ADC_CAL_K_BUS_VOLT, ADC_CAL_K_BUS_VOLT
};

/* 零漂检测掩码：1 = 上电时做平均值检测，0 = 固定用标称零点
 *   不检测的通道：
 *     - PV 电压、母线电压：单极性，零点理论为 0；且上电时机上可能已有
 *       PV 电压 / 母线电压，检测会把真实电压当成零点
 *     - GFCI：见 ADC_CAL_GFCI_TRACK_EN 说明 */
static const Uint16 sTrackMask[ADC_CAL_CH_NUM] =
{
    /* 逆变电流 R/S/T */   1U, 1U, 1U,
    /* 逆变电流峰值 R/S/T*/ 1U, 1U, 1U,
    /* 逆变电压 R/S/T */   1U, 1U, 1U,
    /* 电网电压 R/S/T */   1U, 1U, 1U,
    /* 相直流分量 R/S/T */ 1U, 1U, 1U,
    /* 平衡电路电流 */      1U,
    /* GFCI 漏电流 */       ADC_CAL_GFCI_TRACK_EN,
    /* PV1~PV4 电流 */      1U, 1U, 1U, 1U,
    /* PV1~PV4 电压 */      0U, 0U, 0U, 0U,
    /* 上/下半母线电压 */     0U, 0U
};

/******************************************************************************
 * 运行时状态
 ******************************************************************************/
/* 采样标定全部运行状态：单个实例 stAdcCalib，函数内一律经结构体访问 */
ST_ADC_CALIB_STATE stAdcCalib;

/******************************************************************************
 * @brief  取某通道的本拍码值（12 位域）
 * @note   逆变电流取 CLA 已做 ZERO/PRD 两点平均的结果，抑制开关纹波；
 *         其余通道取 CLA 滤波后的值
 ******************************************************************************/
static float AdcCalibReadCode12(Uint16 u16Ch)
{
    float fCode = 0.0f;

    switch (u16Ch)
    {
        case ADC_CAL_INV_CUR_R:     fCode = Cla1ToCpuMsg.fInvCur[CLA_IDX_R];      break;
        case ADC_CAL_INV_CUR_S:     fCode = Cla1ToCpuMsg.fInvCur[CLA_IDX_S];      break;
        case ADC_CAL_INV_CUR_T:     fCode = Cla1ToCpuMsg.fInvCur[CLA_IDX_T];      break;
        case ADC_CAL_INV_CUR_PK_R:  fCode = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_R];  break;
        case ADC_CAL_INV_CUR_PK_S:  fCode = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_S];  break;
        case ADC_CAL_INV_CUR_PK_T:  fCode = Cla1ToCpuMsg.fInvCurPeak[CLA_IDX_T];  break;

        case ADC_CAL_INV_VOLT_R:    fCode = Cla1ToCpuMsg.fInvVolt[CLA_IDX_R];     break;
        case ADC_CAL_INV_VOLT_S:    fCode = Cla1ToCpuMsg.fInvVolt[CLA_IDX_S];     break;
        case ADC_CAL_INV_VOLT_T:    fCode = Cla1ToCpuMsg.fInvVolt[CLA_IDX_T];     break;

        case ADC_CAL_GRID_VOLT_R:   fCode = Cla1ToCpuMsg.fGridVolt[CLA_IDX_R];    break;
        case ADC_CAL_GRID_VOLT_S:   fCode = Cla1ToCpuMsg.fGridVolt[CLA_IDX_S];    break;
        case ADC_CAL_GRID_VOLT_T:   fCode = Cla1ToCpuMsg.fGridVolt[CLA_IDX_T];    break;

        case ADC_CAL_DC_R:          fCode = Cla1ToCpuMsg.fDc[CLA_IDX_R];          break;
        case ADC_CAL_DC_S:          fCode = Cla1ToCpuMsg.fDc[CLA_IDX_S];          break;
        case ADC_CAL_DC_T:          fCode = Cla1ToCpuMsg.fDc[CLA_IDX_T];          break;
        case ADC_CAL_BAL_CUR:       fCode = Cla1ToCpuMsg.fBalCur;                 break;
        case ADC_CAL_GFCI:          fCode = Cla1ToCpuMsg.fGfciCur;                break;

        case ADC_CAL_PV_CUR1:       fCode = Cla1ToCpuMsg.fPvCur[CLA_IDX_PV1];     break;
        case ADC_CAL_PV_CUR2:       fCode = Cla1ToCpuMsg.fPvCur[CLA_IDX_PV2];     break;
        case ADC_CAL_PV_CUR3:       fCode = Cla1ToCpuMsg.fPvCur[CLA_IDX_PV3];     break;
        case ADC_CAL_PV_CUR4:       fCode = Cla1ToCpuMsg.fPvCur[CLA_IDX_PV4];     break;

        case ADC_CAL_PV_VOLT1:      fCode = Cla1ToCpuMsg.fPvVolt[CLA_IDX_PV1];    break;
        case ADC_CAL_PV_VOLT2:      fCode = Cla1ToCpuMsg.fPvVolt[CLA_IDX_PV2];    break;
        case ADC_CAL_PV_VOLT3:      fCode = Cla1ToCpuMsg.fPvVolt[CLA_IDX_PV3];    break;
        case ADC_CAL_PV_VOLT4:      fCode = Cla1ToCpuMsg.fPvVolt[CLA_IDX_PV4];    break;

        case ADC_CAL_BUS_VOLT_L:    fCode = Cla1ToCpuMsg.fBusVoltL;               break;
        case ADC_CAL_BUS_VOLT_H:    fCode = Cla1ToCpuMsg.fBusVoltH;               break;

        default:                    fCode = 0.0f;                                 break;
    }

    return fCode * ADC_CAL_RAW_TO_CODE12;
}

/******************************************************************************
 * @brief  零漂检测状态机（20kHz 调用）
 ******************************************************************************/
static void AdcCalibZeroTrackStep(void)
{
    Uint16 i;
    float  fMean;
    float  fDiff;
    Uint16 u16Fail;

    if (stAdcCalib.u16TrackEnable == 0U)
    {
        stAdcCalib.eZeroState = ADC_CAL_ZERO_FROZEN;
        return;
    }

    if (stAdcCalib.eZeroState == ADC_CAL_ZERO_PENDING)
    {
        /* 先跳过一段时间，等 CLA 低通从初值收敛到真实偏置 */
        if (stAdcCalib.u16SkipCnt < ADC_CAL_TRACK_SKIP)
        {
            stAdcCalib.u16SkipCnt++;
            return;
        }

        for (i = 0U; i < ADC_CAL_CH_NUM; i++)
        {
            stAdcCalib.fZeroSum[i] = 0.0f;
        }
        stAdcCalib.u16TrackCntRaw  = 0U;
        stAdcCalib.eZeroState = ADC_CAL_ZERO_TRACKING;
        return;
    }

    if (stAdcCalib.eZeroState == ADC_CAL_ZERO_TRACKING)
    {
        for (i = 0U; i < ADC_CAL_CH_NUM; i++)
        {
            if (sTrackMask[i] != 0U)
            {
                stAdcCalib.fZeroSum[i] += stAdcCalib.fCode12[i];
            }
        }

        stAdcCalib.u16TrackCntRaw++;

        if (stAdcCalib.u16TrackCntRaw >= ADC_CAL_TRACK_SAMPLES)
        {
            u16Fail = 0U;

            for (i = 0U; i < ADC_CAL_CH_NUM; i++)
            {
                if (sTrackMask[i] == 0U)
                {
                    continue;
                }

                fMean = stAdcCalib.fZeroSum[i] / (float)ADC_CAL_TRACK_SAMPLES;
                fDiff = fMean - sNominalZero[i];
                if (fDiff < 0.0f)
                {
                    fDiff = -fDiff;
                }

                if (fDiff <= ADC_CAL_ZERO_TOL)
                {
                    stAdcCalib.fZeroCode[i] = fMean;
                }
                else
                {
                    /* 超差：保留标称零点，等现场标定，状态置 FAILED */
                    u16Fail = 1U;
                }
            }

            stAdcCalib.eZeroState = (u16Fail != 0U) ? ADC_CAL_ZERO_FAILED : ADC_CAL_ZERO_DONE;
        }
    }
}

/******************************************************************************
 * 接口实现
 ******************************************************************************/
void AdcCalibInit(void)
{
    Uint16 i;

    for (i = 0U; i < ADC_CAL_CH_NUM; i++)
    {
        stAdcCalib.fZeroCode[i]   = sNominalZero[i];
        stAdcCalib.fScale[i]      = sNominalScale[i];
        stAdcCalib.fCode12[i]     = 0.0f;
        stAdcCalib.fZeroSum[i]    = 0.0f;
        stAdcCalib.fValue[i] = 0.0f;
    }

    stAdcCalib.u16SkipCnt     = 0U;
    stAdcCalib.u16TrackCntRaw    = 0U;
    stAdcCalib.u16TrackEnable = 1U;
    stAdcCalib.eZeroState   = ADC_CAL_ZERO_PENDING;
}

void AdcCalibStartZeroTrack(void)
{
    Uint16 i;

    /* 保留当前零点直到检测完成；从头开始跳过 + 累加 */
    for (i = 0U; i < ADC_CAL_CH_NUM; i++)
    {
        stAdcCalib.fZeroSum[i] = 0.0f;
    }

    stAdcCalib.u16SkipCnt     = 0U;
    stAdcCalib.u16TrackCntRaw    = 0U;
    stAdcCalib.u16TrackEnable = 1U;
    stAdcCalib.eZeroState   = ADC_CAL_ZERO_PENDING;
}

void AdcCalibSetTrackEnable(Uint16 u16Enable)
{
    stAdcCalib.u16TrackEnable = (u16Enable != 0U) ? 1U : 0U;
}

void AdcCalibRefresh(void)
{
    Uint16 i;
    float  fCode;

    for (i = 0U; i < ADC_CAL_CH_NUM; i++)
    {
        fCode       = AdcCalibReadCode12(i);
        stAdcCalib.fCode12[i]  = fCode;
        stAdcCalib.fValue[i] = (fCode - stAdcCalib.fZeroCode[i]) * stAdcCalib.fScale[i];
    }

    AdcCalibZeroTrackStep();

    /* 状态镜像（Modbus 只读） */
    stAdcCalib.u16ZeroState = (Uint16)stAdcCalib.eZeroState;
    stAdcCalib.u16TrackCnt  = stAdcCalib.u16TrackCntRaw;

    /* 两个慢变量转成 0.1 单位镜像，避免上位机做换算 */
    fCode = stAdcCalib.fValue[ADC_CAL_GFCI] * 1000.0f;
    if (fCode > 32767.0f)   { fCode = 32767.0f; }
    if (fCode < -32768.0f)  { fCode = -32768.0f; }
    stAdcCalib.i16ObsGfciMa = (int16)fCode;

    fCode = stAdcCalib.fValue[ADC_CAL_BAL_CUR] * 10.0f;
    if (fCode > 32767.0f)   { fCode = 32767.0f; }
    if (fCode < -32768.0f)  { fCode = -32768.0f; }
    stAdcCalib.i16ObsBalCur = (int16)fCode;
}

float AdcCalibGet(Uint16 u16Ch)
{
    if (u16Ch >= ADC_CAL_CH_NUM)
    {
        return 0.0f;
    }
    return stAdcCalib.fValue[u16Ch];
}

float AdcCalibInvCur(Uint16 u16Idx)
{
    return AdcCalibGet((Uint16)(ADC_CAL_INV_CUR_R + u16Idx));
}

float AdcCalibInvVolt(Uint16 u16Idx)
{
    return AdcCalibGet((Uint16)(ADC_CAL_INV_VOLT_R + u16Idx));
}

float AdcCalibGridVolt(Uint16 u16Idx)
{
    return AdcCalibGet((Uint16)(ADC_CAL_GRID_VOLT_R + u16Idx));
}

float AdcCalibPvCur(Uint16 u16Idx)
{
    return AdcCalibGet((Uint16)(ADC_CAL_PV_CUR1 + u16Idx));
}

float AdcCalibDc(Uint16 u16Idx)
{
    return AdcCalibGet((Uint16)(ADC_CAL_DC_R + u16Idx));
}

float AdcCalibGetZeroCode(Uint16 u16Ch)
{
    if (u16Ch >= ADC_CAL_CH_NUM)
    {
        return 0.0f;
    }
    return stAdcCalib.fZeroCode[u16Ch];
}

void AdcCalibSetZeroCode(Uint16 u16Ch, float fCode)
{
    if (u16Ch >= ADC_CAL_CH_NUM)
    {
        return;
    }
    stAdcCalib.fZeroCode[u16Ch] = fCode;
}

float AdcCalibGetScale(Uint16 u16Ch)
{
    if (u16Ch >= ADC_CAL_CH_NUM)
    {
        return 0.0f;
    }
    return stAdcCalib.fScale[u16Ch];
}

void AdcCalibSetScale(Uint16 u16Ch, float fScale)
{
    if (u16Ch >= ADC_CAL_CH_NUM)
    {
        return;
    }
    stAdcCalib.fScale[u16Ch] = fScale;
}

void AdcCalibRestoreNominal(void)
{
    Uint16 i;

    for (i = 0U; i < ADC_CAL_CH_NUM; i++)
    {
        stAdcCalib.fZeroCode[i] = sNominalZero[i];
        stAdcCalib.fScale[i]    = sNominalScale[i];
    }

    stAdcCalib.eZeroState = ADC_CAL_ZERO_PENDING;
}

E_ADC_CAL_ZERO_STATE AdcCalibGetZeroState(void)
{
    return stAdcCalib.eZeroState;
}

Uint16 AdcCalibGetTrackCount(void)
{
    return stAdcCalib.u16TrackCntRaw;
}

//===========================================================================
// End of file.
//===========================================================================

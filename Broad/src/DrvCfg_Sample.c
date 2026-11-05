/**
 * @file DrvCfg_Sample.c
 * @author yetingheng (004182)
 * @brief EP15K 采样标定表实例。
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 * 每项格式：{ fK, fZeroNom, bTrackZero }
 *   bTrackZero = 1：上电 3 s 后记录一次零漂（有偏置电压的通道）
 *   bTrackZero = 0：不做零漂，零点固定（所有多路选择器通道，单极性）
 */

#include "DrvCfg_Sample.h"

const ST_SAMPLE_CAL_CFG stSampleCalCfg =
{
    /* aInvCurr：逆变电流 R / S / T */
    {
        {ADC_CAL_K_INV_CUR, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_INV_CUR, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_INV_CUR, ADC_CAL_CODE_MID, 1U}
    },
    /* aInvVolt：逆变电压 R / S / T */
    {
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U}
    },
    /* aGridVolt：电网电压 R / S / T */
    {
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_AC_VOLT, ADC_CAL_CODE_MID, 1U}
    },
    /* aInvDc：逆变直流分量 R / S / T */
    {
        {ADC_CAL_K_DC_CUR, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_DC_CUR, ADC_CAL_CODE_MID, 1U},
        {ADC_CAL_K_DC_CUR, ADC_CAL_CODE_MID, 1U}
    },
    /* aPvCurr：PV1 ~ PV4 电流 */
    {
        {ADC_CAL_K_PV_CUR, ADC_CAL_CODE_PV_MID, 1U},
        {ADC_CAL_K_PV_CUR, ADC_CAL_CODE_PV_MID, 1U},
        {ADC_CAL_K_PV_CUR, ADC_CAL_CODE_PV_MID, 1U},
        {ADC_CAL_K_PV_CUR, ADC_CAL_CODE_PV_MID, 1U}
    },
    /* aPvVolt：MUX2 地址 0~3，PV1 ~ PV4 电压（单极性，无零点） */
    {
        {ADC_CAL_K_PV_VOLT, 0.0f, 0U},
        {ADC_CAL_K_PV_VOLT, 0.0f, 0U},
        {ADC_CAL_K_PV_VOLT, 0.0f, 0U},
        {ADC_CAL_K_PV_VOLT, 0.0f, 0U}
    },
    /* aVisoVolt：MUX2 地址 4 */
    {ADC_CAL_K_VISO_VOLT, 0.0f, 0U},
    /* aBusLVolt：MUX1 地址 6，下半母线 */
    {ADC_CAL_K_BUS_VOLT, 0.0f, 0U},
    /* aBusHVolt：MUX1 地址 7，上半母线 */
    {ADC_CAL_K_BUS_VOLT, 0.0f, 0U},
    /* aBalCurr：平衡电路电流 */
    {ADC_CAL_K_BAL_CUR, ADC_CAL_CODE_MID, 1U},
    /* aGfciCurr：GFCI 漏电流（本板按有偏置通道处理，与 EP5K 写死零点不同） */
    {ADC_CAL_K_GFCI, ADC_CAL_CODE_MID, 1U}
};

/**
 * @file ProtectInput.c
 * @author yetingheng (004182)
 * @brief 保护判据源实现：逆变输出与直流分量判据
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 判据源实现：逆变输出故障、输出直流分量。
 */
#include "ProtectInput.h"
#include "SampleData.h"
#include "CtrlIsr.h"
#include "InvStateMachine.h"

#include <math.h>

/* 判据运行状态：单个实例 stProtectInput */
ST_PROTECT_INPUT_STATE stProtectInput;

/* 相别到故障位的映射，显式列出，避免依赖枚举顺序 */
static const E_PROTECT_FAULT sInvOutFault[SAMPLE_PH_NUM] =
{
    PROTECT_FLT_INV_OUT_R,
    PROTECT_FLT_INV_OUT_S,
    PROTECT_FLT_INV_OUT_T
};

void ProtectInputInit(void)
{
    Uint16 i;

    for (i = 0U; i < (Uint16)SAMPLE_PH_NUM; i++)
    {
        stProtectInput.u16NoOutCnt[i] = 0U;
    }
}

/**
 * @brief 判据扫描，10 ms 节拍
 */
void ProtectInputRun10ms(void)
{
    Uint16   ph;
    Uint16   u16Active;
    Uint16   u16Auth;
    Uint16   u16NoOut;
    Uint16   u16Trip;
    float32  fMod;
    float32  fVolt;
    float32  fDev;

    /* 只在闭环运行态判输出质量，软启爬坡阶段电压未建立，不参与 */
    u16Active = (InvSmGetState() == (Uint16)INV_ST_RUN) ? 1U : 0U;
    u16Auth   = (CtrlGetAuthState() == (Uint16)CTRL_AUTH_ON) ? 1U : 0U;
    fMod      = CtrlGetModActual();

    for (ph = 0U; ph < (Uint16)SAMPLE_PH_NUM; ph++)
    {
        fVolt = stSampleRms.fInvVoltRms[ph];

        /* 判据 A：有指令无输出 —— 调制比已经给出，输出电压却建立不起来 */
        u16NoOut = 0U;
        if ((u16Active != 0U) && (u16Auth != 0U) && (fMod > PROTECT_INV_OUT_MOD_MIN)
            && (fVolt < (PROTECT_INV_VREF_VOLT * PROTECT_INV_OUT_LOW_RATIO)))
        {
            u16NoOut = 1U;
        }

        if (u16NoOut != 0U)
        {
            if (stProtectInput.u16NoOutCnt[ph] < (Uint16)PROTECT_INV_OUT_NOOUT_MS)
            {
                stProtectInput.u16NoOutCnt[ph]++;
            }
        }
        else
        {
            stProtectInput.u16NoOutCnt[ph] = 0U;
        }

        /* 判据 B：幅值不合规 —— 稳态下偏离参考电压超过 10% */
        fDev = fabsf(fVolt - PROTECT_INV_VREF_VOLT);

        u16Trip = 0U;
        if ((stProtectInput.u16NoOutCnt[ph] >= (Uint16)PROTECT_INV_OUT_NOOUT_MS)
            || ((u16Active != 0U) && (fDev > (PROTECT_INV_VREF_VOLT * PROTECT_INV_OUT_DEV_RATIO))))
        {
            u16Trip = 1U;
        }

        if (u16Trip != 0U)
        {
            ProtectTrigger(sInvOutFault[ph]);
        }
        else
        {
            ProtectRelease(sInvOutFault[ph]);
        }
    }

    /* 判据 C：输出直流分量 —— 任一相超限即报，位是整机一个 */
    u16Trip = 0U;
    for (ph = 0U; ph < (Uint16)SAMPLE_PH_NUM; ph++)
    {
        if (fabsf(stSampleAvg.fInvDcOfsAvg200[ph]) > PROTECT_INV_DC_OFS_MAX_A)
        {
            u16Trip = 1U;
            break;
        }
    }

    if (u16Trip != 0U)
    {
        ProtectTrigger(PROTECT_FLT_INV_DC_COMP);
    }
    else
    {
        ProtectRelease(PROTECT_FLT_INV_DC_COMP);
    }
}

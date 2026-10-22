/**
 * @file Hw_InvDriver.c
 * @author yetingheng (004182)
 * @brief 三电平逆变桥驱动层实现
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
 * @note 换相原理：斩波模块占空比降到 0 时，A 恒低、B 恒高（死区反相），
 *       与"另一模块静态钳位"的波形逐位相同，因此换相可在单拍内完成。
 *       仅当切出模块占空比未归零（异常）时，退化为两拍换相以求安全。
 */

#include "Hw_InvDriver.h"
#include "Hw_PwmDriver.h"

static const ST_INV_CFG *gInvCfg = (const ST_INV_CFG *)0;

/* 每相运行状态 */
static Uint16 gInvChop[HW_INV_PHASE_MAX];       /* 当前斩波模块编号 */
static Uint16 gInvPending[HW_INV_PHASE_MAX];    /* 1 = 两拍换相的第二拍待完成 */
static float  gInvDuty[HW_INV_PHASE_MAX];       /* 当前已下发占空比 */

/* 取单相配置表项。 */
static const ST_INV_PHASE_CFG *Hw_InvGetPhaseCfg(Uint16 u16Phase)
{
    if (gInvCfg == (const ST_INV_CFG *)0)
    {
        return (const ST_INV_PHASE_CFG *)0;
    }

    if (u16Phase >= gInvCfg->u16PhaseNum)
    {
        return (const ST_INV_PHASE_CFG *)0;
    }

    return &gInvCfg->pstPhaseCfg[u16Phase];
}

/* 把模块压成"静态中点钳位"：A=0 -> 上/下竖管关断，B 经死区反相 =1 -> 横管导通。
 * 同时把 CMPA 影子写回周期值，保证该模块将来被释放时第一拍不产生脉冲。 */
static void Hw_InvForceHold(Uint16 u16PwmId, Uint16 u16Period)
{
    Hw_PwmSetCmpA(u16PwmId, u16Period);
    Hw_PwmSetForce(u16PwmId, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW);
}

/* 初始化：三相全部压到中点钳位，占空比清零，默认正半周。 */
void Hw_InvInit(const ST_INV_CFG *pstInvCfg)
{
    const ST_INV_PHASE_CFG *pstPhaseCfg;
    Uint16 i;

    gInvCfg = pstInvCfg;

    for (i = 0U; i < HW_INV_PHASE_MAX; i++)
    {
        gInvChop[i] = (Uint16)HW_PWM_ID_NULL;
        gInvPending[i] = 0U;
        gInvDuty[i] = 0.0f;
    }

    if ((pstInvCfg == (const ST_INV_CFG *)0) || (pstInvCfg->pstPhaseCfg == (const ST_INV_PHASE_CFG *)0))
    {
        return;
    }

    for (i = 0U; i < pstInvCfg->u16PhaseNum; i++)
    {
        pstPhaseCfg = &pstInvCfg->pstPhaseCfg[i];

        Hw_InvForceHold(pstPhaseCfg->u16PosPwmId, pstPhaseCfg->u16Period);
        Hw_InvForceHold(pstPhaseCfg->u16NegPwmId, pstPhaseCfg->u16Period);

        gInvChop[i] = pstPhaseCfg->u16PosPwmId;
    }

    /* 全局装载触发：INV 六路模块的 CMPA/AQCSFRC 均在 GLDCFG 中，
     * 且 GLDCTL2LINK 已挂到 EPWM1，切到这里触发一次即可让六路同步生效。 */
    Hw_TrigGlobalLoad();
}

/* 下发单相占空比。 */
void Hw_InvSetDuty(Uint16 u16Phase, float fDuty, Uint16 u16Positive)
{
    const ST_INV_PHASE_CFG *pstPhaseCfg;
    Uint16 u16OldChop;
    Uint16 u16NewChop;
    Uint16 u16Hold;
    Uint16 u16CmpA;

    pstPhaseCfg = Hw_InvGetPhaseCfg(u16Phase);
    if (pstPhaseCfg == (const ST_INV_PHASE_CFG *)0)
    {
        return;
    }

    if (fDuty < 0.0f)
    {
        fDuty = 0.0f;
    }

    if (fDuty > HW_INV_DUTY_MAX)
    {
        fDuty = HW_INV_DUTY_MAX;
    }

    u16NewChop = (u16Positive != 0U) ? pstPhaseCfg->u16PosPwmId : pstPhaseCfg->u16NegPwmId;
    u16Hold    = (u16Positive != 0U) ? pstPhaseCfg->u16NegPwmId : pstPhaseCfg->u16PosPwmId;
    u16OldChop = gInvChop[u16Phase];

    if (u16NewChop != u16OldChop)
    {
        if (gInvDuty[u16Phase] > HW_INV_DUTY_EPS)
        {
            /* 异常路径：切出模块占空比未归零。两拍换相，本拍只把两路都压到中点。 */
            Hw_InvForceHold(u16OldChop, pstPhaseCfg->u16Period);
            Hw_InvForceHold(u16NewChop, pstPhaseCfg->u16Period);

            gInvChop[u16Phase] = u16NewChop;
            gInvDuty[u16Phase] = 0.0f;
            gInvPending[u16Phase] = 1U;
            return;
        }

        /* 正常路径：单拍换相。切出路先归零钳位，切入路直接释放。 */
        Hw_InvForceHold(u16OldChop, pstPhaseCfg->u16Period);
        Hw_PwmReleaseForce(u16NewChop);

        gInvChop[u16Phase] = u16NewChop;
        gInvPending[u16Phase] = 0U;
    }
    else if (gInvPending[u16Phase] != 0U)
    {
        /* 两拍换相的第二拍：释放新的斩波模块。 */
        Hw_PwmReleaseForce(u16NewChop);
        gInvPending[u16Phase] = 0U;
    }
    else
    {
        /* 同半周，无需换相。 */
    }

    /* 保持路始终钳位（幂等，保证其影子 CMPA 一直是周期值）。 */
    Hw_InvForceHold(u16Hold, pstPhaseCfg->u16Period);

    /* 占空比换算：A 通道脉冲居中于 CTR=PRD，d = (TBPRD - CMPA) / TBPRD */
    u16CmpA = (Uint16)(((1.0f - fDuty) * (float)pstPhaseCfg->u16Period) + 0.5f);
    if (u16CmpA > pstPhaseCfg->u16Period)
    {
        u16CmpA = pstPhaseCfg->u16Period;
    }

    Hw_PwmSetCmpA(u16NewChop, u16CmpA);
    gInvDuty[u16Phase] = fDuty;

    /* 全局装载触发：INV 六路模块的 CMPA/AQCSFRC 均在 GLDCFG 中，
     * 且 GLDCTL2LINK 已挂到 EPWM1，切到这里触发一次即可让六路同步生效。 */
    Hw_TrigGlobalLoad();
}

/* 单相封波。 */
void Hw_InvStop(Uint16 u16Phase)
{
    const ST_INV_PHASE_CFG *pstPhaseCfg;

    pstPhaseCfg = Hw_InvGetPhaseCfg(u16Phase);
    if (pstPhaseCfg == (const ST_INV_PHASE_CFG *)0)
    {
        return;
    }

    Hw_InvForceHold(pstPhaseCfg->u16PosPwmId, pstPhaseCfg->u16Period);
    Hw_InvForceHold(pstPhaseCfg->u16NegPwmId, pstPhaseCfg->u16Period);

    gInvDuty[u16Phase] = 0.0f;
    gInvPending[u16Phase] = 0U;

    /* 全局装载触发：INV 六路模块的 CMPA/AQCSFRC 均在 GLDCFG 中，
     * 且 GLDCTL2LINK 已挂到 EPWM1，切到这里触发一次即可让六路同步生效。 */
    Hw_TrigGlobalLoad();
}

/* 三相封波。 */
void Hw_InvStopAll(void)
{
    Uint16 i;

    for (i = 0U; i < HW_INV_PHASE_MAX; i++)
    {
        Hw_InvStop(i);
    }
}

/* 读回当前斩波模块。 */
Uint16 Hw_InvGetChopPwm(Uint16 u16Phase)
{
    if (u16Phase >= HW_INV_PHASE_MAX)
    {
        return (Uint16)HW_PWM_ID_NULL;
    }

    return gInvChop[u16Phase];
}

/* 读回当前占空比。 */
float Hw_InvGetDuty(Uint16 u16Phase)
{
    if (u16Phase >= HW_INV_PHASE_MAX)
    {
        return 0.0f;
    }

    return gInvDuty[u16Phase];
}

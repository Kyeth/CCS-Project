/**
 * @file Hw_PwmDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS ePWM驱动实现
 * @version 0.4
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_PwmDriver.h"


static volatile struct EPWM_REGS * const gPwmRegs[HW_PWM_ID_MAX] =
{
    &EPwm1Regs,
    &EPwm2Regs,
    &EPwm3Regs,
    &EPwm4Regs,
    &EPwm5Regs,
    &EPwm6Regs,
    &EPwm7Regs,
    &EPwm8Regs,
    &EPwm9Regs,
    &EPwm10Regs,
    &EPwm11Regs,
    &EPwm12Regs
};


/* 判断PWM编号是否有效。 */
static Uint16 Hw_PwmIdIsValid(Uint16 u16PwmId)
{
    if (u16PwmId >= HW_PWM_ID_MAX)
    {
        return 0U;
    }
    return 1U;
}

/* 配置A通道动作限定器。 */
static void Hw_PwmConfigAqA(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_AQ_CFG *pstAqCfg)
{
    pstPwmRegs->AQCTLA.bit.ZRO = pstAqCfg->u16Zro;
    pstPwmRegs->AQCTLA.bit.PRD = pstAqCfg->u16Prd;
    pstPwmRegs->AQCTLA.bit.CAU = pstAqCfg->u16Cau;
    pstPwmRegs->AQCTLA.bit.CAD = pstAqCfg->u16Cad;
    pstPwmRegs->AQCTLA.bit.CBU = pstAqCfg->u16Cbu;
    pstPwmRegs->AQCTLA.bit.CBD = pstAqCfg->u16Cbd;
}

/* 配置B通道动作限定器。 */
static void Hw_PwmConfigAqB(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_AQ_CFG *pstAqCfg)
{
    pstPwmRegs->AQCTLB.bit.ZRO = pstAqCfg->u16Zro;
    pstPwmRegs->AQCTLB.bit.PRD = pstAqCfg->u16Prd;
    pstPwmRegs->AQCTLB.bit.CAU = pstAqCfg->u16Cau;
    pstPwmRegs->AQCTLB.bit.CAD = pstAqCfg->u16Cad;
    pstPwmRegs->AQCTLB.bit.CBU = pstAqCfg->u16Cbu;
    pstPwmRegs->AQCTLB.bit.CBD = pstAqCfg->u16Cbd;
}

/* 配置死区模块。 */
static void Hw_PwmConfigDeadBand(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_CFG_ITEM *pstPwmCfgItem)
{
    pstPwmRegs->DBRED.all = pstPwmCfgItem->u16DeadTime;
    pstPwmRegs->DBFED.all = pstPwmCfgItem->u16DeadTime;

    /* INV and BUS B outputs are complementary copies of AQ A, with dead time. */
    if ((pstPwmCfgItem->u16PwmType == HW_PWM_TYPE_INV) ||
        (pstPwmCfgItem->u16PwmType == HW_PWM_TYPE_BUS))
    {
        pstPwmRegs->DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
        pstPwmRegs->DBCTL.bit.POLSEL = DB_ACTV_HIC;
        /* Both dead-band paths must use AQ A as their input. */
        pstPwmRegs->DBCTL.bit.IN_MODE = DBA_ALL;
    }
    else
    {
        pstPwmRegs->DBCTL.bit.OUT_MODE = DB_DISABLE;
    }
}

/* 配置SOCA事件触发。 */
static void Hw_PwmConfigSoca(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_ET_CFG *pstEtCfg)
{
    pstPwmRegs->ETSEL.bit.SOCAEN = 0U;

    if (pstEtCfg->bEnable == 0U)
    {
        return;
    }

    pstPwmRegs->ETSEL.bit.SOCASEL = pstEtCfg->u16Sel;
    pstPwmRegs->ETPS.bit.SOCPSSEL = pstEtCfg->bUsePrd2;

    if (pstEtCfg->bUsePrd2 != 0U)
    {
        pstPwmRegs->ETSOCPS.bit.SOCAPRD2 = pstEtCfg->u16Prd2;
    }
    else
    {
        pstPwmRegs->ETPS.bit.SOCAPRD = pstEtCfg->u16Prd;
    }

    pstPwmRegs->ETSEL.bit.SOCAEN = 1U;
}

/* This build uses local ZERO shadow loads only; no global load group. */
static void Hw_PwmConfigGlobalLoad(volatile struct EPWM_REGS *regs,
                                   const ST_PWM_CFG_ITEM *cfg)
{
    regs->EPWMXLINK.bit.CMPALINK = cfg->u16PwmId;
    regs->EPWMXLINK.bit.CMPBLINK = cfg->u16PwmId;
    regs->GLDCTL.bit.GLD = 0U;
}
/* 初始化单个ePWM模块。 */
void Hw_SinglePwmInit(ST_PWM_CFG_ITEM *pstPwmCfgItem)
{
    volatile struct EPWM_REGS *pstPwmRegs;

    if (pstPwmCfgItem == (ST_PWM_CFG_ITEM *)0)
    {
        return;
    }

    if (Hw_PwmIdIsValid(pstPwmCfgItem->u16PwmId) == 0U)
    {
        return;
    }

    pstPwmRegs = gPwmRegs[pstPwmCfgItem->u16PwmId];

    EALLOW;

    /* 1. 配置TZ故障动作。 */
    pstPwmRegs->TZCTL.bit.TZA = pstPwmCfgItem->u16TzActionA;
    pstPwmRegs->TZCTL.bit.TZB = pstPwmCfgItem->u16TzActionB;

    /* 2. 配置初始强制输出状态。 */
    pstPwmRegs->AQCSFRC.bit.CSFA = pstPwmCfgItem->u16InitForceA;
    pstPwmRegs->AQCSFRC.bit.CSFB = pstPwmCfgItem->u16InitForceB;

    /* 3. 配置时基模块。 */
    /* The first period must be active before a nonzero TBCTR is preloaded. */
    pstPwmRegs->TBCTL.bit.PRDLD = TB_IMMEDIATE;
    pstPwmRegs->TBCTL.bit.CTRMODE = pstPwmCfgItem->u16CtrMode;
    pstPwmRegs->TBCTL.bit.PHSDIR = pstPwmCfgItem->u16SyncCntLoadDir;
    pstPwmRegs->TBCTL.bit.CLKDIV = TB_DIV1;
    pstPwmRegs->TBCTL.bit.HSPCLKDIV = TB_DIV1;
    pstPwmRegs->TBCTL.bit.FREE_SOFT = 2U;

    pstPwmRegs->TBPRD = pstPwmCfgItem->u16Period;
    pstPwmRegs->TBCTL.bit.PRDLD = TB_SHADOW;
    pstPwmRegs->TBPRD = pstPwmCfgItem->u16Period;
    if (pstPwmCfgItem->u16SyncMode == HW_PWM_FREE)
    {
        pstPwmRegs->TBCTR = pstPwmCfgItem->u16Phase;
        pstPwmRegs->TBPHS.bit.TBPHS = 0U;
    }
    else
    {
        pstPwmRegs->TBCTR = 0U;
        pstPwmRegs->TBPHS.bit.TBPHS = pstPwmCfgItem->u16Phase;
    }

    /* The Broad table selects ZERO, pass-through, or disabled SYNCO. */
    if (pstPwmCfgItem->u16SyncOutSel == HW_EPWM_SYNC_OUT_FROM_CNTR_ZERO)
    {
        pstPwmRegs->TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;
    }
    else if (pstPwmCfgItem->u16SyncOutSel == HW_EPWM_SYNC_OUT_FROM_SYNCIN)
    {
        pstPwmRegs->TBCTL.bit.SYNCOSEL = TB_SYNC_IN;
    }
    else
    {
        pstPwmRegs->TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    }

    if (pstPwmCfgItem->u16SyncMode == HW_PWM_SLAVE)
    {
        /* Only synchronized slaves reload TBCTR from TBPHS. */
        pstPwmRegs->TBCTL.bit.PHSEN = 1U;
    }
    else
    {
        /* Masters and free-running modules ignore SYNCI. */
        pstPwmRegs->TBCTL.bit.PHSEN = 0U;
    }

    /* 5. 配置比较模块，只使用CMPA和CMPB，不配置CMPC和CMPD。 */
    pstPwmRegs->CMPA.bit.CMPA = pstPwmCfgItem->u16CmpA;
    pstPwmRegs->CMPB.bit.CMPB = pstPwmCfgItem->u16CmpB;

    pstPwmRegs->CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    pstPwmRegs->CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    pstPwmRegs->CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    pstPwmRegs->CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    /* 6. 配置A、B通道动作限定器。 */
    Hw_PwmConfigAqA(pstPwmRegs, &pstPwmCfgItem->stAqA);
    Hw_PwmConfigAqB(pstPwmRegs, &pstPwmCfgItem->stAqB);

    /* 7. 配置死区模块。 */
    Hw_PwmConfigDeadBand(pstPwmRegs, pstPwmCfgItem);

    /* 8. Configure the one SOCA sample trigger. */
    Hw_PwmConfigSoca(pstPwmRegs, &pstPwmCfgItem->stEtSoca);
    pstPwmRegs->ETSEL.bit.SOCBEN = 0U;

    /* 9. 配置全局加载。 */
    Hw_PwmConfigGlobalLoad(pstPwmRegs, pstPwmCfgItem);

    EDIS;
}

/* 按配置表初始化多个ePWM模块。 */
void Hw_PwmInit(ST_PWM_CFG *pstPwmCfg)
{
    Uint16 i;

    if (pstPwmCfg == (ST_PWM_CFG *)0)
    {
        return;
    }

    if (pstPwmCfg->pstPwmCfgItem == (ST_PWM_CFG_ITEM *)0)
    {
        return;
    }


    /* 1. 关闭 TBCLK 同步，允许配置期间时基停摆 */
    Hw_DisableAllPwmCLKSync();

    /* 2. 逐个配置 ePWM 模块的寄存器 */
    for (i = 0U; i < pstPwmCfg->u16ItemNum; i++)
    {
        /* EPWM4/7/10 start the three hardware synchronization groups.
         * SYNCSELECT source codes: EPWM1=0, EPWM4=1, EPWM7=2, EPWM10=3. */
        Uint16 src = pstPwmCfg->pstPwmCfgItem[i].u16SyncInSel;
        Uint16 sel = 0U;
        if (src == HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM4) { sel = 1U; }
        if (src == HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7) { sel = 2U; }
        if (src == HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM10) { sel = 3U; }
        EALLOW;
        if (pstPwmCfg->pstPwmCfgItem[i].u16PwmId == HW_PWM_ID3)
            { SyncSocRegs.SYNCSELECT.bit.EPWM4SYNCIN = sel; }
        if (pstPwmCfg->pstPwmCfgItem[i].u16PwmId == HW_PWM_ID6)
            { SyncSocRegs.SYNCSELECT.bit.EPWM7SYNCIN = sel; }
        if (pstPwmCfg->pstPwmCfgItem[i].u16PwmId == HW_PWM_ID9)
            { SyncSocRegs.SYNCSELECT.bit.EPWM10SYNCIN = sel; }
        EDIS;
        Hw_SinglePwmInit(&pstPwmCfg->pstPwmCfgItem[i]);
    }

    /* TBCLKSYNC由StartSystem在ADC/CLA和全部状态初始化完成后统一放开。 */
}

/* 使能所需ePWM模块的时钟（需在 Hw_PwmInit 之前调用） */
void Hw_EnablePwmClocks(void)
{
    EALLOW;
    /* F2837xS的EPWM1~12门控全部在PCLKCR2。 */
    CpuSysRegs.PCLKCR2.all |= 0x0FFFU;
    EDIS;
}

/* 使能所有ePWM时基时钟（执行一次性上电对齐）。 */
void Hw_EnableAllPwmCLKSync(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1U;
    EDIS;
}

/* 禁止所有ePWM时基时钟。 */
void Hw_DisableAllPwmCLKSync(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0U;
    EDIS;
}

/* Latch every ePWM output low. There is deliberately no release API in this image. */
void Hw_StopAllPWM(void)
{
    Uint16 i;
    EALLOW;
    for (i = 0U; i < HW_PWM_ID_MAX; i++)
    {
        gPwmRegs[i]->TZFRC.bit.OST = 1U;
    }
    EDIS;
}
/* ============ 数字比较（DC）故障源 ============ */
/* 把指定 TRIP 接进 DCAH：TRIPINPUTn 选路，COMPSEL 选比较通道（n = TRIPn - 1）。 */
static void Hw_PwmConfigDcaTrip(volatile struct EPWM_REGS *pstPwmRegs, Uint16 u16Trip)
{
    pstPwmRegs->DCAHTRIPSEL.all = 0U;

    if ((u16Trip >= 1U) && (u16Trip <= 15U))
    {
        pstPwmRegs->DCAHTRIPSEL.all = (Uint16)(1U << (u16Trip - 1U));
        pstPwmRegs->DCTRIPSEL.bit.DCAHCOMPSEL = (Uint16)(u16Trip - 1U);
    }
}

/* 把指定 TRIP 接进 DCBH。 */
static void Hw_PwmConfigDcbTrip(volatile struct EPWM_REGS *pstPwmRegs, Uint16 u16Trip)
{
    pstPwmRegs->DCBHTRIPSEL.all = 0U;

    if ((u16Trip >= 1U) && (u16Trip <= 15U))
    {
        pstPwmRegs->DCBHTRIPSEL.all = (Uint16)(1U << (u16Trip - 1U));
        pstPwmRegs->DCTRIPSEL.bit.DCBHCOMPSEL = (Uint16)(u16Trip - 1U);
    }
}

/* 按配置表配置各模块的 DC 事件（统一走 EVT2 + CBC）。 */
void Hw_PwmInitTripZone(const ST_PWM_TZ_CFG *pstPwmTzCfg)
{
    Uint16 i;
    const ST_PWM_TZ_CFG_ITEM *pstItem;
    volatile struct EPWM_REGS *pstPwmRegs;

    if (pstPwmTzCfg == (const ST_PWM_TZ_CFG *)0)
    {
        return;
    }

    for (i = 0U; i < pstPwmTzCfg->u16ItemNum; i++)
    {
        pstItem = &pstPwmTzCfg->pstPwmTzCfgItem[i];

        if (Hw_PwmIdIsValid(pstItem->u16PwmId) == 0U)
        {
            continue;
        }

        pstPwmRegs = gPwmRegs[pstItem->u16PwmId];

        EALLOW;

        /* 1. 选 TRIP 源 */
        Hw_PwmConfigDcaTrip(pstPwmRegs, pstItem->u16DcaTrip);
        Hw_PwmConfigDcbTrip(pstPwmRegs, pstItem->u16DcbTrip);

        /* 2. 触发条件（TZDCSEL）与动作（TZCTL），都走 EVT2 */
        pstPwmRegs->TZDCSEL.bit.DCAEVT2 = pstItem->u16DcaEvent;
        pstPwmRegs->TZDCSEL.bit.DCBEVT2 = pstItem->u16DcbEvent;
        pstPwmRegs->TZCTL.bit.DCAEVT2   = pstItem->u16DcaAction;
        pstPwmRegs->TZCTL.bit.DCBEVT2   = pstItem->u16DcbAction;

        /* 3. CBC 选源：只把配了触发条件的那一侧放进 CBC */
        pstPwmRegs->TZSEL.bit.DCAEVT2 = (pstItem->u16DcaEvent != 0U) ? 1U : 0U;
        pstPwmRegs->TZSEL.bit.DCBEVT2 = (pstItem->u16DcbEvent != 0U) ? 1U : 0U;

        /* 4. 不开 TZ 中断（判定放软件任务里），清掉配置过程中可能残留的 CBC 标志 */
        pstPwmRegs->TZEINT.bit.CBC = 0U;
        pstPwmRegs->TZCLR.bit.CBC  = 1U;

        EDIS;
    }
}

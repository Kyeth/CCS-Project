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

static ST_PWM_CFG *gPwmCfg = (ST_PWM_CFG *)0;

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

    /* INV 与 BUS 都是"一路 AQ 出波、另一路由死区模块反相生成"的结构：
     * INV 的 B 是桥臂另一只管子，BUS(EPWM8) 的 B 是平衡桥另一半。
     * 死区模块一关（OUT_MODE=DB_DISABLE），B 就只受 AQCTLB 控制，
     * 而配置表里 AQCTLB 全是 NO_ACTION => B 恒为低，示波器上看不到波形
     * （实测 EPWM8A 有 20kHz/50%、EPWM8B 完全没有）。 */
    if ((pstPwmCfgItem->u16PwmType == HW_PWM_TYPE_INV) ||
        (pstPwmCfgItem->u16PwmType == HW_PWM_TYPE_BUS))
    {
        pstPwmRegs->DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
        pstPwmRegs->DBCTL.bit.POLSEL = DB_ACTV_HIC;
        /* IN_MODE 必须是 DBA_ALL(0x0)：EPWMxA 与 EPWMxB 都取 AQ A 的输出，
         * 然后 POLSEL=DB_ACTV_HIC 把 B 反相、DBRED/DBFED 插入死区，
         * 得到标准互补对称 PWM。
         * 原来写 DBA_RED_DBB_FED(0x2)，B 的输入是 DBB = AQCTLB，
         * 而本工程所有 AQCTLB 都是 AQ_NO_ACTION => B 输出恒定，
         * 示波器上 EPWM1B/2B/3B/4B/10B/11B/8B 全部没有波形（A 正常）。 */
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

/* 配置 SOCB 事件（第二采样点）。 */
static void Hw_PwmConfigSocb(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_ET_CFG *pstEtCfg)
{
    pstPwmRegs->ETSEL.bit.SOCBEN = 0U;

    if (pstEtCfg->bEnableB == 0U)
    {
        return;
    }

    pstPwmRegs->ETSEL.bit.SOCBSEL = pstEtCfg->u16SelB;
    pstPwmRegs->ETPS.bit.SOCPSSEL = pstEtCfg->bUsePrd2B;

    if (pstEtCfg->bUsePrd2B != 0U)
    {
        pstPwmRegs->ETSOCPS.bit.SOCBPRD2 = pstEtCfg->u16Prd2B;
    }
    else
    {
        pstPwmRegs->ETPS.bit.SOCBPRD = pstEtCfg->u16PrdB;
    }

    pstPwmRegs->ETSEL.bit.SOCBEN = 1U;
}

/* 配置全局加载。 */
static void Hw_PwmConfigGlobalLoad(volatile struct EPWM_REGS *pstPwmRegs, const ST_PWM_CFG_ITEM *pstPwmCfgItem)
{
    /* EPWMXLINK 复位值 0 指向 EPWM1：不清掉的话，任何模块写 CMPA 都会同时
     * 写进 EPWM1 的 CMPA，EPWM1 的占空比被别的模块反复覆盖。
     * 这里把 CMPA/CMPB 链都指回自己，等于关闭跨模块镜像。 */
    pstPwmRegs->EPWMXLINK.bit.CMPALINK = pstPwmCfgItem->u16PwmId;
    pstPwmRegs->EPWMXLINK.bit.CMPBLINK = pstPwmCfgItem->u16PwmId;

    if (pstPwmCfgItem->bGload == 0U)
    {
        pstPwmRegs->GLDCTL.bit.GLD = 0U;
        return;
    }

    pstPwmRegs->GLDCFG.bit.CMPA_CMPAHR = 1U;
    pstPwmRegs->GLDCFG.bit.CMPB_CMPBHR = 1U;
    pstPwmRegs->GLDCFG.bit.TBPRD_TBPRDHR = 1U;
    pstPwmRegs->GLDCFG.bit.DBRED_DBREDHR = 1U;
    pstPwmRegs->GLDCFG.bit.DBFED_DBFEDHR = 1U;
    pstPwmRegs->GLDCFG.bit.DBCTL = 1U;
    pstPwmRegs->GLDCFG.bit.AQCSFRC = 1U;

    pstPwmRegs->EPWMXLINK.bit.GLDCTL2LINK = pstPwmCfgItem->u16LinkPwm;

    /* OSHTMODE=0：连续模式；GLDMODE=1：每个 PWM 周期 CTR=0 自动装载一次。
     * 原写法 GLDMODE=0 是"不选任何装载脉冲"，影子寄存器永不自动装载，
     * 只能靠 GLDCTL2.OSHTLD 的一次性事件，组内从模块经常漏装载。 */
    /* 台架阶段关闭全局装载（GLD）：CMPA/AQCSFRC 一律直写，写入立即生效、
     * 读回就是真实值。GLD 影子 + 组主/组员链接在本板上反复出现
     * "写了不生效、读回是旧值、只有个别模块出波"，先退回直写路径
     * 保证驱动能出来；代价是丢失六路 CMPA 的严格同步装载，
     * 等控制环路稳定后再按 TI 方式重新启用。 */
    pstPwmRegs->GLDCTL.bit.OSHTMODE = 0U;
    pstPwmRegs->GLDCTL.bit.GLDMODE = 0U;
    pstPwmRegs->GLDCTL.bit.GLD = 0U;
    pstPwmRegs->GLDCTL2.bit.OSHTLD = 0U;
    pstPwmRegs->EPWMXLINK.bit.GLDCTL2LINK = 0U;
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
    pstPwmRegs->TBCTL.bit.PRDLD = TB_SHADOW;
    pstPwmRegs->TBCTL.bit.CTRMODE = pstPwmCfgItem->u16CtrMode;
    pstPwmRegs->TBCTL.bit.PHSDIR = pstPwmCfgItem->u16SyncCntLoadDir;
    pstPwmRegs->TBCTL.bit.CLKDIV = TB_DIV1;
    pstPwmRegs->TBCTL.bit.HSPCLKDIV = TB_DIV1;
    pstPwmRegs->TBCTL.bit.FREE_SOFT = 2U;

    pstPwmRegs->TBPRD = pstPwmCfgItem->u16Period;
    pstPwmRegs->TBCTR = 0U;
    pstPwmRegs->TBPHS.bit.TBPHS = pstPwmCfgItem->u16Phase;

    /* 4. 配置同步网络拓扑 */
    HW_REG_EPWMSYNCINSEL(pstPwmRegs) = pstPwmCfgItem->u16SyncInSel;
    HW_REG_EPWMSYNCOUTEN(pstPwmRegs) = pstPwmCfgItem->u16SyncOutSel;

    if (pstPwmCfgItem->u16SyncMode == HW_PWM_MASTER)
    {
        /* 主模块不接收外部相位加载，纯粹产生SYNCO */
        pstPwmRegs->TBCTL.bit.PHSEN = 0U;
    }
    else if (pstPwmCfgItem->u16SyncMode == HW_PWM_SLAVE)
    {
        /* 从模块接收SYNCI，并将TBPHS加载到TBCTR */
        pstPwmRegs->TBCTL.bit.PHSEN = 1U;
    }
    else
    {
        /* 独立运行模块 */
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

    /* 8. 配置SOCA/SOCB事件触发。 */
    Hw_PwmConfigSoca(pstPwmRegs, &pstPwmCfgItem->stEtSoca);
    Hw_PwmConfigSocb(pstPwmRegs, &pstPwmCfgItem->stEtSoca);

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

    gPwmCfg = pstPwmCfg;

    /* 1. 关闭 TBCLK 同步，允许配置期间时基停摆 */
    Hw_DisableAllPwmCLKSync();

    /* 2. 逐个配置 ePWM 模块的寄存器 */
    for (i = 0U; i < pstPwmCfg->u16ItemNum; i++)
    {
        Hw_SinglePwmInit(&pstPwmCfg->pstPwmCfgItem[i]);
    }

    /* 3. 开启 TBCLK 同步，所有配置好的模块在此刻起跑，瞬间建立相位差 */
    Hw_EnableAllPwmCLKSync();
}

/* 使能所需ePWM模块的时钟（需在 Hw_PwmInit 之前调用） */
void Hw_EnablePwmClocks(void)
{
    EALLOW;
    /* PCLKCR2: Bit0~7 对应 EPWM1~8 */
    CpuSysRegs.PCLKCR2.all |= 0x00FFU;
    /* PCLKCR3: Bit0~3 对应 EPWM9~12 */
    CpuSysRegs.PCLKCR3.all |= 0x000FU;
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

/* 启动指定ePWM输出。 */
void Hw_PwmStart(Uint16 u16PwmId)
{
    if (Hw_PwmIdIsValid(u16PwmId) == 0U)
    {
        return;
    }

    EALLOW;
    gPwmRegs[u16PwmId]->AQCSFRC.bit.CSFA = HW_PWM_FORCE_NONE;
    gPwmRegs[u16PwmId]->AQCSFRC.bit.CSFB = HW_PWM_FORCE_NONE;
    gPwmRegs[u16PwmId]->TZCLR.bit.OST = 1U;
    EDIS;
}

/* 停止指定ePWM输出。 */
void Hw_PwmStop(Uint16 u16PwmId)
{
    if (Hw_PwmIdIsValid(u16PwmId) == 0U)
    {
        return;
    }

    EALLOW;
    gPwmRegs[u16PwmId]->TZFRC.bit.OST = 1U;
    EDIS;
}

/* 启动所有ePWM输出。 */
void Hw_StartAllPWM(void)
{
    Uint16 i;

    for (i = 0U; i < HW_PWM_ID_MAX; i++)
    {
        Hw_PwmStart(i);
    }
}

/* 停止所有ePWM输出。 */
void Hw_StopAllPWM(void)
{
    Uint16 i;

    for (i = 0U; i < HW_PWM_ID_MAX; i++)
    {
        Hw_PwmStop(i);
    }
}

/* 清除所有ePWM的 Trip Zone (TZ) 标志位 */
void Hw_PwmClearAllTripZone(void)
{
    Uint16 i;

    EALLOW;
    for (i = 0U; i < HW_PWM_ID_MAX; i++)
    {
        gPwmRegs[i]->TZCLR.bit.OST = 1U;
        /* CBC 是锁存型：上电初期故障脚还没稳定就会误锁存，
         * 不清掉的话即使故障已消失，输出也会一直被压在低电平
         * （实测 EPWM7 上电后 TZFLG 恒为 0x0012，PV 四路全无驱动）。 */
        gPwmRegs[i]->TZCLR.bit.CBC = 1U;
        gPwmRegs[i]->TZCLR.bit.INT = 1U;
    }
    EDIS;
}

/* 触发配置表中第一个启用全局加载的PWM执行一次性加载。 */
void Hw_TrigGlobalLoad(void)
{
    Uint16 i;
    Uint16 u16PwmId;

    if (gPwmCfg == (ST_PWM_CFG *)0)
    {
        return;
    }

    EALLOW;

    /* 逐个触发“全局加载组主”：判据是 bGload != 0 且 GLDCTL2LINK 指向自己。
     * 本工程有两组组主：逆变组 EPWM1（EPWM2/3/4/10/11 跟随）、
     * PV 组 EPWM7（EPWM9/5/6 跟随）。旧实现只触发配置表里第一个组主，
     * 结果 PV 组的 AQCSFRC/CMPA 永远进不了硬件，PV 四路死波。 */
    for (i = 0U; i < gPwmCfg->u16ItemNum; i++)
    {
        if (gPwmCfg->pstPwmCfgItem[i].bGload == 0U)
        {
            continue;
        }

        if (gPwmCfg->pstPwmCfgItem[i].u16LinkPwm != gPwmCfg->pstPwmCfgItem[i].u16PwmId)
        {
            continue;
        }

        u16PwmId = gPwmCfg->pstPwmCfgItem[i].u16PwmId;

        if (Hw_PwmIdIsValid(u16PwmId) == 0U)
        {
            continue;
        }

        gPwmRegs[u16PwmId]->GLDCTL2.bit.OSHTLD = 1U;
    }

    EDIS;
}

/* 写入CMPA影子寄存器。 */
void Hw_PwmSetCmpA(Uint16 u16PwmId, Uint16 u16CmpA)
{
    if (Hw_PwmIdIsValid(u16PwmId) == 0U)
    {
        return;
    }

    gPwmRegs[u16PwmId]->CMPA.bit.CMPA = u16CmpA;
}

/* 写入CMPB影子寄存器。 */
void Hw_PwmSetCmpB(Uint16 u16PwmId, Uint16 u16CmpB)
{
    if (Hw_PwmIdIsValid(u16PwmId) == 0U)
    {
        return;
    }

    gPwmRegs[u16PwmId]->CMPB.bit.CMPB = u16CmpB;
}

/* 运行期强制A/B通道输出电平。 */
void Hw_PwmSetForce(Uint16 u16PwmId, Uint16 u16ForceA, Uint16 u16ForceB)
{
    if (Hw_PwmIdIsValid(u16PwmId) == 0U)
    {
        return;
    }

    EALLOW;
    gPwmRegs[u16PwmId]->AQCSFRC.bit.CSFA = u16ForceA;
    gPwmRegs[u16PwmId]->AQCSFRC.bit.CSFB = u16ForceB;
    EDIS;
}

/* 释放A/B通道强制。 */
void Hw_PwmReleaseForce(Uint16 u16PwmId)
{
    Hw_PwmSetForce(u16PwmId, HW_PWM_FORCE_NONE, HW_PWM_FORCE_NONE);
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

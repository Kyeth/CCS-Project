/**
 * @file PvStateMachine.c
 * @brief PV Boost 通道状态机 + 电压环 / 电流环 + 扰动观察 MPPT（10kHz）
 *
 * 控制链（每个通道，10kHz 一次）：
 *   1. 采样：PV 电压/电流各做 8 点滑动平均（EP5K 用 Filter8，等价）
 *   2. MPPT（仅 MPPT 态）：扰动电压指令 fVset，判据是**半个扰动周期的功率累加值**
 *   3. 电压指令斜坡：fVsetRamp 以 600V/s 逼近 fVset
 *   4. 电压环 PI：Ref=实测电压、Fbk=电压指令 → 输出电流指令 fIref（负反馈约定）
 *   5. 电流环 PI：Ref=fIref、Fbk=实测电流 → 输出占空比小数
 *   6. 占空比 = (电流环输出 + 前馈 M) × TBPRD，母线过压时按 EP5K BusDutyLimit 再压一次
 */
#include "PvStateMachine.h"
#include "AdcSample.h"
#include "AdcCalib.h"
#include "Hw_PwmDriver.h"

ST_PV_SM gPvSm;

/* 8 点滑动平均（每个通道一份缓冲与累加和，避免每次重算） */
static float  sVpvBuf[PV_SM_CH_NUM][8];
static float  sIpvBuf[PV_SM_CH_NUM][8];
static float  sVpvSum[PV_SM_CH_NUM];
static float  sIpvSum[PV_SM_CH_NUM];
static Uint16 sVpvIdx[PV_SM_CH_NUM];
static Uint16 sIpvIdx[PV_SM_CH_NUM];
static float  sVrawSum[PV_SM_CH_NUM];     /* 电压 10 拍块累加（10kHz/10 → 1kHz） */
static Uint16 sVdivCnt[PV_SM_CH_NUM];

/* 各通道采样量指针（AdcSample 20kHz 瞬时层） */
static const volatile float *sPvVoltPtr[PV_SM_CH_NUM];
static const volatile float *sPvCurPtr[PV_SM_CH_NUM];

/******************************************************************************
 * 基础工具
 ******************************************************************************/
static float PvSmSat(float fVal, float fHi, float fLo)
{
    if (fVal > fHi)
    {
        return fHi;
    }
    if (fVal < fLo)
    {
        return fLo;
    }
    return fVal;
}

static float PvSmAbs(float fVal)
{
    return (fVal >= 0.0f) ? fVal : -fVal;
}

/* 与 EP5K CNTL_PI_F_MACRO 逐行等价的浮点 PI */
static void PvSmPiInit(ST_PV_PI *pstPi, float fKp, float fKi, float fUmax, float fUmin)
{
    pstPi->fRef = 0.0f;
    pstPi->fFbk = 0.0f;
    pstPi->fOut = 0.0f;
    pstPi->fKp = fKp;
    pstPi->fKi = fKi;
    pstPi->fUmax = fUmax;
    pstPi->fUmin = fUmin;
    pstPi->fUp = 0.0f;
    pstPi->fUi = 0.0f;
    pstPi->fV1 = 0.0f;
    pstPi->fI1 = 0.0f;
}

static void PvSmPiRun(ST_PV_PI *pstPi)
{
    /* 比例项 */
    pstPi->fUp = (pstPi->fRef - pstPi->fFbk) * pstPi->fKp;
    /* 积分项（输出饱和时冻结积分，与 EP5K 宏的 (Out==V1) 判据一致） */
    pstPi->fUi = (pstPi->fOut == pstPi->fV1) ? ((pstPi->fKi * pstPi->fUp) + pstPi->fI1) : pstPi->fOut;
    pstPi->fI1 = pstPi->fUi;
    /* 控制输出 */
    pstPi->fV1 = pstPi->fUp + pstPi->fUi;
    pstPi->fOut = (pstPi->fV1 > pstPi->fUmax) ? pstPi->fUmax : pstPi->fV1;
    pstPi->fOut = (pstPi->fOut < pstPi->fUmin) ? pstPi->fUmin : pstPi->fOut;
}

/* 8 点滑动平均（返回最新平均值） */
static float PvSmAvg8(float *pfBuf, float *pfSum, Uint16 *pu16Idx, float fNew)
{
    Uint16 u16Idx = *pu16Idx;

    *pfSum -= pfBuf[u16Idx];
    pfBuf[u16Idx] = fNew;
    *pfSum += fNew;

    u16Idx++;
    if (u16Idx >= 8U)
    {
        u16Idx = 0U;
    }
    *pu16Idx = u16Idx;

    return (*pfSum) * 0.125f;
}

/* 把占空比下发到对应 ePWM 的 CMPA（CMPA=0 即无脉冲） */
static void PvSmApplyDuty(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    Uint16 u16Cmp;

    u16Cmp = (Uint16)(pstCh->fDuty * (float)PV_SM_TBPRD);
    /* ZRO 置高、CAU 置低：CMPA=0 等于整周期为高（100% 占空比），必须留 1 个计数当“关” */
    if (u16Cmp < 1U)
    {
        u16Cmp = 1U;
    }
    Hw_PwmSetCmpA(gPvSm.u16PwmId[u16Ch], u16Cmp);

    /* PV 四路模块在 GLDCFG 里打开了 CMPA/AQCSFRC 的影子装载，
     * 写完 CMPA 必须触发一次全局装载，否则占空比进不了硬件 */
    Hw_TrigGlobalLoad();

    pstCh->u16DutyPermil = (Uint16)(pstCh->fDuty * 1000.0f);
}

/* 停机：占空比清 0，环路复位 */
static void PvSmStop(Uint16 u16Ch, Uint16 u16Fault)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];

    pstCh->fDuty = 0.0f;
    pstCh->fIref = 0.0f;
    pstCh->u16MpptCnt = 0U;
    pstCh->u16DelayCnt = 0U;
    pstCh->u16FastDone = 0U;
    pstCh->u16ErrCnt = 0U;
    pstCh->fPowerNew = 0.0f;
    pstCh->fPowerOld = 0.0f;
    pstCh->fPowerdelta = 0.0f;
    pstCh->fDisturbStep = PV_SM_MPPT_STEP_MAX;
    pstCh->u16DisturbDir = 1U;
    pstCh->u16Cnt = 0U;
    pstCh->u16Fault |= u16Fault;
    pstCh->u16State = (u16Fault == (Uint16)PV_SM_FAULT_NONE) ? (Uint16)PV_ST_IDLE : (Uint16)PV_ST_FAULT;
    PvSmApplyDuty(u16Ch);
}

/******************************************************************************
 * 通道启动：初始化电压指令与两个 PI
 ******************************************************************************/
static void PvSmStart(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    float fVsetInit;

    /* EP5K FastMppt：首次扰动 = 实测电压 - FastStep，且不能低于 MpptOutMin */
    fVsetInit = pstCh->fVpv - PV_SM_FAST_STEP;
    fVsetInit = PvSmSat(fVsetInit, PV_SM_VOLT_MAX, PV_SM_VOLT_MIN);

    pstCh->fVset = fVsetInit;
    pstCh->fVsetRamp = pstCh->fVpv;     /* 从当前实测值开始斜坡，避免起步冲击 */
    pstCh->fVsetStart = fVsetInit;
    pstCh->fDisturbStep = PV_SM_MPPT_STEP_MAX;
    pstCh->u16DisturbDir = 1U;
    pstCh->u16MpptCnt = 0U;
    pstCh->u16DelayCnt = 0U;
    pstCh->u16FastDone = 0U;
    pstCh->u16ErrCnt = 0U;
    pstCh->fPowerNew = 0.0f;
    pstCh->fPowerOld = 0.0f;
    pstCh->fPowerdelta = 0.0f;

    /* 电压环输出为电流指令，上限按本机额定；下限 0（不做负电流） */
    PvSmPiInit(&pstCh->stVoltPi, PV_SM_VOLT_KP, PV_SM_VOLT_KI, PV_SM_VOLT_UMAX, 0.0f);
    /* 电流环输出为占空比修正量，上下限沿用 EP5K */
    PvSmPiInit(&pstCh->stCurrPi, PV_SM_CURR_KP, PV_SM_CURR_KI, PV_SM_CURR_UMAX, PV_SM_CURR_UMIN);
    /* 积分器预置到 -M，使起步占空比约等于前馈值（EP5K 的 CurrPIF.i1 = -M0） */
    pstCh->stCurrPi.fOut = -pstCh->fM;
    pstCh->stCurrPi.fI1 = -pstCh->fM;
    pstCh->stCurrPi.fV1 = -pstCh->fM;

    pstCh->u16Fault = PV_SM_FAULT_NONE;
    pstCh->u16Cnt = 0U;
    pstCh->u16State = (Uint16)PV_ST_SOFT;

    /* 软启从第一档占空比开始 */
    pstCh->fDuty = PV_SM_CURR_UMAX + pstCh->fM;
    pstCh->fDuty = PvSmSat(pstCh->fDuty, PV_SM_DUTY_MAX, 0.0f);
    PvSmApplyDuty(u16Ch);
}

/******************************************************************************
 * MPPT：扰动观察（对应 EP5K NormalMppt）
 * 返回 1 表示本拍更新了电压指令
 ******************************************************************************/
static void PvSmMpptRun(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    float fStepMax;

    /* EP5K：扰动步长上限随 PV 电压分档（低压段细步长，高压段 5V） */
    if (pstCh->fVpv <= 175.0f)
    {
        fStepMax = 2.0f;
    }
    else if (pstCh->fVpv <= 200.0f)
    {
        fStepMax = 3.0f;
    }
    else
    {
        fStepMax = PV_SM_MPPT_STEP_MAX;
    }

    pstCh->u16MpptCnt++;
    if (pstCh->u16MpptCnt > (Uint16)PV_SM_MPPT_PERIOD)
    {
        pstCh->u16MpptCnt = 0U;
    }

    /* 每半个周期的后半段累加 ∑(V×I)，与上一窗口比较——这就是“一段时间内的功率均值” */
    if ((pstCh->u16MpptCnt > (Uint16)PV_SM_MPPT_HALF) && (pstCh->u16MpptCnt <= (Uint16)PV_SM_MPPT_PERIOD))
    {
        pstCh->fPowerNew += (pstCh->fVpv * pstCh->fIpv);
    }
    else
    {
        pstCh->fPowerNew = 0.0f;
    }

    if (pstCh->u16MpptCnt == (Uint16)PV_SM_MPPT_PERIOD)
    {
        /* EP5K BusOVFlag：母线过压时只往“升压、减小功率”方向扰动 */
        if (stSampleAvg.fBusVoltAvg200 > PV_SM_BUS_DUTY_REF)
        {
            pstCh->u16DisturbDir = 1U;
        }

        if (pstCh->fPowerNew < pstCh->fPowerOld)
        {
            /* 功率变小：反向，并把步长收敛 */
            pstCh->u16DisturbDir = (pstCh->u16DisturbDir == 0U) ? 1U : 0U;
            pstCh->fDisturbStep = pstCh->fDisturbStep * 0.6f;
        }
        else
        {
            /* 功率变大：同向，步长加大 */
            pstCh->fDisturbStep += PV_SM_MPPT_STEP_VOLT;
        }

        pstCh->fPowerdelta = pstCh->fPowerNew - pstCh->fPowerOld;

        /* 功率变化不到 1%，认为已在峰值附近，用最小步长试探 */
        if (pstCh->fPowerdelta < (0.01f * pstCh->fPowerNew))
        {
            pstCh->fDisturbStep = PV_SM_MPPT_STEP_MIN;
        }
        /* 电流很小（基本空载）时用最大步长快速找点 */
        if (pstCh->fIpv < 1.0f)
        {
            pstCh->fDisturbStep = fStepMax;
        }
        pstCh->fDisturbStep = PvSmSat(pstCh->fDisturbStep, fStepMax, PV_SM_MPPT_STEP_MIN);

        /* 扰动电压指令 */
        if (pstCh->u16DisturbDir == 0U)
        {
            pstCh->fVset -= pstCh->fDisturbStep;
        }
        else
        {
            pstCh->fVset += pstCh->fDisturbStep;
        }

        /* 指令与实测电压差得太多，说明跟踪失败，拉回实测值附近 */
        if (PvSmAbs(pstCh->fVset - pstCh->fVpv) >= PV_SM_MPPT_STEP_MAX)
        {
            pstCh->u16ErrCnt++;
            if (pstCh->u16ErrCnt >= 3U)
            {
                pstCh->u16ErrCnt = 0U;
                pstCh->fVset = pstCh->fVpv - PV_SM_MPPT_STEP_MAX;
            }
        }
        else
        {
            pstCh->u16ErrCnt = 0U;
        }

        /* 电压指令必须比母线低一截（升压电路无法把 PV 电压推到母线之上） */
        if (pstCh->fVset > (stSampleAvg.fBusVoltAvg200 - PV_SM_BUS_HEAD))
        {
            pstCh->fVset = stSampleAvg.fBusVoltAvg200 - PV_SM_BUS_HEAD;
        }
        pstCh->fVset = PvSmSat(pstCh->fVset, PV_SM_VOLT_MAX, PV_SM_VOLT_MIN);

        pstCh->fPowerOld = pstCh->fPowerNew;
    }
}

/******************************************************************************
 * 电压环 + 电流环 + 占空比输出
 ******************************************************************************/
static void PvSmLoopRun(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    float fBus = stSampleAvg.fBusVoltAvg200;
    float fDutyFrac;
    float fDutyOfs;
    float fBusLimit;
    float fMTarget;
    float fKpCurr;
    float fBusClamp;

    /* ---- 前馈偏置 M = 1 - Vpv/Vbus（EP5K 的 M0/M1） ---- */
    /* EP5K: BusAdptCoeff = 1/clamp(Vbus, 460, 300)，先把母线夹到工作区再算变比 */
    fBusClamp = PvSmSat(fBus, PV_SM_BUS_DUTY_REF, 300.0f);
    fMTarget = 0.0f;
    if (fBusClamp > 1.0f)
    {
        fMTarget = 1.0f - (pstCh->fVpv / fBusClamp);
    }
    fMTarget = PvSmSat(fMTarget, PV_SM_DUTY_MAX, 0.0f);
    /* EP5K：M 以 ±0.01/拍 的速率跟随目标，避免母线跳变时占空比突跳 */
    if (fMTarget > (pstCh->fM + PV_SM_M_SLEW))
    {
        pstCh->fM += PV_SM_M_SLEW;
    }
    else if (fMTarget < (pstCh->fM - PV_SM_M_SLEW))
    {
        pstCh->fM -= PV_SM_M_SLEW;
    }
    else
    {
        pstCh->fM = fMTarget;
    }

    /* ---- 电压指令斜坡（EP5K RmpCntlCalc：600V/s） ---- */
    if (pstCh->fVset > (pstCh->fVsetRamp + PV_SM_VSET_RAMP))
    {
        pstCh->fVsetRamp += PV_SM_VSET_RAMP;
    }
    else if (pstCh->fVset < (pstCh->fVsetRamp - PV_SM_VSET_RAMP))
    {
        pstCh->fVsetRamp -= PV_SM_VSET_RAMP;
    }
    else
    {
        pstCh->fVsetRamp = pstCh->fVset;
    }

    /* ---- 电压环：Ref=实测电压，Fbk=电压指令 → 电流指令 ---- */
    /* EP5K 母线过压分支：切到慢快不同的集——Ki 降到 1/100，以母线 460V 为对象 */
    if (fBus > PV_SM_BUS_DUTY_REF)
    {
        pstCh->stVoltPi.fKi = PV_SM_VOLT_KI_BUS;
        pstCh->stVoltPi.fRef = PV_SM_BUS_DUTY_REF * 10.0f;
        pstCh->stVoltPi.fFbk = fBus * 10.0f;
        pstCh->u16BusOv = 1U;
    }
    else
    {
        pstCh->stVoltPi.fKi = PV_SM_VOLT_KI;
        pstCh->stVoltPi.fRef = pstCh->fVpv;
        pstCh->stVoltPi.fFbk = pstCh->fVsetRamp;
        pstCh->u16BusOv = 0U;
    }
    PvSmPiRun(&pstCh->stVoltPi);
    pstCh->fIref = pstCh->stVoltPi.fOut;

    /* ---- 电流环：Ref=电流指令，Fbk=实测电流 → 占空比修正 ---- */
    pstCh->stCurrPi.fRef = pstCh->fIref;
    pstCh->stCurrPi.fFbk = pstCh->fIpv;
    pstCh->stCurrPi.fUmin = -0.1f - pstCh->fM;
    /* EP5K：电流环增益随电流指令下降（大电流时 Kp 减半），防止高功率段抖动 */
    fKpCurr = 1.5f - (pstCh->fIref * (1.2f / PV_SM_IPV_MAX));
    fKpCurr = PvSmSat(fKpCurr, 1.0f, 0.5f);
    pstCh->stCurrPi.fKp = PV_SM_CURR_KP * fKpCurr;
    /* EP5K 低压大电流段：M 很大、电流很小且电流环输出为负时，切慢增益避免振荡 */
    if ((pstCh->stCurrPi.fOut < (-0.1f * pstCh->fM)) && (pstCh->fM > 0.1f) && (pstCh->fIref < 3.5f))
    {
        pstCh->stCurrPi.fKp = PV_SM_CURR_KP_ADAPT;
        pstCh->stVoltPi.fKp = PV_SM_VOLT_KP_ADAPT;
    }
    else
    {
        pstCh->stVoltPi.fKp = PV_SM_VOLT_KP;
    }
    PvSmPiRun(&pstCh->stCurrPi);
    fDutyFrac = pstCh->stCurrPi.fOut;

    /* ---- 占空比 ---- */
    fDutyOfs = (fDutyFrac + pstCh->fM) * (float)PV_SM_TBPRD;
    /* EP5K 的母线过压限占空比（BusDutyLimit）：母线越高，允许的占空比越小 */
    fBusLimit = 1.0f - (0.1f * (fBus - (PV_SM_BUS_DUTY_REF + 3.0f)));
    fBusLimit = PvSmSat(fBusLimit, 1.0f, 0.0f);
    if (fBusLimit > PV_SM_DUTY_MAX)
    {
        fBusLimit = PV_SM_DUTY_MAX;
    }
    /* 先按“计数”限幅，再换算回 0~1 的占空比 */
    pstCh->fDuty = PvSmSat(fDutyOfs, (float)PV_SM_TBPRD * fBusLimit, 0.0f);
    pstCh->fDuty = pstCh->fDuty / (float)PV_SM_TBPRD;
    PvSmApplyDuty(u16Ch);
}

/******************************************************************************
 * 运行态故障判据
 ******************************************************************************/
static void PvSmCheckFault(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    float fBus = stSampleAvg.fBusVoltAvg200;

    if (pstCh->fIpv > PV_SM_OC_A)
    {
        PvSmStop(u16Ch, PV_SM_FAULT_OC);
    }
    else if (pstCh->fIpv < PV_SM_REVERSE_A)
    {
        PvSmStop(u16Ch, PV_SM_FAULT_REVERSE);
    }
    else if (fBus < (pstCh->fVpv + PV_SM_BUS_MARGIN))
    {
        PvSmStop(u16Ch, PV_SM_FAULT_BUS);
    }
    else if ((pstCh->u16State == (Uint16)PV_ST_MPPT) && (pstCh->fVpv < PV_SM_VOLT_MIN))
    {
        PvSmStop(u16Ch, PV_SM_FAULT_UV);
    }
    else
    {
        /* 无故障 */
    }
}

/******************************************************************************
 * 单通道 10kHz 节拍
 ******************************************************************************/
static void PvSmChannel(Uint16 u16Ch)
{
    ST_PV_SM_CH *pstCh = &gPvSm.stCh[u16Ch];
    float fBus = stSampleAvg.fBusVoltAvg200;
    Uint16 u16BusOk;

    /* 电压与电流的窗口宽度不同，与 EP5K 保持一致：
     *   电压：10 拍块平均(1kHz) + 8 点滑窗 ≈ 8ms，对齐 EP5K PVxVoltFilter8
     *         （其实际窗口是 PeriodPoint>>2 ≈ 7.5ms，不是名字里的 8 点）；
     *   电流：8 点滑窗 ≈ 0.8ms，对齐 EP5K PVxCurrFilter8（8 拍 1.2ms）。 */
    sVrawSum[u16Ch] += *sPvVoltPtr[u16Ch];
    if (++sVdivCnt[u16Ch] >= (Uint16)PV_SM_VOLT_DIV)
    {
        sVdivCnt[u16Ch] = 0U;
        pstCh->fVpv = PvSmAvg8(sVpvBuf[u16Ch], &sVpvSum[u16Ch], &sVpvIdx[u16Ch],
                               sVrawSum[u16Ch] * (1.0f / (float)PV_SM_VOLT_DIV));
        sVrawSum[u16Ch] = 0.0f;
    }
    pstCh->fIpv = PvSmAvg8(sIpvBuf[u16Ch], &sIpvSum[u16Ch], &sIpvIdx[u16Ch], *sPvCurPtr[u16Ch]);
    pstCh->fPower = pstCh->fVpv * pstCh->fIpv;
    pstCh->u16Present = ((pstCh->fVpv > PV_SM_VOLT_MIN) && (pstCh->fVpv <= PV_SM_VOLT_MAX)) ? 1U : 0U;
    u16BusOk = (fBus > (pstCh->fVpv + PV_SM_BUS_MARGIN)) ? 1U : 0U;

    if (pstCh->u16Cnt < 0xFFFFU)
    {
        pstCh->u16Cnt++;
    }

    switch (pstCh->u16State)
    {
        case (Uint16)PV_ST_IDLE:
            pstCh->fDuty = 0.0f;
            PvSmApplyDuty(u16Ch);
            if ((gPvSm.u16Enable != 0U) && (pstCh->u16Present != 0U) && (u16BusOk != 0U))
            {
                PvSmStart(u16Ch);
            }
            break;

        case (Uint16)PV_ST_SOFT:
            /* 软启：环路已投入，等 1.5s 后开始扰动（EP5K DelayCnt=15000） */
            if (pstCh->u16DelayCnt < 0xFFFFU)
            {
                pstCh->u16DelayCnt++;
            }
            PvSmLoopRun(u16Ch);
            PvSmCheckFault(u16Ch);
            if ((pstCh->u16State == (Uint16)PV_ST_SOFT) && (pstCh->u16DelayCnt >= (Uint16)PV_SM_DELAY_CNT))
            {
                pstCh->u16DelayCnt = 0U;
                pstCh->u16MpptCnt = 0U;
                pstCh->fPowerOld = 0.0f;
                pstCh->fVset = pstCh->fVpv - PV_SM_FAST_STEP;
                pstCh->fVset = PvSmSat(pstCh->fVset, PV_SM_VOLT_MAX, PV_SM_VOLT_MIN);
                pstCh->u16State = (Uint16)PV_ST_MPPT;
            }
            break;

        case (Uint16)PV_ST_MPPT:
            if (pstCh->u16Present == 0U)
            {
                PvSmStop(u16Ch, PV_SM_FAULT_NONE);
            }
            else
            {
                PvSmMpptRun(u16Ch);
                PvSmLoopRun(u16Ch);
                PvSmCheckFault(u16Ch);
            }
            break;

        case (Uint16)PV_ST_FAULT:
            pstCh->fDuty = 0.0f;
            PvSmApplyDuty(u16Ch);
            if ((pstCh->u16Cnt >= (Uint16)(PV_SM_FAULT_RETRY_TICK & 0xFFFFU)) &&
                (pstCh->u16Present != 0U) && (u16BusOk != 0U))
            {
                pstCh->u16Fault = PV_SM_FAULT_NONE;
                pstCh->u16Cnt = 0U;
                PvSmStart(u16Ch);
            }
            break;

        default:
            PvSmStop(u16Ch, PV_SM_FAULT_NONE);
            break;
    }
}

/******************************************************************************
 * 初始化
 ******************************************************************************/
void PvSmInit(void)
{
    Uint16 i;
    Uint16 j;

    gPvSm.u16Enable = 1U;
    gPvSm.u16RunMask = 0U;
    gPvSm.u16MpptMask = 0U;

    /* 通道映射：PV1=EPWM7(ID6)、PV2=EPWM9(ID8)、PV3=EPWM5(ID4)、PV4=EPWM6(ID5) */
    gPvSm.u16PwmId[0] = (Uint16)HW_PWM_ID6;
    gPvSm.u16PwmId[1] = (Uint16)HW_PWM_ID8;
    gPvSm.u16PwmId[2] = (Uint16)HW_PWM_ID4;
    gPvSm.u16PwmId[3] = (Uint16)HW_PWM_ID5;

    sPvVoltPtr[0] = &stSampleIns.fPvVoltIns[SAMPLE_PV1];
    sPvVoltPtr[1] = &stSampleIns.fPvVoltIns[SAMPLE_PV2];
    sPvVoltPtr[2] = &stSampleIns.fPvVoltIns[SAMPLE_PV3];
    sPvVoltPtr[3] = &stSampleIns.fPvVoltIns[SAMPLE_PV4];

    sPvCurPtr[0] = &stSampleIns.fPvCurrIns[SAMPLE_PV1];
    sPvCurPtr[1] = &stSampleIns.fPvCurrIns[SAMPLE_PV2];
    sPvCurPtr[2] = &stSampleIns.fPvCurrIns[SAMPLE_PV3];
    sPvCurPtr[3] = &stSampleIns.fPvCurrIns[SAMPLE_PV4];

    for (i = 0U; i < (Uint16)PV_SM_CH_NUM; i++)
    {
        for (j = 0U; j < 8U; j++)
        {
            sVpvBuf[i][j] = 0.0f;
            sIpvBuf[i][j] = 0.0f;
        }
        sVpvSum[i] = 0.0f;
        sIpvSum[i] = 0.0f;
        sVpvIdx[i] = 0U;
        sIpvIdx[i] = 0U;
        sVrawSum[i] = 0.0f;
        sVdivCnt[i] = 0U;

        gPvSm.stCh[i].u16State = (Uint16)PV_ST_IDLE;
        gPvSm.stCh[i].u16Fault = PV_SM_FAULT_NONE;
        gPvSm.stCh[i].u16Present = 0U;
        gPvSm.stCh[i].u16Cnt = 0U;
        gPvSm.stCh[i].u16DutyPermil = 0U;
        gPvSm.stCh[i].u16MpptCnt = 0U;
        gPvSm.stCh[i].u16DelayCnt = 0U;
        gPvSm.stCh[i].u16FastDone = 0U;
        gPvSm.stCh[i].u16ErrCnt = 0U;
        gPvSm.stCh[i].fVpv = 0.0f;
        gPvSm.stCh[i].fIpv = 0.0f;
        gPvSm.stCh[i].fVset = 0.0f;
        gPvSm.stCh[i].fVsetRamp = 0.0f;
        gPvSm.stCh[i].fVsetStart = 0.0f;
        gPvSm.stCh[i].fIref = 0.0f;
        gPvSm.stCh[i].fDuty = 0.0f;
        gPvSm.stCh[i].fM = 0.0f;
        gPvSm.stCh[i].fPowerNew = 0.0f;
        gPvSm.stCh[i].fPowerOld = 0.0f;
        gPvSm.stCh[i].fPower = 0.0f;
        gPvSm.stCh[i].fPowerdelta = 0.0f;
        gPvSm.stCh[i].fDisturbStep = PV_SM_MPPT_STEP_MAX;
        gPvSm.stCh[i].u16DisturbDir = 1U;

        PvSmPiInit(&gPvSm.stCh[i].stVoltPi, PV_SM_VOLT_KP, PV_SM_VOLT_KI, PV_SM_VOLT_UMAX, 0.0f);
        PvSmPiInit(&gPvSm.stCh[i].stCurrPi, PV_SM_CURR_KP, PV_SM_CURR_KI, PV_SM_CURR_UMAX, PV_SM_CURR_UMIN);

        PvSmApplyDuty(i);
    }
}

void PvSmSetEnable(Uint16 u16Enable)
{
    gPvSm.u16Enable = (u16Enable != 0U) ? 1U : 0U;
}

Uint16 PvSmGetEnable(void)
{
    return gPvSm.u16Enable;
}

/******************************************************************************
 * 主任务：10kHz
 ******************************************************************************/
void PvSmTask(void *pArgV)
{
    Uint16 i;
    Uint16 u16RunMask = 0U;
    Uint16 u16MpptMask = 0U;

    (void)pArgV;

    /* 零漂标定没完成前不升压：零点未定，功率判据不可信 */
    if (AdcCalibGetZeroState() != (Uint16)ADC_CAL_ZERO_DONE)
    {
        gPvSm.u16RunMask = 0U;
        gPvSm.u16MpptMask = 0U;
        return;
    }

    for (i = 0U; i < (Uint16)PV_SM_CH_NUM; i++)
    {
        PvSmChannel(i);

        if ((gPvSm.stCh[i].u16State == (Uint16)PV_ST_SOFT) ||
            (gPvSm.stCh[i].u16State == (Uint16)PV_ST_MPPT))
        {
            u16RunMask |= (Uint16)(1U << i);
        }
        if (gPvSm.stCh[i].u16State == (Uint16)PV_ST_MPPT)
        {
            u16MpptMask |= (Uint16)(1U << i);
        }
    }

    gPvSm.u16RunMask = u16RunMask;
    gPvSm.u16MpptMask = u16MpptMask;
}

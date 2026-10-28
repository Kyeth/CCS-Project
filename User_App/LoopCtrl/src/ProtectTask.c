/**
 * @file ProtectTask.c
 * @author yetingheng (004182)
 * @brief 10kHz 硬件过流/短路判定任务实现
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 */

#include "ProtectTask.h"
#include "Hw_ExitDriver.h"
#include "Hw_InvDriver.h"
#include "CtrlIsr.h"
#include "User_ClaTask.h"
#include "AdcCalib.h"
#include "SmUtil.h"

/* 逆变过流（GPIO81 -> INPUT4 -> XINT1）漏桶状态 */
static Uint16 gInvOcCnt;
static Uint16 gInvOcCleanTick;

/* 硬件短路（GPIO82 -> INPUT5 -> XINT2）窗口状态 */
static Uint16 gShortCnt;
static Uint16 gShortWindowTick;

/* 半波/直流分量自适应状态 */
static float  s_fInvDcAvg[3];        /* 三相直流分量 EMA 状态（A） */
static Uint16 s_u16HalfWaveJudge;    /* 连续超限拍数 */
static Uint16 s_u16HalfWaveMode;     /* 1：半波自适应模式生效 */

/* 故障锁存与运行拍数 */
static Uint16 gProtectFaultLatch;

/* 只读镜像（Modbus 点表直接取地址） */
volatile Uint16 gProtectInvOcCnt     = 0U;
volatile Uint16 gProtectShortCnt     = 0U;
volatile Uint16 gProtectFaultFlags   = 0U;
volatile Uint16 gProtectTick         = 0U;
volatile Uint16 gProtectHalfWaveMode = 0U;
volatile Uint16 gProtectHalfWaveJudge= 0U;
volatile int16  gProtectInvDc[3]     = {0, 0, 0};

/* 累加并饱和，避免 16 位回绕把漏桶清零 */
static Uint16 ProtectSatAdd(Uint16 u16Base, Uint16 u16Add)
{
    Uint16 u16Sum;

    u16Sum = (Uint16)(u16Base + u16Add);

    if (u16Sum < u16Base)
    {
        u16Sum = 0xFFFFU;
    }

    return u16Sum;
}

/* 半波/直流分量检测（移植自 EP5K Funtion.c 的 INVAVG 判据）。
 * bUseExit=1 时用退出滞回门限，供"已在半波模式里"的保持判断使用。 */
static Uint16 ProtectHalfWaveOver(Uint16 bUseExit)
{
    Uint16 u16Idx;
    Uint16 bOver;
    float  fLim;
    float  fAbs;
    float  fA;
    float  fCur;

    bOver = 0U;
    fLim  = (bUseExit != 0U) ? PROTECT_HALF_WAVE_EXIT_A : PROTECT_HALF_WAVE_DC_A;

    for (u16Idx = 0U; u16Idx < 3U; u16Idx++)
    {
        /* 逆变电流由 AdcCalib 换算成安培（已扣零漂、已乘系数） */
        fCur = AdcCalibInvCur(u16Idx);

        /* 慢变直流分量：对瞬时电流做极慢 EMA（α=0.001 @10kHz → τ≈100ms） */
        s_fInvDcAvg[u16Idx] += PROTECT_INV_DC_ALPHA
                             * (fCur - s_fInvDcAvg[u16Idx]);

        /* 0.1A 只读镜像，供上位机观察与标定 */
        fA = s_fInvDcAvg[u16Idx] * 10.0f;
        if (fA > 32767.0f)  { fA = 32767.0f; }
        if (fA < -32768.0f) { fA = -32768.0f; }
        gProtectInvDc[u16Idx] = (int16)fA;

        fAbs = (s_fInvDcAvg[u16Idx] < 0.0f) ? -s_fInvDcAvg[u16Idx] : s_fInvDcAvg[u16Idx];
        if (fAbs >= fLim)
        {
            bOver = 1U;
        }
    }

    return bOver;
}

/* 锁存故障并立即封波。 */
static void ProtectLatchFault(Uint16 u16Fault)
{
    gProtectFaultLatch = (Uint16)(gProtectFaultLatch | u16Fault);
    gProtectFaultFlags = gProtectFaultLatch;

    /* 三相桥臂立刻回到中点钳位 */
    Hw_InvStopAll();

    /* TZFRC.OST 把输出锁死在 TZCTL 指定的低电平，
     * 即使控制中断继续跑也不会重新出波 */
    Hw_InvTripAll();

    /* 把故障位同步给 20kHz 控制中断，避免它下一拍又把占空比写回去 */
    if ((u16Fault & PROTECT_FAULT_SHORT) != 0U)
    {
        CtrlSetFault((Uint16)CTRL_FAULT_SHORT);
    }
    else
    {
        CtrlSetFault((Uint16)CTRL_FAULT_HW_OC);
    }
}

/* 初始化：清所有计数与锁存。 */
void ProtectTaskInit(void)
{
    gInvOcCnt = 0U;
    gInvOcCleanTick = 0U;
    gShortCnt = 0U;
    gShortWindowTick = 0U;
    gProtectFaultLatch = (Uint16)PROTECT_FAULT_NONE;
    gProtectTick = 0U;

    s_u16HalfWaveMode  = 0U;
    s_u16HalfWaveJudge = 0U;
    s_fInvDcAvg[0] = 0.0f;
    s_fInvDcAvg[1] = 0.0f;
    s_fInvDcAvg[2] = 0.0f;

    gProtectInvOcCnt = 0U;
    gProtectShortCnt = 0U;
    gProtectFaultFlags = 0U;
    gProtectHalfWaveMode = 0U;
    gProtectHalfWaveJudge = 0U;
    gProtectInvDc[0] = 0;
    gProtectInvDc[1] = 0;
    gProtectInvDc[2] = 0;

    Hw_ExitClearAll();
}

/* 10kHz 判定：由 20kHz 控制中断 ÷2 驱动。 */
void RunProtectTask(void)
{
    Uint16 u16OcEvt;
    Uint16 u16ShortEvt;
    Uint16 u16TripCnt;
    Uint16 u16CleanTick;

    gProtectTick++;

    /* 取走本拍新增的比较器事件数（中断里只做累加） */
    u16OcEvt    = Hw_ExitTakeCount((Uint16)HW_XINT_ID1);
    u16ShortEvt = Hw_ExitTakeCount((Uint16)HW_XINT_ID2);

    /* ---------- 0. 半波/偏磁判定：决定本拍是否启用更灵敏的判据 ----------
     * 只在尚未锁故障、且 CLA 已经跑起来（u16Seq != 0）时评估，
     * 避免上电初期 PWM 未启动、fInvCur 还是初值时误判。 */
    if ((gProtectFaultLatch & (Uint16)(PROTECT_FAULT_INV_OC | PROTECT_FAULT_SHORT)) == 0U)
    {
        if (Cla1ToCpuMsg.u16Seq == 0U)
        {
            s_u16HalfWaveMode  = 0U;
            s_u16HalfWaveJudge = 0U;
        }
        else if (s_u16HalfWaveMode != 0U)
        {
            /* 已在半波模式：用滞回门限判断是否退出 */
            if (ProtectHalfWaveOver(1U) == 0U)
            {
                s_u16HalfWaveMode  = 0U;
                s_u16HalfWaveJudge = 0U;
            }
        }
        else if (ProtectHalfWaveOver(0U) != 0U)
        {
            if (s_u16HalfWaveJudge < (Uint16)PROTECT_HALF_WAVE_JUDGE_TICK)
            {
                s_u16HalfWaveJudge++;
            }

            if (s_u16HalfWaveJudge >= (Uint16)PROTECT_HALF_WAVE_JUDGE_TICK)
            {
                s_u16HalfWaveMode = 1U;
            }
        }
        else
        {
            s_u16HalfWaveJudge = 0U;
        }
    }

    gProtectHalfWaveMode  = s_u16HalfWaveMode;
    gProtectHalfWaveJudge = s_u16HalfWaveJudge;

    /* 本拍生效的过流判据（半波模式收紧到 150 次 / 15ms 恢复） */
    u16TripCnt   = (s_u16HalfWaveMode != 0U) ? (Uint16)PROTECT_INV_OC_TRIP_CNT_HW
                                             : (Uint16)PROTECT_INV_OC_TRIP_CNT;
    u16CleanTick = (s_u16HalfWaveMode != 0U) ? (Uint16)PROTECT_INV_OC_CLEAN_TICK_HW
                                             : (Uint16)PROTECT_INV_OC_CLEAN_TICK;
    /* ---------- 1. 逆变过流：漏桶 ---------- */
    if (u16OcEvt > 0U)
    {
        gInvOcCnt = ProtectSatAdd(gInvOcCnt, u16OcEvt);
        gInvOcCleanTick = 0U;
    }
    else if (gInvOcCnt > 0U)
    {
        gInvOcCleanTick++;

        if (gInvOcCleanTick >= u16CleanTick)
        {
            gInvOcCnt = 0U;
            gInvOcCleanTick = 0U;
        }
    }
    else
    {
        /* 空闲态，保持干净 */
        gInvOcCleanTick = 0U;
    }

    /* ---------- 2. 硬件短路：10ms 窗口内累计 ---------- */
    if (u16ShortEvt > 0U)
    {
        gShortCnt = ProtectSatAdd(gShortCnt, u16ShortEvt);
        gShortWindowTick = 0U;
    }
    else if (gShortWindowTick < (Uint16)PROTECT_SHORT_WINDOW_TICK)
    {
        gShortWindowTick++;
    }
    else
    {
        /* 窗口过期，重新计 */
        gShortCnt = 0U;
        gShortWindowTick = 0U;
    }

    /* ---------- 3. 判定 ---------- */
    if (((gProtectFaultLatch & (Uint16)PROTECT_FAULT_INV_OC) == 0U)
        && (gInvOcCnt >= u16TripCnt))
    {
        ProtectLatchFault((Uint16)PROTECT_FAULT_INV_OC);
    }

    if (((gProtectFaultLatch & (Uint16)PROTECT_FAULT_SHORT) == 0U)
        && (gShortCnt >= (Uint16)PROTECT_SHORT_TRIP_CNT))
    {
        ProtectLatchFault((Uint16)PROTECT_FAULT_SHORT);
    }

    /* ---------- 4. 刷新只读镜像 ---------- */
    gProtectInvOcCnt = gInvOcCnt;
    gProtectShortCnt = gShortCnt;

    /* ---------- 5. 状态机公共时基（1 拍 = 100us） ---------- */
    SmTick();
}

/* 查询故障锁存位。 */
Uint16 ProtectGetFaultFlags(void)
{
    return gProtectFaultLatch;
}

/* 清故障锁存，返回清除前的故障位。 */
Uint16 ProtectClearFault(void)
{
    Uint16 u16Old;

    u16Old = gProtectFaultLatch;

    gProtectFaultLatch = (Uint16)PROTECT_FAULT_NONE;
    gProtectFaultFlags = 0U;

    gInvOcCnt = 0U;
    gInvOcCleanTick = 0U;
    gShortCnt = 0U;
    gShortWindowTick = 0U;

    /* 半波模式保留（负载工况没变），只清判据计数，让它重新累计 */
    s_u16HalfWaveJudge = 0U;
    gProtectHalfWaveJudge = 0U;

    gProtectInvOcCnt = 0U;
    gProtectShortCnt = 0U;

    CtrlClearFault();

    return u16Old;
}

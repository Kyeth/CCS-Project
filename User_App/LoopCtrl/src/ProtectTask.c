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

/* 保护任务全部运行状态：单个实例 stProtectState，函数内一律经结构体访问 */
ST_PROTECT_TASK_STATE stProtectState;

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
        stProtectState.fInvDcAvg[u16Idx] += PROTECT_INV_DC_ALPHA
                             * (fCur - stProtectState.fInvDcAvg[u16Idx]);

        /* 0.1A 只读镜像，供上位机观察与标定 */
        fA = stProtectState.fInvDcAvg[u16Idx] * 10.0f;
        if (fA > 32767.0f)  { fA = 32767.0f; }
        if (fA < -32768.0f) { fA = -32768.0f; }
        stProtectState.i16InvDc[u16Idx] = (int16)fA;

        fAbs = (stProtectState.fInvDcAvg[u16Idx] < 0.0f) ? -stProtectState.fInvDcAvg[u16Idx] : stProtectState.fInvDcAvg[u16Idx];
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
    stProtectState.u16FaultLatch = (Uint16)(stProtectState.u16FaultLatch | u16Fault);

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
    stProtectState.u16InvOcCnt = 0U;
    stProtectState.u16InvOcCleanTick = 0U;
    stProtectState.u16ShortCnt = 0U;
    stProtectState.u16ShortWindowTick = 0U;
    stProtectState.u16FaultLatch = (Uint16)PROTECT_FAULT_NONE;
    stProtectState.u16Tick = 0U;

    stProtectState.u16HalfWaveMode  = 0U;
    stProtectState.u16HalfWaveJudge = 0U;
    stProtectState.fInvDcAvg[0] = 0.0f;
    stProtectState.fInvDcAvg[1] = 0.0f;
    stProtectState.fInvDcAvg[2] = 0.0f;
    stProtectState.i16InvDc[0] = 0;
    stProtectState.i16InvDc[1] = 0;
    stProtectState.i16InvDc[2] = 0;

    Hw_ExitClearAll();
}

/* 10kHz 判定：由 20kHz 控制中断 ÷2 驱动。 */
void RunProtectTask(void)
{
    Uint16 u16OcEvt;
    Uint16 u16ShortEvt;
    Uint16 u16TripCnt;
    Uint16 u16CleanTick;

    stProtectState.u16Tick++;

    /* 取走本拍新增的比较器事件数（中断里只做累加） */
    u16OcEvt    = Hw_ExitTakeCount((Uint16)HW_XINT_ID1);
    u16ShortEvt = Hw_ExitTakeCount((Uint16)HW_XINT_ID2);

    /* ---------- 0. 半波/偏磁判定：决定本拍是否启用更灵敏的判据 ----------
     * 只在尚未锁故障、且 CLA 已经跑起来（u16Seq != 0）时评估，
     * 避免上电初期 PWM 未启动、fInvCur 还是初值时误判。 */
    if ((stProtectState.u16FaultLatch & (Uint16)(PROTECT_FAULT_INV_OC | PROTECT_FAULT_SHORT)) == 0U)
    {
        if (Cla1ToCpuMsg.u16Seq == 0U)
        {
            stProtectState.u16HalfWaveMode  = 0U;
            stProtectState.u16HalfWaveJudge = 0U;
        }
        else if (stProtectState.u16HalfWaveMode != 0U)
        {
            /* 已在半波模式：用滞回门限判断是否退出 */
            if (ProtectHalfWaveOver(1U) == 0U)
            {
                stProtectState.u16HalfWaveMode  = 0U;
                stProtectState.u16HalfWaveJudge = 0U;
            }
        }
        else if (ProtectHalfWaveOver(0U) != 0U)
        {
            if (stProtectState.u16HalfWaveJudge < (Uint16)PROTECT_HALF_WAVE_JUDGE_TICK)
            {
                stProtectState.u16HalfWaveJudge++;
            }

            if (stProtectState.u16HalfWaveJudge >= (Uint16)PROTECT_HALF_WAVE_JUDGE_TICK)
            {
                stProtectState.u16HalfWaveMode = 1U;
            }
        }
        else
        {
            stProtectState.u16HalfWaveJudge = 0U;
        }
    }

    stProtectState.u16HalfWaveMode  = stProtectState.u16HalfWaveMode;
    stProtectState.u16HalfWaveJudge = stProtectState.u16HalfWaveJudge;

    /* 本拍生效的过流判据（半波模式收紧到 150 次 / 15ms 恢复） */
    u16TripCnt   = (stProtectState.u16HalfWaveMode != 0U) ? (Uint16)PROTECT_INV_OC_TRIP_CNT_HW
                                             : (Uint16)PROTECT_INV_OC_TRIP_CNT;
    u16CleanTick = (stProtectState.u16HalfWaveMode != 0U) ? (Uint16)PROTECT_INV_OC_CLEAN_TICK_HW
                                             : (Uint16)PROTECT_INV_OC_CLEAN_TICK;
    /* ---------- 1. 逆变过流：漏桶 ---------- */
    if (u16OcEvt > 0U)
    {
        stProtectState.u16InvOcCnt = ProtectSatAdd(stProtectState.u16InvOcCnt, u16OcEvt);
        stProtectState.u16InvOcCleanTick = 0U;
    }
    else if (stProtectState.u16InvOcCnt > 0U)
    {
        stProtectState.u16InvOcCleanTick++;

        if (stProtectState.u16InvOcCleanTick >= u16CleanTick)
        {
            stProtectState.u16InvOcCnt = 0U;
            stProtectState.u16InvOcCleanTick = 0U;
        }
    }
    else
    {
        /* 空闲态，保持干净 */
        stProtectState.u16InvOcCleanTick = 0U;
    }

    /* ---------- 2. 硬件短路：10ms 窗口内累计 ---------- */
    if (u16ShortEvt > 0U)
    {
        stProtectState.u16ShortCnt = ProtectSatAdd(stProtectState.u16ShortCnt, u16ShortEvt);
        stProtectState.u16ShortWindowTick = 0U;
    }
    else if (stProtectState.u16ShortWindowTick < (Uint16)PROTECT_SHORT_WINDOW_TICK)
    {
        stProtectState.u16ShortWindowTick++;
    }
    else
    {
        /* 窗口过期，重新计 */
        stProtectState.u16ShortCnt = 0U;
        stProtectState.u16ShortWindowTick = 0U;
    }

    /* ---------- 3. 判定 ---------- */
    if (((stProtectState.u16FaultLatch & (Uint16)PROTECT_FAULT_INV_OC) == 0U)
        && (stProtectState.u16InvOcCnt >= u16TripCnt))
    {
        ProtectLatchFault((Uint16)PROTECT_FAULT_INV_OC);
    }

    if (((stProtectState.u16FaultLatch & (Uint16)PROTECT_FAULT_SHORT) == 0U)
        && (stProtectState.u16ShortCnt >= (Uint16)PROTECT_SHORT_TRIP_CNT))
    {
        ProtectLatchFault((Uint16)PROTECT_FAULT_SHORT);
    }

    /* ---------- 5. 状态机公共时基（1 拍 = 100us） ---------- */
    SmTick();
}

/* 查询故障锁存位。 */
Uint16 ProtectGetFaultFlags(void)
{
    return stProtectState.u16FaultLatch;
}

/* 清故障锁存，返回清除前的故障位。 */
Uint16 ProtectClearFault(void)
{
    Uint16 u16Old;

    u16Old = stProtectState.u16FaultLatch;

    stProtectState.u16FaultLatch = (Uint16)PROTECT_FAULT_NONE;

    stProtectState.u16InvOcCnt = 0U;
    stProtectState.u16InvOcCleanTick = 0U;
    stProtectState.u16ShortCnt = 0U;
    stProtectState.u16ShortWindowTick = 0U;

    /* 半波模式保留（负载工况没变），只清判据计数，让它重新累计 */
    stProtectState.u16HalfWaveJudge = 0U;

    stProtectState.u16InvOcCnt = 0U;
    stProtectState.u16ShortCnt = 0U;

    CtrlClearFault();

    return u16Old;
}

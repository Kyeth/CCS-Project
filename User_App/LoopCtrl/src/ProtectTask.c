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

/* 逆变过流（GPIO81 -> INPUT4 -> XINT1）漏桶状态 */
static Uint16 gInvOcCnt;
static Uint16 gInvOcCleanTick;

/* 硬件短路（GPIO82 -> INPUT5 -> XINT2）窗口状态 */
static Uint16 gShortCnt;
static Uint16 gShortWindowTick;

/* 故障锁存与运行拍数 */
static Uint16 gProtectFaultLatch;

/* 只读镜像（Modbus 点表直接取地址） */
volatile Uint16 gProtectInvOcCnt     = 0U;
volatile Uint16 gProtectShortCnt     = 0U;
volatile Uint16 gProtectFaultFlags   = 0U;
volatile Uint16 gProtectTick         = 0U;

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

/* 锁存故障并立即封波。 */
static void ProtectLatchFault(Uint16 u16Fault)
{
    gProtectFaultLatch = (Uint16)(gProtectFaultLatch | u16Fault);
    gProtectFaultFlags = gProtectFaultLatch;

    /* 三相桥臂立刻回到中点钳位 */
    Hw_InvStopAll();

    /* 再上一道硬件锁：TZFRC.OST 把输出锁死在 TZCTL 指定的低电平，
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

    gProtectInvOcCnt = 0U;
    gProtectShortCnt = 0U;
    gProtectFaultFlags = 0U;

    Hw_ExitClearAll();
}

/* 10kHz 判定任务。 */
void ProtectTask(void *pArgV)
{
    Uint16 u16OcEvt;
    Uint16 u16ShortEvt;

    (void)pArgV;

    gProtectTick++;

    /* 取走本拍新增的比较器事件数（中断里只做累加） */
    u16OcEvt    = Hw_ExitTakeCount((Uint16)HW_XINT_ID1);
    u16ShortEvt = Hw_ExitTakeCount((Uint16)HW_XINT_ID2);

    /* ---------- 1. 逆变过流：漏桶 ---------- */
    if (u16OcEvt > 0U)
    {
        gInvOcCnt = ProtectSatAdd(gInvOcCnt, u16OcEvt);
        gInvOcCleanTick = 0U;
    }
    else if (gInvOcCnt > 0U)
    {
        gInvOcCleanTick++;

        if (gInvOcCleanTick >= (Uint16)PROTECT_INV_OC_CLEAN_TICK)
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
        && (gInvOcCnt >= (Uint16)PROTECT_INV_OC_TRIP_CNT))
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

    gProtectInvOcCnt = 0U;
    gProtectShortCnt = 0U;

    CtrlClearFault();

    return u16Old;
}

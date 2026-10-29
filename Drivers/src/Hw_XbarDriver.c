/**
 * @file Hw_XbarDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS X-BAR 驱动实现：InputXbar 引脚通路 + ePWM X-BAR 故障汇聚
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note 寄存器布局说明（下面两张指针表计算说明）：
 *       - InputXbar：INPUT1SELECT~INPUT14SELECT 连续排布，每个 16 位，直接存 GPIO 号；
 *       - ePWM X-BAR：每个 TRIP 占两个 32 位寄存器，TRIPnMUX0TO15CFG 与
 *         TRIPnMUX16TO31CFG，每个 MUX 占 2 位（00 不用 / 01 使能 / 10 取反 / 11 保留）；
 *         TRIPnMUXENABLE 是 32 位，一个 MUX 一位，与 CFG 分开写。
 *       所以配 MUX要同时动 CFG 与 ENABLE，且第 16~31 路 MUX 落在第二个 CFG 寄存器里。
 */

#include "Hw_XbarDriver.h"

/* InputXbar 的 14 个选择寄存器，下标 = E_HW_XBAR_INPUT */
static volatile Uint16 * const gXbarInputSel[HW_XBAR_INPUT_NUM] =
{
    &InputXbarRegs.INPUT1SELECT,
    &InputXbarRegs.INPUT2SELECT,
    &InputXbarRegs.INPUT3SELECT,
    &InputXbarRegs.INPUT4SELECT,
    &InputXbarRegs.INPUT5SELECT,
    &InputXbarRegs.INPUT6SELECT,
    &InputXbarRegs.INPUT7SELECT,
    &InputXbarRegs.INPUT8SELECT,
    &InputXbarRegs.INPUT9SELECT,
    &InputXbarRegs.INPUT10SELECT,
    &InputXbarRegs.INPUT11SELECT,
    &InputXbarRegs.INPUT12SELECT,
    &InputXbarRegs.INPUT13SELECT,
    &InputXbarRegs.INPUT14SELECT
};

/* 每个 TRIP 的 MUX0TO15CFG 基址，下标 = E_HW_XBAR_TRIP
 * 基址 + 0 是 MUX0~15，基址 + 1 是 MUX16~31（两个 32 位寄存器紧挨着） */
static volatile Uint32 * const gXbarTripMuxCfg[HW_XBAR_TRIP_NUM] =
{
    &EPwmXbarRegs.TRIP4MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP5MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP7MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP8MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP9MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP10MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP11MUX0TO15CFG.all,
    &EPwmXbarRegs.TRIP12MUX0TO15CFG.all
};

/* 每个 TRIP 的 MUX 使能寄存器，一个 MUX 一位，覆盖 MUX0~MUX31 */
static volatile Uint32 * const gXbarTripMuxEn[HW_XBAR_TRIP_NUM] =
{
    &EPwmXbarRegs.TRIP4MUXENABLE.all,
    &EPwmXbarRegs.TRIP5MUXENABLE.all,
    &EPwmXbarRegs.TRIP7MUXENABLE.all,
    &EPwmXbarRegs.TRIP8MUXENABLE.all,
    &EPwmXbarRegs.TRIP9MUXENABLE.all,
    &EPwmXbarRegs.TRIP10MUXENABLE.all,
    &EPwmXbarRegs.TRIP11MUXENABLE.all,
    &EPwmXbarRegs.TRIP12MUXENABLE.all
};

/* 设置某个 TRIP 的输出极性。TRIPOUTINV 里 TRIP4~TRIP12 按顺序占 bit0~bit7，
 * 与 E_HW_XBAR_TRIP 的顺序完全一致，所以直接用下标当位号。 */
static void Hw_XbarSetTripInvert(Uint16 u16TripId, Uint16 u16Invert)
{
    Uint32 u32All;

    if (u16TripId >= HW_XBAR_TRIP_NUM)
    {
        return;
    }

    u32All = EPwmXbarRegs.TRIPOUTINV.all;

    if (u16Invert != 0U)
    {
        u32All |= (Uint32)(1UL << u16TripId);
    }
    else
    {
        u32All &= ~((Uint32)(1UL << u16TripId));
    }

    EPwmXbarRegs.TRIPOUTINV.all = u32All;
}

/* 单独改一路 InputXbar 的引脚映射。 */
void Hw_XbarSetInput(Uint16 u16InputId, Uint16 u16GpioNum)
{
    if (u16InputId >= HW_XBAR_INPUT_NUM)
    {
        return;
    }

    EALLOW;
    *(gXbarInputSel[u16InputId]) = u16GpioNum;
    EDIS;
}

/* 单独改一路 TRIP 的 MUX 配置（含使能位）。 */
void Hw_XbarSetTripMux(Uint16 u16TripId, Uint16 u16MuxNum, Uint16 u16MuxMode)
{
    volatile Uint32 *pstCfgReg;
    Uint16 u16Half;
    Uint16 u16Bit;
    Uint32 u32Cfg;
    Uint32 u32En;

    if (u16TripId >= HW_XBAR_TRIP_NUM)
    {
        return;
    }

    if (u16MuxNum >= HW_XBAR_MUX_NUM)
    {
        return;
    }

    /* MUX0~15 在第一个 CFG 寄存器，MUX16~31 在第二个，每个 MUX 占 2 位 */
    u16Half = (Uint16)(u16MuxNum >> 4);
    u16Bit  = (Uint16)((u16MuxNum & 0x000FU) * 2U);
    pstCfgReg = gXbarTripMuxCfg[u16TripId] + u16Half;

    EALLOW;

    u32Cfg = *(pstCfgReg);
    u32Cfg &= ~((Uint32)(3UL << u16Bit));
    u32Cfg |= ((Uint32)((Uint32)(u16MuxMode & 0x0003U) << u16Bit));
    *(pstCfgReg) = u32Cfg;

    /* CFG为该MUX的源选择；TRIPOUTINV才负责整个TRIP的极性。 */
    u32En = *(gXbarTripMuxEn[u16TripId]);

    if (u16MuxMode == HW_XBAR_MUX_DISABLE)
    {
        u32En &= ~((Uint32)(1UL << u16MuxNum));
    }
    else
    {
        u32En |= ((Uint32)(1UL << u16MuxNum));
    }

    *(gXbarTripMuxEn[u16TripId]) = u32En;

    EDIS;
}

/* 按配置表初始化两段 X-BAR。 */
void Hw_XbarInit(const ST_XBAR_CFG *pstXbarCfg)
{
    Uint16 i;

    if (pstXbarCfg == (const ST_XBAR_CFG *)0)
    {
        return;
    }

    /* 1. GPIO -> INPUTx：这是所有下游（XINT/eCAP/ePWM X-BAR）的前提 */
    for (i = 0U; i < pstXbarCfg->u16InputItemNum; i++)
    {
        Hw_XbarSetInput(pstXbarCfg->pstInputCfgItem[i].u16InputId,
                        pstXbarCfg->pstInputCfgItem[i].u16GpioNum);
    }

    /* 2. INPUT -> TRIP：一个 TRIP 可以有多行，天然做或 */
    for (i = 0U; i < pstXbarCfg->u16TripMuxItemNum; i++)
    {
        Hw_XbarSetTripMux(pstXbarCfg->pstTripMuxCfgItem[i].u16TripId,
                          pstXbarCfg->pstTripMuxCfgItem[i].u16MuxNum,
                          pstXbarCfg->pstTripMuxCfgItem[i].u16MuxMode);
    }

    /* 3. TRIP 输出极性 */
    for (i = 0U; i < pstXbarCfg->u16TripOutItemNum; i++)
    {
        EALLOW;
        Hw_XbarSetTripInvert(pstXbarCfg->pstTripOutCfgItem[i].u16TripId,
                             pstXbarCfg->pstTripOutCfgItem[i].u16Invert);
        EDIS;
    }
}

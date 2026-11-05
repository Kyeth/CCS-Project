/**
 * @file Hw_SciDriver.c
 * @author yetingheng (004182)
 * @brief F28374S SCI
 * @version 0.5
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Hw_SciDriver.h"

#define HW_SCI_FIFO_MAX_LEVEL (16U)

static volatile struct SCI_REGS * const gSci[HW_SCI_ID_MAX] =
    { &SciaRegs, &ScibRegs, &ScicRegs, &ScidRegs };
static Uint16 gSciInited[HW_SCI_ID_MAX] = {0U, 0U, 0U, 0U};

Uint16 Hw_SciInit(const ST_SCI_CFG *cfg)
{
    Uint16 i;
    Uint32 divider;
    volatile struct SCI_REGS *regs;
    const ST_SCI_CFG_ITEM *item;
    if (cfg == 0 || cfg->pstSciCfgItem == 0 || cfg->u32LspclkHz == 0UL) return 0U;
    for (i = 0U; i < cfg->u16ItemNum; ++i)
    {
        item = &cfg->pstSciCfgItem[i];
        if (item->u16SciId >= HW_SCI_ID_MAX || item->u32Baudrate == 0UL) return 0U;
        /* 数据格式只实现 8N1（SCICCR=7）；其他值直接报错，避免静默按 8N1 跑。 */
        if (item->u16DataFormat != 0U) return 0U;
        /* 硬件阈值范围 0..16（RXFFIL 是 5 位字段，>=16 都表示满 16）。 */
        if (item->u16RxFifoLevel > HW_SCI_FIFO_MAX_LEVEL) return 0U;
        divider = (cfg->u32LspclkHz + 4UL * item->u32Baudrate) /
                  (8UL * item->u32Baudrate);
        if (divider == 0UL || divider > 65536UL) return 0U;
        divider--;
        EALLOW;
        CpuSysRegs.PCLKCR7.all |= (Uint16)(1U << item->u16SciId);
        EDIS;
        regs = gSci[item->u16SciId];
        regs->SCICTL1.all = 0U;
        regs->SCICCR.all = 7U;            /* 8 data, one stop, no parity */
        regs->SCICCR.bit.LOOPBKENA = item->bLoopback ? 1U : 0U;
        regs->SCIHBAUD.all = (Uint16)((divider >> 8U) & 0xFFUL);
        regs->SCILBAUD.all = (Uint16)(divider & 0xFFUL);
        regs->SCICTL2.all = 0U;           /* No SCI interrupt in minimal image */
        regs->SCIFFTX.all = 0U;
        regs->SCIFFRX.all = 0U;
        regs->SCIFFCT.all = 0U;
        regs->SCIFFTX.bit.SCIFFENA = 1U;
        regs->SCIFFTX.bit.SCIRST = 1U;
        regs->SCIFFTX.bit.TXFIFORESET = 1U;
        regs->SCIFFRX.bit.RXFIFORESET = 1U;
        regs->SCIFFRX.bit.RXFFOVRCLR = 1U;
        regs->SCIFFRX.bit.RXFFINTCLR = 1U;
        regs->SCIFFRX.bit.RXFFIENA = 0U;
        regs->SCIFFRX.bit.RXFFIL = item->u16RxFifoLevel;
        regs->SCICTL1.bit.RXENA = 1U;
        regs->SCICTL1.bit.TXENA = 1U;
        regs->SCICTL1.bit.SWRESET = 1U;
        gSciInited[item->u16SciId] = 1U;
    }
    return 1U;
}

Uint16 Hw_SciTryReadByte(Uint16 id, Uint16 *data)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U || data == 0 ||
        gSci[id]->SCIFFRX.bit.RXFFST == 0U) return 0U;
    *data = gSci[id]->SCIRXBUF.bit.SAR;
    return 1U;
}

Uint16 Hw_SciTryWriteByte(Uint16 id, Uint16 data)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U ||
        gSci[id]->SCIFFTX.bit.TXFFST >= HW_SCI_FIFO_MAX_LEVEL) return 0U;
    gSci[id]->SCITXBUF.bit.TXDT = data & 0xFFU;
    return 1U;
}

Uint16 Hw_SciGetTxFifoLevel(Uint16 id)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U) return 0U;
    return (Uint16)(gSci[id]->SCIFFTX.bit.TXFFST & 0x1FU);
}

Uint16 Hw_SciGetRxFifoLevel(Uint16 id)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U) return 0U;
    return (Uint16)(gSci[id]->SCIFFRX.bit.RXFFST & 0x1FU);
}

Uint16 Hw_SciGetRxStatus(Uint16 id, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear)
{
    volatile struct SCI_REGS *regs;
    Uint16 u16Status = 0U;

    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U ||
        pstStatus == (ST_SCI_RX_STATUS *)0) return 0U;

    regs = gSci[id];

    /* SCIRXST.RXERROR 只是 FE/PE/OE/BRKDT 的合并位，必须逐位取原始原因。 */
    if (regs->SCIRXST.bit.FE != 0U) u16Status |= HW_SCI_RXSTS_FE;
    if (regs->SCIRXST.bit.PE != 0U) u16Status |= HW_SCI_RXSTS_PE;
    if (regs->SCIRXST.bit.OE != 0U) u16Status |= HW_SCI_RXSTS_OE;
    if (regs->SCIRXST.bit.BRKDT != 0U) u16Status |= HW_SCI_RXSTS_BI;
    if (regs->SCIFFRX.bit.RXFFOVF != 0U) u16Status |= HW_SCI_RXSTS_RXFFOVF;

    pstStatus->u16Status = u16Status;
    pstStatus->u16RxFifoLevel = (Uint16)(regs->SCIFFRX.bit.RXFFST & 0x1FU);
    pstStatus->u16TxFifoLevel = (Uint16)(regs->SCIFFTX.bit.TXFFST & 0x1FU);

    if (bClear != 0U)
    {
        /* 只清 FIFO 侧的溢出/中断标志；FE/PE/OE/BI 属于接收逻辑，留给复位。 */
        regs->SCIFFRX.bit.RXFFOVRCLR = 1U;
        regs->SCIFFRX.bit.RXFFINTCLR = 1U;
        regs->SCIFFRX.bit.RXFFOVRCLR = 0U;
        regs->SCIFFRX.bit.RXFFINTCLR = 0U;
    }

    return 1U;
}

Uint16 Hw_SciClearRxError(Uint16 id, Uint16 *pu16Remain)
{
    volatile struct SCI_REGS *regs;
    ST_SCI_RX_STATUS stStatus;

    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U ||
        pu16Remain == (Uint16 *)0) return HW_SCI_CLEAR_UNREADABLE;

    regs = gSci[id];
    *pu16Remain = (Uint16)(regs->SCIFFRX.bit.RXFFST & 0x1FU);

    /* 发送未空闲时不动硬件：同步复位会打断移位寄存器。 */
    if ((regs->SCIFFTX.bit.TXFFST != 0U) || (regs->SCICTL2.bit.TXEMPTY == 0U))
    {
        return HW_SCI_CLEAR_WAIT_TX;
    }

    /* 1) 接收 FIFO 复位并清 FIFO 侧标志。 */
    regs->SCIFFRX.bit.RXFIFORESET = 0U;
    regs->SCIFFRX.bit.RXFIFORESET = 1U;
    regs->SCIFFRX.bit.RXFFOVRCLR = 1U;
    regs->SCIFFRX.bit.RXFFINTCLR = 1U;
    regs->SCIFFRX.bit.RXFFOVRCLR = 0U;
    regs->SCIFFRX.bit.RXFFINTCLR = 0U;

    /* 2) 接收逻辑同步复位：FE/PE/OE/BI 只认 SWRESET 的低脉冲，不是读清。 */
    regs->SCICTL1.bit.RXENA = 0U;
    regs->SCICTL1.bit.SWRESET = 0U;
    regs->SCICTL1.bit.SWRESET = 1U;
    regs->SCICTL1.bit.RXENA = 1U;
    regs->SCICTL1.bit.TXENA = 1U;

    /* 3) 复查：复位是否真的清干净，必须如实上报。 */
    if (Hw_SciGetRxStatus(id, &stStatus, 0U) == 0U)
    {
        return HW_SCI_CLEAR_UNREADABLE;
    }

    *pu16Remain = stStatus.u16RxFifoLevel;

    return (stStatus.u16Status == 0U) ? HW_SCI_CLEAR_OK : HW_SCI_CLEAR_STILL_ERR;
}

Uint16 Hw_SciGetTxIdle(Uint16 id, Uint16 *pu16Idle)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U ||
        pu16Idle == (Uint16 *)0) return 0U;

    if (gSci[id]->SCIFFTX.bit.TXFFST != 0U)
    {
        *pu16Idle = 0U;
        return 1U;
    }

    *pu16Idle = gSci[id]->SCICTL2.bit.TXEMPTY;
    return 1U;
}

Uint16 Hw_SciIsTxIdle(Uint16 id)
{
    Uint16 u16Idle = 0U;

    return (Hw_SciGetTxIdle(id, &u16Idle) == 0U) ? 0U : u16Idle;
}

Uint16 Hw_SciGetRxError(Uint16 id)
{
    ST_SCI_RX_STATUS stStatus;

    /* 不可读不能当作“无错误”：返回 0xFFFF 让调用方按异常处理。 */
    if (Hw_SciGetRxStatus(id, &stStatus, 0U) == 0U) return 0xFFFFU;

    return (stStatus.u16Status != 0U) ? 1U : 0U;
}

#include "Hw_SciDriver.h"

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
        gSci[id]->SCIFFTX.bit.TXFFST >= 16U) return 0U;
    gSci[id]->SCITXBUF.bit.TXDT = data & 0xFFU;
    return 1U;
}

Uint16 Hw_SciGetRxError(Uint16 id)
{
    if (id >= HW_SCI_ID_MAX || gSciInited[id] == 0U) return 0U;
    return (Uint16)(gSci[id]->SCIRXST.bit.RXERROR | gSci[id]->SCIFFRX.bit.RXFFOVF);
}

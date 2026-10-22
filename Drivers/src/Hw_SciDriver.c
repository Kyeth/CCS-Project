/**
 * @file Hw_SciDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS SCI驱动源文件
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_SciDriver.h"

/* LSPCLK频率：SysCtl_setLowSpeedClock(SYSCTL_LSPCLK_PRESCALE_4)配置为SYSCLK/4
 * 与device.h里的DEVICE_LSPCLK_FREQ保持一致 */
#define HW_SCI_LSPCLK_HZ                (50000000UL)

/* BRR上限，SCIHBAUD/SCILBAUD拼起来是16位 */
#define HW_SCI_BRR_MAX                  (65535UL)

/* 外设时钟使能后的稳定延时计数，寄存器手册要求至少5个周期 */
#define HW_SCI_CLOCK_DELAY_CNT          (16U)

/* 发送/接收等待的轮询上限，防止外设异常时死等 */
#define HW_SCI_POLL_TIMEOUT             (200000UL)

/* SCI寄存器基址表 */
static volatile struct SCI_REGS * const gSciRegs[HW_SCI_ID_MAX] =
{
    &SciaRegs,
    &ScibRegs,
    &ScicRegs,
    &ScidRegs
};

/* 各模块当前是否工作在FIFO模式，收发时按此选择判断标志 */
static Uint16 gSciFifoEn[HW_SCI_ID_MAX] = {0U, 0U, 0U, 0U};

/* 取模块寄存器指针，编号非法返回空指针 */
static volatile struct SCI_REGS *Hw_SciGetRegs(Uint16 u16SciId)
{
    if (u16SciId >= (Uint16)HW_SCI_ID_MAX)
    {
        return (volatile struct SCI_REGS *)0;
    }

    return gSciRegs[u16SciId];
}

/* 使能SCI模块时钟，PCLKCR7的bit0~bit3对应SCIA~SCID */
static void Hw_SciEnableClocks(const ST_SCI_CFG_ITEM *pstSciCfgItem)
{
    volatile Uint16 u16Delay;

    EALLOW;
    CpuSysRegs.PCLKCR7.all |= (Uint16)(1U << pstSciCfgItem->u16SciId);
    EDIS;

    /* 时钟刚打开时寄存器不可立即访问，先插入几个周期的空操作 */
    for (u16Delay = 0U; u16Delay < HW_SCI_CLOCK_DELAY_CNT; u16Delay++)
    {
        __asm(" NOP");
    }
}

/* 按LSPCLK计算BRR并写入SCIHBAUD/SCILBAUD，返回1表示成功 */
static Uint16 Hw_SciConfigBaud(volatile struct SCI_REGS *pstRegs, Uint32 u32Baudrate)
{
    Uint32 u32Divider;

    if (u32Baudrate == 0UL)
    {
        return 0U;
    }

    /* SCI异步模式的位时钟是LSPCLK/8，BRR = LSPCLK/(波特率*8) - 1 */
    u32Divider = HW_SCI_LSPCLK_HZ / (u32Baudrate * 8UL);
    if (u32Divider == 0UL)
    {
        return 0U;
    }

    u32Divider = u32Divider - 1UL;
    if (u32Divider > HW_SCI_BRR_MAX)
    {
        return 0U;
    }

    pstRegs->SCIHBAUD.all = (Uint16)((u32Divider >> 8U) & 0x00FFU);
    pstRegs->SCILBAUD.all = (Uint16)(u32Divider & 0x00FFU);

    return 1U;
}

/* 配置SCICCR：字符长度、停止位、校验方式、回环 */
static void Hw_SciConfigFormat(volatile struct SCI_REGS *pstRegs,
                               const ST_SCI_CFG_ITEM *pstSciCfgItem)
{
    Uint16 u16Ccr = 0U;
    Uint16 u16Value;

    /* SCICHAR: 字符长度，0表示1位，7表示8位 */
    u16Value = pstSciCfgItem->u16CharLen;
    if (u16Value > HW_SCI_CHAR_LEN_8BIT)
    {
        u16Value = HW_SCI_CHAR_LEN_8BIT;
    }
    u16Ccr |= (Uint16)(u16Value & 0x0007U);

    /* STOPBITS: 停止位个数 */
    if (pstSciCfgItem->u16StopBits == HW_SCI_STOP_BITS_2)
    {
        u16Ccr |= (Uint16)(1U << 7U);
    }

    /* PARITYENA + PARITY: 0不校验，1奇校验，2偶校验 */
    if (pstSciCfgItem->u16Parity == HW_SCI_PARITY_ODD)
    {
        u16Ccr |= (Uint16)(1U << 5U);
    }
    else if (pstSciCfgItem->u16Parity == HW_SCI_PARITY_EVEN)
    {
        u16Ccr |= (Uint16)((1U << 5U) | (1U << 6U));
    }
    else
    {
        /* 不校验时保持PARITYENA与PARITY为0 */
    }

    /* LOOPBKENA: 内部回环，用于不带外部硬件时自测 */
    if (pstSciCfgItem->bLoopback != 0U)
    {
        u16Ccr |= (Uint16)(1U << 4U);
    }

    pstRegs->SCICCR.all = u16Ccr;
}

/* 配置FIFO：使能16级收发FIFO并按需设置接收中断电平 */
static void Hw_SciConfigFifo(volatile struct SCI_REGS *pstRegs,
                             const ST_SCI_CFG_ITEM *pstSciCfgItem)
{
    Uint16 u16Level;

    if (pstSciCfgItem->bFifoEnable == 0U)
    {
        /* 不用FIFO时关掉增强功能，收发退回单字节缓冲 */
        pstRegs->SCIFFTX.all = 0U;
        pstRegs->SCIFFRX.all = 0U;
        pstRegs->SCIFFCT.all = 0U;
        return;
    }

    /* SCIRST复位SCI收发通道，SCIFFENA使能FIFO增强，TXFIFORESET复位发送FIFO */
    pstRegs->SCIFFTX.all = 0U;
    pstRegs->SCIFFTX.bit.SCIRST = 1U;
    pstRegs->SCIFFTX.bit.SCIFFENA = 1U;
    pstRegs->SCIFFTX.bit.TXFIFORESET = 1U;

    /* RXFIFORESET复位接收FIFO，RXFFOVRCLR清溢出，接收中断电平限制在1~16 */
    pstRegs->SCIFFRX.all = 0U;
    pstRegs->SCIFFRX.bit.RXFIFORESET = 1U;
    pstRegs->SCIFFRX.bit.RXFFOVRCLR = 1U;

    u16Level = pstSciCfgItem->u16RxFifoLevel;
    if (u16Level == 0U)
    {
        u16Level = 1U;
    }
    if (u16Level > HW_SCI_FIFO_DEPTH)
    {
        u16Level = HW_SCI_FIFO_DEPTH;
    }
    pstRegs->SCIFFRX.bit.RXFFIL = u16Level;
    pstRegs->SCIFFRX.bit.RXFFIENA = (pstSciCfgItem->bRxIntEnable != 0U) ? 1U : 0U;

    /* 不用自动波特率，发送FIFO延迟为0 */
    pstRegs->SCIFFCT.all = 0U;

    /* 清发送FIFO中断标志 */
    pstRegs->SCIFFTX.bit.TXFFINTCLR = 1U;
    pstRegs->SCIFFRX.bit.RXFFINTCLR = 1U;
}

void Hw_SciSingleInit(const ST_SCI_CFG_ITEM *pstSciCfgItem)
{
    volatile struct SCI_REGS *pstRegs;

    if (pstSciCfgItem == (const ST_SCI_CFG_ITEM *)0)
    {
        return;
    }

    pstRegs = Hw_SciGetRegs(pstSciCfgItem->u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return;
    }

    Hw_SciEnableClocks(pstSciCfgItem);

    /* 软复位：SWRESET=0期间允许改写配置寄存器 */
    pstRegs->SCICTL1.all = 0U;
    pstRegs->SCICTL1.bit.SWRESET = 0U;

    (void)Hw_SciConfigBaud(pstRegs, pstSciCfgItem->u32Baudrate);
    Hw_SciConfigFormat(pstRegs, pstSciCfgItem);

    /* 不使用发送中断与断开检测中断，全部状态用轮询 */
    pstRegs->SCICTL2.all = 0U;

    /* FREESOFT=0：仿真暂停时SCI立即停止 */
    pstRegs->SCIPRI.all = 0U;

    Hw_SciConfigFifo(pstRegs, pstSciCfgItem);

    gSciFifoEn[pstSciCfgItem->u16SciId] = (pstSciCfgItem->bFifoEnable != 0U) ? 1U : 0U;

    /* 释放软复位并打开收发使能 */
    pstRegs->SCICTL1.bit.SWRESET = 1U;
    pstRegs->SCICTL1.bit.RXENA = 1U;
    pstRegs->SCICTL1.bit.TXENA = 1U;
}

void Hw_SciInit(const ST_SCI_CFG *pstSciCfg)
{
    Uint16 u16Index;

    if ((pstSciCfg == (const ST_SCI_CFG *)0) ||
        (pstSciCfg->pstSciCfgItem == (const ST_SCI_CFG_ITEM *)0))
    {
        return;
    }

    for (u16Index = 0U; u16Index < pstSciCfg->u16ItemNum; u16Index++)
    {
        Hw_SciSingleInit(&pstSciCfg->pstSciCfgItem[u16Index]);
    }
}

Uint16 Hw_SciSendByte(Uint16 u16SciId, Uint16 u16Data)
{
    volatile struct SCI_REGS *pstRegs;
    Uint32 u32Wait = 0UL;

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    if (gSciFifoEn[u16SciId] != 0U)
    {
        /* FIFO模式：等到发送FIFO还有空位 */
        while (pstRegs->SCIFFTX.bit.TXFFST >= HW_SCI_FIFO_DEPTH)
        {
            u32Wait++;
            if (u32Wait > HW_SCI_POLL_TIMEOUT)
            {
                return 0U;
            }
        }
    }
    else
    {
        /* 单字节模式：等到发送缓冲就绪 */
        while (pstRegs->SCICTL2.bit.TXRDY == 0U)
        {
            u32Wait++;
            if (u32Wait > HW_SCI_POLL_TIMEOUT)
            {
                return 0U;
            }
        }
    }

    pstRegs->SCITXBUF.all = (Uint16)(u16Data & 0x00FFU);

    return 1U;
}

Uint16 Hw_SciSendBuffer(Uint16 u16SciId, const Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Index;

    if (pu16Buf == (const Uint16 *)0)
    {
        return 0U;
    }

    for (u16Index = 0U; u16Index < u16Len; u16Index++)
    {
        if (Hw_SciSendByte(u16SciId, pu16Buf[u16Index]) == 0U)
        {
            break;
        }
    }

    return u16Index;
}

Uint16 Hw_SciGetRxCount(Uint16 u16SciId)
{
    volatile struct SCI_REGS *pstRegs;

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    if (gSciFifoEn[u16SciId] != 0U)
    {
        return (Uint16)pstRegs->SCIFFRX.bit.RXFFST;
    }

    return (pstRegs->SCIRXST.bit.RXRDY != 0U) ? 1U : 0U;
}

Uint16 Hw_SciReadByte(Uint16 u16SciId, Uint16 *pu16Data)
{
    volatile struct SCI_REGS *pstRegs;

    if (pu16Data == (Uint16 *)0)
    {
        return 0U;
    }

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    if (Hw_SciGetRxCount(u16SciId) == 0U)
    {
        return 0U;
    }

    /* 读SCIRXBUF会自动清RXRDY，同时清掉FIFO中断标志 */
    *pu16Data = (Uint16)(pstRegs->SCIRXBUF.bit.SAR & 0x00FFU);

    if (gSciFifoEn[u16SciId] != 0U)
    {
        pstRegs->SCIFFRX.bit.RXFFINTCLR = 1U;
    }

    return 1U;
}

Uint16 Hw_SciReadBuffer(Uint16 u16SciId, Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Index;

    if (pu16Buf == (Uint16 *)0)
    {
        return 0U;
    }

    for (u16Index = 0U; u16Index < u16Len; u16Index++)
    {
        if (Hw_SciReadByte(u16SciId, &pu16Buf[u16Index]) == 0U)
        {
            break;
        }
    }

    return u16Index;
}

Uint16 Hw_SciClearRxFifo(Uint16 u16SciId)
{
    volatile struct SCI_REGS *pstRegs;
    Uint32 u32Wait = 0UL;

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    if (gSciFifoEn[u16SciId] == 0U)
    {
        /* 非FIFO模式读一次缓冲即可丢弃当前字符 */
        (void)pstRegs->SCIRXBUF.bit.SAR;
        return 1U;
    }

    /* FIFO复位要求先写0再写1，中间需要几个周期的间隔 */
    pstRegs->SCIFFRX.bit.RXFIFORESET = 0U;
    while (pstRegs->SCIFFRX.bit.RXFIFORESET != 0U)
    {
        u32Wait++;
        if (u32Wait > HW_SCI_POLL_TIMEOUT)
        {
            return 0U;
        }
    }
    pstRegs->SCIFFRX.bit.RXFIFORESET = 1U;
    pstRegs->SCIFFRX.bit.RXFFOVRCLR = 1U;
    pstRegs->SCIFFRX.bit.RXFFINTCLR = 1U;

    return 1U;
}

Uint16 Hw_SciGetRxOverflow(Uint16 u16SciId)
{
    volatile struct SCI_REGS *pstRegs;

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    if (pstRegs->SCIFFRX.bit.RXFFOVF == 0U)
    {
        return 0U;
    }

    /* 溢出标志是写1清除，查到就顺手清掉，避免下次误判 */
    pstRegs->SCIFFRX.bit.RXFFOVRCLR = 1U;

    return 1U;
}

Uint16 Hw_SciGetRxError(Uint16 u16SciId)
{
    volatile struct SCI_REGS *pstRegs;
    Uint16 u16Status;

    pstRegs = Hw_SciGetRegs(u16SciId);
    if (pstRegs == (volatile struct SCI_REGS *)0)
    {
        return 0U;
    }

    u16Status = pstRegs->SCIRXST.all;

    /* PE(bit2)/OE(bit3)/FE(bit4)/BRKDT(bit5)任一置位即为出错，写1清除 */
    if ((u16Status & 0x003CU) == 0U)
    {
        return 0U;
    }

    pstRegs->SCIRXST.all = (Uint16)(u16Status & 0x003CU);

    return 1U;
}

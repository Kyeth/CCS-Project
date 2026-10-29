/**
 * @file Hw_SpiDriver.c
 * @author yetingheng (004182)
 * @brief SPI 主机驱动实现（查询方式，非 FIFO）
 * @version 0.1
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 为什么不用 FIFO：本板 SPI 只用来和 ARM 交换控制命令与状态，
 *       一帧几十字节，查询方式的代码量和出错面都比中断+FIFO 小得多；
 *       真要提吞吐量，再在本驱动上叠 FIFO 中断，不影响上层接口。
 */
#include "Hw_SpiDriver.h"
#include "Hw_GpioDriver.h"

/******************************************************************************
 * 私有定义
 ******************************************************************************/
/* LSPCLK = SYSCLK(200MHz) / 4 = 50MHz，由 device.c 里 SysCtl_setLowSpeedClock 固定 */
#define HW_SPI_LSPCLK_HZ            (50000000UL)

/* BRR 有效范围：3~127 为 LSPCLK/(BRR+1)，0~2 为固定的 LSPCLK/4 */
#define HW_SPI_BRR_MIN              (3U)
#define HW_SPI_BRR_MAX              (127U)

/* 缺省超时：2000us，足够覆盖 8 位字符在 100kHz 下（80us）再留足余量 */
#define HW_SPI_TIMEOUT_US_DEFAULT   (2000U)

/* 片选建立/保持时间：STM32 侧片选后到第一个时钟沿需要一点时间，
 * 这里统一给 1us，靠空循环实现，不需要精确。 */
#define HW_SPI_CS_SETUP_US          (1U)
#define HW_SPI_CS_HOLD_US           (1U)

/******************************************************************************
 * 私有变量
 ******************************************************************************/
static volatile struct SPI_REGS * const gSpiRegs[HW_SPI_ID_MAX] =
{
    &SpiaRegs,
    &SpibRegs,
    &SpicRegs
};

static Uint16 gSpiInited = 0U;
static Uint16 gSpiUsed[HW_SPI_ID_MAX] = {0U, 0U, 0U};

static Uint16 gSpiCharLen[HW_SPI_ID_MAX] = {0U, 0U, 0U};
static Uint16 gSpiCsGpio[HW_SPI_ID_MAX] = {HW_SPI_NO_CS_GPIO, HW_SPI_NO_CS_GPIO, HW_SPI_NO_CS_GPIO};
static Uint16 gSpiCsActiveLevel[HW_SPI_ID_MAX] = {0U, 0U, 0U};
static Uint32 gSpiBaudHz[HW_SPI_ID_MAX] = {0UL, 0UL, 0UL};

/* 超时计数不存时间，存"空循环次数"，比较省事且与主频标定解耦 */
static Uint32 gSpiTimeoutLoops[HW_SPI_ID_MAX] = {0UL, 0UL, 0UL};

static Uint16 gSpiErrCnt[HW_SPI_ID_MAX] = {0U, 0U, 0U};
static Uint32 gSpiXferCnt[HW_SPI_ID_MAX] = {0UL, 0UL, 0UL};

/******************************************************************************
 * 私有函数
 ******************************************************************************/
/**
 * @brief 空循环延时（微秒级粗延时）
 */
static void Hw_SpiDelayUs(Uint16 u16Us)
{
    volatile Uint32 u32Cnt;

    if (u16Us == 0U)
    {
        return;
    }

    u32Cnt = (Uint32)u16Us * HW_SPI_LOOP_PER_US;
    while (u32Cnt > 0UL)
    {
        u32Cnt--;
    }
}

/**
 * @brief 取模块寄存器基址
 */
static volatile struct SPI_REGS *Hw_SpiGetRegs(Uint16 u16SpiId)
{
    if (u16SpiId >= HW_SPI_ID_MAX)
    {
        return (volatile struct SPI_REGS *)0;
    }

    return gSpiRegs[u16SpiId];
}

/**
 * @brief 按目标波特率反算分频并取实际值
 *
 * @note 分频语义：SPI 时钟 = LSPCLK/(BRR+1)（BRR>=3），BRR 越小越快；
 *       这里四舍五入取最接近的分频，保证实际值不会比目标快太多
 *       （从机耐受不了过快的时钟，宁慢勿快）。
 */
static Uint16 Hw_SpiCalcDividor(Uint32 u32BaudHz, Uint16 *pu16Brr, Uint32 *pu32ActualHz)
{
    Uint32 u32Div;

    if ((pu16Brr == (Uint16 *)0) || (pu32ActualHz == (Uint32 *)0))
    {
        return HW_SPI_ERR_PARAM;
    }

    if (u32BaudHz == 0UL)
    {
        u32BaudHz = 1000000UL;      /* 缺省 1MHz，够 STM32 响应中断级命令 */
    }

    /* 分频 = LSPCLK / 目标波特率，四舍五入 */
    u32Div = (HW_SPI_LSPCLK_HZ + (u32BaudHz / 2UL)) / u32BaudHz;

    if (u32Div < (HW_SPI_BRR_MIN + 1UL))
    {
        u32Div = HW_SPI_BRR_MIN + 1UL;      /* 上限 12.5MHz，再快 SPI 也出不来 */
    }

    if (u32Div > (HW_SPI_BRR_MAX + 1UL))
    {
        u32Div = HW_SPI_BRR_MAX + 1UL;      /* 下限约 390kHz，再慢就得改 LSPCLK */
    }

    *pu16Brr = (Uint16)(u32Div - 1UL);
    *pu32ActualHz = HW_SPI_LSPCLK_HZ / u32Div;

    return HW_SPI_OK;
}

/**
 * @brief 片选输出
 */
static void Hw_SpiDriveCs(Uint16 u16SpiId, Uint16 bSelect)
{
    Uint16 u16Level;

    if (gSpiCsGpio[u16SpiId] == HW_SPI_NO_CS_GPIO)
    {
        return;
    }

    if (bSelect != 0U)
    {
        u16Level = (gSpiCsActiveLevel[u16SpiId] != 0U) ? 1U : 0U;
    }
    else
    {
        u16Level = (gSpiCsActiveLevel[u16SpiId] != 0U) ? 0U : 1U;
    }

    Hw_GpioWritePin((Uint32)gSpiCsGpio[u16SpiId], (Uint32)u16Level);
}

/**
 * @brief 收发一个字符（不含片选）
 */
static Uint16 Hw_SpiXferChar(Uint16 u16SpiId, Uint16 u16Tx, Uint16 *pu16Rx)
{
    volatile struct SPI_REGS *pstRegs;
    Uint16 u16Shift;
    Uint16 u16Rx;
    Uint32 u32Wait;

    pstRegs = Hw_SpiGetRegs(u16SpiId);
    if (pstRegs == (volatile struct SPI_REGS *)0)
    {
        return HW_SPI_ERR_PARAM;
    }

    u16Shift = (Uint16)(HW_SPI_CHAR_MAX - gSpiCharLen[u16SpiId]);

    /* 1. 等发送缓冲空 */
    u32Wait = gSpiTimeoutLoops[u16SpiId];
    while (pstRegs->SPISTS.bit.BUFFULL_FLAG != 0U)
    {
        if (u32Wait == 0UL)
        {
            gSpiErrCnt[u16SpiId]++;
            return HW_SPI_ERR_TIMEOUT;
        }
        u32Wait--;
    }

    /* 2. 左对齐写入发送缓冲，写入即启动传输 */
    pstRegs->SPITXBUF = (Uint16)((Uint16)(u16Tx << u16Shift) & 0xFFFFU);

    /* 3. 等接收完成标志 */
    u32Wait = gSpiTimeoutLoops[u16SpiId];
    while (pstRegs->SPISTS.bit.INT_FLAG == 0U)
    {
        if (u32Wait == 0UL)
        {
            gSpiErrCnt[u16SpiId]++;
            return HW_SPI_ERR_TIMEOUT;
        }
        u32Wait--;
    }

    /* 4. 右对齐读回，并清标志 */
    u16Rx = (Uint16)(pstRegs->SPIRXBUF & ((1UL << gSpiCharLen[u16SpiId]) - 1UL));

    if (pstRegs->SPISTS.bit.OVERRUN_FLAG != 0U)
    {
        /* 溢出说明上一个字符没收走，清掉并记一次错（不影响本帧继续） */
        pstRegs->SPISTS.bit.OVERRUN_FLAG = 1U;
        gSpiErrCnt[u16SpiId]++;
    }

    gSpiXferCnt[u16SpiId]++;

    if (pu16Rx != (Uint16 *)0)
    {
        *pu16Rx = u16Rx;
    }

    return HW_SPI_OK;
}

/**
 * @brief 一帧的收发主体（片选已由调用者处理）
 */
static Uint16 Hw_SpiFrame(Uint16 u16SpiId, const Uint16 *pu16Tx, Uint16 *pu16Rx, Uint16 u16Len)
{
    Uint16 u16Idx;
    Uint16 u16Tx;
    Uint16 u16Ret;

    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        if (pu16Tx != (const Uint16 *)0)
        {
            u16Tx = pu16Tx[u16Idx];
        }
        else
        {
            u16Tx = 0xFFFFU;        /* 只收时发全 1，多数从机按空闲处理 */
        }

        if (pu16Rx != (Uint16 *)0)
        {
            u16Ret = Hw_SpiXferChar(u16SpiId, u16Tx, &pu16Rx[u16Idx]);
        }
        else
        {
            u16Ret = Hw_SpiXferChar(u16SpiId, u16Tx, (Uint16 *)0);
        }

        if (u16Ret != HW_SPI_OK)
        {
            return u16Ret;
        }
    }

    return HW_SPI_OK;
}

/******************************************************************************
 * 公开函数
 ******************************************************************************/

/**
 * @brief 初始化 SPI 主机
 */
void Hw_SpiInit(const ST_SPI_CFG *pstSpiCfg)
{
    const ST_SPI_CFG_ITEM *pstItem;
    volatile struct SPI_REGS *pstRegs;
    Uint16 u16Idx;
    Uint16 u16Brr;
    Uint32 u32Actual;

    if ((pstSpiCfg == (const ST_SPI_CFG *)0) ||
        (pstSpiCfg->pstItem == (const ST_SPI_CFG_ITEM *)0))
    {
        return;
    }

    pstItem = pstSpiCfg->pstItem;

    for (u16Idx = 0U; u16Idx < pstSpiCfg->u16ItemNum; u16Idx++)
    {
        if (pstItem[u16Idx].u16SpiId >= HW_SPI_ID_MAX)
        {
            continue;
        }

        pstRegs = gSpiRegs[pstItem[u16Idx].u16SpiId];

        /* ---- 1. 软件复位，关 FIFO，回到经典非 FIFO 模式 ---- */
        pstRegs->SPICCR.bit.SPISWRESET = 0U;
        pstRegs->SPIFFTX.bit.SPIFFENA = 0U;
        pstRegs->SPIFFTX.bit.TXFIFO = 0U;
        pstRegs->SPIFFRX.bit.RXFIFORESET = 0U;
        pstRegs->SPIFFRX.bit.RXFFOVFCLR = 1U;
        pstRegs->SPIFFTX.bit.SPIRST = 1U;

        /* ---- 2. 字符宽度、时钟极性 ---- */
        if ((pstItem[u16Idx].u16CharLen == 0U) ||
            (pstItem[u16Idx].u16CharLen > HW_SPI_CHAR_MAX))
        {
            gSpiCharLen[pstItem[u16Idx].u16SpiId] = 8U;
        }
        else
        {
            gSpiCharLen[pstItem[u16Idx].u16SpiId] = pstItem[u16Idx].u16CharLen;
        }

        pstRegs->SPICCR.bit.SPICHAR = (Uint16)(gSpiCharLen[pstItem[u16Idx].u16SpiId] - 1U);
        pstRegs->SPICCR.bit.CLKPOLARITY = (pstItem[u16Idx].u16ClkPolarity != 0U) ? 1U : 0U;
        pstRegs->SPICCR.bit.SPILBK = 0U;

        /* ---- 3. 主机、发送使能、时钟相位 ---- */
        pstRegs->SPICTL.bit.MASTER_SLAVE = 1U;
        pstRegs->SPICTL.bit.TALK = 1U;
        pstRegs->SPICTL.bit.CLK_PHASE = (pstItem[u16Idx].u16ClkPhase != 0U) ? 1U : 0U;
        pstRegs->SPICTL.bit.SPIINTENA = 0U;
        pstRegs->SPICTL.bit.OVERRUNINTENA = 0U;

        /* ---- 4. 波特率 ---- */
        (void)Hw_SpiCalcDividor(pstItem[u16Idx].u32BaudHz, &u16Brr, &u32Actual);
        pstRegs->SPIBRR.bit.SPI_BIT_RATE = u16Brr;

        gSpiBaudHz[pstItem[u16Idx].u16SpiId] = u32Actual;

        /* ---- 5. 片选与超时 ---- */
        gSpiCsGpio[pstItem[u16Idx].u16SpiId] = pstItem[u16Idx].u16CsGpio;
        gSpiCsActiveLevel[pstItem[u16Idx].u16SpiId] = pstItem[u16Idx].u16CsActiveLevel;

        if (pstItem[u16Idx].u16TimeoutUs == 0U)
        {
            gSpiTimeoutLoops[pstItem[u16Idx].u16SpiId] =
                (Uint32)HW_SPI_TIMEOUT_US_DEFAULT * HW_SPI_LOOP_PER_US;
        }
        else
        {
            gSpiTimeoutLoops[pstItem[u16Idx].u16SpiId] =
                (Uint32)pstItem[u16Idx].u16TimeoutUs * HW_SPI_LOOP_PER_US;
        }

        /* 初始把片选放成"未选中" */
        Hw_SpiDriveCs(pstItem[u16Idx].u16SpiId, 0U);

        /* ---- 6. 释放软件复位 ---- */
        pstRegs->SPICCR.bit.SPISWRESET = 1U;

        /* 仿真暂停时也继续跑，避免断点导致 SPI 停摆 */
        pstRegs->SPIPRI.bit.FREE = 1U;

        gSpiUsed[pstItem[u16Idx].u16SpiId] = 1U;
    }

    gSpiInited = 1U;
}

/**
 * @brief 全双工收发一个字符
 *
 * @note 这个函数不动片选，适合"从机没有片选"或"片选完全由上层控制"的场合。
 */
Uint16 Hw_SpiTransfer(Uint16 u16SpiId, Uint16 u16Tx, Uint16 *pu16Rx)
{
    if ((gSpiInited == 0U) || (u16SpiId >= HW_SPI_ID_MAX) || (gSpiUsed[u16SpiId] == 0U))
    {
        return HW_SPI_ERR_PARAM;
    }

    return Hw_SpiXferChar(u16SpiId, u16Tx, pu16Rx);
}

/**
 * @brief 多字符收发，整帧占住片选
 */
Uint16 Hw_SpiTransferBuf(Uint16 u16SpiId, const Uint16 *pu16Tx, Uint16 *pu16Rx, Uint16 u16Len)
{
    Uint16 u16Ret;

    if ((gSpiInited == 0U) || (u16SpiId >= HW_SPI_ID_MAX) || (gSpiUsed[u16SpiId] == 0U))
    {
        return HW_SPI_ERR_PARAM;
    }

    if ((u16Len == 0U) || ((pu16Tx == (const Uint16 *)0) && (pu16Rx == (Uint16 *)0)))
    {
        return HW_SPI_ERR_PARAM;
    }

    Hw_SpiDriveCs(u16SpiId, 1U);
    Hw_SpiDelayUs(HW_SPI_CS_SETUP_US);

    u16Ret = Hw_SpiFrame(u16SpiId, pu16Tx, pu16Rx, u16Len);

    Hw_SpiDelayUs(HW_SPI_CS_HOLD_US);
    Hw_SpiDriveCs(u16SpiId, 0U);

    return u16Ret;
}

/**
 * @brief 只发不收
 */
Uint16 Hw_SpiWrite(Uint16 u16SpiId, const Uint16 *pu16Tx, Uint16 u16Len)
{
    if (pu16Tx == (const Uint16 *)0)
    {
        return HW_SPI_ERR_PARAM;
    }

    return Hw_SpiTransferBuf(u16SpiId, pu16Tx, (Uint16 *)0, u16Len);
}

/**
 * @brief 只收不发
 */
Uint16 Hw_SpiRead(Uint16 u16SpiId, Uint16 *pu16Rx, Uint16 u16Len)
{
    if (pu16Rx == (Uint16 *)0)
    {
        return HW_SPI_ERR_PARAM;
    }

    return Hw_SpiTransferBuf(u16SpiId, (const Uint16 *)0, pu16Rx, u16Len);
}

/**
 * @brief 手动控制片选
 */
void Hw_SpiSelect(Uint16 u16SpiId, Uint16 bSelect)
{
    if ((gSpiInited == 0U) || (u16SpiId >= HW_SPI_ID_MAX))
    {
        return;
    }

    Hw_SpiDriveCs(u16SpiId, bSelect);
}

/**
 * @brief 读实际波特率
 */
Uint32 Hw_SpiGetBaud(Uint16 u16SpiId)
{
    if (u16SpiId >= HW_SPI_ID_MAX)
    {
        return 0UL;
    }

    return gSpiBaudHz[u16SpiId];
}

/**
 * @brief 统计量
 */
Uint16 Hw_SpiGetErrCnt(Uint16 u16SpiId)
{
    if (u16SpiId >= HW_SPI_ID_MAX)
    {
        return 0U;
    }

    return gSpiErrCnt[u16SpiId];
}

Uint32 Hw_SpiGetXferCnt(Uint16 u16SpiId)
{
    if (u16SpiId >= HW_SPI_ID_MAX)
    {
        return 0UL;
    }

    return gSpiXferCnt[u16SpiId];
}

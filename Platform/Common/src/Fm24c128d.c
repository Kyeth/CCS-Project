/**
 * @file Fm24c128d.c
 * @author yetingheng (004182)
 * @brief FM24C128D（128Kbit I2C EEPROM）器件驱动实现
 * @version 0.2
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 写操作按 64 字节页边界拆分：24C128 的页内地址是回卷的，一次写跨页
 *       会覆盖回页首，所以必须在驱动里把一帧拆成若干"不跨页"的子帧。
 *
 * @note 每个子帧写完都有内部烧写周期（tWR 最大 5ms），期间器件不回 ACK。
 *       本实现每写完一页就做 ACK 轮询（重复发设备地址看是否应答），
 *       轮到应答说明写完了，比固定延时快也比固定延时稳。
 *
 * @note WP 引脚：写之前拉低放开，写完整片恢复保护（若配置了 WP 脚）。
 *       24C128 的 WP 只保护高半区（0x2000 以上），参数区在低半区，
 *       所以即使这里的保护和实际板子语义有出入，参数区也不会被挡住。
 */
#include "Fm24c128d.h"
#include "Hw_I2cDriver.h"
#include "Hw_GpioDriver.h"

/******************************************************************************
 * 私有变量
 ******************************************************************************/
static ST_FM24C128D_CFG gFmCfg;
static Uint16 gFmInited = 0U;

/* 置 1 表示上一帧写完还没确认写周期结束，下一次访问前必须先等 */
static Uint16 gFmNeedWait = 0U;

static Uint16 gFmErrCnt = 0U;
static Uint16 gFmWriteCnt = 0U;
static Uint16 gFmReadCnt = 0U;

/******************************************************************************
 * 私有函数
 ******************************************************************************/

/**
 * @brief 取实际使用的容量
 */
static Uint16 Fm24c128dSize(void)
{
    return (gFmCfg.u16SizeBytes != 0U) ? gFmCfg.u16SizeBytes : FM24C128D_SIZE_BYTES;
}

/* 异步写状态：一次多页写拆成每拍一片，避免把后台调度阻塞数十毫秒 */
static Uint16 sAsyncAddr;
static const Uint16 *sAsyncBuf;
static Uint16 sAsyncLeft;
static Uint16 sAsyncActive;
static Uint16 sAsyncWaitCnt;

/**
 * @brief 写保护脚物理电平输出
 */
static void Fm24c128dWpLevel(Uint16 u16Level)
{
    if (gFmCfg.u16WpGpio > 168U)
    {
        return;
    }

    Hw_GpioWritePin((Uint32)gFmCfg.u16WpGpio, (u16Level != 0U) ? 1UL : 0UL);
}

/**
 * @brief 写一帧"不跨页"的数据：START + 设备地址 + 2 字节字地址 + 数据 + STOP
 *
 * @note 调用者保证 u16Addr 到 u16Addr+u16Len 不跨 64 字节页。
 */
static Uint16 Fm24c128dWriteChunk(Uint16 u16Addr, const Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Ret;
    Uint16 u16Idx;

    Hw_I2cStart();

    u16Ret = Hw_I2cWriteByte((Uint16)((gFmCfg.u16DevAddr7 & 0x007FU) << 1U));
    if (u16Ret == HW_I2C_OK)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)((u16Addr >> 8U) & 0x00FFU));
    }
    if (u16Ret == HW_I2C_OK)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)(u16Addr & 0x00FFU));
    }

    for (u16Idx = 0U; (u16Idx < u16Len) && (u16Ret == HW_I2C_OK); u16Idx++)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)(pu16Buf[u16Idx] & 0x00FFU));
    }

    Hw_I2cStop();

    return u16Ret;
}

/******************************************************************************
 * 公开函数
 ******************************************************************************/

/**
 * @brief 初始化 FM24C128D
 */
void Fm24c128dInit(const ST_FM24C128D_CFG *pstFmCfg)
{
    if (pstFmCfg == (const ST_FM24C128D_CFG *)0)
    {
        return;
    }

    gFmCfg = *pstFmCfg;

    if (gFmCfg.u16DevAddr7 == 0U)
    {
        gFmCfg.u16DevAddr7 = FM24C128D_DEV_ADDR_7BIT;
    }
    if (gFmCfg.u16SizeBytes == 0U)
    {
        gFmCfg.u16SizeBytes = FM24C128D_SIZE_BYTES;
    }
    if (gFmCfg.u16ScratchAddr == 0U)
    {
        /* 默认放在低半区，避免和 WP 策略耦合（参数区占 0x0000~0x0FFF） */
        gFmCfg.u16ScratchAddr = 0x1F00U;
    }

    /* 先放开写保护，否则自检和参数区都写不进去 */
    Fm24c128dWriteProtect(0U);

    gFmNeedWait = 0U;
    gFmErrCnt = 0U;
    gFmWriteCnt = 0U;
    gFmReadCnt = 0U;
    gFmInited = 1U;

    if (gFmCfg.bEnableSelfTest != 0U)
    {
        (void)Fm24c128dSelfTest();
    }
}

/**
 * @brief 等器件写完当前内部烧写周期
 */
Uint16 Fm24c128dWaitReady(void)
{
    Uint16 u16Idx;

    if (gFmInited == 0U)
    {
        return FM24C128D_ERR_PARAM;
    }

    for (u16Idx = 0U; u16Idx < FM24C128D_ACK_POLL_CNT; u16Idx++)
    {
        if (Fm24c128dIsReady() != 0U)
        {
            gFmNeedWait = 0U;
            return FM24C128D_OK;
        }

        Hw_I2cWaitUs(FM24C128D_ACK_POLL_US);
    }

    return FM24C128D_ERR_TIMEOUT;
}

/**
 * @brief 写若干字节，自动按页拆分
 */
Uint16 Fm24c128dWrite(Uint16 u16Addr, const Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Ret;
    Uint16 u16Chunk;
    Uint16 u16PageLeft;

    if ((gFmInited == 0U) || (sAsyncActive != 0U) ||
        (pu16Buf == (const Uint16 *)0) || (u16Len == 0U))
    {
        return FM24C128D_ERR_PARAM;    /* 异步写进行中不允许插入同步写 */
    }

    if (((Uint32)u16Addr + (Uint32)u16Len) > (Uint32)Fm24c128dSize())
    {
        gFmErrCnt++;
        return FM24C128D_ERR_PARAM;
    }

    /* 上一帧的写周期还没结束就先等，否则这次 START 会被 NACK */
    if (gFmNeedWait != 0U)
    {
        u16Ret = Fm24c128dWaitReady();
        if (u16Ret != FM24C128D_OK)
        {
            gFmErrCnt++;
            return u16Ret;
        }
    }

    /* 放开写保护（GPIO99 拉低） */
    Fm24c128dWriteProtect(0U);

    while (u16Len > 0U)
    {
        /* 到本页末尾还剩多少字节 */
        u16PageLeft = (Uint16)(FM24C128D_PAGE_SIZE - (u16Addr & (FM24C128D_PAGE_SIZE - 1U)));
        u16Chunk = (u16Len < u16PageLeft) ? u16Len : u16PageLeft;

        u16Ret = Fm24c128dWriteChunk(u16Addr, pu16Buf, u16Chunk);
        if (u16Ret != HW_I2C_OK)
        {
            gFmErrCnt++;
            gFmNeedWait = 1U;
            Fm24c128dWriteProtect(1U);
            return (u16Ret == HW_I2C_ERR_PARAM) ? FM24C128D_ERR_PARAM : FM24C128D_ERR_I2C;
        }

        /* 等这一页的 tWR 结束（ACK 轮询） */
        gFmNeedWait = 1U;
        u16Ret = Fm24c128dWaitReady();
        if (u16Ret != FM24C128D_OK)
        {
            gFmErrCnt++;
            Fm24c128dWriteProtect(1U);
            return u16Ret;
        }

        gFmWriteCnt++;

        u16Addr = (Uint16)(u16Addr + u16Chunk);
        pu16Buf = pu16Buf + u16Chunk;
        u16Len = (Uint16)(u16Len - u16Chunk);
    }

    /* 恢复写保护 */
    Fm24c128dWriteProtect(1U);

    return FM24C128D_OK;
}

/**
 * @brief 启动异步写，只送第一片
 */
Uint16 Fm24c128dWriteAsyncStart(Uint16 u16Addr, const Uint16 *pu16Buf, Uint16 u16Len)
{
    if ((gFmInited == 0U) || (sAsyncActive != 0U) ||
        (pu16Buf == (const Uint16 *)0) || (u16Len == 0U))
    {
        return FM24C128D_ERR_PARAM;
    }

    if (((Uint32)u16Addr + (Uint32)u16Len) > (Uint32)Fm24c128dSize())
    {
        gFmErrCnt++;
        return FM24C128D_ERR_PARAM;
    }

    sAsyncAddr = u16Addr;
    sAsyncBuf = pu16Buf;
    sAsyncLeft = u16Len;
    sAsyncWaitCnt = 0U;
    sAsyncActive = 1U;

    Fm24c128dWriteProtect(0U);

    return Fm24c128dWriteAsyncPoll();
}

/**
 * @brief 异步写推进：每拍调用一次，直到返回非 BUSY
 */
Uint16 Fm24c128dWriteAsyncPoll(void)
{
    Uint16 u16Ret;
    Uint16 u16Chunk;
    Uint16 u16PageLeft;

    if (sAsyncActive == 0U)
    {
        return FM24C128D_OK;
    }

    /* 上一片还没烧完就下拍再来，不在这里死等 */
    if (gFmNeedWait != 0U)
    {
        if (Fm24c128dIsReady() == 0U)
        {
            if (++sAsyncWaitCnt >= FM24C128D_ASYNC_RETRY_MAX)
            {
                gFmErrCnt++;
                sAsyncActive = 0U;
                Fm24c128dWriteProtect(1U);
                return FM24C128D_ERR_TIMEOUT;
            }

            return FM24C128D_ASYNC_BUSY;
        }

        gFmNeedWait = 0U;
        sAsyncWaitCnt = 0U;
    }

    /* 数据已发完，恢复写保护并收尾 */
    if (sAsyncLeft == 0U)
    {
        sAsyncActive = 0U;
        Fm24c128dWriteProtect(1U);
        return FM24C128D_OK;
    }

    u16PageLeft = (Uint16)(FM24C128D_PAGE_SIZE - (sAsyncAddr & (FM24C128D_PAGE_SIZE - 1U)));
    u16Chunk = (sAsyncLeft < u16PageLeft) ? sAsyncLeft : u16PageLeft;

    if (u16Chunk > FM24C128D_ASYNC_CHUNK_WORDS)
    {
        u16Chunk = FM24C128D_ASYNC_CHUNK_WORDS;
    }

    u16Ret = Fm24c128dWriteChunk(sAsyncAddr, sAsyncBuf, u16Chunk);
    if (u16Ret != HW_I2C_OK)
    {
        gFmErrCnt++;
        sAsyncActive = 0U;
        Fm24c128dWriteProtect(1U);
        return (u16Ret == HW_I2C_ERR_PARAM) ? FM24C128D_ERR_PARAM : FM24C128D_ERR_I2C;
    }

    gFmWriteCnt++;
    gFmNeedWait = 1U;

    sAsyncAddr = (Uint16)(sAsyncAddr + u16Chunk);
    sAsyncBuf += u16Chunk;
    sAsyncLeft = (Uint16)(sAsyncLeft - u16Chunk);

    return FM24C128D_ASYNC_BUSY;
}

/**
 * @brief 读若干字节（先写地址，再重复 START 读）
 */
Uint16 Fm24c128dRead(Uint16 u16Addr, Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Ret;
    Uint16 u16Idx;

    if ((gFmInited == 0U) || (pu16Buf == (Uint16 *)0) || (u16Len == 0U))
    {
        return FM24C128D_ERR_PARAM;
    }

    if (((Uint32)u16Addr + (Uint32)u16Len) > (Uint32)Fm24c128dSize())
    {
        gFmErrCnt++;
        return FM24C128D_ERR_PARAM;
    }

    /* 写周期没结束前读会拿到旧数据/被 NACK，先等 */
    if (gFmNeedWait != 0U)
    {
        u16Ret = Fm24c128dWaitReady();
        if (u16Ret != FM24C128D_OK)
        {
            gFmErrCnt++;
            return u16Ret;
        }
    }

    /* 第一段：START + 设备地址(写) + 2 字节字地址 */
    Hw_I2cStart();

    u16Ret = Hw_I2cWriteByte((Uint16)((gFmCfg.u16DevAddr7 & 0x007FU) << 1U));
    if (u16Ret == HW_I2C_OK)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)((u16Addr >> 8U) & 0x00FFU));
    }
    if (u16Ret == HW_I2C_OK)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)(u16Addr & 0x00FFU));
    }

    /* 第二段：重复 START + 设备地址(读) + 连续读 */
    if (u16Ret == HW_I2C_OK)
    {
        Hw_I2cStart();
        u16Ret = Hw_I2cWriteByte((Uint16)(((gFmCfg.u16DevAddr7 & 0x007FU) << 1U) | 0x0001U));
    }

    for (u16Idx = 0U; (u16Idx < u16Len) && (u16Ret == HW_I2C_OK); u16Idx++)
    {
        /* 除最后一个字节外都回 ACK，让器件继续吐数据 */
        pu16Buf[u16Idx] = Hw_I2cReadByte((u16Idx < (Uint16)(u16Len - 1U)) ? 1U : 0U);
    }

    Hw_I2cStop();

    if (u16Ret != HW_I2C_OK)
    {
        gFmErrCnt++;
        return (u16Ret == HW_I2C_ERR_PARAM) ? FM24C128D_ERR_PARAM : FM24C128D_ERR_I2C;
    }

    gFmReadCnt++;

    return FM24C128D_OK;
}

/**
 * @brief 控制写保护脚
 */
void Fm24c128dWriteProtect(Uint16 bProtect)
{
    Uint16 u16Level;

    /* 配置里给出的是"写保护有效电平"，这里按 bProtect 反推物理电平 */
    u16Level = (bProtect != 0U) ? gFmCfg.u16WpActiveLevel
                                : (Uint16)((gFmCfg.u16WpActiveLevel != 0U) ? 0U : 1U);

    Fm24c128dWpLevel(u16Level);
}

/**
 * @brief 读器件是否在线
 */
Uint16 Fm24c128dIsReady(void)
{
    Uint16 u16Ret;

    if (gFmInited == 0U)
    {
        return 0U;
    }

    Hw_I2cStart();
    u16Ret = Hw_I2cWriteByte((Uint16)((gFmCfg.u16DevAddr7 & 0x007FU) << 1U));
    Hw_I2cStop();

    return (u16Ret == HW_I2C_OK) ? 1U : 0U;
}

/**
 * @brief 自检：保留区写递增图样再读回比对
 */
Uint16 Fm24c128dSelfTest(void)
{
    Uint16 u16WrBuf[FM24C128D_SCRATCH_BYTES];
    Uint16 u16RdBuf[FM24C128D_SCRATCH_BYTES];
    Uint16 u16Idx;
    Uint16 u16Ret;

    if (gFmInited == 0U)
    {
        return FM24C128D_ERR_PARAM;
    }

    for (u16Idx = 0U; u16Idx < FM24C128D_SCRATCH_BYTES; u16Idx++)
    {
        u16WrBuf[u16Idx] = (Uint16)((0xA5U + (u16Idx * 7U)) & 0x00FFU);
        u16RdBuf[u16Idx] = 0x00U;
    }

    u16Ret = Fm24c128dWrite(gFmCfg.u16ScratchAddr, u16WrBuf, FM24C128D_SCRATCH_BYTES);
    if (u16Ret != FM24C128D_OK)
    {
        return u16Ret;
    }

    u16Ret = Fm24c128dRead(gFmCfg.u16ScratchAddr, u16RdBuf, FM24C128D_SCRATCH_BYTES);
    if (u16Ret != FM24C128D_OK)
    {
        return u16Ret;
    }

    for (u16Idx = 0U; u16Idx < FM24C128D_SCRATCH_BYTES; u16Idx++)
    {
        if (u16RdBuf[u16Idx] != u16WrBuf[u16Idx])
        {
            gFmErrCnt++;
            return FM24C128D_ERR_VERIFY;
        }
    }

    return FM24C128D_OK;
}

/**
 * @brief 统计量
 */
Uint16 Fm24c128dGetErrCnt(void)
{
    return gFmErrCnt;
}

Uint16 Fm24c128dGetWriteCnt(void)
{
    return gFmWriteCnt;
}

Uint16 Fm24c128dGetReadCnt(void)
{
    return gFmReadCnt;
}

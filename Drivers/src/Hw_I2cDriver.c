/**
 * @file Hw_I2cDriver.c
 * @author yetingheng (004182)
 * @brief 软件模拟 I2C 主机驱动实现
 * @version 0.1
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 时序约定（SCL 高电平期间改 SDA 只在 START/STOP 里出现）：
 *       START: SDA 高 -> SCL 高 -> SDA 低 -> SCL 低
 *       STOP : SDA 低 -> SCL 高 -> SDA 高
 *       写位 : 先摆 SDA，再拉高 SCL，延时，再拉低 SCL
 *       读位 : 先放 SDA（高阻），再拉高 SCL，延时，读 SDA，再拉低 SCL
 */
#include "Hw_I2cDriver.h"
#include "Hw_GpioDriver.h"

/* 当前配置，未初始化时为空指针 */
static const ST_I2C_CFG *gI2cCfg = (const ST_I2C_CFG *)0;

/* 初始化标志，未初始化时禁止任何总线动作 */
static Uint16 gI2cInited = 0U;

/* 半个位周期的延时，单位 us，初始化时按速率算好 */
static Uint16 gI2cHalfBitUs = 1U;

/* 1us 对应的空循环次数 */
static Uint16 gI2cLoopPerUs = HW_I2C_LOOP_PER_US_DEFAULT;

/* 统计量 */
static Uint16 gI2cNackCnt = 0U;
static Uint16 gI2cXferCnt = 0U;

/**
 * @brief 微秒级忙等延时
 *
 * @note 用 volatile 计数防止编译器把空循环优化掉。
 */
static void Hw_I2cDelayUs(Uint16 u16Us)
{
    volatile Uint32 u32Cnt;

    if (u16Us == 0U)
    {
        return;
    }

    u32Cnt = (Uint32)u16Us * (Uint32)gI2cLoopPerUs;
    while (u32Cnt > 0U)
    {
        u32Cnt--;
    }
}

/**
 * @brief 半个位周期的延时（SCL 高/低各占一半）
 */
static void Hw_I2cHalfBitDelay(void)
{
    Hw_I2cDelayUs(gI2cHalfBitUs);
}

/**
 * @brief 写 SDA 电平，1 = 释放（开漏高阻），0 = 拉低
 */
static void Hw_I2cSdaWrite(Uint16 u16Level)
{
    Hw_GpioWritePin((Uint32)gI2cCfg->u16SdaGpio, (u16Level != 0U) ? 1UL : 0UL);
}

/**
 * @brief 写 SCL 电平，1 = 释放（开漏高阻），0 = 拉低
 */
static void Hw_I2cSclWrite(Uint16 u16Level)
{
    Hw_GpioWritePin((Uint32)gI2cCfg->u16SclGpio, (u16Level != 0U) ? 1UL : 0UL);
}

/**
 * @brief 打一个 SCL 脉冲：高 -> 延时 -> 低
 */
static void Hw_I2cClockPulse(void)
{
    Hw_I2cSclWrite(1U);
    Hw_I2cHalfBitDelay();
    Hw_I2cSclWrite(0U);
    Hw_I2cHalfBitDelay();
}

/**
 * @brief 对外提供的微秒级忙等延时
 */
void Hw_I2cWaitUs(Uint16 u16Us)
{
    Hw_I2cDelayUs(u16Us);
}

/**
 * @brief 初始化软件 I2C 主机
 */
void Hw_I2cInit(const ST_I2C_CFG *pstI2cCfg)
{
    Uint32 u32Speed;

    if (pstI2cCfg == (const ST_I2C_CFG *)0)
    {
        return;
    }

    gI2cCfg = pstI2cCfg;

    /* 1us 循环次数：0 表示用缺省值 */
    gI2cLoopPerUs = (pstI2cCfg->u16LoopPerUs != 0U)
                  ? pstI2cCfg->u16LoopPerUs
                  : HW_I2C_LOOP_PER_US_DEFAULT;

    /* 半位周期 = 0.5 / f，单位 us；400k 时算出 1us，100k 时 5us */
    u32Speed = pstI2cCfg->u32SpeedHz;
    if (u32Speed == 0UL)
    {
        u32Speed = HW_I2C_SPEED_100K;
    }

    gI2cHalfBitUs = (Uint16)(500000UL / u32Speed);
    if (gI2cHalfBitUs == 0U)
    {
        gI2cHalfBitUs = 1U;
    }

    /* 总线空闲态：SDA/SCL 都释放；开漏引脚写 1 就是高阻，由外部上拉拉高 */
    Hw_I2cSclWrite(1U);
    Hw_I2cSdaWrite(1U);
    Hw_I2cDelayUs(gI2cHalfBitUs);

    gI2cInited = 1U;
    gI2cNackCnt = 0U;
    gI2cXferCnt = 0U;

    /* 上电若从机把 SDA 拉死（比如复位中途），先打脉冲把总线放开 */
    if (Hw_I2cReadSda() == 0U)
    {
        (void)Hw_I2cBusRecover();
    }
}

/**
 * @brief 读 SDA 当前电平
 */
Uint16 Hw_I2cReadSda(void)
{
    if (gI2cCfg == (const ST_I2C_CFG *)0)
    {
        return 1U;
    }

    return (Hw_GpioReadPin((Uint32)gI2cCfg->u16SdaGpio) != 0UL) ? 1U : 0U;
}

/**
 * @brief 发送 START 条件
 */
void Hw_I2cStart(void)
{
    if (gI2cInited == 0U)
    {
        return;
    }

    Hw_I2cSdaWrite(1U);         /* SDA 先回高，避免重发起步被当成 STOP */
    Hw_I2cSclWrite(1U);
    Hw_I2cHalfBitDelay();

    Hw_I2cSdaWrite(0U);         /* SCL 高电平期间 SDA 下降 = START */
    Hw_I2cHalfBitDelay();

    Hw_I2cSclWrite(0U);         /* 占用总线，SCL 拉低 */
    Hw_I2cHalfBitDelay();
}

/**
 * @brief 发送 STOP 条件
 */
void Hw_I2cStop(void)
{
    if (gI2cInited == 0U)
    {
        return;
    }

    Hw_I2cSdaWrite(0U);
    Hw_I2cHalfBitDelay();

    Hw_I2cSclWrite(1U);         /* 先放 SCL，再抬 SDA */
    Hw_I2cHalfBitDelay();

    Hw_I2cSdaWrite(1U);         /* SCL 高电平期间 SDA 上升 = STOP */
    Hw_I2cHalfBitDelay();
}

/**
 * @brief 发送一个字节并读应答位
 */
Uint16 Hw_I2cWriteByte(Uint16 u16Byte)
{
    Uint16 u16Bit;
    Uint16 u16Ack;

    if (gI2cInited == 0U)
    {
        return HW_I2C_ERR_PARAM;
    }

    /* MSB 先发；SCL 低电平期间改 SDA */
    for (u16Bit = 0U; u16Bit < 8U; u16Bit++)
    {
        Hw_I2cSdaWrite(((u16Byte & 0x0080U) != 0U) ? 1U : 0U);
        u16Byte = (Uint16)(u16Byte << 1U);

        Hw_I2cSclWrite(1U);
        Hw_I2cHalfBitDelay();
        Hw_I2cSclWrite(0U);
        Hw_I2cHalfBitDelay();
    }

    /* 第 9 个时钟：释放 SDA，读从机应答，0 = ACK */
    Hw_I2cSdaWrite(1U);
    Hw_I2cHalfBitDelay();

    Hw_I2cSclWrite(1U);
    Hw_I2cHalfBitDelay();
    u16Ack = Hw_I2cReadSda();
    Hw_I2cSclWrite(0U);
    Hw_I2cHalfBitDelay();

    if (u16Ack != 0U)
    {
        gI2cNackCnt++;
        return HW_I2C_ERR_NACK;
    }

    return HW_I2C_OK;
}

/**
 * @brief 读一个字节并回应答位
 */
Uint16 Hw_I2cReadByte(Uint16 bAck)
{
    Uint16 u16Bit;
    Uint16 u16Data = 0U;

    if (gI2cInited == 0U)
    {
        return 0U;
    }

    /* 释放 SDA，按位读；SCL 高电平期间数据有效 */
    Hw_I2cSdaWrite(1U);

    for (u16Bit = 0U; u16Bit < 8U; u16Bit++)
    {
        u16Data = (Uint16)(u16Data << 1U);

        Hw_I2cSclWrite(1U);
        Hw_I2cHalfBitDelay();

        if (Hw_I2cReadSda() != 0U)
        {
            u16Data |= 0x0001U;
        }

        Hw_I2cSclWrite(0U);
        Hw_I2cHalfBitDelay();
    }

    /* 第 9 个时钟回 ACK/NACK：0 = 继续读，1 = 最后一个字节 */
    Hw_I2cSdaWrite((bAck != 0U) ? 0U : 1U);
    Hw_I2cHalfBitDelay();

    Hw_I2cSclWrite(1U);
    Hw_I2cHalfBitDelay();
    Hw_I2cSclWrite(0U);
    Hw_I2cHalfBitDelay();

    /* 交还总线：SDA 复位成释放态，避免影响下一个 START */
    Hw_I2cSdaWrite(1U);

    return (Uint16)(u16Data & 0x00FFU);
}

/**
 * @brief 完整写事务：START + 设备地址(写) + 数据 + STOP
 */
Uint16 Hw_I2cWriteTo(Uint16 u16DevAddr, const Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Idx;
    Uint16 u16Ret;

    if ((gI2cInited == 0U) || (pu16Buf == (const Uint16 *)0))
    {
        return HW_I2C_ERR_PARAM;
    }

    Hw_I2cStart();

    u16Ret = Hw_I2cWriteByte((Uint16)((u16DevAddr & 0x007FU) << 1U));
    if (u16Ret != HW_I2C_OK)
    {
        Hw_I2cStop();
        return u16Ret;
    }

    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        u16Ret = Hw_I2cWriteByte((Uint16)(pu16Buf[u16Idx] & 0x00FFU));
        if (u16Ret != HW_I2C_OK)
        {
            Hw_I2cStop();
            return u16Ret;
        }
    }

    Hw_I2cStop();
    gI2cXferCnt++;

    return HW_I2C_OK;
}

/**
 * @brief 完整读事务：START + 设备地址(读) + 数据 + STOP
 */
Uint16 Hw_I2cReadFrom(Uint16 u16DevAddr, Uint16 *pu16Buf, Uint16 u16Len)
{
    Uint16 u16Idx;
    Uint16 u16Ret;

    if ((gI2cInited == 0U) || (pu16Buf == (Uint16 *)0) || (u16Len == 0U))
    {
        return HW_I2C_ERR_PARAM;
    }

    Hw_I2cStart();

    u16Ret = Hw_I2cWriteByte((Uint16)(((u16DevAddr & 0x007FU) << 1U) | 0x0001U));
    if (u16Ret != HW_I2C_OK)
    {
        Hw_I2cStop();
        return u16Ret;
    }

    for (u16Idx = 0U; u16Idx < u16Len; u16Idx++)
    {
        /* 除最后一个字节外都回 ACK */
        pu16Buf[u16Idx] = Hw_I2cReadByte((u16Idx < (Uint16)(u16Len - 1U)) ? 1U : 0U);
    }

    Hw_I2cStop();
    gI2cXferCnt++;

    return HW_I2C_OK;
}

/**
 * @brief 总线恢复：打 9 个 SCL 脉冲 + STOP
 */
Uint16 Hw_I2cBusRecover(void)
{
    Uint16 u16Idx;

    if (gI2cCfg == (const ST_I2C_CFG *)0)
    {
        return 0U;
    }

    /* 恢复不依赖 gI2cInited，上电初始化阶段就要能用 */
    Hw_I2cSdaWrite(1U);

    for (u16Idx = 0U; u16Idx < HW_I2C_RECOVER_PULSE_MAX; u16Idx++)
    {
        if (Hw_I2cReadSda() != 0U)
        {
            break;              /* SDA 已经放开，不用继续打 */
        }

        Hw_I2cClockPulse();
    }

    /* 补一个 STOP，让从机回到空闲态 */
    Hw_I2cSdaWrite(0U);
    Hw_I2cHalfBitDelay();
    Hw_I2cSclWrite(1U);
    Hw_I2cHalfBitDelay();
    Hw_I2cSdaWrite(1U);
    Hw_I2cHalfBitDelay();

    return (Hw_I2cReadSda() != 0U) ? 1U : 0U;
}

/**
 * @brief 读 NACK 次数
 */
Uint16 Hw_I2cGetNackCnt(void)
{
    return gI2cNackCnt;
}

/**
 * @brief 读完整事务次数
 */
Uint16 Hw_I2cGetXferCnt(void)
{
    return gI2cXferCnt;
}

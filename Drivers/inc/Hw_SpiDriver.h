/**
 * @file Hw_SpiDriver.h
 * @author yetingheng (004182)
 * @brief SPI 主机驱动（查询方式，非 FIFO），本板 SPI-A 与 ARM 芯片 STM32F103VET6 通信
 * @version 0.1
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 引脚分配（GPIO 表里已配好，本驱动不重复配置）：
 *       - GPIO54 SPISIMOA  MOSI  主出从入
 *       - GPIO55 SPISOMIA  MISO  主入从出（开漏）
 *       - GPIO56 SPICLKA   SCLK
 *       - GPIO57 SPISTEA   片选，本驱动按"软件片选"使用：
 *         因为 C28x 的 SPISTE 在主机模式下每个字符都会自动翻转，
 *         多字节帧之间会释放从机，STM32 那边会误判帧结束，
 *         所以片选改由 GPIO 手动控制，NSS 语义由本驱动保证。
 *
 * @note 时钟：LSPCLK = SYSCLK/4 = 50MHz（device.c 里 SysCtl_setLowSpeedClock 固定 4 分频），
 *       波特率分频 SPIBRR 的语义是：
 *         BRR = 0~2  ->  LSPCLK/4（固定，最快 12.5MHz）
 *         BRR = 3~127 -> LSPCLK/(BRR+1)
 *       初始化时按配置的目标波特率反算 BRR，取最接近的有效值；
 *       实际波特率可以通过 Hw_SpiGetBaud() 读回来核对。
 *
 * @note 字符宽度：C28x 的 SPITXBUF 是"数据左对齐"的，16 位模式下直接写，
 *       少于 16 位时必须左移 (16 - 字符长度) 位；接收同理要右移回来。
 *       本驱动内部统一处理，上层只看到"低 N 位有效"的数值。
 */
#ifndef HW_SPI_DRIVER_H
#define HW_SPI_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 模块编号
 ******************************************************************************/
#define HW_SPI_ID_A                 (0U)    /* SPI-A：GPIO54~57，接 ARM */
#define HW_SPI_ID_B                 (1U)    /* SPI-B：本板未使用 */
#define HW_SPI_ID_C                 (2U)    /* SPI-C：本板未使用 */
#define HW_SPI_ID_MAX               (3U)

/******************************************************************************
 * 返回值
 ******************************************************************************/
#define HW_SPI_OK                   (0U)    /* 成功 */
#define HW_SPI_ERR_PARAM            (1U)    /* 入参非法或模块未初始化 */
#define HW_SPI_ERR_TIMEOUT          (2U)    /* 等发送缓冲空/等接收标志超时 */
#define HW_SPI_ERR_BUSY             (3U)    /* 上一帧还没结束（不应出现） */

/******************************************************************************
 * 缺省值
 ******************************************************************************/
/* 空循环延时标定：200MHz、-O2 下 while 循环约 4~5 个周期，取 50 次 ≈ 1us。
 * 只用于超时计数，不需要精确，超时值宁可大一点，避免误报。 */
#define HW_SPI_LOOP_PER_US          (50UL)

/* 片选无效值：表示该模块不用软件片选 */
#define HW_SPI_NO_CS_GPIO           (0xFFFFU)

/* 硬件最大字符宽度 */
#define HW_SPI_CHAR_MAX             (16U)

/******************************************************************************
 * 配置
 ******************************************************************************/
typedef struct
{
    Uint16 u16SpiId;            /* 模块号 HW_SPI_ID_A/B/C */
    Uint16 u16CharLen;          /* 字符宽度 1~16 位，本板接 ARM 用 8 或 16 */
    Uint16 u16ClkPolarity;      /* 0: 空闲低电平；1: 空闲高电平（CPOL） */
    Uint16 u16ClkPhase;         /* 0: 第一个边沿采样；1: 第二个边沿采样（CPHA） */
    Uint32 u32BaudHz;           /* 目标波特率，实际值见 Hw_SpiGetBaud() */
    Uint16 u16CsGpio;           /* 软件片选引脚号，0xFFFF 表示不用 */
    Uint16 u16CsActiveLevel;    /* 片选有效电平：0 低有效 / 1 高有效 */
    Uint16 u16TimeoutUs;        /* 单字节传输超时（微秒），0 表示用缺省 2000us */
} ST_SPI_CFG_ITEM;

typedef struct
{
    const ST_SPI_CFG_ITEM *pstItem;  /* 配置项数组 */
    Uint16 u16ItemNum;               /* 有效项数 */
} ST_SPI_CFG;

/**
 * @brief 初始化 SPI 主机
 *
 * @param pstSpiCfg 配置表
 * @note 内部关闭 FIFO，走经典非 FIFO 收发（收发各 1 级缓冲，查询方式够用）。
 *       片选引脚必须先在 GPIO 表里配成普通输出，本驱动只做电平翻转。
 */
extern void Hw_SpiInit(const ST_SPI_CFG *pstSpiCfg);

/**
 * @brief 全双工收发一个字符
 *
 * @param u16SpiId 模块号
 * @param u16Tx    待发数据（低 u16CharLen 位有效）
 * @param pu16Rx   接收数据，可为 NULL 表示不关心
 * @return HW_SPI_OK / HW_SPI_ERR_PARAM / HW_SPI_ERR_TIMEOUT
 */
extern Uint16 Hw_SpiTransfer(Uint16 u16SpiId, Uint16 u16Tx, Uint16 *pu16Rx);

/**
 * @brief 收发多个字符（片选只在整帧开始/结束时动）
 *
 * @param u16SpiId 模块号
 * @param pu16Tx   发送缓冲，一个 Uint16 一个字符；NULL 表示只收（发 0xFFFF 占位）
 * @param pu16Rx   接收缓冲，NULL 表示只发
 * @param u16Len   字符数
 * @return HW_SPI_OK / HW_SPI_ERR_PARAM / HW_SPI_ERR_TIMEOUT
 *
 * @note 这是对 STM32 通信的主入口：一帧数据期间片选保持有效。
 */
extern Uint16 Hw_SpiTransferBuf(Uint16 u16SpiId, const Uint16 *pu16Tx,
                                Uint16 *pu16Rx, Uint16 u16Len);

/**
 * @brief 只发不收
 */
extern Uint16 Hw_SpiWrite(Uint16 u16SpiId, const Uint16 *pu16Tx, Uint16 u16Len);

/**
 * @brief 只收不发
 */
extern Uint16 Hw_SpiRead(Uint16 u16SpiId, Uint16 *pu16Rx, Uint16 u16Len);

/**
 * @brief 手动控制片选
 *
 * @param bSelect 1: 选中（输出有效电平），0: 释放
 * @note 给需要"片选有效 -> 延时 -> 收发"的从机时序用；正常收发不用管，
 *       Hw_SpiTransferBuf 内部已经处理。
 */
extern void Hw_SpiSelect(Uint16 u16SpiId, Uint16 bSelect);

/**
 * @brief 读实际配置出来的波特率（诊断用）
 *
 * @return 实际波特率 Hz，模块未初始化返回 0
 */
extern Uint32 Hw_SpiGetBaud(Uint16 u16SpiId);

/**
 * @brief 统计量：超时次数 / 完成字符数
 */
extern Uint16 Hw_SpiGetErrCnt(Uint16 u16SpiId);
extern Uint32 Hw_SpiGetXferCnt(Uint16 u16SpiId);

#ifdef __cplusplus
}
#endif

#endif /* HW_SPI_DRIVER_H */

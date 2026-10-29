/**
 * @file Fm24c128d.h
 * @author yetingheng (004182)
 * @brief FM24C128D（128Kbit I2C EEPROM）器件驱动
 * @version 0.2
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 器件要点（复旦微 FM24C128D，标准 24C128 家族）：
 *       - 容量 128Kbit = 16384 字节，2 字节字地址（A13~A0），
 *         设备地址高 4 位固定 1010，低 3 位 A2A1A0，本板全接地 => 0x50；
 *       - 页大小 64 字节，一次写不能跨页，跨页必须在驱动里拆开；
 *       - 写完有内部烧写周期 tWR（最大 5ms），这期间器件不回 ACK，
 *         所以每次写页之后要做"ACK 轮询"等它写完，不能只靠固定延时；
 *       - WP 拉高保护高半区（0x2000~0x3FFF），低半区始终可写，
 *         所以参数区放低半区，即使 WP 策略配错也不会把参数写坏 -> 见 EepromParam.h；
 *       - 擦写寿命 100 万次，比 FRAM 低 7 个数量级，写操作必须省着用
 *         （谁来决定写、多久写一次，全部由 EepromParam 层负责）。
 *
 * @note 缓冲约定与工程里其他驱动一致：一个 Uint16 存一个字节（低 8 位有效），
 *       这样和 SCI / CRC16 的接口可以直传，不需要再转一层。
 */
#ifndef FM24C128D_H
#define FM24C128D_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 器件常量
 ******************************************************************************/
#define FM24C128D_SIZE_BYTES        (16384U)    /* 128Kbit = 16KB */
#define FM24C128D_PAGE_SIZE         (64U)       /* 写页边界，跨页必须拆分 */
#define FM24C128D_ADDR_BYTES        (2U)        /* 2 字节字地址 */
#define FM24C128D_DEV_ADDR_7BIT     (0x50U)     /* 1010 000，A2A1A0 全接地 */
#define FM24C128D_SCRATCH_BYTES     (16U)       /* 自检保留区大小 */
#define FM24C128D_HALF_SIZE         (8192U)     /* WP 保护边界：>= 此地址属高半区 */

/* 写周期等待：tWR 最大 5ms。ACK 轮询一次包含"START + 设备地址 + STOP"
 * （400kHz 下约 30~40us）再加 10us 间隔，500 次 ≈ 20ms 总超时，
 * 既覆盖最坏 tWR，也不会在器件真坏时把主循环卡死。 */
#define FM24C128D_ACK_POLL_CNT      (500U)
#define FM24C128D_ACK_POLL_US       (10U)

/* 返回值 */
#define FM24C128D_OK                (0U)
#define FM24C128D_ERR_PARAM         (1U)        /* 入参越界或驱动未初始化 */
#define FM24C128D_ERR_I2C           (2U)        /* I2C 传输失败 */
#define FM24C128D_ERR_VERIFY        (3U)        /* 回读校验失败 */
#define FM24C128D_ERR_TIMEOUT       (4U)        /* 等写周期完成超时 */

/******************************************************************************
 * 器件配置
 ******************************************************************************/
typedef struct
{
    Uint16 u16DevAddr7;         /* 7 位设备地址，A2A1A0 接地时为 0x50 */
    Uint16 u16SizeBytes;        /* 有效容量，0 表示器件缺省 16384 */
    Uint16 u16WpGpio;           /* 写保护引脚号，本板 GPIO99；0xFFFF 表示没接 */
    Uint16 u16WpActiveLevel;    /* 写保护有效电平，1 = 高电平保护（24C128 语义） */
    Uint16 u16ScratchAddr;      /* 自检保留区首地址，0 表示缺省 0x1F00（低半区） */
    Uint16 bEnableSelfTest;     /* 1: 初始化时做一次写读自检 */
} ST_FM24C128D_CFG;

/**
 * @brief 初始化 FM24C128D
 *
 * @note 内部会先把 WP 放到"允许写"，然后可选地跑一次自检（写读比对）。
 *       调用前必须先完成 Hw_I2cInit()。
 */
extern void Fm24c128dInit(const ST_FM24C128D_CFG *pstFmCfg);

/**
 * @brief 从任意地址写若干字节，自动按 64 字节页边界拆分
 *
 * @param u16Addr  起始字地址（0 ~ size-1）
 * @param pu16Buf  数据，一个 Uint16 存一个字节
 * @param u16Len   字节数
 * @return FM24C128D_OK / FM24C128D_ERR_PARAM / FM24C128D_ERR_I2C / FM24C128D_ERR_TIMEOUT
 *
 * @note 函数入口先把 WP 拉低（允许写），写完再按配置恢复保护；
 *       每页写完做 ACK 轮询等待 tWR，轮询超时返回 FM24C128D_ERR_TIMEOUT。
 */
extern Uint16 Fm24c128dWrite(Uint16 u16Addr, const Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 从任意地址读若干字节（顺序读，跨页由器件自动回卷）
 *
 * @param u16Addr  起始字地址（0 ~ size-1）
 * @param pu16Buf  接收缓冲，一个 Uint16 存一个字节
 * @param u16Len   字节数
 * @return FM24C128D_OK / FM24C128D_ERR_PARAM / FM24C128D_ERR_I2C
 */
extern Uint16 Fm24c128dRead(Uint16 u16Addr, Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 控制写保护脚
 *
 * @param bProtect 1: 拉高 WP（保护高半区），0: 拉低 WP（全片可写）
 */
extern void Fm24c128dWriteProtect(Uint16 bProtect);

/**
 * @brief 等器件写完当前内部烧写周期（ACK 轮询）
 *
 * @return FM24C128D_OK 表示器件已应答，FM24C128D_ERR_TIMEOUT 表示超时
 * @note 由 Fm24c128dWrite 内部调用，一般不直接调；写周期内读操作也应先等这里返回。
 */
extern Uint16 Fm24c128dWaitReady(void);

/**
 * @brief 读器件是否在线（发设备地址看有没有 ACK）
 *
 * @return 1 在线，0 不在线
 */
extern Uint16 Fm24c128dIsReady(void);

/**
 * @brief 自检：在保留区写入递增图样再读回比对
 *
 * @return FM24C128D_OK 表示读回一致
 * @note 保留区不在参数区范围内，参数保存层不会使用它。
 */
extern Uint16 Fm24c128dSelfTest(void);

#define FM24C128D_ASYNC_BUSY        (5U)        /* 异步写未完成，下一拍继续轮询 */
#define FM24C128D_ASYNC_CHUNK_WORDS (32U)       /* 单次轮询最多送出的字数，限制单次阻塞时间 */
#define FM24C128D_ASYNC_RETRY_MAX   (100U)      /* 等芯片就绪的最大轮询次数 */

/**
 * @brief 异步写：启动后每拍调用一次 Fm24c128dWriteAsyncPoll 推进
 *
 * @return 启动结果：FM24C128D_ASYNC_BUSY 表示已开始，错误码表示未能启动
 */
extern Uint16 Fm24c128dWriteAsyncStart(Uint16 u16Addr, const Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 异步写推进：送一片数据或等待上一片烧完
 *
 * @return FM24C128D_OK 全部写完；FM24C128D_ASYNC_BUSY 未完成；其它为错误码
 */
extern Uint16 Fm24c128dWriteAsyncPoll(void);
/**
 * @brief 统计量
 */
extern Uint16 Fm24c128dGetErrCnt(void);
extern Uint16 Fm24c128dGetWriteCnt(void);
extern Uint16 Fm24c128dGetReadCnt(void);

#ifdef __cplusplus
}
#endif

#endif /* FM24C128D_H */

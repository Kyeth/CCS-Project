/******************************************************************************
 * @file    UpgradeCore.h
 * @brief   OTA 升级核心：分区、扇区擦除、备份区写入、配置区提交、CRC32
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    分区（CMD/2837xS_FLASH_APP_CLA_lnk.cmd 与 HANDOVER §8 一致）：
 *            BOOT    A+B+C   0x080000~0x085FFF   0x6000 字（24K 字 = 48KiB）
 *            APP     D+E+F+G 0x086000~0x09FFFF   0x1A000 字
 *            BACKUP  H+I+J+K 0x0A0000~0x0B9FFF   0x1A000 字（106,496 字 = 208KiB）
 *            SPARE   L+M     0x0BA000~0x0BDFFF
 *            CFG     N       0x0BE000~0x0BFFEF   0x1FF0 字
 *          注意：链接脚本里的 "104KB" 是"104K 字"，折算字节是 208KiB；本文件的
 *          容量常量一律用字节表述（APP 上限 0x34000 字节 = 0x1A000 字）。
 *
 * @note    文件头 32 字节按字节显式解析，不把串口数组 memcpy 成结构体
 *          （HANDOVER §8.4）：[0] ChipType、[1] FirmwareType、[2..5] 正文长度
 *          （小端，单位字节）、[6..9] 正文 CRC32（小端）、[10..31] 保留。
 *          文件头本身不写进备份区，正文从 0x0A0000 起。
 *
 * @note    CRC32 参数（HANDOVER §8.4，已验证 "123456789" → 0x0376E6E7）：
 *          初值 0xFFFFFFFF、多项式 0x04C11DB7、**不反射**、无最终异或。
 *
 * @note    Flash 操作经 ST_UPGRADE_FLASH_OPS 注入：生产用 F021，主机测试用替身。
 *          写入按 8 字（128 位）块对齐下发，末块用 0xFFFF 补齐；
 *          每次下发至少覆盖一个完整 64 位（4 字）编程单元，同一单元不重复写。
 ******************************************************************************/

#ifndef UPGRADECORE_H
#define UPGRADECORE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 分区常量 ---------------- */

#define UPGRADE_BACKUP_BASE         (0x0A0000UL)
#define UPGRADE_BACKUP_WORDS        (0x1A000UL)
#define UPGRADE_CFG_BASE            (0x0BE000UL)
#define UPGRADE_CFG_WORDS           (0x1FF0UL)

/** 备份区容量（字节）＝ 0x1A000 字 × 2：串口每字节一个 Uint16，落盘两字节拼一字 */
#define UPGRADE_BACKUP_BYTES        (0x34000UL)
/** APP 固件上限（字节），与备份区容量相同 */
#define UPGRADE_APP_MAX_BYTES       (UPGRADE_BACKUP_BYTES)
/** BOOT 固件上限（字节）＝ A+B+C 0x6000 字 × 2 */
#define UPGRADE_BOOT_MAX_BYTES      (0xC000UL)

#define UPGRADE_FILE_HEAD_LEN       (32U)
#define UPGRADE_BLOCK_WORDS         (8U)        /* 128 位编程块 */
#define UPGRADE_CFG_FLAG            (0xAAEEU)

#define UPGRADE_BACKUP_SECTOR_NUM   (4U)        /* 备份区 H/I/J/K 四个扇区 */

#define UPGRADE_BACKUP_SECTOR_NUM   (4U)        /* 备份区 H/I/J/K 四个扇区 */

#define UPGRADE_FW_APP              (1U)
#define UPGRADE_FW_BOOT             (2U)

/* ---------------- 状态码 ---------------- */

#define UPGRADE_OK                  (0U)
#define UPGRADE_ERR_PARAM           (1U)
#define UPGRADE_ERR_FLASH           (2U)
#define UPGRADE_ERR_HEAD            (3U)
#define UPGRADE_ERR_SIZE            (4U)
#define UPGRADE_ERR_CRC             (5U)
#define UPGRADE_ERR_STATE           (6U)

/* ---------------- 类型 ---------------- */

/** 一个可擦除扇区 */
typedef struct
{
    Uint32 u32Start;
    Uint32 u32Words;
} ST_UPGRADE_SECTOR;

/** Flash 操作（生产实现见 UpgradeFlash.c，主机测试用替身） */
typedef struct
{
    /** 擦除一个扇区，返回 1 = 成功 */
    Uint16 (*pfEraseSector)(Uint32 u32SectorStart);
    /** 编程一个 8 字块（地址 128 位对齐），返回 1 = 成功 */
    Uint16 (*pfProgramBlock)(Uint32 u32Addr, const Uint16 *pu16Data, Uint16 u16Words);
    /** 回读若干字，返回 1 = 成功 */
    Uint16 (*pfReadWords)(Uint32 u32Addr, Uint16 *pu16Dst, Uint16 u16Words);
} ST_UPGRADE_FLASH_OPS;

/** 解析后的文件头 */
typedef struct
{
    Uint16 u16ChipType;
    Uint16 u16FirmwareType;
    Uint32 u32FileSize;
    Uint32 u32FileCrc;
} ST_UPGRADE_FILE_HEAD;

/** 配置区内容 */
typedef struct
{
    Uint16 u16UpgradeFlag;      /* 0xAAEE = 有效 */
    Uint16 u16FirmwareType;
    Uint32 u32FileSize;
    Uint32 u32FileCrc;
    Uint16 u16Rsv[6];
} ST_UPGRADE_CFG;

/** 升级上下文 */
typedef struct
{
    const ST_UPGRADE_FLASH_OPS *pstOps;

    Uint32 u32WriteAddr;                        /* 下一个待写的 Flash 地址 */
    Uint32 u32BodyWords;                        /* 已写正文的字数 */
    Uint32 u32BodyBytes;                        /* 已收正文字节数（上限 0x34000，不能用 Uint16） */
    Uint32 u32Crc;                              /* 正文 CRC32 运行值 */
    Uint16 au16Stage[UPGRADE_BLOCK_WORDS];      /* 攒块缓冲 */
    Uint16 u16StageCount;
    Uint16 u16ByteOdd;                          /* 1 = 手上压着半个字（等待第二个字节） */
    Uint16 u16PendingByte;                      /* 压着的那个字节 */
    Uint16 u16Writing;                          /* 1 = 正在写正文 */
} ST_UPGRADE_CTX;

/* ---------------- 扇区表 ---------------- */

/** 备份区四个扇区（H/I/J/K，长度不等，不能假设等长） */
extern const ST_UPGRADE_SECTOR g_astUpgradeBackupSector[4];

/* ---------------- 接口 ---------------- */

Uint16 UpgradeCoreInit(const ST_UPGRADE_FLASH_OPS *pstOps);

/** 整区擦除备份区（H/I/J/K，按各自起点） */
Uint16 UpgradeCoreEraseBackup(void);

/**
 * @brief  擦除备份区的第 u16Index 个扇区（0..3）
 * @note   走注入的 pfEraseSector，主机测试才能替换成替身；升级任务每轮擦一个，
 *         擦完一个就返回调度器，不在一个轮次里连擦四个扇区。
 */
Uint16 UpgradeCoreEraseBackupSector(Uint16 u16Index);

/**
 * @brief  擦除备份区的第 u16Index 个扇区（0..3）
 * @note   走注入的 pfEraseSector，主机测试才能替换成替身；升级任务每轮擦一个，
 *         擦完一个就返回调度器，不在一个轮次里连擦四个扇区。
 */
Uint16 UpgradeCoreEraseBackupSector(Uint16 u16Index);

/** 开始接收正文：复位写指针、CRC 与攒块缓冲 */
Uint16 UpgradeCoreStartBody(void);

/**
 * @brief  写入一段正文
 * @param  pu16Data  正文（每字节一个 Uint16，低 8 位有效）
 * @param  u16Length 字节数
 */
Uint16 UpgradeCoreWriteBody(const Uint16 *pu16Data, Uint16 u16Length);

/** 结束写入：把攒块缓冲用 0xFFFF 补齐后下发 */
Uint16 UpgradeCoreEndBody(void);

Uint32 UpgradeCoreBodyWords(void);

Uint32 UpgradeCoreCrc(void);

/** 按字节解析 32 字节文件头 */
Uint16 UpgradeCoreParseHead(const Uint16 *pu16Head, ST_UPGRADE_FILE_HEAD *pstHead);

/**
 * @brief  校验文件头（机型、固件类型、长度上限、CRC 非零）
 * @param  u16ChipType 本机型编码
 */
Uint16 UpgradeCoreCheckHead(const ST_UPGRADE_FILE_HEAD *pstHead, Uint16 u16ChipType);

/** 提交配置（写配置区并回读确认） */
Uint16 UpgradeCoreCommitCfg(Uint16 u16FirmwareType, Uint32 u32FileSize, Uint32 u32FileCrc);

/** 读配置，返回 UPGRADE_OK 且 u16UpgradeFlag == 0xAAEE 才算有效 */
Uint16 UpgradeCoreReadCfg(ST_UPGRADE_CFG *pstCfg);

Uint16 UpgradeCoreClearCfg(void);

/** CRC32（不反射，poly 0x04C11DB7，初值 0xFFFFFFFF，无最终异或） */
Uint32 UpgradeCoreCrc32(Uint32 u32Crc, const Uint16 *pu16Data, Uint16 u16Length);

#ifdef __cplusplus
}
#endif

#endif

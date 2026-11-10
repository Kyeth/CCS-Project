/******************************************************************************
 * @file    UpgradeCore.c
 * @brief   OTA 升级核心实现
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    字节 → Flash 字的装填规则：串口收到的每个字节占一个 Uint16 的低 8 位，
 *          写进 Flash 时**两个连续字节拼成一个 16 位 Flash 字**（先到的在低字节）。
 *          因此 0x1A000 字的备份区正好装 0x34000 = 212,992 字节。
 *          字节数为奇数时，最后一个字的高字节补 0xFF（擦除态）。
 *
 * @note    写入顺序写死在代码里：先擦 H/I/J/K，再按 8 字块顺序写正文，
 *          最后写配置并回读确认。任何一步失败都返回错误码，由上层决定
 *          回 CAN 并交还端口——本文件不发送任何串口字符。
 ******************************************************************************/

#include "UpgradeCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 数据 ---------------- */

/** 备份区扇区表：E~J 是 32K 字扇区，K~N 是 8K 字扇区，长度不等 */
const ST_UPGRADE_SECTOR g_astUpgradeBackupSector[4] =
{
    { 0x0A0000UL, 0x8000UL },   /* H */
    { 0x0A8000UL, 0x8000UL },   /* I */
    { 0x0B0000UL, 0x8000UL },   /* J */
    { 0x0B8000UL, 0x2000UL }    /* K */
};

static ST_UPGRADE_CTX s_stUpgradeCtx;

/* ---------------- 内部 ---------------- */

/**
 * @brief  下发一个 8 字块
 */
static Uint16 UpgradeProgramStage(void)
{
    Uint16 i;

    if (s_stUpgradeCtx.u16StageCount != UPGRADE_BLOCK_WORDS)
    {
        return UPGRADE_ERR_STATE;
    }

    if ((s_stUpgradeCtx.u32WriteAddr < UPGRADE_BACKUP_BASE) ||
        ((s_stUpgradeCtx.u32WriteAddr + UPGRADE_BLOCK_WORDS) >
         (UPGRADE_BACKUP_BASE + UPGRADE_BACKUP_WORDS)))
    {
        return UPGRADE_ERR_SIZE;
    }

    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfProgramBlock == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    if (s_stUpgradeCtx.pstOps->pfProgramBlock(s_stUpgradeCtx.u32WriteAddr,
                                              s_stUpgradeCtx.au16Stage,
                                              UPGRADE_BLOCK_WORDS) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    s_stUpgradeCtx.u32WriteAddr = s_stUpgradeCtx.u32WriteAddr + UPGRADE_BLOCK_WORDS;
    s_stUpgradeCtx.u16StageCount = 0U;

    for (i = 0U; i < UPGRADE_BLOCK_WORDS; i++)
    {
        s_stUpgradeCtx.au16Stage[i] = 0U;
    }

    return UPGRADE_OK;
}

/**
 * @brief  把一个 Flash 字压进攒块缓冲，满 8 字就下发
 */
static Uint16 UpgradePushWord(Uint16 u16Word)
{
    Uint16 u16Ret;

    s_stUpgradeCtx.au16Stage[s_stUpgradeCtx.u16StageCount] = u16Word;
    s_stUpgradeCtx.u16StageCount++;
    s_stUpgradeCtx.u32BodyWords++;

    if (s_stUpgradeCtx.u16StageCount == UPGRADE_BLOCK_WORDS)
    {
        u16Ret = UpgradeProgramStage();
        if (u16Ret != UPGRADE_OK)
        {
            s_stUpgradeCtx.u16Writing = 0U;
            return u16Ret;
        }
    }

    return UPGRADE_OK;
}

/**
 * @brief  从 16 位字数组里取一段小端 32 位
 */
static Uint32 UpgradeReadUint32(const Uint16 *pu16Buff, Uint16 u16Index)
{
    Uint32 u32Val;

    u32Val = (Uint32)(pu16Buff[u16Index] & 0x00FFU);
    u32Val = u32Val | ((Uint32)(pu16Buff[(Uint16)(u16Index + 1U)] & 0x00FFU) << 8);
    u32Val = u32Val | ((Uint32)(pu16Buff[(Uint16)(u16Index + 2U)] & 0x00FFU) << 16);
    u32Val = u32Val | ((Uint32)(pu16Buff[(Uint16)(u16Index + 3U)] & 0x00FFU) << 24);

    return u32Val;
}

/**
 * @brief  配置区镜像：12 个字（不是字节流，直接按字组装）
 *         [0] 标志、[1] 固件类型、[2..3] 长度 lo/hi、[4..5] CRC lo/hi、[6..11] 保留
 */
static void UpgradeBuildCfg(const ST_UPGRADE_CFG *pstCfg, Uint16 *pu16Buff)
{
    Uint16 i;

    pu16Buff[0] = (Uint16)(pstCfg->u16UpgradeFlag & 0xFFFFU);
    pu16Buff[1] = (Uint16)(pstCfg->u16FirmwareType & 0xFFFFU);
    pu16Buff[2] = (Uint16)(pstCfg->u32FileSize & 0xFFFFU);
    pu16Buff[3] = (Uint16)((pstCfg->u32FileSize >> 16) & 0xFFFFU);
    pu16Buff[4] = (Uint16)(pstCfg->u32FileCrc & 0xFFFFU);
    pu16Buff[5] = (Uint16)((pstCfg->u32FileCrc >> 16) & 0xFFFFU);

    for (i = 0U; i < 6U; i++)
    {
        pu16Buff[(Uint16)(6U + i)] = (Uint16)(pstCfg->u16Rsv[i] & 0xFFFFU);
    }
}

/**
 * @brief  解析配置区镜像（12 个字）
 */
static void UpgradeParseCfg(const Uint16 *pu16Buff, ST_UPGRADE_CFG *pstCfg)
{
    Uint16 i;

    pstCfg->u16UpgradeFlag = pu16Buff[0];
    pstCfg->u16FirmwareType = pu16Buff[1];
    pstCfg->u32FileSize = (Uint32)pu16Buff[2] | ((Uint32)pu16Buff[3] << 16);
    pstCfg->u32FileCrc = (Uint32)pu16Buff[4] | ((Uint32)pu16Buff[5] << 16);

    for (i = 0U; i < 6U; i++)
    {
        pstCfg->u16Rsv[i] = pu16Buff[(Uint16)(6U + i)];
    }
}

/* ---------------- 对外接口 ---------------- */

Uint16 UpgradeCoreInit(const ST_UPGRADE_FLASH_OPS *pstOps)
{
    Uint16 i;

    if ((pstOps == 0) || (pstOps->pfEraseSector == 0) ||
        (pstOps->pfProgramBlock == 0) || (pstOps->pfReadWords == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    s_stUpgradeCtx.pstOps = pstOps;
    s_stUpgradeCtx.u32WriteAddr = UPGRADE_BACKUP_BASE;
    s_stUpgradeCtx.u32BodyWords = 0U;
    s_stUpgradeCtx.u32BodyBytes = 0U;
    s_stUpgradeCtx.u32Crc = 0xFFFFFFFFUL;
    s_stUpgradeCtx.u16StageCount = 0U;
    s_stUpgradeCtx.u16ByteOdd = 0U;
    s_stUpgradeCtx.u16PendingByte = 0U;
    s_stUpgradeCtx.u16Writing = 0U;

    for (i = 0U; i < UPGRADE_BLOCK_WORDS; i++)
    {
        s_stUpgradeCtx.au16Stage[i] = 0U;
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreEraseBackup(void)
{
    Uint16 i;
    Uint16 u16Ret;

    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfEraseSector == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    for (i = 0U; i < 4U; i++)
    {
        u16Ret = s_stUpgradeCtx.pstOps->pfEraseSector(g_astUpgradeBackupSector[i].u32Start);
        if (u16Ret == 0U)
        {
            return UPGRADE_ERR_FLASH;
        }
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreEraseBackupSector(Uint16 u16Index)
{
    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfEraseSector == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    if (u16Index >= UPGRADE_BACKUP_SECTOR_NUM)
    {
        return UPGRADE_ERR_PARAM;
    }

    if (s_stUpgradeCtx.pstOps->pfEraseSector(g_astUpgradeBackupSector[u16Index].u32Start) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreStartBody(void)
{
    Uint16 i;

    s_stUpgradeCtx.u32WriteAddr = UPGRADE_BACKUP_BASE;
    s_stUpgradeCtx.u32BodyWords = 0U;
    s_stUpgradeCtx.u32BodyBytes = 0U;
    s_stUpgradeCtx.u32Crc = 0xFFFFFFFFUL;
    s_stUpgradeCtx.u16StageCount = 0U;
    s_stUpgradeCtx.u16ByteOdd = 0U;
    s_stUpgradeCtx.u16PendingByte = 0U;
    s_stUpgradeCtx.u16Writing = 1U;

    for (i = 0U; i < UPGRADE_BLOCK_WORDS; i++)
    {
        s_stUpgradeCtx.au16Stage[i] = 0U;
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreWriteBody(const Uint16 *pu16Data, Uint16 u16Length)
{
    Uint16 i;
    Uint16 u16Word;
    Uint16 u16Ret;

    if ((pu16Data == 0) || (u16Length == 0U))
    {
        return UPGRADE_ERR_PARAM;
    }

    if (s_stUpgradeCtx.u16Writing == 0U)
    {
        return UPGRADE_ERR_STATE;
    }

    if (((Uint32)s_stUpgradeCtx.u32BodyBytes + (Uint32)u16Length) > UPGRADE_BACKUP_BYTES)
    {
        return UPGRADE_ERR_SIZE;
    }

    s_stUpgradeCtx.u32Crc = UpgradeCoreCrc32(s_stUpgradeCtx.u32Crc, pu16Data, u16Length);

    for (i = 0U; i < u16Length; i++)
    {
        if (s_stUpgradeCtx.u16ByteOdd == 0U)
        {
            s_stUpgradeCtx.u16PendingByte = (Uint16)(pu16Data[i] & 0x00FFU);
            s_stUpgradeCtx.u16ByteOdd = 1U;
        }
        else
        {
            u16Word = (Uint16)(s_stUpgradeCtx.u16PendingByte |
                               (Uint16)((pu16Data[i] & 0x00FFU) << 8));
            s_stUpgradeCtx.u16ByteOdd = 0U;

            u16Ret = UpgradePushWord(u16Word);
            if (u16Ret != UPGRADE_OK)
            {
                return u16Ret;
            }
        }
    }

    s_stUpgradeCtx.u32BodyBytes = s_stUpgradeCtx.u32BodyBytes + (Uint32)u16Length;

    return UPGRADE_OK;
}

Uint16 UpgradeCoreEndBody(void)
{
    Uint16 u16Ret;

    if (s_stUpgradeCtx.u16Writing == 0U)
    {
        return UPGRADE_ERR_STATE;
    }

    s_stUpgradeCtx.u16Writing = 0U;

    /* 奇数字节：最后一个字的高字节补 0xFF（擦除态） */
    if (s_stUpgradeCtx.u16ByteOdd != 0U)
    {
        u16Ret = UpgradePushWord((Uint16)(s_stUpgradeCtx.u16PendingByte | 0xFF00U));
        s_stUpgradeCtx.u16ByteOdd = 0U;
        if (u16Ret != UPGRADE_OK)
        {
            return u16Ret;
        }
    }

    if (s_stUpgradeCtx.u16StageCount == 0U)
    {
        return UPGRADE_OK;
    }

    /* 末块补齐到 8 字：补 0xFFFF 即擦除态，且补齐后仍在备份区内 */
    while (s_stUpgradeCtx.u16StageCount < UPGRADE_BLOCK_WORDS)
    {
        s_stUpgradeCtx.au16Stage[s_stUpgradeCtx.u16StageCount] = 0xFFFFU;
        s_stUpgradeCtx.u16StageCount++;
    }

    u16Ret = UpgradeProgramStage();

    return u16Ret;
}

Uint32 UpgradeCoreBodyWords(void)
{
    return s_stUpgradeCtx.u32BodyWords;
}

Uint32 UpgradeCoreCrc(void)
{
    return s_stUpgradeCtx.u32Crc;
}

Uint16 UpgradeCoreParseHead(const Uint16 *pu16Head, ST_UPGRADE_FILE_HEAD *pstHead)
{
    if ((pu16Head == 0) || (pstHead == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    pstHead->u16ChipType = (Uint16)(pu16Head[0] & 0x00FFU);
    pstHead->u16FirmwareType = (Uint16)(pu16Head[1] & 0x00FFU);
    pstHead->u32FileSize = UpgradeReadUint32(pu16Head, 2U);
    pstHead->u32FileCrc = UpgradeReadUint32(pu16Head, 6U);

    return UPGRADE_OK;
}

Uint16 UpgradeCoreCheckHead(const ST_UPGRADE_FILE_HEAD *pstHead, Uint16 u16ChipType)
{
    Uint32 u32Limit;

    if (pstHead == 0)
    {
        return UPGRADE_ERR_PARAM;
    }

    if (pstHead->u16ChipType != u16ChipType)
    {
        return UPGRADE_ERR_HEAD;
    }

    if ((pstHead->u16FirmwareType != UPGRADE_FW_APP) &&
        (pstHead->u16FirmwareType != UPGRADE_FW_BOOT))
    {
        return UPGRADE_ERR_HEAD;
    }

    if (pstHead->u32FileSize == 0UL)
    {
        return UPGRADE_ERR_SIZE;
    }

    u32Limit = (pstHead->u16FirmwareType == UPGRADE_FW_APP) ?
               UPGRADE_APP_MAX_BYTES : UPGRADE_BOOT_MAX_BYTES;

    if (pstHead->u32FileSize > u32Limit)
    {
        return UPGRADE_ERR_SIZE;
    }

    if (pstHead->u32FileCrc == 0UL)
    {
        return UPGRADE_ERR_CRC;
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreCommitCfg(Uint16 u16FirmwareType, Uint32 u32FileSize, Uint32 u32FileCrc)
{
    ST_UPGRADE_CFG stCfg;
    Uint16 au16Image[16];
    Uint16 au16Back[16];
    Uint16 i;

    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfProgramBlock == 0) ||
        (s_stUpgradeCtx.pstOps->pfReadWords == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    stCfg.u16UpgradeFlag = UPGRADE_CFG_FLAG;
    stCfg.u16FirmwareType = u16FirmwareType;
    stCfg.u32FileSize = u32FileSize;
    stCfg.u32FileCrc = u32FileCrc;
    for (i = 0U; i < 6U; i++)
    {
        stCfg.u16Rsv[i] = 0U;
    }

    for (i = 0U; i < 16U; i++)
    {
        au16Image[i] = 0xFFFFU;
        au16Back[i] = 0U;
    }

    UpgradeBuildCfg(&stCfg, au16Image);

    /* 配置区同样按 8 字块编程（两次），每次覆盖完整的 64 位单元 */
    if (s_stUpgradeCtx.pstOps->pfProgramBlock(UPGRADE_CFG_BASE, &au16Image[0], UPGRADE_BLOCK_WORDS) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    if (s_stUpgradeCtx.pstOps->pfProgramBlock(UPGRADE_CFG_BASE + UPGRADE_BLOCK_WORDS,
                                              &au16Image[UPGRADE_BLOCK_WORDS],
                                              UPGRADE_BLOCK_WORDS) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    if (s_stUpgradeCtx.pstOps->pfReadWords(UPGRADE_CFG_BASE, au16Back, 12U) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    for (i = 0U; i < 12U; i++)
    {
        if (au16Back[i] != au16Image[i])
        {
            return UPGRADE_ERR_FLASH;
        }
    }

    return UPGRADE_OK;
}

Uint16 UpgradeCoreReadCfg(ST_UPGRADE_CFG *pstCfg)
{
    Uint16 au16Buff[16];
    Uint16 i;

    if (pstCfg == 0)
    {
        return UPGRADE_ERR_PARAM;
    }

    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfReadWords == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    for (i = 0U; i < 16U; i++)
    {
        au16Buff[i] = 0U;
    }

    if (s_stUpgradeCtx.pstOps->pfReadWords(UPGRADE_CFG_BASE, au16Buff, 12U) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    UpgradeParseCfg(au16Buff, pstCfg);

    return UPGRADE_OK;
}

Uint16 UpgradeCoreClearCfg(void)
{
    if ((s_stUpgradeCtx.pstOps == 0) || (s_stUpgradeCtx.pstOps->pfEraseSector == 0))
    {
        return UPGRADE_ERR_PARAM;
    }

    if (s_stUpgradeCtx.pstOps->pfEraseSector(UPGRADE_CFG_BASE) == 0U)
    {
        return UPGRADE_ERR_FLASH;
    }

    return UPGRADE_OK;
}

Uint32 UpgradeCoreCrc32(Uint32 u32Crc, const Uint16 *pu16Data, Uint16 u16Length)
{
    Uint32 u32Val = u32Crc;
    Uint16 i;
    Uint16 j;

    if ((pu16Data == 0) || (u16Length == 0U))
    {
        return u32Val;
    }

    for (i = 0U; i < u16Length; i++)
    {
        u32Val = u32Val ^ ((Uint32)(pu16Data[i] & 0x00FFU) << 24);
        for (j = 0U; j < 8U; j++)
        {
            if ((u32Val & 0x80000000UL) != 0UL)
            {
                u32Val = (Uint32)((Uint32)(u32Val << 1) ^ 0x04C11DB7UL);
            }
            else
            {
                u32Val = (Uint32)(u32Val << 1);
            }
        }
    }

    return u32Val;
}

#ifdef __cplusplus
}
#endif

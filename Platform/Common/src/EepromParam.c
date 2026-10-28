/**
 * @file EepromParam.c
 * @author yetingheng (004182)
 * @brief 掉电保存参数框架实现
 * @version 0.2
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 参数清单只认 Modbus 点表里的 MB_ATTR_SAVE 位，不另建第二张表：
 *       点表是"变量有哪些、地址是多少、能不能写"的唯一来源，
 *       保存清单从它派生，避免两处维护走样。
 *
 * @note 数据区镜像 gShadow 是"芯片里应该是什么"的 RAM 副本，
 *       一个 Uint16 存一个字节（与 SCI/CRC16 的约定一致）。
 *       落盘只写脏区间，但 CRC 总是对整块数据区重算，所以镜像必须完整。
 *
 * @note 省写手段（EEPROM 寿命只有 100 万次，必须当回事）：
 *       1. 每项记一个"上次真正写进芯片的值"的 CRC16（gLastHash），
 *          落盘前逐项比对，一项都没变就整次跳过，连参数头都不写；
 *       2. 合并窗口 + 最长等待（见 EepromParamTask）；
 *       3. 落盘限速 u16MinFlushIntervalMs，把最坏写入频率压到 1 次/秒。
 */
#include "EepromParam.h"
#include "Fm24c128d.h"
#include "Crc16.h"

/******************************************************************************
 * 私有定义
 ******************************************************************************/
#define EEPROM_PARAM_HASH_NONE      (0xFFFFU)   /* 0xFFFF 只是"无值"占位 */

/* 一条保存项：变量地址 + 宽度 + 在数据区里的偏移 */
typedef struct
{
    void  *pvData;
    Uint16 u16Size;
    Uint16 u16Offset;
} ST_EEPROM_PARAM_ITEM;

/******************************************************************************
 * 私有变量
 ******************************************************************************/
static const ST_EEPROM_PARAM_CFG *gCfg = (const ST_EEPROM_PARAM_CFG *)0;

static Uint16 gInited = 0U;
static Uint16 gEnabled = 0U;

/* 保存项清单（按点表行序） */
static ST_EEPROM_PARAM_ITEM gItem[EEPROM_PARAM_ITEM_MAX];
static Uint16 gItemNum = 0U;
static Uint16 gPayloadLen = 0U;

/* 数据区镜像：一个 Uint16 存一个字节 */
static Uint16 gShadow[EEPROM_PARAM_DATA_MAX];

/* 每项"上次写进芯片的值"的哈希；bHashValid = 0 时全部视为已变化 */
static Uint16 gLastHash[EEPROM_PARAM_ITEM_MAX];
static Uint16 bHashValid = 0U;

/* 参数头是否还没和数据对齐（数据写成功但头没写成功时保持为 1） */
static Uint16 bHeaderDirty = 0U;

/* 当前数据区 CRC 与落盘次数 */
static Uint16 gCrc = 0U;
static Uint32 gWriteCnt = 0UL;
static Uint16 gErrCnt = 0U;

/* 脏区间：[lo, hi] 是数据区内的字节偏移，bDirty 为 0 时无意义 */
static Uint16 gDirtyLo = 0xFFFFU;
static Uint16 gDirtyHi = 0U;
static Uint16 bDirty = 0U;

/* 内部毫秒计时（由 EepromParamTask 按调度周期累加） */
static Uint32 gElapsedMs = 0UL;
static Uint32 gLastDirtyMs = 0UL;
static Uint32 gFirstDirtyMs = 0UL;
static Uint32 gLastFlushMs = 0UL;

/* Modbus 只读镜像 */
volatile Uint16 gEepromParamStatus = EEPROM_PARAM_ST_DISABLED;
volatile Uint16 gEepromParamItemNum = 0U;
volatile Uint16 gEepromParamWriteCntLo = 0U;
volatile Uint16 gEepromParamErrCnt = 0U;

/******************************************************************************
 * 小工具：小端读写
 ******************************************************************************/
static void EepromParamPutU16(Uint16 *pu16Buf, Uint16 u16Idx, Uint16 u16Val)
{
    pu16Buf[u16Idx] = (Uint16)(u16Val & 0x00FFU);
    pu16Buf[u16Idx + 1U] = (Uint16)((u16Val >> 8U) & 0x00FFU);
}

static Uint16 EepromParamGetU16(const Uint16 *pu16Buf, Uint16 u16Idx)
{
    return (Uint16)(((pu16Buf[u16Idx + 1U] & 0x00FFU) << 8U) | (pu16Buf[u16Idx] & 0x00FFU));
}

static void EepromParamPutU32(Uint16 *pu16Buf, Uint16 u16Idx, Uint32 u32Val)
{
    pu16Buf[u16Idx]      = (Uint16)(u32Val & 0x000000FFUL);
    pu16Buf[u16Idx + 1U] = (Uint16)((u32Val >> 8U) & 0x000000FFUL);
    pu16Buf[u16Idx + 2U] = (Uint16)((u32Val >> 16U) & 0x000000FFUL);
    pu16Buf[u16Idx + 3U] = (Uint16)((u32Val >> 24U) & 0x000000FFUL);
}

static Uint32 EepromParamGetU32(const Uint16 *pu16Buf, Uint16 u16Idx)
{
    return ((Uint32)(pu16Buf[u16Idx + 3U] & 0x00FFU) << 24U)
         | ((Uint32)(pu16Buf[u16Idx + 2U] & 0x00FFU) << 16U)
         | ((Uint32)(pu16Buf[u16Idx + 1U] & 0x00FFU) << 8U)
         | ((Uint32)(pu16Buf[u16Idx] & 0x00FFU));
}

/**
 * @brief 按 Modbus 数据类型取存储宽度
 *
 * @return 2 / 4，未知类型返回 0
 */
static Uint16 EepromParamSizeByType(Uint16 u16ValType)
{
    switch (u16ValType)
    {
        case MB_TYPE_U16:
        case MB_TYPE_I16:
            return 2U;

        case MB_TYPE_U32:
        case MB_TYPE_I32:
        case MB_TYPE_F32:
            return 4U;

        default:
            return 0U;
    }
}

/**
 * @brief 按变量地址查清单下标
 *
 * @return 下标，找不到返回 0xFFFF
 */
static Uint16 EepromParamFindItem(const void *pvData)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        if (gItem[u16Idx].pvData == pvData)
        {
            return u16Idx;
        }
    }

    return 0xFFFFU;
}

/**
 * @brief 把 RAM 里一个保存项的值写进镜像
 */
static void EepromParamItemToShadow(Uint16 u16Idx)
{
    const volatile Uint32 *pu32Src;
    const volatile Uint16 *pu16Src;
    Uint16 u16Off;

    pu16Src = (const volatile Uint16 *)gItem[u16Idx].pvData;
    u16Off = gItem[u16Idx].u16Offset;

    if (gItem[u16Idx].u16Size == 4U)
    {
        pu32Src = (const volatile Uint32 *)gItem[u16Idx].pvData;
        EepromParamPutU32(gShadow, u16Off, (Uint32)(*pu32Src));
    }
    else
    {
        EepromParamPutU16(gShadow, u16Off, (Uint16)(*pu16Src));
    }
}

/**
 * @brief 把镜像里一个保存项的值写回 RAM
 */
static void EepromParamShadowToItem(Uint16 u16Idx)
{
    volatile Uint32 *pu32Dst;
    volatile Uint16 *pu16Dst;
    Uint16 u16Off;

    pu16Dst = (volatile Uint16 *)gItem[u16Idx].pvData;
    u16Off = gItem[u16Idx].u16Offset;

    if (gItem[u16Idx].u16Size == 4U)
    {
        pu32Dst = (volatile Uint32 *)gItem[u16Idx].pvData;
        *pu32Dst = EepromParamGetU32(gShadow, u16Off);
    }
    else
    {
        *pu16Dst = EepromParamGetU16(gShadow, u16Off);
    }
}

/**
 * @brief 计算镜像里某一项的哈希
 */
static Uint16 EepromParamHashItem(Uint16 u16Idx)
{
    return Crc16Modbus(&gShadow[gItem[u16Idx].u16Offset], gItem[u16Idx].u16Size);
}

/**
 * @brief 全部保存项：RAM -> 镜像（上电初值就是出厂默认值）
 */
static void EepromParamBuildShadowFromRam(void)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        EepromParamItemToShadow(u16Idx);
    }
}

/**
 * @brief 全部保存项：镜像 -> RAM
 */
static void EepromParamApplyShadowToRam(void)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        EepromParamShadowToItem(u16Idx);
    }
}

/**
 * @brief 全部保存项：按当前镜像重算哈希（= 认定"芯片里就是这些值"）
 */
static void EepromParamRefreshHash(void)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        gLastHash[u16Idx] = EepromParamHashItem(u16Idx);
    }

    bHashValid = 1U;
}

/**
 * @brief 把脏区间内涉及的保存项从 RAM 同步到镜像
 *
 * @note 脏区间是按项边界更新的，所以这里只需要判断"项的字节区间与脏区间相交"。
 */
static void EepromParamSyncDirtyRange(void)
{
    Uint16 u16Idx;
    Uint16 u16ItemEnd;

    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        u16ItemEnd = (Uint16)(gItem[u16Idx].u16Offset + gItem[u16Idx].u16Size - 1U);

        if ((u16ItemEnd < gDirtyLo) || (gItem[u16Idx].u16Offset > gDirtyHi))
        {
            continue;
        }

        EepromParamItemToShadow(u16Idx);
    }
}

/**
 * @brief 更新 Modbus 只读镜像
 */
static void EepromParamUpdateMirror(void)
{
    gEepromParamItemNum = gItemNum;
    gEepromParamWriteCntLo = (Uint16)(gWriteCnt & 0x0000FFFFUL);
    gEepromParamErrCnt = gErrCnt;
}

/**
 * @brief 清脏标记
 */
static void EepromParamClearDirty(void)
{
    bDirty = 0U;
    gDirtyLo = 0xFFFFU;
    gDirtyHi = 0U;
}


/**
 * @brief 从芯片读参数头 + 数据区并校验
 *
 * @return 1 表示内容可用且已填进镜像，0 表示不可用（保持初值）
 */
static Uint16 EepromParamLoadFromEeprom(void)
{
    Uint16 u16Hdr[EEPROM_PARAM_HDR_SIZE];
    Uint16 u16ItemNum;
    Uint16 u16PayloadLen;
    Uint16 u16Crc;

    if (Fm24c128dRead(EEPROM_PARAM_HDR_ADDR, u16Hdr, EEPROM_PARAM_HDR_SIZE) != FM24C128D_OK)
    {
        return 0U;
    }

    /* 魔数：空片读回来通常全 0xFF 或全 0x00，都对不上 */
    if ((u16Hdr[0] != EEPROM_PARAM_MAGIC_BYTE0) ||
        (u16Hdr[1] != EEPROM_PARAM_MAGIC_BYTE1) ||
        (u16Hdr[2] != EEPROM_PARAM_MAGIC_BYTE2) ||
        (u16Hdr[3] != EEPROM_PARAM_MAGIC_BYTE3))
    {
        return 0U;
    }

    if (EepromParamGetU16(u16Hdr, 4U) != EEPROM_PARAM_VERSION)
    {
        return 0U;      /* 版本变了：点表结构可能也变了，整块作废 */
    }

    u16ItemNum = EepromParamGetU16(u16Hdr, 6U);
    u16PayloadLen = EepromParamGetU16(u16Hdr, 8U);

    if ((u16ItemNum != gItemNum) || (u16PayloadLen != gPayloadLen))
    {
        return 0U;      /* 点表加/删了 SAVE 行，旧数据对不上了 */
    }

    if (Fm24c128dRead(EEPROM_PARAM_DATA_ADDR, gShadow, gPayloadLen) != FM24C128D_OK)
    {
        return 0U;
    }

    u16Crc = Crc16Modbus(gShadow, gPayloadLen);
    if (u16Crc != EepromParamGetU16(u16Hdr, 10U))
    {
        return 0U;      /* 数据被改坏了，回默认值 */
    }

    gCrc = u16Crc;
    gWriteCnt = EepromParamGetU32(u16Hdr, 12U);

    return 1U;
}

/**
 * @brief 扫描点表，建立保存项清单并分配偏移
 */
static void EepromParamScanTables(void)
{
    const ST_MB_REG_ITEM *pstItem;
    Uint16 u16Table;
    Uint16 u16Row;
    Uint16 u16Size;

    for (u16Table = 0U; u16Table < gCfg->u16TableNum; u16Table++)
    {
        if ((u16Table >= EEPROM_PARAM_TABLE_MAX) ||
            (gCfg->pstTable == (const ST_EEPROM_PARAM_TABLE *)0))
        {
            break;
        }

        pstItem = gCfg->pstTable[u16Table].pstRegItem;

        if (pstItem == (const ST_MB_REG_ITEM *)0)
        {
            continue;
        }

        for (u16Row = 0U; u16Row < gCfg->pstTable[u16Table].u16RegNum; u16Row++)
        {
            if ((pstItem[u16Row].u16Attr & MB_ATTR_SAVE) == 0U)
            {
                continue;
            }

            if (pstItem[u16Row].pvData == (void *)0)
            {
                continue;
            }

            u16Size = EepromParamSizeByType(pstItem[u16Row].u16ValType);
            if (u16Size == 0U)
            {
                gErrCnt++;
                continue;
            }

            /* 同一个变量在两处都打了 SAVE，只存一次 */
            if (EepromParamFindItem(pstItem[u16Row].pvData) != 0xFFFFU)
            {
                continue;
            }

            if ((gItemNum >= EEPROM_PARAM_ITEM_MAX) ||
                ((gPayloadLen + u16Size) > EEPROM_PARAM_DATA_MAX))
            {
                gErrCnt++;
                return;     /* 空间/项数超了：后面的不再收 */
            }

            gItem[gItemNum].pvData = pstItem[u16Row].pvData;
            gItem[gItemNum].u16Size = u16Size;
            gItem[gItemNum].u16Offset = gPayloadLen;

            gItemNum++;
            gPayloadLen = (Uint16)(gPayloadLen + u16Size);
        }
    }
}

/******************************************************************************
 * 公开函数
 ******************************************************************************/

/**
 * @brief 初始化参数保存层
 */
void EepromParamInit(const ST_EEPROM_PARAM_CFG *pstEepromCfg)
{
    Uint16 u16Idx;

    if (pstEepromCfg == (const ST_EEPROM_PARAM_CFG *)0)
    {
        return;
    }

    gCfg = pstEepromCfg;

    gItemNum = 0U;
    gPayloadLen = 0U;
    gCrc = 0U;
    gWriteCnt = 0UL;
    gErrCnt = 0U;
    bDirty = 0U;
    bHashValid = 0U;
    bHeaderDirty = 0U;
    gDirtyLo = 0xFFFFU;
    gDirtyHi = 0U;
    gElapsedMs = 0UL;
    gLastDirtyMs = 0UL;
    gFirstDirtyMs = 0UL;
    gLastFlushMs = 0UL;

    for (u16Idx = 0U; u16Idx < EEPROM_PARAM_ITEM_MAX; u16Idx++)
    {
        gLastHash[u16Idx] = EEPROM_PARAM_HASH_NONE;
    }

    /* 1. 扫点表 */
    EepromParamScanTables();

    /* 2. 镜像先填 C 代码里的初值，任何失败路径以此为准 */
    EepromParamBuildShadowFromRam();

    gInited = 1U;

    if (gCfg->bEnable == 0U)
    {
        gEnabled = 0U;
        gEepromParamStatus = EEPROM_PARAM_ST_DISABLED;
        EepromParamUpdateMirror();
        return;
    }

    gEnabled = 1U;

    /* 3. 读芯片校验 */
    if ((gCfg->bFactoryReset != 0U) || (EepromParamLoadFromEeprom() == 0U))
    {
        EepromParamMarkAllDirty();
        bHeaderDirty = 1U;      /* 头也必须重写，否则下次上电认不出 */
    }
    else
    {
        EepromParamApplyShadowToRam();
        EepromParamRefreshHash();   /* 芯片里就是这些值 */
        gEepromParamStatus = EEPROM_PARAM_ST_IDLE;
    }

    EepromParamUpdateMirror();
}

/**
 * @brief 标记某个变量为脏
 */
Uint16 EepromParamMarkDirty(const void *pvData)
{
    Uint16 u16Idx;
    Uint16 u16End;

    if ((gInited == 0U) || (gEnabled == 0U) || (pvData == (const void *)0))
    {
        return 0U;
    }

    u16Idx = EepromParamFindItem(pvData);
    if (u16Idx == 0xFFFFU)
    {
        return 0U;
    }

    u16End = (Uint16)(gItem[u16Idx].u16Offset + gItem[u16Idx].u16Size - 1U);

    if (bDirty == 0U)
    {
        gDirtyLo = gItem[u16Idx].u16Offset;
        gDirtyHi = u16End;
        gFirstDirtyMs = gElapsedMs;
    }
    else
    {
        if (gItem[u16Idx].u16Offset < gDirtyLo)
        {
            gDirtyLo = gItem[u16Idx].u16Offset;
        }
        if (u16End > gDirtyHi)
        {
            gDirtyHi = u16End;
        }
    }

    bDirty = 1U;
    gLastDirtyMs = gElapsedMs;
    gEepromParamStatus = EEPROM_PARAM_ST_DIRTY;

    return 1U;
}

/**
 * @brief 标记并立刻落盘
 */
Uint16 EepromParamSaveVar(const void *pvData)
{
    if (EepromParamMarkDirty(pvData) == 0U)
    {
        return 0U;
    }

    return (EepromParamFlush() == EEPROM_PARAM_ST_IDLE) ? 1U : 0U;
}

/**
 * @brief 把所有保存项标记为脏
 */
void EepromParamMarkAllDirty(void)
{
    if ((gInited == 0U) || (gEnabled == 0U) || (gItemNum == 0U))
    {
        return;
    }

    gDirtyLo = 0U;
    gDirtyHi = (Uint16)(gPayloadLen - 1U);
    bDirty = 1U;
    gLastDirtyMs = gElapsedMs;
    gFirstDirtyMs = gElapsedMs;
    gEepromParamStatus = EEPROM_PARAM_ST_DIRTY;
}

#define EEPROM_FLUSH_DATA           (1U)    /* 正在写数据区 */
#define EEPROM_FLUSH_HEADER         (2U)    /* 正在写参数头 */
#define EEPROM_FLUSH_BLOCK_MAX      (2000U) /* 阻塞落盘的最大轮数，防止芯片异常时死等 */

static Uint16 sFlushStep = 0U;              /* 落盘状态机步骤，0 表示空闲 */
static Uint16 sHdrBuf[EEPROM_PARAM_HDR_SIZE];

/**
 * @brief 装配参数头并启动异步写
 *
 * @return 1 已启动；0 启动失败
 */
static Uint16 EepromParamWriteHeaderAsync(void)
{
    Uint16 u16Idx;
    Uint16 u16Ret;

    for (u16Idx = 0U; u16Idx < EEPROM_PARAM_HDR_SIZE; u16Idx++)
    {
        sHdrBuf[u16Idx] = 0x005AU;   /* 保留区固定 0x5A，未写过的片子不会是这个值 */
    }

    sHdrBuf[0] = EEPROM_PARAM_MAGIC_BYTE0;
    sHdrBuf[1] = EEPROM_PARAM_MAGIC_BYTE1;
    sHdrBuf[2] = EEPROM_PARAM_MAGIC_BYTE2;
    sHdrBuf[3] = EEPROM_PARAM_MAGIC_BYTE3;

    EepromParamPutU16(sHdrBuf, 4U, EEPROM_PARAM_VERSION);
    EepromParamPutU16(sHdrBuf, 6U, gItemNum);
    EepromParamPutU16(sHdrBuf, 8U, gPayloadLen);
    EepromParamPutU16(sHdrBuf, 10U, gCrc);
    EepromParamPutU32(sHdrBuf, 12U, gWriteCnt);

    u16Ret = Fm24c128dWriteAsyncStart(EEPROM_PARAM_HDR_ADDR, sHdrBuf, EEPROM_PARAM_HDR_SIZE);

    return ((u16Ret == FM24C128D_ASYNC_BUSY) || (u16Ret == FM24C128D_OK)) ? 1U : 0U;
}

/**
 * @brief 落盘推进：非阻塞状态机，每调用一次只推进一步
 *
 * @return EEPROM_PARAM_ST_WRITING 表示还要继续调用；ST_IDLE 表示全部写完
 * @note 后台调度按拍调用；掉电等必须写完的场合用 EepromParamFlushBlocking()
 */
Uint16 EepromParamFlush(void)
{
    Uint16 u16Idx;
    Uint16 u16Off;
    Uint16 u16End;
    Uint16 u16Lo = 0xFFFFU;
    Uint16 u16Hi = 0U;
    Uint16 bChanged = 0U;
    Uint16 u16Ret;

    if ((gInited == 0U) || (gEnabled == 0U))
    {
        sFlushStep = 0U;
        return EEPROM_PARAM_ST_DISABLED;
    }

    /* 数据区写：一页要几百微秒，每拍只送一片，送完再收尾 */
    if (sFlushStep == EEPROM_FLUSH_DATA)
    {
        u16Ret = Fm24c128dWriteAsyncPoll();

        if (u16Ret == FM24C128D_ASYNC_BUSY)
        {
            return EEPROM_PARAM_ST_WRITING;
        }

        if (u16Ret != FM24C128D_OK)
        {
            sFlushStep = 0U;
            gErrCnt++;
            EepromParamUpdateMirror();
            gEepromParamStatus = EEPROM_PARAM_ST_ERROR;
            return EEPROM_PARAM_ST_ERROR;       /* 脏标记保留，下个周期重试 */
        }

        /* 数据已经进芯片了，哈希立刻对齐；头写失败也不影响数据正确性 */
        EepromParamRefreshHash();
        gCrc = Crc16Modbus(gShadow, gPayloadLen);
        gWriteCnt++;

        if (EepromParamWriteHeaderAsync() == 0U)
        {
            sFlushStep = 0U;
            gErrCnt++;
            EepromParamUpdateMirror();
            gEepromParamStatus = EEPROM_PARAM_ST_ERROR;
            return EEPROM_PARAM_ST_ERROR;
        }

        sFlushStep = EEPROM_FLUSH_HEADER;
        return EEPROM_PARAM_ST_WRITING;
    }

    /* 参数头写：32 字一片即可写完 */
    if (sFlushStep == EEPROM_FLUSH_HEADER)
    {
        u16Ret = Fm24c128dWriteAsyncPoll();

        if (u16Ret == FM24C128D_ASYNC_BUSY)
        {
            return EEPROM_PARAM_ST_WRITING;
        }

        if (u16Ret != FM24C128D_OK)
        {
            sFlushStep = 0U;
            gErrCnt++;
            EepromParamUpdateMirror();
            gEepromParamStatus = EEPROM_PARAM_ST_ERROR;
            return EEPROM_PARAM_ST_ERROR;
        }

        bHeaderDirty = 0U;
        EepromParamClearDirty();
        gLastFlushMs = gElapsedMs;
        gEepromParamStatus = EEPROM_PARAM_ST_IDLE;
        sFlushStep = 0U;
        EepromParamUpdateMirror();

        return EEPROM_PARAM_ST_IDLE;
    }

    if (gItemNum == 0U)
    {
        return EEPROM_PARAM_ST_IDLE;
    }

    if ((bDirty == 0U) && (bHeaderDirty == 0U))
    {
        return EEPROM_PARAM_ST_IDLE;
    }

    gEepromParamStatus = EEPROM_PARAM_ST_WRITING;

    /* 1. 脏区间从 RAM 同步进镜像 */
    if (bDirty != 0U)
    {
        EepromParamSyncDirtyRange();
    }

    /* 2. 找出值真的变了的项，同时收紧要写的字节区间 */
    for (u16Idx = 0U; u16Idx < gItemNum; u16Idx++)
    {
        if ((bHashValid != 0U) && (EepromParamHashItem(u16Idx) == gLastHash[u16Idx]))
        {
            continue;
        }

        bChanged = 1U;
        u16Off = gItem[u16Idx].u16Offset;
        u16End = (Uint16)(u16Off + gItem[u16Idx].u16Size - 1U);

        if (u16Off < u16Lo)
        {
            u16Lo = u16Off;
        }
        if (u16End > u16Hi)
        {
            u16Hi = u16End;
        }
    }

    /* 3. 没有任何项变化且头也不需要重写：整次跳过，不碰芯片 */
    if ((bChanged == 0U) && (bHeaderDirty == 0U))
    {
        EepromParamClearDirty();
        gLastFlushMs = gElapsedMs;
        gEepromParamStatus = EEPROM_PARAM_ST_IDLE;
        return EEPROM_PARAM_ST_IDLE;
    }

    /* 4. 启动数据区异步写 */
    if (bChanged != 0U)
    {
        u16Ret = Fm24c128dWriteAsyncStart((Uint16)(EEPROM_PARAM_DATA_ADDR + u16Lo),
                                         &gShadow[u16Lo],
                                         (Uint16)(u16Hi - u16Lo + 1U));

        if ((u16Ret != FM24C128D_ASYNC_BUSY) && (u16Ret != FM24C128D_OK))
        {
            gErrCnt++;
            EepromParamUpdateMirror();
            gEepromParamStatus = EEPROM_PARAM_ST_ERROR;
            return EEPROM_PARAM_ST_ERROR;
        }

        sFlushStep = EEPROM_FLUSH_DATA;
        return EEPROM_PARAM_ST_WRITING;
    }

    /* 5. 只有参数头要写 */
    gCrc = Crc16Modbus(gShadow, gPayloadLen);
    gWriteCnt++;

    if (EepromParamWriteHeaderAsync() == 0U)
    {
        gErrCnt++;
        EepromParamUpdateMirror();
        gEepromParamStatus = EEPROM_PARAM_ST_ERROR;
        return EEPROM_PARAM_ST_ERROR;
    }

    sFlushStep = EEPROM_FLUSH_HEADER;
    return EEPROM_PARAM_ST_WRITING;
}

/**
 * @brief 强制落盘：阻塞直到写完或重试超限，用于掉电预处理
 */
Uint16 EepromParamFlushBlocking(void)
{
    Uint16 u16Ret = EEPROM_PARAM_ST_WRITING;
    Uint16 u16Round;

    for (u16Round = 0U; u16Round < EEPROM_FLUSH_BLOCK_MAX; u16Round++)
    {
        u16Ret = EepromParamFlush();

        if (u16Ret != EEPROM_PARAM_ST_WRITING)
        {
            return u16Ret;
        }
    }

    return EEPROM_PARAM_ST_ERROR;
}

/**
 * @brief 周期任务：按合并窗口和限速触发落盘，落盘本身分拍推进
 */
void EepromParamTask(void *pArgV)
{
    Uint32 u32SinceDirty;
    Uint32 u32SinceFirst;

    (void)pArgV;

    if (gInited == 0U)
    {
        return;
    }

    /* 内部计时：以调度周期为步长，不依赖其他模块的时基 */
    gElapsedMs += (Uint32)gCfg->u16TaskPeriodMs;

    if (gEnabled == 0U)
    {
        return;
    }

    /* 上一帧还没写完就只推进状态机，不再看合并窗口 */
    if (sFlushStep != 0U)
    {
        (void)EepromParamFlush();
        return;
    }

    if ((bDirty == 0U) && (bHeaderDirty == 0U))
    {
        return;
    }

    /* 落盘限速：两次落盘之间至少隔 u16MinFlushIntervalMs */
    if ((gElapsedMs - gLastFlushMs) < (Uint32)gCfg->u16MinFlushIntervalMs)
    {
        return;
    }

    u32SinceDirty = gElapsedMs - gLastDirtyMs;
    u32SinceFirst = gElapsedMs - gFirstDirtyMs;

    /* 静默够久就写；一直有写则按最长等待兜底 */
    if ((bDirty != 0U) &&
        (u32SinceDirty < (Uint32)gCfg->u16FlushDelayMs) &&
        (u32SinceFirst < (Uint32)gCfg->u16MaxDelayMs))
    {
        return;
    }

    (void)EepromParamFlush();
}

/**
 * @brief 恢复出厂值：清掉参数头，复位后自动用初值重建
 */
void EepromParamFactoryReset(void)
{
    Uint16 u16Buf[EEPROM_PARAM_HDR_SIZE];
    Uint16 u16Idx;

    if ((gInited == 0U) || (gEnabled == 0U))
    {
        return;
    }

    /* 头写成全 0：魔数对不上，下次上电必然走初值重写分支 */
    for (u16Idx = 0U; u16Idx < EEPROM_PARAM_HDR_SIZE; u16Idx++)
    {
        u16Buf[u16Idx] = 0x0000U;
    }

    (void)Fm24c128dWrite(EEPROM_PARAM_HDR_ADDR, u16Buf, EEPROM_PARAM_HDR_SIZE);

    /* 当前 RAM 值是运行值，不是出厂值，所以这里不假装能就地恢复 */
    EepromParamClearDirty();
    bHeaderDirty = 0U;
    gEnabled = 0U;
    gEepromParamStatus = EEPROM_PARAM_ST_DISABLED;
    EepromParamUpdateMirror();
}

/**
 * @brief 用户数据区写
 */
Uint16 EepromParamBlockWrite(Uint16 u16Offset, const Uint16 *pu16Buf, Uint16 u16Len)
{
    if ((gInited == 0U) || (gEnabled == 0U) || (pu16Buf == (const Uint16 *)0) || (u16Len == 0U))
    {
        return 0U;
    }

    if (((Uint32)u16Offset + (Uint32)u16Len) > (Uint32)EEPROM_PARAM_USER_SIZE)
    {
        return 0U;
    }

    return (Fm24c128dWrite((Uint16)(EEPROM_PARAM_USER_ADDR + u16Offset), pu16Buf, u16Len)
            == FM24C128D_OK) ? 1U : 0U;
}

/**
 * @brief 用户数据区读
 */
Uint16 EepromParamBlockRead(Uint16 u16Offset, Uint16 *pu16Buf, Uint16 u16Len)
{
    if ((gInited == 0U) || (gEnabled == 0U) || (pu16Buf == (Uint16 *)0) || (u16Len == 0U))
    {
        return 0U;
    }

    if (((Uint32)u16Offset + (Uint32)u16Len) > (Uint32)EEPROM_PARAM_USER_SIZE)
    {
        return 0U;
    }

    return (Fm24c128dRead((Uint16)(EEPROM_PARAM_USER_ADDR + u16Offset), pu16Buf, u16Len)
            == FM24C128D_OK) ? 1U : 0U;
}

/**
 * @brief 诊断量
 */
Uint16 EepromParamGetStatus(void)
{
    return gEepromParamStatus;
}

Uint16 EepromParamGetItemNum(void)
{
    return gItemNum;
}

Uint32 EepromParamGetWriteCnt(void)
{
    return gWriteCnt;
}

Uint16 EepromParamGetErrCnt(void)
{
    return gErrCnt;
}

Uint16 EepromParamGetCrc(void)
{
    return gCrc;
}

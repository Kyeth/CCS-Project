/**
 * @file EepromParam.h
 * @author yetingheng (004182)
 * @brief 掉电保存参数框架：由 Modbus 点表标记决定存什么，FM24C128D 负责存
 * @version 0.2
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note ======================= 怎么用 =======================
 *        1. 变量照常定义，挂进任意一张 Modbus 点表；
 *        2. 想让这个变量掉电保存，就把该行的读写属性从 MB_RW_WRITE 改成
 *           MB_RW_WRITE_SAVE（= MB_RW_WRITE | MB_RW_SAVE，多一个 SAVE 位）；
 *        3. 其余什么都不用改：上电自动回填，Modbus 写入成功后自动标脏、定时落盘。
 *
 *        例：
 *            {31U, MB_VAL_U16, MB_RW_WRITE_SAVE, (void *)&gDemoSavedWord, 0, 1000U},
 *
 *        业务里非 Modbus 触发的保存（比如某个状态机自己算出来的参数）：
 *            改完变量后调 EepromParamMarkDirty(&gDemoSavedWord);      // 只标脏
 *            或者 EepromParamSaveVar(&gDemoSavedWord);               // 标脏并立刻落盘
 *        注意：EepromParamMarkDirty 只是标记，变量本身要保证是"点表里带 SAVE 的项"。
 * ===========================================================
 *
 * @note ======================= 空间分配 =======================
 *        FM24C128D 共 16KB（0x0000~0x3FFF），整片按用途切分如下，
 *        划分原则：参数区放低半区（0x0000~0x1FFF），因为 WP 拉高只保护高半区，
 *        所以即使写保护配置出错，参数区也永远写得进去，参数区永不被保护逻辑挡住。
 *
 *        地址范围        大小      用途
 *        0x0000~0x001F    32 B     参数头（魔数/版本/项数/长度/CRC/落盘次数）
 *        0x0020~0x041F   1024 B    参数数据区（SAVE 项按点表行序自动排布）
 *        0x0420~0x0FFF   3040 B    扩展参数区（预留：曲线/记录类数据）
 *        0x1000~0x1EFF   3840 B    用户数据区（业务用 EepromParamBlockWrite/Read 直接读写）
 *        0x1F00~0x1F0F     16 B    器件自检保留区（Fm24c128d 驱动专用）
 *        0x1F10~0x3FFF   4080 B    空闲（高半区，WP 保护）
 *
 *        参数头（32 字节，正好一页，不会跨页写）：
 *           0 ~ 3   魔数 'E' 'P' '1' '5'
 *           4 ~ 5   参数表版本（点表结构变化时改 EEPROM_PARAM_VERSION）
 *           6 ~ 7   保存项个数（与当前点表扫描结果比对，不一致即作废）
 *           8 ~ 9   数据区长度 payloadLen
 *          10 ~ 11  数据区 CRC16（CRC-16/MODBUS，低字节在前）
 *          12 ~ 15  落盘次数（诊断用，也是磨损统计）
 *          16 ~ 31  保留，固定 0x5A，用于识别"从没写过"
 *
 *        数据区里每项 2 或 4 字节（U16/I16 占 2，U32/I32/F32 占 4），小端存放，
 *        偏移由 EepromParamInit 扫点表自动分配：加/删 SAVE 行只改点表，
 *        不用来这里改地址；项数或长度变了，头校验必然失败，旧数据自动作废。
 * ===========================================================
 *
 * @note ======================= 什么时候落盘 =======================
 *        FM24C128D 是 EEPROM，不是 FRAM：每页写完有内部烧写周期（tWR 最大 5ms），
 *        擦写寿命只有 100 万次，所以"写"是稀缺资源，策略如下：
 *
 *          1. 值没变就不写：落盘前逐项和"上次真正写进芯片的值"比对（16 位哈希），
 *             一个项都没变就整次跳过，连参数头都不碰（上位机反复写同一个值不会磨损）；
 *          2. 合并写：写成功后只标脏，最后一次标脏后静默 u16FlushDelayMs（缺省 500ms）
 *             才落盘，连续改参数只写一次；
 *          3. 最长等待：一直有写也要在 u16MaxDelayMs（缺省 5000ms）内落盘一次；
 *          4. 落盘限速：两次落盘之间至少间隔 u16MinFlushIntervalMs（缺省 1000ms），
 *             把最坏情况的写入频率压在 1 次/秒——1 秒一次连续刷 11.5 天才会用完
 *             100 万次寿命，正常业务（改参数是低频人工操作）可以认为是无限寿命；
 *          5. 强制落盘：掉电预处理/关机流程里调 EepromParamFlush()，忽略窗口和限速。
 *
 *        一次落盘的实际代价：数据区按脏区间拆页写（每页一次 tWR 等待）+ 参数头一页，
 *        典型 2~3 页 ≈ 10~15ms，这段时间是阻塞的，所以不要在高频中断里调 Flush。
 * ===========================================================
 *
 * @note 上电流程：读头 -> 校验魔数/版本/项数/长度/CRC -> 全对才回填到变量；
 *       任何一项不对（空片、换版本、点表增删、CRC 错）都保持变量在上电初值，
 *       并把整块数据区重写成"当前初值"，这就是出厂默认值的唯一来源——
 *       默认值只有一份，就是 C 代码里的初始化，不另存副本、不做双备份。
 */
#ifndef EEPROM_PARAM_H
#define EEPROM_PARAM_H

#include "F2837xS_device.h"
#include "ModbusSlave.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 布局常量
 ******************************************************************************/
#define EEPROM_PARAM_MAGIC_BYTE0    (0x0045U)   /* 'E' */
#define EEPROM_PARAM_MAGIC_BYTE1    (0x0050U)   /* 'P' */
#define EEPROM_PARAM_MAGIC_BYTE2    (0x0031U)   /* '1' */
#define EEPROM_PARAM_MAGIC_BYTE3    (0x0035U)   /* '5' */

/* 参数表版本：点表里增删 SAVE 行、或改变变量类型时自增，旧数据自动作废 */
#define EEPROM_PARAM_VERSION        (1U)

#define EEPROM_PARAM_HDR_ADDR       (0x0000U)   /* 参数头起始地址 */
#define EEPROM_PARAM_HDR_SIZE       (32U)       /* 参数头长度 = 一页 */
#define EEPROM_PARAM_DATA_ADDR      (0x0020U)   /* 数据区起始地址 */

/* 数据区上限：超出即视为配置错误，初始化时直接报错不写芯片 */
#define EEPROM_PARAM_DATA_MAX       (1024U)

/* 用户数据区：业务自己的杂项存储（不进 CRC、不受参数框架管理） */
#define EEPROM_PARAM_USER_ADDR      (0x1000U)
#define EEPROM_PARAM_USER_SIZE      (0x0F00U)

/* 最多支持多少张点表、多少个保存项 */
#define EEPROM_PARAM_TABLE_MAX      (4U)
#define EEPROM_PARAM_ITEM_MAX       (64U)

/* 参数保存状态，供 Modbus 只读寄存器观察 */
#define EEPROM_PARAM_ST_IDLE        (0U)    /* 没有待写数据 */
#define EEPROM_PARAM_ST_DIRTY       (1U)    /* 有数据待落盘 */
#define EEPROM_PARAM_ST_WRITING     (2U)    /* 正在写芯片 */
#define EEPROM_PARAM_ST_ERROR       (3U)    /* 上一次落盘失败 */
#define EEPROM_PARAM_ST_DISABLED    (4U)    /* 配置为关闭 */

/******************************************************************************
 * 配置
 ******************************************************************************/

/* 一张参与扫描的点表 */
typedef struct
{
    const ST_MB_REG_ITEM *pstRegItem;   /* 点表首地址 */
    Uint16 u16RegNum;                   /* 有效行数（不是数组 MAX） */
} ST_EEPROM_PARAM_TABLE;

typedef struct
{
    const ST_EEPROM_PARAM_TABLE *pstTable;  /* 点表数组 */
    Uint16 u16TableNum;                     /* 点表个数 */
    Uint16 u16FlushDelayMs;                 /* 合并窗口：最后一次写后等多久落盘 */
    Uint16 u16MaxDelayMs;                   /* 最长等待：一直有写也要落盘 */
    Uint16 u16MinFlushIntervalMs;           /* 落盘限速：两次落盘的最小间隔 */
    Uint16 u16TaskPeriodMs;                 /* EepromParamTask 调度周期，用于内部计时 */
    Uint16 bEnable;                         /* 0: 完全不访问芯片，只在 RAM 里跑 */
    Uint16 bFactoryReset;                   /* 1: 上电忽略芯片内容，用初值重写 */
} ST_EEPROM_PARAM_CFG;

/******************************************************************************
 * 接口
 ******************************************************************************/

/**
 * @brief 初始化参数保存层：扫描点表、读芯片、校验、回填变量
 *
 * @note 调用前必须已完成 Hw_I2cInit() 和 Fm24c128dInit()。
 *       读失败或校验失败时保持变量为初值，并标记全脏等第一次落盘。
 */
extern void EepromParamInit(const ST_EEPROM_PARAM_CFG *pstEepromCfg);

/**
 * @brief 周期任务：按合并窗口/限速把脏数据落盘，建议 1ms 调度一次
 */
extern void EepromParamTask(void *pArgV);

/**
 * @brief 标记某个变量为脏（Modbus 写入后由协议层调用）
 *
 * @param pvData 变量地址，必须是点表里带 MB_RW_SAVE 的项
 * @return 1 标记成功，0 该变量不在保存清单里
 */
extern Uint16 EepromParamMarkDirty(const void *pvData);

/**
 * @brief 标记并立刻落盘（业务里"改完必须马上存"的场合用）
 *
 * @param pvData 变量地址
 * @return 1 已落盘成功，0 标记失败或写芯片失败
 */
extern Uint16 EepromParamSaveVar(const void *pvData);

/**
 * @brief 把所有保存项标记为脏
 */
extern void EepromParamMarkAllDirty(void);

/**
 * @brief 非阻塞落盘推进，每次只推进一步（后台调度按拍调用）
 *
 * @return EEPROM_PARAM_ST_WRITING 表示还要继续调用；ST_IDLE 表示写完
 */
extern Uint16 EepromParamFlush(void);

/**
 * @brief 强制落盘：内部循环调用 EepromParamFlush 直到写完（掉电前/关机流程用）
 *
 * @return 落盘结束时的状态：EEPROM_PARAM_ST_IDLE 表示成功
 */
extern Uint16 EepromParamFlushBlocking(void);

/**
 * @brief 恢复出厂值：清掉参数头，复位后按初值重新生成
 *
 * @note 不立刻改 RAM 里的变量（那些值此时已经是运行值，不是"出厂值"），
 *       清完把保存层停掉，重新上电后自动回到代码里的初值。
 */
extern void EepromParamFactoryReset(void);

/**
 * @brief 用户数据区读写（0x1000 起，3840 字节，业务随便用）
 *
 * @param u16Offset 相对 EEPROM_PARAM_USER_ADDR 的偏移
 * @param pu16Buf   缓冲，一个 Uint16 存一个字节
 * @param u16Len    字节数
 * @return 1 成功，0 失败（越界/芯片出错/保存层被关闭）
 */
extern Uint16 EepromParamBlockWrite(Uint16 u16Offset, const Uint16 *pu16Buf, Uint16 u16Len);
extern Uint16 EepromParamBlockRead(Uint16 u16Offset, Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 诊断量
 */
extern Uint16 EepromParamGetStatus(void);
extern Uint16 EepromParamGetItemNum(void);
extern Uint32 EepromParamGetWriteCnt(void);
extern Uint16 EepromParamGetErrCnt(void);
extern Uint16 EepromParamGetCrc(void);

/* ---- 供 Modbus 点表指针直映的只读镜像 ---- */
extern volatile Uint16 gEepromParamStatus;      /* 当前状态 */
extern volatile Uint16 gEepromParamItemNum;     /* 保存项个数 */
extern volatile Uint16 gEepromParamWriteCntLo;  /* 落盘次数低 16 位 */
extern volatile Uint16 gEepromParamErrCnt;      /* 落盘/校验错误次数 */

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_PARAM_H */

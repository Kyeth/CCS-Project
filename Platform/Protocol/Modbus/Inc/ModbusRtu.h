/**
 * @file ModbusRtu.h
 * @brief Modbus RTU slave protocol core: framing, CRC, function codes, replies.
 *
 * 本模块只处理"字节批次 + 观察时标"，不包含 Hw_SciDriver.h、不访问任何寄存器、
 * 不持有硬件状态。字节从端口层经 ModbusSci 送进来，回复写回实例的 au16TxBuf。
 *
 * 支持的功能码（第一版）：
 *   0x03 读保持寄存器（1..64 个）
 *   0x06 写单寄存器
 *   0x10 写多个寄存器（1..64 个）
 * 其余功能码返回异常 0x01。
 */

#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "F2837xS_device.h"

#define MB_MAX_REGISTERS        (64U)    /* 单次最多操作的寄存器数（5000..5036 需 37） */
#define MB_RX_BUF_SIZE          (256U)   /* 每实例接收缓冲（1 单元 = 1 线上字节） */
#define MB_TX_BUF_SIZE          (144U)   /* 每实例回复缓冲（0x03 读 64 个 = 133 字节） */

#define MB_FRAME_TIMEOUT_MS     (100UL)  /* 候选帧超时：超过即丢弃 */

/* 功能码 */
#define MB_FC_READ_HOLDING      (0x03U)
#define MB_FC_WRITE_SINGLE      (0x06U)
#define MB_FC_WRITE_MULTIPLE    (0x10U)

/* 异常码 */
#define MB_EX_NONE              (0x00U)
#define MB_EX_ILLEGAL_FUNC      (0x01U)
#define MB_EX_ILLEGAL_ADDR      (0x02U)
#define MB_EX_ILLEGAL_VALUE     (0x03U)

/* 接收状态 */
#define MB_RX_HUNT              (0U)     /* 找地址字节 */
#define MB_RX_FRAME             (1U)     /* 累积候选帧 */
#define MB_RX_COMPLETE          (2U)     /* 长度已达，等静默确认 */

#define MB_ADDR_BROADCAST       (0x00U)

/* 业务回调：成功返回 MB_EX_NONE，失败返回可用于应答的标准异常码。 */
typedef Uint16 (*MB_READ_HOLDING)(void *pCtx, Uint16 u16Start, Uint16 u16Count, Uint16 *pu16Values);
typedef Uint16 (*MB_WRITE_HOLDING)(void *pCtx, Uint16 u16Start, Uint16 u16Count, const Uint16 *pu16Values);

typedef struct {
    /* 配置 */
    Uint16 u16SlaveAddr;
    Uint16 u16Enabled;
    Uint32 u32T35Ticks;              /* 3.5 字符静默；0 表示不判静默 */
    Uint32 u32FrameTimeoutTicks;     /* 候选帧超时；0 表示不判超时 */
    void  *pBizCtx;
    MB_READ_HOLDING  pfRead;
    MB_WRITE_HOLDING pfWrite;

    /* 接收状态（跨批次） */
    Uint16 au16RxBuf[MB_RX_BUF_SIZE];
    Uint16 u16RxLen;
    Uint16 u16Expected;
    Uint16 u16State;
    Uint16 u16TxLost;                /* 1 = 上一次回复被强制复位作废 */
    Uint32 u32LastByteTick;          /* 最近一批字节的观察时刻 */
    Uint32 u32CompleteTick;

    /* 待发回复 */
    Uint16 au16TxBuf[MB_TX_BUF_SIZE];
    Uint16 u16TxLen;                 /* >0 表示有回复待提交 */
    Uint16 u16TxOffset;              /* 已提交进端口环的字节数 */

    /* 统计 */
    Uint32 u32RxFrames;              /* 通过校验并执行过的请求数 */
    Uint32 u32RxDropped;             /* 丢弃的候选帧数 */
    Uint32 u32RxIgnored;             /* 合法但不应答（广播读、地址不符等）的帧数 */
    Uint32 u32TxSubmitted;           /* 已全部提交进端口环的回复数 */
    Uint32 u32TxBusyRounds;          /* 因暂停/环满而本圈未提交的次数 */
    Uint32 u32TxLostEvents;          /* 强制复位导致回复作废的次数 */
    Uint32 u32TimeInvalid;           /* 因时标不可信而暂缓提交的次数 */
} ST_MB_INSTANCE;

/** CRC-16/MODBUS：返回 CRC 值本身（低字节先发送由调用方负责）。 */
Uint16 ModbusCrc16(const Uint16 *pu16Data, Uint16 u16Count);

/** 初始化实例（清零状态、装配置）。 */
void ModbusRtuInit(ST_MB_INSTANCE *pst, Uint16 u16SlaveAddr, Uint32 u32T35Ticks,
                   Uint32 u32FrameTimeoutTicks, Uint16 u16Enabled);

/** 装载业务回调与上下文。 */
void ModbusRtuConfigure(ST_MB_INSTANCE *pst, void *pBizCtx,
                        MB_READ_HOLDING pfRead, MB_WRITE_HOLDING pfWrite);

/**
 * @brief 追加一批字节并做长度判定（不执行业务）。
 * @param u32Tick 该批字节被观察到的时刻
 * @param u16TickValid 0 表示时标不可信（此时仍接收，但不据此判超时）
 * @return 1 = 吸收了字节，0 = 未吸收（未使能或参数错）
 */
Uint16 ModbusRtuFeed(ST_MB_INSTANCE *pst, const Uint16 *pu16Data, Uint16 u16Count,
                     Uint32 u32Tick, Uint16 u16TickValid);

/**
 * @brief 帧结束判定：静默满足才校验并执行业务，随后生成回复。
 *        时标不可信时不提交候选请求（尤其不执行写操作）。
 */
void ModbusRtuPoll(ST_MB_INSTANCE *pst, Uint32 u32Tick, Uint16 u16TickValid);

/** 流中断：丢弃候选帧，等待重新同步。 */
void ModbusRtuOnStreamBreak(ST_MB_INSTANCE *pst);

/** 1 = 有回复待提交。 */
Uint16 ModbusRtuHasReply(const ST_MB_INSTANCE *pst);

/** 提交进度：u16Accepted 为本次被端口接受的字节数；1 = 全部入环。 */
Uint16 ModbusRtuConsumeReply(ST_MB_INSTANCE *pst, Uint16 u16Accepted);

/** 强制复位事件：作废尚未提交完的回复（不自动重发），返回 1 表示确有待发回复被作废。 */
Uint16 ModbusRtuDiscardReply(ST_MB_INSTANCE *pst);

/**
 * @brief 直接装载一段已编码好的回复（用于协议切换握手）。
 * @return 1 = 已装载；0 = 参数错或已有待发回复（不覆盖）
 */
Uint16 ModbusRtuLoadReply(ST_MB_INSTANCE *pst, const Uint16 *pu16Frame, Uint16 u16Len);

#endif /* MODBUS_RTU_H */

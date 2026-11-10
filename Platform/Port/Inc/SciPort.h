/**
 * @file SciPort.h
 * @author yetingheng (004182)
 * @brief SCI 端口层：字节环 + 批次观察记录 + 接收状态机 + 发送前缀推进
 * @version 0.2
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 职责（见 docs/review/steps/05-port-layer.md：05A）与 05B 增量
 * （见 docs/review/steps/05b-sci-wiring.md v0.5）：
 *   1. 每 100 us 从驱动 FIFO 搬运最多 8 字节进字节环，并写一条“批次观察记录”；
 *   2. 记录被观察到的时刻，不换算静默时长——静默判定归协议层；
 *   3. 1 ms 前台把 TX 环按“硬件实际接纳的前缀”送入 FIFO，逐字节 Drop；
 *   4. 05B：接收错误分类、受限硬件清除、恢复状态机。
 *
 * 读取者唯一性（05B 定稿）：
 *   FIFO 读取（pfTryRead）只允许发生在 100 us 心跳里；前台只有在硬件复位
 *   阶段访问寄存器（pfGetStatus / pfClearRxError / pfIsTxIdle），
 *   并且在 HW_RESET 状态期间心跳完全跳过该端口的硬件访问。
 *
 * 端口层不直接读 SCI 寄存器，只经 ST_SCI_PORT_DRV 调用驱动。
 */

#ifndef SCI_PORT_H
#define SCI_PORT_H

#include "F2837xS_device.h"
#include "RingBuff.h"
#include "Hw_SciDriver.h"
#include "PortCritical.h"

#define SCI_PORT_NUM                  (3U)
#define SCI_PORT_RX_QUOTA             (8U)    /* 每轮最多 pfTryRead 次数 */
#define SCI_PORT_TX_QUOTA             (16U)   /* 每轮最多送入 FIFO 的字节数 */
#define SCI_PORT_SERVICE_NOMINAL_US   (100UL) /* 名义服务周期 */
#define SCI_PORT_SERVICE_GAP_LIMIT_US (500UL) /* 超过即置 bTimingBad */
#define SCI_PORT_WAIT_TX_LIMIT_MS     (8UL)   /* 等待发送空闲的强制恢复阈值 */

/* 接收状态（05B）：状态、阶段、门禁三者成组提交。 */
#define SCI_RX_IDLE        (0U) /* 正常搬运 */
#define SCI_RX_SW_RESYNC   (1U) /* 纯软件重同步：软件状态已清，ISR 只丢弃 FIFO 残留 */
#define SCI_RX_RECOVER_REQ (2U) /* 硬件错误已锁存：丢弃并等前台推进恢复 */
#define SCI_RX_HW_RESET    (3U) /* 前台正在执行清除/复位：ISR 完全跳过硬件 */
#define SCI_RX_DRAIN_ISR   (4U) /* 硬件已清除：等 ISR 排空残余后回 IDLE */
#define SCI_RX_PAUSED      (5U) /* 恢复失败：停止硬件访问，等显式重试 */

/* 恢复阶段（05B）：可跨轮继续，未执行的动作不推进阶段。 */
#define SCI_REC_PHASE_NONE       (0U)
#define SCI_REC_PHASE_CLASSIFY   (1U) /* 读状态分类，判断是否需要复位 */
#define SCI_REC_PHASE_WAIT_TX    (2U) /* 等发送空闲（最多 8 ms） */
#define SCI_REC_PHASE_RESET      (3U) /* 执行一次受限硬件清除 */
#define SCI_REC_PHASE_RESET_DONE (4U)  /* 评估清除结果 */
#define SCI_REC_PHASE_VERIFY_TX  (5U)  /* 显式重试前的 TX FIFO 残留复核 */

/* SciPortRecover() 返回值。 */
#define SCI_RECOVER_IDLE_OK (0U) /* 本来就不在恢复中 */
#define SCI_RECOVER_OK      (1U) /* 恢复已完成（硬件已清、等 ISR 排空） */
#define SCI_RECOVER_BUSY    (2U) /* 跨轮进行中：状态/阶段见 getter */
#define SCI_RECOVER_FAIL    (3U) /* 恢复失败，端口进入 PAUSED */
#define SCI_RECOVER_UNKNOWN (4U) /* 结果不确定，端口进入 PAUSED */
#define SCI_RECOVER_PARAM   (5U) /* 参数错误或驱动不可用 */

/* SciPortRecover() 的出参 pu16Remain 只报“剩余错误位”（HW_SCI_RXSTS_*），
 * 不再掺入标志位：FE/PE/OE/BI 与任何标志宏都不会撞位。
 * 恢复过程中的状态性信息改用 SciPortRecoveryFlags() 单独查询。 */
#define SCI_RECOVER_FLAG_VALID      (0x0001U) /* 端口正处于恢复流程 */
#define SCI_RECOVER_FLAG_WAIT_TX    (0x0002U) /* 正在等发送空闲 */
#define SCI_RECOVER_FLAG_STILL_ERR  (0x0004U) /* 上次复位后仍有错误位 */
#define SCI_RECOVER_FLAG_UNREADABLE (0x0008U) /* 上次复位结果不可读 */

/* 批次记录单元长度取目标编译器的 sizeof，不硬编码字数。 */
typedef struct {
    Uint32 u32Tick;          /* 本批次被观察到的时刻（搬运之前读取） */
    Uint32 u32TickEmptyLast; /* 最近一次读到空的时刻；仅当 u16EmptyRounds != 0 有效 */
    Uint16 u16SeqStart;      /* 本批次第一个字节的全局接收序号 */
    Uint16 u16Count;         /* 本批次字节数，1..8 */
    Uint16 u16EmptyRounds;   /* 期间连续空读轮数；0 表示无空读观察 */
} ST_SCI_RX_SEG;

/* 批次记录单元的存储字数：一律按 Uint16 单元数计算。
 * C28x 上 sizeof 以 16 位字为单位，主机上以字节为单位，只有除以 sizeof(Uint16)
 * 才能在两套环境下都得到“环元素等于 struct 本身”的同一语义。 */
#define SCI_SEG_ITEM_WORDS \
    ((Uint16)(sizeof(ST_SCI_RX_SEG) / sizeof(Uint16)))

/* 编译期整除检查：结构体必须是 Uint16 的整数倍，否则复制会越界。 */
typedef char SciSegWordsMustDivide[
    ((sizeof(ST_SCI_RX_SEG) % sizeof(Uint16)) == 0U) ? 1 : -1];

/* 驱动接口表：05A 用桩，05B 绑真实驱动。 */
typedef struct {
    Uint16 (*pfTryRead)(Uint16 u16PortId, Uint16 *pu16Data);
    Uint16 (*pfTryWrite)(Uint16 u16PortId, Uint16 u16Data);
    Uint16 (*pfTryGetTick)(Uint32 *pu32Tick);
    Uint16 (*pfGetStatus)(Uint16 u16PortId, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear);
    Uint16 (*pfClearRxError)(Uint16 u16PortId, Uint16 bForce, Uint16 *pu16Remain);
    Uint16 (*pfIsTxIdle)(Uint16 u16PortId, Uint16 *pu16Idle);
} ST_SCI_PORT_DRV;

typedef struct {
    Uint16 u16PortId;              /* HW_SCI_ID_A / C / D */
    const ST_SCI_PORT_DRV *pstDrv;
    ST_RING_BUFF stRxRing;         /* 字节，1 字/单元 */
    ST_RING_BUFF stSegRing;        /* 批次记录，sizeof(ST_SCI_RX_SEG) 字/单元 */
    ST_RING_BUFF stTxRing;         /* 字节，1 字/单元，位于 .portbuf */
    Uint16 u16RxSeq;               /* 生产者：已写入环的字节总数 */
    Uint16 u16RxSeqConsumed;       /* 消费者：已取走的字节总数 */
    Uint16 u16EmptyRounds;         /* 连续空读轮数（饱和） */
    Uint16 u16RoundsFull;          /* 读满 8 字节被截断的轮数 */
    Uint16 u16RxRejected;          /* 整批拒收时丢弃的字节数 */
    Uint16 u16TxRejected;          /* TX 环写入被拒字节数 */
    Uint16 u16TxStall;             /* 某轮一个字节都没送进 FIFO 的次数 */
    Uint16 u16TxBudgetStop;        /* 前台 TX 因单元/整轮预算用尽而收手的次数 */
    Uint16 u16RxErrorCount;        /* 硬件错误使端口进入恢复的轮数 */
    Uint16 u16RxState;             /* SCI_RX_* */
    Uint16 u16RecPhase;            /* SCI_REC_PHASE_* */
    Uint16 u16ClearResult;         /* 最近一次 HW_SCI_CLEAR_* 结果 */
    Uint16 u16RecRemain;           /* 剩余错误位（HW_SCI_RXSTS_*），出参缓存 */
    Uint16 u16RecFifoLevel;        /* CLASSIFY 时读到的 RX FIFO 层数（仅观测） */
    Uint16 bForceReset;            /* 已判定“超阈值强制复位”，尚未执行 */
    Uint16 u16RxErrSeen;           /* 恢复期间看到的错误位并集 */
    Uint16 bTimeBaseValid;         /* 已建立时间基准 */
    Uint16 bRxSuspend;             /* 接收侧挂起：不发布批次 */
    Uint16 bTxSuspended;           /* 发送侧挂起：不发起新的事务 */
    Uint16 bTxResetLoss;           /* 强制复位丢弃了 TX 内容，需协议层重发/裁定 */
    Uint16 bWaitBaseValid;         /* WAIT_TX 的 8 ms 基准已建立 */
    Uint16 bSwInvalidated;         /* 本轮恢复已完成软件侧失效处理 */
    volatile Uint16 bStreamBreak;  /* 数据完整性中断：需协议层重同步 */
    volatile Uint16 bTimingBad;    /* 时间关系不可信 */
    volatile Uint16 bFrozen;       /* 端口停用（保留） */
    volatile Uint16 u16FaultGen;   /* 故障代次：每次新的故障事件 +1 */
    Uint32 u32TxServiceCalls;      /* 前台 TX 服务被调用的次数（诊断） */
    Uint32 u32TxBytesSent;         /* 已提交给硬件的 TX 字节数（诊断） */
    Uint32 u32TxMaxGapTicks;       /* 相邻两次 TX 服务的最大间隔（tick，诊断） */
    Uint32 u32TxLastSvcTick;       /* 上一次 TX 服务所在单元的起点时标 */
    Uint32 u32LastTick;
    Uint32 u32TickEmptyLast;
    Uint32 u32MaxGap;              /* 本端口实测最大服务间隔（tick） */
    Uint32 u32WaitStartTick;       /* WAIT_TX 起点（递减计数器的原始值） */
    Uint32 u32RxErrEvents;         /* 硬件错误事件数（不含纯软件重同步） */
    Uint32 u32RecoverDrop;         /* 恢复期间丢弃的字节数 */
    Uint32 u32TxResetDrop;         /* 强制复位实际清掉的 TX 字节数 */
    Uint32 u32ForceResets;         /* 实际执行的强制复位次数（预算不足不记） */
    Uint32 u32TimeSrcFail;         /* 恢复过程中时间源读取失败次数 */
} ST_SCI_PORT;

/* 临界区：默认使用 PortCritical.c 的真实实现（保存/恢复整字 ST1）；
 * 桩测试用 -DPORT_CRITICAL_HOOKS 注入可模拟屏蔽与延迟投递的实现。 */
#ifdef PORT_CRITICAL_HOOKS
extern ST_PORT_IRQ_STATE PortEnterCriticalHook(void);
extern void PortExitCriticalHook(ST_PORT_IRQ_STATE u16State);
#define PORT_ENTER_CRITICAL()      (PortEnterCriticalHook())
#define PORT_EXIT_CRITICAL(st)     (PortExitCriticalHook(st))
#else
#define PORT_ENTER_CRITICAL()      (PortEnterCriticalReal())
#define PORT_EXIT_CRITICAL(st)     (PortExitCriticalReal(st))
#endif

void SciPortInit(void);
Uint16 SciPortBindDriver(Uint16 u16PortId, const ST_SCI_PORT_DRV *pstDrv);
void SciPortSetSysclkHz(Uint32 u32SysclkHz);

/* 100 us 心跳：搬运 RX（或丢弃 FIFO 残留）。返回本轮有动作的端口数。
 * 调用次数单独计数，便于主机桩验证“初始化失败后心跳不再服务端口”。 */
Uint16 SciPortService100us(void);
Uint32 SciPortServiceCallCount(void);
/* 1 ms 前台：推进 TX。 */
void SciPortTask(void);

/** 只服务一路 SCI 的发送（供 PortTask 做公平轮转） */
void SciPortTaskOne(Uint16 u16PortId);

/* 协议层：取一条批次记录并取出对应字节；无记录返回 0。 */
Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max,
                       ST_SCI_RX_SEG *pstSeg);
/* 协议层：写入待发字节（整批或全拒）。返回实际接纳字节数。 */
Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count);
Uint16 SciPortTxFree(Uint16 u16PortId);

/* 恢复状态机（05B）：只在前台调用，每轮最多执行一个受限动作，可跨轮继续。
 * 返回值 SCI_RECOVER_*。pu16Remain 报“剩余错误位”；参数或驱动不可用时
 * 保持调用方原值不写（可用 0xFFFF 哨兵验证）。状态性信息见 SciPortRecoveryFlags()。 */
Uint16 SciPortRecover(Uint16 u16PortId, Uint16 *pu16Remain);
/* 从 PAUSED 重新开始恢复（显式重试，不重复计入硬件错误事件数）。 */
Uint16 SciPortRecoverRetry(Uint16 u16PortId);

/* 时间源失败后作废所有端口的 WAIT_TX 等待基准（100 us 心跳/1 ms 前台失败路径调用）。
 * 语义：下一次成功读取只建立基准，必须重新累计满判定阈值才允许强制复位。 */
void SciPortInvalidateWaitBases(void);
Uint16 SciPortRxState(Uint16 u16PortId);
Uint16 SciPortRecPhase(Uint16 u16PortId);
Uint16 SciPortClearResult(Uint16 u16PortId);
Uint16 SciPortRxErrorCount(Uint16 u16PortId);
Uint16 SciPortRxErrSeen(Uint16 u16PortId);
Uint16 SciPortRecoveryFlags(Uint16 u16PortId);
Uint16 SciPortRecFifoLevel(Uint16 u16PortId);
Uint32 SciPortForceResets(Uint16 u16PortId);

/** 诊断：前台 TX 服务被调用的次数 */
Uint32 SciPortTxServiceCalls(Uint16 u16PortId);

/** 诊断：已提交给硬件的 TX 字节数 */
Uint32 SciPortTxBytesSent(Uint16 u16PortId);

/** 诊断：相邻两次 TX 服务之间的最大间隔（tick） */
Uint32 SciPortTxMaxGapTicks(Uint16 u16PortId);
Uint32 SciPortRxErrEvents(Uint16 u16PortId);
Uint32 SciPortRecoverDrop(Uint16 u16PortId);
Uint32 SciPortTimeSrcFail(Uint16 u16PortId);

/* 发送侧：恢复期间挂起，强制复位后保持冻结，直到协议层确认。
 * SciPortTxResume() 只在 IDLE 且确实发生过强制复位丢弃时解除冻结。 */
Uint16 SciPortIsTxSuspended(Uint16 u16PortId);
Uint16 SciPortTxResume(Uint16 u16PortId);
Uint16 SciPortTxResetQueue(Uint16 u16PortId);
Uint32 SciPortTxResetDrop(Uint16 u16PortId);

/** 1 = 软件 TX 环与硬件发送器都已空闲（协议交接用）。 */
Uint16 SciPortIsTxIdle(Uint16 u16PortId);

/* 重同步（纯软件）：清两环、序号、观察记录与时间基准，并让 ISR 丢弃 FIFO 残留。
 * 在恢复流程中调用按“合并”处理：只重做软件失效，不改恢复状态与阶段。 */
void SciPortResync(Uint16 u16PortId);

/* 重同步是否仍在跨轮丢弃残留（1 = 仍在丢弃，此时不发布批次）。 */
Uint16 SciPortIsResyncDraining(Uint16 u16PortId);

Uint16 SciPortIsStreamBreak(Uint16 u16PortId);
/* 故障代次：前台先读它，确认时带回。任何新故障都会让代次 +1。 */
Uint16 SciPortFaultGeneration(Uint16 u16PortId);
/* 带代次确认：只有代次与读取时一致才清除；否则保留故障并返回 0。 */
Uint16 SciPortClearStreamBreak(Uint16 u16PortId, Uint16 u16Generation);
Uint16 SciPortIsTimingBad(Uint16 u16PortId);
Uint16 SciPortRxRejected(Uint16 u16PortId);
Uint16 SciPortTxStall(Uint16 u16PortId);
Uint16 SciPortRxUsed(Uint16 u16PortId);
Uint32 SciPortMaxGap(Uint16 u16PortId);
Uint16 SciPortTicksToUs(Uint32 u32Ticks);

#endif

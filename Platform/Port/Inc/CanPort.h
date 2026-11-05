/**
 * @file CanPort.h
 * @author yetingheng (004182)
 * @brief CAN 端口层：帧环 + 硬件覆盖可见性 + 发送队头推进
 * @version 0.1
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 职责（见 docs/review/steps/05-port-layer.md v0.4）：
 *   1. 1 ms 前台服务：按配额从驱动取帧进 RX 帧环、把 TX 帧环队头交给硬件；
 *   2. 硬件覆盖（MsgLst 粘滞位）按对象锁存，同一对象一次覆盖只记一次；
 *   3. 发送结果不确定时冻结 TX 队头，等上层裁定 DROP/RETRY，驱动不自动重发。
 *
 * 端口层不读任何 CAN 寄存器，只经 ST_CAN_PORT_DRV 调用驱动。
 */

#ifndef CAN_PORT_H
#define CAN_PORT_H

#include "F2837xS_device.h"
#include "CanQueue.h"
#include "Hw_CanDriver.h"

#define CAN_PORT_NUM          (2U)
#define CAN_PORT_RX_QUOTA     (4U)   /* 每轮每模块最多取帧数 */
#define CAN_PORT_TX_QUOTA     (2U)   /* 每轮每模块最多提交帧数（已裁定的 2 次尝试） */

/* 待裁定队头动作 */
#define CAN_PORT_HEAD_DROP    (1U)
#define CAN_PORT_HEAD_RETRY   (2U)

typedef struct {
    Uint16 (*pfIsInitialized)(Uint16 u16CanId);
    Uint16 (*pfIsPending)(Uint16 u16CanId);
    Uint16 (*pfReadRx)(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                       Uint16 *pu16Data, Uint16 *pu16Len,
                       Uint16 *pu16FrameType, Uint16 *pu16MsgLst);
    Uint16 (*pfSend)(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                     const Uint16 *pu16Data, Uint16 u16Len);
} ST_CAN_PORT_DRV;

typedef struct {
    Uint16 u16CanId;
    Uint16 u16TxObjIdx;                 /* 相对索引；驱动内部加 TxObjNum */
    Uint16 u16RxObjIdx;                 /* 相对索引；驱动内部加 RxObjNum */
    const ST_CAN_PORT_DRV *pstDrv;
    ST_CAN_QUEUE stRxQueue;
    ST_CAN_QUEUE stTxQueue;
    Uint32 u32ReadOk;
    Uint32 u32ReadBusy;
    Uint32 u32ReadErr;
    Uint32 u32ReadUnknown;
    Uint32 u32SwDrop;                   /* 软件帧环满被丢弃 */
    Uint32 u32TxOk;
    Uint32 u32TxBusy;
    Uint32 u32TxErr;
    Uint32 u32TxUnknown;
    Uint32 u32TxPendingBlock;           /* 因模块仍有未结束事务而未提交的次数 */
    Uint32 u32TxBudgetStop;             /* 因轮次预算用尽而未提交的次数 */
    Uint32 u32TxTypeReject;             /* 因帧类型与模块配置不符被拒的入队次数 */
    Uint32 u32TxRtrReject;              /* 因发送接口不支持 RTR 被拒的入队次数 */
    Uint16 bTxExtId;                    /* 本模块固定发送帧类型：1 = 扩展帧 */
    Uint16 bHwOverwriteSeen;            /* 粘滞：曾发生硬件覆盖 */
    volatile Uint16 bRxFrozen;          /* RX 结果不确定，等恢复通知 */
    volatile Uint16 bTxFrozen;          /* TX 结果不确定，等裁定 */
    volatile Uint16 bHeadPending;       /* TX 队头存在待裁定提交 */
    volatile Uint16 bNotReady;          /* 驱动初始化不可用 */
} ST_CAN_PORT;

void CanPortInit(void);
Uint16 CanPortBindDriver(Uint16 u16CanId, const ST_CAN_PORT_DRV *pstDrv);

/* 1 ms 前台：先 RX 后 TX，两方向各自按配额。 */
void CanPortService(void);

/** 只服务一路 CAN 的接收与发送（供 PortTask 做公平轮转） */
void CanPortServiceOne(Uint16 u16CanId);

/* 协议层：取一帧（无帧返回 0）。 */
Uint16 CanPortPopRx(Uint16 u16CanId, ST_CAN_FRAME *pstFrame);
/* 协议层：推一帧待发（帧环满、帧类型不符、RTR 不支持、DLC 超限都返回 0）。 */
Uint16 CanPortPushTx(Uint16 u16CanId, const ST_CAN_FRAME *pstFrame);

/* 设置本模块固定发送帧类型（1 = 扩展帧）。驱动当前按模块固定发送帧类型，
 * 端口层必须在入队时拒绝不匹配的帧，避免请求内容与实际上总线的报文不一致。 */
/* 设置本模块固定发送帧类型（0=标准帧，非 0=扩展帧）。
   仅作为初始化配置接口：TX 队列非空、存在待裁定队头或驱动有未决事务时拒绝（返回 0）；
   未决检查不看模块当前是否已初始化（重建失败后未决标记仍保留）。
   它只设置端口侧的入队校验规则，不改变驱动：调用者必须保证与驱动实际发送格式一致。 */
Uint16 CanPortSetTxFrameType(Uint16 u16CanId, Uint16 bExtId);
Uint16 CanPortTxTypeReject(Uint16 u16CanId);
Uint16 CanPortTxRtrReject(Uint16 u16CanId);
Uint32 CanPortTxPendingBlock(Uint16 u16CanId);
Uint32 CanPortTxBudgetStop(Uint16 u16CanId);
Uint16 CanPortTxFree(Uint16 u16CanId);

/* 待裁定队头：仅当"存在待裁定、参数合法、驱动已不 pending"时处理；
 * 重复裁定或非法参数不改队列、不改冻结状态，返回 HW_CAN_ERR。 */
Uint16 CanPortResolvePendingHead(Uint16 u16CanId, Uint16 u16Action);

/* 恢复通知：Bus-Off 消除与模块重建分开，由检测到恢复的调用方触发。 */
void CanPortOnBusOffCleared(Uint16 u16CanId);
void CanPortOnModuleRecovered(Uint16 u16CanId);

Uint16 CanPortIsRxFrozen(Uint16 u16CanId);
Uint16 CanPortIsTxFrozen(Uint16 u16CanId);
Uint16 CanPortIsReady(Uint16 u16CanId);
Uint16 CanPortHwOverwriteSeen(Uint16 u16CanId);
Uint32 CanPortSwDrop(Uint16 u16CanId);
Uint32 CanPortReadErr(Uint16 u16CanId);
Uint32 CanPortTxErr(Uint16 u16CanId);

#endif

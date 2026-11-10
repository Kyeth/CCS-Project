/**
 * @file Hw_SciDriver.h
 * @author yetingheng (004182)
 * @brief F28374S SCI 驱动：字节收发、接收状态分类、接收错误清除
 * @version 0.5
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 05B 变更：
 *   1. 新增 ST_SCI_RX_STATUS，把 FE/PE/OE/BI/RXFFOVF 分类后交端口层，
 *      不再把 SCIRXST.RXERROR 与 RXFFOVF 直接或在一起（旧包装无法分类）。
 *   2. Hw_SciClearRxError() 分普通清除与强制复位：bForce == 0 时发送未空闲
 *      就返回 WAIT_TX、不碰硬件；bForce != 0 时允许打断发送，损失由端口层锁存。
 *      出参只报剩余错误位，FIFO 层数走 ST_SCI_RX_STATUS。
 *   3. 原始条目：，按"发送空闲 -> FIFO 复位 -> 同步复位 -> 复查"
 *      执行一次受限的硬件清除，并把结果分类返回。
 *   3. 新增 Hw_SciIsTxIdle()，供端口层在复位前确认移位寄存器已空。
 */

#ifndef HW_SCI_DRIVER_H
#define HW_SCI_DRIVER_H
#include "F2837xS_device.h"

typedef enum { HW_SCI_ID_A=0U, HW_SCI_ID_B, HW_SCI_ID_C, HW_SCI_ID_D, HW_SCI_ID_MAX } E_HW_SCI_ID;

/* 接收状态位（分类证据，来自 SCIRXST 与 SCIFFRX）。 */
#define HW_SCI_RXSTS_FE      (0x0001U) /* 帧错误 */
#define HW_SCI_RXSTS_PE      (0x0002U) /* 奇偶错误 */
#define HW_SCI_RXSTS_OE      (0x0004U) /* 溢出错误（接收缓冲被覆盖） */
#define HW_SCI_RXSTS_BI      (0x0008U) /* 断线检测 */
#define HW_SCI_RXSTS_RXFFOVF (0x0010U)

/* 原始寄存器位掩码：SCIRXST 的 PE/OE/FE/BRKDT 与 SCIFFRX 的 RXFFOVF/RXFFST。
 * 一次整字读取后用它解码，避免逐位访问同一寄存器（外设访问 16 位一次，
 * 逐位读等于重复读同一地址）。位号见 F2837xS_sci.h 的 SCIRXST_BITS / SCIFFRX_BITS。 */
#define HW_SCI_RXST_PE_M     (0x0004U)
#define HW_SCI_RXST_OE_M     (0x0008U)
#define HW_SCI_RXST_FE_M     (0x0010U)
#define HW_SCI_RXST_BRKDT_M  (0x0020U)
#define HW_SCI_FFRX_RXFFOVF_M (0x8000U)
#define HW_SCI_FFRX_RXFFST_M  (0x1F00U)
#define HW_SCI_FFRX_RXFFST_SHIFT (8U) /* FIFO 层数右移位数 */

typedef struct {
    Uint16 u16Status;      /* HW_SCI_RXSTS_* 的位或；0 表示无错误 */
    Uint16 u16RxFifoLevel; /* 读取时刻的 RX FIFO 占用，0..16 */
    Uint16 u16TxFifoLevel; /* 读取时刻的 TX FIFO 占用，0..16 */
} ST_SCI_RX_STATUS;

/* Hw_SciClearRxError() 的结果分类。 */
#define HW_SCI_CLEAR_OK         (0U) /* 复位已完成且复查无残留 */
#define HW_SCI_CLEAR_WAIT_TX    (1U) /* 发送未空闲，未执行复位 */
#define HW_SCI_CLEAR_STILL_ERR  (2U) /* 复位已执行但复查仍有错误位 */
#define HW_SCI_CLEAR_UNREADABLE (3U) /* 参数错或模块未初始化，状态不可读 */
#define HW_SCI_CLEAR_TX_DIRTY   (4U) /* 复位已执行，但回读 TXFFST 仍非零：硬件未接受 */

typedef struct {
    Uint16 u16SciId;
    Uint32 u32Baudrate;
    Uint16 bLoopback;
    Uint16 u16DataFormat;  /* 仅支持 0（8N1）；其他值初始化报错 */
    Uint16 u16RxFifoLevel; /* RX FIFO 中断阈值，硬件范围 0..16；本步不使能中断 */
} ST_SCI_CFG_ITEM;
typedef struct {
    const ST_SCI_CFG_ITEM *pstSciCfgItem;
    Uint16 u16ItemNum;
    Uint32 u32LspclkHz;
} ST_SCI_CFG;

Uint16 Hw_SciInit(const ST_SCI_CFG *cfg);

/**
 * @brief 运行中修改某个 SCI 口的波特率（重算 BRR，保持 8N1 与 FIFO 配置不变）。
 * @return 1 = 已写入；0 = 参数非法或该口未初始化
 */
Uint16 Hw_SciSetBaud(Uint16 u16SciId, Uint32 u32Baudrate);
Uint16 Hw_SciTryReadByte(Uint16 id, Uint16 *data);
Uint16 Hw_SciTryWriteByte(Uint16 id, Uint16 data);
Uint16 Hw_SciGetTxFifoLevel(Uint16 id);
Uint16 Hw_SciGetRxFifoLevel(Uint16 id);

/* 读接收状态分类。返回 1 = 已读取（pstStatus 有效），0 = 参数错/未初始化（出参未写）。
 * bClear != 0 时顺带清 RXFFOVF 与 FIFO 中断标志位；返回的状态是清除前的观察值。 */
Uint16 Hw_SciGetRxStatus(Uint16 id, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear);

/* 一次受限的接收错误清除。返回 HW_SCI_CLEAR_*。
 * bForce = 0：发送未空闲（TXFFST != 0 或 TXEMPTY == 0）直接返回 WAIT_TX，不动硬件。
 * bForce != 0：允许打断正在发送的内容，并按 TRM 一并清空 TX FIFO
 *   （SCICTL1.SWRESET 不会清 FIFO 指针与内容，必须单独给 TXFIFORESET 脉冲）。
 * pu16Remain 只报"剩余错误位"（HW_SCI_RXSTS_* 的组合）：OK 时为 0，STILL_ERR 时非 0。
 * WAIT_TX / UNREADABLE 时不写出参。FIFO 占用请读 Hw_SciGetRxStatus() 的字段。 */
Uint16 Hw_SciClearRxError(Uint16 id, Uint16 bForce, Uint16 *pu16Remain);

/* 查询发送是否完全空闲：TX FIFO 空且移位寄存器空（SCICTL2.TXEMPTY）。
 * 返回 1 = 查询有效（*pu16Idle 给出结果），0 = 模块不可用（查询失败，不能当作忙）。
 * 端口层用“查询失败”与“忙”两种语义驱动不同分支，所以结果必须走状态式接口。 */
Uint16 Hw_SciGetTxIdle(Uint16 id, Uint16 *pu16Idle);

/* 便捷包装：返回 1 = 空闲，0 = 忙或不可用（仅用于不关心区别的调用者）。 */
Uint16 Hw_SciIsTxIdle(Uint16 id);

/* 旧包装：保留给不关心分类的调用者。
 * 返回 0 = 无错误，1 = 有错误，0xFFFF = 状态不可读（不可当作“无错误”）。 */
Uint16 Hw_SciGetRxError(Uint16 id);
#endif

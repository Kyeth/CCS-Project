/**
 * @file Hw_SciDriver.h
 * @brief 主机桩测试用的 SCI 驱动垫片（只声明端口层用到的入口）
 * @version 0.5
 * @date 2026-11-05
 */

#ifndef HW_SCI_DRIVER_SHIM_H
#define HW_SCI_DRIVER_SHIM_H

#include "F2837xS_device.h"

#define HW_SCI_ID_A (0U)
#define HW_SCI_ID_B (1U)
#define HW_SCI_ID_C (2U)
#define HW_SCI_ID_D (3U)

/* 接收状态位（与真实驱动同名同值） */
#define HW_SCI_RXSTS_FE      (0x0001U)
#define HW_SCI_RXSTS_PE      (0x0002U)
#define HW_SCI_RXSTS_OE      (0x0004U)
#define HW_SCI_RXSTS_BI      (0x0008U)
#define HW_SCI_RXSTS_RXFFOVF (0x0010U)

/* Hw_SciClearRxError() 的返回码 */
#define HW_SCI_CLEAR_OK         (0U)
#define HW_SCI_CLEAR_WAIT_TX    (1U)
#define HW_SCI_CLEAR_STILL_ERR  (2U)
#define HW_SCI_CLEAR_UNREADABLE (3U)

typedef struct
{
    Uint16 u16Status;
    Uint16 u16RxFifoLevel;
    Uint16 u16TxFifoLevel;
} ST_SCI_RX_STATUS;

Uint16 Hw_SciTryReadByte(Uint16 id, Uint16 *data);
Uint16 Hw_SciTryWriteByte(Uint16 id, Uint16 data);
Uint16 Hw_SciGetRxStatus(Uint16 id, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear);
Uint16 Hw_SciClearRxError(Uint16 id, Uint16 *pu16Remain);
Uint16 Hw_SciGetTxIdle(Uint16 id, Uint16 *pu16Idle);
Uint16 Hw_SciIsTxIdle(Uint16 id);
Uint16 Hw_SciGetRxError(Uint16 id);

#endif /* HW_SCI_DRIVER_SHIM_H */

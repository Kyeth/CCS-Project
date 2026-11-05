/**
 * @file Hw_CanDriver.h
 * @brief 主机桩测试用的 CAN 驱动垫片（只声明端口层用到的入口与返回码）
 */

#ifndef HW_CAN_DRIVER_SHIM_H
#define HW_CAN_DRIVER_SHIM_H

#include "F2837xS_device.h"

#define HW_CAN_ID_A          (0U)
#define HW_CAN_ID_B          (1U)
#define HW_CAN_DATA_LEN_MAX  (8U)

#define HW_CAN_EMPTY         (0U)
#define HW_CAN_OK            (1U)
#define HW_CAN_BUSY          (2U)
#define HW_CAN_ERR           (3U)
#define HW_CAN_UNKNOWN       (4U)

#define HW_CAN_FRAME_STD     (0U)
#define HW_CAN_FRAME_EXT     (1U)

Uint16 Hw_CanIsInitialized(Uint16 u16CanId);
Uint16 Hw_CanIsPending(Uint16 u16CanId);
Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len);
Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len,
                    Uint16 *pu16FrameType, Uint16 *pu16MsgLst);

#endif /* HW_CAN_DRIVER_SHIM_H */

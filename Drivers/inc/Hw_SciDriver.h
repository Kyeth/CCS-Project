#ifndef HW_SCI_DRIVER_H
#define HW_SCI_DRIVER_H
#include "F2837xS_device.h"

typedef enum { HW_SCI_ID_A=0U, HW_SCI_ID_B, HW_SCI_ID_C, HW_SCI_ID_D, HW_SCI_ID_MAX } E_HW_SCI_ID;
typedef struct {
    Uint16 u16SciId;
    Uint32 u32Baudrate;
    Uint16 bLoopback;
} ST_SCI_CFG_ITEM;
typedef struct {
    const ST_SCI_CFG_ITEM *pstSciCfgItem;
    Uint16 u16ItemNum;
    Uint32 u32LspclkHz;
} ST_SCI_CFG;

Uint16 Hw_SciInit(const ST_SCI_CFG *cfg);
Uint16 Hw_SciTryReadByte(Uint16 id, Uint16 *data);
Uint16 Hw_SciTryWriteByte(Uint16 id, Uint16 data);
Uint16 Hw_SciGetRxError(Uint16 id);
#endif

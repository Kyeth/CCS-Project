#ifndef __FM24C128DTRAMSPORT_H
#define __FM24C128DTRAMSPORT_H

#include "FM24C128D.h"

#ifdef __cplusplus
    extern "C" {
#endif

extern int16 FM24CRead(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen);


extern int16 FM24CWrite(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen);


extern int16 FM24CGetTick(void);


#ifdef __cplusplus
    }
#endif

#endif

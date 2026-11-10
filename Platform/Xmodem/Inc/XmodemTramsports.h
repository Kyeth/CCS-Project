#ifndef __XMODEMTRAMSPORT_H
#define __XMODEMTRAMSPORT_H

#include "F2837xS_device.h"


#ifdef __cplusplus
extern "C" {
#endif




extern int XmodemWrite(const Uint16 *u16Buff, Uint16 u16Length);

/* 端口适配：握手同批已经从环里取走的字节，先还给读路径 */
void XmodemRxPreload(const Uint16 *pu16Data, Uint16 u16Length);


extern int XmodemRead(Uint16 *u16Buff, Uint16 u16MaxLength);





#ifdef __cplusplus
}
#endif


#endif

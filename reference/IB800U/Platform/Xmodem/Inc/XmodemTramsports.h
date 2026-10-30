#ifndef __XMODEMTRAMSPORT_H
#define __XMODEMTRAMSPORT_H

#include "F28003x_device.h"


#ifdef __cplusplus
extern "C" {
#endif




extern int XmodemWrite(const Uint16 *u16Buff, Uint16 u16Length);


extern int XmodemRead(Uint16 *u16Buff, Uint16 u16MaxLength);





#ifdef __cplusplus
}
#endif


#endif

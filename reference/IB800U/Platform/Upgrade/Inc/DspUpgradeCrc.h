#ifndef __DSPUPGRADECRC_H
#define __DSPUPGRADECRC_H

#include "F28003x_device.h"


#ifdef __cplusplus
extern "C" {
#endif


extern Uint32 UpgradeCrc32(Uint16 *pu16Data, Uint32 u32Length, Uint32 u32LastCrc);



#ifdef __cplusplus
}
#endif

#endif

#ifndef __XMODEMSLAVEPROCESS_H
#define __XMODEMSLAVEPROCESS_H

#include "XmodemCore.h"

#ifdef __cplusplus
    extern "C"{
#endif

extern void XmodemProcessInit(void);
extern void TaskXmodemSlave_1(void);

extern ST_XMODEM_PROTOCOL ObjXmodemCtrl;

#ifdef __cplusplus
    }
#endif

#endif

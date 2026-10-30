#ifndef __XMODEMDATAHANDLE_H
#define __XMODEMDATAHANDLE_H

#include "F28003x_device.h"

#include "XmodemCore.h"

#ifdef __cplusplus
extern "C" {
#endif


extern int ReadBlock(Uint16 u16BlockNum, Uint16 *u16Buffer);

extern int WriteBlock(Uint16 u16BlockNum, Uint16 *u16Buffer, Uint16 u16Length);

extern int Trig(void);

extern int Reset(void);

extern int TrigWriteCfg(ST_UPGRADE_FILE_HEADER *pstUpgradeFile, Uint16 u16UpgradeFlag);

extern int ResetWriteCfg(void);

extern int SetTotal(Uint32 u32Total);

#ifdef __cplusplus
    }
#endif


#endif

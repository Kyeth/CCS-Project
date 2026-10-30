#ifndef __VERSIONUSER_H
#define __VERSIONUSER_H

#include "F28003x_Examples.h"
#include "VersionManagement.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define VERSION                 (201200105U)
#define BUILD                   (1U)
#define DEBUG                   (2U)
#define VERSION_FLASH_ADDR      (0x80004)

extern const ST_VERSION stVersion;
extern ST_VERSION stBootloaderVersion;
extern void VersionUserInit(void);

#ifdef __cplusplus
    }
#endif


#endif

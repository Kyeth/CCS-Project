#ifndef __VERSIONMANAGEMENT_H
#define __VERSIONMANAGEMENT_H

#include "F28003x_Examples.h"

#ifdef __cplusplus
    extern "C" {
#endif


typedef union
{
    Uint32 u32VersionAll;
    struct
    {
        Uint16 u16VersionLow;
        Uint16 u16VersionHigh;
    }Word;
}U_VERSION_WORD;

typedef struct
{
    Uint16 u16SN1;
    Uint16 u16SN2;
    Uint16 u16SN3;
    Uint16 u16SN4;
}ST_SERIAL_NUMBER;

typedef struct
{
    U_VERSION_WORD uVersion;        // 版本料号，用于上传到BMS
    Uint16 u16Build;                // 编译号
    Uint16 u16Debug;                // 调试号
    ST_SERIAL_NUMBER stSerialNumBer;// 序列号
}ST_VERSION;


extern void VersionInit(ST_VERSION *pstVersion, Uint32 u32Version, Uint16 u16Build, Uint16 u16Debug);

#ifdef __cplusplus
    }
#endif

#endif

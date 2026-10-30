#include "VersionManagement.h"



void VersionInit(ST_VERSION *pstVersion, Uint32 u32Version, Uint16 u16Build, Uint16 u16Debug)
{
    pstVersion->uVersion.u32VersionAll = u32Version;
    pstVersion->u16Build = u16Build;
    pstVersion->u16Debug = u16Debug;
    pstVersion->stSerialNumBer.u16SN1 = 0;
    pstVersion->stSerialNumBer.u16SN2 = 0;
    pstVersion->stSerialNumBer.u16SN3 = 0;
    pstVersion->stSerialNumBer.u16SN4 = 0;
}

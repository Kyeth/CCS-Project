#include "VersionUser.h"
#include "string.h"

// 需要进行全局变量赋初值，这样才能指定写入固定的flash位置
#pragma DATA_SECTION(stVersion, "version")
const ST_VERSION stVersion = {
    .uVersion = {
        .u32VersionAll = VERSION
    },
    .u16Build = BUILD,
    .u16Debug = DEBUG,
    .stSerialNumBer = {
        .u16SN1 = 0,
        .u16SN2 = 0,
        .u16SN3 = 0,
        .u16SN4 = 0
    }
};

ST_VERSION stBootloaderVersion;

void GetBootLoaderVersion(ST_VERSION* pstVersion)
{
    memcpy(pstVersion, (Uint32*)VERSION_FLASH_ADDR, sizeof(ST_VERSION));
}

void VersionUserInit(void)
{
    // 这行代码确保 stVersion 不会被优化掉
    volatile Uint32 dummy = stVersion.uVersion.u32VersionAll;
    (void)dummy; // 防止未使用变量警告

    GetBootLoaderVersion(&stBootloaderVersion);
}

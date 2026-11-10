/******************************************************************************
 * F021 Flash API 的回调（TI C2000Ware 提供，工程需自带一份）
 *
 * 芯片适配：参考工程用 F28003x 版本，这里换成 F2837xS 的库；函数签名与
 * 节名保持不变，仍随 .flashfunc 一起搬到 RAM 执行。
 ******************************************************************************/
#ifndef _F2837xS
#define _F2837xS
#endif
#include "F021.h"
#include "Dsp2837xSFlash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* F021 头文件在 DEBUG 构建里会用到 driverlib 的 ASSERT 宏，它引用 __error__。
 * 本工程没有别处定义它，这里补一个：升级路径走到非法参数就停下等看门狗。 */
#pragma CODE_SECTION(__error__, ".flashfunc")
void __error__(const char *filename, unsigned long line)
{
    (void)filename;
    (void)line;
    while (1)
    {
        FlashFeedWatchDog();
    }
}

#pragma CODE_SECTION(Fapi_serviceWatchdogTimer, ".flashfunc")

Fapi_StatusType Fapi_serviceWatchdogTimer(void)
{
    FlashFeedWatchDog();
    return (Fapi_Status_Success);
}

#ifdef __cplusplus
}
#endif

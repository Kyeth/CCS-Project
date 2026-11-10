/**
 * @file Fapi_UserDefinedFunctions.c
 * @brief F021 库的回调：擦写等待期间刷新看门狗。
 *
 * 库里有一个默认的空实现，但空实现意味着擦除一整个 32K 字扇区期间
 * 看门狗无人喂。这里用工程自己的看门狗接口替换掉，并且跟其它 F021
 * 调用一起放在 .flashfunc（RAM 运行）——等待循环在 RAM 里跑，回调也
 * 必须在 RAM 里，不能回到正在擦写的 Flash 上取指。
 */

#ifndef _F2837xS
#define _F2837xS
#endif

#include "F021.h"

#include "Hw_WatchDog.h"

#if defined(__TMS320C28XX__)
#pragma CODE_SECTION(Fapi_serviceWatchdogTimer, ".flashfunc")
#endif

/**
 * @brief F021 在状态机等待期间调用，返回后库继续轮询
 */
Fapi_StatusType Fapi_serviceWatchdogTimer(void)
{
    Hw_FeedWatchDog();

    return Fapi_Status_Success;
}

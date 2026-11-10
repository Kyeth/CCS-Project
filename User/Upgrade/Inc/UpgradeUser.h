#ifndef __UPGRADEUSER_H
#define __UPGRADEUSER_H

#include "F2837xS_device.h"

#include "UpgradeCore.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef void (*FuncAppStart)(void);

/* 参考工程就是全局句柄；APP 接线需要直接读写配置 */
extern ST_UPGRADE_HANDLE stUpgradeHandle;


extern void EraseBackUpBank(void);
extern void UpgradeUserInit(void);


extern void CheckUpgrade(void);


extern void TaskUpgradeHandle(void);

extern void TaskUpgrade(void);

#ifdef __cplusplus
}
#endif


#endif

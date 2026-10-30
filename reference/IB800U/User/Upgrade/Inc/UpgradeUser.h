#ifndef __UPGRADEUSER_H
#define __UPGRADEUSER_H

#include "F28003x_device.h"

#include "UpgradeCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FuncAppStart)(void);
extern void UpgradeUserInit(void);
extern void CheckUpgrade(void);
extern void TaskUpgradeHandle(void);
extern void TaskCheckUpgrade(void);
extern ST_UPGRADE_HANDLE stUpgradeHandle;

#ifdef __cplusplus
}
#endif


#endif

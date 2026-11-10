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


/* 返回 0 = 全部扇区擦除成功，其他 = 第一个失败码（上层必须停止推进） */
extern int16 EraseBackUpBank(void);
extern void UpgradeUserInit(void);


extern void CheckUpgrade(void);


extern void TaskUpgradeHandle(void);

/* 返回 0 = 进行中，1 = 已写入配置（函数内部已复位），2 = 中止（调用方负责收尾） */
extern Uint16 TaskUpgrade(void);

#ifdef __cplusplus
}
#endif


#endif

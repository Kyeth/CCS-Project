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
/* 返回 0 = APP 四个扇区全部擦除成功 */
extern int16 EraseAppBank(void);
/* 返回 0 = BOOT 三个扇区（A/B/C）全部擦除成功；自升级专用 */
extern int16 EraseBootBank(void);
extern void UpgradeUserInit(void);
/* 开始/中止一次升级会话时清掉会话级标志 */
extern void UpgradeUserSessionReset(void);


extern void CheckUpgrade(void);
/* 中止收尾：清会话与接收残留，APP 有效则跳转，否则重新准备接收 */
extern void UpgradeUserAbort(void);


extern void TaskUpgradeHandle(void);

/* 返回 0 = 进行中，1 = 已写入配置（函数内部已复位），2 = 中止（调用方负责收尾） */
extern Uint16 TaskUpgrade(void);

#ifdef __cplusplus
}
#endif


#endif

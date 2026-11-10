#include "AppUpgrade.h"
#include "UpgradeUser.h"
#include "UpgradeCore.h"
#include "Dsp2837xSFlash.h"
#include "XmodemCore.h"
#include "XmodemSlaveProcess.h"
#include "XmodemTramsports.h"
#include "Hw_SciDriver.h"
#include "SciPort.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 升级会话的三个阶段 */
#define APP_UP_PHASE_WAIT_TX   (0U)   /* 等 8 字节握手应答真正出环 */
#define APP_UP_PHASE_ERASE     (1U)   /* 擦备份区 + 写"未完成"升级配置 */
#define APP_UP_PHASE_RECEIVE   (2U)   /* 交给参考工程的 TaskUpgrade() 收包 */

static Uint16 s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
static Uint16 s_u16UpActive = 0U;
static void (*s_pfPause)(void) = 0;

extern ST_UPGRADE_HANDLE stUpgradeHandle;


/**
 * @brief 写一份"升级进行中"的配置，让参考工程的 TaskUpgrade() 守卫
 *        （ReadUpgradeCfg == 1 && GetUpgradeFirmwareFlag == 0）通过。
 *        参考工程的 APP 侧是由 Modbus 升级命令写这份配置，本工程在交接时写。
 */
static void AppUpWriteProgressCfg(void)
{
    memset(&stUpgradeHandle.stUpgradeCfg, 0, sizeof(ST_UPGRADE_CFG));
    stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0xAAEEU;
    stUpgradeHandle.stUpgradeCfg.u16UpgradeUart = 0U;   /* SCI_ID0，与参考工程一致 */
    stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud = 0U;   /* 0 = 保持 38400 */
    stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0U;
    stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = CalcUpgradeCfgSum(&stUpgradeHandle);

    (void)EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
    (void)WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr,
                         &stUpgradeHandle.stUpgradeCfg,
                         (Uint32)(sizeof(ST_UPGRADE_CFG) / sizeof(Uint16)));
}


void AppUpgradeInit(void)
{
    XmodemProcessInit();
    Dsp2837xSFlashInit();
    UpgradeUserInit();

    s_u16UpActive = 0U;
    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
}


Uint16 AppUpgradeStart(Uint16 u16SciId, const Uint16 *pu16Tail, Uint16 u16TailLen)
{
    if (u16SciId != HW_SCI_ID_A)
    {
        return 0U;
    }

    /* 握手同批多取上来的字节先还给协议层的读路径 */
    XmodemRxPreload(pu16Tail, u16TailLen);

    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u16UpActive = 1U;

    /* 交接即暂停功率输出与控制动作；端口服务、时基、看门狗继续跑 */
    if (s_pfPause != 0)
    {
        s_pfPause();
    }

    return 1U;
}


void AppUpgradeSetPauseHook(void (*pfPause)(void))
{
    s_pfPause = pfPause;
}


Uint16 AppUpgradeIsActive(void)
{
    return s_u16UpActive;
}


void AppUpgradeTask(void)
{
    if (s_u16UpActive == 0U)
    {
        return;
    }

    switch (s_u16UpPhase)
    {
        case APP_UP_PHASE_WAIT_TX:
            /* 握手应答还留在环里时不能擦除：擦除期间主循环服务不到端口 */
            if (SciPortIsTxIdle(HW_SCI_ID_A) != 0U)
            {
                s_u16UpPhase = APP_UP_PHASE_ERASE;
            }
            break;

        case APP_UP_PHASE_ERASE:
            EraseBackUpBank();
            AppUpWriteProgressCfg();
            s_u16UpPhase = APP_UP_PHASE_RECEIVE;
            break;

        case APP_UP_PHASE_RECEIVE:
            /* 参考工程自己的收包流程：XmodemReceive + 收完后写配置并复位 */
            TaskUpgrade();
            break;

        default:
            break;
    }
}


#ifdef __cplusplus
}
#endif

#include "AppUpgrade.h"
#include "AppModbus.h"
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
static Uint16 s_u16FlashReady = 0U;
static Uint32 s_u32AbortCount = 0U;
static void (*s_pfPause)(void) = 0;

extern ST_UPGRADE_HANDLE stUpgradeHandle;

static void AppUpgradeAbort(void);


/**
 * @brief 写一份"升级进行中"的配置，让参考工程的 TaskUpgrade() 守卫
 *        （ReadUpgradeCfg == 1 && GetUpgradeFirmwareFlag == 0）通过。
 *        参考工程的 APP 侧是由 Modbus 升级命令写这份配置，本工程在交接时写。
 * @return 0 = 成功，其他 = 擦除/写入失败码
 */
static int16 AppUpWriteProgressCfg(void)
{
    int16 i16Ret = 0;

    memset(&stUpgradeHandle.stUpgradeCfg, 0, sizeof(ST_UPGRADE_CFG));
    stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0xAAEEU;
    stUpgradeHandle.stUpgradeCfg.u16UpgradeUart = 0U;   /* SCI_ID0，与参考工程一致 */
    stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud = 0U;   /* 0 = 保持 38400 */
    stUpgradeHandle.stUpgradeCfg.u16FirmwareOver = 0U;
    stUpgradeHandle.stUpgradeCfg.u32UpgradeCfgSum = CalcUpgradeCfgSum(&stUpgradeHandle);

    /* 必要适配修正：参考实现忽略配置区擦除/写入的返回值，
     * 配置没写进去也会继续开始收包。这里逐级上报。 */
    i16Ret = EraseSingleSector(stUpgradeHandle.u32UpgradeCfgAddr);
    if (i16Ret != 0)
    {
        return i16Ret;
    }

    return WriteFlashData(stUpgradeHandle.u32UpgradeCfgAddr,
                          &stUpgradeHandle.stUpgradeCfg,
                          (Uint32)sizeof(ST_UPGRADE_CFG));
}


void AppUpgradeInit(void)
{
    s_u16UpActive = 0U;
    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u16FlashReady = 0U;

    /* 顺序同参考工程 PlatformInit()：协议实例 -> Flash API -> 升级句柄 */
    XmodemProcessInit();
    s_u16FlashReady = (Dsp2837xSFlashInit() == 0) ? 1U : 0U;
    UpgradeUserInit();
}


Uint16 AppUpgradeStart(Uint16 u16SciId, const Uint16 *pu16Tail, Uint16 u16TailLen)
{
    if (u16SciId != HW_SCI_ID_A)
    {
        return 0U;
    }

    /* Flash API 没初始化成功就不能进升级（擦写会失败） */
    if (s_u16FlashReady == 0U)
    {
        return 0U;
    }

    /* 握手同批多取上来的字节先还给协议层的读路径（内部会清掉上一次的半包） */
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


Uint32 AppUpgradeAbortCount(void)
{
    return s_u32AbortCount;
}


/**
 * @brief 中止收尾：停止接收、清本次会话状态、把端口交还 Modbus
 * @note  功率输出保持关闭（暂停钩子没有恢复动作），主机重新握手即可再升级。
 */
static void AppUpgradeAbort(void)
{
    s_u16UpActive = 0U;
    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u32AbortCount++;

    /* 协议层与适配层都清成初始态，避免上一次的半包混进下一次会话 */
    (void)XmodemProcessInit();
    XmodemRxReset();

    /* 把端口还给 Modbus：丢弃候选/未发完的回复，重新同步流 */
    AppModbusReleasePort(APP_UP_PORT_INDEX);
}


void AppUpgradeTask(void)
{
    Uint16 u16Ret = 0U;

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
            if (EraseBackUpBank() != 0)
            {
                AppUpgradeAbort();
                break;
            }
            if (AppUpWriteProgressCfg() != 0)
            {
                AppUpgradeAbort();
                break;
            }
            s_u16UpPhase = APP_UP_PHASE_RECEIVE;
            break;

        case APP_UP_PHASE_RECEIVE:
            /* 参考工程自己的收包流程：XmodemReceive + 收完后校验、写配置、复位 */
            u16Ret = TaskUpgrade();
            if (u16Ret == 2U)
            {
                AppUpgradeAbort();
            }
            break;

        default:
            break;
    }
}


#ifdef __cplusplus
}
#endif

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

/* 本次会话的串口速率：决定"接收中"配置里 u16UpgradeBaud 的编码。 */
static Uint32 s_u32UpBaud = 19200UL;

/* 已按握手请求停机（旧 ARM 一发 F0 就要停功率输出，不能等到交接） */
static Uint16 s_u16StopRequested = 0U;


/**
 * @brief 把实际波特率折算成配置里的编码（x100），BOOT 重入时按它恢复
 */
static Uint16 AppUpBaudCode(Uint32 u32Baud)
{
    if (u32Baud >= 115200UL) {
        return 1152U;
    }
    if (u32Baud >= 38400UL) {
        return 384U;
    }
    if (u32Baud >= 19200UL) {
        return 192U;
    }

    return 0U;
}


/**
 * @brief
 * @return 0 = 成功，其他 = 擦除/写入失败码
 */
static int16 AppUpWriteProgressCfg(void)
{
    int16 i16Ret = 0;

    memset(&stUpgradeHandle.stUpgradeCfg, 0, sizeof(ST_UPGRADE_CFG));
    stUpgradeHandle.stUpgradeCfg.u16UpgradeFlag = 0xAAEEU;
    stUpgradeHandle.stUpgradeCfg.u16UpgradeUart = 0U;   /* SCI-A */
    /* 编码按本次实际速率写：掉电重入时 BOOT 用它恢复串口（115200 → 1152） */
    stUpgradeHandle.stUpgradeCfg.u16UpgradeBaud = AppUpBaudCode(s_u32UpBaud);
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


/**
 * @brief 升级许可 + 停机请求（在接受 F0 命令时调用）
 * @return 1 = 许可，可以继续握手；0 = 拒绝（忙或 Flash API 未就绪）
 * @note  旧 ARM 收到 F0 应答后就会退出业务通信，所以停机必须在这里做；
 *        之后若握手失败，按既定规则功率输出保持关闭，由复位恢复。
 */
Uint16 AppUpgradePrepare(void)
{
    if ((s_u16FlashReady == 0U) || (s_u16UpActive != 0U))
    {
        return 0U;
    }

    if ((s_u16StopRequested == 0U) && (s_pfPause != 0))
    {
        s_pfPause();
    }

    s_u16StopRequested = 1U;

    return 1U;
}


/**
 * @brief ARM 自身升级通知：只关驱动，不进入 DSP 升级流程
 * @return 1 = 已处理（重复收到可以重复回复）
 * @note  不切速、不擦 Flash、不写升级配置、不复位、不发 'C'；
 *        之后若真的收到 DSP 升级握手，AppUpgradePrepare() 不会再重复停机。
 */
Uint16 AppUpgradeArmNotify(void)
{
    if ((s_u16StopRequested == 0U) && (s_pfPause != 0))
    {
        s_pfPause();
    }

    s_u16StopRequested = 1U;

    return 1U;
}


void AppUpgradeInit(void)
{
    s_u16UpActive = 0U;
    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u16FlashReady = 0U;
    s_u16StopRequested = 0U;

    /* 顺序同参考工程 PlatformInit()：协议实例 -> Flash API -> 升级句柄 */
    XmodemProcessInit();
    s_u16FlashReady = (Dsp2837xSFlashInit() == 0) ? 1U : 0U;
    UpgradeUserInit();
}


Uint16 AppUpgradeStart(Uint16 u16SciId, Uint32 u32Baud,
                       const Uint16 *pu16Tail, Uint16 u16TailLen)
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

    /* 新会话开始：清掉上一次的会话级标志（尾部完成标志） */
    UpgradeUserSessionReset();

    /* 握手同批多取上来的字节先还给协议层的读路径（内部会清掉上一次的半包） */
    XmodemRxPreload(pu16Tail, u16TailLen);

    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u16UpActive = 1U;
    s_u32UpBaud = u32Baud;

    /* 停机按既定规则只做一次；AppUpgradePrepare() 可能已经做过 */
    if ((s_u16StopRequested == 0U) && (s_pfPause != 0))
    {
        s_pfPause();
    }

    s_u16StopRequested = 1U;

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
 * @author yetingheng (004182)
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 */
static void AppUpgradeAbort(void)
{
    s_u16UpActive = 0U;
    s_u16UpPhase = APP_UP_PHASE_WAIT_TX;
    s_u32AbortCount++;

    /* 协议层、适配层与升级层的会话状态全清，避免上一次的半包与完成标志混进下一次会话 */
    (void)XmodemProcessInit();
    XmodemRxReset();
    UpgradeUserSessionReset();

    /* 失败收尾：切回业务速率成功后才把端口还给 Modbus；失败时端口停在恢复态
     * 由 AppModbus 逐轮重试，避免硬件还在 115200 而主站按 19200 发帧。 */
    AppModbusRestorePort(APP_UP_PORT_INDEX, 19200UL);
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
            /* 自己的收包流程：XmodemReceive + 收完后校验、写配置、复位 */
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

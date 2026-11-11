/**
 * @file main.c
 * @brief EP15K_BOOT 最小 Bootloader 主程序（F28374S，入口 0x080000）
 * @version 1.0
 * @date 2026-11-10
 *
 * @note 本工程按参考工程 IB800U_BootLoader（commit 68701bd）移植，只保留 BOOT
 *       需要的模块：SCI 驱动、RingBuff、端口层（SciPort/PortTask/PortCritical）、
 *       XMODEM、Flash（F021）与 Upgrade（升级配置/回拷/CRC）。
 *       控制功能（ADC/PWM/CLA/CAN/采样/Modbus）与主机测试代码不在本工程内。
 *
 * @note 上电流程：
 *       1. 把 .TI.ramfunc 与 .flashfunc 从 Flash 搬到 RAM（F021 擦写必须在 RAM 跑）
 *       2. 时钟/时基/GPIO/SCI/CPU Timer1（100 us 心跳）
 *       3. 平台层：端口环 + 端口任务 + Xmodem + Flash + UpgradeUserInit
 *       4. CheckUpgrade()：有完整升级包则校验+回拷+跳 APP；没有则按 APP 有效性决定
 *          跳 APP 还是留在 BOOT 收包
 *       5. 主循环：喂狗 + 端口服务 + 升级接收状态机
 */

#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "DrvCfg_EP15K.h"
#include "Hw_TimeBase.h"
#include "Hw_WatchDog.h"
#include "SciPort.h"
#include "PortTask.h"
#include "UpgradeCore.h"
#include "UpgradeUser.h"
#include "Dsp2837xSFlash.h"
#include "XmodemSlaveProcess.h"
#include "sysctl.h"

/* .TI.ramfunc / .flashfunc 的搬运符号，由 CMD/2837xS_FLASH_BOOT_CLA_lnk.cmd 定义 */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;
extern Uint16 _FlashfuncLoadStart;
extern Uint16 _FlashfuncLoadSize;
extern Uint16 _FlashfuncRunStart;

/* 非 0 表示某个必要外设没起来：心跳里据此不服务端口，避免把坏状态当正常 */
static Uint16 s_u16InitError;

/**
 * @brief 从 Flash 把一段代码复制到它的运行地址。
 */
static void CopyRunFromLoad(Uint16 *pu16Run, const Uint16 *pu16Load, Uint16 u16Words)
{
    Uint16 i;

    for (i = 0U; i < u16Words; i++)
    {
        pu16Run[i] = pu16Load[i];
    }
}

/**
 * @brief 搬 .TI.ramfunc（CPU Timer/SysCtrl 等 RAM 函数）与 .flashfunc（F021 + Flash 驱动）。
 *
 * @note 必须在任何 Flash 擦写之前执行：F021 库与 Flash 驱动都链接在 Flash、运行在
 *       RAM，未搬运时调用会跑到未初始化的 RAM 上。搬运长度取自链接符号，单位是字。
 */
static void CopyRamSections(void)
{
    CopyRunFromLoad(&RamfuncsRunStart, &RamfuncsLoadStart,
                    (Uint16)(((Uint32)&RamfuncsLoadSize) & 0xFFFFUL));
    CopyRunFromLoad(&_FlashfuncRunStart, &_FlashfuncLoadStart,
                    (Uint16)(((Uint32)&_FlashfuncLoadSize) & 0xFFFFUL));
}

/**
 * @brief 100 us 心跳：清中断标志后驱动端口层的接收服务与发送推进。
 */
static __interrupt void Isr_Heartbeat(void)
{
    /* TIF 只在这里清；Hw_CpuTimerIsrTick() 不动它 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);
    PortTaskServiceFromHeartbeat(s_u16InitError);
}

/**
 * @brief 配置中断向量与使能位（CPU Timer1 = INT13，不经 PIEIER 分组）。
 */
static void InterruptVectTableInit(void)
{
    DINT;
    InitPieCtrl();
    IER = 0U;
    IFR = 0U;
    InitPieVectTable();

    EALLOW;
    PieVectTable.TIMER1_INT = &Isr_Heartbeat;
    EDIS;

    /* CPU Timer1 属 CPU 级 INT13，不在 PIE 分组内，只需 IER 位 */
    IER |= M_INT13;
}

/**
 * @brief 芯片与板级外设初始化（只做 BOOT 需要的部分）。
 */
static void PeripheralsInit(void)
{
    if (Hw_ClockInit(&stClockCfg) == 0U)
    {
        s_u16InitError = 1U;
    }

    if ((s_u16InitError == 0U) && (Hw_TimeBaseInit(stClockCfg.u32SysclkHz) == 0U))
    {
        s_u16InitError = 1U;
    }

    InterruptVectTableInit();

    /* 板级表先把驱动使能（GPIO88）驱动为低，再复用其它引脚 */
    Hw_GpioInit(&stGpioCfg);
    Hw_GpioWritePin(88U, 0U);

    if ((s_u16InitError == 0U) && (Hw_SciInit(&stSciCfg) == 0U))
    {
        s_u16InitError = 1U;
    }

    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
}

/**
 * @brief 平台层初始化：端口环、端口任务、Xmodem、Flash API、升级句柄。
 */
static void PlatformInit(void)
{
    SciPortInit();
    PortTaskInit();
    (void)PortTaskRegister();

    XmodemProcessInit();

    if (Dsp2837xSFlashInit() != 0)
    {
        s_u16InitError = 1U;
    }

    UpgradeUserInit();
}

/**
 * @brief 启动时基与中断，打开看门狗，然后进主循环。
 */
static void StartSystem(void)
{
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);

    EINT;
    ERTM;

    Hw_EnableWatchDog();
    Hw_FeedWatchDog();
}

/**
 * @brief BOOT 主程序。
 *
 * @note CheckUpgrade() 在"有完整升级包"或"APP 有效"时不返回（回拷后跳 APP /
 *       直接跳 APP）；只有在"APP 无效或需要继续收包"时返回，主循环才开始收升级。
 */
void main(void)
{
    CopyRamSections();  /* 先把 RAM 代码段搬到运行地址，Flash 擦写才有 RAM 版 F021 */

    PeripheralsInit();
    PlatformInit();

    CheckUpgrade();     /* 不返回：已跳 APP；返回：留在 BOOT 收升级 */
    StartSystem();

    while (1)
    {
        Hw_FeedWatchDog();
        (void)PortTaskService();

        /* 2 = 本次会话被中止：由升级层收尾（清残留；APP 有效则启动，空白则重新收包） */
        if (TaskUpgrade() == 2U)
        {
            UpgradeUserAbort();
        }
    }
}

/**
 * @file SystemStateMachine.h
 * @author yetingheng (004182)
 * @brief 系统总状态机：上电流程编排、开机/停机命令、故障聚合与复位
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 *
 * @note 分工：
 *       - 系统总状态机（本文件）只管“流程与聚合”：什么时候允许开机、什么时候
 *         停机、子系统谁报故障、故障怎么清；它不直接写 PWM 与继电器。
 *       - 逆变/PV/BUS 子状态机各自管自己那一片的执行细节（发波、继电器、环路）。
 *
 *       上电流程：INIT -> WAIT_ZERO（等 ADC 零漂）-> SELFTEST（自检）-> STANDBY
 *       开机：STANDBY -> STARTUP（调 InvSmStart，等子状态机进 RUN）-> RUN
 *       停机：RUN -> STOPPING（调 InvSmStop，等回 SELF）-> STANDBY
 *       故障：任意 -> FAULT（断继电器、封锁）-> RECOVER -> STANDBY
 */

#ifndef SYSTEM_STATE_MACHINE_H
#define SYSTEM_STATE_MACHINE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 状态枚举
 ******************************************************************************/
typedef enum
{
    SYS_ST_INIT = 0U,       /* 上电初始化：封锁、继电器断开 */
    SYS_ST_WAIT_ZERO,       /* 等 ADC 零漂检测完成 */
    SYS_ST_SELFTEST,        /* 自检：采样/子状态机/通讯在线 */
    SYS_ST_STANDBY,         /* 待机：允许开机 */
    SYS_ST_STARTUP,         /* 开机中：等逆变状态机进入 RUN */
    SYS_ST_RUN,             /* 运行 */
    SYS_ST_STOPPING,        /* 停机中：等子状态机回到 SELF */
    SYS_ST_FAULT,           /* 故障：封锁、断继电器、等复位 */
    SYS_ST_RECOVER,         /* 故障恢复：稳定一段时间再回待机 */
    SYS_ST_MAX
} E_SYS_STATE;

/******************************************************************************
 * 故障位（聚合）
 ******************************************************************************/
#define SYS_FAULT_NONE          (0x0000U)
#define SYS_FAULT_ZERO          (0x0001U)   /* ADC 零漂检测失败 */
#define SYS_FAULT_SELFTEST      (0x0002U)   /* 系统自检不通过 */
#define SYS_FAULT_INV           (0x0004U)   /* 逆变侧故障 */
#define SYS_FAULT_PV            (0x0008U)   /* PV 侧故障（预留） */
#define SYS_FAULT_BUS           (0x0010U)   /* 母线/平衡电路故障（预留） */
#define SYS_FAULT_ATS           (0x0020U)   /* ATS 通讯/动作异常 */
#define SYS_FAULT_ARM           (0x0040U)   /* ARM 通讯异常（预留） */

/******************************************************************************
 * 时间参数（1ms 任务，单位 ms）
 ******************************************************************************/
#define SYS_SM_INIT_MS          (100U)
#define SYS_SM_SELFTEST_MS      (300U)
#define SYS_SM_STARTUP_MS       (5000U)
#define SYS_SM_STOPPING_MS      (3000U)
#define SYS_SM_RECOVER_MS       (500U)

/******************************************************************************
 * 结构
 ******************************************************************************/
typedef struct
{
    volatile Uint16 u16State;       /* 当前状态，见 E_SYS_STATE */
    volatile Uint16 u16Fault;       /* 故障位，见 SYS_FAULT_* */
    volatile Uint16 u16StartCmd;    /* 开机命令（Modbus 写，1=开机） */
    volatile Uint16 u16ResetCmd;    /* 复位命令（Modbus 写，1=请求复位） */
    volatile Uint16 u16InvState;    /* 逆变状态机状态镜像 */
    volatile Uint16 u16InvFault;    /* 逆变故障位镜像 */
    volatile Uint32 u32RunMs;       /* 本次运行时长 */
    Uint16 u16Cnt;                  /* 状态内计时（ms） */
    Uint16 u16LastState;
} ST_SYS_SM;

extern ST_SYS_SM gSysSm;

/******************************************************************************
 * API
 ******************************************************************************/
void SysSmInit(void);
void SysSmTask(void *pArgV);        /* 1kHz（调度 Time=10） */
Uint16 SysSmGetState(void);
Uint16 SysSmGetFault(void);
Uint16 SysSmIsRun(void);            /* 1 = 系统处于运行态 */

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STATE_MACHINE_H */

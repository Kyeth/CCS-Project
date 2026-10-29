/**
 * @file Hw_PwmDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS ePWM驱动头文件
 * @version 0.4
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef HW_PWM_DRIVER_H
#define HW_PWM_DRIVER_H

#include "F2837xS_device.h"
#include "F2837xS_EPwm_defines.h"

#ifdef __cplusplus
extern "C" {
#endif


/* 同步输出源定义 (EPWMSYNCOUTEN) */
#define HW_EPWM_SYNC_OUT_FROM_DISABLE            0x00U    /* 禁用同步输出 */
#define HW_EPWM_SYNC_OUT_FROM_SOFTWARE           0x01U
#define HW_EPWM_SYNC_OUT_FROM_CNTR_ZERO          0x02U
#define HW_EPWM_SYNC_OUT_FROM_CNTR_COMPARE_B     0x04U
#define HW_EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C     0x08U
#define HW_EPWM_SYNC_OUT_FROM_CNTR_COMPARE_D     0x10U
#define HW_EPWM_SYNC_OUT_FROM_DCA_EVT1_SYNC      0x20U
#define HW_EPWM_SYNC_OUT_FROM_DCB_EVT1_SYNC      0x40U
#define HW_EPWM_SYNC_OUT_FROM_ALL                0x7FU

/* 同步输入源定义 (EPWMSYNCINSEL) */
#define HW_EPWM_SYNC_IN_FROM_DISABLE             0x00U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1       0x01U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM2       0x02U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM3       0x03U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM4       0x04U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM5       0x05U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM6       0x06U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7       0x07U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM8       0x08U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM9       0x09U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM10      0x0AU
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM11      0x0BU
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM12      0x0CU
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_ECAP1       0x11U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_ECAP2       0x12U
#define HW_EPWM_SYNC_IN_FROM_SYNCOUT_ECAP3       0x13U
#define HW_EPWM_SYNC_IN_FROM_INPUTXBAR_OUT5      0x18U
#define HW_EPWM_SYNC_IN_FROM_INPUTXBAR_OUT6      0x19U

/* F2837xS的同步由TBCTL.SYNCOSEL和SyncSocRegs.SYNCSELECT控制。
 * 本代EPWM_REGS偏移+3/+6是保留地址，不可写入。 */

typedef enum
{
    HW_PWM_TYPE_INV = 0U,
    HW_PWM_TYPE_PV,
    HW_PWM_TYPE_BUS,
    HW_PWM_TYPE_FAN
} E_HW_PWM_TYPE;

typedef enum
{
    HW_PWM_MASTER = 0U,    /* 同步主源 (发出SYNCO) */
    HW_PWM_SLAVE,          /* 同步从机 (接收SYNCI) */
    HW_PWM_FREE            /* 自由运行 (忽略同步) */
} E_HW_PWM_SYNC_MODE;

typedef enum
{
    HW_PWM_ID0 = 0U,
    HW_PWM_ID1,
    HW_PWM_ID2,
    HW_PWM_ID3,
    HW_PWM_ID4,
    HW_PWM_ID5,
    HW_PWM_ID6,
    HW_PWM_ID7,
    HW_PWM_ID8,
    HW_PWM_ID9,
    HW_PWM_ID10,
    HW_PWM_ID11,
    HW_PWM_ID_MAX,
    HW_PWM_ID_NULL = 0xFFFFU
} E_HW_PWM_ID;

#define HW_PWM_FORCE_NONE                         0U
#define HW_PWM_FORCE_LOW                          1U
#define HW_PWM_FORCE_HIGH                         2U

#define HW_PWM_TZ_HIGH_Z                          0U
#define HW_PWM_TZ_FORCE_HIGH                      1U
#define HW_PWM_TZ_FORCE_LOW                       2U
#define HW_PWM_TZ_DO_NOTHING                      3U

typedef struct
{
    Uint16 u16Zro;
    Uint16 u16Prd;
    Uint16 u16Cau;
    Uint16 u16Cad;
    Uint16 u16Cbu;
    Uint16 u16Cbd;
} ST_PWM_AQ_CFG;

typedef struct
{
    Uint16 bEnable;
    Uint16 u16Sel;
    Uint16 bUsePrd2;
    Uint16 u16Prd;
    Uint16 u16Prd2;

    /* SOCB 事件：用于第二采样点（例如 CTR=PRD 采逆变电流峰值） */
    Uint16 bEnableB;        /* 1: 使能 SOCB */
    Uint16 u16SelB;         /* 见 E_HW_PWM_ET_SEL */
    Uint16 bUsePrd2B;       /* 1: 用 SOCBPRD2 分频，0: 用 SOCBPRD */
    Uint16 u16PrdB;
    Uint16 u16Prd2B;
} ST_PWM_ET_CFG;

typedef struct
{
    Uint16 u16PwmId;
    Uint16 u16PwmType;
    Uint16 u16Period;
    Uint16 u16CmpA;
    Uint16 u16CmpB;
    Uint16 u16Phase;        /* 相位偏移 (TBPHS) */
    Uint16 u16CtrMode;

    Uint16 u16SyncMode;     /* 见 E_HW_PWM_SYNC_MODE */
    Uint16 u16SyncCntLoadDir;
    Uint16 u16SyncInSel;    /* 见 HW_EPWM_SYNC_IN_FROM_xxx */
    Uint16 u16SyncOutSel;   /* 见 HW_EPWM_SYNC_OUT_FROM_xxx */

    Uint16 bGload;
    Uint16 u16LinkPwm;

    Uint16 u16DeadTime;
    Uint16 u16TzActionA;
    Uint16 u16TzActionB;
    Uint16 u16InitForceA;
    Uint16 u16InitForceB;

    ST_PWM_AQ_CFG stAqA;
    ST_PWM_AQ_CFG stAqB;
    ST_PWM_ET_CFG stEtSoca;
} ST_PWM_CFG_ITEM;

typedef struct
{
    ST_PWM_CFG_ITEM *pstPwmCfgItem;
    Uint16 u16ItemNum;
} ST_PWM_CFG;

/* 时钟使能：在 Hw_PwmInit 前必须调用 */
extern void Hw_EnablePwmClocks(void);

/* 初始化单个ePWM模块。 */
extern void Hw_SinglePwmInit(ST_PWM_CFG_ITEM *pstPwmCfgItem);

/* 按配置表初始化多个ePWM模块。 */
extern void Hw_PwmInit(ST_PWM_CFG *pstPwmCfg);

/* 使能所有ePWM时基时钟（执行一次性上电对齐）。 */
extern void Hw_EnableAllPwmCLKSync(void);

/* 禁止所有ePWM时基时钟。 */
extern void Hw_DisableAllPwmCLKSync(void);

/* 启动指定ePWM输出。 */
extern void Hw_PwmStart(Uint16 u16PwmId);

/* 停止指定ePWM输出。 */
extern void Hw_PwmStop(Uint16 u16PwmId);

/* 启动所有ePWM输出。 */
extern void Hw_StartAllPWM(void);

/* 停止所有ePWM输出。 */
extern void Hw_StopAllPWM(void);

/* 清除所有ePWM的Trip Zone（TZ）标志位。 */
extern void Hw_PwmClearAllTripZone(void);

/* 触发配置表中第一个启用全局加载的PWM执行一次性加载。 */
extern void Hw_TrigGlobalLoad(void);

/* 写入CMPA影子寄存器（在 TBCTL 装载点生效，LOADAMODE = CC_CTR_ZERO）。 */
extern void Hw_PwmSetCmpA(Uint16 u16PwmId, Uint16 u16CmpA);

/* 写入CMPB影子寄存器（在 TBCTL 装载点生效，LOADBMODE = CC_CTR_ZERO）。 */
extern void Hw_PwmSetCmpB(Uint16 u16PwmId, Uint16 u16CmpB);

/* 运行期强制A/B通道输出电平（AQCSFRC）。
 * 说明：死区全使能模式下 B 通道由死区模块由 A 取反得到，
 * 因此强制 CSFB 仅作为封波时的兜底。 */
extern void Hw_PwmSetForce(Uint16 u16PwmId, Uint16 u16ForceA, Uint16 u16ForceB);

/* 释放A/B通道强制，回到动作限定器 + 死区产生的波形。 */
extern void Hw_PwmReleaseForce(Uint16 u16PwmId);

/* ============ 数字比较（DC）故障源配置 ============
 * 硬件机制（F2837xS ePWM 的 TZ 子模块）：
 *   - DCAH/DCBH 各是一个“或门”，输入取自 ePWM X-BAR 的 TRIP4~TRIP12，
 *     用 DCAHTRIPSEL/DCBHTRIPSEL 的 TRIPINPUTn 位选择哪几路进来，
 *     再由 DCTRIPSEL.DCAHCOMPSEL/DCBHCOMPSEL 指定比较的是哪一路；
 *   - 比较结果按 TZDCSEL 的编码形成 4 个事件：
 *       DCAEVT1 -> 只能 OSHT（一次性封波），动作在 TZCTL.DCAEVT1，作用 EPWMxA
 *       DCAEVT2 -> 可 CBC（逐波限流），  动作在 TZCTL.DCAEVT2，作用 EPWMxA
 *       DCBEVT1 -> 只能 OSHT，           动作在 TZCTL.DCBEVT1，作用 EPWMxB
 *       DCBEVT2 -> 可 CBC，              动作在 TZCTL.DCBEVT2，作用 EPWMxB
 *     A 侧事件只作用 A 引脚、B 侧事件只作用 B 引脚，TZCTL 里没有交叉选择；
 *   - 本工程统一用 EVT2 + CBC：过流尖峰只逐波压缩占空比，不永久封波；
 *     永久封波仍由软件判故障后走 TZFRC.OST（Hw_PwmStop / Hw_InvStop）。
 *     逆变模块的 B 由死区模块从 A 取反产生，只砍 A 挡不住下管，故 A/B 两套都要配。
 */

/* TRIP 号：填 4/5/7/8/9 这类硬件 TRIPn 编号（0 表示该侧不接） */
#define HW_PWM_TRIP_NONE        (0U)
#define HW_PWM_TRIP_4           (4U)
#define HW_PWM_TRIP_5           (5U)
#define HW_PWM_TRIP_7           (7U)
#define HW_PWM_TRIP_8           (8U)
#define HW_PWM_TRIP_9           (9U)
#define HW_PWM_TRIP_10          (10U)
#define HW_PWM_TRIP_11          (11U)
#define HW_PWM_TRIP_12          (12U)

/* 一个 ePWM 模块的 DC 故障源配置 */
typedef struct
{
    Uint16 u16PwmId;
    Uint16 u16DcaTrip;      /* DCAH 的 TRIP 源，见 HW_PWM_TRIP_x，0 = A 侧不接 */
    Uint16 u16DcbTrip;      /* DCBH 的 TRIP 源，0 = B 侧不接（PV 模块只有 A 通道） */
    Uint16 u16DcaEvent;     /* TZDCSEL.DCAEVT2：TZ_DCAH_LOW / TZ_DCAH_HI，0 = 禁用 */
    Uint16 u16DcbEvent;     /* TZDCSEL.DCBEVT2：TZ_DCBH_LOW / TZ_DCBH_HI，0 = 禁用 */
    Uint16 u16DcaAction;    /* TZCTL.DCAEVT2：HW_PWM_TZ_FORCE_LOW 等，见 HW_PWM_TZ_xxx */
    Uint16 u16DcbAction;    /* TZCTL.DCBEVT2 */
} ST_PWM_TZ_CFG_ITEM;

typedef struct
{
    const ST_PWM_TZ_CFG_ITEM *pstPwmTzCfgItem;
    Uint16 u16ItemNum;
} ST_PWM_TZ_CFG;

/* 按配置表把 ePWM X-BAR 的 TRIP 接进各模块的 DCAH/DCBH，并配置 CBC 事件与动作。
 * 必须在 Hw_XbarInit 之后调用（TRIP 得先有源）。 */
extern void Hw_PwmInitTripZone(const ST_PWM_TZ_CFG *pstPwmTzCfg);

#ifdef __cplusplus
}
#endif

#endif /* HW_PWM_DRIVER_H */

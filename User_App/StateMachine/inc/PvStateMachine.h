/**
 * @file PvStateMachine.h
 * @brief PV Boost 通道状态机 + 电压环/电流环 + 扰动观察 MPPT（移植自 EP5K）
 *
 * 控制结构（对应 EP5K app/src/PVStateMachine1.c 的 PVControlModular）：
 *
 *   MpptFunc（扰动观察，输出 PV 电压指令）
 *        └→ 电压指令斜坡（600 V/s）
 *              └→ 电压环 PI：输入 PV 实测电压与电压指令，输出**电流指令**（A）
 *                    └→ 电流环 PI：输入电流指令与实测电流，输出**占空比小数**
 *                          └→ Duty = (电流环输出 + 前馈 M) × TBPRD
 *
 * 电压环的输入极性沿用 EP5K 的约定：PI 输入取 Ref=实测电压、Fbk=电压指令，
 * 于是“实测电压高于指令” → 电流指令增大 → PV 电压被拉低，构成负反馈。
 *
 * MPPT 判据（对应 EP5K algorithm/src/MPPTFunc.c）：
 * - 扰动对象是**电压指令**，不是占空比；
 * - 每半个扰动周期（375 ms）累加 ∑(V×I)，与上一窗口的累加值比较：
 *   功率下降 → 反向并把步长乘 0.6；功率上升 → 步长加上 StepVolt（0.6 V）；
 * - 前后窗口功率差 < 1% 认为到顶，步长压到 1 V（避免原地不动）；
 * - 首次进入 MPPT 先做一次快速扰动（指令 = 实测电压 − FastStep），1.5 s 后才正常扰动。
 *
 * 与 EP5K 的差异（硬件决定）：
 * - EP5K 每路 PV 有独立电压采样；我们 PV 电压走 MUX 轮换，控制周期内只做 8 点滑动平均；
 * - 额定电流按本工程 30 A/路配置（EP5K 为 16.2 A），电压环输出上限随之放大。
 *
 * 通道映射（Broad/src/DrvCfg_EP15K.c）：PV1=EPWM7(ID6)、PV2=EPWM9(ID8)、
 * PV3=EPWM5(ID4)、PV4=EPWM6(ID5)，开关频率 20 kHz，TBPRD=5000。
 */
#ifndef PV_STATE_MACHINE_H
#define PV_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "F2837xS_Device.h"

/* ==================== 通道与硬件参数 ==================== */
#define PV_SM_CH_NUM            (4U)        /* PV1~PV4 */
#define PV_SM_TBPRD             (5000U)     /* 20kHz UP，与 DrvCfg 一致 */
#define PV_SM_CTL_HZ            (10000U)    /* 控制任务频率：10kHz（与 EP5K 相同） */
/* 电压反馈 10 拍块平均（10kHz → 1kHz），再进 8 点滑窗 ≈ 8ms 窗口 */
#define PV_SM_VOLT_DIV               (10U)

/* ==================== 输入判据（对齐 EP5K） ==================== */
#define PV_SM_VOLT_MIN          (79.0f)     /* 低于此值认为无 PV（EP5K MpptOutMin） */
#define PV_SM_VOLT_MAX          (450.0f)    /* 高于此值认为 PV 过压/无 PV（EP5K VPVmax） */
#define PV_SM_BUS_MARGIN        (20.0f)     /* 母线必须高出 PV 电压这么多才能升压 */
#define PV_SM_BUS_HEAD          (10.0f)     /* 电压指令必须比母线低这么多（EP5K BusVolt-10） */
#define PV_SM_OC_A              (18.0f)     /* PV 过流（EP5K 同值） */
#define PV_SM_REVERSE_A         (-2.0f)     /* 反灌电流门限 */

/* ==================== 电压环/电流环（EP5K 增益） ==================== */
#define PV_SM_VOLT_KP           (0.05f)     /* EP5K VoltPIF.Kp */
#define PV_SM_VOLT_KI           (0.005f)    /* EP5K VoltPIF.Ki（母线过压分支才用 0.00005） */
#define PV_SM_VOLT_KI_BUS       (0.00005f)  /* 母线过压快环积分增益（EP5K 降到 1/100） */
#define PV_SM_VOLT_UMAX         (30.0f)     /* 电压环输出上限 = 电流指令上限 A（本机 30A/路） */
#define PV_SM_IPV_MAX           (30.0f)     /* 电流环上限 A（EP5K IPVmax=16.2，本机 30A/路） */
#define PV_SM_VOLT_KP_ADAPT     (0.15f)     /* EP5K 低压大电流段的电压环 Kp */
#define PV_SM_CURR_KP_ADAPT     (0.003f)    /* EP5K 低压大电流段的电流环 Kp */
#define PV_SM_CURR_KP           (0.0009f)   /* EP5K CurrPIF.Kp */
#define PV_SM_CURR_KI           (0.04f)     /* EP5K CurrPIF.Ki */
#define PV_SM_CURR_UMAX         (0.10f)     /* 电流环输出上限（占空比小数） */
#define PV_SM_CURR_UMIN         (-0.10f)    /* 电流环输出下限 */
#define PV_SM_DUTY_MAX          (0.95f)     /* 占空比硬限幅 */
#define PV_SM_VSET_RAMP         (0.06f)     /* 电压指令斜坡 0.06V/100us = 600V/s（EP5K） */
#define PV_SM_BUS_DUTY_REF      (460.0f)    /* 母线限占空比的参考电压（EP5K VpvRef=460），本工程待按实际母线设定 */
#define PV_SM_M_SLEW            (0.01f)     /* 前馈 M 每拍最大变化（EP5K __fsat(M1, M+0.01, M-0.01)） */

/* ==================== MPPT（EP5K MpptFunc 参数） ==================== */
#define PV_SM_MPPT_PERIOD       (7500U)     /* 扰动周期 = 750ms @10kHz（EP5K 30000/4） */
#define PV_SM_MPPT_HALF         (3750U)     /* 半周期，后半周期累加功率 */
#define PV_SM_MPPT_STEP_MAX     (5.0f)      /* 扰动步长上限 V */
#define PV_SM_MPPT_STEP_MIN     (1.0f)      /* 扰动步长下限 V */
#define PV_SM_MPPT_STEP_VOLT    (0.6f)      /* 功率上升时步长增量 V */
#define PV_SM_FAST_STEP         (5.0f)      /* 首次快速扰动步长 V（= 高压段 cMpptStepMax） */
#define PV_SM_DELAY_CNT         (15000U)    /* 快速扰动后 1.5s 才开始正常扰动 */
#define PV_SM_SOFT_MS           (500U)      /* 软启时间（电压指令爬升到目标） */
#define PV_SM_FAULT_RETRY_TICK  (50000U)    /* 故障后重试间隔 5s @10kHz */

/* ==================== 状态 ==================== */
typedef enum
{
    PV_ST_IDLE = 0U,    /* 无 PV / 未使能：占空比 0 */
    PV_ST_SOFT = 1U,    /* 软启：电压指令平滑爬升，环路已投入 */
    PV_ST_MPPT = 2U,    /* 扰动观察 */
    PV_ST_FAULT = 3U    /* 故障：占空比 0，等重试 */
} E_PV_SM_STATE;

#define PV_SM_FAULT_NONE        (0x0000U)
#define PV_SM_FAULT_OC          (0x0001U)   /* 过流 */
#define PV_SM_FAULT_UV          (0x0002U)   /* MPPT 中 PV 电压跌落 */
#define PV_SM_FAULT_BUS         (0x0004U)   /* 母线电压不足 */
#define PV_SM_FAULT_REVERSE     (0x0008U)   /* 反灌 */

/* 与 EP5K CNTL_PI_F_MACRO 等价的浮点 PI */
typedef struct
{
    float fRef;         /* 输入：给定 */
    float fFbk;         /* 输入：反馈 */
    float fOut;         /* 输出 */
    float fKp;          /* 比例 */
    float fKi;          /* 积分 */
    float fUmax;        /* 输出上限 */
    float fUmin;        /* 输出下限 */
    float fUp;          /* 比例项 */
    float fUi;          /* 积分项 */
    float fV1;          /* 未限幅输出 */
    float fI1;          /* 积分器存储 */
} ST_PV_PI;

/* 单通道运行上下文 */
typedef struct
{
    Uint16 u16State;        /* E_PV_SM_STATE */
    Uint16 u16Fault;        /* PV_SM_FAULT_* */
    Uint16 u16Present;      /* 1=检测到 PV 存在 */
    Uint16 u16Cnt;          /* 状态计时，100us 节拍 */
    Uint16 u16DutyPermil;   /* 占空比千分比，给点表看 */

    Uint16 u16MpptCnt;      /* 扰动周期计数（10kHz） */
    Uint16 u16DelayCnt;     /* 快速扰动后的等待计数 */
    Uint16 u16FastDone;     /* 1=已做过快速扰动 */
    Uint16 u16ErrCnt;       /* 扰动发散计数 */
    Uint16 u16BusOv;        /* 1=母线过压快环生效 */

    float  fVpv;            /* PV 电压（8 点滑动平均） */
    float  fIpv;            /* PV 电流（8 点滑动平均） */
    float  fVset;           /* MPPT 输出的电压指令 */
    float  fVsetRamp;       /* 斜坡后的电压指令 */
    float  fVsetStart;      /* 软启起始电压指令 */
    float  fIref;           /* 电压环输出 = 电流指令 */
    float  fDuty;           /* 当前占空比 0~1 */
    float  fM;              /* 前馈偏置 1-Vpv/Vbus */
    float  fPowerNew;       /* 本窗口功率累加 */
    float  fPowerOld;       /* 上一窗口功率累加 */
    float  fPower;          /* 瞬时功率 [W]，给点表看 */
    float  fPowerdelta;     /* 前后窗口功率差 */
    float  fDisturbStep;    /* 当前扰动步长 V */
    Uint16 u16DisturbDir;   /* 1=升压方向（指令+），0=降压方向（指令-） */

    ST_PV_PI stVoltPi;
    ST_PV_PI stCurrPi;
} ST_PV_SM_CH;

typedef struct
{
    ST_PV_SM_CH stCh[PV_SM_CH_NUM];
    Uint16 u16Enable;       /* 1=允许运行（由系统状态机/调试置位） */
    Uint16 u16RunMask;      /* bit0~3：各通道处于 SOFT/MPPT */
    Uint16 u16MpptMask;     /* bit0~3：各通道处于 MPPT */
    Uint16 u16PwmId[PV_SM_CH_NUM];  /* 通道 -> HW_PWM_ID */
} ST_PV_SM;

extern ST_PV_SM gPvSm;

/* ==================== 接口 ==================== */
extern void PvSmInit(void);
extern void RunPvStateMachine(void);             /* 20kHz 控制中断 ÷2 = 10kHz */
extern void PvSmSetEnable(Uint16 u16Enable);    /* 0=全部停机 */
extern Uint16 PvSmGetEnable(void);

#ifdef __cplusplus
}
#endif

#endif /* PV_STATE_MACHINE_H */

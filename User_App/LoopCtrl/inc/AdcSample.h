/**
 * @file AdcSample.h
 * @brief ADC 采样结果的两层封装：瞬时值（控制内部用）与对外值（RMS / 慢速滤波）
 * @author yetingheng (004182)
 * @version 0.1
 * @date 2026-10-26
 *
 * @copyright Copyright (c) 2026
 *
 * 为什么分两层：
 *   - 控制环（PLL、电流环、中点平衡、过流保护）吃的是**瞬时值**，任何附加的
 *     低通都会吃掉相位裕度，所以 gAdcIns 每 20 kHz 原样刷新，不做任何滤波；
 *   - 上位机 / 软示波器 / ARM 要看的是**物理量的稳定读数**，交流取一个工频周期
 *     的 RMS、直流取块平均后再一阶滞后，这就是 gAdcPub，1 kHz 刷新一次。
 *
 * 命名约定（TI 风格：量 + 相别/编号 + 属性后缀）：
 *   fGridVoltR      电网 R 相电压瞬时值 [V]
 *   fGridVoltRmsR   电网 R 相电压有效值 [V]（工频周期 RMS + 一阶滞后）
 *   fInvCurPkR      逆变 R 相电流工频周期峰值 [A]（逐周期保持 + 每周期衰减）
 *   fBusVoltSlow    母线电压慢速值 [V]（1 ms 块平均 + 一阶滞后）
 *   fPvCur1Slow     第 1 路 PV 电流慢速值 [A]
 */
#ifndef ADC_SAMPLE_H
#define ADC_SAMPLE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 刷新频率与滤波参数
 ******************************************************************************/

/* 瞬时值刷新频率（CtrlIsr，20 kHz）与对外刷新频率（调度任务，1 kHz） */
#define ADC_SAMPLE_FAST_HZ          (20000U)
#define ADC_SAMPLE_TASK_HZ          (1000U)

/* 直流量的块平均长度：20 kHz / 20 = 1 kHz，一块正好对应对外一次刷新 */
#define ADC_SAMPLE_SLOW_DIV         (ADC_SAMPLE_FAST_HZ / ADC_SAMPLE_TASK_HZ)

/* 交流 RMS 的窗口长度由锁相角 theta 过零收口（50 Hz 下 400 拍 = 20 ms）；
 * 离网或 theta 停住时用最大样本数兜底，避免窗口无限长 */
#define ADC_SAMPLE_RMS_WIN_MIN      (10U)       /* 少于 10 拍不采信（约 0.5 ms） */
#define ADC_SAMPLE_RMS_WIN_MAX      (600U)      /* 30 ms 强制收口 */

/* 一阶滞后系数：alpha = 1 / (tau * fs)，fs 取对外刷新率 1 kHz
 *   ALPHA_RMS  tau = 150 ms -> 到 99% 约 0.69 s
 *   ALPHA_BUS  tau = 100 ms -> 0.46 s（中点平衡要看得上大小扰动）
 *   ALPHA_SLOW tau = 200 ms -> 0.92 s（PV、相直流分量） */
#define ADC_SAMPLE_ALPHA_RMS        (0.00667f)
#define ADC_SAMPLE_ALPHA_BUS        (0.01000f)
#define ADC_SAMPLE_ALPHA_SLOW       (0.00500f)

/* 峰值保持：每工频周期衰减一次，再用本周期的最大值顶上去 */
#define ADC_SAMPLE_PK_DECAY         (0.90f)

/* gAdcPub.u16Valid 位定义 */
#define ADC_SAMPLE_VALID_RMS        (0x0001U)   /* 至少收口过一个 RMS 周期 */
#define ADC_SAMPLE_VALID_ZERO       (0x0002U)   /* 上电零漂检测已完成 */
#define ADC_SAMPLE_VALID_PLL        (0x0004U)   /* 锁相已锁定（PLL 改造后置位） */

/******************************************************************************
 * 瞬时值层：20 kHz 刷新，控制环专用，禁止加滤波
 ******************************************************************************/
typedef struct
{
    /* ---- 三相交流量（有符号）---- */
    float fGridVoltR;           /* 电网电压 [V] */
    float fGridVoltS;
    float fGridVoltT;
    float fInvVoltR;            /* 逆变电压 [V] */
    float fInvVoltS;
    float fInvVoltT;
    float fInvCurR;             /* 逆变电流 [A] */
    float fInvCurS;
    float fInvCurT;

    /* ---- 直流量 ---- */
    float fDcCurR;              /* 相直流分量 [A]（硬件 B0/B1/B2 专用通道） */
    float fDcCurS;
    float fDcCurT;
    float fBalCur;              /* 平衡电路电流 [A]（D4） */
    float fGfciCur;             /* GFCI 漏电流 [A]（D1） */
    float fBusVoltL;            /* 上/下半母线 [V] */
    float fBusVoltH;
    float fBusVolt;             /* 母线电压 = L + H [V] */
    float fPvCur1;              /* PV 电流 [A] */
    float fPvCur2;
    float fPvCur3;
    float fPvCur4;
    float fPvVolt1;             /* PV 电压 [V] */
    float fPvVolt2;
    float fPvVolt3;
    float fPvVolt4;

    /* ---- 多路选择器（由上层按 u16MuxAddr 解释是哪一路）---- */
    float fMuxVoltB3;
    float fMuxVoltD3;
} ST_ADC_SAMPLE_INS;

/******************************************************************************
 * 对外层：1 kHz 刷新，上位机 / 软示波器 / ARM 专用
 ******************************************************************************/
typedef struct
{
    /* ---- 交流有效值：工频周期 RMS + 一阶滞后 ---- */
    float fGridVoltRmsR;        /* 电网电压有效值 [V] */
    float fGridVoltRmsS;
    float fGridVoltRmsT;
    float fInvVoltRmsR;         /* 逆变电压有效值 [V] */
    float fInvVoltRmsS;
    float fInvVoltRmsT;
    float fInvCurRmsR;          /* 逆变电流有效值 [A] */
    float fInvCurRmsS;
    float fInvCurRmsT;
    float fBalCurRms;           /* 平衡电路电流有效值 [A] */
    float fGfciRms;             /* GFCI 漏电流有效值 [A] */

    /* ---- 交流峰值：逐工频周期保持 ---- */
    float fInvCurPkR;           /* 逆变电流峰值 [A] */
    float fInvCurPkS;
    float fInvCurPkT;

    /* ---- 直流量：块平均 + 一阶滞后 ---- */
    float fBusVoltSlow;         /* 母线电压 [V] */
    float fBusVoltLSlow;
    float fBusVoltHSlow;
    float fDcCurSlowR;          /* 相直流分量 [A] */
    float fDcCurSlowS;
    float fDcCurSlowT;
    float fBalCurSlow;          /* 平衡电路电流 [A] */
    float fPvCur1Slow;          /* PV 电流 [A] */
    float fPvCur2Slow;
    float fPvCur3Slow;
    float fPvCur4Slow;
    float fPvVolt1Slow;         /* PV 电压 [V] */
    float fPvVolt2Slow;
    float fPvVolt3Slow;
    float fPvVolt4Slow;

    /* ---- 派生量 / 状态 ---- */
    float  fGridFreq;           /* 电网频率 [Hz]（取自 PLL） */
    Uint16 u16Valid;            /* ADC_SAMPLE_VALID_xxx 位掩码 */
} ST_ADC_SAMPLE_PUB;

/******************************************************************************
 * 全局实例
 ******************************************************************************/
extern volatile ST_ADC_SAMPLE_INS gAdcIns;  /* 20 kHz 刷新，ISR 写、后台读 */
extern ST_ADC_SAMPLE_PUB gAdcPub;   /* 1 kHz 刷新 */

/******************************************************************************
 * 接口
 ******************************************************************************/
/* 初始化：清零两层数据与内部累加器，放在 CtrlIsrInit 之前 */
extern void AdcSampleInit(void);

/* 20 kHz：在 CtrlIsrRun 末尾调用（此时本拍的 AdcCalib 与 theta 都已更新） */
extern void AdcSampleFast(void);

/* 1 kHz 任务：块平均 + 一阶滞后 + 峰值保持，由调度器按 10 个节拍调用 */
extern void AdcSampleTask(void *pArgV);

#ifdef __cplusplus
}
#endif

#endif /* ADC_SAMPLE_H */

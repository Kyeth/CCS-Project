/**
 * @file SampleData.h
 * @author yetingheng (004182)
 * @brief 统一采样数据结构（五层）
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 *
 * L1 ST_SAMPLE_RAW     : 20 kHz 一次的 ADC 原始码值，按测点名归档
 * L2 ST_SAMPLE_INS     : (code - zero) * K，工程值 V / A，与 L1 逐字段对应
 * L3 ST_SAMPLE_FLT     : 块平均（Avg8 400 us / AvgHalf 半周期 / AvgMux 16 轮）
 * L4 ST_SAMPLE_AC      : 交流周期值（RMS / 峰值），窗口由 stPLLData.ThetaR 收口
 * L5 ST_SAMPLE_DISPLAY : 强滤波显示值，1 kHz 任务更新
 *
 * 命名规则：<类型前缀><物理量><单位或角色>
 *   a开头 = 数组，u16/u32 = 码值与计数器，f = float 工程值。
 *
 * 本文件只放数据形状与实例的外部声明，实例集中在 SampleData.c 定义，
 * 避免任何模块自己开一份局部副本。
 *
 * 层次与用途：
 *   控制环取 L2 瞬时值或 L3 的 Avg8；AvgHalf 用于有功与直流分量；
 *   MUX 通道（母线 / PV / VISO 电压）刷新率 5.6 ms，只用于监测、保护与显示。
 */

#ifndef SAMPLE_DATA_H
#define SAMPLE_DATA_H

#include "F2837xS_device.h"

/* 相序索引约定：R = 0, S = 1, T = 2 */
#define SAMPLE_PH_NUM       (3U)
/* PV 支路数：PV1 = 0 … PV4 = 3 */
#define SAMPLE_PV_NUM       (4U)

/* 多路选择器地址线取值（A2..A0 = GPIO42/47/46 的组合值）。
 * 环为 0→1→2→3→4→6→7→0，地址 5 不使用。
 * B3（MUX2）只在地址 0~4 有意义，D3（MUX1）只在地址 6、7 有意义。 */
#define SAMPLE_MUX2_ADDR_PV1    (0U)    /* 0~3 = PV1~PV4 电压 */
#define SAMPLE_MUX2_ADDR_VISO   (4U)
#define SAMPLE_MUX1_ADDR_BUSL   (6U)    /* 下半母线 */
#define SAMPLE_MUX1_ADDR_BUSH   (7U)    /* 上半母线 */

/* ---------- 频率模式（m00702 / m00721） ---------- */

#define SAMPLE_FREQ_50HZ        (50.0f)
#define SAMPLE_FREQ_60HZ        (60.0f)

/*! 额定频率模式。默认 50 Hz，可被上层切换到 60 Hz。 */
typedef enum
{
    SAMPLE_FREQ_MODE_50HZ = 0U,
    SAMPLE_FREQ_MODE_60HZ = 1U
} E_SAMPLE_FREQ_MODE;

/*! 运行时采样配置：额定频率与两个派生长度。
 *  切换频率模式时三个字段一起刷新，当前正在累加的块不打断。 */
typedef struct
{
    float  fNominalFreq;    /* 50.0f / 60.0f，窗口上限、半周期块长度都用它 */
    Uint16 u16FreqMode;     /* E_SAMPLE_FREQ_MODE */
    Uint16 u16HalfSteps;    /* 半周期块长度：50 Hz → 200，60 Hz → 167 */
} ST_SAMPLE_CFG;

extern volatile ST_SAMPLE_CFG stSampleCfg;

/* ---------- L1 ---------- */

/*! L1：原始码值。uint16 直接对应 ADC 结果寄存器，范围 0~4095。 */
typedef struct
{
    Uint16 u16InvCurrCode[SAMPLE_PH_NUM];    /* 逆变电流，D14/D2/A2 → R/S/T */
    Uint16 u16InvVoltCode[SAMPLE_PH_NUM];    /* 逆变电压，C4/C3/A0 */
    Uint16 u16GridVoltCode[SAMPLE_PH_NUM];   /* 电网电压，A5/A1/A3 */
    Uint16 u16InvDcCode[SAMPLE_PH_NUM];      /* 逆变直流分量，B0/B1/B2 */
    Uint16 u16PvCurrCode[SAMPLE_PV_NUM];     /* PV 电流，A4/C2/D0/D15 */
    Uint16 u16BalCurrCode;                   /* 平衡电路电流，D4 */
    Uint16 u16GfciCurrCode;                  /* GFCI 漏电流，D1 */
    Uint16 u16PvVoltCode[SAMPLE_PV_NUM];     /* MUX2 地址 0~3：PV1~PV4 电压 */
    Uint16 u16VisoCode;                      /* MUX2 地址 4：VISO 电压 */
    Uint16 u16BusLCode;                      /* MUX1 地址 6：下半母线 */
    Uint16 u16BusHCode;                      /* MUX1 地址 7：上半母线 */
    Uint16 u16MuxAddr;                       /* 本拍地址线值，正常为 0~4 / 6 / 7 */
    Uint32 u32Seq;                           /* 每次 ISR 加一，兼作丢拍检查 */
} ST_SAMPLE_RAW;

/* ---------- L2 ---------- */

/*! L2：瞬时工程值，单位 V / A。字段与 L1 一一对应。 */
typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    float fInvVolt[SAMPLE_PH_NUM];
    float fGridVolt[SAMPLE_PH_NUM];
    float fInvDc[SAMPLE_PH_NUM];
    float fPvCurr[SAMPLE_PV_NUM];
    float fBalCurr;
    float fGfciCurr;
    float fPvVolt[SAMPLE_PV_NUM];
    float fVisoVolt;
    float fBusLVolt;
    float fBusHVolt;
} ST_SAMPLE_INS;

/*! 上电零漂结果，字段与 L2 一一对应，外加状态字。 */
typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    float fInvVolt[SAMPLE_PH_NUM];
    float fGridVolt[SAMPLE_PH_NUM];
    float fInvDc[SAMPLE_PH_NUM];
    float fPvCurr[SAMPLE_PV_NUM];
    float fBalCurr;
    float fGfciCurr;
    float fPvVolt[SAMPLE_PV_NUM];   /* 多路选择器不做零漂，恒为 0 */
    float fVisoVolt;
    float fBusLVolt;
    float fBusHVolt;
    Uint16 u16State;                /* E_ADC_CAL_STATE */
    Uint16 u16Valid;                /* 1 = 零点有效（TRACK 完成），0 = 仍是标称值或已判故障 */
} ST_SAMPLE_ZERO;

/* ---------- L3 ---------- */

#define SAMPLE_AVG8_STEPS       (8U)    /* Avg8 块长度，400 us @ 20 kHz */
#define SAMPLE_AVG_MUX_STEPS    (16U)   /* AvgMux 轮数，16 × 350 us ≈ 5.6 ms */

/*! Avg8 累加器：只装 400 us 块用到的通道。 */
typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    float fInvVolt[SAMPLE_PH_NUM];
    float fGridVolt[SAMPLE_PH_NUM];
    float fInvPwr[SAMPLE_PH_NUM];
    float fInvPwrTotal;
} ST_SAMPLE_AVG8_ACC;

/*! AvgHalf 累加器：半周期块，覆盖除 MUX 以外的全部通道。 */
typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    float fInvVolt[SAMPLE_PH_NUM];
    float fGridVolt[SAMPLE_PH_NUM];
    float fInvDc[SAMPLE_PH_NUM];
    float fInvPwr[SAMPLE_PH_NUM];
    float fInvPwrTotal;
    float fPvCurr[SAMPLE_PV_NUM];
    float fBalCurr;
    float fGfciCurr;
} ST_SAMPLE_HALF_ACC;

/*! MUX 累加器：每个多路选择器地址各累加一路，轮数由 u16CntMux 记。 */
typedef struct
{
    float fPvVolt[SAMPLE_PV_NUM];
    float fVisoVolt;
    float fBusLVolt;
    float fBusHVolt;
} ST_SAMPLE_MUX_ACC;

/*! L3：块平均发布值 + 累加器 + 计数器。累加器放在结构里，调试时能直接
 *  看到“半个块的当前值”。 */
typedef struct
{
    /* Avg8：400 us，控制环“当前值” */
    float fInvCurrAvg8[SAMPLE_PH_NUM];
    float fInvVoltAvg8[SAMPLE_PH_NUM];
    float fGridVoltAvg8[SAMPLE_PH_NUM];
    float fInvPwrAvg8[SAMPLE_PH_NUM];
    float fInvPwrTotalAvg8;

    /* AvgHalf：半个工频周期，长度随额定频率自适应 */
    float fInvCurrAvgHalf[SAMPLE_PH_NUM];
    float fInvVoltAvgHalf[SAMPLE_PH_NUM];
    float fGridVoltAvgHalf[SAMPLE_PH_NUM];
    float fInvDcAvgHalf[SAMPLE_PH_NUM];
    float fPvCurrAvgHalf[SAMPLE_PV_NUM];
    float fBalCurrAvgHalf;
    float fGfciCurrAvgHalf;
    float fInvPwrAvgHalf[SAMPLE_PH_NUM];
    float fInvPwrTotalAvgHalf;

    /* AvgMux：16 轮 ≈ 5.6 ms，MUX 通道专用 */
    float fPvVoltAvgMux[SAMPLE_PV_NUM];
    float fVisoVoltAvgMux;
    float fBusLAvgMux;
    float fBusHAvgMux;

    ST_SAMPLE_AVG8_ACC stAcc8;
    ST_SAMPLE_HALF_ACC stAccHalf;
    ST_SAMPLE_MUX_ACC  stMuxAcc;

    Uint16 u16Cnt8;                 /* 0 ~ SAMPLE_AVG8_STEPS-1 */
    Uint16 u16CntHalf;              /* 0 ~ u16HalfSteps-1 */
    Uint16 u16CntMux;               /* 0 ~ SAMPLE_AVG_MUX_STEPS-1 */
    Uint16 reserved;
} ST_SAMPLE_FLT;

/* ---------- L4 ---------- */

#define SAMPLE_AC_VALID_RMS     (0x0001U)
#define SAMPLE_AC_VALID_PEAK    (0x0002U)
#define SAMPLE_AC_VALID_THETA   (0x0004U)   /* 由角度回绕收口 */
#define SAMPLE_AC_VALID_FORCED  (0x0008U)   /* 靠样本上限强制收口，值仅供参考 */

/*! L4 累加器：平方和与窗口内绝对值峰值。 */
typedef struct
{
    float fInvCurrSq[SAMPLE_PH_NUM];
    float fInvVoltSq[SAMPLE_PH_NUM];
    float fGridVoltSq[SAMPLE_PH_NUM];
    float fInvCurrPk[SAMPLE_PH_NUM];
    float fInvVoltPk[SAMPLE_PH_NUM];
    float fGridVoltPk[SAMPLE_PH_NUM];
} ST_SAMPLE_AC_ACC;

/*! L4：交流周期值。窗口边界由 stPLLData.ThetaR 回绕给出。 */
typedef struct
{
    float fInvCurrRms[SAMPLE_PH_NUM];    /* A */
    float fInvVoltRms[SAMPLE_PH_NUM];    /* V */
    float fGridVoltRms[SAMPLE_PH_NUM];   /* V */
    float fInvCurrPk[SAMPLE_PH_NUM];     /* A，窗口内 |x| 最大值 */
    float fInvVoltPk[SAMPLE_PH_NUM];
    float fGridVoltPk[SAMPLE_PH_NUM];

    ST_SAMPLE_AC_ACC stAcc;

    Uint16 u16WinSamples;                /* 本窗口实际样本数（= 每周期样本数） */
    Uint16 u16WinCnt;                    /* 当前窗口已累加样本数 */
    Uint16 u16WinMaxSamples;             /* 强制收口上限，随频率模式刷新 */
    Uint16 u16Valid;                     /* SAMPLE_AC_VALID_* 位组合 */
} ST_SAMPLE_AC;

/* ---------- L5 ---------- */

/*! L5：显示值。1 kHz 任务里对 L3 / L4 做一级强滤波，专供人看。 */
typedef struct
{
    float fInvCurrRmsDisplay[SAMPLE_PH_NUM];
    float fInvVoltRmsDisplay[SAMPLE_PH_NUM];
    float fGridVoltRmsDisplay[SAMPLE_PH_NUM];
    float fPvVoltDisplay[SAMPLE_PV_NUM];
    float fVisoVoltDisplay;
    float fBusLDisplay;
    float fBusHDisplay;
    float fPvCurrDisplay[SAMPLE_PV_NUM];
    float fBalCurrDisplay;
    float fGfciCurrDisplay;
    Uint16 u16Cnt;                      /* 已执行过的显示刷新次数 */
    Uint16 bFirst;                      /* 1 = 已完成首拍直接赋值 */
} ST_SAMPLE_DISPLAY;

extern ST_SAMPLE_RAW     stSampleRaw;
extern ST_SAMPLE_INS     stSampleIns;
extern ST_SAMPLE_ZERO    stSampleZero;
extern ST_SAMPLE_FLT     stSampleFlt;
extern ST_SAMPLE_AC      stSampleAc;
extern ST_SAMPLE_DISPLAY stSampleDisplay;

/**
 * @brief 清零 L1~L5、stPLLData，并按 Broad 层默认频率初始化 stSampleCfg。
 *        必须在 ADC 中断使能之前调用。
 */
void SampleDataInit(void);

/**
 * @brief 切换 50 / 60 Hz 频率模式。非法入参按 50 Hz 处理。
 */
void SampleSetFreqMode(Uint16 u16Mode);

/**
 * @brief 写入任意额定频率（45 ~ 65 Hz），越界钳位。
 * @return 1 = 入参在范围内并已生效；0 = 被钳位后生效
 */
Uint16 SampleSetNominalFreq(float fNominalFreq);

#endif /* SAMPLE_DATA_H */

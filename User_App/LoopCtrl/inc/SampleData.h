/**
 * @file SampleData.h
 * @brief 采样数据统一结构体：码值 / 瞬时 / 统计 / 有效值 / 状态
 * @author yetingheng (004182)
 * @version 0.2
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 * 命名规则：f<源><相或序号><量><角色>
 *   源   Inv / Grid / Bus / Pv / Bal / Gfci / Mux
 *   量   Volt / Curr / Ofs / Pwr / Freq
 *   角色 Raw / Ins / Avg8 / Avg200 / Rms / Pk
 * 三相量一律 float[3] + E_SAMPLE_PHASE，PV 一律 float[4] + E_SAMPLE_PV。
 * 单位一律 V / A / Hz / W，协议里的 0.1V 缩放放在 DataExchange 层。
 */
#ifndef SAMPLE_DATA_H
#define SAMPLE_DATA_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 相位索引：所有三相量的数组下标，顺序与 RST 一致 */
typedef enum
{
    SAMPLE_PH_R = 0,
    SAMPLE_PH_S = 1,
    SAMPLE_PH_T = 2,
    SAMPLE_PH_NUM = 3
} E_SAMPLE_PHASE;

/* PV 路号索引：所有 PV 数组的下标 */
typedef enum
{
    SAMPLE_PV1 = 0,
    SAMPLE_PV2 = 1,
    SAMPLE_PV3 = 2,
    SAMPLE_PV4 = 3,
    SAMPLE_PV_NUM = 4
} E_SAMPLE_PV;

/* 统计窗口长度：Avg8 = 8 拍 400us（2.5kHz），Avg200 = 200 拍 10ms（100Hz） */
#define SAMPLE_AVG8_DIV             (8U)
#define SAMPLE_AVG200_DIV           (200U)

/* stSampleStatus.u16Valid 位定义 */
#define SAMPLE_VALID_RMS            (0x0001U)   // 至少收口过一个工频周期
#define SAMPLE_VALID_ZERO           (0x0002U)   // 上电零漂跟踪已完成
#define SAMPLE_VALID_PLL            (0x0004U)   // 锁相已锁定

/******************************************************************************
 * 码值层：CLA 直出，20 kHz，不做任何处理
 ******************************************************************************/
typedef struct
{
    float fInvCurrRaw[SAMPLE_PH_NUM];       // 逆变电流码值
    float fInvCurrPkRaw[SAMPLE_PH_NUM];     // CTR=PRD 第二采码值，逐波限流回读
    float fInvVoltRaw[SAMPLE_PH_NUM];       // 逆变电压码值
    float fGridVoltRaw[SAMPLE_PH_NUM];      // 电网电压码值
    float fInvDcOfsRaw[SAMPLE_PH_NUM];      // 相电流直流分量码值
    float fBalCurrRaw;                      // 平衡桥电流码值
    float fGfciCurrRaw;                     // GFCI 漏电流码值
    float fPvCurrRaw[SAMPLE_PV_NUM];        // PV 电流码值
    float fMuxB3Raw;                        // MUX2 原始值（正母线）
    float fMuxD3Raw;                        // MUX1 原始值（PV1-4 / 负母线）
    Uint16 u16MuxAddr;                      // 本轮 MUX 地址
    Uint16 u16Seq;                          // CLA 心跳序号
} ST_SAMPLE_RAW;

/******************************************************************************
 * 瞬时层：物理量，20 kHz，控制环专用，禁止再加滤波
 ******************************************************************************/
typedef struct
{
    float fInvCurrIns[SAMPLE_PH_NUM];       // 逆变电流 [A]（ZERO/PRD 两点均值）
    float fInvVoltIns[SAMPLE_PH_NUM];       // 逆变电压 [V]
    float fGridVoltIns[SAMPLE_PH_NUM];      // 电网电压 [V]
    float fInvDcOfsIns[SAMPLE_PH_NUM];      // 相电流直流分量 [A]
    float fBalCurrIns;                      // 平衡桥电流 [A]
    float fGfciCurrIns;                     // GFCI 漏电流 [A]
    float fPvCurrIns[SAMPLE_PV_NUM];        // PV 电流 [A]
    float fPvVoltIns[SAMPLE_PV_NUM];        // PV 电压 [V]，MUX 轮询 3.33 kHz
    float fBusPosVoltIns;                   // 正母线 [V]，MUX 轮询 3.33 kHz
    float fBusNegVoltIns;                   // 负母线 [V]，MUX 轮询 3.33 kHz
    float fBusVoltIns;                      // 总母线 = 正 + 负 [V]
} ST_SAMPLE_INS;

/******************************************************************************
 * 统计层：块平均，2.5 kHz / 100 Hz 两档
 ******************************************************************************/
typedef struct
{
    float fPvVoltAvg8[SAMPLE_PV_NUM];       // PV 电压 2.5 kHz 平均值 [V]
    float fPvCurrAvg8[SAMPLE_PV_NUM];       // PV 电流 2.5 kHz 平均值 [A]
    float fBusPosVoltAvg8;                  // 正母线 2.5 kHz 平均值 [V]
    float fBusNegVoltAvg8;                  // 负母线 2.5 kHz 平均值 [V]
    float fBusVoltAvg8;                     // 总母线 2.5 kHz 平均值 [V]
    float fInvPwrAvg8[SAMPLE_PH_NUM];       // 逆变分相有功 2.5 kHz 平均值 [W]

    float fBusPosVoltAvg200;                // 正母线 100 Hz 平均值 [V]
    float fBusNegVoltAvg200;                // 负母线 100 Hz 平均值 [V]
    float fBusVoltAvg200;                   // 总母线 100 Hz 平均值 [V]
    float fInvDcOfsAvg200[SAMPLE_PH_NUM];   // 相电流直流分量 100 Hz 平均值 [A]
    float fBalCurrAvg200;                   // 平衡桥电流 100 Hz 平均值 [A]
    float fInvPwrAvg200[SAMPLE_PH_NUM];     // 逆变分相有功 100 Hz 平均值 [W]
    float fInvPwrTotal;                     // 三相总有功 = 三相加和 [W]
} ST_SAMPLE_AVG;

/******************************************************************************
 * 有效值层：工频周期 RMS 与峰值
 ******************************************************************************/
typedef struct
{
    float fInvCurrRms[SAMPLE_PH_NUM];       // 逆变电流有效值 [A]
    float fInvVoltRms[SAMPLE_PH_NUM];       // 逆变电压有效值 [V]
    float fGridVoltRms[SAMPLE_PH_NUM];      // 电网电压有效值 [V]
    float fBalCurrRms;                      // 平衡桥电流有效值 [A]
    float fGfciCurrRms;                     // GFCI 漏电流有效值 [A]
    float fInvCurrPk[SAMPLE_PH_NUM];        // 逆变电流峰值 [A]，仅供显示
} ST_SAMPLE_RMS;

/******************************************************************************
 * 状态层：派生量与标志
 ******************************************************************************/
typedef struct
{
    float  fGridFreq;                       // 电网频率 [Hz]，取自 PLL
    Uint16 u16Valid;                        // SAMPLE_VALID_xxx 位掩码
} ST_SAMPLE_STATUS;

/******************************************************************************
 * 全局实例
 ******************************************************************************/
extern volatile ST_SAMPLE_RAW stSampleRaw;      // 20 kHz 中断写
extern volatile ST_SAMPLE_INS stSampleIns;      // 20 kHz 中断写，后台读
extern ST_SAMPLE_AVG  stSampleAvg;             // 20 kHz 累加，收口时写
extern ST_SAMPLE_RMS  stSampleRms;             // 工频周期收口，1 kHz 任务滤波
extern ST_SAMPLE_STATUS stSampleStatus;        // 1 kHz 任务写

/* 清零全部实例，放在 AdcCalib / CtrlIsr 初始化之前 */
extern void SampleDataInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLE_DATA_H */

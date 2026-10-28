/******************************************************************************
 * @file    AdcCalib.h
 * @author  yetingheng (004182)
 * @brief   ADC 采样标定层：码值 -> 工程值 + 上电零漂检测
 * @version 1.0
 * @date    2026-11-05
 *
 * @note    数据流：
 *          ADC（12 位，0~4095）→ CLA（滤波、多点选择器归档，输出码值）
 *          → 本模块（减零漂、乘系数）→ 控制环 / 保护 / 软示波器 / Modbus
 *
 * @note    量程依据《ADC采样标定》表（15KW 机型）：
 *          | 引脚 | 物理量        | 量程 / 零点                          |
 *          | A0   | T 相逆变电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | A1   | S 相电网电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | A2   | T 相逆变电流  | ±100A ↔ 0~3V，0A ↔ 1.5V              |
 *          | A3   | T 相电网电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | A4   | PV1 电流      | 30A ↔ 3V，0A ↔ 5/3V                  |
 *          | A5   | R 相电网电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | B0   | R 相直流分量  | ±2.95A ↔ 0~3V，0A ↔ 1.5V             |
 *          | B1   | S 相直流分量  | ±2.95A ↔ 0~3V，0A ↔ 1.5V             |
 *          | B2   | T 相直流分量  | ±2.95A ↔ 0~3V，0A ↔ 1.5V             |
 *          | B3   | 多路选择器2   | 1033V ↔ 3V（PV 电压）/ 488V ↔ 3V（VISO）|
 *          | C2   | PV2 电流      | 30A ↔ 3V，0A ↔ 5/3V                  |
 *          | C3   | S 相逆变电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | C4   | R 相逆变电压  | ±462.69V ↔ 0~3V，0V ↔ 1.5V           |
 *          | D0   | PV3 电流      | 30A ↔ 3V，0A ↔ 5/3V                  |
 *          | D1   | GFCI 漏电流   | 500mA ↔ 3V，0mA ↔ 1.5V（系数待考证）  |
 *          | D2   | S 相逆变电流  | ±100A ↔ 0~3V，0A ↔ 1.5V              |
 *          | D3   | 多路选择器1   | 563.64V ↔ 3V（上/下半母线）           |
 *          | D4   | 平衡电路电流  | ±36.4A ↔ 0~3V，0A ↔ 1.5V             |
 *          | D14  | R 相逆变电流  | ±100A ↔ 0~3V，0A ↔ 1.5V              |
 *          | D15  | PV4 电流      | 30A ↔ 3V，0A ↔ 5/3V                  |
 *
 * @note    码值域：F2837xS 12 位单端转换结果 ADCRESULT 为 **0~4095**
 *          （不是左对齐到 16 位）。这一点与 EP5K《Sample.c》一致：其零点
 *          检测比较 2048、系数换算按 >>12 做，说明原始码值就是 0~4095。
 *          若以后改成 16 位模式，只需改 ADC_CAL_RAW_TO_CODE12 一个宏。
 *
 * @note    零漂检测：上电后（功率级还没发波、电流为 0）对带 1.5V 偏置的
 *          通道做平均值，得到各自的真实零点；40ms 内完成 200ms 平均。
 *          EP5K《Sample.c》CalibrateADZero() 同思路（跳 30000 拍、平均 4000 点）。
 *          GFCI 通道默认 **不** 做零漂检测，理由见 AdcCalib.c 顶部说明。
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#ifndef ADC_CALIB_H
#define ADC_CALIB_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 码值域与基准
 ******************************************************************************/
/* 12 位满量程码值 */
#define ADC_CAL_CODE_FULL           (4095.0f)
/* 1.5V 偏置对应的码值（双极性通道的标称零点） */
#define ADC_CAL_CODE_MID            (2047.5f)
/* 5/3V 偏置对应的码值（PV 电流的标称零点）：4095 * (5/3) / 3 */
#define ADC_CAL_CODE_PV_MID         (2275.0f)
/* ADC 参考电压（VREFHI），本板 3.0V */
#define ADC_CAL_REF_VOLT            (3.0f)
/* ADCRESULT -> 12 位码值的比例。12 位模式右对齐，直接用 1.0f；
 * 若换 16 位分辨率（0~65535）改成 4096/65536 = 0.0625f 即可。 */
#define ADC_CAL_RAW_TO_CODE12       (1.0f)

/******************************************************************************
 * 标定系数（工程值 / 码值）
 *   双极性通道：K = 满量程物理量 / 2047.5
 *   单极性通道：K = 满量程物理量 / 4095
 ******************************************************************************/
/* 逆变电流：±100A */
#define ADC_CAL_K_INV_CUR           (100.0f / ADC_CAL_CODE_MID)
/* 逆变电压 / 电网电压：±462.69V */
#define ADC_CAL_K_AC_VOLT           (462.69f / ADC_CAL_CODE_MID)
/* 相直流分量：±2.95A */
#define ADC_CAL_K_DC_CUR            (2.95f / ADC_CAL_CODE_MID)
/* 平衡电路电流：±36.4A */
#define ADC_CAL_K_BAL_CUR           (36.4f / ADC_CAL_CODE_MID)
/* GFCI 漏电流：500mA ↔ 3V，0mA ↔ 1.5V → 0.5A / 2047.5（单位 A） */
#define ADC_CAL_K_GFCI              (0.5f / ADC_CAL_CODE_MID)
/* PV 电流：30A ↔ 3V，0A ↔ 5/3V → 30A 对应 (4095-2275)=1820 个码值 */
#define ADC_CAL_K_PV_CUR            (30.0f / (ADC_CAL_CODE_FULL - ADC_CAL_CODE_PV_MID))
/* PV 电压（MUX2）：1033V ↔ 3V（单极性） */
#define ADC_CAL_K_PV_VOLT           (1033.0f / ADC_CAL_CODE_FULL)
/* 上/下半母线电压（MUX1）：563.64V ↔ 3V（单极性） */
#define ADC_CAL_K_BUS_VOLT          (563.64f / ADC_CAL_CODE_FULL)

/******************************************************************************
 * 零漂检测配置
 ******************************************************************************/
/* 上电先跳过的控制拍数（20kHz 下 4000 拍 = 200ms，等 CLA 低通收敛） */
#define ADC_CAL_TRACK_SKIP          (4000U)
/* 平均拍数（20kHz 下 4000 拍 = 200ms，可覆盖 10 个 50Hz 周波） */
#define ADC_CAL_TRACK_SAMPLES       (4000U)
/* 零漂合格范围（码值）：实际零点与标称零点偏差超过该值视为异常，
 * 保留标称零点并置 FAILED 状态，避免把明显错误的零点灌进控制环 */
#define ADC_CAL_ZERO_TOL            (307.0f)     /* ±15% 码值 */
/* GFCI 是否参与上电零漂检测
 *   0：不参与，固定用标称零点（默认）
 *   1：参与
 * 置 0 的理由：GFCI 检测的正是漏电流，若机器上电时就存在漏电，自动零点
 * 会把这部分漏电"吃掉"，之后再也测不到。EP5K 也是在 Protect.c 里把零点
 * 写死成 2132（而不是做上电检测），只是它那个 2132 已经包含硬件偏置漂移。 */
#define ADC_CAL_GFCI_TRACK_EN       (0U)

/******************************************************************************
 * 通道枚举（顺序即 gAdcCalValue 下标，软示波器按地址直接取用）
 ******************************************************************************/
typedef enum
{
    ADC_CAL_INV_CUR_R = 0,
    ADC_CAL_INV_CUR_S,
    ADC_CAL_INV_CUR_T,
    ADC_CAL_INV_CUR_PK_R,
    ADC_CAL_INV_CUR_PK_S,
    ADC_CAL_INV_CUR_PK_T,
    ADC_CAL_INV_VOLT_R,
    ADC_CAL_INV_VOLT_S,
    ADC_CAL_INV_VOLT_T,
    ADC_CAL_GRID_VOLT_R,
    ADC_CAL_GRID_VOLT_S,
    ADC_CAL_GRID_VOLT_T,
    ADC_CAL_DC_R,
    ADC_CAL_DC_S,
    ADC_CAL_DC_T,
    ADC_CAL_BAL_CUR,
    ADC_CAL_GFCI,
    ADC_CAL_PV_CUR1,
    ADC_CAL_PV_CUR2,
    ADC_CAL_PV_CUR3,
    ADC_CAL_PV_CUR4,
    ADC_CAL_PV_VOLT1,
    ADC_CAL_PV_VOLT2,
    ADC_CAL_PV_VOLT3,
    ADC_CAL_PV_VOLT4,
    ADC_CAL_BUS_VOLT_L,
    ADC_CAL_BUS_VOLT_H,
    ADC_CAL_CH_NUM
} E_ADC_CAL_CH;

/******************************************************************************
 * 零漂检测状态
 ******************************************************************************/
typedef enum
{
    ADC_CAL_ZERO_PENDING = 0U,      /* 还没开始（跳过期内） */
    ADC_CAL_ZERO_TRACKING,          /* 正在累加 */
    ADC_CAL_ZERO_DONE,              /* 完成，零点已生效 */
    ADC_CAL_ZERO_FAILED,            /* 超差，保留标称零点 */
    ADC_CAL_ZERO_FROZEN             /* 被禁止（不自动检测） */
} E_ADC_CAL_ZERO_STATE;

/******************************************************************************
 * 对外观测：每拍刷新后的工程值（A / V），供软示波器、Modbus 直接取地址
 ******************************************************************************/
extern volatile float gAdcCalValue[ADC_CAL_CH_NUM];

/* 只读镜像：给 Modbus 点表直接取地址用（0.1 单位，避免上位机做换算） */
extern volatile Uint16 gAdcCalZeroState;    /* 零漂检测状态，见 E_ADC_CAL_ZERO_STATE */
extern volatile Uint16 gAdcCalTrackCnt;     /* 已累加拍数 */
extern volatile int16  gAdcCalObsGfciMa;    /* GFCI 电流，mA */
extern volatile int16  gAdcCalObsBalCur;    /* 平衡电路电流，0.1A */

/******************************************************************************
 * 接口
 ******************************************************************************/

/**
 * @brief  标定层初始化：系数/零点回到标称值，零漂检测状态复位
 * @note   需在 CtrlIsrInit() 之前调用（控制环一开跑就要有可用的系数）
 */
extern void AdcCalibInit(void);

/**
 * @brief  重新开始一次上电零漂检测（例如故障复位后手动触发）
 * @note   调用前必须保证功率级未发波、电流为 0
 */
extern void AdcCalibStartZeroTrack(void);

/**
 * @brief  禁止/使能零漂检测（1=使能，0=冻结在当前零点）
 */
extern void AdcCalibSetTrackEnable(Uint16 u16Enable);

/**
 * @brief  20kHz 刷新：码值 -> 工程值，同时推进零漂检测状态机
 * @note   在控制中断（CtrlIsrRun）最前面调用，保证本拍所有消费者拿到的
 *         是同一份已换算的数据
 */
extern void AdcCalibRefresh(void);

/**
 * @brief  取通道工程值（A 或 V，单位见文件头表格）
 */
extern float AdcCalibGet(Uint16 u16Ch);

/**
 * @brief  三相 / 四路便捷读取
 */
extern float AdcCalibInvCur(Uint16 u16Idx);
extern float AdcCalibInvVolt(Uint16 u16Idx);
extern float AdcCalibGridVolt(Uint16 u16Idx);
extern float AdcCalibPvCur(Uint16 u16Idx);
extern float AdcCalibDc(Uint16 u16Idx);

/**
 * @brief  读/写零点（码值）、读写系数（工程值/码值）
 * @note   写入后立即生效，下一拍刷新即用新值；用于台架标定（接标准源，
 *         先用 AdcCalibSetScale 修系数，再用 AdcCalibSetZeroCode 修零点）
 */
extern float AdcCalibGetZeroCode(Uint16 u16Ch);
extern void  AdcCalibSetZeroCode(Uint16 u16Ch, float fCode);
extern float AdcCalibGetScale(Uint16 u16Ch);
extern void  AdcCalibSetScale(Uint16 u16Ch, float fScale);

/**
 * @brief  恢复全部标称系数与零点（清掉自动检测结果）
 */
extern void AdcCalibRestoreNominal(void);

/**
 * @brief  查询零漂检测状态 / 已累加拍数
 */
extern E_ADC_CAL_ZERO_STATE AdcCalibGetZeroState(void);
extern Uint16 AdcCalibGetTrackCount(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_CALIB_H */

/******************************************************************************
 * @file    User_ClaTask.h
 * @author  yetingheng (004182)
 * @brief   CLA Task1（采样滤波 + 多路选择器归档）对外接口与数据结构
 * @version 0.1
 * @date    2026-10-23
 *
 * @note    数据流：
 *          EPWM1_SOCA → 四路ADC转换 → ADCA SOC15 EOC → ADCINT1
 *          → CLA Task1（滤波 + 选通归档）→ CLA1_1 中断 → CPU 控制中断
 *
 * @note    ADC 结果与信号对应关系（依据《15KW软件管脚整理文档》）：
 *          | ADC-A SOC0 ch2  T相逆变电流 | ADC-B SOC0 ch2  T相直流分量 |
 *          | ADC-A SOC3 ch3  T相电网电压 | ADC-B SOC1 ch0  R相直流分量 |
 *          | ADC-A SOC4 ch4  PV1电流     | ADC-B SOC2 ch1  S相直流分量 |
 *          | ADC-A SOC5 ch5  R相电网电压 | ADC-B SOC3 ch3  多路选择器2 |
 *          | ADC-A SOC6 ch0  T相逆变电压 | ADC-C SOC2 ch2  PV2电流     |
 *          | ADC-A SOC7 ch1  S相电网电压 | ADC-C SOC3 ch3  S相逆变电压 |
 *          | ADC-A SOC15 ch0 序列末位    | ADC-C SOC4 ch4  R相逆变电压 |
 *          | ADC-D SOC0 ch2  S相逆变电流 | ADC-D SOC4 ch0  PV3电流     |
 *          | ADC-D SOC1 ch14 R相逆变电流 | ADC-D SOC5 ch4  平衡电路电流 |
 *          | ADC-D SOC2 ch1  GFCI电流    | ADC-D SOC6 ch15 PV4电流     |
 *          | ADC-D SOC3 ch3  多路选择器1 |                             |
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#ifndef __USER_CLATASK_H
#define __USER_CLATASK_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 相序下标（所有三相数组统一按 R/S/T 排列）
 ******************************************************************************/
#define CLA_IDX_R   (0U)
#define CLA_IDX_S   (1U)
#define CLA_IDX_T   (2U)
#define CLA_PHASE_NUM   (3U)

/******************************************************************************
 * PV 下标
 ******************************************************************************/
#define CLA_IDX_PV1 (0U)
#define CLA_IDX_PV2 (1U)
#define CLA_IDX_PV3 (2U)
#define CLA_IDX_PV4 (3U)
#define CLA_PV_NUM  (4U)

/******************************************************************************
 * 多路选择器地址编码（GPIO46/47/42 = 地址线 A/B/C，两个 MUX 共用）
 *   MUX2 输出 → ADCB3：地址 0~3 = PV1~PV4 电压，地址 4 = VISO
 *   MUX1 输出 → ADCD3：地址 6 = BUSL，地址 7 = BUSH
 ******************************************************************************/
#define CLA_MUX_ADDR_PV1    (0U)
#define CLA_MUX_ADDR_PV2    (1U)
#define CLA_MUX_ADDR_PV3    (2U)
#define CLA_MUX_ADDR_PV4    (3U)
#define CLA_MUX_ADDR_VISO   (4U)
#define CLA_MUX_ADDR_BUSL   (6U)
#define CLA_MUX_ADDR_BUSH   (7U)
#define CLA_MUX_ADDR_MAX    (8U)

/******************************************************************************
 * ADC 结果寄存器映射（改 ADC 配置表时必须同步修改这里）
 ******************************************************************************/
/* —— ADC-A —— */
#define CLA_RES_INV_CUR_T       (AdcaResultRegs.ADCRESULT0)    // ch2  T相逆变电流
#define CLA_RES_GRID_VOLT_T     (AdcaResultRegs.ADCRESULT3)    // ch3  T相电网电压
#define CLA_RES_PV1_CUR         (AdcaResultRegs.ADCRESULT4)    // ch4  PV1电流
#define CLA_RES_GRID_VOLT_R     (AdcaResultRegs.ADCRESULT5)    // ch5  R相电网电压
#define CLA_RES_INV_VOLT_T      (AdcaResultRegs.ADCRESULT6)    // ch0  T相逆变电压
#define CLA_RES_GRID_VOLT_S     (AdcaResultRegs.ADCRESULT7)    // ch1  S相电网电压
/* —— ADC-B —— */
#define CLA_RES_DC_T            (AdcbResultRegs.ADCRESULT0)    // ch2  T相直流分量
#define CLA_RES_DC_R            (AdcbResultRegs.ADCRESULT1)    // ch0  R相直流分量
#define CLA_RES_DC_S            (AdcbResultRegs.ADCRESULT2)    // ch1  S相直流分量
#define CLA_RES_MUX2            (AdcbResultRegs.ADCRESULT3)    // ch3  多路选择器2
/* —— ADC-C —— */
#define CLA_RES_PV2_CUR         (AdccResultRegs.ADCRESULT2)    // ch2  PV2电流
#define CLA_RES_INV_VOLT_S      (AdccResultRegs.ADCRESULT3)    // ch3  S相逆变电压
#define CLA_RES_INV_VOLT_R      (AdccResultRegs.ADCRESULT4)    // ch4  R相逆变电压
/* —— ADC-D —— */
#define CLA_RES_INV_CUR_S       (AdcdResultRegs.ADCRESULT0)    // ch2  S相逆变电流
#define CLA_RES_INV_CUR_R       (AdcdResultRegs.ADCRESULT1)    // ch14 R相逆变电流
#define CLA_RES_GFCI            (AdcdResultRegs.ADCRESULT2)    // ch1  GFCI电流
#define CLA_RES_MUX1            (AdcdResultRegs.ADCRESULT3)    // ch3  多路选择器1
#define CLA_RES_PV3_CUR         (AdcdResultRegs.ADCRESULT4)    // ch0  PV3电流
#define CLA_RES_BAL_CUR         (AdcdResultRegs.ADCRESULT5)    // ch4  平衡电路电流
#define CLA_RES_PV4_CUR         (AdcdResultRegs.ADCRESULT6)    // ch15 PV4电流

/******************************************************************************
 * 一阶低通滤波系数 alpha（alpha = 2*pi*fc*Ts / (1 + 2*pi*fc*Ts)）
 *   AC   ：fc≈2kHz @20kHz    —— 电网/逆变电压，只压开关噪声
 *   PVCUR：fc≈500Hz @20kHz   —— PV电流，供MPPT
 *   MUX  ：fc≈120Hz @3.33kHz —— 选通电压（每路刷新周期300us）
 * 逆变电流不做数字滤波，直接送控制环（保相位裕度）
 ******************************************************************************/
#define CLA_LPF_ALPHA_AC        (0.06f)
#define CLA_LPF_ALPHA_PVCUR     (0.15f)
#define CLA_LPF_ALPHA_MUX       (0.20f)

/******************************************************************************
 * CLA → CPU 消息结构（放在 Cla1ToCpuMsgRAM）
 * 所有数值均为 ADC 码值域（12位、0~4095），标定换算在 CPU 侧完成
 ******************************************************************************/
typedef struct
{
    float  fInvCur[CLA_PHASE_NUM];      // 逆变电流（未滤波）
    float  fInvVolt[CLA_PHASE_NUM];     // 逆变电压（已滤波）
    float  fGridVolt[CLA_PHASE_NUM];    // 电网电压（已滤波）
    float  fPvCur[CLA_PV_NUM];          // PV1~PV4 电流（已滤波）
    float  fPvVolt[CLA_PV_NUM];         // PV1~PV4 电压（已滤波，MUX2轮换）
    float  fBusVoltL;                   // 下半母线电压（已滤波，MUX1）
    float  fBusVoltH;                   // 上半母线电压（已滤波，MUX1）
    float  fMuxB3;                      // MUX2 原始滤波值（调试用）
    float  fMuxD3;                      // MUX1 原始滤波值（调试用）
    Uint16 u16Seq;                      // 采样序号，每周期 +1
    Uint16 u16MuxAddr;                  // 本周期的选通地址（0~7）
} ST_CLA_TO_CPU;

/******************************************************************************
 * CPU → CLA 消息结构（放在 CpuToCla1MsgRAM）
 *   u16MuxAddr 是"当前真正驱动在 GPIO46/47/42 上的地址"，由 CPU 在原地址切换
 *   之后写入；CLA 依据它把本周期 B3/D3 的结果归档到正确的通道，因此不存在
 *   CPU 中断延迟导致归档错位的问题
 ******************************************************************************/
typedef struct
{
    Uint16 u16MuxAddr;                  // 当前生效的选通地址（0~7）
    Uint16 u16Enable;                   // 1：使能滤波与选通归档；0：只采不归档
    float  fTheta;                      // 预留：并网角度（CLA 侧如需角度可用）
    float  fInvCurOfs[CLA_PHASE_NUM];   // 预留：三相电流零漂补偿
} ST_CPU_TO_CLA;

/******************************************************************************
 * CLA 滤波状态（放在 Cla1DataRam，RAMLS5）
 ******************************************************************************/
typedef struct
{
    float  fInvVolt[CLA_PHASE_NUM];     // 逆变电压滤波状态
    float  fGridVolt[CLA_PHASE_NUM];    // 电网电压滤波状态
    float  fPvCur[CLA_PV_NUM];          // PV电流滤波状态
    float  fMuxValue[6];                // 0~3:PV1~4电压  4:BUSL  5:BUSH
} ST_CLA_FILT;

/******************************************************************************
 * 全局实例（定义在 User_ClaTask.cla 中）
 ******************************************************************************/
extern ST_CLA_TO_CPU Cla1ToCpuMsg;
extern ST_CPU_TO_CLA CpuToCla1Msg;
extern ST_CLA_FILT   stClaFilt;

/******************************************************************************
 * CLA 任务入口（由 Hw_ClaInit 通过配置表映射到 MVECT1）
 ******************************************************************************/
extern __interrupt void Cla1Task1(void);

#ifdef __cplusplus
}
#endif

#endif // __USER_CLATASK_H

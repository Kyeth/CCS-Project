#ifndef __PLATFORMMANAGER_H
#define __PLATFORMMANAGER_H

#include "F28003x_Examples.h"
#include "Hw_AdcDriver.h"


#ifdef __cplusplus
    extern "C" {
#endif

#define MODESCI_MODBUS   0
#define MODESCI_XMODEM   1


typedef struct
{
    int16  VNTC_2;
    int16  VEnvironment;
    int16  VTransformer_1;
    int16  VTransformer_2;
    int16  Heating;
    Uint16 VAirBack;
    Uint16 VAirFront;
    Uint16 VBusFilter1;
    Uint16 VBat;
    int16  IBus;
    Uint16 IBranch_1;
    Uint16 IBranch_2;
    Uint16 IErr;
    int16  AvgPower;
    Uint16 VBusRaw;
    Uint16 VBus;
    int16  VBusFilter;
    Uint16 VBatRaw;
    int16  IBusRaw;
    Uint16 Vref;
    Uint16 LimitBrustHigh;
    Uint16 LimitBrustLow;
    Uint16 Rsv1;
    Uint16 Rsv2;
    Uint16 Rsv3;
    Uint16 Rsv4;
}ST_CommData;


typedef struct
{
    int16 VoutZero;             // 高压侧输出电压零点
    int16 VPZero;               // 空开后端电压零点
    int16 IoutZero;             // 高压侧输出电流零点
    int16 VbatZero;             // 低压侧电池电压零点
    int16 VP1Zero;              // 空开前端电压零点

    int16 DoubleSet;

    int32 VoutRawSum;
    int32 IoutRawSum;

    float DoubleK;

    float Ireal;                // 实际电流(用于判断能量流动方向)

    float VBusSampCoefficient;         // 母线电压采样系数
    float VAirFrontSampCoefficient;    // 空开前端电压采样系数
    float VAirBackSampCoefficient;     // 空开后端电压采样系数
    float VBatSampCoefficient;         // 电池电压采样系数

    float IBusSampCoefficient;         // 母线输出谐振电流采样系数

    float IBranch_1SampCoefficient;    // 支路1谐振电流采样系数
    float IBranch_2SampCoefficient;    // 支路2谐振电流采样系数
    float HeatingSampCoefficient;    // 支路2谐振电流采样系数

    float VNTC_1SampCoefficient;         //
    float VNTC_2SampCoefficient;


    float VBus;         // 母线电压
    float VBusFilter;   // 带通
    float VBusFilter1;  // 低通
    float VBusRaw;      // 母线采样滤波前

    float VAirFront;    // 空开前端电压
    float VAirBack;     // 空开后端电压
    float VBat;         // 电池电压
    float VBatRaw;      // 电池电压滤波前

    float IBusRaw;
    float IBusSum;
    float IBus;         // 母线输出谐振电流
    float IBusFilter;   // 母线输出电流滤波

    float IBranch_1;    // 支路1谐振电流
    float IBranch_2;    // 支路2谐振电流
    float IHeatingFilm; // 加热膜电流
    float IErr;         // 谐振电流均值
    float IErrFilter;   // 谐振电流滤波值

    float Heating;      // 加热膜电流

    float VNTC_1;
    float VNTC_2;          // NTC2

    float VTransformer_1;  // 低压管1
    float VTransformer_2;  // 变压器2

    float VEnvironment;    // 环境温度
    Uint16 u16MdsVBus;          // 母线电压，将真实值放大了10倍
    int16 i16MdsIBus;          // 母线电流，将真实值放大了10倍
    int16 i16MdsPower;          // 母线输出功率，放电为正，充电为负
    Uint16 u16MdsIHeatingFilm;   // 加热膜电流，将真实值放大了10倍

    float VBatSum;          // 临时用求滑动，后续删掉
    float PowerAim;         // 目标功率
    int16 InstantPower;     // 瞬时功率
    int16 AvgPower;         // 平均功率
    Uint16 PowerCnt;
    int32 PowerSum;
    int16 ComPower;
}ST_SAMPLE;



typedef struct
{
    float b0, b1, b2;
    float a1, a2;

    float x1, x2; // 输入延迟
    float y1, y2; // 输出延迟

    float w0;     // 数字角频率
    float alpha;  // alpha参数

    float Q;     // 品质因数，控制陷波宽度
    float SampleFreq;  // 采样频率(Hz)

    float CenterFreq; // 当前中心频率
    float output;
}ST_BUS_FILTER;


typedef struct
{
    Uint16 *pAdcARawResult;     // 指向存放ADCA结果寄存器中原始值的指针
    Uint16 *pAdcBRawResult;     // 指向存放ADCB结果寄存器中原始值的指针
    Uint16 *pAdcCRawResult;     // 指向存放ADCC结果寄存器中原始值的指针
    Uint16 u16AdcARawResult[16];    // 存放ADCA结果寄存器中取得的原始值
    Uint16 u16AdcBRawResult[16];    // 存放ADCB结果寄存器中取得的原始值
    Uint16 u16AdcCRawResult[16];    // 存放ADCC结果寄存器中取得的原始值
    Uint16 ModeSciID0           :2;     // 0:Modbus     1:Xmodem
    Uint16 ModeSciID1           :2;     // 0:Modbus     1:Xmodem
    Uint16 ModbusTimeOutFlag_1  :1;     // 0:通信正常   1:通信异常
    Uint16 ModbusTimeOutFlag_2  :1;     // 0:通信正常   1:通信异常
    Uint16 PlatformIsInit       :1;     // 0:未完成初始化     1:已完成初始化
    Uint16 bRsv                 :9;    // 保留
    Uint16 u16ZeroDirftIsInit;          // 0:未完成零飘初始化     1:已完成零飘初始化
    Uint16 u16IBusZeroDirftCnt;
    Uint16 u16IBusZeroDirft;                // 零飘标定值
    Uint32 u32IBusZeroDirftSum;
    Uint16 u16AdcCnt;
    Uint16 u16WorkFrequency;              // 逆变器工作频率
    float fIBusZeroSet;

    Uint16 u16AllowChargePowerMax;        // 运行最大充电功率
    Uint16 u16AllowDischargePowerMax;     // 运行最大放电功率

}SYS_PLATFORM_CTRL;


static inline Uint16 GetModeSciID0(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    return pObjPlatformCtrl->ModeSciID0;
}

static inline void SetModeSciID0(SYS_PLATFORM_CTRL *pObjPlatformCtrl, Uint16 Mode)
{
    pObjPlatformCtrl->ModeSciID0 = Mode;
}


static inline Uint16 GetModeSciID1(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    return pObjPlatformCtrl->ModeSciID1;
}

static inline void SetModeSciID1(SYS_PLATFORM_CTRL *pObjPlatformCtrl, Uint16 Mode)
{
    pObjPlatformCtrl->ModeSciID1 = Mode;
}



static inline void SetModbusTimeOutFlag_1(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    pObjPlatformCtrl->ModbusTimeOutFlag_1 = 1;
}

static inline void ClearModbusTimeOutFlag_1(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    pObjPlatformCtrl->ModbusTimeOutFlag_1 = 0;
}


static inline void SetModbusTimeOutFlag_2(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    pObjPlatformCtrl->ModbusTimeOutFlag_2 = 1;
}

static inline void ClearModbusTimeOutFlag_2(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    pObjPlatformCtrl->ModbusTimeOutFlag_2 = 0;
}

static inline Uint16 GetPlatformIsInit(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    return pObjPlatformCtrl->PlatformIsInit;
}


static inline void SetPlatformInit(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    pObjPlatformCtrl->PlatformIsInit = 1;
}


extern void AdcValueCopyPlatform(SYS_PLATFORM_CTRL *pObjPlatformCtrl, Uint16 u16AdcId);
extern void PlatformSimplingCoefficientInit( ST_SAMPLE *pObjSample);
extern SYS_PLATFORM_CTRL ObjPlatformCtrl;
extern ST_CommData CommDataTrans;
extern ST_SAMPLE ObjSample;
extern ST_BUS_FILTER ObjBusFilter;
extern ST_BUS_FILTER ObjIoscFilter;
extern ST_BUS_FILTER ObjIbusFilter;

extern float AD_to_Temperature(uint16_t ad_sample, const uint16_t* value_table, int16_t table_size);
extern void TaskTemp(void);
extern void InitTrapFilter(ST_BUS_FILTER* filter, Uint16 freq);
extern float GetFilterTrap(ST_BUS_FILTER* filter, float value, float PcsFreq);
extern void ZeroDirftCheck(SYS_PLATFORM_CTRL *pObjPlatformCtrl);
extern void TaskFilterComm(void);
extern void TaskPlatform(void);

#ifdef __cplusplus
    }
#endif

#endif

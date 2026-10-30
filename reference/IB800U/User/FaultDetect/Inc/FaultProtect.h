#ifndef __FAULTPROTECT_H
#define __FAULTPROTECT_H


#include "F28003x_device.h"
#include "LLCControl.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 EepromFault      :1; // EE增益读取故障
        Uint16 BusSoftStartFail :1; // Bus软启故障
        Uint16 Null1            :1;
        Uint16 Null2            :1;

        Uint16 Null3            :1;
        Uint16 BmsCommFault     :1; // BMS通讯故障
        Uint16 HvRelayFail      :1; // 高压继电器吸合故障
        Uint16 Null4            :1;

        Uint16 Null5            :1;
        Uint16 Null6            :1;
        Uint16 Null7            :1;
        Uint16 ExtRelayFail     :1; // 外部断路器未闭合故障

        Uint16 BusPreChgFail    :1; // 高压预充故障
        Uint16 HardwareZeroFail :1; // 零漂异常
        Uint16 HardwareSpsFail  :1; // 硬件辅源欠压
        Uint16 HeatFail         :1; // 加热膜未工作，却有电流故障
    }Bits;
}U_FAULT_WORD_BMS1;

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 Hd_DischgCurrOver1  :1; // 第1路硬件放电过流保护
        Uint16 Hd_ChgCurrOver1     :1; // 第1路硬件充电过流保护
        Uint16 Hd_DischgCurrOver2  :1; // 第2路硬件放电过流保护
        Uint16 Hd_ChgCurrOver2     :1; // 第2路硬件充电过流保护

        Uint16 SF_DischgCurrOver1  :1; // 软件第1路放电过流保护
        Uint16 SF_ChgCurrOver1     :1; // 软件第1路充电过流保护
        Uint16 SF_DischgCurrOver2  :1; // 软件第2路放电过流保护
        Uint16 SF_ChgCurrOver2     :1; // 软件第2路充电过流保护

        Uint16 Hd_BusCurrOver      :1; // 母线硬件过流保护
        Uint16 SF_BusCurrOver_Chg    :1;   // 软件母线放电过流保护
        Uint16 SF_BusCurrOver_DisChg :1;   // 软件母线充电过流保护
        Uint16 SopOverTimeOut :1;      // 超过总充电SOP限值超时故障

        Uint16 McanCommFail   :1;      // MCAN通讯故障
        Uint16 RelayOpenFail  :1;      // 继电器粘连故障
        Uint16 DriverFail1    :1;      // 第1路驱动损坏
        Uint16 DriverFail2    :1;      // 第2路驱动损坏
    }Bits;
}U_FAULT_WORD_BMS2;

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 BmsOverVolt      :1; // Bms过压保护
        Uint16 BmsUnderrVolt    :1; // Bms欠压保护
        Uint16 BusOverVolt      :1; // BUS(Boost端)过压保护
        Uint16 BusUnderVolt     :1; // BUS(Boost端)欠压保护

        Uint16 BusOverVolt_LV   :1; // BUS过压保护
        Uint16 BusUnderVolt_LV  :1; // BUS欠压保护
        Uint16 HeatSinkTempFail :1; // 散热器过温保护
        Uint16 Inside1TempFail  :1; // 内部环温1保护

        Uint16 Inside2TempFail  :1; // 内部环温2保护
        Uint16 InductorTempFail :1; // 灌胶电感保护
        Uint16 BatPosTempFail   :1; // BAT+端子附近温度保护
        Uint16 BatNegTempFail   :1; // BAT-端子附近温度保护

        Uint16 VoutNegTempFail  :1; // Vout-端子附近温度保护
        Uint16 Null1    :1;         // Null1
        Uint16 Null2    :1;         // Null2
        Uint16 Null3    :1;         // Null3
    }Bits;
}U_FAULT_WORD_BMS3;

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 HeatResistorFail  :1; // 加热膜电阻异常
        Uint16 HeatCurrSmall     :1; // 加热膜工作，却没电流故障
        Uint16 HeatOverCurr      :1; // 加热膜工作，过流故障
        Uint16 OverTemp          :1; // 过温告警

        Uint16 EepromFault_Others:1; // EE其它读写故障
        Uint16 CanCommFail_DG    :1; // 调试CAN无通信
        Uint16 RS485CommFail_DG  :1; // 调试485无通信
        Uint16 Null1    :1;

        Uint16 Null2    :1;
        Uint16 Null3    :1;
        Uint16 Null4    :1;
        Uint16 Null5    :1;

        Uint16 Null6    :1;
        Uint16 Null7    :1;
        Uint16 Null8    :1;
        Uint16 Null9    :1;
    }Bits;
}U_ALARM_WORD_BMS;

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 bit0     :1;
        Uint16 bit1     :1;
        Uint16 bit2     :1;
        Uint16 bit3     :1;
        Uint16 bit4     :1;
        Uint16 bit5     :1;
        Uint16 bit6     :1;
        Uint16 bit7     :1;
        Uint16 bit8     :1;
        Uint16 bit9     :1;
        Uint16 bit10    :1;
        Uint16 bit11    :1;
        Uint16 bit12    :1;
        Uint16 bit13    :1;
        Uint16 bit14    :1;
        Uint16 bit15    :1;
    }Bits;
}U_FAULT_WORD;

typedef struct
{

    U_FAULT_WORD_BMS1 unFaultBMS1;
    U_FAULT_WORD_BMS2 unFaultBMS2;
    U_FAULT_WORD_BMS3 unFaultBMS3;
    U_ALARM_WORD_BMS  unAlarmBMS;
    Uint16 u16FaultNum;

    U_FAULT_WORD unFaultAlarmWord1;              // 故障告警字

    U_FAULT_WORD unFaultStatusWord1;             // 故障状态字
    U_FAULT_WORD unFaultMaskWord1;               // 故障屏蔽字

    U_FAULT_WORD unFaultStatusWord2;             // 故障状态字
    U_FAULT_WORD unFaultMaskWord2;               // 故障屏蔽字

    U_FAULT_WORD unFaultStatusWord3;             // 故障状态字
    U_FAULT_WORD unFaultMaskWord3;               // 故障屏蔽字

    U_FAULT_WORD unFaultStatusWord4;             // 故障状态字
    U_FAULT_WORD unFaultMaskWord4;               // 故障屏蔽字

    U_FAULT_WORD unBMSFaultStatusWord1;          // BMS故障状态字
    U_FAULT_WORD unBMSFaultMaskWord1;            // BMS故障屏蔽字

    U_FAULT_WORD unBMSFaultStatusWord2;          // BMS故障状态字
    U_FAULT_WORD unBMSFaultMaskWord2;            // BMS故障屏蔽字

    U_FAULT_WORD unBMSIOFaultStatusWord1;          // BMS的IO故障状态字
    U_FAULT_WORD unBMSIOFaultMaskWord1;            // BMS的IO故障屏蔽字

    U_FAULT_WORD unBMSIOFaultStatusWord2;          // BMS的IO故障状态字
    U_FAULT_WORD unBMSIOFaultMaskWord2;            // BMS的IO故障屏蔽字

    U_FAULT_WORD unBMSFaultAlarmWord1;           // BMS故障告警字

    Uint16 u16VBusOPPPointLevel_1;              // 1级输出功率过载保护点设置,倍率:10
    Uint16 u16VBusOPPPointLevel_2;              // 2级输出功率过载保护点设置,倍率:10
    Uint16 u16VBusOPPPointLevel_3;              // 3级输出功率过载保护点设置,倍率:10
    Uint16 u16VBusOVPPoint;                     // 母线电压过压保护点设置,倍率:10
    Uint16 u16VBusUVPPoint;                     // 母线电压欠压保护点设置,倍率:10
    Uint16 u16IBusOCPPoint;                     // 母线电流过流保护点设置,倍率:10
    Uint16 u16IBusUCPPoint;                     // 母线电流过流保护点设置,倍率:10
    Uint16 u16IBranchECPPoint;                  // 支路电流均流保护点设置,倍率:10
    Uint16 u16VBatUVPPoint;                     // 电池电压欠压保护点设置,倍率:10
    Uint16 u16VBatOVPPoint;                     // 电池电压过压保护点设置,倍率:10
    Uint16 u16VRatioPPointLevel_1;              // 1级电压变比保护点设置，倍率:10
    Uint16 u16VRatioPPointLevel_2;              // 2级电压变比保护点设置，倍率:10
    Uint16 u16VRatioPPointLevel_3;              // 3级电压变比保护点设置，倍率:10
    Uint16 u16LimitCurrentPoint;                // 限制电流过载保护点设置，倍率:10
    Uint16 u16RadiatorPoint;                    // 散热器温度保护点设置，倍率:1
    Uint16 u16VLowPipePoint;                    // 低压管温度保护点设置，倍率:1
    Uint16 u16AmbientPoint;                     // 环境温度保护点设置，倍率:1
    Uint16 u16TransformerPoint;                 // 变压器温度保护点设置，倍率:1
    int16  i16CryoProtectionPoint;              // 低温保护点，倍率:1
    Uint16 u16LimitAirPoint;                    // 空开两端电压保护点设置，倍率:10
    Uint16 u16LimitRelayPoint;                  // 继电器两端电压保护点设置，倍率:10
    Uint16 u16IoscOCPPoint;                     // 谐振电流3V单点保护点设置，倍率:1

    Uint32 u32VBusOPPTimesSetLevel_1;           // 1级输出功率过载保护时间门限设置
    Uint32 u32VBusOPPTimesSetLevel_2;           // 2级输出功率过载保护时间门限设置
    Uint32 u32VBusOPPTimesSetLevel_3;           // 3级输出功率过载保护时间门限设置
    Uint32 u32VBusOVPTimesSet;                  // 母线电压过压保护时间门限设置
    Uint32 u32VBusUVPTimesSet;                  // 母线电压欠压保护时间门限设置
    Uint32 u32IBusOCPTimesSet;                  // 母线电流过流保护时间门限设置
    Uint32 u32IBranchECPTimesSet;               // 支路电流均流保护时间门限设置
    Uint32 u32VBatUVPTimesSet;                  // 电池电压欠压保护时间门限设置
    Uint32 u32VBatOVPTimesSet;                  // 电池电压过压保护时间门限设置
    Uint32 u32VRatioPTimeSetLevel_1;            // 1级电压变比保护时间门限设置
    Uint32 u32VRatioPTimeSetLevel_2;            // 2级电压变比保护时间门限设置
    Uint32 u32VRatioPTimeSetLevel_3;            // 3级电压变比保护时间门限设置
    Uint32 u32SoftOVPTimesSet;                  // 软启动长时间保护时间门限设置
    Uint32 u32LimitAirTimesSet;                 // 空开两端电压保护时间门限设置
    Uint32 u32LimitRelayTimesSet;               // 继电器两端电压保护时间门限设置
    Uint32 u32BatAuxUPTimesSet;                 // 低压辅源欠压保护时间门限设置
    Uint32 u32BusAuxUPTimesSet;                 // 高压辅源欠压保护时间门限设置
    Uint32 u32FanCheckTimesSet;                 // 风扇自检告警保护时间门限设置
    Uint32 u32ParallelVersionCheckTimeSet;      // 并机版本检测保护时间门限设置
    Uint32 u32LimitCurrentTimesSet;             // 限制电流过载保护时间门限设置
    Uint32 u32TemperatureTimesSet;              // 温度保护时间门限设置
    Uint32 u32IoscOCPTimesSet;                  // 谐振电流单点保护时间门限设置
    Uint32 u32IoscUCPTimesSet;                  // 谐振电流检测时间
    Uint32 u32Heat1TimesSet;
    Uint32 u32Heat2TimesSet;
    Uint32 u32Heat3TimesSet;
    Uint32 u32Heat4TimesSet;

    Uint32 u32VBusOPPCntLevel_1;                // 1级输出功率过载保护时间计数器
    Uint32 u32VBusOPPCntLevel_2;                // 2级输出功率过载保护时间计数器
    Uint32 u32VBusOPPCntLevel_3;                // 3级输出功率过载保护时间计数器
    Uint32 u32VBusOVPCnt;                       // 母线电压过压保护时间计数器
    Uint32 u32VBusUVPCnt;                       // 母线电压欠压保护时间计数器
    Uint32 u32IBusOCPCnt;                       // 母线电流过流保护时间计数器
    Uint32 u32IBranchECPCnt;                    // 支路电流均流保护时间计数器
    Uint32 u32VBatUVPCnt;                       // 电池电压欠压保护时间计数器
    Uint32 u32VBatOVPCnt;                       // 电池电压欠压保护时间计数器
    Uint32 u32VRatioPCntLevel_1;                // 1级电压变比保护时间计数器
    Uint32 u32VRatioPCntLevel_2;                // 2级电压变比保护时间计数器
    Uint32 u32VRatioPCntLevel_3;                // 3级电压变比保护时间计数器
    Uint32 u32SoftOVPCnt;                       // 软启动长时间保护时间计数器
    Uint32 u32BusLowTimer;                      // 软启动BUS电压低保护时间计数器
    Uint32 u32BusLowCheckCnt;                   // BUS电压低检测时间计数器
    Uint32 u32LimitAirCnt;                      // 空开两端电压保护时间计数器
    Uint32 u32LimitRelayCnt;                    // 继电器两端电压保护时间计数器
    Uint32 u32ParallelVersionCheckCnt;          // 并机版本检测保护时间计数器
    Uint32 u32LimitCurrentCnt;                  // 限制电流过载保护时间计数器
    Uint32 u32TemperatureCnt;                   // 温度保护时间门限设置
    Uint32 u32BatAuxUPCnt;                      // 低压辅源保护时间计数器
    Uint32 u32BusAuxUPCnt;                      // 高压辅源保护时间计数器
    Uint16 u16VBusStepCnt;
    Uint32 u32FanCheckCnt;                      // 风扇自检告警时间计数器
    Uint32 u32IoscOCPCnt;                       // 谐振电流单点保护时间计数器
    Uint32 u32IoscUCPCnt;                       // 谐振电流检测计数器
    Uint32 u32Heat1Cnt;
    Uint32 u32Heat2Cnt;
    Uint32 u32Heat3Cnt;
    Uint32 u32Heat4Cnt;

    Uint16 u16FaultReset;                       // 故障复位
    Uint16 u16TotalFaultSign;                   // 总故障标志
    Uint16 u16SoftFPTakesEffect;                // 软件故障保护生效标志
    Uint16 u16HardFPTakesEffect;                // 硬件故障保护生效标志
    Uint16 u16IoscCheckFinish;                  // 谐振电流单点故障标志

    Uint16 u16BusAuxDelayCnt;
}ST_PROTECT;

extern ST_PROTECT stSysProtect;
extern void SysProtectPointInit(ST_PROTECT *pstSysProtect);
extern void TaskFaultDetect(void);
extern void SetTaskFaultBit(void);

#ifdef __cplusplus
    }
#endif

#endif

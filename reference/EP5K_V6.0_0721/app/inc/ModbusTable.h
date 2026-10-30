/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name:ModbusTable.h
** Latest modified Date:2021-08-27
** Latest Version: V1.0
** Description:handleModbus tables
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2021-08-27
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/
#ifndef _MODBUSTABLE_H
#define _MODBUSTABLE_H

#include "stdint.h"

#define SW 501926
// ------------------------ARM MODBUS 写寄存器--------------------------
typedef struct
{
    uint16_t BMSCommErr:1;        // bit0 BMS 通信故障
    uint16_t BMSOffAlarm:1;       // bit1 BMS 提前关机警告
    uint16_t E2PROMErr:1;         // bit2 EEPROM 异常
    uint16_t CFGIErr:1;           // bit3 GFCI 异常

    uint16_t PowerLimit:1;        // bit4 限功率标志
    uint16_t TempOver:1;          // bit5 温度保护标志
    uint16_t ADZeroErr:1;         // bit6 AD 零点异常
    uint16_t BMSChgProt:1;        // bit7 BMS 充电保护

    uint16_t BMSDigChgProt:1;     // bit8 BMS 放电保护
    uint16_t BMSOSErr:1;          // bit9 BMS 系统故障
    uint16_t FANOnOff:1;          // bit10 风扇启动标志
    uint16_t ATEMode:1;           // bit11 ATE模式
    uint16_t Res2:4;
}ARM_FAULT_T;
typedef union
{
    uint16_t       all;
    ARM_FAULT_T    bit;
}ARM_FAULT_U;

// 标定系数
typedef struct
{
    uint16_t Uinv1K;        // 逆变1电压比例系数
    uint16_t Uinv2K;        // 逆变2电压比例系数
    uint16_t Iinv1K1;        // 逆变1电流比例系数（逆变）
    uint16_t Iinv2K1;        // 逆变2电流比例系数（逆变）
    uint16_t Iinv1K2;        // 逆变1电流比例系数（逆变）
    uint16_t Iinv2K2;        // 逆变2电流比例系数（逆变）
    uint16_t Iinv1K3;        // 逆变1电流比例系数（整流）
    uint16_t Iinv2K3;        // 逆变2电流比例系数（整流）
    uint16_t Iinv1K4;        // 逆变1电流比例系数（整流）
    uint16_t Iinv2K4;        // 逆变2电流比例系数（整流）
    uint16_t Ugrid1K;       // 电网1电压比例系数
    uint16_t Ugrid2K;       // 电网2电压比例系数

    uint16_t ULoad1K;       // 负载1电压比例系数
    uint16_t ULoad2K;       // 负载2电压比例系数
    uint16_t ILoad1K;       // 负载1电流比例系数
    uint16_t ILoad2K;       // 负载2电流比例系数

    uint16_t UPV1K;         // PV1电压比例系数
    uint16_t UPV2K;         // PV2电压比例系数
    uint16_t UPV3K;         // PV3电压比例系数
    uint16_t UPV4K;         // PV4电压比例系数

    uint16_t IPV1K;         // PV1电流比例系数
    uint16_t IPV2K;         // PV2电流比例系数
    uint16_t IPV3K;         // PV3电流比例系数
    uint16_t IPV4K;         // PV4电流比例系数

    uint16_t BUSK;        // BUS电压比例系数
    uint16_t BUS1K;         // BUS1电压比例系数
    uint16_t BUS2K;         // BUS2电压比例系数

    uint16_t Uinv1B;        // 逆变1电压零飘
    uint16_t Uinv2B;        // 逆变2电压零飘
    uint16_t Iinv1B;        // 逆变1电流零飘
    uint16_t Iinv2B;        // 逆变2电流零飘
    uint16_t Ugrid1B;       // 电网1电压零飘
    uint16_t Ugrid2B;       // 电网2电压零飘

    uint16_t UPV1B;         // PV1电压比例零飘
    uint16_t UPV2B;         // PV2电压比例零飘
    uint16_t UPV3B;         // PV3电压比例零飘

    uint16_t IPV1B;         // PV1电流比例零飘
    uint16_t IPV2B;         // PV2电流比例零飘
    uint16_t IPV3B;         // PV3电流比例零飘

    uint16_t BUS1B;         // BUS1电压比例零飘
    uint16_t BUS2B;         // BUS2电压比例零飘
    uint16_t UBAT1B;        // 电池电压比例零飘
}ARM_AD_COEFF_T;

// ARM 使能位
typedef struct
{
    uint16_t GridRly:1;     // 并网继电器使能
    uint16_t LoadRly:1;     // 负载继电器使能
    uint16_t ByPassRly:1;   // 旁路继电器
    uint16_t ChgRly:1;      // 预充继电器使能

    uint16_t Res1:1;        // 保留
    uint16_t Res2:1;        // 保留
    uint16_t Res3:1;        // 保留
    uint16_t PV1PWM:1;      // PV1发波使能

    uint16_t PV2PWM:1;      // PV2发波使能
    uint16_t PV3PWM:1;      // PV3发波使能
    uint16_t Inv1PWM:1;     // inv1 pwm 使能
    uint16_t Inv2PWM:1;     // inv2 pwm 使能

    uint16_t Res4:1;        // 保留
    uint16_t LLC1PWM:1;     // LLC1 pwm 使能
    uint16_t LLC2PWM:1;     // LLC2 pwm 使能
    uint16_t Res5:1;        // 保留

}ARM_ENABLE_T;

//DSP写ARM标志位
struct  DSP_WRITE_BITS
{
    uint16_t BUSControl:1;               //0     PCS控母线标志    1:控制母线 0:不控母线
    uint16_t LowPowerMode:1;             //1     BMS低功耗标志    1:低功耗开启 0:低功耗关闭 该位已不使用
    uint16_t BUSShortErr:1;              //2     半母线短路故障     1:短路 0:正常
    uint16_t PVKeepOCErr:1;              //3     PV持续过流故障   1:过流 0:正常
    uint16_t StopRecoverFlag:1;          //4     硬件故障无法恢复标志 1:无法恢复 0:正常恢复
    uint16_t BatteryPowerOn:1;           //5     电池包激活完成标志 1:激活完成 0:未激活
    uint16_t ParaWiringErr:1;            //6     并机接线方式错误
    uint16_t pwmEn:1;                    //7
    uint16_t Rs:8;                        //8~15  预留
};
union DSP_WRITE_REG
{
    uint16_t                all;
   struct DSP_WRITE_BITS   bit;
};

typedef struct
{
    uint16_t Res1:3;

    uint16_t ATS:1;          		// bit3 ATS标签
    uint16_t DieselMode:1;   		// bit4 柴油发电机模式
//    uint16_t MicroInvterMode:1;     // bit2
//    uint16_t Optimizer:1;         	// bit8
    uint16_t Res5:4;               //5678

    uint16_t CTCheck:1;             // bit9 Arm CT自检错误
    uint16_t Res2:1;
    uint16_t AntiFlowEn:1;          // bit11
    uint16_t CTDir:1;          // bit12
    uint16_t CTFault:1;          // bit13
    uint16_t Res3:2;
}MODEL_ENABLE_T;
// 控制使能位
typedef union
{
    uint16_t         all;
    MODEL_ENABLE_T   bit;
}MODEL_ENABLE_U;

// 控制使能位
typedef union
{
    uint16_t       all;
    ARM_ENABLE_T   bit;
}ARM_ENABLE_U;

typedef struct
{
    uint16_t WFlag;                     // 写常规数据包标志,有写请求时该位置1,解析完成清0
    uint16_t WAdKCoefFlag;              // 写 AD 校准比例参数标志位
    uint16_t WAdBCoefFlag;              // 写 AD 校准零飘参数标志位
    uint16_t WAuthenFlag;               // 写认证标志位
    uint16_t FreqEnFlag;                // 写认证标志位
    MODEL_ENABLE_U Model;               // 模式设置

    int16_t DCChangeFlg;
    int16_t DCComponent1;               // 逆变 1 输出电流直流分量,单位0.1
    int16_t DCComponent2;               // 逆变 2 输出电流直流分量
    float DCI1;               // 逆变 1 输出电流直流分量,单位0.1
    float DCI2;
    int16_t DCIProtect1;               // 逆变 1 输出电流直流分量,单位0.1
    int16_t DCIProtect2;               // 逆变 2 输出电流直流分量
    int16_t ActivePowerSetR;           // R 相有功功率设定值,正发电,负充电
    int16_t ActivePowerSetS;           // S 相有功功率设定值

    int16_t InActivePowerSetR;         // R 相无功功率设定值,仅认证需要无功控制
    int16_t InActivePowerSetS;         // S 相无功功率设定值
    int16_t PVTotalP;                  // 老化测试PV总功率
    int16_t GridVTotalP;               // 老化测试电网总功率

    int16_t BMSChgMaxVoltSet;          // BMS 允许充电电压
    int16_t BMSChgMaxPowerSet;         // BMS 允许充电功率
    int16_t BMSDischgMaxPowerSet;      // BMS 允许放电功率
    int16_t BMSSOC;                    // BMS 电量

    uint16_t SystemPowerOn;            // 系统开关机设置
    uint16_t Restore;                  // 恢复出厂设置
    uint16_t FrequencySet;             // 频率设置   0-50hz; 1-60hz

    uint16_t PVParallel;               // PV 并联模式
    uint16_t LoadVolt1;                // R 相负载电压 rms
    uint16_t LoadVolt2;                // S 相负载电压 rms
    uint16_t FanSpeed;                 // 风扇档位控制
    uint16_t Grid1SValue;              // 电网1视在功率
    uint16_t Grid2SValue;              // 电网2视在功率

    uint16_t AgingLable;               // 老化标签
    uint16_t EnableGridChg;            // 并网充电使能;1-使能只能控电压

    int16_t PVMaxPowerSet;             // PV 最大功率设置,默认9000
    int16_t PVTempLimitPower;          // PV温度限功率
    int16_t GridChgMaxCurrSet;         // 电网允许充电电流
    int16_t GridDischgMaxCurrSet;      // 电网允许放电电流

    uint16_t PV1PowerAim;              // PV1目标功率
    uint16_t PV2PowerAim;              // PV2目标功率
    uint16_t PV3PowerAim;              // PV3目标功率
    uint16_t PV4PowerAim;             // PV4目标功率
    uint16_t GridEnhanceEn;            // 电网增强模式使能
    uint16_t ATETest;                  // ATE测试标志
    uint16_t PVSourceType;             // PV源类型0-PV;1-直流源
    uint16_t OpenLoopEn;               // 逆变开环使能
    uint16_t BATPackNum;               // 电池包数量
    uint16_t GridOnEn;              // 并网使能 0-离网 1-并网
    uint16_t ParallelEn;            // 并联使能
	uint16_t MicroInvterPowerMax;   // 微逆最大额定功率
    uint16_t UV1;                   // 电网欠压保护
    uint16_t UV1Dly;                // 电网欠压保护时间 2s~50s
    uint16_t UV2;                   // 电网欠压保护
    uint16_t UV2Dly;                // 电网欠压保护时间 0.16s~21s
    uint16_t OV1;                   // 电网过压保护
    uint16_t OV1Dly;                // 电网过压保护时间 1s~13s
    uint16_t OV2;                   // 电网过压保护
    uint16_t OV2Dly;                // 电网过压保护时间 fixed 0.16s

    uint16_t UF1;                   // 电网欠频保护
    uint16_t UF1Dly;                // 电网欠频保护时间 2s~50s
    uint16_t UF2;                   // 电网欠频保护
    uint16_t UF2Dly;                // 电网欠频保护时间 0.16s~21s
    uint16_t OF1;                   // 电网过频保护
    uint16_t OF1Dly;                // 电网过频保护时间 1s~13s
    uint16_t OF2;                   // 电网过频保护
    uint16_t OF2Dly;                // 电网过频保护时间 fixed 0.16s
    uint16_t CountryID;             // 认证国家
    uint16_t EmgChagEn;             // 紧急充电使能

    uint16_t ReConnectDly;          // 并网重连时间
    uint16_t BatState;              // 电池包状态
    ARM_FAULT_U Fault;              // ARM 故障字
    ARM_ENABLE_U ARMEnable;         // ARM 使能
    ARM_AD_COEFF_T ADCoeff;         // AD 标定系数
}ARM_WRITE_DATA;


typedef struct
{
    //--------------------------------------------------------------
    uint16_t PV1OverCurr:1;    // 0     PV1 输入过流
    uint16_t PV2OverCurr:1;    // 1     PV2 输入过流
    uint16_t PV3OverCurr:1;    // 2     PV3 输入过流
    uint16_t PV1OverVolt:1;    // 3     PV1 输入过压

    uint16_t PV2OverVolt:1;    // 4     PV2 输入过压
    uint16_t PV3OverVolt:1;    // 5     PV3 输入过压
    uint16_t PV1ISO:1;         // 6     PV1 ISO 故障
    uint16_t PV2ISO:1;         // 7     PV2 ISO 故障

    uint16_t PV3ISO:1;         // 8     PV3 ISO 故障
    uint16_t PV1OCVH:1;        // 9     PV 硬件过流故障
    uint16_t PV2OCVH:1;        // 10    PV 硬件过流故障
    uint16_t PV3OCVH:1;        // 11    PV 硬件过流故障

    uint16_t GFCIHardError:1;       // 12
    uint16_t GFCI:1;           // 13
    uint16_t PhaseErr:1;       // 14    相序错误
    uint16_t FanFault:1;       // 15    风机故障
    //---------------------------------------------------------------
}PCS_FAULT2_T;
typedef union
{
    uint16_t all;
    PCS_FAULT2_T bit;
}PCS_FAULT2_U;

// ----------------------ARM MODBUS 读寄存器---------------------------
typedef struct
{
    uint16_t ARMEnable;             // 使能控制状态
    uint16_t RInvState;             // R 相逆变状态
    uint16_t UrINV;                 // R 相逆变电压
    uint16_t IrINV;                 // R 相逆变电流
    int16_t  PrINV;                 // R 相逆变功率
    uint16_t UrLoad;                // R 相负载电压
    uint16_t IrLoad;                // R 相负载电流
    int16_t  PrLoad;                // R 相负载功率
    uint16_t UrGrid;                // R 相电网电压
    uint16_t IrGrid;                // R 相电网电流
    int16_t  PrGrid;                // R 相电网功率

    uint16_t SInvState;             // S 相逆变状态
    uint16_t UsINV;                 // S 相逆变电压
    uint16_t IsINV;                 // S 相逆变电流
    int16_t  PsINV;                 // S 相逆变功率
    uint16_t UsLoad;                // S 相负载电压
    uint16_t IsLoad;                // S 相负载电流
    int16_t  PsLoad;                // S 相负载功率
    uint16_t UsGrid;                // S 相电网电压
    uint16_t IsGrid;                // S 相电网电流
    int16_t  PsGrid;                // S 相电网功率

    uint16_t InvFreq;               // 逆变器频率,并网时等于电网频率,离网时等于系统频率
    uint16_t GridFreq;              // 电网频率,实测数据

    uint16_t PCSErr1;               // PCS 故障信息1
    PCS_FAULT2_U PCSErr2;           // PCS 故障信息2
    uint16_t PCSErr3;               // PCS 故障信息3
    uint16_t PCSErr4;               // PCS 故障信息4
    uint16_t PCSAlarm1;             // PCS 告警信息1

    uint16_t VerionH;               // 版本信息高字节
    uint16_t VerionL;               // 版本信息低字节

    int16_t MaxPowerSetR;           // R 允许最大输出功率
    int16_t MaxPowerSetS;           // S 允许最大输出功率

    int16_t BusVolt;                //总母线电压
    int16_t Bus1Volt;               //+母线电压
    int16_t Bus2Volt;               //-母线电压

    int16_t UPV1;                   // PV1电压
    int16_t IPV1;                   // PV1电流
    int16_t PPV1;                   // PV1功率
    int16_t UPV2;                   // PV2电压
    int16_t IPV2;                   // PV2电流
    int16_t PPV2;                   // PV2功率
    int16_t UPV3;                   // PV3电压
    int16_t IPV3;                   // PV3电流
    int16_t PPV3;                   // PV3功率
    int16_t UPV4;                   // PV3电压
    int16_t IPV4;                   // PV3电流
    int16_t PPV4;                   // PV3功率

    uint16_t StandbyP;              // 待机功耗
//    uint16_t pwmEn;                // S 相负载电压

    union DSP_WRITE_REG DSPWrite_Flag;

    float DCCurr1Rms;
    float DCCurr2Rms;

    uint16_t SOC;                 //
}ARM_READ_DATA;




extern ARM_WRITE_DATA ArmWriteData;
extern ARM_READ_DATA ArmReadData;

/******************************************************************/
// Function declaration
/******************************************************************/
void InitModbusTables(void);
uint16_t ReadRegisterTables(uint16_t addr);
void WriteRegisterTables(int16_t data, uint16_t addr);
uint16_t ReadRegisterDebug(uint16_t addr);
void WriteRegisterDebug(int16_t data, uint16_t addr);
#endif


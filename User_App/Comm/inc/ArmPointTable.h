/*
 * ArmPointTable.h
 *
 * ARM（SCI-A）通讯点表的镜像结构体。协议字段与内部变量彻底解耦：
 *   - 点表的值指针一律指向 stArmMirror，不直接指向业务变量；
 *   - DataExchangeRx() 把镜像里的命令搬到内部，DataExchangeTx() 反向搬运。
 * 三相命名统一 R / S / T；功率量有符号，逆变输出为正、整流吸收为负。
 */
#ifndef ARM_POINT_TABLE_H
#define ARM_POINT_TABLE_H

#include "F2837xS_device.h"
#include "ModbusSlave.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 一、ARM 下发命令 0~57 */
typedef struct
{
    Uint16  u16RPhDcComp;                      // 0 L1相直流分量 [mA]
    Uint16  u16SPhDcComp;                      // 1 L2相直流分量 [mA]
    Uint16  u16Spare;                          // 2 预留
    Uint16  u16RPhPwrRef;                      // 3 L1相目标有功功率 [电网给设备供电为负，设备给电网馈电为正，W]
    Uint16  u16SPhPwrRef;                      // 4 L2相目标有功功率 [电网给设备供电为负，设备给电网馈电为正，W]
    Uint16  u16Spare_2;                        // 5 预留
    Uint16  u16RPhGridPwrLoadPwr;              // 6 L1相馈网功率与负载功率之和 [W]
    Uint16  u16SPhGridPwrLoadPwr;              // 7 L2相馈网功率与负载功率之和 [W]
    Uint16  u16Spare_3;                        // 8 预留
    Uint16  u16BMSAllowChgVolt;                // 9 BMS允许充电电压 [0.01V]
    Uint16  u16BMSAllowChgPwr;                 // 10 BMS允许充电功率 [W]
    Uint16  u16BMSAllowDisPwr;                 // 11 BMS允许放电功率 [W]
    Uint16  u16OnOffSet;                       // 12 系统开关机设置 [0-OFF，1-ON]
    Uint16  u16FactoryResetSet;                // 13 恢复出厂设置 [1使能，0失能]
    Uint16  u16FreqSet;                        // 14 频率设置 [0-50Hz,1-60Hz]
    Uint16  u16PvRefPwr;                       // 15 光伏目标功率 [W]
    Uint16  u16GridMaxChgCurSet;               // 16 电网最大充电电流设置 [A]
    Uint16  u16GridMaxDisCurSet;               // 17 电网最大放电电流设置 [A]
    Uint16  u16ARMStateWord;                   // 18 ARM状态字 [0位：BMS通讯故障 （0-正常；1-异常）]
    Uint16  u16ARMEnState;                     // 19 ARM使能状态 [0位：并网继电器使能状态]
    Uint16  u16PVParallelEnSet;                // 20 PV并联使能设置 [bit0表示PV1的并联状态，0-OFF，1-ON]
    Uint16  u16RPhLoadVolt;                    // 21 L1相负载电压 [0.1V]
    Uint16  u16SPhLoadVolt;                    // 22 L2相负载电压 [0.1V]
    Uint16  u16Spare_4;                        // 23 预留
    Uint16  u16FanCtrl;                        // 24 风扇档位控制 [0-0%，1-25%，2-50%，3-75%，4-100]
    Uint16  u16VoltSet;                        // 25 电压设置 [0x00:100V低压/220V高压]
    Uint16  u16Set;                            // 26 地区设置 [1-美规；2-日规； 3-欧规；4-澳规； 5-中国；6]
    Uint16  u16RPhGridApparentPwr;             // 27 L1相电网视在功率 [W]
    Uint16  u16SPhGridApparentPwr;             // 28 L2相电网视在功率 [W]
    Uint16  u16Spare_5;                        // 29 预留
    Uint16  u16PVTempLimPwr;                   // 30 PV温度限功率 [W]
    Uint16  u16PV3RefPwr;                      // 31 PV3目标功率 [W]
    Uint16  u16GridModeSet;                    // 32 电网增强模式设置 [0：关闭 1：开启]
    Uint16  u16PV4RefPwr;                      // 33 PV4目标功率 [W]
    Uint16  u16RPhGridRelayNeutralVolt;        // 34 L1相并网继电器中点电压 [0.1V]
    Uint16  u16Spare_6;                        // 35 老化标签 [0-不老化，1-老化]
    Uint16  u16InvParallelGridCtrl;            // 36 逆变器并/离网控制 [0：离网  1：并网]
    Uint16  u16PvSourceSet;                    // 37 光伏输入源设置 [0-MPPT，1-直流源]
    Uint16  u16PV1RefPwr;                      // 38 PV1目标功率 [W]
    Uint16  u16PV2RefPwr;                      // 39 PV2目标功率 [W]
    Uint16  u16SPhGridRelayNeutralVolt;        // 40 L2相并网继电器中点电压 [0.1V]
    Uint16  u16InvOpenLoopEn;                  // 41 逆变开环使能 [0：禁止  1：使能]
    Uint16  u16BMSChg;                         // 42 BMS紧急充电 [1：紧急充电   0：无紧急充电]
    Uint16  u16BattRunState;                   // 43 电池包运行状态 [自检0x44,关机0x33，运行0x22， 待机0x11]
    Uint16  u16PV1CurLimVal;                   // 44 PV1电流限制值 [0.1A]
    Uint16  u16PV2CurLimVal;                   // 45 PV2电流限制值 [0.1A]
    Uint16  u16PV3CurLimVal;                   // 46 PV3电流限制值 [0.1A]
    Uint16  u16PV4CurLimVal;                   // 47 PV4电流限制值 [0.1A]
    Uint16  u16BUSVolt;                        // 48 BUS稳压电压 [0.1V]
    Uint16  u16SNBit0Bit15;                    // 49 SN码 Bit0-Bit15
    Uint16  u16SNBit16Bit31;                   // 50 SN码 Bit16-Bit31
    Uint16  u16SNBit32Bit47;                   // 51 SN码 Bit32-Bit47
    Uint16  u16SNBit48Bit63;                   // 52 SN码 Bit48-Bit63
    Uint16  u16StateEn;                        // 53 状态使能 [bit0: 老化使能  (0-禁止; 1-使能)]
    Uint16  u16GFCI;                           // 54 GFCI标志位 [是否使用上位机设置GFCI系数（0:使用 1：不使用）]
    Uint16  u16BattSOC;                        // 55 电池包SOC [SOC:0%~100%（数值范围：0~65535）]
    Uint16  u16BMS;                            // 56 BMS加热请求 [0：BMS未发出加热请求  1：BMS发出加热请求 （数]
    Uint16  u16BMSBusVolt;                     // 57 BMS母线电压 [数值范围：0~65535 单位：0.1V]
} ST_ARM_CMD;

/* 二、EMS 透传参数 500~566 */
typedef struct
{
    Uint16  u16ModeEn;                         // 500 模式使能 [0-恒功率因数模式 1-电压-无功功率模式 2-有功-无]
    Uint16  u16ConstPfSet;                     // 501 恒功率因数设置 [单位：0.01 数据范围：-32768~+32767]
    Uint16  u16ConstPfSet_2;                   // 502 恒功率因数超前/滞后的设置 [数据范围：0~65535]
    Uint16  u16CalVolt;                        // 503 基准电压 [单位：0.01% 数据范围：0~65535]
    Uint16  u16DynAdjSetCalVoltEn;             // 504 自动调整基准电压使能 [使能/不使能 数据范围：0~65535]
    Uint16  u16AdjSetCalVoltConstTime;         // 505 调整基准电压恒定的时间 [单位：0.1s，范围：300S-5000S 数据范围：0]
    Uint16  u16VoltQV1;                        // 506 电压无功V1 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltQV2;                        // 507 电压无功V2 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltQV3;                        // 508 电压无功V3 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltQV4;                        // 509 电压无功V4 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltQQ1;                        // 510 电压无功Q1 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16VoltQQ2;                        // 511 电压无功Q2 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16VoltQQ3;                        // 512 电压无功Q3 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16VoltQQ4;                        // 513 电压无功Q4 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16OpenLoopRespTime;               // 514 开环响应时间 [单位：0.01s，范围：1s-90s 数据范围：0~65]
    Uint16  u16PQP1;                           // 515 有功无功P1 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQP2;                           // 516 有功无功P2 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQP3;                           // 517 有功无功P3 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQP1Rev;                        // 518 有功无功P1(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQP2Rev;                        // 519 有功无功P2(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQP3Rev;                        // 520 有功无功P3(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ1;                           // 521 有功无功Q1 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ2;                           // 522 有功无功Q2 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ3;                           // 523 有功无功Q3 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ1Rev;                        // 524 有功无功Q1(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ2Rev;                        // 525 有功无功Q2(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16PQQ3Rev;                        // 526 有功无功Q3(取反) [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16ConstQ;                         // 527 恒定的无功功率 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16VoltVarV1;                      // 528 电压有功V1 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltVarV2;                      // 529 电压有功V2 [单位：0.01% 数据范围：0~65535]
    Uint16  u16VoltVarP1;                      // 530 电压有功P1 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16VoltVarP2;                      // 531 电压有功P2 [单位：0.01% 数据范围：-32768~+32767]
    Uint16  u16OpenLoopRespTime2;              // 532 开环响应时间2 [单位：0.01 范围：0.5s-60s 数据范围：0~6]
    Uint16  u16OvProtPt1;                      // 533 过压保护点1 [单位：0.01% 数据范围：0~65535]
    Uint16  u16OvProtPt2;                      // 534 过压保护点2 [单位：0.01% 数据范围：0~65535]
    Uint16  u16OvDelayProtTime1;               // 535 过压延迟保护时间1 [单位：0.01 范围：1.0s-13s 数据范围：0~6]
    Uint16  u16OvDelayProtTime2;               // 536 过压延迟保护时间2 [单位：0.01 范围：固定0.16s 数据范围：0~65]
    Uint16  u16UvProtPt1;                      // 537 欠压保护点1 [单位：0.01% 数据范围：0~65535]
    Uint16  u16UvProtPt2;                      // 538 欠压保护点2 [单位：0.01% 数据范围：0~65535]
    Uint16  u16UvDelayProtTime1;               // 539 欠压延迟保护时间1 [单位：0.01s 范围：21s-50s 数据范围：0~6]
    Uint16  u16UvDelayProtTime2;               // 540 欠压延迟保护时间2 [单位：0.01s 范围：2s-21s 数据范围：0~65]
    Uint16  u16OfProtPt1;                      // 541 过频保护点1 [单位：0.01Hz 数据范围：0~65535]
    Uint16  u16OfProtPt2;                      // 542 过频保护点2 [单位：0.01Hz 数据范围：0~65535]
    Uint16  u16OfDelayProtTime1LowWordByte;    // 543 过频延迟保护时间1低字节 [单位：0.01s 范围：180s-1000s 数据范围：]
    Uint16  u16OfDelayProtTime1HighWordByte;   // 544 过频延迟保护时间1高字节
    Uint16  u16OfDelayProtTime2LowWordByte;    // 545 过频延迟保护时间2低字节 [单位：0.01s 范围：0.16s-1000s 数据范围]
    Uint16  u16OfDelayProtTime2HighWordByte;   // 546 过频延迟保护时间2高字节
    Uint16  u16UfProt1;                        // 547 欠频保护1 [0.01Hz 数据范围：0~65535]
    Uint16  u16UfProt2;                        // 548 欠频保护2 [0.01Hz 数据范围：0~65535]
    Uint16  u16UfDelayProtTime1LowWordByte;    // 549 欠频延迟保护时间1低字节 [单位：0.01s 范围：180s-1000s 数据范围：]
    Uint16  u16UfDelayProtTime1HighWordByte;   // 550 欠频延迟保护时间1高字节
    Uint16  u16UfDelayProtTime2LowWordByte;    // 551 欠频延迟保护时间2低字节 [单位：0.01s 范围：0.16s-1000s 数据范围]
    Uint16  u16UfDelayProtTime2HighWordByte;   // 552 欠频延迟保护时间2高字节
    Uint16  u16OfPOffsetValDBOF;               // 553 过频正偏移值DBOF [单位：0.001Hz 数据范围：0~65535]
    Uint16  u16OfNOffsetValDBUF;               // 554 过频负偏移值DBUF [单位：0.001Hz 数据范围：0~65535]
    Uint16  u16OfPOffsetValCoefKOF;            // 555 过频正偏移值系数KOF [单位：0.001Hz 数据范围：0~65535]
    Uint16  u16OfNOffsetValCoefKUF;            // 556 过频负偏移值系数KUF [单位：0.001Hz 数据范围：0~65535]
    Uint16  u16OpenLoopRespTime3;              // 557 开环响应时间3 [单位：0.01s 范围：0.2s-10s 数据范围：0~]
    Uint16  u16GridEn;                         // 558 并网使能 [使能/禁止 数据范围：0~65535]
    Uint16  u16AllowGridMaxGridVolt;           // 559 允许并网的最大电网电压 [单位：0.01% 数据范围：0~65535]
    Uint16  u16AllowGridMinGridVolt;           // 560 允许并网的最小电网电压 [单位：0.01% 数据范围：0~65535]
    Uint16  u16AllowGridMaxGridFreq;           // 561 允许并网的最大电网频率 [单位：0.01Hz 数据范围：0~65535]
    Uint16  u16AllowGridMinGridFreq;           // 562 允许并网的最小电网频率 [单位：0.01Hz 数据范围：0~65535]
    Uint16  u16GridDelayTime;                  // 563 并网延迟时间 [单位：1s 范围：0-600S 数据范围：0~65535]
    Uint16  u16GridRandDelayTime;              // 564 并网随机延迟时间 [单位：1s 范围：1-1000S 数据范围：0~6553]
    Uint16  u16GridPwrTime;                    // 565 并网功率爬升时间 [单位：1s 范围：1-1000S 数据范围：0~6553]
    Uint16  u16MaxPwr;                         // 566 最大有功功率 [单位：0.01% 数据范围：-32768~+32767]
} ST_ARM_EMS;

/* 三、采样与功率标定 1000~1061 */
typedef struct
{
    Uint16  u16InvRVoltSampleCoef;             // 1000 逆变L1电压采样系数 [0-65535]
    Uint16  u16InvSVoltSampleCoef;             // 1001 逆变L2电压采样系数 [0-65535]
    Uint16  u16Spare_7;                        // 1002 预留
    Uint16  u16InvRPhaseCurSampleCoefHeavy;    // 1003 逆变L1相电流采样系数_重载 [0-65535]
    Uint16  u16InvSPhaseCurSampleCoefHeavy;    // 1004 逆变L2相电流采样系数_重载 [0-65535]
    Uint16  u16Spare_8;                        // 1005 预留
    Uint16  u16GridRVoltSampleCoef;            // 1006 电网L1电压采样系数 [0-65535]
    Uint16  u16GridSVoltSampleCoef;            // 1007 电网L2电压采样系数 [0-65535]
    Uint16  u16Spare_9;                        // 1008 预留
    Uint16  u16LoadRPhaseCurSampleCoef;        // 1009 负载L1相电流采样系数 [0-65535]
    Uint16  u16LoadSPhaseCurSampleCoef;        // 1010 负载L2相电流采样系数 [0-65535]
    Uint16  u16Spare_10;                       // 1011 预留
    Uint16  u16PV1VoltSampleCoef;              // 1012 PV1电压采样系数 [0-65535]
    Uint16  u16PV2VoltSampleCoef;              // 1013 PV2电压采样系数 [0-65535]
    Uint16  u16PV3VoltSampleCoef;              // 1014 PV3电压采样系数 [0-65535]
    Uint16  u16PV4VoltSampleCoef;              // 1015 PV4电压采样系数 [0-65535]
    Uint16  u16PV1CurSampleCoef;               // 1016 PV1电流采样系数 [0-65535]
    Uint16  u16PV2CurSampleCoef;               // 1017 PV2电流采样系数 [0-65535]
    Uint16  u16PV3CurSampleCoef;               // 1018 PV3电流采样系数 [0-65535]
    Uint16  u16PV4CurSampleCoef;               // 1019 PV4电流采样系数 [0-65535]
    Uint16  u16PV1IsoResSampleCoef;            // 1020 PV1绝缘电阻采样系数 [0-65535]
    Uint16  u16PV2IsoResSampleCoef;            // 1021 PV2绝缘电阻采样系数 [0-65535]
    Uint16  u16PV3IsoResSampleCoef;            // 1022 PV3绝缘电阻采样系数 [0-65535]
    Uint16  u16PV4IsoResSampleCoef;            // 1023 PV4绝缘电阻采样系数 [0-65535]
    Uint16  u16BUS1VoltSampleCoefPBUSVoltSampleCoef; // 1024 BUS1电压采样系数（正BUS电压采样系数） [0-65535]
    Uint16  u16BUS2VoltSampleCoefNBUSVoltSampleCoef; // 1025 BUS2电压采样系数（负BUS电压采样系数） [0-65535]
    Uint16  u16BUS3VoltSampleCoef;             // 1026 BUS3电压采样系数 [0-65535]
    Uint16  u16LLC1VoltSampleCoef;             // 1027 LLC1电压采样系数 [0-65535]
    Uint16  u16LLC2VoltSampleCoef;             // 1028 LLC2电压采样系数 [0-65535]
    Uint16  u16LLC3VoltSampleCoef;             // 1029 LLC3电压采样系数 [0-65535]
    Uint16  u16GFCILeakCurSampleCoef;          // 1030 GFCI漏电流采样系数 [0-65535]
    Uint16  u16DirDC1BUCKBOOSTCurSampleCoef;   // 1031 双向DC1（BUCK/BOOST）电流采样系数 [0-65535]
    Uint16  u16DirDC2BUCKBOOSTCurSampleCoef;   // 1032 双向DC2（BUCK/BOOST）电流采样系数 [0-65535]
    Uint16  u16InvRCurSampleCoefLight;         // 1033 逆变L1电流采样系数_轻载 [0-65535]
    Uint16  u16InvSCurSampleCoefLight;         // 1034 逆变L2电流采样系数_轻载 [0-65535]
    Uint16  u16Spare_11;                       // 1035 预留
    Uint16  u16SetCurRCurSampleCoefHeavy;      // 1036 整流L1电流采样系数_重载 [0-65535]
    Uint16  u16SetCurSCurSampleCoefHeavy;      // 1037 整流L2电流采样系数_重载 [0-65535]
    Uint16  u16Spare_12;                       // 1038 预留
    Uint16  u16SetCurRCurSampleCoefLight;      // 1039 整流L1电流采样系数_轻载 [0-65535]
    Uint16  u16SetCurSCurSampleCoefLight;      // 1040 整流L2电流采样系数_轻载 [0-65535]
    Uint16  u16Spare_13;                       // 1041 预留
    Uint16  u16InvRPwrCalPCoefHeavy;           // 1042 逆变L1功率校正系数_重载 [0-65535]
    Uint16  u16InvSPwrCalPCoefHeavy;           // 1043 逆变L2功率校正系数_重载 [0-65535]
    Uint16  u16Spare_14;                       // 1044 预留
    Uint16  u16InvRPwrCalPCoefLight;           // 1045 逆变L1功率校正系数_轻载 [0-65535]
    Uint16  u16InvSPwrCalPCoefLight;           // 1046 逆变L2功率校正系数_轻载 [0-65535]
    Uint16  u16Spare_15;                       // 1047 预留
    Uint16  u16SetCurRPwrCalPCoefHeavy;        // 1048 整流L1功率校正系数_重载 [0-65535]
    Uint16  u16SetCurSPwrCalPCoefHeavy;        // 1049 整流L2功率校正系数_重载 [0-65535]
    Uint16  u16Spare_16;                       // 1050 预留
    Uint16  u16SetCurRPwrCalPCoefLight;        // 1051 整流L1功率校正系数_轻载 [0-65535]
    Uint16  u16SetCurSPwrCalPCoefLight;        // 1052 整流L2功率校正系数_轻载 [0-65535]
    Uint16  u16Spare_17;                       // 1053 预留
    Uint16  u16InvRPhCtrlVoltCalPCoef;         // 1054 逆变L1相控制电压校正系数 [0-65535]
    Uint16  u16InvSPhCtrlVoltCalPCoef;         // 1055 逆变L2相控制电压校正系数 [0-65535]
    Uint16  u16Spare_18;                       // 1056 预留
    Uint16  u16InvRLoadVoltCalPCoef;           // 1057 逆变L1负载电压校正系数 [0-65535]
    Uint16  u16InvSLoadVoltCalPCoef;           // 1058 逆变L2负载电压校正系数 [0-65535]
    Uint16  u16Spare_19;                       // 1059 预留
    Uint16  u16InvRNeutralVoltCalPCoef;        // 1060 逆变L1中点电压校正系数 [0-65535]
    Uint16  u16InvSNeutralVoltCalPCoef;        // 1061 逆变L2中点电压校正系数 [0-65535]
} ST_ARM_CAL;

/* 四、并网门限 IEEE1547 1500~1581 */
typedef struct
{
    Uint16  u16Spare_20;                       // 1500 机器的测试类型 [0：CAT_A； 1：CAT_B 数据范围：0~6553]
    Uint16  u16FaultCal;                       // 1501 机器的故障执行标准 [0:CAT_I; 1:CAT_II; 2:CAT_III]
    Uint16  u16Spare_21;                       // 1502 孤岛类别 [0:UNCAT 1:INT_ISLAND_CAP 2:B]
    Uint16  u16MaxInQLimit;                    // 1503 最大注入无功功率的额定值 [单位：kVar 数据范围：0~65535]
    Uint16  u16MaxQLimit;                      // 1504 最大吸收无功功率的额定值 [单位：kVar 数据范围：0~65535]
    Uint16  u16HoldMode;                       // 1505 支持的模式 [0:DIS;  1:EN; bit0:CONST_PF ]
    Uint16  u16AllowGridMode;                  // 1506 允许并网模式的选择 [默认值:0：ENABLED 0：ENABLED 1：DI]
    Uint16  u16InGridVoltMinVal;               // 1507 进入电网电压的最小值 [单位：0.1V 默认值：110.0V 范围值：[105.]
    Uint16  u16InGridVoltMaxVal;               // 1508 进入电网电压的最大值 [单位：0.1V 默认值：126V [126, 127.2]
    Uint16  u16InGridFreqMinVal;               // 1509 进入电网频率的最小值 [单位：0.01Hz 默认值：59.5Hz 范围值：[59]
    Uint16  u16InGridFreqMaxVal;               // 1510 进入电网频率的最大值 [单位：0.01Hz 默认值：60.1Hz 范围值：[60]
    Uint16  u16RandDelayEn;                    // 1511 软启随机延迟使能 [默认值:0：DISABLED 0：DISABLED 1：]
    Uint16  u16MinDelayTime;                   // 1512 软启最小延迟时间 [单位：1S 默认值：300S 范围值:[0S, 600S]
    Uint16  u16SoftStartTime;                  // 1513 软启动爬坡时间 [单位：1S 默认值：300S 范围值:[1S, 1000]
    Uint16  u16ConstPFValModeEn;               // 1514 恒定PF值模式使能 [默认值:0：DISABLED 0：DISABLED 1：]
    Uint16  u16PfPF;                           // 1515 功率因数PF超前/滞后 [默认值：0:吸收 0：吸收 1：注入 数据范围：0~65]
    Uint16  u16ConstPFVal;                     // 1516 恒定的PF值 [单位：0.01 默认值：吸收(-0.9) 范围值：吸收[]
    Uint16  u16ConstPfPFMinOpenLoopRespTime;   // 1517 达到恒功率因数PF的最小开环响应时间 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16ConstPfPFMaxOpenLoopRespTime;   // 1518 达到恒功率因数PF的最大开环响应时间 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16ConstQEn;                       // 1519 恒定的无功功率使能 [默认值:0：DISABLED 0：DISABLED 1：]
    Uint16  u16ConstQDir;                      // 1520 恒定的无功功率方向选择 [默认值：0:吸收 0：吸收 1：注入 数据范围：0~65]
    Uint16  u16ConstQVal;                      // 1521 恒定的无功功率值 [单位：Var 默认值：-1980 范围值：吸收[-198]
    Uint16  u16ConstQMinOpenLoopRespTime;      // 1522 达到恒定无功功率的最小开环响应时间 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16ConstQMaxOpenLoopRespTime;      // 1523 达到恒定无功功率的最大开环响应时间 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16GridVoltAdjSetQEn;              // 1524 电网电压调整无功功率使能 [默认值：0:DISABLED 0：DISABLED 1：]
    Uint16  u16GridVoltAdjSetQVoltCal;         // 1525 电网电压调整无功功率时的电压基准 [单位：0.1V 默认值：120V 范围值：[114, 1]
    Uint16  u16GridVoltDynAdjSetQEn;           // 1526 电网电压自动调整无功功率使能 [默认值：0:DISABLED 0：DISABLED 1：]
    Uint16  u16GridVoltDynAdjSetQOpenLoopRespTime; // 1527 电网电压自动调整无功功率的开环响应时间 [单位：1S 默认值：300S 范围值:[300,5000]
    Uint16  u16GridVoltQCurveV2Val;            // 1528 电网电压与无功功率曲线中V2值 [单位：0.1V 默认值：117.6V 范围值：[110.]
    Uint16  u16GridVoltQCurveQ2Val;            // 1529 电网电压与无功功率曲线中Q2值 [单位：Var 默认值：0 范围值：[-1980, 198]
    Uint16  u16GridVoltQCurveV3Val;            // 1530 电网电压与无功功率曲线中V3值 [单位：0.1V 默认值：122.4V 范围值：[114V]
    Uint16  u16GridVoltQCurveQ3Val;            // 1531 电网电压与无功功率曲线中Q3值 [单位：Var 默认值：0 范围值：[-1980, 198]
    Uint16  u16GridVoltQCurveV1Val;            // 1532 电网电压与无功功率曲线中V1值 [单位：0.1V 默认值：110.4V 范围值：[92.4]
    Uint16  u16GridVoltQCurveQ1Val;            // 1533 电网电压与无功功率曲线中Q1值 [单位：Var 默认值：1980 范围值：[0, 1980]
    Uint16  u16GridVoltQCurveV4Val;            // 1534 电网电压与无功功率曲线中V4值 [单位：0.1V 默认值：129.6V 范围值：[116.]
    Uint16  u16GridVoltQCurveQ4Val;            // 1535 电网电压与无功功率曲线中Q4值 [单位：Var 默认值：-1980 范围值：[-1980,]
    Uint16  u16QVCurveVoltOpenLoopRespTime;    // 1536 Q(V)曲线中电压开环响应时间 [单位：1S 默认值：5S 范围值：[1S,5000S] ]
    Uint16  u16QPPwrAdjSetQEn;                 // 1537 Q(P)有功功率调整无功功率使能 [默认值：0:DISABLED 0：DISABLED 1：]
    Uint16  u16QPCurveMaxOutPwr;               // 1538 Q(P)曲线中最大的输出有功功率 [单位：1W 默认值：4500W 范围：[2250W, 4]
    Uint16  u16QPCurveOutPwr;                  // 1539 Q(P)曲线中中等大的输出有功功率 [单位：1W 默认值：2250W 范围值：[1800W, ]
    Uint16  u16QPCurveMinOutPwr;               // 1540 Q(P)曲线中最小的输出有功功率 [单位：1W 默认值：900W 范围值：[0W, 3150]
    Uint16  u16QPCurveMinLoadPwr;              // 1541 Q(P)曲线中最小的负载有功功率 [单位：1W 默认值：0W 范围值：[-4500W,-31]
    Uint16  u16QPCurveLoadPwr;                 // 1542 Q(P)曲线中中等大的负载有功功率 [单位：1W 默认值：-2250 范围值：[-3600W,]
    Uint16  u16QPCurveMaxLoadPwr;              // 1543 Q(P)曲线中最大的负载有功功率 [单位：1W 默认值：-4500 范围值：[-4500W,]
    Uint16  u16QPCurveMaxOutQ;                 // 1544 Q(P)曲线中最大的输出无功功率 [单位：Var 默认值：-1980 范围值：[-1980,]
    Uint16  u16QPCurveOutQ;                    // 1545 Q(P)曲线中中等大的输出无功功率 [单位：Var 默认值：0 [-1980, 1980] 数]
    Uint16  u16QPCurveMinOutQ;                 // 1546 Q(P)曲线中最小的输出无功功率 [单位：Var 默认值：0 [-1980, 1980] 数]
    Uint16  u16QPCurveMaxLoadQ;                // 1547 Q(P)曲线中最大的负载无功功率 [单位：Var 默认值：0 范围值：[-1980, 198]
    Uint16  u16QPCurveLoadQ;                   // 1548 Q(P)曲线中中等大的负载无功功率 [单位：Var 默认值：0 范围值：[-1980, 198]
    Uint16  u16QPCurveMinLoadQ;                // 1549 Q(P)曲线中最小的负载无功功率 [单位：Var 默认值：1980 范围值：[-1980, ]
    Uint16  u16QPOpenLoopRespTimeMinVal;       // 1550 Q(P)开环响应时间最小值 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16QPOpenLoopRespTimeMaxVal;       // 1551 Q(P)开环响应时间最大值 [单位：1S 默认值：10S 数据范围：0~65535]
    Uint16  u16PVModeEn;                       // 1552 P(V)模式使能 [默认值：0:DISABLED 0：DISABLED 1：]
    Uint16  u16PVCurvePwrLowStartVoltV1Max;    // 1553 P(V)曲线中功率降低时的启动电压V1上限 [单位：0.1V 默认值：127.2V 范围值：[126V]
    Uint16  u16PVCurvePwrLowStartVoltV1MaxValRespP1Pwr; // 1554 P(V)曲线中功率降低时的启动电压V1上限值对应的P1有功功率 [单位：1W 默认值：4500W 范围值：[4500W, ]
    Uint16  u16PVCurvePwrLowMaxVoltV2;         // 1555 P(V)曲线中功率降低时的上限电压V2 [单位：0.1V 默认值：132V 范围值：[127.2V]
    Uint16  u16PVCurveV2PtRespOutPwrP2Val;     // 1556 P(V)曲线中V2点对应的输出有功功率P2值 [单位：1W 默认值：0 范围值：[0W, 4500W] ]
    Uint16  u16PVCurveV2PtRespLoadPwrP2Val;    // 1557 P(V)曲线中V2点对应的负载有功功率P2值 [单位：1W 默认值：0W 范围：[-4500W,0W] ]
    Uint16  u16PVCurveOpenLoopRespTime;        // 1558 P(V)曲线的开环响应时间 [单位：0.01S 默认值：10S 范围值：[0.5S, ]
    Uint16  u16OvProtPt2_2;                    // 1559 过压保护点2 [单位：0.1V 默认值：144V 范围值：[144V,1]
    Uint16  u16OvProtPt2HoldContTime;          // 1560 过压保护点2持续的时间 [单位：0.01S 默认值：0.16S 范围值：[0.16]
    Uint16  u16OvProtPt1_2;                    // 1561 过压保护点1 [单位：0.1V 默认值：132V 范围值：[132V, ]
    Uint16  u16OvProtPt1HoldContTime;          // 1562 过压保护点1持续的时间 [单位：1S 默认值：13S 范围值：[1S, 13S] ]
    Uint16  u16UvProtPt1_2;                    // 1563 欠压保护点1 [单位：0.1V 默认值：105.6V 范围值：[0V, ]
    Uint16  u16UvProtPt1HoldContTime;          // 1564 欠压保护点1持续的时间 [单位：1S 默认值：21S 范围值：[2S, 50S] ]
    Uint16  u16UvProtPt2_2;                    // 1565 欠压保护点2 [单位：0.1V 默认值：60V 范围值：[0V, 60V]
    Uint16  u16UvProtPt2HoldContTime;          // 1566 欠压保护点2持续的时间 [单位：0.01S 默认值：2S 范围值：[[0.16S,]
    Uint16  u16OfProtPt2_2;                    // 1567 过频保护点2 [单位：0.01Hz 默认值：62Hz 范围值：[61.8]
    Uint16  u16OfProtPt2HoldContTime;          // 1568 过频保护点2持续时间 [单位：0.01S 默认值：0.16S 范围值：[0.16]
    Uint16  u16OfProtPt1_2;                    // 1569 过频保护点1 [单位：0.01Hz 默认值：61.2Hz 范围值：[61]
    Uint16  u16OfProtPt1HoldContTime;          // 1570 过频保护点1持续时间 [单位：1S 默认值：300S 范围值：[180S, 10]
    Uint16  u16UfProtPt1;                      // 1571 欠频保护点1 [单位：0.01Hz 默认值：58.5Hz 范围值：[50]
    Uint16  u16UfProtPt1HoldContTime;          // 1572 欠频保护点1持续时间 [单位：1S 默认值：300S 范围值：[180S, 10]
    Uint16  u16UfProtPt2;                      // 1573 欠频保护点2 [单位：0.01Hz 默认值：56.5Hz 范围值：[50]
    Uint16  u16UfProtPt2HoldContTime;          // 1574 欠频保护点2持续时间 [单位：0.01S 默认值：0.16S 范围值：[0.16]
    Uint16  u16PFModeEn;                       // 1575 P(F)模式使能 [默认值：0:DISABLED 0：DISABLED 1：]
    Uint16  u16PFOfOffsetQty;                  // 1576 P(F)过频偏移量 [单位：0.001Hz 默认值：0.036Hz 范围值：[]
    Uint16  u16PFUfOffsetQty;                  // 1577 P(F)欠频偏移量 [单位：0.001Hz 默认值：0.017Hz 范围值：[]
    Uint16  u16PFOfOffsetQtyCoef;              // 1578 P(F)过频偏移量系数 [单位：0.01 默认值：0.07 范围值：[0.02, ]
    Uint16  u16PFUfOffsetQtyCoef;              // 1579 P(F)欠频偏移量系数 [单位：0.01 默认值：0.02 范围值：[0.02, ]
    Uint16  u16PFCurveOpenLoopRespTime;        // 1580 P(F)曲线开环响应时间 [单位：0.01S 默认值：5S 范围值：[0.2S, 1]
    Uint16  u16Spare_22;                       // 1581 非计划性孤岛现象 [默认值：0:ENABLED 0：ENABLED 1：DI]
} ST_ARM_GRID;

/* 五、DSP 预留 2000~2003 */
typedef struct
{
    Uint16  u16Spare_23;                       // 2000 协议表格索引号 [1~65535]
    Uint16  u16VarQty1;                        // 2001 变量1
    Uint16  u16VarQty2;                        // 2002 变量2
    Uint16  u16VarQty3;                        // 2003 变量3
} ST_ARM_SPARE;

/* 六、两级保护定值 3000~3040 */
typedef struct
{
    Uint16  u16HighEn;                         // 3000 高级功能使能 [0~1，0-65535]
    Uint16  u16Spare_24;                       // 3001 国家索引号 [0-德国；1-西班牙；2-法国；3-欧规；4-荷兰；5-]
    Uint16  u16UvLevel1Volt;                   // 3002 电网欠压LEVEL1保护值 [[1000,1099]，0.1V，0-65535]
    Uint16  u16UvLevel1Time;                   // 3003 电网欠压LEVEL1触发时间 [[1,2200]，10ms，0-65535]
    Uint16  u16UvLevel2Volt;                   // 3004 电网欠压LEVEL2保护值 [[550,650]，0.1V，0-65535]
    Uint16  u16UvLevel2Time;                   // 3005 电网欠压LEVEL2触发时间 [[1,300]，10ms，0-65535]
    Uint16  u16PfSet;                          // 3006 超前和滞后功率因数设置 [[-1.00,-0.90]*100，[0.90,1.00]
    Uint16  u16RegionSet;                      // 3007 区域设置 [区域为澳大利亚时：0-Australia A;1-Aus]
    Uint16  u16ModeEn_2;                       // 3008 模式使能 [0-invalid;1-enable;2-disable]
    Uint16  u16PwrVar;                         // 3009 每分钟功率变化比例 [0~100%，1%，0-65535]
    Uint16  u16OvAvgVolt;                      // 3010 均值过压保护值 [[1350,1450]，0.1V，0-65535]
    Uint16  u16OvLevel1Volt;                   // 3011 电网高压LEVEL1保护值 [[1330,1500]，0.1V，0-65535]
    Uint16  u16OvLevel1Time;                   // 3012 电网高压LEVEL1保护时间 [[1,1500]，10ms，0-65535]
    Uint16  u16OvLevel2Volt;                   // 3013 电网高压LEVEL2保护值 [[1400,1600]，0.1V，0-65535]
    Uint16  u16OvLevel2Time;                   // 3014 电网高压LEVEL2保护时间 [[1,20]，10ms，0-65535]
    Uint16  u16QP;                             // 3015 无功占额定有功比例 [0~100%，1%，0-65535]
    Uint16  u16VoltWattRespModeParamVw1Set;    // 3016 Volt–watt响应模式的参数Vw1设置 [235V-255V，O.1V，Vw1<Vw2，0-655]
    Uint16  u16VoltWattRespModeParamVw2Set;    // 3017 Volt–watt响应模式的参数Vw2设置 [240V-265V，0.1V，Vw1<Vw2，0-655]
    Uint16  u16Prot;                           // 3018 保留
    Uint16  u16UfLevel1Freq;                   // 3019 电网欠频LEVEL1保护值 [[5800,5900]，0.01Hz，0-65535]
    Uint16  u16UfLevel1Time;                   // 3020 电网欠频LEVEL1保护时间 [[1,50000]，10ms，0-65535]
    Uint16  u16UfLevel2Freq;                   // 3021 电网欠频LEVEL2保护值 [[5000,5700]，0.01Hz，0-65535]
    Uint16  u16UfLevel2Time;                   // 3022 电网欠频LEVEL2保护时间 [[1,20]，10ms，0-65535]
    Uint16  u16VvarandvoltwattRespModeParamVv1Set; // 3023 V-var and volt-watt响应模式的参数Vv1设置 [180V-230V，O.1V，Vv1<Vv2<Vv3<V]
    Uint16  u16VvarandvoltwattRespModeParamVv2Set; // 3024 V-var and volt-watt响应模式的参数Vv2设置 [180V-230V，O.1V，Vv1<Vv2<Vv3<V]
    Uint16  u16VvarandvoltwattRespModeParamVv3Set; // 3025 V-var and volt-watt响应模式的参数Vv3设置 [230V-265V，O.1V，Vv1<Vv2<Vv3<V]
    Uint16  u16VvarandvoltwattRespModeParamNVv4Set; // 3026 V-var and volt-watt响应模式的参数 Vv4设置 [230V-265V，O.1V，Vv1<Vv2<Vv3<V]
    Uint16  u16OfLevel1Freq;                   // 3027 电网过频LEVEL1保护值 [[6100,6200]，0.01Hz，0-65535]
    Uint16  u16OfLevel1Time;                   // 3028 电网过频LEVEL1保护时间 [[1,40000]，10ms，0-65535]
    Uint16  u16OfLevel2Freq;                   // 3029 电网过频LEVEL2保护值 [[6180,6600]，0.01Hz，0-65535]
    Uint16  u16OfLevel2Time;                   // 3030 电网过频LEVEL2保护时间 [[1,20]，10ms，0-65535]
    Uint16  u16Prot_2;                         // 3031 保留 [0-65535]
    Uint16  u16Prot_3;                         // 3032 保留 [0-65535]
    Uint16  u16Prot_4;                         // 3033 保留 [0-65535]
    Uint16  u16Prot_5;                         // 3034 保留 [0-65535]
    Uint16  u16ReconnectLevel1;                // 3035 电网重连电压下限值 [[1050,1150]，0.1V，0-65535]
    Uint16  u16ReconnectLevel1Time;            // 3036 电网重连电压上限值 [[1200,1300]，0.1V，0-65535]
    Uint16  u16ReconnectLevel2;                // 3037 电网重连频率下限值 [[5920,5980]，0.01Hz，0-65535]
    Uint16  u16ReconnectLevel2Time;            // 3038 电网重连频率上限值 [[6000,6100]，0.01Hz，0-65535]
    Uint16  u16GridErrReconnectTime;           // 3039 电网异常重连时间 [[60,40000]，1S，0-65535]
    Uint16  u16Recover;                        // 3040 恢复默认 [1，0-65535]
} ST_ARM_PROTSET;

/* 七、状态与故障发布 5000~5065 */
typedef struct
{
    Uint16  u16StateEnSet;                     // 5000 状态使能设置 [0位：并网继电器使能状态]
    Uint16  u16RPhInvState;                    // 5001 L1相逆变器工作状态 [0-停机;1-离网运行;2-旁路；3-并网;4-用电网充]
    Uint16  u16RPhInvVolt;                     // 5002 L1相逆变器电压 [0.1V]
    Uint16  u16RPhInvCur;                      // 5003 L1相逆变器电流 [0.1A]
    int16   i16RPhInvPwr;                      // 5004 L1相逆变器功率 [W]
    Uint16  u16RPhGridVolt;                    // 5007 L1相电网电压 [0.1V]
    Uint16  u16SPhInvState;                    // 5010 L2相逆变器工作状态 [0-停机;1-离网运行;2-旁路；3-并网;4-用电网充]
    Uint16  u16SPhInvVolt;                     // 5011 L2相逆变器电压 [0.1V]
    Uint16  u16SPhInvCur;                      // 5012 L2相逆变器电流 [0.1A]
    int16   i16SPhInvPwr;                      // 5013 L2相逆变器功率 [W]
    Uint16  u16SPhGridVolt;                    // 5016 L2相电网电压 [0.1V]
    Uint16  u16TPhInvState;                    // 5019 T相逆变器工作状态
    Uint16  u16TPhInvVolt;                     // 5020 T相逆变器电压
    Uint16  u16TPhInvCur;                      // 5021 T相逆变器电流
    int16   i16TPhInvPwr;                      // 5022 T相逆变器功率
    Uint16  u16TPhGridVolt;                    // 5025 T相电网电压
    Uint16  u16InvFreq;                        // 5028 逆变器频率 [0.01hz]
    Uint16  u16GridFreq;                       // 5029 电网频率 [0.01hz]
    Uint16  u16PCSFault1;                      // 5030 PCS故障信息1 [0位：BUS1过压(EP600:BUS过压)]
    Uint16  u16PCSFault2;                      // 5031 PCS故障信息2 [0位：PV1过流]
    Uint16  u16PCSFault3;                      // 5032 PCS故障信息3 [0位：零漂异常]
    Uint16  u16PCSFault4;                      // 5033 PCS故障信息4 [0位：PV严重高压]
    Uint16  u16PCSAlarm1;                      // 5034 PCS告警信息1 [0位：电网电压过高]
    Uint16  u16DSPVersionH;                    // 5035 DSP版本号-H [如400012表示4000.12]
    Uint16  u16DSPVersionL;                    // 5036 DSP版本号-L
    Uint16  u16BusVolt;                        // 5037 总母线电压 [0.1V]
    Uint16  u16BusVoltP;                       // 5038 正母线电压 [0.1V]
    Uint16  u16RPhMaxPwrOut;                   // 5039 L1相允许最大功率输出 [W]
    Uint16  u16SPhMaxPwrOut;                   // 5040 L2相允许最大功率输出 [W]
    Uint16  u16BusVoltN;                       // 5041 负母线电压 [0.1V]
    Uint16  u16PV1VoltIn;                      // 5042 PV1输入电压 [0.1V]
    Uint16  u16PV1CurOut;                      // 5043 PV1输出电流 [0.1A]
    int16   i16PV1OutPwr;                      // 5044 PV1输出功率 [W]
    Uint16  u16PV2VoltIn;                      // 5045 PV2输入电压 [0.1V]
    Uint16  u16PV2CurOut;                      // 5046 PV2输出电流 [0.1A]
    int16   i16PV2OutPwr;                      // 5047 PV2输出功率 [W]
    Uint16  u16PV3VoltIn;                      // 5048 PV3输入电压 [0.1V]
    Uint16  u16PV3CurOut;                      // 5049 PV3输出电流 [0.1A]
    int16   i16PV3OutPwr;                      // 5050 PV3输出功率 [W]
    Uint16  u16PV4VoltIn;                      // 5051 PV4输入电压 [0.1V]
    Uint16  u16PV4CurOut;                      // 5052 PV4输出电流 [0.1V]
    int16   i16PV4OutPwr;                      // 5053 PV4输出功率 [W]
    Uint16  u16PCSFault5;                      // 5054 PCS故障信息5 [0位：逆变器并机状态异常]
    Uint16  u16ParallelNum;                    // 5055 逆变器并机数量 [PCS]
    Uint16  u16BUSRefVolt;                     // 5056 BUS目标电压 [0.1V]
    Uint16  u16InvRPhLoadVolt;                 // 5057 逆变L1相负载电压 [0.1V]
    Uint16  u16InvSPhLoadVolt;                 // 5058 逆变L2相负载电压 [0.1V]
    Uint16  u16Spare_25;                       // 5059 预留 [0.1V]
    Uint16  u16DSPCtrlBits;                    // 5060 DSP控制位 [0位：0-逆变器未控母线电压,1-逆变器在控母线电压]
    Uint16  u16RNeutralVolt;                   // 5061 L1中点电压 [0.1V]
    Uint16  u16SNeutralVolt;                   // 5062 L2中点电压 [0.1V]
    Uint16  u16Spare_26;                       // 5063 预留 [0.1V]
    Uint16  u16GFCILeakCur;                    // 5065 GFCI漏电流 [mA]
} ST_ARM_STATUS;

/* 八、SunSpec/DER 映射 5500~5510 */
typedef struct
{
    Uint16  u16InvVersionLowWordByte;          // 5500 逆变器硬件版本号低字节 [ASCII型数据 数据范围：0~65535]
    Uint16  u16InvVersionHighWordByte;         // 5501 逆变器硬件版本号高字节 [ASCII型数据 数据范围：0~65535]
    int16   i16Pwr;                            // 5502 有功功率 [单位：W 数据范围：-32768~+32767]
    int16   i16Q;                              // 5503 无功功率 [单位：vars 数据范围：-32768~+32767]
    Uint16  u16Freq;                           // 5504 频率 [单位：0.01Hz 数据范围：0~65535]
    Uint16  u16State;                          // 5505 操作状态 [0 = Not applicable/Unknown]
    Uint16  u16State_2;                        // 5506 连接状态 [0 = Connected               ]
    Uint16  u16AlarmState;                     // 5507 告警状态 [BitMap 0 = DER_FAULT_OVER_CU]
    Uint16  u16InvState;                       // 5508 逆变器状态 [0 - N/A                  1 -]
    Uint16  u16CtrlState;                      // 5509 控制的状态 [0 – local control 1 – remote]
    Uint16  u16BattState;                      // 5510 电池状态 [0 – storage charging        ]
} ST_ARM_DER;

/* 九、测量发布 8000~8032 */
typedef struct
{
    Uint16  u16InvRVolt;                       // 8000 逆变L1电压 [0.1V]
    Uint16  u16InvSVolt;                       // 8001 逆变L2电压 [0.1V]
    Uint16  u16Spare_27;                       // 8002 预留
    Uint16  u16InvRPhaseCur;                   // 8003 逆变L1相电流 [0.1A]
    Uint16  u16InvSPhaseCur;                   // 8004 逆变L2相电流 [0.1A]
    Uint16  u16Spare_28;                       // 8005 预留
    Uint16  u16GridRVolt;                      // 8006 电网L1电压 [0.1V]
    Uint16  u16GridSVolt;                      // 8007 电网L2电压 [0.1V]
    Uint16  u16Spare_29;                       // 8008 预留
    Uint16  u16Spare_30;                       // 8011 预留
    Uint16  u16PV1Volt;                        // 8012 PV1电压 [0.1V]
    Uint16  u16PV2Volt;                        // 8013 PV2电压 [0.1V]
    Uint16  u16PV3Volt;                        // 8014 PV3电压 [0.1V]
    Uint16  u16PV4Volt;                        // 8015 PV4电压 [0.1V]
    Uint16  u16PV1Cur;                         // 8016 PV1电流 [0.1A]
    Uint16  u16PV2Cur;                         // 8017 PV2电流 [0.1A]
    Uint16  u16PV3Cur;                         // 8018 PV3电流 [0.1A]
    Uint16  u16PV4Cur;                         // 8019 PV4电流 [0.1A]
    Uint16  u16BUS1VoltPBUSVolt;               // 8024 BUS1电压（正BUS电压） [0.1V]
    Uint16  u16BUS2VoltNBUSVolt;               // 8025 BUS2电压（负BUS电压） [0.1V]
    Uint16  u16GFCILeakCur_2;                  // 8030 GFCI漏电流 [mA]
} ST_ARM_MEAS;

/* 全部协议字段的镜像，点表只引用这里 */
typedef struct
{
    ST_ARM_CMD     stCmd;
    ST_ARM_EMS     stEms;
    ST_ARM_CAL     stCal;
    ST_ARM_GRID    stGrid;
    ST_ARM_SPARE   stSpare;
    ST_ARM_PROTSET stProtSet;
    ST_ARM_STATUS  stStatus;
    ST_ARM_DER     stDer;
    ST_ARM_MEAS    stMeas;
} ST_ARM_MIRROR;

/* 点表条目数 */
#define ARM_POINT_NUM               (399U)

extern ST_ARM_MIRROR stArmMirror;
extern const ST_MB_REG_ITEM stMbRegItemArm[ARM_POINT_NUM];

#ifdef __cplusplus
}
#endif

#endif /* ARM_POINT_TABLE_H */

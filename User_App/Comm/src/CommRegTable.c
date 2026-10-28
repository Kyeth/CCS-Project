/**
 * @file CommRegTable.c
 * @author yetingheng (004182)
 * @brief 通信点表实现：ARM / 上位机 / TVS 三张 Modbus 点表
 * @version 0.2
 * @date 2026-10-24
 * @copyright Copyright (c) 2026
 *
 * @note 表项字段：{寄存器地址, 数据类型, 读写属性, 变量地址, 下限, 上限}
 *       地址可以不用连续、也不用排序，驱动按"基地址 + 类型宽度"匹配。
 *
 * @note 数组按 XXX_MAX 声明、按 XXX_NUM 使用，所以加行时不需要改数组大小，
 *       只把 CommRegTable.h 里对应的 NUM 宏改成实际行数。
 *
 * @note ======================= 地址规划 =======================
 *       0     ~ 26     软示波器控制块（上位机表）
 *       27    ~ 99     预留
 *       100   ~ 499    上位机调试变量区
 *       500   ~ 10000  ARM 点表（地址待 ARM 侧通信内容确定）
 *       10001 ~ 65535  预留 / TVS（地址待 TVS 侧通信内容确定）
 *
 *       同一个从机（同一条串口）内地址不能重复，否则 MbFindReg() 命中先出现
 *       的那一行；不同串口之间（SCIA / SCID / SCIC）地址可以随便重叠。
 * ================================================================
 */

#include "CommRegTable.h"
#include "PvStateMachine.h"
#include "SoftOsc.h"
#include "EepromParam.h"   /* 保存层诊断量的 Modbus 镜像 */
#include "ProtectTask.h"   /* 硬件保护任务状态量 */
#include "AdcCalib.h"      /* ADC 标定状态量 */
#include "AdcSample.h"     /* 对外采样量 */
#include "AtsProto.h"      /* ATS 旁听镜像 */
#include "AtsCtl.h"        /* ATS 干接点状态 */
#include "SystemStateMachine.h" /* 系统/逆变状态机 */
#include "InvStateMachine.h"
#include "CtrlIsr.h"           /* 发波授权状态机的观测镜像 */

/* 掉电保存示范变量：地址 31 / 32，写属性带 SAVE 位。
 * 这两个变量只是示范，正式点表里任何一个变量都可以照此标记。 */
Uint16  gMbSavedDemoWord  = 1234U;
float32 gMbSavedDemoFloat = 1.5F;

/* ============================ ARM 点表 ============================ */

/* ARM 点表已迁到 ArmPointTable.c（399 项，地址升序，值指针指向 stArmMirror）。
 * 本文件只保留上位机（PC）与 TVS 两张表。 */

/* ============================ TVS 点表 ============================ */
/*
 * TVS 走标准 Modbus，但SCIC 只收不发。
 *   - 收到 06 / 10 写寄存器：照常落到变量上
 *   - 读请求、异常应答：一律不回复
 *
 * 地址等 TVS 侧通信内容确定后再排，现在放一行占位。
 */
Uint16 gTvsRefSlotData[4] = {0U, 0U, 0U, 0U};

const ST_MB_REG_ITEM stMbRegItemTvs[MB_TVS_REG_MAX] =
{
    {20000U, MB_VAL_U16, MB_RW_WRITE, (void *)&gTvsRefSlotData[0], 0, 65535U},
};

/* ============================ 上位机点表 ============================ */
const ST_MB_REG_ITEM stMbRegItemPc[MB_PC_REG_MAX] =
{
    /* ---------- 0~11：12 路选源（写变量的寄存器地址）---------- */
    {0U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[0].u16SrcId,  0, 65535U},
    {1U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[1].u16SrcId,  0, 65535U},
    {2U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[2].u16SrcId,  0, 65535U},
    {3U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[3].u16SrcId,  0, 65535U},
    {4U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[4].u16SrcId,  0, 65535U},
    {5U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[5].u16SrcId,  0, 65535U},
    {6U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[6].u16SrcId,  0, 65535U},
    {7U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[7].u16SrcId,  0, 65535U},
    {8U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[8].u16SrcId,  0, 65535U},
    {9U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[9].u16SrcId,  0, 65535U},
    {10U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[10].u16SrcId,  0, 65535U},
    {11U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[11].u16SrcId,  0, 65535U},

    /* ---------- 12~23：对应通道的放大倍数 ---------- */
    {12U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[0].u16Multiple, 1, 10000U},
    {13U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[1].u16Multiple, 1, 10000U},
    {14U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[2].u16Multiple, 1, 10000U},
    {15U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[3].u16Multiple, 1, 10000U},
    {16U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[4].u16Multiple, 1, 10000U},
    {17U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[5].u16Multiple, 1, 10000U},
    {18U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[6].u16Multiple, 1, 10000U},
    {19U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[7].u16Multiple, 1, 10000U},
    {20U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[8].u16Multiple, 1, 10000U},
    {21U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[9].u16Multiple, 1, 10000U},
    {22U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[10].u16Multiple, 1, 10000U},
    {23U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscChCfg[11].u16Multiple, 1, 10000U},

    /* ---------- 24~26：发送模式 / 单边组别 / 已发帧数 ---------- */
    {24U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscMode,       0, 1U},
    {25U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSoftOscGroupSel,   0, 1U},
    {26U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gSoftOscFrameCnt,   0, 65535U},

    /* ---------- 27~30：掉电保存层诊断（只读镜像）---------- */
    {27U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gEepromParamStatus,     0, 65535U},
    {28U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gEepromParamItemNum,    0, 65535U},
    {29U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gEepromParamWriteCntLo, 0, 65535U},
    {30U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gEepromParamErrCnt,     0, 65535U},

    /* ---------- 31~33：掉电保存示范变量（MB_RW_WRITE_SAVE）---------- */
    {31U, MB_VAL_U16, MB_RW_WRITE_SAVE, (void *)&gMbSavedDemoWord,   0, 65535U},
    {32U, MB_VAL_F32, MB_RW_WRITE_SAVE, (void *)&gMbSavedDemoFloat, -10000, 10000U},

    /* ---------- 33~36：硬件保护任务状态（10kHz 漏桶判定）---------- */
    {33U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16InvOcCnt,    0, 65535U},
    {34U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16ShortCnt,    0, 65535U},
    {35U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16FaultLatch,  0, 65535U},
    {36U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16Tick,        0, 65535U},

    /* ---------- 37~41：半波自适应诊断（只读）---------- */
    {37U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16HalfWaveMode,   0, 1U},
    {38U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stProtectState.u16HalfWaveJudge,  0, 65535U},
    {39U, MB_VAL_I16, MB_RW_READ_ONLY, (void *)&stProtectState.i16InvDc[0], -30000, 30000U},  /* R 相，0.1A */
    {40U, MB_VAL_I16, MB_RW_READ_ONLY, (void *)&stProtectState.i16InvDc[1], -30000, 30000U},  /* S 相，0.1A */
    {41U, MB_VAL_I16, MB_RW_READ_ONLY, (void *)&stProtectState.i16InvDc[2], -30000, 30000U},  /* T 相，0.1A */

    /* ---------- 42~45：ADC 标定状态（只读）---------- */
    {42U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stAdcCalib.u16ZeroState,   0, 65535U},          /* 0待测/1累加/2完成/3超差/4冻结 */
    {43U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stAdcCalib.u16TrackCnt,    0, 65535U},          /* 已累加拍数 */
    {44U, MB_VAL_I16, MB_RW_READ_ONLY, (void *)&stAdcCalib.i16ObsGfciMa, -30000, 30000U},       /* GFCI 电流，mA */
    {45U, MB_VAL_I16, MB_RW_READ_ONLY, (void *)&stAdcCalib.i16ObsBalCur, -30000, 30000U},       /* 平衡电路电流，0.1A */

    /* ---------- 46~49：对外采样量示例（AdcSample，只读，F32）---------- */
    {46U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&stSampleRms.fGridVoltRms[SAMPLE_PH_R], 0, 10000U},   /* 电网 R 相电压有效值 V */
    {47U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&stSampleRms.fInvCurrRms[SAMPLE_PH_R], -10000, 10000U}, /* 逆变 R 相电流有效值 A */
    {48U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&stSampleAvg.fBusVoltAvg200, 0, 10000U},    /* 母线电压 V */
    {49U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&stSampleAvg.fPvVoltAvg8[SAMPLE_PV1], 0, 10000U},    /* PV1 电压 V */

    /* ---------- ATS 旁听镜像（50~57）：ARM<->ATS 链路上的实时量 ---------- */
    {50U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fInFreq, 0, 100U},          /* 输入正序频率 Hz */
    {51U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fOutFreq, 0, 100U},         /* 输出正序频率 Hz */
    {52U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fVoltDiff, -1000, 1000U},   /* 正序电压差 V */
    {53U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fPhaseDiff, -360, 360U},    /* 正序相位差 ° */
    {54U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fInVoltA, 0, 1000U},        /* 输入 A 相电压 V */
    {55U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gAtsInfo.fOutVoltA, 0, 1000U},       /* 输出 A 相电压 V */
    {56U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsInfo.u16FrameCnt, 0, 65535U},    /* ATS 应答帧计数 */
    {57U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsInfo.u16Online, 0, 1U},          /* ATS 在线标志 */
    {58U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsCtl.u16State, 0, 1U},            /* ATS 状态 1:闭合 */
    {59U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsCtl.u16Request, 0, 1U},          /* ATS 期望状态 */
    {60U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsCtl.eActState, 0, 2U},           /* ATS 动作状态 0空闲 1等待 2超时 */
    {61U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gAtsCtl.u16ActCnt, 0, 65535U},       /* ATS 成功动作次数 */

    /* ---------- 状态机（62~67） ---------- */
    {62U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gSysSm.u16State, 0, 9U},             /* 系统状态 0~8 */
    {63U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gSysSm.u16Fault, 0, 127U},           /* 系统故障位 */
    {64U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gInvSm.u16State, 0, 7U},             /* 逆变状态 0~7 */
    {65U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gInvSm.u16Fault, 0, 31U},            /* 逆变故障位 */
    {66U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSysSm.u16StartCmd, 0, 1U},          /* 开机命令 1=开机 */
    {67U, MB_VAL_U16, MB_RW_WRITE, (void *)&gSysSm.u16ResetCmd, 0, 1U},          /* 复位命令 1=复位 */
    {68U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.u16RunMask, 0, 15U},          /* PV 运行通道掩码 */
    {69U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.u16MpptMask, 0, 15U},         /* PV MPPT 通道掩码 */
    {70U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[0].u16DutyPermil, 0, 1000U},  /* PV1 占空比‰ */
    {71U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[1].u16DutyPermil, 0, 1000U},  /* PV2 占空比‰ */
    {72U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[2].u16DutyPermil, 0, 1000U},  /* PV3 占空比‰ */
    {73U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[3].u16DutyPermil, 0, 1000U},  /* PV4 占空比‰ */

    /* ---------- PV 电压指令与功率（74~81，10kHz 环路的观测量） ---------- */
    {74U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[0].fVset, 0, 450U},   /* PV1 电压指令 V */
    {75U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[1].fVset, 0, 450U},   /* PV2 电压指令 V */
    {76U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[2].fVset, 0, 450U},   /* PV3 电压指令 V */
    {77U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[3].fVset, 0, 450U},   /* PV4 电压指令 V */
    {78U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[0].fPower, 0, 15000U}, /* PV1 功率 W */
    {79U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[1].fPower, 0, 15000U}, /* PV2 功率 W */
    {80U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[2].fPower, 0, 15000U}, /* PV3 功率 W */
    {81U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&gPvSm.stCh[3].fPower, 0, 15000U}, /* PV4 功率 W */

    /* ---------- 发波授权观测（82~84，20kHz 控制中断裁定） ---------- */
    {82U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlObs.u16AuthState, 0, 3U},        /* 0=OFF 1=WAIT 2=ON 3=SOFT_OFF */
    {83U, MB_VAL_F32, MB_RW_READ_ONLY, (void *)&stCtrlObs.fModActual, 0, 1U},        /* 实际下发调制比 */

    /* ---------- 84~89：控制中断执行余量（只读，单位 = 200MHz 周期数）---------- */
    {84U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u16LastCnt,   0, 65535U},  /* 上一拍占用周期数 */
    {85U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u16MaxCnt,    0, 65535U},  /* 峰值占用周期数 */
    {86U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u16MinCnt,    0, 65535U},  /* 谷值占用周期数 */
    {87U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u16OverCnt,   0, 65535U},  /* 超预算拍数 */
    {88U, MB_VAL_U32, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u32AccCnt,    0, 4294967295U}, /* 累计占用周期数 */
    {89U, MB_VAL_U16, MB_RW_READ_ONLY, (void *)&stCtrlMargin.u16SampleCnt, 0, 65535U},  /* 采样拍数 */

};

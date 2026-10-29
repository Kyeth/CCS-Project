/**
 * @file DataExchange.c
 * @author yetingheng (004182)
 * @brief 点表镜像与内部变量交换：发布段上行、命令段下行
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 批 0.7：骨架 + 已确定来源的发布项。
 * 未接通的字段留 TODO，批 0.8 / 0.9 逐个补齐，不在这里猜数据源。
 */
#include "DataExchange.h"
#include "ArmPointTable.h"
#include "AdcSample.h"
#include "ProtectFault.h"
#include "InvStateMachine.h"

/* 版本与固定发布量 */
#define DX_FW_VERSION_H     (100U)      // 5035 DSP 版本号高字
#define DX_FW_VERSION_L     (0U)        // 5036 DSP 版本号低字
#define DX_HW_VERSION       (0x0100U)   // 5500/5501 硬件版本，低/高字节
#define DX_PHASE_MAX_PWR    (5000U)     // 5039/5040 单相允许最大功率 [W]
#define DX_PARALLEL_NUM     (1U)        // 5055 并机数量
#define DX_BUS_REF_VOLT     (700.0f)    // 5056 BUS 目标电压 [V]，批 1.0 接电压环
#define DX_DER_CONNECTED    (1U)        // 5506 连接状态：1 = Connected
#define DX_DER_LOCAL_CTRL   (0U)        // 5509 控制状态：0 = local

/* 协议缩放系数 */
#define DX_VOLT_SCALE       (10.0f)     // 0.1 V
#define DX_CUR_SCALE        (10.0f)     // 0.1 A
#define DX_FREQ_SCALE       (100.0f)    // 0.01 Hz
#define DX_GFCI_SCALE       (1000.0f)   // mA

static Uint16 DxSatU16(float32 fVal);   // 限幅到 0~65535
static int16  DxSatI16(float32 fVal);   // 限幅到 -32768~32767
static Uint16 DxInvRunState(void);      // 逆变器工作状态映射

/**
 * @brief 初始化镜像
 */
void DataExchangeInit(void)
{
    Uint16 *pu16Dst = (Uint16 *)&stArmMirror;
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < (Uint16)(sizeof(ST_ARM_MIRROR) / sizeof(Uint16)); u16Idx++)
    {
        pu16Dst[u16Idx] = 0U;
    }
}

/**
 * @brief ARM 命令搬运，批 0.9 实现
 */
void DataExchangeRx(void)
{
    // TODO(批 0.9)：开关机、并离网、功率给定、保护定值
}

/**
 * @brief 内部状态搬到发布镜像
 */
void DataExchangeTx(void)
{
    Uint16 u16InvRunState;

    /* ---- 5000~5029 状态与故障 ---- */
    stArmMirror.stStatus.u16StateEnSet  = 0U;   // TODO(批0.9)：并网继电器使能状态回显

    stArmMirror.stStatus.u16RPhInvVolt  = DxSatU16(stSampleRms.fInvVoltRms[SAMPLE_PH_R] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16SPhInvVolt  = DxSatU16(stSampleRms.fInvVoltRms[SAMPLE_PH_S] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16TPhInvVolt  = DxSatU16(stSampleRms.fInvVoltRms[SAMPLE_PH_T] * DX_VOLT_SCALE);

    stArmMirror.stStatus.u16RPhInvCur   = DxSatU16(stSampleRms.fInvCurrRms[SAMPLE_PH_R] * DX_CUR_SCALE);
    stArmMirror.stStatus.u16SPhInvCur   = DxSatU16(stSampleRms.fInvCurrRms[SAMPLE_PH_S] * DX_CUR_SCALE);
    stArmMirror.stStatus.u16TPhInvCur   = DxSatU16(stSampleRms.fInvCurrRms[SAMPLE_PH_T] * DX_CUR_SCALE);

    stArmMirror.stStatus.i16RPhInvPwr   = DxSatI16(stSampleAvg.fInvPwrAvg200[SAMPLE_PH_R]);
    stArmMirror.stStatus.i16SPhInvPwr   = DxSatI16(stSampleAvg.fInvPwrAvg200[SAMPLE_PH_S]);
    stArmMirror.stStatus.i16TPhInvPwr   = DxSatI16(stSampleAvg.fInvPwrAvg200[SAMPLE_PH_T]);

    stArmMirror.stStatus.u16RPhGridVolt = DxSatU16(stSampleRms.fGridVoltRms[SAMPLE_PH_R] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16SPhGridVolt = DxSatU16(stSampleRms.fGridVoltRms[SAMPLE_PH_S] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16TPhGridVolt = DxSatU16(stSampleRms.fGridVoltRms[SAMPLE_PH_T] * DX_VOLT_SCALE);

    u16InvRunState = DxInvRunState();
    stArmMirror.stStatus.u16RPhInvState = u16InvRunState;
    stArmMirror.stStatus.u16SPhInvState = u16InvRunState;
    stArmMirror.stStatus.u16TPhInvState = u16InvRunState;

    stArmMirror.stStatus.u16InvFreq     = DxSatU16(stSampleStatus.fGridFreq * DX_FREQ_SCALE);
    stArmMirror.stStatus.u16GridFreq    = DxSatU16(stSampleStatus.fGridFreq * DX_FREQ_SCALE);

    /* ---- 5030~5054 故障与告警：直接取保护框架的协议字 ---- */
    stArmMirror.stStatus.u16PCSFault1   = stProtect.u16Proto[PROTECT_PW_5030];
    stArmMirror.stStatus.u16PCSFault2   = stProtect.u16Proto[PROTECT_PW_5031];
    stArmMirror.stStatus.u16PCSFault3   = stProtect.u16Proto[PROTECT_PW_5032];
    stArmMirror.stStatus.u16PCSFault4   = stProtect.u16Proto[PROTECT_PW_5033];
    stArmMirror.stStatus.u16PCSAlarm1   = stProtect.u16Proto[PROTECT_PW_5034];
    stArmMirror.stStatus.u16PCSFault5   = stProtect.u16Proto[PROTECT_PW_5054];
    stArmMirror.stStatus.u16DSPCtrlBits = stProtect.u16Proto[PROTECT_PW_5060];

    /* ---- 5035~5056 母线、功率、并机 ---- */
    stArmMirror.stStatus.u16DSPVersionH = DX_FW_VERSION_H;
    stArmMirror.stStatus.u16DSPVersionL = DX_FW_VERSION_L;

    stArmMirror.stStatus.u16BusVolt     = DxSatU16(stSampleAvg.fBusVoltAvg200 * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16BusVoltP    = DxSatU16(stSampleAvg.fBusPosVoltAvg200 * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16BusVoltN    = DxSatU16(stSampleAvg.fBusNegVoltAvg200 * DX_VOLT_SCALE);

    stArmMirror.stStatus.u16RPhMaxPwrOut = DX_PHASE_MAX_PWR;
    stArmMirror.stStatus.u16SPhMaxPwrOut = DX_PHASE_MAX_PWR;

    stArmMirror.stStatus.u16ParallelNum  = DX_PARALLEL_NUM;
    stArmMirror.stStatus.u16BUSRefVolt   = DxSatU16(DX_BUS_REF_VOLT * DX_VOLT_SCALE);   // TODO(批1.0)：接 BUS 电压环目标值
    /* ---- 5057~5065 负载电压、中点、绝缘、GFCI ---- */
    stArmMirror.stStatus.u16InvRPhLoadVolt = DxSatU16(stSampleRms.fInvVoltRms[SAMPLE_PH_R] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16InvSPhLoadVolt = DxSatU16(stSampleRms.fInvVoltRms[SAMPLE_PH_S] * DX_VOLT_SCALE);
    stArmMirror.stStatus.u16RNeutralVolt   = 0U;   // TODO：中点电压无采样通道
    stArmMirror.stStatus.u16SNeutralVolt   = 0U;
    stArmMirror.stStatus.u16GFCILeakCur    = DxSatU16(stSampleRms.fGfciCurrRms * DX_GFCI_SCALE);

    /* ---- 5500~5510 DER ---- */
    stArmMirror.stDer.u16InvVersionLowWordByte  = DX_HW_VERSION & 0x00FFU;
    stArmMirror.stDer.u16InvVersionHighWordByte = (DX_HW_VERSION >> 8U) & 0x00FFU;
    stArmMirror.stDer.i16Pwr             = DxSatI16(stSampleAvg.fInvPwrTotal);
    stArmMirror.stDer.i16Q               = 0;      // TODO(批0.9)：无功需按相位/幅值指令估算
    stArmMirror.stDer.u16Freq            = DxSatU16(stSampleStatus.fGridFreq * DX_FREQ_SCALE);
    stArmMirror.stDer.u16State           = u16InvRunState;
    stArmMirror.stDer.u16State_2         = (Uint16)DX_DER_CONNECTED;
    stArmMirror.stDer.u16AlarmState      = stProtect.u16Proto[PROTECT_PW_5034];
    stArmMirror.stDer.u16InvState        = u16InvRunState;
    stArmMirror.stDer.u16CtrlState       = (Uint16)DX_DER_LOCAL_CTRL;
    stArmMirror.stDer.u16BattState       = 0U;     // TODO(批0.9)：BMS 状态透传

    /* ---- 8000~8032 测量发布 ---- */
    stArmMirror.stMeas.u16InvRVolt      = stArmMirror.stStatus.u16RPhInvVolt;
    stArmMirror.stMeas.u16InvSVolt      = stArmMirror.stStatus.u16SPhInvVolt;
    stArmMirror.stMeas.u16InvRPhaseCur  = stArmMirror.stStatus.u16RPhInvCur;
    stArmMirror.stMeas.u16InvSPhaseCur  = stArmMirror.stStatus.u16SPhInvCur;
    stArmMirror.stMeas.u16GridRVolt     = stArmMirror.stStatus.u16RPhGridVolt;
    stArmMirror.stMeas.u16GridSVolt     = stArmMirror.stStatus.u16SPhGridVolt;
    stArmMirror.stMeas.u16BUS1VoltPBUSVolt = stArmMirror.stStatus.u16BusVoltP;
    stArmMirror.stMeas.u16BUS2VoltNBUSVolt = stArmMirror.stStatus.u16BusVoltN;
    stArmMirror.stMeas.u16GFCILeakCur_2    = stArmMirror.stStatus.u16GFCILeakCur;
    /* ---- PV 电压电流：5042~5053 与 8012~8019 同源 ---- */
    stArmMirror.stMeas.u16PV1Volt = DxSatU16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV1] * DX_VOLT_SCALE);
    stArmMirror.stMeas.u16PV2Volt = DxSatU16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV2] * DX_VOLT_SCALE);
    stArmMirror.stMeas.u16PV3Volt = DxSatU16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV3] * DX_VOLT_SCALE);
    stArmMirror.stMeas.u16PV4Volt = DxSatU16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV4] * DX_VOLT_SCALE);
    stArmMirror.stMeas.u16PV1Cur  = DxSatU16(stSampleAvg.fPvCurrAvg8[SAMPLE_PV1] * DX_CUR_SCALE);
    stArmMirror.stMeas.u16PV2Cur  = DxSatU16(stSampleAvg.fPvCurrAvg8[SAMPLE_PV2] * DX_CUR_SCALE);
    stArmMirror.stMeas.u16PV3Cur  = DxSatU16(stSampleAvg.fPvCurrAvg8[SAMPLE_PV3] * DX_CUR_SCALE);
    stArmMirror.stMeas.u16PV4Cur  = DxSatU16(stSampleAvg.fPvCurrAvg8[SAMPLE_PV4] * DX_CUR_SCALE);

    /* ---- 5042~5053 PV 电压电流功率 ---- */
    stArmMirror.stStatus.u16PV1VoltIn = stArmMirror.stMeas.u16PV1Volt;
    stArmMirror.stStatus.u16PV2VoltIn = stArmMirror.stMeas.u16PV2Volt;
    stArmMirror.stStatus.u16PV3VoltIn = stArmMirror.stMeas.u16PV3Volt;
    stArmMirror.stStatus.u16PV4VoltIn = stArmMirror.stMeas.u16PV4Volt;
    stArmMirror.stStatus.u16PV1CurOut = stArmMirror.stMeas.u16PV1Cur;
    stArmMirror.stStatus.u16PV2CurOut = stArmMirror.stMeas.u16PV2Cur;
    stArmMirror.stStatus.u16PV3CurOut = stArmMirror.stMeas.u16PV3Cur;
    stArmMirror.stStatus.u16PV4CurOut = stArmMirror.stMeas.u16PV4Cur;

    stArmMirror.stStatus.i16PV1OutPwr = DxSatI16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV1] * stSampleAvg.fPvCurrAvg8[SAMPLE_PV1]);
    stArmMirror.stStatus.i16PV2OutPwr = DxSatI16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV2] * stSampleAvg.fPvCurrAvg8[SAMPLE_PV2]);
    stArmMirror.stStatus.i16PV3OutPwr = DxSatI16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV3] * stSampleAvg.fPvCurrAvg8[SAMPLE_PV3]);
    stArmMirror.stStatus.i16PV4OutPwr = DxSatI16(stSampleAvg.fPvVoltAvg8[SAMPLE_PV4] * stSampleAvg.fPvCurrAvg8[SAMPLE_PV4]);
}

/**
 * @brief 逆变器工作状态映射到协议取值（0-停机 1-离网运行 2-旁路 3-并网 4-用电网充）
 */
static Uint16 DxInvRunState(void)
{
    Uint16 u16State = InvSmGetState();

    if ((u16State == (Uint16)INV_ST_GRIDON) || (u16State == (Uint16)INV_ST_RUN))
    {
        return 1U;      // TODO(批1.0)：并网运行后改为 3
    }

    return 0U;
}
/**
 * @brief 1 kHz 任务
 */
void DataExchangeTask(void *pArgV)
{
    (void)pArgV;

    DataExchangeRx();
    DataExchangeTx();
}

/**
 * @brief 浮点限幅到无符号 16 位
 */
static Uint16 DxSatU16(float32 fVal)
{
    if (fVal <= 0.0f)
    {
        return 0U;
    }
    if (fVal >= 65535.0f)
    {
        return 65535U;
    }

    return (Uint16)fVal;
}



/**
 * @brief 浮点限幅到有符号 16 位
 */
static int16 DxSatI16(float32 fVal)
{
    if (fVal <= -32768.0f)
    {
        return (int16)-32768;
    }
    if (fVal >= 32767.0f)
    {
        return (int16)32767;
    }

    return (int16)fVal;
}

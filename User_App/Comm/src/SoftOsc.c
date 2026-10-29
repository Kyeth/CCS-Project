/**
 * @file SoftOsc.c
 * @author yetingheng (004182)
 * @brief CAN-B 软件示波器实现
 * @version 0.1
 * @date 2026-10-22
 * @copyright Copyright (c) 2026
 */
#include "SoftOsc.h"
#include "CtrlIsr.h"
#include "User_ClaTask.h"
#include "CommRegTable.h"
#include "AdcCalib.h"
#include "AdcSample.h"
#include "AtsProto.h"

/* ========================== 可观测源登记表 ==========================
 * 格式：{源编号, 数据类型, 变量地址}
 * 想新增可观测变量，在这里加一行、在 SoftOsc.h 加一个编号即可，
 * 上位机不用改（编号就是 Modbus 里写的值）。
 */
typedef struct
{
    Uint16 u16SrcId;
    Uint16 u16Type;
    void  *pvAddr;
} ST_SOFT_OSC_SRC;

static const ST_SOFT_OSC_SRC gSoftOscSrcTab[] =
{
    {SOFT_OSC_SRC_THETA,        SOFT_OSC_TYPE_FLOAT,  (void *)&stCtrlObs.fThetaDeg},
    {SOFT_OSC_SRC_GRID_FREQ,    SOFT_OSC_TYPE_FLOAT,  (void *)&stCtrlObs.fGridFreq},
    {SOFT_OSC_SRC_MOD_INDEX,    SOFT_OSC_TYPE_FLOAT,  (void *)&stCtrlObs.fModIndex},
    {SOFT_OSC_SRC_FAULT,        SOFT_OSC_TYPE_UINT16, (void *)&stCtrlObs.u16Fault},
    {SOFT_OSC_SRC_SEQ,          SOFT_OSC_TYPE_UINT16, (void *)&Cla1ToCpuMsg.u16Seq},
    {SOFT_OSC_SRC_MUX_ADDR,     SOFT_OSC_TYPE_UINT16, (void *)&Cla1ToCpuMsg.u16MuxAddr},
    {SOFT_OSC_SRC_INV_CUR_R,    SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_R]},
    {SOFT_OSC_SRC_INV_CUR_S,    SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_S]},
    {SOFT_OSC_SRC_INV_CUR_T,    SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_T]},
    {SOFT_OSC_SRC_INV_CUR_PK_R, SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_PK_R]},
    {SOFT_OSC_SRC_INV_CUR_PK_S, SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_PK_S]},
    {SOFT_OSC_SRC_INV_CUR_PK_T, SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_CUR_PK_T]},
    {SOFT_OSC_SRC_INV_VOLT_R,   SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_VOLT_R]},
    {SOFT_OSC_SRC_INV_VOLT_S,   SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_VOLT_S]},
    {SOFT_OSC_SRC_INV_VOLT_T,   SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_INV_VOLT_T]},
    {SOFT_OSC_SRC_GRID_VOLT_R,  SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_GRID_VOLT_R]},
    {SOFT_OSC_SRC_GRID_VOLT_S,  SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_GRID_VOLT_S]},
    {SOFT_OSC_SRC_GRID_VOLT_T,  SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_GRID_VOLT_T]},
    {SOFT_OSC_SRC_PV_CUR1,      SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_CUR1]},
    {SOFT_OSC_SRC_PV_CUR2,      SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_CUR2]},
    {SOFT_OSC_SRC_PV_CUR3,      SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_CUR3]},
    {SOFT_OSC_SRC_PV_CUR4,      SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_CUR4]},
    {SOFT_OSC_SRC_PV_VOLT1,     SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_VOLT1]},
    {SOFT_OSC_SRC_PV_VOLT2,     SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_VOLT2]},
    {SOFT_OSC_SRC_PV_VOLT3,     SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_VOLT3]},
    {SOFT_OSC_SRC_PV_VOLT4,     SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_PV_VOLT4]},
    {SOFT_OSC_SRC_BUS_VOLT_L,   SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_BUS_VOLT_L]},
    {SOFT_OSC_SRC_BUS_VOLT_H,   SOFT_OSC_TYPE_FLOAT,  (void *)&stAdcCalib.fValue[ADC_CAL_BUS_VOLT_H]},

    /* 对外稳定量：来自 SampleData 的 stSampleRms / stSampleAvg（RMS 与慢速滤波后的物理量） */
    {SOFT_OSC_SRC_GRID_VOLT_RMS_R, SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fGridVoltRms[SAMPLE_PH_R]},
    {SOFT_OSC_SRC_GRID_VOLT_RMS_S, SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fGridVoltRms[SAMPLE_PH_S]},
    {SOFT_OSC_SRC_GRID_VOLT_RMS_T, SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fGridVoltRms[SAMPLE_PH_T]},
    {SOFT_OSC_SRC_INV_CUR_RMS_R,   SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fInvCurrRms[SAMPLE_PH_R]},
    {SOFT_OSC_SRC_INV_CUR_RMS_S,   SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fInvCurrRms[SAMPLE_PH_S]},
    {SOFT_OSC_SRC_INV_CUR_RMS_T,   SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleRms.fInvCurrRms[SAMPLE_PH_T]},
    {SOFT_OSC_SRC_BUS_VOLT_SLOW,   SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleAvg.fBusVoltAvg200},
    {SOFT_OSC_SRC_PV_VOLT1_SLOW,   SOFT_OSC_TYPE_FLOAT,  (void *)&stSampleAvg.fPvVoltAvg8[SAMPLE_PV1]},
    {SOFT_OSC_SRC_ATS_PHASE_DIFF,  SOFT_OSC_TYPE_FLOAT,  (void *)&gAtsInfo.fPhaseDiff},
    {SOFT_OSC_SRC_ATS_VOLT_DIFF,   SOFT_OSC_TYPE_FLOAT,  (void *)&gAtsInfo.fVoltDiff},
};

#define SOFT_OSC_SRC_NUM    ((Uint16)(sizeof(gSoftOscSrcTab) / sizeof(gSoftOscSrcTab[0])))

/* 默认的 12 路通道表（改这里就是改默认配置，不用改代码逻辑） */
static const ST_SOFT_OSC_CH_CFG gSoftOscChCfgDef[SOFT_OSC_CH_NUM] =
{
    /* 第 1~6 路：控制链路关键量 */
    {SOFT_OSC_SRC_INV_CUR_R,   100U},   /* ch1  逆变电流 R，x100 */
    {SOFT_OSC_SRC_INV_VOLT_R,   10U},   /* ch2  逆变电压 R，x10  */
    {SOFT_OSC_SRC_GRID_VOLT_R,  10U},   /* ch3  电网电压 R，x10  */
    {SOFT_OSC_SRC_PV_CUR1,     100U},   /* ch4  PV1 电流，x100   */
    {SOFT_OSC_SRC_BUS_VOLT_L,   10U},   /* ch5  下半母线电压，x10 */
    {SOFT_OSC_SRC_BUS_VOLT_H,   10U},   /* ch6  上半母线电压，x10 */

    /* 第 7~12 路：环路状态 */
    {SOFT_OSC_SRC_SEQ,           1U},   /* ch7  采样序号        */
    {SOFT_OSC_SRC_MUX_ADDR,    100U},   /* ch8  MUX 地址，x100  */
    {SOFT_OSC_SRC_THETA,        50U},   /* ch9  锁相角，x50（0~360°→0~18000，x100 会溢出） */
    {SOFT_OSC_SRC_GRID_FREQ,   100U},   /* ch10 电网频率，x100  */
    {SOFT_OSC_SRC_MOD_INDEX,  1000U},   /* ch11 调制比，x1000   */
    {SOFT_OSC_SRC_FAULT,         1U},   /* ch12 故障字          */
};

/* 运行期通道配置（Modbus 可写）与它解析出来的运行表 */
ST_SOFT_OSC_CH_CFG gSoftOscChCfg[SOFT_OSC_CH_NUM];
static ST_SOFT_OSC_CH_CFG gSoftOscChCfgShadow[SOFT_OSC_CH_NUM];
static ST_SOFT_OSC_CH gSoftOscCh[SOFT_OSC_CH_NUM];

/* 发送模式与组别选择：挂到 Modbus 点表里由上位机改写 */
volatile Uint16 gSoftOscMode = SOFT_OSC_MODE_DUAL;
volatile Uint16 gSoftOscGroupSel = 0U;

/* 当前发送的组别：0 = 第 1~6 路，1 = 第 7~12 路 */
static Uint16 gSoftOscGroup = 0U;

/* 发送失败计数，便于调试 */
static Uint16 gSoftOscTxErrCnt = 0U;

/* 已发送帧计数（只读观测点） */
volatile Uint16 gSoftOscFrameCnt = 0U;

/**
 * @brief 按类型取一路变量的当前值，并乘以放大倍数
 * @return 32 位有符号值（以 Uint32 返回，上位机按有符号解析）
 */
static Uint32 SoftOscGetValue(Uint16 u16Ch)
{
    const ST_SOFT_OSC_CH *pstCh;
    Uint16 u16Mul;
    int32 i32Val;

    pstCh = &gSoftOscCh[u16Ch];
    u16Mul = (pstCh->u16Multiple == 0U) ? 1U : pstCh->u16Multiple;
    i32Val = 0;

    if (pstCh->pvAddr == (void *)0)
    {
        return 0UL;
    }

    switch (pstCh->u16Type)
    {
        case SOFT_OSC_TYPE_INT8:
            i32Val = (int32)(*((volatile signed char *)pstCh->pvAddr));
            break;

        case SOFT_OSC_TYPE_UINT8:
            i32Val = (int32)(*((volatile unsigned char *)pstCh->pvAddr));
            break;

        case SOFT_OSC_TYPE_INT16:
            i32Val = (int32)(*((volatile int16 *)pstCh->pvAddr));
            break;

        case SOFT_OSC_TYPE_UINT16:
            i32Val = (int32)(*((volatile Uint16 *)pstCh->pvAddr));
            break;

        case SOFT_OSC_TYPE_INT32:
            i32Val = *((volatile int32 *)pstCh->pvAddr);
            break;

        case SOFT_OSC_TYPE_UINT32:
            i32Val = (int32)(*((volatile Uint32 *)pstCh->pvAddr));
            break;

        case SOFT_OSC_TYPE_FLOAT:
            i32Val = (int32)((*((volatile float32 *)pstCh->pvAddr))
                              * (float32)u16Mul);
            break;

        default:
            i32Val = 0;
            break;
    }

    /* 整型量同样乘倍数（多为 1，个别通道用来放大）
       注意：整型不走 float，避免大数丢精度 */
    if (pstCh->u16Type != SOFT_OSC_TYPE_FLOAT)
    {
        if (u16Mul > 1U)
        {
            i32Val = i32Val * (int32)u16Mul;
        }
    }

    /* 帧里每路只有一个 16 位字段，上位机按有符号解析：
       超出 ±32767 就饱和，而不是回绕（回绕看起来像噪声，很难判断是真溢出） */
    if (i32Val > 32767)
    {
        i32Val = 32767;
    }
    else if (i32Val < -32768)
    {
        i32Val = -32768;
    }

    return (Uint32)i32Val;
}

/**
 * @brief 把一个 32 位值拆成 CAN 的 4 个字节
 *
 * 低 16 位先发（高字节在前），高 16 位后发，与 IB800 的 U16tobp32 一致：
 *   Data_0 = 低16位高字节  Data_1 = 低16位低字节
 *   Data_2 = 高16位高字节  Data_3 = 高16位低字节
 */
static void SoftOscWordToBytes(Uint32 u32Val, Uint16 *pu16Dst)
{
    pu16Dst[0] = (Uint16)((u32Val >> 8) & 0x00FFUL);
    pu16Dst[1] = (Uint16)(u32Val & 0x00FFUL);
    pu16Dst[2] = (Uint16)((u32Val >> 24) & 0x00FFUL);
    pu16Dst[3] = (Uint16)((u32Val >> 16) & 0x00FFUL);
}

/**
 * @brief 查观测源的数据类型
 * @return SOFT_OSC_TYPE_xxx；编号非法返回 0xFFFF
 */
Uint16 SoftOscGetSrcType(Uint16 u16SrcId)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < SOFT_OSC_SRC_NUM; u16Idx++)
    {
        if (gSoftOscSrcTab[u16Idx].u16SrcId == u16SrcId)
        {
            return gSoftOscSrcTab[u16Idx].u16Type;
        }
    }

    return 0xFFFFU;
}

/**
 * @brief 按 Modbus 寄存器地址在点表里反查观测源
 * @param u16RegAddr 点表里的寄存器地址（如 30010）
 * @param pu16Type   输出：SOFT_OSC_TYPE_xxx
 * @param ppvAddr    输出：变量地址
 * @return 1 = 命中；0 = 两张表里都没有这个地址
 * @note  先查 ARM 表再查上位机表；类型直接取该行的 u16ValType，
 *        所以 32 位整数和浮点变量都能被正确取到（帧里仍是 16 位传输，
 *        见 SoftOscGetValue 的说明）。
 */
static Uint16 SoftOscResolveByAddr(Uint16 u16RegAddr, Uint16 *pu16Type, void **ppvAddr)
{
    const ST_MB_REG_ITEM *pstItem = (const ST_MB_REG_ITEM *)0;
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < MB_ARM_REG_NUM; u16Idx++)
    {
        if (stMbRegItemArm[u16Idx].u16RegAddr == u16RegAddr)
        {
            pstItem = &stMbRegItemArm[u16Idx];
            break;
        }
    }

    if (pstItem == (const ST_MB_REG_ITEM *)0)
    {
        for (u16Idx = 0U; u16Idx < MB_PC_REG_NUM; u16Idx++)
        {
            if (stMbRegItemPc[u16Idx].u16RegAddr == u16RegAddr)
            {
                pstItem = &stMbRegItemPc[u16Idx];
                break;
            }
        }
    }

    if ((pstItem == (const ST_MB_REG_ITEM *)0) || (pstItem->pvData == (void *)0))
    {
        return 0U;
    }

    *ppvAddr = pstItem->pvData;

    switch (pstItem->u16ValType)
    {
        case MB_VAL_U16: *pu16Type = SOFT_OSC_TYPE_UINT16; break;
        case MB_VAL_I16: *pu16Type = SOFT_OSC_TYPE_INT16;  break;
        case MB_VAL_U32: *pu16Type = SOFT_OSC_TYPE_UINT32; break;
        case MB_VAL_I32: *pu16Type = SOFT_OSC_TYPE_INT32;  break;
        case MB_VAL_F32: *pu16Type = SOFT_OSC_TYPE_FLOAT;  break;
        default:          *pu16Type = SOFT_OSC_TYPE_UINT16; break;
    }

    return 1U;
}

/**
 * @brief 把 gSoftOscChCfg 解析成"地址 + 类型 + 倍数"
 * @note  12 路 × 28 项线性查表，只在配置变化时执行，不影响 400us 节拍
 */
void SoftOscApplyCfg(void)
{
    Uint16 u16Ch;
    Uint16 u16Idx;
    Uint16 u16Found;
    Uint16 u16RowType = SOFT_OSC_TYPE_UINT16;
    void  *pvRowAddr = (void *)0;

    for (u16Ch = 0U; u16Ch < SOFT_OSC_CH_NUM; u16Ch++)
    {
        u16Found = 0U;

        /* 两种写法都按"寄存器地址"反查点表：
         *   1) 编号 >= SOFT_OSC_SRC_ADDR_MIN（兼容旧用法，覆盖 ARM 段 5000+）；
         *   2) 编号最高位带 SOFT_OSC_SRC_ADDR_FLAG，低 15 位为地址，可覆盖 0~99。 */
        if (((gSoftOscChCfg[u16Ch].u16SrcId >= SOFT_OSC_SRC_ADDR_MIN) ||
             ((gSoftOscChCfg[u16Ch].u16SrcId & SOFT_OSC_SRC_ADDR_FLAG) != 0U)) &&
            (SoftOscResolveByAddr((Uint16)(gSoftOscChCfg[u16Ch].u16SrcId & 0x7FFFU),
                                  &u16RowType, &pvRowAddr) != 0U))
        {
            gSoftOscCh[u16Ch].u16Type = u16RowType;
            gSoftOscCh[u16Ch].pvAddr = pvRowAddr;
            gSoftOscCh[u16Ch].u16Multiple =
                (gSoftOscChCfg[u16Ch].u16Multiple == 0U) ? 1U
                                                         : gSoftOscChCfg[u16Ch].u16Multiple;
            u16Found = 1U;
        }

        for (u16Idx = 0U; (u16Idx < SOFT_OSC_SRC_NUM) && (u16Found == 0U); u16Idx++)
        {
            if (gSoftOscSrcTab[u16Idx].u16SrcId == gSoftOscChCfg[u16Ch].u16SrcId)
            {
                gSoftOscCh[u16Ch].u16Type = gSoftOscSrcTab[u16Idx].u16Type;
                gSoftOscCh[u16Ch].pvAddr = gSoftOscSrcTab[u16Idx].pvAddr;
                gSoftOscCh[u16Ch].u16Multiple =
                    (gSoftOscChCfg[u16Ch].u16Multiple == 0U) ? 1U
                                                             : gSoftOscChCfg[u16Ch].u16Multiple;
                u16Found = 1U;
                break;
            }
        }

        if (u16Found == 0U)
        {
            /* 编号非法或写 0：该通道本帧输出 0 */
            gSoftOscCh[u16Ch].u16Type = SOFT_OSC_TYPE_UINT16;
            gSoftOscCh[u16Ch].pvAddr = (void *)0;
            gSoftOscCh[u16Ch].u16Multiple = 1U;
        }

        gSoftOscChCfgShadow[u16Ch] = gSoftOscChCfg[u16Ch];
    }
}

/* 配置有没有被上位机改过（比较影子副本） */
static Uint16 SoftOscCfgChanged(void)
{
    Uint16 u16Ch;

    for (u16Ch = 0U; u16Ch < SOFT_OSC_CH_NUM; u16Ch++)
    {
        if ((gSoftOscChCfg[u16Ch].u16SrcId != gSoftOscChCfgShadow[u16Ch].u16SrcId) ||
            (gSoftOscChCfg[u16Ch].u16Multiple != gSoftOscChCfgShadow[u16Ch].u16Multiple))
        {
            return 1U;
        }
    }

    return 0U;
}

void SoftOscBindSrc(Uint16 u16Ch, Uint16 u16SrcId, Uint16 u16Multiple)
{
    if (u16Ch >= SOFT_OSC_CH_NUM)
    {
        return;
    }

    gSoftOscChCfg[u16Ch].u16SrcId = u16SrcId;
    gSoftOscChCfg[u16Ch].u16Multiple = u16Multiple;

    SoftOscApplyCfg();
}

/**
 * @brief 按默认表初始化 12 路观测通道
 *
 * 默认配置全部写在 gSoftOscChCfgDef[] 里，这里只负责搬一次并解析；
 * 上位机通过 Modbus 0~11（选源地址）/ 12~23（倍数）改写后，下一帧自动生效。
 */
void SoftOscInit(void)
{
    Uint16 u16Idx;

    for (u16Idx = 0U; u16Idx < SOFT_OSC_CH_NUM; u16Idx++)
    {
        gSoftOscChCfg[u16Idx] = gSoftOscChCfgDef[u16Idx];
    }

    SoftOscApplyCfg();

    gSoftOscFrameCnt = 0U;
    gSoftOscTxErrCnt = 0U;
}

void SoftOscSetChannel(Uint16 u16Ch, Uint16 u16Type,
                       void *pvAddr, Uint16 u16Multiple)
{
    if (u16Ch >= SOFT_OSC_CH_NUM)
    {
        return;
    }

    gSoftOscCh[u16Ch].u16Type = u16Type;
    gSoftOscCh[u16Ch].pvAddr = pvAddr;
    gSoftOscCh[u16Ch].u16Multiple = u16Multiple;
}

void SoftOscTask(void *pArgV)
{
    Uint16 u16Data[SOFT_OSC_DATA_LEN];
    Uint32 u32Id;
    Uint32 u32Ch0;
    Uint32 u32Ch1;
    Uint32 u32Ch2;
    Uint32 u32Ch3;
    Uint32 u32Ch4;
    Uint32 u32Ch5;
    Uint32 u32DataA;
    Uint32 u32DataB;
    Uint16 u16Base;

    (void)pArgV;

    /* 上位机改过通道配置就重新解析一次（变化时才会走到，平时只做 24 次比较） */
    if (SoftOscCfgChanged() != 0U)
    {
        SoftOscApplyCfg();
    }

    gSoftOscFrameCnt = (Uint16)(gSoftOscFrameCnt + 1U);

    /* 单边模式：固定发 gSoftOscGroupSel 指定的那一组，6 路都是 400us 刷新；
     * 双边模式：两组交替，每路 800us 刷新。 */
    if (gSoftOscMode == SOFT_OSC_MODE_SINGLE)
    {
        gSoftOscGroup = (gSoftOscGroupSel == 0U) ? 0U : 1U;
    }

    /* 先取值，再组帧，保证同一帧内的 6 路是同一次采样的快照 */
    u16Base = (gSoftOscGroup == 0U) ? 0U : 6U;

    u32Ch0 = SoftOscGetValue((Uint16)(u16Base + 0U));
    u32Ch1 = SoftOscGetValue((Uint16)(u16Base + 1U));
    u32Ch2 = SoftOscGetValue((Uint16)(u16Base + 2U));
    u32Ch3 = SoftOscGetValue((Uint16)(u16Base + 3U));
    u32Ch4 = SoftOscGetValue((Uint16)(u16Base + 4U));
    u32Ch5 = SoftOscGetValue((Uint16)(u16Base + 5U));

    /* 第 1、2 路进 ID；第 5、6 路进 DATA；第 3、4 路进 DATB */
    if (gSoftOscGroup == 0U)
    {
        u32Id = (((u32Ch0 & 0x0000FFFFUL) << 16) |
                 (u32Ch1 & 0x0000FFFFUL)) & SOFT_OSC_ID_MASK;
    }
    else
    {
        u32Id = (((u32Ch0 & 0x0000FFFFUL) << 16) |
                 (u32Ch1 & 0x0000FFFFUL)) | SOFT_OSC_FRAME_FLAG;
    }

    u32DataA = ((u32Ch4 & 0x0000FFFFUL) << 16) | (u32Ch5 & 0x0000FFFFUL);
    u32DataB = ((u32Ch2 & 0x0000FFFFUL) << 16) | (u32Ch3 & 0x0000FFFFUL);

    SoftOscWordToBytes(u32DataA, &u16Data[0]);
    SoftOscWordToBytes(u32DataB, &u16Data[4]);

    if (Hw_CanSend(SOFT_OSC_CAN_ID, SOFT_OSC_TX_OBJ_IDX, u32Id,
                   u16Data, SOFT_OSC_DATA_LEN) == 0U)
    {
        if (gSoftOscTxErrCnt < 0xFFFFU)
        {
            gSoftOscTxErrCnt++;
        }
    }

    /* 双边模式才交替：本次 1~6 路，下次 7~12 路；单边模式保持不变 */
    if (gSoftOscMode != SOFT_OSC_MODE_SINGLE)
    {
        gSoftOscGroup = (gSoftOscGroup == 0U) ? 1U : 0U;
    }
}
/**
 * @brief 切换发送模式
 * @param u16Mode SOFT_OSC_MODE_DUAL（12 路交替，每路 800us）
 *                SOFT_OSC_MODE_SINGLE（单组 6 路，每路 400us）
 */
void SoftOscSetMode(Uint16 u16Mode)
{
    gSoftOscMode = (u16Mode == SOFT_OSC_MODE_SINGLE)
                 ? SOFT_OSC_MODE_SINGLE : SOFT_OSC_MODE_DUAL;
}

/**
 * @brief 单边模式下选择发送哪一组
 * @param u16Group 0 = 第 1~6 路，1 = 第 7~12 路
 */
void SoftOscSetGroup(Uint16 u16Group)
{
    gSoftOscGroupSel = (u16Group == 0U) ? 0U : 1U;
    gSoftOscGroup = gSoftOscGroupSel;
}

/**
 * @file AtsProto.c
 * @author yetingheng (004182)
 * @brief ATS（京硅 KSiC1-100）Modbus 通道监听与寄存器镜像实现
 * @version 0.1
 * @date 2026-10-27
 * @copyright Copyright (c) 2026
 *
 * 帧格式（Modbus RTU，文档第 5 节）：
 *   读请求  : 地址 03 起始Hi 起始Lo 数量Hi 数量Lo CRCl CRCh          共 8 字节
 *   读应答  : 地址 03 字节数 数据... CRCl CRCh                      共 5+字节数
 *   写单个  : 地址 06 寄存Hi 寄存Lo 值Hi 值Lo CRCl CRCh             共 8 字节（请求与应答同形）
 *   写多个  : 地址 10 起始Hi 起始Lo 数量Hi 数量Lo 字节数 数据... CRC  请求 >= 9 字节
 *             应答与写单个同形（8 字节，原样回送起始地址与数量）
 *
 * 因为请求与应答的从机地址相同（都是 ATS 地址），用**长度**区分：
 * 读请求恒为 8 字节，读应答为 5+字节数（字节数 >= 2，故 >= 7 字节且 != 8）。
 */
#include "AtsProto.h"
#include "Crc16.h"

/* 功能码 */
#define ATS_FUNC_READ               (0x03U)
#define ATS_FUNC_READ_INPUT         (0x04U)
#define ATS_FUNC_WRITE_SINGLE       (0x06U)
#define ATS_FUNC_WRITE_MULTI        (0x10U)

/* 读请求/写单个报文的固定长度 */
#define ATS_REQ_LEN                 (8U)

/* 关键寄存器地址 */
#define ATS_REG_IN_VOLT_A           (0x0080U)
#define ATS_REG_IN_FREQ_POS         (0x008FU)
#define ATS_REG_OUT_VOLT_A          (0x0090U)
#define ATS_REG_OUT_FREQ_POS        (0x0099U)
#define ATS_REG_VOLT_DIFF_POS       (0x009AU)
#define ATS_REG_PHASE_DIFF_POS      (0x009BU)

/* 换算系数 */
#define ATS_COEF_VOLT               (0.01f)     /* 0.01 V  */
#define ATS_COEF_FREQ               (0.01f)     /* 0.01 Hz */
#define ATS_COEF_PHASE              (0.01f)     /* 0.01 °  */

static const ST_ATS_PROTO_CFG *gAtsCfg = (const ST_ATS_PROTO_CFG *)0;

Uint16 gAtsReg[ATS_REG_NUM];
ST_ATS_INFO gAtsInfo;
volatile Uint16 gAtsFrameCnt = 0U;
volatile Uint16 gAtsErrCnt = 0U;
volatile Uint16 gAtsAliveCnt = ATS_ALIVE_MAX + 1U;

/* 最近一次读请求的起始地址与数量，用来把应答数据落到正确位置 */
static Uint16 gAtsPendingReg = 0U;
static Uint16 gAtsPendingNum = 0U;
static Uint16 gAtsPendingValid = 0U;

/* 在线判定的历史帧计数 */
static Uint16 gAtsLastSeen = 0U;

/**
 * @brief 取帧里第 u16Idx 个字节（帧缓存是"一个字节一个 Uint16"）
 */
static Uint16 AtsByte(const Uint16 *pu16Frame, Uint16 u16Idx)
{
    return (Uint16)(pu16Frame[u16Idx] & 0x00FFU);
}

/**
 * @brief 帧里第 u16Idx 个字节起的 16 位大端值
 */
static Uint16 AtsWord(const Uint16 *pu16Frame, Uint16 u16Idx)
{
    return (Uint16)((AtsByte(pu16Frame, u16Idx) << 8)
                  | AtsByte(pu16Frame, (Uint16)(u16Idx + 1U)));
}

void AtsProtoInit(const ST_ATS_PROTO_CFG *pstCfg)
{
    Uint16 u16Idx;

    gAtsCfg = pstCfg;

    for (u16Idx = 0U; u16Idx < ATS_REG_NUM; u16Idx++)
    {
        gAtsReg[u16Idx] = 0U;
    }

    gAtsFrameCnt = 0U;
    gAtsErrCnt = 0U;
    gAtsAliveCnt = (Uint16)(ATS_ALIVE_MAX + 1U);
    gAtsPendingValid = 0U;
    gAtsLastSeen = 0U;

    gAtsInfo.fInFreq = 0.0f;
    gAtsInfo.fOutFreq = 0.0f;
    gAtsInfo.fVoltDiff = 0.0f;
    gAtsInfo.fPhaseDiff = 0.0f;
    gAtsInfo.fInVoltA = 0.0f;
    gAtsInfo.fOutVoltA = 0.0f;
    gAtsInfo.u16Online = 0U;
    gAtsInfo.u16FrameCnt = 0U;
}

/**
 * @brief 读应答：把数据按最近一次请求的起始地址写进镜像
 */
static void AtsOnReadResp(const Uint16 *pu16Frame, Uint16 u16Len)
{
    Uint16 u16ByteCnt;
    Uint16 u16Num;
    Uint16 u16Idx;
    Uint16 u16Reg;

    u16ByteCnt = AtsByte(pu16Frame, 2U);

    if ((u16ByteCnt == 0U) || ((u16ByteCnt & 0x0001U) != 0U))
    {
        gAtsErrCnt++;
        return;
    }

    if (u16Len != (Uint16)(u16ByteCnt + 5U))
    {
        gAtsErrCnt++;
        return;
    }

    /* 没见过对应的请求帧（比如上位机自己的读），位置无法确定，只计帧 */
    if (gAtsPendingValid == 0U)
    {
        gAtsFrameCnt++;
        return;
    }

    u16Num = (Uint16)(u16ByteCnt >> 1);

    if (u16Num > gAtsPendingNum)
    {
        u16Num = gAtsPendingNum;
    }

    for (u16Idx = 0U; u16Idx < u16Num; u16Idx++)
    {
        u16Reg = (Uint16)(gAtsPendingReg + u16Idx);

        if (u16Reg >= ATS_REG_NUM)
        {
            break;
        }

        gAtsReg[u16Reg] = AtsWord(pu16Frame, (Uint16)(3U + (2U * u16Idx)));
    }

    gAtsPendingValid = 0U;
    gAtsFrameCnt++;
}

void AtsProtoOnFrame(Uint16 u16SciId, const Uint16 *pu16Frame, Uint16 u16Len)
{
    Uint16 u16Addr;
    Uint16 u16Func;
    Uint16 u16Reg;
    Uint16 u16Idx;

    if (gAtsCfg == (const ST_ATS_PROTO_CFG *)0)
    {
        return;
    }

    if (u16Len < 4U)
    {
        return;
    }

    if (u16SciId != gAtsCfg->u16SciId)
    {
        return;
    }

    u16Addr = AtsByte(pu16Frame, 0U);

    /* 三种情况归这里管：
     *   1) 地址 = 问询地址 0xFB：单从机时主站（ARM 或我们）用它发请求；
     *   2) 地址 = 配置的 ATS 地址：ATS 的应答，或者主站直接按 ATS 地址发请求；
     *   3) 其它地址 + 有 pending 请求：单从机问询时 ATS 用**自身地址**应答，
     *      而该地址未必等于配置值，所以只要在等应答就收下。
     */
    if ((u16Addr != ATS_ADDR_INQUIRY) &&
        (u16Addr != gAtsCfg->u16SlaveAddr) &&
        (gAtsPendingValid == 0U))
    {
        return;
    }

    if (Crc16ModbusCheck(pu16Frame, u16Len) == 0U)
    {
        gAtsErrCnt++;
        return;
    }

    u16Func = AtsByte(pu16Frame, 1U);

    if ((u16Func == ATS_FUNC_READ) || (u16Func == ATS_FUNC_READ_INPUT))
    {
        if (u16Len == ATS_REQ_LEN)
        {
            /* 读请求：记下起始地址与数量，等应答来落数 */
            gAtsPendingReg = AtsWord(pu16Frame, 2U);
            gAtsPendingNum = AtsWord(pu16Frame, 4U);
            gAtsPendingValid = 1U;
        }
        else
        {
            AtsOnReadResp(pu16Frame, u16Len);
        }

        return;
    }

    if (u16Func == ATS_FUNC_WRITE_SINGLE)
    {
        /* 请求与应答同形，镜像按写入值更新即可（幂等） */
        if (u16Len == ATS_REQ_LEN)
        {
            u16Reg = AtsWord(pu16Frame, 2U);

            if (u16Reg < ATS_REG_NUM)
            {
                gAtsReg[u16Reg] = AtsWord(pu16Frame, 4U);
            }

            gAtsFrameCnt++;
        }

        return;
    }

    if (u16Func == ATS_FUNC_WRITE_MULTI)
    {
        if (u16Len == ATS_REQ_LEN)
        {
            /* 写应答：无数据 */
            gAtsFrameCnt++;
        }
        else if (u16Len >= 9U)
        {
            /* 写请求：起始地址 / 数量 / 字节数 / 数据 */
            u16Reg = AtsWord(pu16Frame, 2U);

            for (u16Idx = 0U; u16Idx < (Uint16)((u16Len - 9U) >> 1); u16Idx++)
            {
                if ((Uint16)(u16Reg + u16Idx) >= ATS_REG_NUM)
                {
                    break;
                }

                gAtsReg[u16Reg + u16Idx] = AtsWord(pu16Frame, (Uint16)(7U + (2U * u16Idx)));
            }
        }

        return;
    }

    /* 异常应答（功能码 | 0x80） */
    if ((u16Func & 0x0080U) != 0U)
    {
        gAtsErrCnt++;
        return;
    }
}

Uint16 AtsGetReg(Uint16 u16Reg)
{
    if (u16Reg >= ATS_REG_NUM)
    {
        return 0U;
    }

    return gAtsReg[u16Reg];
}

int16 AtsGetRegSigned(Uint16 u16Reg)
{
    if (u16Reg >= ATS_REG_NUM)
    {
        return 0;
    }

    return (int16)gAtsReg[u16Reg];
}

Uint16 AtsIsOnline(void)
{
    return (gAtsAliveCnt <= ATS_ALIVE_MAX) ? 1U : 0U;
}

void AtsProtoTask(void *pArgV)
{
    (void)pArgV;

    if (gAtsFrameCnt != gAtsLastSeen)
    {
        gAtsLastSeen = gAtsFrameCnt;
        gAtsAliveCnt = 0U;
    }
    else if (gAtsAliveCnt < 0xFFFFU)
    {
        gAtsAliveCnt++;
    }

    /* 工程量换算（镜像里是原始码值，单位见协议表） */
    gAtsInfo.fInVoltA = (float32)AtsGetReg(ATS_REG_IN_VOLT_A) * ATS_COEF_VOLT;
    gAtsInfo.fOutVoltA = (float32)AtsGetReg(ATS_REG_OUT_VOLT_A) * ATS_COEF_VOLT;
    gAtsInfo.fInFreq = (float32)AtsGetReg(ATS_REG_IN_FREQ_POS) * ATS_COEF_FREQ;
    gAtsInfo.fOutFreq = (float32)AtsGetReg(ATS_REG_OUT_FREQ_POS) * ATS_COEF_FREQ;
    gAtsInfo.fVoltDiff = (float32)AtsGetRegSigned(ATS_REG_VOLT_DIFF_POS) * ATS_COEF_VOLT;
    gAtsInfo.fPhaseDiff = (float32)AtsGetRegSigned(ATS_REG_PHASE_DIFF_POS) * ATS_COEF_PHASE;
    gAtsInfo.u16Online = AtsIsOnline();
    gAtsInfo.u16FrameCnt = gAtsFrameCnt;


    /* 请求帧的映射只在一次应答内有效：100ms 还没等到应答就作废，
     * 避免把之后无关的应答错位写进镜像 */
    gAtsPendingValid = 0U;
}

/**
 * @file AtsProto.h
 * @author yetingheng (004182)
 * @brief ATS（京硅 KSiC1-100）Modbus 通道监听与寄存器镜像
 * @version 0.1
 * @date 2026-10-27
 * @copyright Copyright (c) 2026
 *
 * 总线结构：ATS 与 ARM 走同一条 RS485（我们的 SCIC 只并联 RX，不驱动 TX）。
 * ARM 是主站，周期轮询 ATS；本模块在串口收帧处搭个"旁听"，
 * 把 ATS 应答里的寄存器抓进 gAtsReg[]，供锁相/并网逻辑与上位机观测。
 *
 * 因此：**我们能看到哪些寄存器，取决于 ARM 实际读了哪些**。
 * 若 ARM 的轮询表没有覆盖 0x80~0x9E（并网信息区），本镜像就一直是 0。
 */
#ifndef ATS_PROTO_H
#define ATS_PROTO_H

#include "F28x_Project.h"

/* ATS 寄存器镜像容量：协议表最大 0x12A，留到 0x12F 方便整块读 */
#define ATS_REG_NUM                 (0x0130U)

/* 在线判定：连续多少个 100ms 任务周期没有新应答就判离线（5 -> 500ms） */
#define ATS_ALIVE_MAX               (5U)

/* 单从机问询地址：总线上只有一个从机时，主站可用 0xFB 发问，从机以自身地址应答 */
#define ATS_ADDR_INQUIRY            (0x00FBU)

/* 监听配置 */
typedef struct
{
    Uint16 u16SciId;      /* 监听哪条串口（ATS 与 ARM 共用的那条） */
    Uint16 u16SlaveAddr;  /* ATS 从机地址（1~247） */
} ST_ATS_PROTO_CFG;

/* 换算后的观测值（100ms 刷新，直接给点表/软示波器指向） */
typedef struct
{
    float32 fInFreq;      /* 输入（电网）正序频率 Hz，寄存器 0x8F */
    float32 fOutFreq;     /* 输出（逆变）正序频率 Hz，寄存器 0x99 */
    float32 fVoltDiff;    /* 正序输入输出电压差 V，寄存器 0x9A（有符号） */
    float32 fPhaseDiff;   /* 正序输入输出相位差 °，寄存器 0x9B（有符号，°） */
    float32 fInVoltA;     /* 输入 A 相电压 V，寄存器 0x80 */
    float32 fOutVoltA;    /* 输出 A 相电压 V，寄存器 0x90 */
    Uint16 u16Online;     /* 1 = ATS 在线 */
    Uint16 u16FrameCnt;   /* 已解析的 ATS 应答帧数 */
} ST_ATS_INFO;

extern Uint16 gAtsReg[ATS_REG_NUM];
extern ST_ATS_INFO gAtsInfo;
extern volatile Uint16 gAtsFrameCnt;
extern volatile Uint16 gAtsErrCnt;
extern volatile Uint16 gAtsAliveCnt;

void AtsProtoInit(const ST_ATS_PROTO_CFG *pstCfg);

/**
 * @brief 每收到一帧 Modbus 报文就喂一次（由 ModbusSlave 在地址过滤之前调用）
 * @param u16SciId 报文来自哪条串口
 * @param pu16Frame 帧缓存，一个字节占一个 Uint16 的低 8 位（与 ModbusSlave 一致）
 */
void AtsProtoOnFrame(Uint16 u16SciId, const Uint16 *pu16Frame, Uint16 u16Len);

/* 100ms 周期任务：在线判定 + 工程量换算 */
void AtsProtoTask(void *pArgV);

Uint16 AtsIsOnline(void);
Uint16 AtsGetReg(Uint16 u16Reg);
int16 AtsGetRegSigned(Uint16 u16Reg);

#endif

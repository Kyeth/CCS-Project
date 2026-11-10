/**
 * @file main.c
 * @author yetingheng (004182)
 * @brief Minimal F28374S peripheral bring-up and lifecycle entry points.
 * @brief 本工程主程序入口
 * @version 0.2
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 */

#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "F2837xS_Pie_defines.h"
#include "DrvCfg_EP15K.h"
#include "Hw_TimeBase.h"
#include "Hw_ClaTask.h"
#include "Hw_WatchDog.h"
#include "Schedule.h"
#include "BringupStatus.h"
#include "SciPort.h"
#include "CanPort.h"
#include "PortTask.h"
#include "PortBench.h"
#include "SampleData.h"
#include "AdcSample.h"
#include "Protocol.h"
#include "AppModbus.h"
#include "AppUpgrade.h"

/* ============================================================================
 * 上板测量开关（对应 05B 文档 §8.4）
 *
 * PORT_BENCH_TEST == 0U：不编译任何测量代码；与当前关闭态基线（非 ec59056）
 *   只允许存在 §2.5.4 记录的两处局部变量差异。
 * PORT_BENCH_TEST == 1U：编译回显/灌满任务与 ISR 打点，仅用于上板测量第 1~4 项。
 *
 * 测完改回 0U，重新构建并确认产物与基线一致后再提交；测量期间的打点差异需
 * 记录在 05B 文档 §8.3。
 * ==========================================================================*/

#if (PORT_BENCH_TEST != 0U)
#define BENCH_SCI_PORT   (HW_SCI_ID_D)   /* SCI-D：TX=GPIO93 / RX=GPIO94，38400 8N1 */
#define BENCH_BUF_WORDS  (128U)
/* Must stay below the TX ring capacity (128 slots -> 127 bytes):
 * SciPortWriteTx() uses RingBuffWriteExact(), which rejects the whole batch. */
#define BENCH_FLOOD_LEN  (64U)
#define BENCH_AMP        (4U)            /* 诊断模式 4/5/6 的重复次数 */
#define BENCH_GPIO_MARK  (33U)           /* 配置表里的"测试引脚"，独占不用 */

/* 三路自环帧：SOF(0x5A) | seq_lo | seq_hi | len(=8) | payload[8] | xor */
#define BENCH_LOOP_PORTS   (3U)
#define BENCH_FRAME_PAYLOAD (8U)
#define BENCH_FRAME_LEN    (Uint16)(4U + BENCH_FRAME_PAYLOAD + 1U)
#define BENCH_FRAME_SOF    (0x5AU)
#define BENCH_PARSE_BUF    (48U)

static Uint16 s_au16BenchBuf[BENCH_BUF_WORDS];
static ST_SCI_RX_SEG s_stBenchSeg;

/* 下面四个量从 CCS Expressions 里直接改/看（非 static，保证有符号） */
Uint16 g_u16BenchMode;            /* 0=回显 1=灌满 TX */
Uint32 g_u32BenchTxTotal;         /* 累计写入 TX 环的字节数 */
Uint32 g_u32BenchResumeTries;     /* 强制复位后解冻尝试次数 */
Uint32 g_u32BenchPopCalls;        /* 回显模式下成功取到的批次数 */

Uint32 g_u32IsrMarkTicks;         /* ISR 进入时刻的 Timer0 计数 */
Uint32 g_u32IsrMarkMaxTicks;      /* ISR 最长耗时（tick，200 tick = 1 us） */
Uint16 g_u16IsrMarkValid;

/* ---- 三路自环台架（mode 2）----
 * 用途：不用任何外部适配器，只把每路 TX 与 RX 用短接线连起来，就能同时跑三路
 * 带序号的收发，检查丢失/重复/乱序、环回绕满环、单路故障恢复与控制共存。
 *
 *   g_u16LoopRun       0=停 1=跑；由 0 变 1 时清零全部计数（干净起点）
 *   g_u16LoopPauseMask bit0=A bit1=C bit2=D，置 1 表示"暂停消费该路 RX"，
 *                      用来把软件环灌满（验证拒收计数与不死锁）
 *   g_u16LoopFlood     0=每毫秒每路最多发 1 帧；1=能写就写满（压力模式）
 *   g_u16LoopStopTx    1=停止灌入新帧，但继续解析已有数据（停发排空核对用）
 *
 * 判据看 g_u32LoopTxFrames/RxFrames 是否同步增长、SeqErr/LenErr/ChkErr/PayErr
 * 是否恒为 0，以及 g_u32LoopBytesTx/Rx 是否相等。 */
volatile Uint16 g_u16LoopRun;
volatile Uint16 g_u16LoopPauseMask;
volatile Uint16 g_u16LoopFlood;
volatile Uint16 g_u16LoopStopTx;

volatile Uint32 g_u32LoopTxFrames[3];
volatile Uint32 g_u32LoopRxFrames[3];
volatile Uint32 g_u32LoopBytesTx[3];
volatile Uint32 g_u32LoopBytesRx[3];
volatile Uint32 g_u32LoopSeqErr[3];
volatile Uint32 g_u32LoopLenErr[3];
volatile Uint32 g_u32LoopChkErr[3];
volatile Uint32 g_u32LoopPayErr[3];
volatile Uint32 g_u32LoopHdrErr[3];

static const Uint16 s_au16LoopPort[BENCH_LOOP_PORTS] =
    { HW_SCI_ID_A, HW_SCI_ID_C, HW_SCI_ID_D };
static Uint16 s_au16LoopSeq[BENCH_LOOP_PORTS];
static Uint16 s_au16LoopExp[BENCH_LOOP_PORTS];
static Uint16 s_au16LoopPos[BENCH_LOOP_PORTS];
static Uint16 s_au16LoopAcc[BENCH_LOOP_PORTS][BENCH_FRAME_LEN];
static Uint16 s_au16LoopBuf[BENCH_PARSE_BUF];
static Uint16 s_u16LoopWasRun;

/* GPIO33 打点模式。**不支持运行中切换**：只在两个 ISR 都退出的前台断点处
 * 修改，确认 GPIO33 为低后再恢复运行，并丢弃暂停前后的过渡样本。
 *
 *   0 = 心跳 ISR 全程（含三路 SCI 服务，配合 g_u32IsrMarkMaxTicks 看软件计时）
 *   1 = Isr_Control 全程（20 kHz 控制中断，默认；示波器直接量脉宽与周期）
 *   2 = 心跳 ISR 里 SCI 服务那一段
 *   3 = 心跳 ISR 里定时器分频与毫秒任务段
 *   4 = 诊断：正常服务之后再执行 4 轮 SciPortService100us()（该心跳共 5 轮服务，
 *       会推进排空/发布/观察记录，改变端口状态），只作专项诊断，不进对照数据
 *   5 = 诊断：连续 4 次 Hw_TimeBaseTicks()
 *   6 = 诊断：三路各 4 次 Hw_SciGetRxStatus()
 *   7 = SciPortService100us() 内那一次共享取时（每轮 1 个脉冲）
 *   8 = 正常 RX 路径上的单路状态查询（仅在走到该查询时才有脉冲；恢复态可能跳过，
 *       故不能预设恒为 3 个）
 *   9 = 正常 RX 路径上的单次读取尝试，含返回空的尝试（成功读 n<8 字节后再探空 =
 *       n+1 次调用；连续读满 8 字节 = 8 次；**不含** SciPortIsrDrop() 的排空读取）
 *  10 = 调用点外侧的单路服务调用及活动计数累加（每轮三路各一个脉冲，但三路
 *       路径与脉宽未必相同，须分别记录）
 *  11 = 空窗口参考：一个 if 内连续两次 BenchMarkPin()，其调用结构不同于 7~10
 *       （7~10 为"模式判断 → PortBenchMark → BenchMarkPin"），只作自身读数
 * 同一时刻只有一种模式生效，GPIO33 只有一个脚。 */
volatile Uint16 g_u16BenchMarkMode = 1U;

/**
 * @brief 只翻转打点引脚，不做软件计时（示波器测量用）。
 */
static void BenchMarkPin(Uint16 u16Level)
{
    Hw_GpioWritePin(BENCH_GPIO_MARK, u16Level);
}

/* 供 SciPort.c 在真实调用点打点（05C §2.4）：只翻转引脚，不做软件计时。 */
void PortBenchMark(Uint16 u16Level)
{
    BenchMarkPin(u16Level);
}

static void BenchLoopReset(void)
{
    Uint16 i;
    Uint16 k;

    for (i = 0U; i < BENCH_LOOP_PORTS; ++i)
    {
        s_au16LoopSeq[i] = 0U;
        s_au16LoopExp[i] = 0U;
        s_au16LoopPos[i] = 0U;
        g_u32LoopTxFrames[i] = 0UL;
        g_u32LoopRxFrames[i] = 0UL;
        g_u32LoopBytesTx[i] = 0UL;
        g_u32LoopBytesRx[i] = 0UL;
        g_u32LoopSeqErr[i] = 0UL;
        g_u32LoopLenErr[i] = 0UL;
        g_u32LoopChkErr[i] = 0UL;
        g_u32LoopPayErr[i] = 0UL;
        g_u32LoopHdrErr[i] = 0UL;
        for (k = 0U; k < BENCH_FRAME_LEN; ++k)
        {
            s_au16LoopAcc[i][k] = 0U;
        }
    }
}

/* 校验一整帧并更新计数；只在完整收满 13 字节时调用。 */
static void BenchLoopFinish(Uint16 u16Idx)
{
    Uint16 k;
    Uint16 u16Chk = 0U;
    Uint16 u16Seq;
    Uint16 u16Len;

    for (k = 0U; k < (Uint16)(BENCH_FRAME_LEN - 1U); ++k)
    {
        u16Chk = (Uint16)(u16Chk ^ s_au16LoopAcc[u16Idx][k]);
    }

    if (u16Chk != s_au16LoopAcc[u16Idx][BENCH_FRAME_LEN - 1U])
    {
        g_u32LoopChkErr[u16Idx]++;
        return;
    }

    u16Seq = (Uint16)(s_au16LoopAcc[u16Idx][1] |
                      (Uint16)(s_au16LoopAcc[u16Idx][2] << 8));
    u16Len = s_au16LoopAcc[u16Idx][3];

    if (u16Len != BENCH_FRAME_PAYLOAD)
    {
        g_u32LoopLenErr[u16Idx]++;
        return;
    }

    if (u16Seq != s_au16LoopExp[u16Idx])
    {
        g_u32LoopSeqErr[u16Idx]++;
    }
    s_au16LoopExp[u16Idx] = (Uint16)(u16Seq + 1U);

    for (k = 0U; k < BENCH_FRAME_PAYLOAD; ++k)
    {
        if (s_au16LoopAcc[u16Idx][4U + k] != (Uint16)((u16Seq + k) & 0xFFU))
        {
            g_u32LoopPayErr[u16Idx]++;
            break;
        }
    }

    g_u32LoopRxFrames[u16Idx]++;
}

static void BenchLoopParse(Uint16 u16Idx, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 i;
    Uint16 u16Byte;

    for (i = 0U; i < u16Count; ++i)
    {
        u16Byte = (Uint16)(pu16Data[i] & 0xFFU);

        if (s_au16LoopPos[u16Idx] == 0U)
        {
            if (u16Byte == BENCH_FRAME_SOF)
            {
                s_au16LoopAcc[u16Idx][0] = u16Byte;
                s_au16LoopPos[u16Idx] = 1U;
            }
            else
            {
                g_u32LoopHdrErr[u16Idx]++;
            }
        }
        else
        {
            s_au16LoopAcc[u16Idx][s_au16LoopPos[u16Idx]] = u16Byte;
            s_au16LoopPos[u16Idx]++;

            if (s_au16LoopPos[u16Idx] >= BENCH_FRAME_LEN)
            {
                BenchLoopFinish(u16Idx);
                s_au16LoopPos[u16Idx] = 0U;
            }
        }
    }
}

static void BenchLoopSend(Uint16 u16Idx)
{
    Uint16 au16Frame[BENCH_FRAME_LEN];
    Uint16 k;
    Uint16 u16Seq = s_au16LoopSeq[u16Idx];
    Uint16 u16Chk = 0U;

    au16Frame[0] = BENCH_FRAME_SOF;
    au16Frame[1] = (Uint16)(u16Seq & 0xFFU);
    au16Frame[2] = (Uint16)((u16Seq >> 8) & 0xFFU);
    au16Frame[3] = BENCH_FRAME_PAYLOAD;

    for (k = 0U; k < BENCH_FRAME_PAYLOAD; ++k)
    {
        au16Frame[4U + k] = (Uint16)((u16Seq + k) & 0xFFU);
    }

    for (k = 0U; k < (Uint16)(BENCH_FRAME_LEN - 1U); ++k)
    {
        u16Chk = (Uint16)(u16Chk ^ au16Frame[k]);
    }
    au16Frame[BENCH_FRAME_LEN - 1U] = u16Chk;

    if (SciPortWriteTx(s_au16LoopPort[u16Idx], au16Frame, BENCH_FRAME_LEN) ==
        BENCH_FRAME_LEN)
    {
        s_au16LoopSeq[u16Idx] = (Uint16)(u16Seq + 1U);
        g_u32LoopTxFrames[u16Idx]++;
        g_u32LoopBytesTx[u16Idx] += (Uint32)BENCH_FRAME_LEN;
    }
}

/* mode 2 主体：每路先补发、再不暂停地消费并解析。 */
static void BenchLoopTask(void)
{
    Uint16 i;
    Uint16 u16Got;
    Uint16 u16Port;

    if (g_u16LoopRun == 0U)
    {
        s_u16LoopWasRun = 0U;
        return;
    }

    if (s_u16LoopWasRun == 0U)
    {
        BenchLoopReset();
        s_u16LoopWasRun = 1U;
    }

    for (i = 0U; i < BENCH_LOOP_PORTS; ++i)
    {
        u16Port = s_au16LoopPort[i];

        /* 强制复位之后 TX 会冻结：恢复回 IDLE 后由这里解冻（模拟协议层行为）。 */
        if (SciPortIsTxSuspended(u16Port) != 0U)
        {
            if (SciPortRxState(u16Port) == SCI_RX_IDLE)
            {
                (void)SciPortTxResume(u16Port);
                g_u32BenchResumeTries++;
            }
        }
        else if ((g_u16LoopStopTx == 0U) &&
                 (SciPortTxFree(u16Port) >= BENCH_FRAME_LEN))
        {
            BenchLoopSend(i);

            if (g_u16LoopFlood != 0U)
            {
                while (SciPortTxFree(u16Port) >= BENCH_FRAME_LEN)
                {
                    BenchLoopSend(i);
                }
            }
        }
        else
        {
            /* 已停发，或 TX 环暂时不满也不空：本拍不发，交给发送服务推进 */
        }

        if ((g_u16LoopPauseMask & (Uint16)(1U << i)) == 0U)
        {
            do
            {
                u16Got = SciPortPopBatch(u16Port, s_au16LoopBuf, BENCH_PARSE_BUF,
                                         &s_stBenchSeg);
                if (u16Got != 0U)
                {
                    g_u32LoopBytesRx[i] += (Uint32)u16Got;
                    BenchLoopParse(i, s_au16LoopBuf, u16Got);
                }
            } while (u16Got != 0U);   /* 排空为止：小段连续到达时一段一拍会追不上 */
        }
    }
}

/**
 * @brief 1 ms 回显/灌满/自环测试任务。
 */
void BenchTask(void)
{
    Uint16 u16Got;
    Uint16 u16Put;

    if (g_u16BenchMode == 2U)                    /* 三路自环带序号验收 */
    {
        BenchLoopTask();
        return;
    }

    /* 强制复位后 TX 冻结，这里模拟协议层：恢复回 IDLE 后主动解冻 */
    if (SciPortIsTxSuspended(BENCH_SCI_PORT) != 0U)
    {
        if (SciPortRxState(BENCH_SCI_PORT) == SCI_RX_IDLE)
        {
            (void)SciPortTxResume(BENCH_SCI_PORT);
            g_u32BenchResumeTries++;
        }
        return;
    }

    if (g_u16BenchMode == 0U)                    /* 回显：收到什么发回什么 */
    {
        u16Got = SciPortPopBatch(BENCH_SCI_PORT, s_au16BenchBuf,
                                 (Uint16)(BENCH_BUF_WORDS / 2U), &s_stBenchSeg);
        if (u16Got != 0U)
        {
            g_u32BenchPopCalls++;
            u16Put = SciPortWriteTx(BENCH_SCI_PORT, s_au16BenchBuf, u16Got);
            g_u32BenchTxTotal += (Uint32)u16Put;
        }
    }
    else                                         /* 灌满 TX：0x40..0x7F 循环，便于识别残留 */
    {
        Uint16 i;
        for (i = 0U; i < BENCH_FLOOD_LEN; ++i)
        {
            s_au16BenchBuf[i] = (Uint16)(0x40U + (i & 0x3FU));
        }
        u16Put = SciPortWriteTx(BENCH_SCI_PORT, s_au16BenchBuf, BENCH_FLOOD_LEN);
        g_u32BenchTxTotal += (Uint32)u16Put;
    }
}

/**
 * @brief ISR 入口打点：拉高 GPIO33 并记下 Timer0 计数。
 */
static void BenchIsrMarkBegin(void)
{
    Hw_GpioWritePin(BENCH_GPIO_MARK, 1U);
    if (Hw_TimeBaseIsReady() == 0U)
    {
        g_u16IsrMarkValid = 0U;
        return;
    }
    g_u16IsrMarkValid = 1U;
    g_u32IsrMarkTicks = Hw_TimeBaseTicks();
}

/**
 * @brief ISR 出口打点：拉低 GPIO33，更新最长耗时。
 */
static void BenchIsrMarkEnd(void)
{
    Uint32 u32Elapsed;

    Hw_GpioWritePin(BENCH_GPIO_MARK, 0U);
    if (g_u16IsrMarkValid == 0U)
    {
        return;
    }
    u32Elapsed = Hw_TimeBaseElapsed(g_u32IsrMarkTicks);
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return;
    }
    if (u32Elapsed > g_u32IsrMarkMaxTicks)
    {
        g_u32IsrMarkMaxTicks = u32Elapsed;
    }
}
#endif /* PORT_BENCH_TEST */

static __interrupt void Isr_Control(void);
static __interrupt void Isr_Heartbeat(void);

/**
 * @brief Configure interrupt vectors and enable masks before peripherals run.
 */
static void InterruptVectTableInit(void)
{
    DINT;
    InitPieCtrl();
    IER = 0U;
    IFR = 0U;
    InitPieVectTable();

    EALLOW;
    PieVectTable.CLA1_1_INT = &Isr_Control;
    PieVectTable.TIMER1_INT = &Isr_Heartbeat;
    EDIS;

    PieCtrlRegs.PIEIER11.bit.INTx1 = 1U;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
    IER |= (M_INT11 | M_INT13);
}

/**
 * @brief Initialize the chip and table-driven board peripherals.
 */
void PeripheralsInit(void)
{
    Uint16 i;

    if (Hw_ClockInit(&stClockCfg) == 0U ||
        stSciCfg.u32LspclkHz != stClockCfg.u32LspclkHz ||
        stCanCfg.u32SysclkHz != stClockCfg.u32SysclkHz)
    {
        stBringupStatus.unError.Bits.bClockInitError = 1U;
    }

    /* Free-running time base (CPU Timer0). Every time-bounded wait depends on it,
     * so it must run before CAN or any other module starts waiting on time. */
    if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
        (Hw_TimeBaseInit(stClockCfg.u32SysclkHz) == 0U))
    {
        stBringupStatus.unError.Bits.bCanInitError = 1U;
    }
    InterruptVectTableInit();

    /* The Broad table drives GPIO88 and all relay controls low at boot. */
    Hw_GpioInit(&stGpioCfg);
    Hw_GpioWritePin(88U, 0U);

    Hw_PwmInit(&stPwmCfg);
    Hw_StopAllPWM();
    Hw_XbarInit(&stXbarCfg);
    Hw_PwmInitTripZone(&stPwmTzCfg);
    Hw_AdcInit(&stAdcCfg);
    if (Hw_ClaInit(&stClaCfg) == 0U)
    {
        stBringupStatus.unError.Bits.bClaInitError = 1U;
    }

    AdcSampleInit();

    CpuToCla1Msg.u16MuxAddress = 0U;
    for (i = 0U; i < 8U; i++)
    {
        CpuToCla1Msg.u16NextMux[i] = stClaMuxNext[i];
    }
    Hw_GpioWritePin(46U, 0U);
    Hw_GpioWritePin(47U, 0U);
    Hw_GpioWritePin(42U, 0U);

    if (Hw_SciInit(&stSciCfg) == 0U)
    {
        stBringupStatus.unError.Bits.bSciInitError = 1U;
    }
    /* CAN waits are time-bounded: only start it when the clock is good and the
     * time base really runs. A failed time base blocks CAN instead of hanging. */
    if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
        (stBringupStatus.unError.Bits.bCanInitError == 0U) &&
        (Hw_TimeBaseIsReady() != 0U))
    {
        if (Hw_CanInit(&stCanCfg) == 0U)
        {
            stBringupStatus.unError.Bits.bCanInitError = 1U;
        }
    }
    else
    {
        stBringupStatus.unError.Bits.bCanInitError = 1U;
    }
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
}

/**
 * @brief Initialize platform components: scheduler, then the port layer.
 *
 * 05A only instantiates the ports (rings + driver bindings) so their storage is
 * real and visible to the linker. Neither SciPortService100us() nor
 * CanPortService() is called yet, and no SCI/CAN interrupt is enabled; the
 * wiring that drives them belongs to 05B.
 */
void PlatformInit(void)
{
    Task_Init();
    SciPortInit();
    CanPortInit();

    /* 每模块的发送帧类型固定：CAN-A 标准帧、CAN-B 扩展帧。
       失败只影响端口层可用性，所以单独记一位，不阻断 PWM 放行。 */
    if (CanPortSetTxFrameType(HW_CAN_ID_B, 1U) == 0U)
    {
        stBringupStatus.unError.Bits.bPortInitError = 1U;
    }
}

/* 链接脚本里的搬运符号：LOAD 在 Flash，RUN 在 RAM */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadSize;
extern Uint16 RamfuncsRunStart;
extern Uint16 _FlashfuncLoadStart;
extern Uint16 _FlashfuncLoadSize;
extern Uint16 _FlashfuncRunStart;

/**
 * @brief 把 LOAD 在 Flash、RUN 在 RAM 的代码段搬到运行位置
 * @note  这两段必须在任何被搬函数被调用之前完成。启动代码原本没有搬运：
 *        .TI.ramfunc 里的 Hw_GpioWritePin() 会跳到未初始化的 RAMD0；
 *        .flashfunc 里的 F021 调用链与擦写循环更是必须先从 Flash 拷出来。
 *        符号的"地址"就是长度，这是链接器给 LOAD_SIZE 的约定。
 */
static void CopyRunFromLoad(Uint16 *pu16Run, const Uint16 *pu16Load, Uint16 u16Words)
{
    Uint16 i;

    for (i = 0U; i < u16Words; i++)
    {
        pu16Run[i] = pu16Load[i];
    }
}

/* 升级期间：停功率输出、压制采样/控制动作。端口服务与时基必须继续跑，
   否则擦除期间 'C'/ACK 根本发不出去。 */
static volatile Uint16 s_u16UpgradeHold = 0U;

static void AppUpgradePause(void)
{
    s_u16UpgradeHold = 1U;
    Hw_StopAllPWM();
    Hw_GpioWritePin(88U, 0U);
}

/**
 * @brief 升级结束（含取消）后的恢复
 * @note  按 05F 裁定：不自动重新开启功率输出，只解除采样/控制压制。
 */
static void AppUpgradeResume(void)
{
    s_u16UpgradeHold = 0U;
}

/**
 * @brief 采样任务外面套一层：升级期间不出新的控制动作
 */
static void AdcSampleTaskGuarded(void)
{
    if (s_u16UpgradeHold == 0U)
    {
        AdcSampleTask();
    }
}

static void CopyRamSections(void)
{
    CopyRunFromLoad(&RamfuncsRunStart, &RamfuncsLoadStart,
                    (Uint16)(((Uint32)&RamfuncsLoadSize) & 0xFFFFUL));
    CopyRunFromLoad(&_FlashfuncRunStart, &_FlashfuncLoadStart,
                    (Uint16)(((Uint32)&_FlashfuncLoadSize) & 0xFFFFUL));
}

/**
 * @brief Initialize application data objects when an application is added.
 */
void ApplicationInit(void)
{
    /* Modbus point tables + protocol instance table. The tables are
       validated here so a bad binding is reported at startup, not when
       the first request arrives. */
    AppModbusInit();

    /* 升级层：绑 XMODEM 传输/数据/时基回调，装 F021 擦写操作表。必须在
       协议任务起来之前完成，否则握手交接时升级会话还没准备好。 */
    AppUpgradeInit();
    AppUpgradeSetPauseHook(AppUpgradePause, AppUpgradeResume);
}

/**
 * @brief Register background tasks when application services are added.
 */
void TaskCreate(void)
{
    /* 1 kHz display filter over the L3 / L4 sampling results. */
    if (Task_Start(AdcSampleTaskGuarded, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }

    /* Foreground port service: one unit per 100 us opportunity, driven by the
       heartbeat notification and consumed here in the main loop. The scheduler
       no longer registers a 1 ms port task: that would be a second entry that
       could consume the TX rings or touch CAN behind this one. A port layer
       that failed to initialize is never enabled. */
    if (stBringupStatus.unError.Bits.bPortInitError == 0U)
    {
        (void)PortTaskRegister();
    }

    /* Protocol service runs as its own 1 ms task. 1 ms is the task
       period, not the RTU timing resolution: frame gap and frame
       timeout come from the batch observation ticks, not from the
       number of task runs. */
    if (Task_Start(ProtocolTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }

    /* 升级任务：空闲时直接返回；收到握手后每轮推进一步（擦除/收包/校验）。 */
    if (Task_Start(AppUpgradeTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }

#if (PORT_BENCH_TEST != 0U)
    /* 上板测量任务（§8.4）：回显或灌满 TX，1 ms 一次 */
    if (Task_Start(BenchTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }
#endif
}

/**
 * @brief Start time bases and interrupts without releasing power outputs.
 */
void StartSystem(void)
{
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);
    if (stBringupStatus.unError.all == 0U)
    {
        Hw_EnableAllPwmCLKSync();
        Hw_OpenAllPWM();
        stBringupStatus.bReady = 1U;
    }

    EINT;
    ERTM;
    Hw_EnableWatchDog();
    Hw_FeedWatchDog();
}

static __interrupt void Isr_Control(void)
{
    Uint16 next;
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 1U)
    {
        BenchMarkPin(1U);
    }
#endif
    next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;

    /* Sample the 20 result registers of this tick before advancing the MUX. */
    AdcSampleFast();

    stBringupStatus.u32ControlIsrCount++;
    stBringupStatus.u16CalState = stSampleZero.u16State;
    stBringupStatus.bCalReady = (stSampleZero.u16Valid != 0U) ? 1U : 0U;

    /* CLA selects the next MUX address; CPU owns the GPIO writes. */
    Hw_GpioWritePin(46U, next & 1U);
    Hw_GpioWritePin(47U, (next >> 1U) & 1U);
    Hw_GpioWritePin(42U, (next >> 2U) & 1U);
    CpuToCla1Msg.u16MuxAddress = next;

    Cla1Regs.MICLR.bit.INT1 = 1U;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 1U)
    {
        BenchMarkPin(0U);
    }
#endif
}

static __interrupt void Isr_Heartbeat(void)
{
    static Uint32 lastControlCount = 0UL;

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 0U)
    {
        BenchIsrMarkBegin();
    }
#endif

    /* TIF is cleared here only; Hw_CpuTimerIsrTick() does not touch it. */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    stBringupStatus.u32Tick100us++;

    /* 100 us SCI RX service runs before the 1 ms divider so it keeps its own
       cadence; its budget is bounded by SCI_PORT_RX_QUOTA x 8 reads.
       PortTaskServiceFromHeartbeat() honours bPortInitError: a port layer that
       failed to initialize is never serviced from here either. */
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 2U)
    {
        BenchMarkPin(1U);
    }
#endif
    PortTaskServiceFromHeartbeat(stBringupStatus.unError.Bits.bPortInitError);
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 2U)
    {
        BenchMarkPin(0U);
    }
#endif
#if (PORT_BENCH_TEST != 0U)
    /* 模式 11：空窗口参考——两次 BenchMarkPin() 之间不放任何代码。
       它不含 7~10 的模式判断与跨文件入口，结构不同，不能当作等价标定。 */
    if (g_u16BenchMarkMode == 11U)
    {
        BenchMarkPin(1U);
        BenchMarkPin(0U);
    }
#endif
#if (PORT_BENCH_TEST != 0U)
    /* 诊断模式 4/5/6：同一段重复 BENCH_AMP 次，用来拆出每轮、每次取时、
       每次状态查询各自的单价。注意模式 4 会让该心跳内共执行 5 轮正常服务，
       改变端口状态，只作专项诊断。 */
    if ((g_u16BenchMarkMode == 4U) || (g_u16BenchMarkMode == 5U) ||
        (g_u16BenchMarkMode == 6U))
    {
        ST_SCI_RX_STATUS stTmpStatus;
        Uint16 i2;

        BenchMarkPin(1U);
        for (i2 = 0U; i2 < BENCH_AMP; i2++)
        {
            if (g_u16BenchMarkMode == 4U)
            {
                (void)SciPortService100us();
            }
            else if (g_u16BenchMarkMode == 5U)
            {
                (void)Hw_TimeBaseTicks();
            }
            else
            {
                (void)Hw_SciGetRxStatus(HW_SCI_ID_A, &stTmpStatus, 0U);
                (void)Hw_SciGetRxStatus(HW_SCI_ID_C, &stTmpStatus, 0U);
                (void)Hw_SciGetRxStatus(HW_SCI_ID_D, &stTmpStatus, 0U);
            }
        }
        BenchMarkPin(0U);
    }
#endif

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 3U)
    {
        BenchMarkPin(1U);
    }
#endif
    /* u16TickDivCoef = 10: one return of 1 every 10 hardware interrupts, i.e. 1 ms. */
    if (Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer1) != 0U)
    {
        stBringupStatus.u32HeartbeatMs++;
        Task_OnTick();

        if (lastControlCount == stBringupStatus.u32ControlIsrCount)
        {
            if (stBringupStatus.u16ControlStallMs != 0xFFFFU)
            {
                stBringupStatus.u16ControlStallMs++;
            }
        }
        else
        {
            stBringupStatus.u16ControlStallMs = 0U;
            lastControlCount = stBringupStatus.u32ControlIsrCount;
        }
    }
#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 3U)
    {
        BenchMarkPin(0U);
    }
#endif

#if (PORT_BENCH_TEST != 0U)
    if (g_u16BenchMarkMode == 0U)
    {
        BenchIsrMarkEnd();
    }
#endif
}

void main(void)
{
    CopyRamSections(); // 先把 ROM 里的 RAM 代码段搬到位
    PeripheralsInit(); // Initialize peripherals.
    PlatformInit();    // Initialize platform components.
    ApplicationInit(); // Initialize application data objects.
    TaskCreate();      // Register all background tasks.
    StartSystem();     // Start the system.

    while (1)
    {
        Hw_FeedWatchDog(); // Feed the watchdog.
        (void)PortTaskService(); // 有通知才服务一个单元，没有就立刻返回
        Task_Run();
    }
}

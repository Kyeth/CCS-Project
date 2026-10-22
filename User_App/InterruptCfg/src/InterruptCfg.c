/**
 * @file  InterruptCfg.c
 * @author yetingheng  (004182)
 * @brief 中断配置：PIE 初始化、中断向量注册与中断服务函数
 * @version 0.2
 * @date 2026-10-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "InterruptCfg.h"
#include "F2837xS_device.h"
#include "F2837xS_GlobalPrototypes.h"
#include "F2837xS_Pie_defines.h"
#include "DrvCfg_EP15K.h"
#include "Schedule.h"
#include "CtrlIsr.h"

/* 本工程使用的中断源之一：PIE 组 1（CPU Timer1 / Timer2） */
#define ISR_PIE_GROUP1_MASK         (M_INT1)

/* PIE 组 1 中 CPU Timer1 / Timer2 对应的通道号 */
#define ISR_PIE_CH_TINT1            (13U)
#define ISR_PIE_CH_TINT2            (14U)

/* 本工程使用的另一个中断源：PIE 组 11（CLA1 中断） */
#define ISR_PIE_GROUP11_MASK        (M_INT11)

/* PIE 组 11 中 CLA1 Task1 完成中断的通道号 */
#define ISR_PIE_CH_CLA1_1           (1U)

void InterruptVectTableInit(void)
{
    /* --- 中断系统初始化 --- */
    DINT;                       /* 关全局中断，配置期间不允许响应 */
    InitPieCtrl();              /* 初始化 PIE 控制器（关 PIE 使能，清标志） */
    IER = 0x0000U;              /* 清除所有 CPU 级中断使能 */
    IFR = 0x0000U;              /* 清除所有 CPU 级中断标志 */
    InitPieVectTable();         /* PIE 向量表全部指向默认空函数 */

    /* --- 注册中断服务函数 --- */
    EALLOW;
    PieVectTable.TIMER1_INT = &Isr_CpuTimer1;
    PieVectTable.TIMER2_INT = &Isr_CpuTimer2;
    PieVectTable.CLA1_1_INT = &Isr_Cla1Task1;
    EDIS;

    /* --- 使能 PIE 组内通道，并开放对应的 CPU 级中断 --- */
    PieCtrlRegs.PIEIER1.bit.INTx13 = 1U;    /* CPU Timer1 */
    PieCtrlRegs.PIEIER1.bit.INTx14 = 1U;    /* CPU Timer2 */
    PieCtrlRegs.PIEIER11.bit.INTx1 = 1U;    /* CLA1 Task1 完成 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; /* 应答组 1，允许同组后续中断进入 */
    IER |= ISR_PIE_GROUP1_MASK;

    /* --- 同样开放 CLA1_1（PIE 组 11），20kHz 控制中断 --- */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11; /* 应答组 11，允许同组后续中断进入 */
    IER |= ISR_PIE_GROUP11_MASK;
}

__interrupt void Isr_CpuTimer1(void)
{
    /* 清除定时器硬件中断标志 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    /* 100us 软件心跳：满一个心跳驱动一次任务调度器 */
    if (1U == Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer1))
    {
        SYS_TICK();
    }

    /* 应答 PIE 组 1 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void Isr_CpuTimer2(void)
{
    /* 清除定时器硬件中断标志 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer2);

    /* 1ms 软件心跳：目前只累加心跳计数，毫秒级任务后续挂在这里 */
    (void)Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer2);

    /* 应答 PIE 组 1 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

/* CLA1 Task1 完成中断：20kHz 控制中断 */
__interrupt void Isr_Cla1Task1(void)
{
    /* 控制主体：PLL 锁相、环路占空比下发、过流与 CLA 心跳封波检测 */
    CtrlIsrRun();

    /* 应答 PIE 组 11，允许下一拍 CLA1_1 中断进入 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
}

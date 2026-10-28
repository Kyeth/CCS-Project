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
#include "Hw_WatchDog.h"

/* PIE 组 1：XINT1（1.4）/ XINT2（1.5） */
#define ISR_PIE_GROUP1_MASK         (M_INT1)

/* CPU Timer1 / Timer2 是核心中断 INT13 / INT14（PIE_VECT_TABLE 索引 13/14），
 * 不占 PIE 组 1 的通道，只能通过 IER 的 M_INT13 / M_INT14 开放。 */
#define ISR_CPUINT_TIMER_MASK       (M_INT13 | M_INT14)

/* 本工程使用的另一个中断源：PIE 组 11（CLA1 中断） */
#define ISR_PIE_GROUP11_MASK        (M_INT11)

/* PIE 组 11 中 CLA1 Task1 完成中断的通道号 */
#define ISR_PIE_CH_CLA1_1           (1U)

/* PIE 组 8：SCIC/SCID 收发中断 */
#define ISR_PIE_GROUP8_MASK         (M_INT8)
#define ISR_PIE_CH_SCIC_RX          (5U)
#define ISR_PIE_CH_SCID_RX          (7U)

/* PIE 组 9：SCIA 收发中断 */
#define ISR_PIE_GROUP9_MASK         (M_INT9)

#define ISR_PIE_CH_SCIA_RX          (1U)

/* 控制链停摆后仍允许喂狗的最大节拍数：20ms，覆盖启动与模式切换瞬态 */
#define ISR_WDG_GRACE_TICK          (200U)

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
    PieVectTable.SCIA_RX_INT = &Isr_SciaRx;
    PieVectTable.SCIC_RX_INT = &Isr_ScicRx;
    PieVectTable.SCID_RX_INT = &Isr_ScidRx;
    PieVectTable.XINT1_INT = &Isr_Xint1;
    PieVectTable.XINT2_INT = &Isr_Xint2;
    EDIS;

    /* --- 使能 PIE 组内通道，并开放对应的 CPU 级中断 --- */
    /* CPU Timer1 / Timer2 不在 PIE 通道里，见下方 IER 置位 */
    PieCtrlRegs.PIEIER11.bit.INTx1 = 1U;    /* CLA1 Task1 完成 */
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1U;     /* SCIA 接收 */
    PieCtrlRegs.PIEIER8.bit.INTx5 = 1U;     /* SCIC 接收 */
    PieCtrlRegs.PIEIER8.bit.INTx7 = 1U;     /* SCID 接收 */
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1U;     /* XINT1：逆变过流比较器 */
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1U;     /* XINT2：硬件短路比较器 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; /* 应答组 1，允许同组后续中断进入 */
    IER |= ISR_PIE_GROUP1_MASK;

    /* --- 开放 CPU Timer1 / Timer2：核心中断 INT13 / INT14，不经 PIE --- */
    IER |= ISR_CPUINT_TIMER_MASK;

    /* --- 同样开放 CLA1_1（PIE 组 11），20kHz 控制中断 --- */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11; /* 应答组 11，允许同组后续中断进入 */
    IER |= ISR_PIE_GROUP11_MASK;

    /* --- 开放 SCIA（组 9）与 SCIC/SCID（组 8）的接收中断 --- */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
    IER |= ISR_PIE_GROUP9_MASK;
    IER |= ISR_PIE_GROUP8_MASK;
}

__interrupt void Isr_CpuTimer1(void)
{
    static Uint16 sWdgCtrlCnt = 0U;
    static Uint16 sWdgGrace   = ISR_WDG_GRACE_TICK;
    Uint16 u16CtrlCnt;

    /* 清除定时器硬件中断标志 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    /* 100us 软件心跳：满一个心跳驱动一次任务调度器 */
    if (1U == Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer1))
    {
        SYS_TICK();         /* 自由运行节拍 +1，后台按差值推进，欠拍不丢拍 */
        Task_HealthTick();  /* 后台活性窗口裁决，200ms 收口一次 */

        /* 喂狗条件：20kHz 控制链仍在推进。
         * 控制链停摆（CLA 不触发、ADC 无转换、中断被关）时这里停止喂狗，
         * 由硬件看门狗按 1.6384ms 上限复位；后台任务停摆不在此复位，
         * 那属于 Task stall 故障，由控制中断封锁发波。
         * 硬件看门狗最大超时仅 1.6384ms，喂狗点只能放在中断里。 */
        u16CtrlCnt = CtrlGetCycleCnt();

        if (u16CtrlCnt != sWdgCtrlCnt)
        {
            sWdgCtrlCnt = u16CtrlCnt;
            sWdgGrace   = ISR_WDG_GRACE_TICK;
            Hw_FeedWatchDog();
        }
        else if (sWdgGrace > 0U)
        {
            sWdgGrace--;    /* 启动与模式切换瞬态宽限 */
            Hw_FeedWatchDog();
        }
    }

    /* 核心中断 INT13 不经 PIE，这里不写 PIEACK */
}

__interrupt void Isr_CpuTimer2(void)
{
    /* 清除定时器硬件中断标志 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer2);

    /* 1ms 软件心跳：目前只累加心跳计数，毫秒级任务后续挂在这里 */
    (void)Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer2);

    /* 核心中断 INT14 不经 PIE，这里不写 PIEACK */
}

/* CLA1 Task1 完成中断：20kHz 控制中断 */
__interrupt void Isr_Cla1Task1(void)
{
    /* 控制主体：ADC->CLA 触发链已实测为通，20 kHz 主体就挂在这里 */
    CtrlIsrRun();

    /* 清 CLA 的 MIFR.INT1：CLA1_1 是电平式请求，不清标志会在 PIEACK 后立刻重入 */
    Cla1Regs.MICLR.bit.INT1 = 1U;

    /* 应答 PIE 组 11，允许下一拍 CLA1_1 中断进入 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
}

/* SCI-A 接收中断：只把字节搬进环形缓存，解析交给任务 */
__interrupt void Isr_SciaRx(void)
{
    Hw_SciRxIsr((Uint16)HW_SCI_ID_A);

    /* 应答 PIE 组 9 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/* SCI-C 接收中断：TVS 相位差数据 */
__interrupt void Isr_ScicRx(void)
{
    Hw_SciRxIsr((Uint16)HW_SCI_ID_C);

    /* 应答 PIE 组 8 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}

/* SCI-D 接收中断：上位机点表读写 */
__interrupt void Isr_ScidRx(void)
{
    Hw_SciRxIsr((Uint16)HW_SCI_ID_D);

    /* 应答 PIE 组 8 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}

/* XINT1：逆变过流比较器，中断里只计数，判定放 10kHz 保护任务 */
__interrupt void Isr_Xint1(void)
{
    Hw_ExitIsr((Uint16)HW_XINT_ID1);

    /* 应答 PIE 组 1 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

/* XINT2：硬件短路比较器 */
__interrupt void Isr_Xint2(void)
{
    Hw_ExitIsr((Uint16)HW_XINT_ID2);

    /* 应答 PIE 组 1 */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

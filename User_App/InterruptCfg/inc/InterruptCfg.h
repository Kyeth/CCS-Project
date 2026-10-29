#ifndef __INTERRUPTCFG_H
#define __INTERRUPTCFG_H

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief 工程中断清单（新增外设中断时先在此登记，再在 InterruptCfg.c 中注册）
 *
 * | 中断源          | PIE 组/通道 | ISR 名称          | 状态   |
 * |-----------------|-------------|-------------------|--------|
 * | CLA1 Task1 完成 | INT11.1     | Isr_Cla1Task1     | 已完成 |
 * | CLA1 Task2 完成 | INT11.2     | Isr_Cla1Task2     | 已完成 |
 * | CPU Timer1      | INT1.13     | Isr_CpuTimer1     | 已完成 |
 * | CPU Timer2      | INT1.14     | Isr_CpuTimer2     | 已完成 |
 * | ePWM TZ / ADC   | 待定        | 待定              | 待实现 |
 * | SCI / CAN / ECAP| 待定        | 待定              | 待实现 |
 * | SCI-A 接收      | INT9.1      | Isr_SciaRx        | 已完成 |
 * | SCI-C 接收      | INT8.5      | Isr_ScicRx        | 已完成 |
 * | SCI-D 接收      | INT8.7      | Isr_ScidRx        | 已完成 |
 * | XINT1 (外部)    | INT1.4      | Isr_Xint1         | 已完成 |
 * | XINT2 (外部)    | INT1.5      | Isr_Xint2         | 已完成 |
 */

/* 初始化 PIE 中断控制器、复位中断向量表并注册本工程使用的中断 */
extern void InterruptVectTableInit(void);

/* CPU Timer1 中断：100us 系统节拍，驱动调度器 SYS_TICK() */
extern __interrupt void Isr_CpuTimer1(void);

/* CPU Timer2 中断：1ms 毫秒心跳，供喂狗与慢速任务使用 */
extern __interrupt void Isr_CpuTimer2(void);

/* CLA1 Task1 完成中断：20kHz 控制中断，PLL/环路/封波，内部调用 CtrlIsrRun() */
extern __interrupt void Isr_Cla1Task1(void);
extern __interrupt void Isr_Cla1Task2(void);

/* SCI-A 接收中断：把 ARM 发来的字节搬进环形缓存 */
extern __interrupt void Isr_SciaRx(void);

/* SCI-C 接收中断：把 TVS 发来的字节搬进环形缓存 */
extern __interrupt void Isr_ScicRx(void);

/* SCI-D 接收中断：把上位机发来的字节搬进环形缓存 */
extern __interrupt void Isr_ScidRx(void);

/* XINT1：逆变过流比较器（GPIO81）下降沿，只累加事件计数，判定在 10kHz 保护任务 */
extern __interrupt void Isr_Xint1(void);

/* XINT2：硬件短路比较器（GPIO82）下降沿 */
extern __interrupt void Isr_Xint2(void);

#ifdef __cplusplus
    }
#endif

#endif

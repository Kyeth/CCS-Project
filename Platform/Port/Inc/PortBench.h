/**
 * @file PortBench.h
 * @brief 上板测量开关（05B §8.4 / 05C §2.4）。测量专用，不参与生产逻辑。
 *
 * 单一来源：main.c 与 SciPort.c 都从这里取 PORT_BENCH_TEST，避免两处定义不一致。
 * 测量结束后必须改回 0U 并重建，确认产物与基线一致再提交。
 */
#ifndef PORT_BENCH_H
#define PORT_BENCH_H

#define PORT_BENCH_TEST (1U)

#if (PORT_BENCH_TEST != 0U)

/* 打点模式（CCS Expressions 里修改，但**不支持运行中切换**）。
 *
 *   volatile 只保证每次读取都回到内存，不保证置高与清零读到同一个值：模式在
 *   两个宏之间改变会造成"置高后不清零"或"没有对应高沿的清零"。因此约定：
 *   只在两个 ISR 都已退出的前台断点处暂停 → 修改模式 → 确认 GPIO33 为低 →
 *   恢复运行 → 丢弃暂停前后的过渡样本，再开始采集。
 *
 *   0 = 心跳 ISR 全程          1 = Isr_Control 全程
 *   2 = 心跳里 SCI 服务整段    3 = 心跳里定时器分派与毫秒任务
 *   4 = 诊断：正常服务之后再重复 4 轮服务（会改变端口状态，仅专项诊断）
 *   5/6 = 诊断：重复取时 / 重复状态查询
 *   7 = 共享取时一次           8 = 单次状态查询（正常 RX 路径）
 *   9 = 正常 RX 路径的单次读取尝试（不含恢复排空）
 *  10 = 单路服务调用及活动计数累加
 *  11 = 空窗口参考（结构不同于 7~10，只作自身读数）
 */
extern volatile Uint16 g_u16BenchMarkMode;

void PortBenchMark(Uint16 u16Level);

#define PORT_BENCH_MARK_MODE(mode, level) \
    do { if (g_u16BenchMarkMode == (mode)) { PortBenchMark(level); } } while (0)

#else

#define PORT_BENCH_MARK_MODE(mode, level) ((void)0)

#endif /* PORT_BENCH_TEST */

#endif /* PORT_BENCH_H */

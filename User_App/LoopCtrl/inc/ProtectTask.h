/**
 * @file ProtectTask.h
 * @author yetingheng (004182)
 * @brief 10kHz 硬件过流/短路判定任务：漏桶计数、故障锁存、逆变封波
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note 分工：
 *       - XINT1/XINT2 中断只累加事件数，不判故障（见 Hw_ExitDriver.c）；
 *       - 本任务每 100us（10kHz）取一次事件增量，做漏桶/窗口判定：
 *           逆变过流：累计 400 次事件、且中间没有连续 5ms 静默 -> 判过流
 *           硬件短路：10ms 窗口内出现 3 次事件 -> 判短路
 *         两套门限都沿用 EP5K 的实测经验值（EP5K 的 HandleProtect，10kHz 调用）；
 *       - 判故障后立刻 Hw_InvStopAll() 封波，并把故障位锁进控制中断，
 *         防止 CtrlIsrRun 在下一拍又把占空比写回。
 *
 *       为什么用漏桶而不是“一次就封”：比较器输出本身就是限流的触发源，
 *       真正的逐波限流已经由 ePWM 的 CBC 硬件完成（见 Hw_PwmInitTripZone），
 *       软件这里只判“持续过流”，一次尖峰不该停机。
 */

#ifndef PROTECT_TASK_H
#define PROTECT_TASK_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 逆变过流判据：累计事件数 / 连续静默拍数（10kHz 下 50 拍 = 5ms） */
#define PROTECT_INV_OC_TRIP_CNT         (400U)
#define PROTECT_INV_OC_CLEAN_TICK       (50U)

/* 短路判据：事件次数 / 观察窗口（10kHz 下 100 拍 = 10ms） */
#define PROTECT_SHORT_TRIP_CNT          (3U)
#define PROTECT_SHORT_WINDOW_TICK       (100U)

/* 故障位 */
#define PROTECT_FAULT_NONE              (0x0000U)
#define PROTECT_FAULT_INV_OC            (0x0001U)   /* bit0：逆变过流 */
#define PROTECT_FAULT_SHORT             (0x0002U)   /* bit1：硬件短路 */

/* 初始化：清漏桶与锁存，需在 EINT 之后、任务启动之前调用 */
extern void ProtectTaskInit(void);

/* 10kHz 判定任务，挂 Task_Start(&ProtectTask, NULL, 1U, 3U) */
extern void ProtectTask(void *pArgV);

/* 查询故障锁存位（Modbus 只读镜像也读这个） */
extern Uint16 ProtectGetFaultFlags(void);

/* 清故障锁存（同时清控制中断里的故障位），返回清除前的故障位 */
extern Uint16 ProtectClearFault(void);

/* 只读镜像：给 Modbus 点表直接取地址用 */
extern volatile Uint16 gProtectInvOcCnt;        /* 当前漏桶计数 */
extern volatile Uint16 gProtectShortCnt;        /* 当前窗口内短路次数 */
extern volatile Uint16 gProtectFaultFlags;      /* 故障锁存位 */
extern volatile Uint16 gProtectTick;            /* 已运行拍数（10kHz） */

#ifdef __cplusplus
}
#endif

#endif /* PROTECT_TASK_H */

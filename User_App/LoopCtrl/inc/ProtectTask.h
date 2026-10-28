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

/* ---------------- 半波/直流分量自适应（移植自 EP5K Funtion.c 的 InvOcpHandle） ----------------
 *
 * 三相三电平带半波负载时，某一相电流会长期偏向正/负半周：交流分量正常，但存在
 * 直流偏置。此时逐波限流（CBC）仍然每周期动作，但事件可能很稀疏，"400 次" 要
 * 几分钟才数得满，保护形同虚设。EP5K 的思路是：一旦检测到某相直流分量超限，
 * 就把过流判据从 400 次 / 5ms 恢复 收紧到 150 次 / 15ms 恢复，让保护对"稀疏但
 * 持续"的过流重新变得敏感。
 *
 * 判据链路：CLA 每 20kHz 更新 Cla1ToCpuMsg.fInvCur[]（ADC 码值）
 *          → AdcCalib 换算成安培（扣零漂、乘系数，20kHz 一次）
 *          → 本任务 10kHz 做 EMA 得慢变直流分量
 *          → 连续 400 拍（40ms）超限则进入半波模式。
 */

/* 半波门限：单相 |DC| ≥ 5A 认为出现半波/偏磁（与 EP5K 同值）
 * 电流值由 AdcCalib 以安培给出（已扣零漂、已乘系数），这里不再用码值，
 * 避免零漂和标定系数在两处各维护一份 */
#define PROTECT_HALF_WAVE_DC_A          (5.0f)
/* 退出滞回：降到门限的 80% 才认为半波消失，避免在门限附近来回抖 */
#define PROTECT_HALF_WAVE_EXIT_A        (4.0f)
/* 直流分量 EMA 系数：α=0.001 @10kHz → τ≈100ms，50Hz 基波残留 <3.2% */
#define PROTECT_INV_DC_ALPHA            (0.001f)
/* 连续多少拍超限才承认半波（10kHz 下 400 拍 = 40ms，与 EP5K 同值） */
#define PROTECT_HALF_WAVE_JUDGE_TICK    (400U)
/* 半波模式下的过流判据（EP5K：150 次 / 15ms 恢复） */
#define PROTECT_INV_OC_TRIP_CNT_HW      (150U)
#define PROTECT_INV_OC_CLEAN_TICK_HW    (150U)

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
extern volatile Uint16 gProtectHalfWaveMode;    /* 1：半波自适应模式生效 */
extern volatile Uint16 gProtectHalfWaveJudge;   /* 连续超限拍数（10kHz） */
extern volatile int16  gProtectInvDc[3];        /* 三相直流分量 R/S/T，0.1A 有符号 */

#ifdef __cplusplus
}
#endif

#endif /* PROTECT_TASK_H */

/**
 * @file PllData.h
 * @brief 角度与频率的唯一来源：PLL 输出。
 * @version 0.1
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 *
 * 字段名 ThetaR / ThetaS / ThetaT 由项目所有者指定，与项目内
 * f 前缀的命名规则不同，按指定保留，不要“统一命名”。
 *
 * 所有权约定：
 *   1. 本文件是 stPLLData 的长期居所。PLL 模块落地后继续补在这里（往结构里
 *      加字段可以，改名或搬迁不行），不另建头文件。
 *   2. 唯一写入方是 PLL 模块。离网时角度来自自身频率，并网时来自电网锁相，
 *      工况判断在 PLL 内部完成 —— 其它模块只读，不需要区分工况。
 *   3. 采样层（Platform/Sample）只读 ThetaR 做交流窗口收口；控制环可用
 *      ThetaR / ThetaS / ThetaT 做坐标变换。
 *   4. 采样层的 SampleDataInit() 会把本结构清零，这是采样层唯一的写动作。
 */

#ifndef PLL_DATA_H
#define PLL_DATA_H

#include "F2837xS_device.h"

/*! 全局角度。单位 rad，值域 0 ~ 2π，由 PLL 每拍递增。 */
typedef struct
{
    float ThetaR;   /* A / R 相角度 */
    float ThetaS;   /* B / S 相角度 */
    float ThetaT;   /* C / T 相角度 */
} ST_PLL_DATA;

extern volatile ST_PLL_DATA stPLLData;

#endif /* PLL_DATA_H */

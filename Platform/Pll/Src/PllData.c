/**
 * @file PllData.c
 * @brief 全局角度实例定义。
 * @version 0.1
 * @date 2026-11-04
 *
 * @copyright Copyright (c) 2026
 *
 * 本文件只提供存储。写入方是 PLL 模块（尚未落地），当前全零，
 * 采样层的交流窗口因此走“强制收口”分支（见 AdcSample.c 的 AdcSampleAcStep）。
 */

#include "PllData.h"

volatile ST_PLL_DATA stPLLData;

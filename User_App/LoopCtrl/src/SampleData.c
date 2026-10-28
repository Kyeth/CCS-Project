/**
 * @file SampleData.c
 * @author yetingheng (004182)
 * @brief 采样数据分层结构体的实例与初始化
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 全部实例集中在本文件定义，头文件只做 extern 声明，避免 #10462-D
 * 落在小端 RAM 存放的 .cla 侧。
 */
#include "SampleData.h"

#include <string.h>

volatile ST_SAMPLE_RAW    stSampleRaw;
volatile ST_SAMPLE_INS    stSampleIns;
ST_SAMPLE_AVG             stSampleAvg;
ST_SAMPLE_RMS             stSampleRms;
ST_SAMPLE_STATUS          stSampleStatus;

/**
 * @brief 清零全部采样层实例
 */
void SampleDataInit(void)
{
    (void)memset((void *)&stSampleRaw, 0, sizeof(stSampleRaw));
    (void)memset((void *)&stSampleIns, 0, sizeof(stSampleIns));
    (void)memset(&stSampleAvg, 0, sizeof(stSampleAvg));
    (void)memset(&stSampleRms, 0, sizeof(stSampleRms));
    (void)memset(&stSampleStatus, 0, sizeof(stSampleStatus));
}

#ifndef __FILTER_H
#define __FILTER_H

#include "F28003x_device.h"


#ifdef __cplusplus
    extern "C" {
#endif

#define MAX_MEDIAN_SIZE 5
#define MAX_WINDOW_SIZE 4

#define SWAP_IF(x, y, t) do{if((x)>(y)){t=(x);(x)=(y);(y)=t;}}while(0)

// 中值滤波
typedef struct
{
    Uint16 u16Size;                     // 实际使用点数
    Uint16 u16Index;                    // 当前写入位置
    float fBuffer[MAX_MEDIAN_SIZE];     // 环形缓存区

}ST_MEDIAN_FILTER;

// 一阶滞后混合滤波
typedef struct
{
    float fPrevOutput;                  // 上一次输出值
    float fAlpha;                       // 一阶滞后滤波系数

}ST_FIRST_LAG_FILTER;

// 均值滑动滤波
typedef struct
{
    float fOutSum;
    float fBeta;                       // 滤波系数

}ST_AVG_FILTER;

typedef struct
{
    Uint16 u16Index;
    Uint16 u16WindowSize;
    float fSum;
    Uint16 u16IsFilled;
    float fBuffer[MAX_WINDOW_SIZE];     // 环形缓存区
}ST_SLIDING_WINDOW_FILTER;

typedef struct
{
    Uint16 u16IsInit;                   // 初始化标致
    ST_AVG_FILTER stAvgFilter;
    ST_MEDIAN_FILTER stMedianFilter;
    ST_FIRST_LAG_FILTER stFirstLagFilter;
    ST_SLIDING_WINDOW_FILTER stSlidingWindowFilter;
}ST_MIX_FILTER;

extern ST_MIX_FILTER stAdcFilter1;
extern ST_MIX_FILTER stAdcFilter2;
extern ST_MIX_FILTER stAdcFilter3;
extern ST_MIX_FILTER stAdcFilter4;
extern ST_MIX_FILTER stAdcFilter5;
extern ST_MIX_FILTER stAdcFilter6;
extern ST_MIX_FILTER stAdcFilter7;
extern ST_MIX_FILTER stAdcFilter8;
extern ST_MIX_FILTER stAdcFilter9;
extern ST_MIX_FILTER stAdcFilter10;
extern ST_MIX_FILTER stAdcFilter11;
extern ST_MIX_FILTER stAdcFilter12;
extern ST_MIX_FILTER stAdcFilter13;

extern void FilterInit(ST_MIX_FILTER *pstFilter, Uint16 u16Size, float fAlpha, float fBeta);
extern void FilterSetSize(ST_MIX_FILTER *pstFilter, Uint16 u16NewSize);
extern void FilterSetAlpha(ST_MIX_FILTER *pstFilter, float fNewAlpha);
extern void FilterReset(ST_MIX_FILTER *pstFilter);

#pragma CODE_SECTION(GetMedian_5, ".TI.ramfunc");
// 使用5点排序网络
static inline float GetMedian_5(ST_MIX_FILTER *pstFilter)
{
    float a0 = pstFilter->stMedianFilter.fBuffer[0];
    float a1 = pstFilter->stMedianFilter.fBuffer[1];
    float a2 = pstFilter->stMedianFilter.fBuffer[2];
    float a3 = pstFilter->stMedianFilter.fBuffer[3];
    float a4 = pstFilter->stMedianFilter.fBuffer[4];

    float t;

    SWAP_IF(a0, a1, t);
    SWAP_IF(a3, a4, t);
    SWAP_IF(a0, a3, t);
    SWAP_IF(a1, a2, t);
    SWAP_IF(a2, a4, t);
    SWAP_IF(a1, a3, t);
    SWAP_IF(a0, a1, t);
    SWAP_IF(a2, a3, t);
    SWAP_IF(a1, a2, t);

    return a2;
}

static inline float GetSlingWindow(ST_MIX_FILTER *pstFilter, float fInput)
{
    return 0.0f;
}

#pragma CODE_SECTION(FilterProcsee, ".TI.ramfunc");
static inline float FilterProcsee(ST_MIX_FILTER *pstFilter, float fInput)
{
    float fOutput;

    // 1阶滞后
    fOutput = pstFilter->stFirstLagFilter.fAlpha * fInput + (1.0f - pstFilter->stFirstLagFilter.fAlpha ) * pstFilter->stFirstLagFilter.fPrevOutput;
    pstFilter->stFirstLagFilter.fPrevOutput = fOutput;

    return fOutput;
}

static inline float FilterAvgProcsee(ST_MIX_FILTER *pstFilter, float fInput)
{
    float fOutput = 0;

    pstFilter->stAvgFilter.fOutSum = pstFilter->stAvgFilter.fOutSum + fInput - pstFilter->stAvgFilter.fOutSum * pstFilter->stAvgFilter.fBeta;
    fOutput = pstFilter->stAvgFilter.fOutSum * pstFilter->stAvgFilter.fBeta;

    return fOutput;
}

#ifdef __cplusplus
    }
#endif

#endif

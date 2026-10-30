#include "Filter.h"
#include "string.h"

__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter1;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter2;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter3;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter4;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter5;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter6;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter7;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter8;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter9;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter10;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter11;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter12;
__attribute__((aligned(4)))  ST_MIX_FILTER stAdcFilter13;

void FilterInit(ST_MIX_FILTER *pstFilter, Uint16 u16Size, float fAlpha, float fBeta)
{
    if (u16Size < 3)
    {
        u16Size = 3;
    }
    if (u16Size > MAX_MEDIAN_SIZE)
    {
        u16Size = MAX_MEDIAN_SIZE;
    }
    if (fAlpha < 0.0f)
    {
        fAlpha = 0.0f;
    }
    if (fAlpha > 1.0f)
    {
        fAlpha = 1.0f;
    }

    pstFilter->stMedianFilter.u16Size = u16Size;
    pstFilter->stMedianFilter.u16Index = 0;
    memset(&pstFilter->stMedianFilter.fBuffer[0], 0, sizeof(float) * u16Size);

    pstFilter->stFirstLagFilter.fAlpha = fAlpha;
    pstFilter->stFirstLagFilter.fPrevOutput = 0;

    pstFilter->stAvgFilter.fBeta = fBeta;

//    pstFilter->stSlidingWindowFilter.u16Index = 0;
//    pstFilter->stSlidingWindowFilter.u16IsFilled = 0;
//    pstFilter->stSlidingWindowFilter.u16WindowSize = MAX_WINDOW_SIZE;
    pstFilter->stSlidingWindowFilter.fSum = 0;

    memset(&pstFilter->stSlidingWindowFilter.fBuffer[0], 0, sizeof(float) * MAX_WINDOW_SIZE);

    pstFilter->u16IsInit = 1;
}


//#pragma CODE_SECTION(GetMedian_5, ".TI.ramfunc");
//// 使用5点排序网络
//static inline float GetMedian_5(ST_MIX_MEDIAN_FILTER *pstFilter)
//{
//    float a0 = pstFilter->fBuffer[0];
//    float a1 = pstFilter->fBuffer[1];
//    float a2 = pstFilter->fBuffer[2];
//    float a3 = pstFilter->fBuffer[3];
//    float a4 = pstFilter->fBuffer[4];
//
//    SWAP_IF(a0, a1);
//    SWAP_IF(a3, a4);
//    SWAP_IF(a0, a3);
//    SWAP_IF(a1, a2);
//    SWAP_IF(a2, a4);
//    SWAP_IF(a1, a3);
//    SWAP_IF(a0, a1);
//    SWAP_IF(a2, a3);
//    SWAP_IF(a1, a2);
//
//    return a2;
//}
//
//#pragma CODE_SECTION(FilterProcsee, ".TI.ramfunc");
//float FilterProcsee(ST_MIX_MEDIAN_FILTER *pstFilter, float fInput)
//{
//    float fMedian, fOutput;
//
//    pstFilter->fBuffer[pstFilter->u16Index] = fInput;
//    pstFilter->u16Index = (pstFilter->u16Index + 1) % pstFilter->u16Size;
//
//    fMedian = GetMedian_5(pstFilter);
//
//    fOutput = pstFilter->fAlpha * fMedian + (1.0f - pstFilter->fAlpha ) * pstFilter->fPrevOutput;
//
//    pstFilter->fPrevOutput = fOutput;
//
//    return fOutput;
//
//}


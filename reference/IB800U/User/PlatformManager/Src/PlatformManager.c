#include "PlatformManager.h"
#include "Hw_GpioDriver.h"
#include <stdint.h>
#include <math.h>
#include "LLCControl.h"
#include "Fan.h"
#include "FaultProtect.h"
#include "Schedule.h"

SYS_PLATFORM_CTRL ObjPlatformCtrl;
ST_CommData CommDataTrans;
ST_SAMPLE ObjSample;
ST_BUS_FILTER ObjBusFilter;
ST_BUS_FILTER ObjIoscFilter;
ST_BUS_FILTER ObjIbusFilter;

#define Pi 3.1415926535f
#define TEMP_DIFF_THRESHOLD   20.0f    // 温度偏差阈值（℃）
#define AD_BROKEN_LOW         100      // AD 接近 0 的低阈值
#define AD_BROKEN_HIGH        4000     // AD 接近满量程的高阈值
// 贴片NTC温度表 (-25°C ~ 125°C)
const uint16_t SMT_NTC_Table[] =
{
 4007,4002,3998,3993,3987,3982,3977,3971,3965,3959,3952,3946,3939,3931,3924,3916,3908,3900,3891,3882,
 3873,3863,3853,3843,3832,3821,3810,3798,3786,3773,3760,3747,3733,3719,3705,3690,3674,3658,3642,3625,
 3608,3591,3573,3555,3536,3517,3497,3476,3456,3435,3413,3391,3369,3346,3323,3299,3275,3250,3226,3200,
 3175,3149,3122,3096,3069,3041,3014,2986,2958,2929,2900,2871,2842,2813,2783,2753,2724,2695,2665,2636,
 2606,2576,2546,2516,2486,2456,2426,2396,2366,2335,2305,2275,2245,2215,2184,2154,2124,2095,2065,2036,
 2006,1977,1948,1919,1890,1862,1834,1805,1777,1750,1722,1694,1667,1639,1612,1585,1558,1532,1506,1480,
 1455,1430,1405,1381,1357,1333,1309,1287,1264,1242,1220,1198,1177,1156,1136,1116,1096,1076,1057,1038,
 1020,1002,984,966,949,932,915,899,883,867,852
};
// 插件NTC温度表 (-25°C ~ 125°C)
const uint16_t DIP_NTC_Table[] =
{
    4049,4047,4044,4041,4038,4034,4031,4027,4023,4019,4015,4011,4006,4002,3997,3992,3986,3981,3975,3969,
    3962,3956,3949,3942,3935,3927,3919,3911,3902,3893,3884,3875,3865,3855,3844,3833,3821,3810,3797,3785,
    3771,3758,3744,3729,3714,3699,3683,3667,3650,3632,3614,3596,3576,3557,3537,3516,3495,3473,3450,3427,
    3404,3380,3355,3330,3304,3278,3251,3223,3195,3167,3138,3109,3079,3048,3018,2986,2955,2923,2891,2858,
    2826,2792,2759,2725,2691,2657,2622,2587,2553,2517,2482,2447,2411,2376,2340,2305,2269,2234,2198,2163,
    2127,2092,2057,2022,1987,1952,1918,1883,1849,1816,1782,1749,1716,1684,1652,1620,1588,1557,1526,1496,
    1466,1436,1407,1379,1350,1322,1295,1268,1241,1215,1190,1164,1140,1115,1091,1068,1045,1022,1000,979,
    957,936,916,896,877,857,839,820,802,785,768
};
// 表格大小定义
#define SMT_TABLE_SIZE (sizeof(SMT_NTC_Table) / sizeof(SMT_NTC_Table[0]))
#define DIP_TABLE_SIZE (sizeof(DIP_NTC_Table) / sizeof(DIP_NTC_Table[0]))
// 温度范围定义
#define MIN_TEMP (-25.0f)
#define MAX_TEMP (125.0f)

/**
 * @brief 将AD采样值转换为温度值（分段线性插值）
 * @param ad_sample AD采样值
 * @return 对应的温度值（摄氏度）
 */
float AD_to_Temperature(uint16_t ad_sample, const uint16_t* value_table, int16_t table_size)
{


    int16 i = 0;
    uint32_t diff = 0;
    int16_t min_diff_index = 0;
    uint32_t min_diff = 0xFFFFFFFF; // 初始化为最大值
    // 参数有效性检查
    if (value_table == NULL || table_size <= 0)
    {
        return 25.0f;  // 返回默认温度值
    }

    // 边界检查 - 超过最大值
    if (ad_sample >= value_table[0])
    {
        return MIN_TEMP;
    }
    // 边界检查 - 低于最小值
    if (ad_sample <= value_table[table_size-1])
    {
        return MAX_TEMP;
    }
    // 查找最接近的AD值
    for (i = 0; i < table_size; i++)
    {
        diff = abs((int32_t)ad_sample - (int32_t)value_table[i]);

        if (diff < min_diff)
        {
            min_diff = diff;
            min_diff_index = i;
        }
    }

    // 返回对应的温度（索引i对应温度 = MIN_TEMP + i）
    return MIN_TEMP + (float)min_diff_index;
}

/**
 * @brief 单点温度断线检测
 * @param temp 四个温度值数组（℃）
 * @param ad   四个原始 AD 值数组
 * @return 故障通道编号 0~3，4 表示无故障
 */
uint16_t Detect_NTC_Broken(float temp[4], uint16_t ad[4])
{
    float fSum = 0.0f;
    float fMax_Temp = temp[0];       // 记录最高温度
    float fMin_Temp = temp[0];       // 记录最低温度
    uint16_t u16Max_idx = 0;         // 最高温度对应的通道号
    uint16_t u16Min_idx = 0;         // 最低温度对应的通道号
    uint16_t i;                  // 循环变量
    uint16_t u16Broken_Ch = 4;

    for (i = 0; i < 4; i++)
    {
        fSum += temp[i];
        if (temp[i] > fMax_Temp)
        {
            fMax_Temp = temp[i];
            u16Max_idx = i;
        }
        if (temp[i] < fMin_Temp)
        {
            fMin_Temp = temp[i];
            u16Min_idx = i;
        }
    }

    float fRef_Temp = (fSum - fMax_Temp - fMin_Temp) / 2.0f;

    if ((fMax_Temp - fRef_Temp) > TEMP_DIFF_THRESHOLD)
    {
        if (ad[u16Max_idx] >= AD_BROKEN_HIGH || ad[u16Max_idx] <= AD_BROKEN_LOW)
        {
            u16Broken_Ch = u16Max_idx;   // 确认故障通道
        }
    }

    if (u16Broken_Ch == 4)
    {
        if ((fRef_Temp - fMin_Temp) > TEMP_DIFF_THRESHOLD)
        {
            if (ad[u16Min_idx] >= AD_BROKEN_HIGH || ad[u16Min_idx] <= AD_BROKEN_LOW)
            {
                u16Broken_Ch = u16Min_idx;
            }
        }
    }

    return u16Broken_Ch;
}

void TaskTemp(void)
{
    uint16_t TempError = 4;
    uint16_t ad[4];
    ad[0] = ObjPlatformCtrl.u16AdcARawResult[4]; // VNTC_2 (DIP)
    ad[1] = ObjPlatformCtrl.u16AdcARawResult[6]; // VEnvironment (SMT)
    ad[2] = ObjPlatformCtrl.u16AdcCRawResult[5]; // VTransformer_2 (SMT)
    ad[3] = ObjPlatformCtrl.u16AdcBRawResult[5]; // VTransformer_1 (SMT)
    // 转换成实际温度
    ObjSample.VNTC_2 = AD_to_Temperature(ObjPlatformCtrl.u16AdcARawResult[4], DIP_NTC_Table, (int16_t)DIP_TABLE_SIZE);
    ObjSample.VEnvironment = AD_to_Temperature(ObjPlatformCtrl.u16AdcARawResult[6], SMT_NTC_Table, (int16_t)SMT_TABLE_SIZE);
    ObjSample.VTransformer_2 = AD_to_Temperature(ObjPlatformCtrl.u16AdcCRawResult[5], SMT_NTC_Table, (int16_t)SMT_TABLE_SIZE);
    ObjSample.VTransformer_1 = AD_to_Temperature(ObjPlatformCtrl.u16AdcBRawResult[5], SMT_NTC_Table, (int16_t)SMT_TABLE_SIZE);
    // 准备温度数组
    float temp[4] =
    {
        ObjSample.VNTC_2,
        ObjSample.VEnvironment,
        ObjSample.VTransformer_2,
        ObjSample.VTransformer_1
    };
    TempError = Detect_NTC_Broken(temp,ad);
    if (stSysProtect.unFaultMaskWord2.Bits.bit13 == 0)      // 先判断是否屏蔽该故障
    {
        if(TempError < 4)
        {
            stSysProtect.u16TotalFaultSign = 1;
            stSysProtect.unFaultStatusWord2.Bits.bit13 = 1;
        }
    }
    SetTaskRunStatus(TASK_TEMP_BIT);
}

// 初始化陷波器
void InitTrapFilter(ST_BUS_FILTER* filter, Uint16 freq)
{
    filter->SampleFreq = 20000.0f;
    filter->Q = 5.0f;

    // 初始化为默认系数（中心频率为100hz，实际会立即更新）
    if(freq == 60)
    {
        filter->b0 =  0.00375487f;
        filter->b1 =  0.0f;
        filter->b2 = -0.00375487f;
        filter->a1 = -1.991075f;
        filter->a2 =  0.992490f;
    }
    else
    {
        // 二阶带通
        filter->b0 =  0.003132f;
        filter->b1 =  0.0f;
        filter->b2 = -0.003132f;
        filter->a1 = -1.992774f;   // 注意与之前的区别
        filter->a2 =  0.993775f;
    }
    // 重置状态
    filter->x1 = 0.0f;
    filter->x2 = 0.0f;
    filter->y1 = 0.0f;
    filter->y2 = 0.0f;

    filter->CenterFreq = 0.0f;
}

void UpdateFrequency(ST_BUS_FILTER* filter, Uint16 PcsFreq)
{
    float centerFreq;
    float sinW0;
    float cosW0;
    float alpha;
    float a0;

    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
    // 仅接受通信协议定义的电网频率
    if (PcsFreq == 50)
    {
        centerFreq = 100.0f;
    }
    else if (PcsFreq == 60)
    {
        centerFreq = 120.0f;
    }
    else
    {
        // 通信频率无效时保持当前滤波器系数
        return;
    }
    // 当前已是对应中心频率时，不重复更新
    if (centerFreq == filter->CenterFreq)
    {
        return;
    }
    filter->w0    = 2.0f * Pi * centerFreq / filter->SampleFreq;
    sinW0 = sinf(filter->w0);
    cosW0 = cosf(filter->w0);
    alpha = sinW0 / (2.0f * filter->Q);
    a0    = 1.0f + alpha;

    b0 =  alpha / a0;
    b1 =  0.0f;
    b2 = -alpha / a0;
    a1 = -2.0f * cosW0 / a0;
    a2 =  (1.0f - alpha) / a0;

    filter->b0 = b0;
    filter->b1 = b1;
    filter->b2 = b2;
    filter->a1 = a1;
    filter->a2 = a2;

    filter->CenterFreq = centerFreq;
}

float ApplyTrapFilter(ST_BUS_FILTER* filter, float input)
{
    // 计算输出
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2 - filter->a1 * filter->y1 - filter->a2 * filter->y2;

    // 更新状态变量
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;

    return output;
}


float GetFilterTrap(ST_BUS_FILTER* filter, float value, float PcsFreq)
{
    // 应用滤波
    return ApplyTrapFilter(filter, value);
}

void AdcValueCopyPlatform(SYS_PLATFORM_CTRL *pObjPlatformCtrl, Uint16 u16AdcId)
{
    Uint16 i = 0;
    Uint16 *pu16AdcRawResultPoint = NULL;
    Uint16 *pu16AdcRawResultArry = NULL;

    switch (u16AdcId)
    {
    case ADC_ID0:
        pu16AdcRawResultArry = &pObjPlatformCtrl->u16AdcARawResult[0];
        pu16AdcRawResultPoint = pObjPlatformCtrl->pAdcARawResult;
        break;

    case ADC_ID1:
        pu16AdcRawResultArry = &pObjPlatformCtrl->u16AdcBRawResult[0];
        pu16AdcRawResultPoint = pObjPlatformCtrl->pAdcBRawResult;
        break;

    case ADC_ID2:
        pu16AdcRawResultArry = &pObjPlatformCtrl->u16AdcCRawResult[0];
        pu16AdcRawResultPoint = pObjPlatformCtrl->pAdcCRawResult;
        break;

    default:
        pu16AdcRawResultArry = &pObjPlatformCtrl->u16AdcARawResult[0];
        pu16AdcRawResultPoint = pObjPlatformCtrl->pAdcARawResult;
        break;
    }

    for (i = 0; i < 16; i++)
    {
        pu16AdcRawResultArry[i] = pu16AdcRawResultPoint[i];
    }
}


void PlatformSimplingCoefficientInit( ST_SAMPLE *pObjSample)
{
    pObjSample->VBusSampCoefficient = 0.13428f;           // 母线电压采样系数
    pObjSample->VAirFrontSampCoefficient = 0.13428f;      // 空开前端电压采样系数
    pObjSample->VAirBackSampCoefficient = 0.13428f;       // 空开后端电压采样系数
    pObjSample->VBatSampCoefficient = 0.008278f;           // 电池电压采样系数
    pObjSample->IBusSampCoefficient = 0.01171875f;           //0.0244140625f;// 母线输出谐振电流采样系数0.01171875// 0.0244140625
    pObjSample->IBranch_1SampCoefficient = 0.0040283203f;      // 支路1谐振电流采样系数 0.00537109375f;//
    pObjSample->IBranch_2SampCoefficient = 0.0040283203f;      // 支路2谐振电流采样系数 0.00537109375f;//
    pObjSample->HeatingSampCoefficient = 0.00080186f;            // 加热膜电流采样系数
    pObjSample->DoubleSet = 1;
}

#pragma CODE_SECTION(ZeroDirftCheck, ".TI.ramfunc");
void ZeroDirftCheck(SYS_PLATFORM_CTRL *pObjPlatformCtrl)
{
    if (pObjPlatformCtrl->u16ZeroDirftIsInit == 0)
    {
        if(pObjPlatformCtrl->u16IBusZeroDirftCnt >= 30000)
        {
            pObjPlatformCtrl->u32IBusZeroDirftSum += pObjPlatformCtrl->u16AdcBRawResult[0];
            if (pObjPlatformCtrl->u16IBusZeroDirftCnt >= 31024)
            {
                pObjPlatformCtrl->u16IBusZeroDirft = pObjPlatformCtrl->u32IBusZeroDirftSum >> 10;
                if(((pObjPlatformCtrl->u16IBusZeroDirft > 1648)&&(pObjPlatformCtrl->u16IBusZeroDirft < 2448))||(stSysProtect.unFaultMaskWord2.Bits.bit10 == 1))
                {
                    pObjPlatformCtrl->u16ZeroDirftIsInit = 1;
                }
                else
                {
                    pObjPlatformCtrl->u16ZeroDirftIsInit = 0;
                    stSysProtect.unFaultStatusWord2.Bits.bit10 = 1;
                    stSysProtect.u16TotalFaultSign = 1;
                    stSysProtect.u16SoftFPTakesEffect = 1;
                }
                pObjPlatformCtrl->u16IBusZeroDirftCnt = 0;
                pObjPlatformCtrl->u32IBusZeroDirftSum = 0;
            }
        }
        ++pObjPlatformCtrl->u16IBusZeroDirftCnt;
    }
    else
    {
        if((pObjPlatformCtrl->u16AdcBRawResult[0] - pObjPlatformCtrl->u16IBusZeroDirft) > 0)
        {
            ObjSample.IBusSampCoefficient = 24.0f / (4096.0f - (float)pObjPlatformCtrl->u16IBusZeroDirft);
        }
        else
        {
            ObjSample.IBusSampCoefficient = 24.0f / (float)pObjPlatformCtrl->u16IBusZeroDirft;
        }
    }
}

void TaskFilterComm(void)
{
    UpdateFrequency(&ObjBusFilter, ObjPlatformCtrl.u16WorkFrequency);
}

void TaskPlatform(void)
{

    ObjPlatformCtrl.u16AllowChargePowerMax = (Uint16)((ObjSample.VBat * LLCSystemCtrl.u16ChargeCurrentAllowMax * 0.1f) / 0.99f);
    UPDNLMT(ObjPlatformCtrl.u16AllowChargePowerMax, 4200.0f, 0.0f);
    ObjPlatformCtrl.u16AllowDischargePowerMax = (Uint16)(ObjSample.VBat * LLCSystemCtrl.u16DischargeCurrentAllowMax * 0.1f * 0.96f);
    UPDNLMT(ObjPlatformCtrl.u16AllowDischargePowerMax, 6600.0f, 0.0f);

    ObjSystemCtrl.u16ChargeCurrentLimit = 4000 - stFanHandle.u16ChaThresholdsCoefficient * 480;
    ObjSystemCtrl.u16DischargeCurrentLimit = 6000 - stFanHandle.u16DisThresholdsCoefficient * 480;
    SetTaskRunStatus(TASK_PLATFORM_BIT);
}

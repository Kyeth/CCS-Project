/**
 * @file Hw_AdcDriver.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
/**
 * @file Hw_AdcDriver.c
 * @author yetingheng  (004182)
 * @brief
 * @version 0.2
 * @date 2026-03-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "Hw_AdcDriver.h"
#include "Device.h"
#include "adc.h"
#include "Filter.h"

#ifdef __cplusplus
    extern "C" {
#endif

static volatile struct ADC_REGS *g_pstAdcRegs[] = {&AdcaRegs, &AdcbRegs, &AdccRegs, };
static volatile struct ADC_RESULT_REGS *g_pstAdcResultRegs[] = {&AdcaResultRegs, &AdcbResultRegs, &AdccResultRegs,};
volatile Uint16 g_u16AdcResult[ADC_ID_MAX][16] = {0};



/**
 * @brief 用于初始化单个ADC外设
 * 
 * @param pstAdcCfgItem 入参是一个ST_ADC_CFG_ITEM结构体对象的地址
 */
void Hw_SingleAdcInit(ST_ADC_CFG_ITEM* pstAdcCfgItem)
{
    Uint16 u16AdcId = 0;
    Uint16 i = 0;
//    Uint32 *p32SocAddr = NULL;
//    Uint16 adcOffsetTrimOTPIndex; //index into OTP table of ADC offset trims
//    Uint16 adcOffsetTrim; //temporary ADC offset trim

    u16AdcId = pstAdcCfgItem->u16AdcId;

    EALLOW;

    g_pstAdcRegs[u16AdcId]->ADCCTL2.bit.PRESCALE = 6;   // ADCCLK = InputClk(SYSCLK) / 4 = 30M

    EDIS;
    // 对ADC做一次线性校准
    ADC_setOffsetTrim((Uint32)g_pstAdcRegs[u16AdcId]);

    EALLOW;
    g_pstAdcRegs[u16AdcId]->ADCCTL1.bit.INTPULSEPOS = 1;        // 中断触发时刻
    g_pstAdcRegs[u16AdcId]->ADCCTL1.bit.ADCPWDNZ = 1;
    EDIS;
    DEVICE_DELAY_US(1000);
//    p32SocAddr = &g_pstAdcRegs[u16AdcId]->ADCSOC0CTL;
    // 16个SOC
    for (i = 0; i < 16; i++)
    {
//        ((union ADCSOC0CTL_REG*)p32SocAddr)->bit.CHSEL =  pstAdcCfgItem->pstAdcChCfgItem[i * 4].bChSel;
//        ((union ADCSOC0CTL_REG*)p32SocAddr)->bit.ACQPS = pstAdcCfgItem->pstAdcChCfgItem[i * 4].bAcqps;
//        ((union ADCSOC0CTL_REG*)p32SocAddr)->bit.TRIGSEL = pstAdcCfgItem->pstAdcChCfgItem[i * 4].bTrigSel;
//        p32SocAddr += (sizeof(union ADCSOC0CTL_REG)/2);

        ADC_setupSOC((Uint32)g_pstAdcRegs[u16AdcId], (ADC_SOCNumber)i, (ADC_Trigger)pstAdcCfgItem->pstAdcChCfgItem[i * ADC_ID_MAX].bTrigSel,
                     (ADC_Channel)pstAdcCfgItem->pstAdcChCfgItem[i * ADC_ID_MAX].bChSel,
                     pstAdcCfgItem->pstAdcChCfgItem[i * ADC_ID_MAX].bAcqps);
    }
    EALLOW;
    g_pstAdcRegs[u16AdcId]->ADCINTSEL1N2.bit.INT1SEL = pstAdcCfgItem->bIntSel;
    g_pstAdcRegs[u16AdcId]->ADCINTSEL1N2.bit.INT1E = pstAdcCfgItem->bAdcIntEnable;
    g_pstAdcRegs[u16AdcId]->ADCINTFLGCLR.bit.ADCINT1 = 1;

    EDIS;
}



/**
 * @brief 用于初始化一组ADC，通常用于参数初始化阶段
 * 
 * @param pstAdcCfg :入参是一组ST_ADC_CFG结构体对象的集合
 */
void Hw_AdcInit(ST_ADC_CFG* pstAdcCfg)
{
    Uint16 i = 0;

    // ADC配置成外部参考电压
    AnalogSubsysRegs.ANAREFCTL.bit.ANAREFSEL = 1;
    // ADC配置成3.3V
    AnalogSubsysRegs.ANAREFCTL.bit.ANAREF2P5SEL = 0;

    for (i = 0; i < pstAdcCfg->u16ItemNum; i++)
    {
        Hw_SingleAdcInit(&pstAdcCfg->pstAdcCfgItem[i]);
    }

    // 配置滤波器
    FilterInit(&stAdcFilter1, 5, 0.8, 0.001);
    FilterInit(&stAdcFilter2, 5, 0.8, 0.001);
    FilterInit(&stAdcFilter3, 5, 0.8, 0.001);
    FilterInit(&stAdcFilter4, 5, 0.1, 0.001);
    FilterInit(&stAdcFilter5, 5, 0.1, 0.01);
    FilterInit(&stAdcFilter6, 5, 0.8, 0.01);
    FilterInit(&stAdcFilter7, 5, 0.8, 0.01);
    FilterInit(&stAdcFilter8, 5, 0.8, 0.001);
    FilterInit(&stAdcFilter9, 5, 0.1, 0.05);
    FilterInit(&stAdcFilter10, 5, 0.8, 0.001);
    FilterInit(&stAdcFilter11, 5, 0.8, 0.01);
    FilterInit(&stAdcFilter12, 5, 0.1, 0.0001);
    FilterInit(&stAdcFilter13, 5, 0.7, 0.01);
}



/**
 * @brief 从ADC外设采样结果寄存器中读取采样值
 * 
 * @param u16AdcId  : ADC的ID号
 * @param pu16AdcResult : volatile Uint16* 指针类型的地址
 * @return int16    :-1 参数非法
 *                  : 0 读取成功
 */
#pragma CODE_SECTION(Hw_AdcRead, ".TI.ramfunc");
int16 Hw_AdcRead(Uint16 u16AdcId, Uint16 **pu16AdcResult)
{
//    这里不做判断，目的为了提升效率
//    if ((u16AdcId >= ADC_ID_MAX) || (pu16AdcResult == NULL))
//    {
//        return -1;  // 参数非法
//    }

    *pu16AdcResult = (Uint16 *)&g_u16AdcResult[u16AdcId][0];

//    g_u16AdcResult[u16AdcId][0] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT0;
//    g_u16AdcResult[u16AdcId][1] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT1;
//    g_u16AdcResult[u16AdcId][2] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT2;
//    g_u16AdcResult[u16AdcId][3] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT3;
//    g_u16AdcResult[u16AdcId][4] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT4;
//    g_u16AdcResult[u16AdcId][5] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT5;
//    g_u16AdcResult[u16AdcId][6] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT6;
//    g_u16AdcResult[u16AdcId][7] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT7;
//    g_u16AdcResult[u16AdcId][8] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT8;
//    g_u16AdcResult[u16AdcId][9] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT9;
//    g_u16AdcResult[u16AdcId][10] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT10;
//    g_u16AdcResult[u16AdcId][11] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT11;
//    g_u16AdcResult[u16AdcId][12] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT12;
//    g_u16AdcResult[u16AdcId][13] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT13;
//    g_u16AdcResult[u16AdcId][14] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT14;
//    g_u16AdcResult[u16AdcId][15] = g_pstAdcResultRegs[u16AdcId]->ADCRESULT15;

    // 使用memcpy，效率更高
    memcpy((Uint16 *)&g_u16AdcResult[u16AdcId][0], (const Uint16 *)&g_pstAdcResultRegs[u16AdcId]->ADCRESULT0, 16);

    return 0;
}


#pragma CODE_SECTION(Hw_AdcClearInitFlag, ".TI.ramfunc");
void Hw_AdcClearInitFlag(Uint16 u16AdcId)
{
    ADC_clearInterruptStatus((uint32_t)g_pstAdcRegs[u16AdcId], ADC_INT_NUMBER1);
    g_pstAdcRegs[u16AdcId]->ADCINTOVFCLR.bit.ADCINT1 = 1;
}

#ifdef __cplusplus
    }
#endif

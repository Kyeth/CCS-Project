/**
 * @file Hw_PwmDriver.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_PwmDriver.h"
#include "epwm.h"
#include "f28003x_epwm_defines.h"


volatile struct EPWM_REGS *pstPwmRegs[] = {&EPwm1Regs, &EPwm2Regs, &EPwm3Regs,
                                                  &EPwm4Regs, &EPwm5Regs, &EPwm6Regs,
                                                  &EPwm7Regs, &EPwm8Regs,  };
/**
 * @brief 用于初始化单个ePWM
 * 
 * @param pstPwmCfgItem :入参是一个ST_PWM_CFG_ITEM结构体对象指针
 */
void Hw_SinglePwmInit(ST_PWM_CFG_ITEM *pstPwmCfgItem)
{
    Uint16 u16PwmId = 0;

    u16PwmId = pstPwmCfgItem->u16PwmId;

    EALLOW;
    // 配置TZ故障封波，为了安全，应该强制置为高阻态或者相同电平
    pstPwmRegs[u16PwmId]->TZCTL.bit.TZA = pstPwmCfgItem->bPwmxATzLevel;     // 配置出现故障时，A通道输出电平
    pstPwmRegs[u16PwmId]->TZCTL.bit.TZB = pstPwmCfgItem->bPwmxBTzLevel;     // 配置出现故障时，B通道输出电平
    pstPwmRegs[u16PwmId]->TZFRC.bit.OST = TZ_ENABLE;                    // 停止发波
//    pstPwmRegs[u16PwmId]->TZCLR.bit.CBCPULSE = 3;                       // 关闭CRC周期跳闸清除
//    pstPwmRegs[u16PwmId]->TZFRC.bit.CBC = 1;                            // 触发CBC跳闸事件

    // 关波
    pstPwmRegs[u16PwmId]->AQCSFRC.bit.CSFB = 2;
    pstPwmRegs[u16PwmId]->AQCSFRC.bit.CSFA = 1;

    if (pstPwmCfgItem->bGload == 1)
    {
        // 配置全局加载
        pstPwmRegs[u16PwmId]->GLDCFG.bit.CMPA_CMPAHR = 1;
        pstPwmRegs[u16PwmId]->GLDCFG.bit.CMPB_CMPBHR = 1;
        pstPwmRegs[u16PwmId]->CMPC = 1;
        pstPwmRegs[u16PwmId]->GLDCFG.bit.TBPRD_TBPRDHR = 1;
    	pstPwmRegs[u16PwmId]->GLDCFG.bit.DBFED_DBFEDHR = 1;
    	pstPwmRegs[u16PwmId]->GLDCFG.bit.DBRED_DBREDHR = 1;
        pstPwmRegs[u16PwmId]->GLDCFG.bit.DBCTL = 1;
        pstPwmRegs[u16PwmId]->GLDCFG.bit.AQCSFRC = 1;
        pstPwmRegs[u16PwmId]->EPWMXLINK.bit.GLDCTL2LINK = pstPwmCfgItem->LinkPwm;

        pstPwmRegs[u16PwmId]->GLDCTL.bit.OSHTMODE = 1;
        pstPwmRegs[u16PwmId]->GLDCTL.bit.GLDMODE = 0;
        pstPwmRegs[u16PwmId]->GLDCTL.bit.GLD = 1;
    }

    pstPwmRegs[u16PwmId]->TBCTL.bit.PRDLD = TB_SHADOW;                  // 使能周期值的影子寄存器
    pstPwmRegs[u16PwmId]->TBPRD = pstPwmCfgItem->u32Period;             // 设置周期值
    pstPwmRegs[u16PwmId]->TBCTR = 0;                                    // 清零计数器
    pstPwmRegs[u16PwmId]->TBPHS.bit.TBPHS = pstPwmCfgItem->bPhase;      // 设置移相值
    pstPwmRegs[u16PwmId]->TBCTL.bit.CTRMODE = pstPwmCfgItem->bCtrMode;  // 中心计数模式
    pstPwmRegs[u16PwmId]->TBCTL.bit.CLKDIV = TB_DIV1;
    pstPwmRegs[u16PwmId]->TBCTL.bit.HSPCLKDIV = TB_DIV1;
    pstPwmRegs[u16PwmId]->TBCTL.bit.PHSDIR = pstPwmCfgItem->bSyncCntLoadDir;    // 触发相位同步时，计数方向向下计数0:下  1:上
    pstPwmRegs[u16PwmId]->TBCTL.bit.FREE_SOFT = EPWM_EMULATION_STOP_AFTER_NEXT_TB;        // 仿真模式下自由运行
    pstPwmRegs[u16PwmId]->TBCTL.bit.PHSEN = 1;      // 如果使能了PHSEN，PHS需要2个TBCLK才能加载到CTR，所以需要做一个延迟补偿

    if (pstPwmCfgItem->bSyncMode == PWM_MASTER)
    {
        pstPwmRegs[u16PwmId]->EPWMSYNCINSEL.bit.SEL = 0;    // 配置同步输入信号产生条件
        pstPwmRegs[u16PwmId]->EPWMSYNCOUTEN.all = pstPwmCfgItem->bSyncOutSel; // 配置同步输出信号产生条件
    }
    else if (pstPwmCfgItem->bSyncMode == PWM_SLAVE)    // PWM_SLAVE 和 PWM_BRIDGE模式
    {
        pstPwmRegs[u16PwmId]->EPWMSYNCINSEL.bit.SEL = pstPwmCfgItem->bSyncInSel;    // 配置同步输入信号产生条件
        pstPwmRegs[u16PwmId]->EPWMSYNCOUTEN.all = 0; // 配置同步输出信号产生条件
    }
    else
    {
        pstPwmRegs[u16PwmId]->EPWMSYNCINSEL.bit.SEL = pstPwmCfgItem->bSyncInSel;    // 配置同步输入信号产生条件
        pstPwmRegs[u16PwmId]->EPWMSYNCOUTEN.all = pstPwmCfgItem->bSyncOutSel; // 配置同步输出信号产生条件
    }

    pstPwmRegs[u16PwmId]->CMPA.bit.CMPA = 0;    // 设置比较值
    pstPwmRegs[u16PwmId]->CMPB.bit.CMPB = pstPwmCfgItem->u32Period >> 1;    // 设置比较值
    pstPwmRegs[u16PwmId]->CMPC = 100;
    pstPwmRegs[u16PwmId]->CMPD = 0;
    pstPwmRegs[u16PwmId]->TBCTL2.bit.PRDLDSYNC = EPWM_SHADOW_LOAD_MODE_COUNTER_SYNC;



    // 比较寄存器
    pstPwmRegs[u16PwmId]->CMPCTL.bit.LOADAMODE = EPWM_COMP_LOAD_ON_CNTR_ZERO;           // 配置加载时刻
    pstPwmRegs[u16PwmId]->CMPCTL.bit.LOADBMODE = EPWM_COMP_LOAD_ON_CNTR_ZERO;           // 配置加载时刻
    pstPwmRegs[u16PwmId]->CMPCTL.bit.SHDWAMODE = CC_SHADOW;                 // 使能影子寄存器
    pstPwmRegs[u16PwmId]->CMPCTL.bit.SHDWBMODE = CC_SHADOW;                 // 使能影子寄存器

    pstPwmRegs[u16PwmId]->AQCTL.bit.SHDWAQAMODE = 1;                   // 使能影子寄存器
    pstPwmRegs[u16PwmId]->AQCTL.bit.SHDWAQBMODE = 1;                   // 使能影子寄存器

    // 动作限定寄存器
    pstPwmRegs[u16PwmId]->AQCTLA.bit.CAU = AQ_SET;                          // 配置输出波形
    pstPwmRegs[u16PwmId]->AQCTLA.bit.CBU = AQ_CLEAR;                        // 配置输出波形

    // B通道由CMPB生成
    pstPwmRegs[u16PwmId]->AQCTLB.bit.CAU = AQ_SET;                        // 配置输出波形
    pstPwmRegs[u16PwmId]->AQCTLB.bit.CBU = AQ_CLEAR;                      // 配置输出波形

    // 死区寄存器
    pstPwmRegs[u16PwmId]->DBCTL2.bit.SHDWDBCTLMODE = 1;                     // 使能影子寄存器(关于影子寄存器，在使能的时候，会将影子寄存器的值加载一次)
    pstPwmRegs[u16PwmId]->DBCTL.bit.LOADFEDMODE = EPWM_RED_LOAD_ON_CNTR_ZERO;
    pstPwmRegs[u16PwmId]->DBCTL.bit.LOADREDMODE = EPWM_FED_LOAD_ON_CNTR_ZERO;
    pstPwmRegs[u16PwmId]->DBCTL.bit.HALFCYCLE = EPWM_DB_COUNTER_CLOCK_FULL_CYCLE;
    pstPwmRegs[u16PwmId]->DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;              // 启用FED和RED(使能A和B通道)
    pstPwmRegs[u16PwmId]->DBCTL.bit.POLSEL = DB_ACTV_HIC;                   // 配置输出极性(输出反相)
    pstPwmRegs[u16PwmId]->DBCTL.bit.IN_MODE = DBA_RED_DBB_FED;                      // EPWMxA作为上升沿延迟和下降沿延迟的输入源


    pstPwmRegs[u16PwmId]->DBRED.all = pstPwmCfgItem->bDeadTime;             // 配置死区时间，单位0.01us
    pstPwmRegs[u16PwmId]->DBFED.all = pstPwmCfgItem->bDeadTime;             // 配置死区时间，单位0.01us


    // 配置事件触发
    pstPwmRegs[u16PwmId]->ETSEL.bit.SOCAEN = 1;                             // 使能触发ADC采样事件
    pstPwmRegs[u16PwmId]->ETSEL.bit.SOCASEL = ET_CTRU_CMPB;                 // 配置事件产生时刻
    pstPwmRegs[u16PwmId]->ETSEL.bit.SOCASELCMP = 1;                         // 配置多路选择器选项

    pstPwmRegs[u16PwmId]->ETPS.bit.SOCPSSEL = 1;
    pstPwmRegs[u16PwmId]->ETSOCPS.bit.SOCAPRD2 = 4;                         // 4分频


#if 0
    pstPwmRegs[u16PwmId]->ETPS.bit.INTPRD = 1;     // 配置PWM事件中断
    pstPwmRegs[u16PwmId]->ETSEL.bit.INTEN = 1;     // 使能中断
    pstPwmRegs[u16PwmId]->ETSEL.bit.INTSEL = 1;    // 1:CTR == ZREO 2:CTR == PRD
    pstPwmRegs[u16PwmId]->ETCLR.bit.INT = 1;
#endif
    EDIS;
}



/**
 * @brief 用于初始化一组ePWM，通常用于参数初始化阶段
 * 
 * @param pstPwmCfg :入参是一组ST_PWM_CFG_ITEM结构体对象的集合,即ST_PWM_CFG
 */
void Hw_PwmInit(ST_PWM_CFG *pstPwmCfg)
{
    Uint16 i = 0;
    // 禁用时钟同步所有PWM的时钟
    Hw_DisableAllPwmCLKSync();
    for (i = 0; i < pstPwmCfg->u16ItemNum; i++)
    {
        Hw_SinglePwmInit(&pstPwmCfg->pstPwmCfgItem[i]);
    }

    // 启动所有PWM的时钟
    Hw_EnableAllPwmCLKSync();

    Hw_StopAllPWM();
    // 这里还需要触发全局加载
    Hw_TrigGlobalLoad();
}



/**
 * @brief 使能所有PWM的时钟同步
 * 
 */
void Hw_EnableAllPwmCLKSync(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
}



/**
 * @brief 禁止所有PWM的时钟同步
 * 
 */
void Hw_DisableAllPwmCLKSync(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;
}



#if 0


/**
 * @brief 修改一组ePWM的CMP值
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32ValueArry  :存放对应PwmId数组索引的CMP值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16    -1:参数非法
 *                  -2:ID非法
 *                  0: 设置成功
 */
int16 Hw_PwmSetCompareGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize)
{
    Uint16 i = 0;
    if ((u16PwmIdArry == NULL) || (u32ValueArry == NULL))
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {
        pstPwmRegs[u16PwmIdArry[i]]->CMPA.bit.CMPA = u32ValueArry[i];
        pstPwmRegs[u16PwmIdArry[i]]->CMPB.bit.CMPB = pstPwmRegs[u16PwmIdArry[i]]->TBPRD - u32ValueArry[i];
    }

    EDIS;
    EINT;
    return 0;
}



/**
 * @brief 设置单个ePwm的比较值
 * 
 * @param u16PwmId ePwm的ID号
 * @param u32Value 比较值
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
int16 Hw_PwmSetCompare(Uint16 u16PwmId, Uint32 u32Value)
{
    if ((u16PwmId >= PWM_ID_MAX))
    {
        return -1;      // 参数非法
    }

    DINT;
    EALLOW;

    pstPwmRegs[u16PwmId]->CMPA.bit.CMPA = u32Value;
    pstPwmRegs[u16PwmId]->CMPB.bit.CMPB = pstPwmRegs[u16PwmId]->TBPRD - u32Value;

    EDIS;
    EINT;
    return 0;
}



/**
 * @brief 修改指定PWM的周期值和占空比
 *          周期值取值范围为 PWM_MIN_PRIOD ~ PWM_MAX_PRIOD，
 *          如果超过范围则取边界值
 * @param u16PwmId :ePwm的ID号
 * @param u32Value :周期值
 * @param fDuty    :占空比，单位为百分数
 * @return int16        -1:参数非法
 *                      0: 设置成功
 */
int16 Hw_PwmSetPeriod(Uint16 u16PwmId, Uint32 u32Value, float fDuty)
{
    Uint32 u32OriginPRD = 0;
    Uint32 u32OriginCMP = 0;

    if ((u16PwmId >= PWM_ID_MAX))
    {
        return -1;      // 参数非法
    }

    if(u32Value <= PWM_MIN_PRIOD)
    {
        u32Value = PWM_MIN_PRIOD;
    }
    if (u32Value >= PWM_MAX_PRIOD)
    {
        u32Value = PWM_MAX_PRIOD;
    }

    DINT;
    EALLOW;

    pstPwmRegs[u16PwmId]->TBPRD = u32Value;
    pstPwmRegs[u16PwmId]->CMPA.bit.CMPA = (Uint16)(u32Value * fDuty / 100);

    EDIS;
    EINT;

    return 0;
}



/**
 * @brief 修改一组Pwm的周期值，同时保持原有的占空比不变
 * 
 * @param u16PwmIdArry :存放PwmId的数组
 * @param u32ValueArry :存放对应PwmId数组索引的周期值
 * @param u16ArrySize :PwmId数组大小
 * @param fDuty    :占空比，单位为百分数
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
int16 Hw_PwmSetPeriodGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize, float fDuty)
{
    Uint32 u32OriginPRD = 0;
    Uint32 u32OriginCMP = 0;
    Uint16 i = 0;

    if ((u16PwmIdArry == NULL) || (u32ValueArry == NULL))
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {

        if(u32ValueArry[i] <= PWM_MIN_PRIOD)
        {
            u32ValueArry[i] = PWM_MIN_PRIOD;
        }
        if (u32ValueArry[i] >= PWM_MAX_PRIOD)
        {
            u32ValueArry[i] = PWM_MAX_PRIOD;
        }

        pstPwmRegs[u16PwmIdArry[i]]->TBPRD = u32ValueArry[i];
        pstPwmRegs[u16PwmIdArry[i]]->CMPA.bit.CMPA = (Uint16)(u32ValueArry[i] * fDuty / 100);
    }

    EDIS;
    EINT;
    return 0;
}



/**
 * @brief 修改指定PWM偏移的相位
 * 
 * @param u16PwmId :ePwm的ID号
 * @param u32Value :相位值
 * @return int16        -1:参数非法
 *                      0: 设置成功
 */
int16 Hw_PwmSetPhase(Uint16 u16PwmId, Uint32 u32Value)
{
    Uint32 u32OriginCMP = 0;
    if ((u16PwmId >= PWM_ID_MAX))
    {
        return -1;      // 参数非法
    }

    EALLOW;

    pstPwmRegs[u16PwmId]->TBPHS.bit.TBPHS = u32Value;

    EDIS;
    return 0;
}



/**
 * @brief 修改一组PWM偏移的相位
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32ValueArry  :存放对应PwmId数组索引的相位值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
int16 Hw_PwmSetPhaseGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize)
{
    Uint16 i = 0;

    if ((u16PwmIdArry == NULL) || (u32ValueArry == NULL))
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {
        
        pstPwmRegs[u16PwmIdArry[i]]->TBPHS.bit.TBPHS = u32ValueArry[i];

    }

    EDIS;
    EINT;
    return 0;
}




/**
 * @brief 修改一组PWM的周期值、CMPAB的值
 *
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32PeriodArry  :存放对应PwmId数组索引的周期值
 * @param u32CmpAArry    :存放对应PwmId数组索引的CMPA值
 * @param u32CmpBArry    :存放对应PwmId数组索引的CMPB值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
int16 Hw_PwmSetPeriodAndCMPAB(Uint16 *u16PwmIdArry, Uint32 *u32PeriodArry, Uint32 *u32CmpAArry, Uint32 *u32CmpBArry, Uint16 u16ArrySize)
{
    Uint16 i = 0;

    if ((u16PwmIdArry == NULL) || (u32CmpAArry == NULL) || (u32CmpBArry == NULL) || (u32PeriodArry == NULL))
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {
        pstPwmRegs[u16PwmIdArry[i]]->TBPHS.bit.TBPHS = u32PeriodArry[i];
        pstPwmRegs[u16PwmIdArry[i]]->CMPA.bit.CMPA = u32CmpAArry[i];
        pstPwmRegs[u16PwmIdArry[i]]->CMPB.bit.CMPB = u32CmpBArry[i];
    }

    EDIS;
    EINT;

    return 0;
}



/**
 * @brief 启动单个PWM的波形输出
 * 
 * @param u16PwmId ePwm的ID号
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
int16 Hw_PwmStart(Uint16 u16PwmId)
{
    if ((u16PwmId >= PWM_ID_MAX))
    {
        return -1;      // 参数非法
    }

    EALLOW;

    if (pstPwmRegs[u16PwmId]->TZFLG.bit.OST == 1)
    {
        pstPwmRegs[u16PwmId]->TZFRC.bit.CBC = 1;     // 先触发CBC跳闸事件，关波
        pstPwmRegs[u16PwmId]->TZCLR.bit.OST = 1;     // 清除OST关波
        pstPwmRegs[u16PwmId]->TZCLR.bit.CBC = 0;     // 启动CRC周期跳闸清除CTR == 0时自动清除，然后产生发波
    }


    EDIS;
    return 0;
}


/**
 * @brief 停止单个PWM的波形输出
 * 
 * @param u16PwmId ePwm的ID号
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
int16 Hw_PwmStop(Uint16 u16PwmId)
{
    if ((u16PwmId >= PWM_ID_MAX))
    {
        return -1;      // 参数非法
    }

    EALLOW;

    pstPwmRegs[u16PwmId]->TZCLR.bit.CBC = 3;     // 关闭CRC周期跳闸清除
    pstPwmRegs[u16PwmId]->TZFRC.bit.OST = 1;
//    pstPwmRegs[u16PwmId]->CMPA.bit.CMPA = pstPwmRegs[u16PwmId]->TBPRD;

    EDIS;
    return 0;
}



/**
 * @brief 启动一组PWM输出，通过清除TZ模块故障来输出PWM波形
 * 
 * @param u16PwmIdArry :存放PwmId的数组
 * @param u16ArrySize :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
int16 Hw_PwmStartGroup(Uint16 *u16PwmIdArry, Uint16 u16ArrySize)
{
    Uint16 i = 0;
    if (u16PwmIdArry == NULL)
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {
        if (pstPwmRegs[u16PwmIdArry[i]]->TZFLG.bit.OST == 1)
        {
            pstPwmRegs[u16PwmIdArry[i]]->TZFRC.bit.CBC = 1;     // 先触发CBC跳闸事件，关波
            pstPwmRegs[u16PwmIdArry[i]]->TZCLR.bit.OST = 1;     // 清除OST关波
            pstPwmRegs[u16PwmIdArry[i]]->TZCLR.bit.CBC = 0;     // 启动CRC周期跳闸清除CTR == 0时自动清除，然后产生发波
        }
    }

    EDIS;
    EINT;
    return 0;
}



/**
 * @brief 停止一组PWM输出，通过触发TZ模块故障来停止PWM波形输出
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
int16 Hw_PwmStopGroup(Uint16 *u16PwmIdArry, Uint16 u16ArrySize)
{
    Uint16 i = 0;
    if (u16PwmIdArry == NULL)
    {
        return -1;      // 参数非法
    }

    for (i = 0; i < u16ArrySize; i++)
    {
        if (u16PwmIdArry[i] >= PWM_ID_MAX)
        {
            return -2;   // ID非法
        }
    }

    DINT;
    EALLOW;

    for (i = 0; i < u16ArrySize; i++)
    {
        pstPwmRegs[u16PwmIdArry[i]]->TZCLR.bit.CBC = 3;     // 关闭CRC周期跳闸清除
        pstPwmRegs[u16PwmIdArry[i]]->TZFRC.bit.OST = 1;
        // pstPwmRegs[u16PwmIdArry[i]]->CMPA.bit.CMPA = pstPwmRegs[u16PwmIdArry[i]]->TBPRD;
    }

    EDIS;
    EINT;
    return 0;
}

#endif

/**
 * @brief Pwm外设的控制接口
 *
 * @param u16PwmId :Pwm的ID号
 * @param u16Cmd  :PWM命令枚举
 *                  CFG_SYNC_OUT    可用参数
 *                                  EPWM_SYNC_OUT_FROM_SOFTWARE
 *                                  EPWM_SYNC_OUT_FROM_CNTR_ZERO
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_B
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_D
 *                                  EPWM_SYNC_OUT_FROM_DCA_EVT1_SYNC
 *                                  EPWM_SYNC_OUT_FROM_DCB_EVT1_SYNC
 *                                  EPWM_SYNC_OUT_FROM_ALL
 *
 *                  CFG_SYNC_IN     可用参数
 *                                  EPWM_SYNC_IN_FROM_DISABLE
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM2
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM3
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM4
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM5
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM6
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM8
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP1
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP2
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP3
 *                                  EPWM_SYNC_IN_FROM_INPUTXBAR_OUT5
 *                                  EPWM_SYNC_IN_FROM_INPUTXBAR_OUT6
 *                                  EPWM_SYNC_IN_FROM_FSIRX_TRIG1
 *
 *                  CFG_PHS_IS_ENABLE   可用参数
 *                                      1：使能
 *                                      0：禁用
 *
 *                  CFG_DEALTIME        可用参数
 *                                      0 ~ 2^13-1
 *                                      (需要用户自行关闭PWM，避免在修改时刻前后上下沿死区不一致)
 *
 *                  CFG_SOFT_MODE   软启动发波模式：A和B通道独立，关闭死区
 *
 *                  CFG_NORMAL_MODE 正常发波模式：A和B通道互补，带死区
 * @param vParam :参数
 * @return int
 */

#pragma CODE_SECTION(Hw_PwmCtrl, ".ISR.ramfunc");
int Hw_PwmCtrl(Uint16 u16PwmId, Uint16 u16Cmd, void *vParam)
{
    EALLOW;
//    if (u16PwmId < PWM_ID0 || u16PwmId >= PWM_ID_MAX)
//    {
//        return -1;  // ID非法
//    }


    switch (u16Cmd)
    {

    case CFG_PHS_IS_ENABLE:
        if (vParam == NULL)
        {
            return -2;  // 参数非法
        }
        pstPwmRegs[u16PwmId]->TBCTL.bit.PHSEN = *(Uint16*)vParam;           // 配置相位同步使能

        if (*(Uint16*)vParam)
        {
            pstPwmRegs[u16PwmId]->TBCTL.bit.PHSDIR = 0; // 计数向下
        }
        else
        {
            pstPwmRegs[u16PwmId]->TBCTL.bit.PHSDIR = 1; // 计数向上
        }
        break;

    case CFG_SYNC_IN:
        if (vParam == NULL)
        {
            return -2;  // 参数非法
        }
        pstPwmRegs[u16PwmId]->EPWMSYNCINSEL.bit.SEL = *(Uint16*)vParam;     // 配置同步输入信号产生条件

        break;

    case CFG_SYNC_OUT:
        if (vParam == NULL)
        {
            return -2;  // 参数非法
        }
        pstPwmRegs[u16PwmId]->EPWMSYNCOUTEN.all = *(Uint16*)vParam;         // 配置同步输出信号产生条件
        break;

    case CFG_DEALTIME:
        if (vParam == NULL)
        {
            return -2;  // 参数非法
        }
        pstPwmRegs[u16PwmId]->DBRED.all = *(Uint16*)vParam;             // 配置死区时间，单位0.01us
        pstPwmRegs[u16PwmId]->DBFED.all = *(Uint16*)vParam;             // 配置死区时间，单位0.01us
        break;

    case CFG_SOFT_MODE:
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CAU = AQ_NO_ACTION;                    // 配置输出波形
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CAD = AQ_NO_ACTION;                    // 配置输出波形

        pstPwmRegs[u16PwmId]->AQCTLB.bit.CBU = AQ_CLEAR;                        // 配置输出波形
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CBD = AQ_SET;                          // 配置输出波形

        pstPwmRegs[u16PwmId]->DBCTL.bit.OUT_MODE = DB_DISABLE;                  // 禁用FED和RED(使能A和B通道)
        break;

    case CFG_NORMAL_MODE:
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CBU = AQ_NO_ACTION;                    // 配置输出波形
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CBD = AQ_NO_ACTION;                    // 配置输出波形
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CAU = AQ_CLEAR;                        // 配置输出波形
        pstPwmRegs[u16PwmId]->AQCTLB.bit.CAD = AQ_SET;                          // 配置输出波形
        pstPwmRegs[u16PwmId]->DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;              // 启用FED和RED(使能A和B通道)
        break;

    default:
        break;
    }
    EDIS;
    return 1;
}





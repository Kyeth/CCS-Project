/**
 * @file Hw_EcapDriver.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_EcapDriver.h"



#ifdef __cplusplus
extern "C" {
#endif

static volatile Uint16 *pu16InputXbarSelect[] = {&InputXbarRegs.INPUT7SELECT, &InputXbarRegs.INPUT8SELECT,
                                                 &InputXbarRegs.INPUT9SELECT, &InputXbarRegs.INPUT10SELECT,
                                                 &InputXbarRegs.INPUT11SELECT, &InputXbarRegs.INPUT12SELECT, };

static volatile struct ECAP_REGS *pstEcapRegs[] = {&ECap1Regs, &ECap2Regs, &ECap3Regs,
                                                   };


static ST_ECAP_HANDLE stEcapHandle[ECAP_ID_MAX] = {0};




/**
 * @brief 用于初始化单个eCap外设
 * 
 * @param pstCapCfgItem 入参是一个ST_ECAP_CFG_ITEM结构体对象的地址
 */
void Hw_SingleEcapInit(ST_ECAP_CFG_ITEM *pstCapCfgItem)
{
    Uint16 u16EcapId = 0;
    Uint16 u16StopWrapCnt = 0;

    u16EcapId = pstCapCfgItem->u16EcapId;

    EALLOW;
    *pu16InputXbarSelect[u16EcapId] = pstCapCfgItem->bGPIONum;

    stEcapHandle[u16EcapId].pstEcapRegs = pstEcapRegs[u16EcapId];

    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.TSCTRSTOP = 0;  // 停止计数器
    stEcapHandle[u16EcapId].pstEcapRegs->ECEINT.all = 0x0000;       // 关闭所有中断
    stEcapHandle[u16EcapId].pstEcapRegs->ECCLR.all  = 0xFFFF;       // 清除所有中断标志位
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.PRESCALE = 0;   // 事件触发分频系数，0：1分配
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.CONT_ONESHT = pstCapCfgItem->bCapOneShtMode;

    if (pstCapCfgItem->bCap1OL != DISABLE_EDGE)
    {
        stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.CAP1POL = pstCapCfgItem->bCap1OL & 0x01;
        // 这里不需要把u16StopWrapCnt++;
    }

    if (pstCapCfgItem->bCap2OL != DISABLE_EDGE)
    {
        stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.CAP2POL = pstCapCfgItem->bCap2OL & 0x01;
        u16StopWrapCnt++;
    }

    if (pstCapCfgItem->bCap3OL != DISABLE_EDGE)
    {
        stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.CAP3POL = pstCapCfgItem->bCap3OL & 0x01;
        u16StopWrapCnt++;
    }

    if (pstCapCfgItem->bCap4OL != DISABLE_EDGE)
    {
        stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.CAP4POL = pstCapCfgItem->bCap4OL & 0x01;
        u16StopWrapCnt++;
    }

    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.STOP_WRAP = u16StopWrapCnt & 0x03;
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.SYNCI_EN = 0;   // 禁止
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.SYNCO_SEL = 3;  // 禁止

    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;          // Enable capture units
    ECap1Regs.ECCTL2.bit.REARM = 1;            // arm one-shot
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;        // Start Counter

    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL1.bit.CAPLDEN = 1;    // 使能Ecap模块
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.REARM = 1;      // 复位Mod4状态计数器
    stEcapHandle[u16EcapId].pstEcapRegs->ECCTL2.bit.TSCTRSTOP = 1;  // 启动计数器

    stEcapHandle[u16EcapId].bIsInit = 1;
    EDIS;
}



/**
 * @brief 用于初始化一组eCap，通常用于参数初始化阶段
 * 
 * @param pstCapCfg :入参是一组ST_CAP_CFG结构体对象的集合
 */
void Hw_EcapInit(ST_CAP_CFG *pstCapCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstCapCfg->u16ItemNum; i++)
    {
        Hw_SingleEcapInit(&pstCapCfg->pstEcapCfgItem[i]);
    }
}



/**
 * @brief 获取对应Cap的计数值
 * 
 * @param u16EcapId :eCap的ID
 * @param Index     :eCap的索引(可选1、2、3、4)
 * @return Uint32   :返回对应eCap寄存器的值
 *                  :0xFFFFFFF 索引非法
 */
Uint32 Hw_GetEcapValue(Uint16 u16EcapId, Uint16 Index)
{
    Uint32 u32EcapValue = 0xFFFFFFFF;

    if (u16EcapId < ECAP_ID_MAX)
    {
        switch (Index)
        {
            case 1:
                u32EcapValue = stEcapHandle[u16EcapId].u32EcapValue_1;
                break;

            case 2:
                u32EcapValue = stEcapHandle[u16EcapId].u32EcapValue_2;
                break;

            case 3:
                u32EcapValue = stEcapHandle[u16EcapId].u32EcapValue_3;
                break;

            case 4:
                u32EcapValue = stEcapHandle[u16EcapId].u32EcapValue_4;
                break;
            default:
                u32EcapValue = 0xFFFFFFFF;
                break;
        }
    }

    return u32EcapValue;
}


/**
 * @brief 返回eCap捕获事件
 * 
 * @param u16EcapId :eCap的ID
 * @param Index     :eCap的索引(可选1、2、3、4)
 * @return int16    :1 发生捕获事件
 *                  :0 未发生事件
 *                  :-1 ID非法
 *                  :-2 索引非法
 */
int16 Hw_GetEcapCEVT(Uint16 u16EcapId, Uint16 Index)
{
    int16 i16EcapCEVT = 0;

    if (u16EcapId >= ECAP_ID_MAX)
    {
        return -1;  // ID非法
    }

    switch (Index)
    {
        case 1:
            i16EcapCEVT = stEcapHandle[u16EcapId].bEcapCEVT1;
            break;

        case 2:
            i16EcapCEVT = stEcapHandle[u16EcapId].bEcapCEVT2;
            break;

        case 3:
            i16EcapCEVT = stEcapHandle[u16EcapId].bEcapCEVT3;
            break;

        case 4:
            i16EcapCEVT = stEcapHandle[u16EcapId].bEcapCEVT4;
            break;

        default:
            i16EcapCEVT = -2;
            break;
    }

    return i16EcapCEVT;
}



/**
 * @brief 检测eCap事件是否发生，通常在中断中调用
 * 
 */
void Hw_IsrEcapData(void)
{
    Uint16 i = 0;

    // 遍历所有Ecap模块
    for(i = ECAP_ID0; i < ECAP_ID_MAX; i++)
    {
        // 针对已经初始化过的模块执行读操作
        if (stEcapHandle[i].bIsInit)
        {
            stEcapHandle[i].bEcapCEVT1 = stEcapHandle[i].pstEcapRegs->ECFLG.bit.CEVT1;
            if (stEcapHandle[i].bEcapCEVT1 == 1)
            {
                stEcapHandle[i].u32EcapValue_1 = stEcapHandle[i].pstEcapRegs->CAP1;
            }

            stEcapHandle[i].bEcapCEVT2 = stEcapHandle[i].pstEcapRegs->ECFLG.bit.CEVT2;
            if (stEcapHandle[i].bEcapCEVT2 == 1)
            {
                stEcapHandle[i].u32EcapValue_2 = stEcapHandle[i].pstEcapRegs->CAP2;
            }

            stEcapHandle[i].bEcapCEVT3 = stEcapHandle[i].pstEcapRegs->ECFLG.bit.CEVT3;
            if (stEcapHandle[i].bEcapCEVT3 == 1)
            {
                stEcapHandle[i].u32EcapValue_3 = stEcapHandle[i].pstEcapRegs->CAP3;
            }

            stEcapHandle[i].bEcapCEVT4 = stEcapHandle[i].pstEcapRegs->ECFLG.bit.CEVT4;
            if (stEcapHandle[i].bEcapCEVT4 == 1)
            {
                stEcapHandle[i].u32EcapValue_4 = stEcapHandle[i].pstEcapRegs->CAP4;
            }

            stEcapHandle[i].pstEcapRegs->ECCLR.all = 0xFFFF ;              // 清所有标志
        }
    }


}

#ifdef __cplusplus
}
#endif



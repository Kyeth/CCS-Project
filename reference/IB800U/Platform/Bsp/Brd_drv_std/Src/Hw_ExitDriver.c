/**
 * @file Hw_ExitDriver.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_ExitDriver.h"
#include "F28003x_Pie_defines.h"


#ifdef __cplusplus
extern "C"{
#endif

volatile union  XINT1CR_REG *pstXintRegs[] = {(volatile union  XINT1CR_REG *)&XintRegs.XINT1CR,
                                              (volatile union  XINT1CR_REG *)&XintRegs.XINT2CR,
                                              (volatile union  XINT1CR_REG *)&XintRegs.XINT3CR,
                                              (volatile union  XINT1CR_REG *)&XintRegs.XINT4CR,
                                              (volatile union  XINT1CR_REG *)&XintRegs.XINT5CR,};


volatile Uint16 *pu16InputSelect[] = {&InputXbarRegs.INPUT4SELECT, &InputXbarRegs.INPUT5SELECT,
                                      &InputXbarRegs.INPUT6SELECT, &InputXbarRegs.INPUT13SELECT,
                                      &InputXbarRegs.INPUT14SELECT};



/**
 * @brief 用于初始化单个引脚的外部中断
 * 
 * @param pstExitCfgItem 入参是一个ST_EXIT_CFG_ITEM结构体对象的地址
 */
void Hw_SingleExitInit(ST_EXIT_CFG_ITEM *pstExitCfgItem)
{
    Uint16 u16ExitId = 0;

    u16ExitId = pstExitCfgItem->bExitId;

    EALLOW;
    *pu16InputSelect[u16ExitId] = pstExitCfgItem->bGPIONum;
    pstXintRegs[u16ExitId]->bit.POLARITY = pstExitCfgItem->bTrigPolarity;
    pstXintRegs[u16ExitId]->bit.ENABLE = 1;

    switch(u16ExitId)
    {
    case EXIT_ID0:
        PieCtrlRegs.PIEIER1.bit.INTx4 = 1;
        break;

    case EXIT_ID1:
        PieCtrlRegs.PIEIER1.bit.INTx5 = 1;
        break;

    case EXIT_ID2:
        PieCtrlRegs.PIEIER12.bit.INTx1 = 1;
        break;

    case EXIT_ID3:
        PieCtrlRegs.PIEIER12.bit.INTx2 = 1;
        break;

    case EXIT_ID4:
        PieCtrlRegs.PIEIER12.bit.INTx3 = 1;
        break;
    }
    EDIS;
}



/**
 * @brief 用于初始化一组GPIO的外部中断，通常用于参数初始化阶段
 * 
 * @param pstExitCfg :入参是一组ST_EXIT_CFG结构体对象的集合
 */
void Hw_ExitInit(ST_EXIT_CFG *pstExitCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstExitCfg->u16ItemNum; i++)
    {
        Hw_SingleExitInit(&pstExitCfg->pstExitCfgItem[i]);
    }
}



/**
 * @brief 清除中断标志
 * 
 * @param u16ExitId 外部中断ID号
 * @return int32    :-1 ID非法
 *                  : PieCtrlRegs.PIEACK.all
 */
int32 Hw_ClearExitFlag(Uint16 u16ExitId)
{

    if (u16ExitId >= EXIT_ID_MAX)
    {
        return -1;
    }

    if (u16ExitId < 2)
    {
        PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    }
    else
    {
        PieCtrlRegs.PIEACK.all = PIEACK_GROUP12;
    }

    return PieCtrlRegs.PIEACK.all;
}

#ifdef __cplusplus
}
#endif

/**
 * @file Hw_ExitDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_EXITDRIVER_H
#define __HW_EXITDRIVER_H

#include "F28003x_device.h"


#ifdef __cplusplus
extern "C"{
#endif



#define RISING_EDGE     1
#define FALLING_EDGE    2
#define FULL_EDGE       3


typedef enum
{
    EXIT_ID0 = 0x00,
    EXIT_ID1 = 0x01,
    EXIT_ID2 = 0x02,
    EXIT_ID3 = 0x03,
    EXIT_ID4 = 0x04,
    EXIT_ID_MAX,
    EXIT_ID_NULL = 0xFF,

}E_EXITID;


typedef struct
{
    Uint16 bExitId          :3;
    Uint16 bGPIONum         :8;
    Uint16 bTrigPolarity    :2;     // 0:下降沿触发 1:上升沿触发 3:双边沿触发
    Uint16 bRsv             :3;
}ST_EXIT_CFG_ITEM;

typedef struct
{
    ST_EXIT_CFG_ITEM *pstExitCfgItem;
    Uint16 u16ItemNum;
}ST_EXIT_CFG;



/**
 * @brief 用于初始化单个引脚的外部中断
 * 
 * @param pstExitCfgItem 入参是一个ST_EXIT_CFG_ITEM结构体对象的地址
 */
extern void Hw_SingleExitInit(ST_EXIT_CFG_ITEM *pstExitCfgItem);



/**
 * @brief 用于初始化一组GPIO的外部中断，通常用于参数初始化阶段
 * 
 * @param pstExitCfg :入参是一组ST_EXIT_CFG结构体对象的集合
 */
extern void Hw_ExitInit(ST_EXIT_CFG *pstExitCfg);



/**
 * @brief 清除中断标志
 * 
 * @param u16ExitId 外部中断ID号
 * @return int32    :-1 ID非法
 *                  : PieCtrlRegs.PIEACK.all
 */
extern int32 Hw_ClearExitFlag(Uint16 u16ExitId);

#ifdef __cplusplus
}
#endif




#endif

/******************************************************************************
 * @file    Hw_ClaDriver.h
 * @author  yetingheng (004182)
 * @brief   CLA驱动头文件（裸寄存器实现，不依赖driverlib）
 * @version 0.1
 * @date    2026-10-23
 *
 * @note    CLA Task1 触发链路：
 *          1. EPWM1 CTR=ZERO → SOCA，四路 ADC 同步启动转换
 *          2. ADC-A 的 SOC15（序列末位）转换完成 → ADCINT1
 *          3. ADCINT1 经 DmaClaSrcSelRegs 硬件触发 CLA Task1
 *          4. CLA Task1 完成 → CLA1_1 中断送 CPU（控制中断）
 *
 * @note    存储器划分（与 CMD/2837xS_FLASH_APP_CLA_lnk.cmd 一致）：
 *          - Cla1Prog    : 装载在 FLASH_APP，运行时拷贝到 RAMLS0~RAMLS3
 *          - .const_cla  : 装载在 FLASH_APP，运行时拷贝到 RAMLS4
 *          - Cla1DataRam : RAMLS5（CLA 与 CPU 共享的普通数据）
 *          - CpuToCla1MsgRAM / Cla1ToCpuMsgRAM : CPU 与 CLA 之间的专用消息 RAM
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#ifndef HW_CLA_DRIVER_H
#define HW_CLA_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 任务号枚举（0 基：与 MIER 的位号、MVECT 数组下标、触发源寄存器内偏移一致）
 ******************************************************************************/
typedef enum
{
    HW_CLA_TASK1 = 0U,          // CLA Task1
    HW_CLA_TASK2,               // CLA Task2
    HW_CLA_TASK3,               // CLA Task3
    HW_CLA_TASK4,               // CLA Task4
    HW_CLA_TASK5,               // CLA Task5
    HW_CLA_TASK6,               // CLA Task6
    HW_CLA_TASK7,               // CLA Task7
    HW_CLA_TASK8,               // CLA Task8
    HW_CLA_TASK_MAX,            // 任务数量
    HW_CLA_TASK_NULL = 0xFFU    // 无效任务号
} E_HW_CLA_TASK;

/******************************************************************************
 * 中断向量号枚举（对应 Cla1Regs.MVECT1 ~ MVECT8）
 ******************************************************************************/
typedef enum
{
    HW_CLA_VECT1 = 0U,          // 对应 MVECT1
    HW_CLA_VECT2,               // 对应 MVECT2
    HW_CLA_VECT3,               // 对应 MVECT3
    HW_CLA_VECT4,               // 对应 MVECT4
    HW_CLA_VECT5,               // 对应 MVECT5
    HW_CLA_VECT6,               // 对应 MVECT6
    HW_CLA_VECT7,               // 对应 MVECT7
    HW_CLA_VECT8,               // 对应 MVECT8
    HW_CLA_VECT_MAX             // 向量数量
} E_HW_CLA_VECT;

/******************************************************************************
 * 触发源枚举（写入 DmaClaSrcSelRegs.CLA1TASKSRCSEL1/2 的对应字节）
 * 只列出本项目可能用到的项，其余见 F2837xS TRM 的 CLA 触发源表
 ******************************************************************************/
typedef enum
{
    HW_CLA_TRIG_SOFTWARE = 0U,      // 仅软件 IACK 触发
    HW_CLA_TRIG_ADCA1    = 1U,      // ADC-A 中断1（本项目使用：SOC15 转换完成）
    HW_CLA_TRIG_ADCA2    = 2U,      // ADC-A 中断2
    HW_CLA_TRIG_ADCA3    = 3U,      // ADC-A 中断3
    HW_CLA_TRIG_ADCA4    = 4U,      // ADC-A 中断4
    HW_CLA_TRIG_ADCB1    = 6U,      // ADC-B 中断1
    HW_CLA_TRIG_ADCC1    = 11U,     // ADC-C 中断1
    HW_CLA_TRIG_ADCD1    = 16U,     // ADC-D 中断1
    HW_CLA_TRIG_EPWM1INT = 36U,     // EPWM1 中断
    HW_CLA_TRIG_EPWM2INT = 37U,     // EPWM2 中断
    HW_CLA_TRIG_EPWM3INT = 38U,     // EPWM3 中断
    HW_CLA_TRIG_TINT0    = 68U,     // CPU Timer0 中断
    HW_CLA_TRIG_TINT1    = 69U,     // CPU Timer1 中断（100us 节拍）
    HW_CLA_TRIG_TINT2    = 70U      // CPU Timer2 中断（1ms 心跳）
} E_HW_CLA_TRIG;

/******************************************************************************
 * CLA 配置项结构体
 ******************************************************************************/
typedef struct
{
    Uint16 u16TaskNo;           // 任务号 (HW_CLA_TASK1 ~ HW_CLA_TASK8)
    Uint16 u16TriggerSrc;       // 触发源 (HW_CLA_TRIG_xxx)
    void (*ClaTask)(void);      // CLA 任务函数地址（写入 MVECT，可填 0 表示不映射）
} ST_CLA_CFG_ITEM;

/******************************************************************************
 * CLA 配置表结构体
 ******************************************************************************/
typedef struct
{
    const ST_CLA_CFG_ITEM *pstClaCfgItem;   // 指向配置项数组
    Uint16 u16ItemNum;                      // 配置项数量
} ST_CLA_CFG;

/******************************************************************************
 * 函数声明
 ******************************************************************************/

/**
 * @brief  初始化单个 CLA 任务
 * @param  pstClaCfgItem CLA 配置项指针
 * @note   只负责映射任务向量、设置触发源、使能任务，不做存储器配置
 */
void Hw_ClaSingleInit(const ST_CLA_CFG_ITEM *pstClaCfgItem);

/**
 * @brief  CLA 初始化总入口
 * @param  pstClaCfg CLA 配置表指针
 * @note   依次完成：LS 存储器归属配置 → CLA 程序/常量拷贝到 RAM →
 *         消息 RAM 清零 → 逐任务映射向量/设置触发源/使能任务 → 打开 IACK
 */
void Hw_ClaInit(const ST_CLA_CFG *pstClaCfg);

/**
 * @brief  软件强制触发一个 CLA 任务（IACK 指令）
 * @param  u16TaskNo 任务号 (HW_CLA_TASK1 ~ HW_CLA_TASK8)
 */
void Hw_ClaTaskForce(Uint16 u16TaskNo);

/**
 * @brief 读取 CLA 任务运行溢出标志（任务没跑完又被触发）
 * @param  u16TaskNo 任务号 (HW_CLA_TASK1 ~ HW_CLA_TASK8)
 * @return 1：发生过溢出；0：正常
 */
Uint16 Hw_ClaGetOverflowFlag(Uint16 u16TaskNo);

#ifdef __cplusplus
}
#endif

#endif // HW_CLA_DRIVER_H

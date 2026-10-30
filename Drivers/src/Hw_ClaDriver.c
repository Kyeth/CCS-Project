/******************************************************************************
 * @file    Hw_ClaDriver.c
 * @author  yetingheng (004182)
 * @brief   CLA驱动实现文件（裸寄存器实现，不依赖driverlib）
 * @version 0.1
 * @date    2026-10-23
 *
 * @copyright Copyright (c) 2026
 ******************************************************************************/

#include "Hw_ClaDriver.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 链接器在 CMD 中生成的符号（EABI 模式无下划线前缀）
 * 注意：这些是"符号地址即数值"的链接器符号，必须用 &符号 取值
 ******************************************************************************/
extern Uint32 Cla1ProgRunStart;
extern Uint32 Cla1ProgLoadStart;
extern Uint32 Cla1ProgLoadSize;

extern Uint32 Cla1ConstRunStart;
extern Uint32 Cla1ConstLoadStart;
extern Uint32 Cla1ConstLoadSize;

/* LSxMSEL 中"CPU+CLA1 共享"的编码值 */
#define HW_CLA_LS_MASTER_CPU_CLA1   (1U)

/* 消息 RAM 初始化等待的超时上限（计数循环，约几十微秒 @200MHz 以上） */
#define HW_CLA_MSG_INIT_WAIT_MAX    (100000UL)

/******************************************************************************
 * @brief  判断任务号是否合法
 * @param  u16TaskNo 任务号
 * @return 1：合法；0：非法
 ******************************************************************************/
static Uint16 Hw_ClaTaskNoIsValid(Uint16 u16TaskNo)
{
    return (u16TaskNo < (Uint16)HW_CLA_TASK_MAX) ? 1U : 0U;
}

/******************************************************************************
 * @brief  配置 LS0~LS5 的存储器归属与用途
 * @note   LS0~LS3：CLA 程序空间（CLAPGM=1），LS4~LS5：CLA 数据空间（CLAPGM=0）
 *         必须先让 CLA 能访问该块（MSEL=1），再设置程序/数据属性
 ******************************************************************************/
static void Hw_ClaMemConfig(void)
{
    EALLOW;

    /* 1. LS0~LS5 全部交给 CPU+CLA1 共享 */
    MemCfgRegs.LSxMSEL.bit.MSEL_LS0 = HW_CLA_LS_MASTER_CPU_CLA1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS1 = HW_CLA_LS_MASTER_CPU_CLA1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS2 = HW_CLA_LS_MASTER_CPU_CLA1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS3 = HW_CLA_LS_MASTER_CPU_CLA1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS4 = HW_CLA_LS_MASTER_CPU_CLA1;
    MemCfgRegs.LSxMSEL.bit.MSEL_LS5 = HW_CLA_LS_MASTER_CPU_CLA1;

    /* 2. LS0~LS3 作为 CLA 程序空间，LS4~LS5 作为 CLA 数据空间 */
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS0 = 1U;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS1 = 1U;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS2 = 1U;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS3 = 1U;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS4 = 0U;
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS5 = 0U;

    EDIS;
}

/******************************************************************************
 * @brief 把 CLA 程序和 CLA 常量从 Flash 拷贝到 RAM 运行区
 * @note  Cla1Prog 的 RUN 地址在 RAMLS0~RAMLS3，必须先拷贝再触发任务
 ******************************************************************************/
static void Hw_ClaCopyToRam(void)
{
    Uint32 u32ProgSize = (Uint32)&Cla1ProgLoadSize;
    Uint32 u32ConstSize = (Uint32)&Cla1ConstLoadSize;

    if (u32ProgSize != 0UL)
    {
        (void)memcpy((void *)&Cla1ProgRunStart,
                     (const void *)&Cla1ProgLoadStart,
                     u32ProgSize);
    }

    if (u32ConstSize != 0UL)
    {
        (void)memcpy((void *)&Cla1ConstRunStart,
                     (const void *)&Cla1ConstLoadStart,
                     u32ConstSize);
    }
}

/******************************************************************************
 * @brief 清除 CPU 与 CLA 之间的消息 RAM
 * @note  必须等待 INITDONE 置位后才能继续，否则消息内容是脏的
 ******************************************************************************/
static Uint16 Hw_ClaMsgRamInit(void)
{
    Uint32 u32Wait;

    /* MSGxINIT 属于 MemCfg 寄存器组，受 EALLOW 保护：
     * 不在 EALLOW 区内写入会被硬件直接忽略，MSGxINITDONE 永远不置位，
     * 下面的等待就会永久阻塞（曾导致整机卡死在 Hw_ClaInit()，
     * 后续外设、中断与 StartSystem 全不执行，所有 ePWM 停在强制低）。
     * 等待另外加超时上限，避免任何驱动问题把整机卡死。 */
    EALLOW;
    MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1U;
    EDIS;

    u32Wait = HW_CLA_MSG_INIT_WAIT_MAX;

    while ((MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1U) && (u32Wait > 0UL))
    {
        u32Wait--;
    }
    if (u32Wait == 0UL) { return 0U; }

    EALLOW;
    MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1U;
    EDIS;

    u32Wait = HW_CLA_MSG_INIT_WAIT_MAX;

    while ((MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1U) && (u32Wait > 0UL))
    {
        u32Wait--;
    }
    return (u32Wait != 0UL) ? 1U : 0U;
}

/******************************************************************************
 * @brief  设置某个 CLA 任务的触发源
 * @param  u16TaskNo  任务号
 * @param  u16TrigSrc 触发源编码
 * @note   TASK1~TASK4 在 CLA1TASKSRCSEL1，TASK5~TASK8 在 CLA1TASKSRCSEL2，
 *         每个任务占 8 位
 ******************************************************************************/
static void Hw_ClaSetTriggerSrc(Uint16 u16TaskNo, Uint16 u16TrigSrc)
{
    Uint32 u32Shift = ((Uint32)u16TaskNo & 0x03UL) * 8UL;
    Uint32 u32Mask  = (Uint32)0xFFUL << u32Shift;
    Uint32 u32Val   = ((Uint32)u16TrigSrc & 0xFFUL) << u32Shift;
    volatile Uint32 *pu32Reg;

    EALLOW;

    if (u16TaskNo < 4U)
    {
        pu32Reg = &DmaClaSrcSelRegs.CLA1TASKSRCSEL1.all;
    }
    else
    {
        pu32Reg = &DmaClaSrcSelRegs.CLA1TASKSRCSEL2.all;
    }

    *pu32Reg = (*pu32Reg & ~u32Mask) | u32Val;

    EDIS;
}

/******************************************************************************
 * @brief  初始化单个 CLA 任务
 * @param  pstClaCfgItem CLA 配置项指针
 ******************************************************************************/
void Hw_ClaSingleInit(const ST_CLA_CFG_ITEM *pstClaCfgItem)
{
    Uint16 u16TaskNo;

    if (pstClaCfgItem == 0)
    {
        return;
    }

    u16TaskNo = pstClaCfgItem->u16TaskNo;

    if (Hw_ClaTaskNoIsValid(u16TaskNo) == 0U)
    {
        return;
    }

    EALLOW;

    /* 1. 映射任务入口地址（MVECT1~MVECT8 连续，可按下标访问） */
    if (pstClaCfgItem->ClaTask != 0)
    {
        (&Cla1Regs.MVECT1)[u16TaskNo] =
            (Uint16)((Uint32)pstClaCfgItem->ClaTask & 0xFFFFUL);
    }

    /* 2. 使能该任务的中断（任务结束时向 CPU 发 CLA1_x 中断） */
    Cla1Regs.MIER.all |= (Uint16)(1U << u16TaskNo);

    EDIS;

    /* 3. 设置触发源（函数内部自带 EALLOW/EDIS） */
    Hw_ClaSetTriggerSrc(u16TaskNo, pstClaCfgItem->u16TriggerSrc);
}

/******************************************************************************
 * @brief CLA 初始化总入口
 * @param pstClaCfg CLA 配置表指针
 ******************************************************************************/
Uint16 Hw_ClaInit(const ST_CLA_CFG *pstClaCfg)
{
    Uint16 i;

    if (pstClaCfg == 0)
    {
        return 0U;
    }

    /* 1. 先把程序与常量拷贝到 RAM 运行区：
     *    LSxMSEL 置 1 后该块由 CLA 做主，CPU 写不进去（拷贝会变成空操作），
     *    因此必须按 TI 官方 CLA_configClaMemory() 的顺序，先把代码搬进
     *    RAM（此时 LS 仍归 CPU），再把 LS 划给 CLA。 */
    Hw_ClaCopyToRam();

    /* 2. LS 存储器归属：LS0~LS3 程序、LS4~LS5 数据 */
    Hw_ClaMemConfig();

    /* 3. 消息 RAM 清零 */
    if (Hw_ClaMsgRamInit() == 0U) { return 0U; }

    /* 4. 逐任务映射向量、使能中断、设置触发源 */
    for (i = 0U; i < pstClaCfg->u16ItemNum; i++)
    {
        Hw_ClaSingleInit(&pstClaCfg->pstClaCfgItem[i]);
    }

    /* 5. 允许 IACK 指令软件触发任务 */
    EALLOW;
    Cla1Regs.MCTL.bit.IACKE = 1U;
    EDIS;
    return 1U;
}

/******************************************************************************
 * @brief 软件强制触发一个 CLA 任务
 * @param u16TaskNo 任务号 (HW_CLA_TASK1 ~ HW_CLA_TASK8)
 * @note  通过 MIFRC 强制中断标志实现，比 IACK 指令更便于参数化调用
 ******************************************************************************/
void Hw_ClaTaskForce(Uint16 u16TaskNo)
{
    if (Hw_ClaTaskNoIsValid(u16TaskNo) == 0U)
    {
        return;
    }

    EALLOW;
    Cla1Regs.MIFRC.all = (Uint16)(1U << u16TaskNo);
    EDIS;
}

/******************************************************************************
 * @brief 读取 CLA 任务溢出标志
 * @param u16TaskNo 任务号
 * @return 1：发生过溢出（上次任务未跑完又被触发）；0：正常
 ******************************************************************************/
Uint16 Hw_ClaGetOverflowFlag(Uint16 u16TaskNo)
{
    Uint16 u16Flag;

    if (Hw_ClaTaskNoIsValid(u16TaskNo) == 0U)
    {
        return 0U;
    }

    u16Flag = (Cla1Regs.MIOVF.all >> u16TaskNo) & 0x0001U;

    return u16Flag;
}

#ifdef __cplusplus
}
#endif

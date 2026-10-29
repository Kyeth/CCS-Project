/**
 * @file Hw_CanDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS CAN驱动源文件
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Hw_CanDriver.h"

/* CAN模块时钟源为SYSCLK，与device.h的DEVICE_SYSCLK_FREQ一致 */
#define HW_CAN_SYSCLK_HZ                (200000000UL)

/* 位时间按20个Tq规划：同步段1 + TSEG1(14) + TSEG2(5)，采样点75% */
#define HW_CAN_TQ_PER_BIT               (20UL)
#define HW_CAN_TSEG1                    (13U)
#define HW_CAN_TSEG2                    (4U)
#define HW_CAN_SJW                      (3U)

/* CAN_CTL各控制位的掩码 */
#define HW_CAN_CTL_INIT                 (1UL << 0U)
#define HW_CAN_CTL_CCE                  (1UL << 6U)
#define HW_CAN_CTL_TEST                 (1UL << 7U)
#define HW_CAN_CTL_ABO                  (1UL << 9U)
#define HW_CAN_CTL_SWR                  (1UL << 15U)

/* 关闭奇偶校验的KEY值，PMD字段写5 */
#define HW_CAN_CTL_PARITY_DISABLE       (0x00001400UL)

/* CAN_RAM_INIT：KEY=0xA且bit4=1请求初始化RAM，bit5为完成标志 */
#define HW_CAN_RAM_INIT_KEY             (0x0AUL)
#define HW_CAN_RAM_INIT_REQ             (1UL << 4U)
#define HW_CAN_RAM_INIT_DONE            (1UL << 5U)
#define HW_CAN_RAM_INIT_KEY_DONE        (0x0005UL)
#define HW_CAN_RAM_INIT_MASK            (0x003FUL)

/* IFxCMD的访问位 */
#define HW_CAN_CMD_DATA_B               (1UL << 16U)
#define HW_CAN_CMD_DATA_A               (1UL << 17U)
#define HW_CAN_CMD_TXRQST               (1UL << 18U)
#define HW_CAN_CMD_CONTROL              (1UL << 20U)
#define HW_CAN_CMD_ARB                  (1UL << 21U)
#define HW_CAN_CMD_MASK                 (1UL << 22U)
#define HW_CAN_CMD_DIR                  (1UL << 23U)
#define HW_CAN_CMD_BUSY                 (1UL << 15U)
#define HW_CAN_CMD_MSGNUM_M             (0x00FFUL)

/* IFxARB的Dir/Xtd/MsgVal位 */
#define HW_CAN_ARB_DIR                  (1UL << 29U)
#define HW_CAN_ARB_XTD                  (1UL << 30U)
#define HW_CAN_ARB_MSGVAL               (1UL << 31U)
#define HW_CAN_ID_MASK                  (0x1FFFFFFFUL)

/* IFxMCTL的EoB/TxRqst/UMask/NewDat位 */
#define HW_CAN_MCTL_DLC_M               (0x000FUL)
#define HW_CAN_MCTL_EOB                 (1UL << 7U)
#define HW_CAN_MCTL_TXRQST              (1UL << 8U)
#define HW_CAN_MCTL_UMASK               (1UL << 12U)
#define HW_CAN_MCTL_NEWDAT              (1UL << 15U)

/* CAN_ES的总线关闭标志（bit7） */
#define HW_CAN_ES_BOFF                  (0x0080UL)

/* 报文对象编号范围，F2837xS固定1~32 */
#define HW_CAN_OBJ_NUM_MIN              (1U)

/* 轮询上限，防止模块异常时死等 */
#define HW_CAN_POLL_TIMEOUT             (200000UL)

/* 外设时钟使能后的稳定延时计数 */
#define HW_CAN_CLOCK_DELAY_CNT          (16U)

/* CAN寄存器基址表 */
static volatile struct CAN_REGS * const gCanRegs[HW_CAN_ID_MAX] =
{
    &CanaRegs,
    &CanbRegs
};

/* 保存每个模块的配置项，发送/接收时按它换算报文对象编号 */
static ST_CAN_CFG_ITEM gCanCfg[HW_CAN_ID_MAX];
static Uint16 gCanInited[HW_CAN_ID_MAX] = {0U, 0U};

/* 取模块寄存器指针，编号非法返回空指针 */
static volatile struct CAN_REGS *Hw_CanGetRegs(Uint16 u16CanId)
{
    if (u16CanId >= (Uint16)HW_CAN_ID_MAX)
    {
        return (volatile struct CAN_REGS *)0;
    }

    return gCanRegs[u16CanId];
}

/* 毫秒级空循环延时，用于外设时钟打开后与软复位后的稳定等待 */
static void Hw_CanDelayLoop(Uint16 u16Cnt)
{
    volatile Uint16 u16Delay;

    for (u16Delay = 0U; u16Delay < u16Cnt; u16Delay++)
    {
        __asm(" NOP");
    }
}

/* 使能CAN模块时钟，PCLKCR10的bit0/bit1对应CANA/CANB */
static void Hw_CanEnableClock(const ST_CAN_CFG_ITEM *pstCanCfgItem)
{
    EALLOW;
    CpuSysRegs.PCLKCR10.all |= (Uint16)(1U << pstCanCfgItem->u16CanId);
    EDIS;

    Hw_CanDelayLoop(HW_CAN_CLOCK_DELAY_CNT);
}

/* 等待IF1的Busy位清零，超时返回0 */
static Uint16 Hw_CanWaitIf1Idle(volatile struct CAN_REGS *pstRegs)
{
    Uint32 u32Wait = 0UL;

    while ((pstRegs->CAN_IF1CMD.all & HW_CAN_CMD_BUSY) != 0UL)
    {
        u32Wait++;
        if (u32Wait > HW_CAN_POLL_TIMEOUT)
        {
            return 0U;
        }
    }

    return 1U;
}

/* 等待IF2的Busy位清零，超时返回0 */
static Uint16 Hw_CanWaitIf2Idle(volatile struct CAN_REGS *pstRegs)
{
    Uint32 u32Wait = 0UL;

    while ((pstRegs->CAN_IF2CMD.all & HW_CAN_CMD_BUSY) != 0UL)
    {
        u32Wait++;
        if (u32Wait > HW_CAN_POLL_TIMEOUT)
        {
            return 0U;
        }
    }

    return 1U;
}

/* 按SYSCLK和波特率写CAN_BTR，位时间固定20个Tq */
static Uint16 Hw_CanConfigBitTiming(volatile struct CAN_REGS *pstRegs, Uint32 u32Baudrate)
{
    Uint32 u32Divider;
    Uint16 u16Brp;
    Uint16 u16Brpe;

    if (u32Baudrate == 0UL)
    {
        return 0U;
    }

    /* 分频值 = SYSCLK / (波特率 * 每位Tq数)，BRP字段填分频值减1 */
    u32Divider = HW_CAN_SYSCLK_HZ / (u32Baudrate * HW_CAN_TQ_PER_BIT);
    if (u32Divider == 0UL)
    {
        return 0U;
    }

    u32Divider = u32Divider - 1UL;
    u16Brp = (Uint16)(u32Divider & 0x003FUL);
    u16Brpe = (Uint16)((u32Divider >> 6U) & 0x000FUL);

    pstRegs->CAN_BTR.all = 0UL;
    pstRegs->CAN_BTR.bit.BRP = u16Brp;
    pstRegs->CAN_BTR.bit.BRPE = u16Brpe;
    pstRegs->CAN_BTR.bit.SJW = HW_CAN_SJW;
    pstRegs->CAN_BTR.bit.TSEG1 = HW_CAN_TSEG1;
    pstRegs->CAN_BTR.bit.TSEG2 = HW_CAN_TSEG2;

    return 1U;
}

/* 通过IF1配置一个报文对象
 * bTx=1为发送对象，=0为接收对象；u32Mask为验收屏蔽 */
static void Hw_CanConfigObj(volatile struct CAN_REGS *pstRegs, Uint16 u16ObjNum,
                            Uint32 u32MsgId, Uint32 u32Mask,
                            Uint16 bTx, Uint16 bExtId, Uint16 u16Dlc,
                            Uint16 bUseMask)
{
    Uint32 u32Cmd;
    Uint32 u32Arb;
    Uint32 u32Mctl;
    Uint32 u32Msk = 0UL;

    if ((u16ObjNum < HW_CAN_OBJ_NUM_MIN) || (u16ObjNum > HW_CAN_OBJ_MAX))
    {
        return;
    }

    if (Hw_CanWaitIf1Idle(pstRegs) == 0U)
    {
        return;
    }

    /* 验收屏蔽：Msk每一位为1表示该位参与比较 */
    if (bUseMask != 0U)
    {
        u32Msk = (bExtId != 0U) ? (u32Mask & HW_CAN_ID_MASK)
                                  : ((u32Mask & 0x7FFUL) << 18U);
        if (bExtId != 0U)
        {
            u32Msk |= HW_CAN_ARB_XTD;   /* MXtd：扩展帧位也参与比较 */
        }
    }

    /* 仲裁寄存器：标识符、方向、帧类型、对象有效 */
    u32Arb = (bExtId != 0U) ? (u32MsgId & HW_CAN_ID_MASK)
                                : ((u32MsgId & 0x7FFUL) << 18U);
    if (bExtId != 0U)
    {
        u32Arb |= HW_CAN_ARB_XTD;
    }
    if (bTx != 0U)
    {
        u32Arb |= HW_CAN_ARB_DIR;
    }
    u32Arb |= HW_CAN_ARB_MSGVAL;

    /* 报文控制：DLC、单块结束；收发中断都先不开，全部轮询 */
    u32Mctl = (Uint32)(u16Dlc & HW_CAN_MCTL_DLC_M);
    u32Mctl |= HW_CAN_MCTL_EOB;
    if (bUseMask != 0U)
    {
        u32Mctl |= HW_CAN_MCTL_UMASK;
    }

    pstRegs->CAN_IF1MSK.all = u32Msk;
    pstRegs->CAN_IF1ARB.all = u32Arb;
    pstRegs->CAN_IF1MCTL.all = u32Mctl;

    /* DIR=1表示把IF1内容写入报文对象 */
    u32Cmd = HW_CAN_CMD_DIR | HW_CAN_CMD_ARB | HW_CAN_CMD_CONTROL | HW_CAN_CMD_MASK |
             (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    pstRegs->CAN_IF1CMD.all = u32Cmd;

    (void)Hw_CanWaitIf1Idle(pstRegs);
}

/* 进入初始化：置INIT、初始化报文RAM、软复位、打开配置写使能 */
static Uint16 Hw_CanEnterInit(volatile struct CAN_REGS *pstRegs)
{
    Uint32 u32Wait = 0UL;

    /* 先置INIT并关闭奇偶校验（PMD=5的KEY值） */
    pstRegs->CAN_CTL.all |= (HW_CAN_CTL_INIT | HW_CAN_CTL_PARITY_DISABLE);

    /* 报文RAM初始化：写KEY=0xA并置bit4，等bit5完成 */
    pstRegs->CAN_RAM_INIT.all = HW_CAN_RAM_INIT_REQ | HW_CAN_RAM_INIT_KEY;
    while ((pstRegs->CAN_RAM_INIT.all & HW_CAN_RAM_INIT_MASK) !=
           (HW_CAN_RAM_INIT_DONE | HW_CAN_RAM_INIT_KEY_DONE))
    {
        u32Wait++;
        if (u32Wait > HW_CAN_POLL_TIMEOUT)
        {
            return 0U;
        }
    }

    /* 软复位把模块恢复到复位状态，之后才能改配置 */
    pstRegs->CAN_CTL.all |= HW_CAN_CTL_SWR;
    Hw_CanDelayLoop(HW_CAN_CLOCK_DELAY_CNT);

    /* 配置写使能 */
    pstRegs->CAN_CTL.all |= HW_CAN_CTL_CCE;

    return 1U;
}

/* 退出初始化，模块开始收发 */
static Uint16 Hw_CanExitInit(volatile struct CAN_REGS *pstRegs)
{
    Uint32 u32Wait = 0UL;

    pstRegs->CAN_CTL.all &= ~(HW_CAN_CTL_CCE | HW_CAN_CTL_INIT);

    while ((pstRegs->CAN_CTL.all & HW_CAN_CTL_INIT) != 0UL)
    {
        u32Wait++;
        if (u32Wait > HW_CAN_POLL_TIMEOUT)
        {
            return 0U;
        }
    }

    return 1U;
}

void Hw_CanSingleInit(const ST_CAN_CFG_ITEM *pstCanCfgItem)
{
    volatile struct CAN_REGS *pstRegs;
    Uint16 u16Index;

    if (pstCanCfgItem == (const ST_CAN_CFG_ITEM *)0)
    {
        return;
    }

    pstRegs = Hw_CanGetRegs(pstCanCfgItem->u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return;
    }

    Hw_CanEnableClock(pstCanCfgItem);

    if (Hw_CanEnterInit(pstRegs) == 0U)
    {
        return;
    }

    (void)Hw_CanConfigBitTiming(pstRegs, pstCanCfgItem->u32Baudrate);

    /* 自动总线恢复：总线关闭后自动尝试重新接入 */
    if (pstCanCfgItem->bAutoBusOn != 0U)
    {
        pstRegs->CAN_CTL.all |= HW_CAN_CTL_ABO;
    }
    else
    {
        pstRegs->CAN_CTL.all &= ~HW_CAN_CTL_ABO;
    }

    /* 回环自测模式 */
    if (pstCanCfgItem->bLoopback != 0U)
    {
        pstRegs->CAN_CTL.all |= HW_CAN_CTL_TEST;
        pstRegs->CAN_TEST.bit.LBACK = 1U;
    }

    /* 发送对象：DLC先填0，发送时按实际长度重写 */
    for (u16Index = 0U; u16Index < pstCanCfgItem->u16TxObjCnt; u16Index++)
    {
        Hw_CanConfigObj(pstRegs,
                        (Uint16)(pstCanCfgItem->u16TxObjNum + u16Index),
                        0UL, 0UL, 1U, pstCanCfgItem->bExtId, 0U, 0U);
    }

    /* 接收对象：按配置的验收ID与屏蔽字过滤 */
    for (u16Index = 0U; u16Index < pstCanCfgItem->u16RxObjCnt; u16Index++)
    {
        Hw_CanConfigObj(pstRegs,
                        (Uint16)(pstCanCfgItem->u16RxObjNum + u16Index),
                        pstCanCfgItem->u32RxAcceptId,
                        pstCanCfgItem->u32RxAcceptMask,
                        0U, pstCanCfgItem->bExtId,
                        (Uint16)HW_CAN_DATA_LEN_MAX, 1U);
    }

    if (Hw_CanExitInit(pstRegs) == 0U)
    {
        return;
    }

    /* 保存配置项，后面收发时要用它换算对象编号与帧类型 */
    gCanCfg[pstCanCfgItem->u16CanId] = *pstCanCfgItem;
    gCanInited[pstCanCfgItem->u16CanId] = 1U;
}

void Hw_CanInit(const ST_CAN_CFG *pstCanCfg)
{
    Uint16 u16Index;

    if ((pstCanCfg == (const ST_CAN_CFG *)0) ||
        (pstCanCfg->pstCanCfgItem == (const ST_CAN_CFG_ITEM *)0))
    {
        return;
    }

    for (u16Index = 0U; u16Index < pstCanCfg->u16ItemNum; u16Index++)
    {
        Hw_CanSingleInit(&pstCanCfg->pstCanCfgItem[u16Index]);
    }
}

Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len)
{
    volatile struct CAN_REGS *pstRegs;
    Uint32 u32Cmd;
    Uint32 u32Arb;
    Uint32 u32DataA = 0UL;
    Uint32 u32DataB = 0UL;
    Uint16 u16ObjNum;
    Uint16 u16Index;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    if ((gCanInited[u16CanId] == 0U) || (u16ObjIdx >= gCanCfg[u16CanId].u16TxObjCnt))
    {
        return 0U;
    }

    if (u16Len > HW_CAN_DATA_LEN_MAX)
    {
        u16Len = HW_CAN_DATA_LEN_MAX;
    }

    if (Hw_CanWaitIf1Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    u16ObjNum = (Uint16)(gCanCfg[u16CanId].u16TxObjNum + u16ObjIdx);

    if (pu16Data != (const Uint16 *)0)
    {
        for (u16Index = 0U; u16Index < u16Len; u16Index++)
        {
            if (u16Index < 4U)
            {
                u32DataA |= ((Uint32)(pu16Data[u16Index] & 0x00FFU) << (8U * u16Index));
            }
            else
            {
                u32DataB |= ((Uint32)(pu16Data[u16Index] & 0x00FFU) << (8U * (u16Index - 4U)));
            }
        }
    }

    pstRegs->CAN_IF1DATA.all = u32DataA;
    pstRegs->CAN_IF1DATB.all = u32DataB;

    /* 重写仲裁与控制寄存器：标识符、帧类型按本次实参，方向仍为发送 */
    u32Arb = ((gCanCfg[u16CanId].bExtId != 0U) ?
              (u32MsgId & HW_CAN_ID_MASK) : ((u32MsgId & 0x7FFUL) << 18U)) |
             HW_CAN_ARB_DIR | HW_CAN_ARB_MSGVAL;
    if (gCanCfg[u16CanId].bExtId != 0U)
    {
        u32Arb |= HW_CAN_ARB_XTD;
    }
    pstRegs->CAN_IF1ARB.all = u32Arb;
    pstRegs->CAN_IF1MCTL.all = (Uint32)(u16Len & HW_CAN_MCTL_DLC_M) | HW_CAN_MCTL_EOB;

    u32Cmd = HW_CAN_CMD_DIR | HW_CAN_CMD_ARB | HW_CAN_CMD_CONTROL |
             HW_CAN_CMD_TXRQST | (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    if (u16Len > 0U)
    {
        u32Cmd |= (HW_CAN_CMD_DATA_A | HW_CAN_CMD_DATA_B);
    }

    pstRegs->CAN_IF1CMD.all = u32Cmd;

    if (Hw_CanWaitIf1Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    return 1U;
}

Uint16 Hw_CanIsTxDone(Uint16 u16CanId, Uint16 u16ObjIdx)
{
    volatile struct CAN_REGS *pstRegs;
    Uint16 u16ObjNum;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    if ((gCanInited[u16CanId] == 0U) || (u16ObjIdx >= gCanCfg[u16CanId].u16TxObjCnt))
    {
        return 0U;
    }

    u16ObjNum = (Uint16)(gCanCfg[u16CanId].u16TxObjNum + u16ObjIdx);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    /* 只读控制寄存器即可判断TxRqst是否已由硬件清零 */
    pstRegs->CAN_IF2CMD.all = HW_CAN_CMD_CONTROL |
                              (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    return (((pstRegs->CAN_IF2MCTL.all & HW_CAN_MCTL_TXRQST) == 0UL) ? 1U : 0U);
}

Uint16 Hw_CanIsRxReady(Uint16 u16CanId, Uint16 u16ObjIdx)
{
    volatile struct CAN_REGS *pstRegs;
    Uint16 u16ObjNum;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    if ((gCanInited[u16CanId] == 0U) || (u16ObjIdx >= gCanCfg[u16CanId].u16RxObjCnt))
    {
        return 0U;
    }

    u16ObjNum = (Uint16)(gCanCfg[u16CanId].u16RxObjNum + u16ObjIdx);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    pstRegs->CAN_IF2CMD.all = HW_CAN_CMD_CONTROL |
                              (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    return (((pstRegs->CAN_IF2MCTL.all & HW_CAN_MCTL_NEWDAT) != 0UL) ? 1U : 0U);
}

Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len)
{
    volatile struct CAN_REGS *pstRegs;
    Uint16 u16ObjNum;
    Uint32 u32Mctl;
    Uint32 u32Arb;
    Uint32 u32DataA;
    Uint32 u32DataB;
    Uint16 u16Len;
    Uint16 u16Index;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    if ((gCanInited[u16CanId] == 0U) || (u16ObjIdx >= gCanCfg[u16CanId].u16RxObjCnt))
    {
        return 0U;
    }

    u16ObjNum = (Uint16)(gCanCfg[u16CanId].u16RxObjNum + u16ObjIdx);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    /* 一次把仲裁、控制、数据全部搬到IF2 */
    pstRegs->CAN_IF2CMD.all = HW_CAN_CMD_ARB | HW_CAN_CMD_CONTROL |
                              HW_CAN_CMD_DATA_A | HW_CAN_CMD_DATA_B |
                              (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    if (Hw_CanWaitIf2Idle(pstRegs) == 0U)
    {
        return 0U;
    }

    u32Mctl = pstRegs->CAN_IF2MCTL.all;

    /* NewDat为0说明没有新帧 */
    if ((u32Mctl & HW_CAN_MCTL_NEWDAT) == 0UL)
    {
        return 0U;
    }

    u32Arb = pstRegs->CAN_IF2ARB.all;
    u32DataA = pstRegs->CAN_IF2DATA.all;
    u32DataB = pstRegs->CAN_IF2DATB.all;

    u16Len = (Uint16)(u32Mctl & HW_CAN_MCTL_DLC_M);
    if (u16Len > HW_CAN_DATA_LEN_MAX)
    {
        u16Len = HW_CAN_DATA_LEN_MAX;
    }

    if (pu32MsgId != (Uint32 *)0)
    {
        *pu32MsgId = ((u32Arb & HW_CAN_ARB_XTD) != 0UL) ?
                       (u32Arb & HW_CAN_ID_MASK) : ((u32Arb >> 18U) & 0x7FFUL);
    }

    if (pu16Data != (Uint16 *)0)
    {
        for (u16Index = 0U; u16Index < u16Len; u16Index++)
        {
            if (u16Index < 4U)
            {
                pu16Data[u16Index] = (Uint16)((u32DataA >> (8U * u16Index)) & 0x00FFUL);
            }
            else
            {
                pu16Data[u16Index] = (Uint16)((u32DataB >> (8U * (u16Index - 4U))) & 0x00FFUL);
            }
        }
    }

    if (pu16Len != (Uint16 *)0)
    {
        *pu16Len = u16Len;
    }

    /* 清NewDat：向该对象写TXRQST=1即可同时清掉TxRqst与NewDat */
    pstRegs->CAN_IF2CMD.all = HW_CAN_CMD_TXRQST |
                              (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);

    (void)Hw_CanWaitIf2Idle(pstRegs);

    return 1U;
}

Uint16 Hw_CanGetStatus(Uint16 u16CanId)
{
    volatile struct CAN_REGS *pstRegs;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    return (Uint16)(pstRegs->CAN_ES.all & 0xFFFFUL);
}

Uint16 Hw_CanIsBusOff(Uint16 u16CanId)
{
    volatile struct CAN_REGS *pstRegs;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    return (((pstRegs->CAN_ES.all & HW_CAN_ES_BOFF) != 0UL) ? 1U : 0U);
}

Uint16 Hw_CanRecoverBusOff(Uint16 u16CanId)
{
    volatile struct CAN_REGS *pstRegs;
    Uint32 u32Wait = 0UL;

    pstRegs = Hw_CanGetRegs(u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    /* 总线关闭后置INIT再清INIT即可重新接入总线
     * 注意：这里不能重做报文RAM初始化，否则报文对象配置会被全部清掉 */
    pstRegs->CAN_CTL.all |= HW_CAN_CTL_INIT;

    while ((pstRegs->CAN_CTL.all & HW_CAN_CTL_INIT) == 0UL)
    {
        u32Wait++;
        if (u32Wait > HW_CAN_POLL_TIMEOUT)
        {
            return 0U;
        }
    }

    return Hw_CanExitInit(pstRegs);
}

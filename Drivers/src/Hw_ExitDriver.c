/**
 * @file Hw_ExitDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS 外部中断（XINT1~XINT5）驱动实现
 * @version 0.1
 * @date 2026-10-23
 *
 * @copyright Copyright (c) 2026
 *
 * @note 1. XINTnCR 只有 ENABLE 和 POLARITY 两个有效字段，没有独立的中断标志位，
 *          中断标志在 PIEIFR 里，PIEACK 由调用方负责；
 *       2. 事件计数用“自由运行 16 位累加 + 消费端记上次读到的值”的方式，
 *          中断里只做 ++，任务里做无符号差值，天然处理回绕，不需要临界区；
 *       3. 32 位总计数只在中断里加，任务里读，读的时候可能被中断打断，
 *          但只是诊断量，允许末尾丢一拍。
 */

#include "Hw_ExitDriver.h"

/* 累计事件数：中断里 ++，任务里读 */
static volatile Uint16 gXintEvtAcc[HW_XINT_NUM];

/* 累计事件总数：中断里 ++，只做诊断 */
static volatile Uint32 gXintEvtTotal[HW_XINT_NUM];

/* 消费端上次读到的累加值 */
static Uint16 gXintEvtLast[HW_XINT_NUM];

/* 单独使能/关闭一路 XINT。 */
void Hw_ExitEnable(Uint16 u16XintId, Uint16 u16Enable)
{
    switch (u16XintId)
    {
        case HW_XINT_ID1:
            XintRegs.XINT1CR.bit.ENABLE = (u16Enable != 0U) ? 1U : 0U;
            break;

        case HW_XINT_ID2:
            XintRegs.XINT2CR.bit.ENABLE = (u16Enable != 0U) ? 1U : 0U;
            break;

        case HW_XINT_ID3:
            XintRegs.XINT3CR.bit.ENABLE = (u16Enable != 0U) ? 1U : 0U;
            break;

        case HW_XINT_ID4:
            XintRegs.XINT4CR.bit.ENABLE = (u16Enable != 0U) ? 1U : 0U;
            break;

        case HW_XINT_ID5:
            XintRegs.XINT5CR.bit.ENABLE = (u16Enable != 0U) ? 1U : 0U;
            break;

        default:
            break;
    }
}

/* 按配置表初始化 XINT1~XINT5。 */
void Hw_ExitInit(const ST_EXIT_CFG *pstExitCfg)
{
    Uint16 i;
    Uint16 u16XintId;
    Uint16 u16Edge;

    if (pstExitCfg == (const ST_EXIT_CFG *)0)
    {
        return;
    }

    EALLOW;

    for (i = 0U; i < pstExitCfg->u16ItemNum; i++)
    {
        u16XintId = pstExitCfg->pstExitCfgItem[i].u16XintId;
        u16Edge   = pstExitCfg->pstExitCfgItem[i].u16Edge;

        if (u16XintId >= HW_XINT_NUM)
        {
            continue;
        }

        /* 先关使能，配好极性再打开，避免配置过程中进一次假中断 */
        Hw_ExitEnable(u16XintId, 0U);

        switch (u16XintId)
        {
            case HW_XINT_ID1:
                XintRegs.XINT1CR.bit.POLARITY = u16Edge;
                break;

            case HW_XINT_ID2:
                XintRegs.XINT2CR.bit.POLARITY = u16Edge;
                break;

            case HW_XINT_ID3:
                XintRegs.XINT3CR.bit.POLARITY = u16Edge;
                break;

            case HW_XINT_ID4:
                XintRegs.XINT4CR.bit.POLARITY = u16Edge;
                break;

            case HW_XINT_ID5:
                XintRegs.XINT5CR.bit.POLARITY = u16Edge;
                break;

            default:
                break;
        }

        Hw_ExitEnable(u16XintId, pstExitCfg->pstExitCfgItem[i].bEnable);
    }

    EDIS;
}

/* 中断服务里调用：累加事件计数。 */
void Hw_ExitIsr(Uint16 u16XintId)
{
    if (u16XintId >= HW_XINT_NUM)
    {
        return;
    }

    gXintEvtAcc[u16XintId]++;
    gXintEvtTotal[u16XintId]++;
}

/* 取走自上次调用以来新增的事件数。 */
Uint16 Hw_ExitTakeCount(Uint16 u16XintId)
{
    Uint16 u16Delta;

    if (u16XintId >= HW_XINT_NUM)
    {
        return 0U;
    }

    /* 无符号差值：累加值回绕时依然正确 */
    u16Delta = (Uint16)(gXintEvtAcc[u16XintId] - gXintEvtLast[u16XintId]);
    gXintEvtLast[u16XintId] = gXintEvtAcc[u16XintId];

    return u16Delta;
}

/* 查询累计事件总数。 */
Uint32 Hw_ExitGetTotal(Uint16 u16XintId)
{
    if (u16XintId >= HW_XINT_NUM)
    {
        return 0UL;
    }

    return gXintEvtTotal[u16XintId];
}

/* 清零累计计数。 */
void Hw_ExitClearAll(void)
{
    Uint16 i;

    DINT;

    for (i = 0U; i < HW_XINT_NUM; i++)
    {
        gXintEvtAcc[i]   = 0U;
        gXintEvtTotal[i] = 0UL;
        gXintEvtLast[i]  = 0U;
    }

    EINT;
}

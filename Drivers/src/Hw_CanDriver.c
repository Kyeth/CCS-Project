/**
 * @file Hw_CanDriver.c
 * @author yetingheng (004182)
 * @brief F2837xS CAN驱动源文件
 * @version 0.2
 * @date 2026-11-03
 *
 * @copyright Copyright (c) 2026
 *
 * 说明：
 *  - 位时序、对象配置、收发事务全部带时间预算，预算由 Hw_TimeBase 提供，不再用循环次数。
 *  - IF1 只走发送路径，IF2 只走接收路径；忙状态与完成状态改用 CAN_TXRQ_21/CAN_NDAT_21 查询，不占 IF。
 *  - 提交结果不确定时置未决标志，只有 Hw_CanRecoverModule() 能清除，驱动不自动重发。
 */

#include "Hw_CanDriver.h"
#include "Hw_TimeBase.h"

/* 位时间按20个Tq规划：同步段1 + TSEG1(14) + TSEG2(5)，采样点75% */
#define HW_CAN_TQ_PER_BIT               (20UL)
#define HW_CAN_TSEG1                    (13U)
#define HW_CAN_TSEG2                    (4U)
#define HW_CAN_SJW                      (3U)

/* 波特率预分频取值范围：总分频值 1~1024 */
#define HW_CAN_PRESCALER_MIN            (1UL)
#define HW_CAN_PRESCALER_MAX            (1024UL)

/* 允许的波特率误差：0.5% = 1/200；用"差值 > 目标/200"判断，避免任何乘法溢出 */
#define HW_CAN_BAUD_ERR_DIVISOR         (200UL)

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

/* IFxCMD的访问位（bits[23:16]） */
#define HW_CAN_CMD_DATA_B               (1UL << 16U)
#define HW_CAN_CMD_DATA_A               (1UL << 17U)
#define HW_CAN_CMD_TXRQST               (1UL << 18U)
#define HW_CAN_CMD_CONTROL              (1UL << 20U)
#define HW_CAN_CMD_ARB                  (1UL << 21U)
#define HW_CAN_CMD_MASK                 (1UL << 22U)
#define HW_CAN_CMD_DIR                  (1UL << 23U)
#define HW_CAN_CMD_BUSY                 (1UL << 15U)
#define HW_CAN_CMD_MSGNUM_M             (0x00FFUL)

/* 命令组合（写 .all 时这些位已经位于 bits[23:16]）
 * 读全部并清NewDat = 0x37；写发送对象 = 0xB4（有数据再或 DATA_A/DATA_B 得 0xB7）；
 * 写接收对象 = 0x74（ARB+CONTROL+MASK+DIR 传输） */
#define HW_CAN_CMD_READ_ALL             (HW_CAN_CMD_TXRQST | HW_CAN_CMD_ARB | \
                                         HW_CAN_CMD_CONTROL | HW_CAN_CMD_DATA_A | HW_CAN_CMD_DATA_B)
#define HW_CAN_CMD_WRITE_TX             (HW_CAN_CMD_DIR | HW_CAN_CMD_ARB | \
                                         HW_CAN_CMD_CONTROL | HW_CAN_CMD_TXRQST)
#define HW_CAN_CMD_WRITE_RX             (HW_CAN_CMD_DIR | HW_CAN_CMD_ARB | \
                                         HW_CAN_CMD_CONTROL | HW_CAN_CMD_MASK)

/* IFxMSK：Msk[28:0] + MDir(30) + MXtd(31)，bit29保留 */
#define HW_CAN_MSK_MDIR                 (1UL << 30U)
#define HW_CAN_MSK_MXTD                 (1UL << 31U)
#define HW_CAN_MSK_STD_M                (0x000007FFUL)

/* IFxARB的Dir/Xtd/MsgVal位 */
#define HW_CAN_ARB_DIR                  (1UL << 29U)
#define HW_CAN_ARB_XTD                  (1UL << 30U)
#define HW_CAN_ARB_MSGVAL               (1UL << 31U)
#define HW_CAN_ID_MASK                  (0x1FFFFFFFUL)

/* IFxMCTL的EoB/TxRqst/UMask/IntPnd/MsgLst/NewDat位 */
#define HW_CAN_MCTL_DLC_M               (0x000FUL)
#define HW_CAN_MCTL_EOB                 (1UL << 7U)
#define HW_CAN_MCTL_TXRQST              (1UL << 8U)
#define HW_CAN_MCTL_UMASK               (1UL << 12U)
#define HW_CAN_MCTL_MSGLST              (1UL << 14U)
#define HW_CAN_MCTL_NEWDAT              (1UL << 15U)

/* CAN_ES的总线关闭标志（bit7） */
#define HW_CAN_ES_BOFF                  (0x0080UL)

/* 报文对象编号范围，F2837xS固定1~32 */
#define HW_CAN_OBJ_NUM_MIN              (1U)

/* 时间预算：运行期单个API 20us、单次IF等待 10us；初始化阶段 10ms/阶段、全程 50ms */
#define HW_CAN_API_BUDGET_US            (20UL)
#define HW_CAN_STAGE_BUDGET_US          (10UL)
#define HW_CAN_INIT_OBJ_BUDGET_US       (1000UL)
#define HW_CAN_INIT_STAGE_BUDGET_US     (10000UL)
#define HW_CAN_INIT_TOTAL_BUDGET_US     (50000UL)

/* 等待循环的硬兜底次数：仅在计数被外部停住时避免死等，不参与预算判定 */
#define HW_CAN_WAIT_SPIN_MAX            (1000000UL)

/* 外设时钟使能后的稳定延时计数 */
#define HW_CAN_CLOCK_DELAY_CNT          (16U)

/* 未决标志：分别记录发送/接收路径上一次事务结果不确定 */
#define HW_CAN_PENDING_TX               (0x0001U)
#define HW_CAN_PENDING_RX               (0x0002U)

/* CAN寄存器基址表 */
static volatile struct CAN_REGS * const gCanRegs[HW_CAN_ID_MAX] =
{
    &CanaRegs,
    &CanbRegs
};

/* 保存每个模块的配置项，发送/接收时按它换算报文对象编号 */
static ST_CAN_CFG_ITEM gCanCfg[HW_CAN_ID_MAX];
static Uint16 gCanCfgValid[HW_CAN_ID_MAX] = {0U, 0U};
static Uint16 gCanInited[HW_CAN_ID_MAX] = {0U, 0U};
static Uint16 gCanPending[HW_CAN_ID_MAX] = {0U, 0U};
static Uint16 gCanRecovering[HW_CAN_ID_MAX] = {0U, 0U};
static Uint32 gCanSysclkHz = 0UL;

/* 取模块寄存器指针，编号非法返回空指针 */
static volatile struct CAN_REGS *Hw_CanGetRegs(Uint16 u16CanId)
{
    if (u16CanId >= (Uint16)HW_CAN_ID_MAX)
    {
        return (volatile struct CAN_REGS *)0;
    }

    return gCanRegs[u16CanId];
}

/* 空循环延时，用于外设时钟打开后与软复位后的稳定等待（非计时用途） */
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

/* 预算判定：API 起点与阶段起点各自独立计时，任一超时即失败；时间源失效按超时处理 */
static Uint16 Hw_CanBudgetOk(Uint32 u32ApiStart, Uint32 u32ApiLimit)
{
    Uint32 u32Elapsed;

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    /* 本次读取若失败，故障标志会立刻清零就绪状态，按预算耗尽处理 */
    u32Elapsed = Hw_TimeBaseElapsed(u32ApiStart);

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    return ((u32Elapsed < u32ApiLimit) ? 1U : 0U);
}

static Uint16 Hw_CanStageOk(Uint32 u32ApiStart,
                            Uint32 u32ApiLimit,
                            Uint32 u32StageStart,
                            Uint32 u32StageLimit)
{
    Uint32 u32Now;

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    u32Now = Hw_TimeBaseTicks();

    /* 读取失败会清除就绪标志，必须在使用时间值前复查 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    if ((Uint32)(u32ApiStart - u32Now) >= u32ApiLimit)
    {
        return 0U;
    }

    if ((Uint32)(u32StageStart - u32Now) >= u32StageLimit)
    {
        return 0U;
    }

    return 1U;
}

/**
 * @brief 等待 IF1 空闲
 *
 * 说明：C28x 不允许对 32 位外设寄存器取地址，因此直接传寄存器结构体指针，
 *      在函数内部逐次读取 CAN_IF1CMD。预算按绝对起点计算，不做重新起算。
 */
static Uint16 Hw_CanWaitIf1Idle(volatile struct CAN_REGS *pstRegs, Uint32 u32ApiStart,
                                Uint32 u32ApiLimit, Uint32 u32StageStart,
                                Uint32 u32StageLimit)
{
    Uint32 u32Spin;

    for (u32Spin = 0UL; u32Spin < HW_CAN_WAIT_SPIN_MAX; u32Spin++)
    {
        /* 先确认时间源与两条预算仍然成立，再判定空闲，避免"已空闲"掩盖超预算 */
        if (Hw_CanStageOk(u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
        {
            return 0U;
        }

        if ((pstRegs->CAN_IF1CMD.all & HW_CAN_CMD_BUSY) == 0UL)
        {
            return 1U;
        }
    }

    return 0U;
}

/**
 * @brief 与 Hw_CanWaitIf1Idle 相同，等待 IF2 空闲
 */
static Uint16 Hw_CanWaitIf2Idle(volatile struct CAN_REGS *pstRegs, Uint32 u32ApiStart,
                                Uint32 u32ApiLimit, Uint32 u32StageStart,
                                Uint32 u32StageLimit)
{
    Uint32 u32Spin;

    for (u32Spin = 0UL; u32Spin < HW_CAN_WAIT_SPIN_MAX; u32Spin++)
    {
        /* 先确认时间源与两条预算仍然成立，再判定空闲，避免"已空闲"掩盖超预算 */
        if (Hw_CanStageOk(u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
        {
            return 0U;
        }

        if ((pstRegs->CAN_IF2CMD.all & HW_CAN_CMD_BUSY) == 0UL)
        {
            return 1U;
        }
    }

    return 0U;
}


/**
 * @brief 校验波特率并算出总分频值
 *
 * @return 1 = 合法，0 = 非法（含溢出、超范围、误差过大）
 */
static Uint16 Hw_CanCheckBitTiming(Uint32 u32Baudrate, Uint32 u32SysclkHz, Uint32 *pu32Prescaler)
{
    Uint32 u32Den;
    Uint32 u32Div;
    Uint32 u32Actual;
    Uint32 u32Diff;

    if (u32Baudrate == 0UL)
    {
        return 0U;
    }

    /* 乘法前先挡住溢出：baud * TQ_PER_BIT 不得超过 32 位 */
    if (u32Baudrate > (0xFFFFFFFFUL / HW_CAN_TQ_PER_BIT))
    {
        return 0U;
    }

    u32Den = u32Baudrate * HW_CAN_TQ_PER_BIT;
    if (u32Den == 0UL)
    {
        return 0U;
    }

    u32Div = u32SysclkHz / u32Den;

    /* 总分频值必须落在 1~1024 */
    if ((u32Div < HW_CAN_PRESCALER_MIN) || (u32Div > HW_CAN_PRESCALER_MAX))
    {
        return 0U;
    }

    /* 实际波特率误差检查：实际值 = SYSCLK / (总分频值 * 每bit的Tq数) */
    if (u32Div > (0xFFFFFFFFUL / HW_CAN_TQ_PER_BIT))
    {
        return 0U;
    }

    u32Actual = u32SysclkHz / (u32Div * HW_CAN_TQ_PER_BIT);
    if (u32Actual > u32Baudrate)
    {
        u32Diff = u32Actual - u32Baudrate;
    }
    else
    {
        u32Diff = u32Baudrate - u32Actual;
    }

    /* 误差判据：|实际-目标| <= 目标/200（0.5%）。整式无乘法：
     * 目标/200 最大 21,474,836，差值本身也不超过目标+实际，两侧都留在 32 位内。
     * 不用 diff*1000 <= target*5 的形式，那种写法在高目标波特率时会乘法回绕。 */
    if (u32Diff > (u32Baudrate / HW_CAN_BAUD_ERR_DIVISOR))
    {
        return 0U;
    }

    *pu32Prescaler = u32Div;

    return 1U;
}

/* 按总分频值写CAN_BTR，位时间固定20个Tq */
static void Hw_CanWriteBitTiming(volatile struct CAN_REGS *pstRegs, Uint32 u32Prescaler)
{
    Uint32 u32PrescalerMinus1;

    u32PrescalerMinus1 = u32Prescaler - 1UL;

    pstRegs->CAN_BTR.all = 0UL;
    pstRegs->CAN_BTR.bit.BRP  = (Uint16)(u32PrescalerMinus1 & 0x003FUL);
    pstRegs->CAN_BTR.bit.BRPE = (Uint16)((u32PrescalerMinus1 >> 6U) & 0x000FUL);
    pstRegs->CAN_BTR.bit.SJW  = HW_CAN_SJW;
    pstRegs->CAN_BTR.bit.TSEG1 = HW_CAN_TSEG1;
    pstRegs->CAN_BTR.bit.TSEG2 = HW_CAN_TSEG2;
}

/**
 * @brief 通过IF1写一个报文对象的过滤与初始状态
 *
 * @return HW_CAN_OK / HW_CAN_BUSY / HW_CAN_UNKNOWN / HW_CAN_ERR
 */
/* 配置一个报文对象：IF1 一次整对象写事务 */
static Uint16 Hw_CanConfigObject(volatile struct CAN_REGS *pstRegs, Uint16 u16ObjNum,
                                 Uint32 u32MsgId, Uint32 u32Mask, Uint16 bTx,
                                 Uint16 bExtId, Uint16 u16Dlc, Uint16 bUseMask,
                                 Uint16 bDirCmp, Uint32 u32ApiStart,
                                 Uint32 u32ApiLimit, Uint32 u32StageLimit)
{
    Uint32 u32Msk;
    Uint32 u32Arb;
    Uint32 u32Mctl;
    Uint32 u32StageStart;

    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return HW_CAN_ERR;
    }

    if ((u16ObjNum < HW_CAN_OBJ_NUM_MIN) || (u16ObjNum > HW_CAN_OBJ_MAX))
    {
        return HW_CAN_ERR;
    }

    if (u16Dlc > HW_CAN_DATA_LEN_MAX)
    {
        return HW_CAN_ERR;
    }

    /* ID/掩码位宽按帧类型校验：超宽拒绝，不做静默截断 */
    if (bExtId != 0U)
    {
        if (((u32MsgId & ~HW_CAN_ID_MASK) != 0UL) || ((u32Mask & ~HW_CAN_ID_MASK) != 0UL))
        {
            return HW_CAN_ERR;
        }
    }
    else
    {
        if (((u32MsgId & ~HW_CAN_MSK_STD_M) != 0UL) || ((u32Mask & ~HW_CAN_MSK_STD_M) != 0UL))
        {
            return HW_CAN_ERR;
        }
    }

    u32StageStart = Hw_TimeBaseTicks();

    if (Hw_CanWaitIf1Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    /* 掩码：标准帧的验收位在 Msk[28:18]，必须左移；MXtd 始终参与比较，MDir 由调用方决定 */
    if (bExtId != 0U)
    {
        u32Msk = u32Mask & HW_CAN_ID_MASK;
    }
    else
    {
        u32Msk = (u32Mask & HW_CAN_MSK_STD_M) << 18U;
    }

    u32Msk |= HW_CAN_MSK_MXTD;
    if (bDirCmp != 0U)
    {
        u32Msk |= HW_CAN_MSK_MDIR;
    }

    /* 仲裁寄存器：ID + 方向 + 帧类型 + 有效位 */
    if (bExtId != 0U)
    {
        u32Arb = u32MsgId & HW_CAN_ID_MASK;
        u32Arb |= HW_CAN_ARB_XTD;
    }
    else
    {
        u32Arb = (u32MsgId & HW_CAN_MSK_STD_M) << 18U;
    }

    if (bTx != 0U)
    {
        u32Arb |= HW_CAN_ARB_DIR;
    }

    u32Arb |= HW_CAN_ARB_MSGVAL;

    /* 控制寄存器：DLC + EoB，接收对象再开 UMask */
    u32Mctl = (Uint32)(u16Dlc & HW_CAN_MCTL_DLC_M) | HW_CAN_MCTL_EOB;
    if (bUseMask != 0U)
    {
        u32Mctl |= HW_CAN_MCTL_UMASK;
    }

    /* 下发命令前再确认预算，避免超时后才写硬件 */
    if (Hw_CanBudgetOk(u32ApiStart, u32ApiLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    pstRegs->CAN_IF1MSK.all    = u32Msk;
    pstRegs->CAN_IF1ARB.all    = u32Arb;
    pstRegs->CAN_IF1MCTL.all   = u32Mctl;
    pstRegs->CAN_IF1CMD.all    = (HW_CAN_CMD_WRITE_RX | HW_CAN_CMD_MASK | (Uint32)u16ObjNum);

    u32StageStart = Hw_TimeBaseTicks();

    if (Hw_CanWaitIf1Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        return HW_CAN_UNKNOWN;
    }

    return HW_CAN_OK;
}

/* 进入初始化：置INIT/关闭奇偶校验/复位报文RAM；超时返回HW_CAN_BUSY */
static Uint16 Hw_CanEnterInit(volatile struct CAN_REGS *pstRegs, Uint32 u32ApiStart,
                              Uint32 u32ApiLimit, Uint32 u32StageLimit)
{
    Uint32 u32StageStart;
    Uint32 u32Spin;

    pstRegs->CAN_CTL.all |= (HW_CAN_CTL_INIT | HW_CAN_CTL_PARITY_DISABLE);

    pstRegs->CAN_RAM_INIT.all = (HW_CAN_RAM_INIT_REQ | HW_CAN_RAM_INIT_KEY);

    u32StageStart = Hw_TimeBaseTicks();

    for (u32Spin = 0UL; u32Spin < HW_CAN_WAIT_SPIN_MAX; u32Spin++)
    {
        /* 先确认时间源与两级预算，再接受硬件完成条件：不得凭"状态已满足"绕过检查 */
        if (Hw_CanStageOk(u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
        {
            return HW_CAN_BUSY;
        }

        if ((pstRegs->CAN_RAM_INIT.all & HW_CAN_RAM_INIT_MASK) ==
            (HW_CAN_RAM_INIT_DONE | HW_CAN_RAM_INIT_KEY_DONE))
        {
            break;
        }
    }

    if (u32Spin >= HW_CAN_WAIT_SPIN_MAX)
    {
        /* 硬件状态长时间不变而预算未到期：按时间源/硬件异常处理，不回落到普通超时 */
        return HW_CAN_UNKNOWN;
    }

    pstRegs->CAN_CTL.all |= HW_CAN_CTL_SWR;

    Hw_CanDelayLoop(HW_CAN_CLOCK_DELAY_CNT);

    pstRegs->CAN_CTL.all |= HW_CAN_CTL_CCE;

    return HW_CAN_OK;
}

/* 只进入初始化，不碰报文RAM：供保留对象的 Bus-Off 恢复使用 */
static Uint16 Hw_CanEnterInitPlain(volatile struct CAN_REGS *pstRegs, Uint32 u32ApiStart,
                                   Uint32 u32ApiLimit, Uint32 u32StageLimit)
{
    Uint32 u32StageStart;
    Uint32 u32Spin;

    pstRegs->CAN_CTL.all |= (HW_CAN_CTL_INIT | HW_CAN_CTL_PARITY_DISABLE);

    u32StageStart = Hw_TimeBaseTicks();

    for (u32Spin = 0UL; u32Spin < HW_CAN_WAIT_SPIN_MAX; u32Spin++)
    {
        if (Hw_CanStageOk(u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
        {
            return HW_CAN_BUSY;
        }

        if ((pstRegs->CAN_CTL.all & HW_CAN_CTL_INIT) != 0UL)
        {
            break;
        }
    }

    if (u32Spin >= HW_CAN_WAIT_SPIN_MAX)
    {
        return HW_CAN_UNKNOWN;
    }

    return HW_CAN_OK;
}

/* 退出初始化：清CCE/INIT并等待INIT真正清零 */
static Uint16 Hw_CanExitInit(volatile struct CAN_REGS *pstRegs, Uint32 u32ApiStart,
                             Uint32 u32ApiLimit, Uint32 u32StageLimit)
{
    Uint32 u32StageStart;
    Uint32 u32Spin;

    pstRegs->CAN_CTL.all &= ~(HW_CAN_CTL_CCE | HW_CAN_CTL_INIT);

    u32StageStart = Hw_TimeBaseTicks();

    for (u32Spin = 0UL; u32Spin < HW_CAN_WAIT_SPIN_MAX; u32Spin++)
    {
        if (Hw_CanStageOk(u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
        {
            return HW_CAN_BUSY;
        }

        if ((pstRegs->CAN_CTL.all & HW_CAN_CTL_INIT) == 0UL)
        {
            break;
        }
    }

    if (u32Spin >= HW_CAN_WAIT_SPIN_MAX)
    {
        return HW_CAN_UNKNOWN;
    }

    return HW_CAN_OK;
}

/* 校验一模块配置项的静态合法性，返回1合法；全部在触碰硬件之前完成 */
static Uint16 Hw_CanCheckCfgItem(const ST_CAN_CFG *pstCanCfg, Uint16 u16Index)
{
    const ST_CAN_CFG_ITEM *pstItem;
    const ST_CAN_RX_OBJ_CFG *pstRxCfg;
    Uint32 u32Prescaler;
    Uint32 u32TxEnd;
    Uint32 u32RxEnd;
    Uint16 u16Idx;

    pstItem = &pstCanCfg->pstCanCfgItem[u16Index];

    if (pstItem->u16CanId >= (Uint16)HW_CAN_ID_MAX)
    {
        return 0U;
    }

    /* 同一模块不得重复出现 */
    for (u16Idx = 0U; u16Idx < u16Index; u16Idx++)
    {
        if (pstCanCfg->pstCanCfgItem[u16Idx].u16CanId == pstItem->u16CanId)
        {
            return 0U;
        }
    }

    /* 起始号与数量分别限幅，再用 32 位算区间，杜绝 16 位回绕 */
    if ((pstItem->u16TxObjNum < HW_CAN_OBJ_NUM_MIN) || (pstItem->u16TxObjNum > HW_CAN_OBJ_MAX) ||
        (pstItem->u16TxObjCnt == 0U) || (pstItem->u16TxObjCnt > HW_CAN_OBJ_MAX) ||
        (pstItem->u16RxObjNum < HW_CAN_OBJ_NUM_MIN) || (pstItem->u16RxObjNum > HW_CAN_OBJ_MAX) ||
        (pstItem->u16RxObjCnt == 0U) || (pstItem->u16RxObjCnt > HW_CAN_OBJ_MAX))
    {
        return 0U;
    }

    u32TxEnd = (Uint32)pstItem->u16TxObjNum + (Uint32)pstItem->u16TxObjCnt - 1UL;
    u32RxEnd = (Uint32)pstItem->u16RxObjNum + (Uint32)pstItem->u16RxObjCnt - 1UL;

    if ((u32TxEnd > HW_CAN_OBJ_MAX) || (u32RxEnd > HW_CAN_OBJ_MAX))
    {
        return 0U;
    }

    /* 收发区间不得重叠 */
    if (((Uint32)pstItem->u16RxObjNum <= u32TxEnd) &&
        (u32RxEnd >= (Uint32)pstItem->u16TxObjNum))
    {
        return 0U;
    }

    pstRxCfg = pstItem->pstRxObjCfg;

    if (pstRxCfg == (const ST_CAN_RX_OBJ_CFG *)0)
    {
        return 0U;
    }

    /* 逐接收对象：ID/掩码位宽、初始DLC、方向比较契约（本步固定数据帧） */
    for (u16Idx = 0U; u16Idx < pstItem->u16RxObjCnt; u16Idx++)
    {
        if (pstRxCfg[u16Idx].bExtId != 0U)
        {
            if ((pstRxCfg[u16Idx].u32Id & ~HW_CAN_ID_MASK) != 0UL)
            {
                return 0U;
            }
            if ((pstRxCfg[u16Idx].u32Mask & ~HW_CAN_ID_MASK) != 0UL)
            {
                return 0U;
            }
        }
        else
        {
            if ((pstRxCfg[u16Idx].u32Id & ~HW_CAN_MSK_STD_M) != 0UL)
            {
                return 0U;
            }
            if ((pstRxCfg[u16Idx].u32Mask & ~HW_CAN_MSK_STD_M) != 0UL)
            {
                return 0U;
            }
        }

        if (pstRxCfg[u16Idx].u16InitDlc > HW_CAN_DATA_LEN_MAX)
        {
            return 0U;
        }

        if (pstRxCfg[u16Idx].bDirCmp == 0U)
        {
            return 0U;
        }
    }

    if (Hw_CanCheckBitTiming(pstItem->u32Baudrate, pstCanCfg->u32SysclkHz, &u32Prescaler) == 0U)
    {
        return 0U;
    }

    return 1U;
}

/* 单个模块初始化，返回1成功 */
static Uint16 Hw_CanSingleInit(const ST_CAN_CFG_ITEM *pstCanCfgItem, Uint32 u32SysclkHz)
{
    volatile struct CAN_REGS *pstRegs;
    const ST_CAN_RX_OBJ_CFG *pstRxCfg;
    Uint32 u32Prescaler;
    Uint32 u32ApiStart;
    Uint32 u32ApiLimit;
    Uint32 u32InitStageLimit;
    Uint32 u32ObjStageLimit;
    Uint16 u16Idx;
    Uint16 u16ObjNum;

    if ((pstCanCfgItem == (const ST_CAN_CFG_ITEM *)0) || (u32SysclkHz == 0UL))
    {
        return 0U;
    }

    pstRegs = Hw_CanGetRegs(pstCanCfgItem->u16CanId);
    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return 0U;
    }

    /* 时间基准未就绪时不做任何按时间等待 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    u32ApiStart       = Hw_TimeBaseTicks();
    u32ApiLimit       = Hw_TimeBaseUsToTicks(HW_CAN_INIT_TOTAL_BUDGET_US);
    u32InitStageLimit = Hw_TimeBaseUsToTicks(HW_CAN_INIT_STAGE_BUDGET_US);
    u32ObjStageLimit  = Hw_TimeBaseUsToTicks(HW_CAN_INIT_OBJ_BUDGET_US);

    if ((u32ApiLimit == 0UL) || (u32InitStageLimit == 0UL) || (u32ObjStageLimit == 0UL))
    {
        return 0U;
    }

    Hw_CanEnableClock(pstCanCfgItem);

    if (Hw_CanCheckBitTiming(pstCanCfgItem->u32Baudrate, u32SysclkHz, &u32Prescaler) == 0U)
    {
        return 0U;
    }

    if (Hw_CanEnterInit(pstRegs, u32ApiStart, u32ApiLimit, u32InitStageLimit) != HW_CAN_OK)
    {
        return 0U;
    }

    Hw_CanWriteBitTiming(pstRegs, u32Prescaler);

    /* 自动总线关闭恢复 */
    if (pstCanCfgItem->bAutoBusOn != 0U)
    {
        pstRegs->CAN_CTL.all |= HW_CAN_CTL_ABO;
    }
    else
    {
        pstRegs->CAN_CTL.all &= ~HW_CAN_CTL_ABO;
    }

    /* 测试模式：内部回环 */
    if (pstCanCfgItem->bLoopback != 0U)
    {
        pstRegs->CAN_CTL.all |= HW_CAN_CTL_TEST;
        pstRegs->CAN_TEST.all |= (1UL << 4U);
    }
    else
    {
        pstRegs->CAN_TEST.all &= ~(1UL << 4U);
        pstRegs->CAN_CTL.all &= ~HW_CAN_CTL_TEST;
    }

    /* 发送对象：ID 留 0，方向上为发送，Mask 关掉 */
    for (u16Idx = 0U; u16Idx < pstCanCfgItem->u16TxObjCnt; u16Idx++)
    {
        u16ObjNum = (Uint16)(pstCanCfgItem->u16TxObjNum + u16Idx);
        if (Hw_CanConfigObject(pstRegs, u16ObjNum, 0UL, 0UL, 1U, pstCanCfgItem->bTxExtId,
                               0U, 0U, 0U, u32ApiStart, u32ApiLimit,
                               u32ObjStageLimit) != HW_CAN_OK)
        {
            return 0U;
        }
    }

    /* 接收对象：逐对象独立过滤 */
    pstRxCfg = pstCanCfgItem->pstRxObjCfg;
    for (u16Idx = 0U; u16Idx < pstCanCfgItem->u16RxObjCnt; u16Idx++)
    {
        u16ObjNum = (Uint16)(pstCanCfgItem->u16RxObjNum + u16Idx);
        if (Hw_CanConfigObject(pstRegs, u16ObjNum, pstRxCfg[u16Idx].u32Id,
                               pstRxCfg[u16Idx].u32Mask, 0U, pstRxCfg[u16Idx].bExtId,
                               pstRxCfg[u16Idx].u16InitDlc, 1U, pstRxCfg[u16Idx].bDirCmp,
                               u32ApiStart, u32ApiLimit,
                               u32ObjStageLimit) != HW_CAN_OK)
        {
            return 0U;
        }
    }

    if (Hw_CanExitInit(pstRegs, u32ApiStart, u32ApiLimit, u32InitStageLimit) != HW_CAN_OK)
    {
        return 0U;
    }

    return 1U;
}

Uint16 Hw_CanInit(const ST_CAN_CFG *pstCanCfg)
{
    Uint16 u16Idx;
    Uint16 u16CanId;

    if (pstCanCfg == (const ST_CAN_CFG *)0)
    {
        return 0U;
    }

    if ((pstCanCfg->pstCanCfgItem == (const ST_CAN_CFG_ITEM *)0) || (pstCanCfg->u16ItemNum == 0U))
    {
        return 0U;
    }

    if (pstCanCfg->u16ItemNum > (Uint16)HW_CAN_ID_MAX)
    {
        return 0U;
    }

    /* 时间基准未就绪：直接失败，不进入任何按时间等待 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0U;
    }

    /* 全部静态校验先做完：校验失败不触碰硬件，也不改动既有状态 */
    for (u16Idx = 0U; u16Idx < pstCanCfg->u16ItemNum; u16Idx++)
    {
        if (Hw_CanCheckCfgItem(pstCanCfg, u16Idx) == 0U)
        {
            return 0U;
        }
    }

    gCanSysclkHz = pstCanCfg->u32SysclkHz;

    for (u16Idx = 0U; u16Idx < pstCanCfg->u16ItemNum; u16Idx++)
    {
        u16CanId = pstCanCfg->pstCanCfgItem[u16Idx].u16CanId;

        if (Hw_CanSingleInit(&pstCanCfg->pstCanCfgItem[u16Idx], pstCanCfg->u32SysclkHz) == 0U)
        {
            /* 失败模块保持未初始化，不清除未决标志 */
            gCanInited[u16CanId] = 0U;
            return 0U;
        }

        gCanCfg[u16CanId] = pstCanCfg->pstCanCfgItem[u16Idx];
        gCanCfgValid[u16CanId] = 1U;
        gCanInited[u16CanId] = 1U;
        gCanRecovering[u16CanId] = 0U;

        /* 只有完整重建成功后才清未决 */
        gCanPending[u16CanId] = 0U;
    }

    return 1U;
}

Uint16 Hw_CanIsInitialized(Uint16 u16CanId)
{
    if (u16CanId >= (Uint16)HW_CAN_ID_MAX)
    {
        return 0U;
    }

    return gCanInited[u16CanId];
}

Uint16 Hw_CanIsPending(Uint16 u16CanId)
{
    if (u16CanId >= (Uint16)HW_CAN_ID_MAX)
    {
        return 0U;
    }

    return ((gCanPending[u16CanId] != 0U) ? 1U : 0U);
}

Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len)
{
    volatile struct CAN_REGS *pstRegs;
    const ST_CAN_CFG_ITEM *pstItem;
    Uint32 u32ApiStart;
    Uint32 u32ApiLimit;
    Uint32 u32StageLimit;
    Uint32 u32StageStart;
    Uint32 u32Arb;
    Uint32 u32Cmd;
    Uint32 u32DatA;
    Uint32 u32DatB;
    Uint32 u32Bit;
    Uint16 u16ObjNum;
    Uint16 u16Idx;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return HW_CAN_ERR;
    }

    if ((gCanInited[u16CanId] == 0U) || (gCanRecovering[u16CanId] != 0U))
    {
        return HW_CAN_ERR;
    }

    pstItem = &gCanCfg[u16CanId];

    if (u16ObjIdx >= pstItem->u16TxObjCnt)
    {
        return HW_CAN_ERR;
    }

    if ((u16Len > HW_CAN_DATA_LEN_MAX) || ((u16Len != 0U) && (pu16Data == (const Uint16 *)0)))
    {
        return HW_CAN_ERR;
    }

    /* ID 位宽按帧类型校验：扩展帧超 29 位必须报错，不能静默截断 */
    if (pstItem->bTxExtId != 0U)
    {
        if ((u32MsgId & ~HW_CAN_ID_MASK) != 0UL)
        {
            return HW_CAN_ERR;
        }
    }
    else
    {
        if ((u32MsgId & ~HW_CAN_MSK_STD_M) != 0UL)
        {
            return HW_CAN_ERR;
        }
    }

    /* 未决期间不再下发任何IF命令 */
    if ((gCanPending[u16CanId] & HW_CAN_PENDING_TX) != 0U)
    {
        return HW_CAN_UNKNOWN;
    }

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    /* 硬件对象号 = 发送区起始号 + 序号；忙检查与提交必须指向同一对象 */
    u16ObjNum = (Uint16)(pstItem->u16TxObjNum + u16ObjIdx);

    u32Bit = (Uint32)1UL << (Uint32)(u16ObjNum - 1U);
    if ((pstRegs->CAN_TXRQ_21 & u32Bit) != 0UL)
    {
        return HW_CAN_BUSY;
    }

    u32ApiStart    = Hw_TimeBaseTicks();
    u32ApiLimit    = Hw_TimeBaseUsToTicks(HW_CAN_API_BUDGET_US);
    u32StageLimit  = Hw_TimeBaseUsToTicks(HW_CAN_STAGE_BUDGET_US);

    if ((u32ApiLimit == 0UL) || (u32StageLimit == 0UL))
    {
        return HW_CAN_ERR;
    }

    u32StageStart = Hw_TimeBaseTicks();

    if (Hw_CanWaitIf1Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    /* 数据按单元一字节装进 DATA/DATB */
    u32DatA = 0UL;
    u32DatB = 0UL;

    for (u16Idx = 0U; u16Idx < 4U; u16Idx++)
    {
        if (u16Idx < u16Len)
        {
            u32DatA |= ((Uint32)(pu16Data[u16Idx] & 0x00FFU) << (8U * u16Idx));
        }

        if ((Uint16)(u16Idx + 4U) < u16Len)
        {
            u32DatB |= ((Uint32)(pu16Data[u16Idx + 4U] & 0x00FFU) << (8U * u16Idx));
        }
    }

    pstRegs->CAN_IF1DATA.all  = u32DatA;
    pstRegs->CAN_IF1DATB.all  = u32DatB;

    /* 仲裁寄存器：发送方向 + 有效位 + 帧类型 */
    if (pstItem->bTxExtId != 0U)
    {
        u32Arb = u32MsgId & HW_CAN_ID_MASK;
        u32Arb |= HW_CAN_ARB_XTD;
    }
    else
    {
        u32Arb = (u32MsgId & HW_CAN_MSK_STD_M) << 18U;
    }

    u32Arb |= (HW_CAN_ARB_DIR | HW_CAN_ARB_MSGVAL);

    pstRegs->CAN_IF1ARB.all  = u32Arb;
    pstRegs->CAN_IF1MCTL.all = (Uint32)(u16Len & HW_CAN_MCTL_DLC_M) | HW_CAN_MCTL_EOB;

    u32Cmd = HW_CAN_CMD_WRITE_TX | (Uint32)u16ObjNum;
    if (u16Len != 0U)
    {
        u32Cmd |= (HW_CAN_CMD_DATA_A | HW_CAN_CMD_DATA_B);
    }

    /* 命令下发前最后一次预算确认：此前失败都还没触碰硬件，可安全返回 */
    if (Hw_CanBudgetOk(u32ApiStart, u32ApiLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    u32StageStart = Hw_TimeBaseTicks();

    /* 取时标本身失败时同样不得下发命令：此时还没有触碰硬件 */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    pstRegs->CAN_IF1CMD.all = u32Cmd;

    if (Hw_CanWaitIf1Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        /* 提交已发生但结果不可知：标记未决，交由上层与恢复流程处理 */
        gCanPending[u16CanId] |= HW_CAN_PENDING_TX;
        return HW_CAN_UNKNOWN;
    }

    return HW_CAN_OK;
}

Uint16 Hw_CanIsTxDone(Uint16 u16CanId, Uint16 u16ObjIdx)
{
    volatile struct CAN_REGS *pstRegs;
    const ST_CAN_CFG_ITEM *pstItem;
    Uint32 u32Bit;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if ((pstRegs == (volatile struct CAN_REGS *)0) || (gCanInited[u16CanId] == 0U))
    {
        return 0U;
    }

    pstItem = &gCanCfg[u16CanId];

    if (u16ObjIdx >= pstItem->u16TxObjCnt)
    {
        return 0U;
    }

    u32Bit = (Uint32)1UL << (Uint32)((pstItem->u16TxObjNum + u16ObjIdx) - 1U);

    return (((pstRegs->CAN_TXRQ_21 & u32Bit) == 0UL) ? 1U : 0U);
}

Uint16 Hw_CanIsRxReady(Uint16 u16CanId, Uint16 u16ObjIdx)
{
    volatile struct CAN_REGS *pstRegs;
    const ST_CAN_CFG_ITEM *pstItem;
    Uint32 u32Bit;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if ((pstRegs == (volatile struct CAN_REGS *)0) || (gCanInited[u16CanId] == 0U))
    {
        return 0U;
    }

    pstItem = &gCanCfg[u16CanId];

    if (u16ObjIdx >= pstItem->u16RxObjCnt)
    {
        return 0U;
    }

    u32Bit = (Uint32)1UL << (Uint32)((pstItem->u16RxObjNum + u16ObjIdx) - 1U);

    return (((pstRegs->CAN_NDAT_21 & u32Bit) != 0UL) ? 1U : 0U);
}

Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len,
                    Uint16 *pu16FrameType, Uint16 *pu16MsgLst)
{
    volatile struct CAN_REGS *pstRegs;
    const ST_CAN_CFG_ITEM *pstItem;
    Uint32 u32ApiStart;
    Uint32 u32ApiLimit;
    Uint32 u32StageLimit;
    Uint32 u32StageStart;
    Uint32 u32Arb;
    Uint32 u32Mctl;
    Uint32 u32DatA;
    Uint32 u32DatB;
    Uint16 u16ObjNum;
    Uint16 u16Idx;
    Uint16 u16Len;
    Uint16 u16FrameType;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return HW_CAN_ERR;
    }

    if ((gCanInited[u16CanId] == 0U) || (gCanRecovering[u16CanId] != 0U))
    {
        return HW_CAN_ERR;
    }

    pstItem = &gCanCfg[u16CanId];

    if (u16ObjIdx >= pstItem->u16RxObjCnt)
    {
        return HW_CAN_ERR;
    }

    /* 未决期间不再下发任何IF命令 */
    if ((gCanPending[u16CanId] & HW_CAN_PENDING_RX) != 0U)
    {
        return HW_CAN_UNKNOWN;
    }

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    u32ApiStart   = Hw_TimeBaseTicks();
    u32ApiLimit   = Hw_TimeBaseUsToTicks(HW_CAN_API_BUDGET_US);
    u32StageLimit = Hw_TimeBaseUsToTicks(HW_CAN_STAGE_BUDGET_US);

    if ((u32ApiLimit == 0UL) || (u32StageLimit == 0UL))
    {
        return HW_CAN_ERR;
    }

    u32StageStart = Hw_TimeBaseTicks();

    if (Hw_CanWaitIf2Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    u16ObjNum = (Uint16)(pstItem->u16RxObjNum + u16ObjIdx);

    if (Hw_CanBudgetOk(u32ApiStart, u32ApiLimit) == 0U)
    {
        return HW_CAN_BUSY;
    }

    /* 一次事务同时读回 ARB/CONTROL/DATA，NewDat 由硬件在搬运时清除 */
    u32StageStart = Hw_TimeBaseTicks();

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    pstRegs->CAN_IF2CMD.all = (HW_CAN_CMD_READ_ALL | (Uint32)u16ObjNum);

    if (Hw_CanWaitIf2Idle(pstRegs, u32ApiStart, u32ApiLimit, u32StageStart, u32StageLimit) == 0U)
    {
        gCanPending[u16CanId] |= HW_CAN_PENDING_RX;
        return HW_CAN_UNKNOWN;
    }

    /* IF 侧 MCTL 反映的是清除 NewDat 之前的状态 */
    u32Mctl = pstRegs->CAN_IF2MCTL.all;

    if ((u32Mctl & HW_CAN_MCTL_NEWDAT) == 0UL)
    {
        return HW_CAN_EMPTY;
    }

    u32Arb  = pstRegs->CAN_IF2ARB.all;
    u32DatA = pstRegs->CAN_IF2DATA.all;
    u32DatB = pstRegs->CAN_IF2DATB.all;

    if ((u32Arb & HW_CAN_ARB_XTD) != 0UL)
    {
        u16FrameType = HW_CAN_FRAME_EXT;
        if (pu32MsgId != (Uint32 *)0)
        {
            *pu32MsgId = u32Arb & HW_CAN_ID_MASK;
        }
    }
    else
    {
        u16FrameType = HW_CAN_FRAME_STD;
        if (pu32MsgId != (Uint32 *)0)
        {
            *pu32MsgId = (u32Arb >> 18U) & HW_CAN_MSK_STD_M;
        }
    }

    u16Len = (Uint16)(u32Mctl & HW_CAN_MCTL_DLC_M);
    if (u16Len > HW_CAN_DATA_LEN_MAX)
    {
        u16Len = HW_CAN_DATA_LEN_MAX;
    }

    if (pu16Data != (Uint16 *)0)
    {
        for (u16Idx = 0U; u16Idx < HW_CAN_DATA_LEN_MAX; u16Idx++)
        {
            if (u16Idx < 4U)
            {
                pu16Data[u16Idx] = (Uint16)((u32DatA >> (8U * u16Idx)) & 0x00FFUL);
            }
            else
            {
                pu16Data[u16Idx] = (Uint16)((u32DatB >> (8U * (u16Idx - 4U))) & 0x00FFUL);
            }
        }
    }

    if (pu16Len != (Uint16 *)0)
    {
        *pu16Len = u16Len;
    }

    if (pu16FrameType != (Uint16 *)0)
    {
        *pu16FrameType = u16FrameType;
    }

    if (pu16MsgLst != (Uint16 *)0)
    {
        *pu16MsgLst = (((u32Mctl & HW_CAN_MCTL_MSGLST) != 0UL) ? 1U : 0U);
    }

    return HW_CAN_OK;
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
    Uint32 u32ApiStart;
    Uint32 u32ApiLimit;
    Uint32 u32StageLimit;
    Uint16 u16Result;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if ((pstRegs == (volatile struct CAN_REGS *)0) || (gCanInited[u16CanId] == 0U) ||
        (gCanRecovering[u16CanId] != 0U))
    {
        return HW_CAN_ERR;
    }

    /* 存在未决事务时不得走 Bus-Off 恢复：必须按 Hw_CanRecoverModule() 的契约处理 */
    if (gCanPending[u16CanId] != 0U)
    {
        return HW_CAN_BUSY;
    }

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    u32ApiStart   = Hw_TimeBaseTicks();
    u32ApiLimit   = Hw_TimeBaseUsToTicks(HW_CAN_INIT_TOTAL_BUDGET_US);
    u32StageLimit = Hw_TimeBaseUsToTicks(HW_CAN_INIT_STAGE_BUDGET_US);

    if ((u32ApiLimit == 0UL) || (u32StageLimit == 0UL))
    {
        return HW_CAN_ERR;
    }

    /* 只进出初始化：不调用复位报文RAM/SWR 的助手，收发对象与位时序保持原状 */
    /* 已经写过 CTL.INIT：此后任何"无法确认状态"的结果都按未决处理，锁住该模块收发 */
    u16Result = Hw_CanEnterInitPlain(pstRegs, u32ApiStart, u32ApiLimit, u32StageLimit);
    if (u16Result != HW_CAN_OK)
    {
        gCanPending[u16CanId] |= (HW_CAN_PENDING_TX | HW_CAN_PENDING_RX);
        return HW_CAN_UNKNOWN;
    }

    u16Result = Hw_CanExitInit(pstRegs, u32ApiStart, u32ApiLimit, u32StageLimit);
    if (u16Result != HW_CAN_OK)
    {
        gCanPending[u16CanId] |= (HW_CAN_PENDING_TX | HW_CAN_PENDING_RX);
        return HW_CAN_UNKNOWN;
    }

    return HW_CAN_OK;
}

Uint16 Hw_CanRecoverModule(Uint16 u16CanId)
{
    volatile struct CAN_REGS *pstRegs;

    pstRegs = Hw_CanGetRegs(u16CanId);

    if (pstRegs == (volatile struct CAN_REGS *)0)
    {
        return HW_CAN_ERR;
    }

    /* 能否恢复取决于配置是否有效，而不是当前是否已初始化：恢复失败后必须还能重试 */
    if (gCanCfgValid[u16CanId] == 0U)
    {
        return HW_CAN_ERR;
    }

    if (gCanRecovering[u16CanId] != 0U)
    {
        return HW_CAN_BUSY;
    }

    if (Hw_TimeBaseIsReady() == 0U)
    {
        return HW_CAN_ERR;
    }

    /* 恢复期间禁止其它访问；未决标志保持到重建成功为止 */
    gCanRecovering[u16CanId] = 1U;
    gCanInited[u16CanId] = 0U;

    if (Hw_CanSingleInit(&gCanCfg[u16CanId], gCanSysclkHz) == 0U)
    {
        /* 失败：未决与故障状态保留，配置仍有效，允许再次受控恢复 */
        gCanRecovering[u16CanId] = 0U;
        return HW_CAN_UNKNOWN;
    }

    gCanPending[u16CanId] = 0U;
    gCanInited[u16CanId] = 1U;
    gCanRecovering[u16CanId] = 0U;

    return HW_CAN_OK;
}


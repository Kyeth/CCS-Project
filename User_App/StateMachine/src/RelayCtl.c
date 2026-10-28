/**
 * @file RelayCtl.c
 * @author yetingheng (004182)
 * @brief 继电器统一控制层实现
 * @version 0.1
 * @date 2026-10-27
 *
 * @copyright Copyright (c) 2026
 */

#include "RelayCtl.h"
#include "Hw_GpioDriver.h"

/******************************************************************************
 * 内部数据
 ******************************************************************************/
typedef struct
{
    Uint16 u16Pin;          /* GPIO 编号 */
    Uint16 u16Default;      /* 初始化后的缺省电平 */
} ST_RELAY_PIN;

/* 顺序必须与 E_RELAY_ID 一致 */
static const ST_RELAY_PIN sRelayPin[E_RELAY_NUM] =
{
    { 58U, 0U },    /* E_RELAY_INV_R   */
    { 61U, 0U },    /* E_RELAY_INV_S   */
    { 62U, 0U },    /* E_RELAY_INV_N   */
    { 63U, 0U },    /* E_RELAY_INV_T   */
    { 73U, 0U },    /* E_RELAY_PRECHG  */
    { 66U, 0U },    /* E_RELAY_PE      */
    { 74U, 0U },    /* E_RELAY_AUX     */
    { 50U, 0U },    /* E_RELAY_PV_ISO1 */
    { 51U, 0U }     /* E_RELAY_PV_ISO2 */
};

/* 命令状态位图 */
static Uint32 sRelayMask = 0U;

/******************************************************************************
 * API
 ******************************************************************************/
void RelayCtlInit(void)
{
    Uint16 i;

    sRelayMask = 0U;

    for (i = 0U; i < (Uint16)E_RELAY_NUM; i++)
    {
        Hw_GpioWritePin((Uint32)sRelayPin[i].u16Pin,
                        (sRelayPin[i].u16Default != 0U) ? (Uint32)HW_GPIO_HIGH : (Uint32)HW_GPIO_LOW);

        if (sRelayPin[i].u16Default != 0U)
        {
            sRelayMask |= (1UL << i);
        }
    }
}

void RelayCtlSet(E_RELAY_ID eId, Uint16 u16On)
{
    Uint16 u16Idx = (Uint16)eId;

    if (u16Idx >= (Uint16)E_RELAY_NUM)
    {
        return;
    }

    Hw_GpioWritePin((Uint32)sRelayPin[u16Idx].u16Pin,
                    (u16On != 0U) ? (Uint32)HW_GPIO_HIGH : (Uint32)HW_GPIO_LOW);

    if (u16On != 0U)
    {
        sRelayMask |= (1UL << u16Idx);
    }
    else
    {
        sRelayMask &= ~(1UL << u16Idx);
    }
}

Uint16 RelayCtlGet(E_RELAY_ID eId)
{
    Uint16 u16Idx = (Uint16)eId;

    if (u16Idx >= (Uint16)E_RELAY_NUM)
    {
        return 0U;
    }

    return ((sRelayMask & (1UL << u16Idx)) != 0UL) ? 1U : 0U;
}

void RelayCtlInvAll(Uint16 u16On)
{
    RelayCtlSet(E_RELAY_INV_R, u16On);
    RelayCtlSet(E_RELAY_INV_S, u16On);
    RelayCtlSet(E_RELAY_INV_N, u16On);
    RelayCtlSet(E_RELAY_INV_T, u16On);
}

Uint32 RelayCtlGetMask(void)
{
    return sRelayMask;
}

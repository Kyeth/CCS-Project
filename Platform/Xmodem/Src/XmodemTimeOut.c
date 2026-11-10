#include "XmodemTimeOut.h"


// 这里需要导入定时器的头文件
#include "Hw_TimeBase.h"

#ifdef __cplusplus
extern "C" {
#endif



void TimeInit(void)
{
    return;
}


void TimeStart(ST_TIMEOUT_MANAGER *pstTimeOutManager, Uint32 u32Ms)
{
    if (pstTimeOutManager == NULL)
    {
        return;
    }

    if (pstTimeOutManager->u16IsActive == 0)
    {
        /* 时间源没就绪就不启动计时：宁可在等待态多停一拍，也不要拿无效时刻判超时 */
        if (Hw_TimeBaseIsReady() == 0U)
        {
            return;
        }

        pstTimeOutManager->u16IsActive = 1;
        pstTimeOutManager->u32StartTick = Hw_TimeBaseTicks();
        pstTimeOutManager->u32TimeOut = Hw_TimeBaseUsToTicks(u32Ms * 1000UL);
    }

}

int16 IsTimeOut(ST_TIMEOUT_MANAGER *pstTimeOutManager)
{
    Uint32 u32NowTick = 0;

    if ((pstTimeOutManager == NULL) || (pstTimeOutManager->u16IsActive != 1))
    {
        return 0;
    }

    /* 时间源不可用时不判超时（否则会把刚启动的计时立刻判成到期） */
    if (Hw_TimeBaseIsReady() == 0U)
    {
        return 0;
    }

    u32NowTick = Hw_TimeBaseTicks();

    /* 时基适配：Hw_TimeBase 是向下计数的自由计数器，经过的时间 = 起始值 - 当前值。
     * 参考实现是向上计数，差值方向必须反过来，否则一进等待就判超时。 */
    if ((Uint32)(pstTimeOutManager->u32StartTick - u32NowTick) > pstTimeOutManager->u32TimeOut)
    {
        return 1;
    }

    return 0;
}


void TimeStop(ST_TIMEOUT_MANAGER *pstTimeOutManager)
{
    pstTimeOutManager->u16IsActive = 0;
}





#ifdef __cplusplus
}
#endif

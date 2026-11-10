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

    if (pstTimeOutManager->u16IsActive == 0)
    {
        pstTimeOutManager->u16IsActive = 1;
        pstTimeOutManager->u32StartTick = Hw_TimeBaseTicks();
        pstTimeOutManager->u32TimeOut = Hw_TimeBaseUsToTicks(u32Ms * 1000UL);
    }

}

int16 IsTimeOut(ST_TIMEOUT_MANAGER *pstTimeOutManager)
{
    Uint32 u32NowTick = Hw_TimeBaseTicks();

    if (pstTimeOutManager->u16IsActive == 1)
    {
        if ((u32NowTick - pstTimeOutManager->u32StartTick) > pstTimeOutManager->u32TimeOut)
        {
            return 1;
        }
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

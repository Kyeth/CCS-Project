#include "XmodemTimeOut.h"


// 这里需要导入定时器的头文件
#include "Hw_SysCpuTimer.h"

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
        pstTimeOutManager->u32StartTick = stSysCpuTimer2.TimerTick;
        pstTimeOutManager->u32TimeOut = u32Ms;
    }

}

int16 IsTimeOut(ST_TIMEOUT_MANAGER *pstTimeOutManager)
{
    Uint32 u32NowTick =  stSysCpuTimer2.TimerTick;

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

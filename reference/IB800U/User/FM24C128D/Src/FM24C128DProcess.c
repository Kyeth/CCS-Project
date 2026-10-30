#include "FM24C128D.h"
#include "FM24C128DProcess.h"
#include "FM24C128DTramsport.h"
#include "Hw_SysCpuTimer.h"

#ifdef __cplusplus
    extern "C" {
#endif

void FM24C128DInit(void)
{
    Uint32 u32tick = stSysCpuTimer2.TimerTick;

    ST_FM24C128D_INTF stFM24C128DRwIntf;

    stFM24C128DRwIntf.EepromRead = FM24CRead;
    stFM24C128DRwIntf.EepromWrite = FM24CWrite;
    stFM24C128DRwIntf.EepromGetTick = FM24CGetTick;

    // FM24C128D初始化
    FM24C128DRegister(&stFM24C128DHandle, &stFM24C128DRwIntf, 25, FM24C128D_DATA_ADDR);

    // 设置写使能
    FM24C128DDisableWriteProtection(&stFM24C128DHandle);

}

#ifdef __cplusplus
    }
#endif

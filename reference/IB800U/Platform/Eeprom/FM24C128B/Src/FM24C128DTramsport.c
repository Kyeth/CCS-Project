#include "FM24C128DTramsport.h"
#include "Hw_I2cDriver.h"
#include "Hw_SysCpuTimer.h"

#ifdef __cplusplus
    extern "C" {
#endif

int16 FM24CRead(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen)
{
    int16 iRet = 0;
    ST_I2C_MSG stI2cMsg;
    Uint32 u32NowTick = 0;
    Uint32 u32Tick = 0;

    stI2cMsg.u16State = I2C_IDLE;
    stI2cMsg.u16DataLen = u16DataLen;
    stI2cMsg.u16MemAddr = u16MemAddr;
    stI2cMsg.u16SlaveAddr = pstHandle->u16Addr;
    u32Tick = stSysCpuTimer2.TimerTick;
    while (Hw_I2cRead(I2C_ID0, &stI2cMsg) != I2C_SUCCESS)
    {
        u32NowTick = stSysCpuTimer2.TimerTick;
        if ((u32NowTick - u32Tick) > 3)
        {
            break;
        }
    }

    memcpy(pu16DataBuff, &stI2cMsg.u16DataBuff[0], u16DataLen);
    return iRet;
}

int16 FM24CWrite(ST_FM24C128D_HANDLE *pstHandle,  Uint16 u16MemAddr, Uint16 *pu16DataBuff, Uint16 u16DataLen)
{
    ST_I2C_MSG stI2cMsg;

    stI2cMsg.u16State = I2C_IDLE;
    stI2cMsg.u16DataLen = u16DataLen;
    stI2cMsg.u16MemAddr = u16MemAddr;
    stI2cMsg.u16SlaveAddr = pstHandle->u16Addr;
    memcpy(&stI2cMsg.u16DataBuff[0], pu16DataBuff, u16DataLen);
    return Hw_I2cWrite(I2C_ID0, &stI2cMsg);
}

int16 FM24CGetTick(void)
{
    return stSysCpuTimer2.TimerTick;
}

#ifdef __cplusplus
    }
#endif


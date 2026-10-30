#include "ParamSaveProcess.h"
#include "ParamSave.h"
#include "ParamSaveCfg.h"
#include "FM24C128D.h"

ST_PARAM_SAVE_HANDLE stParamSaveHandle;

int ParamWrite(Uint32 u32Addr, Uint16 *u16Buff, Uint16 u16Size)
{
    FM24C128DWrite(&stFM24C128DHandle, u32Addr, u16Buff, u16Size);
    return 1;
}

int ParamRead(Uint32 u32Addr, Uint16 *u16Buff, Uint16 u16Size)
{
    FM24C128DRead(&stFM24C128DHandle, u32Addr, u16Buff, u16Size);
    return 1;
}

void ParamSaveProcessInit(void)
{
    ST_PARAM_WR_INTF stParamWrIntf;
    stParamWrIntf.ParamRead = ParamRead;
    stParamWrIntf.ParamWrite = ParamWrite;
    ParamSaveInit(&stParamSaveHandle, &stParamCfg, &stParamWrIntf);
}

void TaskParamSave(void)
{
    ParamSaveHandle(&stParamSaveHandle);
}

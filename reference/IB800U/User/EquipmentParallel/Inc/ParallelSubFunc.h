#ifndef __PARALLELSUBFUNCTION_H
#define __PARALLELSUBFUNCTION_H

#include "EquipmentParallelCore.h"
#include "LLCControl.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum
{
    STABLE_VOLTAGE_MODE = 0x1,      // 稳压模式
    FOLLOW_MODE = 0x2,              // 跟随模式
}E_DEVICE_MODE;

extern void HostDetection(ST_BATTERY_HANDLE *pstBatteryHandle);
extern void WireBreakageDetection(ST_BATTERY_HANDLE *pstBatteryHandle, Uint32 u32Cycle);
extern void ParallelCheck(ST_BATTERY_HANDLE *pstBatteryHandle);
extern Uint16 VersionCheck(ST_BATTERY_HANDLE *pstBatteryHandle);
extern void VBusRegulation(ST_BATTERY_HANDLE *pstBatteryHandle, LLC_SYSTEM_CONTROL_PARA *pLLCSystemCtrl);
extern Uint16 GetOnlineNum(ST_BATTERY_HANDLE *pstBatteryHandle);
extern Uint16 GetEquipmentNormalNum(ST_BATTERY_HANDLE *pstBatteryHandle);
extern Uint16 GetCtrlWorkNum(ST_BATTERY_HANDLE *pstBatteryHandle);
extern void AutoSoc(ST_BATTERY_HANDLE *pstBatteryHandle, ST_SAMPLE *pObjSample);
extern E_DEVICE_MODE CheckDevieMode(ST_BATTERY_HANDLE *pstBatteryHandle);
extern void PowerCheck(ST_BATTERY_HANDLE *pstBatteryHandle);
extern void DorCCheck(ST_BATTERY_HANDLE *pstBatteryHandle);

static inline void SetParallelMode(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 IsEnable)
{
    pstBatteryHandle->u16IsParallel = IsEnable;
}

static inline Uint16 GetParallelMode(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16IsParallel;
}

static inline Uint16 GetMasterDeviceIndex(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16MasterDeviceIndex;
}

static inline Uint16 GetSocMaxDeviceIndex(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16SocMaxDeviceIndex;
}

static inline E_DEVICE_MODE GetDeviceMode(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return (E_DEVICE_MODE)pstBatteryHandle->u16DeviceMode;
}

static inline Uint16 GetDorCState(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16CtrlDorCState;
}

static inline Uint16 GetCtrlState(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16CtrlState;
}

static inline Uint16 GetVersionCheckFlag(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    return pstBatteryHandle->u16VersionCheckFlag;
}

#ifdef __cplusplus
    }
#endif

#endif

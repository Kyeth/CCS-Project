#include "ParallelSubFunc.h"
#include "string.h"
#include "PlatformManager.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define SOC_STABLE_SWITCH_THRESHOLD     5
#define INVALID_PACK_INDEX              0xFFFF
#define INVALID_PACK_ADDR               0xFFFF

static Uint16 IsPackSelfOrValid(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16Index)
{
    if (u16Index == 0)
    {
        return 1;
    }

    if ((pstBatteryHandle->stBatteryPack[u16Index].u16IsInit == 1) &&
        (pstBatteryHandle->stBatteryPack[u16Index].u16IsOnline == 1) &&
        (pstBatteryHandle->stBatteryPack[u16Index].stBatteryData.stBroadcastData_1.bIsEquipmentNormal == 1))
    {
        return 1;
    }

    return 0;
}

static Uint16 FindPackIndexByAddr(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16Addr)
{
    Uint16 i = 0;

    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        if (IsPackSelfOrValid(pstBatteryHandle, i) == 1)
        {
            if (pstBatteryHandle->stBatteryPack[i].u16PackAddr == u16Addr)
            {
                return i;
            }
        }
    }

    return INVALID_PACK_INDEX;
}

static Uint16 IsStableCandidateValid(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16Index)
{
    if (IsPackSelfOrValid(pstBatteryHandle, u16Index) == 0)
    {
        return 0;
    }

    if (pstBatteryHandle->stBatteryPack[u16Index].stBatteryData.stBroadcastData_1.bIsStableKey == 0)
    {
        return 1;
    }

    return 0;
}

static Uint16 GetStableAddrFromMasterCmd(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16MasterIndex)
{
    ST_SPECIFIED_CTRL_3 *pstCtrl = NULL;

    if (u16MasterIndex >= MAX_PACK_NUM)
    {
        return INVALID_PACK_ADDR;
    }

    pstCtrl = &pstBatteryHandle->stBatteryPack[u16MasterIndex].stBatteryData.stSpecifiedCtrl_3;

    /*
     * 地址 1 是主机自己。
     * 如果 2~5 都是 FOLLOW，则认为地址 1 是稳压包。
     */
    if (pstCtrl->bSetDeviceMode2 == STABLE_VOLTAGE_MODE)
    {
        return 2;
    }

    if (pstCtrl->bSetDeviceMode3 == STABLE_VOLTAGE_MODE)
    {
        return 3;
    }

    if (pstCtrl->bSetDeviceMode4 == STABLE_VOLTAGE_MODE)
    {
        return 4;
    }

    if (pstCtrl->bSetDeviceMode5 == STABLE_VOLTAGE_MODE)
    {
        return 5;
    }

    return 1;
}

void HostDetection(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;
    Uint16 u16MinAddr = INVALID_PACK_ADDR;
    Uint16 u16MinAddrIndex = INVALID_PACK_INDEX;

    Uint16 u16TargetStableIndex = INVALID_PACK_INDEX;
    Uint16 u16TargetStableAddr = INVALID_PACK_ADDR;
    Uint16 u16TargetStableSoc = 0;

    Uint16 u16CurrentStableIndex = INVALID_PACK_INDEX;
    Uint16 u16CurrentStableSoc = 0;
    Uint16 u16CurrentStableValid = 0;

    // 先统一找地址最小设备。地址最小者为主机。
    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        if (IsPackSelfOrValid(pstBatteryHandle, i) == 1)
        {
            if (pstBatteryHandle->stBatteryPack[i].u16PackAddr < u16MinAddr)
            {
                u16MinAddr = pstBatteryHandle->stBatteryPack[i].u16PackAddr;
                u16MinAddrIndex = i;
            }
        }
    }
    pstBatteryHandle->u16MasterDeviceIndex = u16MinAddrIndex;

    if (u16MinAddrIndex == 0)
    {
        pstBatteryHandle->u16IsMasterSlave = PARALLEL_MASTER;
    }
    else
    {
        pstBatteryHandle->u16IsMasterSlave = PARALLEL_SLAVE;
    }
    // 从机不做 SOC 稳压包选举。从机只根据主机下发的 stSpecifiedCtrl_3 反推出当前主机指定的稳压地址。
    if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_SLAVE)
    {
        if (u16MinAddrIndex != INVALID_PACK_INDEX)
        {
            pstBatteryHandle->u16SocMaxDeviceAddr =
                GetStableAddrFromMasterCmd(pstBatteryHandle, u16MinAddrIndex);

            pstBatteryHandle->u16SocMaxDeviceIndex =
                FindPackIndexByAddr(pstBatteryHandle, pstBatteryHandle->u16SocMaxDeviceAddr);
        }

        return;
    }
    /*
        * 以下只有主机执行。
        * 主机在所有有效设备中选稳压候选：
        * 1. 必须具备稳压许可 bIsStableKey == 0
        * 2. SOC 高者优先
        * 3. SOC 相同时，地址小者优先
        */
       for (i = 0; i < MAX_PACK_NUM; i++)
       {
           if (IsStableCandidateValid(pstBatteryHandle, i) == 1)
           {
               if (u16TargetStableIndex == INVALID_PACK_INDEX)
               {
                   u16TargetStableIndex = i;
                   u16TargetStableSoc =
                       pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bSoc;
               }
               else
               {
                   Uint16 u16Soc =
                       pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bSoc;

                   if (u16Soc > u16TargetStableSoc)
                   {
                       u16TargetStableIndex = i;
                       u16TargetStableSoc = u16Soc;
                   }
                   else if (u16Soc == u16TargetStableSoc)
                   {
                       if (pstBatteryHandle->stBatteryPack[i].u16PackAddr <
                           pstBatteryHandle->stBatteryPack[u16TargetStableIndex].u16PackAddr)
                       {
                           u16TargetStableIndex = i;
                           u16TargetStableSoc = u16Soc;
                       }
                   }
               }
           }
       }

       // 如果没有具备稳压许可的包，则退回地址最小设备。这一般是异常兜底，正常情况下不应该长期走到这里。
       if (u16TargetStableIndex == INVALID_PACK_INDEX)
       {
           u16TargetStableIndex = u16MinAddrIndex;
           u16TargetStableSoc =
               pstBatteryHandle->stBatteryPack[u16TargetStableIndex].stBatteryData.stBroadcastData_1.bSoc;
       }

       // 滞回基准用上一次的稳压地址，而不是上一次的 index。因为 index 是本地数组位置，不能作为跨设备身份。
       u16CurrentStableIndex =
           FindPackIndexByAddr(pstBatteryHandle, pstBatteryHandle->u16SocMaxDeviceAddr);

       if ((u16CurrentStableIndex != INVALID_PACK_INDEX) &&
           (IsStableCandidateValid(pstBatteryHandle, u16CurrentStableIndex) == 1))
       {
           u16CurrentStableValid = 1;
           u16CurrentStableSoc =
               pstBatteryHandle->stBatteryPack[u16CurrentStableIndex].stBatteryData.stBroadcastData_1.bSoc;
       }

       if (u16CurrentStableValid == 1)
       {
           if (u16TargetStableIndex != u16CurrentStableIndex)
           {
               if (u16TargetStableSoc >= (u16CurrentStableSoc + SOC_STABLE_SWITCH_THRESHOLD))
               {
                   pstBatteryHandle->u16SocMaxDeviceIndex = u16TargetStableIndex;
               }
               else
               {
                   pstBatteryHandle->u16SocMaxDeviceIndex = u16CurrentStableIndex;
               }
           }
           else
           {
               pstBatteryHandle->u16SocMaxDeviceIndex = u16CurrentStableIndex;
           }
       }
       else
       {
           pstBatteryHandle->u16SocMaxDeviceIndex = u16TargetStableIndex;
       }

       u16TargetStableAddr =
           pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16SocMaxDeviceIndex].u16PackAddr;

       pstBatteryHandle->u16SocMaxDeviceAddr = u16TargetStableAddr;
}

static Uint16 IsOtherStableReady(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;

    /*
     * i 从 1 开始，明确排除自己。
     * 这里判断的是总线上是否已有其他设备正在稳压并软启完成。
     */
    for (i = 1; i < MAX_PACK_NUM; i++)
    {
        if ((pstBatteryHandle->stBatteryPack[i].u16IsInit == 1) &&
            (pstBatteryHandle->stBatteryPack[i].u16IsOnline == 1) &&
            (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsEquipmentNormal == 1) &&
            (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bGetDeviceMode == STABLE_VOLTAGE_MODE) &&
            (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsSoftFinsh == 1))
        {
            return 1;
        }
    }

    return 0;
}

static void SetMasterSpecifiedMode(ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16StableAddr)
{
    ST_SPECIFIED_CTRL_3 *pstCtrl = NULL;

    pstCtrl = &pstBatteryHandle->stBatteryPack[0].stBatteryData.stSpecifiedCtrl_3;

    /*
     * 先全部置为跟随，再把目标稳压包置为稳压。
     * 地址 1 是主机自己，不需要通过 bSetDeviceMode1 下发给别人。
     */
    pstCtrl->bSetDeviceMode1 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode2 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode3 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode4 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode5 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode6 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode7 = FOLLOW_MODE;
    pstCtrl->bSetDeviceMode8 = FOLLOW_MODE;

    switch (u16StableAddr)
    {
    case 2:
        pstCtrl->bSetDeviceMode2 = STABLE_VOLTAGE_MODE;
        break;

    case 3:
        pstCtrl->bSetDeviceMode3 = STABLE_VOLTAGE_MODE;
        break;

    case 4:
        pstCtrl->bSetDeviceMode4 = STABLE_VOLTAGE_MODE;
        break;

    case 5:
        pstCtrl->bSetDeviceMode5 = STABLE_VOLTAGE_MODE;
        break;

    case 1:
    default:
        break;
    }
}

static Uint16 GetLocalModeFromMasterCmd(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 u16Mode = FOLLOW_MODE;
    ST_SPECIFIED_CTRL_3 *pstCtrl = NULL;

    if (pstBatteryHandle->u16MasterDeviceIndex >= MAX_PACK_NUM)
    {
        return FOLLOW_MODE;
    }

    pstCtrl =
        &pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stSpecifiedCtrl_3;

    switch (pstBatteryHandle->u16LocalAddr)
    {
    case 2:
        u16Mode = pstCtrl->bSetDeviceMode2;
        break;

    case 3:
        u16Mode = pstCtrl->bSetDeviceMode3;
        break;

    case 4:
        u16Mode = pstCtrl->bSetDeviceMode4;
        break;

    case 5:
        u16Mode = pstCtrl->bSetDeviceMode5;
        break;

    default:
        u16Mode = FOLLOW_MODE;
        break;
    }

    if ((u16Mode != STABLE_VOLTAGE_MODE) && (u16Mode != FOLLOW_MODE))
    {
        u16Mode = FOLLOW_MODE;
    }

    return u16Mode;
}


E_DEVICE_MODE CheckDevieMode(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 u16Mode = FOLLOW_MODE;
    Uint16 u16LocalSoftFinish = 0;
    Uint16 u16OtherStableReady = 0;

    /*
     * 主机：根据 HostDetection 选出的 u16SocMaxDeviceAddr 统一下发模式。
     */
    if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_MASTER)
    {
        SetMasterSpecifiedMode(pstBatteryHandle, pstBatteryHandle->u16SocMaxDeviceAddr);

        if (pstBatteryHandle->u16SocMaxDeviceAddr == pstBatteryHandle->u16LocalAddr)
        {
            u16Mode = STABLE_VOLTAGE_MODE;
        }
        else
        {
            u16Mode = FOLLOW_MODE;
        }
    }
    else
    {
        /*
         * 从机：只响应主机命令。
         * 不再根据本机 HostDetection 的 SOC 结果决定自己是否为稳压包。
         */
        u16Mode = GetLocalModeFromMasterCmd(pstBatteryHandle);
    }

    /*
     * 抢占兜底逻辑：
     * 如果当前命令要求本机跟随，但总线上没有其他设备已经处于稳压且软启完成，
     * 而本机软启已经完成，则本机临时接管稳压。
     *
     * 注意：这里不再用 u16SocMaxDeviceAddr 去找目标稳压包。
     * 因为从机本地的 u16SocMaxDeviceIndex / Addr 可能不是主机权威结果。
     */
    if (u16Mode == FOLLOW_MODE)
    {
        u16LocalSoftFinish =
            pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bIsSoftFinsh;

        u16OtherStableReady = IsOtherStableReady(pstBatteryHandle);

        if ((u16OtherStableReady == 0) && (u16LocalSoftFinish == 1))
        {
            u16Mode = STABLE_VOLTAGE_MODE;
        }
    }

    /*
     * 模式切换保护：
     * 1. 切稳压：允许立即切。
     * 2. 切跟随：必须确认其他设备已经稳压并软启完成。
     */
    if (pstBatteryHandle->u16DeviceMode != u16Mode)
    {
        if (u16Mode == STABLE_VOLTAGE_MODE)
        {
            pstBatteryHandle->u16DeviceMode = STABLE_VOLTAGE_MODE;
        }
        else if (u16Mode == FOLLOW_MODE)
        {
            if (IsOtherStableReady(pstBatteryHandle) == 1)
            {
                pstBatteryHandle->u16DeviceMode = FOLLOW_MODE;
            }
            else
            {
                /*
                 * 没有其他稳压设备时，不允许退出稳压。
                 * 返回值也保持当前实际模式，避免上层误以为已经切到跟随。
                 */
                u16Mode = pstBatteryHandle->u16DeviceMode;
            }
        }
    }

    return (E_DEVICE_MODE)pstBatteryHandle->u16DeviceMode;
}

void WireBreakageDetection(ST_BATTERY_HANDLE *pstBatteryHandle, Uint32 u32Cycle)
{
    Uint16 i = 0;
    Uint16 u16OnlineNum = 0;

    // 本机强制在线。stBatteryPack[0] 是本机数据，不应该被通信断线逻辑清除。

    pstBatteryHandle->stBatteryPack[0].u16IsOnline = 1;
    pstBatteryHandle->stBatteryPack[0].u16IsInit = 1;
    pstBatteryHandle->stBatteryPack[0].u16PackAddr = pstBatteryHandle->u16LocalAddr;
    pstBatteryHandle->stBatteryPack[0].u16WirebreakTimeOutCnt = 0;
    pstBatteryHandle->stBatteryPack[0].u16HeartCountBak = pstBatteryHandle->stBatteryPack[0].u16HeartCount;
    u16OnlineNum = 1;

    for (i = 1; i < MAX_PACK_NUM; i++)
    {
        if (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1)
        {
            if (pstBatteryHandle->stBatteryPack[i].u16HeartCountBak == pstBatteryHandle->stBatteryPack[i].u16HeartCount)
            {
                pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutCnt += u32Cycle;
            }
            else
            {
                pstBatteryHandle->stBatteryPack[i].u16IsOnline = 1;
                pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutCnt = 0;
            }

            if (pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutCnt > pstBatteryHandle->stBatteryPack[i].u16WirebreakTimeOutSet)
            {
                pstBatteryHandle->stBatteryPack[i].u16IsOnline = 0;
                pstBatteryHandle->stBatteryPack[i].u16IsInit = 0;
                pstBatteryHandle->stBatteryPack[i].u16PackAddr = 0;
                memset(&(pstBatteryHandle->stBatteryPack[i].stBatteryData), 0, sizeof(ST_BATTERY_DATA));

            }

            pstBatteryHandle->stBatteryPack[i].u16HeartCountBak = pstBatteryHandle->stBatteryPack[i].u16HeartCount;
        }
        if ((pstBatteryHandle->stBatteryPack[i].u16IsInit == 1) && (pstBatteryHandle->stBatteryPack[i].u16IsOnline == 1))
        {
            ++u16OnlineNum;
        }
    }
    pstBatteryHandle->u16OnlineNum = u16OnlineNum;
}

void ParallelCheck(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 u16EquipmentNormalNum = 0;
    Uint16 u16CtrlWorkNum = 0;

    u16EquipmentNormalNum = GetEquipmentNormalNum(pstBatteryHandle);
    u16CtrlWorkNum = GetCtrlWorkNum(pstBatteryHandle);
    u16CtrlWorkNum = u16CtrlWorkNum;

    if (u16EquipmentNormalNum > 1)
    {
        SetParallelMode(pstBatteryHandle, 1);
    }
    else
    {
        SetParallelMode(pstBatteryHandle, 0);
    }
}

Uint16 VersionCheck(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;
    Uint32 u32Version = 0;
    Uint16 u16Flag = 1;

    if (pstBatteryHandle->u16IsParallel)
    {
        u32Version = pstBatteryHandle->stBatteryPack[0].stBatteryData.u32Version;
        for (i = 1; i < MAX_PACK_NUM; i++)
        {
            if (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1)
            {
                if (pstBatteryHandle->stBatteryPack[i].stBatteryData.u32Version != u32Version)
                {
                    u16Flag = 0;
                    break;
                }
            }
        }
    }

    pstBatteryHandle->u16VersionCheckFlag = u16Flag;
    return u16Flag;
}

// 需要考虑调度周期，当发现数值变化时，Vref变化变慢时，对带载是否有影响
// 需要增加一个缓存变量存储Vref，不允许直接修改Vref值
#pragma CODE_SECTION(VBusRegulation, ".ISR.ramfunc");
void VBusRegulation(ST_BATTERY_HANDLE *pstBatteryHandle, LLC_SYSTEM_CONTROL_PARA *pLLCSystemCtrl)
{
    Uint16 u16Index = 0;
    Uint16 u16MaxVBus = 0;
    Uint16 i = 0;

    // 如果是主机，找到最高的母线参考电压，然后广播出去
    // 如果是从机，则需要拿到主机的地址索引，然后将主机的广播的母线电压设置成自己的参考母线电压
    if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_MASTER)
    {
        u16MaxVBus = pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSelfBus;

        for (i = 1; i < MAX_PACK_NUM; i++)
        {
            if ((pstBatteryHandle->stBatteryPack[i].u16IsInit == 1) && (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsEquipmentNormal == 1))
            {
                // 查找最高的母线电压
                if (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bSelfBus > u16MaxVBus)
                {
                    u16MaxVBus = pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bSelfBus;
                }
            }
        }

        // 将控制主机的Vref广播出去
        pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastCtrl_1.bVref = u16MaxVBus;

        pstBatteryHandle->fCtrlVoltage = u16MaxVBus;

    }
    else if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_SLAVE)
    {
        // 寻找主机，然后将他的电压赋值给pLLCSystemCtrl->Vref;
        u16Index = GetMasterDeviceIndex(pstBatteryHandle);
        pstBatteryHandle->fCtrlVoltage = pstBatteryHandle->stBatteryPack[u16Index].stBatteryData.stBroadcastCtrl_1.bVref;
    }
}

Uint16 GetOnlineNum(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;
    Uint16 u16OnlineNum = 0;

    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        if ((pstBatteryHandle->stBatteryPack[i].u16IsOnline == 1))
        {
            ++u16OnlineNum;
        }
    }

    pstBatteryHandle->u16OnlineNum = u16OnlineNum;

    return u16OnlineNum;
}

Uint16 GetEquipmentNormalNum(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;
    Uint16 u16EquipmentNormalNum = 0;

    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        if ((pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsEquipmentNormal == 1) && (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1))
        {
            ++u16EquipmentNormalNum;
        }
    }

    pstBatteryHandle->u16IsEquipmentNormalNum = u16EquipmentNormalNum;

    return u16EquipmentNormalNum;
}

Uint16 GetCtrlWorkNum(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 i = 0;
    Uint16 u16CtrlWorkNum = 0;

    for (i = 0; i < MAX_PACK_NUM; i++)
    {
        if ((pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsSoftFinsh == 1) && (pstBatteryHandle->stBatteryPack[i].u16IsInit == 1))
        {
            ++u16CtrlWorkNum;
        }
    }

    pstBatteryHandle->u16IsCtrlWorkNum = u16CtrlWorkNum;

    return u16CtrlWorkNum;
}

void AutoSoc(ST_BATTERY_HANDLE *pstBatteryHandle, ST_SAMPLE *pObjSample)
{
    float fVbat = pObjSample->VBat;

    if (fVbat > 28.4f)
    {
        pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSoc = 100;
    }
    else if (fVbat < 20.0f)
    {
        pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSoc = 0;
    }

    pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastData_1.bSoc = 100.0f * (fVbat - 20.0f) / (28.5f - 20.0f);
}

void PowerCheck(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    int16 i16Power = 0;
    int16 i16SelfPower = 0;
    int16 i16TotalPower = 0;
    Uint16 i = 0;

    if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_MASTER)
    {
        // 统计总的功率
        for (i = 0; i < MAX_PACK_NUM; i++)
        {
            if ((pstBatteryHandle->stBatteryPack[i].u16IsInit == 1) && (pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_1.bIsEquipmentNormal == 1))
            {
                i16TotalPower += pstBatteryHandle->stBatteryPack[i].stBatteryData.stBroadcastData_2.bPower;
            }
        }

        // 计算分配的功率
        pstBatteryHandle->stBatteryPack[0].stBatteryData.stBroadcastCtrl_1.bTotalPower = i16TotalPower;
        LLCSystemCtrl.PowerTotal = i16TotalPower;
        if(i16TotalPower > 0)
        {
            // 放电
            i16SelfPower = (int16)(((float)LLCSystemCtrl.u16DischargeCoefficient / 1000.0f) * (float)i16TotalPower);
        }
        else
        {
            // 充电
            i16SelfPower = (int16)(((float)LLCSystemCtrl.u16ChargeCoefficient / 1000.0f) * (float)i16TotalPower);
        }

        pstBatteryHandle->i16CtrlPackPower = i16SelfPower;
    }
    else
    {
        // 从机响应主机下发的命令

        i16Power = pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stBroadcastCtrl_1.bTotalPower;
        LLCSystemCtrl.PowerTotal = i16Power;
        if(i16Power > 0)
        {
            // 放电
            i16SelfPower = (int16)(((float)LLCSystemCtrl.u16DischargeCoefficient / 1000.0f) * (float)i16Power);
        }
        else
        {
            // 充电
            i16SelfPower = (int16)(((float)LLCSystemCtrl.u16ChargeCoefficient / 1000.0f) * (float)i16Power);
        }

        pstBatteryHandle->i16CtrlPackPower = i16SelfPower;
    }
}

void DorCCheck(ST_BATTERY_HANDLE *pstBatteryHandle)
{
    Uint16 u16DorCState = 0;
    if (pstBatteryHandle->u16IsMasterSlave == PARALLEL_SLAVE)
    {
        switch (pstBatteryHandle->u16LocalAddr)
        {
        // 地址为1的设备不可能为从机
        case 1:
            break;

        case 2:
            u16DorCState = pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stSpecifiedCtrl_3.bSetDOrCState2;
            break;

        case 3:
            u16DorCState = pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stSpecifiedCtrl_3.bSetDOrCState3;
            break;

        case 4:
            u16DorCState = pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stSpecifiedCtrl_3.bSetDOrCState4;
            break;

        case 5:
            u16DorCState = pstBatteryHandle->stBatteryPack[pstBatteryHandle->u16MasterDeviceIndex].stBatteryData.stSpecifiedCtrl_3.bSetDOrCState5;
            break;
        }

        pstBatteryHandle->u16CtrlDorCState = u16DorCState;
    }
}

#ifdef __cplusplus
    }
#endif

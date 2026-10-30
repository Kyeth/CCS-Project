/**
 * @file
 * @author yetingheng  (004182)
 * @brief
 * @version 0.1
 * @date 2026-03-26
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "FaultProtect.h"
#include "PlatformManager.h"
#include "SysStateMachine.h"
#include "math.h"
#include "LLCControl.h"
#include "ParallelSubFunc.h"
#include "Hw_GpioDriver.h"
#include "Fan.h"
#include "Heating.h"
#include "Schedule.h"

#ifdef __cplusplus
    extern "C" {
#endif

ST_PROTECT stSysProtect;

void SysProtectPointInit(ST_PROTECT *pstSysProtect)
{
    pstSysProtect->unFaultBMS1.u16Word = 0;
    pstSysProtect->unFaultBMS2.u16Word = 0;
    pstSysProtect->unFaultBMS3.u16Word = 0;
    pstSysProtect->unAlarmBMS.u16Word = 0;
    pstSysProtect->u16FaultNum = 0;

    pstSysProtect->unFaultAlarmWord1.u16Word = 0;
    pstSysProtect->unFaultMaskWord1.u16Word = 0;
    pstSysProtect->unFaultMaskWord2.u16Word = 0;
    pstSysProtect->unFaultMaskWord3.u16Word = 2065;
    pstSysProtect->unFaultMaskWord4.u16Word = 0;
    pstSysProtect->unFaultStatusWord1.u16Word = 0;
    pstSysProtect->unFaultStatusWord2.u16Word = 0;
    pstSysProtect->unFaultStatusWord3.u16Word = 0;
    pstSysProtect->unFaultStatusWord4.u16Word = 0;

    // 设置保护点门限
    pstSysProtect->u16VBusOPPPointLevel_1 = 6008;       // 设置功率为6.2kw, 倍率:1, 单位kw
    pstSysProtect->u16VBusOPPPointLevel_2 = 6600;       // 设置功率为6.6kw, 倍率:1, 单位kw
    pstSysProtect->u16VBusOPPPointLevel_3 = 7000;       // 设置功率为7.2kw, 倍率:1, 单位kw
    pstSysProtect->u16VBusOVPPoint = 4800;            // 设置过压门限为480V, 倍率:10, 单位V
    pstSysProtect->u16VBusUVPPoint = 3000;            // 设置欠压门限为480V, 倍率:10, 单位V
    pstSysProtect->u16IBusOCPPoint = 300;             // 设置过流门限为30A, 倍率:10, 单位A
    pstSysProtect->u16IBusUCPPoint = 240;             // 设置过流门限为-24A, 倍率:10, 单位A
    pstSysProtect->u16IBranchECPPoint = 200;           // 设置均流门限为2A, 倍率:10, 单位A
    pstSysProtect->u16VBatUVPPoint = 140;              // 设置欠压门限为14V, 倍率:10, 单位V
    pstSysProtect->u16VBatOVPPoint = 320;              // 设置过压门限为31V, 倍率:10, 单位V
    pstSysProtect->u16VRatioPPointLevel_1 = 120;       // 设置电压变比门限为12.0V,倍率:10,单位V
    pstSysProtect->u16VRatioPPointLevel_2 = 132;       // 设置电压变比门限为13.0V,倍率:10,单位V
    pstSysProtect->u16VRatioPPointLevel_3 = 143;       // 设置电压变比门限为14.5V,倍率:10,单位V
    pstSysProtect->u16LimitCurrentPoint = 20;          // 限制电流门限为1A,倍率:10,单位A (当BMS下发最大允许电流为0时，此时最大允许电流为1A)
    pstSysProtect->u16RadiatorPoint = 79;             // 散热器温度保护门限为73摄氏度，倍率:1,单位摄氏度
    pstSysProtect->u16VLowPipePoint = 93;             // 低压管温度保护门限为85摄氏度，倍率:1,单位摄氏度
    pstSysProtect->u16AmbientPoint = 87;              // 环境温度保护门限为82摄氏度，倍率:1,单位摄氏度
    pstSysProtect->u16TransformerPoint = 93;          // 变压器温度保护门限为81摄氏度，倍率:1,单位摄氏度
    pstSysProtect->u16LimitAirPoint = 50;            // 空开两端电压差保护门限为10V,倍率:10,单位V
    pstSysProtect->u16LimitRelayPoint = 100;          // 继电器两端电压差保护门限为10V,倍率:10,单位V
    pstSysProtect->u16IoscOCPPoint = 4000;            // 谐振电流单点保护门限为4000，倍率:1

    // 设置保护时间,用设置值乘以保护函数的调度周期
    pstSysProtect->u32VBusOPPTimesSetLevel_1 = 1800000;
    pstSysProtect->u32VBusOPPTimesSetLevel_2 = 10000;
    pstSysProtect->u32VBusOPPTimesSetLevel_3 = 1000;         // 表示立刻触发
    pstSysProtect->u32VBusOVPTimesSet = 10;
    pstSysProtect->u32VBusUVPTimesSet = 1000;
    pstSysProtect->u32IBusOCPTimesSet = 5000;
    pstSysProtect->u32IBranchECPTimesSet = 1000;
    pstSysProtect->u32VBatUVPTimesSet = 50;
    pstSysProtect->u32VBatOVPTimesSet = 5;
    pstSysProtect->u32VRatioPTimeSetLevel_1 = 0;
    pstSysProtect->u32VRatioPTimeSetLevel_2 = 50;
    pstSysProtect->u32VRatioPTimeSetLevel_3 = 100;
    pstSysProtect->u32SoftOVPTimesSet = 3500;
    pstSysProtect->u32LimitAirTimesSet = 300;
    pstSysProtect->u32LimitRelayTimesSet = 1000;
    pstSysProtect->u32ParallelVersionCheckTimeSet = 50;
    pstSysProtect->u32LimitCurrentTimesSet = 10;
    pstSysProtect->u32TemperatureTimesSet = 4000;
    pstSysProtect->u32BatAuxUPTimesSet = 100;
    pstSysProtect->u32BusAuxUPTimesSet = 100;
    pstSysProtect->u32IoscOCPTimesSet = 1;
    pstSysProtect->u32FanCheckTimesSet = 10;
    pstSysProtect->u32Heat1TimesSet = 10000;
    pstSysProtect->u32Heat2TimesSet = 10000;
    pstSysProtect->u32Heat3TimesSet = 10000;
    pstSysProtect->u32Heat4TimesSet = 10000;
    // 计数器复位
    pstSysProtect->u32VBusOPPCntLevel_1 = 0;
    pstSysProtect->u32VBusOPPCntLevel_2 = 0;
    pstSysProtect->u32VBusOPPCntLevel_3 = 0;
    pstSysProtect->u32VBusOVPCnt = 0;
    pstSysProtect->u32VBusUVPCnt = 0;
    pstSysProtect->u32IBusOCPCnt = 0;
    pstSysProtect->u32IBranchECPCnt = 0;
    pstSysProtect->u32VBatUVPCnt = 0;
    pstSysProtect->u32VBatOVPCnt = 0;
    pstSysProtect->u32VRatioPCntLevel_1 = 0;
    pstSysProtect->u32VRatioPCntLevel_2 = 0;
    pstSysProtect->u32VRatioPCntLevel_3 = 0;
    pstSysProtect->u32SoftOVPCnt = 0;
    pstSysProtect->u32LimitAirCnt = 0;
    pstSysProtect->u32LimitRelayCnt = 0;
    pstSysProtect->u32ParallelVersionCheckCnt = 0;
    pstSysProtect->u32LimitCurrentCnt = 0;
    pstSysProtect->u32TemperatureCnt = 0;
    pstSysProtect->u32BatAuxUPCnt = 0;
    pstSysProtect->u32BusAuxUPCnt = 0;
    pstSysProtect->u32IoscOCPCnt = 0;
    pstSysProtect->u32FanCheckCnt = 0;
    pstSysProtect->u32Heat1Cnt = 0;
    pstSysProtect->u32Heat2Cnt = 0;
    pstSysProtect->u32Heat3Cnt = 0;
    pstSysProtect->u32Heat4Cnt = 0;
    pstSysProtect->u16FaultReset = 0;
    pstSysProtect->unFaultStatusWord3.u16Word = 0;

    pstSysProtect->u32BusLowTimer = 0;
    pstSysProtect->u32BusLowCheckCnt = 0;

}

void OverVoltageProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord3.Bits.bit6 == 0)         // 先判断是否屏蔽该故障
            && ((Uint32)(ObjSample->VBus*10) > pstSysProtect->u16VBusOVPPoint))      // 再判断是否过压
    {
        // 如果超时高于设定时间
        if (pstSysProtect->u32VBusOVPCnt >= pstSysProtect->u32VBusOVPTimesSet)
        {
            pstSysProtect->unFaultStatusWord3.Bits.bit6  = 1;
            pstSysProtect->u16TotalFaultSign = 1;
            pstSysProtect->u32VBusOVPCnt = 0;
            pstSysProtect->u16SoftFPTakesEffect = 1;
        }
        else
        {
            pstSysProtect->u32VBusOVPCnt += u16CycleMs;
        }
    }
    else
    {
        pstSysProtect->u32VBusOVPCnt = 0;
    }
}

void UnderVoltageProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord3.Bits.bit8 == 0)         // 先判断是否屏蔽该故障
            && ((Uint32)(ObjSample->VBus*10) < pstSysProtect->u16VBusUVPPoint))      // 再判断是否欠压
    {
        // 如果超时高于设定时间
        if (pstSysProtect->u32VBusUVPCnt >= pstSysProtect->u32VBusUVPTimesSet)
        {
            pstSysProtect->unFaultStatusWord3.Bits.bit8 = 1;
            pstSysProtect->u16TotalFaultSign = 1;

            pstSysProtect->u32VBusUVPCnt = 0;
            pstSysProtect->u16SoftFPTakesEffect = 1;
        }
        else
        {
            pstSysProtect->u32VBusUVPCnt += u16CycleMs;
        }
    }
    else
    {
        pstSysProtect->u32VBusUVPCnt = 0;
    }
}

void OverLoadProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    float fPower = 0;

    fPower = ObjSample->VBusFilter1 * ObjSample->IBus;

    if (pstSysProtect->unFaultMaskWord3.Bits.bit7 == 0)      // 先判断是否屏蔽该故障
    {
        if ((Uint32)fPower > pstSysProtect->u16VBusOPPPointLevel_3)    // 判断是否达到3级过载
        {
            if (pstSysProtect->u32VBusOPPCntLevel_3 >= pstSysProtect->u32VBusOPPTimesSetLevel_3)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit7 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32VBusOPPCntLevel_3 = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;

            }
            else
            {
                pstSysProtect->u32VBusOPPCntLevel_3 += u16CycleMs;
            }

        }
        else if ((Uint32)fPower > pstSysProtect->u16VBusOPPPointLevel_2)   // 判断是否达到2级过载
        {
            if (pstSysProtect->u32VBusOPPCntLevel_2 >= pstSysProtect->u32VBusOPPTimesSetLevel_2)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit7 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32VBusOPPCntLevel_2 = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;

            }
            else
            {
                pstSysProtect->u32VBusOPPCntLevel_2 += u16CycleMs;
            }

        }
        else if ((Uint32)fPower > pstSysProtect->u16VBusOPPPointLevel_1)   // 判断是否达到1级过载
        {
            if (pstSysProtect->u32VBusOPPCntLevel_1 >= pstSysProtect->u32VBusOPPTimesSetLevel_1)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit7 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32VBusOPPCntLevel_1 = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;

            }
            else
            {
                pstSysProtect->u32VBusOPPCntLevel_1 += u16CycleMs;
            }

        }
        else
        {
            pstSysProtect->u32VBusOPPCntLevel_1 = 0;
            pstSysProtect->u32VBusOPPCntLevel_2 = 0;
            pstSysProtect->u32VBusOPPCntLevel_3 = 0;
        }
    }

}

void IoscErrorProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord2.Bits.bit14 == 0)      // 先判断是否屏蔽该故障
    {
        if ((ObjPlatformCtrl.u16AdcARawResult[11] > pstSysProtect->u16IoscOCPPoint)||(ObjPlatformCtrl.u16AdcCRawResult[2] > pstSysProtect->u16IoscOCPPoint))
        {
            if (pstSysProtect->u32IoscOCPCnt >= pstSysProtect->u32IoscOCPTimesSet)
            {
                pstSysProtect->unFaultStatusWord2.Bits.bit14 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32IoscOCPCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32IoscOCPCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32IoscOCPCnt = 0;
            if (pstSysProtect->u32IoscUCPCnt >= pstSysProtect->u32IoscUCPTimesSet)
            {
                pstSysProtect->u32IoscUCPCnt = 0;
                pstSysProtect->u16IoscCheckFinish = 1;
            }
            else
            {
                pstSysProtect->u32IoscUCPCnt += u16CycleMs;
            }
        }
    }
}

void OverCurrentProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord3.Bits.bit5 == 0)      // 先判断是否屏蔽该故障
    {
        if (((ObjSample->IBus * 10) > pstSysProtect->u16IBusOCPPoint)||((ObjSample->IBus * -10) > pstSysProtect->u16IBusUCPPoint))
        {
            if (pstSysProtect->u32IBusOCPCnt >= pstSysProtect->u32IBusOCPTimesSet)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit5 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32IBusOCPCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32IBusOCPCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32IBusOCPCnt = 0;
        }
    }
}

void EqualCurrentProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord3.Bits.bit4 == 0)      // 先判断是否屏蔽该故障
    {
        if ((Uint32)((fabsf(ObjSample->IBranch_1 - ObjSample->IBranch_2)) *10) > pstSysProtect->u16IBranchECPPoint)
        {
            if (pstSysProtect->u32IBranchECPCnt >= pstSysProtect->u32IBranchECPTimesSet)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit4 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32IBranchECPCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;

            }
            else
            {
                pstSysProtect->u32IBranchECPCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32IBranchECPCnt = 0;
        }
    }
}

void VbatUnderVoltageProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord3.Bits.bit9 == 0)         // 先判断是否屏蔽该故障
            && ((Uint32)(ObjSample->VBatRaw*10) < pstSysProtect->u16VBatUVPPoint))      // 再判断是否欠压
    {
        // 如果超时高于设定时间
        if (pstSysProtect->u32VBatUVPCnt >= pstSysProtect->u32VBatUVPTimesSet)
        {
            pstSysProtect->unFaultStatusWord3.Bits.bit9 = 1;
            pstSysProtect->u16TotalFaultSign = 1;

            pstSysProtect->u32VBatUVPCnt = 0;
            pstSysProtect->u16SoftFPTakesEffect = 1;

        }
        else
        {
            pstSysProtect->u32VBatUVPCnt += u16CycleMs;
        }
    }
    else
    {
        pstSysProtect->u32VBatUVPCnt = 0;
    }
}

void VbatOverVoltageProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord3.Bits.bit10 == 0)         // 先判断是否屏蔽该故障
            && ((Uint32)(ObjSample->VBatRaw*10) > pstSysProtect->u16VBatOVPPoint))      // 再判断是否欠压
    {
        // 如果超时高于设定时间
        if (pstSysProtect->u32VBatOVPCnt >= pstSysProtect->u32VBatOVPTimesSet)
        {
            pstSysProtect->unFaultStatusWord3.Bits.bit10 = 1;
            pstSysProtect->u16TotalFaultSign = 1;

            pstSysProtect->u32VBatOVPCnt = 0;
            pstSysProtect->u16SoftFPTakesEffect = 1;
        }
        else
        {
            pstSysProtect->u32VBatOVPCnt += u16CycleMs;
        }
    }
    else
    {
        pstSysProtect->u32VBatOVPCnt = 0;
    }
}

void VoltageRatioProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    Uint32 u32Ratio = 0;

    u32Ratio = (Uint32)(ObjSample->VBus / ObjSample->VBat) * 10;

    if (pstSysProtect->unFaultMaskWord3.Bits.bit11 == 0)
    {
        if (u32Ratio < pstSysProtect->u16VRatioPPointLevel_1)
        {
            if (pstSysProtect->u32VRatioPCntLevel_1 >= pstSysProtect->u32VRatioPTimeSetLevel_1)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit11 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32VRatioPCntLevel_1 = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                // 如果变比小于最苛刻的状态，那么次级状态也满足
                pstSysProtect->u32VRatioPCntLevel_1 += u16CycleMs;
                pstSysProtect->u32VRatioPCntLevel_2 += u16CycleMs;
                pstSysProtect->u32VRatioPCntLevel_3 += u16CycleMs;
            }
        }
        else if (u32Ratio < pstSysProtect->u16VRatioPPointLevel_2)
        {
            if (pstSysProtect->u32VRatioPCntLevel_2 >= pstSysProtect->u32VRatioPTimeSetLevel_2)
            {
                pstSysProtect->u32VRatioPCntLevel_2 = 0;
            }
            else
            {
                pstSysProtect->u32VRatioPCntLevel_2 += u16CycleMs;
                pstSysProtect->u32VRatioPCntLevel_3 += u16CycleMs;
            }
        }
        else if (u32Ratio < pstSysProtect->u16VRatioPPointLevel_3)
        {
            if (pstSysProtect->u32VRatioPCntLevel_3 >= pstSysProtect->u32VRatioPTimeSetLevel_3)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit11 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32VRatioPCntLevel_3 = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32VRatioPCntLevel_3 += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32VRatioPCntLevel_1 = 0;
            pstSysProtect->u32VRatioPCntLevel_2 = 0;
            pstSysProtect->u32VRatioPCntLevel_3 = 0;
        }
    }
    else
    {
        pstSysProtect->u32VRatioPCntLevel_1 = 0;
        pstSysProtect->u32VRatioPCntLevel_2 = 0;
        pstSysProtect->u32VRatioPCntLevel_3 = 0;
    }
}

void ParallelVersionCheck(ST_PROTECT *pstSysProtect, ST_BATTERY_HANDLE *pstBatteryHandle, Uint16 u16CycleMs)
{
    Uint16 u16Flag = 0;

    u16Flag = GetVersionCheckFlag(pstBatteryHandle);

    if (pstSysProtect->unFaultMaskWord3.Bits.bit12 == 0)
    {
        if (u16Flag == 0)
        {
            if (pstSysProtect->u32ParallelVersionCheckCnt >= pstSysProtect->u32ParallelVersionCheckTimeSet)
            {
                pstSysProtect->u16TotalFaultSign = 1;
                pstSysProtect->unFaultStatusWord3.Bits.bit12 = 1;

                pstSysProtect->u32ParallelVersionCheckCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32ParallelVersionCheckCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32ParallelVersionCheckCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32ParallelVersionCheckCnt = 0;
    }
}

void CirCuitBreakProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord2.Bits.bit2 == 0))
    {
        if ((fabsf(ObjSample->VAirFront * 10 - ObjSample->VAirBack * 10)) > pstSysProtect->u16LimitAirPoint)
        {
            if (pstSysProtect->u32LimitAirCnt >= pstSysProtect->u32LimitAirTimesSet)
            {
                pstSysProtect->unFaultStatusWord2.Bits.bit2 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32LimitAirCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32LimitAirCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32LimitAirCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32LimitAirCnt = 0;
    }
}

void RelayBreakProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord2.Bits.bit3 == 0))
    {
        if ((fabsf(ObjSample->VAirFront * 10 - ObjSample->VBusRaw * 10)) > pstSysProtect->u16LimitRelayPoint)
        {
            if (pstSysProtect->u32LimitRelayCnt >= pstSysProtect->u32LimitRelayTimesSet)
            {
                pstSysProtect->unFaultStatusWord2.Bits.bit3 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32LimitRelayCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32LimitRelayCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32LimitRelayCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32LimitRelayCnt = 0;
    }
}

void SoftLongTimeProtection(ST_PROTECT *pstSysProtect, Uint16 u16CycleMs)
{
    if ((pstSysProtect->unFaultMaskWord3.Bits.bit13 == 0))
    {
        // 如果超时高于设定时间
        if (pstSysProtect->u32SoftOVPCnt >= pstSysProtect->u32SoftOVPTimesSet)
        {
            pstSysProtect->unFaultStatusWord3.Bits.bit13 = 1;
            pstSysProtect->u16TotalFaultSign = 1;

            pstSysProtect->u32SoftOVPCnt = 0;
            pstSysProtect->u16SoftFPTakesEffect = 1;

        }
        else
        {
            pstSysProtect->u32SoftOVPCnt += u16CycleMs;
        }
    }
    else
    {
        pstSysProtect->u32SoftOVPCnt = 0;
    }
}

#define VBUS_CHECK_SLOPE          (0.15f) //(0.15f) //(0.18f) //(0.20f) //(0.29f)
#define VBUS_CHECK_VOLT_MAX       (VBUS_CHECK_SLOPE * 1000)

void BusSoftStartProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    float fDesireVbusVolt;

    pstSysProtect->u32BusLowTimer += u16CycleMs;
    if (pstSysProtect->u32BusLowTimer > 1000) // 1000 * 1ms = 1s
    {
        pstSysProtect->u32BusLowTimer = 1000;
    }

    fDesireVbusVolt = (float)pstSysProtect->u32BusLowTimer * VBUS_CHECK_SLOPE;
    if (fDesireVbusVolt < 0)
    {
        fDesireVbusVolt = 0;
    }
    else if (fDesireVbusVolt > VBUS_CHECK_VOLT_MAX)
    {
        fDesireVbusVolt = VBUS_CHECK_VOLT_MAX;
    }

    // VBUS voltage check
    if (ObjSample->VBusRaw > fDesireVbusVolt)
    {
        if (pstSysProtect->u32BusLowCheckCnt > 0)
        {
            pstSysProtect->u32BusLowCheckCnt--;
        }
    }
    else
    {
        pstSysProtect->u32BusLowCheckCnt++;
    }

    // Bus voltage Low timer out
    if (pstSysProtect->u32BusLowCheckCnt >= 60) // 60ms
    {
        pstSysProtect->u16TotalFaultSign = 1;
        pstSysProtect->unFaultStatusWord2.Bits.bit9 = 1;

        pstSysProtect->u32BusLowTimer = 0;
        pstSysProtect->u32BusLowCheckCnt = 0;
        pstSysProtect->u16SoftFPTakesEffect = 1;
    }
}

void LimitCurrentProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord3.Bits.bit14 == 0)
    {
        if ((ObjSample->IBus*10) > pstSysProtect->u16LimitCurrentPoint)
        {
            if (pstSysProtect->u32LimitCurrentCnt >= pstSysProtect->u32LimitCurrentTimesSet)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit14 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32LimitCurrentCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32LimitCurrentCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32LimitCurrentCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32LimitCurrentCnt = 0;
    }
}

void OverTemperatureProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord3.Bits.bit15 == 0)
    {
        if ((ObjSample->VNTC_2 > pstSysProtect->u16RadiatorPoint)
                || (ObjSample->VEnvironment > pstSysProtect->u16AmbientPoint)
                || (ObjSample->VTransformer_1 > pstSysProtect->u16VLowPipePoint)
                || (ObjSample->VTransformer_2 > pstSysProtect->u16TransformerPoint))
        {
            if (pstSysProtect->u32TemperatureCnt >= pstSysProtect->u32TemperatureTimesSet)
            {
                pstSysProtect->unFaultStatusWord3.Bits.bit15 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32TemperatureCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32TemperatureCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32TemperatureCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32TemperatureCnt = 0;
    }
}

void BatAuxSrcProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord2.Bits.bit7 == 0)      // 先判断是否屏蔽该故障
    {
        if (Hw_GpioReadPin(17) == 0)
        {
            if (pstSysProtect->u32BatAuxUPCnt >= pstSysProtect->u32BatAuxUPTimesSet)
            {
                pstSysProtect->unFaultStatusWord2.Bits.bit7 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32BatAuxUPCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32BatAuxUPCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32BatAuxUPCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32BatAuxUPCnt = 0;
    }
}

void BusAuxSrcProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if (pstSysProtect->unFaultMaskWord2.Bits.bit8 == 0)      // 先判断是否屏蔽该故障
    {
        if (Hw_GpioReadPin(40) == 0)
        {
            if (pstSysProtect->u32BusAuxUPCnt >= pstSysProtect->u32BusAuxUPTimesSet)
            {
                pstSysProtect->unFaultStatusWord2.Bits.bit8 = 1;
                pstSysProtect->u16TotalFaultSign = 1;

                pstSysProtect->u32BusAuxUPCnt = 0;
                pstSysProtect->u16SoftFPTakesEffect = 1;
            }
            else
            {
                pstSysProtect->u32BusAuxUPCnt += u16CycleMs;
            }
        }
        else
        {
            pstSysProtect->u32BusAuxUPCnt = 0;
        }
    }
    else
    {
        pstSysProtect->u32BusAuxUPCnt = 0;
    }
}

void FanCheck(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if(ObjSystemCtrl.u16FanFlag == 0)
    {
        stFanHandle.u16FanSpeedLevel = 1;
        if (pstSysProtect->u32FanCheckCnt >= pstSysProtect->u32FanCheckTimesSet)
        {
            if (Hw_GpioReadPin(20) == 0)
            {
               pstSysProtect->unFaultAlarmWord1.Bits.bit7 = 1;
            }
            else
            {
                pstSysProtect->unFaultAlarmWord1.Bits.bit7 = 0;
            }
            if (Hw_GpioReadPin(21) == 0)
            {
               pstSysProtect->unFaultAlarmWord1.Bits.bit8 = 1;
            }
            else
            {
                pstSysProtect->unFaultAlarmWord1.Bits.bit8 = 0;
            }
            ObjSystemCtrl.u16FanFlag = 1;
            stFanHandle.u16FanSpeedLevel = 0;
        }
        else
        {
            pstSysProtect->u32FanCheckCnt += u16CycleMs;
        }
    }
}

void FANWarning(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    if(ObjSystemCtrl.u16FanFlag == 1)
    {
        if(stFanHandle.u16FanSpeedLevel > 0)
        {
            if (Hw_GpioReadPin(20) == 0)
            {
               pstSysProtect->unFaultAlarmWord1.Bits.bit7 = 1;
            }
            else
            {
                pstSysProtect->unFaultAlarmWord1.Bits.bit7 = 0;
            }
            if (Hw_GpioReadPin(21) == 0)
            {
               pstSysProtect->unFaultAlarmWord1.Bits.bit8 = 1;
               pstSysProtect->u16RadiatorPoint = 68;             // 散热器温度保护门限为68摄氏度，倍率:1,单位摄氏度
               pstSysProtect->u16VLowPipePoint = 85;             // 低压管温度保护门限为85摄氏度，倍率:1,单位摄氏度
               pstSysProtect->u16AmbientPoint = 81;              // 环境温度保护门限为81摄氏度，倍率:1,单位摄氏度
               pstSysProtect->u16TransformerPoint = 81;          // 变压器温度保护门限为81摄氏度，倍率:1,单位摄氏度
            }
            else
            {
                pstSysProtect->unFaultAlarmWord1.Bits.bit8 = 0;
                pstSysProtect->u16RadiatorPoint = 79;             // 散热器温度保护门限为68摄氏度，倍率:1,单位摄氏度
                pstSysProtect->u16VLowPipePoint = 93;             // 低压管温度保护门限为85摄氏度，倍率:1,单位摄氏度
                pstSysProtect->u16AmbientPoint = 87;              // 环境温度保护门限为81摄氏度，倍率:1,单位摄氏度
                pstSysProtect->u16TransformerPoint = 93;          // 变压器温度保护门限为81摄氏度，倍率:1,单位摄氏度
            }
        }
        else
        {
            // 挡位为0时，Fault引脚电平为0，因此这里不处理
        }
    }
}

void HeatingFilmProtection(ST_PROTECT *pstSysProtect, ST_SAMPLE *ObjSample, Uint16 u16CycleMs)
{
    float Resistance = 0;
    if (pstSysProtect->unFaultMaskWord2.Bits.bit15 == 0)      // 先判断是否屏蔽该故障
    {
        Resistance = ObjSample->VBus / ObjSample->Heating;
        // 1.The heater film has no drive signal, but current was detected
        if(stHeatingHandle.u16HeatingLevelBak == 0)
        {
            if(ObjSample->Heating > 0.05f)
            {
                if (pstSysProtect->u32Heat1Cnt >= pstSysProtect->u32Heat1TimesSet)
                {
                    pstSysProtect->unFaultStatusWord2.Bits.bit15 = 1;
                    pstSysProtect->u16TotalFaultSign = 1;

                    pstSysProtect->u32Heat1Cnt = 0;
                    pstSysProtect->u16SoftFPTakesEffect = 1;
                }
                else
                {
                    pstSysProtect->u32Heat1Cnt += u16CycleMs;
                }
            }
            else
            {
                pstSysProtect->u32Heat1Cnt = 0;
            }
        }
        else
        {
            // 2.The heating film driver is on, but no current is detected
            if(ObjSample->Heating < 0.05f)
            {
                if (pstSysProtect->u32Heat2Cnt >= pstSysProtect->u32Heat2TimesSet)
                {
                    pstSysProtect->unFaultStatusWord2.Bits.bit15 = 1;
                    pstSysProtect->u16TotalFaultSign = 1;

                    pstSysProtect->u32Heat2Cnt = 0;
                    pstSysProtect->u16SoftFPTakesEffect = 1;
                }
                else
                {
                    pstSysProtect->u32Heat2Cnt += u16CycleMs;
                }
            }
            else
            {
                pstSysProtect->u32Heat2Cnt = 0;
            }
            // 3.The resistance values corresponding to the heating film at different heating levels are out of range
            switch(stHeatingHandle.u16HeatingLevelBak)
            {
            case 1:
            case 2:
            {
                // 1330R±5%
                if((Resistance < 1200.0f)||(Resistance > 1400.0f))
                {
                    if (pstSysProtect->u32Heat3Cnt >= pstSysProtect->u32Heat3TimesSet)
                    {
                        pstSysProtect->unFaultStatusWord2.Bits.bit15 = 1;
                        pstSysProtect->u16TotalFaultSign = 1;

                        pstSysProtect->u32Heat3Cnt = 0;
                        pstSysProtect->u16SoftFPTakesEffect = 1;
                    }
                    else
                    {
                        pstSysProtect->u32Heat3Cnt += u16CycleMs;
                    }
                }
                break;
            }
            case 3:
            {
                // 665R±5%
                if((Resistance < 600.0f)||(Resistance > 700.0f))
                {
                    if (pstSysProtect->u32Heat4Cnt >= pstSysProtect->u32Heat4TimesSet)
                    {
                        pstSysProtect->unFaultStatusWord2.Bits.bit15 = 1;
                        pstSysProtect->u16TotalFaultSign = 1;

                        pstSysProtect->u32Heat4Cnt = 0;
                        pstSysProtect->u16SoftFPTakesEffect = 1;
                    }
                    else
                    {
                        pstSysProtect->u32Heat4Cnt += u16CycleMs;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }
    else
    {
        pstSysProtect->u32Heat1Cnt = 0;
    }
}

void FaultBitUpdateToBms(ST_PROTECT *pstSysProtect)
{
    // unFaultBMS1 Update
    pstSysProtect->unFaultBMS1.Bits.EepromFault = 0;
    pstSysProtect->unFaultBMS1.Bits.BusSoftStartFail = pstSysProtect->unFaultStatusWord3.Bits.bit13; // 软启超时
    pstSysProtect->unFaultBMS1.Bits.Null1 = pstSysProtect->unFaultStatusWord3.Bits.bit7;  // 过载保护
    pstSysProtect->unFaultBMS1.Bits.Null2 = pstSysProtect->unFaultStatusWord3.Bits.bit11; // 变比保护

    pstSysProtect->unFaultBMS1.Bits.Null3 = pstSysProtect->unFaultStatusWord3.Bits.bit14; // 禁止带载保护
    pstSysProtect->unFaultBMS1.Bits.BmsCommFault = 0;
    pstSysProtect->unFaultBMS1.Bits.HvRelayFail = pstSysProtect->unFaultStatusWord2.Bits.bit2; // 空开继电器保护
    pstSysProtect->unFaultBMS1.Bits.Null4 = pstSysProtect->unFaultStatusWord2.Bits.bit1; //吸电保护（继电器粘连）

    pstSysProtect->unFaultBMS1.Bits.Null5 = pstSysProtect->unFaultStatusWord2.Bits.bit11; //第一路谐振电流采样断路故障
    pstSysProtect->unFaultBMS1.Bits.Null6 = pstSysProtect->unFaultStatusWord2.Bits.bit12; //第二路谐振电流采样断路故障
    pstSysProtect->unFaultBMS1.Bits.Null7 = pstSysProtect->unFaultStatusWord2.Bits.bit14; //谐振电流3V单点故障
    pstSysProtect->unFaultBMS1.Bits.ExtRelayFail = pstSysProtect->unFaultStatusWord2.Bits.bit3; // 继电器驱动保护

    pstSysProtect->unFaultBMS1.Bits.BusPreChgFail = pstSysProtect->unFaultStatusWord2.Bits.bit0; // 预充超时保护
    pstSysProtect->unFaultBMS1.Bits.HardwareZeroFail = pstSysProtect->unFaultStatusWord2.Bits.bit10; // IBus采样零漂异常
    pstSysProtect->unFaultBMS1.Bits.HardwareSpsFail = pstSysProtect->unFaultStatusWord2.Bits.bit7 | \
                                                      pstSysProtect->unFaultStatusWord2.Bits.bit8; // 低压辅源欠压 || 高压辅源欠压
    pstSysProtect->unFaultBMS1.Bits.HeatFail = 0;

    // unFaultBMS2 Update
    pstSysProtect->unFaultBMS2.Bits.Hd_DischgCurrOver1 = pstSysProtect->unFaultStatusWord3.Bits.bit1; // 谐振腔1过流
    pstSysProtect->unFaultBMS2.Bits.Hd_ChgCurrOver1 = pstSysProtect->unFaultStatusWord3.Bits.bit1;    // 谐振腔1过流
    pstSysProtect->unFaultBMS2.Bits.Hd_DischgCurrOver2 = pstSysProtect->unFaultStatusWord3.Bits.bit2; // 谐振腔2过流
    pstSysProtect->unFaultBMS2.Bits.Hd_ChgCurrOver2 = pstSysProtect->unFaultStatusWord3.Bits.bit2;    // 谐振腔2过流

    pstSysProtect->unFaultBMS2.Bits.SF_DischgCurrOver1 = pstSysProtect->unFaultStatusWord3.Bits.bit4; // 谐振腔均流 > 2A
    pstSysProtect->unFaultBMS2.Bits.SF_ChgCurrOver1 = pstSysProtect->unFaultStatusWord3.Bits.bit4;    // 谐振腔均流 > 2A
    pstSysProtect->unFaultBMS2.Bits.SF_DischgCurrOver2 = pstSysProtect->unFaultStatusWord3.Bits.bit4; // 谐振腔均流 > 2A
    pstSysProtect->unFaultBMS2.Bits.SF_ChgCurrOver2 = pstSysProtect->unFaultStatusWord3.Bits.bit4;    // 谐振腔均流 > 2A

    pstSysProtect->unFaultBMS2.Bits.Hd_BusCurrOver = pstSysProtect->unFaultStatusWord3.Bits.bit3;        // IBUS过流(硬件)
    pstSysProtect->unFaultBMS2.Bits.SF_BusCurrOver_Chg = pstSysProtect->unFaultStatusWord3.Bits.bit5;    // IBUS过流(软件)
    pstSysProtect->unFaultBMS2.Bits.SF_BusCurrOver_DisChg = pstSysProtect->unFaultStatusWord3.Bits.bit5; // IBUS过流(软件)
    pstSysProtect->unFaultBMS2.Bits.SopOverTimeOut = pstSysProtect->unFaultStatusWord2.Bits.bit9; // BUS电压软启动失败

    pstSysProtect->unFaultBMS2.Bits.McanCommFail = 0;
    pstSysProtect->unFaultBMS2.Bits.RelayOpenFail = pstSysProtect->unFaultStatusWord2.Bits.bit2 | \
                                                    pstSysProtect->unFaultStatusWord2.Bits.bit5; // 继电器粘连
    pstSysProtect->unFaultBMS2.Bits.DriverFail1 = pstSysProtect->unFaultStatusWord2.Bits.bit3;   // 继电器驱动保护
    pstSysProtect->unFaultBMS2.Bits.DriverFail2 = pstSysProtect->unFaultStatusWord3.Bits.bit0;   // 软件驱动故障

    // unFaultBMS3 Update
    pstSysProtect->unFaultBMS3.Bits.BmsOverVolt = 0;
    pstSysProtect->unFaultBMS3.Bits.BmsUnderrVolt = 0;
    pstSysProtect->unFaultBMS3.Bits.BusOverVolt =  pstSysProtect->unFaultStatusWord3.Bits.bit6; // VBUS过压
    pstSysProtect->unFaultBMS3.Bits.BusUnderVolt = pstSysProtect->unFaultStatusWord3.Bits.bit8; // VBUS欠压

    pstSysProtect->unFaultBMS3.Bits.BusOverVolt_LV = pstSysProtect->unFaultStatusWord3.Bits.bit9; // VBAT欠压
    pstSysProtect->unFaultBMS3.Bits.BusUnderVolt_LV = pstSysProtect->unFaultStatusWord3.Bits.bit10; // VBAT过压
    pstSysProtect->unFaultBMS3.Bits.HeatSinkTempFail = 0;
    pstSysProtect->unFaultBMS3.Bits.Inside1TempFail = pstSysProtect->unFaultStatusWord2.Bits.bit13; // 温度NTC断线故障

    pstSysProtect->unFaultBMS3.Bits.Inside2TempFail = 0;
    pstSysProtect->unFaultBMS3.Bits.InductorTempFail = 0;
    pstSysProtect->unFaultBMS3.Bits.BatPosTempFail = 0;
    pstSysProtect->unFaultBMS3.Bits.BatNegTempFail = 0;

    pstSysProtect->unFaultBMS3.Bits.VoutNegTempFail = 0;
    pstSysProtect->unFaultBMS3.Bits.Null1 = pstSysProtect->unFaultStatusWord3.Bits.bit12; // 软件版本故障
    pstSysProtect->unFaultBMS3.Bits.Null2 = 0;
    pstSysProtect->unFaultBMS3.Bits.Null3 = 0;

    // unAlarmBMS Update
    pstSysProtect->unAlarmBMS.Bits.HeatResistorFail = 0;
    pstSysProtect->unAlarmBMS.Bits.HeatCurrSmall = 0;
    pstSysProtect->unAlarmBMS.Bits.HeatOverCurr = 0;
    pstSysProtect->unAlarmBMS.Bits.OverTemp = pstSysProtect->unFaultStatusWord3.Bits.bit15; // 过温保护

    pstSysProtect->unAlarmBMS.Bits.EepromFault_Others = 0;
    pstSysProtect->unAlarmBMS.Bits.CanCommFail_DG = 0;
    pstSysProtect->unAlarmBMS.Bits.RS485CommFail_DG = 0;
    pstSysProtect->unAlarmBMS.Bits.Null1 = pstSysProtect->unFaultAlarmWord1.Bits.bit7;   // 外部风扇故障

    pstSysProtect->unAlarmBMS.Bits.Null2 = pstSysProtect->unFaultAlarmWord1.Bits.bit8;   // 内部风扇故障
    pstSysProtect->unAlarmBMS.Bits.Null3 = 0;
    pstSysProtect->unAlarmBMS.Bits.Null4 = 0;
    pstSysProtect->unAlarmBMS.Bits.Null5 = 0;

    pstSysProtect->unAlarmBMS.Bits.Null6 = 0;
    pstSysProtect->unAlarmBMS.Bits.Null7 = 0;
    pstSysProtect->unAlarmBMS.Bits.Null8 = 0;
    pstSysProtect->unAlarmBMS.Bits.Null9 = 0;

    pstSysProtect->u16FaultNum = 0;
}

void SetTaskFaultBit(void)
{
    stSysProtect.u16TotalFaultSign = 1;
    stSysProtect.unFaultStatusWord3.Bits.bit0 = 1;
}

void TaskFaultDetect(void)
{
    Uint16 u16TaskCyc = 1;      // 任务调度周期
    E_SYS_STATE eSysState = SYS_STATE_IDLE;

    eSysState = GetSysMachineState(&ObjSystemCtrl);     // 获取系统状态
    switch (eSysState)
    {
        case SYS_STATE_CHECK:
            IoscErrorProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            break;
        case SYS_STATE_NORMAL_OPERATION:
            FanCheck(&stSysProtect, &ObjSample, u16TaskCyc);
            CirCuitBreakProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverLoadProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverCurrentProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            EqualCurrentProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            VbatUnderVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            VbatOverVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverTemperatureProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            BatAuxSrcProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            FANWarning(&stSysProtect, &ObjSample, u16TaskCyc);
            HeatingFilmProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            if (GetLLCState(&ObjSystemCtrl) > SYS_STATE_SOFT_START)
            {
                VoltageRatioProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            }

            if (GetLLCState(&ObjSystemCtrl) == LLC_SOFT_WAIT_STATE)
            {
                SoftLongTimeProtection(&stSysProtect, u16TaskCyc);
                BusSoftStartProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            }
            else
            {
                stSysProtect.u32SoftOVPCnt = 0;
                stSysProtect.u32BusLowTimer = 0;
                stSysProtect.u32BusLowCheckCnt = 0;
            }

            if ((GetLLCState(&ObjSystemCtrl) > LLC_SOFT_WAIT_STATE)&&(GetLLCState(&ObjSystemCtrl) < LLC_AGING_STATE))
            {
                RelayBreakProtection(&stSysProtect, &ObjSample, u16TaskCyc);
                UnderVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
                if(++stSysProtect.u16BusAuxDelayCnt >= 1000)
                {
                    stSysProtect.u16BusAuxDelayCnt = 1000;
                    BusAuxSrcProtection(&stSysProtect, &ObjSample, u16TaskCyc);
                }
            }
            else
            {
                stSysProtect.u16BusAuxDelayCnt = 0;
            }

            if ((GetLLCState(&ObjSystemCtrl) == LLC_DISCHARGE_STATE) && (LLCSystemCtrl.u16DischargeCurrentAllowMax == 0))
            {
                LimitCurrentProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            }
            break;

        case SYS_STATE_POWER_RESTRICTION:       // 在低功耗模式下不做均流保护
            OverVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            UnderVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverLoadProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            OverCurrentProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            VbatUnderVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            VbatOverVoltageProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            if (GetLLCState(&ObjSystemCtrl) > SYS_STATE_SOFT_START)
            {
                VoltageRatioProtection(&stSysProtect, &ObjSample, u16TaskCyc);
            }
            break;
    }

//    ParallelVersionCheck(&stSysProtect, &stBatteryHandle, u16TaskCyc);

    if (stSysProtect.u16FaultReset)
    {
        stSysProtect.unFaultAlarmWord1.u16Word = 0;
        stSysProtect.unFaultStatusWord1.u16Word = 0;
        stSysProtect.unFaultStatusWord2.u16Word = 0;
        stSysProtect.unFaultStatusWord3.u16Word = 0;
        stSysProtect.unFaultStatusWord4.u16Word = 0;
        stSysProtect.u16SoftFPTakesEffect = 0;
        stSysProtect.u16HardFPTakesEffect = 0;
        stSysProtect.u16FaultReset = 0;
        stSysProtect.u16TotalFaultSign = 0;
        ObjSystemCtrl.unSysCtrlEnable.u16Word = 0;
        ObjSystemCtrl.Open = 0;
    }
    FaultBitUpdateToBms(&stSysProtect);
    SetTaskRunStatus(TASK_FAULT_DETECT_BIT);
}

#ifdef __cplusplus
    }
#endif

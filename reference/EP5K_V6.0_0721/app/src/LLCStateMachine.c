/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: LLCStateMachine.c
Author: tzz
Date:2022年2月15日
Description: LLC状态机控制文件
Version:0.0.2
Function List: 1.LLC变量初始化;2.LLC软启动
***********************************************************/
#include <string.h>
#include "bsp.h"
#include "Sample.h"
#include "Protect.h"
#include "LLCStateMachine.h"
#include "ConfigSystemPara.h"
#include "InvStateMachine.h"
#include "ModbusTable.h"
#include "User_ISOCal.h"
#include "OnGridInv2.h"

LLC_STATE_DATA LLCWorkState;

LLC_ADJUST_DATA LLCAdjustData1, LLCAdjustData2;         // 低压侧
LLC_ADJUST_DATA LLCAdjustData11, LLCAdjustData22;       // 高压侧
/*************************************************
Function: InitLLCAdjustData
Description: 修改LLC开关频率
Input: None
Output: None
Frequency: Once
*************************************************/
void InitLLCAdjustData(void)
{
    memset(&LLCAdjustData1, 0, sizeof(LLC_ADJUST_DATA));
    memset(&LLCAdjustData2, 0, sizeof(LLC_ADJUST_DATA));
    memset(&LLCAdjustData11, 0, sizeof(LLC_ADJUST_DATA));
    memset(&LLCAdjustData22, 0, sizeof(LLC_ADJUST_DATA));
    memset(&LLCWorkState, 0, sizeof(LLC_STATE_DATA));

    // LLC1低压侧
    LLCAdjustData1.SoftFinishIinterval = 60;
    LLCAdjustData1.SoftStartPeriod = 125;   // 250;       // 每 25ms PWM 递增一次,递增到 50% 需要 25ms*300 时间
    LLCAdjustData1.LLCPwmPeriod = (uint16_t)(LLC_EPWM_PERIOD);
    LLCAdjustData1.LLCPwmHalfPeriod = LLCAdjustData1.LLCPwmPeriod/2;
    LLCAdjustData1.LLCDeathTime = 0.4f;         // 死区0.3us-0.35(450n)->0.37-0.23(350)->0.45(580n)
    // 死区计数偏移值10,如果无偏移LLC软启动时会出现PWM断续的情况    // 0.12(200ns)
    LLCAdjustData1.LLCDeathTimeCount = (LLCAdjustData1.LLCDeathTime*100/1.0f + 10.0f);
    LLCAdjustData1.LLCID = 1;

    // LLC1高压侧
    LLCAdjustData11 = LLCAdjustData1;
    LLCAdjustData11.LLCDeathTime = 0.35f;        // 死区0.3us
    LLCAdjustData11.LLCDeathTimeCount = (LLCAdjustData11.LLCDeathTime*100/1.0f + 10.0f);
    LLCAdjustData11.LLCID = 3;

    // LLC2低压侧
    LLCAdjustData2 = LLCAdjustData1;
    LLCAdjustData2.LLCID = 2;

    // LLC2高压侧
    LLCAdjustData22 = LLCAdjustData11;
    LLCAdjustData22.LLCID = 4;

    LLCWorkState.LDeathTime = 0.4f;      // 0.4
    LLCWorkState.HDeathTime = 0.35f;     // 0.45->0.55f
    LLCWorkState.LLCFreq = 80.0f;        // 80
}
void ReInitLLCAdjustData(float lDeathTime, float hDeathTime, float llcFreq)
{
    uint16_t llcTBPRD = LLC_EPWM_PERIOD;

    llcTBPRD = ((PWM_SYSTEM_FREQUENCY / 1000.0f) / llcFreq) / 2.0f;

    // 低压侧死区
    LLCAdjustData1.SoftFinishIinterval = 60;
    LLCAdjustData1.SoftStartPeriod = 125;       //  250;   // 每 25ms PWM 递增一次,递增到 50% 需要 25ms*LLCPwmHalfPeriod300(左右) 时间
    LLCAdjustData1.LLCPwmPeriod = (uint16_t)(llcTBPRD);
    LLCAdjustData1.LLCPwmHalfPeriod = LLCAdjustData1.LLCPwmPeriod/2;
    LLCAdjustData1.LLCDeathTime = lDeathTime;  // 死区0.35us(0.45us),0.26(0.35),0.3(0.4)->0.4(500,470)->(600,500)
    // 死区计数偏移值10,如果无偏移LLC软启动时会出现PWM断续的情况
    LLCAdjustData1.LLCDeathTimeCount = (LLCAdjustData1.LLCDeathTime*100/1.0f + 10.0f);

    LLCAdjustData1.LLCID = 1;

    // LLC2低压侧
    LLCAdjustData2 = LLCAdjustData1;
    LLCAdjustData2.LLCID = 2;

    // LLC1高压侧死区
    LLCAdjustData11 = LLCAdjustData1;
    LLCAdjustData11.LLCDeathTime = hDeathTime;  // 死区0.3(0.45US)->0.26->0.3
    // 死区计数偏移值10,如果无偏移LLC软启动时会出现PWM断续的情况
    LLCAdjustData11.LLCDeathTimeCount = (LLCAdjustData11.LLCDeathTime*100/1.0f + 10.0f);      // 100M 时钟
    LLCAdjustData11.LLCID = 3;

    // LLC2高压侧
    LLCAdjustData22 = LLCAdjustData11;
    LLCAdjustData22.LLCID = 4;

    LLCWorkState.LDeathTime = lDeathTime;
    LLCWorkState.HDeathTime = hDeathTime;
    LLCWorkState.LLCFreq = llcFreq;
}
/*************************************************
Function: LLCOpenLoopSoftStart
Description: LLC 软启动控制
Input: None
Output: None
Frequency: 10KHz
*************************************************/
void LLCOpenLoopSoftStart(LLC_ADJUST_DATA *llc)
{
    uint16_t cmpA, cmpB;

    if (llc->LLCSoftStarEnable == 1)        // 软启是否使能
    {
        if (llc->LLCSoftStartFinish == 0)   // 软启动是否完成
        {
            llc->SoftStartCnt++;
            if (llc->SoftStartCnt >= llc->SoftStartPeriod)    // 20ms
            {
                llc->SoftStartCnt = 0;
                llc->PwmDuty++;

                if (llc->PwmDuty >= llc->LLCPwmPeriod)
                {
                    llc->PwmDuty = llc->LLCPwmPeriod;

                    llc->SoftFinishDelayCnt++;

                    if (llc->SoftFinishDelayCnt >= llc->SoftFinishIinterval)
                    {
                        llc->SoftFinishDelayCnt = 0;
                        llc->LLCSoftStartFinish = 1;
                    }
                }

                if (llc->PwmDuty >= llc->LLCDeathTimeCount)
                {
                    cmpA = llc->LLCPwmPeriod - llc->PwmDuty + llc->LLCDeathTimeCount - 10;
                    cmpB = llc->PwmDuty - llc->LLCDeathTimeCount + 10;

                    UpdateLLCPwmComp(llc->LLCID, cmpA, cmpB);
                }
                else
                {
                    cmpA = llc->LLCPwmPeriod - 10;
                    cmpB = 10;

                    UpdateLLCPwmComp(llc->LLCID, cmpA, cmpB);
                }
            }
        }

#if 1
        else
        {
            cmpA = llc->LLCPwmPeriod - llc->PwmDuty + llc->LLCDeathTimeCount - 10;
            cmpB = llc->PwmDuty - llc->LLCDeathTimeCount + 10;

            UpdateLLCPwmComp(llc->LLCID, cmpA, cmpB);

            // 充电时全开
            if (InvStateData.WorkState == OnGridState)
            {
                // R相,待机时打嗝模式,发功率时全开
                if ((SystemPara.ActivePowerSetR == 0) && (IrCtrl.RmpCtrl.SetpointValue == 0))
                {
                    DisableLLCPWMHbyID(1);            // 关闭高压侧驱动

                    if (LLCAdjustData1.VbusSamp > (LLCAdjustData1.VbusRated + 7.0f))
                    {
                        DisableLLCPWMLbyID(1);        // 关闭低压侧驱动后母线电压会降低
                    }
                    else if (LLCAdjustData1.VbusSamp < (LLCAdjustData1.VbusRated - 5.0f))
                    {
                        EnableLLCPWMLbyID(1);         // 开启低压侧驱动后BUS电压会慢慢升高
                    }

                    LLCAdjustData1.WaitFlg = 1;       // 低功耗状态下
                }
                else
                {
                    if (LLCAdjustData1.WaitFlg == 1)
                    {
                        DisableLLCPWMLbyID(1);
                        // 逆变先锁波等待LLC两侧电压合适再退出低功耗状态
                        if (LLCAdjustData1.VbusSamp < (LLCAdjustData1.VbusRated - 5.0f))
                        {
                            LLCAdjustData1.WaitFlg = 0;
                            EnableLLCPWMLbyID(1);
                            EnableLLCPWMHbyID(1);
                        }
                    }
                    else
                    {
                        EnableLLCPWMLbyID(1);
                        EnableLLCPWMHbyID(1);
                    }
                }

                // S相,待机时打嗝模式,发功率时全开
                if ((SystemPara.ActivePowerSetS == 0) && (IsCtrl.RmpCtrl.SetpointValue == 0))
                {
                    DisableLLCPWMHbyID(2);             // 关闭高压侧驱动

                    if (LLCAdjustData2.VbusSamp > (LLCAdjustData2.VbusRated + 7.0f))
                    {
                        DisableLLCPWMLbyID(2);
                    }
                    else if (LLCAdjustData2.VbusSamp < (LLCAdjustData2.VbusRated - 5.0f))
                    {
                        EnableLLCPWMLbyID(2);
                    }

                    LLCAdjustData2.WaitFlg = 1;       // 低功耗状态下
                }
                else
                {
                    if (LLCAdjustData2.WaitFlg == 1)
                    {
                        DisableLLCPWMLbyID(2);
                        if (LLCAdjustData2.VbusSamp < (LLCAdjustData2.VbusRated - 5.0f))
                        {
                            LLCAdjustData2.WaitFlg = 0;
                            EnableLLCPWMLbyID(2);
                            EnableLLCPWMHbyID(2);
                        }
                    }
                    else
                    {
                        EnableLLCPWMLbyID(2);
                        EnableLLCPWMHbyID(2);
                    }
                }
            }
            // 离网状态下LLC高压侧驱动关闭、低压侧驱动根据母线电压大小选择开通还是关闭
            else
            {
//                DisableLLCPWMHbyID(llc->LLCID);

                if (SystemPara.LLCBrustMode == 1)
                {
                    if (llc->VbusSamp > (llc->VbusRated + 5.0f))
                    {
                        EnableLLCPWMHbyID(llc->LLCID);          // 启动高压侧驱动
                        DisableLLCPWMLbyID(llc->LLCID);         // 关闭低压侧驱动
                    }
                    else if (llc->VbusSamp <= (llc->VbusRated - 5.0f))
                    {
                        DisableLLCPWMHbyID(llc->LLCID);         // 关闭高压侧驱动
                        EnableLLCPWMLbyID(llc->LLCID);          // 启动低压侧驱动
                    }
                    else
                    {
                        DisableLLCPWMHbyID(llc->LLCID);         // 关闭高压侧驱动
                        DisableLLCPWMLbyID(llc->LLCID);         // 关闭低压侧驱动
                    }
                }
                else
                {
                    EnableLLCPWMHbyID(llc->LLCID);          // 启动高压侧驱动
                    EnableLLCPWMLbyID(llc->LLCID);          // 启动低压侧驱动
                }
            }
        }
#endif
    }
    else
    {
        llc->LLCSoftStartFinish = 0;  // 软启完成标志
        llc->SoftFinishDelayCnt = 0;  // 软启执行时间
        llc->SoftStartCnt = 0;        // 软启动计数器
        llc->PwmDuty = 0;             // 占空比
        DisableAllLLCPwm(llc->LLCPwmPeriod);    // 禁止LLC
    }
}
uint16_t GetLLCSoftStartSate(void)
{
    // LLC软启结束
    if (LLCAdjustData1.LLCSoftStartFinish == 1 &&
            LLCAdjustData2.LLCSoftStartFinish == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*************************************************
Function: RunLLCStateMachine
Description: LLC工作状态控制-LLC_INIT_STATE,LLC_SELF_STATE,
** LLC_RUN_STATE
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(RunLLCStateMachine, ".TI.ramfunc");
void RunLLCStateMachine(void)
{
    switch (LLCWorkState.State)
    {
        case LLC_INIT_STATE:
        {
            LLCAdjustData1.LLCSoftStarEnable = 0;           // 软启标志置1
            LLCAdjustData11.LLCSoftStarEnable = 0;          // 软启标志置1

            LLCAdjustData2.LLCSoftStarEnable = 0;           // 软启标志置2
            LLCAdjustData22.LLCSoftStarEnable = 0;          // 软启标志置2

            DisableAllLLCPwm(LLCAdjustData1.LLCPwmPeriod);  // PWM 禁止

            // 等待电池包OK,零飘检测结束
            if ((SampleIns.CalSampleZeroFlag == 1) && (ISOData.Finished == 1) && (SystemError.ArmFault&0x02))
            {
                LLCWorkState.Count = 0;
                LLCWorkState.State = LLC_SELF_STATE;
            }

            break;
        }

        case LLC_SELF_STATE:
        {
            LLCAdjustData1.LLCSoftStarEnable = 0;   // 软启标志置1
            LLCAdjustData11.LLCSoftStarEnable = 0;   // 软启标志置1
            LLCOpenLoopSoftStart(&LLCAdjustData1);
            LLCOpenLoopSoftStart(&LLCAdjustData11);

            LLCAdjustData2.LLCSoftStarEnable = 0;   // 软启标志置2
            LLCAdjustData22.LLCSoftStarEnable = 0;   // 软启标志置2
            LLCOpenLoopSoftStart(&LLCAdjustData2);
            LLCOpenLoopSoftStart(&LLCAdjustData22);

            DisableAllLLCPwm(LLCAdjustData1.LLCPwmPeriod);
            // 无故障且母线电压小于100V,启动LLC
            if ((SystemError.StateErr.bit.LLCFault == 0)
                    && (SampleRms.Bus1VoltFilter8 < LLC_START_VOLT_MAX)
                    && (SampleRms.Bus2VoltFilter8 < LLC_START_VOLT_MAX)
                    && ((SystemError.ArmFault&ARM_FAULT_INV_BITS) == 0)
                    && (SystemError.StateErr.bit.INVFault == 0))
            {
                if (((SystemPara.SystemPowerOn == 1) || (SystemPara.LLCOpenLoopEn == 1)) && (++LLCWorkState.Count >= (2000*10))) // 2s
                {
                    LLCWorkState.Count = 0;

                    LLCAdjustData1.WaitFlg = 0;
                    LLCAdjustData2.WaitFlg = 0;

                    // 复位 TZ(LLC 过流检测)
                    REENABLELLC1TZ;
                    REENABLELLC2TZ;

                    ENABLE_LLC1PWM;
                    ENABLE_LLC2PWM;

                    LLCWorkState.State = LLC_RUN_STATE;
                }

                else if (LLCWorkState.LLCParaFlg == 1)
                {
                    LLCWorkState.LLCParaFlg = 0;
                    // 修改死区参数
                    ReInitLLCAdjustData(LLCWorkState.LDeathTime, LLCWorkState.HDeathTime, LLCWorkState.LLCFreq);
                    // 修改LLC频率
                    ReConfigLLCpwm(LLCWorkState.LLCFreq);
                }
            }
            else
            {
                LLCWorkState.Count = 0;
            }

            break;
        }
        case LLC_RUN_STATE:
        {
            // LLC1
            LLCAdjustData1.VbusRated = 2.66f*SampleRms.BATVoltFilter8;
            LLCAdjustData1.VbusRated = __fsat(LLCAdjustData1.VbusRated, 295.0f, 230);
            LLCAdjustData11.VbusRated = LLCAdjustData1.VbusRated;
            LLCAdjustData1.VbusSamp = SampleIns.Bus1VoltIns;//SampleRms.Bus1VoltFilter8;
            LLCAdjustData11.VbusSamp = LLCAdjustData1.VbusSamp;
            // LLC2
            LLCAdjustData2.VbusRated = 2.66f*SampleRms.BATVoltFilter8;
            LLCAdjustData2.VbusRated = __fsat(LLCAdjustData2.VbusRated, 295.0f, 230);
            LLCAdjustData22.VbusRated = LLCAdjustData2.VbusRated;
            LLCAdjustData2.VbusSamp = SampleIns.Bus2VoltIns; // SampleRms.Bus2VoltFilter8;
            LLCAdjustData22.VbusSamp = LLCAdjustData2.VbusSamp;

            LLCAdjustData1.LLCSoftStarEnable = 1;    // 软启标志置1
            LLCAdjustData11.LLCSoftStarEnable = 1;   // 软启标志置1
            LLCOpenLoopSoftStart(&LLCAdjustData1);
            LLCOpenLoopSoftStart(&LLCAdjustData11);

            LLCAdjustData2.LLCSoftStarEnable = 1;    // 软启标志置2
            LLCAdjustData22.LLCSoftStarEnable = 1;   // 软启标志置2
            LLCOpenLoopSoftStart(&LLCAdjustData2);
            LLCOpenLoopSoftStart(&LLCAdjustData22);

            // LLC软启执行1s后BUS电压应大于100V,此处添加BUS欠压检测
            if (++LLCWorkState.Count == 20000)
            {
                if (SampleRms.Bus1VoltFilter8 < 100.0f)
                {
                    SetBusLowVoltErr(1);
                }

                if (SampleRms.Bus2VoltFilter8 < 100.0f)
                {
                    SetBusLowVoltErr(2);
                }
            }
            if (LLCWorkState.Count > 20000)
            {
                LLCWorkState.Count = 20000;
            }

            // 如果有故障或关机指令
            if (SystemError.StateErr.bit.LLCFault != 0)         // LLC相关故障要停机
            {
                LLCWorkState.Count = 0;
                LLCWorkState.State = LLC_SELF_STATE;

                DisableAllLLCPwm(LLCAdjustData1.LLCPwmPeriod);
            }
            else if (SystemError.StateErr.bit.INVFault || ((SystemPara.SystemPowerOn == 0) && (SystemPara.LLCOpenLoopEn != 1)))
            {
                // 关机指令下发后先关逆变LLC延时20ms,否则可能会出现逆变过流的情况,逆变故障后LLC延时关闭
                if (++LLCWorkState.DelayCnt >= 200)
                {
                    LLCWorkState.DelayCnt = 0;
                    LLCWorkState.Count = 0;
                    LLCWorkState.State = LLC_SELF_STATE;
                }
            }

            break;
        }
        default:
        {
            SystemPara.SystemPowerOn = 0;
            SystemPara.LLCOpenLoopEn = 0;

            DisableAllLLCPwm(LLCAdjustData1.LLCPwmPeriod);

            LLCAdjustData1.LLCSoftStarEnable = 0;    // 软启标志置1
            LLCAdjustData11.LLCSoftStarEnable = 0;   // 软启标志置1

            LLCAdjustData2.LLCSoftStarEnable = 0;    // 软启标志置2
            LLCAdjustData22.LLCSoftStarEnable = 0;   // 软启标志置2

            LLCWorkState.Count = 0;
            LLCWorkState.State = LLC_SELF_STATE;

            break;
        }
    }
}

/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: CalculateAngle.c
Author: tzz
Version : 0.0.2
Date:2021年10月12日
Description: 离网并网锁相,离网时采用定频率变点数的方式
Function List: 离网角度更新、相位同步、并网角度更新
***********************************************************/
#include <bsp.h>
#include <string.h>
#include "Sample.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "ConfigSystemPara.h"
#include <math.h>
#include "Protect.h"
#include "Parallel.h"
#include "OnGridInv2.h"
#include "MicroInvmode.h"
#include"Energy_Manage.h"
#include "Function.h"


INV_PHASE InvPhase;
CAPTURE_DATA CaptureData;
PIDCntl_F    GridLock;

static void CalculateOffGridAngle(void);
static void SyncGridPhase(uint16_t state, uint32_t capValue);

/*************************************************
Function: InitCalculateAngleData
Description: 角度模块变量初始化
Input: None
Output: None
Frequency: Once
*************************************************/
void InitCalculateAngleData(void)
{
    memset(&CaptureData, 0, sizeof(CAPTURE_DATA));
    memset(&InvPhase, 0, sizeof(INV_PHASE));

    // 频率捕获值周期值下限, 系统时钟频率
    CaptureData.FacLowCycle = (uint32_t)(SYSTEM_FREQUENCY / CAP_PERIOD_HI);
    // 频率捕获值周期值上限, 系统时钟频率
    CaptureData.FacHighCycle = (uint32_t)(SYSTEM_FREQUENCY / CAP_PERIOD_LO);
    // 系统默认频率
    CaptureData.FreqR = SystemPara.InvRateFreq;
    CaptureData.FreqS = CaptureData.FreqR;
    CaptureData.LastFreqR = CaptureData.FreqR;
    CaptureData.LastFreqS = CaptureData.FreqS;

    // 默认 delta theta
    InvPhase.DeltR = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/CaptureData.FreqR);
    InvPhase.DeltS = InvPhase.DeltR;
    // 周期采用点数,向上取整数
    InvPhase.PeriodPoint = (uint16_t)(PWM_ISR_FREQUENCY/CaptureData.FreqR + 0.5f);
    InvPhase.PeriodPointMin = (uint16_t)(PWM_ISR_FREQUENCY/CAP_PERIOD_HI + 0.5f);
    InvPhase.PeriodPointMax = (uint16_t)(PWM_ISR_FREQUENCY/CAP_PERIOD_LO + 0.5f);

    InvPhase.PeriodPointS = InvPhase.PeriodPoint;

    ResetOffGridAngle();

    Init_PI_Controller(&GridLock, 0.3, 0.1, 2, -2);
}
/*************************************************
Function: ResetOffGridAngle
Description: 复位
Input: None
Output: None
Frequency: Once
*************************************************/
void ResetOffGridAngle(void)
{
    if (SystemPara.FrequencySet == 1)
    {
        InvPhase.ThetaPointS = 83 + InvPhase.ThetaPointR;   // 60Hz
    }
    else if (SystemPara.FrequencySet == 0)
    {
        InvPhase.ThetaPointS = 100 + InvPhase.ThetaPointR;  // 50Hz
    }

    InvPhase.ThetaS = InvPhase.ThetaR + PI_S;
}
/*************************************************
Function: CalculateOffGridAngle
Description: 计算离网角度
Input: None
Output: None
Frequency: 10K
*************************************************/
//#pragma CODE_SECTION(CalculateOffGridAngle, ".TI.ramfunc");
static void CalculateOffGridAngle(void)
{
    if(InvStateData.WorkState < 5)
    {
        if (InvPhase.ThetaR >= (PI_S*2.0f  - 0.000005f))
        {
            InvPhase.ThetaR = InvPhase.ThetaR - PI_S*2.0f;
            // 用于计算交流信号有效值的周期点数,需要做限幅处理
            InvPhase.PeriodPoint = InvPhase.ThetaPointR;
            InvPhase.PeriodPoint = (uint16_t)__fsat((float)InvPhase.PeriodPoint, (float)InvPhase.PeriodPointMax, (float)InvPhase.PeriodPointMin);
            InvPhase.ThetaPointR = 0;
        }

        if (InvPhase.ThetaR >= (PI_S*1.0f - 0.000005f))
        {
            InvPhase.ThetaPointS = 0;
            InvPhase.ThetaS = InvPhase.ThetaR + PI_S;
        }

//        if (InvPhase.ThetaS >= (PI_S*2.0f - 0.000005f))
//        {
//            InvPhase.ThetaPointS = 0;
//            InvPhase.ThetaS = InvPhase.ThetaS - PI_S*2.0f;
//        }
    }

    if (InvPhase.ThetaPointR > InvPhase.PeriodPoint)
    {
        InvPhase.ThetaPointR = InvPhase.PeriodPoint;
    }
    if (InvPhase.ThetaPointS > InvPhase.PeriodPoint)
    {
        InvPhase.ThetaPointS = InvPhase.PeriodPoint;
    }

    InvPhase.SinR = __sin(InvPhase.ThetaR + DroopCntl.sigma.a);   // 计算R相角度的正弦值
    InvPhase.CosR = __cos(InvPhase.ThetaR + DroopCntl.sigma.a);   // 计算R相角度的余弦值

    InvPhase.SinS = __sin(InvPhase.ThetaS + DroopCntl.sigma.b);   // 计算S相角度的正弦值
    InvPhase.CosS = __cos(InvPhase.ThetaS + DroopCntl.sigma.b);   // 计算S相角度的余弦值
}
/*************************************************
Function: UpdateAngle
Description: 更新角度值,孤岛保护在此处添加
Input: None
Output: None
Frequency: 10K
*************************************************/
//#pragma CODE_SECTION(UpdateAngle, ".TI.ramfunc");
void UpdateAngle(void)
{
    // 并网状态下 S 相需要单独锁相
    if (InvStateData.WorkState >= 5 )
    {
        InvPhase.ThetaPointR++;
        InvPhase.ThetaPointS++;
        InvPhase.ThetaR = (PI_S + 2*InvPhase.DeltR) -IrCtrl.ThetaSpll;
        InvPhase.ThetaS = (PI_S + 2*InvPhase.DeltR) -IsCtrl.ThetaSpll;

//        InvPhase.ThetaR += InvPhase.DeltR;
//        InvPhase.ThetaS += InvPhase.DeltS;
    }
    else
    {
        InvPhase.ThetaPointR++;
        InvPhase.ThetaR += (InvPhase.DeltR + ShareData.theta_Adj);
        InvPhase.ThetaPointS++;
//        InvPhase.ThetaS += (InvPhase.DeltR + ShareData.theta_Adj);
        InvPhase.ThetaS = InvPhase.ThetaR + PI_S;

        // 孤岛保护相关参数清零
        IslandDataR.IslandEnable = 0;
        IslandDataR.SumDeltaF = 0;
        IslandDataR.TimeCheckCnt = 0;
        IslandDataR.CheckIntervalCnt = 0;

        IslandDataS.IslandEnable = 0;
        IslandDataS.SumDeltaF = 0;
        IslandDataS.TimeCheckCnt = 0;
        IslandDataS.CheckIntervalCnt = 0;
    }

    InvPhase.HrThetaR = InvPhase.ThetaR;
}
/*************************************************
Function: SyncGridPhase
Description: R相电网相位同步
Input: None
Output: None
Frequency:
*************************************************/
//#pragma CODE_SECTION(SyncGridPhase, ".TI.ramfunc");
static void SyncGridPhase(uint16_t state, uint32_t capValue)
{
    static int PhaseLockCnt = 0;

    if (state >= 5)
    {
        if (capValue > CaptureData.FacLowCycle && capValue < CaptureData.FacHighCycle)    // 频率在合法范围内
        {
            // 更新Delta
            InvPhase.DeltR = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/CaptureData.FreqR);
//            if(InvPhase.ThetaR >= ((2*PI_S)-InvPhase.DeltR*5))//防止切离网前角度计算错误
//            {
//                InvPhase.ThetaR = InvPhase.DeltR* 0.5f;
//            }
            // 更新周期点数,需要做限值处理
            InvPhase.PeriodPoint = InvPhase.ThetaPointR;
            InvPhase.PeriodPoint = (uint16_t)__fsat((float)InvPhase.PeriodPoint, (float)InvPhase.PeriodPointMax, (float)InvPhase.PeriodPointMin);
            InvPhase.ThetaPointR = 0;
        }
    }
    // 开始同步
    if (state == 4 && InvPhase.SyncBeginFlag == 1)
    {
        if (capValue > CaptureData.FacLowCycle && capValue < CaptureData.FacHighCycle)
        {
            //TODO 油机时还是要改锁相 追电网角度 IrCtrl.ThetaSpll
            if(InvPhase.ThetaR<PI_S)
            {
                GridLock.xp[0] = (InvPhase.ThetaR) - 2*InvPhase.DeltR;
            }
            else
            {
                GridLock.xp[0] = (InvPhase.ThetaR - (2*PI_S)- 2*InvPhase.DeltR);   //相位滞后
            }
            PI_Controller(&GridLock);

            InvPhase.DeltR = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/(CaptureData.FreqR - GridLock.yp[0]));
//            InvPhase.DeltS = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/CaptureData.FreqS);
            InvPhase.DeltS = InvPhase.DeltR;

            if (InvPhase.SyncReadyFlag == 0) // && GetcModuleMode()
            {
                // 如果当前角度在0度附近，直接同步。零点左右2点内即认为重合
                if(fabsf(GridLock.xp[0])< InvPhase.DeltR)
                //if ((InvPhase.ThetaR >= ((2*PI_S)-InvPhase.DeltR)) || (InvPhase.ThetaR <= (InvPhase.DeltR)))
                {
                    //等待3周期？
                    if( ++PhaseLockCnt > 15 )
                    {
//                        InvPhase.ThetaR = 0;
//                        InvPhase.ThetaPointR = 0;
                        InvPhase.SyncReadyFlag = 1;     // 同步完成标志
                    }
                }
                else
                {
                    PhaseLockCnt =0;

//                    InvPhase.ThetaPointR += 1;
//                    InvPhase.ThetaR += (InvPhase.DeltR*1);
//
//                    InvPhase.ThetaPointS += 1;
//                    InvPhase.ThetaS += (InvPhase.DeltS*1);
                }
            }
        }
        else
        {
            PhaseLockCnt = 0;
            GridLock.yp[0] = 0;
            GridLock.yp[1] = 0;
            InvPhase.SyncReadyFlag = 0;
        }
    }
    else
    {
        PhaseLockCnt = 0;
        GridLock.yp[0] = 0;
        GridLock.yp[1] = 0;
    }
}
/*************************************************
Function: CalculateAngle
Description: 计算R相位
Input: None
Output: None
Frequency:10kHz
*************************************************/
//#pragma CODE_SECTION(CalculateAngle, ".TI.ramfunc");
void CalculateAngle(uint16_t state, uint16_t capFlag, uint32_t capValue, int16_t gridVolt)
{
    // R相捕获标志
    if (capFlag == 1)
    {
        SyncGridPhase(state, capValue);                  // 相位同步
    }

    // 逆变状态机进入到UPS状态下之后,延时5s开始和电网相位进行同步
    // 电网电压属于[80,150],该参数需要根据实际系统进行修改
//    if (gridVolt >= 50 && gridVolt <= 160 &&          // 电网电压在正常范围
//            capValue > CaptureData.FacLowCycle &&     // 捕获周期值在合理范围内
//            capValue < CaptureData.FacHighCycle &&
//            state == 4)                               // UPS待机状态下相位同步
    if ((gridVolt >= 50) && (gridVolt <= 160) && (state == 4))
    {
        InvPhase.SyncDelayCount++;
    }
    else
    {
        InvPhase.SyncDelayCount = 0;
    }
    // 5s之后开始同步
    if(InvPhase.SyncDelayCount >= 50000)
    {
        InvPhase.SyncDelayCount = 51000;
        InvPhase.SyncBeginFlag = 1;                    // 5s后开始同步
    }
    else
    {
        InvPhase.SyncBeginFlag = 0;
        InvPhase.SyncReadyFlag = 0;
    }

    // 离网/停机/故障状态下,频率固定为系统频率 SystemPara.FreqSetF
    if (state < 4)
    {
        InvPhase.SyncDelayCount = 0;
        if (SystemPara.MicroInverterMode)
        {
            InvPhase.DeltR = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/MicroInvmode.FreqCntl.yp[0]);
            InvPhase.DeltS = InvPhase.DeltR;
            SystemPara.InvRateFreq = MicroInvmode.FreqCntl.yp[0];
        }
        else
        {
            InvPhase.DeltR = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/SystemPara.InvRateFreq);
            InvPhase.DeltS = InvPhase.DeltR;
        }

    }
}
/*************************************************
Function: CalculateAngle
Description: 计算S相位
Input: None
Output: None
Frequency:
*************************************************/
//#pragma CODE_SECTION(CalculateAngle2, ".TI.ramfunc");
void CalculateAngle2(uint16_t state, uint16_t capFlag, uint32_t capValue, int16_t gridVolt)
{
    // S相捕获标志
    if (capFlag == 1)
    {
        if (state == 5)
        {
            if (capValue > CaptureData.FacLowCycle && capValue < CaptureData.FacHighCycle)    // 频率在合法范围内
            {
                InvPhase.DeltS = (PI_S*2.0f)/(PWM_ISR_FREQUENCY/CaptureData.FreqS);
                if(InvPhase.ThetaS >= ((2*PI_S)-InvPhase.DeltS*5))
                {
                    InvPhase.ThetaS = InvPhase.DeltS * 0.5f;
                }
                // 更新周期点数,需要做限值处理
                InvPhase.PeriodPointS = InvPhase.ThetaPointS;
                InvPhase.PeriodPointS = (uint16_t)__fsat((float)InvPhase.PeriodPointS, (float)InvPhase.PeriodPointMax, (float)InvPhase.PeriodPointMin);
                InvPhase.ThetaPointS = 0;
            }
        }
    }
    CalculateOffGridAngle();  // 离网状态或待机状态计算角度
}
/*************************************************
Function: HandleInvterPhase
Description: R/S相相位处理，
* 上电后会出现 ECap2Regs.ECFLG.bit.CEVT1 = 1 的情况
* 初始化时要将该值清零一下
* 100us检测一次捕获标志,最大存在100us延时
Input: None
Output: None
Frequency:
*************************************************/
//#pragma CODE_SECTION(HandleInvterPhase, ".TI.ramfunc");
void HandleInvterPhase(void)
{
    float tempFreq = 0;
    uint16_t capFlag = 0;
    uint32_t capValue = 0;

    // 电网接入R相
    capFlag = ECap1Regs.ECFLG.bit.CEVT1;    // 捕获事件1标志

    if(ECap2Regs.ECFLG.bit.CEVT1 == 1)
    {
        CaptureData.RZeroFlag = ECap2Regs.ECFLG.bit.CEVT1;
        // 清除捕获时间标志
        ECap2Regs.ECCLR.all = 0xFFFF;
        ECap2Regs.ECCTL2.bit.REARM = 1;     // 重启捕获
    }
    else
    {
        CaptureData.RZeroFlag = 0;
    }

    if (capFlag)
    {
        capValue = ECap1Regs.CAP1;
        ECap1Regs.ECCLR.bit.CEVT1 = 1;      // 清除捕获时间标志
        ECap1Regs.ECCTL2.bit.REARM = 1;     // 重启捕获

        // ------------计算R相频率----------------------------------------
        CaptureData.RawValueR[1] = capValue;

        if (CaptureData.RawValueR[1] >= CaptureData.RawValueR[0])
        {
            CaptureData.CmpDataR = CaptureData.RawValueR[1] - CaptureData.RawValueR[0];
        }
        else
        {
            CaptureData.CmpDataR = 0xFFFFFFFF + CaptureData.RawValueR[1] - CaptureData.RawValueR[0];
        }

        // 非并网状态下每次捕获都要更新变量 CaptureData.RawValueR[0]
        if (InvStateData.WorkState <= OffGridState)
        {
            CaptureData.RawValueR[0] = CaptureData.RawValueR[1];
        }

        // --------------------------CaptureData.CmpDataR < CaptureData.FacHighCycle----------------------------------
        if (CaptureData.CmpDataR > CaptureData.FacLowCycle)
        {
            CaptureData.FreqR = (float)SYSTEM_FREQUENCY/(float)CaptureData.CmpDataR;

//            CaptureData.FreqR = __fsat(CaptureData.FreqR, CAP_PERIOD_HI, CAP_PERIOD_LO);

            CaptureData.LastFreqR = CaptureData.FreqR;

            IslandDataR.CapFlag = 1;

            CaptureData.RawValueR[0] = CaptureData.RawValueR[1];    // 放在此处是为了解决硬件捕获误触发的问题
        }
        else
        {
            CaptureData.FreqR = CaptureData.LastFreqR;
        }

        // ---------------------------求取频率平均值,用于对外显示------------
        if (CaptureData.CntFreqR >= 5)
        {
            CaptureData.AvgFreqR = CaptureData.SumFreqR / 5.0f;
            CaptureData.SumFreqR = 0;
            CaptureData.CntFreqR = 0;
        }
        CaptureData.CntFreqR++;
        CaptureData.SumFreqR += CaptureData.FreqR;
    }
    else
    {
        capValue = 0;
    }

    // 使用 R相电网捕获作为同步源
    CalculateAngle((uint16_t)InvStateData.WorkState, CaptureData.RZeroFlag, CaptureData.CmpDataR, SampleRms.Grid1VoltRms);

    if (SampleRms.Grid1VoltRms < 25.0f)
    {
        CaptureData.AvgFreqR = 0;
        CaptureData.FreqR = 0;
        CaptureData.LastFreqR = 0;

        InvPhase.GridPeriodPoint = PWM_ISR_FREQUENCY/SystemPara.InvRateFreq;
    }
    else
    {
        // 计算电网当前频率下 10kHz采样频率时单周期采样点数
        tempFreq = __fsat(CaptureData.AvgFreqR, 65.0f, 45.0f);
        InvPhase.GridPeriodPoint = 0.5f + PWM_ISR_FREQUENCY/tempFreq;
    }

    // ----------------------------------------------------------------------------
    // 电网接入S相
    capFlag = ECap4Regs.ECFLG.bit.CEVT1;    // 频率捕获标志
    if(ECap5Regs.ECFLG.bit.CEVT1 == 1)
    {
        CaptureData.SZeroFlag = 1;
        // 清除捕获时间标志
        ECap5Regs.ECCLR.all = 0xFFFF;
        ECap5Regs.ECCTL2.bit.REARM = 1;     // 重启捕获
    }
    else
    {
        CaptureData.SZeroFlag = 0;
    }
    if (capFlag)
    {
        capValue = ECap4Regs.CAP1;          // 读取捕获值
        ECap4Regs.ECCLR.bit.CEVT1 = 1;      // 清除捕获时间标志
        ECap4Regs.ECCTL2.bit.REARM = 1;     // 重启捕获

        // -------------------- 计算S相频率 --------------------------------------------
        CaptureData.RawValueS[1] = capValue;

        if (CaptureData.RawValueS[1] >= CaptureData.RawValueS[0])
        {
            CaptureData.CmpDataS = CaptureData.RawValueS[1] - CaptureData.RawValueS[0];
        }
        else
        {
            CaptureData.CmpDataS = 0xFFFFFFFF + CaptureData.RawValueS[1] - CaptureData.RawValueS[0];
        }

        // 非并网状态下每次捕获都要更新变量 CaptureData.RawValueR[0]
        if (InvStateData.WorkState <= OffGridState)
        {
            CaptureData.RawValueS[0] = CaptureData.RawValueS[1];
        }

        //---------------------------------CaptureData.CmpDataS < CaptureData.FacHighCycle------------------------------------------
        if (CaptureData.CmpDataS > CaptureData.FacLowCycle)
        {
            CaptureData.FreqS = (float)SYSTEM_FREQUENCY/(float)CaptureData.CmpDataS;
//            CaptureData.FreqS = __fsat(CaptureData.FreqS, CAP_PERIOD_HI, CAP_PERIOD_LO);

            CaptureData.LastFreqS = CaptureData.FreqS;

            IslandDataS.CapFlag = 1;

            CaptureData.RawValueS[0] = CaptureData.RawValueS[1];
        }
        else
        {
            CaptureData.FreqS = CaptureData.LastFreqS;
        }
        // -----------------------求取频率平均值,用于对外显示--------------------------------
        if (CaptureData.CntFreqS >= 5)
        {
            CaptureData.AvgFreqS = CaptureData.SumFreqS / 5.0f;
            CaptureData.SumFreqS = 0;
            CaptureData.CntFreqS = 0;
        }
        CaptureData.CntFreqS++;
        CaptureData.SumFreqS += CaptureData.FreqS;
        // ---------------------------------------------------------------------------
    }
    else
    {
        capValue = 0;
    }
    // 使用 S相电网捕获作为同步源
    CalculateAngle2((uint16_t)InvStateData.WorkState, CaptureData.SZeroFlag, CaptureData.CmpDataS, SampleRms.Grid2VoltRms);

    if (SampleRms.Grid2VoltRms < 25.0f)
    {
        CaptureData.AvgFreqS = 0;
        CaptureData.FreqS = 0;
        CaptureData.LastFreqS = 0;
    }
}
/*************************************************
Function: AFDAntiIslanding
Description: AFD 孤岛检测,
Input: None
Output: None
Frequency:
*************************************************/
float Freq_R[4] = {0,0,0,0};
float Freq_S[4] = {0,0,0,0};
int16 FreqCNT_R = 0,FreqCNT_S = 0;
float AvgSum_R = 0,AvgSum_S = 0;
float AvgFreq_R = 0,AvgFreq_S = 0;

void AFDAntiIslanding(void)
{   
   
    float thetaR = IrCtrl.ThetaSpll;
    float thetaS = IsCtrl.ThetaSpll;

    // 并网状态下 S 相需要单独锁相
    if (InvStateData.WorkState == 5)
    {
       /* if (IslandDataR.AllEnable == 1)
        {
            // 每1s使能一次孤岛检测,连续检测10个周波后停止
            if (++IslandDataR.CheckIntervalCnt >= IslandDataR.CheckInterval)
            {
                IslandDataR.CheckIntervalCnt = 0;
                IslandDataR.IslandEnable = 1;
            }

            if (++IslandDataS.CheckIntervalCnt >= IslandDataS.CheckInterval)
            {
                IslandDataS.CheckIntervalCnt = 0;
                IslandDataS.IslandEnable = 1;
            }
        }
        else
        {
            // 孤岛保护相关参数清零
            IslandDataR.IslandEnable = 0;
            IslandDataR.SumDeltaF = 0;
            IslandDataR.TimeCheckCnt = 0;
            IslandDataR.CheckIntervalCnt = 0;

            IslandDataS.IslandEnable = 0;
            IslandDataS.SumDeltaF = 0;
            IslandDataS.TimeCheckCnt = 0;
            IslandDataS.CheckIntervalCnt = 0;

            EnergyManage.IslandQR = 0;
            EnergyManage.IslandQS = 0;
        }*/

        // ----------------R相----------------------------
        thetaR = PI_S - IrCtrl.ThetaSpll;    // [0~2PI]
        if (thetaR > (2.0f*PI_S))
        {
            thetaR = 2.0f*PI_S;
        }
        if (IslandDataR.IslandEnable)
        {
            // 正向扰频,周期减小频率增大
            if (IslandDataR.Dir == 0)
            {
                
                thetaR *= (1.0f + IslandDataR.DeltaWt);     // IslandDataR.DeltaWt 初始值应设置为0
            }
            // 反向扰频,周期变大频率减小
            else if (IslandDataR.Dir == 1)
            {
                thetaR *= (1.0f + IslandDataR.DeltaWt);
            }

            // 每个周期扰动频率执行一次递增
            if (IslandDataR.CapFlag == 1)
            {
                // 更新 DeltaWt
//                AvgSum_R += CaptureData.FreqR;
//			  	AvgSum_R -= Freq_R[FreqCNT_R];
			  	Freq_R[FreqCNT_R] = CaptureData.FreqR;
			  	if(++FreqCNT_R >= 4)
			  	{
			  		FreqCNT_R = 0;
			  	}
//				AvgFreq_R = AvgSum_R/4;

				AvgFreq_R = (Freq_R[0]+Freq_R[1]+Freq_R[2]+Freq_R[3])*0.25f;

				IslandDataR.DeltaF=fabs(CaptureData.FreqR-AvgFreq_R);

			     if(EnergyManage.P1out < 500)
			     {
			         EnergyManage.IslandQR = IslandDataR.DeltaF*15000;//小功率时不要功率因素矫正用的200VAR
			     }
			     else
			     {
			         EnergyManage.IslandQR=200+IslandDataR.DeltaF*15000;
			     }
				if(IslandDataR.DeltaF>=0)
				{
				  IslandDataR.Dir =0;
				  IslandDataR.DeltaWt=0;//0.02+IslandDataR.DeltaF;
				}
				else
				{
				  IslandDataR.Dir =1;
				  IslandDataR.DeltaWt=-0;//-0.02+IslandDataR.DeltaF;
				}
				EnergyManage.IslandQR =__fsat(EnergyManage.IslandQR,1200, -1200);
				IslandDataR.DeltaWt =__fsat(IslandDataR.DeltaWt, IslandDataS.MaxDeltaF, -IslandDataS.MaxDeltaF);
                /*
                IslandDataR.DeltaWt = IslandDataR.SumDeltaF/CaptureData.FreqR;
                IslandDataR.SumDeltaF += IslandDataR.DeltaF;

                if (IslandDataR.SumDeltaF >= IslandDataR.MaxDeltaF)
                {
                    IslandDataR.SumDeltaF = IslandDataR.MaxDeltaF;
                }

                // 完成TimeCheck个周波的扰动后停止
                if (++IslandDataR.TimeCheckCnt >= IslandDataR.TimeCheck)
                {
                    IslandDataR.TimeCheckCnt = 0;
                    IslandDataR.IslandEnable = 0;

                    IslandDataR.Dir = !IslandDataR.Dir;
                }
                */
                IslandDataR.CapFlag = 0;
            }
        }
        else
        {/*
            // 恢复
            if (IslandDataR.AllEnable == 1)
            {
                if (IslandDataR.CapFlag == 1)
                {
                    IslandDataR.SumDeltaF -= IslandDataR.DeltaF;
                    IslandDataR.SumDeltaF = __fsat(IslandDataR.SumDeltaF, IslandDataR.MaxDeltaF, 0.0f);

                    IslandDataR.DeltaWt = IslandDataR.SumDeltaF/CaptureData.FreqR;
                    IslandDataR.CapFlag = 0;
                }
                // 反向恢复
                if (IslandDataR.Dir == 1)
                {
                    thetaR *= (1 + IslandDataR.DeltaWt);
                }
                // 反向恢复
                else if (IslandDataR.Dir == 0)
                {
                    thetaR *= (1 - IslandDataR.DeltaWt);
                }
            }
            */
        }

        // -------------------S相---------------------
        thetaS = PI_S - IsCtrl.ThetaSpll;    // [0~2PI]
        if (thetaS > (2.0f*PI_S))
        {
            thetaS = 2.0f*PI_S;
        }
        if (IslandDataS.IslandEnable)
        {
            // 正向扰频,周期减小频率增大
            if (IslandDataS.Dir == 0)
            {
                thetaS *= (1 + IslandDataS.DeltaWt);
            }
            // 反向扰频,周期变大频率减小
            else if (IslandDataS.Dir == 1)
            {
                thetaS *= (1 - IslandDataS.DeltaWt);
            }

            // 每个周期扰动频率执行一次递增
            if (IslandDataS.CapFlag == 1)
            {
                AvgSum_S += CaptureData.FreqS;
			  	AvgSum_S -= Freq_S[FreqCNT_S];
			  	Freq_S[FreqCNT_S] = CaptureData.FreqS;
			  	if(++FreqCNT_S >= 4)
			  	{
			  		FreqCNT_S = 0;
			  	}
				AvgFreq_S = AvgSum_S/4;
				IslandDataS.DeltaF=fabs(CaptureData.FreqS-AvgFreq_S);

			     if(EnergyManage.P2out < 500)
			     {
			         EnergyManage.IslandQS = IslandDataS.DeltaF*15000;
			     }
			     else
			     {
			         EnergyManage.IslandQS=200+IslandDataS.DeltaF*15000;
			     }
				if(IslandDataR.DeltaF>=0)
				{
				  IslandDataS.Dir =0;
				  IslandDataS.DeltaWt=0;//0.02+IslandDataS.DeltaF;
				}
				else
				{
				  IslandDataS.Dir =1;
				  IslandDataS.DeltaWt=-0;//-0.02+IslandDataS.DeltaF;
				}
				EnergyManage.IslandQS =__fsat(EnergyManage.IslandQS,1200, -1200);
				IslandDataS.DeltaWt= __fsat(IslandDataS.DeltaWt, IslandDataS.MaxDeltaF, -IslandDataS.MaxDeltaF);
                // 更新 DeltaWt
                /*
                IslandDataS.DeltaWt = IslandDataS.SumDeltaF/CaptureData.FreqS;

                IslandDataS.SumDeltaF += IslandDataS.DeltaF;

                if (IslandDataS.SumDeltaF >= IslandDataS.MaxDeltaF)
                {
                    IslandDataS.SumDeltaF = IslandDataS.MaxDeltaF;
                }

                // 完成TimeCheck个周波的扰动后停止
                if (++IslandDataS.TimeCheckCnt >= IslandDataS.TimeCheck)
                {
                    IslandDataS.TimeCheckCnt = 0;
                    IslandDataS.IslandEnable = 0;

                    IslandDataS.Dir = !IslandDataS.Dir;
                }
                 */
                IslandDataS.CapFlag = 0;
            }
        }
        else
        { /*
            // 恢复
            if (IslandDataS.AllEnable == 1)
            {
                if (IslandDataS.CapFlag == 1)
                {
                    IslandDataS.SumDeltaF -= IslandDataS.DeltaF;
                    IslandDataS.SumDeltaF = __fsat(IslandDataS.SumDeltaF, IslandDataS.MaxDeltaF, 0.0f);

                    IslandDataS.DeltaWt = IslandDataS.SumDeltaF/CaptureData.FreqS;
                    IslandDataS.CapFlag = 0;
                }
                // 反向恢复
                if (IslandDataS.Dir == 1)
                {
                    thetaS *= (1 + IslandDataS.DeltaWt);
                }
                // 反向恢复
                else if (IslandDataS.Dir == 0)
                {
                    thetaS *= (1 - IslandDataS.DeltaWt);
                }
            }
            */
        }
    }
    else
    {
        EnergyManage.IslandQR = 0;
        EnergyManage.IslandQS = 0;
    }

    IrCtrl.AFDTheta = thetaR;
    IsCtrl.AFDTheta = thetaS;
    IrCtrl.Sin = __sin(thetaR);
    IsCtrl.Sin = __sin(thetaS);
}
/*************************************************
Function: 软件锁相
Description:
Input: None
Output: None
Frequency:
*************************************************/
void SoftPLL(void)
{
    // 更新正交分解参数
    SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) ((2 * PI_S) * CaptureData.FreqS), &IrCtrl.Usogi);
    SPLL_1ph_SOGI_F_coeff_update(((float) (1.0 / PWM_ISR_FREQUENCY)), (float) ((2 * PI_S) * CaptureData.FreqS), &IsCtrl.Usogi);

    //spll
    IrCtrl.Usogi.u[0] = SampleIns.Grid1VoltIns;//
    SPLL_1ph_SOGI_F_FUNC(&IrCtrl.Usogi);
    IsCtrl.Usogi.u[0] = SampleIns.Grid2VoltIns;//
    SPLL_1ph_SOGI_F_FUNC(&IsCtrl.Usogi);

    IrCtrl.ThetaSpll = __atan2(IrCtrl.Usogi.osg_u[0] ,IrCtrl.Usogi.osg_qu[0]);
    IsCtrl.ThetaSpll = __atan2(IsCtrl.Usogi.osg_u[0] ,IsCtrl.Usogi.osg_qu[0]);

    if(IrCtrl.ThetaSpll > 0  && IrCtrl.ThetaSpllLast < 0 )
    {
        //电网过零标志
        InvStateData.GridZCD = 1;
        IrCtrl.DeltaTheta =IrCtrl.ThetaSpllLast - (IrCtrl.ThetaSpll - (2*PI_S));
    }
    else
    {
        IrCtrl.DeltaTheta = IrCtrl.ThetaSpllLast -IrCtrl.ThetaSpll;
    }

    if(IsCtrl.ThetaSpll > 0  && IsCtrl.ThetaSpllLast < 0 )
    {
        IsCtrl.DeltaTheta =IsCtrl.ThetaSpllLast - (IsCtrl.ThetaSpll - (2*PI_S));
    }
    else
    {
        IsCtrl.DeltaTheta = IsCtrl.ThetaSpllLast- IsCtrl.ThetaSpll;
    }


//    IrCtrl.FreqCal = IrCtrl.DeltaTheta * 10000 / (2*PI_S);
//    IsCtrl.FreqCal = IsCtrl.DeltaTheta * 10000 / (2*PI_S);
    IrCtrl.ThetaSpllLast = IrCtrl.ThetaSpll;
    IsCtrl.ThetaSpllLast = IsCtrl.ThetaSpll;
#if 1
    AFDAntiIslanding();
#else
    IrCtrl.Sin = __sin(IrCtrl.ThetaSpll);
    IsCtrl.Sin = __sin(IsCtrl.ThetaSpll);
#endif

}

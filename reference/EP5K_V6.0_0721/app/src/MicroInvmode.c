/*
 * MicroInvmode.c
 *
 *  Created on: 2023年12月8日
 *      Author: tian
 */
#include "MicroInvmode.h"
#include "ConfigSystemPara.h"
#include "PowerCalculator.h"

MicroInvmode_STRUCT MicroInvmode;
/*************************************************
Function: InitMicroInvterPara
Description: 初始化系统参数配置模块变量
Input: None
Output: None
Frequency: Once
*************************************************/
void InitMicroInverterPara(void)
{
    MicroInvmode.OFDeadband = 0.036;
    MicroInvmode.FH = 62.5;
    MicroInvmode.FN = 60;
    MicroInvmode.Pmax = 3000;
    MicroInvmode.PcLimit = 3000;
    MicroInvmode.kOF_P    = (MicroInvmode.FH - MicroInvmode.FN - MicroInvmode.OFDeadband) /MicroInvmode.Pmax ;    //
    Init_PI_Controller(&MicroInvmode.FreqCntl, 0.02, 0.0038, (MicroInvmode.FH + 0.2), MicroInvmode.FN);
    MicroInvmode.FreqCntl.yp[0] = SystemPara.InvRateFreq;
}
/*************************************************
Function: 微逆调频
Description:
Input: None
Output: None
Frequency:
*************************************************/
void MicroInvterModeFreqChange(void)
{
//    float powerTemp = 0;

    if (SystemPara.MicroInverterMode)
    {
#if 0
        powerTemp = SystemPower.Inv1.PowerTemp + SystemPower.Inv2.PowerTemp;

        if (powerTemp < -5.0f)
        {
            // 超出电池限值
            if (fabs(powerTemp) >= MicroInvmode.PcLimit)
            {
                MicroInvmode.PloadAve = 0;
            }
            else
            {
                MicroInvmode.PloadAve = MicroInvmode.PcLimit;
            }
        }
        else
        {
            MicroInvmode.PloadAve = MicroInvmode.PcLimit;
        }
#endif

        MicroInvmode.PloadAve = 0;    // 0.5*MicroInvmode.PloadAve + 0.5*powerTemp;
        MicroInvmode.FreqCntl.xp[0] = __fsat(-MicroInvmode.kOF_P*(MicroInvmode.PcLimit + MicroInvmode.PloadAve), 5.0f, -5.0f);// 2HZ/2s  =  0.01Hz/10ms
        PI_Controller(&MicroInvmode.FreqCntl);
    }
    else
    {
        MicroInvmode.FreqCntl.yp[0] =  SystemPara.InvRateFreq;
    }
}

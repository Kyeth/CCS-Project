/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name: MPPT.c
** Latest modified Date:2021-10-06
** Latest Version: V1.0
** Description:
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2021-10-06
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/
#include <MPPTFunc.h>

/******************************************************************************
// Export Function-Prototype: NormalMppt
// Description  : MPPT正常扰动功能函数
// Input        : @Private-MPPT控制变量指针
// Output       : None
// Return Value : None
// Calls        :
// Called By    : MpptFunc()
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(NormalMppt, ".TI.ramfunc");
void NormalMppt(MPPT_DATA *Private)
{
    Private->MpptPeriodCnt++;
    if (Private->MpptPeriodCnt > Private->DisturbPeriod)
    {
        Private->MpptPeriodCnt = 0;
    }
    if ((Private->MpptPeriodCnt > Private->DisturbHalfPeriod) && (Private->MpptPeriodCnt <= Private->DisturbPeriod))
    {
        Private->PowerNew += (*Private->PVVoltAvg) * (*Private->PVCurrentAVg);   // 电池电压*电感电流
    }
    else
    {
        Private->PowerNew = 0;
    }
    if (Private->MpptPeriodCnt == Private->DisturbPeriod)   // 1s
    {

        if (Private->PowerNew < Private->PowerOld)
        {
        // 当前功率偏小
            if(Private->DisturbDir == 0)
            {
                Private->DisturbDir = 1;
            }
            else
            {
                Private->DisturbDir = 0;
            }
            Private->DisturbStep = Private->DisturbStep * 0.6f;

        }
        // 当前电压对应功率比上次大
        else
        {
            Private->DisturbStep += Private->StepVolt;
        }

        //步长限幅
        Private->Powerdelta = Private->PowerNew - Private->PowerOld;

        if(Private->Powerdelta < (0.01 * Private->PowerNew))
        {
            Private->DisturbStep = 1;
        }
        if((*Private->PVCurrentAVg) < 1)
        {
            Private->DisturbStep = Private->cMpptStepMax;
        }
        Private->DisturbStep = __fsat(Private->DisturbStep, Private->cMpptStepMax, Private->cMpptStepMin);
        //电压扰动
        if(Private->BusOVFlag)
        {
            Private->DisturbDir = 1;
            Private->BusOVFlag = 0;
        }

        if (Private->DisturbDir == 0)
        {
            Private->Output -= Private->DisturbStep;
        }
        else
        {
            Private->Output += Private->DisturbStep;
        }

        //扰动异常
//        if (fabsf(Private->Output - (*Private->PVVoltAvg)) >= 20)
//        {
//            Private->MPPtErrorcnt++;
//            if (Private->MPPtErrorcnt >= 3)
//            {
//                Private->MPPtErrorcnt = 0;
//                Private->Output = (*Private->PVVoltAvg) - 10;
//            }
//        }
//        else
//        {
//            Private->MPPtErrorcnt = 0;
//        }
        if (fabsf(Private->Output - (*Private->PVVoltAvg)) >= Private->cMpptStepMax)
        {
            Private->MPPtErrorcnt++;
            if (Private->MPPtErrorcnt >= 3)
            {
                Private->MPPtErrorcnt = 0;
                Private->Output = (*Private->PVVoltAvg) - Private->cMpptStepMax;
            }
        }
        else
        {
            Private->MPPtErrorcnt = 0;
        }

        // 限幅
        Private->Output = __fmin(Private->Output, (*Private->BusVolt - 10));//开路电压大于母线
        __fsat(Private->Output, Private->MpptOutMax, Private->MpptOutMin);

        Private->PowerOld = Private->PowerNew;
    }
}
/******************************************************************************
// Export Function-Prototype: FastMppt
// Description  : MPPT快速扰动功能函数
// Input        : @Private-MPPT控制变量指针
// Output       : None
// Return Value : None
// Calls        :
// Called By    : MpptFunc()
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void FastMppt(MPPT_DATA *Private)
{
    Private->Output = (*Private->PVVoltAvg) - Private->FastStep;
    Private->FastMpptFlag = 1;                   // 完成快速扰动,第一次
    if (Private->Output > Private->MpptOutMax)   // 对PV1目标电压进行限幅
    {
        Private->Output = Private->MpptOutMax;
    }
    if (Private->Output < Private->MpptOutMin)
    {
        Private->Output = Private->MpptOutMin;
    }
}
/******************************************************************************
// Export Function-Prototype: FastMppt
// Description  : MPPT快速扰动功能函数
// Input        : @Private-MPPT控制变量指针;@volt-PV电压;@curr-PV电流;@bat-电池电压
// Output       : None
// Return Value : None
// Calls        :
// Called By    : MpptFunc()
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(MpptFunc, ".TI.ramfunc");
void MpptFunc(MPPT_DATA *mppt)
{
    if (mppt->MPPTEnable == 1)
    {
        if (mppt->FastMpptFlag == 0)     // PV1快速追踪
        {
            FastMppt(mppt);
            mppt->DelayCnt = 0 ;
        }
        else
        {
            mppt->DelayCnt++;
            if (mppt->DelayCnt >= 15000)    // 3s 后开始扰动
            {
                mppt->DelayCnt = 15000;
                if(mppt->turn ==  mppt->ID)//4多路轮流计算
                {
                    NormalMppt(mppt);
                }
            }
        }
    }
    else
    {
        mppt->FastMpptFlag = 0;
        mppt->PowerOld = 0;
        mppt->MpptPeriodCnt = 0;
        mppt->DisturbDir = 0;
        mppt->MPPtErrorcnt = 0;
        mppt->DisturbStep = mppt->cMpptStepMax;
    }
}

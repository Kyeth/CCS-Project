/*
* @file
* @author yetingheng  (004182)
* @brief
* @version 0.1
* @date 2026-03-26
*
* @copyright Copyright (c) 2026
*
*/

#include "F28x_Project.h"
#include "Hw_CanDriver.h"
#include "FaultProtect.h"
#include "SysStateMachine.h"
#include "LLCControl.h"
#include "TaskCla.h"
#include "PlatformManager.h"
#include "Hw_GpioDriver.h"
#include "ParallelProcess.h"
#include "ModbusRegProcess.h"
#include "User_CAN.h"
#include "ModbusSlaveComm.h"
#include "Schedule.h"

// 数据类型参考Modbus
typedef enum
{
    OSC_TYPE_INT8,
    OSC_TYPE_UINT8,
    OSC_TYPE_INT16,
    OSC_TYPE_UINT16,
    OSC_TYPE_INT32,
    OSC_TYPE_UINT32,
    OSC_TYPE_FLOAT
}E_OSC_TYPE;

typedef struct
{
    Uint16 u16DataType;
    void *pvOscDataAddr;
    Uint16 u16Multiple;
}ST_OSCDATA;

unsigned int u16OscSetEnable = 0;
unsigned int u16OscSetEnableISR = 0;
volatile unsigned int u16OscChannel[12];
volatile unsigned int u16OscChannel_Multiple[12] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
ST_OSCDATA stOscData[12];
ST_OSCDATA stDafaultOscData[12] = {{OSC_TYPE_UINT16, &ObjSystemCtrl.BurstFlag, 1000},
                                   {OSC_TYPE_FLOAT, &ObjSample.VBus, 10},
                                   {OSC_TYPE_FLOAT, &LLCSystemCtrl.Vref, 10},
                                   {OSC_TYPE_FLOAT, &ObjSample.VBatRaw, 100},
                                   {OSC_TYPE_FLOAT, &ObjSample.IBusRaw, 100},
                                   {OSC_TYPE_FLOAT, &ObjSample.IErr, 100},
                                   {OSC_TYPE_FLOAT, &CtrlPara.fPhase, 1000},
                                   {OSC_TYPE_FLOAT, &LLCSystemCtrl.Ratio, 100},
                                   {OSC_TYPE_FLOAT, &CtrlPara.fClaFreqSet, 10},
                                   {OSC_TYPE_FLOAT, &ObjSample.VBusFilter, 10},
                                   {OSC_TYPE_FLOAT, &LLCSystemCtrl.fPowerTar, 1},
                                   {OSC_TYPE_INT16, &CtrlPara.BoostSet, 1000},};

extern ST_MODBUS_SLAVE_TABLE_CRTL ObjModbusSlaveCtrl_1;    // Modbus从站对象

void TaskOscParamProcess(void)
{
    int16 i16Ret = 0, i = 0;
    ST_REG_SLAVE_ITEM stDrcRegTemp;

    if (u16OscSetEnable == 1)
    {
        for (i = 0; i < 12; i++)
        {
            i16Ret = ReadModbusRegisterData(ObjModbusSlaveCtrl_1.stSlaveTableArry[ObjModbusSlaveCtrl_1.eCurrentSlaveTableIndex].pstProtocolTable,
                                   u16OscChannel[i], &stDrcRegTemp);
            if (i16Ret == 0)
            {
                stOscData[i].pvOscDataAddr = stDrcRegTemp.puRegVal;
                stOscData[i].u16DataType = stDrcRegTemp.bValType;
                stOscData[i].u16Multiple = u16OscChannel_Multiple[i];
            }
            else
            {
                stOscData[i].pvOscDataAddr = stDafaultOscData[i].pvOscDataAddr;
                stOscData[i].u16DataType = stDafaultOscData[i].u16DataType;
                stOscData[i].u16Multiple = stDafaultOscData[i].u16Multiple;
            }
        }
        u16OscSetEnableISR = 1;
    }
    else
    {
        u16OscSetEnableISR = 0;
    }
    SetTaskRunStatus(TASK_OSC_PARAM_BIT);
}


int GetChannelValue(ST_OSCDATA *pstOscData, int Index)
{
    int Result = 0;

    switch (pstOscData[Index].u16DataType)
    {
    case OSC_TYPE_UINT16:
        Result = (int)((*(Uint16*)pstOscData[Index].pvOscDataAddr) * pstOscData[Index].u16Multiple);
        break;

    case OSC_TYPE_INT16:
        Result = (int)((*(int16*)pstOscData[Index].pvOscDataAddr) * pstOscData[Index].u16Multiple);
        break;

    case OSC_TYPE_INT32:
        Result = (int)((*(int32*)pstOscData[Index].pvOscDataAddr) * pstOscData[Index].u16Multiple);
        break;

    case OSC_TYPE_UINT32:
        Result = (int)((*(Uint32*)pstOscData[Index].pvOscDataAddr) * pstOscData[Index].u16Multiple);
        break;

    case OSC_TYPE_FLOAT:
        Result = (int)((*(float*)pstOscData[Index].pvOscDataAddr) * pstOscData[Index].u16Multiple);
        break;
    }
    return Result;
}




//***************************************************************************
void U16tobp32(Uint16 MDH_H,Uint16 MDH_L,Uint16 MDL_H,Uint16 MDL_L)
{
    CanaRegs.CAN_IF2DATA.bit.Data_0 = MDL_H >> 8;
    CanaRegs.CAN_IF2DATA.bit.Data_1 = MDL_H & 0xFF;
    CanaRegs.CAN_IF2DATA.bit.Data_2 = MDL_L >> 8;
    CanaRegs.CAN_IF2DATA.bit.Data_3 = MDL_L & 0xFF;
    CanaRegs.CAN_IF2DATB.bit.Data_4 = MDH_H >> 8;
    CanaRegs.CAN_IF2DATB.bit.Data_5 = MDH_H & 0xFF;
    CanaRegs.CAN_IF2DATB.bit.Data_6 = MDH_L >> 8;
    CanaRegs.CAN_IF2DATB.bit.Data_7 = MDH_L & 0xFF;
}

//*****************************************************************************
Uint32 MDL_Rx;
float MDH_Rx;

extern float DutyBat;

Uint16 u16UserCanCnt;
Uint16 u16UserCanCnt1;
#pragma CODE_SECTION(User_Can, ".ISR.ramfunc");
void User_Can(void)
{
    ST_CAN_FRAME stCanFrame1;
    Uint16 u16ReadSize = 0;
    Uint16 ID_12,ID_16;

    u16ReadSize = Hw_CanRead(CAN_ID0, &stCanFrame1, 32);
    if (u16ReadSize)
    {
        switch (stCanFrame1.unDataA.Words.LowWord)
        {
        case 0:
        // 示例 Value = stMCanFrame.unDataB.Words.LowWord
        break;
        }
    }

    u16UserCanCnt1++;
    if (u16OscSetEnableISR == 0)
    {

        if (u16UserCanCnt1 % 2 == 0)
            {
                u16UserCanCnt++;
                if(u16UserCanCnt%2 == 0)
                {

                    ID_12 = GetChannelValue(&stDafaultOscData[0], 6);//(CpuAdcSample.ParalelCtrlState *1000);//(CtrlPara.SoftStartVolt);
                    ID_16 = (int)GetChannelValue(&stDafaultOscData[0],7);//(CtrlPara.fs0*10);//
                    stCanFrame1.bID = (((Uint32)ID_12 << 16) | ((Uint32)ID_16)) | (0x10000000);
                    stCanFrame1.bDLC = 8;
                    stCanFrame1.bIDE = 1;
                    stCanFrame1.bRTR = 0;
                    stCanFrame1.unDataB.Words.HighWord = (int)GetChannelValue(&stDafaultOscData[0], 8);//CtrlPara.fClaFreqSet
                    stCanFrame1.unDataB.Words.LowWord = (int)GetChannelValue(&stDafaultOscData[0], 9);//CtrlPara.fClaFreqSet
                    stCanFrame1.unDataA.Words.HighWord = (int)GetChannelValue(&stDafaultOscData[0], 10);
                    stCanFrame1.unDataA.Words.LowWord = (int)GetChannelValue(&stDafaultOscData[0], 11);

                    Hw_CanWrite(CAN_ID0, &stCanFrame1, 1);
                }
                else
                {

                    ID_12 = (int)GetChannelValue(&stDafaultOscData[0], 0);//(LLCSystemCtrl.RelayFlag * 1000);//ObjPlatformCtrl.u16AdcBRawResult[0];
                    ID_16 = (int)GetChannelValue(&stDafaultOscData[0], 1);//CtrlPara.DisCurrPICtrl.i1;//
                    stCanFrame1.bID =(((Uint32)ID_12 << 16) | ((Uint32)ID_16)) & (0x0FFFFFFF);
                    stCanFrame1.bDLC = 8;
                    stCanFrame1.bIDE = 1;
                    stCanFrame1.bRTR = 0;
                    stCanFrame1.unDataB.Words.HighWord = (int)GetChannelValue(&stDafaultOscData[0], 2);//(ObjSample.VBat * 100);//CtrlPara.DisVoltPICtrl.up
                    stCanFrame1.unDataB.Words.LowWord = (int)GetChannelValue(&stDafaultOscData[0], 3);//(ObjSample.VAirFront*10);//ObjSample.IErrFilter * 100);//CtrlPara.DisCurrPICtrl.up LLCSystemCtrl.Vref * 10
                    stCanFrame1.unDataA.Words.HighWord = (int)GetChannelValue(&stDafaultOscData[0], 4);//(CpuAdcSample.Iref *100);//(CtrlPara.ClaRefSoftFreq.SetpointValue);//(ObjPlatformCtrl.u16AdcBRawResult[2]);//(ObjSample.IBus * 10);CtrlPara.ClaRefFs0.StepValue
                    stCanFrame1.unDataA.Words.LowWord = (int)GetChannelValue(&stDafaultOscData[0], 5);//(ObjPlatformCtrl.u16AdcARawResult[11]);
                    Hw_CanWrite(CAN_ID0, &stCanFrame1, 1);
                }
            }
    }
    else
    {
        if (u16UserCanCnt1 % 2 == 0)
        {
            u16UserCanCnt++;
            if(u16UserCanCnt%2 == 0)
            {

                ID_12 = GetChannelValue(&stOscData[0], 6);//(CpuAdcSample.ParalelCtrlState *1000);//(CtrlPara.SoftStartVolt);
                ID_16 = (int)GetChannelValue(&stOscData[0],7);//(CtrlPara.fs0*10);//
                stCanFrame1.bID = (((Uint32)ID_12 << 16) | ((Uint32)ID_16)) | (0x10000000);
                stCanFrame1.bDLC = 8;
                stCanFrame1.bIDE = 1;
                stCanFrame1.bRTR = 0;
                stCanFrame1.unDataB.Words.HighWord = (int)GetChannelValue(&stOscData[0], 8);//CtrlPara.fClaFreqSet
                stCanFrame1.unDataB.Words.LowWord = (int)GetChannelValue(&stOscData[0], 9);//CtrlPara.fClaFreqSet
                stCanFrame1.unDataA.Words.HighWord = (int)GetChannelValue(&stOscData[0], 10);
                stCanFrame1.unDataA.Words.LowWord = (int)GetChannelValue(&stOscData[0], 11);

                Hw_CanWrite(CAN_ID0, &stCanFrame1, 1);

            }
            else
            {
                ID_12 = (int)GetChannelValue(&stOscData[0], 0);//(LLCSystemCtrl.RelayFlag * 1000);//ObjPlatformCtrl.u16AdcBRawResult[0];
                ID_16 = (int)GetChannelValue(&stOscData[0], 1);//CtrlPara.DisCurrPICtrl.i1;//
                stCanFrame1.bID =(((Uint32)ID_12 << 16) | ((Uint32)ID_16)) & (0x0FFFFFFF);
                stCanFrame1.bDLC = 8;
                stCanFrame1.bIDE = 1;
                stCanFrame1.bRTR = 0;
                stCanFrame1.unDataB.Words.HighWord = (int)GetChannelValue(&stOscData[0], 2);//(ObjSample.VBat * 100);//CtrlPara.DisVoltPICtrl.up
                stCanFrame1.unDataB.Words.LowWord = (int)GetChannelValue(&stOscData[0], 3);//(ObjSample.VAirFront*10);//ObjSample.IErrFilter * 100);//CtrlPara.DisCurrPICtrl.up LLCSystemCtrl.Vref * 10
                stCanFrame1.unDataA.Words.HighWord = (int)GetChannelValue(&stOscData[0], 4);//(CpuAdcSample.Iref *100);//(CtrlPara.ClaRefSoftFreq.SetpointValue);//(ObjPlatformCtrl.u16AdcBRawResult[2]);//(ObjSample.IBus * 10);CtrlPara.ClaRefFs0.StepValue
                stCanFrame1.unDataA.Words.LowWord = (int)GetChannelValue(&stOscData[0], 5);//(ObjPlatformCtrl.u16AdcARawResult[11]);
                Hw_CanWrite(CAN_ID0, &stCanFrame1, 1);
            }
        }
    }
}


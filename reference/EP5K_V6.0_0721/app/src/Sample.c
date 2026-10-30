/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name: Sample.c
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
#define   _GLOBALS
#include <math.h>
#include <string.h>
#include "F28x_Project.h"
#include "Sample.h"
#include "PowerCalculator.h"
#include "ConfigSystemPara.h"
#include "PVStateMachine.h"
#include "InvStateMachine.h"
#include "CalculateAngle.h"
#include "Protect.h"
#include "bsp.h"
#include "Function.h"
#include "OnGridInv2.h"
#include "ModbusTable.h"
ADC_SAMPLE_RAW SampleADRaw;                   // 原始值
ADC_SAMPLE_INS SampleIns;                     // 瞬时值
ADC_SAMPLE_RMS SampleRms;                     // 有效值
AD_SAMPLE_ZERO_CALIBRATE SampleZeroCalibrate; // 零飘
ADC_SAMPLE_COEF SampleCalibCoef;              // 标定系数
SAMPLE_DISPLAY SampleDisplayData;             // 显示数据

#pragma DATA_SECTION(Cla1ToCpuMsg,"Cla1ToCpuMsgRAM");
#pragma DATA_SECTION(CpuToCla1Msg,"CpuToCla1MsgRAM");

/******************************************************************************
// Export Function-Prototype: GeneralMoudlarSample
// Description  : 初始化 AD 采样模块变量
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    : InitModularData()
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void InitSampleData(void)
{
    memset(&SampleADRaw, 0, sizeof(ADC_SAMPLE_RAW));
    memset(&SampleIns, 0, sizeof(ADC_SAMPLE_INS));
    memset(&SampleRms, 0, sizeof(ADC_SAMPLE_RMS));
    memset(&SampleCalibCoef, 0, sizeof(ADC_SAMPLE_COEF));
    memset(&SampleDisplayData, 0, sizeof(SAMPLE_DISPLAY));

    SampleIns.PeriodPoint = (SystemPara.FrequencySet == 1) ? 167 :200;

    //105 105.1 104.7 104.1 104.1
    //200 200   199.5 199.2 199.6
    //300 300   299.4 299.6 300.2
    //400 399.1 398.6 399.1 400.1
    // 采样标定系数
    SampleCalibCoef.BUSVoltK   = 6265;
    SampleCalibCoef.Bus1VoltK  = 3290;    //  //3315 ->3259
    SampleCalibCoef.Bus2VoltK  = 3315;

    SampleCalibCoef.PV1VoltK   = 6285;     // @sjj100V工况校准,TI运放
    SampleCalibCoef.PV2VoltK   = 6263;     // 6245->6263
    SampleCalibCoef.PV3VoltK   = 6324;     // 6270->6324
    SampleCalibCoef.PV4VoltK   = 6304;     // 6250->6304
    SampleCalibCoef.PV1CurrK   = (4544*1.5);  //(4500*1.5);  //
    SampleCalibCoef.PV2CurrK   = (4544*1.5);  //(4570*1.5);  //
    SampleCalibCoef.PV3CurrK   = (4544*1.5);  //(4530*1.5);  //
    SampleCalibCoef.PV4CurrK   = (4550*1.5);  //(4550*1.5);  //
    SampleCalibCoef.PV1Effc  = 0.01;
    SampleCalibCoef.PV2Effc  = 0.01;
    SampleCalibCoef.PV3Effc  = 0.01;
    SampleCalibCoef.PV4Effc  = 0.01;


    SampleCalibCoef.Inv1VoltR  = 0.1;
    SampleCalibCoef.Inv2VoltR  = 0.1;

    SampleCalibCoef.Inv1CurrK  = 2359;    //@sjj已校准TI运放 // 放电
    SampleCalibCoef.Inv2CurrK  = 2392;    //@sjj已校准TI运放 // 放电
//    SampleCalibCoef.Inv1CurrK  = 2282;    //@sjj已校准TI运放 // 充电
//    SampleCalibCoef.Inv2CurrK  = 2238;    //@sjj已校准TI运放 // 充电

    SampleCalibCoef.Grid1VoltK = 6620;    //@sjj已校准
    SampleCalibCoef.Grid2VoltK = 6645;    //@sjj已校准
    SampleCalibCoef.Inv1VoltK  = 6642;    //@sjj已校准TI运放
    SampleCalibCoef.Inv2VoltK  = 6629;    //@sjj已校准TI运放
    SampleCalibCoef.Load1VoltK = 6706;    //@sjj已校准TI运放
    SampleCalibCoef.Load2VoltK = 6745;    //@sjj已校准TI运放
    SampleCalibCoef.Mid1VoltK  = 6680;
    SampleCalibCoef.Mid2VoltK  = 6730;

    SampleCalibCoef.Load1CurrK = 2020;
    SampleCalibCoef.Load2CurrK = 2020;
    SampleCalibCoef.DCCurr1K  = (1980*2 * (1.5/1.4888f));  //直流分量1.98A->1.49V；
    SampleCalibCoef.DCCurr2K  = (1980*2 * (1.5/1.4888f));  //
    SampleCalibCoef.Grid1CurrK  = (1431*2);  //
    SampleCalibCoef.Grid2CurrK  = (1431*2);  //143.12A->3V
    SampleCalibCoef.ACPVCurrK  = (1431*2);   //143.12Adc->3V
    SampleCalibCoef.LnCurrK = (691*2);  // 69.1A->3V

    ResetPowerCalculator(&SystemPower.Grid1);
    ResetPowerCalculator(&SystemPower.Grid2);
    ResetPowerCalculator(&SystemPower.Inv1);
    ResetPowerCalculator(&SystemPower.Inv2);
    ResetPowerCalculator(&SystemPower.Load1);
    ResetPowerCalculator(&SystemPower.Load2);
}
//=================================================================================
// 函数名称:  GPIO多路选择器
// 函数功能:  选通GPIO的开关状态影响外部电子开关选项
// 输入参数:  GPIO46 GPIO47 GPIO42 对应S0 S1 S2
// 输出参数: 无
// 其它说明: 无
//=================================================================================
void Gpio_Multiplexer(int16_t senum)
{
    switch(senum)
    {
    case L1_LOAD_CUR:
    {
        //L1相负载电流
        SEL_A_S0_OFF
        SEL_B_S1_OFF
        SEL_C_S2_OFF
        break;
    }
    case L2_LOAD_CUR:
    {
        //L2相负载电流
        SEL_A_S0_ON
        SEL_B_S1_OFF
        SEL_C_S2_OFF
        break;
    }
    case L1_MID_VOL:
    {
        //L1相中线电压
        SEL_A_S0_OFF
        SEL_B_S1_OFF
        SEL_C_S2_ON
        break;
    }
    case L2_MID_VOL:
    {
        //L2相中线电压
        SEL_A_S0_ON
        SEL_B_S1_OFF
        SEL_C_S2_ON
        break;
    }
    case L1_LOAD_VOL:
    {
        //L1相负载电压
        SEL_A_S0_OFF
        SEL_B_S1_ON
        SEL_C_S2_ON
        break;
    }
    case L2_LOAD_VOL:
    {
        //L2相负载电压
        SEL_A_S0_ON
        SEL_B_S1_ON
        SEL_C_S2_ON
        break;
    }
    default:break;
    }
}
/******************************************************************************
// Export Function-Prototype: GeneralMoudlarSample
// Description  : 检查采样零飘值是否正确,零飘值要在理论值的±15%范围内
// Input        : None
// Output       : None
// Return Value : 1-零飘值检测异常;0-零飘值正常
// Calls        :
// Called By    : CalibrateADZero()
// History      :
// 1.Date       : 2022/04/13
// Author       : tzz
// Modification : Created Function
******************************************************************************/
uint16_t CheckADZero(void)
{
    uint16_t ret = 0;

    if (abs(SampleZeroCalibrate.PV1CurrZero - 2275) > 410)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.PV2CurrZero - 2275) > 410)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.PV3CurrZero - 2275) > 410)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.PV4CurrZero - 2275) > 410)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Load1CurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Load2CurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.LnCurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.ACPVCurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Load1VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Load2VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Mid1VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Mid2VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Inv1VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Inv2VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Inv1CurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Inv2CurrZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Grid1VoltZero - 2048) > 205)
    {
        ret = 1;
    }
    else if (abs(SampleZeroCalibrate.Grid2VoltZero - 2048) > 205)
    {
        ret = 1;
    }

    return ret;
}

/******************************************************************************
// Exported Function-Prototype:
// Description  : 读取 AD 原始数据
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    : ISRTimer1()
// History      :
// 1.Date       : 2021/07/29
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(ReadADCRawValues, ".TI.ramfunc");
void ReadADCRawValues(void)
{
    //负载电流12、负载电压12、Vbus、VISO、Vpre-cL12
    //M.DCI-L12、M.BUSHL、M.I-ACPV_L1、NC、Temp7、Temp8
    SampleADRaw.Load1CurrRaw = Cla1ToCpuMsg.adresult1[0];
    SampleADRaw.Load2CurrRaw = Cla1ToCpuMsg.adresult1[1];
    SampleADRaw.Load1VoltRaw = Cla1ToCpuMsg.adresult1[2];
    SampleADRaw.Load2VoltRaw = Cla1ToCpuMsg.adresult1[3];
    SampleADRaw.BUSVoltRaw   = Cla1ToCpuMsg.adresult1[4];
    SampleADRaw.Mid1VoltRaw = Cla1ToCpuMsg.adresult1[6];
    SampleADRaw.Mid2VoltRaw = Cla1ToCpuMsg.adresult1[7];

    SampleADRaw.DCCurr1Raw = Cla1ToCpuMsg.adresult2[0];    // 上半BUS电压
    SampleADRaw.DCCurr2Raw = Cla1ToCpuMsg.adresult2[1];    // 下半BUS电压
    SampleADRaw.Bus1VoltRaw = Cla1ToCpuMsg.adresult2[2];   // 上半BUS电压
    SampleADRaw.Bus2VoltRaw = Cla1ToCpuMsg.adresult2[3];   // 下半BUS电压
    SampleADRaw.ACPVCurrRaw = Cla1ToCpuMsg.adresult2[4];   // 下半BUS电压

    SampleADRaw.LnCurrRaw = AdcdResultRegs.ADCRESULT15;

    SampleADRaw.Grid1CurrRaw = AdcdResultRegs.ADCRESULT4;
    SampleADRaw.Grid2CurrRaw = AdcaResultRegs.ADCRESULT0;

//    SampleADRaw.Inv1VoltRaw = Cla1ToCpuMsg.adresultV1;
//    SampleADRaw.Inv2VoltRaw = Cla1ToCpuMsg.adresultV2;

    SampleADRaw.Inv1VoltRaw = (AdccResultRegs.ADCRESULT4 + AdccResultRegs.ADCRESULT7)>>1;   //逆变电压
    SampleADRaw.Inv2VoltRaw = (AdccResultRegs.ADCRESULT3 + AdccResultRegs.ADCRESULT6)>>1;

    SampleADRaw.Inv1CurrRaw = (AdcdResultRegs.ADCRESULT14 + AdcdResultRegs.ADCRESULT13)>>1;   //逆变电流
    SampleADRaw.Inv2CurrRaw = (AdcdResultRegs.ADCRESULT2  + AdcdResultRegs.ADCRESULT12)>>1;

    SampleADRaw.Grid1VoltRaw = AdcaResultRegs.ADCRESULT5;    //电网电压
    SampleADRaw.Grid2VoltRaw = AdcaResultRegs.ADCRESULT1;

    SampleADRaw.PV1VoltRaw = AdcbResultRegs.ADCRESULT1;     //PV1电压
    SampleADRaw.PV2VoltRaw = AdcbResultRegs.ADCRESULT0;     //PV2电压
    SampleADRaw.PV3VoltRaw = AdcbResultRegs.ADCRESULT2;     //PV3电压
    SampleADRaw.PV4VoltRaw = AdcaResultRegs.ADCRESULT2;     //PV4电压
    SampleADRaw.PV1CurrRaw = (AdcaResultRegs.ADCRESULT4 + AdcaResultRegs.ADCRESULT6)>>1;     //PV1电流
    SampleADRaw.PV2CurrRaw = (AdccResultRegs.ADCRESULT2 + AdccResultRegs.ADCRESULT5)>>1;     //PV2电流
    SampleADRaw.PV3CurrRaw = (AdcdResultRegs.ADCRESULT0 + AdcdResultRegs.ADCRESULT5)>>1;     //PV3电流
    SampleADRaw.PV4CurrRaw = (AdcaResultRegs.ADCRESULT3 + AdcaResultRegs.ADCRESULT7)>>1;     //PV4电流

//    SampleADRaw.AlterSelectRaw = AdcdResultRegs.ADCRESULT3;   //选通采样

}
/************************************************************************
 // 确保PV开路电压比母线小至少20V,但是与真实零点偏移，会导致PV采样电流不准
************************************************************************/
void PVAutoCheck(void)
{
      int PVnumber = 0;
//    if(((SampleADRaw.PV1VoltRaw + 130 < SampleADRaw.BUSVoltRaw)
//        &&(SampleADRaw.PV2VoltRaw + 130 < SampleADRaw.BUSVoltRaw)
//        &&(SampleADRaw.PV3VoltRaw + 130 < SampleADRaw.BUSVoltRaw)
//        &&(SampleADRaw.PV4VoltRaw + 130 < SampleADRaw.BUSVoltRaw))
//        || SampleADRaw.BUSVoltRaw < 200)
//    {
      SampleIns.PVAutoZeroFlag = 1;

//    }
//    else
//    {
//        SampleIns.PVAutoZeroFlag = 0;
//    }

      // 93V/9  440V/3 //PV激活 >75V工作
      if(SampleADRaw.BUSVoltRaw > 280)
      {
          if(SampleADRaw.PV1VoltRaw > SampleADRaw.BUSVoltRaw )
          {
              PVnumber++;
          }
          if(SampleADRaw.PV2VoltRaw > SampleADRaw.BUSVoltRaw )
          {
              PVnumber++;
          }
          if(SampleADRaw.PV3VoltRaw > SampleADRaw.BUSVoltRaw )
          {
              PVnumber++;
          }
          if(SampleADRaw.PV4VoltRaw > SampleADRaw.BUSVoltRaw )
          {
              PVnumber++;
          }
      }

      if(PVnumber)
      {
          SampleZeroCalibrate.PV1CurrZero2 = -__fsat((1200L*10*4096/(float)6265)/(float)SampleADRaw.BUSVoltRaw,9,3)/PVnumber;
          SampleZeroCalibrate.PV2CurrZero2 = SampleZeroCalibrate.PV1CurrZero2;
          SampleZeroCalibrate.PV3CurrZero2 = SampleZeroCalibrate.PV1CurrZero2;
          SampleZeroCalibrate.PV4CurrZero2 = SampleZeroCalibrate.PV1CurrZero2;
      }

      if(SampleADRaw.PV1VoltRaw < SampleADRaw.BUSVoltRaw )
      {
          SampleZeroCalibrate.PV1CurrZero2 = 0;
      }
      if(SampleADRaw.PV2VoltRaw < SampleADRaw.BUSVoltRaw )
      {
          SampleZeroCalibrate.PV2CurrZero2 = 0;
      }
      if(SampleADRaw.PV3VoltRaw < SampleADRaw.BUSVoltRaw )
      {
          SampleZeroCalibrate.PV3CurrZero2 = 0;
      }
      if(SampleADRaw.PV4VoltRaw < SampleADRaw.BUSVoltRaw )
      {
          SampleZeroCalibrate.PV4CurrZero2 = 0;
      }

}
/******************************************************************************
// Local Function-Prototype: GeneralMoudlarADC
// Description  : 检查电压电流AD零飘值是否在正常范围内,正常应该是2048附近,±15%
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/08/30
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void CalibrateADZero(void)
{
    if (SampleIns.CalSampleZeroFlag == 0)
    {
        // 上电前3秒采样数据不做处理
        SampleIns.LoadZeroCalCnt++;
        if (SampleIns.LoadZeroCalCnt >= 30000)
        {
            if(SampleIns.ACSampleZeroFlag == 0)
            {
                // 4096次零飘累加值
                if (SampleIns.LoadZeroCalCnt <= 34000)
                {
                    SampleIns.Grid1VoltSum  += SampleADRaw.Grid1VoltRaw;
                    SampleIns.Grid2VoltSum  += SampleADRaw.Grid2VoltRaw;
                    SampleIns.Load1VoltSum   += SampleADRaw.Load1VoltRaw;
                    SampleIns.Load2VoltSum   += SampleADRaw.Load2VoltRaw;
                    SampleIns.Inv1VoltSum   += SampleADRaw.Inv1VoltRaw;
                    SampleIns.Inv2VoltSum   += SampleADRaw.Inv2VoltRaw;
                    SampleIns.Inv1CurrSum   += SampleADRaw.Inv1CurrRaw;
                    SampleIns.Inv2CurrSum   += SampleADRaw.Inv2CurrRaw;
                    SampleIns.Mid1VoltSum   += SampleADRaw.Mid1VoltRaw;
                    SampleIns.Mid2VoltSum   += SampleADRaw.Mid2VoltRaw;
                    SampleIns.DCCurr1Sum   += SampleADRaw.DCCurr1Raw;
                    SampleIns.DCCurr2Sum   += SampleADRaw.DCCurr2Raw;

                    SampleIns.Grid1CurrSum   += SampleADRaw.Grid1CurrRaw;
                    SampleIns.Grid2CurrSum   += SampleADRaw.Grid2CurrRaw;
                    SampleIns.Load1CurrSum   += SampleADRaw.Load1CurrRaw;
                    SampleIns.Load2CurrSum   += SampleADRaw.Load2CurrRaw;
                    SampleIns.ACPVCurrSum   += SampleADRaw.ACPVCurrRaw;
                    SampleIns.LnCurrSum   += SampleADRaw.LnCurrRaw;

                    if(SampleIns.LoadZeroCalCnt == 34000)
                    {
                        //PV = 2502 2031 2028
                        //PV = 666  2027 2026
                        //PV = 0    2026 2024

                        //单相
//                        SampleZeroCalibrate.Inv1VoltZero  = (int16)(2034);  //V2.0参数
//                        SampleZeroCalibrate.Inv2VoltZero  = (int16)(2032);  //

//                        SampleZeroCalibrate.Inv1VoltZero  = (int16)(2056);  //RCD=2056 R=2058
//                        SampleZeroCalibrate.Inv2VoltZero  = (int16)(2078);  //RCD=2078 R=2071  //双火  Inv2VoltZero = Inv1VoltZero + 22
                        //
//                        SampleZeroCalibrate.Load1VoltZero  = (int16)(2032);
//                        SampleZeroCalibrate.Load2VoltZero  = (int16)(2032);
//                        SampleZeroCalibrate.Load1CurrZero  = (int16)(2033);
//                        SampleZeroCalibrate.Load2CurrZero  = (int16)(2033);
//                        SampleZeroCalibrate.Grid1CurrZero  = (int16)(2048);
//                        SampleZeroCalibrate.Grid2CurrZero  = (int16)(2007); //硬件
//                        SampleZeroCalibrate.ACPVCurrZero  = (int16)(2048);
//                        SampleZeroCalibrate.LnCurrZero  = (int16)(2033);

                        SampleZeroCalibrate.Grid1CurrZero = (int16)(SampleIns.Grid1CurrSum *0.00025f) ;
                        SampleZeroCalibrate.Grid2CurrZero = (int16)(SampleIns.Grid2CurrSum *0.00025f) ;
                        SampleZeroCalibrate.Load1CurrZero = (int16)(SampleIns.Load1CurrSum *0.00025f) ;
                        SampleZeroCalibrate.Load2CurrZero = (int16)(SampleIns.Load2CurrSum *0.00025f) ;
                        SampleZeroCalibrate.ACPVCurrZero = (int16)(SampleIns.ACPVCurrSum *0.00025f) ;
                        SampleZeroCalibrate.LnCurrZero = (int16)(SampleIns.LnCurrSum *0.00025f) ;

                        SampleZeroCalibrate.DCCurr1Zero = (int16)(SampleIns.DCCurr1Sum *0.00025f);
                        SampleZeroCalibrate.DCCurr2Zero = (int16)(SampleIns.DCCurr2Sum *0.00025f);
                        SampleZeroCalibrate.Load1VoltZero  = (int16)(SampleIns.Load1VoltSum *0.00025f);
                        SampleZeroCalibrate.Load2VoltZero  = (int16)(SampleIns.Load2VoltSum *0.00025f);
                        SampleZeroCalibrate.Grid1VoltZero = (int16)(SampleIns.Grid1VoltSum *0.00025f);
                        SampleZeroCalibrate.Grid2VoltZero = (int16)(SampleIns.Grid2VoltSum *0.00025f);
                        SampleZeroCalibrate.Inv1VoltZero  = (int16)(SampleIns.Inv1VoltSum *0.00025f);
                        SampleZeroCalibrate.Inv2VoltZero  = (int16)(SampleIns.Inv2VoltSum *0.00025f);
                        SampleZeroCalibrate.Inv1CurrZero  = (int16)(SampleIns.Inv1CurrSum *0.00025f);
                        SampleZeroCalibrate.Inv2CurrZero  = (int16)(SampleIns.Inv2CurrSum *0.00025f);
                        SampleZeroCalibrate.Mid1VoltZero  = (int16)(SampleIns.Mid1VoltSum *0.00025f);
                        SampleZeroCalibrate.Mid2VoltZero  = (int16)(SampleIns.Mid2VoltSum *0.00025f);

                        SampleIns.ACSampleZeroFlag = 1;
                        SampleIns.Grid1VoltSum  = 0;
                        SampleIns.Grid2VoltSum  = 0;
                        SampleIns.Load1VoltSum   = 0;
                        SampleIns.Load2VoltSum   = 0;
                        SampleIns.Inv1VoltSum   = 0;
                        SampleIns.Inv2VoltSum   = 0;
                        SampleIns.Inv1CurrSum   = 0;
                        SampleIns.Inv2CurrSum   = 0;
                        SampleIns.Mid1VoltSum   = 0;
                        SampleIns.Mid2VoltSum   = 0;
                        SampleIns.DCCurr1Sum= 0;
                        SampleIns.DCCurr2Sum= 0;

                        SampleIns.Grid1CurrSum   = 0;
                        SampleIns.Grid2CurrSum   = 0;
                        SampleIns.Load1CurrSum   = 0;
                        SampleIns.Load2CurrSum   = 0;
                        SampleIns.ACPVCurrSum   = 0;
                        SampleIns.LnCurrSum   = 0;


                    }
                }
            }
            if(SampleIns.DCSampleZeroFlag == 0)
            {
                PVAutoCheck();
                if(SampleIns.PVAutoZeroFlag == 1)
                {
                    if (SampleIns.LoadZeroCalCnt <= 34000)// 4000次零飘累加值
                    {
                        SampleIns.PV1CurrSum8  += SampleADRaw.PV1CurrRaw;
                        SampleIns.PV2CurrSum8  += SampleADRaw.PV2CurrRaw;
                        SampleIns.PV3CurrSum8  += SampleADRaw.PV3CurrRaw;
                        SampleIns.PV4CurrSum8  += SampleADRaw.PV4CurrRaw;

                        if (SampleIns.LoadZeroCalCnt == 34000)
                        {
                            SampleZeroCalibrate.PV1CurrZero  = (int16)(SampleIns.PV1CurrSum8 *0.00025f) + SampleZeroCalibrate.PV1CurrZero2;
                            SampleZeroCalibrate.PV2CurrZero  = (int16)(SampleIns.PV2CurrSum8 *0.00025f) + SampleZeroCalibrate.PV2CurrZero2 ;
                            SampleZeroCalibrate.PV3CurrZero  = (int16)(SampleIns.PV3CurrSum8 *0.00025f) + SampleZeroCalibrate.PV3CurrZero2 ;
                            SampleZeroCalibrate.PV4CurrZero  = (int16)(SampleIns.PV4CurrSum8 *0.00025f) + SampleZeroCalibrate.PV4CurrZero2 ;
                            SampleIns.DCSampleZeroFlag = 1;
                            SampleIns.PV1CurrSum8  = 0;
                            SampleIns.PV2CurrSum8  = 0;
                            SampleIns.PV3CurrSum8  = 0;
                            SampleIns.PV4CurrSum8  = 0;
                        }
                    }
                }
                else
                {
                    SampleZeroCalibrate.PV1CurrZero = 2256;
                    SampleZeroCalibrate.PV2CurrZero = 2271;
                    SampleZeroCalibrate.PV3CurrZero = 2274;
                    SampleZeroCalibrate.PV4CurrZero = 2273;
                    SampleIns.DCSampleZeroFlag = 1;
                }
            }
        }
        if((SampleIns.CalSampleZeroFlag == 0)
          &&(SampleIns.ACSampleZeroFlag == 1)
          &&(SampleIns.DCSampleZeroFlag == 1))
//                &&(SampleIns.SelectSampleZeroFlag == 1)
        {
            // 判断零飘是否正常
            if (CheckADZero() == 1)
            {
                SampleIns.CalSampleZeroFlag = 0;       // 零飘检测异常
                SystemError.Fault3.bit.ADZeroErr = 1;
            }
            else
            {
                SampleIns.CalSampleZeroFlag = 1;       // 零飘检测结束
            }

            SampleIns.LoadZeroCalCnt = 0;
        }
    }
}
/*************************************************
Function: CorrectPVVolt
Description: 修正PV电压采样值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
float CorrectPVVolt(float raw)
{
    float ret = 0;

    if(raw >= 300.0f)
    {
        ret = raw*1.0f;
    }
    else
    {
        ret = raw*1.002f;
    }

    return ret;
}
/*************************************************
Function: MulTiSecWait
Description: 多路选择器切换等待
Input: None
Output: None
Frequency: 10KHz
*************************************************/
void MulTiSecWait(uint16_t Delay)
{
    if(++SampleIns.Count4 >= Delay)
    {
        SampleIns.Count4 = 0;
        SampleIns.SwitchStep ++;
    }
}
/*************************************************
Function: CalculatePower
Description: 计算交流采样信号有效值及功率值
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(CalculatePower, ".TI.ramfunc");
void CalculatePower(void)
{
    int16_t tempData = 0;
    float fPeriodPoint = 0;
    float sqrteTemp = 0;

    if (SampleIns.CalSampleZeroFlag || SystemError.Fault3.bit.ADZeroErr)   // 零飘结束
    {
        // ----------------------------PV电压/电流-----------------------------
        // pv1 voltage
        SampleIns.PV1VoltIns = ((int32_t)SampleCalibCoef.PV1VoltK*SampleADRaw.PV1VoltRaw>>12)*0.1f;
        SampleIns.PV1VoltIns = CorrectPVVolt(SampleIns.PV1VoltIns);

        SampleIns.PV1VoltSum8 += SampleIns.PV1VoltIns;
        // pv2 voltage
        SampleIns.PV2VoltIns = ((int32_t)SampleCalibCoef.PV2VoltK*SampleADRaw.PV2VoltRaw>>12)*0.1f;
        SampleIns.PV2VoltIns = CorrectPVVolt(SampleIns.PV2VoltIns);

        SampleIns.PV2VoltSum8 += SampleIns.PV2VoltIns;
        // pv3 voltage
        SampleIns.PV3VoltIns = ((int32_t)SampleCalibCoef.PV3VoltK*SampleADRaw.PV3VoltRaw>>12)*0.1f;
        SampleIns.PV3VoltIns = CorrectPVVolt(SampleIns.PV3VoltIns);

        SampleIns.PV3VoltSum8 += SampleIns.PV3VoltIns;
        // pv4 voltage
        SampleIns.PV4VoltIns = ((int32_t)SampleCalibCoef.PV4VoltK*SampleADRaw.PV4VoltRaw>>12)*0.1f;
        SampleIns.PV4VoltIns = CorrectPVVolt(SampleIns.PV4VoltIns);

        SampleIns.PV4VoltSum8 += SampleIns.PV4VoltIns;

        //pv效率跟pv电压的关系
        //pv效率跟pv电流的关系
        // pv1 current
        tempData = SampleADRaw.PV1CurrRaw - SampleZeroCalibrate.PV1CurrZero;
        SampleIns.PV1CurrIns = ((int32_t)SampleCalibCoef.PV1CurrK*tempData>>12)*SampleCalibCoef.PV1Effc;// + 0.067
        SampleIns.PV1CurrSum8 += SampleIns.PV1CurrIns;
        // pv2 current
        tempData = SampleADRaw.PV2CurrRaw - SampleZeroCalibrate.PV2CurrZero;
        SampleIns.PV2CurrIns = ((int32_t)SampleCalibCoef.PV2CurrK*tempData>>12)*SampleCalibCoef.PV2Effc;// + 0.067
        SampleIns.PV2CurrSum8 += SampleIns.PV2CurrIns;
        // pv3 current
        tempData = SampleADRaw.PV3CurrRaw - SampleZeroCalibrate.PV3CurrZero;
        SampleIns.PV3CurrIns = ((int32_t)SampleCalibCoef.PV3CurrK*tempData>>12)*SampleCalibCoef.PV3Effc;// + 0.067
        SampleIns.PV3CurrSum8 += SampleIns.PV3CurrIns;
        // pv4 current
        tempData = SampleADRaw.PV4CurrRaw - SampleZeroCalibrate.PV4CurrZero;
        SampleIns.PV4CurrIns = ((int32_t)SampleCalibCoef.PV4CurrK*tempData>>12)*SampleCalibCoef.PV4Effc ;//+ 0.067
        SampleIns.PV4CurrSum8 += SampleIns.PV4CurrIns;

        // 直流量 母线电压 200 点滤波
        SampleIns.Count1++;
        if(InvPhase.ThetaPointR == (SampleIns.PeriodPoint>>2))
//        if (++SampleIns.Count1 >= SampleIns.PeriodPoint)
        {
            fPeriodPoint = 1.0f/(int)SampleIns.Count1;
            SampleIns.Count1 = 0;
            SampleRms.BUSVoltFilter200 = SampleIns.BUSVoltSum200 * fPeriodPoint;
            SampleRms.Bus1VoltFilter200 = SampleIns.Bus1VoltSum200 * fPeriodPoint;      // 滤波后的值作为输出控制, 6 cycle
            SampleRms.Bus2VoltFilter200 = SampleIns.Bus2VoltSum200 * fPeriodPoint;      // 滤波后的值作为输出控制
            SampleRms.PV1VoltFilter8 = SampleIns.PV1VoltSum8 * fPeriodPoint;
            SampleRms.PV2VoltFilter8 = SampleIns.PV2VoltSum8 * fPeriodPoint;
            SampleRms.PV3VoltFilter8 = SampleIns.PV3VoltSum8 * fPeriodPoint;
            SampleRms.PV4VoltFilter8 = SampleIns.PV4VoltSum8 * fPeriodPoint;

            SampleIns.BUSVoltSum200 = 0;
            SampleIns.Bus1VoltSum200 = 0;
            SampleIns.Bus2VoltSum200 = 0;
            SampleIns.PV1VoltSum8 = 0;
            SampleIns.PV2VoltSum8 = 0;
            SampleIns.PV3VoltSum8 = 0;
            SampleIns.PV4VoltSum8 = 0;
        }

//        SampleIns.Count5++;
//        if(InvPhase.ThetaPointR == 1)
//        {
//            fPeriodPoint = 1.0f/(int)SampleIns.Count5;
//            SampleIns.Count5 = 0;
//            SampleRms.BUSVoltFilter200 = SampleIns.BUSVoltSum200 * fPeriodPoint;
//            SampleIns.BUSVoltSum200 = 0;
//        }


//        SampleRms.BUSVoltFilter200 += (SampleIns.BUSVoltIns - SampleIns.BusBuffer[InvPhase.ThetaPointR])*0.005;
//        SampleIns.BusBuffer[InvPhase.ThetaPointR] =  SampleIns.BUSVoltIns;

        // 直流量 8点滤波
        if (++SampleIns.Count2 >= 8)
        {
            SampleIns.Count2 = 0;

            SampleRms.BUSVoltFilter8 = SampleIns.BUSVoltSum8 * 0.125f;
            SampleRms.Bus1VoltFilter8 = SampleIns.Bus1VoltSum8 * 0.125f;      // 滤波后的值作为输出控制
            SampleRms.Bus2VoltFilter8 = SampleIns.Bus2VoltSum8 * 0.125f;      // 滤波后的值作为输出控制

//            SampleRms.PV1VoltFilter8 = SampleIns.PV1VoltSum8 * 0.125f;
//            SampleRms.PV2VoltFilter8 = SampleIns.PV2VoltSum8 * 0.125f;
//            SampleRms.PV3VoltFilter8 = SampleIns.PV3VoltSum8 * 0.125f;
//            SampleRms.PV4VoltFilter8 = SampleIns.PV4VoltSum8 * 0.125f;

            SampleRms.PV1CurrFilter8 = SampleIns.PV1CurrSum8 * 0.125f;
            SampleRms.PV2CurrFilter8 = SampleIns.PV2CurrSum8 * 0.125f;
            SampleRms.PV3CurrFilter8 = SampleIns.PV3CurrSum8 * 0.125f;
            SampleRms.PV4CurrFilter8 = SampleIns.PV4CurrSum8 * 0.125f;

            SampleIns.BUSVoltSum8 = 0;
            SampleIns.Bus1VoltSum8 = 0;
            SampleIns.Bus2VoltSum8 = 0;

//            SampleIns.PV1VoltSum8 = 0;
//            SampleIns.PV2VoltSum8 = 0;
//            SampleIns.PV3VoltSum8 = 0;
//            SampleIns.PV4VoltSum8 = 0;

            SampleIns.PV1CurrSum8 = 0;
            SampleIns.PV2CurrSum8 = 0;
            SampleIns.PV3CurrSum8 = 0;
            SampleIns.PV4CurrSum8 = 0;
        }

        // ----------------------------电网电压--------------------------------
        SampleIns.CntGridRMS++;
        //@sjj从过零开始计算
        if((InvStateData.GridZCD && SampleRms.Grid1VoltRms > 20)|| (SampleIns.CntGridRMS >= 2*SampleIns.GridPeriodPoint))
        {
            fPeriodPoint = 1.0f/(int)SampleIns.CntGridRMS;
            SampleRms.Mid1VoltRms = sqrtf(SampleIns.Mid1VoltSum * fPeriodPoint);
            SampleIns.Mid1VoltSum = 0;
            SampleRms.Mid2VoltRms = sqrtf(SampleIns.Mid2VoltSum * fPeriodPoint);
            SampleIns.Mid2VoltSum = 0;

            sqrteTemp = SampleIns.Grid1VoltSum * fPeriodPoint;
            SampleRms.Grid1VoltRms = sqrtf(sqrteTemp);
            sqrteTemp = SampleIns.Grid2VoltSum * fPeriodPoint;
            SampleRms.Grid2VoltRms = sqrtf(sqrteTemp);

            SampleIns.CntGridRMS = 0;
            SampleIns.Grid1VoltSum = 0;
            SampleIns.Grid2VoltSum = 0;
        }

        InvStateData.GridZCD = 0;

        // 交流信号计算
        SampleIns.Count3++;
        if(InvPhase.ThetaPointR == 1)
//        if (SampleIns.Count3 >= SampleIns.PeriodPoint)
        {
            fPeriodPoint = 1.0f/(int)SampleIns.Count3;
            SampleIns.Count3 = 0;

            // 电流有效值
            SampleRms.DCCurr1Rms = SampleIns.DCCurr1Sum * fPeriodPoint;
            SampleIns.DCCurr1Sum = 0;  // 清零
            SampleRms.DCCurr2Rms = SampleIns.DCCurr2Sum * fPeriodPoint;
            SampleIns.DCCurr2Sum = 0;  // 清零

            SampleRms.LnCurrRms = sqrtf(SampleIns.LnCurrSum * fPeriodPoint);
            SampleIns.LnCurrSum = 0;
            SampleRms.LnDCCurrFilter200 = SampleIns.LnDCCurrSum200 * fPeriodPoint;
            SampleIns.LnDCCurrSum200 = 0;

            SampleRms.Load1CurrRms = sqrtf(SampleIns.Load1CurrSum * fPeriodPoint);
            SampleIns.Load1CurrSum = 0;
            SampleRms.Load2CurrRms = sqrtf(SampleIns.Load2CurrSum * fPeriodPoint);
            SampleIns.Load2CurrSum = 0;

            SampleRms.Load1VoltRms = sqrtf(SampleIns.Load1VoltSum * fPeriodPoint);
            SampleIns.Load1VoltSum = 0;
            SampleRms.Load2VoltRms = sqrtf(SampleIns.Load2VoltSum * fPeriodPoint);
            SampleIns.Load2VoltSum = 0;

            // ----------------------------INVOut1------------------------------
            // 电压有效值
            sqrteTemp = SampleIns.Inv1VoltSum * fPeriodPoint;             // 7 cycle
            SampleRms.Inv1VoltRms = sqrtf(sqrteTemp);                  // 6 cycle
            SampleIns.Inv1VoltSum = 0;  // 清零
            // 电流有效值
            sqrteTemp = SampleIns.Inv1CurrSum * fPeriodPoint;
            SampleRms.Inv1CurrRms = sqrtf(sqrteTemp);
            SampleIns.Inv1CurrSum = 0;  // 清零
            // ---------------------------INVOut2-------------------------------
            // 电压有效值
            sqrteTemp = SampleIns.Inv2VoltSum * fPeriodPoint;
            SampleRms.Inv2VoltRms = sqrtf(sqrteTemp);
            SampleIns.Inv2VoltSum = 0;
            // 电流有效值
            sqrteTemp = SampleIns.Inv2CurrSum * fPeriodPoint;
            SampleRms.Inv2CurrRms = sqrtf(sqrteTemp);
            SampleIns.Inv2CurrSum = 0;
            // ACPV
            sqrteTemp = SampleIns.ACPVCurrSum * fPeriodPoint;
            SampleRms.ACPVCurrRms = sqrtf(sqrteTemp);
            SampleIns.ACPVCurrSum = 0;
            //半波判断
            FuncPara.INVIAVG1 = FuncPara.INVISUM1 * fPeriodPoint;
            FuncPara.INVISUM1 = 0;
            FuncPara.INVIAVG2 = FuncPara.INVISUM2 * fPeriodPoint;
            FuncPara.INVISUM2 = 0;

            FuncPara.INVVAVG1 = FuncPara.INVVSUM1 * fPeriodPoint;
            FuncPara.INVVSUM1 = 0;
            FuncPara.INVVAVG2 = FuncPara.INVVSUM2 * fPeriodPoint;
            FuncPara.INVVSUM2 = 0;
            // 计算逆变输出的视在功率,用于过载保护
            ApparentPowerCalculator(&SystemPower.ApInv1, SampleRms.Inv1VoltRms, SampleRms.Inv1CurrRms);     // 逆变1视在功率
            ApparentPowerCalculator(&SystemPower.ApInv2, SampleRms.Inv2VoltRms, SampleRms.Inv2CurrRms);     //

            // 负载视在功率
            ApparentPowerCalculator(&SystemPower.ApLoad1, SampleRms.Load1VoltRms, SampleRms.Load1CurrRms);
            ApparentPowerCalculator(&SystemPower.ApLoad2, SampleRms.Load2VoltRms, SampleRms.Load2CurrRms);

            //电压调整系数
            if(InvStateData.WorkState == OffGridState || InvStateData.WorkState == UpsState)
            {
                SampleCalibCoef.Inv1VoltR = 0.1 - SampleRms.Inv1CurrRms * (0.003*0.1/27);
                SampleCalibCoef.Inv2VoltR = 0.1 - SampleRms.Inv2CurrRms * (0.003*0.1/27);
            }
            else
            {
                SampleCalibCoef.Inv1VoltR = 0.1;
                SampleCalibCoef.Inv2VoltR = 0.1;
            }

            SampleIns.AcCalOverFlag = 1;
        }

        fPeriodPoint = SampleIns.PeriodPoint;       // 使用临时变量转换数据类型
        // PV1 功率计算, 工作或PV高压灌电时计算，PV未工作时将功率清零
        if ((PV1WorkState.State == MPPT_STATE)||(SampleIns.PV1VoltIns > (SampleIns.BUSVoltIns - 10)))
        {
            PowerCalculator(&SystemPower.PV1, SampleRms.PV1VoltFilter8, SampleRms.PV1CurrFilter8, fPeriodPoint);// 93 cycle
        }
        else
        {
            ResetPowerCalculator(&SystemPower.PV1);
        }
        // PV2 功率计算
        if ((PV2WorkState.State == MPPT_STATE)||(SampleIns.PV2VoltIns > (SampleIns.BUSVoltIns - 10)))
        {
            PowerCalculator(&SystemPower.PV2, SampleRms.PV2VoltFilter8, SampleRms.PV2CurrFilter8, fPeriodPoint);
        }
        else
        {
            ResetPowerCalculator(&SystemPower.PV2);
        }
        // PV3 功率计算
        if ((PV3WorkState.State == MPPT_STATE)||(SampleIns.PV3VoltIns > (SampleIns.BUSVoltIns - 10)))
        {
            PowerCalculator(&SystemPower.PV3, SampleRms.PV3VoltFilter8, SampleRms.PV3CurrFilter8, fPeriodPoint);
        }
        else
        {
            ResetPowerCalculator(&SystemPower.PV3);
        }
        // PV4 功率计算
        if ((PV4WorkState.State == MPPT_STATE)||(SampleIns.PV4VoltIns > (SampleIns.BUSVoltIns - 10)))
        {
            PowerCalculator(&SystemPower.PV4, SampleRms.PV4VoltFilter8, SampleRms.PV4CurrFilter8, fPeriodPoint);
        }
        else
        {
            ResetPowerCalculator(&SystemPower.PV4);
        }

        if (InvStateData.WorkState >= 3)
        {
            // 逆变有功功率
            PowerCalculator(&SystemPower.Load1, SampleIns.Load1VoltIns, SampleIns.Load1CurrIns, fPeriodPoint);
            PowerCalculator(&SystemPower.Load2, SampleIns.Load2VoltIns, SampleIns.Load2CurrIns, fPeriodPoint);
            PowerCalculator(&SystemPower.Inv1, SampleIns.Inv1VoltIns, SampleIns.Inv1CurrIns, fPeriodPoint); //68 cycle
            PowerCalculator(&SystemPower.Inv2, SampleIns.Inv2VoltIns, SampleIns.Inv2CurrIns, fPeriodPoint);
            PowerCalculator(&SystemPower.Grid1, SampleIns.Grid1VoltIns, SampleIns.Grid1CurrIns, fPeriodPoint);
            PowerCalculator(&SystemPower.Grid2, SampleIns.Grid2VoltIns, SampleIns.Grid2CurrIns, fPeriodPoint);
        }
        else
        {
            // 逆变功率数据清零
            ResetPowerCalculator(&SystemPower.Inv1);
            ResetPowerCalculator(&SystemPower.Inv2);
            // 负载功率数据清零
            ResetPowerCalculator(&SystemPower.Load1);
            ResetPowerCalculator(&SystemPower.Load2);
            //
            ResetPowerCalculator(&SystemPower.Grid1);
            ResetPowerCalculator(&SystemPower.Grid2);
        }
    }
}
/******************************************************************************
// Local Function-Prototype: GeneralMoudlarSample
// Description  : 初始化 AD 采样模块变量
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    : HandleSample()
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(CalculateSampleValue, ".TI.ramfunc");
void CalculateSampleValue(void)
{
    int16_t tempData = 0;
    uint16_t  theta = 0;

    if (SampleIns.CalSampleZeroFlag || SystemError.Fault3.bit.ADZeroErr)   // 零飘结束
    {
        // ----------------------------BUS电压-----------------------------
        // BUS voltage
        SampleIns.BUSVoltIns = ((int32_t)SampleCalibCoef.BUSVoltK*SampleADRaw.BUSVoltRaw>>12)*0.1f;
        SampleIns.BUSVoltSum8 += SampleIns.BUSVoltIns;
        SampleIns.BUSVoltSum200 += SampleIns.BUSVoltIns;
        // ------------------------------BUS1/2------------------------------
        // BUS1
        SampleIns.Bus1VoltIns = ((int32_t)SampleCalibCoef.Bus1VoltK*SampleADRaw.Bus1VoltRaw>>12)*0.1f;         // 14 cycle
        SampleIns.Bus1VoltSum8 += SampleIns.Bus1VoltIns;    // 4 cycle
        SampleIns.Bus1VoltSum200 += SampleIns.Bus1VoltIns;
        // BUS2
        SampleIns.Bus2VoltIns = ((int32_t)SampleCalibCoef.Bus2VoltK*SampleADRaw.Bus2VoltRaw>>12)*0.1f;
        SampleIns.Bus2VoltSum8 += SampleIns.Bus2VoltIns;
        SampleIns.Bus2VoltSum200 += SampleIns.Bus2VoltIns;

        // BUS voltage buffer
        if(InvPhase.ThetaPointR >= (InvPhase.PeriodPoint>>1))
            theta = __fmin(InvPhase.ThetaPointR - (InvPhase.PeriodPoint>>1),129);
        else
            theta = __fmin(InvPhase.ThetaPointR,129);

        float bustmp= __fsat(SampleIns.BusLastData[theta],SampleIns.BUSVoltIns + 30,SampleIns.BUSVoltIns - 30);
        SampleIns.BUSVoltInsCom = (SampleIns.BUSVoltIns + bustmp)*0.5;
        SampleIns.BusLastData[theta] = SampleIns.BUSVoltIns;

        // ----------------------------INV1------------------------------
        //电流系数适应
        if(IrCtrl.RmpCtrl.SetpointValue >= 0)
        {
            SampleCalibCoef.Inv1CurrK  = 2359;    //@sjj已校准TI运放 // 放电
            SampleCalibCoef.Inv2CurrK  = 2392;    //@sjj已校准TI运放 // 放电
        }
        else
        {
            SampleCalibCoef.Inv1CurrK  = 2282;    //@sjj已校准TI运放 // 充电
            SampleCalibCoef.Inv2CurrK  = 2238;    //@sjj已校准TI运放 // 充电
        }
        //电压
        tempData = SampleADRaw.Inv1VoltRaw - SampleZeroCalibrate.Inv1VoltZero;
        SampleIns.Inv1VoltIns = ((int32_t)SampleCalibCoef.Inv1VoltK*tempData>>12)*SampleCalibCoef.Inv1VoltR;
        SampleIns.Inv1VoltSum += SampleIns.Inv1VoltIns*SampleIns.Inv1VoltIns;
        //电流
        tempData = SampleADRaw.Inv1CurrRaw - SampleZeroCalibrate.Inv1CurrZero;
        SampleIns.Inv1CurrIns = ((int32_t)SampleCalibCoef.Inv1CurrK*tempData>>12)*0.1f;
        SampleIns.Inv1CurrSum += SampleIns.Inv1CurrIns*SampleIns.Inv1CurrIns;
        //半波判断L1
        FuncPara.INVISUM1 += SampleIns.Inv1CurrIns;
        FuncPara.INVVSUM1 += SampleIns.Inv1VoltIns;

        tempData = SampleADRaw.DCCurr1Raw - SampleZeroCalibrate.DCCurr1Zero;
        SampleIns.DCCurr1Ins = ((int32_t)SampleCalibCoef.DCCurr1K*tempData>>12);
        SampleIns.DCCurr1Sum += SampleIns.DCCurr1Ins;
        // ---------------------------INV2-------------------------------
        //电压
        tempData = SampleADRaw.Inv2VoltRaw - SampleZeroCalibrate.Inv2VoltZero;
        SampleIns.Inv2VoltIns = ((int32_t)SampleCalibCoef.Inv2VoltK*tempData>>12)*SampleCalibCoef.Inv2VoltR;
        SampleIns.Inv2VoltSum += SampleIns.Inv2VoltIns*SampleIns.Inv2VoltIns;
        // 电流
        tempData = SampleADRaw.Inv2CurrRaw - SampleZeroCalibrate.Inv2CurrZero;
        SampleIns.Inv2CurrIns = ((int32_t)SampleCalibCoef.Inv2CurrK*tempData>>12)*0.1f;
        SampleIns.Inv2CurrSum += SampleIns.Inv2CurrIns*SampleIns.Inv2CurrIns;
        // acpv
        tempData = SampleADRaw.ACPVCurrRaw - SampleZeroCalibrate.ACPVCurrZero;
        SampleIns.ACPVCurrIns = ((int32_t)SampleCalibCoef.ACPVCurrK*tempData>>12)*0.1f;
        SampleIns.ACPVCurrSum += SampleIns.ACPVCurrIns*SampleIns.ACPVCurrIns;
        //半波判断L2
        FuncPara.INVISUM2 += SampleIns.Inv2CurrIns;
        FuncPara.INVVSUM2 += SampleIns.Inv2VoltIns;

        tempData = SampleADRaw.DCCurr2Raw - SampleZeroCalibrate.DCCurr2Zero;
        SampleIns.DCCurr2Ins = ((int32_t)SampleCalibCoef.DCCurr2K*tempData>>12);
        SampleIns.DCCurr2Sum += SampleIns.DCCurr2Ins;
        // ----------------------------电网电压--------------------------------
        // GRID1
        tempData = SampleADRaw.Grid1VoltRaw - SampleZeroCalibrate.Grid1VoltZero;
        SampleIns.Grid1VoltIns = ((int32_t)SampleCalibCoef.Grid1VoltK*tempData>>12)*0.1f;
        SampleIns.Grid1VoltSum += SampleIns.Grid1VoltIns*SampleIns.Grid1VoltIns;
        // GRID2
        tempData = SampleADRaw.Grid2VoltRaw - SampleZeroCalibrate.Grid2VoltZero;
        SampleIns.Grid2VoltIns = ((int32_t)SampleCalibCoef.Grid2VoltK*tempData>>12)*0.1f;
        SampleIns.Grid2VoltSum += SampleIns.Grid2VoltIns*SampleIns.Grid2VoltIns;
        if(ArmWriteData.Model.bit.CTDir==1)
        {
            tempData = SampleZeroCalibrate.Grid1CurrZero-SampleADRaw.Grid1CurrRaw ;
        }
        else
        {
            tempData = SampleADRaw.Grid1CurrRaw - SampleZeroCalibrate.Grid1CurrZero;
        }

        SampleIns.Grid1CurrIns = ((int32_t)SampleCalibCoef.Grid1CurrK*tempData>>12)*0.1f;
        SampleIns.Grid1CurrSum += SampleIns.Grid1CurrIns*SampleIns.Grid1CurrIns;
        // GRID2
        if(ArmWriteData.Model.bit.CTDir==1)
        {
            tempData = SampleZeroCalibrate.Grid2CurrZero-SampleADRaw.Grid2CurrRaw;
        }
        else
        {
            tempData = SampleADRaw.Grid2CurrRaw - SampleZeroCalibrate.Grid2CurrZero;
        }
        SampleIns.Grid2CurrIns = ((int32_t)SampleCalibCoef.Grid2CurrK*tempData>>12)*0.1f;
        SampleIns.Grid2CurrSum += SampleIns.Grid2CurrIns*SampleIns.Grid2CurrIns;

        tempData = SampleADRaw.Mid1VoltRaw - SampleZeroCalibrate.Mid1VoltZero;
        SampleIns.Mid1VoltIns = ((int32_t)SampleCalibCoef.Mid1VoltK*tempData >>12)*0.1f;
        SampleIns.Mid1VoltSum += SampleIns.Mid1VoltIns * SampleIns.Mid1VoltIns;
        tempData = SampleADRaw.Mid2VoltRaw - SampleZeroCalibrate.Mid2VoltZero;
        SampleIns.Mid2VoltIns = ((int32_t)SampleCalibCoef.Mid2VoltK*tempData >>12)*0.1f;
        SampleIns.Mid2VoltSum += SampleIns.Mid2VoltIns * SampleIns.Mid2VoltIns;
        // ----------------------------负载--------------------------------
        //
        tempData = SampleADRaw.Load1VoltRaw - SampleZeroCalibrate.Load1VoltZero;
        SampleIns.Load1VoltIns = ((int32_t)SampleCalibCoef.Load1VoltK*tempData >>12)*SampleCalibCoef.Inv1VoltR;
        SampleIns.Load1VoltSum += SampleIns.Load1VoltIns * SampleIns.Load1VoltIns;
        tempData = SampleADRaw.Load2VoltRaw - SampleZeroCalibrate.Load2VoltZero;
        SampleIns.Load2VoltIns = ((int32_t)SampleCalibCoef.Load2VoltK*tempData >>12)*SampleCalibCoef.Inv2VoltR;
        SampleIns.Load2VoltSum += SampleIns.Load2VoltIns * SampleIns.Load2VoltIns;

        tempData = SampleADRaw.Load1CurrRaw - SampleZeroCalibrate.Load1CurrZero;
        SampleIns.Load1CurrIns = ((int32_t)SampleCalibCoef.Load1CurrK*tempData>>12)*0.1f;
        SampleIns.Load1CurrSum += SampleIns.Load1CurrIns * SampleIns.Load1CurrIns;
        tempData = SampleADRaw.Load2CurrRaw - SampleZeroCalibrate.Load2CurrZero;
        SampleIns.Load2CurrIns = - ((int32_t)SampleCalibCoef.Load2CurrK*tempData>>12)*0.1f;
        SampleIns.Load2CurrSum += SampleIns.Load2CurrIns * SampleIns.Load2CurrIns;

        tempData = SampleADRaw.LnCurrRaw - SampleZeroCalibrate.LnCurrZero;
        SampleIns.LnCurrIns = - ((int32_t)SampleCalibCoef.LnCurrK*tempData>>12)*0.1f;
        SampleIns.LnCurrSum += SampleIns.LnCurrIns * SampleIns.LnCurrIns;
        SampleIns.LnDCCurrSum200 += SampleIns.LnCurrIns;
        /*  -----------------------------------------------------------*/
    }
}
/*************************************************
Function: CalculateDisplayData
Description: 对外显示数据滤波  10ms
Input: None
Output: None
Frequency:
*************************************************/
void CalculateDisplayData(void)
{
    if (SampleIns.CalSampleZeroFlag || SystemError.Fault3.bit.ADZeroErr)
    {
        // 直流信号 200ms 均值滤波后作为显示数据
        if (++SampleDisplayData.Count1 > 20)     // 2000-50Hz;2400-60Hz
        {
            SampleDisplayData.Count1 = 0;

            SampleDisplayData.PV1Volt = SampleDisplayData.PV1VoltSum * 0.05f;
            SampleDisplayData.PV2Volt = SampleDisplayData.PV2VoltSum * 0.05f;
            SampleDisplayData.PV3Volt = SampleDisplayData.PV3VoltSum * 0.05f;
            SampleDisplayData.PV4Volt = SampleDisplayData.PV4VoltSum * 0.05f;

            SampleDisplayData.PV1VoltSum = 0;
            SampleDisplayData.PV2VoltSum = 0;
            SampleDisplayData.PV3VoltSum = 0;
            SampleDisplayData.PV4VoltSum = 0;
        }
        else
        {
//            SampleDisplayData.PV1VoltSum += SampleIns.PV1VoltIns;
//            SampleDisplayData.PV2VoltSum += SampleIns.PV2VoltIns;
//            SampleDisplayData.PV3VoltSum += SampleIns.PV3VoltIns;
//            SampleDisplayData.PV4VoltSum += SampleIns.PV4VoltIns;
            SampleDisplayData.PV1VoltSum += SampleRms.PV1VoltFilter8;
            SampleDisplayData.PV2VoltSum += SampleRms.PV2VoltFilter8;
            SampleDisplayData.PV3VoltSum += SampleRms.PV3VoltFilter8;
            SampleDisplayData.PV4VoltSum += SampleRms.PV4VoltFilter8;
        }

        // 交流信号 16次均值滤波
        if (SampleIns.AcCalOverFlag == 1)          // 交流有效值计算完成标志
        {
            SampleIns.AcCalOverFlag = 0;

            if (++SampleDisplayData.Count2 > 16)
            {
                SampleDisplayData.Count2 = 0;

                SampleDisplayData.Inv1Volt = SampleDisplayData.Inv1VoltSum * 0.0625f;
                SampleDisplayData.Inv2Volt = SampleDisplayData.Inv2VoltSum * 0.0625f;
                SampleDisplayData.Inv1Curr = SampleDisplayData.Inv1CurrSum * 0.0625f;
                SampleDisplayData.Inv2Curr = SampleDisplayData.Inv2CurrSum * 0.0625f;
                SampleDisplayData.Load1Curr = SampleDisplayData.Load1CurrSum * 0.0625f;
                SampleDisplayData.Load2Curr = SampleDisplayData.Load2CurrSum * 0.0625f;
                SampleDisplayData.Load1Volt = SampleDisplayData.Load1VoltSum * 0.0625f;
                SampleDisplayData.Load2Volt = SampleDisplayData.Load2VoltSum * 0.0625f;
                SampleDisplayData.Grid1Volt = SampleDisplayData.Grid1VoltSum * 0.0625f;
                SampleDisplayData.Grid2Volt = SampleDisplayData.Grid2VoltSum * 0.0625f;

                SampleDisplayData.Inv1VoltSum = 0;
                SampleDisplayData.Inv2VoltSum = 0;
                SampleDisplayData.Inv1CurrSum = 0;
                SampleDisplayData.Inv2CurrSum = 0;
                SampleDisplayData.Load1CurrSum = 0;
                SampleDisplayData.Load2CurrSum = 0;
                SampleDisplayData.Load1VoltSum = 0;
                SampleDisplayData.Load2VoltSum = 0;
                SampleDisplayData.Grid1VoltSum = 0;
                SampleDisplayData.Grid2VoltSum = 0;
            }
            else
            {
                SampleDisplayData.Inv1VoltSum += SampleRms.Inv1VoltRms;
                SampleDisplayData.Inv2VoltSum += SampleRms.Inv2VoltRms;
                SampleDisplayData.Inv1CurrSum += SampleRms.Inv1CurrRms;
                SampleDisplayData.Inv2CurrSum += SampleRms.Inv2CurrRms;
                SampleDisplayData.Load1CurrSum += SampleRms.Load1CurrRms;
                SampleDisplayData.Load2CurrSum += SampleRms.Load2CurrRms;
                SampleDisplayData.Load1VoltSum += SampleRms.Load1VoltRms;
                SampleDisplayData.Load2VoltSum += SampleRms.Load2VoltRms;
                SampleDisplayData.Grid1VoltSum += SampleRms.Grid1VoltRms;
                SampleDisplayData.Grid2VoltSum += SampleRms.Grid2VoltRms;
            }
        }
    }
}
/******************************************************************************
// Export Function-Prototype: GeneralMoudlarSample
// Description  : 计算 AD 采样结果
// Input        : None
// Output       : None
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
#pragma CODE_SECTION(HandleSample, ".TI.ramfunc");
void HandleSample(uint16_t type)
{
    SampleIns.PeriodPoint = InvPhase.PeriodPoint;                   // 交流周期采样点数
    SampleIns.GridPeriodPoint = InvPhase.GridPeriodPoint;           // 交流周期采样点数
    // 分开运行减少单次运行消耗的时间
    if (type == 1)
    {
        // 读取AD寄存器原始值
        ReadADCRawValues();
        // 零飘处理
        CalibrateADZero();
        // 电流系数校准
        //ADCoeffModify();  主循环
        // 计算采样结果
        CalculateSampleValue();
    }
    else if (type == 2)
    {
        // 添加功率计算
        CalculatePower();
        // 计算显示数据
        //CalculateDisplayData(); //主循环
    }
}


//
float InvPowerCalibrate(float val)
{
    if(val > 0)
    {
        return (val*0.996f + 10);  //1.0126    //100 - 118 500 - 512  1000 - 1009  2000 - 2009  2500 - 2503
    }
    else
    {
        //V4
        return val;

        //V3
//        if(val < -2050)
//        {
//            return (val*0.995f);
////            return (val*1.06f + 149);
//        }
//        else
//        {
//            return (val*0.986f);
//        }
          //50HZ  1.0385   //100 - 87  1000- 983  1500-1479  2000- 1972 2100 - 2072 2200 - 2174  2500- 2493   2747 - 2763
          //50HZ                                              2000 - 1972                            2748-2733
          //60HZ
    }


}





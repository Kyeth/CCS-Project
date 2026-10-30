/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name:ModbusTable.c
** Latest modified Date:2021-08-27
** Latest Version: V1.0
** Description:
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2021-08-27
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/
#include "string.h"
#include "ModbusTable.h"
#include "Protect.h"
#include "Sample.h"
#include "ConfigSystemPara.h"
#include "PVStateMachine.h"
#include "MPPTFunc.h"
#include "PowerCalculator.h"
#include "User_I2C.h"
#include "CalculateAngle.h"
#include "Energy_Manage.h"
#include "OffGridInv2.h"
#include "ParaProtocol.h"
#include "Parallel.h"
#include "BoardTest.h"
#include "nPnZ_Cntl.h"
#include "Function.h"

ARM_WRITE_DATA ArmWriteData;
ARM_READ_DATA ArmReadData;

extern MPPT_DATA Mppt1,Mppt2,Mppt3;

extern uint16_t ArmCommuErrCnt;
extern Uint32 RunPeriod;

int IsInRange(float data, float down, float up)
{
    if (data > up || data < down)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
/******************************************************************************
// Local Function-Prototype: GeneralMoudlarModbus
// Description  :
// Input        :
// Output       :
// Return Value : None
// Calls        :
// Called By    :
// History      :
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void InitModbusTables(void)
{
    memset(&ArmWriteData, 0, sizeof(ARM_WRITE_DATA));
    memset(&ArmReadData, 0, sizeof(ARM_WRITE_DATA));

    ArmReadData.ARMEnable = 0xffff;     // 默认 ARM 使能位全部开启

    // 软件版本
    ArmReadData.VerionH = 500170106>>16;
    ArmReadData.VerionL = 500170106&0xffff;
    // 逆变允许最大输出功率,默认额定
    ArmReadData.MaxPowerSetR = INV_RATE_POWER;
    ArmReadData.MaxPowerSetS = INV_RATE_POWER;
}
/******************************************************************************
// Local Function-Prototype: GeneralMoudlarModbus
// Description  :
// Input        :
// Output       :
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/08/27
// Author       : tzz
// Modification : Created Function
******************************************************************************/
uint16_t ReadRegisterTables(uint16_t addr)
{
    uint16_t value = 0;

    switch(addr)
    {
        // 3000~3019 并网认证参数
        case 3000:
        {
            value = Protect.Value.GridUV1*10;
            break;
        }
        case 3001:
        {
            value = Protect.Delay.GridUV1Dly*10;
            break;
        }
        case 3002:
        {
            value = Protect.Value.GridUV2*10;
            break;
        }
        case 3003:
        {
            value = Protect.Delay.GridUV2Dly*10;
            break;
        }
        case 3004:
        {
            value = Protect.Value.GridOV1*10;
            break;
        }
        case 3005:
        {
            value = Protect.Delay.GridOV1Dly*10;
            break;
        }
        case 3006:
        {
            value = Protect.Value.GridOV2*10;
            break;
        }
        case 3007:
        {
            value = Protect.Delay.GridOV2Dly*10;
            break;
        }
        // 欠频
        case 3008:
        {
            value = Protect.Value.GridUF1*100;
            break;
        }
        case 3009:
        {
            value = Protect.Delay.GridUF1Dly*10;
            break;
        }
        case 3010:
        {
            value = Protect.Value.GridOF1*100;
            break;
        }
        case 3011:
        {
            value = Protect.Delay.GridOF1Dly*10;
            break;
        }
        case 3018:
        {
            value = Protect.ClrDly.GridReConnectDly;    // 异常重连时间
            break;
        }
        // 过频
        case 3020:
        {
            value = Protect.Value.GridUF2*100;
            break;
        }
        case 3021:
        {
            value = Protect.Delay.GridUF2Dly*10;
            break;
        }
        case 3022:
        {
            value = Protect.Value.GridOF2*100;
            break;
        }
        case 3023:
        {
            value = Protect.Delay.GridOF2Dly*10;
            break;
        }

        case 3026:
        {
            value = SystemPara.CountryID;              // 认证国家设置
            break;
        }
        case 4000:
        {
            value = EnergyManage.Enable.all;
            break;
        }
        case 4001:
        {
            value=(int16)(Cosphimode.Cosphi*100.0f);
            break;
        }
        case 4002:
        {
            value=(Uint16)(Cosphimode.Cosphi_exc);
            break;
        }
        case 4003:
        {
            value=(int16)(QUmode.Vref*10000.0f/120.0f);
            break;
        }
        case 4004:
        {
            value=QUmode.Vref_AdjEn;
            break;
        }
        case 4005:
        {
           value=(Uint16)(QUmode.Lpf_Tc*10.0f);//(30000*10ms/1000)*10=300
           break;
        }
        case 4006:
        {
            value=(int16)(QUmode.V1*10000.0f/120.0);
            break;
        }
        case 4007:
        {
            value=(int16)(QUmode.V2*10000.0f/120.0);
            break;
        }
        case 4008:
        {
            value=(int16)(QUmode.V3*10000.0f/120.0);
            break;
        }
        case 4009:
        {
           value=(int16)(QUmode.V4*10000.0f/120.0);
           break;
        }
        case 4010:
        {
           value=(int16)(QUmode.Q1*10000.0f/4500.0f);
           break;
        }
        case 4011:
        {
            value=(int16)(QUmode.Q2*10000.0f/4500.0f);
            break;
        }
        case 4012:
        {
            value=(int16)(QUmode.Q3*10000.0f/4500.0f);
            break;
        }
        case 4013:
        {
           value=(int16)(QUmode.Q4*10000.0f/4500.0f);
           break;
        }
        case 4014:
        {
           value=(Uint16)(QUmode.Tr*100.0f);
           break;
        }
        case 4015:
        {
            value=(int16)(QPmode.P1_P*10000/4500.0f);
            break;
        }
        case 4016:
        {
            value=(int16)(QPmode.P2_P*10000/4500.0f);
            break;
        }
        case 4017:
        {
            value=(int16)(QPmode.P3_P*10000/4500.0f);
            break;
        }
        case 4018:
        {
            value=(int16)(QPmode.P1_N*10000/4500.0f);
            break;
        }
        case 4019:
        {
            value=(int16)(QPmode.P2_N*10000/4500.0f);
            break;
        }
        case 4020:
        {
            value=(int16)(QPmode.P3_N*10000/4500.0f);
            break;
        }
        case 4021:
        {
            value=(int16)(QPmode.Q1_P*10000/4500.0f);
            break;
        }
        case 4022:
        {
            value=(int16)(QPmode.Q2_P*10000/4500.0f);
            break;
        }
        case 4023:
        {
            value=(int16)(QPmode.Q3_P*10000/4500.0f);
            break;
        }
        case 4024:
        {
            value=(int16)(QPmode.Q1_N*10000/4500.0f);
            break;
        }
        case 4025:
        {
            value=(int16)(QPmode.Q2_N*10000/4500.0f);
            break;
        }
        case 4026:
        {
            value=(int16)(QPmode.Q3_N*10000/4500.0f);
            break;
        }
        case 4027:
        {
           value=(int16)(CQmode.Q1*10000/4500.0f);
           break;
        }
        case 4028:
        {
            value=(int16)(PUmode.V1*10000.0f/120.0);
            break;
        }
        case 4029:
        {
            value=(int16)(PUmode.V2*10000.0f/120.0);
            break;
        }
        case 4030:
        {
            value=(int16)(PUmode.P1*10000/4500.0f);
            break;
        }
        case 4031:
        {
           value=(int16)(PUmode.P2*10000/4500.0f);
           break;
        }
        case 4032:
        {
           value=(Uint16)(PUmode.Tr*100.0f);
           break;
        }
        case 4053:
        {
            value=(int16)(PFmode.OFDeadband*1000.0);
            break;
        }
        case 4054:
        {
            value=(int16)(PFmode.UFDeadband*1000.0);
            break;
        }
        case 4055:
        {
            value=(int16)(PFmode.kOF_F*1000.0);
            break;
        }
        case 4056:
        {
            value=(int16)(PFmode.kUF_F*1000.0);
            break;
        }
        case 4057:
        {
            value=(Uint16)(PFmode.Tr*100.0f);
            break;
        }
        case 4058:
        {
            value=ESmode.ES_PERMIT_SERVICE_EN;
            break;
        }
        case 4059:
        {
            value=(int16)(ESmode.ES_OV*10000.0f/120.0);//过压ESmode.ES_OV= data/10.0f;//过压
            break;
        }
        case 4060:
        {
            value=(int16)(ESmode.ES_UV*10000.0f/120.0);//欠压
            break;
        }
        case 4061:
        {
            value=(int16)(ESmode.ES_OF*100.0f);//过频
            break;
        }

        case 4062:
        {
            value=(int16)(ESmode.ES_UF*100.0f);//欠频
            break;
        }
        case 4063:
        {
            value =(int16)(ESmode.ES_DELAY/10000.0);//ESmode.ES_DELAY= data*100l;   //最小单位1s
            break;
        }
        case 4064:
        {
            value = (int16)(ESmode.ES_RANDOMIZED_DELAY/100);
            break;
        }

        case 4065:
        {
             value = (int16)(ESmode.ES_RAMP_RATE/100);
             break;
        }
        case 4066:
        {
             value =(int16)(EnergyManage.PLimitCmd*10000/4500.0f);
             break;
        }
        case 5000:
        {
            value = ArmReadData.ARMEnable;
            break;
        }
        case 5001:
        {
            value = ArmReadData.RInvState;
            break;
        }
        case 5002:
        {
            value = ArmReadData.UrINV;
            break;
        }
        case 5003:
        {
            value = ArmReadData.IrINV;
            break;
        }
        case 5004:
        {
            value = ArmReadData.PrINV;
            break;
        }
        case 5005:
        {
            value = ArmReadData.IrLoad;
            break;
        }
        case 5006:
        {
            value = ArmReadData.PrLoad;
            break;
        }
        case 5007:
        {
            value = ArmReadData.UrGrid;
            break;
        }
        case 5008:
        {
            value = SystemPara.ActivePowerSetR;//ArmReadData.IrGrid;
            break;
        }
        case 5009:
        {
            value = EnergyManage.Temp1;//ArmReadData.PrGrid;
            break;
        }
        case 5010:
        {
            value = ArmReadData.SInvState;
            break;
        }
        case 5011:
        {
            value = ArmReadData.UsINV;
            break;
        }
        case 5012:
        {
            value = ArmReadData.IsINV;
            break;
        }
        // TODO--逆变器1数据
        case 5013:
        {
            value = ArmReadData.PsINV;
            break;
        }
        case 5014:
        {
            value = ArmReadData.IsLoad;
            break;
        }
        case 5015:
        {
            value = ArmReadData.PsLoad;
            break;
        }
        case 5016:
        {
            value = ArmReadData.UsGrid;
            break;
        }
        case 5017:
        {
            value = ArmReadData.IsGrid;
            break;
        }
        case 5018:
        {
            value = EnergyManage.Temp3;//ArmReadData.PsGrid;
            break;
        }
        //5019-5027预留

        case 5028:
        {
            value = ArmReadData.InvFreq;
            break;
        }
        case 5029:
        {
            value = ArmReadData.GridFreq;
            break;
        }
        // TODO 故障告警
        case 5030:
        {
            value = ArmReadData.PCSErr1;
            break;
        }
        case 5031:
        {
            value = ArmReadData.PCSErr2.all;
            break;
        }
        case 5032:
        {
            value = ArmReadData.PCSErr3;
            break;
        }
        case 5033:
        {
            value = ArmReadData.PCSErr4;
            break;
        }
        case 5034:
        {
            value = ArmReadData.PCSAlarm1; //&(0XFFFF^0x21C0)去掉PV低压;
            break;
        }
        case 5035:
        {
            value = ArmReadData.VerionH;
            break;
        }
        case 5036:
        {
            value = ArmReadData.VerionL;
            break;
        }
        case 5037:
        {
            value = ArmReadData.BusVolt;
            break;
        }
        case 5038:
        {
            value = ArmReadData.Bus1Volt;
            break;
        }
        // TODO 允许功率限值
        case 5039:
        {
            value = ArmReadData.MaxPowerSetR;
            break;
        }
        case 5040:
        {
            value = ArmReadData.MaxPowerSetS;
            break;
        }
        case 5041:
        {
            value = ArmReadData.Bus2Volt;
            break;
        }
        // TODO PV 数据
        case 5042:
        {
            value = ArmReadData.UPV1;
            break;
        }
        case 5043:
        {
            value = ArmReadData.IPV1;
            break;
        }
        case 5044:
        {
            value = ArmReadData.PPV1;
            break;
        }
        case 5045:
        {
            value = ArmReadData.UPV2;
            break;
        }
        case 5046:
        {
            value = ArmReadData.IPV2;
            break;
        }
        case 5047:
        {
            value = ArmReadData.PPV2;
            break;
        }
        case 5048:
        {
            value = ArmReadData.UPV3;
            break;
        }
        case 5049:      // PV3 功率
        {
            value = ArmReadData.IPV3;
            break;
        }
        case 5050:
        {
            value = ArmReadData.PPV3;
            break;
        }
        case 5051:        // PV4
        {
            value = ArmReadData.UPV4;
            break;
        }
        case 5052:
        {
            value = ArmReadData.IPV4;
            break;
        }
        case 5053:
        {
            value = ArmReadData.PPV4;
            break;
        }
        //

        case 5057:
        {
            value = ArmReadData.UrLoad;
            break;
        }
        case 5058:
        {
            value = ArmReadData.UsLoad;
            break;
        }

        case 5060:
        {
            //并网PWM使能
            ArmReadData.DSPWrite_Flag.all = 0;

            if(InvStateData.WorkState == OnGridState)
            {
                if((EPwm1Regs.AQCSFRC.bit.CSFA == 0) && (EPwm2Regs.AQCSFRC.bit.CSFA == 0))
                {
                    ArmReadData.DSPWrite_Flag.all = (1<<7);
                }
            }
            if(FuncPara.BatPowering)
            {
                ArmReadData.DSPWrite_Flag.all |= (1<<5);
            }

            value = ArmReadData.DSPWrite_Flag.all;
            break;
        }

        //后续待添加
        case 5064:
        {
            value = FuncPara.ISOMin;
            break;
        }

        case 5065:
        {
            value = GFCIData.GFCIRms; //
            break;
        }

        case 5066:
        {
            value = (int)ArmReadData.DCCurr1Rms; //
            break;
        }

        case 5067:
        {
            value = (int)ArmReadData.DCCurr2Rms; //
            break;
        }

        case 5069:
        {
            value = SampleRms.ACPVCurrRms*10; //
            break;
        }

        // TODO 逆变输出
        case 8000:      // R 相电压
        {
            value = ArmReadData.UrINV;
            break;
        }
        case 8001:      // S 相电压
        {
            value = ArmReadData.UsINV;
            break;
        }
        case 8003:      // R 相电流
        {
            value = ArmReadData.IrINV;
            break;
        }
        case 8004:
        {
            value = ArmReadData.IsINV;
            break;
        }
        case 8006:      // R 相电网
        {
            value = ArmReadData.UrGrid;
            break;
        }
        case 8007:      // S相电网
        {
            value = ArmReadData.UsGrid;
            break;
        }

        case 8009:      // R 相负载电流
        {
            value = ArmReadData.IrLoad;
            break;
        }
        case 8010:      // S 相负载电流
        {
            value = ArmReadData.IsLoad;
            break;
        }

        case 8012:      // PV1 电压
        {
            value = ArmReadData.UPV1;
            break;
        }
        case 8013:      // PV2 电压
        {
            value = ArmReadData.UPV2;
            break;
        }
        case 8014:      // PV3 电压
        {
            value = ArmReadData.UPV3;
            break;
        }
        case 8015:      // PV1 电流
        {
            value = ArmReadData.IPV1;
            break;
        }
        case 8016:      // PV2 电流
        {
            value = ArmReadData.IPV2;
            break;
        }
        case 8017:      // PV3 电流
        {
            value = ArmReadData.IPV3;
            break;
        }
        case 8021:      // 电池电压
        {
            value = SampleRms.BUSVoltFilter8;
            break;
        }
        case 8024:      // 电池电压
        {
            value = SampleRms.Bus1VoltFilter200;
            break;
        }
        case 8025:      // 电池电压
        {
            value = SampleRms.Bus2VoltFilter200;
            break;
        }

        // 采样 AD 值
        case 8510:      // PV1电压
        {
            value = SampleADRaw.PV1VoltRaw;
            break;
        }
        case 8511:      // PV2电压
        {
            value = SampleADRaw.PV2VoltRaw;
            break;
        }
        case 8512:      // PV3电压
        {
            value = SampleADRaw.PV3VoltRaw;
            break;
        }
        case 8513:      // PV1电压
        {
            value = SampleADRaw.PV1CurrRaw;
            break;
        }
        case 8514:      // PV2电压
        {
            value = SampleADRaw.PV2CurrRaw;
            break;
        }
        case 8515:      // PV3电流
        {
            value = SampleADRaw.PV3CurrRaw;
            break;
        }
        case 8516:      // R电网电压
        {
            value = SampleADRaw.Grid1VoltRaw;
            break;
        }
        case 8517:      // S电网电压
        {
            value = SampleADRaw.Grid2VoltRaw;
            break;
        }
        case 8518:      // R负载电流
        {
            value = SampleADRaw.AlterSelectRaw;
            break;
        }
        case 8519:      // S负载电流
        {
            value = 0;
            break;
        }
        case 8520:      // R负载电流
        {
            value = SampleADRaw.Inv1VoltRaw;
            break;
        }
        case 8521:      // S负载电流
        {
            value = SampleADRaw.Inv2VoltRaw;
            break;
        }
        case 8522:      // R逆变电流
        {
            value = SampleADRaw.Inv1CurrRaw;
            break;
        }
        case 8523:      // S逆变电流
        {
            value = SampleADRaw.Inv2CurrRaw;
            break;
        }
        case 8524:      // 电池电压
        {
            value = SampleADRaw.BUSVoltRaw;
            break;
        }
        case 8525:      // 电池电流
        {
            value = 0;
            break;
        }
        case 8526:      // BUS1
        {
            value = SampleADRaw.Bus1VoltRaw;
            break;
        }
        case 8527:      // BUS2
        {
            value = SampleADRaw.Bus2VoltRaw;
            break;
        }
        case 8528:      // GFCI
        {
    //        value = GFCIData.GFCIRms;
            break;
        }
        case 8529:      // PV ISO
        {
            value = 0;
            break;
        }

        default:
            break;
    }

    return value;
}
/******************************************************************************
// Local Function-Prototype: GeneralMoudlarModbus
// Description  : write Modbus registers
// Input        :
// Output       : None
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/08/27
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void WriteRegisterTables(int16_t data, uint16_t addr)
{
    float temp = 0;
    switch(addr)
    {
        case 0:
        {
            // R 相逆变电流直流分量,单位0.1A,限制该值在 [-15, 15]范围内
            ArmWriteData.DCIProtect1=ArmWriteData.DCComponent1 = data;
            break;
        }
        case 1:
        {
            // S 相逆变电流直流分量,单位0.1A,限制该值在 [-15, 15]范围内
            ArmWriteData.DCIProtect2=ArmWriteData.DCComponent2 = data;
            break;
        }

        //2预留

        case 3:
        {
            ArmWriteData.ActivePowerSetR = data;
            break;
        }
        case 4:
        {
            ArmWriteData.ActivePowerSetS = data;
            break;
        }

        //5预留

        case 6:
        {
            ArmWriteData.WFlag = 1;                 // 写标志置1
            ArmWriteData.InActivePowerSetR = data;
            break;
        }
        case 7:
        {
            ArmWriteData.InActivePowerSetS = data;
            break;
        }

        //8预留

        case 9:
        {
            ArmWriteData.BMSChgMaxVoltSet = data;
            break;
        }
        case 10:
        {
            ArmWriteData.BMSChgMaxPowerSet = data;
            break;
        }
        case 11:
        {
            ArmWriteData.BMSDischgMaxPowerSet = data;
            break;
        }

        case 12:
        {
            ArmWriteData.SystemPowerOn = data;
            break;
        }
        case 13:
        {
            ArmWriteData.Restore = data;
            break;
        }
        case 14:
        {
            ArmWriteData.FrequencySet = data;
            break;
        }

        // PV 最大目标功率
        case 15:
        {
            ArmWriteData.PVMaxPowerSet = data;
            break;
        }
        case 16:
        {
            ArmWriteData.GridChgMaxCurrSet = data;
            break;
        }
        case 17:
        {
            ArmWriteData.GridDischgMaxCurrSet = data;
            break;
        }
        case 18:
        {
            ArmWriteData.Fault.all = data;
            break;
        }
        case 19:
        {
            ArmWriteData.ARMEnable.all = data;
            break;
        }
        case 20:        // PV 并联模式
        {
            ArmWriteData.WFlag = 1;                 // 写标志置1
            ArmWriteData.PVParallel = data;
            break;
        }
        case 21:        // R相负载电压
        {
            ArmWriteData.LoadVolt1 = data;
            break;
        }
        case 22:        // S相负载电压
        {
            ArmWriteData.WFlag = 1;                 // 写标志置1
            ArmWriteData.LoadVolt2 = data;
            break;
        }
        case 24:        // 风扇档位控制
        {
            ArmWriteData.FanSpeed = data;
            break;
        }
        //25、26预留

        case 27:        // R相电网视在功率
        {
            ArmWriteData.Grid1SValue = data;
            break;
        }
        case 28:        // S相电网视在功率
        {
            ArmWriteData.Grid2SValue = data;
            break;
        }

        //29、30预留
        case 30:
        {
            ArmWriteData.PVTempLimitPower = data;
            break;
        }

        case 31:        // PV3目标功率
        {
//            ArmWriteData.PV3PowerAim = data;
            break;
        }
        case 32:        // 电网增强模式
        {
            ArmWriteData.GridEnhanceEn = data;
            break;
        }
        case 33:
        {
//            ArmWriteData.PV4PowerAim = data;
            break;
        }

        // 34预留

        case 35:        // 老化标签
        {
            ArmWriteData.AgingLable = data;
            break;
        }
        case 36:        // 逆变器并网使能
        {
            ArmWriteData.GridOnEn = data;
            break;
        }

        case 37:        // PV源类型
        {
            ArmWriteData.PVSourceType = data;
            break;
        }
        case 38:        // PV1目标功率
        {
//            ArmWriteData.PV1PowerAim = data;
            break;
        }
        case 39:        // PV2目标功率
        {
//            ArmWriteData.PV2PowerAim = data;
            ArmWriteData.WFlag = 1;                 // 写标志置1
            break;
        }

        //40预留

        case 41:        // 逆变器开环使能
        {
            ArmWriteData.OpenLoopEn = data;
            ArmWriteData.WFlag = 1;                 // 写标志置1
            break;
        }

        case 42:        // 紧急充电使能
        {
//            ArmWriteData.EmgChagEn = data;
//            ArmWriteData.EmgChagEn = 1;     //test
            break;
        }

        case 43:        // 电池包运行状态
        {
            ArmWriteData.BatState = data;
            break;
        }
        //44+为预留给电池包

        case 53:        // ARM模式使能
        {
            ArmWriteData.Model.all = data;
            break;
        }

        case 55:        // 电池包SOC
        {
            ArmWriteData.BMSSOC = data;
            break;
        }

        case 56:        // 急救使能
        {
//            ArmWriteData.EmgChagEn = data;
            break;
        }
        // TODO 标定系数
        case 1000:
        {
            ArmWriteData.ADCoeff.Uinv1K = data;
            // 逆变 1 电压
            temp = ArmWriteData.ADCoeff.Uinv1K;
            temp = __fsat(temp, 7500.0f, 5500.0f);  // 710*0.92~710*1.08
            SampleCalibCoef.Inv1VoltK =  temp;
            break;
        }
        case 1001:
        {
            ArmWriteData.ADCoeff.Uinv2K = data;
            // 逆变 2 电压
            temp = ArmWriteData.ADCoeff.Uinv2K;
            temp = __fsat(temp, 7500.0f, 5500.0f);
            SampleCalibCoef.Inv2VoltK =  temp;
            break;
        }

        //1002预留

        case 1003:
        {
            ArmWriteData.ADCoeff.Iinv1K2 = data;
            break;
        }
        case 1004:
        {
            ArmWriteData.ADCoeff.Iinv2K2 = data;
            break;
        }

        //1005预留

        case 1006:
        {
            ArmWriteData.ADCoeff.Ugrid1K = data;
            // 电网 1
            temp = ArmWriteData.ADCoeff.Ugrid1K;
            temp = __fsat(temp, 7000.0f, 6500.0f);   // 710*0.92~710*1.08
            SampleCalibCoef.Grid1VoltK = temp;
            break;
        }
        case 1007:
        {
            ArmWriteData.ADCoeff.Ugrid2K = data;
            // 电网 2
            temp = ArmWriteData.ADCoeff.Ugrid2K;
            temp = __fsat(temp, 7000.0f, 6500.0f);
            SampleCalibCoef.Grid2VoltK = temp;
            break;
        }

        //1008预留

        case 1009:
        {
            ArmWriteData.ADCoeff.ILoad1K = data;
            break;
        }
        case 1010:
        {
            ArmWriteData.ADCoeff.ILoad2K = data;
            break;
        }

        //1011预留

        case 1012:
        {
            ArmWriteData.ADCoeff.UPV1K = data;
            break;
        }
        case 1013:
        {
            ArmWriteData.ADCoeff.UPV2K = data;
            break;
        }
        case 1014:
        {
            ArmWriteData.ADCoeff.UPV3K = data;
            break;
        }
        case 1015:
        {
            ArmWriteData.ADCoeff.UPV4K = data;
            break;
        }
        case 1016:
        {
            ArmWriteData.ADCoeff.IPV1K = data;
            break;
        }
        case 1017:
        {
            ArmWriteData.ADCoeff.IPV2K = data;
            break;
        }
        case 1018:
        {
            ArmWriteData.ADCoeff.IPV3K = data;
            break;
        }
        case 1019:
        {
            ArmWriteData.ADCoeff.IPV4K = data;
            break;
        }

        //1021-1023预留给PV绝缘电阻采样系数

        case 1024:
        {
//            // BUS1
//            ArmWriteData.ADCoeff.BUS1K = data;
//            temp = ArmWriteData.ADCoeff.BUS1K;
//            temp = __fsat(temp, 3500.0f, 2500.0f);   // 366*0.92~366*1.08
//            SampleCalibCoef.Bus1VoltK = temp;
            break;
        }
        case 1025:
        {
//            ArmWriteData.ADCoeff.BUS2K = data;
//            // BUS2
//            temp = ArmWriteData.ADCoeff.BUS2K;
//            temp = __fsat(temp, 3500.0f, 2500.0f);
//            SampleCalibCoef.Bus2VoltK = temp;
            break;
        }
        case 1026:
        {
//            ArmWriteData.ADCoeff.BUSK = data;
//            // BUS 电压
//            temp = ArmWriteData.ADCoeff.BUSK;
//            temp = __fsat(temp, 6500.0f, 6000.0f);
//            SampleCalibCoef.BUSVoltK = temp;
            break;
        }

        //
        case 1033:
        {
            ArmWriteData.ADCoeff.Iinv1K1 = data;
            break;
        }
        case 1034:
        {
            ArmWriteData.ADCoeff.Iinv2K1 = data;
            break;
        }

        case 1036:
        {//整L1重
            ArmWriteData.ADCoeff.Iinv1K4 = data;
            break;
        }
        case 1037:
        {//整L2重
            ArmWriteData.ADCoeff.Iinv2K4 = data;
            break;
        }
        //
        case 1039:
        {//整L1轻
            ArmWriteData.ADCoeff.Iinv1K3 = data;
            break;
        }
        case 1040:
        {//整L2轻
            ArmWriteData.ADCoeff.Iinv2K3 = data;
            break;
        }
        //
        case 1057:
        {
            ArmWriteData.ADCoeff.ULoad1K = data;
            // 负载电压1
            temp = ArmWriteData.ADCoeff.ULoad1K;
            temp = __fsat(temp, 7000.0f, 6500.0f);
            SampleCalibCoef.Load1VoltK =  temp;
            break;
        }
        case 1058:
        {
            ArmWriteData.ADCoeff.ULoad2K = data;
            // 负载电压2
            temp = ArmWriteData.ADCoeff.ULoad2K;
            temp = __fsat(temp, 7000.0f, 6500.0f);
            SampleCalibCoef.Load2VoltK =  temp;
            break;
        }

        // TODO 认证相关
        // 3000~3019 并网认证参数

        //3000高级功能使能

        case 3001:
        {
            //ArmWriteData.CountryID = data;
            break;
        }
        case 3002:                              // UV2
        {
            //ArmWriteData.WAuthenFlag =1;
            ArmWriteData.UV1 = data ;

            if (IsInRange((float)ArmWriteData.UV1, SystemPara.InvRateVolt*5, SystemPara.InvRateVolt*10))       // 默认105.6
            {
                Protect.Value.GridUV1 = ArmWriteData.UV1 / 10.0f;          // [35, 1056]  0.1V
                Protect.Value.RecoveryGridUV1 = Protect.Value.GridUV1 + 2;
            }
            break;
        }
        case 3003:
        {
            ArmWriteData.UV1Dly = data;
            if (IsInRange((float)ArmWriteData.UV1Dly, -0.0f, 500.0f))    // 默认21*100
            {
                Protect.Delay.GridUV1Dly = ArmWriteData.UV1Dly;            // [200, 5000]  10ms
            }
            break;
        }
        case 3004:
        {
//            ArmWriteData.UV2 = data;            // OV1
//            if (IsInRange((float)ArmWriteData.UV2, 350.0f, 600.0f))        // 默认 60V
//            {
//                Protect.Value.GridUV2 = ArmWriteData.UV2 / 10.0f;          // [35, 600]  0.1V
//            }
            break;
        }
        case 3005:
        {
//            ArmWriteData.UV2Dly = data;
//            if (IsInRange((float)ArmWriteData.UV2Dly, 16.0f, 2100.0f))     // 默认2s
//            {
//                Protect.Delay.GridUV2Dly = ArmWriteData.UV2Dly;            // [16, 200]  10ms
//            }
            break;
        }


     //           case 3006://超前和滞后功率因数设置
     //           {
     //               SciData.PAheadLagSet   = WriteDataChar[InPCSi] ;
     //               break ;
     //           }
     //           case 3007://区域设置
     //           {
     //               SciData.Area_Set  = WriteDataChar[InPCSi]  ;
     //               break ;
     //           }
     //           case 3008://模式使能
     //           {
     //               SciData.Mode_Enable  = WriteDataChar[InPCSi] ;
     //               break ;
     //           }
     //           case 3009://每分钟功率变化比例
     //           {
     //               SciData.PChange_Permin = WriteDataChar[InPCSi] ;
     //               break ;
     //           }
     //           case 3010://均值过压保护值
     //           {
     //               SciData.OV_Avg   = WriteDataChar[InPCSi] ;
     //               break ;
     //           }

        case 3011:                              // UV2
        {
            ArmWriteData.OV1 = data ;
            if (IsInRange((float)ArmWriteData.OV1, SystemPara.InvRateVolt*10, SystemPara.InvRateVolt*12))
            {
                Protect.Value.GridOV1 = ArmWriteData.OV1 / 10.0f;          // [1320, 1440]  0.1V
                Protect.Value.RecoveryGridOV1 = Protect.Value.GridOV1 - 2;
            }
            break;
        }
        case 3012:
        {
            ArmWriteData.OV1Dly = data;
            if (IsInRange((float)ArmWriteData.OV1Dly, -0.0f, 500.0f))    // 默认13s
            {
                Protect.Delay.GridOV1Dly = ArmWriteData.OV1Dly;            // [100, 2300]  10ms
            }
            break;
        }
        case 3013:
        {
//            ArmWriteData.OV2 = data;            // OV1
//            if (IsInRange((float)ArmWriteData.OV2, 1430.0f, 1450.0f))
//            {
//                Protect.Value.GridOV2 = ArmWriteData.OV2 / 10.0f;           // [1320, 1440]  0.1V
//            }
            break;
        }
        case 3014:
        {
//            ArmWriteData.OV2Dly = data;
//            if (IsInRange((float)ArmWriteData.OV2Dly, 16.0f, 16.0f))        // 默认160ms
//            {
//                Protect.Delay.GridOV2Dly = ArmWriteData.OV2Dly;             // [100, 2300]  10ms
//            }
            break;
        }
     //           case 3015://无功占额定有功比例
     //           {
     //               SciData.QP_Percent   = WriteDataChar[InPCSi] ;
     //               break ;
     //           }
     //           case 3016://Volt–watt响应模式的参数Vw1设置
     //           {
     //               SciData.Vw1   = WriteDataChar[InPCSi] ;
     //               break ;
     //           }
     //           case 3017://Volt–watt响应模式的参数Vw2设置
     //           {
     //               SciData.Vw2  = WriteDataChar[InPCSi];
     //               break ;
     //           }
     //           case 3018://保留
     //           {
     //               SciData.reservation1 = WriteDataChar[InPCSi] ;
     //               break ;
     //           }

        case 3019:                              // UV2
        {
            ArmWriteData.UF1 = data ;

            if (IsInRange((float)ArmWriteData.UF1, SystemPara.InvRateFreq*80, SystemPara.InvRateFreq*100))
            {
                Protect.Value.GridUF1 = (float)ArmWriteData.UF1 * 0.01f;    // [5700, 5900]  0.01Hz
                Protect.Value.RecoveryGridUF1 = Protect.Value.GridUF1 + 0.5f;
            }
            break;
        }
        case 3020:
        {
            ArmWriteData.UF2Dly = data;
            if (IsInRange((float)ArmWriteData.UF2Dly, 16.0f, 200.0f))     // 默认160ms
            {
                Protect.Delay.GridUF2Dly = ArmWriteData.UF2Dly;             // [10, 5000]  10ms
                Protect.Delay.GridUF1Dly = Protect.Delay.GridUF2Dly;
            }
//            ArmWriteData.UF1Dly = data;
//            if (IsInRange((float)ArmWriteData.UF1Dly,18000.0f, 65000.0f))   // 默认30s
//            {
//                Protect.Delay.GridUF1Dly = ArmWriteData.UF1Dly;             // [1000, 5000]  10ms
//            }
            break;
        }
        case 3021:
        {
//            ArmWriteData.UF2 = data;            // OV1
//            if (IsInRange((float)ArmWriteData.UF2, 5000.0f, 5700.0f))
//            {
//                Protect.Value.GridUF2 = (float)ArmWriteData.UF2 * 0.01f;    // [5000, 5900]  0.01Hz
//            }
            break;
        }
        case 3022:
        {
//            ArmWriteData.UF2Dly = data;
//            if (IsInRange((float)ArmWriteData.UF2Dly, 16.0f, 65000.0f))     // 默认160ms
//            {
//                Protect.Delay.GridUF2Dly = ArmWriteData.UF2Dly;             // [10, 5000]  10ms
//            }
            break;
        }

        case 3027:                              // UV2
        {
            ArmWriteData.OF1 = data ;
            if (IsInRange((float)ArmWriteData.OF1, SystemPara.InvRateFreq*100, SystemPara.InvRateFreq*110))       // 默认65
            {
                Protect.Value.GridOF1 = (float)ArmWriteData.OF1 * 0.01f;    // [6100, 6600]  0.01Hz
                Protect.Value.RecoveryGridOF1 = Protect.Value.GridOF1 - 0.5f;
            }
            break;
        }
        case 3028:
        {
            ArmWriteData.OF2Dly = data;
            if (IsInRange((float)ArmWriteData.OF2Dly, 16.0f, 200.0f))
            {
                Protect.Delay.GridOF2Dly = ArmWriteData.OF2Dly;             // [16,6000]  10ms
                Protect.Delay.GridOF1Dly = Protect.Delay.GridOF2Dly;
            }
//            ArmWriteData.OF1Dly = data;
//            if (IsInRange((float)ArmWriteData.OF1Dly, 18000.0f, 65000.0f))
//            {
//                Protect.Delay.GridOF1Dly = ArmWriteData.OF1Dly;             // [3000, 6000]  10ms
//            }
            break;
        }
        case 3029:
        {
//            ArmWriteData.OF2 = data;            // OV1
//            if (IsInRange((float)ArmWriteData.OF2, 6180.0f, 6600.0f))
//            {
//                Protect.Value.GridOF2 = (float)ArmWriteData.OF2 * 0.01f;    // [6180, 6600]  0.01Hz
//            }
            break;
        }
        case 3030:
        {
//            ArmWriteData.OF2Dly = data;
//            if (IsInRange((float)ArmWriteData.OF2Dly, 16.0f, 65000.0f))
//            {
//                Protect.Delay.GridOF2Dly = ArmWriteData.OF2Dly;             // [16,6000]  10ms
//            }
            break;
        }

        case 3039:
        {
            ArmWriteData.ReConnectDly = data;
            if (IsInRange((float)ArmWriteData.ReConnectDly, 1000.0f, 30000.0f))     // 默认20s,单位10ms
            {
                Protect.ClrDly.GridReConnectDly = ArmWriteData.ReConnectDly;        // [20,300]  1s

                // 电网过/欠压 告警复位
                Protect.ClrDly.GridOVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
                Protect.ClrDly.GridUVRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
                // 电网过/欠频 告警复位
                Protect.ClrDly.GridOFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
                Protect.ClrDly.GridUFRstDly = Protect.ClrDly.GridReConnectDly;      // 2min->20s
            }

            break;
        }

        //待整理
         case 500:
        {
            EnergyManage.Enable.all=(Uint16)data;
            EnergyManage.Enable.bit.CosphiMode = 1;
            break;
        }
        case 501:
        {
            EnergyManage.Enable.bit.CosphiMode = 1;
            Cosphimode.Cosphi=__fsat(data/100.0,1.0,0.8);
            break;
        }
        case 502:
        {
            EnergyManage.Enable.bit.CosphiMode = 1;
            Cosphimode.Cosphi_exc=__fsat((float)data,1.0,0.0);
            break;
        }
        /*
        case 533://过压保护点1
        {
			  Protect.Value.GridOV1 =__fsat(data/100.0,130,100);
			  Protect.Value.GridOV1 = SystemPara.InvRateVolt*Protect.Value.GridOV1;   // 116V
			  break;
		}
        case 535://过压延迟保护时间1
		{
			Protect.Delay.GridOV1Dly = __fsat(data,1000,10)-4;//10ms
			break;
		}
        case 537://欠压保护点1
		{
			  Protect.Value.GridUV1 =__fsat(data/100.0,100,70);
			  Protect.Value.GridUV1 = SystemPara.InvRateVolt*Protect.Value.GridUV1;   // 116V
			  break;
		}
		case 539://欠压延迟保护时间1
		{
			 Protect.Delay.GridUV1Dly = __fsat(data,1000,10)-4;//10ms
			break;
		}
		case 541://过频保护点1
		{
			Protect.Value.GridOF1 =__fsat(data,7000,5000);
			Protect.Value.GridOF1 =Protect.Value.GridOF1/100;
			  break;
		}
		case 543://过频延迟保护时间1
		{
			Protect.Delay.GridOF1Dly = __fsat(data,1000,10)-4;//10ms
			break;
		}
		case 547://欠过频保护点1
		{
			Protect.Value.GridUF1 =__fsat(data,6000,4500);
			Protect.Value.GridUF1 =Protect.Value.GridUF1/100;
			  break;
		}
		case 549://欠频延迟保护时间1
		{
			Protect.Delay.GridUF1Dly = __fsat(data,1000,10)-4;//10ms
			break;
		}
		case 563:
		{
			ESmode.ES_DELAY=__fsat(data*10000.0,6000000.0,0.0);//ESmode.ES_DELAY= data*100l;   //最小单位1s
			break;
		}
		*/
        /*
         case 500:
        {
            EnergyManage.Enable.all=(Uint16)data;
            break;
        }
        case 501:
        {
            Cosphimode.Cosphi=__fsat(data/100.0,1.0,0.9);
            break;
        }
        case 502:
        {
            Cosphimode.Cosphi_exc=__fsat((float)data,1.0,0.0);
            break;
        }
        case 503:
        {
            QUmode.Vref=__fsat(data*0.012f,126.0,114.0); //rer 1.0=120.0
            break;
        }
        case 504:
        {
            QUmode.Vref_AdjEn=__fsat((float)data,1.0,0.0);
            break;
        }
        case 505:
        {
           QUmode.Lpf_Tc=__fsat((Uint16)data*0.1,5000.0,300.0);
           break;
        }
        case 506:
        {
            QUmode.V1=__fsat(data*0.012,123.6,92.4);
            break;
        }
        case 507:
        {
            QUmode.V2=__fsat(data*0.012,126.0,110.4);
            break;
        }
        case 508:
        {
            QUmode.V3=__fsat(data*0.012,129.6,114.0);
            break;
        }
        case 509:
        {
           QUmode.V4=__fsat(data*0.012,147.6,116.4);
           break;
        }
        case 510:
        {
           QUmode.Q1=__fsat(data*0.45f,1980.0,0.0);
           break;
        }
        case 511:
        {
            QUmode.Q2=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 512:
        {
            QUmode.Q3=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 513:
        {
           QUmode.Q4=__fsat((int)data*0.45f,0.0,-1980.0);
           break;
        }
        case 514:
        {
           QUmode.Tr=__fsat((data*0.01f),90.0,1.0);
           break;
        }
        case 515:
        {
            QPmode.P1_P=__fsat(data*0.45f,3150.0,0.0);
            break;
        }
        case 516:
        {
            QPmode.P2_P=__fsat(data*0.45f,3600.0,1800.0);
            break;
        }
        case 517:
        {
            QPmode.P3_P=__fsat(data*0.45f,4500.0,2250.0);
            break;
        }
        case 518:
        {
            QPmode.P1_N=__fsat((int)data*0.45f,0.0,-3150.0);
            break;
        }
        case 519:
        {
            QPmode.P2_N=__fsat((int)data*0.45f,-1800.0,-3600.0);
            break;
        }
        case 520:
        {
            QPmode.P3_N=__fsat((int)data*0.45f,-2250.0,-4500);
            break;
        }
        case 521:
        {
            QPmode.Q1_P=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 522:
        {
            QPmode.Q2_P=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 523:
        {
            QPmode.Q3_P=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 524:
        {
            QPmode.Q1_N=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 525:
        {
            QPmode.Q2_N=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 526:
        {
            QPmode.Q3_N=__fsat((int)data*0.45f,1980.0,-1980.0);
            break;
        }
        case 527:
        {
           CQmode.Q1=__fsat((int)data*0.45f,1980.0,-1980.0);
           CQmode.Q2=__fsat((int)data*0.45f,1980.0,-1980.0);
           break;
        }
        case 528:
        {
            PUmode.V1=__fsat(data*0.012,130.8,126.0);
            break;
        }
        case 529:
        {
            PUmode.V2=__fsat(data*0.012,132.0,127.2);
            break;
        }
        case 530:
        {
            PUmode.P1=__fsat((int)data*0.45f,4500.0,4500.0);
            break;
        }
        case 531:
        {
           PUmode.P2=__fsat((int)data*0.45f,0,-4500.0);
           break;
        }
        case 532:
        {
           PUmode.Tr=__fsat(data*0.01f,60.0,0.5);
           break;
        }
        case 553:
        {
            PFmode.OFDeadband=__fsat(data/1000.0,1.0,0.017);
            break;
        }
        case 554:
        {
            PFmode.UFDeadband=__fsat(data/1000.0,1.0,0.017);
            break;
        }
        case 555:
        {
            PFmode.kOF_F=__fsat(data/1000.0,0.05,0.02);
            break;
        }
        case 556:
        {
            PFmode.kUF_F=__fsat(data/1000.0,0.05,0.02);
            break;
        }
        case 557:
        {
            PFmode.Tr=__fsat(data/100.0,10.0,0.2);
            break;
        }
        case 558:
        {
            ESmode.ES_PERMIT_SERVICE_EN= __fsat(data,1.0,0.0);
            break;
        }
        case 559:
        {
            ESmode.ES_OV=__fsat(data*0.012,144.0,128.40);//过压ESmode.ES_OV= data/10.0f;//过压(1.07,1.2)
            break;
        }
        case 560:
        {
            ESmode.ES_UV=__fsat(data*0.012,114.0,105.6);//欠压(0.88,0.95)
            break;
        }
        case 561:
        {
            ESmode.ES_OF=__fsat(data*0.01,65.0,61.0);//过频
            break;
        }
        case 562:
        {
            ESmode.ES_UF=__fsat(data*0.01,59.0,55.0);//欠频
            break;
        }
        case 563:
        {
            ESmode.ES_DELAY=__fsat(data*10000.0,6000000.0,0.0);//ESmode.ES_DELAY= data*100l;   //最小单位1s
            break;
        }
        case 564:
        {
            ESmode.ES_RANDOMIZED_DELAY=__fsat(data*100.0,100000.0,0.0);
            break;
        }
        case 565:
        {
            ESmode.ES_RAMP_RATE=__fsat(data*100.0,100000.0,0.0);
            break;
        }
        case 566:
        {
            EnergyManage.PLimitCmd=__fsat((int)data*0.45f,4500.0,0.0);
            break;
        }
*/
        default:
        {
            break;
        }
    }
}
uint16_t ReadRegisterDebug(uint16_t addr)
{
    uint16_t value = 0;

    switch(addr)
    {
        case 1000:
        {
            value = SampleCalibCoef.Inv1VoltK;
            break;
        }
        case 1001:
        {
            value = SampleCalibCoef.Inv2VoltK;
            break;
        }

        case 1003:
        {
            value = SampleCalibCoef.Inv1CurrK;
            break;
        }
        case 1004:
        {
            value = SampleCalibCoef.Inv2CurrK;
            break;
        }

        case 1006:
        {
            value = SampleCalibCoef.Grid1VoltK;
            break;
        }
        case 1007:
        {
            value = SampleCalibCoef.Grid2VoltK;
            break;
        }
        case 1009:
        {
            value = SampleCalibCoef.Load1CurrK;
            break;
        }
        case 1010:
        {
            value = SampleCalibCoef.Load2CurrK;
            break;
        }
        case 1012:
        {
            value = SampleCalibCoef.PV1VoltK;
            break;
        }
        case 1013:
        {
            value = SampleCalibCoef.PV2VoltK;
            break;
        }
        case 1014:
        {
            value = SampleCalibCoef.PV3VoltK;
            break;
        }
        case 1015:
        {
            value = SampleCalibCoef.PV1CurrK;
            break;
        }
        case 1016:
        {
            value = SampleCalibCoef.PV2CurrK;
            break;
        }
        case 1017:
        {
            value = SampleCalibCoef.PV3CurrK;
            break;
        }
        case 1021:
        {
            value = SampleCalibCoef.BUSVoltK;
            break;
        }
        case 1024:
        {
            value = SampleCalibCoef.Bus1VoltK;
            break;
        }
        case 1025:
        {
            value = SampleCalibCoef.Bus2VoltK;
            break;
        }

       case 4000:
       {
           value = EnergyManage.Enable.all;
           break;
       }

        // TODO ---------------------------输出信息-----------------------------
        case 5001:
        {
            value = InvStateData.WorkState;
            break;
        }
        case 5002:
        {
            value = ArmReadData.UrINV;
            break;
        }
        case 5003:
        {
            value = ArmReadData.IrINV;
            break;
        }
        case 5004:
        {
            value = ArmReadData.PrINV;
            break;
        }
        case 5005:
        {
            value = ArmReadData.IrLoad;
            break;
        }
        case 5006:
        {
            value = ArmReadData.PrLoad;
            break;
        }
        case 5007:
        {
            value = ArmReadData.UrGrid;
            break;
        }
        case 5008:
        {
            value = ArmReadData.IrGrid;
            break;
        }
        case 5009:
        {
            value = ArmReadData.PrGrid;
            break;
        }
        case 5010:
        {
            value = InvStateData.WorkState;
            break;
        }
        case 5011:
        {
            value = ArmReadData.UsINV;
            break;
        }
        case 5012:
        {
            value = ArmReadData.IsINV;
            break;
        }
        // TODO--逆变器1数据
        case 5013:
        {
            value = ArmReadData.PsINV;
            break;
        }
        case 5014:
        {
            value = ArmReadData.IsLoad;
            break;
        }
        case 5015:
        {
            value = ArmReadData.PsLoad;
            break;
        }
        case 5016:
        {
            value = ArmReadData.UsGrid;
            break;
        }
        case 5017:
        {
            value = ArmReadData.IsGrid;
            break;
        }
        case 5018:
        {
            value = ArmReadData.PsGrid;
            break;
        }

        // TODO PV 数据
        case 5019:
        {
            value = PV1WorkState.State;
            break;
        }
        case 5020:
        {
            value = ArmReadData.UPV1;
            break;
        }
        case 5021:
        {
            value = ArmReadData.IPV1;
            break;
        }
        case 5022:
        {
            value = ArmReadData.PPV1;
            break;
        }
        case 5023:
        {
            value = PV2WorkState.State;
            break;
        }
        case 5024:
        {
            value = ArmReadData.UPV2;
            break;
        }
        case 5025:
        {
            value = ArmReadData.IPV2;
            break;
        }
        case 5026:
        {
            value = ArmReadData.PPV2;
            break;
        }
        case 5027:
        {
            value = PV3WorkState.State;
            break;
        }
        case 5028:
        {
            value = ArmReadData.UPV3;
            break;
        }
        case 5029:      // PV3 功率
        {
            value = ArmReadData.IPV3;
            break;
        }
        case 5030:
        {
            value = ArmReadData.PPV3;
            break;
        }
        case 5031:
        {
            value = SampleRms.BUSVoltFilter8*10.0f;
            break;
        }
        case 5032:
        {
            value = SampleRms.Bus1VoltFilter8*10.0f;    // VBUS
            break;
        }
        case 5033:
        {
            value = SampleRms.Bus2VoltFilter8*10.0f;
            break;
        }
        // --------------------故障信息----------------------------------------
        case 5034:
        {
            value = SystemError.Fault1.all;
            break;
        }
        case 5035:
        {
            value = SystemError.Fault2.all;
            break;
        }
        case 5036:
        {
            value = SystemError.Fault3.all;
            break;
        }
        case 5037:
        {
            value = SystemAlarm.all;

            break;
        }
        case 5038:
        {
            value = 0;
            break;
        }
        case 5039:
        {
            value = 0;
            break;
        }
        case 5040:
        {
            value = 0;
            break;
        }
        case 5041:
        {
            value = InvError.Inv1Err.all;
            break;
        }
        case 5042:
        {
            value = InvError.Inv2Err.all;
            break;
        }
        case 5043:
        {
            value = GridError.Grid1Err.all;
            break;
        }
        case 5044:
        {
            value = GridError.Grid2Err.all;
            break;
        }
        case 5045:
        {
            value = ArmWriteData.Fault.all;     // ARM故障信息
            break;
        }
        // 软件版本号
        case 5046:
        {
            value = ArmReadData.VerionH;
            break;
        }
        case 5047:
        {
            value = ArmReadData.VerionL;
            break;
        }

        // ------------------------------设置项-------------------------------
        // 有功功率设置
        case 5048:
        {
            value = SystemPara.SystemPowerOn;
            break;
        }
        case 5049:
        {
            value = (int16_t)SystemPara.ActivePowerSetR;
            break;
        }
        case 5050:
        {
            value = (int16_t)SystemPara.ActivePowerSetS;
            break;
        }
        // TODO 允许功率限值
        case 5051:
        {
//            value = SystemPara.BMSChgMaxCurrSet;
            break;
        }
        case 5052:
        {
//            value = SystemPara.BMSDischgMaxCurrSet;
            break;
        }
        case 5053:
        {
            value = PVSystemData.PVParallel;    // PV 并联方式选择
            break;
        }
        case 5054:
        {
            value = PVSystemData.PV1PowerAim;
            break;
        }
        case 5055:
        {
            value = PVSystemData.PV2PowerAim;
            break;
        }
        case 5056:
        {
            value = PVSystemData.PV3PowerAim;
            break;
        }
        case 5057:
        {
            value = PVSystemData.PVSourceType;
            break;
        }
        // EEPROM 格式化
        case 5058:
        {
            value = User_I2cFlag.bit.EeData_clr;
            break;
        }
        case 5059:
        {
            value = (int16_t)SystemPara.InActivePowerSetR;
            break;
        }
        case 5060:
        {
            value = (int16_t)SystemPara.InActivePowerSetS;
            break;
        }

        // 正端绝缘电阻
        case 5061:
        {
            value = 0;
            break;
        }
        case 5062:
        {
            value = 0;
            break;
        }
        // 负端绝缘电阻
        case 5063:
        {
            value = 0;
            break;
        }
        case 5064:
        {
            value = 0;
            break;
        }
        case 5065:
        {
            value = SystemPara.FanSpeed;
            break;
        }
        case 5066:
        {
            value = SystemError.Fault4.all;
            break;
        }
        case 5067:
        {
            value = ArmWriteData.Model.all;
            break;
        }

        case 6000:
        {
            value = ArmWriteData.ATETest;
            break;
        }
        case 6001:
        {
            value = ArmWriteData.AgingLable;
            break;
        }
        case 6002:
        {
            value = ArmWriteData.EnableGridChg;
            break;
        }
        case 6003:
        {
            value = ArmWriteData.GridVTotalP;
            break;
        }
        case 6004:
        {
            value = ArmWriteData.PVTotalP;
            break;
        }
        case 6005:
        {
            value = SystemPower.ApInv1.Average;
            break;
        }
        case 6006:
        {
            value = SystemPower.ApInv2.Average;
            break;
        }

        // 并机相关
        case 7000:
        {
            value = Flag_Local.Word&0xffff;
            break;
        }
        case 7001:
        {
            value = (Flag_Local.Word>>16)&0xffff;
            break;
        }
        case 7002:
        {
            value = ParaEvent.Word&0xffff;
            break;
        }
        case 7003:
        {
            value = (ParaEvent.Word>>16)&0xffff;
            break;
        }

        case 7004:
        {
            value = UrCtrl.FwStartOver;
            break;
        }
        case 7005:
        {
            value = UsCtrl.FwStartOver;
            break;
        }
        case 7006:
        {
            value = UrCtrl.RefTarget*10;
            break;
        }
        case 7007:
        {
            value = UsCtrl.RefTarget*10;
            break;
        }
        case 7008:
        {
            value = DebugData.UintData1;
            break;
        }
        case 7009:
        {
            value = DebugData.UintData2;
            break;
        }

        case 7014:
        {
            value = *(uint16_t *)(&VaRmsCntl.Ref);
            break;
        }
        case 7015:
        {
            value = *((uint16_t *)(&VaRmsCntl.Ref) + 1);
            break;
        }
        case 7016:
        {
            value = *(uint16_t *)(&VaRmsCntl.yp[0]);
            break;
        }
        case 7017:
        {
            value = *((uint16_t *)(&VaRmsCntl.yp[0]) + 1);
            break;
        }
        case 7018:
        {
            value = RunPeriod;
            break;
        }
        case 7019:
        {
            value = ShareData.TBCTR_delay;
            break;
        }

        case 7020:
        {
            value = DroopCntl.Vadj.a*1000;
            break;
        }
        case 7021:
        {
            value = DroopCntl.Vadj.b*1000;
            break;
        }
        case 7022:
        {
            value = DroopCntl.Ro*1000;
            break;
        }
        case 7023:
        {
            value = ShareData.theta_delay;
            break;
        }

        default:
            break;
    }

    return value;
}
 // 调试数据直接写入到控制变量当中
void WriteRegisterDebug(int16_t data, uint16_t addr)
{
    switch(addr)
    {
       // 有功功率设置
        case 5048:
        {
            SystemPara.SystemPowerOn = data;
            break;
        }
        case 5049:
        {
            SystemPara.ActivePowerSetR = data;
            break;
        }
        case 5050:
        {
            SystemPara.ActivePowerSetS = data;
            break;
        }
       // TODO 允许功率限值
        case 5051:
        {
//            SystemPara.BMSChgMaxCurrSet =  data;
            break;
        }
        case 5052:
        {
//            SystemPara.BMSDischgMaxCurrSet = data;
            break;
        }
        case 5053:
        {
            PVSystemData.PVParallel = data;    // PV 并联方式选择
            break;
        }
        case 5054:
        {
            PVSystemData.PV1PowerAim = data;
            break;
        }
        case 5055:
        {
            PVSystemData.PV2PowerAim = data;
            break;
        }
        case 5056:
        {
            PVSystemData.PV3PowerAim = data;
            break;
        }
        case 5057:
        {
            PVSystemData.PVSourceType = data;
            break;
        }
        // EEPROM 格式化
        case 5058:
        {
            User_I2cFlag.bit.EeData_clr = data;
            break;
        }
        case 5059:
        {
            SystemPara.InActivePowerSetR = data;
            break;
        }
        case 5060:
        {
            SystemPara.InActivePowerSetS = data;
            break;
        }
        case 5065:
        {
            SystemPara.FanSpeed = data;
            break;
        }

        case 6060:
        {
            ESmode.ES_PERMIT_SERVICE_EN=data;
            break;
        }
        case 7008:
        {
            DebugData.UintData1 = data;
            break;
        }
        case 7009:
        {
            DebugData.UintData2 = data;
            break;
        }

        case 7019:
        {
            ShareData.TBCTR_delay = data;
            break;
        }
        case 7022:
        {
            DroopCntl.Ro = data*0.001f;
            break;
        }
        case 7023:
        {
            ShareData.theta_delay = data;
            break;
        }

        default:
            break;
    }
}



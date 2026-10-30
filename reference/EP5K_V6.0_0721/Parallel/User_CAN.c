//###########################################################################
#include "F28x_Project.h"
#include "Sample.h"
#include "PVStateMachine.h"
#include "Function.h"
#include "Protect.h"
#include "PowerCalculator.h"
#include "OnGridInv2.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "OnGridInv2.h"
#include "ModbusTable.h"
#include "Update.h"
#include "ConfigSystemPara.h"
#include "Energy_Manage.h"
#include "OffGridInv2.h"
#include "BUSStateMachine.h"
#include "bsp.h"

//***************************************************************************
void U16tobp32(Uint16 MDH_H,Uint16 MDH_L,Uint16 MDL_H,Uint16 MDL_L)
{
    CanbRegs.CAN_IF2DATA.bit.Data_0 = MDL_H >> 8;
    CanbRegs.CAN_IF2DATA.bit.Data_1 = MDL_H & 0xFF;
    CanbRegs.CAN_IF2DATA.bit.Data_2 = MDL_L >> 8;
    CanbRegs.CAN_IF2DATA.bit.Data_3 = MDL_L & 0xFF;
    CanbRegs.CAN_IF2DATB.bit.Data_4 = MDH_H >> 8;
    CanbRegs.CAN_IF2DATB.bit.Data_5 = MDH_H & 0xFF;
    CanbRegs.CAN_IF2DATB.bit.Data_6 = MDH_L >> 8;
    CanbRegs.CAN_IF2DATB.bit.Data_7 = MDH_L & 0xFF;
}

//*****************************************************************************
//#pragma CODE_SECTION(User_Can, ".TI.ramfunc");

Uint32 MDL_Rx;
float MDH_Rx;
extern PIDCntl_F               PreSyncCntl;
void User_Can(void)
{
    Uint16 ID_12,ID_16,MDL_L,MDL_H,MDH_L,MDH_H;
    static Uint16 CAN_CH_MUX=0;
    Uint16 CanTX_flag;
    Uint32 tmp;

    if(CanbRegs.CAN_INT.bit.INT0ID == 1)   //接收到消息
    {
        CanbRegs.CAN_GLB_INT_CLR.all = 1;
        CanbRegs.CAN_IF1CMD.all = 0x1B0001;
        if(CanbRegs.CAN_IF1MCTL.bit.NewDat == 1)
        {
            MDL_Rx=(((Uint32)CanbRegs.CAN_IF1DATA.bit.Data_0<<24)    //序号
            		+((Uint32)CanbRegs.CAN_IF1DATA.bit.Data_1<<16)
					+((Uint32)CanbRegs.CAN_IF1DATA.bit.Data_2<<8)
					+(CanbRegs.CAN_IF1DATA.bit.Data_3));

            tmp=	(((Uint32)CanbRegs.CAN_IF1DATB.bit.Data_4 <<24)
            		+((Uint32)CanbRegs.CAN_IF1DATB.bit.Data_5 <<16)
					+((Uint32)CanbRegs.CAN_IF1DATB.bit.Data_6 <<8)
					+(CanbRegs.CAN_IF1DATB.bit.Data_7));

            MDH_Rx= *(float *)(&tmp);    //参数

//-------------------------------------------------------
        	switch(MDL_Rx)
        	{
    			case 0:	PVSystemData.PVParallel = 15;break;
    			case 1:	PVSystemData.PVParallel = 7;break;

    			case 2:	PVSystemData.PVParallel = 3;break;
    			case 3:	break;

    			case 4:	break;
    			case 5: break;

    			case 6:	break;
    			case 7:	break;

                case 8: break;
                case 9: break;

    			default: break;
        	}
        }
    }

//============================================================================
//    if(Task_Ptr==Standby_Mode) return;      //待机时不进行CAN通讯

    CanTX_flag=CanbRegs.CAN_ES.bit.TxOk;    //先读取以清除该位
    if(CanTX_flag==1)  //发送完成
    {
        CAN_CH_MUX=!CAN_CH_MUX;
        if(CAN_CH_MUX)  //1~6通道
        {
//            ID_12=(int)(EPwm8Regs.CMPA.bit.CMPA);
//            ID_16=(int)(SampleIns.LnCurrIns*10);  //(UrCtrl.IL*100)              ((FuncPara.INVIAVG1 + FuncPara.INVIAVG2)*10);// ((UrCtrl.Fw + UrCtrl.KBus)*10);
//            MDH_H=(int)(PreSyncCntl.Ref);//(UrCtrl.PRVars.Out*10);
//            MDH_L=(int)(PreSyncCntl.Fbk);//(EPwm1Regs.CMPA.bit.CMPA);
//            MDL_H=(int)(PreSyncCntl.xp[0]);  //(UrCtrl.Out* UrCtrl.BusVolt*10); //SampleIns.LnCurrIns    //(UrCtrl.PRVars.Out + UrCtrl.R3Vars.Out + UrCtrl.R5Vars.Out + UrCtrl.R7Vars.Out);  // UrCtrl.Fw *10);           //(SampleIns.LnCurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(PreSyncCntl.yp[0]*10000);

            ID_12=(int)(SampleIns.BUSVoltIns);
            ID_16=(int)(SampleIns.PV1CurrIns* 10);
            MDH_H=(int)(SampleIns.PV1VoltIns);
            MDH_L=(int)(PV1WorkState.CurrPIF.Ref* 10);
            MDL_H=(int)(PV1WorkState.M * PV_PWM_PERIOD);
            MDL_L=(int)(PV1WorkState.Duty);

//            ID_12=(int)(InvPhase.ThetaR*100);    //
//            ID_16=(int)((IrCtrl.PRVars.Out+ IrCtrl.PR3Vars.Out + IrCtrl.PR7Vars.Out)*IrCtrl.Period);
//            MDH_H=(int)(IrCtrl.PICtrl.Out*IrCtrl.Period);//(SampleIns.Inv2CurrIns *10);
//            MDH_L=(int)(IrCtrl.RepCtrl.CfOut*IrCtrl.Period);//(EPwm1Regs.CMPA.bit.CMPA);UsCtrl.RepCtrl.CfOut*2000
//            MDL_H=(int)(IrCtrl.Usogi.osg_u[0]);// IrCtrl.Fw*IrCtrl.Period (SampleIns.Load2CurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(SampleIns.Inv1VoltIns); //

//            ID_12=(int)(InvStateData.WorkState);    //
//            ID_16=(int)(SystemError.ArmFault);    //(SampleIns.LnCurrIns *10);                // (UsCtrl.RepCtrl.CfOut*200); // //(SampleIns.Grid1VoltIns * 10);
//            MDH_H=(int)(SystemError.StateErr.bit.INVFault);//(SampleIns.Inv2CurrIns *10);                         //UsCtrl.BusVolt;              //(UsCtrl.PWMDuty);            //(CaptureData.FreqR*10);//(SampleIns.Grid2VoltIns * 10);
//            MDH_L=(int)(UrCtrl.KBus*10);//(EPwm1Regs.CMPA.bit.CMPA);UsCtrl.RepCtrl.CfOut*2000
//            MDL_H=(int)(UrCtrl.Period);//(SampleIns.Load2CurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(UrCtrl.PRVars.Out*10); //

//            ID_12=(int)(EPwm8Regs.CMPA.bit.CMPA);
//            ID_16=(int)(SampleIns.LnCurrIns*10);  //(UrCtrl.IL*100)              ((FuncPara.INVIAVG1 + FuncPara.INVIAVG2)*10);// ((UrCtrl.Fw + UrCtrl.KBus)*10);
//            MDH_H=(int)(SampleIns.Inv1CurrIns*10);//(UrCtrl.PRVars.Out*10);
//            MDH_L=(int)(SampleIns.Inv2CurrIns*10);//(EPwm1Regs.CMPA.bit.CMPA);
//            MDL_H=(int)(SampleIns.Inv1VoltIns*10);  //(UrCtrl.Out* UrCtrl.BusVolt*10); //SampleIns.LnCurrIns    //(UrCtrl.PRVars.Out + UrCtrl.R3Vars.Out + UrCtrl.R5Vars.Out + UrCtrl.R7Vars.Out);  // UrCtrl.Fw *10);           //(SampleIns.LnCurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(SampleIns.Inv2VoltIns*10);

//            ID_12=(int)(SampleIns.Bus2VoltIns);
//            ID_16=(int)(SampleIns.Inv1CurrIns*10);//(SampleIns.Grid1VoltIns * 10);UrCtrl.RepCtrl.CfOut*200
//            MDH_H=(int)(SampleIns.Inv2CurrIns*10);//(SampleIns.Load1CurrIns*10); //SampleIns.Inv1CurrIns );//(UrCtrl.PWMDuty);            //(CaptureData.FreqR*10);//(SampleIns.Grid2VoltIns * 10);
//            MDH_L=(int)(SampleIns.Inv2VoltIns);//(EPwm1Regs.CMPA.bit.CMPA);
//            MDL_H=(int)(SampleIns.Inv1VoltIns);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(UrCtrl.RepCtrl.CfOut*200); //(SampleIns.Inv1CurrIns*10);//IrCtrl.RmpCtrl.TargetValue*10); //(SampleIns.Inv1CurrIns );
            U16tobp32(MDH_H,MDH_L,MDL_H,MDL_L);
            CanbRegs.CAN_IF2ARB.all=((Uint32)((ID_12 & 0x0FFF)|0xE000)<<16)+ID_16;
            CanbRegs.CAN_IF2CMD.all = 0xB70011;         //Tx enable
        }
//-----------------------------------------------------------------------------
        else    //7~12通道
        {

//            ID_12=(int)(SampleIns.Bus1VoltIns);
//            ID_16=(int)(UrCtrl.LowPoWVars.Out*10);//(SampleIns.LnCurrIns *10);                // (UsCtrl.RepCtrl.CfOut*200); // //(SampleIns.Grid1VoltIns * 10);
//            MDH_H=(int)(UrCtrl.PRVars.Out*10);//(SampleIns.Inv2CurrIns *10);                         //UsCtrl.BusVolt;              //(UsCtrl.PWMDuty);            //(CaptureData.FreqR*10);//(SampleIns.Grid2VoltIns * 10);
//            MDH_L=(int)(SampleIns.Load1CurrIns*10);//(EPwm1Regs.CMPA.bit.CMPA);UsCtrl.RepCtrl.CfOut*2000
//            MDL_H=(int)((UrCtrl.Fw)*10);//(SampleIns.Load2CurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(UrCtrl.RepCtrl.CfOut*200); //(SampleIns.Inv1CurrIns*10);

//            ID_12=(int)(SampleIns.Bus1VoltIns);    //
//            ID_16=(int)(SampleIns.Inv1VoltIns);    //(SampleIns.LnCurrIns *10);                // (UsCtrl.RepCtrl.CfOut*200); // //(SampleIns.Grid1VoltIns * 10);
//            MDH_H=(int)(UrCtrl.Out1*10);//(SampleIns.Inv2CurrIns *10);                         //UsCtrl.BusVolt;              //(UsCtrl.PWMDuty);            //(CaptureData.FreqR*10);//(SampleIns.Grid2VoltIns * 10);
//            MDH_L=(int)(UrCtrl.Out2*10);//EPwm1Regs.AQCSFRC.all(EPwm1Regs.CMPA.bit.CMPA);UsCtrl.RepCtrl.CfOut*2000
//            MDL_H=(int)(UrCtrl.PWMDuty);//(SampleIns.Load2CurrIns*10);//(EPwm2Regs.CMPA.bit.CMPA );
//            MDL_L=(int)(UrCtrl.CAMP); //

            ID_12=(int)(*PV1WorkState.Mppt.PVVoltAvg);
            ID_16=(int)(PV1WorkState.Mppt.Output);
            MDH_H=(int)(SampleIns.PV1CurrIns* 10);
            MDH_L=(int)(SampleIns.PV2CurrIns* 10);
            MDL_H=(int)(SampleIns.PV3CurrIns* 10);
            MDL_L=(int)(SampleIns.PV4CurrIns* 10);

//            ID_12=(int)(0);
//            ID_16=(int)(SampleIns.BUSVoltIns);
//            MDH_H=(int)(SystemError.Fault1.bit.BusHigh);
//            MDH_L=(int)(ProtectGroup.BUSH1.ProtectCnt);
//            MDL_H=(int)(ProtectGroup.BUS1H.ProtectCnt);
//            MDL_L=(int)(ProtectGroup.BUS2H.ProtectCnt);
            U16tobp32(MDH_H,MDH_L,MDL_H,MDL_L);
            CanbRegs.CAN_IF2ARB.all=((Uint32)((ID_12 & 0x0FFF)|0xF000)<<16)+ID_16;
            CanbRegs.CAN_IF2CMD.all = 0xB70011;
        }
    }
}

//===========================================================================
// No more.
//===========================================================================

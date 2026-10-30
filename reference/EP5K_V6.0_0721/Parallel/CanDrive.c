/*
 * McanDrive.c
 *
 *  Created on: 2024年3月16日
 *      Author: SJJ
 *      基于CAN的驱动层
 */
#include "Parallel.h"

/**********************************************************************
* PURPOSE :  查询是否收到信息,读取
* CALLED BY: 1ms
**********************************************************************/
char sCheckCanRx(User_CAN_DATA_UNION *CanMsg)
{
    if (CanaRegs.CAN_IF3OBS.bit.IF3Upd)
    {
        CanMsg->ID = CanaRegs.CAN_IF3ARB.all;       //read to clear IF3Upd
        CanMsg->DATA_32 = CanaRegs.CAN_IF3DATA.all;
        CanMsg->DATB_32 = CanaRegs.CAN_IF3DATB.all;

        if ((CanMsg->DATA_32 == 0x33221100) && (CanMsg->DATB_32 == 0x77665544))
        {
            while(CanaRegs.CAN_IF2CMD.bit.Busy);
            CanaRegs.CAN_IF2DATA.all = 0xBBAA9988;
            CanaRegs.CAN_IF2DATB.all = 0xFFEEDDCC;
            CanaRegs.CAN_IF2ARB.all = ((Uint32)(0xE000)<<16) + 0x189bff51;
            CanaRegs.CAN_IF2CMD.all = (0xA70000 + CmdWithId);
        }
        CanaRegs.CAN_IF3OBS.all = 0x1f;
        return 1;
    }
    return 0;
}

// 2 float
void sSendFloatFrame(Uint16 Command, float dataA, float dataB)
{
    User_CAN_DATA_UNION CanData;

    CanData.DATA_F = dataA;
    CanData.DATB_F = dataB;

    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2DATA.all = CanData.DATA_32;
    CanaRegs.CAN_IF2DATB.all = CanData.DATB_32;
//    CanaRegs.CAN_IF2ARB.all = ((Uint32) (0xE000) << 16) + (ShareData.CommAddr << 8U) + Command; //ID
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)Command + (ShareData.CommAddr<<8U)) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xA70000 + CmdWithId);
}
// 2 int32
void sSendUint32Frame(Uint16 Command, Uint32 dataA, Uint32 dataB)
{
    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2DATA.all = dataA;
    CanaRegs.CAN_IF2DATB.all = dataB;
//    CanaRegs.CAN_IF2ARB.all = ((Uint32) (0xE000) << 16) + (ShareData.CommAddr << 8U) + Command; //ID  1110
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)Command + (ShareData.CommAddr<<8U)) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xA70000 + CmdWithId);
}
// 4 int16
void sSendInt16Frame(Uint16 Command, Uint16 data0, Uint16 data1, Uint16 data2, Uint16 data3)
{
    User_CAN_DATA_UNION CanData;

    CanData.Data_0 = data0;
    CanData.Data_1 = data1;
    CanData.Data_2 = data2;
    CanData.Data_3 = data3;

    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2DATA.all = CanData.DATA_32;
    CanaRegs.CAN_IF2DATB.all = CanData.DATB_32;
//    CanaRegs.CAN_IF2ARB.all = ((Uint32) (0xE000) << 16) + (ShareData.CommAddr << 8U) + Command; //ID
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)Command + (ShareData.CommAddr<<8U)) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xA70000 + CmdWithId);
}

//发送波形
void sSendGraph(Uint16 data1, Uint16 data2, Uint16 data3, Uint16 data4, Uint16 data5, Uint16 data6)
{
////    if(McanaRegs.MCAN_TXBRP.all)
////        return;
//    mcanTxElement[CmdSendGragh]->id  = ((uint32_t)data2 |((uint32_t)data1 << 16U));
//    mcanTxElement[CmdSendGragh]->data= ((uint32_t)data3 |((uint32_t)data4 << 16U));
//    mcanTxElement[CmdSendGragh]->datb= ((uint32_t)data5 |((uint32_t)data6 << 16U));
//    McanaRegs.MCAN_TXBAR.all = (1U << CmdSendGragh);   //MCAN_TXBAR
}

/**********************************************************************
* PURPOSE :  发送载波同步帧  , 从机接收中断
* CALLED BY: master    4ms
**********************************************************************/
void sSendPwmTBCTR(void)
{
    DINT;
    if (EPwm1Regs.TBSTS.bit.CTRDIR)
    {
        CanaRegs.CAN_IF2DATA.all = EPwm1Regs.TBCTR;
    }
    else
    {
        CanaRegs.CAN_IF2DATA.all = -EPwm1Regs.TBCTR;
    }
    CanaRegs.CAN_IF2CMD.all = (0x860000 + CmdPwmSync); // 1000  0110  2  == 写入 / 发送 + 数据A  / 2号邮箱
    EINT;
}
/**********************************************************************
* PURPOSE :  发送相位同步帧  , 从机接收中断
* CALLED BY: master    4ms
**********************************************************************/
void sSendPhaseData(void)
{
    int16 Data;
    DINT;
    ShareData.theta_End = CpuTimer0.RegsAddr->TIM.all;
#ifdef Vision_IntType
    // if CpuTimer0.RegsAddr->TIM.all = (20000 ~ 19800 )
    if(ShareData.theta_End > ShareData.theta_Start)Data = (*ShareData.theta_Pt + 1) * 20.48;
    else Data = (*ShareData.theta_Pt + (ShareData.theta_Start - ShareData.theta_End)*0.00005) * 20.48;
#else
    Data = *ShareData.theta_Pt * 651.9;
#endif
    CanaRegs.CAN_IF2DATA.all = Data;
    CanaRegs.CAN_IF2CMD.all = (0x860000 + CmdPhaseSync); // 1000  0110  3  == 写入 / 发送 + 数据A  / 3号邮箱
    EINT;
}
///**********************************************************************
//* PURPOSE :  发送继电器动作
//* CALLED BY: master
//**********************************************************************/
//void sSendCmdRlySync( Uint16 rlyAction)
//{
//    CanaRegs.CAN_IF2DATA.all = rlyAction;
//    CanaRegs.CAN_IF2CMD.all = (0x860000 + CmdSendWorkState); // 1000  0110  4
//}
/**********************************************************************
* PURPOSE :  发送工作模式
* CALLED BY:
**********************************************************************/
void sCmdSendWorkState( Uint16 WorkState)
{
    CanaRegs.CAN_IF2DATA.all = WorkState;
    CanaRegs.CAN_IF2CMD.all = (0x860000 + CmdSendWorkState); // 1000  0110  5
}


static uint32_t canisrcnt;
#pragma CODE_SECTION(Para_CanIsr, ".TI.ramfunc");
interrupt void Para_CanIsr(void)
{
    canisrcnt++;
    //1.通信正常,解析CAN包
    CanaRegs.CAN_IF1CMD.all = 0x3B0008; //传出ID + NewDat / Clear Int Pending Bit+数据A+数据B / 邮箱8
    if (CanaRegs.CAN_IF1MCTL.bit.NewDat == 1) //new data into the data portion of this message object
    {
        if ((CanaRegs.CAN_IF1ARB.bit.ID & 0x3fffff) == (((uint32_t)CmdPwmSync) << 18U))
        {
            if(EPwm1Regs.TBSTS.bit.CTRDIR) ShareData.TBCTR_local = EPwm1Regs.TBCTR;
            else ShareData.TBCTR_local = -EPwm1Regs.TBCTR;
            MasterData.TBCTR_master = CanaRegs.CAN_IF1DATA.all & 0xffff;
            ParaCycles.Bits.CAN_Sync = 1;
            ParaCycles.Bits.CAN_MasterInNet = 1;
        }
        else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0x3fffff) == (((uint32_t)CmdPhaseSync) << 18U))
        {
            #ifdef Vision_IntType    //(0 - 200)
            ShareData.theta_End = CpuTimer0.RegsAddr->TIM.all;
            if(ShareData.theta_End < ShareData.theta_Start)
                ShareData.theta_local = (*ShareData.theta_Pt + (ShareData.theta_Start - ShareData.theta_End )*0.00005) * 20.48;
            else
                ShareData.theta_local = *ShareData.theta_Pt * 20.48 + 20.48;
            #else                    //(-0.5 ~ 0.5)
                ShareData.theta_local = *ShareData.theta_Pt * 651.9;
            #endif
                MasterData.theta_master = CanaRegs.CAN_IF1DATA.all & 0xffff;
            ParaCycles.Bits.CAN_Spll = 1;
            ParaCycles.Bits.CAN_MasterInNet = 1;

        }
//        else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdRlySync)
//        {
//            ParaEvent.Bits.WaitForCloseRly = CanaRegs.CAN_IF1DATA.all;
//            ParaEvent.Bits.WaitForTurnOff = !CanaRegs.CAN_IF1DATA.all;
//        }
//        else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdSendWorkState)
//        {
//            Flag_Local.Bits.WorkState_para = CanaRegs.CAN_IF1DATA.all;
//            ParaEvent.Bits.eStateReceive = 1;
//        }
    }

//    CANIntCnt++;
    CanaRegs.CAN_GLB_INT_CLR.all = 1;     //会清除NewDat位
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;


//    //TODO 应让IO中断可打断
//    if(CanaRegs.CAN_INT.bit.INT0ID == 0xC)
//    {
//        CanaRegs.CAN_IF1CMD.all = 0x1F000C;
////            ATESend();
//    }
//    else
//    {
//        //1.通信正常,解析CAN包
//        CanaRegs.CAN_IF1CMD.all = 0x3B0001;         // 传出 ID + NewDat / Clear Int Pending Bit+数据A+数据B / 邮箱1
//        if (CanaRegs.CAN_IF1MCTL.bit.NewDat == 1)   // new data into the data portion of this message object
//        {
//           // CanaRegs.CAN_IF1CMD.all = 0x1F0001;
//            // CanaRegs.CAN_IF1CMD.all = 0x1F0009;
//            if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdPwmSync)
//            {
//                if (EPwm1Regs.TBSTS.bit.CTRDIR)
//                {
//                    ShareData.TBCTR_local = EPwm1Regs.TBCTR;
//                }
//                else
//                {
//                    ShareData.TBCTR_local = -EPwm1Regs.TBCTR;
//                }
//
//                MasterData.TBCTR_master = CanaRegs.CAN_IF1DATA.all & 0xffff;
//                ParaCycles.Bits.CAN_Sync = 1;
//    //            ParaCycles.Bits.CAN_MasterInNet = 1;
//            }
//            else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdPhaseSync)
//            {
//                #ifdef Vision_IntType    //(0 - 200)
//                ShareData.theta_End = CpuTimer0.RegsAddr->TIM.all;
//                if(ShareData.theta_End < ShareData.theta_Start)
//                    ShareData.theta_local = (*ShareData.theta_Pt + (ShareData.theta_Start - ShareData.theta_End )*0.00005) * 20.48;
//                else
//                    ShareData.theta_local = *ShareData.theta_Pt * 20.48 + 20.48;
//                #else                    //(-0.5 ~ 0.5)
//                    ShareData.theta_local = *ShareData.theta_Pt * 651.9;
//                #endif
//                MasterData.theta_master = CanaRegs.CAN_IF1DATA.all & 0xffff;
//                ParaCycles.Bits.CAN_Spll = 1;
//                ParaCycles.Bits.CAN_MasterInNet = 1;  //AT1
//            }
////            else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdRlySync)
////            {
////                ParaEvent.Bits.WaitForCloseRly = CanaRegs.CAN_IF1DATA.all&(1L<<0);
////                ParaEvent.Bits.WaitForTurnOff = !ParaEvent.Bits.WaitForCloseRly;
////            }
//            else if ((CanaRegs.CAN_IF1ARB.bit.ID & 0xff) == CmdSendWorkState)
//            {
//                Flag_Local.Bits.WorkState_para = CanaRegs.CAN_IF1DATA.all;
//                ParaEvent.Bits.eStateReceive = 1;
//            }
//        }
//    }
//
//    CanaRegs.CAN_GLB_INT_CLR.all = 1;
//    PieCtrlRegs.PIEACK.all = PIEACK_GROUP9;
}

/**********************************************************************
* PURPOSE :  CAN总线故障处理
* CALLED BY:
**********************************************************************/
void sCommCanFailChk(void)
{
//    if(McanaRegs.MCAN_PSR.bit.BO)
//    {
//        if(McanaRegs.MCAN_CCCR.bit.INIT)
//        {
//            McanaRegs.MCAN_CCCR.bit.INIT = 0;
//        }
//        Flag_Local.Bits.CommCanFail = 1;
//    }
}

void sUserDebugData(User_CAN_DATA_UNION *CanMsg)
{
     switch (CanMsg->Data_1)
     {
//         case 0x40: Input.Start=__fsat((float)CanMsg->Data_0, 1,0);
//                 break;

         case 0x19: DroopCntl.Ro = __fsat((float)CanMsg->Data_0 * 0.1, 4,0);
                 break;

         case 0x1A: DroopCntl.Rc = __fsat((float)CanMsg->Data_0 * 0.01, 0.3,-0.3);
                 break;

         case 0x2F: VaRmsCntl.Ki = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                    VbRmsCntl.Ki = VaRmsCntl.Ki;
                 break;

         case 0x30: VaRmsCntl.Kp = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                    VbRmsCntl.Kp = VaRmsCntl.Kp;
                 break;

         case 0x31: DroopCntl.Kq = __fsat((float)CanMsg->Data_0 * 0.0001, 1,0);
                 break;

         case 0x32: DroopCntl.QaCntl.Ki = __fsat((float)CanMsg->Data_0 * 1e-6f, 1,0);
                    DroopCntl.QbCntl.Ki = DroopCntl.QaCntl.Ki;
                 break;

         case 0x33: DroopCntl.Kq2 = __fsat((float)CanMsg->Data_0 * 1e-6f, 1,0);
                    DroopCntl.QaCntl.Kp = DroopCntl.Kq2;
                    DroopCntl.QbCntl.Kp = DroopCntl.Kq2;
                 break;

         case 0x34: DroopCntl.Kp = __fsat((float)CanMsg->Data_0 * 0.0001 , 1,0);
                 break;

         case 0x35: DroopCntl.Kp2 = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                 break;

         case 0x36: VaIdcCntl.Ki = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                    VbIdcCntl.Ki = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                 break;

         case 0x37: VaIdcCntl.Kp = __fsat((float)CanMsg->Data_0 * 0.1, 1,0);
                    VbIdcCntl.Kp = __fsat((float)CanMsg->Data_0 * 0.1, 1,0);
                 break;

         case 0x38: DroopCntl.Kdc = __fsat((float)CanMsg->Data_0 * 0.001, 1,0);
                 break;

         case 0x3C: ShareData.Q_limit = __fsat((float)CanMsg->Data_0,1000,0);
                 break;

         case 0x3D: ShareData.P_limit = __fsat((float)CanMsg->Data_0,10000,0);
                 break;

         default:
                 break;
     }
}




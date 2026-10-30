/*
 * ParaCntl.c
 *
 *  Created on: 2024楠烇拷1閺堬拷24閺冿拷
 *      Author: DLadmin
 *      閻楀牊婀伴敍姝�2.0 PR021
 *
 */
#include "Parallel.h"
#include "string.h"
#include "protect.h"
#include "InvStateMachine.h"
#include "bsp.h"
#include "OffGridInv2.h"
#include "Sample.h"
#include "CalculateAngle.h"
#include "ConfigSystemPara.h"

#define PowerBufferSize    (125)    //200个点
int Qa_local_Buffer[PowerBufferSize];
int Qb_local_Buffer[PowerBufferSize];
int Pa_local_Buffer[PowerBufferSize];
int Pb_local_Buffer[PowerBufferSize];
PIDCntl_F   VaRmsCntl,VbRmsCntl;
PIDCntl_F   VaIdcCntl,VbIdcCntl;

typedef struct{
    Uint16 time;     //
    Uint16 cnt;      //
}FLAG_CHECK_STRUCT;

 char mFlagCounter(Uint16 flag,FLAG_CHECK_STRUCT * Counter)
{
    if (flag)
    {
        Counter->cnt++;
    }
    else
    {
        Counter->cnt = 0;
    }
    if (Counter->cnt >= Counter->time)
    {
        Counter->cnt = 0;
        return (1);
    }
    else
    {
        return (0);
    }
}

void ControlFaultCheck(void);
void Init_Droop_Vars(DROOPCntl_F* v);

void InitParaCntl(void)
{
    #define  Vrms_sat  10    //濮濓絽鐖堕崷锟�1V娴犮儱鍞�

    Flag_Local.Bits.invPwmOn = 0;

    memset(Pa_local_Buffer, 0, sizeof(Pa_local_Buffer));
    memset(Pb_local_Buffer, 0, sizeof(Pb_local_Buffer));
    memset(Qa_local_Buffer, 0, sizeof(Qa_local_Buffer));
    memset(Qb_local_Buffer, 0, sizeof(Qb_local_Buffer));
    memset(&ShareData.P_local, 0, sizeof(ShareData.P_local));
    memset(&ShareData.Q_local, 0, sizeof(ShareData.Q_local));
    memset(&ShareData.P_Sum, 0, sizeof(ShareData.P_Sum));
    memset(&ShareData.Q_Sum, 0, sizeof(ShareData.Q_Sum));
    Init_PI_Controller(&VaRmsCntl, 0.02f, 0.02f, Vrms_sat, -Vrms_sat);    //0.02,0.02 -> 0.01, 0.005
    Init_PI_Controller(&VbRmsCntl, 0.02f, 0.02f, Vrms_sat, -Vrms_sat);
    Init_PI_Controller(&VaIdcCntl, 0.1f, 0.002f, 10, -10);
    Init_PI_Controller(&VbIdcCntl, 0.1f, 0.002f, 10, -10);
    Init_PI_Controller(&DroopCntl.QaCntl, 4e-4f, 4e-6f, 0.05, -0.05);  //0.01
    Init_PI_Controller(&DroopCntl.QbCntl, 4e-4f, 4e-6f, 0.05, -0.05);  //4e-5f

    VaRmsCntl.Ref = (*ShareData.SystemVolt);
    VbRmsCntl.Ref = (*ShareData.SystemVolt);

    Init_Droop_Vars(&DroopCntl);
}

Struct_Abc_Int Q_local_inst;
Struct_Abc_Int P_local_inst;

#pragma CODE_SECTION(OffGridPowerCal, ".TI.ramfunc");
void OffGridPowerCal(void)
{
    #define  I_MOVE_BUFF_SIZE   ((*ShareData.Systemfreq)*(2.0f/PWM_ISR_FREQUENCY))

    Uint16 bufferCnt = 0;

    Struct_Abc_F InvVolt, InvCurr;

//    Flag_Local.Bits.invPwmOn = UrCtrl.FwStartOver && UsCtrl.FwStartOver && (SystemPara.OpenLoopEn == 0);

    InvCurr.a = SampleIns.Inv1CurrIns;
    InvCurr.b = SampleIns.Inv2CurrIns;
    InvVolt.a = SampleIns.Inv1VoltIns;
    InvVolt.b = SampleIns.Inv2VoltIns;

    bufferCnt = InvPhase.ThetaPointR >> 1;                  // 10K/60 = 166
    if (bufferCnt >= PowerBufferSize)
    {
        bufferCnt= (PowerBufferSize - 1);
    }
    // 无功调节
    if (InvPhase.ThetaPointR % 2)
    {
        // 滤波电容有无功/若加感性负载也带来无功
        Q_local_inst.a = -__cos(InvPhase.ThetaR) * UrCtrl.RefTarget * InvCurr.a;
        Q_local_inst.b = -__cos(InvPhase.ThetaS) * UsCtrl.RefTarget * InvCurr.b;
        ShareData.Q_Sum.a += (Q_local_inst.a - Qa_local_Buffer[bufferCnt]);
        ShareData.Q_Sum.b += (Q_local_inst.b - Qb_local_Buffer[bufferCnt]);
        ShareData.Q_local.a = ShareData.Q_Sum.a * I_MOVE_BUFF_SIZE;
        ShareData.Q_local.b = ShareData.Q_Sum.b * I_MOVE_BUFF_SIZE;
        Qa_local_Buffer[bufferCnt] =  Q_local_inst.a;
        Qb_local_Buffer[bufferCnt] =  Q_local_inst.b;
//        //
//        if(InvPhase.ThetaPointR == 1)
//        {
//            ShareData.Q_local.a = 0;
//            ShareData.Q_local.b = 0;
//        }

    }
    // 有功调节
    else
    {
        P_local_inst.a = InvVolt.a * InvCurr.a;
        P_local_inst.b = InvVolt.b * InvCurr.b;
        ShareData.P_Sum.a += (P_local_inst.a - Pa_local_Buffer[bufferCnt]);//*I_MOVE_BUFF_SIZE;
        ShareData.P_Sum.b += (P_local_inst.b - Pb_local_Buffer[bufferCnt]);//*I_MOVE_BUFF_SIZE;
        ShareData.P_local.a = ShareData.P_Sum.a * I_MOVE_BUFF_SIZE;
        ShareData.P_local.b = ShareData.P_Sum.b * I_MOVE_BUFF_SIZE;
        Pa_local_Buffer[bufferCnt] =  P_local_inst.a;
        Pb_local_Buffer[bufferCnt] =  P_local_inst.b;
    }
}
/**********************************************************************
* PURPOSE :  闁棗褰夐悽闈涘竾楠炲懎锟借壈鐨熼懞锟�
* CALLED BY: 200usTask
**********************************************************************/
void ParaControl()
{
//    static Uint32 TimerVoltAmpAdj;
//    static Uint32 TimerVoltAveAdj;
    if( Flag_Local.Bits.invPwmOn)
//    if( Flag_Local.Bits.invPwmOn && Status.bits.RlyChkOk)
    {
        if(ParaCycles.Bits.CrossZeroUpdate)
        {
            ParaCycles.Bits.CrossZeroUpdate = 0;

//            TrackTargetVL1(ats.TargetVoltL1);
//            TrackTargetVL2(ats.TargetVoltL2);
//            TrackTargetVBus(ats.TargetVoltBus);

            if(sGetModuleType() == cSlaveModule )
            {
                VaRmsCntl.xp[0] = 0;
                VbRmsCntl.xp[0] = 0;
                VaRmsCntl.max = Vrms_sat;
                VaRmsCntl.min = -Vrms_sat;
                VbRmsCntl.max = Vrms_sat;
                VbRmsCntl.min = -Vrms_sat;
            }
            else
            {
//                float tmp = 100;
//                tmp = __sqrt((*ShareData.VCap.a)*(*ShareData.VCap.a)-(*ShareData.V0.a)*(*ShareData.V0.a));
//                VaRmsCntl.xp[0] = __fsat( VaRmsCntl.Ref - tmp , 3 , -5);
//                tmp = __sqrt((*ShareData.VCap.b)*(*ShareData.VCap.b)-(*ShareData.V0.b)*(*ShareData.V0.b));
//                VbRmsCntl.xp[0] = __fsat( VbRmsCntl.Ref - tmp , 3 , -5);

                VaRmsCntl.xp[0] = __fsat( VaRmsCntl.Ref - (*ShareData.VCap.a) , 3 , -5);
                VbRmsCntl.xp[0] = __fsat( VbRmsCntl.Ref - (*ShareData.VCap.b) , 3 , -5);
                VaRmsCntl.max = 2;
                VaRmsCntl.min = -2;
                VbRmsCntl.max = 2;
                VbRmsCntl.min = -2;
                DroopCntl.sigma.a = 0;
                DroopCntl.sigma.b = 0;
            }

//            VaRmsCntl.xp[0] -= DroopCntl.Kp * (*ShareData.P_local.a);
//            VbRmsCntl.xp[0] -= DroopCntl.Kp * (*ShareData.P_local.b);

//                CpuToCla1Msg.QdroopTheta1 = Cla1ToCpuMsg.AvgValue.Q1 * DroopCntl.Kq;
//                CpuToCla1Msg.QdroopTheta2 = Cla1ToCpuMsg.AvgValue.Q2 * DroopCntl.Kq;

            if(Flag_Local.Bits.MasterExist && sGetModuleType() == cSlaveModule) // && Status.bits.RlyMClosed
            {
                ShareData.P_Soc = 0;   //无soc
                VaRmsCntl.xp[0] -= DroopCntl.Kp2 * (DroopCntl.P_delta.a - ShareData.P_Soc);
                VbRmsCntl.xp[0] -= DroopCntl.Kp2 * (DroopCntl.P_delta.b - ShareData.P_Soc);

                //q droop
                DroopCntl.QaCntl.xp[0] = DroopCntl.Q_delta.a;
                PI_Controller(&DroopCntl.QaCntl);
                DroopCntl.sigma.a =DroopCntl.QaCntl.yp[0];

                DroopCntl.QbCntl.xp[0] = DroopCntl.Q_delta.b;
                PI_Controller(&DroopCntl.QbCntl);
                DroopCntl.sigma.b =DroopCntl.QbCntl.yp[0];

                mRefRamp(DroopCntl.QaCntl.Kp , DroopCntl.Kq2, 4e-5f);
                DroopCntl.QbCntl.Kp = DroopCntl.QaCntl.Kp;
                //dc
//                VaIdcCntl.xp[0] = -DroopCntl.Idc_delta.a;
//                VbIdcCntl.xp[0] = -DroopCntl.Idc_delta.b;
//                PI_Controller(&VaIdcCntl);
//                PI_Controller(&VbIdcCntl);

                //閹垹顦叉妯款吇
                DroopCntl.P_delta.a = ShareData.P_Soc;
                DroopCntl.P_delta.b = ShareData.P_Soc;
                DroopCntl.Q_delta.a = ShareData.P_Soc;
                DroopCntl.Q_delta.b = ShareData.P_Soc;
            }

            PI_Controller(&VaRmsCntl);
            PI_Controller(&VbRmsCntl);
//            else if(sGetModuleType() == cMasterModule)
//            {
//                DroopCntl.sigma.a = 0;
//                DroopCntl.sigma.b = 0;
////                Init_PI_Controller(&VaRmsCntl, 0.02f, 0.02f, Vrms_sat, -Vrms_sat);    //0.02,0.02 -> 0.01, 0.005
////                Init_PI_Controller(&VbRmsCntl, 0.02f, 0.02f, Vrms_sat, -Vrms_sat);
//            }
            //濞村鍣洪崐鍏兼纯閺傦拷
            ControlFaultCheck();
//            ShareData.local.Vavg.a = 0.99f * ShareData.local.Vavg.a + 0.01f * (Cla1ToCpuMsg.VL1CntlRmsValue);
///            ShareData.local.Vbus.a = 0.99f * ShareData.local.Vbus.a + 0.01f * (Cla1ToCpuMsg.AvgValue.VBus);
//            ShareData.local.Vavg.a = 0.99f * ShareData.local.Vavg.a + 0.01f * (*ShareData.VCap.a);
//            ShareData.local.V0.a = 0.995f * ShareData.local.V0.a + 0.005f * (*ShareData.V0.a);
//            ShareData.local.Pavg.a = 0.9f * ShareData.local.Pavg.a + 0.1f * (*ShareData.P_local.a);
//            ShareData.local.Iavg.a = 0.9f * ShareData.local.Iavg.a + 0.1f * (*ShareData.Idc_local.a);
        }
    }

    if(ParaEvent.Bits.eClearParaCntl)
    {
        InitParaCntl();
        InitOffGridInvControlData();
        ParaEvent.Bits.eClearParaCntl = 0;
    }
}

void sCalEquilibriumPower(void)
{
//    //0閸旂喓宸奸崪灞惧姬閸旂喓宸兼稉宥呮儙閻€劌娼庣悰鈥冲閼筹拷
//    if(fabsf(*ShareData.P_local.a) > 100
//    && *ShareData.P_local.a) <  )
//    {
//         = __fsat(    );
//    }
//    else
//    {
//        ShareData.P_Soc = 0;
//    }


}

//
void ControlFaultCheck(void)
{
    static FLAG_CHECK_STRUCT SlaveRmsFaultCnt = {15000,0};    //1500ms/100us
    static FLAG_CHECK_STRUCT MasterRmsFaultCnt = {15000,0};   //1500ms/100us

    //open circuit check
    if(Flag_Local.Bits.invPwmOn && ParaEvent.Bits.enParallel)
    {
        //杈撳嚭姝ｉケ鍜岋紝杈撳嚭鐢靛帇楂�
        if(mFlagCounter( (VaRmsCntl.yp[0] > (Vrms_sat*0.9) && (VaRmsCntl.Ref - (*ShareData.VCap.a)) < -10)
                      || (VbRmsCntl.yp[0] > (Vrms_sat*0.9) && (VbRmsCntl.Ref - (*ShareData.VCap.b)) < -10)
                      ,&SlaveRmsFaultCnt))
        {
            Flag_Local.Bits.Open = 1;   //鏈満寮�璺紝涓绘満甯﹁浇锛屽睆钄界爫娉�
        }
        //杈撳嚭璐熼ケ鍜�
        if(mFlagCounter( (VaRmsCntl.yp[0] < -(Vrms_sat*0.9))
                      || (VbRmsCntl.yp[0] < -(Vrms_sat*0.9))
                      ,&MasterRmsFaultCnt))
        {
            Flag_Local.Bits.Open = 1;   //涓绘満寮�璺紝浠庢満甯﹁浇
        }
    }
    else
    {
        SlaveRmsFaultCnt.cnt = 0;
        MasterRmsFaultCnt.cnt = 0;
    }

//                if(DroopCntl.P_delta.a > 2900)
//                {
//
//                }

}

void Init_DroopCoeffs(Uint16 enable)
{
    DroopCntl.Vadj.a = 1;
    DroopCntl.Vadj.b = 1;

    if(enable)
    {
        DroopCntl.Rc  = 0.0f;
        DroopCntl.Ro  = 0.3f;   //0.3
        DroopCntl.Kq  = 0;      //0.0002
        DroopCntl.Kq2 = 5e-6f;   //5e-6f 閺傦拷2閸欙拷
        DroopCntl.Kp  = 0.0003;  //4000w = 1.2v
        DroopCntl.Kp2 = 0.050;   //0.02
        DroopCntl.Kdc = 0.000;   //0.003
    }
    else
    {
        DroopCntl.Rc  = 0.0f;
        DroopCntl.Ro  = 0.06f;   //0.06
        DroopCntl.Kq  = 0;
        DroopCntl.Kq2 = 0;
        DroopCntl.Kp  = 0.0000;   //0.0025
        DroopCntl.Kp2 = 0;
        DroopCntl.Kdc = 0.0;
    }
}
void Init_Droop_Vars(DROOPCntl_F* v)
{
//    ShareData.P_avg.a = 0;
//    ShareData.Q_avg.a = 0;
    DroopCntl.Iadj.a = 0;
    DroopCntl.sigma.a = 0;
    DroopCntl.sigma.b = 0;
    //    CpuToCla1Msg.QdroopTheta1 = 0;
    DroopCntl.P_delta.a = 0;
    DroopCntl.P_delta.b = 0;
    DroopCntl.P_delta.c = 0;
    DroopCntl.Q_delta.a = 0;
    DroopCntl.Q_delta.b = 0;
    DroopCntl.Q_delta.c = 0;
    DroopCntl.Idc_delta.a = 0;
    DroopCntl.Pdc_delta = 0;
    //
}

void Para_ResetWorkState(void)
{
    static Uint16 workstatelocal = InitState;

    if(workstatelocal == OffGridState || workstatelocal == UpsState)
    {
        if(InvStateData.WorkState != OffGridState && InvStateData.WorkState != UpsState)
        ParaEvent.Bits.eClearParaCntl = 1;
    }

    workstatelocal = InvStateData.WorkState;

    // 离网切并网判断
    if(workstatelocal == OffGridState || workstatelocal == UpsState)
    {
        Flag_Local.Bits.OnGridWorking =  (InvPhase.SyncReadyFlag == 1);
        Flag_Local.Bits.OffGridWorking = 1;
    }
    else if(workstatelocal == OnGridState || workstatelocal == ByPass)
    {
        Flag_Local.Bits.OnGridWorking =  (SystemAlarm.bit.LossGrid == 0);
        Flag_Local.Bits.OffGridWorking = 0;
    }
    else
    {
        Flag_Local.Bits.OnGridWorking = 0;
        Flag_Local.Bits.OffGridWorking = 0;
    }

}

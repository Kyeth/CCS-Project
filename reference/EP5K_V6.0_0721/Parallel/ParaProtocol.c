/*
 * ParaProtocol.c
 *
 *  Created on: 2024楠烇拷1閺堬拷24閺冿拷
 *      Author: DLadmin
 *      閻楀牊婀伴敍姝�2.0 PR021
 */
#include "Parallel.h"
#include "CalculateAngle.h"
//娑撶粯婧�缂佺喕顓稿Ο鈥虫健閺佹壆娲�
static Uint16 uwRedundancyNum = 0;     //redundancy module number in parallel system.
static Uint16 uwLoadNum = 0;           //the number of module with load
static Uint16 uwTotalNum = 0;          //total module number;
static Uint16 uwFaultNum = 0;          //fault module number;
static Uint16 uwSlaveNum = 0;          //slave module number;
static Uint16 uwNewNum = 0;            //new module number;
static Uint16 uwReceiveNewCnt = 0;
static Uint16 uwReceiveSlaveCnt = 0;
static Uint16 uwOnlineSlaveReturnCnt = 0;

//濡�虫健閺嶅洤绻旀担锟�,dsp閸戝搫宸禝D
PARA_STATUS_UNION Flag_Local;
PARA_STATUS_UNION Flag_para[cParaMaxNum];
Uint32 UID_para[cParaMaxNum] = {0,};    //TODO 1300使用SN编址？

extern void sMasterModuleInit(void);
extern void sSlaveModuleInit(void);
void sTrackAvePower(bool enable ,Uint16 address);
void sCalcMinSop(Uint16 address);
void sSocActiveEquilibrium(void);
void SaveSystemUID(Uint32 uid ,Uint32 Flag);
void CheckModuleNum();
void sSendDebug(Uint16 cmd);

/**********************************************************************
* PURPOSE :
* CALLED BY: Master
**********************************************************************/
void sCANParsingForMasterModule(void)
{
    User_CAN_DATA_UNION ParsingCan = {0,};

    if (sCheckCanRx(&ParsingCan))
    {
        if(ParsingCan.Cmd == CmdMultiCheck)
        {
            sSlaveModuleInit();
        }
        else if(ParsingCan.Cmd == CmdSystemTurnOff)
        {
            Flag_Local.Bits.SystemRunOk = 0;
        }
        else if (ParsingCan.Cmd == Cmd3PhasePowerP)    //楠炶埖婧�娑撳鐎崝鐔哄芳鐠嬪啳濡�
        {
            ShareData.P_para[ParsingCan.SrcAddr].a = (int)ParsingCan.Data_0;
            ShareData.P_para[ParsingCan.SrcAddr].b = (int)ParsingCan.Data_1;
            ShareData.P_para[ParsingCan.SrcAddr].c = (int)ParsingCan.Data_2;
            sTrackAvePower(1 , ParsingCan.SrcAddr);
        }
        else if (ParsingCan.Cmd == Cmd3PhasePowerQ)    //楠炶埖婧�娑撳鐎崝鐔哄芳鐠嬪啳濡�
        {
            ShareData.Q_para[ParsingCan.SrcAddr].a = (int)ParsingCan.Data_0;
            ShareData.Q_para[ParsingCan.SrcAddr].b = (int)ParsingCan.Data_1;
            ShareData.Q_para[ParsingCan.SrcAddr].c = (int)ParsingCan.Data_2;
            sTrackAvePower(0 , ParsingCan.SrcAddr);
            ShareData.SendCnt_para = ParsingCan.Data_3;
            ShareData.SendCnt_delta = ShareData.SendCnt_local - ShareData.SendCnt_para;
        }
        else if (ParsingCan.Cmd == CmdBatInfo)    //楠炶埖婧�娑撳鐎崝鐔哄芳鐠嬪啳濡�
        {
            ShareData.SOP_para[ParsingCan.SrcAddr] = (int)ParsingCan.Data_0;
            ShareData.SOC_para[ParsingCan.SrcAddr] = (int)ParsingCan.Data_1;
//            sCalcMinSop(ParsingCan.SrcAddr);
        }
        else if (ParsingCan.Cmd == CmdCompeteMaster)
        {
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
        }
        else if (ParsingCan.Cmd == ReplyNewMsg)
        {
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
            uwReceiveNewCnt++;
        }
        else if (ParsingCan.Cmd == ReplySlaveMsg)
        {
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
            uwReceiveSlaveCnt++;
        }
//        else if (ParsingCan.Cmd == ReplySynAction)
//        {
//            uwOnlineSlaveReturnCnt++;
//        }
//        else if (ParsingCan.Cmd == ReplyDeltaTheta)
//        {
//            CANTEST[17] = (int)ParsingCan.Data_0 * 0.001f;
//            CANTEST[18] = (int)ParsingCan.Data_1 * 0.001f;
//        }
        else if (ParsingCan.Cmd == UserDebugData)
        {
            sUserDebugData(&ParsingCan);
        }
    }
}

/**********************************************************************
* PURPOSE :
* CALLED BY: Slave
**********************************************************************/
void sCANParsingForSlaveModule(void)
{
    User_CAN_DATA_UNION ParsingCan = {0,};

    if (sCheckCanRx(&ParsingCan))
    {
        switch(ParsingCan.Cmd)
        {
//            case( InqSynAction):
//            {
//               if(ParaCycles.Bits.InquirySynAction == 0)
//               {
//                   ParaCycles.Bits.InquirySynAction = 1;
//                   sSendUint32Frame(ReplySynAction, ShareData.UID, Flag_Local.Word);
//               }
//               break;
//            }
            case( CmdSystemTurnOff):  //
            {
               Flag_Local.Bits.SystemRunOk = 0;
               break;
            }
            case( CmdMasterTurnOnCheckOk):  //
            {
               Flag_Local.Bits.SystemRunOk = 1;
               break;
            }
            case( Cmd3PhasePowerP):    //楠炶埖婧�娑撳鐎崝鐔哄芳鐠嬪啳濡�
            {
                ShareData.P_para[ParsingCan.SrcAddr].a = (int)ParsingCan.Data_0;
                ShareData.P_para[ParsingCan.SrcAddr].b = (int)ParsingCan.Data_1;
                ShareData.P_para[ParsingCan.SrcAddr].c = (int)ParsingCan.Data_2;

//                if(ShareData.P_local_SendCnt != ParsingCan.Data_3)  //if different time, send local info
                {
                    sSendActivePower();
                    ShareData.P_local_SendCnt = ParsingCan.Data_3;
                }
                sTrackAvePower(1 , ParsingCan.SrcAddr);
//                ShareData.Pdc_para = (int)ParsingCan.Data_3;
//                DroopCntl.Pdc_delta = __fsat( *ShareData.Pdc_local - ShareData.Pdc_para , ShareData.P_limit , -ShareData.P_limit);
                break;
            }
            case( Cmd3PhasePowerQ):    //楠炶埖婧�娑撳鐎崝鐔哄芳鐠嬪啳濡�
            {
                ShareData.Q_para[ParsingCan.SrcAddr].a = (int)ParsingCan.Data_0;
                ShareData.Q_para[ParsingCan.SrcAddr].b = (int)ParsingCan.Data_1;
                ShareData.Q_para[ParsingCan.SrcAddr].c = (int)ParsingCan.Data_2;

//                if(ShareData.Q_local_SendCnt != ParsingCan.Data_3)  //if different time, send local info  //11
                {
                    sSendReactivePower();
                    ShareData.Q_local_SendCnt = ParsingCan.Data_3;
                }

                sTrackAvePower(0 , ParsingCan.SrcAddr);
//                ShareData.SendCnt_para = ParsingCan.Data_3;
//                ShareData.SendCnt_delta = ShareData.SendCnt_local - ShareData.SendCnt_para;
                break;
            }
            case( CmdCompeteMaster):
            {
                sSendUint32Frame(ReplyUID, ShareData.UID, Flag_Local.Word);
                SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);            //read UID
                break;
            }
            case( ReplyUID):
            {
                SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);            //read UID
                break;
            }
            case( InqSlaveMoudle):
            {
                Flag_Local.Bits.CommCanFail = 0;    //(鐎甸�涚艾閺堝绨烘稉銏犲瘶閸ㄥ绗夐懗鎴掔濞嗏剝鏁归崚鏉挎皑濞撳懘娅庨弫鍛存)
                sSendUint32Frame(ReplySlaveMsg, ShareData.UID ,Flag_Local.Word);
                SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);            //read UID
                break;
            }
            case( ReplySlaveMsg):
            {
                SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);            //read UID
                break;
            }
            case( CmdMasterAssignLocation):
            {
                //not support
                if(ShareData.UID == ParsingCan.DATA_32 && ParsingCan.DATB_32 < cParaMaxNum)
                {
                    ShareData.CommAddr = ParsingCan.DATB_32;
                    Flag_Local.Bits.SrcAddr = ShareData.CommAddr;
                }
                break;
            }
//            case( InqSynAction):
//            {
//                sSendUint32Frame(ReplySynAction, 0, 0);
//                break;
//            }
            case( CmdDcCurrent):      //楠炶埖婧�閻╁瓨绁﹂崚鍡涘櫤鐠嬪啳濡�
            {
                ShareData.Idc_para.a = ((float)(int)ParsingCan.Data_0 * 0.01f);//
                ShareData.Idc_para.b = ((float)(int)ParsingCan.Data_1 * 0.01f);
                ShareData.Idc_para.c = ((float)(int)ParsingCan.Data_2 * 0.01f);
                DroopCntl.Idc_delta.a = __fsat((*ShareData.Idc_local.a - ShareData.Idc_para.a) , ShareData.Idc_limit , -ShareData.Idc_limit);
                DroopCntl.Idc_delta.b = __fsat((*ShareData.Idc_local.b - ShareData.Idc_para.b) , ShareData.Idc_limit , -ShareData.Idc_limit);
                DroopCntl.Idc_delta.c = __fsat((*ShareData.Idc_local.c - ShareData.Idc_para.c) , ShareData.Idc_limit , -ShareData.Idc_limit);
                break;
            }
            case( CmdVoltAvgAdj):     //楠炶埖婧�閻㈤潧甯囩拫鍐Ν閿涘矂鍣伴弽宄版▕瀵倸绱╃挧椋庢畱
            {
                ShareData.para.Vavg.a = ParsingCan.Data_0 * 0.02f;
                ShareData.para.Vavg.b = ParsingCan.Data_1 * 0.02f;

                DroopCntl.Vadj.a = ShareData.para.Vavg.a/ShareData.local.Vavg.a;
                DroopCntl.Vadj.b = ShareData.para.Vavg.b/ShareData.local.Vavg.b;

//                mRefRamp(DroopCntl.Vadj.a,(ShareData.para.Vavg.a/ShareData.local.Vavg.a),0.0001f);
//                mRefRamp(DroopCntl.Vadj.b,(ShareData.para.Vavg.b/ShareData.local.Vavg.b),0.0001f);

                DroopCntl.Vadj.a = __fsat(DroopCntl.Vadj.a , 1.02 , 0.98);
                DroopCntl.Vadj.b = __fsat(DroopCntl.Vadj.b , 1.02 , 0.98);
                DroopCntl.Vadj.c = __fsat(DroopCntl.Vadj.c , 1.02 , 0.98);
                break;
            }
            case( CmdVoltDcAdj):     //楠炶埖婧�閻㈤潧甯囩拫鍐Ν閿涘矂鍣伴弽宄版▕瀵倸绱╃挧椋庢畱
            {
                ShareData.para.V0.a = (float)(int)ParsingCan.Data_0 * 0.02f;
                ShareData.para.V0.b = (float)(int)ParsingCan.Data_1 * 0.02f;
                mRefRamp(DroopCntl.V0adj.a,(ShareData.para.V0.a-ShareData.local.V0.a),0.01f);
                mRefRamp(DroopCntl.V0adj.b,(ShareData.para.V0.b-ShareData.local.V0.b),0.01f);
                DroopCntl.V0adj.a = __fsat(DroopCntl.V0adj.a , 5 , -5);
                DroopCntl.V0adj.b = __fsat(DroopCntl.V0adj.b , 5 , -5);
                DroopCntl.V0adj.c = __fsat(DroopCntl.V0adj.c , 5 , -5);
                break;
            }
            case( CmdDcAvgAdj):
            {
                ShareData.para.Iavg.a = (int)ParsingCan.Data_0 * 0.02f;
                ShareData.para.Iavg.b = (int)ParsingCan.Data_1 * 0.02f;
                DroopCntl.Iave_delta.a = ShareData.local.Iavg.a -  ShareData.para.Iavg.a;
                DroopCntl.Iave_delta.b = ShareData.local.Iavg.b -  ShareData.para.Iavg.b;
                DroopCntl.Iadj.a -= VaIdcCntl.Ki *(DroopCntl.Iave_delta.a);
                DroopCntl.Iadj.b -= VbIdcCntl.Ki *(DroopCntl.Iave_delta.b);
                DroopCntl.Iadj.a = __fsat(DroopCntl.Iadj.a , 10 , -10);
                DroopCntl.Iadj.b = __fsat(DroopCntl.Iadj.b , 10 , -10);
                break;
            }

//            case( CmdDeltaTheta):    //
//            {
//                ShareData.droopTheta_para.a = (int)ParsingCan.Data_0 * 0.001f;
//                ShareData.droopTheta_para.b = (int)ParsingCan.Data_1 * 0.001f;
//                if(ParsingCan.SrcAddr == (ShareData.CommAddr - 1))  //0->1->2->... send in turn
//                {
//                    int data0,data1;
//                    data0 = CpuToCla1Msg.QdroopTheta1 * 1000.0f;
//
//                    sSendInt16Frame(ReplyDeltaTheta, data0, data1, 0, 0);
//                }
//                break;
//            }

            case( CmdBatInfo):
            {
//                ShareData.SOP_para[ParsingCan.SrcAddr] = (int)ParsingCan.Data_0;
//                ShareData.SOC_para[ParsingCan.SrcAddr] = (int)ParsingCan.Data_1;
////                sCalcMinSop(ParsingCan.SrcAddr);
//                sSocActiveEquilibrium();
                break;
            }

            case( CmdSystemSet):
            {
//                if(ParsingCan.Data_3 == cSetTypeStartFlag)
//                {
//                    Input.Start = ParsingCan.Data_0;
//                    Input.TurnOffFlag = 11;
//                }
//                else if(ParsingCan.Data_3 == cSetTypeTurnOff)
//                {
//                    Input.Start = 0;
//                    Input.TurnOffFlag = 12;
//                }
                break;
            }
            case( CmdSlave2Master):
            {
                if(ParsingCan.DATA_32 == ShareData.UID)
                {
                    sMasterModuleInit();
                    sSendUint32Frame(CmdMultiCheck,ShareData.UID, Flag_Local.Word);

                    //閸樼喐娼甸惃鍕瘜閺堝搫绨茬拠銉ュ綁娑撹桨绮犻張锟�
                    //SaveSlaveUID(ParsingCan.DATB_32);
                    //UID_para[0] = ParsingCan.DATB_32;
                }
                break;
            }
            case( UserDebugData):
            {
                sUserDebugData(&ParsingCan);
                break;
            }

            default:
            {
                sSendDebug(ParsingCan.Cmd);
                break;
            }
          }
    }
}
/**********************************************************************
* PURPOSE :
* CALLED BY: New
**********************************************************************/
void sCANParsingForNewModule(void)
{
    User_CAN_DATA_UNION ParsingCan ={0,};

    if (sCheckCanRx(&ParsingCan))
    {
        //1.0閸忣剙鍙￠幐鍥︽姢
        if (ParsingCan.Cmd == CmdCompeteMaster)              //read UID and send UID
        {
            sSendUint32Frame(ReplyUID, ShareData.UID, Flag_Local.Word);
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
        }
        else if(ParsingCan.Cmd == ReplyUID)                  //read UID
        {
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
        }
        else if (ParsingCan.Cmd == CmdMasterAssignLocation)
        {
            //閼汇儰瀵岄張鍝勫絺閻ㄥ垊ID閺勵垵鍤滃鍗炲灟閹存劒璐熸禒搴㈡簚
            if(ShareData.UID == ParsingCan.DATA_32 && ParsingCan.DATB_32 < cParaMaxNum)
            {
                ShareData.CommAddr = ParsingCan.DATB_32;
                Flag_Local.Bits.SrcAddr = ShareData.CommAddr;
                Flag_Local.Bits.AddressingOk = 1;
                sSlaveModuleInit();
            }
        }
        //master inquiry new module command
        else if (ParsingCan.Cmd == InqNewModule)
        {
            sSendUint32Frame(ReplyNewMsg, ShareData.UID, Flag_Local.Word);
            SaveSystemUID(ParsingCan.DATA_32,ParsingCan.DATB_32);
            Flag_Local.Bits.MasterExist = 1;
        }
    }
}

/**********************************************************************
* PURPOSE :  the master InquiryNewModule
* CALLED BY: master
**********************************************************************/
void sInquiryNewModule(void)
{
    Uint16 uwCheckId;

    if (ParaCycles.Bits.InquiryNewModule)
    {
        ParaCycles.Bits.InquiryNewModule = 0;
        uwReceiveNewCnt = 0;
        sSendUint32Frame(InqNewModule, ShareData.UID, Flag_Local.Word);
    }
    else
    {
        //there are new modules' responses
        if (uwReceiveNewCnt != 0)
        {
            for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
            {
                if( Flag_para[uwCheckId].Bits.ParaMode == cNewModule)
                {
                    sSendUint32Frame(CmdMasterAssignLocation,UID_para[uwCheckId],(uwCheckId+1));
                    uwReceiveNewCnt --;
                    break;
                }
            }
            ParaCycles.Bits.SystemSetSend = 1;            //閸氭垶鏌婂Ο鈥虫健閸欐垿锟戒胶閮寸紒鐔活啎缂冿拷
            Flag_Local.Bits.ParaRunning = 1;
            Init_DroopCoeffs(1);
        }
        else
            ParaCycles.Bits.InquiryNewModule = 1;
    }
}

/**********************************************************************
* PURPOSE :  the master InquirySlaveModule 缂佺喕顓告禒搴㈡簚閺佸府绱濇禒搴㈡簚閻樿埖锟斤拷
* CALLED BY: master
**********************************************************************/
void sInquirySlaveModule(void)
{
    Uint16 uwCheckId = 0;

    //閻㈠彉绨弬鐗堟簚閹绘帒鍙嗙�佃壈鍤ф禒搴㈡簚閸︽澘娼冩稉宥咁嚠閿涘矂鍣搁弬鏉垮瀻闁板秴婀撮崸锟�
    if(ParaCycles.Bits.InquirySlaveModule == 0)
    {
        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
        {
            if( Flag_para[uwCheckId].Bits.ParaMode == cSlaveModule &&  Flag_para[uwCheckId].Bits.SrcAddr != (uwCheckId+1) )
            {
                sSendUint32Frame(CmdMasterAssignLocation,UID_para[uwCheckId],(uwCheckId+1));
                ParaCycles.Bits.InquirySlaveModule = 1;
                return;
            }
        }
    }

    ParaCycles.Bits.InquirySlaveModule = 0;
    uwReceiveSlaveCnt = 0;
    sSendUint32Frame(InqSlaveMoudle, ShareData.UID, Flag_Local.Word);
}


/**********************************************************************
* PURPOSE :  the master CheckSystemStatus
* CALLED BY: master
**********************************************************************/
void sMasterCheckSystemStatus(void)
{
    Uint16 uwCheckId;
    Uint16 SlaveReadyNum = 0;

    //1.缂佺喕顓稿Ο鈥虫健閺佹壆娲�
    CheckModuleNum( );

//    //2.瀵拷閺堢儤褰欓幍锟�      //( 绾喛顓诲В蹇撳酱閺堝搫娅掗柈鑺ヮ劀鐢瓕绐� + 缂佈呮暩閸ｃ劍婀崥绋挎値 + 楠炶埖婧�濡�崇础鏉╂劘顢� + 娑撶粯婧�鐎涙ê婀�)
//    if (!Flag_Local.Bits.OffGridWorking && Flag_Local.Bits.PowerOnOk
//            && Flag_Local.Bits.ParaRunning && Flag_Local.Bits.AddressingOk
//            && Flag_Local.Bits.SelfCheck)
//    {
//        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
//        {
//            if( Flag_para[uwCheckId].Bits.PowerOnOk
//            &&  Flag_para[uwCheckId].Bits.PhaseLocked
//            &&  Flag_para[uwCheckId].Bits.PwmSyncOk
//            &&  Flag_para[uwCheckId].Bits.MasterExist
//            &&  Flag_para[uwCheckId].Bits.AddressingOk
//            && !Flag_para[uwCheckId].Bits.MoudleFault
//            &&  Flag_para[uwCheckId].Bits.SelfCheck     //楠炶埖婧�娴ｈ儻鍏橀崣鐗堟殶OK
//            &&  Flag_para[uwCheckId].Bits.freq == Flag_Local.Bits.freq
//            &&  Flag_para[uwCheckId].Bits.volt == Flag_Local.Bits.volt)
//            {
//                SlaveReadyNum ++;    //濮濓絽鐖跺銉ょ稊閻拷
//            }
//        }
//
//        //TODO CAN閺�璺哄煂閸ョ偛顦查惃鍕矤閺堢儤鏆�
//        if(SlaveReadyNum == uwSlaveNum          //3閸涖劍婀￠崘鍛躬缁捐法娈�
//        && SlaveReadyNum == uwReceiveSlaveCnt   //閺堫剚顐奸柅姘繆閸︺劎鍤庨惃锟�
//        && SlaveReadyNum == (ShareData.RealNumber - 1)) //ems鐠佸墽鐤嗛惃鍕酱閺侊拷
//        {
//            if(Flag_Local.Bits.SystemRunOk == 0)
//            {
//                sSendUint32Frame(CmdMasterTurnOnCheckOk,0,0); //TODO 娴犲孩婧�濞屸剝鏁归崚锟� 婵″倷缍嶆径鍕倞閿涳拷
//                Flag_Local.Bits.SystemRunOk = 1;
//                return;
//            }
//        }
//    }
//
//    if(Flag_Local.Bits.SystemRunOk)
//    {
//        //3.娑撶粯婧�鐎靛湱閮寸紒鐔荤箻鐞涘瞼娲冨ù瀣剁礉婵″倹鐏夋潻鎰攽娑擃厽婀侀張鍝勬珤閺佸懘娈伴幋鏍т粻閺堝搫鍨崗鎶芥４缁崵绮�
//        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
//        {
//            if(Flag_para[uwCheckId].Bits.AddressingOk
//            &&(Flag_para[uwCheckId].Bits.MoudleFault
//            || Flag_para[uwCheckId].Bits.PowerOnOk == 0))
//            {
//                //閸忚櫕婧�楠炶泛绠嶉幘锟�,鏉╂瑩鍣烽崣顏冪窗閸忔娊锟藉棗褰�
//                sSendUint32Frame(CmdSystemTurnOff,0,0);
//                Flag_Local.Bits.SystemRunOk = 0;
//                return;
//                //TODO 鐎靛綊娓堕崗鎶芥４dcdc閻ㄥ嫰銆嶉惄顔碱槱閻烇拷
//            }
//        }
//    }
//    else
//    {
//        //4.娑撶粯婧�鐎靛湱閮寸紒鐔荤箻鐞涘瞼娲冨ù瀣剁礉婵″倹鐏夋稉缁樻簚婢跺嫪绨崗铏簚閿涘矁锟藉苯鍙剧�瑰啯婧�閸ｃ劌顦╂禍搴ょ箥鐞涘瞼濮搁幀渚婄礉閸掓瑥鍙ч梻顓犻兇缂侊拷
//        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
//        {
//            if(Flag_para[uwCheckId].Bits.SystemRunOk)
//            {
//                sSendUint32Frame(CmdSystemTurnOff,0,0);
//                Flag_Local.Bits.SystemRunOk = 0;
//                return;
//            }
//        }
//    }
}


//  閸欐垿锟戒箒ID 閸滐拷  楠炶埖婧�閻樿埖锟斤拷, 婵″倹鐏夋稉缁樻簚瀹歌尙绮＄敮锕佹祰娑撳秴鍘戠拋绋垮瀼閹诡澀瀵岄張锟�/娑撶粯婧�閺佸懘娈扮粩瀣煝閸掑洦宕茬粵澶岀搼
void sSendCompeteData(void)
{
    sSendUint32Frame(CmdCompeteMaster, ShareData.UID, Flag_Local.Word);
}

//閸欐垿锟戒礁鎮撳銉や繆閸欏嘲宓嗘稉杞板瘜閺堬拷
//void sMasterInNetInfo(void)
//{
//    sSendInt16Frame(CmdModuleNum, uwRedundancyNum, uwLoadNum, uwFaultNum, uwTotalNum);
//}

/**********************************************************************
* PURPOSE :  閸欐垿锟戒焦婀侀崝鐔峰閻滃洤娼庨崚鍡楁姎
* CALLED BY: master  4ms
**********************************************************************/
void sSendActivePower(void)
{
    int16 data0,data1,data2,data3;

    ShareData.P_para[ShareData.CommAddr].a = ShareData.P_local.a; //save local power
    ShareData.P_para[ShareData.CommAddr].b = ShareData.P_local.b; //save local power
    ShareData.P_para[ShareData.CommAddr].c = ShareData.P_local.c; //save local power

    if(Flag_Local.Bits.invPwmOn)
    {
        data0 = ShareData.P_local.a;
        data1 = ShareData.P_local.b;
        data2 = ShareData.P_local.c;
        data3 = ShareData.P_local_SendCnt++;   //同步帧     //*ShareData.Pdc_local
        sSendInt16Frame(Cmd3PhasePowerP, data0, data1, data2, data3);
    }

}

/**********************************************************************
* PURPOSE :  閸欐垿锟戒焦妫ら崝鐔峰閻滃洤娼庨崚鍡楁姎
* CALLED BY: master  4ms
**********************************************************************/
void sSendReactivePower(void)
{
    int16 data0,data1,data2,data3;

    ShareData.Q_para[ShareData.CommAddr].a = ShareData.Q_local.a; //save local power
    ShareData.Q_para[ShareData.CommAddr].b = ShareData.Q_local.b; //save local power
    ShareData.Q_para[ShareData.CommAddr].c = ShareData.Q_local.c; //save local power

    if(Flag_Local.Bits.invPwmOn)
    {
        data0 = (float)ShareData.Q_local.a;
        data1 = (float)ShareData.Q_local.b;
        data2 = (float)ShareData.Q_local.c;
        data3 = ShareData.Q_local_SendCnt++;     //follow first power share
        sSendInt16Frame(Cmd3PhasePowerQ, data0, data1, data2, data3); // CmdSendQ
    }
}

/**********************************************************************
* PURPOSE :  閸欐垿锟戒胶娲垮ù浣烘暩濞达拷
* CALLED BY: master  4ms
**********************************************************************/
void sSendDcCurrent(void)
{
    int16 data0,data1,data2;

    data0 = *ShareData.Idc_local.a * 100.0f ;//
    data1 = *ShareData.Idc_local.b * 100.0f ;
    data2 = *ShareData.Idc_local.c * 100.0f ;
    sSendInt16Frame(CmdDcCurrent, data0, data1, data2, 0);
}

/**********************************************************************
* PURPOSE :  主机故障即将退出，命令从机取代主机
* CALLED BY: master  4ms
**********************************************************************/
char sCheckMasterToSlave (void)
{
    Uint16 uwCheckId;

    //判断主机被动停止逆变
    if((ParaEvent.Bits.MasterFault || Flag_Local.Bits.MoudleFault) && Flag_Local.Bits.CommCanFail == 0)
    {
        //寻找第一台正常机器,若无,继续当主机
        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
        {
            if(Flag_para[uwCheckId].Bits.SelfCheck)
            {
                ParaEvent.Bits.MasterToBeSlave = 1;
                ParaEvent.Bits.MasterFault = 0;
                sSendUint32Frame(CmdSlave2Master,UID_para[0],ShareData.RealNumber);
                return 1;
            }
        }
        ParaEvent.Bits.MasterFault = 0;
    }
    else
    {
        ParaEvent.Bits.MasterToBeSlave = 0;
    }
    return 0;
}
/**********************************************************************
* PURPOSE :  缁旂偘绨ゆ稉缁樻簚
* CALLED BY: master  4ms
**********************************************************************/
bool sMasterCompetition(void)
{
    #define cMasterCompetitionDely    3//compete master delay 16ms
    static Uint16 uwMasterCompetitionTimeCnt = 0;

    //have received master's message,exit the competition
    if(ParaCycles.Bits.CAN_MasterInNet)
    {
        ParaCycles.Bits.CAN_MasterInNet = 0;    //reset for the next detection
        Flag_Local.Bits.MasterExist = 1;
        uwMasterCompetitionTimeCnt = 0;
        ParaCycles.Bits.MasterCompetitionOver = 1;
        return (false);
    }

//    if(ParaEvent.Bits.MasterToBeSlave)
//    {
//        sSendUint32Frame(CmdSlave2Master,UID_para[0],ShareData.RealNumber);
//        return (false);
//    }
//    else if(Flag_Local.Bits.invPwmOn)
//    {
//        return (false);
//    }

    //Competition begin
    if (uwMasterCompetitionTimeCnt == 0)
    {
        UID_para[0] = 0; //韫囧懘銆忓〒鍛竴閿涘奔绗夐悞鏈电瑓濞喡ょ箹娑撶嫪D濞屸�冲棘娑撳海鐝垫禍澶涚礉娑旂喍绶烽悞璺虹秼娑撳秳绗傛稉缁樻簚
        sSendCompeteData();
        uwMasterCompetitionTimeCnt++;
    }
    //wait to receive other module's compete messages
    else if (uwMasterCompetitionTimeCnt < cMasterCompetitionDely)  //鏉╃偟鐢荤粵澶婄窡16ms
    {
        uwMasterCompetitionTimeCnt++;
    }
    else if (uwMasterCompetitionTimeCnt == cMasterCompetitionDely)
    {
        uwMasterCompetitionTimeCnt = 0;
        ParaCycles.Bits.MasterCompetitionOver = 1;

        if (UID_para[0] != 0) //there is another module on the CAN bus(閸欘亝婀侀弨璺哄煂娴滃棙鐦懛顏勭箒婢堆呮畱UID閹靛秵鍨氭稉杞板瘜濡�虫健閿涘矂妲诲銏″复閸忋儵娼張顒�宕楃拋顔炬畱CAN)
        {
            if (ShareData.UID < UID_para[0])//succeed in the competition
            {
                Flag_Local.Bits.ParaRunning = 1;//楠炶埖婧�鏉╂劘顢�
                return (true);
            }
            else //fail in the competition
            {
                return (false);
            }
        }
        //there is only one module on the can bus (UID_para[0] = 0鐠囧瓨妲戝▽鈩冩箒閺堝搫娅掗崶鐐差槻 )
        //Ack Error: The message this CAN Core transmitted was not acknowledged by another node.
        //楠炶埖婧�閸欙絼绗夐懗鍊熺箾鐠嬪啳鐦疌AN閿涘苯鎯侀崚姗焌naRegs.CAN_ES.bit.LEC != 3
//        else if(sEnableParallel() == 0)     //CanaRegs.CAN_ES.bit.LEC == 3
//        {
//            //TODO 婢跺嫪绨崡鏇熸簚鏉╂劘顢戝Ο鈥崇础閿涘苯褰傞崙鍝勬啞鐠�锔芥Ц閸氾箑绻曠拋鎷岀箾CAN缁捐￥锟戒竼AN缁炬寧鏅犻梾锟�
//            return (true);
//        }
    }
    return (false);
}


void sMasterInNetDetect(void)
{
    #define cMasterInNetDetectDelay              17//delay of detecting master in net
    static Uint16 uwMasterInNetDetectTimeCnt = 0; //detecting count of master in net
    //not receive master's message
    if (!ParaCycles.Bits.CAN_MasterInNet)
    {
        if (uwMasterInNetDetectTimeCnt < cMasterInNetDetectDelay)
        {
            uwMasterInNetDetectTimeCnt++;
        }
        else   //鏉╃偟鐢� 40ms 閺堫亝鏁归崚棰佸瘜閺堣桨淇婇崣锟�,master is not in the net
        {
            Flag_Local.Bits.MasterExist = 0;
            ParaCycles.Bits.MasterInNetDetectOver = 1;
            uwMasterInNetDetectTimeCnt = 0;
        }
    }
    //have received master's message,the master is in the net
    else
    {
        ParaCycles.Bits.CAN_MasterInNet = 0;    //reset for the next detection
        Flag_Local.Bits.MasterExist = 1;
        ParaCycles.Bits.MasterInNetDetectOver = 1;
        uwMasterInNetDetectTimeCnt = 0;
    }
}


/**********************************************************************
* PURPOSE :  娑擄拷閸欑増婧�閸ｃ劌寮弫鏉垮綁閺囪揪绱濋崣鎴犵舶閹碉拷閺堝婧�閸ｏ拷
* CALLED BY: master/slave
* Parameters : uwSynSetSelect 鐠佸墽鐤嗘禍鍡楁憿娑擃亜寮弫锟�        uwSendModule娑擄拷 master/slave
**********************************************************************/
void sSendSystemSetting(Uint16 uwSynSetSelect,Uint16 uwSendModule)
{
    ParaEvent.Bits.eParaSet = 0;

//    if(uwSynSetSelect == cSetTypeStartFlag)
//    {
//        sSendInt16Frame(CmdSystemSet, Input.Start, 0, 0, uwSynSetSelect);
//    }
//    else if(uwSynSetSelect == cSetTypeTurnOff)
//    {
//        if(Input.Start == 0)
//        {
//            sSendInt16Frame(CmdSystemSet, 0, 0, 0, uwSynSetSelect);
//        }
//    }
//    else if(uwSynSetSelect & cSetTypeInputVoltRange)
//    {
//        sSendUint32Frame(CmdSystemSet, 100, 200);
//    }
//    else if(uwSynSetSelect & cSetTypeOutputVoltage)
//    {
//        sSendUint32Frame(CmdSystemSet, 100, 200);
//    }
//    else if(uwSynSetSelect & cSetTypeOutputFreq)
//    {
//        sSendUint32Frame(CmdSystemSet, 100, 200);
//    }
//    else if(uwSynSetSelect & cSetTypeRunningMode)
//    {
//        sSendUint32Frame(CmdSystemSet, 100, 200);
//    }
//    else if(uwSynSetSelect & cSetTypeOutputPF)
//    {
//        sSendUint32Frame(CmdSystemSet, 100, 200);
//    }
}

//data3閸欘垳鏁ら弶銉︾垼鐠囧棙鏁归崚鏉挎憿娑擃亙淇婇幁锟�
void sSendMasterData(void)
{
    int16 data0,data1,data2,data3;
    static Uint16 SendMasterDataCyc = 0;

    if (SendMasterDataCyc == 0)
    {
//        data0 = ShareData.local.Vavg.a * 50.0f;
//        data1 = ShareData.local.Vavg.b * 50.0f;
//        data2 = ShareData.local.Vavg.c * 50.0f;
//        data3 = 0;
//        sSendInt16Frame(CmdVoltAvgAdj, data0, data1, data2, data3);
    }
    else if (SendMasterDataCyc == 1)
    {
//        data0 = ShareData.local.Iavg.a * 50.0f;
//        data1 = ShareData.local.Iavg.b * 50.0f;
//        data2 = ShareData.local.Iavg.c * 50.0f;
//        data3 = 0;
//        sSendInt16Frame(CmdDcAvgAdj, data0, data1, data2, data3);
    }
    else if (SendMasterDataCyc == 2)
    {
//        data0 = ShareData.local.V0.a * 50.0f;
//        data1 = ShareData.local.V0.b * 50.0f;
//        sSendInt16Frame(CmdVoltDcAdj, data0, data1, 0, 0);
    }
    else if (SendMasterDataCyc == 4)
    {
////        data0 = Cla1ToCpuMsg.AvgValue.VBat * 0.1 *(*ShareData.SOP_local);
//        data1 = *ShareData.SOC_local;
////        data2 = 0;
////        data3 = ;
//        sSendInt16Frame(CmdBatInfo, data0, data1, data2, 0);
    }
    else
    {
        sSendDebug(0x40 + SendMasterDataCyc);
    }


    if (++SendMasterDataCyc > 11)
        SendMasterDataCyc = 0;
}


void sSendDebug(Uint16 cmd)
{
    int16 data0,data1,data2,data3;

    switch(cmd)
    {
        case(0x40):  //
        {
            data0 = cmd;
            data1 = cmd;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x41):  //
        {
            data0 = cmd;
            data1 = cmd;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x42):  //
        {
            data0 = cmd;
            data1 = cmd;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x43):  //
        {
            data0 = cmd;
            data1 = cmd;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x44):  //
        {
            //        data0 = SystemError.Fault1.all;
            //        data1 = SystemError.Fault2.all;
            //        data2 = SystemError.Fault3.all;
            //        data3 = SystemAlarm.all;
            data0 = cmd;
            data1 = cmd;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x45):  //
        {
            data0 = UID_para[0];
            data1 = UID_para[1];
            data2 = ShareData.theta_Adj * 10000.0f;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x46):  //
        {
            data0 = VaRmsCntl.yp[0] * 50.0f;
            data1 = VbRmsCntl.yp[0] * 50.0f;
            data2 = DroopCntl.sigma.a * 10000;
            data3 = DroopCntl.sigma.b * 10000;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }

        case(0x50):  //
        {
            data0 = ShareData.local.Pavg.a;
            data1 = ShareData.local.Pavg.b;
            data2 = ShareData.local.Pavg.c;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x51):  //
        {
            data0 = ShareData.local.Vbus.a * 50.0f;
            data1 = ShareData.local.Vbus.b * 50.0f;
            data2 = cmd;
            data3 = cmd;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }
        case(0x52):  //
        {
            data0 = *ShareData.VCap.a;
            data1 = *ShareData.VCap.b;
            data2 = *ShareData.VCap.c;
            sSendInt16Frame(cmd, data0, data1, data2, data3);
           break;
        }

        default:
          break;
      }
}


/**********************************************************************
* PURPOSE :  鐎涙ê鍋峌ID楠炴湹绮犵亸蹇撳煂婢堆勫笓鎼达拷    閺�璺哄煂閻╁摜鐡戦幋鏍ㄦ纯鐏忓繒娈慤ID 閹存牕缍嬮崜宥勮礋缁岋拷,鐎涙ê鍋峌ID,娴犲孩婧�1娑撶皢ara[0]
* CALLED BY: 閹恒儲鏁归崚鐨峣d
**********************************************************************/
void SaveSystemUID(Uint32 uid ,Uint32 Flag)
{
    Uint16  uwCheckId;
    Uint32  UIDtmp;
    Uint16  Flagtmp;

//    ResistanceMatching(uid);

    for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
    {
        if(uid == 0)
        {
            break;
        }
        else if(uid == UID_para[uwCheckId])//閺�璺哄煂閻╁摜鐡戦惃鍒処D
        {
            Flag_para[uwCheckId].Word  = Flag;
            break;
        }
        else if(uid < UID_para[uwCheckId]  ||  0 == UID_para[uwCheckId]) //閹烘帒绨�
        {
            UIDtmp = UID_para[uwCheckId];
            Flagtmp = Flag_para[uwCheckId].Word;

            UID_para[uwCheckId] = uid;
            Flag_para[uwCheckId].Word  = Flag;

            uid = UIDtmp;
            Flag = Flagtmp;
        }
    }
}

void sInitUIDTable()
{
    Uint16 uwCheckId;

    for(uwCheckId = 0;uwCheckId < cParaMaxNum; uwCheckId++)
    {
        UID_para[uwCheckId]  = 0;
        Flag_para[uwCheckId].Word = 0;
    }

    uwTotalNum = 0;
    uwRedundancyNum = 0;
    uwLoadNum = 0;
    uwFaultNum = 0;
    uwSlaveNum = 0;
    uwNewNum = 0;
}

/**********************************************************************
* PURPOSE :  缂佺喕顓搁弫浼村櫤
* CALLED BY: master
**********************************************************************/
void CheckModuleNum( )
{
    Uint16 uwCheckId;
    Uint16 LoadNumTmp = 0;           //the number of module with load
    Uint16 TotalNumTmp = 1;          //total module number;
    Uint16 FaultNumTmp = 0;          //fault module number;
    Uint16 SlaveNumTmp = 0;          //slave module number;
    Uint16 NewNumTmp = 0;            //new module number;

    for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
    {
        if(UID_para[uwCheckId] != 0)
        {
            if(Flag_para[uwCheckId].Bits.OfflineCheck++ >= 3)// count 2 times 缁撅拷60ms
            {
                //娴犲孩婧�閺堫亜娲栨径宥勫瘜閺堢尨绱濇稉宥嗗Г閺佸懘娈�(閺�顖涘瘮闁劌鍨庨張鍝勬珤缁傝崵鍤�)
                //娑撳秵绔婚梽銈堫嚉ID閿涘瞼鐡戝鍛従閹垹顦查柌宥堢箾閿涳拷(缁楊兛绔村▎锟犲櫢閺傜増甯撴惔蹇撶暚閹存劕鎮楅敍灞肩瑝閺�顖涘瘮闁插秵鏌婇幒鎺戠碍)
//                UID_para[uwCheckId] = 0;
//                Flag_para[uwCheckId].Word = 0;
                Flag_para[uwCheckId].Bits.OfflineCheck = 3;
                if(Flag_Local.Bits.ParaRunning)
                {
                    Flag_Local.Bits.CommCanFail = 1;
                }
            }
            else
            {
                TotalNumTmp ++;
            }

            if(Flag_para[uwCheckId].Bits.CommCanFail || Flag_para[uwCheckId].Bits.MoudleFault)
                FaultNumTmp ++;
            if(Flag_para[uwCheckId].Bits.OffGridWorking == 1 )
                LoadNumTmp ++;
            if(Flag_para[uwCheckId].Bits.ParaMode == cSlaveModule)
                SlaveNumTmp ++;
            if(Flag_para[uwCheckId].Bits.ParaMode == cNewModule)
                NewNumTmp ++;
        }
    }

    uwTotalNum = TotalNumTmp;
    uwFaultNum = FaultNumTmp;
    uwSlaveNum = SlaveNumTmp;
    uwLoadNum  = LoadNumTmp;
    uwNewNum   = NewNumTmp;

    ShareData.RealNumber = uwTotalNum;

    //2.绾喛顓婚獮鑸垫簚妫板嫯顔曢崣鐗堟殶閿涘本鍨氶崝鐔锋倵瀵拷婵绱崸锟�
    if( ShareData.RealNumber == uwTotalNum && ShareData.RealNumber > 1)
    {
        Flag_Local.Bits.AddressingOk = 1;
        Flag_Local.Bits.CommCanFail = 0;
        Flag_Local.Bits.TotalNumFault = 0;
//        ParaEvent.Bits.eStartAssignLocation = 1;
    }
    else
    {
        //3.閸欑増鏆熸径褌绨０鍕啎閸婄》绱濇稉濠冨Гapp闁插秴鎯庨獮鑸垫簚缁崵绮�?
//        if( ShareData.RealNumber < uwTotalNum )
        Flag_Local.Bits.TotalNumFault = 1;
    }



}

/**********************************************************************
* PURPOSE :  楠炲啿娼庨崝鐔哄芳鏉╁�熼嚋
* CALLED BY:
**********************************************************************/
void sTrackAvePower(bool enable ,Uint16 address)
{
    Uint16 uwCheckId = 0;
    Struct_Abc_F totalPower = {0,0,0};
    float Knumber = 1.0f/(ShareData.RealNumber - 1);

    if(ShareData.CommAddr == (ShareData.RealNumber - 1) && address == (ShareData.RealNumber - 2)
            || address == (ShareData.RealNumber - 1))    //if last frame cal ave value
    {
        if(enable == 1)
        {
            for(uwCheckId = 0; uwCheckId < ShareData.RealNumber; uwCheckId++)
            {
                if(uwCheckId != ShareData.CommAddr)   //
                {
                    totalPower.a += ShareData.P_para[uwCheckId].a;
                    totalPower.b += ShareData.P_para[uwCheckId].b;
                    totalPower.c += ShareData.P_para[uwCheckId].c;
                }
            }
            DroopCntl.P_delta.a = __fsat(
                    ShareData.P_local.a
                            - totalPower.a * Knumber,
                    ShareData.P_limit, -ShareData.P_limit);
            DroopCntl.P_delta.b = __fsat(
                    ShareData.P_local.b
                            - totalPower.b * Knumber,
                    ShareData.P_limit, -ShareData.P_limit);
            DroopCntl.P_delta.c = __fsat(
                    ShareData.P_local.c
                            - totalPower.c * Knumber,
                    ShareData.P_limit, -ShareData.P_limit);
        }
        else
        {
            for(uwCheckId = 0; uwCheckId < ShareData.RealNumber; uwCheckId++)
            {
                if(uwCheckId != ShareData.CommAddr)
                {
                    totalPower.a += ShareData.Q_para[uwCheckId].a;
                    totalPower.b += ShareData.Q_para[uwCheckId].b;
                    totalPower.c += ShareData.Q_para[uwCheckId].c;
                }
            }
            DroopCntl.Q_delta.a = __fsat(
                    ShareData.Q_local.a
                            - totalPower.a * Knumber,
                    ShareData.Q_limit, -ShareData.Q_limit);
            DroopCntl.Q_delta.b = __fsat(
                    ShareData.Q_local.b
                            - totalPower.b * Knumber,
                    ShareData.Q_limit, -ShareData.Q_limit);
            DroopCntl.Q_delta.c = __fsat(
                    ShareData.Q_local.c
                            - totalPower.c * Knumber,
                    ShareData.Q_limit, -ShareData.Q_limit);
        }
    }
}

/**********************************************************************
* PURPOSE :  濮瑰倹娓剁亸寮抩p
* CALLED BY:
*********************************************************************
void sCalcMinSop(Uint16 address)
{
    Uint16 uwCheckId;
    int16 data0,data1;
    float minSop = Cla1ToCpuMsg.AvgValue.VBat * 0.1 *(*ShareData.SOP_local);

    if(address == (ShareData.CommAddr - 1))  //0->1->2->... send power in turn
    {
         data0 = minSop;
         data1 = *ShareData.SOC_local;
         sSendInt16Frame(CmdBatInfo, data0, data1, 0, 0);
    }

    //瑜版挻鏁归崚鏉跨啲閸欐壆娈慡op閹存牗婀伴張杞拌礋鐏忔儳褰撮弨璺哄煂閸撳秳绔撮崣鎵畱Sop閿涘本鐪伴張锟界亸寮抩p閵嗭拷
    //閸嬪洩顔曢獮鑸垫簚閸欑増鏆� == 0,娑撳秵澧界悰灞伙拷鍌氫海鐠佹儳鑻熼張鍝勫酱閺佷即鏁婄拠锟�(閸嬪繐銇�)閿涘本妫ゅ▔鏇熸暪閸掓澘鐔崣棰佷繆閹垯绗夐幍褑顢戦妴锟�
    if(ShareData.CommAddr == (ShareData.RealNumber - 1) && address == (ShareData.RealNumber - 2)
            || address == (ShareData.RealNumber - 1))
    {
        for(uwCheckId = 0; uwCheckId < ShareData.RealNumber; uwCheckId++)
        {
            if(uwCheckId != ShareData.CommAddr)
            {
                minSop = __fmin(minSop,ShareData.SOP_para[uwCheckId]);
            }
        }
    }

    ShareData.SOP_min = minSop;
}*/

/**********************************************************************
* PURPOSE :  娑擄拷閸欑殜oc鏉堝啩缍嗛弮璁圭礉閸婃儳鎮滄禍搴ょ翻閸戦缚绶濈亸锟�/鏉堟挸鍙嗘潏鍐樋閻㈢敻鍣�
* CALLED BY: Slave
**********************************************************************/
//void sSocActiveEquilibrium(void)
//{
//    float DeltaSoc;
//
//    if(sGetModuleType() == cSlaveModule)
//    {
//        //瑜版悞oc瀹割喖顦挎禍锟� 10%閺冿拷,婢舵碍鍨ㄧ亸鎴ｇ翻閸戯拷10%閸旂喓宸�  //闁插洨鏁ゆ禒鍛瑢娑撶粯婧�濮ｆ棁绶濋惃鍕摜閻ｃ儻绱�
//        DeltaSoc = (int)*ShareData.SOC_local - (int)ShareData.SOC_para[0];
//        //閸欑姴濮為崸鍥��閸旂喓宸�
//        ShareData.P_Soc = __fsat(DeltaSoc * 20.0f ,100,-100);
//    }
//}

/**********************************************************************
* PURPOSE :  楠炶埖婧�缁崵绮烘稉濠勬暩濡拷濞村绱濋崡鏇熸簚閼奉亝顥呯�瑰本鍨氶崥搴ょ箥鐞涘矉绱濋獮鑸垫簚缁涘绶熼幍锟介張澶嬫簚閸ｏ拷
* CALLED BY:
**********************************************************************/
//#pragma CODE_SECTION(Para_CheckPowerOn, ".TI.ramfunc");
//Uint16 Para_CheckPowerOn(void)
//{
//    if(Flag_Local.Bits.ParaRunning)
//    {
//        Flag_Local.Bits.PowerOnOk = 1;
//        return (Flag_Local.Bits.SystemRunOk);
//    }
//    else if(sEnableParallel() == 0)
//    {
//        return 1;
//    }
//    return 0;
//}

/**********************************************************************
* PURPOSE :   楠炶埖婧�缁崵绮洪崗铏簚
* CALLED BY:
**********************************************************************/
//#pragma CODE_SECTION(Para_PowerOff, ".TI.ramfunc");
//void Para_PowerOff(void)
//{
//    Flag_Local.Bits.SystemRunOk = 0;  //papalleladd
//    Flag_Local.Bits.PowerOnOk = 0;
//
//    int uwCheckId;
//    for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
//    {
//        Flag_para[uwCheckId].Bits.PowerOnOk = 0;
//    }
//}


/**********************************************************************
* PURPOSE :   閸欐垿锟戒礁鑻熼張铏归兇缂佺喎鍙ч張锟�
* CALLED BY:
**********************************************************************/
//void Para_SendPowerOff(void)
//{
//    Flag_Local.Bits.SystemRunOk = 0;
////    Flag_Local.Bits.PowerOnOk = 0;
//    ParaEvent.Bits.eTurnOffSystem = 1;
//}

/**********************************************************************
* PURPOSE :   婵″倹鐏夊銉ょ稊濡�崇础閺�鐟板綁娴滃棴绱濋獮鎸庢尡閸欐垿锟戒焦鏌婂Ο鈥崇础
* CALLED BY:
//**********************************************************************/
//#pragma CODE_SECTION(Para_SendWorkState, ".TI.ramfunc");
//void Para_SendWorkState(Uint16 mode)
//{
//    static PARA_STATUS_UNION Flag_old;
//    if(Flag_old.Word != Flag_Local.Word)
//    {
//        Flag_old.Word = Flag_Local.Word;
//        return 1;
//    }
//
//}

/**********************************************************************
* PURPOSE :   如果并机工作模式改变了，返回1
* CALLED BY:
**********************************************************************/
//Uint16 Para_CheckWorkState(Uint16 mode)
//{
//    if(ParaEvent.Bits.eStateReceive)
//    {
//        ParaEvent.Bits.eStateReceive = 0;
//        if(Flag_Local.Bits.WorkState_para == mode)
//        {
//            switch(mode)
//            {
//                case 0:      //关机 //通知另一台机器关机就行了
//                 //   if(SciData.SystemSwitchOn == 1)SciData.SystemSwitchOn = 0;
//                    break;
//                case 1:      //离网
//                    break;
//                case 3:    //ups态
//                    break;
//                case 4:    //并网
//                    break;
//                default: break;
//            }
//            return 1;
//        }
//    }
//    return 0;
//}


/**********************************************************************
* PURPOSE :   如果并机工作模式改变了，返回1
* CALLED BY:
**********************************************************************/
Uint16 ChkNewFlagLocal(void)
{
    int uwCheckId;
    static PARA_STATUS_UNION OldFlag = {0};

    if (Flag_Local.Bits.OffGridWorking != OldFlag.Bits.OffGridWorking
     || Flag_Local.Bits.OnGridWorking != OldFlag.Bits.OnGridWorking )
    {
        OldFlag = Flag_Local;
        return InqSlaveMoudle;
    }
    OldFlag = Flag_Local;
    //继电器跟随
    if(Flag_Local.Bits.OffGridWorking)
    {
        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
        {
            if(Flag_para[uwCheckId].Bits.OnGridWorking)
            {
                return 1;
            }
        }
    }
    //
    if(Flag_Local.Bits.OnGridWorking)
    {
        for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
        {
            if(Flag_para[uwCheckId].Bits.OffGridWorking)
            {
                return 1;
            }
        }
    }
    return 0;
}

//
Uint16 Para_CheckInvRly(void)
{
    //如果有机器吸合电网继电器，则后来的需等待旁路信号走旁路开机。
    //如果有机器吸合负载继电器，则后来的需等待旁路信号走旁路开机。 或者等待离网同步完成再吸合负载继电器。
    //如果有机器吸合负载继电器，并且本机吸合，强制发送电网同步信号再吸合电网继电器。
    int uwCheckId;
    for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
    {
        if(Flag_para[uwCheckId].Bits.OffGridWorking)
        {
            return 1;
        }
    }
    return 0;
}
/**********************************************************************
* PURPOSE :  CheckGridRly
* CALLED BY:
**********************************************************************/
#pragma CODE_SECTION(CheckGridRly, ".TI.ramfunc");
Uint16 CheckGridRly(void)
{
    return Flag_para[0].Bits.OnGridWorking;
}

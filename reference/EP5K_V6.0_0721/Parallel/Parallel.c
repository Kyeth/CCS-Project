/*
 * Parallel.c
 *
 *  Created on: 2024婵°倗濮烽崕鎴﹀箯閿燂拷1闂備礁鎼悧鐐哄箯閿燂拷24闂備礁鎼崰娑㈠箯閿燂拷
 *      Author: DLadmin
 *      闂備胶绮〃鍛存偋婵犲偊鑰垮ù鐓庣摠閺咁剚鎱ㄥ┑鎾村2.0 PR021
 */
#include "Parallel.h"
#include "string.h"
#include "InvStateMachine.h"
#include "Parallel.h"
//#include "InverterControl.h"
#include "bsp.h"
#include "OffGridInv2.h"
#include "Sample.h"
#include "CalculateAngle.h"
#include "BoardTest.h"
#include "Protect.h"
#include "string.h"
#include "User_Modbus.h"
#include "ConfigSystemPara.h"
#include "ModbusTable.h"
#include <Function.h>

Uint32                 ParamsTimer;
PARA_CYCLE_UNION       ParaCycles;
PARA_EVENT_UNION       ParaEvent;
PARA_ShareData_STRUCT  ShareData ={0,};
PARA_ShareData_STRUCT  MasterData ={0,};

//閺勫墽銇�
AD_SAMPLE_Float32       ParaSample;
DROOPCntl_F             DroopCntl;
PIDCntl_F               PwmSyncCntl;
PIDCntl_F               PreSyncCntl;

void sDealPwmSync(void);
void sDealPhaseSync(void);
void sCommCanFailChk(void);
void ATECanReInit(void);
void sGetCanMos();
void sUpdateStatus(void);
void sNewModule(void);
void sSlaveModule(void);
void sSlaveModuleInit(void);
void sMasterModule(void);
void sMasterModuleInit(void);
void sSetModuleType(Uint16 type);
char sCheckCrossZero(void);
char sCheckMoudleFault(void);
/*******************************************************************************
Discription: 闂傚倷鐒﹀妯肩矓閸洘鍋柛鈩冪〒瀹撲線鏌ょ粙璺ㄤ粵缂併劍纰嶆穱濠囶敍濡ゅ瀚归柧蹇ｅ亜娴犳挳姊哄Ч鍥у閻庢凹鍨冲▎銏ゎ敇閵忕姷顦ч梺闈涱槶閸斿海绱欐繝姘厱闁瑰瓨绻冮幆鍫拷娈垮櫘閸撶喎顕ｉ锔芥櫢闁跨噦鎷� 闂備焦瀵ч崘濠氬箯閿燂拷 闂備焦鐪归崹钘夘焽瑜旈、娆撳箛閻楀牆浠洪梻鍌氱墛缁嬫垹绮╂ィ鍐╃厸闁割偅绻嶅Σ褰掓煛閸涱偄鍔︾�规洘鍔欓弫鎾绘晸閿燂拷 闂備礁鎼悧鍡浰囬娑欏弿婵炴垯鍨诲畵渚�鏌涢幇顒�绾х紓宥呮閺屾稒绻濊箛鏂款伓 闂傚倷绶￠崑鍕涘Δ鍕╀汗濠㈣埖鍔栭崑銊╂⒒閸喓鈽夌紒鏃堢畺閺岋綁鍩￠崨顔界彸缂傚倸绋勯幏锟� 缂傚倷鐒﹂崘濠氬箯閿燂拷
* CALLED BY:
*******************************************************************************/
void InitParallel(void)
{
    memset(&ShareData, 0, sizeof(PARA_ShareData_STRUCT));
    memset(&MasterData, 0, sizeof(PARA_ShareData_STRUCT));

    ParaCycles.Word = 0;
    ParaEvent.Word = 0;
    Flag_Local.Word = 0;
    Flag_Local.Bits.ParaMode = cNewModule;

    ShareData.Systemfreq   = &SystemPara.InvRateFreq;
    ShareData.SystemVolt   = &SystemPara.InvRateVolt;

    ShareData.theta_Pt      = &InvPhase.HrThetaR;          //逆变电压相位
    ShareData.VCap.a        = &SampleRms.Inv1VoltRms;    //电容电压有效值
    ShareData.VCap.b        = &SampleRms.Inv2VoltRms;    //电容电压有效值

    ShareData.V0.a        = &FuncPara.INVVAVG1;    //电容电压有效值
    ShareData.V0.b        = &FuncPara.INVVAVG2;    //电容电压有效值


//    ShareData.P_local.a     = &Cla1ToCpuMsg.P1MoveValue;
//    ShareData.Q_local.a     = &Cla1ToCpuMsg.AvgValue.Q1;

//    ShareData.Idc_local.a   = &Cla1ToCpuMsg.IL1MoveValue;
//    ShareData.Pdc_local     = &Cla1ToCpuMsg.AvgValue.DcWatt;
//    ShareData.SOC_local     = &BmsToPcsRegs.soc;
//    ShareData.SOP_local     = &BmsToPcsRegs.ChgSop;
//    ShareData.SOP_min       = cChargePower;
    ShareData.local.Vavg.a  = (*ShareData.SystemVolt);
    ShareData.local.Vavg.b  = (*ShareData.SystemVolt);
    ShareData.local.Vavg.c  = (*ShareData.SystemVolt);
    ShareData.theta_Adj     = 0;
    ShareData.theta_delay   = 25;     //EP800 = 30 ~ 37   // PR021=25
    ShareData.theta_gain    = 55;
    ShareData.Q_limit       = 60;     //60 - 2閸欑増鏌婇張鍝勮嫙閼憋拷
    ShareData.P_limit       = (2750*0.5);   //
    ShareData.Idc_limit     = 10;
    ShareData.TBCTR_delay   = 60;     //6000 - 50us

    ShareData.UID = *(Uint32*) 0x000703CC;  //28374
//    ShareData.UID = *(Uint32*) 0x0007020C;  //280039

    #define EPwmSyncRegs  EPwm1Regs
    EPwm2Regs.EPWMXLINK.bit.TBPRDLINK = 0;
    EPwm3Regs.EPWMXLINK.bit.TBPRDLINK = 0;
    EPwm4Regs.EPWMXLINK.bit.TBPRDLINK = 0;
    ShareData.TBPRD = EPwmSyncRegs.TBPRD;

    sInitUIDTable();
    Init_PI_Controller(&PwmSyncCntl, 0.0004,      0, 2, -2);
//    Init_PI_Controller(&PreSyncCntl, (0.01) , (0.01), 6, -6);  //+-6HZ
    Init_PI_Controller(&PreSyncCntl, 0.00003 ,0.00001, 6.28/(PWM_ISR_FREQUENCY/6), - 6.28/(PWM_ISR_FREQUENCY/6));  //ep800的参数 +-6HZ
    Init_DroopCoeffs(0);
    InitParaCntl();//
    InitParallelCan();
}
/**********************************************************************
* PURPOSE :
* CALLED BY:
**********************************************************************/
void ATECanReInit(void)
{
    static uint16_t ATEModeMem = 0;
    if(ATEModeMem != ArmWriteData.Fault.bit.ATEMode)
    {
        InitParallelCan();
    }
    ATEModeMem = ArmWriteData.Fault.bit.ATEMode;
}

void ParallelTask(void)
{
    sCommCanFailChk();

    ATECanReInit();

    Para_StartSystem();
    //role
    //sSetModuleType(cMasterModule);
    //sSetModuleType(cSlaveModule);

    if (sGetModuleType() == cNewModule)
    {
        sNewModule();
    }
    else if (sGetModuleType() == cSlaveModule)
    {
        sSlaveModule();
    }
    else if (sGetModuleType() == cMasterModule)
    {
        sMasterModule();
    }
    else
    {
        sSetModuleType(cNewModule);
    }

    ParaControl();
}

void sMasterModuleInit(void)
{
    Flag_Local.Bits.ParaMode = cMasterModule;
    Flag_Local.Bits.MasterExist = 1;
    Init_DroopCoeffs(Flag_Local.Bits.ParaRunning);

    ParaCycles.Bits.CAN_MasterInNet = 0;
    ParaCycles.Bits.MasterInNetDetectOver = 0;
    ParaCycles.Bits.MasterCompetitionOver = 0;

    VaRmsCntl.yp[1] = 0;
    VbRmsCntl.yp[1] = 0;
    ShareData.CommAddr = 0;
    Flag_Local.Bits.SrcAddr = ShareData.CommAddr;
    Init_PI_Controller(&PwmSyncCntl, 0.0004,      0, 2, -2);
    Init_PI_Controller(&PreSyncCntl, 0.00003 ,0.00001, 6.28/(PWM_ISR_FREQUENCY/6), - 6.28/(PWM_ISR_FREQUENCY/6));  //ep800的参数 +-6HZ
    ShareData.theta_Adj = 0; //
}


void sMasterModule(void)
{
    static Uint16 Can500usCyc = 0;
    static Uint16 Can5msCyc = 0;

    sCANParsingForMasterModule();

    if(Flag_Local.Bits.ParaMode != cMasterModule)
    return;

    //3.0
    if(ParamsTimer - CpuTimer1.RegsAddr->TIM.all >= cTimer500us)
    {
        ParamsTimer = CpuTimer1.RegsAddr->TIM.all;

        sUpdateStatus();

        //event1
//        if(ParaEvent.Bits.eTurnOffSystem)
//        {
//            sSendUint32Frame(CmdSystemTurnOff,0,0);
//            ParaEvent.Bits.eTurnOffSystem = 0;
//            return;
//        }
        if (ParaCycles.Bits.CAN_MasterInNet)
        {
            if(ParaEvent.Bits.MasterToBeSlave)
            {
                sSlaveModuleInit();
                ParaEvent.Bits.MasterToBeSlave = 0;
            }
            else
            {
                sSendUint32Frame(CmdMultiCheck,ShareData.UID, Flag_Local.Word);
                ParaCycles.Bits.CAN_MasterInNet = 0;
            }
            return;
        }

        if(ChkNewFlagLocal()==InqSlaveMoudle)
        {
            sSendUint32Frame(InqSlaveMoudle, ShareData.UID, Flag_Local.Word);
            return;
        }

//        if(McanaRegs.MCAN_TXBAR.all)
//            return;
        if(++Can500usCyc > 11)
        {
            Can500usCyc = 0;
        }
        //1ms Poll Avoid communication conflicts
        if (Can500usCyc == 0)
        {
            sSendReactivePower();
        }
        else if (Can500usCyc == 3)
        {
            sSendActivePower();
        }
        else if (Can500usCyc == 6)
        {
            sSendPhaseData();  //
        }
        else if (Can500usCyc == 7)
        {
            if(sCheckMasterToSlave() == 0)  //主机故障时切换为从机
            {
                sSendPwmTBCTR();
            }
        }
        else if (Can500usCyc == 8)
        {
            if(ChkNewFlagLocal())
            {
                sSendUint32Frame(InqSlaveMoudle, ShareData.UID, Flag_Local.Word);
            }
//            sSendDcCurrent();
        }
        else if (Can500usCyc == 9)
        {
            //5ms Poll  Slow information
            if (Can5msCyc == 0)
            {
                sInquirySlaveModule();
            }
            else if (Can5msCyc == 1)
            {
                sInquiryNewModule();
            }
            else if (Can5msCyc == 2)
            {
                sSendMasterData();
            }
            else if (Can5msCyc == 3)
            {
//                sSendSystemSetting(cSetTypeStartFlag,cMasterModule);
//                sSendSystemSetting(cSetTypeTurnOff,cMasterModule);
            }
            else if (Can5msCyc == 4)
            {
                sMasterCheckSystemStatus();   //5 * 12 *500us = 30ms
            }
            if(++Can5msCyc > 4)
            Can5msCyc = 0;
        }
    }
}

void sSlaveModuleInit(void)
{
    InitParaCntl();
    sInitUIDTable();

    Flag_Local.Bits.ParaMode = cSlaveModule;
    Flag_Local.Bits.ParaRunning = 1;
    Init_DroopCoeffs(1);

    ParaCycles.Bits.MasterInNetDetectOver = 0;
    ParaCycles.Bits.MasterCompetitionOver = 0;
    ParaCycles.Bits.CAN_MasterInNet = 1;

    VaRmsCntl.yp[1] = 0;
    VbRmsCntl.yp[1] = 0;

//    sInitUIDTable();
    Init_PI_Controller(&PwmSyncCntl, 0.0004,      0, 2, -2);
    Init_PI_Controller(&PreSyncCntl, 0.00003 ,0.00001, 6.28/(PWM_ISR_FREQUENCY/6), - 6.28/(PWM_ISR_FREQUENCY/6));  //ep800的参数 +-6HZ
    ShareData.theta_Adj = 0; //

}


void sSlaveModule(void)
{
    //1.spll
    if(ParaCycles.Bits.CAN_Spll)
    {
        sDealPhaseSync();
    }
    //2.pwm
    if (ParaCycles.Bits.CAN_Sync)
    {
        sDealPwmSync();
    }
//    //3.
//    if(ParaEvent.Bits.eTurnOffSystem)
//    {
//        sSendUint32Frame(CmdSystemTurnOff,0,0);
//        ParaEvent.Bits.eTurnOffSystem = 0;
//        return;
//    }

    //4.CAN receive
    sCANParsingForSlaveModule();

    if(Flag_Local.Bits.ParaMode != cSlaveModule)
        return;

    //5.闁诲氦顫夌喊宥咁渻閸屾稓顩烽悹鍥ㄥ絻椤拷
    if(ParamsTimer - CpuTimer1.RegsAddr->TIM.all >= cTimer5ms)
    {
        ParamsTimer = CpuTimer1.RegsAddr->TIM.all;

        sUpdateStatus();

        //5.2 濠碘槅鍋撻幏閿嬬箾閺夋埈鍎庣紒鏃�娼欓埢搴ㄥ灳閹绘帞妲ｉ梺鍝勭墢閸庛倕危閹间礁瑙﹂柨鏇嫹闁宦板姂瀹曠兘鏁撻敓锟�(5ms),婵炴垶鎸哥粔鎾偤閵娾晛鎹堕柕濞垮劚閻忕喖寮堕埡鍌溾槈闁告瑥妫欑粙澶屾媼瀹曞洨鐓犵紓浣规⒒閸嬫鑺遍敓锟�/闂佽桨绀侀幊妯衡枔閿燂拷, 闁诲孩绋掗敋婵狅拷椤忓牆绀嗘繛鍡樻尭濞呫垻绱撻崘鎯ф灍闁绘牭缍�缁犳稑顪冮崜褏孝
        //濠碘槅鍋撻幏閿嬬箾閺夋埈鍎忛柛鈺佹湰缁嬪绱掑Ο鑽ょ毣婵炴垶鎸哥粔鏉戯耿椤忓懏鍎熼柡鍐ㄦ祩閸ゅ鏌涜箛姘彧婵犙嶆嫹闁哄鏅滆摫婵″弶鎮傞獮搴綖椤撶偟妲ｉ梺鍝勭墳閹凤拷      1.婵炴垶鎸剧划顖氣攦閿熻棄銆掑顓犵畼闁告枮鍥ф嵍闁靛鍔庨悰鎾绘煕閺冨倸鏋旂�圭鍚嬮幏鍛村幢濡亶锕傛⒑椤愶絿鐓ǎ鍥э功閻ヮ亪骞愭惔銏㈠姺婵炲瓨绮屽Λ妤冭姳閺屻儱绀傞柧蹇ｅ亝缁拷    2.婵炴垶鎸剧划顖氣攦閿熺晫鐥褍鏋涢柣銈呭缁嬪顓奸崨顔垮惈婵炲瓨绮ｉ幏锟�,闂佸搫顦崕鎵箔婢舵劕鐭楀┑鐘叉祫閹峰嘲顫濇潏鈺佸绩婵炲濮寸�涒晛鈹冮敓浠嬫煏閸℃锟藉爼鏁撻挊澶婏拷鍫曟晸閺傘倖瀚�
        if(!ParaCycles.Bits.MasterInNetDetectOver)
        {
            sMasterInNetDetect();
        }
        else if(!ParaCycles.Bits.MasterCompetitionOver && !Flag_Local.Bits.MasterExist)
        {
            if(sMasterCompetition() == true)
            {
                sSendPwmTBCTR();
                sMasterModuleInit();
            }
            else
            {
                if(ParaCycles.Bits.MasterCompetitionOver)
                {
                    Flag_Local.Bits.CommCanFail = 1;
                }
            }
        }
        //婵炴垶鎸剧划顖氣攦閿熶粙鏌熼崫鍕垫Ш闁搞倕瀛╃粋宥夊幢濡櫣浠愰梺绋跨箺濞呮洖鈹冮敓钘壝瑰鎰,闁荤喐娲戠欢姘跺闯閹间礁妫橀柟娈垮枤瑜拌棄霉濠婂喚妲烽悗闈涙湰缁傛帡鏁撻敓锟�,婵炴垶鎸剧划顖氣攦閿熶粙鎮楀☉娅亜锕㈤鍫濈婵炲棗娴氬Σ璇裁归敐蹇斿
        else
        {
            ParaCycles.Bits.MasterInNetDetectOver = 0;
            ParaCycles.Bits.MasterCompetitionOver = 0;
        }
    }
}

/*******************************************************************************
Description:    when module powered on,it's be in sNewModule.if detect that master
                exist,send its UID to master,wait for master assign physical
                address ,to be slave if detect master lose,take master compete,
                if win,to be master,else wait for master assign physical address
                to be slave.
*******************************************************************************/
void sNewModule(void)
{
    sCANParsingForNewModule();

    if(Flag_Local.Bits.ParaMode != cNewModule)
        return;

    if(ParamsTimer - CpuTimer1.RegsAddr->TIM.all >= cTimer5ms)
    {
        ParamsTimer = CpuTimer1.RegsAddr->TIM.all;

        sUpdateStatus();

        if(!ParaCycles.Bits.MasterInNetDetectOver)
        {
            sMasterInNetDetect();
        }
        else if(!ParaCycles.Bits.MasterCompetitionOver && !Flag_Local.Bits.MasterExist)
        {
            if(sMasterCompetition() == true)
            {
                sSendPwmTBCTR();
                sMasterModuleInit();
            }
        }
        else
        {
            ParaCycles.Bits.MasterInNetDetectOver = 0;
            ParaCycles.Bits.MasterCompetitionOver = 0;
        }
    }
}


/*******************************************************************************
Discription:    闂佸搫娲ら悺銊╁蓟婵″細m闂佸憡绋忛崝宥咃耿閿燂拷     36k,5ms/200hz闂佸搫娲ら悺銊╁蓟婵犲啰鈻旈柨鐔绘閳诲酣鏁撻敓锟�, 婵炴垶鎼幏鐑芥煕韫囨岸鍝哄褏鏅幃浼存晸閿燂拷180濠电偛妫撮幏锟� . 婵炲濯Σ锟�=x/180
* CALLED BY:    ISR
*******************************************************************************/
#pragma CODE_SECTION(Para_GetPwmStep, ".TI.ramfunc");
void Para_GetPwmStep(void)
{
    static float StepResidueSum = 0;
    int16 Step;
    //闂佺儵鏅犻弲鑼礊閸涱垱鎯ラ柛娑卞幖椤ゅ巵,闁圭厧鐡ㄩ弻銊┞烽崘鈺佺窞鐟滃秹骞婇崱娑樺珘闁绘帪鎷�
    Step = PwmSyncCntl.yp[0];

    StepResidueSum += (PwmSyncCntl.yp[0] - (float) Step);

    if (StepResidueSum > 1)
    {
        Step = Step + 1;                    //闂佸搫鐗嗛ˇ閬嶎敄娓氾拷瀹曪綁鎮块婊咁吋缂備胶濮甸妵鐐电箔婢舵劖鍤勯柣锝呮湰閺嗭拷++ --
        StepResidueSum = StepResidueSum - 1;
    }
    else if (StepResidueSum < -1)
    {
        Step = Step - 1;
        StepResidueSum = StepResidueSum + 1.0f;
    }

    EPwmSyncRegs.TBPRD  = ShareData.TBPRD + Step;

    if(InvPhase.ThetaPointR == 1)
    {
        ParaCycles.Bits.CrossZeroUpdate = 1;
    }
}

//闁哄鍋犳慨銈囦焊濠靛瑙﹂悘鐐佃檸閸斿嫰鏌熺挩澶婂暙閻擄拷
void sDealPwmSync(void)
{
    #define uwSyncOKLimit    130     //130/2777 = 5%
    #define uwSyncLossLimit  400

    static Uint16 PwmSyncOkCnt = 0;
    static Uint16 PwmSyncLossCnt = 0;
            float TmpTBPRD = EPwmSyncRegs.TBPRD;
            float tmpError = 0;

    ParaCycles.Bits.CAN_Sync = 0;

    PwmSyncCntl.Ref = ShareData.TBCTR_local;
    PwmSyncCntl.Fbk = MasterData.TBCTR_master - ShareData.TBCTR_delay;     //CAN婵犮垹鐖㈤崨顓у敽闂佹寧绋戦悧濠勫垝閹间礁瑙﹂柨鐕傛嫹21.5us/闁烩剝甯掗幊搴㈡櫠閿燂拷34us闂佹寧鍐婚幏锟�
    tmpError = PwmSyncCntl.Ref - PwmSyncCntl.Fbk;

    if (tmpError > TmpTBPRD)
    {
        tmpError -= 2.0f * TmpTBPRD;
    }
    else if (tmpError < -TmpTBPRD)
    {
        tmpError += 2.0f * TmpTBPRD;
    }

//    PwmSyncCntl.xp[0] = tmpError;
    PwmSyncCntl.xp[0] = 0.97f * PwmSyncCntl.xp[0] + 0.03f * tmpError; //闂佽绨肩徊濠氾綖婢舵劕鏋佺憸鐗堝笒缁�鍡涙煙濞堝灝鏋熼柣锝呭船鑿愰柛銉到婢ф彃霉閻撳孩鍣烘繛鑹邦嚙閳藉顭ㄦ惔銏℃澑濠电偠鎻徊浠嬫煀閿濆鏅搁柤鎭掑劤椤ｆ煡鏌熼煬鎻掞拷鏇㈠煝閺冨牞缍栨い鏂垮⒔椤︻噣姊洪幖鐐插姢濠殿喖纾幏褰掓偄閸涘﹤顫￠梺鍏间航閸庨亶鎯冮幆顬ュ酣宕堕敐蹇斿闁告劖褰冮埢蹇涙倵閻熸澘顥忛柛鐘虫礈缁厽寰勭�ｎ亶鍤ゅ┑顔筋焾娴滎剙鐣烽幇鐗堢厱婵炲棙鐟︾�氾拷
    PI_Controller(&PwmSyncCntl);

    //ErrorObserv( tmpError);

    // PwmSync ok judgement
    if(fabsf(tmpError) <= uwSyncOKLimit) //0.5
    {
        if(++PwmSyncOkCnt > 15)
        {
            PwmSyncOkCnt = 15;
            PwmSyncLossCnt = 0;
            Flag_Local.Bits.PwmSyncOk = 1;
        }
    }
    else if(fabsf(tmpError) > uwSyncLossLimit)
    {
        PwmSyncOkCnt = 0;
        if(++PwmSyncLossCnt > 2)   //濠电姴顭堥崐鏍焊閿燂拷
        {
            PwmSyncLossCnt = 2;
            Flag_Local.Bits.PwmSyncOk = 0;
        }
    }
}


void sDealPhaseSync(void)
{
    #define uwPhaseOKLimit    35  //+-2婵炴垶鎼╂禍鐐哄磻閿燂拷 = 3.6鎺�
    #define uwPhaseLossLimit  80  //+-5婵炴垶鎼╂禍鐐哄磻閿燂拷 = 9鎺�

    static Uint16 PhaseSyncOkCnt = 0;
    float tmpError = 0;
//    static Uint16 PhaseSyncLossCnt = 0;
    ParaCycles.Bits.CAN_Spll = 0;

    if(1)
    {
        PreSyncCntl.Ref = ShareData.theta_local;
        PreSyncCntl.Fbk = MasterData.theta_master + ShareData.theta_delay; //can瀵ゆ儼绻滅悰銉ヤ缉 (16.6ms/4096)
        tmpError = PreSyncCntl.Ref - PreSyncCntl.Fbk;
    }

    //(-2048 閸掞拷 2048)娴狅綀銆� -PI 閸掞拷 PI
    if (tmpError > 2048.0f)  //2048 is optimum
    {
        tmpError -= 4096.0f;
    }
    else if (tmpError < -2048.0f)
    {
        tmpError += 4096.0f;
    }

//    if(Flag_Local.Bits.PhaseLocked)
//    {
////        PreSyncCntl.xp[0] = 0.9f * PreSyncCntl.xp[0] + 0.1f * tmpError;
//    }
//    else
//    {
        PreSyncCntl.xp[0] = tmpError;
//    }

    //楠炲墎缍夐崥灞绢劄娑撳秴浼愭担锟�

    if(InvStateData.WorkState == ByPass
            || InvStateData.WorkState == OnGridState)
//            Status.bits.spllRun)// || Status.bits.AtsSyncMode
    {
        PreSyncCntl.xp[1] = 0;
        PreSyncCntl.yp[0] = 0;
        PreSyncCntl.yp[1] = 0;
    }
    else
    {
         PI_Controller(&PreSyncCntl);
    }

    ShareData.theta_Adj = -PreSyncCntl.yp[0];

//    ErrorObserv( PreSyncCntl.xp[0]);

    // phaselock ok judgement
    if(fabsf(PreSyncCntl.xp[0])<= uwPhaseOKLimit && fabsf(PreSyncCntl.yp[0]) < PreSyncCntl.max)
    {
        if(++PhaseSyncOkCnt > 10)
        {
            PhaseSyncOkCnt = 10;
            Flag_Local.Bits.PhaseLocked = 1;
        }
    }
    else if(fabsf(PreSyncCntl.xp[0]) > uwPhaseLossLimit || fabsf(PreSyncCntl.yp[0]) == PreSyncCntl.max)
    {
        Flag_Local.Bits.PhaseLocked = 0;
        PhaseSyncOkCnt = 0;
    }
}

//Uint16 sGetCommCanFail(void)
//{
//    return Flag_Local.Bits.CommCanFail;
//}

Uint16 sGetModuleType(void)
{
    return Flag_Local.Bits.ParaMode;
}

void sSetModuleType(Uint16 type)
{
    Flag_Local.Bits.ParaMode = type;
}

/**********************************************************************
* PURPOSE :  EMS娴ｈ儻鍏橀獮鑸垫簚缁崵绮�
* CALLED BY:    master
**********************************************************************/
bool sEnableParallel(void)
{
    return ParaEvent.Bits.enParallel;
}

/**********************************************************************
* PURPOSE :  EMS閸掓繂顫愰崠鏍ц嫙閺堣櫣閮寸紒锟� 闁板秶鐤嗛獮鑸垫簚閸欑増鏆� 闁板秶鐤嗙粋鑽ょ秹閻㈤潧甯囬妴渚�顣堕悳锟�
* CALLED BY:    master
**********************************************************************/
void Para_StartSystem(void)
{
    Uint16 SystemSwitch = 0;
    Uint16 WorkingStateNew = 0;
static Uint16 WorkingStateOld = 0;
    Uint16 BatFaultState = 0;

    if(ArmWriteData.Fault.bit.ATEMode) return;
    //1.get arm data
    BatFaultState = (ArmWriteData.BatState == 0x33);
    SystemSwitch = SystemPara.SystemPowerOn;
    WorkingStateNew =( (InvStateData.WorkState == OffGridState)
                    ||(InvStateData.WorkState == UpsState)
                    ||(InvStateData.WorkState == OnGridState));
    if(Flag_Local.Bits.AddressingOk||(ShareData.RealNumber>1))
    {
        Flag_Local.Bits.enParallel = 1;
        ShareData.RealNumber = 2;
    }
    else
        Flag_Local.Bits.enParallel = (SystemPara.ParallelEn == 1);

    Flag_Local.Bits.freq = (SystemPara.FrequencySet == 1);  //频率设置
//    Flag_Local.Bits.volt = (SystemPara.InvVoltySet == 1); //电压设置


    //2.fault
    //如果并机使能，台数应大于1
    //如果并机未使能，台数应等于1
    if((Flag_Local.Bits.enParallel == 1 &&(Flag_Local.Bits.ParaMode == cNewModule ))
    || (Flag_Local.Bits.enParallel == 1 && Flag_Local.Bits.ParaMode == cMasterModule && ShareData.RealNumber <= 1 )
    || (Flag_Local.Bits.enParallel == 0 && Flag_Local.Bits.ParaMode == cSlaveModule)
    || (Flag_Local.Bits.enParallel == 0 && Flag_Local.Bits.AddressingOk)
    || (ShareData.RealNumber > cParaMaxNum))
    {
        Flag_Local.Bits.AppCfgFault = 1;
    }
    else
    {
        Flag_Local.Bits.AppCfgFault = 0;
    }

    if(SystemSwitch)
    {
        if( Flag_Local.Bits.AppCfgFault || Flag_Local.Bits.Open)
        {
            SystemError.Fault4.bit.ParaSet = 1;
        }
        if( Flag_Local.Bits.CommCanFail && Flag_Local.Bits.enParallel)
        {
            if(Flag_Local.Bits.ParaMode == cMasterModule && WorkingStateNew)
            {
               //主机运行时不报通信故障
            }
            else
            {
                SystemError.Fault4.bit.ParaCommu = 1;
            }
        }
    }

    //spll fault
    if(Flag_Local.Bits.ParaMode == cSlaveModule)
    {
        if(Flag_Local.Bits.PhaseLocked == 0 && (Flag_Local.Bits.freq != Flag_para[0].Bits.freq ))
        {
            if(SystemSwitch)
            {
//                SystemError.Fault4.bit.ParaSet = 1;
            }
        }
    }

    //
    //fault clear
    if( Flag_Local.Bits.AppCfgFault == 0 && Flag_Local.Bits.Open == 0)
    {
        SystemError.Fault4.bit.ParaSet = 0;
    }

    if( Flag_Local.Bits.CommCanFail == 0 || Flag_Local.Bits.enParallel == 0)
    {
        SystemError.Fault4.bit.ParaCommu = 0;
    }

    //3.主机停机/旁路，需让出主机
    if(WorkingStateNew == 0 && Flag_Local.Bits.ParaMode == cMasterModule)
    {
        if(WorkingStateOld)
        {
            ParaEvent.Bits.MasterFault = 1;
        }

        if(Para_CheckInvRly() || CheckGridRly())
        {

        }

//        if(Flag_para.Bits.OffGridWorking == 1)
//        {
//            ParaEvent.Bits.MasterFault = 1;
//        }
//        if(SystemSwitch && SciData.BatterySocAvg == 0)
//        {
//            ParaEvent.Bits.MasterFault = 1;
//        }
    }
    WorkingStateOld = WorkingStateNew;

    //4.下电通信关闭 ||如果有不可恢复故障，则彻底退出并机
//    if(BatFaultState)
//    {
//        if(SampleIns.BUSVoltIns < 2500)
//        {
//            Flag_Local.Bits.ComTurnOff = 1;
//        }
//    }
//    else
//    {
//        Flag_Local.Bits.ComTurnOff = 0;
//    }

    //
//    if(SciData.SystemSwitchOn == 0)
//    {
//        ParaEvent.Bits.MasterFault = 1;
//    }

    //5.能自检的优先自检，需统一的由主机检测 //&& Flag_Local.Bits.AddressingOk
    if(Flag_Local.Bits.enParallel && Flag_Local.Bits.MoudleFault == 0 && Flag_Local.Bits.Open == 0 && SystemSwitch)
    {
        if(SystemError.StateErr.bit.INVFault == 0)
        {
            Flag_Local.Bits.SelfCheck = 1; //设置正常
        }
//        if( SampleDataStruct.PN_BUS_VOLT > 3700)
//        {
//            if( (++NewModule_PN_BUS_VOLTLowCnt > 30000) && ((Inverter_Error.all & 0x7FFF) == 0) && ((Inverter_Error2.all & 0xB000) == 0) && ((Inverter_Error3.all & 0x0D81) == 0) )
//            {
//                NewModule_PN_BUS_VOLTLowCnt = 0;
                Flag_Local.Bits.SelfCheck = 1; //设置正常
//            }
//        }
//        else
//        {
////            NewModule_PN_BUS_VOLTLowCnt = 0;
//        }
    }
    else
    {
        Flag_Local.Bits.SelfCheck = 0; //设置异常
    }

}
/**********************************************************************
* PURPOSE :  楠炶埖婧�閺佸懘娈伴妴浣圭垼韫囷拷
* CALLED BY: all
**********************************************************************/
void sUpdateStatus(void)
{
//    SYSTEM_FAULT_BITS ErrorTableMst = {0xffff};  //鏉╁洦鎶ら獮鑸垫簚閺佸懘娈�
//    ErrorTableMst.bit.paraFault = 0;
//
//    //娴ｈ儻鍏樼拋鍓х枂+閸欑増鏆熺拋鍓х枂鐎瑰本鍨氶敍灞炬弓鏉╂稑鍙嗛獮鑸垫簚缁崵绮�
//    //閺堫亣顔曠純顔艰嫙閺堣桨濞囬懗鏂ょ礉鏉╂稑鍙嗛獮鑸垫簚缁崵绮�
//    if( (sEnableParallel() && sGetModuleType() == cNewModule && Flag_Local.Bits.SelfCheck)
//       ||(sEnableParallel() == 0 && sGetModuleType() != cNewModule ))
//    {
//        Flag_Local.Bits.AppCfgFault = 1;
//    }
//    else
//    {
//        Flag_Local.Bits.AppCfgFault = 0;
//    }
//
//    if((ErrorTable.all & ErrorTableMst.all) || AlarmTable.all >= 0X0020)
//    {
//        Flag_Local.Bits.MoudleFault = 1;
//    }
//    else
//    {
//        Flag_Local.Bits.MoudleFault = 0;
//    }
//
//    if(sEnableParallel())
//    {
//        //婢х偛濮炲銈呮啞鐠�锟�,闁矮淇婃稉銏犮亼閻ㄥ嫭婧�閸ｃ劌鍙ч張锟�
//        if(Flag_Local.Bits.CommCanFail)
//            AlarmTable.bit.ParaCanLoss = 1;
//        //娑擄拷閸欑増鏅犻梾婊愮礉娑撳秴鍘戠拋绋垮従鐎瑰啫褰村锟介張锟�
//        AlarmTable.bit.MoudleFault = sCheckMoudleFault();
//    }
//    else
//    {
//        AlarmTable.bit.ParaCanLoss = 0;
//        AlarmTable.bit.MoudleFault = 0;
//    }
//
//    //閺佸懘娈板Ч鍥拷锟�
//    ErrorTable.bit.paraFault = Flag_Local.Bits.AppCfgFault
//                            || Flag_Local.Bits.TotalNumFault
//                            || Flag_Local.Bits.Open
//                            || AlarmTable.bit.ParaCanLoss   //鐟欙箑褰傞崑婊勬簚
//                            || AlarmTable.bit.MoudleFault;  //鐟欙箑褰傞崑婊勬簚
//
//
//    //
//    Flag_Local.Bits.freq = (Input.SystemFreq == 50);
//    Flag_Local.Bits.volt = (Input.SystemVolt == 100);
}

/**********************************************************************
* PURPOSE :
* CALLED BY:
**********************************************************************/
char sCheckMoudleFault(void)
{
    Uint16 uwCheckId;

    for(uwCheckId = 0; uwCheckId < cParaMaxNum; uwCheckId++)
    {
        if(Flag_para[uwCheckId].Bits.MoudleFault)
        {
            return 1;
        }
    }
    return 0;
}

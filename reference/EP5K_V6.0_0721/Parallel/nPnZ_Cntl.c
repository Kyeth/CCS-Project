/*
 * Controller.c
 *
 *  Created on: 2022婵°倗濮烽崕鎴﹀箯閿燂拷6闂備礁鎼悧鐐哄箯閿燂拷30闂備礁鎼崰娑㈠箯閿燂拷
 *      Author: DLadmin
 */
#include "F28x_Project.h"
#include "nPnZ_Cntl.h"
#include "string.h"
//----------------------------pi------------------------------------------//
#pragma CODE_SECTION(PI_Controller, ".TI.ramfunc");
void PI_Controller(PIDCntl_F *v)
{
    if(v->satflag == 0)                                   //闂傚鍋勫ù鍌炲磻婵犲洦鏅搁柣锝呰嫰閺嬪酣鏌熸總鍛婃暠闁诡垱妫冨顕�鏁撻悾灞炬殰鐟滅増甯楅崵鎺楁煙闁缚绨婚柍璇茬箻閺岋綁鍩�椤掞拷婵″吋銇勯妶鍛殻闁哄苯鐭佺粻娑㈠箻閸撲緡鏀�
    {
        v->yp[1] = v->yp[0];                              //濠电姭鎷冮崨顓濇闂侀潻绲介幖顐︻敋閿濆鏅搁柨鐕傛嫹,缂傚倸鍊搁崐钘夆枍閿濆纾婚柨婵嗩槸閸欏﹥銇勯弽顐㈠壋闁归鍏橀弫鎾绘嚍閵夘喗顔囬梺鍛婎殘閸嬬偟锟芥碍鐓￠悡顐﹀炊鐠鸿桨澹曠紓浣介哺缁诲牆顕ｉ锟介幃鈺佺暦閸ャ儻鎷锋繝鍕＝濞达絽鎽滈幗鐘绘倵閸偅鐓ユい顓滃姂閸ㄩ箖宕橀懠顒傚炊缂傚倷绀侀惉濂稿磿閵堝鍎婃い鏇嫹闁轰礁绉堕敓鑺ョ〒閸ㄦ牠姊洪棃娑欘棛闁汇垻甯甸梻鍌欑贰閸犳牠鏁撻悾灞芥瀻闁告ɑ鎸冲娲敃椤掍礁顏跺┑掳鍊栭〃鍛暦椤掑嫬绠柨鐕傛嫹 闂備焦瀵ч崘濠氬箯閿燂拷
    }
    else
    {
        v->yp[1] = v->yp[1] + v->Ki * v->xp[0];           //濠电偠鎻徊鐣岀矓閸洘鍋柛鈩冾殢閸ゆ洟鐓崶銉ュ姢缂侊拷婢跺瞼纾介柛灞剧箖椤忕娀鏌涚�ｅ墎绉�殿喕鍗抽、娑㈡倻閸ャ劌顏堕梺璺ㄥ枙婵倝銆冨鍫濈闁绘劖娼欓顖炴⒑瑜版帞銆屾繝顫兌濡叉劕鈹戦崶鈺婃祫闂佸憡鐗▍娆撴⒑閻撳骸甯犵紒顔界懇閺佹捇寮妶鍡楊伓濠德板�栭〃鍛暦椤掑嫬绠柨鐕傛嫹
        v->yp[1] = __fsat(v->yp[1], v->Imax, -v->Imin);
    }
    v->yp[0] = v->yp[1] + v->Kp * (v->xp[0] - v->xp[1]) + v->Ki * v->xp[0];
    v->xp[1] = v->xp[0];
    v->yp[0] = __fsat(v->yp[0], v->max, v->min);
}

void Init_PI_Controller(PIDCntl_F *v ,float kp , float ki , float max , float min)
{
    v->Ref = 0;
    v->Kp = kp;
    v->Ki = ki;
    v->max = max;
    v->min = min;
    v->xp[0] = 0;
    v->xp[1] = 0;
    v->yp[0] = 0;
    v->yp[1] = 0;
    v->satflag = 0;
}

//----------------------------pr------------------------------------------//
// PI =Kp+ (2*s*wrc)/(s^2+2*s*wrc+wo^2)
void Init_PRCoeff( CNTL_2P2Z_F_COEFFS *coef1, float kp, float ki, float wo, float fs, float wrc )
{
    float temp1, wo_adjusted;

 //   wo_adjusted=2*fs*tanf(wo/(2*fs));

    wo_adjusted = wo;
    temp1=1/(4*fs*fs+wo_adjusted*wo_adjusted+4*fs*wrc);
    coef1->Coeff_TempB0=4*wrc*fs * temp1;

    coef1->Coeff_B0= ki * coef1->Coeff_TempB0;
    coef1->Coeff_B1= 0;
    coef1->Coeff_B2=-coef1->Coeff_B0;
    coef1->Coeff_A1=-(-8*fs*fs+2*wo_adjusted*wo_adjusted) * temp1;
    coef1->Coeff_A2=-(1 - (8*fs*wrc) * temp1);

    coef1->Coeff_B0+=kp;
    coef1->Coeff_B1-=kp*coef1->Coeff_A1;
    coef1->Coeff_B2-=kp*coef1->Coeff_A2;
}

void Init_PRVars(CNTL_2P2Z_F_VARS *k, float max , float min )
{
    // Initialize variables
    k->Ref = 0;
    k->Fdbk = 0;
    k->Errn = 0;
    k->Errn1 = 0;
    k->Errn2 = 0;
    k->Out = 0;
    k->Out1 = 0;
    k->Out2 = 0;
    k->Max = max;
    k->Min = min;

}

//*********** Function Definition ********//
// Calculates a second order control law with IIR filter and programmable output saturation.
// @param CNTL_2P2Z_F_Coeffs structure with proper coefficient values.
// @param CNTL_2P2Z_F_Vars structure with internal & output values.
// @return CNTL_2P2Z_F_Vars Out parameter.
#pragma CODE_SECTION(CNTL_2P2Z_F_FUNC, ".TI.ramfunc");
void CNTL_2P2Z_F_FUNC(CNTL_2P2Z_F_COEFFS *v, CNTL_2P2Z_F_VARS *k){
    /* Calculate error */
    k->Out = (v->Coeff_A2 *k->Out2)  + (v->Coeff_A1 * k->Out1)
            +(v->Coeff_B2 *k->Errn2) + (v->Coeff_B1 * k->Errn1) + (v->Coeff_B0 * k->Errn);

    /* Update error values */
    k->Errn2 = k->Errn1;
    k->Errn1 = k->Errn;

    /* Saturated output */
//    k->Out = (k->Out < k->Max) ? k->Out : k->Max;
//    k->Out = (k->Out > k->Min) ? k->Out : k->Min;
    k->Out = __fsat(k->Out, k->Max, k->Min);

    /* Store outputs */
    k->Out2 = k->Out1;
    k->Out1 = k->Out;

}

//----------------------------闂傚倷鐒﹁ぐ鍐矓閹绢啟鍥晸閿燂拷------------------------------------------//
#pragma CODE_SECTION(Rept_Controller, ".TI.ramfunc");
void Rept_Controller(REPTCntl_F *v)
{
    if (++v->OutPt >= v->Prd)
        v->OutPt = 0;

    v->ErrPt = v->OutPt - v->Tc;

    if (v->ErrPt < 0)
        v->ErrPt = v->ErrPt + v->Prd;

    v->ErrArray[v->ErrPt] = 0.99 * v->ErrArray[v->ErrPt] + 0.01 * v->Err;

    v->Out = __fsat(v->Kr * v->ErrArray[v->OutPt], v->Max, v->Min);
}

void Init_Rept_Controller(REPTCntl_F *v,float Kr ,int Tc,float Max,float Min)
{
    memset((void*)&(v->ErrArray),0,sizeof(v->ErrArray));

    v->Out = 0;
    v->Kr = Kr;
    v->Tc = Tc;
    v->Max = Max;
    v->Min = Min;
}

//婵犳鍠氶幊鎾诲磹缁嬫鐒介幖娣妼濡﹢鎮峰▎蹇擃伀濠㈢櫢鎷�//闂佽绻愮换瀣归崶顑﹀绻濋崶褏顓洪梺褰掑亰閸樿棄鈻嶆繝鍥ㄢ拻闁告侗鍠氶惌瀣亜閵夈儲鍣归棁澶愭倵閿濆簼绨绘慨锝嗗姍閺屸剝绗熸繝鍐伓
#define ERR_BUFF_SIZE  500

       float errRMS = 0;
static float errorbuffer[ERR_BUFF_SIZE];
static float errsum = 0;
static Uint16 errCnt;

//#pragma CODE_SECTION(ErrorObserv, ".TI.ramfunc");
float ErrorObserv(float error)
{
    errsum += error*error;
    errCnt++;
    if(errCnt>(ERR_BUFF_SIZE - 1))
    {
        errRMS = __sqrt(errsum/(float)ERR_BUFF_SIZE);
        errsum = 0;
        errCnt=0;
    }
    errorbuffer[errCnt] = error;

    return errRMS;
}


 float nPnZ_max[10];
 void findMax(Uint16 addr,float val)
{
     switch(addr)
     {
         case 0:  if(val > nPnZ_max[0])nPnZ_max[0] = val; break;
         case 1:  if(val > nPnZ_max[1])nPnZ_max[1] = val; break;
         case 2:  if(val > nPnZ_max[2])nPnZ_max[2] = val; break;
         case 3:  if(val > nPnZ_max[3])nPnZ_max[3] = val; break;
         case 4:  if(val > nPnZ_max[4])nPnZ_max[4] = val; break;
         case 5:  if(val > nPnZ_max[5])nPnZ_max[5] = val; break;
         case 6:  if(val > nPnZ_max[6])nPnZ_max[6] = val; break;
         case 7:  if(val > nPnZ_max[7])nPnZ_max[7] = val; break;
         case 8:  if(val > nPnZ_max[8])nPnZ_max[8] = val; break;
         case 9:  if(val > nPnZ_max[9])nPnZ_max[9] = val; break;
         default: break;
     }
}


 void resetMax(Uint16 addr)
{
    if(addr>=10)return;
    nPnZ_max[addr] = 0;
}

/*
 * Energy_Manage.c
 *
 *  Created on: 2022年5月9日
 *      Author: DLadmin
 */
#include"Energy_Manage.h"
#include"Sample.h"
#include "CalculateAngle.h"
#include "F2837xS_device.h"
#include "Protect.h"
#include "User_Database.h"
#include "User_I2C.h"
#include "bsp.h"
#include "ConfigSystemPara.h"

extern char InvOnGridCnt5SChk(void);
extern char GetINVStation(void);
extern ADC_SAMPLE_RMS SampleRms;
PLIMIT_STRUCT Plimit;
ESMODE_STRUCT ESmode;
CPMODE_STRUCT CPmode;
CQMODE_STRUCT CQmode;
PFMODE_STRUCT PFmode;
PUMODE_STRUCT PUmode;
QUMODE_STRUCT QUmode;
QPMODE_STRUCT QPmode;
COSPHIMODE_STRUCT QCosphimode;
ENERGYMANAGE_STRUCT EnergyManage;
COSPHIMODE_STRUCT Cosphimode;
GRIDCHECK_STRUCT GridCheck;
Uint16 EMinitEN=0;

//extern CAPTURE_DATA_TEST CaptureDataTest;

void InitESmode(void);
void InitPUmode(void);
void InitPFmode(void);
void InitQUmode(void);
void InitQPmode(void);
void InitCQmode(void);
void InitCosphimode(void);

float sESmode(void);
float sPUmode(void);
float sPFmode(void);
float sQUmode(void);
float sQPmode(void);
float sCosphiMode(void);
char sESmodeCheck(void);

void GridPreConnectCheck(void);
void GridVoltCheck(void);
void GridFreqCheck(void);

float PQtcurveSet(char *Value,float Tr,float Ts,unsigned int* MsCnt);

unsigned char OverLevelChk(float uwCompareData, float uwHighLever, unsigned char ubFilter, unsigned char *pubCounter);
unsigned char UnderLevelChk(float uwCompareData, float uwLowLever,  unsigned char ubFilter,  unsigned char *pubCounter);
float Calc_lineK(float x1,float y1,float x2,float y2);

union Alarm_REG INV_Status;

Uint16 EMData_WriteFirstFlag=0;
Uint16 EMData_shadow[EMDataNum];
/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitEnergyManage(void)
{
    User_EME2ROMData_Read();
    ESmode.ES_PERMIT_SERVICE_EN=1; //允许并网
    if(1)//(EMData_shadow[EMDataNum-1]!=0x55)        //第一次上电，数据使用默认值
    {
        EnergyManage.Vrated = 101.0f;
        EnergyManage.Frated = 50.0f;
        EnergyManage.Prated = 2750.0f;
        EnergyManage.Srated = 2750.0f;
        EnergyManage.P1Command = 2750;
        EnergyManage.Q1Command = 0;
        EnergyManage.P2Command = 2750;
        EnergyManage.Q2Command = 0;
        EnergyManage.P_Tr=5.0;  //5.0s
        EnergyManage.Q_Tr=0.01; //0.01s
        EnergyManage.P1out = 0;
        EnergyManage.Q1out = 0;
        EnergyManage.P2out = 0;
        EnergyManage.Q2out = 0;
        EnergyManage.Enable.all = 0;
        EnergyManage.PLimitCmd = 0.5*EnergyManage.Prated;
        InitESmode();
        InitPFmode();  //频率有功
        InitPUmode();  //电压功率
        InitQUmode();  //电压无功
        InitQPmode();  //有功无功
        InitCQmode();  //恒定无功
        InitCosphimode();   //恒定功率因数
        EMinitEN=0x55;
        EMData_WriteFirstFlag=1;
    }
    else //使用记录在EEPROM中的历史参数
    {
        EMdatainit();
    }

}

/**********************************************************************
* FUNCION :
* PURPOSE :计算PQ的参考值
* CALLED BY:并网任务中执行 10ms
**********************************************************************/

float V1_Avg=0.0,V2_Avg=0.0;
float F1_Avg=60.0,F2_Avg=60.0;
float SOC=0.0;

void FIR_V(float Value1,float Value2, int N)
{
    static float  V1[10]={0,0,0,0,0,0,0,0,0,0};
    static float  V2[10]={0,0,0,0,0,0,0,0,0,0};
    float Sum1=0,Sum2=0;
    int i=0;

    if(N>10) N=10;

    for(i=N-1;i>0;i--)
    {
        V1[i]=V1[i-1];
        V2[i]=V2[i-1];
    }

    V1[0]=Value1;
    V2[0]=Value2;

    for(i=0;i<N;i++)
    {
        Sum1+=V1[i];
        Sum2+=V2[i];
    }

    V1_Avg=(Sum1/N);//+0.4;
    V2_Avg=(Sum2/N);//+0.1;
}

void FIR_F(float Value1,float Value2, int N)
{
    static float  V1[10]={0,0,0,0,0,0,0,0,0,0};
    static float  V2[10]={0,0,0,0,0,0,0,0,0,0};
    float Sum1=0,Sum2=0;
    int i=0;

    if(N>10) N=10;

    for(i=N-1;i>0;i--)
    {
        V1[i]=V1[i-1];
        V2[i]=V2[i-1];
    }

    V1[0]=Value1;
    V2[0]=Value2;

    for(i=0;i<N;i++)
    {
        Sum1+=V1[i];
        Sum2+=V2[i];
    }

    F1_Avg=(Sum1/N);
    F2_Avg=(Sum2/N);
}

void sEnergyManage(void)
{
    float P1tmp,P2tmp,Q1tmp,Q2tmp,P_Tr, Q_Tr,Delta_F,Delta_V;
//    static float Pre_P1tmp=0.0,Pre_P2tmp=0.0,Pre_Q1tmp=0.0,Pre_Q2tmp=0.0,Delta_Q1=0.0,Delta_Q2=0.0,Q1_Set=0.0,Q2_Set=0.0,Q1_Use=0.0,Q2_Use=0.0;
//    static float Delta_P1=0.0,Delta_P2=0.0,P1_Set=0.0,P2_Set=0.0,P1_Use=0.0,P2_Use=0.0;
//    static char P1_Reset=0,Q1_Reset=0,P2_Reset=0,Q2_Reset=0;
//    static unsigned int Q1_MsCnt=0,P1_MsCnt=0,Q2_MsCnt=0,P2_MsCnt=0;

    FIR_V(SampleRms.Grid1VoltRms,SampleRms.Grid2VoltRms,5);
    FIR_F(CaptureData.FreqR,CaptureData.FreqS,5);

    EnergyManage.V1 = SampleRms.Grid1VoltRms;
    EnergyManage.V2 = SampleRms.Grid2VoltRms;
    EnergyManage.f1=  CaptureData.FreqR;
    EnergyManage.f2=  CaptureData.FreqS;
    P_Tr=EnergyManage.P_Tr;
    Q_Tr=EnergyManage.Q_Tr;
	
    EnergyManage.P1Command=SystemPara.ActivePowerSetR;
	EnergyManage.P2Command=SystemPara.ActivePowerSetS;

    P1tmp= __fsat(EnergyManage.P1Command,4500,-4500);    //功率输入设定
    Q1tmp= __fsat(EnergyManage.Q1Command,1980,-1980);    //功率输入设定
    P2tmp= __fsat(EnergyManage.P2Command,4500,-4500);    //功率输入设定
    Q2tmp= __fsat(EnergyManage.Q2Command,1980,-1980);    //功率输入设定

    EMmodeENcheck();    //检测是否满足并网条件（未并网前，需开启并网使能，并且满足电压频率要求，并网后只需满足并网使能要求）

    if((ESmode.ES_EN==0)||(ESmode.ES_STATUS<ES_RAMP))   //没有切到并网状态时
    {
        ESmode.P=0;
        ESmode.SlopStep=0;
        EnergyManage.P1out=0;
        EnergyManage.P2out=0;
        EnergyManage.Q1out=0;
        EnergyManage.Q2out=0;
        P1tmp=0;
        P2tmp=0;
        Q1tmp=0;
        Q2tmp=0;
        return;
    }
    else if(ESmode.ES_STATUS!=ES_FINISH)
    {
        ESmode.P_SET=P1tmp;
        P1tmp=sESmode();
        P2tmp=P1tmp;
    }
    else
    {
        ESmode.ES_CNT=0;
    }

    //1.计算P 上位机指令/启动/PU/PF/限制指令/降额
    if(EnergyManage.Enable.bit.ActivePowerLimit)
    {
        P1tmp = __fsat(EnergyManage.PLimitCmd,4500,-4500);
        P2tmp = __fsat(EnergyManage.PLimitCmd,4500,-4500);
    }

    if(EnergyManage.Enable.bit.PFmode)  //频率支撑 //频率有功5.15
    {
        Delta_F=fabs(PFmode.F1pre-F1_Avg);
        if((Delta_F>=0.08)||(PFmode.P1pre!=P1tmp)) //P目标发生变化，或者实际频率发生变化。
        {
            if(PFmode.cnt1++>3)
            {
                PFmode.cnt1=0;
                PFmode.P1pre=P1tmp;
                PFmode.F1pre=F1_Avg;
                PFmode.Pref=P1tmp;
                PFmode.F=F2_Avg;
                PFmode.P1out= sPFmode();
            }
        }
        else
        {
            PFmode.cnt1=0;
        }

        P1tmp=__fsat(PFmode.P1out,4500,-4500);

        Delta_F=fabs(PFmode.F2pre-F2_Avg);
        if((Delta_F>=0.08)||(PFmode.P2pre!=P2tmp))
        {
            if(PFmode.cnt2++>3)
            {
                PFmode.cnt2=0;
                PFmode.P2pre=P2tmp;
                PFmode.F2pre=F2_Avg;
                PFmode.Pref=P2tmp;
                PFmode.F=F2_Avg;
                PFmode.P2out= sPFmode();
            }
        }
        else
        {
            PFmode.cnt2=0;
        }

        P2tmp=__fsat(PFmode.P2out,4500,-4500);

        P_Tr=__fmax(PFmode.Tr,P_Tr);
    }

    if(EnergyManage.Enable.bit.VrefAdj)     //电压无功 5.14.5
    {
        QUmode.Lpf_A1 = QUmode.Lpf_Ts/QUmode.Lpf_Tc;
        QUmode.Lpf_A0 = 1 - QUmode.Lpf_A1;

        QUmode.V=EnergyManage.V1;
        QUmode.Vref = QUmode.Lpf_A0*QUmode.Vref + QUmode.Lpf_A1*QUmode.V;
    }

    if(EnergyManage.Enable.bit.CQmode)          //恒定无功模式5.14.8  //
    {
        Q1tmp = CQmode.Q1;
        Q2tmp = CQmode.Q2;
    }
    else if(EnergyManage.Enable.bit.QUmode)     //电压无功模式5.14.4 5.14.6
    {
        Delta_V=fabs(V1_Avg-QUmode.V1_Pre);

        if(Delta_V>=0.5)   //电压变化值大于0.8v，认为目标改变，开始重新调整目标功率
        {
            if(QUmode.cnt1++>2)
            {
                QUmode.V=V1_Avg;
                QUmode.Q1_out= sQUmode();          //计算需要输出的Q值
                PUmode.V = V1_Avg;
                PUmode.P1out = sPUmode();
                QUmode.V1_Pre=V1_Avg;
            }
        }
        else
        {
            QUmode.cnt1=0;
        }
        Delta_V=fabs(V2_Avg-QUmode.V2_Pre);  //电压变化值小于0.8v，认为到达稳定电压点，开始调整电压

        if(Delta_V>=0.5)   //电压变化值大于0.8v，认为目标改变，开始重新调整目标功率
        {
            if(QUmode.cnt2++>2)
            {
                QUmode.V=V2_Avg;
                QUmode.Q2_out= sQUmode();          //计算需要输出的Q值
                PUmode.V = V2_Avg;
                PUmode.P2out = sPUmode();
                QUmode.V2_Pre=V2_Avg;
            }
        }
        else
        {
            QUmode.cnt2=0;
        }
        if(V1_Avg>QUmode.V5)
        {
           QUmode.limitQ1=QUmode.Q4 -QUmode.Q4*(V1_Avg-QUmode.V5)/(QUmode.V6-QUmode.V5);
        }
        else
        {
        	QUmode.limitQ1=QUmode.Q4;
        }
        Q1tmp=__fmax(QUmode.Q1_out,QUmode.limitQ1);
		if(Q1tmp<-500)
		{
		   Q1tmp=Q1tmp-300;//-200抵消孤岛扰动无功 ，L1相偏小100，补偿-100
		}
		else
		{
		   Q1tmp=Q1tmp-200;//-200抵消孤岛扰动无功
		}
        if(V2_Avg>QUmode.V5)
        {
           QUmode.limitQ2=QUmode.Q4 -QUmode.Q4*(V2_Avg-QUmode.V5)/(QUmode.V6-QUmode.V5);
        }
        else
        {
           QUmode.limitQ2=QUmode.Q4;
        }
        Q2tmp=__fmax(QUmode.Q2_out,QUmode.limitQ2);//负数取大的
        Q2tmp=Q2tmp-200;//-200抵消孤岛扰动无功

        Q_Tr=QUmode.Tr;

	    P1tmp=__fmin(PUmode.P1out,P1tmp);
	    P2tmp=__fmin(PUmode.P2out,P2tmp);
	    P_Tr=__fmax(PUmode.Tr,P_Tr);
    }
    else if(EnergyManage.Enable.bit.QPmode)     //有功无功模式5.14.7
    {
        if(P1tmp>4050)
        {
            P1tmp=4050;
        }
        else if(P1tmp<-4050)
        {
            P1tmp=-4050;
        }
        else
        {

        }

        QPmode.P = P1tmp;
        Q1tmp= sQPmode();

        if(P2tmp>4050)
        {
            P2tmp=4050;
        }
        else if(P2tmp<-4050)
        {
            P2tmp=-4050;
        }
        else
        {

        }
        QPmode.P = P2tmp;
        Q2tmp= sQPmode();
    }
    else if(EnergyManage.Enable.bit.CosphiMode) //恒定功率因数模式5.14.3
    {
          Cosphimode.P = P1tmp;//__fsat(P1tmp,(fabs(Cosphimode.Cosphi)*EnergyManage.Prated),((-EnergyManage.Prated)*fabs(Cosphimode.Cosphi)));
          Q1tmp = sCosphiMode();
          if(Cosphimode.Cosphi_exc==0)
          {
          	if(P1tmp>2500)
  			{
  			 Q1tmp=Q1tmp*0.89;
  			}
          	else if(P1tmp>2200)
  			{
  			 Q1tmp=Q1tmp*0.95;
  			}
  			else if(P1tmp>1900)
  			{
  			  //Q1tmp=Q1tmp*0.8;
  			}
  			else if(P1tmp>1200)
  			{
  			  Q1tmp=Q1tmp*1.01;
  			}
  			else if(P1tmp>700)
  			{
  			   Q1tmp=Q1tmp*0.92;
  			}
  			else if(P1tmp>0)
  			{
  				Q1tmp=Q1tmp*0.88;
  			}
  			else if(P1tmp>-700)
  			{
  				Q1tmp=Q1tmp*0.5;
  			}
  			else if(P1tmp>-1200)
  			{
  				Q1tmp=Q1tmp*0.75;
  			}
  			else if(P1tmp>-2200)
  			{
  				Q1tmp=Q1tmp*0.9;
  			}
  			else
  			{
  				Q1tmp=Q1tmp*0.87;
  			}
          	if(Cosphimode.Cosphi<0.83)
            {
                 Q1tmp=Q1tmp*0.88;
            }
            else if(Cosphimode.Cosphi<0.86)
            {
                  Q1tmp=Q1tmp*0.95;//0.92
            }
          }
          else
          {
          	if(P1tmp>2500)
  			{
  			 Q1tmp=Q1tmp*0.92;
  			}
          	else if(P1tmp>2100)
            {
             Q1tmp=Q1tmp*0.97;
            }
  			else if(P1tmp>1700)
  			{
  			  Q1tmp=Q1tmp*1.01;
  			}
  			else if(P1tmp>1200)
  			{
  			  Q1tmp=Q1tmp*1.07;
  			}
  			else if(P1tmp>700)
  			{
  			   Q1tmp=Q1tmp*1.2;
  			}
  			else if(P1tmp>0)
  			{
  				Q1tmp=Q1tmp*1.3;
  			}
  			else if(P1tmp>-700)
  			{
  				Q1tmp=Q1tmp*0.45;
  			}
  			else if(P1tmp>-1200)
  			{
  				Q1tmp=Q1tmp*0.65;
  			}
  			else if(P1tmp>-2200)
  			{
  				Q1tmp=Q1tmp*0.76;
  			}
  			else
  			{
  				Q1tmp=Q1tmp*0.85;
  			}

          	 if(Cosphimode.Cosphi<0.83)
             {
          	     Q1tmp=Q1tmp*0.88;
             }
             else if(Cosphimode.Cosphi<0.86)
             {
                  Q1tmp=Q1tmp*0.95;//0.92
             }
          }
          Cosphimode.P = P2tmp;//__fsat(P1tmp,(fabs(Cosphimode.Cosphi)*EnergyManage.Prated),((-EnergyManage.Prated)*fabs(Cosphimode.Cosphi)));
          Q2tmp = sCosphiMode();
          if(Cosphimode.Cosphi_exc==0)
          {
            if(P2tmp>2500)
            {
                Q2tmp=Q2tmp*0.89;
            }
            else if(P2tmp>2200)
            {
                Q2tmp=Q2tmp*0.95;
            }
            else if(P2tmp>1900)
            {
                //Q2tmp=Q2tmp*0.8;
            }
  			else if(P2tmp>1200)
  			{
  			  Q2tmp=Q2tmp*1.02;
  			}
  			else if(P2tmp>700)
  			{
  			   Q2tmp=Q2tmp*0.92;
  			}
  			else if(P2tmp>0)
  			{
  				Q2tmp=Q2tmp*0.88;
  			}
  			else if(P2tmp>-700)
  			{
  				Q2tmp=Q2tmp*0.6;
  			}
  			else if(P2tmp>-1200)
  			{
  				Q2tmp=Q2tmp*0.75;
  			}
  			else if(P2tmp>-2200)
  			{
  				Q2tmp=Q2tmp*0.9;
  			}
  			else
  			{
  				Q2tmp=Q2tmp*0.88;
  			}
            if(Cosphimode.Cosphi<0.83)
            {
                Q2tmp=Q2tmp*0.90;
            }
            else if(Cosphimode.Cosphi<0.86)
            {
                Q2tmp=Q2tmp*0.95;//0.92
            }
          }
          else
          {   if(P2tmp>2500)
  			{
  			   Q2tmp=Q2tmp*0.92;
  			}
              else if(P2tmp>2100)
  			{
  			   Q2tmp=Q2tmp*0.97;
  			}
  			else if(P2tmp>1700)
  			{
  			  Q2tmp=Q2tmp*1.01;
  			}
  			else if(P2tmp>1200)
  			{
  			  Q2tmp=Q2tmp*1.07;
  			}
  			else if(P2tmp>700)
  			{
  			   Q2tmp=Q2tmp*1.2;
  			}
  			else if(P2tmp>0)
  			{
  				Q2tmp=Q2tmp*1.3;
  			}
  			else if(P2tmp>-700)
  			{
  				Q2tmp=Q2tmp*0.45;
  			}
  			else if(P2tmp>-1200)
  			{
  				Q2tmp=Q2tmp*0.65;
  			}
  			else if(P2tmp>-2200)
  			{
  				Q2tmp=Q2tmp*0.76;
  			}
  			else
  			{
  				Q2tmp=Q2tmp*0.85;
  			}
              if(Cosphimode.Cosphi<0.83)
              {
                  Q2tmp=Q2tmp*0.90;
              }
              else if(Cosphimode.Cosphi<0.86)
              {
                  Q2tmp=Q2tmp*0.95;//0.92
              }
          }
     }
    else if(EnergyManage.Enable.bit.PUmode)  //电压有功5.14.9 5.14.10
    {
       Delta_V=fabs(V1_Avg-PUmode.V1_Avg_Pre);

       if(Delta_V>=0.5)
       {
          if(PUmode.cnt1++>=5)
          {
              PUmode.cnt1=0;
              PUmode.V = V1_Avg;
              PUmode.P1out = sPUmode();
              PUmode.V1_Avg_Pre=V1_Avg;
          }
       }
       else
       {
           PUmode.cnt1=0;
       }

       P1tmp=__fmin(PUmode.P1out,P1tmp);

       Delta_V=fabs(V2_Avg-PUmode.V2_Avg_Pre);

       if(Delta_V>=0.5)
       {
          if(PUmode.cnt2++>=5)
          {
              PUmode.cnt2=0;
              PUmode.V = V2_Avg;
              PUmode.P2out = sPUmode();
              PUmode.V2_Avg_Pre=V2_Avg;
          }
       }
       else
       {
           PUmode.cnt2=0;
       }

       P2tmp=__fmin(PUmode.P2out,P2tmp);
       P_Tr=__fmax(PUmode.Tr,P_Tr);

    }
    else
    {
//        if(P1tmp>=4500) {Q1tmp = Q1tmp-400;}
//        if(P1tmp>=0) {Q1tmp = Q1tmp-100;}
    }
//    if(P1tmp!=Pre_P1tmp)
//    {
//        P1_Set=P1tmp;
//        Delta_P1=P1_Set-P1_Use;
//        P1_Reset=1;
//    }
//
//    if(P2tmp!=Pre_P2tmp)
//    {
//        P2_Set=P2tmp;
//        Delta_P2=P2_Set-P2_Use;
//        P2_Reset=1;
//    }
//
//    if(Q1tmp!=Pre_Q1tmp)
//    {
//        Q1_Set=Q1tmp;
//        Delta_Q1=Q1_Set-Q1_Use;
//        Q1_Reset=1;
//    }
//
//    if(Q2tmp!=Pre_Q2tmp)
//    {
//        Q2_Set=Q2tmp;
//        Delta_Q2=Q2_Set-Q2_Use;
//        Q2_Reset=1;
//    }
//
//    Pre_P1tmp=P1tmp;
//    Pre_P2tmp=P2tmp;
//    Pre_Q1tmp=Q1tmp;
//    Pre_Q2tmp=Q2tmp;
//
//    P1tmp=P1_Set-(Delta_P1*PQtcurveSet(&P1_Reset,P_Tr,0.01,&P1_MsCnt));  //0.17
//    P1_Use=P1tmp;
//    Q1tmp=Q1_Set-(Delta_Q1*PQtcurveSet(&Q1_Reset,Q_Tr,0.01,&Q1_MsCnt));
//    Q1_Use=Q1tmp;
//    P2tmp=P2_Set-(Delta_P2*PQtcurveSet(&P2_Reset,P_Tr,0.01,&P2_MsCnt));
//    P2_Use=P2tmp;
//    Q2tmp=Q2_Set-(Delta_Q2*PQtcurveSet(&Q2_Reset,Q_Tr,0.01,&Q2_MsCnt));
//    Q2_Use=Q2tmp;


    if(((P1tmp*P1tmp)+(Q1tmp*Q1tmp))>=(EnergyManage.Srated*EnergyManage.Srated))    //过载降额
    {
        if(P1tmp>0)
        {
            P1tmp=__sqrt((EnergyManage.Srated*EnergyManage.Srated)-(Q1tmp*Q1tmp));
        }
        else
        {
            P1tmp=-__sqrt((EnergyManage.Srated*EnergyManage.Srated)-(Q1tmp*Q1tmp));
        }
    }

    if(((P2tmp*P2tmp)+(Q2tmp*Q2tmp))>=(EnergyManage.Srated*EnergyManage.Srated))    //过载降额
    {
        if(P2tmp>0)
        {
            P2tmp=__sqrt((EnergyManage.Srated*EnergyManage.Srated)-(Q2tmp*Q2tmp));
        }
        else
        {
            P2tmp=-__sqrt((EnergyManage.Srated*EnergyManage.Srated)-(Q2tmp*Q2tmp));
        }
    }
     EnergyManage.P1out = P1tmp;//__fsat(P1tmp,Plimit.Smax,-Plimit.Smax);
     EnergyManage.P2out = P2tmp;//__fsat(P1tmp,Plimit.Smax,-Plimit.Smax);
     EnergyManage.Q1out = Q1tmp+EnergyManage.IslandQR; //
     EnergyManage.Q2out = Q2tmp+EnergyManage.IslandQS;

}

/**********************************************************************
* FUNCION :
* PURPOSE : 电网故障侦测初始化
* CALLED BY:
**********************************************************************/
void InitGridCheck(void)
{
    GridCheck.FaultMsg.all = 0;
    GridCheck.Enable.all = 0X0fff;

    GridCheck.OF2 = 62;
    GridCheck.OF1 = 61.2;
    GridCheck.UF1 = 58.5;
    GridCheck.UF2 = 56.5;
    GridCheck.OV2 = EnergyManage.Vrated*1.2;
    GridCheck.OV1 = EnergyManage.Vrated*1.1;
    GridCheck.UV1 = EnergyManage.Vrated*0.88;
    GridCheck.UV2 = EnergyManage.Vrated*0.5;

    GridCheck.OF2LossTime = 0.16;//二级过频保护时间，默认0.16s，可设0.16~1000s
    GridCheck.OF1LossTime = 300;//一级过频保护时间，默认300s ，可设180~1000s
    GridCheck.UF1LossTime = 300;//一级欠频保护时间，默认300s ，可设180~1000s
    GridCheck.UF2LossTime = 0.16;//二级欠频保护时间，默认0.16s，  可设0.16~1000s
    GridCheck.OV2LossTime = 0.16;
    GridCheck.OV1LossTime = 13;
    GridCheck.UV1LossTime = 21;
    GridCheck.UV2LossTime = 2;

    GridCheck.UVConnect = EnergyManage.Vrated* 0.917;
    GridCheck.OVConnect = EnergyManage.Vrated* 1.05;
    GridCheck.UFConnect = 59.5;
    GridCheck.OFConnect = 60.1;
    GridCheck.ConnectTime = 2;
}


/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitESmode(void)
{
    ESmode.P_SET = 0;
    ESmode.ES_PERMIT_SERVICE_EN=1; //允许并网
    ESmode.ES_EN=0;
    ESmode.ES_CNT=0;
    ESmode.ES_DELAY=10*5000L;//5s
    ESmode.ES_RANDOMIZED_DELAY=0;//300*100l;
    ESmode.ES_RAMP_RATE =0; //300*100l;//300s
    ESmode.ES_OF=65.0;//60.3;
    ESmode.ES_UF=55.0;//58.5;
    ESmode.ES_OV=1.1*EnergyManage.Vrated;//1.05*EnergyManage.Vrated;
    ESmode.ES_UV=0.88*EnergyManage.Vrated;//0.917*EnergyManage.Vrated;
    ESmode.P = 0;
}

/**********************************************************************
* FUNCION
* PURPOSE :机器缓启动
* CALLED BY:
**********************************************************************/
ES_STATUS_TYPE GetsESmodeStatus(void)
{
    return ESmode.ES_STATUS;
}

extern int CheckEntryServiceStatus(float uf,float lf,float uv, float lv);

/*************************************************
Function: CheckEntryServiceStatus
Description: 检查EntryService条件是否满足
Input: @uf-频率上限;@lf-频率下限;@uv-电压上限;@lv-电压下限
Output: None
Return: 0-条件不满足;1-条件满足
Frequency:
*************************************************/
int CheckEntryServiceStatus(float uf,float lf,float uv, float lv)
{
    if (SampleRms.Grid1VoltRms < lv || SampleRms.Grid1VoltRms > uv)
    {
       return 0;
    }

    if (SampleRms.Grid2VoltRms < lv || SampleRms.Grid2VoltRms > uv)
    {
        return 0;
    }

   if (CaptureData.FreqR < lf || CaptureData.FreqR > uf)
   {
       return 0;
   }

   if (CaptureData.FreqS < lf || CaptureData.FreqS > uf)
   {
       return 0;
   }

   return 1;
}

void sESmodeinit(void)
{
    ESmode.P=0;
    ESmode.ES_CNT=0;
    ESmode.SlopStep=0;
    ESmode.ES_STATUS=ES_INIT;
    ESmode.ES_DELAY_CNT=0;
}

void EMmodeENcheck(void)
{
    if(ESmode.ES_PERMIT_SERVICE_EN) //并网条件检测
    {
        if(ESmode.ES_STATUS >= ES_DELAY)
        {
            ESmode.ES_EN=1;
        }
        else if(1)//(CheckEntryServiceStatus(ESmode.ES_OF,ESmode.ES_UF,ESmode.ES_OV,ESmode.ES_UV)==1))
        {
           ESmode.ES_EN=1;
        }
        else
        {
            ESmode.ES_EN=0;
        }
    }
    else
    {
        ESmode.ES_EN=0;
    }

    if(InvStateData.WorkState<=ErrorState)   //初始时
    {
        ESmode.ES_STATUS=ES_INIT;
        ESmode.ES_CNT=0;
        ESmode.ES_DELAY_CNT=0;
    }

    if((ESmode.ES_EN==0)&&(ESmode.ES_STATUS!=ES_FINISH))
    {
        SystemAlarm.bit.ESOR=1;
    }
    else
    {
        SystemAlarm.bit.ESOR=0;
    }
}

float sESmode(void)
{
    if(ESmode.ES_RAMP_RATE==0.0)
    {
        ESmode.SlopStep=ESmode.P_SET;
    }
    else ESmode.SlopStep = ESmode.P_SET/((float)ESmode.ES_RAMP_RATE);//每10ms步进

    if((++ESmode.ES_CNT)>(ESmode.ES_RANDOMIZED_DELAY))
    {
        if(ESmode.ES_CNT>(ESmode.ES_RAMP_RATE))
        {
            ESmode.ES_CNT=0;
            ESmode.P = ESmode.P_SET;
            ESmode.ES_STATUS=ES_FINISH;
        }
        else
        {
            ESmode.P += ESmode.SlopStep;
        }
    }
    else
    {
        ESmode.ES_STATUS=ES_DELAY;
    }

    return ESmode.P;
}


/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitPFmode(void)
{
    PFmode.Pref = 4500;
    PFmode.OFDeadband = 0.036;
    PFmode.UFDeadband = 0.036;
    PFmode.kOF_F = 0.05;
    PFmode.kUF_F = 0.05;
    PFmode.kOF_P = (60*PFmode.kOF_F)/EnergyManage.Prated;
    PFmode.kUF_P = (60*PFmode.kUF_F)/EnergyManage.Prated;
    PFmode.FH = 62.0;
    PFmode.FL = 58.8;
    PFmode.Tr =5;   //5s
}

/**********************************************************************
* FUNCION :
* PURPOSE :频率有功响应
* CALLED BY:
**********************************************************************/
float sPFmode(void)
{
    PFmode.kOF_P = (60*PFmode.kOF_F)/EnergyManage.Prated;
    PFmode.kUF_P = (60*PFmode.kUF_F)/EnergyManage.Prated;

    PFmode.F = __fsat(PFmode.F,PFmode.FH,PFmode.FL);
    PFmode.Fdelta = PFmode.F - EnergyManage.Frated; //平移

    if(PFmode.Fdelta > PFmode.OFDeadband)   //过频 ，防止反向
    {
        PFmode.P = PFmode.Pref - ((PFmode.Fdelta - PFmode.OFDeadband)/PFmode.kOF_P);
        PFmode.P=__fsat(PFmode.P, PFmode.Pref,-4500);
    }

    else if(PFmode.Fdelta < -PFmode.UFDeadband) //欠频
    {
        PFmode.P = PFmode.Pref - ((-PFmode.Fdelta + PFmode.UFDeadband)/(PFmode.kUF_P));
        PFmode.P=__fmax(PFmode.P,4500);
    }
    else
    {
        PFmode.P = PFmode.Pref;
    }
    return PFmode.P;
}

/**********************************************************************
* FUNCION :
* PURPOSE :功率电压响应
* CALLED BY:
**********************************************************************/
void InitPUmode(void)
{
	PUmode.P1 = 1.0*2750.0;
	PUmode.P2 = EnergyManage.Prated*(0.0f);
	PUmode.V1 = EnergyManage.Vrated*1.1;
	PUmode.V2 = EnergyManage.Vrated*1.14;
	PUmode.K12 = Calc_lineK(PUmode.V1,PUmode.P1,PUmode.V2,PUmode.P2);
	PUmode.Tr=10.0f;
	PUmode.P=2750.0;
}

/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
float sPUmode()
{
    PUmode.K12 = Calc_lineK(PUmode.V1,PUmode.P1,PUmode.V2,PUmode.P2);

    if(PUmode.V < PUmode.V1)
    {
        PUmode.P = PUmode.P1;
    }
    else if(PUmode.V < PUmode.V2)
    {
        PUmode.P = PUmode.P1 + PUmode.K12*(PUmode.V - PUmode.V1);
    }
    else
    {
        PUmode.P = PUmode.P2;
    }
    return PUmode.P;
}

/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitQUmode(void)
{
    QUmode.Vref = EnergyManage.Vrated*1.0f;
    QUmode.Vref_AdjEn=0;
//    QUmode.V1_delta = -EnergyManage.Vrated*0.17;
//    QUmode.V2_delta = -EnergyManage.Vrated*0.01;
//    QUmode.V3_delta = EnergyManage.Vrated*0.02;
//    QUmode.V4_delta = EnergyManage.Vrated*0.05;
//    QUmode.V1 = 0.92*EnergyManage.Vrated;//QUmode.Vref + QUmode.V1_delta;
//    QUmode.V2 = 0.98*EnergyManage.Vrated;//QUmode.Vref + QUmode.V2_delta;
    QUmode.V3 = 1.02*EnergyManage.Vrated;//QUmode.Vref + QUmode.V3_delta;
    QUmode.V4 = 1.06*EnergyManage.Vrated;//QUmode.Vref + QUmode.V4_delta;
    QUmode.V5 = 1.1*EnergyManage.Vrated;
    QUmode.V6 = 1.14*EnergyManage.Vrated;
    QUmode.V_delta = 0;
//    QUmode.Q1 = EnergyManage.Srated * 0.44;
//    QUmode.Q2 = EnergyManage.Srated * 0.0;
    QUmode.Q3 = EnergyManage.Srated * 0.0;
    QUmode.Q4 = -EnergyManage.Srated*  0.6;
    QUmode.K12 = Calc_lineK(QUmode.V1,QUmode.Q1,QUmode.V2,QUmode.Q2);
    QUmode.K23 = Calc_lineK(QUmode.V2,QUmode.Q2,QUmode.V3,QUmode.Q3);
    QUmode.K34 = Calc_lineK(QUmode.V3,QUmode.Q3,QUmode.V4,QUmode.Q4);

    QUmode.Lpf_Tc = 300.0;
    QUmode.Lpf_Ts = 0.01;
    QUmode.Lpf_A1 = QUmode.Lpf_Ts/QUmode.Lpf_Tc;
    QUmode.Lpf_A0 = 1 - QUmode.Lpf_A1;

    QUmode.Tr=5.0;
    QUmode.Q1_out=0;
    QUmode.Q2_out=0;
    QUmode.Q1_MsCnt=0;
    QUmode.Q2_MsCnt=0;
}
/**********************************************************************
* FUNCION :
* PURPOSE : 曲线向左平移Vref
* CALLED BY:
**********************************************************************/
float sQUmode(void)
{
    QUmode.V1_delta=QUmode.V1-QUmode.Vref;
    QUmode.V2_delta=QUmode.V2-QUmode.Vref;
    QUmode.V3_delta=QUmode.V3-QUmode.Vref;
    QUmode.V4_delta=QUmode.V4-QUmode.Vref;

    QUmode.K12 = Calc_lineK(QUmode.V1,QUmode.Q1,QUmode.V2,QUmode.Q2);
//    QUmode.K23 = Calc_lineK(QUmode.V2,QUmode.Q2,QUmode.V3,QUmode.Q3);
    QUmode.K34 = Calc_lineK(QUmode.V3,QUmode.Q3,QUmode.V4,QUmode.Q4);

    QUmode.V_delta = QUmode.V - QUmode.Vref;    // 117.5-120.0=-2.5

    if(QUmode.V_delta < QUmode.V1_delta)
    {
        QUmode.Q = QUmode.Q1;
    }
    else if(QUmode.V_delta <= QUmode.V2_delta)
    {
        QUmode.Q = QUmode.Q2 - QUmode.K12*( QUmode.V2_delta - QUmode.V_delta);
    }
    else if(QUmode.V_delta <= QUmode.V3_delta)
    {
        QUmode.Q=QUmode.Q3;
    }
    else if(QUmode.V_delta <= QUmode.V4_delta)
    {
        QUmode.Q = QUmode.Q4 - QUmode.K34*(QUmode.V4_delta-QUmode.V_delta);
    }
    else
    {
        QUmode.Q = QUmode.Q4;
    }

    return QUmode.Q;
}

/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitQPmode(void)
{
    QPmode.P3_P = EnergyManage.Prated * 1.0;
    QPmode.P2_P = EnergyManage.Prated * 0.5;
    QPmode.P1_P = EnergyManage.Prated * 0.2;
    QPmode.P3_N = -EnergyManage.Prated *1.0;
    QPmode.P2_N = -EnergyManage.Prated * 0.5;
    QPmode.P1_N = -EnergyManage.Prated * 0.2;
    QPmode.Q3_P = -EnergyManage.Srated * 0.44;
    QPmode.Q2_P = -EnergyManage.Srated * 0.0;
    QPmode.Q1_P = -EnergyManage.Srated * 0.0;
    QPmode.Q3_N = EnergyManage.Srated * 0.44;
    QPmode.Q2_N = EnergyManage.Srated * 0.0;
    QPmode.Q1_N = EnergyManage.Srated * 0.0;

    QPmode.K01_P = Calc_lineK(0,0,QPmode.P1_P,QPmode.Q1_P);
    QPmode.K12_P = Calc_lineK(QPmode.P1_P,QPmode.Q1_P,QPmode.P2_P,QPmode.Q2_P);
    QPmode.K23_P = Calc_lineK(QPmode.P2_P,QPmode.Q2_P,QPmode.P3_P,QPmode.Q3_P);
    QPmode.K01_N = Calc_lineK(QPmode.P1_N,QPmode.Q1_N,0,0);
    QPmode.K12_N = Calc_lineK(QPmode.P2_N,QPmode.Q2_N,QPmode.P1_N,QPmode.Q1_N);
    QPmode.K23_N = Calc_lineK(QPmode.P3_N,QPmode.Q3_N,QPmode.P2_N,QPmode.Q2_N);
}
/**********************************************************************
* FUNCION :
* PURPOSE :计算折线Q(P)的值
* CALLED BY:
**********************************************************************/
float sQPmode(void)
{
    QPmode.K01_P = Calc_lineK(0,0,QPmode.P1_P,QPmode.Q1_P);
    QPmode.K12_P = Calc_lineK(QPmode.P1_P,QPmode.Q1_P,QPmode.P2_P,QPmode.Q2_P);
    QPmode.K23_P = Calc_lineK(QPmode.P2_P,QPmode.Q2_P,QPmode.P3_P,QPmode.Q3_P);
    QPmode.K01_N = Calc_lineK(QPmode.P1_N,QPmode.Q1_N,0,0);
    QPmode.K12_N = Calc_lineK(QPmode.P2_N,QPmode.Q2_N,QPmode.P1_N,QPmode.Q1_N);
    QPmode.K23_N = Calc_lineK(QPmode.P3_N,QPmode.Q3_N,QPmode.P2_N,QPmode.Q2_N);

    if(QPmode.P > 0)
    {
        if(QPmode.P < QPmode.P1_P)
        {
            QPmode.Q = QPmode.K01_P * QPmode.P;
        }
        else if(QPmode.P < QPmode.P2_P)
        {
            QPmode.Q = QPmode.Q1_P + QPmode.K12_P*(QPmode.P - QPmode.P1_P);
        }
        else if(QPmode.P < QPmode.P3_P)
        {
            QPmode.Q = QPmode.Q2_P + QPmode.K23_P*(QPmode.P - QPmode.P2_P);
        }
        else
        {
            QPmode.Q = QPmode.Q3_P;
        }
    }
    else
    {
        if(QPmode.P > QPmode.P1_N)
        {
            QPmode.Q = QPmode.K01_N * QPmode.P;
        }
        else if(QPmode.P > QPmode.P2_N)
        {
            QPmode.Q = QPmode.Q1_N + QPmode.K12_N*(QPmode.P - QPmode.P1_N);
        }
        else if(QPmode.P > QPmode.P3_N)
        {
            QPmode.Q = QPmode.Q2_N + QPmode.K23_N*(QPmode.P - QPmode.P2_N);
        }
        else
        {
            QPmode.Q = QPmode.Q3_N;
        }
    }
    return QPmode.Q;
}

/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:`
**********************************************************************/
void InitCQmode(void)
{
    CQmode.Q1=0;
    CQmode.Q2=0;
}


/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
void InitCosphimode(void)
{
    Cosphimode.P = EnergyManage.Prated;
    Cosphimode.Cosphi_exc=0;
    Cosphimode.Cosphi = 1.0;
    Cosphimode.PFmin_inj = 0.8;
    Cosphimode.PFmin_ab = -0.8;
    Cosphimode.Q = 0;
}

/**********************************************************************
* FUNCION :
* PURPOSE :
* CALLED BY:
**********************************************************************/
float sCosphiMode(void)
{
    float TmpCosphi;
    float Tanphi;
    TmpCosphi = __fsat(Cosphimode.Cosphi,1,Cosphimode.PFmin_inj);

    if(Cosphimode.Cosphi_exc == 0)  //0为正，1为负
    {
        Tanphi = __sqrt((1.0 - TmpCosphi * TmpCosphi)/(TmpCosphi*TmpCosphi));
    }
    else
    {
        Tanphi = -1.0*__sqrt((1.0 - TmpCosphi * TmpCosphi)/(TmpCosphi*TmpCosphi));
    }

    Cosphimode.Q = Cosphimode.P * Tanphi;
    return Cosphimode.Q;
}


/**********************************************************************
* FUNCION :
* PURPOSE : 计算两点间的斜率
* CALLED BY:
**********************************************************************/
 float Calc_lineK(float x1,float y1,float x2,float y2)
{
    float k=0;

    if(x1==x2)
    {
        k = 0;
    }
    else
    {
        k = (y2-y1)/(x2-x1);
    }
    return k;
}


 /**********************************************************************
 * FUNCION :
 * PURPOSE : 计算P(t)/Q(t)曲线
 * CALLED BY:
 **********************************************************************/
 float PQtcurveSet(char* Reset,float Tr,float Ts,unsigned int* MsCnt) //P(t)/Q(t)曲线函数,输入值1：有功/无功最终目标，输入值2：时间常数，输出值：有功/无功当前目标
 {
     float tau=Tr/2.3,temp=0.0;

     if(*Reset==1)
     {
         *MsCnt=0;
         *Reset=0;
     }

     if(*MsCnt<(unsigned int)(tau*0.5/Ts))   //0.5tau,0.01s=10ms
     {

           temp=(0.7854*(*MsCnt/(tau/Ts))+0.0135);   //0.7584(t/tau)+0.0135

     }
     else if(*MsCnt<=(unsigned int)(tau*1.0/Ts))  //1.0tau
     {

         temp=(0.4764*(*MsCnt/(tau/Ts))+0.1635);

     }
     else if(*MsCnt<=(unsigned int)(tau*2.0/Ts))  //2.0tau
     {

         temp=(0.2298*(*MsCnt/(tau/Ts))+0.4208);   //0.2298(t/tau)+0.4208
     }
     else if(*MsCnt<=(unsigned int)(tau*3.0/Ts))//3.0tau
     {

         temp=(0.0845*(*MsCnt/(tau/Ts))+0.7024);   //0.0845(t/tau)+0.7024
     }
     else if(*MsCnt<=(unsigned int)(tau*4.0/Ts)) //4.0tau
     {
         temp=(0.0311*(*MsCnt/(tau/Ts))+0.8594);   //0.0311(t/tau)+0.8594
     }
     else
     {
         temp=1.0;
         *MsCnt=((unsigned int)(tau*4.0/Ts))+1;
     }

     (*MsCnt)++;

     temp=1.0-temp;

     return temp;
 }


 /*********************************************
  * 逆变器开关机状态
  *
  *
  *
  ****************************************/
 char GetInvOnOffStation(void)
 {
     if(InvStateData.WorkState>2)
     {
         return 2;
     }
     else return 1;
 }


/*********************************************
* 电网连接状态
*
*
*
****************************************/
 char GetInvConnectStation(void)
 {
     if((InvStateData.WorkState==OnGridState)&&(ESmode.ES_STATUS==ES_FINISH))
     {
         return 5;
     }
     else return 0;
 }

/*************************************************
 * 告警状态
 *
 *
 *
 *************************************************/
 void INVStateUpdate(void)
 {

     if(SystemAlarm.bit.GridVoltH)   //电网高压
     {
         INV_Status.bit.OVER_VOLTAGE=1;
     }
     else INV_Status.bit.OVER_VOLTAGE=0;

     if(SystemAlarm.bit.GridVoltL)   //电网低压
     {
         INV_Status.bit.UNDER_VOLTAGE=1;
     }
     else INV_Status.bit.UNDER_VOLTAGE=0;

     if(SystemAlarm.bit.GridFreqH)   //电网高频
     {
         INV_Status.bit.OVER_FREQUENCY=1;
     }
     else INV_Status.bit.OVER_FREQUENCY=0;

     if(SystemAlarm.bit.GridFreqL)   //电网低频
     {
         INV_Status.bit.UNDER_FREQUENCY=1;
     }
     else INV_Status.bit.UNDER_FREQUENCY=0;
 }

 /****************************************
  * 逆变器状态
  *
  *
  *
  *
  ****************************************/
 char GetInvStation(void)
 {
     if(InvStateData.WorkState==ErrorState)
     {
         return 7;
     }
     else if(InvStateData.WorkState==OffGridState)
     {
         return 2;
     }
     else if(InvStateData.WorkState==OnGridState)
     {
         return 8;
     }
     else if(InvStateData.WorkState==InitState)
     {
         return 6;
     }
     else return 0;
 }

/**********************************
 * 控制模式
 *
 *
 *
 **********************************/
 char GetControlmode()
 {
     if(EnergyManage.Enable.all==0)
     {
         return 0;
     }
     else
     {
         return 1;
     }
 }

 /************************************
  * 充放电状态
  *
  *
  ************************************/
 char GetInvStorageStation(void)
 {
     if((EnergyManage.P1out>0)||(EnergyManage.P1out>0))
     {
         return 1;
     }
     else if((EnergyManage.P1out<0)||(EnergyManage.P1out<0))
     {
         return 0;
     }
     else
     {
         return 3;
     }
 }

 struct EMData_REGS{
 //地址        变量类型    读/写     缩放因子        最大值         最小值     指针地址
 Uint16 addr;    char type;  char RW;    float scale;   long upper;   int low;    void *p;
 };

 struct EMData_REGS EMData[EMDataNum]={
  //==================================================================================================
  //地址    类型      读/写     缩放   最大值        最小值     指针地址    //变量名
  //==================================================================================================
  //只读
 {0,     'U',    'W',    1.0,       65535,     0,          &EnergyManage.Enable.all},
 {1,     'F',    'W',    100.0,     100,       90,         &Cosphimode.Cosphi},
 {2,     'U',    'W',    1.0,       1,         0,          &Cosphimode.Cosphi_exc},
 {3,     'F',    'W',    100.0,     12600,     11400,      &QUmode.Vref},
 {4,     'U',    'W',    1.0,       1,         0,          &QUmode.Vref_AdjEn},
 {5,     'F',    'W',    10.0,      50000,     3000,       &QUmode.Lpf_Tc},
 {6,     'F',    'W',    100.0,     12360,     9240,       &QUmode.V1},
 {7,     'F',    'W',    100.0,     12600,     11040,      &QUmode.V2},
 {8,     'F',    'W',    100.0,     12960,     11400,      &QUmode.V3},
 {9,     'F',    'W',    100.0,     14760,     11640,      &QUmode.V4},
 {10,    'F',    'W',    1.0,       1980,      0,          &QUmode.Q1},
 {11,    'F',    'W',    1.0,       1980,      -1980,      &QUmode.Q2},
 {12,    'F',    'W',    1.0,       1980,      -1980,      &QUmode.Q3},
 {13,    'F',    'W',    1.0,       0,         -1980,      &QUmode.Q4},
 {14,    'F',    'W',    100.0,     9000,      100,        &QUmode.Tr},
 {15,    'F',    'W',    1.0,       3150,      0,          &QPmode.P1_P},
 {16,    'F',    'W',    1.0,       3600,      1800,       &QPmode.P2_P},
 {17,    'F',    'W',    1.0,       4500,      2250,       &QPmode.P3_P},
 {18,    'F',    'W',    1.0,       0,         -3150,      &QPmode.P1_N},
 {19,    'F',    'W',    1.0,       -1800,     -3600,      &QPmode.P2_N},
 {20,    'F',    'W',    1.0,       -2250,     -4500,      &QPmode.P3_N},
 {21,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q1_P},
 {22,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q2_P},
 {23,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q3_P},
 {24,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q1_N},
 {25,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q2_N},
 {26,    'F',    'W',    1.0,       1980,      -1980,      &QPmode.Q3_N},
 {27,    'F',    'W',    1.0,       1980,      -1980,      &CQmode.Q1},
 {28,    'F',    'W',    1.0,       1980,      -1980,      &CQmode.Q2},
 {29,    'F',    'W',    100.0,     13080,     12600,      &PUmode.V1},
 {30,    'F',    'W',    100.0,     13200,     12720,      &PUmode.V2},
 {31,    'F',    'W',    1.0,       4500,      4500,       &PUmode.P1},
 {32,    'F',    'W',    1.0,       0,         -4500,      &PUmode.P2},
 {33,    'F',    'W',    100.0,     6000,      50,         &PUmode.Tr},
 {34,    'F',    'W',    100.0,     14400,     13200,      &Protect.Value.GridOV1},
 {35,    'F',    'W',    100.0,     14400,     14400,      &Protect.Value.GridOV2},
 {36,    'L',    'W',    1.0,       1300,      100,        &Protect.Delay.GridOV1Dly},
 {37,    'L',    'W',    1.0,       16,        16,         &Protect.Delay.GridOV2Dly},
 {38,    'F',    'W',    100.0,     10560,     3500,       &Protect.Value.GridUV1},
 {39,    'F',    'W',    100.0,     6000,      3500,       &Protect.Value.GridUV2},
 {40,    'L',    'W',    1.0,       5000,      200,        &Protect.Delay.GridUV1Dly},
 {41,    'L',    'W',    1.0,       2100,      16,         &Protect.Delay.GridUV2Dly},
 {42,    'F',    'W',    100.0,     6600,      6100,       &Protect.Value.GridOF1},
 {43,    'F',    'W',    100.0,     6600,      6180,       &Protect.Value.GridOF2},
 {44,    'H',    'W',    1.0,       65000,    16,         (Uint16 *)(&Protect.Delay.GridOF1Dly)},
 {45,    'U',    'W',    1.0,       65535,     0,          (((Uint16 *)(&Protect.Delay.GridOF1Dly))+1)},
 {46,    'H',    'W',    1.0,       65000,    16,         (Uint16 *)(&Protect.Delay.GridOF2Dly)},
 {47,    'U',    'W',    1.0,       65535,     0,          (((Uint16 *)(&Protect.Delay.GridOF2Dly))+1)},
 {48,    'F',    'W',    100.0,     5900,      5000,       &Protect.Value.GridUF1},
 {49,    'F',    'W',    100.0,     5700,      5000,       &Protect.Value.GridUF2},
 {50,    'H',    'W',    1.0,       65000,    16,         (Uint16 *)(&Protect.Delay.GridUF1Dly)},
 {51,    'U',    'W',    1.0,       65535,     0,          (((Uint16 *)(&Protect.Delay.GridUF1Dly))+1)},
 {52,    'H',    'W',    1.0,       65000,    16,         ((Uint16 *)(&Protect.Delay.GridUF2Dly))},
 {53,    'U',    'W',    1.0,       65535,     0,          (((Uint16 *)(&Protect.Delay.GridUF2Dly))+1)},
 {54,    'F',    'W',    1000.0,    1000,      17,         &PFmode.OFDeadband},
 {55,    'F',    'W',    1000.0,    1000,      17,         &PFmode.UFDeadband},
 {56,    'F',    'W',    1000.0,    50,        20,         &PFmode.kOF_F},
 {57,    'F',    'W',    1000.0,    50,        20,         &PFmode.kUF_F},
 {58,    'F',    'W',    100.0,     1000,      20,         &PFmode.Tr},
 {59,    'F',    'W',    100.0,     14400,     12840,      &ESmode.ES_OV},
 {60,    'F',    'W',    100.0,     11400,     10560,      &ESmode.ES_UV},
 {61,    'F',    'W',    100.0,     6500,      6100,       &ESmode.ES_OF},
 {62,    'F',    'W',    100.0,     5900,      5500,       &ESmode.ES_UF},
 {63,    'H',    'W',    1.0,       6000000,   0,          (Uint16 *)(&ESmode.ES_DELAY)},
 {64,    'U',    'W',    1.0,       65535,     0,          ((Uint16 *)(&ESmode.ES_DELAY))+1},
 {65,    'L',    'W',    0.01f,     1000,      0,          &ESmode.ES_RANDOMIZED_DELAY},
 {66,    'L',    'W',    0.01f,     1000,      0,          &ESmode.ES_RAMP_RATE},
 {67,    'F',    'W',    1.0,       4500,      0,          &EnergyManage.PLimitCmd},
 {68,    'U',    'W',    1.0,       65535,     0,          &EMinitEN},
 };

 struct I2CMSG EMData_Read={
     I2C_MSGSTAT_INACTIVE,
     EEPROM_SLAVE_ADDR,
     EEPROM_ReadBytes,
     EEPROM_HIGH_ADDR,
     EEPROM_LOW_ADDR,
 };

 struct I2CMSG EMData_Write = {
     I2C_MSGSTAT_INACTIVE,
     EEPROM_SLAVE_ADDR,
     EEPROM_WriteBytes,
     EEPROM_HIGH_ADDR,
     EEPROM_LOW_ADDR,
 };


void User_EME2ROMData_Read(void)
{
     mSecCNT_struct mSecCNT = {0,0};
     Uint16 EeData_Addr=0x3F00;
     char i=0;
     for(;i<EMDataNum;i++)
     {
         EMData_Read.MemoryHighAddr = (EeData_Addr+(i*2)) >> 8;
         EMData_Read.MemoryLowAddr = (EeData_Addr+(i*2)) & 0x00ff;
         while((UserI2c_Read(&EMData_Read)!=I2C_SUCCESS)&&(User_usCount(&mSecCNT) < 100000L));//防阻塞
         {
             asm(" NOP");
         }
         EMData_shadow[i]=(Uint16)((EMData_Read.MsgBuffer[0]<<8)+EMData_Read.MsgBuffer[1]);
     }
}

void EMdatainit(void)
{
    int i=0;
    long value=0;

    for(;i<EMDataNum;i++)
    {
        if(EMData[i].type=='U')
          {
              value=__fsat(EMData_shadow[i],EMData[i].upper,EMData[i].low);
              value=value/EMData[i].scale;
              *(Uint16 *)EMData[i].p=(Uint16)value;

          }
          else if(EMData[i].type=='I')
          {
              value=__fsat((int16)EMData_shadow[i],EMData[i].upper,EMData[i].low);
              value=value/EMData[i].scale;
              *(int16 *)EMData[i].p=value;
          }
          else if(EMData[i].type=='L')
          {
              value=__fsat(EMData_shadow[i],EMData[i].upper,EMData[i].low);
              value=value/EMData[i].scale;
              *( long *)EMData[i].p=value;
          }
          else if(EMData[i].type=='H')
          {
              *((Uint16 *)(&value))=EMData_shadow[i];
              *(((Uint16 *)&value)+1)=EMData_shadow[i+1];
               value=__fsat(value,EMData[i].upper,EMData[i].low);
               *(long *)EMData[i].p=value;
               i++;
          }
          else
          {
              value=__fsat((int16)EMData_shadow[i],EMData[i].upper,EMData[i].low);
              *(float*)EMData[i].p=value/EMData[i].scale;
          }
    }
}


Uint16 EMDataBase_Read(Uint16 i)  //读参数
{
    Uint16 Data;

    //否则读取Database表中的数据（可写的参数均可读）
    if ((EMData[i].RW == 'R') || (EMData[i].RW == 'W'))
    {
        if(EMData[i].type == 'F')
        {
            Data=(int)(EMData[i].scale*(*((float*)EMData[i].p)));
        }
        else if(EMData[i].type == 'U')
        {
            Data=(Uint16)(EMData[i].scale*(*(Uint16*)EMData[i].p));
        }
        else if(EMData[i].type == 'I')
        {
            Data =(int)( EMData[i].scale*(*(int*)EMData[i].p));
        }
        else if(EMData[i].type == 'L')
        {
            Data =(Uint16)(EMData[i].scale*(*(unsigned long*)EMData[i].p));
        }
        else if(EMData[i].type == 'H')
        {
            Data =(Uint16)(EMData[i].scale*(*(Uint16*)EMData[i].p));
        }

        return Data;
    }
    else
        return 0;
}


void User_EMData_Check(void)
{
    static Uint16 EMCheckNO=0;
    static Uint16 EeData_Addr=0x3F00;
    static int EMDataWrite_EN=0;
    Uint16 Value=0;
    Uint16 Addr=0;

    if(EMDataWrite_EN==0)   //判断是否可写
    {
        for(;EMCheckNO<EMDataNum;EMCheckNO++)
        {
            Addr=EeData_Addr+(EMCheckNO*2);
            Value=EMDataBase_Read(EMCheckNO);

            if(EMData_WriteFirstFlag==1)
            {
                EMData_shadow[EMCheckNO]=Value;
                EMData_Write.MemoryHighAddr=Addr>> 8;
                EMData_Write.MemoryLowAddr =Addr & 0xff;
                EMData_Write.MsgBuffer[0]=(Value>>8);
                EMData_Write.MsgBuffer[1]=(Value&0xff);
                EMDataWrite_EN=1;
                EMCheckNO++;
                break;
            }
            else
            {
                if(EMData_shadow[EMCheckNO]!=Value)
                {
                   EMData_shadow[EMCheckNO]=Value;
                   EMData_Write.MemoryHighAddr=Addr>> 8;
                   EMData_Write.MemoryLowAddr =Addr & 0xff;
                   EMData_Write.MsgBuffer[0]=(Value>>8);
                   EMData_Write.MsgBuffer[1]=(Value&0xff);
                   EMDataWrite_EN=1;
                   EMCheckNO++;
                   break;
                }
            }
        }
    }

    if(EMDataWrite_EN==1)
    {
        if(UserI2c_Write(&EMData_Write)!=I2C_SUCCESS)
        {
            return ;
        }
        else
        {
            EMDataWrite_EN=0;
        }
    }

    if(EMCheckNO>=EMDataNum)
    {
        EMCheckNO=0;
        EMData_WriteFirstFlag=0;
        EeData_Addr=0x3F00;
    }
}




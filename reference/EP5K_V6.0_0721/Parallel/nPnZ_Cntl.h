/*
 * Controller.h
 *
 *  Created on: 2022��6��30��
 *      Author: DLadmin
 */

#ifndef MODULE_NPNZ_CNTL_H_
#define MODULE_NPNZ_CNTL_H_

#define ReptBufferSize  (500)    //20k/40hz

typedef struct
{
    float Ref, Fbk;
    float Kp;
    float Ki;
    float xp[2], yp[2];
    float max;
    float min;
    float Imax;
    float Imin;
    int satflag;
} PIDCntl_F;

typedef struct {
    float Coeff_B2;
    float Coeff_B1;
    float Coeff_B0;
    float Coeff_A2;
    float Coeff_A1;
    float Coeff_TempB0;
} CNTL_2P2Z_F_COEFFS;

typedef struct {
    float Out1;
    float Out2;
    float Errn;
    float Errn1;
    float Errn2;
    float Ref;
    float Fdbk;
    float Out;
    float Max;
    float Min;
} CNTL_2P2Z_F_VARS;

typedef struct
{
    int  ErrPt;
    int  OutPt;
    int  Tc;
    int  Prd;

    float Err;
    float Out;
    float ErrArray[ReptBufferSize];
    float Kr;      //比例系数
    float Max;
    float Min;
} REPTCntl_F;

//*********** Function Declarations *******//
void PI_Controller(PIDCntl_F *v);
void Init_PI_Controller(PIDCntl_F *v ,float kp , float ki , float max , float min );
//void Update_PI_Coeff( CNTL_2P2Z_F_COEFFS *coef1, float kp, float ki );

#define Init_CNTL_2P2Z_F_VARS_  Init_PRVars
void Init_PRVars(CNTL_2P2Z_F_VARS *k, float max , float min );
void CNTL_2P2Z_F_FUNC(CNTL_2P2Z_F_COEFFS *v, CNTL_2P2Z_F_VARS *k);
void Init_PRCoeff(CNTL_2P2Z_F_COEFFS *coef1, float kp, float ki, float wo, float fs, float wrc );
//void Update_PR_Coeff(CNTL_2P2Z_F_COEFFS *coef1, float kp, float ki, float wo, float fs, float wrc );

void Init_Rept_Controller(REPTCntl_F *v,float Kr ,int Tc,float Max,float Min);
void Rept_Controller(REPTCntl_F *v);


float ErrorObserv(float error);
//void findMax(Uint16 addr,float val);
//void resetMax(Uint16 addr);
extern float errRMS;
extern float nPnZ_max[10];

#define mRefRamp(v,vset,step)  \
       {if(v>(vset+step))      \
           v -=step;        \
        else if((v+step)<(vset))\
           v +=step;        \
        else                \
           v=vset;}

#endif /* MODULE_NPNZ_CNTL_H_ */

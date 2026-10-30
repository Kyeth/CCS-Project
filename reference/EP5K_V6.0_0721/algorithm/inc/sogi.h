/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: sogi.h
Author: tzz
Version: 0.0.2
Date: 2022年2月15日
Description: 申明函数,变量类型定义
*************************************************/
#ifndef ALGORITHM_INC_SOGI_H_
#define ALGORITHM_INC_SOGI_H_

typedef struct
{
    float osg_k;
    float osg_x;
    float osg_y;
    float osg_b0;
    float osg_b2;
    float osg_a1;
    float osg_a2;
    float osg_qb0;
    float osg_qb1;
    float osg_qb2;
}SPLL_1ph_SOGI_F_OSG_COEFF;

typedef struct
{
    float u[3];  // Ac Input
    float osg_u[3];
    float osg_qu[3];
    float u_Q[2];
    float u_D[2];
    SPLL_1ph_SOGI_F_OSG_COEFF osg_coeff;
}SPLL_1ph_SOGI_F;


typedef struct
{
    float x[3];
    float y[3];
    float a0;
    float a1;
    float b0;
    float b1;
    float b2;
} Filter_STRUCT;

void SPLL_1ph_SOGI_F_coeff_update(float delta_T, float wn, SPLL_1ph_SOGI_F *spll);
void SPLL_1ph_SOGI_F_FUNC(SPLL_1ph_SOGI_F *spll_obj);

void Init_VffFilter(Filter_STRUCT *v);
void VffFilter_Func (Filter_STRUCT *v);
#endif /* ALGORITHM_INC_SOGI_H_ */

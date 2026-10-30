/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name: sogi.c
** Latest modified Date:2022年6月6日
** Latest Version: V1.0
** Description:正弦信号正交分解,初始化SPLL_1ph_SOGI_F_coeff_update(),调用SPLL_1ph_SOGI_F_FUNC()
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2021-10-06
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/
#include "sogi.h"
#include "ConfigSystemPara.h"
/*************************************************
Function: SPLL_1ph_SOGI_F_coeff_update
Description: 初始化正交分解信号发生器
Input: @delta_T-调用频率;@wn-2*pi*fn;@spll-spll变量结构体指针
Output: None
Frequency: Once
*************************************************/
void SPLL_1ph_SOGI_F_coeff_update(float delta_T, float wn, SPLL_1ph_SOGI_F *spll)
{
    float osgx,osgy,temp;
    spll->osg_coeff.osg_k=(float)(0.5);
    osgx=(float)(2.0*0.5*wn*delta_T);
    spll->osg_coeff.osg_x=(float)(osgx);
    osgy=(float)(wn*delta_T*wn*delta_T);
    spll->osg_coeff.osg_y=(float)(osgy);
    temp=(float)1.0/(osgx+osgy+4.0);
    spll->osg_coeff.osg_b0=((float)osgx*temp);
    spll->osg_coeff.osg_b2=((float)(-1.0)*spll->osg_coeff.osg_b0);
    spll->osg_coeff.osg_a1=((float)(2.0*(4.0-osgy))*temp);
    spll->osg_coeff.osg_a2=((float)(osgx-osgy-4)*temp);
    spll->osg_coeff.osg_qb0=((float)(0.5*osgy)*temp);
    spll->osg_coeff.osg_qb1=(spll->osg_coeff.osg_qb0*(float)(2.0));
    spll->osg_coeff.osg_qb2=spll->osg_coeff.osg_qb0;
}
/*************************************************
Function: SPLL_1ph_SOGI_F_FUNC
Description: 正交分解信号发生器
Input: @spll-spll变量结构体指针;spll_obj->osg_u[0]
Output: @spll_obj->osg_u[0];@spll_obj->osg_qu[0]
Frequency: delta_T
*************************************************/
void SPLL_1ph_SOGI_F_FUNC(SPLL_1ph_SOGI_F *spll_obj)
{
    // Update the spll_obj->u[0] with the grid value before calling this routine

    //-------------------------------//
    // Orthogonal Signal Generator   //
    //-------------------------------//
    spll_obj->osg_u[0]=(spll_obj->osg_coeff.osg_b0*(spll_obj->u[0]-spll_obj->u[2])) + (spll_obj->osg_coeff.osg_a1*spll_obj->osg_u[1]) + (spll_obj->osg_coeff.osg_a2*spll_obj->osg_u[2]);

    spll_obj->osg_u[2]=spll_obj->osg_u[1];
    spll_obj->osg_u[1]=spll_obj->osg_u[0];

    spll_obj->osg_qu[0]=(spll_obj->osg_coeff.osg_qb0*spll_obj->u[0]) + (spll_obj->osg_coeff.osg_qb1*spll_obj->u[1]) + (spll_obj->osg_coeff.osg_qb2*spll_obj->u[2]) + (spll_obj->osg_coeff.osg_a1*spll_obj->osg_qu[1]) + (spll_obj->osg_coeff.osg_a2*spll_obj->osg_qu[2]);

    spll_obj->osg_qu[2]=spll_obj->osg_qu[1];
    spll_obj->osg_qu[1]=spll_obj->osg_qu[0];

    spll_obj->u[2]=spll_obj->u[1];
    spll_obj->u[1]=spll_obj->u[0];
}

//#pragma CODE_SECTION(VffFilter_Func, ".TI.ramfunc");
void  VffFilter_Func (Filter_STRUCT *v)
{
    v->y[0]=v->a0*v->y[1] + v->a1*v->y[2] + v->b0*v->x[0] + v->b1*v->x[1] + v->b2*v->x[2];
    v->x[2]=v->x[1];
    v->x[1]=v->x[0];
    v->y[2]=v->y[1];
    v->y[1]=v->y[0];
}

void Init_VffFilter(Filter_STRUCT *v)
{
    if(!SystemPara.FrequencySet)
    {//50
        v->a0 = 1.9216629;
        v->a1 = -0.92310465;
        v->b0 = 0.039688814;
        v->b1 = 7.929833E-005;
        v->b2 = -0.039609516;
    }
    else
    {//60
        v->a0 = 1.9148916;
        v->a1 = -0.91666367;
        v->b0 = 0.042421902;
        v->b1 = 7.06443E-005;
        v->b2 = -0.042351258;
    }

}

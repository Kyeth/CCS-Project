/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name: CNTL_PI_F.c
** Latest modified Date:2022年6月6日
** Latest Version: V1.0
** Description:抗饱和积分的PI控制器算法模块
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
#include "CNTL_PI_F.h"

/******************************************************************************
// Export Function-Prototype: CNTL_PI_F_FUNC
// Description  : 抗饱和积分的PI控制器 函数模块
// Input        : @v-PI结构体变量指针
// Output       : None
// Return Value : None
// Calls        :
// Called By    :
// History      :
// 1.Date       : 2021/10/06
// Author       : tzz
// Modification : Created Function
******************************************************************************/
void CNTL_PI_F_FUNC(CNTL_PI_F *v)
{
    /* proportional term */
    v->up = ((v->Ref - v->Fbk)*v->Kp);

    /* integral term */
    v->ui = (v->Out == v->v1)?((v->Ki* v->up)+ v->i1) : v->i1;
    v->i1 = v->ui;

    /* control output */
    v->v1 = (v->up + v->ui);
    v->Out = (v->v1 > v->Umax)?(v->Umax) : v->v1;
    v->Out = (v->Out < v->Umin)?(v->Umin) : v->Out;
}

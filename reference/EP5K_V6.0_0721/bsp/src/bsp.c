/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: bsp.c
Author: tzz
Date:2022年2月11日
Description: 驱动模块初始化
Version:0.0.2
Function List:
***********************************************************/
#include "bsp.h"
#include "Parallel.h"
/*************************************************
Function: InitBSP
Description: 驱动模块初始化所有使用到的外设
Input: None
Output: None
Frequency: Once
*************************************************/
void InitBSP(void)
{
    Uint32  delay500msCnt = 0;
    Uint16 i = 0;
    Uint32 delayusCnt = 0;

    InitSysCtrl();

    InitGpio();

    DINT;

    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();

    InitDIDO();

    InitADC();

    InitEPwm();

    InitSci('A');
    InitSci('D');

    InitECap();

 //   InitCan();
 //   InitParallelCan();
    InitCanOscilloscope();
    InitI2C();

    InitUserTimers();

    CLA_Init();
    // 延时2000ms
    while(!User_usTimer(&delay500msCnt, 5000000L));

    // 添加驱动电源缓起
    for (i = 0; i < 50; i++)
    {
        delayusCnt = i*100;
        SPS_ON
        DELAY_US(delayusCnt);
        SPS_OFF
        delayusCnt = 5000L - delayusCnt;
        DELAY_US(delayusCnt);
    }
    SPS_ON
}
/*************************************************
Function: EnableDog
Description: 使能看门狗
Input: None
Output: None
Frequency: Once
*************************************************/
void EnableDog(void)
{
    // 使能看门狗
    EALLOW;
    WdRegs.WDCR.all = 0x0028;       // 使用默认时钟系数10M/512,13ms触发，最大可设置13*64=834
    EDIS;
}



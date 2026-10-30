/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: F2837xS_can.c
Author:
Version : 0.0.2
Date:2022年02月11日
Description: CAN 配置文件
Function List:
***********************************************************/
//
// Included Files
//
#include "F2837xS_device.h"
#include "F2837xS_Examples.h"

//
// InitCAN - Initializes the CAN-A controller after reset.
//
#define BRP_1M 3
#define BRP_500k 2
#define BRP_250k 1
#define BRP_125k 0
/*************************************************
Function: InitCanGpio
Description: CANA GPIO初始化
Input: None
Output: None
Frequency: Once
*************************************************/
////*****************************************************************************
//void InitCanGpio(void)
//{
//    EALLOW;
//
//    //=====================can--TX
//    GpioCtrlRegs.GPBQSEL1.bit.GPIO37 = 0;
//    GpioCtrlRegs.GPBPUD.bit.GPIO37   = 0;
//    GpioCtrlRegs.GPBGMUX1.bit.GPIO37 = 1;  // All GPIO   TX
//    GpioCtrlRegs.GPBMUX1.bit.GPIO37 = 2;  // All GPIO   TX
//    GpioCtrlRegs.GPBDIR.bit.GPIO37  = 1;
//    //=====================can---RX
//    GpioCtrlRegs.GPBQSEL1.bit.GPIO36 = 0;
//    GpioCtrlRegs.GPBPUD.bit.GPIO36   = 0;
//    GpioCtrlRegs.GPBGMUX1.bit.GPIO36 = 1;  // All GPIO   TX
//    GpioCtrlRegs.GPBMUX1.bit.GPIO36  = 2;  // All GPIO   TX
//    GpioCtrlRegs.GPBDIR.bit.GPIO36   = 0;
//
//    EDIS;
//}
/*************************************************
Function: InitCanA
Description: InitCanA初始化
Input: None
Output: None
Frequency: Once
*************************************************/
//void InitCanA(void)
//{
//    EALLOW;
//    CanaRegs.CAN_CTL.bit.Init = 1;//初始化CAN
//    CanaRegs.CAN_CTL.bit.PMD = 0x5;//不使能优先级
//    CanaRegs.CAN_RAM_INIT.all = 0x1A;//初始化CANRAM
//    while((CanaRegs.CAN_RAM_INIT.all & 0x3F) != 0x25);
//
//    CanaRegs.CAN_CTL.bit.SWR = 1;//软件复位一次
//    DELAY_US(1);
//    CanaRegs.CAN_CTL.bit.CCE = 1; //允许CPU配置一次
//
//    CanaRegs.CAN_ABOTR = 10000;//Auto-Bus-On   功能重启等待时间   时基与CAN时基一致
//    CanaRegs.CAN_CTL.bit.ABO = 1;//使能Auto-Bus-On功能   CANH和CANL之间短路或者总线上有错误帧时，会导致CAN出现崩溃停发现象   这里增加can崩溃时的复位重发功能
//    CanaRegs.CAN_CTL.bit.IE0 = 1; //使能中断CAN0
//    CanaRegs.CAN_CTL.bit.EIE = 0; //不使能中断ERROR
//    CanaRegs.CAN_CTL.bit.SIE = 0; //不使能中断CAN0接收发送
//    CanaRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 1; //使能CANAINT0 PIE中断。
//
//    CanaRegs.CAN_CTL.bit.Init = 1;//初始化CAN
//    CanaRegs.CAN_CTL.bit.CCE = 1; //允许CPU配置一次
//
//    switch(BRP_1M)	//SYSCLKOUT/((BRPreg+1)*(TSEG1+TSEG2+3))
//    {
//        case 0://125k
//        {
//            CanaRegs.CAN_BTR.bit.BRP = 99;     	//BRP = BRPreg + 1
//            CanaRegs.CAN_BTR.bit.BRPE = 0;     	//3 samples
//            CanaRegs.CAN_BTR.bit.SJW = 3;      	//4 TQ
//            CanaRegs.CAN_BTR.bit.TSEG1 = 8;   	//8 TQ, 60% SP
//            CanaRegs.CAN_BTR.bit.TSEG2 = 5;    	//8 TQ
//            break;
//        }
//        case 1://250k
//        {
//            CanaRegs.CAN_BTR.bit.BRP = 49;
//            CanaRegs.CAN_BTR.bit.BRPE = 0;
//            CanaRegs.CAN_BTR.bit.SJW = 3;
//            CanaRegs.CAN_BTR.bit.TSEG1 = 8;
//            CanaRegs.CAN_BTR.bit.TSEG2 = 5;
//            break;
//        }
//        case 2://500k
//        {
//            CanaRegs.CAN_BTR.bit.BRP = 39;
//            CanaRegs.CAN_BTR.bit.BRPE = 0;
//            CanaRegs.CAN_BTR.bit.SJW = 3;
//            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
//            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
//            break;
//        }
//        case 3://1M
//        {
//            CanaRegs.CAN_BTR.bit.BRP = 19;
//            CanaRegs.CAN_BTR.bit.BRPE = 0;
//            CanaRegs.CAN_BTR.bit.SJW = 3;
//            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
//            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
//            break;
//        }
//        default:
//            break;
//    }
//    EDIS;
//}

void InitCanB(void)
{
    EALLOW;
    //==================GPIO20 canB---TX
    GpioCtrlRegs.GPAQSEL2.bit.GPIO20 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO20   = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO20 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 3;
    GpioCtrlRegs.GPADIR.bit.GPIO20  = 1;
    //==================GPIO21 canB---RX
    GpioCtrlRegs.GPAQSEL2.bit.GPIO21 = 3;
    GpioCtrlRegs.GPAPUD.bit.GPIO21   = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO21 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 3;
    GpioCtrlRegs.GPADIR.bit.GPIO21  = 0;
    EDIS;

    EALLOW;
    CanbRegs.CAN_CTL.bit.Init = 1;
    CanbRegs.CAN_RAM_INIT.all = 0x1A;
    while((CanbRegs.CAN_RAM_INIT.all & 0x3F) != 0x25);

    CanbRegs.CAN_CTL.bit.SWR = 1;
    DELAY_US(2);
    CanbRegs.CAN_CTL.bit.CCE = 1;
    CanbRegs.CAN_ABOTR = 10000;
    CanbRegs.CAN_CTL.bit.ABO = 1;
    CanbRegs.CAN_CTL.bit.Init = 1;
    CanbRegs.CAN_CTL.bit.CCE = 1;

    switch(BRP_1M)  //SYSCLKOUT/((BRPreg+1)*(TSEG1+TSEG2+3))
    {
        case BRP_125k:
        {
            CanbRegs.CAN_BTR.bit.BRP = 31;      //BRP = BRPreg + 1
            CanbRegs.CAN_BTR.bit.BRPE = 2;
            CanbRegs.CAN_BTR.bit.SJW = 1;
            CanbRegs.CAN_BTR.bit.TSEG1 = 4;     //8 TQ, 60% SP
            CanbRegs.CAN_BTR.bit.TSEG2 = 3;     //8 TQ
            break;
        }
        case BRP_250k:
        {
            CanbRegs.CAN_BTR.bit.BRP = 15;
            CanbRegs.CAN_BTR.bit.BRPE = 1;
            CanbRegs.CAN_BTR.bit.SJW = 1;
            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        case BRP_500k:
        {
            CanbRegs.CAN_BTR.bit.BRP = 39;
            CanbRegs.CAN_BTR.bit.BRPE = 0;
            CanbRegs.CAN_BTR.bit.SJW = 1;
            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        case BRP_1M:
        {
            CanbRegs.CAN_BTR.bit.BRP = 19;
            CanbRegs.CAN_BTR.bit.BRPE = 0;
            CanbRegs.CAN_BTR.bit.SJW = 1;
            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        default:
            break;
    }
    EDIS;
}
/*************************************************
Function: InitCan
Description: CAN模块初始化
Input: None
Output: None
Frequency: Once
*************************************************/
//void InitCan(void)
//{
//    InitCanGpio();
//    InitCanA();
//
//    EALLOW;
//
//    //邮箱1接收
//    while(CanaRegs.CAN_IF1CMD.bit.Busy);
//    //帧头ID
//    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;//使能
//    CanaRegs.CAN_IF1ARB.bit.Xtd = 1;//0;//0-STD,1-EXT
//    CanaRegs.CAN_IF1ARB.bit.Dir = 0;//0-Rx,1-Tx
//
////    ID屏蔽设置       标准帧需要屏蔽这段为啥?因为之前的配置屏蔽了18-28位中的部分
////    CanaRegs.CAN_IF1MSK.bit.MDir = 1;
////    CanaRegs.CAN_IF1MSK.bit.MXtd = 1;
////    CanaRegs.CAN_IF1MSK.bit.Msk = 0x0;//0x1FFFFFFF;
//    //邮箱设置
//    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8个字节
//    CanaRegs.CAN_IF1MCTL.bit.EoB = 1; //只有一帧
//    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//允许接收中断
//    CanaRegs.CAN_IF1MCTL.bit.UMask = 1;//使能滤波
////    CanaRegs.CAN_IF1MSK.bit.Msk = 0;//默认=0x1FFFFFFF;//0=不屏蔽
//    CanaRegs.CAN_IF1ARB.bit.ID = 0x1555AAA0;
//    CanaRegs.CAN_IF1CMD.all = 0xF40001;
//
//    //17发送邮箱
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    //帧头ID
//    CanaRegs.CAN_IF2ARB.bit.MsgVal = 1;//使能
//    CanaRegs.CAN_IF2ARB.bit.Xtd = 1;//扩展帧
//    CanaRegs.CAN_IF2ARB.bit.Dir = 1;//发送邮箱
//    CanaRegs.CAN_IF2ARB.bit.ID=0xABC;
//
//    //邮箱设置
//    CanaRegs.CAN_IF2MCTL.bit.DLC = 8; //8个字节
//    CanaRegs.CAN_IF2MCTL.bit.EoB = 1; //只有一帧
//    CanaRegs.CAN_IF2MCTL.bit.TxIE = 0;//不能=1，否则会一直进CAN中断
//    CanaRegs.CAN_IF2MCTL.bit.RxIE = 0;
//
//    CanaRegs.CAN_IF2CMD.all = 0xB40011;
//    CanaRegs.CAN_CTL.bit.Init = 0;//CAN模块工作在正常模式
//    CanaRegs.CAN_CTL.bit.CCE = 0; //允许CPU配置一次
//
//    EDIS;
//}

void InitCanOscilloscope(void)
{
    InitCanB();// 数字示波器CAN(GPIO20/21)

    EALLOW;
    // CanB邮箱1
    while(CanbRegs.CAN_IF1CMD.bit.Busy);
    CanbRegs.CAN_IF1MCTL.bit.DLC = 8;
    CanbRegs.CAN_IF1MCTL.bit.EoB = 1;
    CanbRegs.CAN_IF1MCTL.bit.RxIE = 1;
    CanbRegs.CAN_IF1MCTL.bit.UMask = 1;
    CanbRegs.CAN_IF1ARB.bit.ID = 0x1555AAA0;
    CanbRegs.CAN_IF1CMD.all = 0xF00001;

    while(CanbRegs.CAN_IF2CMD.bit.Busy);
    CanbRegs.CAN_IF2ARB.bit.MsgVal = 1;
    CanbRegs.CAN_IF2ARB.bit.Xtd = 1;
    CanbRegs.CAN_IF2ARB.bit.Dir = 1;
    CanbRegs.CAN_IF2ARB.bit.ID=0xABC;

    CanbRegs.CAN_IF2MCTL.bit.DLC = 8;
    CanbRegs.CAN_IF2MCTL.bit.EoB = 1;
    CanbRegs.CAN_IF2MCTL.bit.TxIE = 0;
    CanbRegs.CAN_IF2MCTL.bit.RxIE = 0;

    CanbRegs.CAN_IF2CMD.all = 0xB40011;
    CanbRegs.CAN_CTL.bit.Init = 0;
    CanbRegs.CAN_CTL.bit.CCE = 0;

    CanbRegs.CAN_CTL.bit.SIE = 0;   //Disabled - RxOk, TxOk and LEC can generate an interrupt
    CanbRegs.CAN_CTL.bit.EIE = 0;   //Disabled - PER, BOff and EWarn bits can generate an interrupt
    CanbRegs.CAN_CTL.bit.IE0 = 1;   //连到中断线0
    CanbRegs.CAN_CTL.bit.Init = 0;
    CanbRegs.CAN_CTL.bit.CCE = 0;   //The CPU has no write access to the configuration registers.
    CanbRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 0 ;//连到PIE

    EDIS;
}
//===========================================================================
// End of file.
//==============================================

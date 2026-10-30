//###########################################################################
//
// FILE:    f28003x_can.c
//
// TITLE:
//
//###########################################################################
//
// Included Files
//
#include "f28x_project.h"
#include "device.h"

//*****************************************************************************
void InitCanGpio(void)
{
    EALLOW;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO30 = 3; // Asynch input
    GpioCtrlRegs.GPAPUD.bit.GPIO30 = 0;  //CANARX
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 0x1;

    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;  //CANATX
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0x1;
    EDIS;
}

//*****************************************************************************
void InitCanA(void)
{
    EALLOW;
    CanaRegs.CAN_CTL.bit.Init = 1;//初始化CAN
    CanaRegs.CAN_CTL.bit.PMD = 0x5;//不使能优先级
    CanaRegs.CAN_RAM_INIT.all = 0x1A;//初始化CANRAM
    while((CanaRegs.CAN_RAM_INIT.all & 0x3F) != 0x25);

    CanaRegs.CAN_CTL.bit.SWR = 1;//软件复位一次
    DEVICE_DELAY_US(1);
    CanaRegs.CAN_CTL.bit.CCE = 1; //允许CPU配置一次

    CanaRegs.CAN_ABOTR = 10000;//Auto-Bus-On   功能重启等待时间   时基与CAN时基一致
    CanaRegs.CAN_CTL.bit.ABO = 1;//使能Auto-Bus-On功能   CANH和CANL之间短路或者总线上有错误帧时，会导致CAN出现崩溃停发现象   这里增加can崩溃时的复位重发功能
    CanaRegs.CAN_CTL.bit.IE0 = 1; //使能中断CAN0
    CanaRegs.CAN_CTL.bit.EIE = 0; //不使能中断ERROR
    CanaRegs.CAN_CTL.bit.SIE = 0; //不使能中断CAN0接收发送
    CanaRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 1; //使能CANAINT0 PIE中断。

    CanaRegs.CAN_CTL.bit.Init = 1;//初始化CAN
    CanaRegs.CAN_CTL.bit.CCE = 1; //允许CPU配置一次

    //波特率=120MHz/(BRP*BITtime)
    //采样点（2+TSEG1）/BITtime BITtime=（1+TSEG1）+（1+TSEG2）+1
    switch(3)
    {
        case 0://125k
        {
            //125kbps
            CanaRegs.CAN_BTR.bit.BRP = 59;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 3;
            CanaRegs.CAN_BTR.bit.TSEG1 = 8;
            CanaRegs.CAN_BTR.bit.TSEG2 = 5;
            break;
        }
        case 1://250k
        {
            CanaRegs.CAN_BTR.bit.BRP = 29;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 3;
            CanaRegs.CAN_BTR.bit.TSEG1 = 8;
            CanaRegs.CAN_BTR.bit.TSEG2 = 5;
            break;
        }
        case 2://500k
        {
            CanaRegs.CAN_BTR.bit.BRP = 23;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 3;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        case 3://1M
        {
            CanaRegs.CAN_BTR.bit.BRP = 11;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 3;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        default:
            break;
    }
    EDIS;
}

//*****************************************************************************
extern interrupt void CAN_Read_isr(void);

void InitCan_SoftOscilloscope(void)
{
    InitCanGpio();
    InitCanA();
    EALLOW;
    //邮箱1接收
    while(CanaRegs.CAN_IF1CMD.bit.Busy);
    //帧头ID
    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;//使能
    CanaRegs.CAN_IF1ARB.bit.Xtd = 1;//0;//0-STD,1-EXT
    CanaRegs.CAN_IF1ARB.bit.Dir = 0;//0-Rx,1-Tx

//    ID屏蔽设置       标准帧需要屏蔽这段为啥?因为之前的配置屏蔽了18-28位中的部分
//    CanaRegs.CAN_IF1MSK.bit.MDir = 1;
//    CanaRegs.CAN_IF1MSK.bit.MXtd = 1;
//    CanaRegs.CAN_IF1MSK.bit.Msk = 0x0;//0x1FFFFFFF;
    //邮箱设置
    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8个字节
    CanaRegs.CAN_IF1MCTL.bit.EoB = 1; //只有一帧
    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//允许接收中断
    CanaRegs.CAN_IF1MCTL.bit.UMask = 1;//使能滤波
//    CanaRegs.CAN_IF1MSK.bit.Msk = 0;//默认=0x1FFFFFFF;//0=不屏蔽
    CanaRegs.CAN_IF1ARB.bit.ID = 0x1555AAA0;
    CanaRegs.CAN_IF1CMD.all = 0xF00001;

    //17发送邮箱
    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    //帧头ID
    CanaRegs.CAN_IF2ARB.bit.MsgVal = 1;//使能
    CanaRegs.CAN_IF2ARB.bit.Xtd = 1;//扩展帧
    CanaRegs.CAN_IF2ARB.bit.Dir = 1;//发送邮箱
    CanaRegs.CAN_IF2ARB.bit.ID=0xABC;

    //邮箱设置
    CanaRegs.CAN_IF2MCTL.bit.DLC = 8; //8个字节
    CanaRegs.CAN_IF2MCTL.bit.EoB = 1; //只有一帧
    CanaRegs.CAN_IF2MCTL.bit.TxIE = 0;//不能=1，否则会一直进CAN中断
    CanaRegs.CAN_IF2MCTL.bit.RxIE = 0;

    CanaRegs.CAN_IF2CMD.all = 0xB40011;
    CanaRegs.CAN_CTL.bit.Init = 0;//CAN模块工作在正常模式
    CanaRegs.CAN_CTL.bit.CCE = 0; //允许CPU配置一次
    EDIS;
}

//===========================================================================
// End of file.
//===========================================================================

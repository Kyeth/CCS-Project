/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: F2837xS_Sci.c
Author: tzz
Version : 0.0.2
Date:2022年02月11日
Description: EPWM 配置文件
Function List:
***********************************************************/
#include "bsp.h"

/*************************************************
Function: InitSciaGpio
Description: 初始化SCI的GPIO
Input: None
Output: None
Frequency: Once
*************************************************/
void InitSciaGpio()
{
    EALLOW;

   // SCI--TX-GPIO48
    GpioCtrlRegs.GPBQSEL2.bit.GPIO48 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO48   = 0;
    GpioCtrlRegs.GPBGMUX2.bit.GPIO48 = 1;
    GpioCtrlRegs.GPBMUX2.bit.GPIO48 = 2;
    GpioCtrlRegs.GPBDIR.bit.GPIO48  = 1;
    // SCI--RX-GPIO49
    GpioCtrlRegs.GPBQSEL2.bit.GPIO49 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO49   = 0;
    GpioCtrlRegs.GPBGMUX2.bit.GPIO49 = 1;
    GpioCtrlRegs.GPBMUX2.bit.GPIO49 = 2;
    GpioCtrlRegs.GPBDIR.bit.GPIO49  = 0;

    EDIS;
}
/*************************************************
Function: InitScidGpio
Description: 初始化SCI的GPIO
Input: None
Output: None
Frequency: Once
*************************************************/
void InitScidGpio()
{
    EALLOW;

   // SCI--TX-GPIO93
    GpioCtrlRegs.GPCQSEL2.bit.GPIO93 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO93   = 0;
    GpioCtrlRegs.GPCGMUX2.bit.GPIO93 = 1;
    GpioCtrlRegs.GPCMUX2.bit.GPIO93 = 2;
    GpioCtrlRegs.GPCDIR.bit.GPIO93  = 1;
    // SCI--RX-GPIO94
    GpioCtrlRegs.GPCQSEL2.bit.GPIO94 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO94   = 0;
    GpioCtrlRegs.GPCGMUX2.bit.GPIO94 = 1;
    GpioCtrlRegs.GPCMUX2.bit.GPIO94 = 2;
    GpioCtrlRegs.GPCDIR.bit.GPIO94  = 0;

    EDIS;
}
/*************************************************
Function: InitSci
Description: 初始化SCI
Input: @sci-SCI名称只支持SCIA和SCID
Output: None
Frequency: Once
*************************************************/
void InitSci(char sci)
{
    if (sci == 'A')
    {
        // init scia for arm commu
        InitSciaGpio();

        SciaRegs.SCICCR.all = 0x0007;           //.all的配置与.bit等效
        SciaRegs.SCICCR.bit.STOPBITS = 0;
        SciaRegs.SCICCR.bit.PARITY = 0;         //0-Odd parity; 1-Even parity
        SciaRegs.SCICCR.bit.PARITYENA = 0;      //0-Parity disable; 1-Parity enable
        SciaRegs.SCICCR.bit.LOOPBKENA = 0;      //0-loopback disable; 1-loopback enable
        SciaRegs.SCICCR.bit.ADDRIDLE_MODE = 0;  //0-idle-line; 1-address-bit
        SciaRegs.SCICCR.bit.SCICHAR = 0x7;      //character length = 8

//        SciaRegs.SCIHBAUD.all = 0x0;            // 1M
//        SciaRegs.SCILBAUD.all = 0x005;

        // BRR = (LSPCLK/Baud/8-1)
        SciaRegs.SCIHBAUD.all = 0x0001;         // 19200 baud @LSPCLK = 50M:(LSPCLK/Baud/8-1)
        SciaRegs.SCILBAUD.all = 0x0045;

        SciaRegs.SCICTL1.bit.RXERRINTENA = 0;   //禁用接收错误中断
        SciaRegs.SCICTL1.bit.SWRESET = 0;       //复位SCI和状态寄存器
        SciaRegs.SCICTL1.bit.TXWAKE = 0;        //禁用TXWAKE功能
        SciaRegs.SCICTL1.bit.SLEEP = 0;         //禁用SLEEP功能
        SciaRegs.SCICTL1.bit.TXENA = 1;         //发送使能
        SciaRegs.SCICTL1.bit.RXENA = 1;         //接收使能
        SciaRegs.SCICTL2.all = 0x0003;

        SciaRegs.SCIFFTX.all = 0xE040;            // 使能发送 FIFO
        SciaRegs.SCIFFRX.all = 0x2044;            // 使能接收 FIFO
        SciaRegs.SCIFFCT.all = 0x0;

        SciaRegs.SCICTL1.all = 0x0023;  // Relinquish SCI from Reset
    }
    else if (sci == 'D')
    {
        // init scid for pc debug
        InitScidGpio();

        ScidRegs.SCICCR.all =0x0007;
        ScidRegs.SCICCR.bit.STOPBITS=0;
        ScidRegs.SCICCR.bit.PARITY = 0;         // 0-Odd parity; 1-Even parity
        ScidRegs.SCICCR.bit.PARITYENA = 0;      // 0-Parity disable; 1-Parity enable
        ScidRegs.SCICCR.bit.LOOPBKENA = 0;      // 0-loopback disable; 1-loopback enable
        ScidRegs.SCICCR.bit.ADDRIDLE_MODE = 0;  // 0-idle-line; 1-address-bit
        ScidRegs.SCICCR.bit.SCICHAR = 0x7;      // character length = 8
        ScidRegs.SCIHBAUD.all = 0x0002;         // 9600 baud @LSPCLK = 50M(LSPCLK/9600/8-1)
        ScidRegs.SCILBAUD.all = 0x008A;

        ScidRegs.SCICTL1.bit.RXERRINTENA = 0;   // 禁用接收错误中断
        ScidRegs.SCICTL1.bit.SWRESET = 0;       // 复位SCI和状态寄存器
        ScidRegs.SCICTL1.bit.TXWAKE = 0;        // 禁用TXWAKE功能
        ScidRegs.SCICTL1.bit.SLEEP = 0;         // 禁用SLEEP功能
        ScidRegs.SCICTL1.bit.TXENA = 1;         // 发送使能
        ScidRegs.SCICTL1.bit.RXENA = 1;         // 接收使能
        ScidRegs.SCICTL2.all =0x0003;

        ScidRegs.SCIFFTX.all=0xE040;            // 使能发送 FIFO
        ScidRegs.SCIFFRX.all=0x2044;            // 使能接收 FIFO
        ScidRegs.SCIFFCT.all=0x0;

        ScidRegs.SCICTL1.all =0x0023;          // Relinquish SCI from Reset
    }
}

//
// End of file
//

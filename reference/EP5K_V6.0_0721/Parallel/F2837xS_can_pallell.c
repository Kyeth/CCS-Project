//###########################################################################
//
// FILE:   F2837xS_can.c
//
// TITLE:  F2837xS CAN Support Functions.
//
//###########################################################################
// $TI Release: F2837xS Support Library v3.12.00.00 $
// $Release Date: Fri Feb 12 19:06:50 IST 2021 $
// $Copyright:
// Copyright (C) 2014-2021 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Included Files
//
#include "F2837xS_device.h"
#include "F2837xS_Examples.h"
#include "Parallel.h"
#include <ModbusTable.h>

//
// InitCAN - Initializes the CAN-A controller after reset.
//
#define BRP_1M      3
#define BRP_500k    2
#define BRP_250k    1
#define BRP_125k    0

//
////*****************************************************************************
void InitCanA(void)
{
    uint16_t BotRate = 0;

    EALLOW;
    GpioCtrlRegs.GPBGMUX1.bit.GPIO36 = 0x1;
    GpioCtrlRegs.GPBGMUX1.bit.GPIO37 = 0x1;

    GpioCtrlRegs.GPBMUX1.bit.GPIO36 = 0x2;      //CANARX
    GpioCtrlRegs.GPBMUX1.bit.GPIO37 = 0x2;      //CANATX

    GpioCtrlRegs.GPBQSEL1.bit.GPIO36 = 3;       // Asynch input
    GpioCtrlRegs.GPBPUD.bit.GPIO37 = 0;
    EDIS;

    EALLOW;
    CanaRegs.CAN_CTL.bit.Init = 1;//��ʼ��CAN
    CanaRegs.CAN_RAM_INIT.all = 0x1A;//��ʼ��CANRAM
    while((CanaRegs.CAN_RAM_INIT.all & 0x3F) != 0x25);

    CanaRegs.CAN_CTL.bit.SWR = 1;//�����λһ��
    DELAY_US(2);
    CanaRegs.CAN_CTL.bit.CCE = 1; //����CPU����һ��
    CanaRegs.CAN_ABOTR = 10000;//Auto-Bus-On   ���������ȴ�ʱ��, ʱ����CANʱ��һ��
    CanaRegs.CAN_CTL.bit.ABO = 1;//ʹ��Auto-Bus-On����   CANH��CANL֮���·�����������д���֡ʱ���ᵼ��CAN���ֱ���ͣ������   ��������can����ʱ�ĸ�λ�ط�����

    CanaRegs.CAN_CTL.bit.Init = 1;//��ʼ��CAN
    CanaRegs.CAN_CTL.bit.CCE = 1; //����CPU����һ��

    BotRate = (ArmWriteData.Fault.bit.ATEMode == 1)? BRP_250k:BRP_1M;

    switch(BotRate)
    {
        case BRP_125k:
        {
            CanaRegs.CAN_BTR.bit.BRP = 31;      //BRP = BRPreg + 1
            CanaRegs.CAN_BTR.bit.BRPE = 2;
            CanaRegs.CAN_BTR.bit.SJW = 1;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;     //8 TQ, 60% SP
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;     //8 TQ
            break;
        }
        case BRP_250k:
        {
            CanaRegs.CAN_BTR.bit.BRP = 15;
            CanaRegs.CAN_BTR.bit.BRPE = 1;
            CanaRegs.CAN_BTR.bit.SJW = 1;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        case BRP_500k:
        {
            CanaRegs.CAN_BTR.bit.BRP = 39;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 1;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        case BRP_1M:
        {
            CanaRegs.CAN_BTR.bit.BRP = 19;
            CanaRegs.CAN_BTR.bit.BRPE = 0;
            CanaRegs.CAN_BTR.bit.SJW = 1;
            CanaRegs.CAN_BTR.bit.TSEG1 = 4;
            CanaRegs.CAN_BTR.bit.TSEG2 = 3;
            break;
        }
        default:
            break;
    }
    EDIS;
}
//
//void InitCanB(void)
//{
//    EALLOW;
//    GpioCtrlRegs.GPAGMUX1.bit.GPIO12 = 0;   //  CANTXB (O)
//    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;    //  CANTXB (O)
//    GpioCtrlRegs.GPAGMUX1.bit.GPIO13 = 0;   //  CANRXB (I)
//    GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 2;    //  CANRXB (I)
//    GpioCtrlRegs.GPAQSEL1.bit.GPIO13 = 3;   // Asynch input
//    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;
//    GpioCtrlRegs.GPAPUD.bit.GPIO13 = 0;
//    GpioCtrlRegs.GPADIR.bit.GPIO12  = 0;
//    GpioCtrlRegs.GPADIR.bit.GPIO13  = 1;
//    EDIS;
//
//    EALLOW;
//    CanbRegs.CAN_CTL.bit.Init = 1;//��ʼ��CAN
//    CanbRegs.CAN_RAM_INIT.all = 0x1A;//��ʼ��CANRAM
//    while((CanbRegs.CAN_RAM_INIT.all & 0x3F) != 0x25);
//
//    CanbRegs.CAN_CTL.bit.SWR = 1;//�����λһ��
//    DELAY_US(2);
//    CanbRegs.CAN_CTL.bit.CCE = 1; //����CPU����һ��
//    CanbRegs.CAN_ABOTR = 10000;//Auto-Bus-On   ���������ȴ�ʱ��, ʱ����CANʱ��һ��
//    CanbRegs.CAN_CTL.bit.ABO = 1;//ʹ��Auto-Bus-On����   CANH��CANL֮���·�����������д���֡ʱ���ᵼ��CAN���ֱ���ͣ������   ��������can����ʱ�ĸ�λ�ط�����
//
//    CanbRegs.CAN_CTL.bit.Init = 1;//��ʼ��CAN
//    CanbRegs.CAN_CTL.bit.CCE = 1; //����CPU����һ��
//
//    switch(BRP_1M)  //SYSCLKOUT/((BRPreg+1)*(TSEG1+TSEG2+3))
//    {
//        case BRP_125k:
//        {
//            CanbRegs.CAN_BTR.bit.BRP = 31;      //BRP = BRPreg + 1
//            CanbRegs.CAN_BTR.bit.BRPE = 2;
//            CanbRegs.CAN_BTR.bit.SJW = 1;
//            CanbRegs.CAN_BTR.bit.TSEG1 = 4;     //8 TQ, 60% SP
//            CanbRegs.CAN_BTR.bit.TSEG2 = 3;     //8 TQ
//            break;
//        }
//        case BRP_250k:
//        {
//            CanbRegs.CAN_BTR.bit.BRP = 15;
//            CanbRegs.CAN_BTR.bit.BRPE = 1;
//            CanbRegs.CAN_BTR.bit.SJW = 1;
//            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
//            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
//            break;
//        }
//        case BRP_500k:
//        {
//            CanbRegs.CAN_BTR.bit.BRP = 39;
//            CanbRegs.CAN_BTR.bit.BRPE = 0;
//            CanbRegs.CAN_BTR.bit.SJW = 1;
//            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
//            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
//            break;
//        }
//        case BRP_1M:
//        {
//            CanbRegs.CAN_BTR.bit.BRP = 19;
//            CanbRegs.CAN_BTR.bit.BRPE = 0;
//            CanbRegs.CAN_BTR.bit.SJW = 1;
//            CanbRegs.CAN_BTR.bit.TSEG1 = 4;
//            CanbRegs.CAN_BTR.bit.TSEG2 = 3;
//            break;
//        }
//        default:
//            break;
//    }
//    EDIS;
//
//    EALLOW;
//    //  邮箱1 接收同步帧/不接收其他帧
//    while(CanbRegs.CAN_IF1CMD.bit.Busy);
//    CanbRegs.CAN_IF1MCTL.bit.DLC = 8; //8Byte
//    CanbRegs.CAN_IF1MCTL.bit.EoB = 1; //֡
//    CanbRegs.CAN_IF1MCTL.bit.UMask = 0;// Use Mask (Msk[28:0], MXtd, and MDir) for acceptance filtering
//    //  邮箱16 接收软件示波器
//    while(CanbRegs.CAN_IF1CMD.bit.Busy);
//    CanbRegs.CAN_IF1MCTL.bit.RxIE = 0;//接收连到Int = 0
//    CanbRegs.CAN_IF1ARB.all = 0xD555AAA0;
//    CanbRegs.CAN_IF1CMD.all = 0xF00010;    // 1111    写入 +Mask+ID+控制  / 邮箱16
//    //-----------------------------------------------------------//
//    //   邮箱2  载波同步  主机发
//    while(CanbRegs.CAN_IF2CMD.bit.Busy);
//    CanbRegs.CAN_IF2MCTL.bit.DLC = 8; //8Byte
//    CanbRegs.CAN_IF2MCTL.bit.EoB = 1; // is a single message object or the last object  in a FIFO Buffer Block.
//    CanbRegs.CAN_IF2MCTL.bit.TxIE = 0;
//    //   邮箱8  其他         ID当场配置
//    while(CanbRegs.CAN_IF2CMD.bit.Busy);
//    CanbRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0010; //ID
//    CanbRegs.CAN_IF2CMD.all = 0xB00008;  // 1011     写入 +ID+控制  /  邮箱8
//
////    CanbRegs.CAN_CTL.bit.Test =1;
////    CanbRegs.CAN_TEST.bit.EXL = 1;
////    CanbRegs.CAN_TEST.bit.LBACK = 1;
////    CanbRegs.CAN_TEST.bit.SILENT = 1;
//
//    //   CanbRegs.CAN_CTL.bit.DAR = 1;   //禁止CAN的自动重传 (故障或仲裁失败)
//    CanbRegs.CAN_CTL.bit.SIE = 0;   //Disabled - RxOk, TxOk and LEC can generate an interrupt
//    CanbRegs.CAN_CTL.bit.EIE = 0;   //Disabled - PER, BOff and EWarn bits can generate an interrupt
//    CanbRegs.CAN_CTL.bit.IE0 = 1;   //连到中断线0
//    CanbRegs.CAN_CTL.bit.Init = 0;
//    CanbRegs.CAN_CTL.bit.CCE = 0;   //The CPU has no write access to the configuration registers.
//    CanbRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 0 ;//连到PIE
//    EDIS;
//}
//
////*****************************************************************************
//void InitCan(void)
//{
//    InitCanA();
//    EALLOW;
//    //  邮箱1 接收同步帧/不接收其他帧
////    while(CanaRegs.CAN_IF1CMD.bit.Busy);
////    CanaRegs.CAN_IF1MSK.all = 0xFFFFFFF0;                //屏蔽ID
////    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8Byte
////    CanaRegs.CAN_IF1MCTL.bit.EoB = 1; //֡
////    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//接收连到Int
////    CanaRegs.CAN_IF1MCTL.bit.UMask = 1;// Use Mask (Msk[28:0], MXtd, and MDir) for acceptance filtering
////    CanaRegs.CAN_IF1ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xC000)<<16)+0x0001; //ID
////    CanaRegs.CAN_IF1CMD.all = 0xF00001;    // 1111    写入 +Mask+ID+控制  / 邮箱1
//    //  邮箱9 接收其他帧/不接收同步帧
//    while(CanaRegs.CAN_IF1CMD.bit.Busy);//
//    CanaRegs.CAN_IF1MSK.all = 0xFFFFFF00;
//    CanaRegs.CAN_IF1MCTL.bit.EoB = 0;//使能fifo
//    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//
//    CanaRegs.CAN_IF1ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xC000)<<16)+0x0010; //ID
//    CanaRegs.CAN_IF1CMD.all = 0xF00009;    // 1111    写入 +Mask+ID+控制  / 邮箱9
//    //  邮箱10 接收其他帧/不接收同步帧
//    while(CanaRegs.CAN_IF1CMD.bit.Busy);//
//    CanaRegs.CAN_IF1MSK.all = 0xFFFFFF00;
//    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
//    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//
//    CanaRegs.CAN_IF1ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xC000)<<16)+0x0010; //ID
//    CanaRegs.CAN_IF1CMD.all = 0xF00000A;    // 1111    写入 +Mask+ID+控制  / 邮箱10
//    CanaRegs.CAN_IF3UPD = 0x0300;   //IF3观测邮箱9、10
//    CanaRegs.CAN_IF3OBS.all = 0x1f; //启动Updata
////  邮箱16 接收软件示波器
//    while(CanaRegs.CAN_IF1CMD.bit.Busy);
//    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
//    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//接收连到Int = 0
//    CanaRegs.CAN_IF1ARB.bit.ID = 0xD555AAA0;
//    CanaRegs.CAN_IF1CMD.all = 0xF00010;    // 1111    写入 +Mask+ID+控制  / 邮箱16
////    邮箱1 接收
////    while(CanaRegs.CAN_IF1CMD.bit.Busy);
////    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;//使能
////    CanaRegs.CAN_IF1ARB.bit.Xtd = 1;//0;//0-STD,1-EXT
////    CanaRegs.CAN_IF1ARB.bit.Dir = 0;
////    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8个字节
////    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
////    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//接收连到Int = 0
////    CanaRegs.CAN_IF1ARB.bit.ID = 0x1555AAA0;
////    CanaRegs.CAN_IF1CMD.all = 0xF40001;    // 1111    写入 +Mask+ID+控制  / 邮箱16
//
//    //-----------------------------------------------------------//
//    //   邮箱2  载波同步  主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2MCTL.bit.DLC = 8; //8Byte
//    CanaRegs.CAN_IF2MCTL.bit.EoB = 1; // is a single message object or the last object  in a FIFO Buffer Block.
//    CanaRegs.CAN_IF2MCTL.bit.TxIE = 0;
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0001; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00002;  // 1011     写入 +ID+控制  /  邮箱2
//    //   邮箱3  工频同步  主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0003; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00003;  // 1011     写入 +ID+控制  /  邮箱3
//    //   邮箱4  继电器同步  主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0004; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00004;  // 1011     写入 +ID+控制  /  邮箱4
//    //   邮箱5  模式切换   主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0005; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00005;  // 1011     写入 +ID+控制  /  邮箱5
//    //   邮箱7  其他         主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0015; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00007;  // 1011     写入 +ID+控制  /  邮箱7
//    //   邮箱8  其他         ID当场配置
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0010; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00008;  // 1011     写入 +ID+控制  /  邮箱8
//
//    //CanaRegs.CAN_CTL.bit.DAR = 1; // 1 = 禁止CAN的自动重传 (故障或仲裁失败)
//    //CanaRegs.CAN_CTL.bit.SIE = 0;   // 1 = Enabled - RxOk, TxOk and LEC can generate an interrupt
//    //CanaRegs.CAN_CTL.bit.EIE = 1;   // 1 = Enabled - PER, BOff and EWarn bits can generate an interrupt
//    CanaRegs.CAN_CTL.bit.IE0 = 1;   //连到中断线0
//    CanaRegs.CAN_CTL.bit.Init = 0;
//    CanaRegs.CAN_CTL.bit.CCE = 0;   //The CPU has no write access to the configuration registers.
//    CanaRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 1 ;//连到PIE
//    EDIS;
//
//    //InitCanB();
//}

void InitParallelCan(void)
{
    InitCanA();
    EALLOW;
//    //  邮箱8 接收同步帧/不接收其他帧
    while(CanaRegs.CAN_IF1CMD.bit.Busy);
    CanaRegs.CAN_IF1MSK.all = 0xFFC3FFFF;                //屏蔽ID
    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8Byte
    CanaRegs.CAN_IF1MCTL.bit.EoB = 1; //֡
    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//接收连到Int
    CanaRegs.CAN_IF1MCTL.bit.UMask = 1;// Use Mask (Msk[28:0], MXtd, and MDir) for acceptance filtering
    CanaRegs.CAN_IF1ARB.all= ((uint32_t)CmdPhaseSync) << 18U;
    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;
    CanaRegs.CAN_IF1CMD.all = 0xF00008;    // 1111    写入 +Mask+ID+控制  / 邮箱8
    //  邮箱9 接收其他帧/不接收同步帧
    while(CanaRegs.CAN_IF1CMD.bit.Busy);//
    CanaRegs.CAN_IF1MSK.all = 0x0;
//    CanaRegs.CAN_IF1MSK.bit.MDir = 0;
//    CanaRegs.CAN_IF1MSK.all = 0xFFFFFF00;
    CanaRegs.CAN_IF1MCTL.bit.EoB = 0;//使能fifo
    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//
    CanaRegs.CAN_IF1ARB.all = ((uint32_t)CmdWithId) << 18U;
    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;
    CanaRegs.CAN_IF1CMD.all = 0x00F00009;    // 1111    写入 +Mask+ID+控制  / 邮箱9
    //  邮箱10 接收其他帧/不接收同步帧
    while(CanaRegs.CAN_IF1CMD.bit.Busy);//
    CanaRegs.CAN_IF1MSK.all = 0x0;
//    CanaRegs.CAN_IF1MSK.all = 0xE0000000;
    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//
//    CanaRegs.CAN_IF1ARB.all=((Uint32)((0x0000 & 0x0FFF)|0x8000)<<16)+0x0000; //ID
    CanaRegs.CAN_IF1CMD.all = 0x00F0000A;    // 1111    写入 +Mask+ID+控制  / 邮箱10
    CanaRegs.CAN_IF3UPD = 0x0300;   //IF3观测邮箱9、10
    CanaRegs.CAN_IF3OBS.all = 0x1f; //启动Updata
    //  邮箱16 接收软件示波器
//    while(CanaRegs.CAN_IF1CMD.bit.Busy);
//    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
//    CanaRegs.CAN_IF1MCTL.bit.RxIE = 0;//接收连到Int = 0
//    CanaRegs.CAN_IF1ARB.bit.ID = 0xD555AAA0;
//    CanaRegs.CAN_IF1CMD.all = 0xF00010;    // 1111    写入 +Mask+ID+控制  / 邮箱16
    //    邮箱1 接收
    //    while(CanaRegs.CAN_IF1CMD.bit.Busy);
    //    CanaRegs.CAN_IF1ARB.bit.MsgVal = 1;//使能
    //    CanaRegs.CAN_IF1ARB.bit.Xtd = 1;//0;//0-STD,1-EXT
    //    CanaRegs.CAN_IF1ARB.bit.Dir = 0;
    //    CanaRegs.CAN_IF1MCTL.bit.DLC = 8; //8个字节
    //    CanaRegs.CAN_IF1MCTL.bit.EoB = 1;//禁止fifo
    //    CanaRegs.CAN_IF1MCTL.bit.RxIE = 1;//接收连到Int = 0
    //    CanaRegs.CAN_IF1ARB.bit.ID = 0x1555AAA0;
    //    CanaRegs.CAN_IF1CMD.all = 0xF40001;    // 1111    写入 +Mask+ID+控制  / 邮箱16

    //-----------------------------------------------------------//
    //   邮箱2  载波同步  主机发
    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2MCTL.bit.DLC = 8; //8Byte
    CanaRegs.CAN_IF2MCTL.bit.EoB = 1; // is a single message object or the last object  in a FIFO Buffer Block.
    CanaRegs.CAN_IF2MCTL.bit.TxIE = 0;
    CanaRegs.CAN_IF2ARB.bit.MsgVal = 1;
    CanaRegs.CAN_IF2ARB.bit.Dir = 1;
    CanaRegs.CAN_IF2ARB.bit.Xtd = 0;
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)CmdPwmSync) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xB00000 + CmdPwmSync);  // 1011 =  写入 +ID+控制  /  邮箱
    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)CmdPhaseSync) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xB00000 + CmdPhaseSync);  // 1011     写入 +ID+控制  /  邮箱
    while(CanaRegs.CAN_IF2CMD.bit.Busy);
    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)CmdWithId) << 18U;
    CanaRegs.CAN_IF2CMD.all = (0xB00000 + CmdWithId);  // 1011     写入 +ID+控制  /  邮箱

//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.bit.ID =  ((uint32_t)0x0003) << 18U;
////    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0003; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00003;  // 1011     写入 +ID+控制  /  邮箱3
//    //   邮箱4  继电器同步  主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0004; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00004;  // 1011     写入 +ID+控制  /  邮箱4
//    //   邮箱5  模式切换   主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0005; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00005;  // 1011     写入 +ID+控制  /  邮箱5
//    //   邮箱7  其他         主机发
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0015; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00007;  // 1011     写入 +ID+控制  /  邮箱7
    //   邮箱8  其他         ID当场配置
//    while(CanaRegs.CAN_IF2CMD.bit.Busy);
//    CanaRegs.CAN_IF2ARB.all=((Uint32)((0x0000 & 0x0FFF)|0xE000)<<16)+0x0010; //ID
//    CanaRegs.CAN_IF2CMD.all = 0xB00008;  // 1011     写入 +ID+控制  /  邮箱8

    //CanaRegs.CAN_CTL.bit.DAR = 1; // 1 = 禁止CAN的自动重传 (故障或仲裁失败)
    //CanaRegs.CAN_CTL.bit.SIE = 0;   // 1 = Enabled - RxOk, TxOk and LEC can generate an interrupt
    //CanaRegs.CAN_CTL.bit.EIE = 1;   // 1 = Enabled - PER, BOff and EWarn bits can generate an interrupt
    CanaRegs.CAN_CTL.bit.IE0 = 1;   //连到中断线0
    CanaRegs.CAN_CTL.bit.Init = 0;
    CanaRegs.CAN_CTL.bit.CCE = 0;   //The CPU has no write access to the configuration registers.
    CanaRegs.CAN_GLB_INT_EN.bit.GLBINT0_EN = 1 ;//连到PIE
    EDIS;
//    CanbRegs.CAN_TEST.bit.RX = 1;
}
//===========================================================================
// End of file.
//==============================================

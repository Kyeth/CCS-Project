/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: F2837xS_ECap.c
Function List:
***********************************************************/
#include "F2837xS_device.h"     // F2837xS Headerfile Include File
#include "F2837xS_Examples.h"   // F2837xS Examples Include File

void InitECapGpio()
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 0;
    InputXbarRegs.INPUT7SELECT = 24;      // INPUT7-->eCAP1
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 0;
    InputXbarRegs.INPUT8SELECT = 25;      // INPUT8-->eCAP2
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0 ;
    InputXbarRegs.INPUT10SELECT = 26;     // INPUT10-->eCAP4
    GpioCtrlRegs.GPAPUD.bit.GPIO27 = 0 ;
    InputXbarRegs.INPUT11SELECT = 27;     // INPUT11-->eCAP5
    GPIO_SetupPinOptions(24, GPIO_INPUT, GPIO_QUAL3);
    GPIO_SetupPinOptions(25, GPIO_INPUT, GPIO_QUAL3);
    GPIO_SetupPinOptions(26, GPIO_INPUT, GPIO_QUAL3);
    GPIO_SetupPinOptions(27, GPIO_INPUT, GPIO_QUAL3);
//    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 2;   // 3 samples
//    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 2;   // 3 samples

    EDIS;
}

void InitECap(void)
{
    InitECapGpio();

//    GPIO_SetupPinOptions(24, GPIO_INPUT, GPIO_ASYNC);
//    GPIO_SetupPinOptions(25, GPIO_INPUT, GPIO_ASYNC);

    //===================cap1
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 0;        // Make sure the counter is stopped
    ECap1Regs.ECEINT.all = 0x0000;             // Disable all capture __interrupts

    ECap1Regs.ECCTL1.bit.PRESCALE = 0;         // 每个载波触发一次
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 0;      // One-shot
    ECap1Regs.ECCTL2.bit.STOP_WRAP = 0;        // Stop at 1 events

    ECap1Regs.ECCTL1.bit.CAP1POL = 1;          // Falling edge

    ECap1Regs.ECCTL2.bit.SYNCI_EN = 0;         // Enable sync in      禁止
    ECap1Regs.ECCTL2.bit.SYNCO_SEL = 3;        // Pass through        禁止

    ECap1Regs.ECCLR.all = 0xFFFF;              // Clear all CAP interrupt flags

    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;          // Enable capture units
    ECap1Regs.ECCTL2.bit.REARM = 1;            // arm one-shot
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;        // Start Counter
    //===================cap2
    ECap2Regs.ECCTL2.bit.TSCTRSTOP = 0;        // Make sure the counter is stopped
    ECap2Regs.ECEINT.all = 0x0000;             // Disable all capture __interrupts

    ECap2Regs.ECCTL1.bit.PRESCALE = 0;         // 每个载波触发一次
    ECap2Regs.ECCTL2.bit.CONT_ONESHT = 0;      // One-shot
    ECap2Regs.ECCTL2.bit.STOP_WRAP = 0;        // Stop at 1 events
    ECap2Regs.ECCTL1.bit.CAP1POL = 1;          // 1:Falling edge  0:Rising edge
    ECap2Regs.ECCTL2.bit.SYNCI_EN = 0;         // Enable sync in      禁止
    ECap2Regs.ECCTL2.bit.SYNCO_SEL = 3;        // Pass through        禁止

    ECap2Regs.ECCLR.all = 0xFFFF;              // Clear all CAP interrupt flags

    ECap2Regs.ECCTL1.bit.CAPLDEN = 1;          // Enable capture units
    ECap2Regs.ECCTL2.bit.REARM = 1;            // arm one-shot
    ECap2Regs.ECCTL2.bit.TSCTRSTOP = 1;        // Start Counter
    //===================cap4
    ECap4Regs.ECCTL2.bit.TSCTRSTOP = 0;        // Make sure the counter is stopped
    ECap4Regs.ECEINT.all = 0x0000;             // Disable all capture __interrupts

    ECap4Regs.ECCTL1.bit.PRESCALE = 0;         // 每个载波触发一次
    ECap4Regs.ECCTL2.bit.CONT_ONESHT = 0;      // One-shot
    ECap4Regs.ECCTL2.bit.STOP_WRAP = 0;        // Stop at 1 events

    ECap4Regs.ECCTL1.bit.CAP1POL = 1;          // Falling edge

    ECap4Regs.ECCTL2.bit.SYNCI_EN = 0;         // Enable sync in      禁止
    ECap4Regs.ECCTL2.bit.SYNCO_SEL = 3;        // Pass through        禁止

    ECap4Regs.ECCLR.all = 0xFFFF;              // Clear all CAP interrupt flags

    ECap4Regs.ECCTL1.bit.CAPLDEN = 1;          // Enable capture units
    ECap4Regs.ECCTL2.bit.REARM = 1;            // arm one-shot
    ECap4Regs.ECCTL2.bit.TSCTRSTOP = 1;        // Start Counter
    //===================cap5
    ECap5Regs.ECCTL2.bit.TSCTRSTOP = 0;        // Make sure the counter is stopped
    ECap5Regs.ECEINT.all = 0x0000;             // Disable all capture __interrupts

    ECap5Regs.ECCTL1.bit.PRESCALE = 0;         // 每个载波触发一次
    ECap5Regs.ECCTL2.bit.CONT_ONESHT = 0;      // One-shot
    ECap5Regs.ECCTL2.bit.STOP_WRAP = 0;        // Stop at 1 events
    ECap5Regs.ECCTL1.bit.CAP1POL = 1;          // 1:Falling edge  0:Rising edge
    ECap5Regs.ECCTL2.bit.SYNCI_EN = 0;         // Enable sync in      禁止
    ECap5Regs.ECCTL2.bit.SYNCO_SEL = 3;        // Pass through        禁止

    ECap5Regs.ECCLR.all = 0xFFFF;              // Clear all CAP interrupt flags

    ECap5Regs.ECCTL1.bit.CAPLDEN = 1;          // Enable capture units
    ECap5Regs.ECCTL2.bit.REARM = 1;            // arm one-shot
    ECap5Regs.ECCTL2.bit.TSCTRSTOP = 1;        // Start Counter
}
//===========================================================================
// End of file.
//===========================================================================

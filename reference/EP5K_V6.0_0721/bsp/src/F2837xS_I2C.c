//###########################################################################
//
// FILE:   F2837xS_I2C.c
//
// TITLE:  F2837xS I2C Initialization & Support Functions.
//
//###########################################################################
// $TI Release: F2837xS Support Library v180 $
// $Release Date: Fri Nov  6 16:27:58 CST 2015 $
// $Copyright: Copyright (C) 2014-2015 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

#include "F2837xS_device.h"     // F2837xS Headerfile Include File
#include "F2837xS_Examples.h"   // F2837xS Examples Include File

//---------------------------------------------------------------------------
// Example: I2cAGpioConfig(), I2cBGpioConfig()
//---------------------------------------------------------------------------
// These functions configures GPIO pins to function as I2C pins
//
// Each GPIO pin can be configured as a GPIO pin or up to 3 different
// peripheral functional pins. By default all pins come up as GPIO
// inputs after reset.
//
void I2cAGpioConfig(Uint16 I2caDataClkPin)
{
    EALLOW;

    switch(I2caDataClkPin)
    {
        case I2C_A_GPIO0_GPIO1:
            /* Enable internal pull-up for the selected I2C pins */
            GpioCtrlRegs.GPAPUD.bit.GPIO0 = 0;     // Enable pull-up for GPIO0 (SDAA)
            GpioCtrlRegs.GPAPUD.bit.GPIO1 = 0;     // Enable pull-up for GPIO1 (SDLA)

            /* Set qualification for the selected I2C pins */
            GpioCtrlRegs.GPAQSEL1.bit.GPIO0 = 3;   // Async/no qualification (I/ps sync
            GpioCtrlRegs.GPAQSEL1.bit.GPIO1 = 3;   // to SYSCLKOUT by default)

            /* Configure which of the possible GPIO pins will be I2C_A pins using GPIO regs*/
            GpioCtrlRegs.GPAGMUX1.bit.GPIO0 = 1;    // Configure GPIO0 for SDAA operation
            GpioCtrlRegs.GPAMUX1.bit.GPIO0  = 2;    // Configure GPIO0 for SDAA operation

            GpioCtrlRegs.GPAGMUX1.bit.GPIO1 = 1;    // Configure GPIO1 for SDLA operation
            GpioCtrlRegs.GPAMUX1.bit.GPIO1  = 2;    // Configure GPIO1 for SDLA operation

            break;


        case I2C_A_GPIO32_GPIO33:
            /* Enable internal pull-up for the selected I2C pins */
            GpioCtrlRegs.GPBPUD.bit.GPIO32 = 0;
            GpioCtrlRegs.GPBPUD.bit.GPIO33 = 0;

            /* Set qualification for the selected I2C pins */
            GpioCtrlRegs.GPBQSEL1.bit.GPIO32 = 3;
            GpioCtrlRegs.GPBQSEL1.bit.GPIO33 = 3;

            /* Configure which of the possible GPIO pins will be I2C_A pins using GPIO regs*/
            GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 1;
            GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 1;

            break;

        case I2C_A_GPIO42_GPIO43:
            /* Enable internal pull-up for the selected I2C pins */

            /* Set qualification for the selected I2C pins */

            /* Configure which of the possible GPIO pins will be I2C_A pins using GPIO regs*/
            break;

        case I2C_A_GPIO91_GPIO92:
            /* Enable internal pull-up for the selected I2C pins */
            GpioCtrlRegs.GPCPUD.bit.GPIO91 = 0;
            GpioCtrlRegs.GPCPUD.bit.GPIO92 = 0;

            /* Set qualification for the selected I2C pins */
            GpioCtrlRegs.GPCQSEL2.bit.GPIO91 = 3;
            GpioCtrlRegs.GPCQSEL2.bit.GPIO92 = 3;

            /* Configure which of the possible GPIO pins will be I2C_A pins using GPIO regs*/

            GpioCtrlRegs.GPCGMUX2.bit.GPIO91 = 1;   GpioCtrlRegs.GPCMUX2.bit.GPIO91 = 2;
            GpioCtrlRegs.GPCGMUX2.bit.GPIO92 = 1;   GpioCtrlRegs.GPCMUX2.bit.GPIO92 = 2;


            // WP 输出
            GpioDataRegs.GPCCLEAR.bit.GPIO90 = 1;   // 设置初始状态为低电平
            GpioCtrlRegs.GPCPUD.bit.GPIO90 = 0;     // PUSHPULL
            GpioCtrlRegs.GPCMUX2.bit.GPIO90 = 0;    // GPIO
            GpioCtrlRegs.GPCDIR.bit.GPIO90 = 1;     // output

            break;

        case I2C_A_GPIO63104_GPIO105:
            /* Enable internal pull-up for the selected I2C pins */

            /* Set qualification for the selected I2C pins */

            /* Configure which of the possible GPIO pins will be I2C_A pins using GPIO regs*/
            break;

        default:

            break;

    } // End of Switch
    EDIS;
} //////////// End of I2cAGpioConfig ///////////////////////////////////////

void InitI2C(void)
{
    I2cAGpioConfig(I2C_A_GPIO91_GPIO92);

    EALLOW;
    I2caRegs.I2CMDR.bit.IRS = 0;
    I2caRegs.I2COAR.all = 0x01;                 // 0为7位地址模式;1为10位地址模式
    // 400K
    I2caRegs.I2CPSC.all = 19;                   // 10MHz Prescaler - need 7-12 Mhz on module clk
    I2caRegs.I2CCLKL = 10;                      // NOTE: must be non zero
    I2caRegs.I2CCLKH = 5;                       // NOTE: must be non zero
    I2caRegs.I2CIER.all =0;                     // Disable SCD & ARDY interrupts
    I2caRegs.I2CMDR.all =0x0020;                // Take I2C out of reset
                                                // Stop I2C when suspended
    I2caRegs.I2CFFTX.all = 0x6000;              // Enable FIFO mode and TXFIFO
    I2caRegs.I2CFFRX.all = 0x2000;              // Enable RXFIFO, clear RXFFINT
    EDIS;
}

//===========================================================================
// End of file.
//===========================================================================

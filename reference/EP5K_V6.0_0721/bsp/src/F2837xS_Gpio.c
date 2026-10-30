//###########################################################################
//
// FILE:    F2837xS_Gpio.c
//
// TITLE:    GPIO module support functions
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

//
// InitGpio - Sets all pins to be muxed to GPIO in input mode with pull-ups
//            enabled. Also disables open drain and polarity inversion and sets
//            the qualification to synchronous. Also unlocks all GPIOs
//
void InitGpio()
{
    volatile Uint32 *gpioBaseAddr;
    Uint16 regOffset;

    //
    //Disable pin locks
    //
    EALLOW;
    GpioCtrlRegs.GPALOCK.all = 0x00000000;
    GpioCtrlRegs.GPBLOCK.all = 0x00000000;
    GpioCtrlRegs.GPCLOCK.all = 0x00000000;
    GpioCtrlRegs.GPDLOCK.all = 0x00000000;
    GpioCtrlRegs.GPELOCK.all = 0x00000000;
    GpioCtrlRegs.GPFLOCK.all = 0x00000000;

    //
    //Fill all registers with zeros. Writing to each register separately
    //for six GPIO modules would make this function *very* long. Fortunately,
    //we'd be writing them all with zeros anyway, so this saves a lot of space.
    //
    gpioBaseAddr = (Uint32 *)&GpioCtrlRegs;
    for (regOffset = 0; regOffset < sizeof(GpioCtrlRegs)/2; regOffset++)
    {
        //
        //Hack to avoid enabling pull-ups on all pins. GPyPUD is offset
        //0x0C in each register group of 0x40 words. Since this is a
        //32-bit pointer, the addresses must be divided by 2.
        //
        if (regOffset % (0x40/2) != (0x0C/2))
        {
            gpioBaseAddr[regOffset] = 0x00000000;
        }
    }

    gpioBaseAddr = (Uint32 *)&GpioDataRegs;
    for (regOffset = 0; regOffset < sizeof(GpioDataRegs)/2; regOffset++)
    {
        gpioBaseAddr[regOffset] = 0x00000000;
    }

    EDIS;
}
/******************************************************************************
// Export Function-Prototype: InitDIDO
// Description  : 初始化数字量输入输出,该函数用户自己实现;
// * 输出引脚配置为推完输出,输入引脚配置为上拉输入
// Input        : None
// Output       : None
// Return Value : None
// Modification : Created Function
******************************************************************************/
void InitDIDO(void)
{
    EALLOW;

    // TODO config output
    //===================GPIO22 作为TEST脚
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;   // All outputs
    //==================GPIO32 与门关断同步PWM驱动信号
    GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 1;   // All outputs
    GpioCtrlRegs.GPBPUD.bit.GPIO32 = 1;

    GpioCtrlRegs.GPBCSEL2.bit.GPIO42 = 1;
    GpioCtrlRegs.GPBCSEL2.bit.GPIO46 = 1;
    GpioCtrlRegs.GPBCSEL2.bit.GPIO47 = 1;
    //==================GPIO42 多路选择器C-0 S2
    GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO42 = 1;   // All outputs
    //==================GPIO46 多路选择器A-0 S0
    GpioCtrlRegs.GPBMUX1.bit.GPIO46 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO46 = 1;   // All outputs
    //==================GPIO47 多路选择器B-0 S1
    GpioCtrlRegs.GPBMUX1.bit.GPIO47 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO47 = 1;   // All outputs
    //==================GPIO50 第1路PV正极绝缘检测继电器驱动
    GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO50 = 1;   // All outputs
    //==================GPIO51 第2路PV正极绝缘检测继电器驱动
    GpioCtrlRegs.GPBMUX2.bit.GPIO51 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO51 = 1;   // All outputs
    //==================GPIO52 用于内循环风扇控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO52 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO52 = 1;   // All outputs
    //==================GPIO58 用于L1相并网继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO58 = 1;   // All outputs
    //==================GPIO59 用于L2相中间继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO59 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO59 = 1;   // All outputs
    //==================GPIO60 用于L1相中间继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO60 = 1;   // All outputs
    //==================GPIO61 用于L2相并网继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO61 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO61 = 1;   // All outputs
    //==================GPIO62 用于N相中间继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO62 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO62 = 1;   // All outputs
    //==================GPIO63 用于N相并网继电器控制
    GpioCtrlRegs.GPBMUX2.bit.GPIO63 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO63 = 1;   // All outputs
    //==================GPIO66 用于N相逆变继电器控制
    GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1;   // All outputs
    //==================GPIO67 用于火线逆变继电器控制
    GpioCtrlRegs.GPCMUX1.bit.GPIO67 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1;   // All outputs
    //==================GPIO68 用于火线逆变继电器控制
    GpioCtrlRegs.GPCMUX1.bit.GPIO68 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO68 = 1;   // All outputs
    //==================GPIO69 用于驱动供电使能
    GpioCtrlRegs.GPCMUX1.bit.GPIO69 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO69 = 1;   // All outputs
    //==================GPIO70 接地继电器
    GpioCtrlRegs.GPCMUX1.bit.GPIO70 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO70 = 1;   // All outputs
    //==================GPIO71 漏电流检测使能
    GpioCtrlRegs.GPCMUX1.bit.GPIO71 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO71 = 1;   // All outputs
    //==================GPIO73 BUS预充继电器
    GpioCtrlRegs.GPCMUX1.bit.GPIO73 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO73 = 1;   // All  outputs
    //==================GPIO74 AUS继电器-用于M3辅源继电器控制
    GpioCtrlRegs.GPCMUX1.bit.GPIO74 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO74 = 1;   // All  outputs
    //==================GPIO75 DSP OK信号 表示DSP可以握手
    GpioCtrlRegs.GPCMUX1.bit.GPIO75 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO75 = 1;   // All  outputs
    //==================GPIO87  激活电池使能信号
    GpioCtrlRegs.GPCMUX2.bit.GPIO87 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO87 = 1;   // All outputs
    //==================GPIO99 过流基准控制（用于降低PWM功耗后轻载降低短路阈值）
    GpioCtrlRegs.GPDMUX1.bit.GPIO99 = 0;  // All GPIO
    GpioCtrlRegs.GPDDIR.bit.GPIO99 = 1;   // All outputs

    // TODO config input
    //==================GPIO11 内部风扇故障信号
    GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;   // All inputs
    GpioCtrlRegs.GPAQSEL1.bit.GPIO11 = 1;   // 3 samples
    //==================GPIO28 PV1反灌过流
    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO28 = 0;   // All inputs
    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 1;   // 3 samples
    //==================GPIO29 PV2反灌过流
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO29 = 0;   // All inputs
    GpioCtrlRegs.GPAQSEL2.bit.GPIO29 = 1;   // 3 samples
    //==================GPIO30 PV3反灌过流
    GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO30 = 0;   // All inputs
    GpioCtrlRegs.GPAQSEL2.bit.GPIO30 = 1;   // 3 samples
    //==================GPIO31 PV4反灌过流
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 0;     // 0-PULLUP
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;  // All GPIO
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 0;   // All inputs
    GpioCtrlRegs.GPAQSEL2.bit.GPIO31 = 1;   // 3 samples
    //==================GPIO38 PV4过流

    //==================GPIO39 PV1过流

    //==================GPIO40 PV2过流

    //==================GPIO41 PV3过流

    //==================GPIO76 MCU-ARM  OK输入
    GpioCtrlRegs.GPCMUX1.bit.GPIO76 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO76 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL1.bit.GPIO76 = 1;   // 3 samples
    //==================GPIO77 平衡电路过流输入
    GpioCtrlRegs.GPCMUX1.bit.GPIO77 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO77 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL1.bit.GPIO77 = 1;   // 3 samples
    //==================GPIO78 外部风扇故障输入
    GpioCtrlRegs.GPCMUX1.bit.GPIO78 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO78 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL1.bit.GPIO78 = 1;   // 3 samples
    //==================GPIO79 L1相逆变过流(A)砍波保护

    //==================GPIO80 硬件BUS过压输入
    GpioCtrlRegs.GPCMUX2.bit.GPIO80 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO80 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL2.bit.GPIO80 = 1;   // 3 samples
    //==================GPIO81 L2相逆变过流(A)砍波保护

    //==================GPIO83 逆变不可控过流(A)短路封波

    //==================GPIO86 12V辅助电源欠压(10.08V)告警
    GpioCtrlRegs.GPCPUD.bit.GPIO86 = 0;     // 0-PULLUP
    GpioCtrlRegs.GPCMUX2.bit.GPIO86 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO86 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL2.bit.GPIO86 = 1;   // 3 samples

    EDIS;
}

//
// GPIO_SetupPinMux - Set the peripheral muxing for the specified pin. The
//                    appropriate parameters can be found in the GPIO Muxed
//                    Pins table in the datasheet. Use the GPIO index row
//                   (0 to 15) to select a muxing option for the GPIO.
//
void GPIO_SetupPinMux(Uint16 pin, Uint16 cpu, Uint16 peripheral)
{
    volatile Uint32 *gpioBaseAddr;
    volatile Uint32 *mux, *gmux, *csel;
    Uint16 pin32, pin16, pin8;

    pin32 = pin % 32;
    pin16 = pin % 16;
    pin8 = pin % 8;
    gpioBaseAddr = (Uint32 *)&GpioCtrlRegs + (pin/32)*GPY_CTRL_OFFSET;

    //
    //Sanity check for peripheral values
    //
    if (peripheral > 0xF)
    {
        return;
    }

    //
    //Create pointers to the appropriate registers. This is a workaround
    //for the way GPIO registers are defined. The standard definition
    //in the header file makes it very easy to do named accesses of one
    //register or bit, but hard to do arbitrary numerical accesses. It's
    //easier to have an array of GPIO modules with identical registers,
    //including arrays for multi-register groups like GPyCSEL1-4. But
    //the header file doesn't define anything we can turn into an array,
    //so manual pointer arithmetic is used instead.
    //
    mux = gpioBaseAddr + GPYMUX + pin32/16;
    gmux = gpioBaseAddr + GPYGMUX + pin32/16;
    csel = gpioBaseAddr + GPYCSEL + pin32/8;

    //
    //Now for the actual function
    //
    EALLOW;

    //
    //To change the muxing, set the peripheral mux to 0/GPIO first to avoid
    //glitches, then change the group mux, then set the peripheral mux to
    //its target value. Finally, set the CPU select. This procedure is
    //described in the TRM. Unfortunately, since we don't know the pin in
    //advance we can't hardcode a bitfield reference, so there's some tricky
    //bit twiddling here.
    //
    *mux &= ~(0x3UL << (2*pin16));
    *gmux &= ~(0x3UL << (2*pin16));
    *gmux |= (Uint32)((peripheral >> 2) & 0x3UL) << (2*pin16);
    *mux |= (Uint32)(peripheral & 0x3UL) << (2*pin16);

    *csel &= ~(0x3L << (4*pin8));
    *csel |= (Uint32)(cpu & 0x3L) << (4*pin8);

    //
    //WARNING: This code does not touch the analog mode select registers,
    //which are needed to give the USB module control of its IOs.
    //

    EDIS;
}

//
// GPIO_SetupPinOptions - Setup up the GPIO input/output options for the
//                        specified pin.
//                        The flags are a 16-bit mask produced by ORing
//                        together options.
//                        For input pins, the valid flags are:
//                        GPIO_PULLUP  Enable pull-up
//                        GPIO_INVERT  Enable input polarity inversion
//                        GPIO_SYNC    Synchronize the input latch to PLLSYSCLK
//                                     (default-you don't need to specify this)
//                        GPIO_QUAL3   Use 3-sample qualification
//                        GPIO_QUAL6   Use 6-sample qualification
//                        GPIO_ASYNC   Do not use synchronization or
//                                     qualification
//                        (Note: only one of SYNC, QUAL3, QUAL6, or ASYNC is
//                         allowed)
//
//                        For output pins, the valid flags are:
//                        GPIO_OPENDRAIN   Output in open drain mode
//                        GPIO_PULLUP      If open drain enabled, also enable
//                                         the pull-up and the input
//                                         qualification flags
//                                        (SYNC/QUAL3/QUAL6/SYNC) listed above.
//
//                        With no flags, the default input state is synchronous
//                        with no pull-up or polarity inversion. The default
//                        output state is the standard digital output.
//
void GPIO_SetupPinOptions(Uint16 pin, Uint16 output, Uint16 flags)
{
    volatile Uint32 *gpioBaseAddr;
    volatile Uint32 *dir, *pud, *inv, *odr, *qsel;
    Uint32 pin32, pin16, pinMask, qual;

    pin32 = pin % 32;
    pin16 = pin % 16;
    pinMask = 1UL << pin32;
    gpioBaseAddr = (Uint32 *)&GpioCtrlRegs + (pin/32)*GPY_CTRL_OFFSET;

    //
    //Create pointers to the appropriate registers. This is a workaround
    //for the way GPIO registers are defined. The standard definition
    //in the header file makes it very easy to do named accesses of one
    //register or bit, but hard to do arbitrary numerical accesses. It's
    //easier to have an array of GPIO modules with identical registers,
    //including arrays for multi-register groups like GPyQSEL1-2. But
    //the header file doesn't define anything we can turn into an array,
    //so manual pointer arithmetic is used instead.
    //
    dir = gpioBaseAddr + GPYDIR;
    pud = gpioBaseAddr + GPYPUD;
    inv = gpioBaseAddr + GPYINV;
    odr = gpioBaseAddr + GPYODR;
    qsel = gpioBaseAddr + GPYQSEL + pin32/16;

    EALLOW;

    //
    //Set the data direction
    //
    *dir &= ~pinMask;
    if (output == 1)
    {
        //
        //Output, with optional open drain mode and pull-up
        //
        *dir |= pinMask;

        //
        //Enable open drain if necessary
        //
        if (flags & GPIO_OPENDRAIN)
        {
            *odr |= pinMask;
        }
        else
        {
            *odr &= ~pinMask;
        }

        //
        //Enable pull-up if necessary. Open drain mode must be active.
        //
        if (flags & (GPIO_OPENDRAIN | GPIO_PULLUP))
        {
            *pud &= ~pinMask;
        }
        else
        {
            *pud |= pinMask;
        }
    }
    else
    {
        //
        //Input, with optional pull-up, qualification, and polarity inversion
        //
        *dir &= ~pinMask;

        //
        //Enable pull-up if necessary
        //
        if (flags & GPIO_PULLUP)
        {
            *pud &= ~pinMask;
        }
        else
        {
            *pud |= pinMask;
        }

        //
        //Invert polarity if necessary
        //
        if (flags & GPIO_INVERT)
        {
            *inv |= pinMask;
        }
        else
        {
            *inv &= ~pinMask;
        }
    }

    //
    //Extract the qualification parameter and load it into the register.
    //This is also needed for open drain outputs, so we might as well do it
    //all the time.
    //
    qual = (flags & GPIO_ASYNC) / GPIO_QUAL3;
    *qsel &= ~(0x3L << (2 * pin16));
    if (qual != 0x0)
    {
        *qsel |= qual << (2 * pin16);
    }

    EDIS;
}

//
// GPIO_SetupLock - Enable or disable the GPIO register bit lock for the
//                  specified pin.
//                  The valid flags are:
//                  GPIO_UNLOCK   Unlock the pin setup register bits for the
//                                specified pin
//                  GPIO_LOCK     Lock the pin setup register bits for the
//                                specified pin
//
void GPIO_SetupLock(Uint16 pin, Uint16 flags)
{
    volatile Uint32 *gpioBaseAddr;
    volatile Uint32 *lock;
    Uint32 pin32, pinMask;

    pin32 = pin % 32;
    pinMask = 1UL << pin32;
    gpioBaseAddr = (Uint32 *)&GpioCtrlRegs + (pin/32)*GPY_CTRL_OFFSET;

    //
    //Create pointers to the appropriate registers. This is a workaround
    //for the way GPIO registers are defined. The standard definition
    //in the header file makes it very easy to do named accesses of one
    //register or bit, but hard to do arbitrary numerical accesses. It's
    //easier to have an array of GPIO modules with identical registers,
    //including arrays for multi-register groups like GPyQSEL1-2. But
    //the header file doesn't define anything we can turn into an array,
    //so manual pointer arithmetic is used instead.
    //
    lock = gpioBaseAddr + GPYLOCK;

    EALLOW;
    if(flags)
    {
        //
        //Lock the pin
        //
        *lock |= pinMask;
    }
    else
    {
        //
        //Unlock the pin
        //
        *lock &= ~pinMask;
    }
    EDIS;
}

//
// GPIO_SetupXINT1Gpio - External interrupt setup
//
void GPIO_SetupXINT1Gpio(Uint16 pin)
{
    EALLOW;
    InputXbarRegs.INPUT4SELECT = pin;       //Set XINT1 source to GPIO-pin
    EDIS;
}

//
// GPIO_SetupXINT2Gpio - External interrupt setup
//
void GPIO_SetupXINT2Gpio(Uint16 pin)
{
    EALLOW;
    InputXbarRegs.INPUT5SELECT = pin;       //Set XINT2 source to GPIO-pin
    EDIS;
}

//
// GPIO_SetupXINT3Gpio - External interrupt setup
//
void GPIO_SetupXINT3Gpio(Uint16 pin)
{
    EALLOW;
    InputXbarRegs.INPUT6SELECT = pin;       //Set XINT3 source to GPIO-pin
    EDIS;
}

//
// GPIO_SetupXINT4Gpio - External interrupt setup
//
void GPIO_SetupXINT4Gpio(Uint16 pin)
{
    EALLOW;
    InputXbarRegs.INPUT13SELECT = pin;     //Set XINT4 source to GPIO-pin
    EDIS;
}

//
// GPIO_SetupXINT5Gpio - External interrupt setup
//
void GPIO_SetupXINT5Gpio(Uint16 pin)
{
    EALLOW;
    InputXbarRegs.INPUT14SELECT = pin;     //Set XINT5 source to GPIO-pin
    EDIS;
}

//
// GPIO_EnableUnbondedIOPullupsFor176Pin - Enable pullups for the unbonded
//                                         GPIOs on the 176PTP package:
//                                         GPIOs     Grp Bits
//                                         95-132    C   31
//                                                   D   31:0
//                                                   E   4:0
//                                         134-168   E   31:6
//                                                   F   8:0
//
void GPIO_EnableUnbondedIOPullupsFor176Pin()
{
    EALLOW;
    GpioCtrlRegs.GPCPUD.all = ~0x80000000;  //GPIO 95
    GpioCtrlRegs.GPDPUD.all = ~0xFFFFFFF7;  //GPIOs 96-127
    GpioCtrlRegs.GPEPUD.all = ~0xFFFFFFDF;  //GPIOs 128-159 except for 133
    GpioCtrlRegs.GPFPUD.all = ~0x000001FF;  //GPIOs 160-168
    EDIS;
}

//
// GPIO_EnableUnbondedIOPullupsFor100Pin - Enable pullups for the unbonded
//                                         GPIOs on the 100PZ package:
//                                         GPIOs     Grp Bits
//                                         0-1       A   1:0
//                                         5-9       A   9:5
//                                         22-40     A   31:22
//                                                   B   8:0
//                                         44-57     B   25:12
//                                         67-68     C   4:3
//                                         74-77     C   13:10
//                                         79-83     C   19:15
//                                         93-168    C   31:29
//                                                   D   31:0
//                                                   E   31:0
//                                                   F   8:0
//
void GPIO_EnableUnbondedIOPullupsFor100Pin()
{
    EALLOW;
    GpioCtrlRegs.GPAPUD.all = ~0xFFC003E3;  //GPIOs 0-1, 5-9, 22-31
    GpioCtrlRegs.GPBPUD.all = ~0x03FFF1FE;  //GPIOs 32-40, 44-57
    GpioCtrlRegs.GPCPUD.all = ~0xE10FBC18;  //GPIOs 67-68, 74-77, 79-83, 93-95
    GpioCtrlRegs.GPDPUD.all = ~0xFFFFFFF7;  //GPIOs 96-127
    GpioCtrlRegs.GPEPUD.all = ~0xFFFFFFFF;  //GPIOs 128-159
    GpioCtrlRegs.GPFPUD.all = ~0x000001FF;  //GPIOs 160-168
    EDIS;
}

//
// GPIO_EnableUnbondedIOPullups - Enable IO pullups for specific package
//
void GPIO_EnableUnbondedIOPullups()
{
    unsigned char pin_count = ((DevCfgRegs.PARTIDL.all & 0x00000700) >> 8) ;

    //
    // 5 = 100 pin
    // 6 = 176 pin
    // 7 = 337 pin
    //
    if(pin_count == 5)
    {
        GPIO_EnableUnbondedIOPullupsFor100Pin();
    }
    else if (pin_count == 6)
    {
        GPIO_EnableUnbondedIOPullupsFor176Pin();
    }
    else
    {
        //
        //do nothing - this is 337 pin package
        //
    }
}

//
// GPIO_ReadPin - Read the GPyDAT register bit for the specified pin.
//                Note that this returns the actual state of the pin,
//                not the state of the output latch.
//
Uint16 GPIO_ReadPin(Uint16 pin)
{
    volatile Uint32 *gpioDataReg;
    Uint16 pinVal;

    gpioDataReg = (volatile Uint32 *)&GpioDataRegs + (pin/32)*GPY_DATA_OFFSET;
    pinVal = (gpioDataReg[GPYDAT] >> (pin % 32)) & 0x1;

    return pinVal;
}

//
// GPIO_WritePin - Set the GPyDAT register bit for the specified pin.
//
void GPIO_WritePin(Uint16 pin, Uint16 outVal)
{
    volatile Uint32 *gpioDataReg;
    Uint32 pinMask;

    gpioDataReg = (volatile Uint32 *)&GpioDataRegs + (pin/32)*GPY_DATA_OFFSET;
    pinMask = 1UL << (pin % 32);

    if (outVal == 0)
    {
        gpioDataReg[GPYCLEAR] = pinMask;
    }
    else
    {
        gpioDataReg[GPYSET] = pinMask;
    }
}

//
// End of file
//

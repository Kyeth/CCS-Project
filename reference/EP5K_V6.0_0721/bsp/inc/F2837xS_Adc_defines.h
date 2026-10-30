//###########################################################################
//
// FILE:   F2837xS_Adc_defines.h
//
// TITLE:  #defines used in ADC examples
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

#ifndef F2837xS_ADC_DEFINES_H
#define F2837xS_ADC_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Defines
//
#define ADC_CHANNEL_0        0
#define ADC_CHANNEL_1        1
#define ADC_CHANNEL_2        2
#define ADC_CHANNEL_3        3
#define ADC_CHANNEL_4        4
#define ADC_CHANNEL_5        5
#define ADC_CHANNEL_6        6
#define ADC_CHANNEL_7        7

#define ADC_CHANNEL_8        8
#define ADC_CHANNEL_9        9
#define ADC_CHANNEL_10       10
#define ADC_CHANNEL_11       11
#define ADC_CHANNEL_12       12
#define ADC_CHANNEL_13       13
#define ADC_CHANNEL_14       14
#define ADC_CHANNEL_15       15

//
//definitions for specifying an ADC
//
#define ADC_ADCA 0
#define ADC_ADCB 1
#define ADC_ADCC 2
#define ADC_ADCD 3

//
//definitions for selecting ADC resolution
//
#ifndef _DUAL_HEADERS
#define ADC_RESOLUTION_12BIT 0
#define ADC_RESOLUTION_16BIT 1
#else
#define ADC_BITRESOLUTION_12BIT 0
#define ADC_BITRESOLUTION_16BIT 1
#endif
//
//definitions for selecting ADC signal mode
//(single-ended mode is only a valid mode for 12-bit resolution)
//
#define ADC_SIGNALMODE_SINGLE 0
#define ADC_SIGNALMODE_DIFFERENTIAL 1

typedef enum
{
    ADC_CLK_DIV_1_0 = 0,            //!< ADCCLK = (input clock) / 1.0
    ADC_CLK_DIV_2_0 = 2,            //!< ADCCLK = (input clock) / 2.0
    ADC_CLK_DIV_2_5 = 3,            //!< ADCCLK = (input clock) / 2.5
    ADC_CLK_DIV_3_0 = 4,            //!< ADCCLK = (input clock) / 3.0
    ADC_CLK_DIV_3_5 = 5,            //!< ADCCLK = (input clock) / 3.5
    ADC_CLK_DIV_4_0 = 6,            //!< ADCCLK = (input clock) / 4.0
    ADC_CLK_DIV_4_5 = 7,            //!< ADCCLK = (input clock) / 4.5
    ADC_CLK_DIV_5_0 = 8,            //!< ADCCLK = (input clock) / 5.0
    ADC_CLK_DIV_5_5 = 9,            //!< ADCCLK = (input clock) / 5.5
    ADC_CLK_DIV_6_0 = 10,           //!< ADCCLK = (input clock) / 6.0
    ADC_CLK_DIV_6_5 = 11,           //!< ADCCLK = (input clock) / 6.5
    ADC_CLK_DIV_7_0 = 12,           //!< ADCCLK = (input clock) / 7.0
    ADC_CLK_DIV_7_5 = 13,           //!< ADCCLK = (input clock) / 7.5
    ADC_CLK_DIV_8_0 = 14,           //!< ADCCLK = (input clock) / 8.0
    ADC_CLK_DIV_8_5 = 15            //!< ADCCLK = (input clock) / 8.5
} ADC_ClkPrescale;

typedef enum
{
    ADC_TRIGGER_SW_ONLY     = 0,     //!< Software only
    ADC_TRIGGER_CPU1_TINT0  = 1,     //!< CPU1 Timer 0, TINT0
    ADC_TRIGGER_CPU1_TINT1  = 2,     //!< CPU1 Timer 1, TINT1
    ADC_TRIGGER_CPU1_TINT2  = 3,     //!< CPU1 Timer 2, TINT2
    ADC_TRIGGER_GPIO        = 4,     //!< GPIO, ADCEXTSOC
    ADC_TRIGGER_EPWM1_SOCA  = 5,     //!< ePWM1, ADCSOCA
    ADC_TRIGGER_EPWM1_SOCB  = 6,     //!< ePWM1, ADCSOCB
    ADC_TRIGGER_EPWM2_SOCA  = 7,     //!< ePWM2, ADCSOCA
    ADC_TRIGGER_EPWM2_SOCB  = 8,     //!< ePWM2, ADCSOCB
    ADC_TRIGGER_EPWM3_SOCA  = 9,     //!< ePWM3, ADCSOCA
    ADC_TRIGGER_EPWM3_SOCB  = 10,    //!< ePWM3, ADCSOCB
    ADC_TRIGGER_EPWM4_SOCA  = 11,    //!< ePWM4, ADCSOCA
    ADC_TRIGGER_EPWM4_SOCB  = 12,    //!< ePWM4, ADCSOCB
    ADC_TRIGGER_EPWM5_SOCA  = 13,    //!< ePWM5, ADCSOCA
    ADC_TRIGGER_EPWM5_SOCB  = 14,    //!< ePWM5, ADCSOCB
    ADC_TRIGGER_EPWM6_SOCA  = 15,    //!< ePWM6, ADCSOCA
    ADC_TRIGGER_EPWM6_SOCB  = 16,    //!< ePWM6, ADCSOCB
    ADC_TRIGGER_EPWM7_SOCA  = 17,    //!< ePWM7, ADCSOCA
    ADC_TRIGGER_EPWM7_SOCB  = 18,    //!< ePWM7, ADCSOCB
    ADC_TRIGGER_EPWM8_SOCA  = 19,    //!< ePWM8, ADCSOCA
    ADC_TRIGGER_EPWM8_SOCB  = 20,    //!< ePWM8, ADCSOCB
    ADC_TRIGGER_EPWM9_SOCA  = 21,    //!< ePWM9, ADCSOCA
    ADC_TRIGGER_EPWM9_SOCB  = 22,    //!< ePWM9, ADCSOCB
    ADC_TRIGGER_EPWM10_SOCA = 23,   //!< ePWM10, ADCSOCA
    ADC_TRIGGER_EPWM10_SOCB = 24,   //!< ePWM10, ADCSOCB
    ADC_TRIGGER_EPWM11_SOCA = 25,   //!< ePWM11, ADCSOCA
    ADC_TRIGGER_EPWM11_SOCB = 26,   //!< ePWM11, ADCSOCB
    ADC_TRIGGER_EPWM12_SOCA = 27,   //!< ePWM12, ADCSOCA
    ADC_TRIGGER_EPWM12_SOCB = 28   //!< ePWM12, ADCSOCB
} ADC_Trigger;

typedef enum
{
    ADC_CH_ADCIN0 = 0,           //!< single-ended, ADCIN0
    ADC_CH_ADCIN1 = 1,           //!< single-ended, ADCIN1
    ADC_CH_ADCIN2 = 2,           //!< single-ended, ADCIN2
    ADC_CH_ADCIN3 = 3,           //!< single-ended, ADCIN3
    ADC_CH_ADCIN4 = 4,           //!< single-ended, ADCIN4
    ADC_CH_ADCIN5 = 5,           //!< single-ended, ADCIN5
    ADC_CH_ADCIN6 = 6,           //!< single-ended, ADCIN6
    ADC_CH_ADCIN7 = 7,           //!< single-ended, ADCIN7
    ADC_CH_ADCIN8 = 8,           //!< single-ended, ADCIN8
    ADC_CH_ADCIN9 = 9,           //!< single-ended, ADCIN9
    ADC_CH_ADCIN10 = 10,         //!< single-ended, ADCIN10
    ADC_CH_ADCIN11 = 11,         //!< single-ended, ADCIN11
    ADC_CH_ADCIN12 = 12,         //!< single-ended, ADCIN12
    ADC_CH_ADCIN13 = 13,         //!< single-ended, ADCIN13
    ADC_CH_ADCIN14 = 14,         //!< single-ended, ADCIN14
    ADC_CH_ADCIN15 = 15,         //!< single-ended, ADCIN15
    ADC_CH_ADCIN0_ADCIN1 = 0,    //!< differential, ADCIN0 and ADCIN1
    ADC_CH_ADCIN2_ADCIN3 = 2,    //!< differential, ADCIN2 and ADCIN3
    ADC_CH_ADCIN4_ADCIN5 = 4,    //!< differential, ADCIN4 and ADCIN5
    ADC_CH_ADCIN6_ADCIN7 = 6,    //!< differential, ADCIN6 and ADCIN7
    ADC_CH_ADCIN8_ADCIN9 = 8,    //!< differential, ADCIN8 and ADCIN9
    ADC_CH_ADCIN10_ADCIN11 = 10, //!< differential, ADCIN10 and ADCIN11
    ADC_CH_ADCIN12_ADCIN13 = 12, //!< differential, ADCIN12 and ADCIN13
    ADC_CH_ADCIN14_ADCIN15 = 14  //!< differential, ADCIN14 and ADCIN15
} ADC_Channel;


#ifdef __cplusplus
}
#endif /* extern "C" */

#endif   // - end of F2837xS_ADC_DEFINES_H

//
// End of file
//

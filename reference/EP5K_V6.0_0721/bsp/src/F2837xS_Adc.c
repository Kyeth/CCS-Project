//###########################################################################
// FILE:   F2837xS_Adc.c
// TITLE:  F2837xS Adc Support Functions.
//###########################################################################
// $TI Release: F2837xS Support Library v160 $
// $Release Date: Mon Jun 15 14:14:46 CDT 2015 $
// $Copyright: Copyright (C) 2014-2015 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

#include "F2837xS_device.h"     // F2837xS Headerfile Include File
#include "F2837xS_Examples.h"   // F2837xS Examples Include File

/*************************************************
Function: SetupADC
Description: ADC通道初始化,逆变电感电流采样使用双通道
Input: None
Output: None
Frequency: Once
*************************************************/
static void SetupADC(void)
{
    // Select the channels to convert and end of conversion flag
    EALLOW;

    // ADCA 采样通道配置
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;  // ADCINA0关联到CHSEL0，SOC0对应result0  采样VISO
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;  // 电网L2相电压
    AdcaRegs.ADCSOC2CTL.bit.CHSEL = 2;  // PV4电压
    AdcaRegs.ADCSOC3CTL.bit.CHSEL = 3;  // PV4电流
    AdcaRegs.ADCSOC4CTL.bit.CHSEL = 4;  // PV1电流
    AdcaRegs.ADCSOC5CTL.bit.CHSEL = 5;  // 电网L1相电压
    AdcaRegs.ADCSOC6CTL.bit.CHSEL = 4;  // PV1电流2
    AdcaRegs.ADCSOC7CTL.bit.CHSEL = 3;  // PV4电流2
    // ADCA 采样窗口配置
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 49;    // sample window is 51 SYSCLK cycles
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = 49;
    AdcaRegs.ADCSOC2CTL.bit.ACQPS = 49;
    AdcaRegs.ADCSOC3CTL.bit.ACQPS = 49;
    AdcaRegs.ADCSOC4CTL.bit.ACQPS = 149;
    AdcaRegs.ADCSOC5CTL.bit.ACQPS = 29;
    AdcaRegs.ADCSOC6CTL.bit.ACQPS = 149;
    AdcaRegs.ADCSOC7CTL.bit.ACQPS = 149;
    // ADCA 触发源配置
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 7;    //19-epwm8SOCA
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 7;    // 7-epwm2SOCA todo
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL = 13;   //13-epwm5SOCA
    AdcaRegs.ADCSOC3CTL.bit.TRIGSEL = 13;   //13-epwm5SOCA
    AdcaRegs.ADCSOC4CTL.bit.TRIGSEL = 15;   //15-epwm6SOCA
    AdcaRegs.ADCSOC5CTL.bit.TRIGSEL = 5;    //5-epwm1SOCA
    AdcaRegs.ADCSOC6CTL.bit.TRIGSEL = 16;   //16-epwm6SOCB
    AdcaRegs.ADCSOC7CTL.bit.TRIGSEL = 14;   //14-epwm5SOCB

    // ADCB 采样通道配置
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = 0;  // PV2电压
    AdcbRegs.ADCSOC1CTL.bit.CHSEL = 1;  // PV1电压
    AdcbRegs.ADCSOC2CTL.bit.CHSEL = 2;  // PV3电压
    AdcbRegs.ADCSOC3CTL.bit.CHSEL = 3;  // 多路2
    // ADCB 采样窗口配置
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = 49;    //sample window is acqps + 1 SYSCLK cycles
    AdcbRegs.ADCSOC1CTL.bit.ACQPS = 49;
    AdcbRegs.ADCSOC2CTL.bit.ACQPS = 49;
    AdcbRegs.ADCSOC3CTL.bit.ACQPS = 49;
    // ADCB 触发源配置
    AdcbRegs.ADCSOC0CTL.bit.TRIGSEL = 17;     //17-epwm7SOCA
    AdcbRegs.ADCSOC1CTL.bit.TRIGSEL = 15;     //15-epwm6SOCA
    AdcbRegs.ADCSOC2CTL.bit.TRIGSEL = 23;     //23-epwm10SOCA
//    AdcbRegs.ADCSOC3CTL.bit.TRIGSEL = 19;     //19-epwm8SOCA

    // ADCC 采样通道配置
    AdccRegs.ADCSOC2CTL.bit.CHSEL = 2;  // PV2电流
    AdccRegs.ADCSOC3CTL.bit.CHSEL = 3;  // 逆变L2相电压
    AdccRegs.ADCSOC4CTL.bit.CHSEL = 4;  // 逆变L1相电压
    AdccRegs.ADCSOC5CTL.bit.CHSEL = 2;  // PV2电流2
    AdccRegs.ADCSOC6CTL.bit.CHSEL = 3;  // 逆变L2相电压2
    AdccRegs.ADCSOC7CTL.bit.CHSEL = 4;  // 逆变L1相电压2
    // ADCC 采样窗口配置
    AdccRegs.ADCSOC2CTL.bit.ACQPS = 149;
    AdccRegs.ADCSOC3CTL.bit.ACQPS = 49;
    AdccRegs.ADCSOC4CTL.bit.ACQPS = 49;
    AdccRegs.ADCSOC5CTL.bit.ACQPS = 149;
    AdccRegs.ADCSOC6CTL.bit.ACQPS = 49;
    AdccRegs.ADCSOC7CTL.bit.ACQPS = 49;
    // ADCC 触发源配置
    AdccRegs.ADCSOC2CTL.bit.TRIGSEL = 17;      //17-epwm7SOCA
    AdccRegs.ADCSOC3CTL.bit.TRIGSEL = 7;       // 7-epwm2SOCA
    AdccRegs.ADCSOC4CTL.bit.TRIGSEL = 5;       // 5-epwm1SOCA
    AdccRegs.ADCSOC5CTL.bit.TRIGSEL = 18;      //18-epwm7SOCB
    AdccRegs.ADCSOC6CTL.bit.TRIGSEL = 8;       //
    AdccRegs.ADCSOC7CTL.bit.TRIGSEL = 6;       //

    // ADCD 采样通道配置
    AdcdRegs.ADCSOC0CTL.bit.CHSEL = 0;   //PV3电流
    AdcdRegs.ADCSOC1CTL.bit.CHSEL = 1;   //GFCI漏电流
    AdcdRegs.ADCSOC2CTL.bit.CHSEL = 2;   //逆变L2相电流
    AdcdRegs.ADCSOC3CTL.bit.CHSEL = 3;   //多路1
    AdcdRegs.ADCSOC4CTL.bit.CHSEL = 4;   //PNBus采样
    AdcdRegs.ADCSOC5CTL.bit.CHSEL = 0;   //PV3电流2
    AdcdRegs.ADCSOC6CTL.bit.CHSEL = 14;  //逆变L1相电流
    AdcdRegs.ADCSOC12CTL.bit.CHSEL = 2;  //逆变L2相电流2
    AdcdRegs.ADCSOC13CTL.bit.CHSEL = 14; //逆变L1相电流2
    AdcdRegs.ADCSOC14CTL.bit.CHSEL = 14; //逆变L1相电流
    AdcdRegs.ADCSOC15CTL.bit.CHSEL = 15; //下半母线


    // ADCD 采样窗口配置
    AdcdRegs.ADCSOC0CTL.bit.ACQPS = 149;
    AdcdRegs.ADCSOC1CTL.bit.ACQPS = 49;
    AdcdRegs.ADCSOC2CTL.bit.ACQPS = 49;
    AdcdRegs.ADCSOC3CTL.bit.ACQPS = 49;
    AdcdRegs.ADCSOC4CTL.bit.ACQPS = 49;
    AdcdRegs.ADCSOC5CTL.bit.ACQPS = 149;
    AdcdRegs.ADCSOC6CTL.bit.ACQPS = 49;
    AdcdRegs.ADCSOC12CTL.bit.ACQPS = 49;//349
    AdcdRegs.ADCSOC13CTL.bit.ACQPS = 49;//349
    AdcdRegs.ADCSOC14CTL.bit.ACQPS = 49;//349
    AdcdRegs.ADCSOC15CTL.bit.ACQPS = 49;

    // ADCD 触发源配置
    AdcdRegs.ADCSOC0CTL.bit.TRIGSEL  = 23;   //23-epwm10SOCA
    AdcdRegs.ADCSOC1CTL.bit.TRIGSEL  = 5;    // 5-epwm1SOCA
    AdcdRegs.ADCSOC2CTL.bit.TRIGSEL  = 7;    // 7-epwm2SOCA   //5
//    AdcdRegs.ADCSOC3CTL.bit.TRIGSEL  = 5;    // 5-epwm1SOCA
    AdcdRegs.ADCSOC4CTL.bit.TRIGSEL  = 7;    //19-epwm8SOCA
    AdcdRegs.ADCSOC5CTL.bit.TRIGSEL  = 24;   //24-epwm10SOCB
    AdcdRegs.ADCSOC6CTL.bit.TRIGSEL = 5;    // 5-epwm1SOCA
    AdcdRegs.ADCSOC12CTL.bit.TRIGSEL = 8;    // 8-epwm2SOCB
    AdcdRegs.ADCSOC13CTL.bit.TRIGSEL = 6;    // 6-epwm1SOCB
    AdcdRegs.ADCSOC14CTL.bit.TRIGSEL = 5;    // 5-epwm1SOCA
    AdcdRegs.ADCSOC15CTL.bit.TRIGSEL = 5;    //19-epwm8SOCA

    // Config Event
    /*以下为全桥部分PWM模块-------------------------------------------*/
//    EPwm1Regs.ETSEL.bit.SOCAEN  = 0;
//    EPwm1Regs.ETSEL.bit.SOCASEL = 2;//1-(TBCTR = 0)  2-(TBCTR = period) 3-(TBCTR = period|0) 4、5(=cmpa) 6、7(=cmpb)
//    EPwm1Regs.ETPS.bit.SOCAPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
//    EPwm1Regs.ETSEL.bit.SOCAEN  = 1;
//
//    EPwm1Regs.ETSEL.bit.SOCBSEL = 1;//1-(TBCTR = 0)  2-(TBCTR = period) 3-(TBCTR = period|0) 4、5(=cmpa) 6、7(=cmpb)
//    EPwm1Regs.ETPS.bit.SOCBPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
//    EPwm1Regs.ETSEL.bit.SOCBEN  = 1;

    EPwm1Regs.ETSEL.bit.SOCASEL = 4;//4 CMPC when the timer is incrementing
    EPwm1Regs.ETPS.bit.SOCAPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
    EPwm1Regs.ETSEL.bit.SOCAEN  = 1;
    EPwm1Regs.ETSEL.bit.SOCASELCMP = 1;

    EPwm1Regs.ETSEL.bit.SOCBSEL = 7;//7 CMPD when the timer is incrementing
    EPwm1Regs.ETPS.bit.SOCBPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
    EPwm1Regs.ETSEL.bit.SOCBEN  = 1;
    EPwm1Regs.ETSEL.bit.SOCBSELCMP = 1;

    EPwm2Regs.ETSEL.bit.SOCASEL = 4;//4 CMPC when the timer is incrementing
    EPwm2Regs.ETPS.bit.SOCAPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
    EPwm2Regs.ETSEL.bit.SOCAEN  = 1;
    EPwm2Regs.ETSEL.bit.SOCASELCMP = 1;

    EPwm2Regs.ETSEL.bit.SOCBSEL = 7;//7 CMPD when the timer is decrementing
    EPwm2Regs.ETPS.bit.SOCBPRD  = 1;//1-Generate the EPWMxSOCA pulse on the first event:
    EPwm2Regs.ETSEL.bit.SOCBEN  = 1;
    EPwm2Regs.ETSEL.bit.SOCBSELCMP = 1;
//
//    EPwm2Regs.ETSEL.bit.SOCAEN  = 0;
//    EPwm2Regs.ETSEL.bit.SOCASEL = 2;
//    EPwm2Regs.ETPS.bit.SOCAPRD  = 1;
//    EPwm2Regs.ETSEL.bit.SOCAEN  = 1;

//    EPwm3Regs.ETSEL.bit.SOCAEN  = 0;
//    EPwm3Regs.ETSEL.bit.SOCASEL = 2;
//    EPwm3Regs.ETPS.bit.SOCAPRD  = 1;
//    EPwm3Regs.ETSEL.bit.SOCAEN  = 1;
//
//    EPwm4Regs.ETSEL.bit.SOCAEN  = 0;
//    EPwm4Regs.ETSEL.bit.SOCASEL = 2;
//    EPwm4Regs.ETPS.bit.SOCAPRD  = 1;
//    EPwm4Regs.ETSEL.bit.SOCAEN  = 1;
    /*以下为PV部分PWM模块-------------------------------------------*/
    EPwm6Regs.ETSEL.bit.SOCAEN  = 0;
    EPwm6Regs.ETSEL.bit.SOCASEL = 2;
    EPwm6Regs.ETPS.bit.SOCAPRD  = 1;
    EPwm6Regs.ETSEL.bit.SOCAEN  = 1;

    EPwm6Regs.ETSEL.bit.SOCBEN  = 0;
    EPwm6Regs.ETSEL.bit.SOCBSEL = 6;
    EPwm6Regs.ETPS.bit.SOCBPRD  = 1;
    EPwm6Regs.ETSEL.bit.SOCBEN  = 1;

    EPwm7Regs.ETSEL.bit.SOCAEN  = 0;
    EPwm7Regs.ETSEL.bit.SOCASEL = 2;
    EPwm7Regs.ETPS.bit.SOCAPRD  = 1;
    EPwm7Regs.ETSEL.bit.SOCAEN  = 1;

    EPwm7Regs.ETSEL.bit.SOCBEN  = 0;
    EPwm7Regs.ETSEL.bit.SOCBSEL = 6;
    EPwm7Regs.ETPS.bit.SOCBPRD  = 1;
    EPwm7Regs.ETSEL.bit.SOCBEN  = 1;

    EPwm10Regs.ETSEL.bit.SOCAEN  = 0;
    EPwm10Regs.ETSEL.bit.SOCASEL = 2;
    EPwm10Regs.ETPS.bit.SOCAPRD  = 1;
    EPwm10Regs.ETSEL.bit.SOCAEN  = 1;

    EPwm10Regs.ETSEL.bit.SOCBEN  = 0;
    EPwm10Regs.ETSEL.bit.SOCBSEL = 6;
    EPwm10Regs.ETPS.bit.SOCBPRD  = 1;
    EPwm10Regs.ETSEL.bit.SOCBEN  = 1;

    EPwm5Regs.ETSEL.bit.SOCAEN  = 0;
    EPwm5Regs.ETSEL.bit.SOCASEL = 2;
    EPwm5Regs.ETPS.bit.SOCAPRD  = 1;
    EPwm5Regs.ETSEL.bit.SOCAEN  = 1;

    EPwm5Regs.ETSEL.bit.SOCBEN  = 0;
    EPwm5Regs.ETSEL.bit.SOCBSEL = 6;
    EPwm5Regs.ETPS.bit.SOCBPRD  = 1;
    EPwm5Regs.ETSEL.bit.SOCBEN  = 1;
    /*以下为BUS部分PWM模块-------------------------------------------*/
    EPwm8Regs.ETSEL.bit.SOCAEN  = 0;
    EPwm8Regs.ETSEL.bit.SOCASEL = 2;
    EPwm8Regs.ETPS.bit.SOCAPRD  = 1;
    EPwm8Regs.ETSEL.bit.SOCAEN  = 1;

    EDIS;

    DELAY_US(1000);
}

/* 
* Set the resolution and signalmode for a given ADC. This will ensure that
* the correct trim is loaded. 
*/
void AdcSetMode(Uint16 adc, Uint16 resolution, Uint16 signalmode)
{
	Uint16 adcOffsetTrimOTPIndex; //index into OTP table of ADC offset trims
	Uint16 adcOffsetTrim; //temporary ADC offset trim
	
	//re-populate INL trim
	CalAdcINL(adc);
	
	if(0xFFFF != *((Uint16*)GetAdcOffsetTrimOTP)){
		//offset trim function is programmed into OTP, so call it

		//calculate the index into OTP table of offset trims and call
		//function to return the correct offset trim
		adcOffsetTrimOTPIndex = 4*adc + 2*resolution + 1*signalmode;
		adcOffsetTrim = (*GetAdcOffsetTrimOTP)(adcOffsetTrimOTPIndex);
	}
	else {
		//offset trim function is not populated, so set offset trim to 0
		adcOffsetTrim = 0;
	}

	//Apply the resolution and signalmode to the specified ADC.
	//Also apply the offset trim and, if needed, linearity trim correction.
	switch(adc){
		case ADC_ADCA:
			AdcaRegs.ADCCTL2.bit.RESOLUTION = resolution;
			AdcaRegs.ADCCTL2.bit.SIGNALMODE = signalmode;
			AdcaRegs.ADCOFFTRIM.all = adcOffsetTrim;
			if(ADC_RESOLUTION_12BIT == resolution){

				//12-bit linearity trim workaround
				AdcaRegs.ADCINLTRIM1 &= 0xFFFF0000;
				AdcaRegs.ADCINLTRIM2 &= 0xFFFF0000;
				AdcaRegs.ADCINLTRIM4 &= 0xFFFF0000;
				AdcaRegs.ADCINLTRIM5 &= 0xFFFF0000;
			}
		break;
		case ADC_ADCB:
			AdcbRegs.ADCCTL2.bit.RESOLUTION = resolution;
			AdcbRegs.ADCCTL2.bit.SIGNALMODE = signalmode;
			AdcbRegs.ADCOFFTRIM.all = adcOffsetTrim;
			if(ADC_RESOLUTION_12BIT == resolution){

				//12-bit linearity trim workaround
				AdcbRegs.ADCINLTRIM1 &= 0xFFFF0000;
				AdcbRegs.ADCINLTRIM2 &= 0xFFFF0000;
				AdcbRegs.ADCINLTRIM4 &= 0xFFFF0000;
				AdcbRegs.ADCINLTRIM5 &= 0xFFFF0000;
			}
		break;
		case ADC_ADCC:
			AdccRegs.ADCCTL2.bit.RESOLUTION = resolution;
			AdccRegs.ADCCTL2.bit.SIGNALMODE = signalmode;
			AdccRegs.ADCOFFTRIM.all = adcOffsetTrim;
			if(ADC_RESOLUTION_12BIT == resolution){

				//12-bit linearity trim workaround
				AdccRegs.ADCINLTRIM1 &= 0xFFFF0000;
				AdccRegs.ADCINLTRIM2 &= 0xFFFF0000;
				AdccRegs.ADCINLTRIM4 &= 0xFFFF0000;
				AdccRegs.ADCINLTRIM5 &= 0xFFFF0000;
			}
		break;
		case ADC_ADCD:
			AdcdRegs.ADCCTL2.bit.RESOLUTION = resolution;
			AdcdRegs.ADCCTL2.bit.SIGNALMODE = signalmode;
			AdcdRegs.ADCOFFTRIM.all = adcOffsetTrim;
			if(ADC_RESOLUTION_12BIT == resolution){

				//12-bit linearity trim workaround
				AdcdRegs.ADCINLTRIM1 &= 0xFFFF0000;
				AdcdRegs.ADCINLTRIM2 &= 0xFFFF0000;
				AdcdRegs.ADCINLTRIM4 &= 0xFFFF0000;
				AdcdRegs.ADCINLTRIM5 &= 0xFFFF0000;
			}
		break;
	}
}

/* 
* Loads INL trim values from OTP into the trim registers of the specified ADC.
* Use only as part of AdcSetMode function, since linearity trim correction
* is needed for some modes.
*/
void CalAdcINL(Uint16 adc)
{
	switch(adc){
		case ADC_ADCA:
			if(0xFFFF != *((Uint16*)CalAdcaINL)){
				//trim function is programmed into OTP, so call it
				(*CalAdcaINL)();
			}
			else {
				//do nothing, no INL trim function populated
			}
			break;
		case ADC_ADCB:
			if(0xFFFF != *((Uint16*)CalAdcbINL)){
				//trim function is programmed into OTP, so call it
				(*CalAdcbINL)();
			}
			else {
				//do nothing, no INL trim function populated
			}
			break;
		case ADC_ADCC:
			if(0xFFFF != *((Uint16*)CalAdccINL)){
				//trim function is programmed into OTP, so call it
				(*CalAdccINL)();
			}
			else {
				//do nothing, no INL trim function populated
			}
			break;
		case ADC_ADCD:
			if(0xFFFF != *((Uint16*)CalAdcdINL)){
				//trim function is programmed into OTP, so call it
				(*CalAdcdINL)();
			}
			else {
				//do nothing, no INL trim function populated
			}
			break;
	}
}
/*************************************************
Function: InitADC
Description: ADC模块初始化
Input: None
Output: None
Frequency: Once
*************************************************/
void InitADC(void)
{
    EALLOW;

    // -----------------ADCA------------
    AdcaRegs.ADCCTL2.bit.PRESCALE = ADC_CLK_DIV_4_0; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    //Set pulse positions to late
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    //power up the ADC
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    //delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

    // -----------------ADCB------------
    AdcbRegs.ADCCTL2.bit.PRESCALE = ADC_CLK_DIV_4_0;//6;//2;//6; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    //Set pulse positions to late
    AdcbRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    //power up the ADC
    AdcbRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    //delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

    // -----------------ADCC------------
    AdccRegs.ADCCTL2.bit.PRESCALE = ADC_CLK_DIV_4_0; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCC, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    //Set pulse positions to late
    AdccRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    //power up the ADC
    AdccRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    //delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

    // -----------------ADCD------------
    AdcdRegs.ADCCTL2.bit.PRESCALE = ADC_CLK_DIV_4_0; //set ADCCLK divider to /4
    AdcSetMode(ADC_ADCD, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    //Set pulse positions to late
    AdcdRegs.ADCCTL1.bit.INTPULSEPOS = 1;
    //power up the ADC
    AdcdRegs.ADCCTL1.bit.ADCPWDNZ = 1;
    //delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

    EDIS;

    SetupADC();
}

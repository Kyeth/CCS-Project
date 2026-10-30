/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: F2837xS_EPwm.c
Author: tzz
Version : 0.0.2
Date:2022年02月11日
Description: EPWM 配置文件
Function List:
***********************************************************/
#include "bsp.h"


//uint16_t PosCnt = 0, PosFlag = 0;
//uint16_t NegCnt = 0, NegFlag = 0;
//uint16_t PosCnt2 = 0, PosFlag2 = 0;
//uint16_t NegCnt2 = 0, NegFlag2 = 0;
//uint16_t DeadBandComp = 2;

static void InitAllEPwmGpio(void);
static void ConfigPVpwm(void);
//static void ConfigLLCpwm(void);
static void ConfigBUSPwm(void);
static void ConfigINVPwm(void);
static void ConfigFanPwm(void);

/*************************************************
Function: InitAllEPwmGpio
Description: 初始化所有的EPWM的GPIO
Input: None
Output: None
Frequency: Once
*************************************************/
static void InitAllEPwmGpio(void)
{
    EALLOW;
    //=================epwm1
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up on GPIO0 (EPWM1A)
    GpioCtrlRegs.GPAPUD.bit.GPIO1 = 1;    // Disable pull-up on GPIO1 (EPWM1B)

    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // Configure GPIO0 as EPWM1A
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;   // Configure GPIO1 as EPWM1B
    //=================epwm2
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;    // Disable pull-up on GPIO2 (EPWM2A)
    GpioCtrlRegs.GPAPUD.bit.GPIO3 = 1;    // Disable pull-up on GPIO3 (EPWM2B)

    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;   // Configure GPIO2 as EPWM2A
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;   // Configure GPIO3 as EPWM2B
    //=================epwm3
    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1;    // Disable pull-up on GPIO4 (EPWM3A)
    GpioCtrlRegs.GPAPUD.bit.GPIO5 = 1;    // Disable pull-up on GPIO5 (EPWM3B)

    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;   // Configure GPIO4 as EPWM3A
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;   // Configure GPIO5 as EPWM3B
    //==================epwm4
    GpioCtrlRegs.GPAPUD.bit.GPIO6 = 1;    // Disable pull-up on GPIO7 (EPWM4A)
    GpioCtrlRegs.GPAPUD.bit.GPIO7 = 1;    // Disable pull-up on GPIO7 (EPWM4B)

    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 1;   // Configure GPIO7 as EPWM4A
    GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 1;   // Configure GPIO7 as EPWM4B
    //==================epwm5
    GpioCtrlRegs.GPAPUD.bit.GPIO8 = 1;    // Disable pull-up on GPIO8 (EPWM5A)

    GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 1;   // Configure GPIO8 as EPWM5A
    //==================epwm6
    GpioCtrlRegs.GPAPUD.bit.GPIO10 = 1;    // Disable pull-up on GPIO10 (EPWM6A)

    GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 1;   // Configure GPIO10 as EPWM6A
    //===================epwm7
    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 1;    // Disable pull-up on GPIO12 (EPWM7A)

    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 1;   // Configure GPIO12 as EPWM7A
    //==================epwm8
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 1;    // Disable pull-up on GPIO14 (EPWM8A)
    GpioCtrlRegs.GPAPUD.bit.GPIO15 = 1;    // Disable pull-up on GPIO15 (EPWM8B)

    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 1;   // Configure GPIO14 as EPWM8A
    GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 1;   // Configure GPIO15 as EPWM8B
    //===================epwm9
    //===================epwm10
    GpioCtrlRegs.GPAGMUX2.bit.GPIO18 = 1;  // Configure GPIO18 as EPWM10A

    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 1;    // Disable pull-up on GPIO18(EPWM10A)

    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;   // Configure GPIO18 as EPWM10A
    //===================epwm11
    //===================epwm12
    GpioCtrlRegs.GPAGMUX2.bit.GPIO23 = 1;  // Configure GPIO23 as EPWM12B

    GpioCtrlRegs.GPAPUD.bit.GPIO23 = 1;    // Disable pull-up on GPIO23 (EPWM12B)

    GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 1;   // Configure GPIO23 as EPWM12B

    EDIS;
}
/*************************************************
Function: ConfigINVPwm
Description: 配置逆变PWM
Input: None
Output: None
Frequency: Once
*************************************************/
static void ConfigINVPwm(void)
{
    EALLOW;

    // Counter Mode
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;  // 上下计数模式
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm4Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    // EPWM set Clock Prescaler
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // HSPCLK分频=1（100MHz）
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;           // 时基时钟分频=1
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm4Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    // EPWM disable Phase ShiftLoad
//    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;     // 主同步源-计数器置0后输出同步信号
//    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;      // 从EPWM1同步
//    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;      // 从EPWM1同步
//    EPwm4Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;      // 从EPWM1同步
//    EPwm1Regs.TBCTL.bit.PHSEN = 0;
//    EPwm2Regs.TBCTL.bit.PHSEN = 0;          // 允许相位同步
//    EPwm3Regs.TBCTL.bit.PHSEN = 0;          // 允许相位同步
//    EPwm4Regs.TBCTL.bit.PHSEN = 0;          // 允许相位同步
    // EPWM setTime Base Period
    EPwm1Regs.TBPRD = INV_EPWM_PERIOD;
    EPwm2Regs.TBPRD = INV_EPWM_PERIOD;
    EPwm3Regs.TBPRD = INV_EPWM_PERIOD;
    EPwm4Regs.TBPRD = INV_EPWM_PERIOD;
    // EPWM set Phase Shift
//    EPwm1Regs.TBPHS.bit.TBPHS = 0;
//    EPwm2Regs.TBPHS.bit.TBPHS = INV_EPWM_PERIOD - 1; //待定，会导致启机波形变差
//    EPwm3Regs.TBPHS.bit.TBPHS = 0;
//    EPwm4Regs.TBPHS.bit.TBPHS = 0;
    // EPWM set Time Base Counter
    EPwm1Regs.TBCTR = 0;
    EPwm2Regs.TBCTR = 0;
    EPwm3Regs.TBCTR = 0;
    EPwm4Regs.TBCTR = 0;
    // EPWM set Counter Compare Shadow Load Mode
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CMPA影子寄存器
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;   // 计数器归零时加载
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm4Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm4Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm4Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm4Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    // Set Compare values
    EPwm1Regs.CMPA.all = INV_EPWM_PERIOD;
    EPwm1Regs.CMPB.bit.CMPB = 0;
    EPwm2Regs.CMPA.all = INV_EPWM_PERIOD;
    EPwm2Regs.CMPB.bit.CMPB = 0;
    EPwm3Regs.CMPA.all = INV_EPWM_PERIOD;
    EPwm3Regs.CMPB.bit.CMPB = 0;
    EPwm4Regs.CMPA.all = INV_EPWM_PERIOD;
    EPwm4Regs.CMPB.bit.CMPB = 0;
    // Set actions
    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm4Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm4Regs.AQCTLA.bit.CAD = AQ_CLEAR;

    EPwm1Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR;
    EPwm2Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLB.bit.CAD = AQ_CLEAR;
    EPwm3Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLB.bit.CAD = AQ_CLEAR;
    EPwm4Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm4Regs.AQCTLB.bit.CAD = AQ_CLEAR;

    // Set actions both RED and FED S4/S5
    // both RED and FED death band source is EPWMA(default)
    EPwm1Regs.DBCTL.bit.IN_MODE = 2;                // EPWM1A为所有的源
    EPwm1Regs.DBCTL.bit.DEDB_MODE = 0;
    EPwm2Regs.DBCTL.bit.IN_MODE = 2;
    EPwm2Regs.DBCTL.bit.DEDB_MODE = 0;
    EPwm3Regs.DBCTL.bit.IN_MODE = 2;
    EPwm3Regs.DBCTL.bit.DEDB_MODE = 0;
    EPwm4Regs.DBCTL.bit.IN_MODE = 2;
    EPwm4Regs.DBCTL.bit.DEDB_MODE = 0;
    // Set the RED and FED values
    EPwm1Regs.DBFED.all = DEADTIME_FALLOFF_COUNT;   // 0.8us（80周期）
    EPwm1Regs.DBRED.all = DEADTIME_RISEON_COUNT;    // 0.8us（80周期）
    EPwm2Regs.DBFED.all = DEADTIME_FALLOFF_COUNT;
    EPwm2Regs.DBRED.all = DEADTIME_RISEON_COUNT;
    EPwm3Regs.DBFED.all = DEADTIME_FALLOFF_COUNT;
    EPwm3Regs.DBRED.all = DEADTIME_RISEON_COUNT;
    EPwm4Regs.DBFED.all = DEADTIME_FALLOFF_COUNT;
    EPwm4Regs.DBRED.all = DEADTIME_RISEON_COUNT;
    // S2-HIGH, S3-LOW 0 Invert only the Rising Edge delayed output (AHC)
    EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;       // 下降沿延迟被反转
    EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm3Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm4Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    // S1=1,S0=1
    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;  // DBM完全有效（即，RED和FED均有效）
    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm3Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm4Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    // DO NOT Switch Output A with Output B
    EPwm1Regs.DBCTL.bit.OUTSWAP = 0;

    EPwm2Regs.DBCTL.bit.OUTSWAP = 0;

//    EPwm2Regs.DBCTL.bit.OUTSWAP = 3;
    EPwm3Regs.DBCTL.bit.OUTSWAP = 0;
    EPwm4Regs.DBCTL.bit.OUTSWAP = 0;

    EPwm1Regs.AQSFRC.bit.RLDCSF = 3;                // 软件强制拉低时立即执行,3-load immediately;0-zero
    EPwm2Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm3Regs.AQSFRC.bit.RLDCSF = 3;
    EPwm4Regs.AQSFRC.bit.RLDCSF = 3;

    EPwm1Regs.CMPC = 100;//INV_EPWM_PERIOD * 0.1f;   //
    EPwm1Regs.CMPD = 2400;//INV_EPWM_PERIOD * 0.9f;   //
    EPwm2Regs.CMPC = 100;//INV_EPWM_PERIOD * 0.1f;   //
    EPwm2Regs.CMPD = 2400;//INV_EPWM_PERIOD * 0.9f;   //
    // PWM1中断使能
#if 1
    EPwm1Regs.ETSEL.bit.INTSEL = 1;//ET_CTR_PRD;
    EPwm1Regs.ETSEL.bit.INTEN = 1;              // PWM中断使能
    EPwm1Regs.ETPS.bit.INTPRD  = 1;             // 2次触发一次中断

    PieVectTable.EPWM1_INT = &ISREpwm1;
    IER |= M_INT3;
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
#endif

    EPwm11Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;  // 上下计数模式
    EPwm11Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // HSPCLK分频=1（100MHz）
    EPwm11Regs.TBCTL.bit.CLKDIV = TB_DIV1;           // 时基时钟分频=1
    EPwm11Regs.TBPRD = (INV_EPWM_PERIOD/4);          // 4*2 = 8倍
    EPwm11Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;     // CMPA影子寄存器
    EPwm11Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;     //
    EPwm11Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;   // 计数器归零时加载
    EPwm11Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EDIS;
}
#if 0
/*************************************************
Function: DischargeInvC
Description: BUS放电
 * 当逆变并网过程中电网断开时,逆变停止输出,此时逆变输出电容两端会出现一个和母线一样大小的
 * 正向或反向的电压,如果继续离网发波将会出现输出电压持续5~10ms的震荡。
 * 因此需要将电容两端电量释放到bus,将H桥改造成boost电路即可
 * type-1:逆变1输出有正向电压,则逆变2一定是反向电压
 * 逆变1的低频管EPWM2B常通,高频管EPWM1B占空比为10%,EPWM1A、EPWM2A断开
 * 逆变2的高频管EPWM4B常通,低频管EPWM3B占空比为10%,EPWM3A、EPWM4A断开
 * type-2:逆变1输出有反向电压,则逆变2一定是正向电压
 * 逆变1的高频管EPWM1B常通,低频管EPWM2B占空比为10%,EPWM1A、EPWM2A断开
 * 逆变2的低频管EPWM3B常通,高频管EPWM4B占空比为10%,EPWM3A、EPWM4A断开
 *
Input: None
Output: None
Frequency:
*************************************************/
void DischargeInvC(uint16_t type)
{
    // 逆变1输出正向电压
    if (type == 1)
    {
        // 先强制拉低 EPWM1、EPWM2
//        DisableINV1Pwm();
//        DisableINV2Pwm();
        DISABLE_INV1PWM;
        DISABLE_INV2PWM;
        DISABLE_INV3PWM;
        DISABLE_INV4PWM;
        // 逆变器1------------EPWM2B常通,高频管EPWM1B占空比为40%,EPWM1A、EPWM2A断开
        EPwm1Regs.CMPA.bit.CMPA = INV_EPWM_PERIOD*0.6;

        EPwm1Regs.AQCTLB.bit.CAU = AQ_SET;
        EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR;

        EPwm1Regs.AQCSFRC.bit.CSFA = 1;
        EPwm1Regs.AQCSFRC.bit.CSFB = 0;     // 40%占空比
        EPwm2Regs.AQCSFRC.bit.CSFA = 1;
        EPwm2Regs.AQCSFRC.bit.CSFB = 2;     // 强制拉高

        // 逆变器2------------高频管EPWM4B常通,低频管EPWM3B占空比为40%,EPWM3A、EPWM4A断开
        EPwm3Regs.CMPA.bit.CMPA = INV_EPWM_PERIOD*0.6;

        EPwm3Regs.AQCTLB.bit.CAU = AQ_SET;
        EPwm3Regs.AQCTLB.bit.CAD = AQ_CLEAR;

        EPwm3Regs.AQCSFRC.bit.CSFA = 1;
        EPwm4Regs.AQCSFRC.bit.CSFB = 2;     // 强制拉高
        EPwm4Regs.AQCSFRC.bit.CSFA = 1;
        EPwm3Regs.AQCSFRC.bit.CSFB = 0;     // 40%占空比
    }

    // 逆变1输出反向电压
    if (type == 2)
    {
        // 先强制拉低 EPWM1、EPWM2
//        DisableINV1Pwm();
//        DisableINV2Pwm();
        DISABLE_INV1PWM;
        DISABLE_INV2PWM;
        DISABLE_INV3PWM;
        DISABLE_INV4PWM;
        // 逆变器1------------高频管EPWM1B常通,低频管EPWM2B占空比为40%,EPWM1A、EPWM2A断开
        EPwm2Regs.CMPA.bit.CMPA = INV_EPWM_PERIOD*0.6;

        EPwm2Regs.AQCTLB.bit.CAU = AQ_SET;
        EPwm2Regs.AQCTLB.bit.CAD = AQ_CLEAR;

        EPwm1Regs.AQCSFRC.bit.CSFA = 1;
        EPwm1Regs.AQCSFRC.bit.CSFB = 2;     // 强制拉高
        EPwm2Regs.AQCSFRC.bit.CSFA = 1;
        EPwm2Regs.AQCSFRC.bit.CSFB = 0;     // 40%占空比

        // 逆变器2------------低频管EPWM3B常通,高频管EPWM4B占空比为40%,EPWM3A、EPWM4A断开
        EPwm4Regs.CMPA.bit.CMPA = INV_EPWM_PERIOD*0.6;

        EPwm4Regs.AQCTLB.bit.CAU = AQ_SET;
        EPwm4Regs.AQCTLB.bit.CAD = AQ_CLEAR;

        EPwm3Regs.AQCSFRC.bit.CSFA = 1;
        EPwm4Regs.AQCSFRC.bit.CSFB = 0;     // 40%占空比
        EPwm4Regs.AQCSFRC.bit.CSFA = 1;
        EPwm3Regs.AQCSFRC.bit.CSFB = 2;     // 强制拉高
    }
}

/*************************************************
Function: RecoverInvPwm
Description: 放电结束后恢复原有配置
Input: None
Output: None
Frequency: Once
*************************************************/
void RecoverInvPwm(void)
{
//    DisableINV1Pwm();
//    DisableINV2Pwm();
    DISABLE_INV1PWM;
    DISABLE_INV2PWM;

    EPwm1Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_CLEAR;
    EPwm4Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm4Regs.AQCTLA.bit.CAD = AQ_CLEAR;
}
#endif
/*************************************************
Function: RecoverInvPwm2
Description: 放电结束后恢复原有配置
Input: None
Output: None
Frequency: Once
*************************************************/
//void RecoverInvPwm2(void)
//{
//    DISABLE_INV1PWM;
//    DISABLE_INV2PWM;
//    DISABLE_INV3PWM;
//    DISABLE_INV4PWM;
//
//    // 工频管
//    EPwm1Regs.AQCTLA.bit.CAU = AQ_NO_ACTION;
//    EPwm1Regs.AQCTLA.bit.CAD = AQ_NO_ACTION;
//}

/*************************************************
Function: InitPV1Tz
Description:
Input: None
Output: None
Frequency:
*************************************************/
void InitPV1Tz(void)
{
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO39 = 0;   // All inputs
    GpioCtrlRegs.GPBQSEL1.bit.GPIO39 = 1;   // 3 samples

    InputXbarRegs.INPUT1SELECT = 39;//GPIO39-Input1

    EPwmXbarRegs.TRIP4MUX0TO15CFG.bit.MUX1 = 1;  // INPUTXBAR1 作为 MUX1 的输入
    EPwmXbarRegs.TRIP4MUXENABLE.bit.MUX1 = 1;    // enable the mux1
    EPwmXbarRegs.TRIPOUTINV.bit.TRIP4 = 0;       // 1-输出高电平;0-输出低电平

    EPwm6Regs.DCBHTRIPSEL.bit.TRIPINPUT4 = 1;
    EPwm6Regs.DCTRIPSEL.bit.DCBHCOMPSEL = DC_TRIPIN4;

    EPwm6Regs.TZDCSEL.bit.DCBEVT2 = TZ_DCBH_LOW;     // DCBH = LOW, DCBL = don't care
    EPwm6Regs.TZSEL.bit.DCBEVT2 = 1;

    EPwm6Regs.TZCTL.bit.TZA = TZ_NO_CHANGE;
    EPwm6Regs.TZCTL.bit.TZB = TZ_NO_CHANGE;

    EPwm6Regs.TZCLR.bit.CBC = 1;
    EPwm6Regs.TZEINT.bit.CBC = 0;                   // Disable cycle-by-cycle interrupt generation.
    EDIS;
}

/*************************************************
Function: InitPV2Tz
Description:
Input: None
Output: None
Frequency:
*************************************************/
void InitPV2Tz(void)
{
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO40 = 0;   // All inputs
    GpioCtrlRegs.GPBQSEL1.bit.GPIO40 = 1;   // 3 samples

    InputXbarRegs.INPUT2SELECT = 40;//GPIO40-Input2

    EPwmXbarRegs.TRIP5MUX0TO15CFG.bit.MUX3 = 1;  // INPUTXBAR2 作为 MUX3 的输入
    EPwmXbarRegs.TRIP5MUXENABLE.bit.MUX3 = 1;    // enable the mux3
    EPwmXbarRegs.TRIPOUTINV.bit.TRIP5 = 0;       // 1-输出高电平;0-输出低电平

    EPwm7Regs.DCBHTRIPSEL.bit.TRIPINPUT5 = 1;
    EPwm7Regs.DCTRIPSEL.bit.DCBHCOMPSEL = DC_TRIPIN5;

    EPwm7Regs.TZDCSEL.bit.DCBEVT2 = TZ_DCBH_LOW;     // DCBH = LOW, DCBL = don't care
    EPwm7Regs.TZSEL.bit.DCBEVT2 = 1;

    EPwm7Regs.TZCTL.bit.TZA = TZ_NO_CHANGE;
    EPwm7Regs.TZCTL.bit.TZB = TZ_NO_CHANGE;

    EPwm7Regs.TZCLR.bit.CBC = 1;
    EPwm7Regs.TZEINT.bit.CBC = 0;                   // Disable cycle-by-cycle interrupt generation.
    EDIS;
}
/*************************************************
Function: InitPV3Tz
Description:
Input: None
Output: None
Frequency:
*************************************************/
void InitPV3Tz(void)
{
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO41 = 0;   // All inputs
    GpioCtrlRegs.GPBQSEL1.bit.GPIO41 = 1;   // 3 samples

    InputXbarRegs.INPUT3SELECT = 41;//GPIO41-Input3

    EPwmXbarRegs.TRIP7MUX0TO15CFG.bit.MUX5 = 1;  // INPUTXBAR3 作为 MUX5 的输入
    EPwmXbarRegs.TRIP7MUXENABLE.bit.MUX5 = 1;    // enable the mux5
    EPwmXbarRegs.TRIPOUTINV.bit.TRIP7 = 0;       // 1-输出高电平;0-输出低电平

    EPwm10Regs.DCBHTRIPSEL.bit.TRIPINPUT7 = 1;
    EPwm10Regs.DCTRIPSEL.bit.DCBHCOMPSEL = DC_TRIPIN7;

    EPwm10Regs.TZDCSEL.bit.DCBEVT2 = TZ_DCBH_LOW;     // DCBH = LOW, DCBL = don't care
    EPwm10Regs.TZSEL.bit.DCBEVT2 = 1;

    EPwm10Regs.TZCTL.bit.TZA = TZ_NO_CHANGE;
    EPwm10Regs.TZCTL.bit.TZB = TZ_NO_CHANGE;

    EPwm10Regs.TZCLR.bit.CBC = 1;
    EPwm10Regs.TZEINT.bit.CBC = 0;                   // Disable cycle-by-cycle interrupt generation.
    EDIS;
}
/*************************************************
Function: InitPV4Tz
Description:
Input: None
Output: None
Frequency:
*************************************************/
void InitPV4Tz(void)
{
    EALLOW;
    GpioCtrlRegs.GPBMUX1.bit.GPIO38 = 0;  // All GPIO
    GpioCtrlRegs.GPBDIR.bit.GPIO38 = 0;   // All inputs
    GpioCtrlRegs.GPBQSEL1.bit.GPIO38 = 1;   // 3 samples

    InputXbarRegs.INPUT4SELECT = 38;//GPIO38-Input4

    EPwmXbarRegs.TRIP8MUX0TO15CFG.bit.MUX7 = 1;  // INPUTXBAR4 作为 MUX7 的输入
    EPwmXbarRegs.TRIP8MUXENABLE.bit.MUX7 = 1;    // enable the mux7
    EPwmXbarRegs.TRIPOUTINV.bit.TRIP8 = 0;       // 1-输出高电平;0-输出低电平

    EPwm5Regs.DCBHTRIPSEL.bit.TRIPINPUT8 = 1;
    EPwm5Regs.DCTRIPSEL.bit.DCBHCOMPSEL = DC_TRIPIN8;

    EPwm5Regs.TZDCSEL.bit.DCBEVT2 = TZ_DCBH_LOW;     // DCBH = LOW, DCBL = don't care
    EPwm5Regs.TZSEL.bit.DCBEVT2 = 1;

    EPwm5Regs.TZCTL.bit.TZA = TZ_NO_CHANGE;
    EPwm5Regs.TZCTL.bit.TZB = TZ_NO_CHANGE;

    EPwm5Regs.TZCLR.bit.CBC = 1;
    EPwm5Regs.TZEINT.bit.CBC = 0;                   // Disable cycle-by-cycle interrupt generation.
    EDIS;
}
/*************************************************
Function: InitINV1Tz
Description: 逆变1 gpio79-inv1-INPUT3SELECT,使用IO触发TZ CBC事件
Input: None
Output: None
Frequency: Once
*************************************************/
void InitINV1Tz(void)
{
    EALLOW;

    GpioCtrlRegs.GPCMUX1.bit.GPIO79 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO79 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL1.bit.GPIO79 = 1;   // 3 samples
    GPIO_SetupXINT4Gpio(79);           //外部中断4绑定引脚79
    XintRegs.XINT4CR.bit.POLARITY = 0;          //下降沿触发
    XintRegs.XINT4CR.bit.ENABLE = 1;            //使能外部中断1 XINT1

    PieVectTable.XINT4_INT = &XINT4_ISR;
    EDIS;
}
/*************************************************
Function: InitINV2Tz
Description: 逆变2 gpio79-inv1-INPUT3SELECT,使用IO触发TZ CBC事件
Input: None
Output: None
Frequency: Once
*************************************************/
void InitINV2Tz(void)
{
    EALLOW;

    GpioCtrlRegs.GPCMUX2.bit.GPIO81 = 0;  // All GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO81 = 0;   // All inputs
    GpioCtrlRegs.GPCQSEL2.bit.GPIO81 = 1;   // 3 samples
    GPIO_SetupXINT3Gpio(81);//外部中断3绑定引脚81
    XintRegs.XINT3CR.bit.POLARITY = 0;          // 下降沿触发
    XintRegs.XINT3CR.bit.ENABLE = 1;            //使能外部中断3 XINT3

    PieVectTable.XINT3_INT = &XINT3_ISR;
    EDIS;

}
/*************************************************
Function: InitINVOCXint
Description: 逆变过流信号外部中断信号触发
Input: None
Output: None
Frequency:
*************************************************/
void InitINVOCXint(void)
{
    EALLOW;

    GpioCtrlRegs.GPCMUX2.bit.GPIO83 = 0;          // GPIO
    GpioCtrlRegs.GPCDIR.bit.GPIO83 = 0;           // input
    GpioCtrlRegs.GPCQSEL2.bit.GPIO83 = 0x01;      // XINT1 Synch to SYSCLKOUT only
    GpioCtrlRegs.GPCCTRL.bit.QUALPRD0 =0x01;      // 2*9*5ns

    XintRegs.XINT2CR.bit.POLARITY = 0;            // negative edge interrupt
    XintRegs.XINT2CR.bit.ENABLE = 1;              // Enable XINT2

    InputXbarRegs.INPUT5SELECT = 83;              // Set XINT2 source to GPIO-pin

    PieVectTable.XINT2_INT = &xint2_isr;
    EDIS;
    IER |= M_INT1|M_INT12;
    PieCtrlRegs.PIEIER1.bit.INTx5 = 1;
    PieCtrlRegs.PIEIER12.bit.INTx1 = 1;
    PieCtrlRegs.PIEIER12.bit.INTx2 = 1;
}


/*************************************************
Function: ConfigPVpwm
Description: 配置PV PWM
Input: None
Output: None
Frequency:
*************************************************/
static void ConfigPVpwm(void)
{
    EALLOW;
    //----------PV1--------------------------------------------------//
    EPwm6Regs.TBPRD = PV_PWM_PERIOD;                     // Set timer period
    EPwm6Regs.TBPHS.bit.TBPHS = 0x0000;           // Phase is 0
    EPwm6Regs.TBCTR = 0;                      // Clear counter

    // Setup TBCLK
    EPwm6Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Count up
    EPwm6Regs.TBCTL.bit.PHSEN = TB_DISABLE;       // Disable phase loading
    EPwm6Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // Clock ratio to SYSCLKOUT
    EPwm6Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // Slow so we can observe on
    // the scope
    // Setup compare
    EPwm6Regs.CMPA.bit.CMPA = 50;
    EPwm6Regs.CMPB.bit.CMPB = 2450;
    // Setup shadowing
    EPwm6Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm6Regs.CMPCTL.bit.LOADAMODE = 2;//CC_CTR_PRD;

    EPwm6Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm6Regs.AQCTLA.bit.ZRO = AQ_SET;

    // Active high complementary PWMs - Setup the deadband
    EPwm6Regs.DBCTL.bit.OUT_MODE =  DB_DISABLE;
    //----------PV2--------------------------------------------------//
    EPwm7Regs.TBPRD = PV_PWM_PERIOD;                        // Set timer period
    EPwm7Regs.TBPHS.bit.TBPHS = 0x0000;            // Phase is 0
    EPwm7Regs.TBCTR = 0;                      // Clear counter

    // Setup TBCLK
    EPwm7Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Count up
    EPwm7Regs.TBCTL.bit.PHSEN = TB_DISABLE;       // Disable phase loading
    EPwm7Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // Clock ratio to SYSCLKOUT
    EPwm7Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // Slow so we can observe on
    // the scope
    // Setup compare
    EPwm7Regs.CMPA.bit.CMPA = 0;
    EPwm7Regs.CMPB.bit.CMPB = PV_PWM_PERIOD/2;
    // Setup shadowing
    EPwm7Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm7Regs.CMPCTL.bit.LOADAMODE = 2;


    EPwm7Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm7Regs.AQCTLA.bit.ZRO = AQ_SET;

    // Active high complementary PWMs - Setup the deadband
    EPwm7Regs.DBCTL.bit.OUT_MODE =  DB_DISABLE;
    //----------PV3--------------------------------------------------//
    EPwm10Regs.TBPRD = PV_PWM_PERIOD;                        // Set timer period
    EPwm10Regs.TBPHS.bit.TBPHS = 0x0000;            // Phase is 0
    EPwm10Regs.TBCTR = 0;                      // Clear counter

    // Setup TBCLK
    EPwm10Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Count up
    EPwm10Regs.TBCTL.bit.PHSEN = TB_DISABLE;       // Disable phase loading
    EPwm10Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // Clock ratio to SYSCLKOUT
    EPwm10Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // Slow so we can observe on
    // the scope
    // Setup compare
    EPwm10Regs.CMPA.bit.CMPA = 0;
    EPwm10Regs.CMPB.bit.CMPB = PV_PWM_PERIOD/2;
    // Setup shadowing
    EPwm10Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm10Regs.CMPCTL.bit.LOADAMODE = 2;

    EPwm10Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm10Regs.AQCTLA.bit.ZRO = AQ_SET;

    // Active high complementary PWMs - Setup the deadband
    EPwm10Regs.DBCTL.bit.OUT_MODE =  DB_DISABLE; // DBM无效
    //----------PV4--------------------------------------------------//
    EPwm5Regs.TBPRD = PV_PWM_PERIOD;                   // Set timer period
    EPwm5Regs.TBPHS.bit.TBPHS = 0x0000;            // Phase is 0
    EPwm5Regs.TBCTR = 0x0000;                      // Clear counter
    // Setup TBCLK
    EPwm5Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Count up
    EPwm5Regs.TBCTL.bit.PHSEN = TB_DISABLE;  // Disable phase loading
    EPwm5Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;       // Clock ratio to SYSCLKOUT
    EPwm5Regs.TBCTL.bit.CLKDIV = TB_DIV1;          // Slow so we can observe on
    // Setup compare
    EPwm5Regs.CMPA.bit.CMPA = 0;
    EPwm5Regs.CMPB.bit.CMPB = PV_PWM_PERIOD/2;
    // Setup shadowing
    EPwm5Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm5Regs.CMPCTL.bit.LOADAMODE = 2; // Load on Zero

    EPwm5Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm5Regs.AQCTLA.bit.ZRO = AQ_SET;

    // Active high complementary PWMs
    EPwm5Regs.DBCTL.bit.OUT_MODE =  DB_DISABLE;  // DBM无效
#if 0
    // 设置相位偏移,PWM1作为输入同步源,9相位不同步,10相对1偏移120度,11相对1偏移240度
    //pv1 PWM
//    EPwm9Regs.TBCTL.bit.PHSEN = 0;
//    EPwm9Regs.TBPHS.bit.TBPHS = 0;
//    EPwm9Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;// ePWM1 SYNCO is generated on CTR=0
//    EPwm9Regs.TBCTR = 0x0000;
//    //pv2 PWM
//    EPwm10Regs.TBPHS.bit.TBPHS = (PV_PWM_PERIOD * 2.0f) / 3.0f;
//    EPwm10Regs.TBCTR = 0x0000;
//    EPwm10Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;
//    EPwm10Regs.TBCTL.bit.PHSEN = 1;
//    //pv3 PWM
//    EPwm11Regs.TBPHS.bit.TBPHS = (PV_PWM_PERIOD * 4.0f) / 3.0f;
//    EPwm11Regs.TBCTR = 0x0000;
//    EPwm11Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_IN;
//    EPwm11Regs.TBCTL.bit.PHSEN = 1;
    EPwm9Regs.TBCTL.bit.FREE_SOFT = 0X03;

    EPwm9Regs.TBCTR = 0x0000;
    EPwm10Regs.TBCTR = (PV_PWM_PERIOD * 0.05f);
    EPwm11Regs.TBCTR = (PV_PWM_PERIOD * 1.0f);


#endif

    EDIS;
}
#if 0
/*************************************************
Function: EnablePVPwm
Description: 使能 PV PWM
Input: @channel-1使能PV1;2-使能PV2;3-使能PV3
Output: None
Frequency:
*************************************************/
void EnablePVPwm(uint16_t channel)
{
    if (channel == 1)
    {
        EPwm9Regs.AQCSFRC.bit.CSFA = 0;
    }
    else if (channel == 2)
    {
        EPwm10Regs.AQCSFRC.bit.CSFA = 0;
    }
    else if (channel == 3)
    {
        EPwm11Regs.AQCSFRC.bit.CSFA = 0;
    }
}
/*************************************************
Function: DisablePVPwm
Description: 禁止 PV PWM
Input: @channel-1禁止PV1;2-禁止PV2;3-禁止PV3
Output: None
Frequency:
*************************************************/
void DisablePVPwm(uint16_t channel)
{
    if (channel == 1)
    {
        EPwm9Regs.CMPA.bit.CMPA = PV_PWM_PERIOD-1;
        EPwm9Regs.AQCSFRC.bit.CSFA = 1;
    }
    else if (channel == 2)
    {
        EPwm10Regs.CMPA.bit.CMPA = PV_PWM_PERIOD-1;
        EPwm10Regs.AQCSFRC.bit.CSFA = 1;
    }
    else if (channel == 3)
    {
        EPwm11Regs.CMPA.bit.CMPA = PV_PWM_PERIOD-1;
        EPwm11Regs.AQCSFRC.bit.CSFA = 1;
    }
}
#endif
/*************************************************
Function: ConfigFanPwm
Description: 配置平衡电路 PWM
Input: None
Output: None
Frequency:
*************************************************/
static void ConfigBUSPwm(void)
{
    EALLOW;
    // TZ模块配置
    EPwm8Regs.TZCTL.bit.TZA = TZ_FORCE_LO; // 故障时A口强制低电平
    EPwm8Regs.TZCTL.bit.TZB = TZ_FORCE_LO; // 故障时B口强制低电平
    EPwm8Regs.TZFRC.bit.OST = 1;         // 初始化时立即关闭PWM输出
    EDIS;

    EPwm8Regs.TBPRD = BUS_PWM_PERIOD;                   // Set timer period
    EPwm8Regs.TBPHS.bit.TBPHS = 0x0000;            // Phase is 0
    EPwm8Regs.TBCTR = 0x0000;                      // Clear counter
    // Setup TBCLK
    EPwm8Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;  // Count up
    EPwm8Regs.TBCTL.bit.PHSEN = TB_DISABLE;         // Disable phase loading
    EPwm8Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;        // Clock ratio to SYSCLKOUT
    EPwm8Regs.TBCTL.bit.CLKDIV = TB_DIV1;           // Slow so we can observe on
    // the scope
    // Setup compare
    EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD/2;
    // Setup shadowing
    EPwm8Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm8Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm8Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // Load on Zero
    EPwm8Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm8Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm8Regs.AQCTLA.bit.CAD = AQ_SET;
    EPwm8Regs.AQCTLB.bit.CAU = AQ_CLEAR;
    EPwm8Regs.AQCTLB.bit.CAD = AQ_SET;

    EPwm8Regs.DBCTL.bit.IN_MODE = 2;
    EPwm8Regs.DBCTL.bit.POLSEL = 2;     //B反
    EPwm8Regs.DBCTL.bit.OUT_MODE = 3;

    // Active high complementary PWMs
//    EPwm8Regs.DBCTL.bit.OUT_MODE =  DB_FULL_ENABLE;  // DBM完全有效（即，RED和FED均有效）控制S0、S1开关
//    EPwm8Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
//    EPwm8Regs.DBCTL.bit.IN_MODE = DBA_ALL;
    EPwm8Regs.DBRED.all = DEADTIME_RISEON_COUNT;
    EPwm8Regs.DBFED.all = DEADTIME_FALLOFF_COUNT;
}
/*************************************************
Function: ConfigFanPwm
Description: 配置FAN PWM
Input: None
Output: None
Frequency:
*************************************************/
static void ConfigFanPwm(void)
{
    EALLOW;

    // Counter Mode
    EPwm12Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm12Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm12Regs.TBCTL.bit.CLKDIV = TB_DIV1;
    EPwm12Regs.TBCTL.bit.PHSEN = 0;
    // EPWM setTime Base Period
    EPwm12Regs.TBPRD = FAN_PWM_PERIOD;
    // EPWM set Phase Shift
    EPwm12Regs.TBPHS.bit.TBPHS = 0;
    // Set Compare values
    EPwm12Regs.CMPB.bit.CMPB = FAN_PWM_PERIOD;
    // EPWM set Counter Compare Shadow Load Mode
    EPwm12Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm12Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    // Set actions
    EPwm12Regs.AQCTLB.bit.CBU = AQ_SET;
    EPwm12Regs.AQCTLB.bit.CBD = AQ_CLEAR;
    // DeadBand disabled
    EPwm12Regs.DBCTL.bit.OUT_MODE = DB_DISABLE;

    EDIS;
}
/*************************************************
Function: StopFan
Description: 关闭风机
Input: None
Output: None
Frequency:
*************************************************/
void StopFan(void)
{
    EPwm12Regs.CMPB.bit.CMPB = FAN_PWM_PERIOD;
    EPwm12Regs.AQCSFRC.bit.CSFB = 1;
}
/*************************************************
Function: StartFan
Description: 启动风机-默认50%风速
Input: None
Output: None
Frequency:
*************************************************/
void StartFan(void)
{
    EPwm12Regs.CMPB.bit.CMPB = FAN_PWM_PERIOD;
    EPwm12Regs.AQCSFRC.bit.CSFB = 0;
}
/*************************************************
Function: SetFanSpeed
Description: 设置风机速度
Input: @speed-风机速度(0~1.0)
Output: None
Frequency:
*************************************************/
void SetFanSpeed(float speed)
{
    float temp = 0;

    temp = __fsat(speed, 1.0f, 0.0f);

    temp = 1.0 - temp;

    EPwm12Regs.CMPB.bit.CMPB = FAN_PWM_PERIOD*temp;
}

///*************************************************
//Function: UpdateINVPwmComp3
//Description: 更新 INV PWM;
//** 双极性调制方式
//Input: @id-1-逆变1;2-逆变2; @cmp-占空比值 @period-PWM开关周期值
//Output: None
//Frequency:
////*************************************************/
//void UpdateINVPwmComp3(uint16_t id, int16_t cmp, uint16_t period, uint16_t flag)
//{
//    static uint16_t mux = 1;
//
//    volatile struct EPWM_REGS* EPwmRegsPT = &EPwm1Regs;
//
//    if (id == 1)
//    {
////        mux = !mux;
//    	if(PWMONflag)
//    	{
//    	    if(cmp > 100)
//    	    {
//    	        if(mux)
//    	        {
//    	            EPwm1Regs.AQCSFRC.bit.CSFA = 0; //开启上管
//    	        }
//    	        else
//    	        {
//    	            EPwm1Regs.AQCSFRC.bit.CSFA = 1;
//    	        }
//                EPwm1Regs.AQCSFRC.bit.CSFB = 2;  //关闭下管
////    	        EPwm1Regs.CMPA.bit.CMPA = (-cmp + ((int)(period)));
//    	    }
//    	    else if(cmp < -100)
//    	    {
//                if(mux)
//                {
//                    EPwm1Regs.AQCSFRC.bit.CSFB = 0;
//                }
//                else
//                {
//                    EPwm1Regs.AQCSFRC.bit.CSFB = 2;
//                }
//                EPwm1Regs.AQCSFRC.bit.CSFA = 1;  //关闭上管
////                EPwm1Regs.CMPA.bit.CMPA = (-cmp);
//    	    }
//    	    else
//    	    {
//    	        EPwm1Regs.AQCSFRC.all= 9;
//    	    }
//
//            if(cmp > 0)
//            {
//                EPwm1Regs.CMPA.bit.CMPA = (-cmp + ((int)period - 1));
//            }
//            else
//            {
//                EPwm1Regs.CMPA.bit.CMPA = (-cmp + 1);
//            }
//
//    	}
//    	else
//    	{
//    	    EPwm1Regs.CMPA.bit.CMPA = (-cmp + period)*0.5;
//    	}
//    }
//    else if (id == 2)
//    {
////        cmp = -cmp;   //交错？
//        if(PWMONflag)
//        {
//            if(cmp > 100)
//            {
//                if(mux)
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFA = 0;
//                }
//                else
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFA = 1;  //A是下管
//                }
//                EPwm2Regs.AQCSFRC.bit.CSFB = 2;
//            }
//            else if(cmp < -100)
//            {
//                if(mux)
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFB = 0;
//                }
//                else
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFB = 2;
//                }
//                EPwm2Regs.AQCSFRC.bit.CSFA = 1;
//            }
//            else
//            {
//                EPwm2Regs.AQCSFRC.all= 9;
//            }
//
//            if(cmp > 0)
//            {
//                EPwm2Regs.CMPA.bit.CMPA = (-cmp + ((int)period - 1));
//            }
//            else
//            {
//                EPwm2Regs.CMPA.bit.CMPA = (-cmp + 1);
//            }
//        }
//        else
//        {
//            EPwm2Regs.CMPA.bit.CMPA = (-cmp + period)*0.5;
//        }
//    }
//}


/*************************************************
Function: UpdateINVPwmComp3
Description: 更新 INV PWM;
** 双极性调制方式
Input: @id-1-逆变1;2-逆变2; @cmp-占空比值 @period-PWM开关周期值
Output: None
Frequency:
*************************************************/
void UpdateINVPwmComp3(uint16_t id, int16_t cmp, int16_t period, uint16_t flag)
{
    static uint16_t mux = 1;

    volatile struct EPWM_REGS* EPwmRegsPT = &EPwm1Regs;

    if (id == 1)
    {
//        EPwmRegsPT = &EPwm1Regs;
//        mux = !mux;   //打嗝？
    }
    else if (id == 2)
    {
        EPwmRegsPT = &EPwm2Regs;
//        cmp = -cmp;   //交错？
    }

    if(PWMONflag)
    {
        if(cmp > 100)
        {
            if(mux)
            {
                EPwmRegsPT->AQCSFRC.bit.CSFA = 0;
            }
            else
            {
                EPwmRegsPT->AQCSFRC.bit.CSFA = 1;  //A是下管
            }
            EPwmRegsPT->AQCSFRC.bit.CSFB = 2;
        }
        else if(cmp < -100)
        {
            if(mux)
            {
                EPwmRegsPT->AQCSFRC.bit.CSFB = 0;
            }
            else
            {
                EPwmRegsPT->AQCSFRC.bit.CSFB = 2;
            }
            EPwmRegsPT->AQCSFRC.bit.CSFA = 1;
        }
        else
        {
            EPwmRegsPT->AQCSFRC.all= 9;
        }

        if(cmp > 0)
        {
            EPwmRegsPT->CMPA.bit.CMPA = (-cmp + ((int)period - 1));
        }
        else
        {
            EPwmRegsPT->CMPA.bit.CMPA = (-cmp + 1);
        }
    }
    else
    {
        EPwmRegsPT->CMPA.bit.CMPA = (-cmp + (int)period)*0.5;
    }
}
//void UpdateINVPwmComp(uint16_t id, int16_t cmp, uint16_t period, uint16_t flag)
//{
//    if (id == 1)
//    {
//        if(PWMONflag)
//        {
//            if(cmp > 30)
//            {
//                if(EPwm1Regs.AQCSFRC.bit.CSFB == 0)
//                {
//                    EPwm1Regs.AQCSFRC.all= 9;
//                }
//                else
//                {
//                    EPwm1Regs.AQCSFRC.bit.CSFA = 0; //开启上管
//                    EPwm1Regs.AQCSFRC.bit.CSFB = 2;  //关闭下管
//                }
//            }
//            else if(cmp < -30)
//            {
//                if(EPwm1Regs.AQCSFRC.bit.CSFA == 0)
//                {
//                    EPwm1Regs.AQCSFRC.all= 9;
//                }
//                else
//                {
//                    EPwm1Regs.AQCSFRC.bit.CSFB = 0;
//                    EPwm1Regs.AQCSFRC.bit.CSFA = 1;  //关闭上管
//                }
//            }
//            else
//            {
//                EPwm1Regs.AQCSFRC.all= 9;
//            }
//
//            if(cmp > 0)
//            {
//                EPwm1Regs.CMPA.bit.CMPA = (-cmp + ((int)period - 1));
//            }
//            else
//            {
//                EPwm1Regs.CMPA.bit.CMPA = (-cmp + 1);
//            }
//
//        }
//        else
//        {
//            EPwm1Regs.CMPA.bit.CMPA = (-cmp + period)*0.5;
//        }
//    }
//    else if (id == 2)
//    {
//        cmp = -cmp;
//        if(PWMONflag)
//        {
//            if(cmp > 30)
//            {
//                if(EPwm2Regs.AQCSFRC.bit.CSFB == 0)
//                {
//                    EPwm2Regs.AQCSFRC.all= 9;
//                }
//                else
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFA = 0; //开启上管
//                    EPwm2Regs.AQCSFRC.bit.CSFB = 2;  //关闭下管
//                }
//            }
//            else if(cmp < -30)
//            {
//                if(EPwm2Regs.AQCSFRC.bit.CSFA == 0)
//                {
//                    EPwm2Regs.AQCSFRC.all= 9;
//                }
//                else
//                {
//                    EPwm2Regs.AQCSFRC.bit.CSFB = 0;
//                    EPwm2Regs.AQCSFRC.bit.CSFA = 1;  //关闭上管
//                }
//            }
//            else
//            {
//                EPwm2Regs.AQCSFRC.all= 9;
//            }
//
//            if(cmp > 0)
//            {
//                EPwm2Regs.CMPA.bit.CMPA = (-cmp + ((int)period - 1));
//            }
//            else
//            {
//                EPwm2Regs.CMPA.bit.CMPA = (-cmp + 1);
//            }
//        }
//        else
//        {
//            EPwm2Regs.CMPA.bit.CMPA = (-cmp + period)*0.5;
//        }
//    }
//}


/*************************************************
Function: InitEPwm
Description: 初始化所有PWM配置
Input: None
Output: None
Frequency:Once
*************************************************/
void InitEPwm(void)
{
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;      // Stop all the TB clocks
    EDIS;

    // GPIO 初始化
    InitAllEPwmGpio();
    // INV 配置
    ConfigINVPwm();

    INV_PWM_P_ON;//允许开启PWM3、PWM4

    DISABLE_INV1PWM;
    DISABLE_INV2PWM;
    DISABLE_INV3PWM;
    DISABLE_INV4PWM;

    InitINV1Tz();
    InitINV2Tz();
    InitINVOCXint();

#if 0
//    LLC 配置
    ConfigLLCpwm();
    DisableAllLLCPwm(LLC_EPWM_PERIOD);
    InitLLCTZ();
#endif
    // BUS 配置
    ConfigBUSPwm();

    // PV 配置
    ConfigPVpwm();
    InitPV1Tz();
    InitPV2Tz();
    InitPV3Tz();
    InitPV4Tz();
    DISABLE_PV1PWM;
    DISABLE_PV2PWM;
    DISABLE_PV3PWM;
    DISABLE_PV4PWM;
    // FAN
    ConfigFanPwm();
    StopFan();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
    //尝试错开采样触发时机
    EPwm1Regs.TBCTR = 0;
    EPwm2Regs.TBCTR = 0;
    EPwm6Regs.TBCTR = 500;
    EPwm7Regs.TBCTR = PV_PWM_PERIOD/4 + 500;
    EPwm10Regs.TBCTR = PV_PWM_PERIOD/4*2 + 500;
    EPwm5Regs.TBCTR = PV_PWM_PERIOD/4*3 + 500;
}

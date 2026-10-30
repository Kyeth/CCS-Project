//#############################################################################
//
/**********************************Copyright(c)********************************
**
**                               PowerOak Co.,LTD.
**
**
**
**--------------File
**Info-------------------------------------------------------------------------
** File name: bsp.h
** Latest modified Date:2022年2月11日
** Latest Version: V1.0
** Description: 该文件是用户开发的BSP驱动公共头文件,
** 定义了驱动各个模块参数配置宏,声明了驱动函数接口以及函数使用;硬件平台28374S
** 第一部分： Defines related to clock configuration
** 第二部分： Defines for pin numbers and other GPIO configuration
** 第三部分: Defines data type,Declare Modular variable
** 第四部分 ：Function Prototypes
**-----------------------------------------------------------------------------
** Created by:tzz
** Created date: 2022年2月11日
** Version: V1.0
** Descriptions:
**-----------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version: V1.0
** Descriptions:
******************************************************************************/

#ifndef BSP_INC_BSP_H_
#define BSP_INC_BSP_H_

#include "F28x_Project.h"

//*****************************************************************************
//
// TODO ----- DIDO
//
//*****************************************************************************
//=====================================GPIO输出声明=====================================//

//==========================GPIO22 TEST脚
#define     TEST_ON    GpioDataRegs.GPASET.bit.GPIO22 = 1;    // 置高
#define     TEST_OFF   GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;  // 置低
#define     TEST_TOGGLE GpioDataRegs.GPATOGGLE.bit.GPIO22 = 1;  // 置低
//==========================GPIO32 与门关断同步PWM驱动信号
#define     INV_PWM_P_ON        GpioDataRegs.GPBSET.bit.GPIO32 = 1;        // 允许PWM3和4的驱动
#define     INV_PWM_P_OFF       GpioDataRegs.GPBCLEAR.bit.GPIO32 = 1;      // 禁止PWM3和4的驱动
//==========================GPIO42 多路选择器C-0 S2
#define     SEL_C_S2_ON         GpioDataRegs.GPBSET.bit.GPIO42 = 1;        // 选择器S2闭合
#define     SEL_C_S2_OFF        GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;      // 选择器S2断开
//==========================GPIO46 多路选择器A-0 S0
#define     SEL_A_S0_ON         GpioDataRegs.GPBSET.bit.GPIO46 = 1;        // 选择器S0闭合
#define     SEL_A_S0_OFF        GpioDataRegs.GPBCLEAR.bit.GPIO46 = 1;      // 选择器S0断开
//==========================GPIO47 多路选择器B-0 S1
#define     SEL_B_S1_ON         GpioDataRegs.GPBSET.bit.GPIO47 = 1;        // 选择器S1闭合
#define     SEL_B_S1_OFF        GpioDataRegs.GPBCLEAR.bit.GPIO47 = 1;      // 选择器S1断开
//==========================GPIO50 第1路PV正极绝缘检测继电器
#define     PV1_ISO_TEST_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO50 = 1;    // 继电器闭合
#define     PV1_ISO_TEST_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO50 = 1;  // 继电器断开
//==========================GPIO51 第2路PV正极绝缘检测继电器驱动
#define     PV2_ISO_TEST_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO51 = 1;    // 继电器闭合
#define     PV2_ISO_TEST_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO51 = 1;  // 继电器断开
//==========================GPIO52 用于内循环风扇控制
#define     FAN2_ON     GpioDataRegs.GPBSET.bit.GPIO52 = 1;    // 置高
#define     FAN2_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO52 = 1;  // 置低
//==========================GPIO58 用于L1相并网继电器控制
#define     GRID_L1_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO58 = 1;      // 继电器闭合
#define     GRID_L1_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO58 = 1;    // 继电器断开
//==========================GPIO59 用于L2相中间继电器控制
#define     MID_L2_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO59 = 1;      // 继电器闭合
#define     MID_L2_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO59 = 1;    // 继电器断开
//==========================GPIO60 用于L1相中间继电器控制
#define     MID_L1_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO60 = 1;      // 继电器闭合
#define     MID_L1_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO60 = 1;    // 继电器断开
//==========================GPIO61 用于L2相并网继电器控制
#define     GRID_L2_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO61 = 1;      // 继电器闭合
#define     GRID_L2_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO61 = 1;    // 继电器断开
//==========================GPIO62 N相中间继电器控制
#define     N_MID_RLY_ON    {GpioDataRegs.GPBSET.bit.GPIO62 = 1;InvStateData.MidRlyFlg = 1;};   // 继电器闭合
#define     N_MID_RLY_OFF   {GpioDataRegs.GPBCLEAR.bit.GPIO62 =1;InvStateData.MidRlyFlg = 0;};    // 继电器断开
//==========================GPIO63 N相并网继电器控制
#define     N_GRID_RLY_ON     GpioDataRegs.GPBSET.bit.GPIO63 = 1;    // 继电器闭合
#define     N_GRID_RLY_OFF    GpioDataRegs.GPBCLEAR.bit.GPIO63 = 1;  // 继电器断开
//==========================GPIO66 Load继电器N线驱动
#define     N_LOAD_RLY_ON       GpioDataRegs.GPCSET.bit.GPIO66 = 1;    // 继电器闭合
#define     N_LOAD_RLY_OFF      GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;  // 继电器断开
//==========================GPIO67 Load继电器驱动
#define     LOAD_RLY1_ON     GpioDataRegs.GPCSET.bit.GPIO67 = 1;    // 继电器闭合
#define     LOAD_RLY1_OFF    GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;  // 继电器断开
//==========================GPIO68 Load继电器驱动
#define     LOAD_RLY2_ON     GpioDataRegs.GPCSET.bit.GPIO68 = 1;    // 继电器闭合
#define     LOAD_RLY2_OFF    GpioDataRegs.GPCCLEAR.bit.GPIO68 = 1;  // 继电器断开
//==========================GPIO69 驱动电源开关
#define     SPS_ON     GpioDataRegs.GPCSET.bit.GPIO69 = 1;    // 置高
#define     SPS_OFF    GpioDataRegs.GPCCLEAR.bit.GPIO69 = 1;  // 置低
//==========================GPIO70 接地继电器驱动
#define     PE_RLY_ON     GpioDataRegs.GPCSET.bit.GPIO70 = 1;      // 继电器闭合
#define     PE_RLY_OFF    GpioDataRegs.GPCCLEAR.bit.GPIO70 = 1;    // 继电器断开
//==========================GPIO71   GFCI-50MA
#define     GFCI_50mA_ON    GpioDataRegs.GPCCLEAR.bit.GPIO71 = 1;  // GFCI漏电流注入
#define     GFCI_50mA_OFF   GpioDataRegs.GPCSET.bit.GPIO71 = 1;    // GFCI漏电流结束
//==========================GPIO73 BUS预充继电器控制
#define     BUS_RLY_ON      GpioDataRegs.GPCSET.bit.GPIO73 = 1;    // 继电器闭合
#define     BUS_RLY_OFF     GpioDataRegs.GPCCLEAR.bit.GPIO73 = 1;  // 继电器断开
//==========================GPIO74 M3辅助源继电器控制
#define     AUS_RLY_ON     GpioDataRegs.GPCCLEAR.bit.GPIO74 = 1;   // 继电器闭合
#define     AUS_RLY_OFF    GpioDataRegs.GPCSET.bit.GPIO74 = 1;     // 继电器断开
//==========================GPIO75 DSP OK信号灯输出
#define     DSP_OK_ON   GpioDataRegs.GPCSET.bit.GPIO75 = 1 ;    // 灯亮
#define     DSP_OK_OFF  GpioDataRegs.GPCCLEAR.bit.GPIO75 = 1;   // 灯灭
#define     TOGGLE_RUN_LED GpioDataRegs.GPCTOGGLE.bit.GPIO75 = 1;   // 灯闪
//==========================GPIO87  激活电池使能信号
#define     POWER_ON    GpioDataRegs.GPCSET.bit.GPIO87 = 1;     // 置高
#define     POWER_OFF   GpioDataRegs.GPCCLEAR.bit.GPIO87 = 1;   // 置低
//==========================GPIO99  过流基准信号
#define     CURR_REF_ON    GpioDataRegs.GPDSET.bit.GPIO99 = 1;     // 过流阈值砍半
#define     CURR_REF_OFF   GpioDataRegs.GPDCLEAR.bit.GPIO99 = 1;   // 过流阈值恢复
//=====================================GPIO输入声明=====================================//
    //==================GPIO11 内部风扇故障
#define FAN2_FAULT_DATA  GpioDataRegs.GPADAT.bit.GPIO11
    //==================GPIO28 PV1反灌过流保护
#define PV1_REOC_DATA    GpioDataRegs.GPADAT.bit.GPIO28
    //==================GPIO29 PV2反灌过流保护
#define PV2_REOC_DATA    GpioDataRegs.GPADAT.bit.GPIO29
    //==================GPIO30 PV3反灌过流保护
#define PV3_REOC_DATA    GpioDataRegs.GPADAT.bit.GPIO30
    //==================GPIO31 PV4反灌过流保护
#define PV4_REOC_DATA    GpioDataRegs.GPADAT.bit.GPIO31
    //==================GPIO76 MCU-ARM OK信号
#define MCU_OK_DATA     GpioDataRegs.GPCDAT.bit.GPIO76
    //==================GPIO77 平衡电路过流输入
#define BUS_OC_DATA     GpioDataRegs.GPCDAT.bit.GPIO77
    //==================GPIO78 外部风扇故障
#define FAN_FAULT_DATA  GpioDataRegs.GPCDAT.bit.GPIO78
    //==================GPIO79 L1相逆变过流(220A)砍波保护
#define INV1_OCP_DATA    GpioDataRegs.GPCDAT.bit.GPIO79
    //==================GPIO80 硬件BUS过压输入
#define BUS_OV_DATA     GpioDataRegs.GPCDAT.bit.GPIO80
    //==================GPIO81 L2相逆变过流(220A)砍波保护
#define INV2_OCP_DATA     GpioDataRegs.GPCDAT.bit.GPIO81
    //==================GPIO83 逆变不可控过流(240A)短路封波
#define AC_OC_DATA      GpioDataRegs.GPCDAT.bit.GPIO83
    //==================GPIO86 12V辅助电源欠压(10.08V)告警
#define POWER_LOW_DATA  GpioDataRegs.GPCDAT.bit.GPIO86

// TODO ePWM
// ePWM 时钟是 系统时钟的 1/2,和 DSP280049是有差异的
//-----------------INV -------------------------------------------
#define PWM_ISR_FREQUENCY (10*1000)             // PWM 中断频率 改为单次触发中断 (10*1000)->(20*1000)
#define SYSTEM_FREQUENCY  (200000000l)          // 系统频率 200M
#define PWM_SYSTEM_FREQUENCY  (200000000l/2)    // EPWM 时钟频率 100M EPWMCLK
#define INV_PWM_FREQ_KHZ  (20.0f)               // PWM 频率  20KHz
#define INV_EPWM_PERIOD   (((PWM_SYSTEM_FREQUENCY / 1000.0f) / INV_PWM_FREQ_KHZ) / 2.0f)   // 2500
// 死区设置
#define DEAD_TIME_RISEON  (0.75f)    // 死区-单位us
#define DEAD_TIME_FALLOFF (0.75f)    // 死区-单位us
#define DEADTIME_RISEON_COUNT (PWM_SYSTEM_FREQUENCY / 1000000 * DEAD_TIME_RISEON)
#define DEADTIME_FALLOFF_COUNT (PWM_SYSTEM_FREQUENCY / 1000000 * DEAD_TIME_FALLOFF)

#define ENABLE_INV1PWM  {EPwm1Regs.AQCSFRC.bit.CSFA = 0;EPwm1Regs.AQCSFRC.bit.CSFB = 0;}

#define ENABLE_INV2PWM  {EPwm2Regs.AQCSFRC.bit.CSFA = 0;EPwm2Regs.AQCSFRC.bit.CSFB = 0;}

#define ENABLE_INV3PWM  {EPwm3Regs.AQCSFRC.bit.CSFA = 0;EPwm3Regs.AQCSFRC.bit.CSFB = 0;}

#define ENABLE_INV4PWM  {EPwm4Regs.AQCSFRC.bit.CSFA = 0;EPwm4Regs.AQCSFRC.bit.CSFB = 0;}

#define DISABLE_INV1PWM  {EPwm1Regs.AQCSFRC.all= 9;\
                         EPwm1Regs.CMPA.bit.CMPA = 0.5*EPwm1Regs.TBPRD;}

#define DISABLE_INV2PWM  {EPwm2Regs.AQCSFRC.all= 9;\
                         EPwm2Regs.CMPA.bit.CMPA = 0.5*EPwm2Regs.TBPRD;}

#define DISABLE_INV3PWM  {EPwm3Regs.AQCSFRC.all= 9;}

#define DISABLE_INV4PWM  {EPwm4Regs.AQCSFRC.all= 9;}
//---------------PV--------------------------------------------------
#define PV_PWM_FREQ_KHZ  (20.0f)     // PV 驱动开关频率20K
#define PV_PWM_PERIOD (((PWM_SYSTEM_FREQUENCY / 1000.0f) / PV_PWM_FREQ_KHZ) - 1)        // 2221

#define DISABLE_PV1PWM  {EPwm6Regs.CMPA.bit.CMPA = 1;EPwm6Regs.AQCSFRC.bit.CSFA = 1;}
#define DISABLE_PV2PWM  {EPwm7Regs.CMPA.bit.CMPA = 1;EPwm7Regs.AQCSFRC.bit.CSFA = 1;}
#define DISABLE_PV3PWM  {EPwm10Regs.CMPA.bit.CMPA = 1;EPwm10Regs.AQCSFRC.bit.CSFA = 1;}
#define DISABLE_PV4PWM  {EPwm5Regs.CMPA.bit.CMPA = 1;EPwm5Regs.AQCSFRC.bit.CSFA = 1;}

#define ENABLE_PV1PWM  {EPwm6Regs.AQCSFRC.bit.CSFA = 0;}
#define ENABLE_PV2PWM  {EPwm7Regs.AQCSFRC.bit.CSFA = 0;}
#define ENABLE_PV3PWM  {EPwm10Regs.AQCSFRC.bit.CSFA = 0;}
#define ENABLE_PV4PWM  {EPwm5Regs.AQCSFRC.bit.CSFA = 0;}
#if 0
//---------------LLC-------------------------------------------------
#define LLC_PWM_FREQ_KHZ (80.0f)    // PWM 频率 100KHz   87.0f->83.0F->80.5->79->78.5->78
#define LLC_EPWM_PERIOD (((PWM_SYSTEM_FREQUENCY / 1000.0f) / LLC_PWM_FREQ_KHZ) / 2.0f)     // 500

#define ENABLE_LLC1PWM {EPwm6Regs.AQCSFRC.bit.CSFA = 0;EPwm6Regs.AQCSFRC.bit.CSFB = 0;\
                        EPwm5Regs.AQCSFRC.bit.CSFA = 0;EPwm5Regs.AQCSFRC.bit.CSFB = 0;}
#define ENABLE_LLC2PWM {EPwm8Regs.AQCSFRC.bit.CSFA = 0;EPwm8Regs.AQCSFRC.bit.CSFB = 0;\
                        EPwm7Regs.AQCSFRC.bit.CSFA = 0;EPwm7Regs.AQCSFRC.bit.CSFB = 0;}

#define DISABLE_LLC1PWM {EPwm5Regs.CMPA.bit.CMPA = LLC_EPWM_PERIOD-1;EPwm5Regs.CMPB.bit.CMPB = 1;\
                        EPwm5Regs.AQCSFRC.bit.CSFA = 1;EPwm5Regs.AQCSFRC.bit.CSFB = 1;\
                        EPwm6Regs.CMPA.bit.CMPA = LLC_EPWM_PERIOD-1;EPwm6Regs.CMPB.bit.CMPB = 1;\
                        EPwm6Regs.AQCSFRC.bit.CSFA = 1;EPwm6Regs.AQCSFRC.bit.CSFB = 1;}
#define DISABLE_LLC2PWM {EPwm7Regs.CMPA.bit.CMPA = LLC_EPWM_PERIOD-1;EPwm7Regs.CMPB.bit.CMPB = 1;\
                        EPwm7Regs.AQCSFRC.bit.CSFA = 1;EPwm7Regs.AQCSFRC.bit.CSFB = 1;\
                        EPwm8Regs.CMPA.bit.CMPA = LLC_EPWM_PERIOD-1;EPwm8Regs.CMPB.bit.CMPB = 1;\
                        EPwm8Regs.AQCSFRC.bit.CSFA = 1;EPwm8Regs.AQCSFRC.bit.CSFB = 1;}
#endif

#define BUS_PWM_FREQ_KHZ  (20.0f)     // 单位KHz              // BUS 驱动开关频率 35K
#define BUS_PWM_PERIOD (((PWM_SYSTEM_FREQUENCY / 1000.0f) / BUS_PWM_FREQ_KHZ) / 2.0f)        // 1428
#define BUS_PWM_PERIOD_HALF (BUS_PWM_PERIOD / 2.0f)

#define DISABLE_BUSPWM  {EPwm8Regs.CMPA.bit.CMPA = BUS_PWM_PERIOD_HALF;EPwm8Regs.AQCSFRC.bit.CSFA = 1;EPwm8Regs.AQCSFRC.bit.CSFB = 1;}
#define ENABLE_BUSPWM   {EPwm8Regs.AQCSFRC.bit.CSFA = 0;EPwm8Regs.AQCSFRC.bit.CSFB = 0;}
//---------------FAN--------------------------------------------------
#define FAN_PWM_FREQ_KHZ  (10.0f)     // FAN 驱动开关频率 25K->10
#define FAN_PWM_PERIOD (((PWM_SYSTEM_FREQUENCY / 1000.0f) / FAN_PWM_FREQ_KHZ) / 2.0f)      // 2500
#if 0
// 使用了 EALLOW EDIS 不能被打断,否则可能存在问题
#define REENABLELLC1TZ  {EALLOW;EPwm5Regs.TZCLR.bit.OST = 1;EPwm6Regs.TZCLR.bit.OST = 1;\
                        EPwm5Regs.TZCLR.bit.INT = 1; EDIS;}
#define REENABLELLC2TZ  {EALLOW;EPwm7Regs.TZCLR.bit.OST = 1;EPwm8Regs.TZCLR.bit.OST = 1;\
                        EPwm7Regs.TZCLR.bit.INT = 1; EDIS;}
#endif

// TODO ---------------DI/O------------------------------------------------
void InitDIDO(void);

// TODO ---------------ADC-------------------------------------------------
void InitADC(void);

// TODO ---------------EPWM------------------------------------------------
void InitEPwm(void);
//void ConfigPVpwm(void);
__interrupt void epwm5_tzint_isr(void);
__interrupt void epwm7_tzint_isr(void);
__interrupt void xint2_isr(void);
__interrupt void xint1_isr(void);
__interrupt void xint3_isr(void);

//void DisableINV1Pwm(void);
//void DisableINV2Pwm(void);
//void EnableINV1Pwm(void);
//void EnableINV2Pwm(void);

//void EnablePVPwm(uint16_t channel);
//void DisablePVPwm(uint16_t channel);

//void RecoverInvPwm(void);
void RecoverInvPwm2(void);
void DischargeInvC(uint16_t type);
//void UpdateINVPwmComp(uint16_t id, int16_t cmp, uint16_t period);
//void UpdateINVPwmComp24(uint16_t id, int16_t cmp, uint16_t period);

void UpdateINVPwmCompPhase(uint16_t id, int16_t cmp, uint16_t period, float phase);
//void UpdateINVPwmComp3(uint16_t id, int16_t cmp, uint16_t period);
void UpdateINVPwmComp(uint16_t id, int16_t cmp, uint16_t period, uint16_t flag);
void UpdateINVPwmComp3(uint16_t id, int16_t cmp, int16_t period, uint16_t flag);
void UpdateLLCPwmComp(uint16_t id, uint16_t cmpA, uint16_t cmpB);
void ClearINVPwmComp3(void);
void ClearINV1PwmComp3(void);
void ClearINV2PwmComp3(void);

void DisableLLCPWMLbyID(uint16_t id);
void DisableLLCPWMHbyID(uint16_t id);
void EnableLLCPWMLbyID(uint16_t id);
void EnableLLCPWMHbyID(uint16_t id);
// FAN
void StopFan(void);
void StartFan(void);
void SetFanSpeed(float speed);

__interrupt void ISREpwmcLA(void);
__interrupt void ISREpwm1(void);
__interrupt void adc_isr(void);

// TODO ---------------Timer--------------------------------------------
typedef struct {
    Uint16 reset;
    Uint32 Timer;
}mSecCNT_struct;
void InitUserTimers(void);
interrupt void cpu_timer0_isr(void);
Uint16 User_usTimer(Uint32 *User_Timer, Uint32 mSec);     // 1ms定时
Uint32 User_usCount(mSecCNT_struct *mSecCNT);

// TODO ---------------SCI----------------------------------------------
void InitSci(char sci);

// TODO ---------------ECAP---------------------------------------------
void InitECap(void);

// TODO ---------------ECAN---------------------------------------------
void InitCan(void);
void InitCanOscilloscope(void);
// TODO ---------------IIC---------------------------------------------
void InitI2C(void);
void InitBSP(void);

void EnableDog(void);

void CLA_Init(void);

extern uint16_t PWMONflag;
#endif /* BSP_INC_BSP_H_ */

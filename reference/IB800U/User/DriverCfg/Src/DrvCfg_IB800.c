/*
 * DrvCfg_Ep1300.c
 *
 *  Created on: 2025年5月16日
 *      Author: 17537
 */

#include "DrvCfg_IB800.h"
#include "TaskCla.h"
#include "InterruptCfg.h"

#ifdef __cplusplus
    extern "C" {
#endif

/*
 *  所有用到的IO口初始化配置均在此处
 */
ST_GPIO_CFG_ITEM stGpioCfgItem[] =
{
     {6,    GPIO,       DIR_DO, 1, GPIO_OD_DISABEL},        // LED1
     {34,   GPIO,       DIR_DO, 1, GPIO_OD_DISABEL},        // LED2

     {17,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 低压辅源欠压保护
     {20,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 外部风扇故障
     {21,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 内部风扇故障
     {40,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 高压辅源欠压保护
     {47,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 支路1过流保护
     {49,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 支路2过流保护
     {53,   GPIO,       DIR_DI, 1, GPIO_OD_DISABEL},        // 输入过流保护
     
     {5,    GPIO,       DIR_DO, 1, GPIO_OD_DISABEL},        // CAN终端电阻开关
     {23,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // Relay
     {16,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // Fan
	 {33,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // Fan2
     {41,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // 预充电阻
     {50,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // 加热膜1档控制信号
     {51,   GPIO,       DIR_DO, 0, GPIO_OD_DISABEL},        // 加热膜2档控制信号

     {0,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM1A
     {1,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM1B

     {2,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM2A
     {3,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM2B

     {4,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM3A 加热膜隔离电源驱动信号
//     {5,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM3B

     {22,   PERIPH11,   DIR_DO, 1, GPIO_OD_DISABEL},          // PWM4A
     {7,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM4B

     {8,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM5A
     {9,    PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM5B

     {10,   PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM6A  加热膜主路控制信号
//     {11,   PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM6B

     {14,   PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // PWM8A  风扇控制信号

     {29,   PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // SCIA_TX
     {28,   PERIPH1,    DIR_DI, 1, GPIO_OD_DISABEL},          // SCIA_RX    SCI不能使能开漏模式

     {12,   PERIPH5,    DIR_DO, 1, GPIO_OD_DISABEL},          // SCIB_TX
     {13,   PERIPH5,    DIR_DI, 1, GPIO_OD_DISABEL},          // SCIB_RX    SCI不能使能开漏模式

     {31,   PERIPH1,    DIR_DO, 1, GPIO_OD_DISABEL},          // CANA_TX
     {30,   PERIPH1,    DIR_DI, 1, GPIO_OD_DISABEL},          // CANA_RX    CAN不能使能开漏模式

//     {12,   PERIPH5,    DIR_DO, 1, GPIO_OD_DISABEL},          // M_CANB_TX
//     {13,   PERIPH5,    DIR_DI, 1, GPIO_OD_DISABEL},          // M_CANB_RX    M_CAN不能使能开漏模式

     {27,   PERIPH9,    DIR_DO, 1, GPIO_OD_DISABEL},          // I2C_SCL
     {26,   PERIPH9,    DIR_DI, 1, GPIO_OD_DISABEL},          // I2C_SDA

     {25,   GPIO,    DIR_DO, 0, GPIO_OD_DISABEL},           // I2C_WP

     {60,   PERIPH3,    DIR_DO, 1, GPIO_OD_DISABEL},          // 调试MCANA_TX
     {61,   PERIPH3,    DIR_DI, 1, GPIO_OD_DISABEL},          // 调试MCANA_RX    CAN不能使能开漏模式
};

ST_GPIO_CFG stGpioCfg = {stGpioCfgItem, sizeof(stGpioCfgItem)/sizeof(stGpioCfgItem[0])};



/*
 * 外部中断初始化配置
 */
ST_EXIT_CFG_ITEM stExitCfgItem[] =
{
     {EXIT_ID0, 47,  0, 0},
     {EXIT_ID1, 49,  0, 0},
     {EXIT_ID2, 53,  0, 0},
};
ST_EXIT_CFG stExitCfg = {stExitCfgItem, sizeof(stExitCfgItem)/sizeof(stExitCfgItem[0])};



/*
 * SCI外设初始化配置
 */
ST_SCI_CFG_ITEM stSciCfgItem[] =
{
     {SCI_ID0, 38400, DATABITS_8, STOPBITS_1, DISABLE_PARITY, 0, 0, 0, 0},
     {SCI_ID1, 38400, DATABITS_8, STOPBITS_1, DISABLE_PARITY, 0, 0, 0, 0},
};
ST_SCI_CFG stSciCfg = {stSciCfgItem, sizeof(stSciCfgItem)/sizeof(stSciCfgItem[0])};



///*
// * 输入捕获初始化配置
// */
//ST_ECAP_CFG_ITEM stEcapCfgItemp[] =
//{
//     {ECAP_ID0, 24, FALLING_EDGE, DISABLE_EDGE, DISABLE_EDGE, DISABLE_EDGE, EC_ONESHOT},
//     {ECAP_ID1, 25, FALLING_EDGE, DISABLE_EDGE, DISABLE_EDGE, DISABLE_EDGE, EC_ONESHOT},
//     {ECAP_ID2, 26, FALLING_EDGE, DISABLE_EDGE, DISABLE_EDGE, DISABLE_EDGE, EC_ONESHOT},
//     {ECAP_ID3, 27, FALLING_EDGE, DISABLE_EDGE, DISABLE_EDGE, DISABLE_EDGE, EC_ONESHOT},
//     {ECAP_ID4, 29, FALLING_EDGE, DISABLE_EDGE, DISABLE_EDGE, DISABLE_EDGE, EC_ONESHOT},
//};
//ST_CAP_CFG stEcapCfg = {stEcapCfgItemp, sizeof(stEcapCfgItemp)/sizeof(stEcapCfgItemp[0])};



/*
 * PWM外设初始化配置
 */
// 同步关系：从第一个PWM开始，每3个固定为一组（查看28374d用户手册1916页Figure15-7）
ST_PWM_CFG_ITEM stPwmCfgItem[] =
{
 //             PWM_ID     周期值   是否Gload   linkPWM   相位  主从同步  CNT计数方向      死区时间(通过控制死区时间来控制波形通断)  chA故障电平  chB故障电平 计数器模式                 同步输入源选择                         同步输出源选择
/* ePwm1 */     {PWM_ID0,  600,    1,           PWM_ID3,  0,  PWM_SLAVE,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7,     EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C },
/* ePwm2 */     {PWM_ID1,  600,    1,           PWM_ID3,  0,  PWM_SLAVE,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7,     EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C },
/* ePwm3 */     {PWM_ID2,  1200,   0,           PWM_ID2,  0,  PWM_MASTER,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_DISABLE,     0 },
/* ePwm4 */     {PWM_ID3,  600,    1,           PWM_ID3,  0,  PWM_SLAVE,     1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7,     0 },
/* ePwm5 */     {PWM_ID4,  600,    1,           PWM_ID3,  0,  PWM_SLAVE,     1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7,     0 },
/* ePwm6 */     {PWM_ID5,  12000,  0,           PWM_ID5,  0,  PWM_MASTER,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_DISABLE,     0 },
/* ePwm7 */     {PWM_ID6,  600,    1,           PWM_ID3,  0,  PWM_MASTER,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_DISABLE,     EPWM_SYNC_OUT_FROM_CNTR_ZERO },
/* ePwm8 */     {PWM_ID7,  600,    0,           PWM_ID7,  0,  PWM_MASTER,    1,        60,         2,          2,     EPWM_COUNTER_MODE_UP,  EPWM_SYNC_IN_FROM_DISABLE,     0 },
};

ST_PWM_CFG stPwmCfg = {stPwmCfgItem, sizeof(stPwmCfgItem)/sizeof(stPwmCfgItem[0])};



/*
 * ADC外设初始化配置
 */

// A4: NTC2采样
// A5：NTC1采样（在V2板中取消）
// A6: 内部环境温度采样
// A10: 加热膜电流采样（在V2板中新增）
// A11: 支路1电流采样


// B0: 输出电流采样
// B2: VBat电压采样
// B3: 空开前端电压采样
// B5：变压器1温度采样

// C2: 支路2电流采样
// C5: 变压器2温度采样
// C7: 空开后端电压采样
// C9: VBus电压采样


// 采样窗口时间最大为511
const ST_ADC_CH_CFG_ITEM stAdcChCfgItem[] =
{
 //         AdcA                AdcB                AdcC
 //       {触发源，采样窗口时间，采样通道}
/*SOC0*/    {0x00, 60, 0},      {0x05, 30, 0},      {0x00, 30, 0},
/*SOC1*/    {0x00, 60, 1},      {0x00, 60, 1},      {0x00, 60, 1},
/*SOC2*/    {0x00, 30, 2},      {0x05, 30, 2},      {0x05, 60, 2},
/*SOC3*/    {0x00, 30, 3},      {0x05, 30, 3},      {0x00, 60, 3},
/*SOC4*/    {0x05, 30, 4},      {0x00, 60, 4},      {0x00, 60, 4},
/*SOC5*/    {0x00, 30, 5},      {0x05, 30, 5},      {0x05, 30, 5},
/*SOC6*/    {0x05, 30, 6},      {0x00, 60, 6},      {0x00, 60, 6},
/*SOC7*/    {0x00, 60, 7},      {0x00, 60, 7},      {0x05, 30, 7},
/*SOC8*/    {0x00, 60, 8},      {0x00, 60, 8},      {0x00, 60, 8},
/*SOC9*/    {0x00, 60, 9},      {0x00, 60, 9},      {0x05, 60, 9},
/*SOC10*/   {0x05, 30, 10},     {0x00, 60, 10},     {0x00, 60, 10},
/*SOC11*/   {0x05, 60, 11},     {0x00, 60, 11},     {0x00, 60, 11},
/*SOC12*/   {0x00, 30, 12},     {0x00, 30, 12},     {0x00, 60, 12},
/*SOC13*/   {0x00, 60, 13},     {0x00, 60, 13},     {0x00, 60, 13},
/*SOC14*/   {0x00, 60, 14},     {0x00, 60, 14},     {0x00, 60, 14},
/*SOC15*/   {0x00, 60, 15},     {0x00, 60, 15},     {0x00, 60, 15},
};

// 关于产生中断的选择，如果使用adc同步采样，则选择使用数量最多的ADC，且最后的那个soc作为eoc的中断触发
const ST_ADC_CFG_ITEM stAdcCfgItem[] =
{
     {ADC_ID0, &stAdcChCfgItem[0], ADC_RESOLUTION_BIT_12, ADC_SIGNALMODE_SINGLE, 0, 11},        // 使用AdcA的12作为ADC采样完成中断
     {ADC_ID1, &stAdcChCfgItem[1], ADC_RESOLUTION_BIT_12, ADC_SIGNALMODE_SINGLE, 0, 3},
     {ADC_ID2, &stAdcChCfgItem[2], ADC_RESOLUTION_BIT_12, ADC_SIGNALMODE_SINGLE, 0, 4},
};

ST_ADC_CFG stAdcCfg = {(ST_ADC_CFG_ITEM *)&stAdcCfgItem[0], sizeof(stAdcCfgItem)/sizeof(stAdcCfgItem[0])};

/*
 * CAN外设初始化配置
 */
const ST_CANFIFO_CFG_ITEM stCanFifoCfgItem[] =
{
 // 邮箱编号    帧类型     邮箱操作方向  数据长度    接收中断使能 接收掩码使能 接收掩码    邮箱组 保留  邮箱接收的ID   保留
    {1,         1,          0,          8,          0,          0,          0x00000000, 0,      0,  0x0FE00000,     0},
    {2,         1,          0,          8,          0,          0,          0x00000000, 1,      0,  0x0FE00000,     0},
    {3,         1,          0,          8,          0,          0,          0x00000000, 1,      0,  0x0FE00000,     0},
};

ST_CANFIFO_CFG stCanFifoCfg = {(ST_CANFIFO_CFG_ITEM *)&stCanFifoCfgItem[0], sizeof(stCanFifoCfgItem)/ sizeof(stCanFifoCfgItem[0])};

const ST_CAN_CFG_ITEM stCanCfgItem[] =
{
    {CAN_ID0, CANBAUD_1000K, 0x0000000F, &stCanFifoCfg},
};
ST_CAN_CFG stCanCfg = {(ST_CAN_CFG_ITEM *)&stCanCfgItem[0], sizeof(stCanCfgItem)/sizeof(stCanCfgItem[0])};

/*
 * MCAN外设初始化配置
 */
const ST_MCAN_EXT_FILTER stMCanExtFilerItem[] =
{
 // 接收掩码      保留  邮箱接收的ID   保留
    {0xFFFF0000,      0,  0x00001,     0},
    {0xFFFF0000,      0,  0x00002,     0},
    {0xFFFF0000,      0,  0x00003,     0},
};

const ST_MCAN_STD_FILTER stMCanStdFilerItem[] =
{
 // 接收掩码      保留  邮箱接收的ID   保留
    {0xFFFF0000,      0,  0x00004,     0},
    {0xFFFF0000,      0,  0x00005,     0},
    {0xFFFF0000,      0,  0x00006,     0},
};

const ST_MCAN_CFG_ITEM stMCanCfgItem[] =
{
     {MCAN_ID0, MCANBAUD_500K,
      (ST_MCAN_EXT_FILTER *)&stMCanExtFilerItem[0], sizeof(stMCanExtFilerItem)/sizeof(stMCanExtFilerItem[0]),
      (ST_MCAN_STD_FILTER *)&stMCanStdFilerItem[0], sizeof(stMCanStdFilerItem)/sizeof(stMCanStdFilerItem[0])},
};
ST_MCAN_CFG stMCanCfg = {(ST_MCAN_CFG_ITEM *)&stMCanCfgItem[0], sizeof(stMCanCfgItem)/sizeof(stMCanCfgItem[0])};

/*
 * IIC外设初始化配置
 */
ST_I2C_CFG_ITEM stI2cCfgItem[] =
{
     {I2C_ID0, 0x01, 100000},
};
ST_I2C_CFG stI2cCfg = {stI2cCfgItem, sizeof(stI2cCfgItem)/sizeof(stI2cCfgItem[0])};

/*
 * CLA核心初始化配置
 */
ST_CLA_CFG_ITEM stClaCfgItem[] =
{
 //  任务编号       任务向量号   触发方式                CLA任务       任务完成中断服务函数
     {TASK_NO_1, CLA_MVECT_1,   CLA_TRIGGER_SOFTWARE, TaskCla_1,        TaskCla_1_Isr},
};
ST_CLA_CFG stClaCfg = {stClaCfgItem, sizeof(stClaCfgItem)/sizeof(stClaCfgItem[0])};

#ifdef __cplusplus
    }
#endif

#include "F2837xS_Device.h"
#include "F2837xS_Examples.h"


#include "string.h"

#include "DrvCfg_EP15K.h"

#include "Hw_WatchDog.h"

#include "Hw_GpioDriver.h"
#include "Hw_PwmDriver.h"
#include "InterruptCfg.h"
#include "Schedule.h"
#include "CtrlIsr.h"
/**
 * @brief 芯片外设初始化
 *
 */
void PeripheralsInit(void)
{
    //---------------------------------------------------------------------------
    DINT;   // Disable Global interrupt INTM
    //---------------------------------------------------------------------------
    // 1. 系统时钟、外设时钟、看门狗初始化
    InitSysCtrl();
    // 2. 初始化 PIE 控制器、向量表并注册中断（内部已完成 InitPieCtrl/InitPieVectTable）
    InterruptVectTableInit();    // 配置中断复位函数
//    Hw_WatchDogInit();    // 初始化看门狗



//    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
//    memcpy(&FlashAPIRunStart, &FlashAPILoadStart, (size_t)&FlashAPILoadSize);

    // 这里配置CPUTimer
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer2);

    // 初始化外设
    Hw_GpioInit(&stGpioCfg);    /* GPIO复用是外设引脚的前提 */
    Hw_PwmInit(&stPwmCfg);      /* PWM提供ADC的SOC触发源 */
    Hw_InvInit(&stInvCfg);      /* 逆变驱动绑定 R/S/T 相与正负半周模块，必须在 Hw_PwmInit 之后 */
    Hw_AdcInit(&stAdcCfg);      /* ADC依赖PWM的SOC */
    Hw_ClaInit(&stClaCfg);      /* CLA由ADC INT触发，内部完成程序搬运 */
    Hw_EcapInit(&stEcapCfg);    /* eCAP1~6：电网过零与电网频率信号捕获 */
    Hw_SciInit(&stSciCfg);      /* SCI-A/C/D 串口，SCIC 与外部TVS通信 */
    Hw_CanInit(&stCanCfg);      /* CAN-A / CAN-B */
//    Hw_ExitInit(&stExitCfg);    /* 外部中断最后，依赖PIE已配置 */


    // 开启定时器
    // 这里开启定时器的目的是为了使得通信相关接口可以正常工作
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);   /* 100us 节拍，驱动 SYS_TICK */
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer2);   /* 1ms 心跳 */

    EINT;   // Enable Global interrupt INTM
    ERTM;   // Enable Global real-time interrupt DBGM
}

/**
 * @brief 应用层数据对象初始化
 *
 */
void ApplicationInit(void)
{}

/**
 * @brief 平台组件功能初始化
 *
 */
void PlatformInit(void)
{
    CtrlIsrInit();   /* 控制中断状态复位，并把 MUX 选通到 PV1 */
}

/**
 * @brief 任务创建
 *
 * 任务中不允许存在死循环，一定需要存在退出条件
 * 否则会触发看门狗复位
 */
void TaskCreate(void)
{}

/**
 * @brief 启动系统
 *
 */
void StartSystem(void)
{}


void main(void)
{
    PeripheralsInit();// 初始化外设
    ApplicationInit();// 应用层数据对象初始化
    PlatformInit();   // 平台组件功能初始化
    TaskCreate();     // 创建所有任务
    StartSystem();    // 启动系统

    while(1)
    {
        Hw_FeedWatchDog(); // 喂狗
        Task_Run();
    }
}

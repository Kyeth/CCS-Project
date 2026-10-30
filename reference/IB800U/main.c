/**
 * @file main.c
 * @author hanyuhang (008467)
 * @brief
 * @version 0.1
 * @date 2025-06-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <Device.h>
#include "ModbusSlaveComm.h"
#include "driverlib.h"
#include "device.h"
#include "f28x_project.h"
#include "LLCControl.h"
#include "Hw_SysCpuTimer.h"
#include "SysStateMachine.h"
#include "Schedule.h"
#include "InterruptCfg.h"
#include "DrvCfg_IB800.h"
#include "XmodemSlaveProcess.h"
#include "InternalCanProcess.h"
#include "FM24C128D.h"
#include "Hw_WatchDog.h"
#include "FM24C128DProcess.h"
#include "PlatformManager.h"
#include "ModbusSlaveBusiness.h"
#include "Hw_MCanDriver.h"
#include "FaultProtect.h"
#include "VersionUser.h"
#include "ParallelProcess.h"
#include "ParamSaveProcess.h"
#include "UpgradeUser.h"
#include "Led.h"
#include "Heating.h"
#include "Fan.h"
#include "User_CAN.h"

/**
 * @brief 芯片外设初始化
 * 
 */
void PeripheralsInit(void)
{
    //---------------------------------------------------------------------------
    EALLOW;
    DINT;   // Disable Global interrupt INTM
    EDIS;
    //---------------------------------------------------------------------------

    Hw_WatchDogInit(5);    // 初始化看门狗超时时间为3ms
    Device_init();         // 初始化系统时钟
    Device_initGPIO();     // 解锁所有GPIO
    InterruptVectTableInit();    // 配置中断复位函数

    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
    memcpy(&FlashAPIRunStart, &FlashAPILoadStart, (size_t)&FlashAPILoadSize);

    // 这里配置一个CPUTime1
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer2);

    // 初始化外设
    Hw_GpioInit(&stGpioCfg);
    Hw_SciInit(&stSciCfg);
    Hw_CanInit(&stCanCfg);
    Hw_AdcInit(&stAdcCfg);
    Hw_PwmInit(&stPwmCfg);
    Hw_I2cInit(&stI2cCfg);
    Hw_ClaInit(&stClaCfg);
    Hw_MCanInit(&stMCanCfg);
    Hw_ExitInit(&stExitCfg);

    // 开启定时器
    // 这里开启定时器的目的是为了使得通信相关接口可以正常工作
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer2);

    EALLOW;
    EINT;   // Enable Global interrupt INTM
    ERTM;   // Enable Global real-time interrupt DBGM
    EDIS;
}

/**
 * @brief 应用层数据对象初始化
 * 
 */
void ApplicationInit(void)
{
    InitTestData();    // 调试使用

    InitTrapFilter(&ObjBusFilter, 50);
//    InitTrapFilter(&ObjIoscFilter);
//    InitTrapFilter(&ObjIbusFilter);

    PlatformSimplingCoefficientInit(&ObjSample);    // 采样系数初始化

//    InitTestData();
//    InitLoopData();
}

/**
 * @brief 平台组件功能初始化
 * 
 */
void PlatformInit(void)
{
    VersionUserInit();    // 版本初始化
    ModbusSlaveInit();    // Modbus从站初始化
    SysProtectPointInit(&stSysProtect);    // 系统保护点设置
//    Flash280039CInit();
    InitLowPowerModeData();
    BatteryParallelInit();    // 电池并包初始化
    FM24C128DInit();
    Dsp280039FlashInit();
    UpgradeUserInit();
    LedInit();
    FanInit();
    HeatingInit();
//    ParamSaveProcessInit();
}

/**
 * @brief 任务创建
 * 
 * 任务中不允许存在死循环，一定需要存在退出条件
 * 否则会触发看门狗复位
 */
void TaskCreate(void)
{
    Task_Start(&TaskSysMachine, NULL, 1, 0);        // 系统状态机，调度周期100us
    Task_Start(&TaskPlatform, NULL, 100, 0);
    Task_Start(&TaskFilterComm, NULL, 10000, 0);    // 滤波器参数更新任务，调度周期1s
    Task_Start(&TaskBatteryParallelProcess, NULL, 20, 0);
    Task_Start(&TaskFaultDetect, NULL, 10, 0);      // FaultDetect故障保护任务

    Task_Start(&TaskModbusSlave_1, NULL, 10, 0);    // Modbus通信任务，调度周期1ms
    Task_Start(&TaskModbusSlave_2, NULL, 10, 0);    // Modbus通信任务，调度周期1ms

    Task_Start(&TaskCheckUpgrade, NULL, 50, 0);     // 升级请求检测任务
	Task_Start(&TaskBatteryParallelComm, NULL, 30, 0);

    Task_Start(&TaskLed, NULL, 5000, 0);            // LED任务
    Task_Start(&TaskHeating, NULL, 20000, 0);       // 加热膜任务
    Task_Start(&TaskFan, NULL, 10000, 0);           // 风扇任务

//    Task_Start(&TaskPowerManager, NULL, 10000, 0);
//    Task_Start(&TaskLowPowerMode, NULL, 10, 0);// 低功耗任务
//    Task_Start(&TaskTestProcess, NULL, 1, 0);

	Task_Start(&TaskTemp, NULL, 10000, 0);
	Task_Start(&TaskOscParamProcess, NULL, 10000, 0);   // 软件示波器任务
}

/**
 * @brief 启动系统
 * 
 */
void StartSystem(void)
{
    Hw_EnableWatchDog();  // 启动看门狗
    Hw_FeedWatchDog();    // 执行一次喂狗，避免复位
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);

    // 这个目的是为了解决启动时出现的单个脉冲问题，后续通过函数封装一下
    EALLOW;
    EPwm1Regs.TZCLR.bit.OST = 1;
    EPwm2Regs.TZCLR.bit.OST = 1;
    EPwm3Regs.TZCLR.bit.OST = 1;
    EPwm4Regs.TZCLR.bit.OST = 1;
    EPwm5Regs.TZCLR.bit.OST = 1;
    EPwm6Regs.TZCLR.bit.OST = 1;
    EPwm8Regs.TZCLR.bit.OST = 1;
    EDIS;

    // 标记系统平台完成初始化，中断相关函数的逻辑可以执行
    SetPlatformInit(&ObjPlatformCtrl);
}

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


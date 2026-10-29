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
#include "AdcCalib.h"
#include "AdcSample.h"
#include "SoftOsc.h"
#include "Hw_SciDriver.h"
#include "ModbusSlave.h"
#include "DataExchange.h"
#include "EepromParam.h"
#include "Fm24c128d.h"
#include "Hw_I2cDriver.h"
#include "Hw_SpiDriver.h"
#include "ProtectTask.h"
#include "AtsCtl.h"
#include "RelayCtl.h"
#include "InvStateMachine.h"
#include "SystemStateMachine.h"
#include "PvStateMachine.h"
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
    /* 看门狗在 StartSystem() 末尾使能：必须在 100us 定时器与 20kHz 控制链
     * 都已运行之后，否则初始化期间的毫秒级阻塞会被 1.6384ms 上限的看门狗复位。 */



//    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
//    memcpy(&FlashAPIRunStart, &FlashAPILoadStart, (size_t)&FlashAPILoadSize);

    // 这里配置CPUTimer
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer1);
    Hw_CpuTimerInit((ST_SYS_TIMER *)&stSysCpuTimer2);

    // 初始化外设
    Hw_GpioInit(&stGpioCfg);    /* GPIO复用是外设引脚的前提 */
    Hw_XbarInit(&stXbarCfg);    /* GPIO -> INPUTx -> TRIPn：过流/短路信号的汇聚路径 */
    Hw_PwmInit(&stPwmCfg);      /* PWM提供ADC的SOC触发源 */
    Hw_PwmInitTripZone(&stPwmTzCfg);  /* 硬件过流/短路 -> DCAH/DCBH 逐波限流(CBC)，必须在 Xbar 之后 */
    Hw_InvInit(&stInvCfg);      /* 逆变驱动绑定 R/S/T 相与正负半周模块，必须在 Hw_PwmInit 之后 */
    Hw_AdcInit(&stAdcCfg);      /* ADC依赖PWM的SOC */
    Hw_ClaInit(&stClaCfg);      /* CLA由ADC INT触发，内部完成程序搬运 */
    Hw_EcapInit(&stEcapCfg);    /* eCAP1~6：电网过零与电网频率信号捕获 */
    Hw_SciInit(&stSciCfg);      /* SCI-A/C/D 串口，SCIC 与外部TVS通信 */
    Hw_CanInit(&stCanCfg);      /* CAN-A / CAN-B */

    Hw_I2cInit(&stI2cCfg);          /* 软件I2C（GPIO91/92）：FM24C128D 的物理层 */
    Fm24c128dInit(&stFm24c128dCfg); /* FM24C128D：写保护脚 GPIO99，上电做一次自检 */
    Hw_SpiInit(&stSpiCfg);          /* SPI-A（GPIO54~57）：与 ARM 芯片 STM32F103VET6 通信 */
    Hw_ExitInit(&stExitCfg);    /* 外部中断最后，依赖PIE已配置 */


    // 开启定时器
    // 这里开启定时器的目的是为了使得通信相关接口可以正常工作
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer1);   /* 100us 节拍，驱动 SYS_TICK */
    Hw_CpuTimerStart((ST_SYS_TIMER *)&stSysCpuTimer2);   /* 200MHz 自由运行计数器，供控制中断测余量 */

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
    AdcCalibInit();            /* ADC 标定：系数/零点回标称值，并启动上电零漂检测 */
    AdcSampleInit();           /* 采样两层封装：瞬时值与对外值清零 */
    AtsProtoInit(&stAtsProtoCfg);
    AtsCtlInit();              /* ATS 干接点：读 53 状态、52 归零 */
    RelayCtlInit();            /* 继电器统一控制层：全部断开 */
    InvSmInit();               /* 逆变状态机：封锁 + 继电器断开 */
    SysSmInit();               /* 系统总状态机：进入上电流程 */
    PvSmInit();                /* PV 状态机 + MPPT：四路 Boost 全部封波待命 */
    CtrlIsrInit();
    ModbusInit(&stMbCfg);          // 从机：SCIA 给 ARM、SCID 给上位机
    DataExchangeInit();            // 协议镜像清零
    EepromParamInit(&stEepromParamCfg);  /* 掉电保存：扫点表 -> 读芯片 -> 校验 -> 回填变量 */
    ProtectTaskInit();                    /* 硬件保护：清漏桶与故障锁存 */
    SoftOscInit();    /* 绑定软示波器默认的 12 路观测通道 */   /* 控制中断状态复位，并把 MUX 选通到 PV1 */
}

/**
 * @brief 任务创建
 *
 * 任务中不允许存在死循环，一定需要存在退出条件
 * 否则会触发看门狗复位
 */
void TaskCreate(void)
{
    /* 软件示波器：基准节拍 100us，4 个节拍 = 400us 发一帧，两组通道交替 */
    Task_Start(&SoftOscTask, NULL, 4U, 0U);

    /* 通信层：100us 扫描（补发送 FIFO + 判帧），200us 解析 */
    Task_Start(&Hw_SciScanAllTask, NULL, 1U, 1U);
    Task_Start(&ModbusTask, NULL, 2U, 2U);

    /* 协议搬运：1kHz，Rx 把 ARM 命令落到内部，Tx 把内部状态刷到发布镜像 */
    Task_Start(&DataExchangeTask, NULL, DATA_EXCHANGE_TASK_TICK, 4U);

    /* 掉电保存：调度节拍 100us，10 个节拍 = 1ms 检查一次合并窗口，
     * 真正写芯片的频率由 stEepromParamCfg 限速，不在这里控制 */
    Task_Start(&EepromParamTask, NULL, 10U, 9U);

    /* 硬件保护：调度节拍 100us，1 个节拍跑一次 = 10kHz，
     * 与 EP5K 的 HandleProtect 同频，漏桶门限（400 次 / 5ms 静默）直接沿用 */
    /* 保护判定已迁入 20kHz 控制中断（÷2 = 10kHz），见 CtrlIsr.c 的 CtrlSmSchedulerRun() */

    /* 对外采样量：调度节拍 100us，10 个节拍 = 1kHz 刷新 gAdcPub
     * （RMS 一个工频周期收口一次，直流量 1ms 块平均后一阶滞后） */
    Task_Start(&AdcSampleTask, NULL, 10U, 5U);
    Task_Start(&AtsProtoTask, NULL, 1000U, 6U);
    Task_Start(&AtsCtlTask,    NULL,   10U, 7U);    /* 1ms：ATS 状态去抖与动作超时 */
    /* 状态机四件套（Inv / Bus / Pv / Sys）已迁入 20kHz 控制中断内部分频调用，
     * 不再作为后台任务注册，见 CtrlIsr.c 的 CtrlSmSchedulerRun()。 */
}

/**
 * @brief 启动系统
 *
 * @note 这里只做三件事：
 *       1. 清掉上电可能残留的 Trip Zone 标志，否则第一拍中断进不来；
 *       2. 三相桥臂用硬件钳位（AQCSFRC 强制低），与占空比解耦；
 *       3. 放出 ePWM 时基：CTR 起跑 -> EPWM1 SOCA -> ADC -> ADCINT1 -> CLA Task1
 *          -> CLA1_1 中断 -> CtrlIsrRun()，发波授权保持 0。
 *
 *       要真正发波，走的是 CtrlIsr 内部的发波授权状态机（20kHz 裁定）：
 *         CtrlSetPwmEnable(1U);        发波请求
 *         gCtrlModIndex > 0            调制比请求值（CtrlSetModIndex() 设置）
 *       状态机额外要求"无故障 + 零漂标定完成"，并等 10ms 后按 40/s 的斜率
 *       把实际调制比爬到请求值；撤销请求时按 80/s 降回 0 再封波。
 */
void StartSystem(void)
{
    /* 1. 清 Trip Zone 残留标志，避免 OST 一直封着导致 SOC/中断不正常 */
    Hw_PwmClearAllTripZone();

    /* 2. 三相桥臂回到硬件钳位状态（这一步与上电初始化幂等） */
    Hw_InvStopAll();

    /* 3. 放出全部 ePWM 时基，开始产生 SOC 触发链 */
    Hw_EnableAllPwmCLKSync();

    /* 4. 上电保持发波请求关闭，等 InvSm 走到 CHK/SOFT 才由状态机申请发波 */
    CtrlSetPwmEnable(0U);

    /* 5. 使能硬件看门狗。
     *    F2837xS 看门狗由 10MHz 内部振荡器驱动，8 位计数配合最大 64 分频，
     *    超时上限只有 1.6384ms，无法直接做 200ms 级监视，因此分两层：
     *      硬件层：喂狗点放在 100us 定时中断里，条件是 20kHz 控制链仍在推进，
     *              控制链停摆即停止喂狗，由复位兜底；
     *      软件层：Task_HealthTick 以 200ms 窗口监视后台任务活性，
     *              超时锁存 Task stall 故障，由控制中断封锁发波，不做复位。
     *    使能点必须放在 100us 定时器与 20kHz 中断链都已运行之后。 */
    Hw_WatchDogInit(2U);
    Hw_EnableWatchDog();
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
        /* 喂狗已移到 100us 定时中断，条件与复位分工见 StartSystem() 第 5 步 */
        Task_Run();
    }
}

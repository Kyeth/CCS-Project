/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: main.c
Author: tzz
Version : 0.0.2
Date:2022年02月11日
Description: main 主程序文件
Function List: 各个时间片任务主函数,PWM中断入口函数
***********************************************************/
#include "F28x_Project.h"
#include "bsp.h"
#include "Sample.h"
#include "Protect.h"
#include "LLCStateMachine.h"
#include "PVStateMachine.h"
#include "CalculateAngle.h"
#include "InvStateMachine.h"
#include "OffGridInv2.h"
#include "OnGridInv2.h"
#include "ConfigSystemPara.h"
#include "BoardTest.h"
#include "User_can.h"
#include "User_Modbus.h"
#include "Update.h"
#include "ModbusTable.h"
#include "User_Eeprom.h"
#include "Sector.h"
#include "Energy_Manage.h"
#include "Parallel.h"
#include "MicroInvmode.h"
#include "BUSStateMachine.h"
#include <Function.h>

// 任务时间片
#define TASK_1MS_PERIOD     1000L
#define TASK_2MS_PERIOD     1200L
#define TASK_10MS_PERIOD    10000L
#define TASK_500MS_PERIOD   500000L

// 写标志位
#pragma DATA_SECTION(AppStartFlag, "flashflag_app_start")
const uint16_t AppStartFlag = 0x55AA;
#pragma DATA_SECTION(AppEndFlag, "flashflag_app_end")
const uint16_t AppEndFlag = 0x55AA;
#pragma DATA_SECTION(AppVerFlag, "flashflag_app_version")
const uint16_t AppVerFlag = 0x0514;

// 任务时间计数器
Uint32  Task1msCnt;
Uint32  Task2msCnt;
Uint32  Task10msCnt;
Uint32  Task500msCnt;

// 以下变量用于临时测试使用,正式版需删除
Uint32 RunPeriod = 0;
int16_t Temp = 0;
int16_t change = 0;
extern Uint16 EMinitEN;
Uint16 PWMONflag;
void DataExchange(void);

/*************************************************
Function: InitModularData
Description: 各个功能模块变量初始化
Input: None
Output: None
Frequency: Once
*************************************************/
void InitModularData(void)
{
    // 系统参数初始化要置于模块初始化之前,
    // 部分模块中参数根据系统参数进行初始化
    InitSystemControlData();
    // 采样模块初始化
    InitSampleData();
    // 保护模块变量初始化
    InitProtectData();
    // 功能模块变量初始化
    InitFuncData();

    InitModbusTables();
    // modbus 通信模块变量初始化
    InitUserModbusData();
    // 平衡电路 模块变量初始化
    InitBusBalanceData();
    // LLC 模块初始化
//    InitLLCAdjustData();
    // MPPT 模块变量初始化
    //InitMpptData();
    // PV 控制模块变量初始化
    //InitPVControlData();
    // PV 状态机模块初始化
    InitPVStateMachineData();
    // 锁相角度计算模块机变量初始化
    InitCalculateAngleData();
    // 逆变状态机模块变量初始化
    InitInvStateMachineData();
    // 离网控制变量初始化
    InitOffGridInvControlData();
    // 并网控制变量初始化
    InitOnGridInvControlData();
    // EEPROM初始化
    User_Eeprom_Init();
    //认证参数初始化
    InitEnergyManage();

    InitMicroInverterPara();

    InitParallel();
    // 不使用电池包时测试时使用
//    SystemError.ArmFault = 0x02;
//    SystemPara.ModbusWEnable = 0;
//    SystemPara.BMSDischgMaxCurrSet = 10.0f;
}
/*************************************************
Function: Task1ms
Description: 1ms任务函数,负载保护、升级请求响应
Input: None
Output: None
Frequency: 1ms
*************************************************/
//#pragma CODE_SECTION(Task1ms, ".TI.ramfunc");
void Task1ms(void)
{
    // 总功率过载保护
//    CheckInvTotalLoad();
    // 升级检测
    CheckUpdateFlag();
    // 硬件故障自动复位
    CheckAutoRstErrCode();

    // 检查电池限并网功率
    CheckBATLimitChargeState();

    // AD零飘结束后开始PV绝缘检测

    ISOCheck();
    //
    CalculatePVCurrLimite();    // 限流值计算
}
/*************************************************
Function: Task2ms
Description: 2ms任务函数,485通信
Input: None
Output: None
Frequency: 2ms
*************************************************/
void Task2ms(void)
{
    HandleModbus();
}
/*************************************************
Function: Task10ms
Description: 10ms任务函数,更新对外显示数据、电网电压异常监测、
电网频率异常监测、逆变电压异常监测、逆变电流异常监测
Input: None
Output: None
Frequency: 10ms
*************************************************/
void Task10ms(void)
{
    // 更新对外数据
    DataExchange();
    // 电网电压检测
    CheckGridVolt();
    // 电网频率检测
    CheckGridFreq();
    // 检测逆变电压
    CheckInvterVolt();
    // 逆变过载
    CheckInvLoad();
    // 继电器开路
    RlyOpenCheck();
    // 检查当前状态是否允许并网充电
    CheckGridChargeState();
    // 直流分量判断
    Check_AcDc_Vde0126();
    // 日规机器不需要并网认证
    if (SystemPara.CountryID == 1)
    {
        ESmode.ES_EN = 1;
        SystemAlarm.bit.ESOR = 0;
        ESmode.ES_DELAY_CNT = ESmode.ES_DELAY;
    }
    else if (SystemPara.CountryID == 0)
    {
        //认证电网支撑
        sEnergyManage();
    }

    // 微逆调频模式
    MicroInvterModeFreqChange();
    // 激活
    BatActiveTask();
}
/*************************************************
Function: Task500ms
Description: 500ms任务函数,风机异常监测、调试指示灯控制、485通信异常监测
Input: None
Output: None
Frequency: 500ms
*************************************************/
//#pragma CODE_SECTION(Task500ms, ".TI.ramfunc");
void Task500ms(void)
{
    static uint32_t task500Count = 0;
    static uint16_t task500Flag = 0;

    // 10s后使能风机
//    if (task500Flag != 1 && task500Count > 20)
//    {
//        task500Flag = 1;
//
//        StartFan();
//    }
//    else
//    {
//        if (task500Count++ > 30)
//        {
//            task500Count = 30;
//        }
//
//        if (SystemPara.FanCtrEn == 1)
//        {
//            ControlFanSpeed(SystemPara.FanSpeed);
//        }
//    }
//
//    // 10s 后开始检测风机状态
//    if (task500Count >= 30)
//    {
//        // 风机调速使能且风速不为0的状态下开始检测
//        if ((SystemPara.FanCtrEn == 1) && (SystemPara.FanSpeed > 0))
//        {
//            CheckFanState();
//        }
//        else
//        {
//            Protect.Count.FanErrCnt = 0;
//        }
//    }
    CheckFanState();

    TOGGLE_RUN_LED;

    //E2ROM 记录认证历史数据
    User_EMData_Check();
    // 通信状态监测
    CheckCommunicationFault();

//    if (ArmWriteData.Fault.bit.DefaultEn)
//    {
//        EMinitEN = 0;
//    }
}
/*************************************************
Function: main
Description: main 函数入口,初始化驱动模块、初始化模块变量、调用各个时间片任务
Input: None
Output: None
Frequency: always
*************************************************/
void main(void)
{
    // 外设初始化
    InitBSP();
    //
    initFlashSectors(0);

    // 初始化模块变量
    InitModularData();

    // 使能看门狗
    EnableDog();
    // 使能中断
    EALLOW;
    PieVectTable.CANA0_INT = &Para_CanIsr;    //paralell add
    IER |= M_INT9;
    PieCtrlRegs.PIEIER9.bit.INTx5 = 1;           //paralell add //9.5 CANA0

#if 0
    //改为adc中断，加快环路
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 1;  // End of SOC1 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;    // Enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Make sure INT1 flag is cleared
    PieVectTable.ADCA1_INT = &adc_isr;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;      //Adc中断
#endif
    EDIS;

    EINT;  // Enable Global interrupt INTM
    ERTM;  // Enable Global realtime interrupt DBGM

    Task1msCnt = 0;
    Task2msCnt = 0;
    Task10msCnt = 0;
    Task500msCnt = 0;
	BusStateData.StartState = Init_State;
/*----------------------调试数据-----------------------*/
    SystemPara.BMSDischgMaxPowerSet = 1.0f;//用于允许放电进入离网
//    FuncPara.RlyGridOver = 1;
//    FuncPara.RlyMidOver = 1;
/*----------------------后续删除-----------------------*/
    while (1)
    {
        ParallelTask();

        if (User_usTimer(&Task1msCnt, TASK_1MS_PERIOD))        // 1ms
        {
            Task1ms();
        }

        if (User_usTimer(&Task2msCnt, TASK_2MS_PERIOD))        // 2ms
        {
            Task2ms();
        }

        if (User_usTimer(&Task10msCnt, TASK_10MS_PERIOD))      // 10ms
        {
            Task10ms();
        }

        if (User_usTimer(&Task500msCnt, TASK_500MS_PERIOD))    // 500ms
        {
            Task500ms();
        }

        MainProtect();

        //User_Eeprom();
    }
}
/*************************************************
Function: ISREpwm1
Description: Epwm1 中断入口
Input: None
Output: None
Frequency: 10KHz
*************************************************/
#pragma CODE_SECTION(ISREpwm1, ".TI.ramfunc");
__interrupt void ISREpwm1(void)
{
    IER |= M_INT2;                        // 允许被epwm_tzisr打断
    IER |= M_INT9;                        // paralleladd 允许被canA打断
    IER |= M_INT3;                        // paralleladd 允许被canA打断
    PieCtrlRegs.PIEACK.all = 0xFFFF;      // Enable PIE interrupts
    asm(" NOP");                          // Wait one cycle

    EINT;
//    Cla1ForceTask1();
    EPwm1Regs.ETCLR.bit.INT = 1;
    // 测试代码运行时间
    CpuTimer2.InterruptCount = CpuTimer2.RegsAddr->TIM.all;
    // 1-1--------------------------------------------------------------
    // 采样处理-瞬时值计算
   // TEST_ON
    if(change==0)
    {   // 测试代码运行时间
//        if(PWMONflag==1)
//        {
//            ENABLE_INV1PWM;
//            ENABLE_INV2PWM;
//            ENABLE_INV3PWM;
//            ENABLE_INV4PWM;
//        }

        HandleSample(1);
        HandleInvterPhase();
        SoftPLL();
        ECO();
        RunInvStateMachine();
        UpdateAngle();
        CheckAntiIslandingProtect();
        RunBusState();
        CheckGridVoltLoss();
        OCPSoft();
        Para_GetPwmStep();
    }

    ++change;
    if(change==2)//16us
    {
        change=0;

//        if(PWMONflag==1)
//        {
//            if(EPwm1Regs.AQCSFRC.bit.CSFB == 0)   //
//            {
//                EPwm1Regs.AQCSFRC.bit.CSFB = 2;
////                EPwm1Regs.CMPA.bit.CMPA = 1;
//            }
//            if(EPwm1Regs.AQCSFRC.bit.CSFA == 0)  //上管OK
//            {
////                EPwm1Regs.AQCSFRC.bit.CSFA = 1;
//                EPwm1Regs.CMPA.bit.CMPA = (EPwm1Regs.TBPRD - 1);
//            }
//            if(EPwm2Regs.AQCSFRC.bit.CSFB == 0)   //
//            {
//                EPwm2Regs.AQCSFRC.bit.CSFB = 2;
////                EPwm2Regs.CMPA.bit.CMPA = 1;
//            }
//            if(EPwm2Regs.AQCSFRC.bit.CSFA == 0)  //上管OK
//            {
////                EPwm2Regs.AQCSFRC.bit.CSFA = 1;
//                EPwm2Regs.CMPA.bit.CMPA = (EPwm2Regs.TBPRD - 1);
//            }
//        }

        //  RunBusState();
        // PV 状态机
        RunPVStateMachine();

        HandleProtect();

        InvPwmReload();
        InvPwmReloadOnGrid();

        HandleSample(2);
        if(ArmWriteData.Model.bit.AntiFlowEn == 1&&SystemPara.ActivePowerSetRTemp>=0&&SystemPara.ActivePowerSetSTemp>=0)
        {
            AntiFlow();
        }
        else
        {
            SystemPara.ActivePowerSetR=SystemPara.ActivePowerSetRTemp;
            SystemPara.ActivePowerSetS=SystemPara.ActivePowerSetSTemp;
        }
//        ECO();
        //    BoardTest();    // 127~189
        User_Can();         // 87
        // CheckAntiIslandingProtect();

    }
      //  TEST_OFF

    static uint16_t fancnt = 0;

    if(++fancnt > 3)//4档
    {
        fancnt = 0;
    }

    if(fancnt < SystemPara.FanSpeed)  //4档
    {
        FAN2_ON
    }
    else
    {
        FAN2_OFF
    }

    SystemPara.SysTimerCount = CpuTimer2.RegsAddr->TIM.bit.MSW;     // 用于存EEPROM
    CpuTimer2.InterruptCount -= CpuTimer2.RegsAddr->TIM.all;
//          RunPeriod = CpuTimer2.InterruptCount;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

#pragma CODE_SECTION(ISREpwmcLA, ".TI.ramfunc");
__interrupt void ISREpwmcLA(void)
{
    InvPhase.HrThetaR += (InvPhase.DeltR * (0.0625));
    EPwm11Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
}

//interrupt void adc_isr(void)
//{
//    IER |= M_INT2;                        // 允许被epwm_tzisr打断
//    IER |= M_INT9;                        // paralleladd 允许被canA打断
//    IER |= M_INT3;                        // paralleladd 允许被canA打断
//    PieCtrlRegs.PIEACK.all = 0xFFFF;      // Enable PIE interrupts
//    asm(" NOP");                          // Wait one cycle
//
//    EINT;
//    // 测试代码运行时间
//    CpuTimer2.InterruptCount = CpuTimer2.RegsAddr->TIM.all;
//    // 1-1--------------------------------------------------------------
//    // 采样处理-瞬时值计算
//    TEST_ON
//    if(change==0)
//    {   // 测试代码运行时间
////        if(PWMONflag==1)
////        {
////            ENABLE_INV1PWM;
////            ENABLE_INV2PWM;
////            ENABLE_INV3PWM;
////            ENABLE_INV4PWM;
////        }
//
//        HandleSample(1);
//        HandleInvterPhase();
//        SoftPLL();
//        ECO();
//        RunInvStateMachine();
//        UpdateAngle();
//        CheckAntiIslandingProtect();
//        RunBusState();
//    }
//
//    ++change;
//    if(change==2)//16us
//    {
//        change=0;
//
//        if(PWMONflag==1)
//        {
////            if(EPwm1Regs.AQCSFRC.bit.CSFB == 0)   //
////            {
////                EPwm1Regs.AQCSFRC.bit.CSFB = 2;
//////                EPwm1Regs.CMPA.bit.CMPA = 1;
////            }
////            if(EPwm1Regs.AQCSFRC.bit.CSFA == 0)  //上管OK
////            {
//////                EPwm1Regs.AQCSFRC.bit.CSFA = 1;
////                EPwm1Regs.CMPA.bit.CMPA = (EPwm1Regs.TBPRD - 1);
////            }
////            if(EPwm2Regs.AQCSFRC.bit.CSFB == 0)   //
////            {
////                EPwm2Regs.AQCSFRC.bit.CSFB = 2;
//////                EPwm2Regs.CMPA.bit.CMPA = 1;
////            }
////            if(EPwm2Regs.AQCSFRC.bit.CSFA == 0)  //上管OK
////            {
//////                EPwm2Regs.AQCSFRC.bit.CSFA = 1;
////                EPwm2Regs.CMPA.bit.CMPA = (EPwm2Regs.TBPRD - 1);
////            }
//        }
//
//        HandleSample(2);
//        //  RunBusState();
//
//        CheckGridVoltLoss();
//
//        // PV 状态机
//        RunPVStateMachine();
//        HandleProtect();
//        OCPSoft();
////        ECO();
//        //    BoardTest();        // 127~189
//        User_Can();         // 87
//        // CheckAntiIslandingProtect();
//
//        Para_GetPwmStep();
//
//    }
//        TEST_OFF
//    SystemPara.SysTimerCount = CpuTimer2.RegsAddr->TIM.bit.MSW;     // 用于存EEPROM
//    CpuTimer2.InterruptCount -= CpuTimer2.RegsAddr->TIM.all;
//          RunPeriod = CpuTimer2.InterruptCount;
//
//    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
//    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
//}

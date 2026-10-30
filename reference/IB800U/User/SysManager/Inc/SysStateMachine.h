#ifndef __SYSSTATEMACHINE_H
#define __SYSSTATEMACHINE_H

#include "F28003x_Examples.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef union
{
    Uint16 u16Word;
    struct
    {
        Uint16 bit0     :1;
        Uint16 bit1     :1;
        Uint16 bit2     :1;
        Uint16 bit3     :1;
        Uint16 bit4     :1;
        Uint16 bit5     :1;
        Uint16 bit6     :1;
        Uint16 bit7     :1;
        Uint16 bit8     :1;
        Uint16 bit9     :1;
        Uint16 bit10    :1;
        Uint16 bit11    :1;
        Uint16 bit12    :1;
        Uint16 bit13    :1;
        Uint16 bit14    :1;
        Uint16 bit15    :1;
    }Bits;
}U_SYS_WORD;


typedef enum
{
    SYS_STATE_IDLE,
    SYS_STATE_CHECK,
    SYS_STATE_SOFT_START,
    SYS_STATE_WAIT_TO_RUN,
    SYS_STATE_NORMAL_OPERATION,
    SYS_STATE_FAULT_SHUTDOWN,
    SYS_STATE_POWER_RESTRICTION,
}E_SYS_STATE;

typedef enum
{
    LLC_IDLE_STATE,             // 空闲态
    LLC_SOFT_STATE,             // 软启态
    LLC_SOFT_WAIT_STATE,        // 等待软启结束态
    LLC_CHARGE_STATE,           // 充电态
    LLC_DISCHARGE_STATE,        // 放电态
    LLC_STANDBY_STATE,          // 待机态
    LLC_AGING_STATE,            // 老化态
}LLC_STATE;

typedef enum
{
    IB800JP_STATE,              // 日本
    IB800_STATE,                // 欧洲
}PROJECT_STATE;

typedef enum
{
    DC_Init,                    // 初始态
    DC_StandBy,                 // 待机态
    DC_Soft,                    // 缓启态
    DC_Work,                    // 运行态
    DC_Error,                   // 故障态
}DC_BMS_STATE;

typedef enum
{
    OPERATE_MODE_CHARGE,
    OPERATE_MODE_DISCHARGE,
    OPERATE_MODE_STANDBY,
}E_OPERATE_MODE;

typedef enum
{
    POWER_MODE_SOC,
    POWER_MODE_CURRENT,
    POWER_MODE_STANDBY
}E_POWER_MODE;

typedef struct
{
    E_SYS_STATE SysState;           // 枚举类型：E_SYS_STATE
    LLC_STATE LLCState;             // 枚举类型：LLC_STATE
    DC_BMS_STATE DCState;           // 枚举类型：DC_BMS_STATE
    PROJECT_STATE ProjectState;     // 枚举类型：PROJECT_STATE
    Uint16 OperateMode      :2;     // 枚举类型：E_OPERATE_MODE
    Uint16 PowerMode        :2;     // 枚举类型：E_POWER_MODE
    Uint16 IsMaster         :1;     // 1:表示主机；  0:表示从机
    Uint16 LLCMode;                 // 0:切换状态；  1:充电状态；  2:放电状态；

    Uint16 Open;                    // 开关机
    Uint16 ClosedLoopMode;          // 0:开环运行模式 1：闭环运行模式
    Uint16 u16TestMode;             // 0:正常模式；  1:测试模式；
    Uint16 u16TestSoftStartMode;    // 软启模式设定 0:充电软启    1:放电软启CHARGE_SWITCH
    Uint16 u16TestChargeCfgSwitch;  // 0:充电调频闭环模式     1:充电调宽闭环模式
    Uint16 u16TestChargeZoom;       // 用于设定充电切换条件（充电模式调试阶段用）

    Uint16 BurstFlag;               // 1:触发长时打嗝；0:初始状态
    Uint16 BurstCnt;                // 触发计数
    Uint16 ParamFlag;               // 用于标记CLA计算参数完成
    Uint16 u16IsLoad;               // 带载标记
    Uint16 u16FanFlag;              // 风扇自检标志


    U_SYS_WORD unSysCtrlEnable;     // 系统控制使能位
    U_SYS_WORD unAgingSigns;        // 老化标志：0xAA-进入老化模式 0x55-进入正常模式 0xA0-进入单板测试模式
    Uint16 unStateSign;             // 运行模式设置：0-静置 1-CCCV-恒流恒压充 2-CCD-恒流放
    Uint16 u16MaintainVolt;         // 电池电压设置 0.01V
    Uint16 u16MaintainCurrent;      // 维护电流：根据运行模式区分充放电 0.1A
    Uint16 u16RunSet;               // 运行命令
    Uint16 u16ClearFaultSet;        // 清除故障命令
    Uint16 u16BatteryStatus;        // 电池状态:0-空闲 1-充电 2-放电
    Uint16 u16TotalBatteryVoltage;  // 电池总电压(0.1V)
    Uint16 u16TotalBatteryCurrent;  // 电池总电流(0.1A)
    Uint16 u16DischargeCurrentLimit;// 放电过温限流值(0.1A)
    Uint16 u16ChargeCurrentLimit;   // 充电过温限流值(0.1A)
    Uint16 u16HeatingFilmEnable;    // 加热膜使能
    Uint16 u16ATEEnableFlag;        // ATE使能标志
    Uint16 u16AgingSignsFlag;       // 老化使能标志
    Uint16 u16HeatingFilmEnableFlag;    // 加热膜使能标志

}ST_SYS_CTRL;


static inline void SetSysMachineState(ST_SYS_CTRL *pObjSystemCtrl, E_SYS_STATE eSysState)
{
    pObjSystemCtrl->SysState = eSysState;
}

static inline E_SYS_STATE GetSysMachineState(ST_SYS_CTRL *pObjSystemCtrl)
{
    return pObjSystemCtrl->SysState;
}

static inline LLC_STATE GetLLCState(ST_SYS_CTRL *pObjSystemCtrl)
{
    return pObjSystemCtrl->LLCState;
}

static inline E_OPERATE_MODE GetSysOperateMode(ST_SYS_CTRL *pObjSystemCtrl)
{
    return (E_OPERATE_MODE)pObjSystemCtrl->OperateMode;
}

static inline E_POWER_MODE GetSysPowerMode(ST_SYS_CTRL *pObjSystemCtrl)
{
    return (E_POWER_MODE)pObjSystemCtrl->PowerMode;
}

static inline Uint16 IsSysMaster(ST_SYS_CTRL *pObjSystemCtrl)
{
    return pObjSystemCtrl->IsMaster;
}


extern void TaskSysMachine(void);

extern void TaskTestProcess(void);

extern ST_SYS_CTRL ObjSystemCtrl;

#ifdef __cplusplus
    }
#endif

#endif

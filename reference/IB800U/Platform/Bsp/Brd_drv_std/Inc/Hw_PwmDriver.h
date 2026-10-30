/**
 * @file Hw_PwmDriver.h
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-26
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW_PWMDRIVER_H
#define __HW_PWMDRIVER_H


#include "F28003x_device.h"
#include "epwm.h"

#ifdef __cplusplus
    extern "C" {
#endif



#define PWM_MAX_PRIOD   1500
#define PWM_MIN_PRIOD   200

#define PWM_TB_SYNC_IN       0x0
#define PWM_TB_CTR_ZERO      0x1
#define PWM_TB_CTR_CMPB      0x2
#define PWM_TB_SYNC_DISABLE  0x3


// 同步输出源选择
#define EPWM_SYNC_OUT_FROM_SOFTWARE         EPWM_SYNCOUTEN_SWEN
#define EPWM_SYNC_OUT_FROM_CNTR_ZERO        EPWM_SYNCOUTEN_ZEROEN
#define EPWM_SYNC_OUT_FROM_CNTR_COMPARE_B   EPWM_SYNCOUTEN_CMPBEN
#define EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C   EPWM_SYNCOUTEN_CMPCEN
#define EPWM_SYNC_OUT_FROM_CNTR_COMPARE_D   EPWM_SYNCOUTEN_CMPDEN
#define EPWM_SYNC_OUT_FROM_DCA_EVT1_SYNC    EPWM_SYNCOUTEN_DCAEVT1EN
#define EPWM_SYNC_OUT_FROM_DCB_EVT1_SYNC    EPWM_SYNCOUTEN_DCBEVT1EN
#define EPWM_SYNC_OUT_FROM_ALL              EPWM_SYNC_OUT_SOURCE_M


// 同步输入源选择
#define EPWM_SYNC_IN_FROM_DISABLE           0x0
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1     0x1
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM2     0x2
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM3     0x3
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM4     0x4
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM5     0x5
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM6     0x6
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7     0x7
#define EPWM_SYNC_IN_FROM_SYNCOUT_EPWM8     0x8
#define EPWM_SYNC_IN_FROM_SYNCOUT_ECAP1     0x11
#define EPWM_SYNC_IN_FROM_SYNCOUT_ECAP2     0x12
#define EPWM_SYNC_IN_FROM_SYNCOUT_ECAP3     0x13
#define EPWM_SYNC_IN_FROM_INPUTXBAR_OUT5    0x18
#define EPWM_SYNC_IN_FROM_INPUTXBAR_OUT6    0x19
#define EPWM_SYNC_IN_FROM_FSIRX_TRIG1       0x1F


// PWM_Ctrl接口的命令字
enum
{
    CFG_PHS_IS_ENABLE,
    CFG_SYNC_OUT,
    CFG_SYNC_IN,
    CFG_DEALTIME,
    CFG_SOFT_MODE,
    CFG_NORMAL_MODE,
};

// 配置PWM主从模式
enum
{
    PWM_MASTER = 0x0,       // 主机模式：同步输出有效，同步输入无效
    PWM_SLAVE  = 0x1,       // 从机模式：同步输出无效，同步输入有效,同时使能相位同步
    PWM_BRIDGE = 0x2,       // 桥接模式：即能被主机同步，也能同步从机，即同步输出和同步输入均有效，同时使能相位同步
};




typedef enum ePwmId
{
    PWM_ID0 = 0x00,
    PWM_ID1 = 0x01,
    PWM_ID2,
    PWM_ID3,
    PWM_ID4,
    PWM_ID5,
    PWM_ID6,
    PWM_ID7,
    PWM_ID8,
    PWM_ID9,
    PWM_ID10,
    PWM_ID11,
    PWM_ID_MAX,
    PWM_ID_NULL,
}E_PWMID;

typedef struct
{
    Uint16 u16PwmId;
    Uint32 u32Period;
    Uint16 bGload               :8;     // 0:禁用全局加载 1:启用全局加载
    Uint16 LinkPwm              :8;     // 全局加载的LINK的PWM
    Uint32 bPhase               :30;    // 相位
    Uint16 bSyncMode            :2;     // 0:Master; 1:Slave; 2:桥接
                                        // 作为主机时，禁用相位同步，作为从机时，使能相位同步，在下一个TBCLK载入相位值
    Uint16 bSyncCntLoadDir      :1;     // 当产生同步事件时，CNT计数方向 0:向下计数  1:向上计数
    Uint16 bDeadTime            :13;    // 死区时间
    Uint16 bPwmxATzLevel        :2;     // 保护电平
    Uint16 bPwmxBTzLevel        :2;     // 保护电平
    Uint16 bCtrMode             :2;     // 计数器模式
    Uint16 bSyncInSel           :5;     // 同步输入源选择  0:表示不使能
    Uint16 bSyncOutSel          :5;     // 同步输出源选择  0:表示不使能
    Uint16 bRsv                 :2;     // 保留
}ST_PWM_CFG_ITEM;

typedef struct
{
    ST_PWM_CFG_ITEM *pstPwmCfgItem;
    Uint16 u16ItemNum;
}ST_PWM_CFG;




/**
 * @brief 用于初始化单个ePWM
 * 
 * @param pstPwmCfgItem :入参是一个ST_PWM_CFG_ITEM结构体对象指针
 */
extern void Hw_SinglePwmInit(ST_PWM_CFG_ITEM *pstPwmCfgItem);



/**
 * @brief 用于初始化一组ePWM，通常用于参数初始化阶段
 * 
 * @param pstPwmCfg :入参是一组ST_PWM_CFG_ITEM结构体对象的集合,即ST_PWM_CFG
 */
extern void Hw_PwmInit(ST_PWM_CFG *pstPwmCfg);



/**
 * @brief 使能所有PWM的时钟同步
 * 
 */
extern void Hw_EnableAllPwmCLKSync(void);



/**
 * @brief 禁止所有PWM的时钟同步
 * 
 */
extern void Hw_DisableAllPwmCLKSync(void);


#if 0


/**
 * @brief 修改一组ePWM的CMP值
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32ValueArry  :存放对应PwmId数组索引的CMP值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16    -1:参数非法
 *                  -2:ID非法
 *                  0: 设置成功
 */
extern int16 Hw_PwmSetCompareGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize);



/**
 * @brief 设置单个ePwm的比较值
 * 
 * @param u16PwmId ePwm的ID号
 * @param u32Value 比较值
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
extern int16 Hw_PwmSetCompare(Uint16 u16PwmId, Uint32 u32Value);



/**
 * @brief 修改指定PWM的周期值和占空比
 *          周期值取值范围为 PWM_MIN_PRIOD ~ PWM_MAX_PRIOD，
 *          如果超过范围则取边界值
 * @param u16PwmId :ePwm的ID号
 * @param u32Value :周期值
 * @param fDuty    :占空比，单位为百分数
 * @return int16        -1:参数非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmSetPeriod(Uint16 u16PwmId, Uint32 u32Value, float fDuty);



/**
 * @brief 修改一组Pwm的周期值，同时保持原有的占空比不变
 * 
 * @param u16PwmIdArry :存放PwmId的数组
 * @param u32ValueArry :存放对应PwmId数组索引的周期值
 * @param u16ArrySize :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmSetPeriodGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize, float fDuty);



/**
 * @brief 修改指定PWM偏移的相位
 * 
 * @param u16PwmId :ePwm的ID号
 * @param u32Value :相位值
 * @return int16        -1:参数非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmSetPhase(Uint16 u16PwmId, Uint32 u32Value);



/**
 * @brief 修改一组PWM偏移的相位
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32ValueArry  :存放对应PwmId数组索引的相位值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmSetPhaseGroup(Uint16 *u16PwmIdArry, Uint32 *u32ValueArry, Uint16 u16ArrySize);



/**
 * @brief 修改一组PWM的周期值、CMPAB的值
 *
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u32PeriodArry  :存放对应PwmId数组索引的周期值
 * @param u32CmpAArry    :存放对应PwmId数组索引的CMPA值
 * @param u32CmpBArry    :存放对应PwmId数组索引的CMPB值
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmSetPeriodAndCMPAB(Uint16 *u16PwmIdArry, Uint32 *u32PeriodArry, Uint32 *u32CmpAArry, Uint32 *u32CmpBArry, Uint16 u16ArrySize);



/**
 * @brief 启动单个PWM的波形输出
 * 
 * @param u16PwmId ePwm的ID号
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
extern int16 Hw_PwmStart(Uint16 u16PwmId);



/**
 * @brief 停止单个PWM的波形输出
 * 
 * @param u16PwmId ePwm的ID号
 * @return int16 -1:参数非法
 *              0: 设置成功
 */
extern int16 Hw_PwmStop(Uint16 u16PwmId);



/**
 * @brief 启动一组PWM输出，通过清除TZ模块故障来输出PWM波形
 * 
 * @param u16PwmIdArry :存放PwmId的数组
 * @param u16ArrySize :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmStartGroup(Uint16 *u16PwmIdArry, Uint16 u16ArrySize);



/**
 * @brief 停止一组PWM输出，通过触发TZ模块故障来停止PWM波形输出，同时修改CMP值为周期值
 * 
 * @param u16PwmIdArry  :存放PwmId的数组
 * @param u16ArrySize   :PwmId数组大小
 * @return int16        -1:参数非法
 *                      -2:ID非法
 *                      0: 设置成功
 */
extern int16 Hw_PwmStopGroup(Uint16 *u16PwmIdArry, Uint16 u16ArrySize);

#endif

/**
 * @brief Pwm外设的控制接口
 *
 * @param u16PwmId :Pwm的ID号
 * @param u16Cmd  :PWM命令枚举
 *                  CFG_SYNC_OUT    可用参数
 *                                  EPWM_SYNC_OUT_FROM_SOFTWARE
 *                                  EPWM_SYNC_OUT_FROM_CNTR_ZERO
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_B
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_C
 *                                  EPWM_SYNC_OUT_FROM_CNTR_COMPARE_D
 *                                  EPWM_SYNC_OUT_FROM_DCA_EVT1_SYNC
 *                                  EPWM_SYNC_OUT_FROM_DCB_EVT1_SYNC
 *                                  EPWM_SYNC_OUT_FROM_ALL
 *
 *                  CFG_SYNC_IN     可用参数
 *                                  EPWM_SYNC_IN_FROM_DISABLE
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM2
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM3
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM4
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM5
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM6
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_EPWM8
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP1
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP2
 *                                  EPWM_SYNC_IN_FROM_SYNCOUT_ECAP3
 *                                  EPWM_SYNC_IN_FROM_INPUTXBAR_OUT5
 *                                  EPWM_SYNC_IN_FROM_INPUTXBAR_OUT6
 *                                  EPWM_SYNC_IN_FROM_FSIRX_TRIG1
 *
 *                  CFG_PHS_IS_ENABLE   可用参数
 *                                      1：使能
 *                                      0：禁用
 *
 *                  CFG_DEALTIME        可用参数
 *                                      0 ~ 2^13-1
 *                                      (需要用户自行关闭PWM，避免在修改时刻前后上下沿死区不一致)
 *
 *                  CFG_SOFT_MODE   软启动发波模式：A和B通道独立，关闭死区
 *
 *                  CFG_NORMAL_MODE 正常发波模式：A和B通道互补，带死区
 * @param vParam :参数
 * @return int
 */
extern int Hw_PwmCtrl(Uint16 u16PwmId, Uint16 u16Cmd, void *vParam);



static inline void Hw_SetPWM1_CMPA(Uint16 u16CmpValue)
{
    EPwm1Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM2_CMPA(Uint16 u16CmpValue)
{
    EPwm2Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM3_CMPA(Uint16 u16CmpValue)
{
    EPwm3Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM4_CMPA(Uint16 u16CmpValue)
{
    EPwm4Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM5_CMPA(Uint16 u16CmpValue)
{
    EPwm5Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM6_CMPA(Uint16 u16CmpValue)
{
    EPwm6Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM7_CMPA(Uint16 u16CmpValue)
{
    EPwm7Regs.CMPA.bit.CMPA = u16CmpValue;
}

static inline void Hw_SetPWM8_CMPA(Uint16 u16CmpValue)
{
    EPwm8Regs.CMPA.bit.CMPA = u16CmpValue;
}


static inline void Hw_SetPWM1_CMPB(Uint16 u16CmpValue)
{
    EPwm1Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM2_CMPB(Uint16 u16CmpValue)
{
    EPwm2Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM3_CMPB(Uint16 u16CmpValue)
{
    EPwm3Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM4_CMPB(Uint16 u16CmpValue)
{
    EPwm4Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM5_CMPB(Uint16 u16CmpValue)
{
    EPwm5Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM6_CMPB(Uint16 u16CmpValue)
{
    EPwm6Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM7_CMPB(Uint16 u16CmpValue)
{
    EPwm7Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM8_CMPB(Uint16 u16CmpValue)
{
    EPwm8Regs.CMPB.bit.CMPB = u16CmpValue;
}

static inline void Hw_SetPWM1_CMPC(Uint16 u16CmpValue)
{
    EPwm1Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM2_CMPC(Uint16 u16CmpValue)
{
    EPwm2Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM3_CMPC(Uint16 u16CmpValue)
{
    EPwm3Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM4_CMPC(Uint16 u16CmpValue)
{
    EPwm4Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM5_CMPC(Uint16 u16CmpValue)
{
    EPwm5Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM6_CMPC(Uint16 u16CmpValue)
{
    EPwm6Regs.CMPC = u16CmpValue;
}

static inline void Hw_SetPWM1_CMPD(Uint16 u16CmpValue)
{
    EPwm1Regs.CMPD = u16CmpValue;
}


static inline void Hw_SetPWM1_PHS(Uint16 u16PhsValue)
{
    EPwm1Regs.TBPHS.bit.TBPHS = u16PhsValue;
}

static inline void Hw_SetPWM2_PHS(Uint16 u16PhsValue)
{
    EPwm2Regs.TBPHS.bit.TBPHS = u16PhsValue;
}

static inline void Hw_SetPWM3_PHS(Uint16 u16PhsValue)
{
    EPwm3Regs.TBPHS.bit.TBPHS = u16PhsValue;
}

static inline void Hw_SetPWM4_PHS(Uint16 u16PhsValue)
{
    EPwm4Regs.TBPHS.bit.TBPHS = u16PhsValue;
}

static inline void Hw_SetPWM5_PHS(Uint16 u16PhsValue)
{
    EPwm5Regs.TBPHS.bit.TBPHS = u16PhsValue;
}

static inline void Hw_SetPWM6_PHS(Uint16 u16PhsValue)
{
    EPwm6Regs.TBPHS.bit.TBPHS = u16PhsValue;
}


static inline void Hw_SetPWM1_PRD(Uint16 u16PrdValue)
{
    EPwm1Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM2_PRD(Uint16 u16PrdValue)
{
    EPwm2Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM3_PRD(Uint16 u16PrdValue)
{
    EPwm3Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM4_PRD(Uint16 u16PrdValue)
{
    EPwm4Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM5_PRD(Uint16 u16PrdValue)
{
    EPwm5Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM6_PRD(Uint16 u16PrdValue)
{
    EPwm6Regs.TBPRD = u16PrdValue;
}

static inline void Hw_SetPWM7_PRD(Uint16 u16PrdValue)
{
    EPwm7Regs.TBPRD = u16PrdValue;
}


static inline void Hw_SetPWM8_PRD(Uint16 u16PrdValue)
{
    EPwm8Regs.TBPRD = u16PrdValue;
}

#if 1
static inline void Hw_StartPWM1(void)
{
    EPwm1Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM2(void)
{
    EPwm2Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM3(void)
{
    EPwm3Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM4(void)
{
    EPwm4Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM5(void)
{
    EPwm5Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM6(void)
{
    EPwm6Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM7(void)
{
    EPwm7Regs.AQCSFRC.all = 0;
}

static inline void Hw_StartPWM8(void)
{
    EPwm8Regs.AQCSFRC.all = 0;
}
#endif

static inline void Hw_StartAllPWM(void)
{
    EPwm1Regs.AQCSFRC.all = 0;
    EPwm2Regs.AQCSFRC.all = 0;
//    EPwm3Regs.AQCSFRC.all = 0;
    EPwm4Regs.AQCSFRC.all = 0;
    EPwm5Regs.AQCSFRC.all = 0;
//    EPwm6Regs.AQCSFRC.all = 0;

}


#if 1
static inline void Hw_StopPWM1(void)
{
    EPwm1Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM2(void)
{
    EPwm2Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM3(void)
{
    EPwm3Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM4(void)
{
    EPwm4Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM5(void)
{
    EPwm5Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM6(void)
{
    EPwm6Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM7(void)
{
    EPwm7Regs.AQCSFRC.all = 9;
}

static inline void Hw_StopPWM8(void)
{
    EPwm8Regs.AQCSFRC.all = 9;
}

#endif


static inline void Hw_StopAllPWM(void)
{
    // B通道置高、A通道置低
    EPwm1Regs.AQCSFRC.all = 9;
    EPwm2Regs.AQCSFRC.all = 9;
//    EPwm3Regs.AQCSFRC.all = 9;
    EPwm4Regs.AQCSFRC.all = 9;
    EPwm5Regs.AQCSFRC.all = 9;
//    EPwm6Regs.AQCSFRC.all = 9;
}


static inline void Hw_SetPWM1Mode_Normal(void)
{
    EPwm1Regs.DBRED.all = 42;
    EPwm1Regs.DBFED.all = 42;
}

static inline void Hw_SetPWM2Mode_Normal(void)
{
    EPwm2Regs.DBRED.all = 42;
    EPwm2Regs.DBFED.all = 42;
}

static inline void Hw_SetPWM3Mode_Normal(void)
{
    EPwm3Regs.DBRED.all = 42;
    EPwm3Regs.DBFED.all = 42;
}


static inline void Hw_SetPWM4Mode_Normal(void)
{
    EPwm4Regs.DBRED.all = 42;
    EPwm4Regs.DBFED.all = 42;

}

static inline void Hw_SetPWM5Mode_Normal(void)
{
    EPwm5Regs.DBRED.all = 42;
    EPwm5Regs.DBFED.all = 42;
}

static inline void Hw_SetPWM6Mode_Normal(void)
{
    EPwm6Regs.DBRED.all = 42;
    EPwm6Regs.DBFED.all = 42;
}

static inline void Hw_SetPWM1Mode_Soft(void)
{
    EPwm1Regs.DBRED.all = 575;
    EPwm1Regs.DBFED.all = 575;
}

static inline void Hw_SetPWM2Mode_Soft(void)
{
    EPwm2Regs.DBRED.all = 575;
    EPwm2Regs.DBFED.all = 575;

}

static inline void Hw_SetPWM3Mode_Soft(void)
{
    EPwm3Regs.DBRED.all = 575;
    EPwm3Regs.DBFED.all = 575;

}


static inline void Hw_SetPWM4Mode_Soft(void)
{
    EPwm4Regs.DBRED.all = 575;
    EPwm4Regs.DBFED.all = 575;

}

static inline void Hw_SetPWM5Mode_Soft(void)
{
    EPwm5Regs.DBRED.all = 575;
    EPwm5Regs.DBFED.all = 575;

}

static inline void Hw_SetPWM6Mode_Soft(void)
{
    EPwm6Regs.DBRED.all = 575;
    EPwm6Regs.DBFED.all = 575;
}



static inline void Hw_SetPWM1_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm1Regs.DBRED.all = u16Red;
    EPwm1Regs.DBFED.all = u16Fed;
}


static inline void Hw_SetPWM2_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm2Regs.DBRED.all = u16Red;
    EPwm2Regs.DBFED.all = u16Fed;
}

static inline void Hw_SetPWM3_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm3Regs.DBRED.all = u16Red;
    EPwm3Regs.DBFED.all = u16Fed;
}

static inline void Hw_SetPWM4_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm4Regs.DBRED.all = u16Red;
    EPwm4Regs.DBFED.all = u16Fed;
}

static inline void Hw_SetPWM5_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm5Regs.DBRED.all = u16Red;
    EPwm5Regs.DBFED.all = u16Fed;
}

static inline void Hw_SetPWM6_DealTime(Uint16 u16Red, Uint16 u16Fed)
{
    EPwm6Regs.DBRED.all = u16Red;
    EPwm6Regs.DBFED.all = u16Fed;
}

static inline void Hw_TrigGlobalLoad(void)
{
    EPwm4Regs.GLDCTL2.bit.OSHTLD = 1;
}


#ifdef __cplusplus
    }
#endif

#endif

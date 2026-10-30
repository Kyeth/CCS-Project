#ifndef APP_INC_FUNCTION_H_
#define APP_INC_FUNCTION_H_

#include "stdint.h"

struct FUNC_PARA //暂时放在InitSystemControlData中初始化
{
    //测试标志位
    float TestFlag1;
    float TestFlag2;
    float TestFlag3;

    //ISO检测变量
    uint16_t ISOFinishFlag;
    uint16_t ISOCount;
    int16_t  ISOVoltTemp;
    uint16_t ISOJumpDelay;
    uint32_t ISOVoltSum1;
    uint32_t ISOVoltSum2;
    float    ISOVoltTemp1;
    float    ISOVoltTemp2;
    float    ISOBusVolt;
    float    ISOVoltFinal1;
    float    ISOVoltFinal2;
    float    ISORx;
    float    ISORy;
    float    ISOMin;
    //并网目标功率调整
    float    L1PAimOld;
    float    L1PAimUse;
    float    L2PAimOld;
    float    L2PAimUse;
    uint16_t L1AdjustCnt;
    uint16_t L2AdjustCnt;
    //斩波信号处理
    uint16_t OCL1;
    uint16_t OCL1Count;
    uint16_t OCL1RecCount;
    uint16_t OCL2;
    uint16_t OCL2Count;
    uint16_t OCL2RecCount;
    uint16_t ProtectCount;
    uint16_t RecoverCount;
    uint16_t OCL3;
    uint16_t OCL3Count;
    uint16_t OCL3RecCount;
    uint16_t HalfWaveJudge;//判断半波负载
    float  INVISUM1;
    float  INVIAVG1;
    float  INVISUM2;
    float  INVIAVG2;

    float  INVVSUM1;
    float  INVVAVG1;
    float  INVVSUM2;
    float  INVVAVG2;
    //离网开环补偿
    float   InvOffset;
    float   InvLimit;
    //并网切离网时机
        //辅助电网掉电检测（空载）
    float   Grid1VOld;
    float   Grid2VOld;
    float   DletaV1;
    float   DletaV2;
    uint16_t BeOnToOff;
    uint16_t ForceToOff;
    //电池包激活逻辑
    uint16_t BatPowering;
    uint16_t BatPowerCnt;
    uint32_t BatPowerDly;
    uint16_t BatPowerTry;
    uint16_t BatState;
    uint16_t BatPowered;
    //配合保护
    uint16_t OL1ErrorTime;
    uint16_t OL2ErrorTime;
    uint16_t OL1RecoverTime;//故障恢复次数
    uint16_t OL2RecoverTime;
    //PV高压关驱动回滞
    uint16_t PVHVFlag;
    //继电器检测变量
    uint16_t RlyCheckSt0;
    uint16_t RlyCheckSt1;
    uint16_t RlyCheckSt2;
    uint16_t RlyCheckSt3;
    uint16_t RlyDelay;
    uint16_t RlyError;
    uint16_t RlyGridOver;
    uint16_t RlyMidOver;
    uint16_t RlyInvOver;
    uint16_t RlyBpOver;
    uint16_t PreChgOver;
};


extern struct FUNC_PARA FuncPara;
void InitFuncData(void);
void Scope(void);
uint16_t ArmReadDsp(uint16_t addr);
void InitAddrMap(void);
void ISOCheck(void);
void GridRlyCheck(void);
void InvRlyCheck(void);
void MidRlyCheck(void);
void BpRlyCheck(void);

void RlyOpenCheck(void);
void SwitchToSelfCheckState(void);
void InvOcpHandle(void);
void PVHVCloseDrive(void);
void ADCoeffModify(void);
void BatActiveTask(void);
void CorrectActivePowerAim(float L1Target, float L2Target);
void SelfOnToOffHandle(void);
void ForceOnToOffHandle(void);
void OffGridSoftStart(void);


#endif /* APP_INC_FUNCTION_H_ */

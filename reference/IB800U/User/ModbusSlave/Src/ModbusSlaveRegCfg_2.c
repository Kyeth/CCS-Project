#include "ModbusSlaveRegCfg_2.h"
#include "Hw_AdcDriver.h"
#include "PlatformManager.h"
#include "ModbusSlaveComm.h"
#include "LLCControl.h"
#include "SysStateMachine.h"
#include "Hw_SysCpuTimer.h"
#include "FaultProtect.h"
#include "VersionUser.h"
#include "ParallelProcess.h"
#include "Heating.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDS_ITEM_OF(arr)            (sizeof(arr) / sizeof(arr[0]))

Uint16 g_u16TestArry[86] = {1,2,3,4,5,6,7,8,9,10,11,12,13};

/*******************************************************************************************************************************************************************************************************************************************************************************************************************
   (寄存器地址保持增序!!!)                                                                                信号全局动态配置表
********************************************************************************************************************************************************************************************************************************************************************************************************************
  寄存器地址         读写类型             数据类型         保存类型            值指针         值下限           值上限
********************************************************************************************************************************************************************************************************************************************************************************************************************/
const ST_REG_SLAVE_ITEM g_stRegSlaveItem_2[] =
{
     {0,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&LLCSystemCtrl.u16ChargeCurrentAllowMax),        0,    65535},
     {1,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&LLCSystemCtrl.u16DischargeCurrentAllowMax),        0,    65535},
     {2,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&LLCSystemCtrl.u16ChargeCoefficient),        0,    1000},
     {3,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&LLCSystemCtrl.u16DischargeCoefficient),        0,    1000},
     {4,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.unSysCtrlEnable),        0,    65535},
     {5,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16LocalAddr),        0,    65535},
     {6,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16Soc),        0,    65535},
     {7,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16SetPackMaxNum),        0,    65535},
     {8,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stHeatingHandle.u16HeatingLevel),        0,    65535},
     {9,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjPlatformCtrl.u16WorkFrequency),        0,    65535},
     {10,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[10]),        0,    65535},
     {11,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[11]),        0,    65535},
     {12,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[12]),        0,    65535},
     {13,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.stSerialNumBer.u16SN1),        0,    65535},
     {14,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.stSerialNumBer.u16SN2),        0,    65535},
     {15,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.stSerialNumBer.u16SN3),        0,    65535},
     {16,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.stSerialNumBer.u16SN4),        0,    65535},
     {17,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.unAgingSigns),        0,    65535},
     {18,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.unStateSign),        0,    65535},
     {19,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16MaintainVolt),        0,    65535},
     {20,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16MaintainCurrent),        0,    65535},
     {21,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16RunSet),        0,    65535},
     {22,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16ClearFaultSet),        0,    65535},
     {23,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unBMSFaultStatusWord1),        0,    65535},
     {24,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unBMSFaultStatusWord2),        0,    65535},
     {25,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unBMSFaultAlarmWord1),        0,    65535},
     {26,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&LLCSystemCtrl.u16PackPower),        0,    65535},
     {27,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[27]),        0,    65535},
     {28,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[28]),        0,    65535},
     {29,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[29]),        0,    65535},
     {30,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16Soh),        0,    65535},
     {31,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[31]),        0,    65535},
     {32,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[32]),        0,    65535},
     {33,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[33]),        0,    65535},
     {34,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[34]),        0,    65535},
     {35,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[35]),        0,    65535},
     {36,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[36]),        0,    65535},
     {37,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[37]),        0,    65535},
     {38,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[38]),        0,    65535},
     {39,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[39]),        0,    65535},
     {40,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[40]),        0,    65535},
     {41,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[41]),        0,    65535},
     {42,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unBMSIOFaultStatusWord1),        0,    65535},
     {43,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unBMSIOFaultStatusWord2),        0,    65535},
     {44,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[44]),        0,    65535},
     {45,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[45]),        0,    65535},
     {46,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[46]),        0,    65535},
     {47,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[47]),        0,    65535},
     {48,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[48]),        0,    65535},
     {49,    READ_WRITE,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[49]),        0,    65535},

     {50,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.DCState),        0,    65535},
     {51,    READ_ONLY,     TYPE_INT16,     NOT_SAVE,  (U_REG_VAL *)(&ObjSample.i16MdsIBus),        0,    65535},
     {52,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSample.u16MdsVBus),        0,    65535},
     {53,    READ_ONLY,     TYPE_INT16,     NOT_SAVE,  (U_REG_VAL *)(&ObjSample.i16MdsPower),        0,    65535},
//     {51,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultStatusWord1.u16Word),        0,    65535},
//     {52,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultStatusWord1.u16Word),        0,    65535},
//     {53,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultStatusWord1.u16Word),        0,    65535},
     {54,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultBMS1.u16Word),        0,    65535},
     {55,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultBMS2.u16Word),        0,    65535},
     {56,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unFaultBMS3.u16Word),        0,    65535},
     {57,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.unAlarmBMS.u16Word),        0,    65535},
     {58,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stSysProtect.u16FaultNum),        0,    65535},
     {59,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.uVersion.Word.u16VersionHigh),        0,    65535},
     {60,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stVersion.uVersion.Word.u16VersionLow),        0,    65535},
     {61,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16DischargeCurrentLimit),        0,    6000},
     {62,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16ChargeCurrentLimit),        0,    4000},
     {63,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16HeatingFilmEnableFlag),        0,    65535},
     {64,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSample.u16MdsIHeatingFilm),        0,    65535},
     {65,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[65]),        0,    65535},
     {66,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[66]),        0,    65535},
     {67,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16ATEEnableFlag),        0,    65535},
     {68,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16AgingSignsFlag),        0,    65535},
     {69,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjSystemCtrl.u16HeatingFilmEnableFlag),        0,    65535},
     {70,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[70]),        0,    65535},
     {71,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[71]),        0,    65535},
     {72,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16OnlineNum),        0,    65535},
     {73,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16IsCtrlWorkNum),        0,    65535},
     {74,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16IsEquipmentNormalNum),        0,    65535},
     {75,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&stBatteryHandle.u16IsParallel),        0,    65535},
     {76,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[76]),        0,    65535},
     {77,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[77]),        0,    65535},
     {78,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[78]),        0,    65535},
     {79,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[79]),        0,    65535},
     {80,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[80]),        0,    65535},
     {81,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[81]),        0,    65535},
     {82,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[82]),        0,    65535},
     {83,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[83]),        0,    65535},
     {84,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[84]),        0,    65535},
     {85,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[81]),        0,    65535},
     {86,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[82]),        0,    65535},
     {87,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[83]),        0,    65535},
     {88,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[84]),        0,    65535},
     {89,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[81]),        0,    65535},
     {90,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[82]),        0,    65535},
     {91,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[83]),        0,    65535},
     {92,    READ_ONLY,     TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&g_u16TestArry[84]),        0,    65535},

     {1080,    READ_WRITE,    TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16FirmwareType),        0,   1},
     {1081,    READ_WRITE,    TYPE_UINT16,    NOT_SAVE,  (U_REG_VAL *)(&ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16UpdateBaud),        0,   66530},

};
const ST_REG_SLAVE_CFG stRegSlaveCfg_2 = {(ST_REG_SLAVE_ITEM*)&g_stRegSlaveItem_2, MDS_ITEM_OF(g_stRegSlaveItem_2)};




#ifdef __cplusplus
}
#endif

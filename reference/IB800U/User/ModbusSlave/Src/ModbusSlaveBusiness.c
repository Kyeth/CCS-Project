#include "ModbusSlaveBusiness.h"
#include "ModbusSlaveComm.h"
#include "Hw_WatchDog.h"

#ifdef __cplusplus
extern "C" {
#endif

void ModbusSlaveCallBack_1(void)
{
    Uint16 u16FirmwareType = ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16FirmwareType;
    u16FirmwareType = u16FirmwareType;

    ModbusTimeOutCnt_2 = 0;
    if (((ObjModbusSlaveCtrl_1.stMdsFrame.bAddr == ObjModbusSlaveCtrl_1.bModbusSlaveAddr)    // 是否符合本机地址
            || (ObjModbusSlaveCtrl_1.stMdsFrame.bAddr == 0x00) )                             // 是否符合广播地址
            && (ObjModbusSlaveCtrl_1.stMdsFrame.u16RegStartAddr == 1080)                    // 是否符合升级地址
            && (ObjModbusSlaveCtrl_1.stMdsFrame.u16RegNums == 2)                            // 是否寄存器长度等于2
//            && (u16FirmwareType > 0 && u16FirmwareType <= 3)                                 // 是否符合接收的固件类型
            && ObjModbusSlaveCtrl_1.stMdsFrame.bFuncCmd == 0x10)
    {
        // 这里需要写升级标志
        ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16MdsIsUpdate = 1;

    }
}

void ModbusSlaveCallBack_2(void)
{
    Uint16 u16FirmwareType = ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16FirmwareType;
    u16FirmwareType = u16FirmwareType;

    ModbusTimeOutCnt_2 = 0;
    if (((ObjModbusSlaveCtrl_2.stMdsFrame.bAddr == ObjModbusSlaveCtrl_2.bModbusSlaveAddr)    // 是否符合本机地址
            || (ObjModbusSlaveCtrl_2.stMdsFrame.bAddr == 0x00) )                             // 是否符合广播地址
            && (ObjModbusSlaveCtrl_2.stMdsFrame.u16RegStartAddr == 1080)                    // 是否符合升级地址
            && (ObjModbusSlaveCtrl_2.stMdsFrame.u16RegNums == 2)                            // 是否寄存器长度等于2
//            && (u16FirmwareType > 0 && u16FirmwareType <= 3)                                 // 是否符合接收的固件类型
            && ObjModbusSlaveCtrl_2.stMdsFrame.bFuncCmd == 0x10)
    {
        // 这里需要写升级标志
        ObjModbusSlaveCtrl_1.stMdsUpdateParam.u16MdsIsUpdate = 1;

    }
}

#ifdef __cplusplus
}
#endif

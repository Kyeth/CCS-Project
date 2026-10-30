#include "ModbusSlaveRegCfg_1.h"
#include "ModbusSlaveRegCfg_2.h"
#include "ModbusSlaveComm.h"
#include "Modbus.h"
#include "ModbusSlave.h"
#include "Hw_SciDriver.h"
#include "PlatformManager.h"
#include "string.h"
#include "ModbusSlaveBusiness.h"
#include "Schedule.h"

#define MODBUS_TIMEOUT  18000

ST_MODBUS_SLAVE_TABLE_CRTL ObjModbusSlaveCtrl_1;    // Modbus从站对象
ST_MODBUS_SLAVE_TABLE_CRTL ObjModbusSlaveCtrl_2;    // Modbus从站对象

static Uint16 MdsBuff_1[256] = {0};   // Modbus数据空间
static Uint16 MdsBuff_2[256] = {0};   // Modbus数据空间
Uint16 ModbusTimeOutCnt_1 = 0;        // Modbus超时统计计数器
Uint16 ModbusTimeOutCnt_2 = 0;        // Modbus超时统计计数器

void ModbusSlaveInit(void)
{
    // 将Modbus寄存器通信点表、回调函数注册进控制对象
    ModbusSlaveTableRegister(&ObjModbusSlaveCtrl_1,  MODBUS_SLAVE_DEFAULT,
                             (ST_REG_SLAVE_CFG *)&stRegSlaveCfg_1,
                             ModbusSlaveCallBack_1);

    // 设置当前通信使用的通信点表
    SetModbusSlaveTableType(&ObjModbusSlaveCtrl_1, MODBUS_SLAVE_DEFAULT);
    // 设置从机通信地址
    SetModbusSlaveAddr(&ObjModbusSlaveCtrl_1, 1);



    // 将Modbus寄存器通信点表、回调函数注册进控制对象
    ModbusSlaveTableRegister(&ObjModbusSlaveCtrl_2,  MODBUS_SLAVE_DEFAULT,
                             (ST_REG_SLAVE_CFG *)&stRegSlaveCfg_2,
                             ModbusSlaveCallBack_2);

    // 设置当前通信使用的通信点表
    SetModbusSlaveTableType(&ObjModbusSlaveCtrl_2, MODBUS_SLAVE_DEFAULT);
    // 设置从机通信地址
    SetModbusSlaveAddr(&ObjModbusSlaveCtrl_2, 1);
}

void TaskModbusSlave_1(void)
{
    Uint16 ReceiveSize = 0;

    // 定义Modbus数据对象
    ST_MDS_DATA stMdsData;
    stMdsData.pu16DataBuff = MdsBuff_1;     // 给数据对象指针赋值实际数据空间
    stMdsData.u16DataLenth = 0;           // 清空接收数据的长度，在接收时：表示接收的数据长度，
                                        // 在发送时，表示发送的数据长度

    ST_MDS_FRAME *stMdsFrame = NULL;

    // 需要在初始化串口外设，通过在系统系统时已经初始化
    // SCI_ID0的通信模式为Modbus时才执行
    if (GetModeSciID0(&ObjPlatformCtrl) == MODESCI_MODBUS)
    {
        // 通信超时统计
        ModbusTimeOutCnt_1++;
        if (ModbusTimeOutCnt_1 >= MODBUS_TIMEOUT)
        {
            ModbusTimeOutCnt_1 = MODBUS_TIMEOUT;
            // 这里做一个通信超时标志
            SetModbusTimeOutFlag_1(&ObjPlatformCtrl);
        }
        else
        {
            ClearModbusTimeOutFlag_1(&ObjPlatformCtrl);
        }

        memset(stMdsData.pu16DataBuff, 0, 256);
        // 从串口接收Modbus帧
        ReceiveSize = Hw_SciRead(SCI_ID0, stMdsData.pu16DataBuff, 256);
        if (ReceiveSize)
        {
            stMdsData.u16DataLenth = ReceiveSize;

            // 调用Modbus从站协议
            stMdsFrame = ModbusSlaveHandle(&ObjModbusSlaveCtrl_1, &stMdsData);

            // 响应Modbus请求帧
            if(stMdsFrame)
            {
                Hw_SciWrite(SCI_ID0, stMdsFrame->pstMdsData->pu16DataBuff, stMdsFrame->pstMdsData->u16DataLenth);
            }
        }
    }
    SetTaskRunStatus(TASK_MOBUS_SLAVE1_BIT);
}

void TaskModbusSlave_2(void)
{
    Uint16 ReceiveSize = 0;

    // 定义Modbus数据对象
    ST_MDS_DATA stMdsData;
    stMdsData.pu16DataBuff = MdsBuff_2;     // 给数据对象指针赋值实际数据空间
    stMdsData.u16DataLenth = 0;             // 清空接收数据的长度，在接收时：表示接收的数据长度，
                                            // 在发送时，表示发送的数据长度

    ST_MDS_FRAME *stMdsFrame = NULL;

    // 需要在初始化串口外设，通过在系统系统时已经初始化
    // SCI_ID0的通信模式为Modbus时才执行
    if (GetModeSciID1(&ObjPlatformCtrl) == MODESCI_MODBUS)
    {
        // 通信超时统计
        ModbusTimeOutCnt_2++;
        if (ModbusTimeOutCnt_2 >= MODBUS_TIMEOUT)
        {
            ModbusTimeOutCnt_2 = MODBUS_TIMEOUT;
            // 这里做一个通信超时标志
            SetModbusTimeOutFlag_2(&ObjPlatformCtrl);
        }
        else
        {
            ClearModbusTimeOutFlag_2(&ObjPlatformCtrl);
        }

        memset(stMdsData.pu16DataBuff, 0, 256);
        // 从串口接收Modbus帧
        ReceiveSize = Hw_SciRead(SCI_ID1, stMdsData.pu16DataBuff, 256);
        if (ReceiveSize)
        {
            stMdsData.u16DataLenth = ReceiveSize;

            // 调用Modbus从站协议
            stMdsFrame = ModbusSlaveHandle(&ObjModbusSlaveCtrl_2, &stMdsData);

            // 响应Modbus请求帧
            if(stMdsFrame)
            {
                Hw_SciWrite(SCI_ID1, stMdsFrame->pstMdsData->pu16DataBuff, stMdsFrame->pstMdsData->u16DataLenth);
            }
        }
    }
    SetTaskRunStatus(TASK_MOBUS_SLAVE2_BIT);
}

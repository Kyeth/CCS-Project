//*****************************************************************************
#ifndef User_Modbus_H
#define User_Modbus_H

#ifdef __cplusplus
extern "C" {
#endif

//*****************************************************************************
#define MaxAin  					Database_Num   	 	//模拟量输入通道数
#define MaxHold						Database_Num   		//保持寄存器通道数

#define ILLEGAL_FUNCTION        	0x01                        
#define ILLEGAL_DATA_ADDRESS    	0x02
#define ILLEGAL_DATA_VALUE      	0x03
#define SLAVE_DEVICE_FAILURE    	0x04
#define ACKNOWLEDGE             	0x05
#define SLAVE_DEVICE_BUSY       	0x06

#define FIFO_NUM 16
////=============================================================================
struct User_485_Info
{
    Uint16 SdataANum;       // 已装载byte
    Uint16 RdataANum;       // 已接收byte
    Uint16 SciTXFlag;       // 数据发送标记
    Uint16 SciRXFlag;       // 数据接收标记
    Uint16 ModbusTxNum;     // modbus 发送总 bytes
    Uint16 ModbusRxNum;     // modbus 接收总 bytes
    Uint16 MaxAddr;          // 最大有效地址
    Uint16 ModbusID;        // 初始化时设从机通讯地址
    Uint16 RdataA[256];      // 接收数组
    Uint16 SdataA[256];      // 发送数组
    char SciName;            // 串口名称 A~D
    volatile struct SCI_REGS *SciReg; // Sci寄存器指针
};

//=============================================================================
struct HoldRegister
{
    Uint16 AddrStart;                         //寄存器起始地址
    Uint16 TempAddr;
    Uint16 CRCCheck;
    Uint16 RegistersCount;                    //要读寄存器的个数
    Uint16 ByteCount;                         //字节计数
    Uint16 TempData;
};

void InitUserModbusData(void);
void HandleModbus(void);
void SetUserModbusID(Uint16 id);
//*****************************************************************************
#ifdef __cplusplus
}
#endif /* extern "C" */

#endif  

///////////////////////////////////////////////////////////////////////////////
// No more.
///////////////////////////////////////////////////////////////////////////////




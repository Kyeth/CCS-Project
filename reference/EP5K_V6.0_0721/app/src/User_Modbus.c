/************************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
FileName: User_Modbus.c
Author: tzz
Date:2022年2月15日
Description: 485通信
Version:0.0.2
Function List:
***********************************************************/
#include "bsp.h"
#include "string.h"
#include "F28x_Project.h"
#include "User_Modbus.h"
#include "ModbusTable.h"
#include "Protect.h"
#include "User_Database.h"
#include "Update.h"
#include "User_Eeprom.h"
//***************************************************************************
struct HoldRegister UserRegiser;    //
struct HoldRegister ArmRegiser;     //
struct User_485_Info UserComInfo;   // 调试
struct User_485_Info ArmComInfo;    // ARM 通信

uint16_t ArmCommuErrCnt;

const Uint16 crc16Table[256] = {
    0x0000, 0xC1C0, 0x81C1, 0x4001, 0x01C3, 0xC003, 0x8002, 0x41C2, 0x01C6, 0xC006, 0x8007, 0x41C7,
    0x0005, 0xC1C5, 0x81C4, 0x4004, 0x01CC, 0xC00C, 0x800D, 0x41CD, 0x000F, 0xC1CF, 0x81CE, 0x400E,
    0x000A, 0xC1CA, 0x81CB, 0x400B, 0x01C9, 0xC009, 0x8008, 0x41C8, 0x01D8, 0xC018, 0x8019, 0x41D9,
    0x001B, 0xC1DB, 0x81DA, 0x401A, 0x001E, 0xC1DE, 0x81DF, 0x401F, 0x01DD, 0xC01D, 0x801C, 0x41DC,
    0x0014, 0xC1D4, 0x81D5, 0x4015, 0x01D7, 0xC017, 0x8016, 0x41D6, 0x01D2, 0xC012, 0x8013, 0x41D3,
    0x0011, 0xC1D1, 0x81D0, 0x4010, 0x01F0, 0xC030, 0x8031, 0x41F1, 0x0033, 0xC1F3, 0x81F2, 0x4032,
    0x0036, 0xC1F6, 0x81F7, 0x4037, 0x01F5, 0xC035, 0x8034, 0x41F4, 0x003C, 0xC1FC, 0x81FD, 0x403D,
    0x01FF, 0xC03F, 0x803E, 0x41FE, 0x01FA, 0xC03A, 0x803B, 0x41FB, 0x0039, 0xC1F9, 0x81F8, 0x4038,
    0x0028, 0xC1E8, 0x81E9, 0x4029, 0x01EB, 0xC02B, 0x802A, 0x41EA, 0x01EE, 0xC02E, 0x802F, 0x41EF,
    0x002D, 0xC1ED, 0x81EC, 0x402C, 0x01E4, 0xC024, 0x8025, 0x41E5, 0x0027, 0xC1E7, 0x81E6, 0x4026,
    0x0022, 0xC1E2, 0x81E3, 0x4023, 0x01E1, 0xC021, 0x8020, 0x41E0, 0x01A0, 0xC060, 0x8061, 0x41A1,
    0x0063, 0xC1A3, 0x81A2, 0x4062, 0x0066, 0xC1A6, 0x81A7, 0x4067, 0x01A5, 0xC065, 0x8064, 0x41A4,
    0x006C, 0xC1AC, 0x81AD, 0x406D, 0x01AF, 0xC06F, 0x806E, 0x41AE, 0x01AA, 0xC06A, 0x806B, 0x41AB,
    0x0069, 0xC1A9, 0x81A8, 0x4068, 0x0078, 0xC1B8, 0x81B9, 0x4079, 0x01BB, 0xC07B, 0x807A, 0x41BA,
    0x01BE, 0xC07E, 0x807F, 0x41BF, 0x007D, 0xC1BD, 0x81BC, 0x407C, 0x01B4, 0xC074, 0x8075, 0x41B5,
    0x0077, 0xC1B7, 0x81B6, 0x4076, 0x0072, 0xC1B2, 0x81B3, 0x4073, 0x01B1, 0xC071, 0x8070, 0x41B0,
    0x0050, 0xC190, 0x8191, 0x4051, 0x0193, 0xC053, 0x8052, 0x4192, 0x0196, 0xC056, 0x8057, 0x4197,
    0x0055, 0xC195, 0x8194, 0x4054, 0x019C, 0xC05C, 0x805D, 0x419D, 0x005F, 0xC19F, 0x819E, 0x405E,
    0x005A, 0xC19A, 0x819B, 0x405B, 0x0199, 0xC059, 0x8058, 0x4198, 0x0188, 0xC048, 0x8049, 0x4189,
    0x004B, 0xC18B, 0x818A, 0x404A, 0x004E, 0xC18E, 0x818F, 0x404F, 0x018D, 0xC04D, 0x804C, 0x418C,
    0x0044, 0xC184, 0x8185, 0x4045, 0x0187, 0xC047, 0x8046, 0x4186, 0x0182, 0xC042, 0x8043, 0x4183,
    0x0041, 0xC181, 0x8180, 0x4040,
};

void InitUserModbusData(void)
{
    memset(&UserComInfo, 0, sizeof(struct User_485_Info));
    memset(&ArmComInfo, 0, sizeof(struct User_485_Info));
    memset(&UserRegiser, 0, sizeof(struct HoldRegister));
    memset(&ArmRegiser, 0, sizeof(struct HoldRegister));

    // SCID 调试用
    UserComInfo.SciTXFlag = 1;
    UserComInfo.ModbusTxNum = 256;
    UserComInfo.ModbusRxNum = 256;
    UserComInfo.ModbusID = 0x01;
    UserComInfo.SciName = 'D';
    UserComInfo.SciReg = &ScidRegs;
    UserComInfo.MaxAddr = 0xffff;
    // SCID 用于 ARM 通信
    ArmComInfo.SciTXFlag = 1;
    ArmComInfo.ModbusTxNum = 256;
    ArmComInfo.ModbusRxNum = 256;
    ArmComInfo.ModbusID = 0x01;
    ArmComInfo.SciName = 'A';
    ArmComInfo.SciReg = &SciaRegs;
    ArmComInfo.MaxAddr = 0xffff;
}
void SetUserModbusID(Uint16 id)
{
    UserComInfo.ModbusID = id;
}
//***************************************************************************
//函数名：获取保持寄存器值
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void GetRegisterVal(Uint16 Addr,Uint16 *ReadData, char com)
{
    if (com == 'A')             // ARM通信
    {
        *ReadData = ReadRegisterTables(Addr);

        ClearCommunicationFaultCount();
    }
    else if (com == 'D')        // debug sci
    {
        ClearRS485CommunicationFaultCount();
        if(Addr >= 20000)
        {
            Addr-=20000;
            if(ErrArray_NO>0)   //ErrArray_NO>0时上传EEPROM中存储的故障数据
            {
                if(Addr>=Database_Num) Addr=0;

                if(*(Uint16*)Database[Addr].p==ErrArray_Num)        //ErrArray_Num和ErrArray_NO不从EEPROM中取
                    *ReadData=ErrArray_Num;

                else if(*(Uint16*)Database[Addr].p==ErrArray_NO)
                    *ReadData=ErrArray_NO;

                else
                    *ReadData=Eeprom_shadow[Addr];          //其它数据从EEPROM中读取
            }
            else
            {
                *ReadData = Database_Read(Addr);
            }
        }
        else
        {
            *ReadData = ReadRegisterDebug(Addr);
        }
    }
}
//***************************************************************************
//函数名：设置保持寄存器值
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void SetRegisterVal(Uint16 Addr, Uint16 SetState, char com)
{
    if (com == 'A')             // ARM 通信
    {
        WriteRegisterTables(SetState, Addr);

        ClearCommunicationFaultCount();
    }
    else if (com == 'D')        // debug SCI
    {
        ClearRS485CommunicationFaultCount();

        if (Addr >= 20000)
        {
            Database_Write(Addr-20000, SetState);
        }
        WriteRegisterDebug(SetState, Addr);
    }
}
		 
//***************************************************************************
//函数名：Modbus CRC校验
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
Uint16 Crc16(Uint16 *puchMsg, Uint16 usDataLen) 
{ 
	Uint16 uchCRCHi = 0xFF ;
	Uint16 uchCRCLo = 0xFF ;
	Uint32 uIndex ;
	while (usDataLen--)
	{
		uIndex = uchCRCHi ^ *puchMsg++ ;
		uchCRCHi = uchCRCLo ^ (crc16Table[uIndex] >>8) ;
		uchCRCLo = (crc16Table[uIndex] & 0xFF) ;
	}
	return (uchCRCHi << 8 | uchCRCLo) ;
}

//***************************************************************************
//函数名：Modbus错误应答
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void ModbusExcep(struct User_485_Info *modbus, Uint16 ExceptionCode)
{
	Uint16 CRCCheck = 0;
	
	modbus->SdataA[0] = modbus->ModbusID;
	modbus->SdataA[1] = modbus->RdataA[1] + 0x80;
	modbus->SdataA[2] = ExceptionCode;

	CRCCheck  = Crc16(modbus->SdataA, 3);
	modbus->SdataA[3] = CRCCheck >> 8;
	modbus->SdataA[4] = CRCCheck & 0xFF;
	modbus->ModbusTxNum = 5;

	// 错误计数
	if (modbus->SciName == 'A')
	{
	    ArmCommuErrCnt++;
	}
}

//***************************************************************************
//函数名：接收数据从机地址检查
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
Uint16 AddrCheck(Uint16 AddrStart, Uint16 Num, Uint16 AddrMax)
{
	Uint16 Result = 0;

	//错误2：查询中的数据地址对于从机来说是非法的	
	if (Num <= 0 || (AddrStart + Num > AddrMax))
	{
		Result = 1;
	}

	return Result;
}

//***************************************************************************
//函数名：读保持寄存器03H
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void ReadHoldRegisters(struct HoldRegister *reg, struct User_485_Info *modbus)
{
	Uint16 i = 0;

	reg->AddrStart = (modbus->RdataA[2]<<8) + modbus->RdataA[3];
	reg->TempAddr = reg->AddrStart;
	reg->RegistersCount = (modbus->RdataA[4]<<8) + modbus->RdataA[5];
	reg->ByteCount = reg->RegistersCount * 2;
	
	if (AddrCheck(reg->AddrStart, reg->RegistersCount, modbus->MaxAddr))
	{
	    ModbusExcep(modbus, ILLEGAL_DATA_ADDRESS); // 非法地址

		return;
	}

	for (i = 0; i < reg->ByteCount; i+=2,reg->TempAddr++)
	{
		GetRegisterVal(reg->TempAddr, &reg->TempData, modbus->SciName);
		modbus->SdataA[i+3] = reg->TempData >> 8;
		modbus->SdataA[i+4] = reg->TempData & 0xff;
	}
	
	modbus->SdataA[0] = modbus->ModbusID;
	modbus->SdataA[1] = modbus->RdataA[1];
	modbus->SdataA[2] = reg->ByteCount;
	reg->CRCCheck = Crc16(modbus->SdataA, reg->ByteCount + 3);
	modbus->SdataA[reg->ByteCount + 3] = reg->CRCCheck >> 8;
	modbus->SdataA[reg->ByteCount + 4] = reg->CRCCheck & 0xff;
	modbus->ModbusTxNum = reg->ByteCount + 5;
}

//***************************************************************************
//函数名：设置单个保持寄存器06H
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void PresetSingleRegister(struct User_485_Info *modbus)
{
	Uint16  AddrStart=0;
	Uint16  TempAddr=0;
	Uint16  SetData=0;
	Uint16   i=0;
	
	AddrStart = (modbus->RdataA[2]<<8) + modbus->RdataA[3];
	TempAddr = AddrStart;
	SetData = (modbus->RdataA[4]<<8) + modbus->RdataA[5];

	if (AddrCheck(AddrStart, 1, modbus->MaxAddr))
	{
	    ModbusExcep(modbus, ILLEGAL_DATA_ADDRESS); // 非法地址

		return;
	}

	SetRegisterVal(TempAddr, SetData, modbus->SciName);

	for (i = 0; i < modbus->RdataANum; i++)
	{
	    modbus->SdataA[i] = modbus->RdataA[i];
	}
	modbus->ModbusTxNum = 8;
}
//***************************************************************************
//函数名：设置多个保持寄存器10H
//输  入:
//输  出:
//描  述:
//***************************************************************************
void PresetMultipleRegister(struct HoldRegister *reg, struct User_485_Info *modbus)
{
    Uint16  SetData = 0;
    Uint16  i = 0;

    reg->AddrStart = (modbus->RdataA[2]<<8) + modbus->RdataA[3];
    reg->TempAddr = reg->AddrStart;
    reg->RegistersCount = (modbus->RdataA[4]<<8) + modbus->RdataA[5];
    reg->ByteCount = reg->RegistersCount * 2;

    // 升级指令不需要响应
    if ((reg->AddrStart == 0x0438) && (modbus->SciName == 'A'))
    {
        return;
    }

    if (AddrCheck(reg->AddrStart, reg->RegistersCount, modbus->MaxAddr))
    {
        ModbusExcep(modbus, ILLEGAL_DATA_ADDRESS); // 非法地址

        return;
    }

    for (i = 0; i < reg->ByteCount; i+=2, reg->TempAddr++)
    {
        SetData = (modbus->RdataA[i+7]<<8) + modbus->RdataA[i+8];
        SetRegisterVal(reg->TempAddr, SetData, modbus->SciName);
    }

    for(i = 0; i < 6; i++)
    {
        modbus->SdataA[i] = modbus->RdataA[i];
    }

    reg->CRCCheck = Crc16(modbus->SdataA, 6);
    modbus->SdataA[6] = reg->CRCCheck >> 8;
    modbus->SdataA[7] = reg->CRCCheck & 0xff;
    modbus->ModbusTxNum = 8;
}

//***************************************************************************
//函数名：Modbus协议主程序
//输  入: 
//输  出:
//描  述: 
//***************************************************************************
void UserModbus(struct HoldRegister *reg, struct User_485_Info *modbus)
{
    Uint16 CRCCheck = 0;

	switch(modbus->RdataA[1])              // 判断功能码
	{
		case 0x03:                            // 读保持存储器的内容
		case 0x04:
		{
            CRCCheck = Crc16(modbus->RdataA, (modbus->RdataANum) - 2 );
            if(CRCCheck == modbus->RdataA[modbus->RdataANum - 1] +
                    (modbus->RdataA[modbus->RdataANum - 2]<<8))
            {
                ReadHoldRegisters(reg, modbus);       // CRC校验正确
            }
            else
            {
                ModbusExcep(modbus, 0x02); // CRC校验不正确
            }
            break;
		}
		case 0x06://设置单个保持寄存器
		{
            CRCCheck = Crc16(modbus->RdataA, modbus->RdataANum - 2);
            if(CRCCheck == modbus->RdataA[modbus->RdataANum - 1] +
                    (modbus->RdataA[modbus->RdataANum - 2] << 8))
            {
                PresetSingleRegister(modbus);    // 解析
            }
            else
            {
                ModbusExcep(modbus, 0x02); // CRC 校验不正确
            }
            break;
		}
		case 0x10://设置
		{
            CRCCheck = Crc16(modbus->RdataA,modbus->RdataANum - 2);
            if (CRCCheck == modbus->RdataA[modbus->RdataANum-1] +
                    (modbus->RdataA[modbus->RdataANum - 2]<<8))
            {
                PresetMultipleRegister(reg, modbus);   // CRC 校验正确
            }
            else
            {
                ModbusExcep(modbus, 0x02);  // CRC 校验不正确
            }
            break;
		}
		default://从机接收到的功能码不能识别
		{
            ModbusExcep(modbus, ILLEGAL_FUNCTION);
            break;
		}
	}
}
//***************************************************************************
Uint16 User485Read(struct User_485_Info *modbus)
{
    if (modbus->SciReg->SCIFFRX.bit.RXFFST == 0)  //未收到数据
    {
        if ((modbus->SciRXFlag >= 5) || (modbus->SciReg->SCIRXST.bit.BRKDT == 1)) //超时5个循环周期
        {
            InitSci(modbus->SciName);                  // SCI复位
            modbus->SdataANum = 0;
            modbus->RdataANum = 0;
            modbus->SciTXFlag = 1;
            modbus->SciRXFlag = 0;

            return 0;
        }

        if (modbus->RdataANum == 0)               // 上次是否有收到数据？
        {
            return 0;
        }
        else if (modbus->RdataANum >= 8)          // 上次有8个数据
        {
            return 1;
        }
        else                                       // 上次收到的数据不到一帧
        {
            modbus->SciRXFlag++;                  // 超时等待中
        }

    }

    while (modbus->SciReg->SCIFFRX.bit.RXFFST != 0)//收到新数据
    {
        modbus->RdataA[modbus->RdataANum++] = modbus->SciReg->SCIRXBUF.all;
        if (modbus->RdataANum >= modbus->ModbusRxNum)
        {
            modbus->RdataANum = 0;
        }
    }
    return 0;
}
//***************************************************************************
Uint16 User485Write(struct User_485_Info *modbus)
{
    char *msg;
    Uint16 num;
    Uint16 i = 0;

    msg = (char*)modbus->SdataA;
    num = modbus->ModbusTxNum - modbus->SdataANum;

    if (modbus->SciReg->SCIFFTX.bit.TXFFST != 0)    // 是否有未发送完的数
    {
        return 0;
    }
    if (modbus->SdataANum >= modbus->ModbusTxNum)   // 数据装载完毕
    {
        modbus->ModbusTxNum = 0;
        modbus->SdataANum = 0;

        return 1;
    }

    //有待装载的数则开始进行数据发送
    if (num >= FIFO_NUM)                           //超出FIFO的发送
    {
        for (i = 0; i < FIFO_NUM; i++)
        {
            modbus->SciReg->SCITXBUF.all = msg[modbus->SdataANum++];
        }

        return 0;
    }
    else                                //未超出部分的发送
    {
        for (i = 0; i < num; i++)
        {
            modbus->SciReg->SCITXBUF.all = msg[modbus->SdataANum++];
        }

        return 0;
    }
}

//***************************************************************************
void User485(struct HoldRegister *reg, struct User_485_Info *modbus)
{
    Uint16 ret = 0;

    if (modbus->SciTXFlag == 0)                           // 上次发送未完
    {
        ret = User485Write(modbus);

        if (ret == 1)
        {
            modbus->SciTXFlag = 1;
        }

        return;
    }

    // 发送完毕，等待10个循环周期
    if ((modbus->SciTXFlag >= 1) && (modbus->SciTXFlag < 10)) // 发送与接收之间所需的时间间隔
    {
        modbus->SciTXFlag++;
        return;
    }
    else                                         // 等待完毕，开始接收
    {
        if (User485Read(modbus) == 1)           // 收到数据
        {
            if (modbus->RdataA[0] == modbus->ModbusID)     // 判断从机地址是否正确 由于只是ARM与DSP通讯，后续可能需要删除该判断
            {
                UserModbus(reg, modbus);                   // Modbus 数据解析及回复
                modbus->SciTXFlag = 0;
            }
            Protect.Enable.bit.GFCI = 1;
            modbus->RdataANum = 0;
        }
        else if (modbus->RdataANum == 5)
        {
            // DSP 升级指令,需要跳转到 BootLoader
            if (modbus->RdataA[0] == 0xf0 &&
                    modbus->RdataA[1] == 0x00 &&
                    modbus->RdataA[2] == 0x12 &&
                    modbus->RdataA[3] == 0x03 &&
                    modbus->RdataA[4] == 0x04 &&
                    ReadUpdateState())
            {
                modbus->SdataA[0] = 0xf0;
                modbus->SdataA[1] = 0;
                modbus->SdataA[2] = 0x12;
                modbus->SdataA[3] = 0x05;
                modbus->SdataA[4] = 0x06;
                modbus->ModbusTxNum = 5;

                UpdateData.UpdateFlag = 1;

                modbus->SciTXFlag = 0;
                modbus->RdataANum = 0;
            }
            // ARM 升级,不需要跳转
            else if (modbus->RdataA[0] == 0xf0 &&
                    modbus->RdataA[1] == 0x01 &&
                    modbus->RdataA[2] == 0x12 &&
                    modbus->RdataA[3] == 0x07 &&
                    modbus->RdataA[4] == 0x08 &&
                    ReadUpdateState())
            {
                modbus->SdataA[0] = 0xf0;
                modbus->SdataA[1] = 0x01;
                modbus->SdataA[2] = 0x12;
                modbus->SdataA[3] = 0x09;
                modbus->SdataA[4] = 0x0A;
                modbus->ModbusTxNum = 5;

                UpdateData.UpdateFlag = 2;
                Protect.Enable.bit.GFCI = 0;

                modbus->SciTXFlag = 0;
                modbus->RdataANum = 0;
            }
        }
    }
}
// 2ms 调用一次
static Uint16 updateRspDly = 0;     // 升级响应延时
void HandleModbus(void)
{
    // arm 升级指令下发1s后响应arm指令
    if ((UpdateData.UpdateFlag == 0) || (updateRspDly >= 500))
    {
        if (UpdateData.UpdateFlag == 2)
        {
            // 应答ARM升级后延时200ms再清零UpdateFlag
            if (++updateRspDly == 600)
            {
                UpdateData.UpdateFlag = 0;      // 复位 ARM 升级指令
                updateRspDly = 0;
            }
        }

        User485(&ArmRegiser, &ArmComInfo);
    }
    else
    {
        updateRspDly++;
    }

//    User485(&UserRegiser, &UserComInfo);
}


//===========================================================================
// No more.
//===========================================================================

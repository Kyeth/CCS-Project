/*
 * User_Eeprom.c
 * */
//###########################################################################
#include "F28x_Project.h"
#include "User_Database.h"
#include "bsp.h"
#include "User_Eeprom.h"
#include "User_I2C.h"
#include "Protect.h"
//***************************************************************************
Uint16 ErrArray_Num;                       //Eeprom中存放的故障数据的总数（+1）
Uint16 ErrArray_NO;                        //Eeprom中故障数组的序号

struct I2CMSG EEPROM_Write = {
    I2C_MSGSTAT_INACTIVE,
    EEPROM_SLAVE_ADDR,
    EEPROM_WriteBytes,
    EEPROM_HIGH_ADDR,
    EEPROM_LOW_ADDR,
};
struct I2CMSG EEPROM_Read = {
    I2C_MSGSTAT_INACTIVE,
    EEPROM_SLAVE_ADDR,
    EEPROM_ReadBytes,
    EEPROM_HIGH_ADDR,
    EEPROM_LOW_ADDR,
};
////***************************************************************************
//void User_Eeprom_Clr(void)
//{
//	static Uint16 i=0,j=0;
//	struct I2CMSG EEPROM_Init={0,0x50,2,0,0};
////============================================================================
//	//将EEPROM内容全部清零
//	if (User_I2cFlag.bit.EeData_clr == 1)
//	{
//		if (i < EEPROM_HIGH_ADDR)				//余一页作为固定数据存储(_zyb@20190826)
//		{
//			EEPROM_Init.MemoryHighAddr=i;
//			EEPROM_Init.MemoryLowAddr=j;
//	    	EEPROM_Init.MsgBuffer[0]=0x0;
//			EEPROM_Init.MsgBuffer[1]=0x0;
//			if(UserI2c_Write(&EEPROM_Init)!=I2C_SUCCESS) return;
//			j+=2;
//			if(j==0x100)
//			{
//				j=0;
//				i++;
//			}
//		}
//		else
//		{
//			i=0;
//			ErrArray_Num=1;
//			User_I2cFlag.bit.EeData_clr=0;
//		}
//	}
//}
////***************************************************************************
void User_Eeprom_Init(void)
{
	//初始化时从Eeprom里读取最末尾的故障数组，并作为ErrArray_Num的初值
    mSecCNT_struct mSecCNT = {0,0};
	Uint16 i,tmp;
	ErrArray_Num = 0;

	for(i = 0; i <= ((EEPROM_HIGH_ADDR<<8)+EEPROM_LOW_ADDR); i+=2*Database_Num)
	{
		EEPROM_Read.MemoryHighAddr = i >> 8;
		EEPROM_Read.MemoryLowAddr = i & 0xff;
		// 防止阻塞-100ms超时退出
		while ((UserI2c_Read(&EEPROM_Read)!=I2C_SUCCESS) && (User_usCount(&mSecCNT) < 100000L))
		{
		    asm(" NOP");
		}

		tmp = (EEPROM_Read.MsgBuffer[1]<<8) + EEPROM_Read.MsgBuffer[0];

        ErrArray_Num++;
		if (tmp == 0)
        {
		    break;
        }
		if (ErrArray_Num == 80)
		{
		    tmp = 0;
		}
	}
}
//
////****************************************************************************
//void User_Eeprom_Write(void)
//{
//	Uint16 EeData_Addr;
//	Uint16 Database_ReadData;
//	static Uint16 Data_WriteNum=0;
//
////============================================================================
////EeWrite_EN==1时，将当前Database数据写入Eeprom中，写完后ErrArray_Num加1
//
//	if (User_I2cFlag.bit.EeWrite_EN == 1)
//	{
//		EeData_Addr = 2*((ErrArray_Num-1)*Database_Num+Data_WriteNum);//数据地址=数组总数*Database数组个数
//
//		if (EeData_Addr >= ((EEPROM_HIGH_ADDR<<8) + EEPROM_LOW_ADDR) - Database_Num*2)
//		{
//            User_I2cFlag.bit.EeData_clr = 1;
//            User_I2cFlag.bit.EeWrite_EN = 0;
//            Data_WriteNum = 0;
//            return;
//		}
//
//		EEPROM_Write.MemoryHighAddr = EeData_Addr >> 8;
//		EEPROM_Write.MemoryLowAddr = EeData_Addr & 0xff;
//
////		Database_ReadData = Database_Read(Data_WriteNum);
//		Database_ReadData = Eeprom_Wshadow[Data_WriteNum];
//
//		EEPROM_Write.MsgBuffer[1] = Database_ReadData >> 8;		//高8位
//		EEPROM_Write.MsgBuffer[0] = Database_ReadData & 0xff;	//低8位
//
//		if(UserI2c_Write(&EEPROM_Write) != I2C_SUCCESS)	//上次EEPROM未写完
//		{
//			User_I2cFlag.bit.EeBusy=1;
//			return;
// 		}
//		else	//写完本个数据后
//		{
//			Data_WriteNum++;
//			if(Data_WriteNum<Database_Num)	return;	//等待全部写完
//
//			Data_WriteNum = 0;
//			User_I2cFlag.bit.EeBusy = 0;
//
//			ErrArray_Num++;
//			User_I2cFlag.bit.EeWrite_EN = 0;	//清标志位
//
//			Database_Clear();
//		}
//	}
//}
//
////****************************************************************************
//void User_Eeprom_Read(void)
//{
//	Uint16 EeData_Addr;
//	Uint16 Database_WriteData;
//	static Uint16 Data_ReadNum=0;
//
////============================================================================
////EeRead_EN==1时，读取Eeprom中第ErrArray_NO组故障数据至Eeprom_shadow中
//
//	if(User_I2cFlag.bit.EeRead_EN==1)
//	{
//		EeData_Addr=2*((ErrArray_NO-1)*Database_Num+Data_ReadNum);	//故障数据地址=序号*数组个数
//
//		if(EeData_Addr>=(EEPROM_HIGH_ADDR<<8)+EEPROM_LOW_ADDR)
//			EeData_Addr=0x0;
//
//		EEPROM_Read.MemoryHighAddr = EeData_Addr >> 8;
//		EEPROM_Read.MemoryLowAddr = EeData_Addr & 0xff;
//
//		if(UserI2c_Read(&EEPROM_Read)!=I2C_SUCCESS)			//上次EEPROM未读完
//		{
//			User_I2cFlag.bit.EeBusy=1;
//			return;
//  		}
//		else	//读完当前之后
//		{
//			Database_WriteData=(EEPROM_Read.MsgBuffer[1]<<8)+EEPROM_Read.MsgBuffer[0];
//			Eeprom_shadow[Data_ReadNum]=Database_WriteData;
//
//			Data_ReadNum++;
//			if(Data_ReadNum<Database_Num)	return;			//等待全部读完
//
//			Data_ReadNum=0;
//			User_I2cFlag.bit.EeBusy=0;
//			User_I2cFlag.bit.EeRead_EN=0;	//清标志位
//		}
//	}
//}
////****************************************************************************
//
//void User_Eeprom(void)
//{
////    if (CheckErrState())
////    {
////        User_I2cFlag.bit.EeWrite_EN = 1;
////    }
//
//    if (ErrArray_NO > 0)
//    {
//        User_I2cFlag.bit.EeRead_EN = 1;
//    }
//
//    if (User_I2cFlag.bit.EeWrite_EN==1)
//    {
//        Database_Save();
//    }
//
//    // 格式化
//	if (User_I2cFlag.bit.EeData_clr==1)
//	{
//		User_Eeprom_Clr();
//	}
//
//	else if (User_I2cFlag.bit.EeRead_EN==1)
//	{
//		User_Eeprom_Read();
//	}
//
//	else if (User_I2cFlag.bit.EeWrite_EN==1)
//	{
//		User_Eeprom_Write();
//	}
//}


//===========================================================================
// No more.
//===========================================================================


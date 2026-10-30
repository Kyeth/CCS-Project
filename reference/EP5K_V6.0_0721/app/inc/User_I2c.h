//*****************************************************************************
#ifndef User_UserI2c_H
#define User_UserI2c_H

#ifdef __cplusplus
extern "C" {
#endif

#include "F28x_Project.h"

// Error Messages
#define I2C_ERROR               0xFFFF
#define I2C_ARB_LOST_ERROR      0x0001
#define I2C_NACK_ERROR          0x0002
#define I2C_BUS_BUSY_ERROR      0x1000
#define I2C_STP_NOT_READY_ERROR 0x5555
#define I2C_NO_FLAGS            0xAAAA
#define I2C_SUCCESS             0x0000

// Clear Status Flags
#define I2C_CLR_AL_BIT          0x0001
#define I2C_CLR_NACK_BIT        0x0002
#define I2C_CLR_ARDY_BIT        0x0004
#define I2C_CLR_RRDY_BIT        0x0008
#define I2C_CLR_SCD_BIT         0x0020

// Interrupt Source Messages
#define I2C_NO_ISRC             0x0000
#define I2C_ARB_ISRC            0x0001
#define I2C_NACK_ISRC           0x0002
#define I2C_ARDY_ISRC           0x0003
#define I2C_RX_ISRC             0x0004
#define I2C_TX_ISRC             0x0005
#define I2C_SCD_ISRC            0x0006
#define I2C_AAS_ISRC            0x0007

// I2CMSG structure defines
#define I2C_NO_STOP  0
#define I2C_YES_STOP 1
#define I2C_RECEIVE  0
#define I2C_TRANSMIT 1
//#define I2C_MAX_BUFFER_SIZE 	16

// I2C Slave State defines
#define I2C_NOTSLAVE      0
#define I2C_ADDR_AS_SLAVE 1
#define I2C_ST_MSG_READY  2

// I2C Slave Receiver messages defines
#define I2C_SND_MSG1 1
#define I2C_SND_MSG2 2

// I2C State defines
#define I2C_IDLE               0
#define I2C_SLAVE_RECEIVER     1
#define I2C_SLAVE_TRANSMITTER  2
#define I2C_MASTER_RECEIVER    3
#define I2C_MASTER_TRANSMITTER 4

// I2C  Message Commands for I2CMSG struct
#define I2C_MSGSTAT_INACTIVE          0x0000
#define I2C_MSGSTAT_SEND_WITHSTOP     0x0010
#define I2C_MSGSTAT_WRITE_BUSY        0x0011
#define I2C_MSGSTAT_SEND_NOSTOP       0x0020
#define I2C_MSGSTAT_SEND_NOSTOP_BUSY  0x0021
#define I2C_MSGSTAT_RESTART           0x0022
#define I2C_MSGSTAT_READ_BUSY         0x0023

// Generic defines
#define I2C_TRUE  1
#define I2C_FALSE 0
#define I2C_YES   1
#define I2C_NO    0
#define I2C_DUMMY_BYTE 0


//--------------------------------------------
// Structures
//--------------------------------------------
//
//// I2C Message Structure
//struct I2CMSG {
//  Uint16 MsgStatus;             // Word stating what state msg is in:
//                                //   I2C_MSGCMD_INACTIVE = do not send msg
//                                //   I2C_MSGCMD_BUSY = msg start has been sent,
//                                //                     awaiting stop
//                                //   I2C_MSGCMD_SEND_WITHSTOP = command to send
//                                //       master trans msg complete with a stop bit
//                                //   I2C_MSGCMD_SEND_NOSTOP = command to send
//                                //       master trans msg without the stop bit
//                                //   I2C_MSGCMD_RESTART = command to send a restart
//                                //       as a master receiver with a stop bit
//  Uint16 SlaveAddress;          // I2C address of slave msg is intended for
//  Uint16 NumOfBytes;            // Num of valid bytes in (or to be put in MsgBuffer)
//  Uint16 MemoryHighAddr;        // EEPROM address of data associated with msg (high byte)
//  Uint16 MemoryLowAddr;         // EEPROM address of data associated with msg (low byte)
//  Uint16 MsgBuffer[I2C_MAX_BUFFER_SIZE];    // Array holding msg data - max that
//                                            // MAX_BUFFER_SIZE can be is 4 due to
//                                            // the FIFO's
//};

//*****************************************************************************
#define I2C_MSGSTAT_READ_BUSY1          0x1110
#define I2C_MSGSTAT_READ_BUSY2          0x2220
#define I2C_MSGSTAT_WRITE_BUSY1         0x0111
#define I2C_MSGSTAT_WRITE_BUSY2         0x0222
#define I2C_MSGSTAT_WRITE_BUSY3         0x0333
#define I2C_MSGSTAT_WRITE_BUSY4         0x0444

#define EEPROM_SLAVE_ADDR				0x50						//FM24C256从机地址0x50
#define	EEPROM_WriteBytes				2
#define	EEPROM_ReadBytes				2
#define	EEPROM_HIGH_ADDR				0x3F						//FM24C128高地址范围：00~3F
#define	EEPROM_LOW_ADDR					0x00						//FM24C128低地址范围：00~FF

#define	tWR								5000							//FM24C128写入延迟5ms

//*****************************************************************************
struct User_I2cFlag_BITS{
	Uint16 EeBusy:1;
	Uint16 EeRead_EN:1;
	Uint16 EeWrite_EN:1;
	Uint16 EeData_clr:1;		//Eeprom清空
	Uint16 ErrData_Write:1;		//写入新故障
	Uint16 ErrData_Read:1;		//读取故障数据
	Uint16 rsvd1:10;			//预留
};

union User_I2cFlag_REGS{
   	Uint16                 		all;
   	struct User_I2cFlag_BITS  	bit;
};

extern union User_I2cFlag_REGS User_I2cFlag;
extern Uint16 UserI2c_Write(struct I2CMSG *UserI2cMSG);
extern Uint16 UserI2c_Read(struct I2CMSG *UserI2cMSG);

//*****************************************************************************
#ifdef __cplusplus
}
#endif /* extern "C" */

#endif

///////////////////////////////////////////////////////////////////////////////
// No more.
///////////////////////////////////////////////////////////////////////////////




//###########################################################################
#include "User_Database.h"
#include "User_I2C.h"
#include "bsp.h"

//***************************************************************************
union User_I2cFlag_REGS User_I2cFlag={0x00};	//注意：将I2C_MAX_BUFFER_SIZE定义为16

Uint16 UserI2c_Read(struct I2CMSG *msg)
{
	Uint16 i;
	switch(msg->MsgStatus)
	{
//============================================================================
		case I2C_MSGSTAT_INACTIVE:
	 	{
			if (I2caRegs.I2CMDR.bit.STP == 1)
 				return I2C_STP_NOT_READY_ERROR;

			I2caRegs.I2CSAR.all = msg->SlaveAddress; 		//Set slave address
			if (I2caRegs.I2CSTR.bit.BB == 1)
				return I2C_BUS_BUSY_ERROR;
			else
			{
				I2caRegs.I2CCNT = 2;						// Setup how many bytes to send
				I2caRegs.I2CDXR.all = msg->MemoryHighAddr;
				I2caRegs.I2CDXR.all = msg->MemoryLowAddr;	// Configure fifo data for byte
				I2caRegs.I2CMDR.all = 0x2620;				// Send data to setup EEPROM address
			}
		}
//============================================================================
		case I2C_MSGSTAT_READ_BUSY1:
		{
			if(I2caRegs.I2CSTR.bit.ARDY == 0)
			{
				msg->MsgStatus=I2C_MSGSTAT_READ_BUSY1;
				return I2C_MSGSTAT_READ_BUSY;
			}
			else
			{
				// Check to make sure key value received is correct
				I2caRegs.I2CCNT = msg->NumOfBytes;					// Setup how many bytes to expect
				I2caRegs.I2CMDR.all = 0x2C20;						// Send start as master receiver			
			}
		}
//============================================================================			
		case I2C_MSGSTAT_READ_BUSY2:
		{	
			if(I2caRegs.I2CMDR.bit.STP == 1)
			{
				msg->MsgStatus=I2C_MSGSTAT_READ_BUSY2;
				return I2C_MSGSTAT_READ_BUSY;	
			}
			else
			{
				for(i=0;i<msg->NumOfBytes;i++)
		   			msg->MsgBuffer[i]= I2caRegs.I2CDRR.all;

				msg->MsgStatus=I2C_MSGSTAT_INACTIVE;
				return I2C_SUCCESS;
			}			
		}

//============================================================================
		default:
			return I2C_ERROR;
	}
}

//***************************************************************************
Uint16 UserI2c_Write(struct I2CMSG *msg)
{

	static mSecCNT_struct mSecCNT={0,0};
	if(User_usCount(&mSecCNT) < tWR)     	//写入完成后，延迟tWR才允许再次写(_zyb@20220618)
		return I2C_MSGSTAT_WRITE_BUSY4;

    if (I2caRegs.I2CMDR.bit.STP == 1)
        return I2C_STP_NOT_READY_ERROR;

    I2caRegs.I2CSAR.all = msg->SlaveAddress;
    if(I2caRegs.I2CSTR.bit.BB == 1)
        return I2C_BUS_BUSY_ERROR;

    I2caRegs.I2CCNT = msg->NumOfBytes + 2;
    I2caRegs.I2CDXR.all = msg->MemoryHighAddr;
    I2caRegs.I2CDXR.all = msg->MemoryLowAddr;
    I2caRegs.I2CDXR.all = msg->MsgBuffer[0];
    I2caRegs.I2CDXR.all = msg->MsgBuffer[1];
    I2caRegs.I2CMDR.all = 0x6E20;

    mSecCNT.reset=0;
    return I2C_SUCCESS;
}	

//===========================================================================
// No more.
//===========================================================================


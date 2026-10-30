//###########################################################################
#include "F28x_Project.h"
#include "User_Header.h"

//***************************************************************************
// These are defined by the linker file
 extern uint32_t Cla1ProgRunStart, Cla1ProgLoadStart, Cla1ProgLoadSize;

extern __interrupt void Cla1BackgroundTask(void);
extern __interrupt void Cla1Task1(void);

__interrupt void Cla1Task2(void){}
__interrupt void Cla1Task3(void){}
__interrupt void Cla1Task4(void){}
__interrupt void Cla1Task5(void){}
__interrupt void Cla1Task6(void){}
__interrupt void Cla1Task7(void){}
__interrupt void Cla1Task8(void){}

//***************************************************************************
// CLA ISRs
__interrupt void cla1Isr1( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr2( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr3( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr4( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr5( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr6( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr7( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

__interrupt void cla1Isr8( void)
{
	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
}

//***************************************************************************
void InitCLA(void)
{
// CLA_configClaMemory - Configure CLA memory sections
	EALLOW;

// Copy over code from FLASH to RAM
    memcpy((uint32_t *)&Cla1ProgRunStart, (uint32_t *)&Cla1ProgLoadStart,
           (uint32_t)&Cla1ProgLoadSize);

// Initialize and wait for CLA1ToCPUMsgRAM
	MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1;
	while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1){};

// Initialize and wait for CPUToCLA1MsgRAM
	MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1;
	while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1){};

// Next configure LS0RAM and LS1RAM as data spaces for the CLA
// First configure the CLA to be the master for LS0(1) and then
// set the spaces to be code blocks
	MemCfgRegs.LSxMSEL.bit.MSEL_LS0 = 1;
	MemCfgRegs.LSxMSEL.bit.MSEL_LS1 = 1;

	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS0 = 0;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS1 = 0;

// Select LS2RAM,LS3RAM to be the programming space for the CLA
// First configure the CLA to be the master for LS4 and LS5 and then
// set the space to be a program block
	MemCfgRegs.LSxMSEL.bit.MSEL_LS2 = 1;
	MemCfgRegs.LSxMSEL.bit.MSEL_LS3 = 1;

	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS2 = 1;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS3 = 1;

// Suppressing #770-D conversion from pointer to smaller integer
// The CLA address range is 16 bits so the addresses passed to the MVECT
// registers will be in the lower 64KW address space. Turn the warning
// back on after the MVECTs are assigned addresses

#pragma diag_suppress=770

	Cla1Regs.MVECT1 = (uint16_t)(&Cla1Task1);
	Cla1Regs.MVECT2 = (uint16_t)(&Cla1Task2);
	Cla1Regs.MVECT3 = (uint16_t)(&Cla1Task3);
	Cla1Regs.MVECT4 = (uint16_t)(&Cla1Task4);
	Cla1Regs.MVECT5 = (uint16_t)(&Cla1Task5);
	Cla1Regs.MVECT6 = (uint16_t)(&Cla1Task6);
	Cla1Regs.MVECT7 = (uint16_t)(&Cla1Task7);
    Cla1Regs.MVECT8 = (uint16_t)(&Cla1Task8);

    //配置PWM1_INT以触发CLA
    EPwm1Regs.ETSEL.bit.INTSEL = ET_CTR_ZERO;   // INT on Zero event
    EPwm1Regs.ETPS.bit.INTPRD = ET_1ST;     	// Generate INT on 1st event
    EPwm1Regs.ETCLR.bit.INT = 1;				// Clear PWM_INT
    EPwm1Regs.ETSEL.bit.INTEN = 1;  			// Enable INT

	DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK1 = CLA_TRIG_EPWM1INT;

	Cla1Regs._MVECTBGRND = (uint16_t)&Cla1BackgroundTask;	//使能CLA后台任务(_zyb@20230901)
	Cla1Regs._MCTLBGRND.bit.TRIGEN = 0U;
	Cla1Regs._MCTLBGRND.bit.BGEN = 1U;
	Cla1Regs._MCTLBGRND.bit.BGSTART = 1U;	//启动后台任务

#pragma diag_suppress=770

// Enable the IACK instruction to start a task on CLA in software
// for all  8 CLA tasks. Also, globally enable all 8 tasks (or a
// subset of tasks) by writing to their respective bits in the MIER register
	Cla1Regs.MCTL.bit.IACKE = 1;
	Cla1Regs.MIER.all = 0x00FF;

// Configure the vectors for the end-of-task interrupt for all 8 tasks
	PieVectTable.CLA1_1_INT = &cla1Isr1;
	PieVectTable.CLA1_2_INT = &cla1Isr2;
	PieVectTable.CLA1_3_INT = &cla1Isr3;
	PieVectTable.CLA1_4_INT = &cla1Isr4;
	PieVectTable.CLA1_5_INT = &cla1Isr5;
	PieVectTable.CLA1_6_INT = &cla1Isr6;
	PieVectTable.CLA1_7_INT = &cla1Isr7;
	PieVectTable.CLA1_8_INT = &cla1Isr8;

// Enable CLA interrupts at the group and subgroup levels
	PieCtrlRegs.PIEIER11.all = 0xFFFF;
	IER |= M_INT11 ;

    EDIS;
}

//===========================================================================
// No more.
//===========================================================================

//Cla1ForceTask1();

//__mdebugstop();

//for(i=0;i<6;i++);		//延迟1us=20周期*6

//###########################################################################
#include "bsp.h"
//***************************************************************************
extern uint32_t Cla1funcsRunStart, Cla1funcsLoadStart, Cla1funcsLoadSize;

__interrupt void Cla1Task1(void);
//__interrupt void Cla1Task2(void);
//__interrupt void Cla1Task3(void);
//__interrupt void Cla1Task4(void);
//__interrupt void Cla1Task5(void);
//__interrupt void Cla1Task6(void);
//__interrupt void Cla1Task7(void);
//__interrupt void Cla1Task8(void);

//***************************************************************************
// CLA ISRs
//__interrupt void cla1_task1_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task2_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task3_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task4_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task5_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task6_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task7_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}
//
//__interrupt void cla1_task8_isr( void)
//{
//	PieCtrlRegs.PIEACK.bit.ACK11 = 1;
//}

//***************************************************************************
void CLA_Init(void)
{
    EALLOW;
   /*  Assign user defined ISR to the PIE vector table */
//    PieVectTable.CLA1_1_INT = &cla1_task1_isr;
//    PieVectTable.CLA1_2_INT = &cla1_task2_isr;
//    PieVectTable.CLA1_3_INT = &cla1_task3_isr;
//    PieVectTable.CLA1_4_INT = &cla1_task4_isr;
//    PieVectTable.CLA1_5_INT = &cla1_task5_isr;
//    PieVectTable.CLA1_6_INT = &cla1_task6_isr;
//    PieVectTable.CLA1_7_INT = &cla1_task7_isr;
//    PieVectTable.CLA1_8_INT = &cla1_task8_isr;

    // the memory to the CLA and when an interrupt comes in
    // Call this function even if Load and Run address is the same!
    memcpy((uint32_t *)&Cla1funcsRunStart, (uint32_t *)&Cla1funcsLoadStart,(uint32_t)&Cla1funcsLoadSize);

   /*  Compute all CLA task vectors */ 
    Cla1Regs.MVECT1 = (uint16_t)(&Cla1Task1);
//    Cla1Regs.MVECT2 = (uint16_t)(&Cla1Task2);
//    Cla1Regs.MVECT3 = (uint16_t)(&Cla1Task3);
//    Cla1Regs.MVECT4 = (uint16_t)(&Cla1Task4);
//    Cla1Regs.MVECT5 = (uint16_t)(&Cla1Task5);
//    Cla1Regs.MVECT6 = (uint16_t)(&Cla1Task6);
//    Cla1Regs.MVECT7 = (uint16_t)(&Cla1Task7);
//    Cla1Regs.MVECT8 = (uint16_t)(&Cla1Task8);
               
    //  Step 3 : Mapping CLA tasks
    //
    // Enable the IACK instruction to start a task on CLA in software
    // for all  8 CLA tasks. Also, globally enable all 8 tasks (or a
    // subset of tasks) by writing to their respective bits in the
    // MIER register
    //
    Cla1Regs.MCTL.bit.IACKE = 1;
    Cla1Regs.MIER.all = 0x00FF;

//    ////≈‰÷√CLA_TRIG_ADCAINT1“‘¥•∑¢CLA
    EPwm11Regs.ETSEL.bit.INTSEL = ET_CTR_PRDZERO;   // INT on Zero event
    EPwm11Regs.ETPS.bit.INTPRD = ET_1ST;            // Generate INT on 1st event
    EPwm11Regs.ETCLR.bit.INT = 1;                   // Clear PWM_INT
    EPwm11Regs.ETSEL.bit.INTEN = 1;                 // Enable INT
    DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK1 = CLA_TRIG_EPWM11INT; //

	// Initialize and wait for CLA1ToCPUMsgRAM
	//
	MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1;
	while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1){};

	//
	// Initialize and wait for CPUToCla1ToCpuMsgRAM
	//
	MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1;
	while(MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1){};

	//
	// Select LS2RAM,LS3RAM,LS4RAM and LS5RAM to be the programming space for the CLA
	// First configure the CLA to be the master for LS4 and LS5 and then
	// set the space to be a program block
	//
	MemCfgRegs.LSxMSEL.bit.MSEL_LS0 = 1;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS0 = 1;
//	MemCfgRegs.LSxMSEL.bit.MSEL_LS3 = 1;
//	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS3 = 1;
//	MemCfgRegs.LSxMSEL.bit.MSEL_LS4 = 1;
//	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS4 = 1;
	MemCfgRegs.LSxMSEL.bit.MSEL_LS1 = 1;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS1 = 1;

	//
	// Next configure LS0RAM and LS1RAM as data spaces for the CLA
	// First configure the CLA to be the master for LS0(1) and then
	// set the spaces to be code blocks
	//
	MemCfgRegs.LSxMSEL.bit.MSEL_LS4 = 1;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS4 = 0;

	MemCfgRegs.LSxMSEL.bit.MSEL_LS5 = 1;
	MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS5 = 0;

//    /*  Enable CLA interrupts at the group and subgroup levels */
    PieVectTable.EPWM11_INT = &ISREpwmcLA;
    PieCtrlRegs.PIEIER3.bit.INTx11 = 1;
	PieCtrlRegs.PIEIER11.all       = 0xFFFF;

    EDIS;
}


//===========================================================================
// No more.
//===========================================================================



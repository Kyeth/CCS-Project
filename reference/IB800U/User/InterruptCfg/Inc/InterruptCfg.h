#ifndef __INTERRUPTCFG_H
#define __INTERRUPTCFG_H

#ifdef __cplusplus
    extern "C" {
#endif

#define ISR_TIMER_CHK_EN        (0) //(1)

extern __interrupt void TaskCla_1_Isr(void);
extern void InterruptVectTableInit(void);
extern void TaskComm(void);

#ifdef __cplusplus
    }
#endif

#endif

#ifndef __USER_CAN_H
#define __USER_CAN_H

extern unsigned int u16OscSetEnable;
extern volatile unsigned int u16OscChannel[12];
extern volatile unsigned int u16OscChannel_Multiple[12];

extern void TaskOscParamProcess(void);

#endif

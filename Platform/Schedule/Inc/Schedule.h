/**
 * @file Schedule.h
 * @brief Cooperative background task scheduler driven by the 1 ms heartbeat.
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "F2837xS_device.h"

void Task_Init(void);
Uint16 Task_Start(void (*callback)(void), Uint16 periodMs, Uint16 offsetMs);
void Task_OnTick(void);
void Task_Run(void);

#endif /* SCHEDULE_H */

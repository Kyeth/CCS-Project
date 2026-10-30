#ifndef __EQUIPMENTPARALLELTRAMSPORT_H
#define __EQUIPMENTPARALLELTRAMSPORT_H

#include "EquipmentParallelCore.h"

#ifdef __cplusplus
    extern "C" {
#endif


extern int16 ParallelReadTramsport(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize);


extern int16 ParallelWriteTramsport(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16WriteSize);


extern int16 ClearBusError(void);

#ifdef __cplusplus
    }
#endif

#endif

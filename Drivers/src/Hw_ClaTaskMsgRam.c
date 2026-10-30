#include "Hw_ClaTask.h"

/* Definitions belong to a CPU translation unit for CLA data model compatibility. */
#pragma DATA_SECTION(Cla1ToCpuMsg, "Cla1ToCpuMsgRAM")
#pragma DATA_SECTION(CpuToCla1Msg, "CpuToCla1MsgRAM")
volatile ST_CLA_TO_CPU_MIN Cla1ToCpuMsg;
volatile ST_CPU_TO_CLA_MIN CpuToCla1Msg;

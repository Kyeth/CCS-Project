#include "ParallelCfg.h"
#include "EquipmentParallelCore.h"
#include "stddef.h"

#ifdef __cplusplus
    extern "C" {
#endif

// ID数据拆分：低8位为节点ID 中12位是为帧序号 高9位为帧类型
#define HEART       0x200
#define DATA        0x210

ST_BATTERY_ITEM stBatteryItem[] =
{
 // 帧类型             帧序号   帧有效  数据长度     对象个数    对象属性
     {HEARTBEAT_HEAD,   HEART,      VAILD,  8,          2,      {offsetof(ST_BATTERY_DATA, u32Version), PARALLEL_TYPE_UINT32}, {offsetof(ST_BATTERY_DATA, stBroadcastData_1) , PARALLEL_TYPE_UINT32} },
     {DATA_HEAD,        DATA,       VAILD,  8,          2,      {offsetof(ST_BATTERY_DATA, stBroadcastCtrl_1) , PARALLEL_TYPE_UINT32}, {offsetof(ST_BATTERY_DATA, stSpecifiedCtrl_3) , PARALLEL_TYPE_UINT32} },
     {DATA_HEAD,        DATA+1,     VAILD,  4,          1,      {offsetof(ST_BATTERY_DATA, stBroadcastData_2) , PARALLEL_TYPE_UINT32} },
};

ST_BATTERY_CFG stBatteryCfg = {stBatteryItem, (sizeof(stBatteryItem)/sizeof(stBatteryItem[0]))};

#ifdef __cplusplus
    }
#endif

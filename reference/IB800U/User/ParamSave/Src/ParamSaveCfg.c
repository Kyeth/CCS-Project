#include "ParamSaveCfg.h"
#include "PlatformManager.h"

#ifdef __cplusplus
    extern "C" {
#endif

ST_PARAM_ITEM stParamItem[] =
{
     // 索引号   是否保存标志      数据类型             保留       参数地址
     {0,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.VBusSampCoefficient},
     {1,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.VAirFrontSampCoefficient},
     {2,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.VAirBackSampCoefficient},
     {3,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.VBatSampCoefficient},
     {4,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.IBusSampCoefficient},
     {5,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.IBranch_1SampCoefficient},
     {6,        1,      PARAM_TYPE_FLOAT,       0,       (U_PARAM_VAL*)&ObjSample.IBranch_2SampCoefficient},
};

ST_PARAM_CFG stParamCfg = {stParamItem, sizeof(stParamItem)/sizeof(stParamItem[0])};

#ifdef __cplusplus
    }
#endif

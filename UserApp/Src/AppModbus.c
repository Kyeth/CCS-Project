/**
 * @file AppModbus.c
 * @brief 应用层 Modbus 点表（类型化绑定）与协议实例配置表。
 */

#include "AppModbus.h"
#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "DrvCfg_EP15K.h"
#include "Hw_SciDriver.h"

/* 业务变量：每路一套，供点表绑定与调试观察。
 * 类型保持不变（Uint16/int16/Uint32/int32/float），不做统一强转。 */
static Uint16 s_au16TestMode[APP_MB_PORTS];      /* 0x0000，读写 */
static int16 s_ai16TestOffset[APP_MB_PORTS];    /* 0x0001，读写 */
static Uint32 s_au32TestCounter[APP_MB_PORTS];   /* 0x0003..0x0004，读写 */
static int32 s_ai32TestTrim[APP_MB_PORTS];      /* 0x0005..0x0006，读写 */
static float  s_afTestGain[APP_MB_PORTS];        /* 0x0007..0x0008，读写 */
static Uint16 s_au16TestStatus[APP_MB_PORTS];    /* 0x0009，只读 */

/* 一张点表：地址严格递增、32 位项占两个寄存器（高字在前）。 */
#define APP_MB_POINT_TBL(i)                                                                    \
    {                                                                                          \
        { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, (void *)&s_au16TestMode[i],                 \
          0UL, 1000UL, 0.0f, 0.0f },                                                           \
        { 0x0001U, MB_PT_I16, MB_PT_READWRITE, 0U, (void *)&s_ai16TestOffset[i],               \
          (Uint32)(-500L), 500UL, 0.0f, 0.0f },                                                \
        { 0x0003U, MB_PT_U32, MB_PT_READWRITE, 0U, (void *)&s_au32TestCounter[i],              \
          0UL, 1000000UL, 0.0f, 0.0f },                                                        \
        { 0x0005U, MB_PT_I32, MB_PT_READWRITE, 0U, (void *)&s_ai32TestTrim[i],                 \
          (Uint32)(-100000L), 100000UL, 0.0f, 0.0f },                                          \
        { 0x0007U, MB_PT_F32, MB_PT_READWRITE, 0U, (void *)&s_afTestGain[i],                   \
          0UL, 0UL, -50.0f, 150.0f },                                                          \
        { 0x0009U, MB_PT_U16, MB_PT_READ, 0U, (void *)&s_au16TestStatus[i],                    \
          0UL, 65535UL, 0.0f, 0.0f }                                                           \
    }

static const ST_MODBUS_POINT s_astPointTblA[] = APP_MB_POINT_TBL(0);
static const ST_MODBUS_POINT s_astPointTblC[] = APP_MB_POINT_TBL(1);
static const ST_MODBUS_POINT s_astPointTblD[] = APP_MB_POINT_TBL(2);

#define APP_MB_COUNT(tbl)  ((Uint16)(sizeof(tbl) / sizeof((tbl)[0])))

static const ST_MODBUS_POINT *const s_apstPointTbl[APP_MB_PORTS] = {
    s_astPointTblA, s_astPointTblC, s_astPointTblD
};
static const Uint16 s_au16PointCount[APP_MB_PORTS] = {
    APP_MB_COUNT(s_astPointTblA), APP_MB_COUNT(s_astPointTblC), APP_MB_COUNT(s_astPointTblD)
};
static const Uint16 s_au16SciId[APP_MB_PORTS] = {
    (Uint16)HW_SCI_ID_A, (Uint16)HW_SCI_ID_C, (Uint16)HW_SCI_ID_D
};
static const Uint16 s_au16SlaveAddr[APP_MB_PORTS] = { 1U, 1U, 2U };

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_astProtoCfg, ".protobuf")
#endif
static ST_PROTO_INSTANCE s_astProtoCfg[APP_MB_PORTS];

static Uint16 s_u16Enabled = 0U;
static Uint32 s_u32TableErrors = 0UL;

static Uint32 AppMbBaudOf(Uint16 u16SciId);

void AppModbusInit(void)
{
    Uint16 u16Index;

    ProtocolInit();
    ModbusProtoClear();

    s_u16Enabled = 0U;
    s_u32TableErrors = 0UL;

    if (ProtocolRegisterOps(&stModbusProtoOps) == 0U) {
        return;   /* 没有可用的 Modbus 实现：三路都不启用 */
    }

    for (u16Index = 0U; u16Index < APP_MB_PORTS; u16Index++) {
        ST_PROTO_INSTANCE *pstCfg = &s_astProtoCfg[u16Index];
        const ST_MODBUS_POINT *pstTbl = s_apstPointTbl[u16Index];
        Uint16 u16TblCount = s_au16PointCount[u16Index];

        pstCfg->u16SciId = s_au16SciId[u16Index];
        pstCfg->u16Type = (Uint16)PROTO_TYPE_MODBUS;
        pstCfg->u16SlaveAddr = s_au16SlaveAddr[u16Index];
        pstCfg->u16Enabled = 1U;
        pstCfg->u16TblCount = u16TblCount;
        pstCfg->u32Baud = AppMbBaudOf(pstCfg->u16SciId);
        pstCfg->pstTbl = pstTbl;
        pstCfg->pCtx = ModbusProtoState(u16Index);

        if (ModbusTblValidate(pstTbl, u16TblCount) == 0U) {
            s_u32TableErrors++;   /* 配置错误在初始化期暴露，不留到收报文时 */
            pstCfg->u16Enabled = 0U;
            continue;
        }

        if (ProtocolAddInstance(pstCfg) != 0U) {
            s_u16Enabled++;
        }
    }
}

Uint16 AppModbusEnabledCount(void)
{
    return s_u16Enabled;
}

Uint32 AppModbusTableErrors(void)
{
    return s_u32TableErrors;
}

const ST_PROTO_INSTANCE *AppModbusInstance(Uint16 u16Index)
{
    if (u16Index >= APP_MB_PORTS) {
        return 0;
    }
    return &s_astProtoCfg[u16Index];
}

static Uint32 AppMbBaudOf(Uint16 u16SciId)
{
    const ST_SCI_CFG_ITEM *pstItem = stSciCfg.pstSciCfgItem;
    Uint16 u16Index;

    if (pstItem == 0) {
        return 0UL;
    }

    for (u16Index = 0U; u16Index < stSciCfg.u16ItemNum; u16Index++) {
        if (pstItem[u16Index].u16SciId == u16SciId) {
            return pstItem[u16Index].u32Baudrate;
        }
    }

    return 0UL;
}

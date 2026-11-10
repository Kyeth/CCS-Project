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
/* 点表基址 5000：主站可以从 5000 连续读到 5036（37 个寄存器）。 */
static Uint16 s_au16TestMode[APP_MB_PORTS];      /* 5000，读写 */
static int16 s_ai16TestOffset[APP_MB_PORTS];    /* 5001，读写 */
static Uint32 s_au32TestCounter[APP_MB_PORTS];   /* 5003..5004，读写 */
static int32 s_ai32TestTrim[APP_MB_PORTS];      /* 5005..5006，读写 */
static float  s_afTestGain[APP_MB_PORTS];        /* 5007..5008，读写 */
static Uint16 s_au16TestStatus[APP_MB_PORTS];    /* 5009，只读 */

/* 5010..5034 未定义：用只读零占位，保证 5000..5036 这段连续可读。 */
static const Uint16 s_u16Zero = 0U;
static const Uint16 s_au16Reserved0[25] = { 0U };

/* 5035/5036：版本寄存器（只读）。 */
/* 5035/5036：项目版本 503900 = 0x0007B05C，按高字在前占两个寄存器。 */
static const Uint16 s_u16AppVersion = 0x0007U;   /* 5035：高字 */
static const Uint16 s_u16AppBuild   = 0xB05CU;   /* 5036：低字 */

/* ARM 升级握手回复：01 10 04 38 00 02 C1 35（CRC 已按该请求算好）。 */
static const Uint16 s_au16ArmReply[8] = {
    0x01U, 0x10U, 0x04U, 0x38U, 0x00U, 0x02U, 0xC1U, 0x35U
};
#define APP_MB_ARM_REPLY_LEN   (8U)
#define APP_MB_ARM_SCI_ID      (HW_SCI_ID_A)   /* 暂定 SCI-A 作为升级端口，待确认 */

/* 一张点表：地址严格递增、32 位项占两个寄存器（高字在前）。 */
#define APP_MB_VER_POINT(addr, pvar)                                                           \
        { (addr), MB_PT_U16, MB_PT_READ, 0U, (void *)(pvar), 0UL, 65535UL, 0.0f, 0.0f }

#define APP_MB_ZERO_POINT(addr)                                                                \
        { (addr), MB_PT_U16, MB_PT_READ, 0U, (void *)&s_u16Zero, 0UL, 65535UL, 0.0f, 0.0f }

#define APP_MB_RESERVED_POINT(idx)                                                             \
        { (Uint16)(5010U + (idx)), MB_PT_U16, MB_PT_READ, 0U,                                   \
          (void *)&s_au16Reserved0[idx], 0UL, 65535UL, 0.0f, 0.0f }

#define APP_MB_POINT_TBL(i)                                                                    \
    {                                                                                          \
        { 5000U, MB_PT_U16, MB_PT_READWRITE, 0U, (void *)&s_au16TestMode[i],                   \
          0UL, 1000UL, 0.0f, 0.0f },                                                           \
        { 5001U, MB_PT_I16, MB_PT_READWRITE, 0U, (void *)&s_ai16TestOffset[i],                 \
          (Uint32)(-500L), 500UL, 0.0f, 0.0f },                                                \
        APP_MB_ZERO_POINT(5002U),                                                              \
        { 5003U, MB_PT_U32, MB_PT_READWRITE, 0U, (void *)&s_au32TestCounter[i],                \
          0UL, 1000000UL, 0.0f, 0.0f },                                                        \
        { 5005U, MB_PT_I32, MB_PT_READWRITE, 0U, (void *)&s_ai32TestTrim[i],                   \
          (Uint32)(-100000L), 100000UL, 0.0f, 0.0f },                                          \
        { 5007U, MB_PT_F32, MB_PT_READWRITE, 0U, (void *)&s_afTestGain[i],                     \
          0UL, 0UL, -50.0f, 150.0f },                                                          \
        { 5009U, MB_PT_U16, MB_PT_READ, 0U, (void *)&s_au16TestStatus[i],                      \
          0UL, 65535UL, 0.0f, 0.0f },                                                          \
        APP_MB_RESERVED_POINT(0),  APP_MB_RESERVED_POINT(1),  APP_MB_RESERVED_POINT(2),        \
        APP_MB_RESERVED_POINT(3),  APP_MB_RESERVED_POINT(4),  APP_MB_RESERVED_POINT(5),        \
        APP_MB_RESERVED_POINT(6),  APP_MB_RESERVED_POINT(7),  APP_MB_RESERVED_POINT(8),        \
        APP_MB_RESERVED_POINT(9),  APP_MB_RESERVED_POINT(10), APP_MB_RESERVED_POINT(11),       \
        APP_MB_RESERVED_POINT(12), APP_MB_RESERVED_POINT(13), APP_MB_RESERVED_POINT(14),       \
        APP_MB_RESERVED_POINT(15), APP_MB_RESERVED_POINT(16), APP_MB_RESERVED_POINT(17),       \
        APP_MB_RESERVED_POINT(18), APP_MB_RESERVED_POINT(19), APP_MB_RESERVED_POINT(20),       \
        APP_MB_RESERVED_POINT(21), APP_MB_RESERVED_POINT(22), APP_MB_RESERVED_POINT(23),       \
        APP_MB_RESERVED_POINT(24),                                                             \
        APP_MB_VER_POINT(5035U, &s_u16AppVersion),                                             \
        APP_MB_VER_POINT(5036U, &s_u16AppBuild)                                                \
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

/**
 * @brief ARM 升级握手入口。
 *
 * 命中 6 字节起始模式后由协议层调用；返回握手回复长度即表示接管端口。
 * 握手内容已确定：请求 01 10 04 38 00 02，回复 01 10 04 38 00 02 C1 35。
 * 交接时机（握手全部入环、硬件发完）由协议层负责。
 */
static Uint16 AppMbArmHook(void *pCtx, const Uint16 *pu16Prefix, Uint16 u16PrefixLen,
                           Uint16 *pu16Reply, Uint16 u16ReplyMax)
{
    Uint16 u16Idx;

    (void)pCtx;
    (void)pu16Prefix;   /* 前缀已由协议层逐字节匹配，这里只回握手 */
    if (pu16Reply == (Uint16 *)0) {
        return 0U;
    }
    if ((u16PrefixLen != MB_ARM_PREFIX_LEN) || (u16ReplyMax < APP_MB_ARM_REPLY_LEN)) {
        return 0U;
    }
    for (u16Idx = 0U; u16Idx < APP_MB_ARM_REPLY_LEN; u16Idx++) {
        pu16Reply[u16Idx] = s_au16ArmReply[u16Idx];
    }
    return APP_MB_ARM_REPLY_LEN;
}

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

    /* ARM 升级入口：候选帧起点匹配 01 10 04 38 00 02 时交给升级流程。 */
    ModbusProtoSetArmHook(APP_MB_ARM_SCI_ID, (void *)0, AppMbArmHook);

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

/**
 * @file AppModbus.c
 * @brief 应用层 Modbus 点表与升级回调（参照 ST_REG_SLAVE_ITEM 的写法）。
 *
 * 点表按寄存器地址升序排列，每一项登记：地址、数据类型、读写权限、保存属性、
 * 绑定的变量指针、整数上下限与浮点上下限。类型转换、完整性检查、多寄存器写的
 * 原子提交都由平台的点表模块完成，应用层只维护"地址与变量的对应关系"。
 *
 * 升级入口沿用参考实现：1080（固件类型，0..1）与 1081（升级波特率，0..66530）
 * 两个可写寄存器；收到"写 2 个寄存器到 1080"的 0x10 命令时，回调把升级请求
 * 标志置 1。标准 0x10 回声作为握手回复，回声发完再由协议层交接端口。
 *
 * 与参考实现的字段对应：
 *   ST_REG_SLAVE_ITEM              ST_MODBUS_POINT
 *   ------------------------------  ----------------------------------------
 *   寄存器地址                       u16Addr
 *   READ_WRITE / READ_ONLY          MB_PT_READWRITE / MB_PT_READ / MB_PT_WRITE
 *   TYPE_UINT16 / TYPE_INT16        MB_PT_U16 / MB_PT_I16（另支持 U32/I32/F32）
 *   NOT_SAVE                        MB_PT_NOT_SAVE
 *   (U_REG_VAL *)(&变量)             (void *)(&变量)  类型化访问，保留 volatile
 *   值下限 / 值上限                  u32Min / u32Max（浮点用 fMin / fMax）
 */

#include "AppModbus.h"
#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "DrvCfg_EP15K.h"
#include "Hw_SciDriver.h"

/* 点表项数：等价于参考实现的 MDS_ITEM_OF(arr)。 */
#define MDS_ITEM_OF(arr)        ((Uint16)(sizeof(arr) / sizeof((arr)[0])))

/* 升级地址与数量：与参考实现一致（1080 固件类型、1081 升级波特率）。 */
#define APP_MB_UPGRADE_ADDR     (1080U)
#define APP_MB_UPGRADE_NUMS     (2U)
#define APP_MB_FUNC_WRITE_MULTI (0x10U)
#define APP_MB_ARM_PORT_INDEX   (0U)          /* 升级入口先挂在 SCI-A，待确认 */

/* 升级参数对象：对应参考实现的 stMdsUpdateParam。 */
typedef struct {
    Uint16 u16FirmwareType;   /* 1080：固件类型，0..1 */
    Uint16 u16UpdateBaud;     /* 1081：升级波特率，0..66530 */
    Uint16 u16MdsIsUpdate;    /* 1 = 已收到升级请求 */
} ST_MB_UPGRADE_PARAM;

static ST_MB_UPGRADE_PARAM s_astUpgradeParam[APP_MB_PORTS];

/* 业务变量：每路一套，供点表绑定与调试观察。
 * 类型保持不变（Uint16/int16/Uint32/int32/float），不做统一强转。 */
static Uint16 s_au16TestMode[APP_MB_PORTS];      /* 5000，读写 */
static int16  s_ai16TestOffset[APP_MB_PORTS];    /* 5001，读写 */
static Uint32 s_au32TestCounter[APP_MB_PORTS];   /* 5003..5004，读写 */
static int32  s_ai32TestTrim[APP_MB_PORTS];      /* 5005..5006，读写 */
static float  s_afTestGain[APP_MB_PORTS];        /* 5007..5008，读写 */
static Uint16 s_au16TestStatus[APP_MB_PORTS];    /* 5009，只读 */

/* 5010..5034 未定义：用只读零占位，保证 5000..5036 这段连续可读。 */
static const Uint16 s_u16Zero = 0U;
static const Uint16 s_au16Reserved0[25] = { 0U };

/* 5035/5036：项目版本 503900 = 0x0007B05C，按高字在前占两个寄存器。 */
static const Uint16 s_u16AppVersion = 0x0007U;   /* 5035：高字 */
static const Uint16 s_u16AppBuild   = 0xB05CU;   /* 5036：低字 */

#define APP_MB_POINT(addr, type, access, pvar, umin, umax, fmin, fmax)            \
        { (addr), (type), (access), MB_PT_NOT_SAVE,                                \
          (void *)(pvar), (umin), (umax), (fmin), (fmax) }

#define APP_MB_VER_POINT(addr, pvar)                                              \
        APP_MB_POINT((addr), MB_PT_U16, MB_PT_READ, (pvar), 0UL, 65535UL, 0.0f, 0.0f)

#define APP_MB_ZERO_POINT(addr)                                                   \
        APP_MB_POINT((addr), MB_PT_U16, MB_PT_READ, &s_u16Zero, 0UL, 65535UL, 0.0f, 0.0f)

#define APP_MB_RESERVED_POINT(idx)                                                \
        APP_MB_POINT((Uint16)(5010U + (idx)), MB_PT_U16, MB_PT_READ,              \
                     &s_au16Reserved0[idx], 0UL, 65535UL, 0.0f, 0.0f)

/* 一张点表：地址严格升序、32 位项占两个寄存器（高字在前）。
 * 升级寄存器排在前面（1080/1081），随后是 5000..5036 的测试与版本区。 */
#define APP_MB_POINT_TBL(i)                                                       \
    {                                                                             \
        APP_MB_POINT(APP_MB_UPGRADE_ADDR, MB_PT_U16, MB_PT_READWRITE,             \
                     &s_astUpgradeParam[i].u16FirmwareType, 0UL, 1UL, 0.0f, 0.0f), \
        APP_MB_POINT((Uint16)(APP_MB_UPGRADE_ADDR + 1U), MB_PT_U16,               \
                     MB_PT_READWRITE, &s_astUpgradeParam[i].u16UpdateBaud,         \
                     0UL, 66530UL, 0.0f, 0.0f),                                   \
        APP_MB_POINT(5000U, MB_PT_U16, MB_PT_READWRITE,                           \
                     &s_au16TestMode[i], 0UL, 1000UL, 0.0f, 0.0f),                 \
        APP_MB_POINT(5001U, MB_PT_I16, MB_PT_READWRITE,                           \
                     &s_ai16TestOffset[i], (Uint32)(-500L), 500UL, 0.0f, 0.0f),    \
        APP_MB_ZERO_POINT(5002U),                                                 \
        APP_MB_POINT(5003U, MB_PT_U32, MB_PT_READWRITE,                           \
                     &s_au32TestCounter[i], 0UL, 1000000UL, 0.0f, 0.0f),           \
        APP_MB_POINT(5005U, MB_PT_I32, MB_PT_READWRITE,                           \
                     &s_ai32TestTrim[i], (Uint32)(-100000L), 100000UL, 0.0f, 0.0f), \
        APP_MB_POINT(5007U, MB_PT_F32, MB_PT_READWRITE,                           \
                     &s_afTestGain[i], 0UL, 0UL, -50.0f, 150.0f),                  \
        APP_MB_POINT(5009U, MB_PT_U16, MB_PT_READ,                                \
                     &s_au16TestStatus[i], 0UL, 65535UL, 0.0f, 0.0f),              \
        APP_MB_RESERVED_POINT(0),  APP_MB_RESERVED_POINT(1),                      \
        APP_MB_RESERVED_POINT(2),  APP_MB_RESERVED_POINT(3),                      \
        APP_MB_RESERVED_POINT(4),  APP_MB_RESERVED_POINT(5),                      \
        APP_MB_RESERVED_POINT(6),  APP_MB_RESERVED_POINT(7),                      \
        APP_MB_RESERVED_POINT(8),  APP_MB_RESERVED_POINT(9),                      \
        APP_MB_RESERVED_POINT(10), APP_MB_RESERVED_POINT(11),                     \
        APP_MB_RESERVED_POINT(12), APP_MB_RESERVED_POINT(13),                     \
        APP_MB_RESERVED_POINT(14), APP_MB_RESERVED_POINT(15),                     \
        APP_MB_RESERVED_POINT(16), APP_MB_RESERVED_POINT(17),                     \
        APP_MB_RESERVED_POINT(18), APP_MB_RESERVED_POINT(19),                     \
        APP_MB_RESERVED_POINT(20), APP_MB_RESERVED_POINT(21),                     \
        APP_MB_RESERVED_POINT(22), APP_MB_RESERVED_POINT(23),                     \
        APP_MB_RESERVED_POINT(24),                                                \
        APP_MB_VER_POINT(5035U, &s_u16AppVersion),                                \
        APP_MB_VER_POINT(5036U, &s_u16AppBuild)                                   \
    }

static const ST_MODBUS_POINT s_astPointTblA[] = APP_MB_POINT_TBL(0);
static const ST_MODBUS_POINT s_astPointTblC[] = APP_MB_POINT_TBL(1);
static const ST_MODBUS_POINT s_astPointTblD[] = APP_MB_POINT_TBL(2);

static const ST_MODBUS_POINT *const s_apstPointTbl[APP_MB_PORTS] = {
    s_astPointTblA, s_astPointTblC, s_astPointTblD
};
static const Uint16 s_au16PointCount[APP_MB_PORTS] = {
    MDS_ITEM_OF(s_astPointTblA), MDS_ITEM_OF(s_astPointTblC), MDS_ITEM_OF(s_astPointTblD)
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
 * @brief 升级回调：对应参考实现的 ModbusSlaveCallBack_1 / _2。
 *
 * 触发条件与参考实现一致：本机地址或广播 && 起始地址 1080 && 寄存器数 2 &&
 * 功能码 0x10。命中后置升级请求标志，返回 8 字节标准 0x10 回声作为握手回复
 * （01 10 04 38 00 02 C1 35）；未命中返回 0，字节交回普通 Modbus。
 */
static Uint16 AppModbusCallBack(Uint16 u16Port, const Uint16 *pu16Head,
                                Uint16 *pu16Reply, Uint16 u16ReplyMax)
{
    const ST_PROTO_INSTANCE *pstCfg;
    Uint16 u16Addr;
    Uint16 u16Func;
    Uint16 u16Start;
    Uint16 u16Nums;
    Uint16 u16Crc;
    Uint16 u16Idx;

    if ((u16Port >= APP_MB_PORTS) || (pu16Head == (const Uint16 *)0) ||
        (pu16Reply == (Uint16 *)0) || (u16ReplyMax < 8U)) {
        return 0U;
    }

    pstCfg = &s_astProtoCfg[u16Port];

    u16Addr  = (Uint16)(pu16Head[0] & 0xFFU);
    u16Func  = (Uint16)(pu16Head[1] & 0xFFU);
    u16Start = (Uint16)(((pu16Head[2] & 0xFFU) << 8) | (pu16Head[3] & 0xFFU));
    u16Nums  = (Uint16)(((pu16Head[4] & 0xFFU) << 8) | (pu16Head[5] & 0xFFU));

    if (((u16Addr == pstCfg->u16SlaveAddr) || (u16Addr == 0U)) &&
        (u16Start == APP_MB_UPGRADE_ADDR) &&
        (u16Nums == APP_MB_UPGRADE_NUMS) &&
        (u16Func == APP_MB_FUNC_WRITE_MULTI)) {
        s_astUpgradeParam[u16Port].u16MdsIsUpdate = 1U;

        for (u16Idx = 0U; u16Idx < 6U; u16Idx++) {
            pu16Reply[u16Idx] = (Uint16)(pu16Head[u16Idx] & 0xFFU);
        }
        u16Crc = ModbusCrc16(pu16Reply, 6U);
        pu16Reply[6] = (Uint16)(u16Crc & 0xFFU);
        pu16Reply[7] = (Uint16)((u16Crc >> 8) & 0xFFU);
        return 8U;
    }

    return 0U;
}

/**
 * @brief 协议层的 ARM 入口：把端口号从上下文取出后交给升级回调。
 */
static Uint16 AppMbArmHook(void *pCtx, const Uint16 *pu16Prefix, Uint16 u16PrefixLen,
                           Uint16 *pu16Reply, Uint16 u16ReplyMax)
{
    Uint16 u16Port = (Uint16)(Uint32)pCtx;

    if (u16PrefixLen < MB_ARM_PREFIX_LEN) {
        return 0U;
    }
    return AppModbusCallBack(u16Port, pu16Prefix, pu16Reply, u16ReplyMax);
}

void AppModbusInit(void)
{
    Uint16 u16Index;

    ProtocolInit();
    ModbusProtoClear();

    s_u16Enabled = 0U;
    s_u32TableErrors = 0UL;

    for (u16Index = 0U; u16Index < APP_MB_PORTS; u16Index++) {
        s_astUpgradeParam[u16Index].u16FirmwareType = 0U;
        s_astUpgradeParam[u16Index].u16UpdateBaud = 0U;   /* 0 = 沿用当前端口波特率 */
        s_astUpgradeParam[u16Index].u16MdsIsUpdate = 0U;
    }

    if (ProtocolRegisterOps(&stModbusProtoOps) == 0U) {
        return;   /* 没有可用的 Modbus 实现：三路都不启用 */
    }

    /* 升级入口：候选命令起点匹配"<本机地址或广播> 10 04 38 00 02"时交给回调。 */
    ModbusProtoSetArmHook(s_au16SciId[APP_MB_ARM_PORT_INDEX],
                          (void *)(Uint32)APP_MB_ARM_PORT_INDEX, AppMbArmHook);

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

Uint16 AppModbusUpgradeFlag(Uint16 u16Port)
{
    if (u16Port >= APP_MB_PORTS) {
        return 0U;
    }
    return s_astUpgradeParam[u16Port].u16MdsIsUpdate;
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

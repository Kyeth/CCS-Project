/**
 * @file AppModbus.c
 * @brief 应用层 Modbus 点表（参照 ST_REG_SLAVE_ITEM 的写法）与升级 ACK 回调。
 *
 * 点表按寄存器地址升序排列，每一项登记：地址、数据类型、读写权限、保存属性、
 * 绑定的变量指针、整数上下限与浮点上下限。类型转换、完整性检查、多寄存器写的
 * 原子提交都由平台的点表模块完成，应用层只维护"地址与变量的对应关系"。
 *
 * 三路分工：
 *   - SCI-A：正常业务口（19200），点表内容以本工程协议为准（当前为测试/版本区，
 *            协议确定后只改这张表）；
 *   - SCI-C / SCI-D：按同一格式各初始化一行参考表项，协议尚未确定。
 *
 * 升级入口（只挂 SCI-A）：
 *   识别到 01 10 04 38 00 02 后回 8 字节 ACK（01 10 04 38 00 02 C1 35），
 *   紧接着发 'C'（0x43）表示可以收包；ACK 与 'C' 都发完、硬件空闲后
 *   把 SCI-A 从 19200 切到 115200，进入 Xmodem 接收。
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
#include "AppModbusProtoTbl.h"
#include "ModbusProto.h"
#include "ModbusRtu.h"
#include "ModbusTbl.h"
#include "Hw_SciDriver.h"
#include "DrvCfg_EP15K.h"

/* 点表项数：等价于参考实现的 MDS_ITEM_OF(arr)。 */
#define MDS_ITEM_OF(arr)        ((Uint16)(sizeof(arr) / sizeof((arr)[0])))

/* 升级参数 */
#define APP_MB_ARM_SCI_ID       (HW_SCI_ID_A)      /* 升级只挂 SCI-A */
#define APP_MB_ARM_PORT_INDEX   (0U)
#define APP_MB_UPGRADE_BAUD     (115200UL)         /* 进入 Xmodem 后的接收波特率 */
#define APP_MB_ARM_ACK_LEN      (8U)
#define APP_MB_XMODEM_READY     (0x43U)            /* 'C'：请开始发送数据包 */

/* 每路的升级状态（对应参考实现的 u16MdsIsUpdate）。 */
static Uint16 s_au16UpgradeFlag[APP_MB_PORTS];
static Uint16 s_u16XmodemReady = 0U;               /* 1 = 已切到 115200 收包 */

/* SCI-C / SCI-D 的占位零值：协议未定，只放一行参考表项。 */
static const Uint16 s_u16Zero = 0U;

#define APP_MB_POINT(addr, type, access, pvar, umin, umax, fmin, fmax)            \
        { (addr), (type), (access), MB_PT_NOT_SAVE,                                \
          (void *)(pvar), (umin), (umax), (fmin), (fmax) }

#define APP_MB_ZERO_POINT(addr)                                                   \
        APP_MB_POINT((addr), MB_PT_U16, MB_PT_READ, &s_u16Zero, 0UL, 65535UL, 0.0f, 0.0f)

#define APP_MB_POINT_TBL_REF()                                                    \
    {                                                                             \
        APP_MB_ZERO_POINT(0U)                                                     \
    }

static const ST_MODBUS_POINT s_astPointTblC[] = APP_MB_POINT_TBL_REF();
static const ST_MODBUS_POINT s_astPointTblD[] = APP_MB_POINT_TBL_REF();

static const ST_MODBUS_POINT *const s_apstPointTbl[APP_MB_PORTS] = {
    g_astAppProtoPointTbl, s_astPointTblC, s_astPointTblD
};
static const Uint16 s_au16PointCount[APP_MB_PORTS] = {
    APP_PROTO_REG_COUNT, MDS_ITEM_OF(s_astPointTblC), MDS_ITEM_OF(s_astPointTblD)
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
 * @brief 升级 ACK 回调：对应参考实现的 ModbusSlaveCallBack_1。
 *
 * 只有 SCI-A 注册本回调。协议层已按"<本机地址或广播> 10 04 38 00 02"匹配过
 * 前六字节，这里复核一遍并返回 9 字节：8 字节 ACK + 'C'。
 * ACK = 01 10 04 38 00 02 C1 35（标准 0x10 回声），'C' 表示可以开始发数据包。
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
        (pu16Reply == (Uint16 *)0) || (u16ReplyMax < (APP_MB_ARM_ACK_LEN + 1U))) {
        return 0U;
    }

    pstCfg = &s_astProtoCfg[u16Port];

    u16Addr  = (Uint16)(pu16Head[0] & 0xFFU);
    u16Func  = (Uint16)(pu16Head[1] & 0xFFU);
    u16Start = (Uint16)(((pu16Head[2] & 0xFFU) << 8) | (pu16Head[3] & 0xFFU));
    u16Nums  = (Uint16)(((pu16Head[4] & 0xFFU) << 8) | (pu16Head[5] & 0xFFU));

    if (((u16Addr == pstCfg->u16SlaveAddr) || (u16Addr == 0U)) &&
        (u16Start == 1080U) && (u16Nums == 2U) && (u16Func == 0x10U)) {
        s_au16UpgradeFlag[u16Port] = 1U;

        for (u16Idx = 0U; u16Idx < 6U; u16Idx++) {
            pu16Reply[u16Idx] = (Uint16)(pu16Head[u16Idx] & 0xFFU);
        }
        u16Crc = ModbusCrc16(pu16Reply, 6U);
        pu16Reply[6] = (Uint16)(u16Crc & 0xFFU);
        pu16Reply[7] = (Uint16)((u16Crc >> 8) & 0xFFU);
        pu16Reply[8] = APP_MB_XMODEM_READY;          /* 'C' */
        return (Uint16)(APP_MB_ARM_ACK_LEN + 1U);
    }

    return 0U;
}

/** 协议层的 ARM 入口：取端口号后交给升级回调。 */
static Uint16 AppMbArmHook(void *pCtx, const Uint16 *pu16Prefix, Uint16 u16PrefixLen,
                           Uint16 *pu16Reply, Uint16 u16ReplyMax)
{
    Uint16 u16Port = (Uint16)(Uint32)pCtx;

    if (u16PrefixLen < MB_ARM_PREFIX_LEN) {
        return 0U;
    }
    return AppModbusCallBack(u16Port, pu16Prefix, pu16Reply, u16ReplyMax);
}

/**
 * @brief 交接完成：ACK 与 'C' 都已发完、硬件空闲，把 SCI-A 切到 115200。
 */
static void AppMbArmDoneHook(void)
{
    if (Hw_SciSetBaud((Uint16)APP_MB_ARM_SCI_ID, APP_MB_UPGRADE_BAUD) != 0U) {
        s_u16XmodemReady = 1U;
    }
}

void AppModbusInit(void)
{
    Uint16 u16Index;

    ProtocolInit();
    ModbusProtoClear();

    s_u16Enabled = 0U;
    s_u32TableErrors = 0UL;
    s_u16XmodemReady = 0U;

    for (u16Index = 0U; u16Index < APP_MB_PORTS; u16Index++) {
        s_au16UpgradeFlag[u16Index] = 0U;
    }

    if (ProtocolRegisterOps(&stModbusProtoOps) == 0U) {
        return;   /* 没有可用的 Modbus 实现：三路都不启用 */
    }

    /* 升级只挂 SCI-A：候选命令起点匹配后回 ACK + 'C'。 */
    ModbusProtoSetArmHook(s_au16SciId[APP_MB_ARM_PORT_INDEX],
                          (void *)(Uint32)APP_MB_ARM_PORT_INDEX, AppMbArmHook);
    ModbusProtoSetArmDoneHook((void *)0, AppMbArmDoneHook);

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
    return s_au16UpgradeFlag[u16Port];
}

Uint16 AppModbusXmodemReady(void)
{
    return s_u16XmodemReady;
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

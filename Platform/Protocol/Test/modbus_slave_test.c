/**
 * @file modbus_slave_test.c
 * @brief 05E 主机测试：寄存器表结构、参考层寄存器访问、端口层桥接的收发路径。
 *
 * 编译（在本目录下，需要主机 cc）：
 *   cc -std=c99 -Wall -Wextra -I./shim -I../../Port/Test/shim -I../../Port/Inc \
 *      -I../../Common/Inc -I../Inc -I../Modbus/Inc -I../../../Drivers/inc \
 *      -I../../../UserApp/Inc \
 *      modbus_slave_test.c ../Src/Protocol.c ../Modbus/Src/Modbus.c \
 *      ../Modbus/Src/ModbusSlave.c ../Modbus/Src/ModbusRegProcess.c \
 *      ../Modbus/Src/ModbusPublic.c ../../../UserApp/Src/AppModbus.c \
 *      ../../../UserApp/Src/AppModbusRegCfg.c -o modbus_slave_test
 *
 * 替身按真实接口建模，不按"方便测试"建模：
 *   - SciPortPopBatch() 一次返回一条批次记录（本测试按最长 8 字节切），记录带
 *     自己的观察时标；单条记录装不进 u16Max 时返回 0（真实接口同此语义）。
 *   - SciPortWriteTx() 整批装不下就一个字节都不收，返回 0。
 *   - 时标是 Timer0 向下计数，"过去多久" = (Uint32)(last - now)。
 *
 * 覆盖：表结构不变量、二分查找边界、读/写单/写多、广播不回复、CRC 错不回复、
 * 地址不匹配不回复、半帧不提前收帧、回复跨轮入环、端口非空闲丢半帧。
 */

#include <stdio.h>
#include <string.h>

#include "Protocol.h"
#include "Modbus.h"
#include "SciPort.h"
#include "Hw_SciDriver.h"
#include "Hw_TimeBase.h"
#include "DrvCfg_EP15K.h"
#include "AppModbus.h"
#include "AppModbusRegCfg.h"

static int s_iChecks = 0;
static int s_iFails = 0;

static void Check(int iCond, const char *pszName)
{
    s_iChecks++;
    if (iCond == 0)
    {
        s_iFails++;
        printf("FAIL: %s\n", pszName);
    }
}

/* ================= 时间基准替身（向下计数） ================= */

static Uint32 s_u32Now = 1000000UL;
static Uint16 s_u16TimeBaseReady = 1U;

Uint32 Hw_TimeBaseTicks(void)
{
    return s_u32Now;
}

Uint16 Hw_TimeBaseIsReady(void)
{
    return s_u16TimeBaseReady;
}

Uint32 Hw_TimeBaseUsToTicks(Uint32 u32Us)
{
    return u32Us; /* 1 tick = 1 us */
}

Uint32 Hw_TimeBaseElapsed(Uint32 u32Start)
{
    return (Uint32)(u32Start - s_u32Now);
}

static void Advance(Uint32 u32Ticks)
{
    s_u32Now = s_u32Now - u32Ticks;
}

/* ================= 板级配置替身 ================= */

static const ST_SCI_CFG_ITEM s_astSciCfgItem[3] = {
    { HW_SCI_ID_A, 19200UL, 0U, 0U, 8U },
    { HW_SCI_ID_C, 115200UL, 0U, 0U, 8U },
    { HW_SCI_ID_D, 38400UL, 0U, 0U, 8U }
};

ST_SCI_CFG stSciCfg = { &s_astSciCfgItem[0], 3U, 50000000UL };

/* ================= 端口层替身 ================= */

#define STUB_PORTS     (4U)   /* 按 HW_SCI_ID 下标 */
#define STUB_SEG_MAX   (8U)   /* 单条批次最多字节数 */
#define STUB_SEG_DEPTH (32U)
#define STUB_TX_DEPTH  (128U)

typedef struct {
    Uint16 au16Byte[STUB_SEG_MAX];
    Uint16 u16Count;
    Uint32 u32Tick;
} ST_STUB_SEG;

static ST_STUB_SEG s_astSeg[STUB_PORTS][STUB_SEG_DEPTH];
static Uint16 s_au16SegHead[STUB_PORTS];
static Uint16 s_au16SegTail[STUB_PORTS];

static Uint16 s_au16Tx[STUB_PORTS][STUB_TX_DEPTH];
static Uint16 s_au16TxGot[STUB_PORTS][STUB_TX_DEPTH];
static Uint16 s_u16TxLen[STUB_PORTS];
static Uint16 s_u16TxGotLen[STUB_PORTS];
static Uint16 s_u16TxFree[STUB_PORTS];
static Uint16 s_u16RxState[STUB_PORTS];

static void StubReset(void)
{
    Uint16 i;

    for (i = 0U; i < STUB_PORTS; i++) {
        s_au16SegHead[i] = 0U;
        s_au16SegTail[i] = 0U;
        s_u16TxLen[i] = 0U;
        s_u16TxGotLen[i] = 0U;
        s_u16TxFree[i] = STUB_TX_DEPTH;
        s_u16RxState[i] = SCI_RX_IDLE;
    }
}

/** 开始一个新场景：清掉本端口的接收队列与"已发出"记录。 */
static void StubTxClear(Uint16 u16Id)
{
    s_au16SegHead[u16Id] = 0U;
    s_au16SegTail[u16Id] = 0U;
    s_u16TxLen[u16Id] = 0U;
    s_u16TxGotLen[u16Id] = 0U;
    s_u16TxFree[u16Id] = STUB_TX_DEPTH;
}

/** 端口环排空：把已接受的字挪进"已发出"记录，恢复空闲量。 */
static void StubTxDrain(Uint16 u16Id)
{
    Uint16 i;

    for (i = 0U; i < s_u16TxLen[u16Id]; i++) {
        s_au16TxGot[u16Id][s_u16TxGotLen[u16Id] + i] = s_au16Tx[u16Id][i];
    }
    s_u16TxGotLen[u16Id] = (Uint16)(s_u16TxGotLen[u16Id] + s_u16TxLen[u16Id]);
    s_u16TxLen[u16Id] = 0U;
    s_u16TxFree[u16Id] = STUB_TX_DEPTH;
}

/** 主机发一段字节：按 ≤8 字节切成批次记录，每条记录带当前时标。 */
static void PushBytes(Uint16 u16Id, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 u16Off = 0U;

    while (u16Off < u16Count) {
        Uint16 u16Take = (Uint16)(u16Count - u16Off);
        ST_STUB_SEG *pstSeg;

        if (u16Take > STUB_SEG_MAX) {
            u16Take = STUB_SEG_MAX;
        }
        pstSeg = &s_astSeg[u16Id][s_au16SegTail[u16Id] % STUB_SEG_DEPTH];
        memcpy(pstSeg->au16Byte, &pu16Data[u16Off], (size_t)u16Take * sizeof(Uint16));
        pstSeg->u16Count = u16Take;
        pstSeg->u32Tick = s_u32Now;
        s_au16SegTail[u16Id]++;
        u16Off = (Uint16)(u16Off + u16Take);
    }
}

Uint16 SciPortPopBatch(Uint16 u16PortId, Uint16 *pu16Dst, Uint16 u16Max, ST_SCI_RX_SEG *pstSeg)
{
    ST_STUB_SEG *pstSrc;

    if ((u16PortId >= STUB_PORTS) || (s_au16SegHead[u16PortId] == s_au16SegTail[u16PortId])) {
        return 0U;
    }

    pstSrc = &s_astSeg[u16PortId][s_au16SegHead[u16PortId] % STUB_SEG_DEPTH];
    if (pstSrc->u16Count > u16Max) {
        return 0U; /* 真实接口：一条记录装不进 u16Max 时返回 0 */
    }

    memcpy(pu16Dst, pstSrc->au16Byte, (size_t)pstSrc->u16Count * sizeof(Uint16));
    memset(pstSeg, 0, sizeof(*pstSeg));
    pstSeg->u32Tick = pstSrc->u32Tick;
    pstSeg->u32TickEmptyLast = pstSrc->u32Tick;
    pstSeg->u16Count = pstSrc->u16Count;
    s_au16SegHead[u16PortId]++;

    return pstSrc->u16Count;
}

Uint16 SciPortWriteTx(Uint16 u16PortId, const Uint16 *pu16Data, Uint16 u16Count)
{
    Uint16 i;

    if ((u16PortId >= STUB_PORTS) || (u16Count == 0U) || (u16Count > s_u16TxFree[u16PortId])) {
        return 0U; /* 整批或全拒 */
    }

    for (i = 0U; i < u16Count; i++) {
        s_au16Tx[u16PortId][s_u16TxLen[u16PortId] + i] = pu16Data[i];
    }
    s_u16TxLen[u16PortId] = (Uint16)(s_u16TxLen[u16PortId] + u16Count);
    s_u16TxFree[u16PortId] = (Uint16)(s_u16TxFree[u16PortId] - u16Count);

    return u16Count;
}

Uint16 SciPortTxFree(Uint16 u16PortId)
{
    return (u16PortId < STUB_PORTS) ? s_u16TxFree[u16PortId] : 0U;
}

Uint16 SciPortIsTxSuspended(Uint16 u16PortId)
{
    (void)u16PortId;
    return 0U;
}

Uint16 SciPortRxState(Uint16 u16PortId)
{
    return (u16PortId < STUB_PORTS) ? s_u16RxState[u16PortId] : SCI_RX_PAUSED;
}

/* ================= 帧与 CRC ================= */

/** 逐位算标准 Modbus CRC-16（poly 0xA001，初值 0xFFFF），用于交叉验证查表实现。 */
static Uint16 TestCrcBitwise(const Uint16 *pu16Data, Uint16 u16Len)
{
    Uint16 u16Crc = 0xFFFFU;
    Uint16 i;
    Uint16 j;

    for (i = 0U; i < u16Len; i++) {
        u16Crc = (Uint16)(u16Crc ^ pu16Data[i]);
        for (j = 0U; j < 8U; j++) {
            if ((u16Crc & 0x0001U) != 0U) {
                u16Crc = (Uint16)((u16Crc >> 1) ^ 0xA001U);
            } else {
                u16Crc = (Uint16)(u16Crc >> 1);
            }
        }
    }

    return u16Crc;
}

/** 组一条 RTU 帧：正文字节 + CRC（低字节在前）。 */
static Uint16 BuildFrame(const Uint16 *pu16Body, Uint16 u16BodyLen, Uint16 *pu16Out)
{
    Uint16 u16Crc;
    Uint16 i;

    for (i = 0U; i < u16BodyLen; i++) {
        pu16Out[i] = pu16Body[i];
    }
    u16Crc = TestCrcBitwise(pu16Out, u16BodyLen);
    pu16Out[u16BodyLen] = (Uint16)(u16Crc & 0xFFU);
    pu16Out[u16BodyLen + 1U] = (Uint16)((u16Crc >> 8) & 0xFFU);

    return (Uint16)(u16BodyLen + 2U);
}

#define TBL_COUNT ((Uint16)(sizeof(g_astAppRegSlaveItem) / sizeof(g_astAppRegSlaveItem[0])))

/* ================= 表结构 ================= */

static void TestTable(void)
{
    const ST_REG_SLAVE_ITEM *pstItem = &g_astAppRegSlaveItem[0];
    int iAscending = 1;
    int iLimitsOk = 1;
    int iTypesOk = 1;
    int iBindOk = 1;
    int iSaveOk = 1;
    int iSignedOk = 1;
    Uint16 u16Signed = 0U;
    Uint16 u16ReadOnly = 0U;
    Uint16 i;

    for (i = 0U; i < TBL_COUNT; i++) {
        if ((i > 0U) && (pstItem[i].u16RegAddr <= pstItem[i - 1U].u16RegAddr)) {
            if (iAscending != 0) {
                printf("  地址乱序：序号 %u 地址 %u 在 %u 之后\n",
                       (unsigned)i, (unsigned)pstItem[i].u16RegAddr,
                       (unsigned)pstItem[i - 1U].u16RegAddr);
            }
            iAscending = 0;
        }
        if ((Uint32)pstItem[i].i32DownLimitVal > pstItem[i].u32UpLimitVal) {
            iLimitsOk = 0;
        }
        if ((pstItem[i].bValType != TYPE_UINT16) && (pstItem[i].bValType != TYPE_INT16)) {
            iTypesOk = 0;
        }
        if ((pstItem[i].bRWType != READ_ONLY) && (pstItem[i].bRWType != READ_WRITE)) {
            iTypesOk = 0;
        }
        if (pstItem[i].puRegVal == (U_REG_VAL *)0) {
            iBindOk = 0;
        }
        if (pstItem[i].bParamSaveType != NOT_SAVE) {
            iSaveOk = 0;
        }
        if (pstItem[i].bValType == TYPE_INT16) {
            u16Signed++;
            if (pstItem[i].i32DownLimitVal >= 0) {
                iSignedOk = 0;
            }
        }
        if (pstItem[i].bRWType == READ_ONLY) {
            u16ReadOnly++;
        }
    }

    Check(TBL_COUNT == APP_MODBUS_REG_COUNT, "table: 项数与 APP_MODBUS_REG_COUNT 一致");
    Check(stRegSlaveCfg_App.u16Num == TBL_COUNT, "table: ST_REG_SLAVE_CFG.u16Num 与数组长度一致");
    Check(stRegSlaveCfg_App.pstReg == (ST_REG_SLAVE_ITEM *)&g_astAppRegSlaveItem[0],
          "table: ST_REG_SLAVE_CFG.pstReg 指向表头");
    Check(iAscending != 0, "table: 地址严格升序（二分查找的前提）");
    Check(iLimitsOk != 0, "table: 每项下限 <= 上限");
    Check(iTypesOk != 0, "table: 类型只有 int16/uint16，属性只有 READ_ONLY/READ_WRITE");
    Check(iBindOk != 0, "table: 每项都绑定变量");
    Check(iSaveOk != 0, "table: 保存属性统一为 NOT_SAVE");
    Check(iSignedOk != 0, "table: 有符号点的下限为负");

    printf("  表项 %u：有符号 %u，只读 %u，首地址 %u，末地址 %u\n",
           (unsigned)TBL_COUNT, (unsigned)u16Signed, (unsigned)u16ReadOnly,
           (unsigned)pstItem[0].u16RegAddr, (unsigned)pstItem[TBL_COUNT - 1U].u16RegAddr);
}

/* ================= 寄存器查找 ================= */

static void TestLookup(void)
{
    const ST_REG_SLAVE_ITEM *pstItem = &g_astAppRegSlaveItem[0];
    ST_REG_SLAVE_ITEM stGet;
    Uint16 u16GapAddr = 0xFFFFU;
    Uint16 i;
    int iFound = 1;

    memset(&stGet, 0, sizeof(stGet));

    if (ReadModbusRegisterData((ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                               pstItem[0].u16RegAddr, &stGet) != 0) {
        iFound = 0;
    } else if (stGet.u16RegAddr != pstItem[0].u16RegAddr) {
        iFound = 0;
    }
    Check(iFound != 0, "lookup: 首项查得到且地址回填正确");

    iFound = 1;
    if (ReadModbusRegisterData((ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                               pstItem[TBL_COUNT - 1U].u16RegAddr, &stGet) != 0) {
        iFound = 0;
    } else if (stGet.u16RegAddr != pstItem[TBL_COUNT - 1U].u16RegAddr) {
        iFound = 0;
    }
    Check(iFound != 0, "lookup: 末项查得到且地址回填正确");

    Check(ReadModbusRegisterData((ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                                 pstItem[TBL_COUNT / 2U].u16RegAddr, &stGet) == 0,
          "lookup: 中项查得到");

    Check(ReadModbusRegisterData((ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                                 (Uint16)(pstItem[TBL_COUNT - 1U].u16RegAddr + 1U), &stGet) == -2,
          "lookup: 表尾之后报 -2");

    for (i = 1U; i < TBL_COUNT; i++) {
        if (pstItem[i].u16RegAddr > (Uint16)(pstItem[i - 1U].u16RegAddr + 1U)) {
            u16GapAddr = (Uint16)(pstItem[i - 1U].u16RegAddr + 1U);
            break;
        }
    }
    if (u16GapAddr != 0xFFFFU) {
        Check(ReadModbusRegisterData((ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                                     u16GapAddr, &stGet) == -3,
              "lookup: 空洞地址报 -3");
        printf("  空洞样例：地址 %u 不在表内\n", (unsigned)u16GapAddr);
    }
}

/* ================= 收发 ================= */

#define PORT_A (HW_SCI_ID_A)
#define PORT_C (HW_SCI_ID_C)
#define PORT_D (HW_SCI_ID_D)

/* 从表头地址开始的连续段长度：读多寄存器的用例要落在这一段里。 */
static Uint16 s_u16RunFromStart = 0U;

static void FindRun(void)
{
    const ST_REG_SLAVE_ITEM *pstItem = &g_astAppRegSlaveItem[0];
    Uint16 i;

    s_u16RunFromStart = 1U;
    for (i = 1U; i < TBL_COUNT; i++) {
        if (pstItem[i].u16RegAddr != (Uint16)(pstItem[i - 1U].u16RegAddr + 1U)) {
            break;
        }
        s_u16RunFromStart++;
    }
}

/** 走一轮协议任务（三个实例各服务一次）。 */
static void ServiceOnce(void)
{
    ProtocolTask();
}

/** 清场 → 发一帧 → 等静默到期 → 服务一轮。返回 1 = 有回复落地。 */
static int Exchange(Uint16 u16Id, const Uint16 *pu16Frame, Uint16 u16Len)
{
    StubTxClear(u16Id);
    PushBytes(u16Id, pu16Frame, u16Len);
    Advance(2100U);                 /* A 口 19200 的 3.5 字符是 2006 us */
    ServiceOnce();
    StubTxDrain(u16Id);

    return (s_u16TxGotLen[u16Id] != 0U) ? 1 : 0;
}

static void TestFrames(void)
{
    Uint16 au16Frame[300];
    Uint16 au16Body[300];
    Uint16 u16Len;
    Uint16 u16N;
    Uint16 u16ReplyLen;
    Uint16 u16Free;
    Uint16 u16Round;
    Uint32 u32Frames;
    Uint16 u16Crc;

    StubReset();
    AppModbusInit();

    Check(AppModbusInitErrors() == 0UL, "init: 三个实例登记无错误");
    Check(AppModbusEnabledCount() == 3U, "init: 三个实例都进任务");
    Check(AppModbusInstance(0U) != (const ST_PROTO_INSTANCE *)0, "init: 实例 0 可查");
    Check((AppModbusInstance(0U) != (const ST_PROTO_INSTANCE *)0) &&
          (AppModbusInstance(0U)->u16SciId == PORT_A), "init: 实例 0 是 A 口");
    Check(AppModbusInstance(1U)->u16SlaveAddr == 1U, "addr: C 口从机地址 1");
    Check(AppModbusInstance(2U)->u16SlaveAddr == 2U, "addr: D 口从机地址 2");

    /* ---- 1. 标准 0x03 读 2 个寄存器 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x02U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "read: 有回复");
    Check(s_u16TxGotLen[PORT_A] == 9U, "read: 回复长度 3+2*2+2 = 9");
    Check(s_au16TxGot[PORT_A][0] == 0x01U, "read: 回复从机地址");
    Check(s_au16TxGot[PORT_A][1] == 0x03U, "read: 回复功能码");
    Check(s_au16TxGot[PORT_A][2] == 0x04U, "read: 字节数 = 寄存器数*2");
    u16Crc = TestCrcBitwise(&s_au16TxGot[PORT_A][0], 7U);
    Check(s_au16TxGot[PORT_A][7] == (Uint16)(u16Crc & 0xFFU), "read: CRC 低字节在前");
    Check(s_au16TxGot[PORT_A][8] == (Uint16)((u16Crc >> 8) & 0xFFU), "read: CRC 高字节在后");

    /* ---- 2. 0x06 写单个寄存器，再读回 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x06U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x12U; au16Body[5] = 0x34U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "write6: 有回复");
    Check(s_u16TxGotLen[PORT_A] == 8U, "write6: 应答是原帧回显，8 字节");
    Check(memcmp(&s_au16TxGot[PORT_A][0], au16Frame, (size_t)8U * sizeof(Uint16)) == 0,
          "write6: 回显内容与请求一致");

    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "readback: 有回复");
    Check(s_u16TxGotLen[PORT_A] == 7U, "readback: 1 个寄存器回复 7 字节");
    Check(s_au16TxGot[PORT_A][3] == 0x12U, "readback: 高字节是刚写进去的 0x12");
    Check(s_au16TxGot[PORT_A][4] == 0x34U, "readback: 低字节是刚写进去的 0x34");

    /* ---- 3. 广播：不回复，但写要生效 ---- */
    au16Body[0] = 0x00U; au16Body[1] = 0x06U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x7FU;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 0, "broadcast: 不回复");

    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "broadcast: 写生效后能读回");
    Check(s_au16TxGot[PORT_A][4] == 0x7FU, "broadcast: 广播写确实改到了寄存器");

    /* ---- 4. CRC 错不回复 ---- */
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    au16Frame[u16Len - 1U] = (Uint16)(au16Frame[u16Len - 1U] ^ 0xFFU);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 0, "crc: 校验错不回复");

    /* ---- 5. 地址不匹配不回复 ---- */
    au16Body[0] = 0x07U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 0, "addr: 非本站地址不回复");

    /* ---- 6. 读表外地址 → 异常应答 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x7FU; au16Body[3] = 0xFFU;   /* 32767，超出表尾 */
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "error: 越界要回异常帧");
    Check(s_u16TxGotLen[PORT_A] == 5U, "error: 异常帧 5 字节");
    /* 参考层的异常帧第 2 字节 = MODBUS_ERR_BASE_CODE(0x80) + eErrorType，
     * 不是规范要求的 (请求功能码 | 0x80)；ERROR_ADDRESS = 0x02，所以是 0x82。 */
    Check(s_au16TxGot[PORT_A][1] == 0x82U, "error: 异常帧第 2 字节 = 0x80 + ERROR_ADDRESS");
    Check(s_au16TxGot[PORT_A][2] == 0x7FU, "error: 第 3 字节是起始地址高字节（参考层写法）");

    /* ---- 7. 半帧不提前收帧：帧内 1 ms 静默不该触发组帧 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);

    StubTxClear(PORT_A);
    PushBytes(PORT_A, &au16Frame[0], 4U);
    Advance(1000U);                /* 小于 A 口的 2006 us 静默门限 */
    ServiceOnce();
    Check(s_u16TxLen[PORT_A] == 0U, "half: 帧内静默不足门限时不组帧");
    PushBytes(PORT_A, &au16Frame[4], (Uint16)(u16Len - 4U));
    Advance(2100U);
    ServiceOnce();
    StubTxDrain(PORT_A);
    Check(s_u16TxGotLen[PORT_A] == 7U, "half: 补齐后正常回复");

    /* ---- 8. 两帧粘在一起：按 CRC 失败处理，不拆开 ---- */
    StubTxClear(PORT_A);
    PushBytes(PORT_A, au16Frame, u16Len);
    PushBytes(PORT_A, au16Frame, u16Len);   /* 间隔 0：合并成一帧 */
    Advance(2100U);
    ServiceOnce();
    Check(s_u16TxLen[PORT_A] == 0U, "merged: 两帧合并后 CRC 不过，不回复");

    /* ---- 9. 0x10 写 1080：协议表里没有这个地址，应答是异常帧 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x10U;
    au16Body[2] = 0x04U; au16Body[3] = 0x38U;   /* 1080 */
    au16Body[4] = 0x00U; au16Body[5] = 0x02U;   /* 2 个寄存器 */
    au16Body[6] = 0x04U;                        /* 4 字节 */
    au16Body[7] = 0x00U; au16Body[8] = 0x01U;
    au16Body[9] = 0x00U; au16Body[10] = 0x02U;
    u16Len = BuildFrame(au16Body, 11U, au16Frame);
    Check(Exchange(PORT_A, au16Frame, u16Len) == 1, "upgrade: 1080 落表外，仍要回异常帧");
    Check(s_u16TxGotLen[PORT_A] == 5U, "upgrade: 异常帧 5 字节");
    Check(s_au16TxGot[PORT_A][1] == 0x82U, "upgrade: 异常帧第 2 字节 = 0x80 + ERROR_ADDRESS");
    Check(AppModbusUpgradeFlag(0U) == 0U, "upgrade: 写被拒时不得置升级标志");

    /* ---- 10. 长回复跨轮入环 ---- */
    u16N = s_u16RunFromStart;
    if (u16N > 120U) {
        u16N = 120U;   /* MAX_REG_NUM */
    }
    u16ReplyLen = (Uint16)(3U + (2U * u16N) + 2U);
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = (Uint16)(u16N & 0xFFU);
    u16Len = BuildFrame(au16Body, 6U, au16Frame);

    u16Free = 64U;
    if (u16Free >= u16ReplyLen) {
        u16Free = (Uint16)(u16ReplyLen / 2U);
    }
    u32Frames = AppModbusTxFrames(0U);

    StubTxClear(PORT_A);
    PushBytes(PORT_A, au16Frame, u16Len);
    s_u16TxFree[PORT_A] = u16Free;      /* 环只放得下 u16Free 字，逼出分批 */
    Advance(2100U);
    ServiceOnce();
    Check(s_u16TxLen[PORT_A] == u16Free, "chunk: 第一轮只送得动空闲量");
    Check(u16ReplyLen > u16Free, "chunk: 本用例的回复确实超过环空闲量");
    Check(AppModbusTxFrames(0U) == u32Frames, "chunk: 没送完不算发出");

    u16Round = 0U;
    while ((s_u16TxGotLen[PORT_A] < u16ReplyLen) && (u16Round < 12U)) {
        StubTxDrain(PORT_A);
        ServiceOnce();
        u16Round++;
    }
    StubTxDrain(PORT_A);

    Check(s_u16TxGotLen[PORT_A] == u16ReplyLen, "chunk: 分多次送完，总长正确");
    Check(s_au16TxGot[PORT_A][0] == 0x01U, "chunk: 首字节是从机地址");
    Check(s_au16TxGot[PORT_A][1] == 0x03U, "chunk: 第二字节是功能码");
    Check(s_au16TxGot[PORT_A][2] == (Uint16)((u16N * 2U) & 0xFFU), "chunk: 字节数字段正确");
    Check(AppModbusTxFrames(0U) == (u32Frames + 1UL), "chunk: 完整送出一帧后计数 +1");
    u16Crc = TestCrcBitwise(&s_au16TxGot[PORT_A][0], (Uint16)(u16ReplyLen - 2U));
    Check(s_au16TxGot[PORT_A][u16ReplyLen - 2U] == (Uint16)(u16Crc & 0xFFU),
          "chunk: 长回复的 CRC 低字节正确");
    Check(s_au16TxGot[PORT_A][u16ReplyLen - 1U] == (Uint16)((u16Crc >> 8) & 0xFFU),
          "chunk: 长回复的 CRC 高字节正确");
    printf("  长回复用例：%u 个寄存器，回复 %u 字节，每轮 %u 字节，共 %u 轮\n",
           (unsigned)u16N, (unsigned)u16ReplyLen, (unsigned)u16Free, (unsigned)u16Round);

    /* ---- 11. 端口不在接收态时丢半帧 ---- */
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = g_astAppRegSlaveItem[0].u16RegAddr;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(au16Body, 6U, au16Frame);

    StubTxClear(PORT_A);
    PushBytes(PORT_A, au16Frame, 4U);
    ServiceOnce();                 /* 先取进缓冲，静默没到，不组帧 */
    Check(AppModbusRxDropped(0U) == 0UL, "resync: 正常接收态不丢");
    s_u16RxState[PORT_A] = SCI_RX_SW_RESYNC;
    ServiceOnce();
    Check(AppModbusRxDropped(0U) == 4UL, "resync: 端口非空闲时半帧计入丢弃");
    Check(s_u16TxLen[PORT_A] == 0U, "resync: 丢半帧不产生回复");
    s_u16RxState[PORT_A] = SCI_RX_IDLE;

    /* ---- 12. 时间基准不可用时退化为"本轮没取到新字节就算静默" ---- */
    StubTxClear(PORT_A);
    s_u16TimeBaseReady = 0U;
    PushBytes(PORT_A, au16Frame, u16Len);
    ServiceOnce();                 /* 本轮取到了新字节：不组帧 */
    Check(s_u16TxLen[PORT_A] == 0U, "timebase: 取到新字节的当轮不组帧");
    ServiceOnce();                 /* 本轮没取到新字节：组帧 */
    StubTxDrain(PORT_A);
    Check(s_u16TxGotLen[PORT_A] == 7U, "timebase: 退化规则下也能收帧");
    s_u16TimeBaseReady = 1U;
}

int main(void)
{
    FindRun();

    TestTable();
    TestLookup();
    TestFrames();

    printf("\nmodbus_slave_test: %d 项检查，%d 项失败\n", s_iChecks, s_iFails);

    return (s_iFails == 0) ? 0 : 1;
}

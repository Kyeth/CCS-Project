/**
 * @file AppModbus.c
 * @brief Modbus RTU 从机应用层：三路 SCI 从站实例 + 参考协议层与端口层的桥接。
 *
 * 05E 的分工：
 *   - Platform/Protocol/Modbus 是参考工程的原文件，逐字照搬，只换了设备头。
 *     本文件不猜它的内部结构，只用它公开的四个入口：
 *     ModbusSlaveTableRegister / SetModbusSlaveTableType / SetModbusSlaveAddr /
 *     ModbusSlaveHandle。
 *   - 本文件只做三件事：建三个 ST_MODBUS_SLAVE_TABLE_CRTL 实例；把端口层的
 *     字节批次拼成一条完整帧交给 ModbusSlaveHandle()；把返回的回复交给
 *     SciPortWriteTx()。
 *   - 帧边界用"批次观察时刻 + 3.5 字符静默"判断，不用参考层的 MODBUS_TIMEOUT
 *     轮询计数：1 ms 任务周期是任务节拍，不是帧间隔的度量单位。批次时标
 *     ST_SCI_RX_SEG.u32Tick 来自同一个 Timer0 时间基准，所以两者可以直接比较。
 *
 * 数据空间：参考层把回复写回传进去的同一个缓冲区（ModbusSlaveDataHandle() 里的
 * memset + 组帧），所以每个端口只有一块 256 字的缓冲，收帧与回帧串行复用。
 * 回复没被端口环整帧接纳之前不接受新帧，保证这块缓冲不会被下一帧的请求覆盖。
 */

#include "AppModbus.h"
#include "AppModbusRegCfg.h"
#include "Modbus.h"
#include "ModbusSlave.h"
#include "Hw_SciDriver.h"
#include "Hw_TimeBase.h"
#include "SciPort.h"
#include "DrvCfg_EP15K.h"

/* 参考层的数据空间长度（与参考工程 ModbusSlaveComm.c 的 MdsBuff_1[256] 一致）。 */
#define APP_MB_DATA_MAX        (256U)

/* 每轮取批次的配额：段数与字节数都要有上限，1 ms 任务不能被一路读下去。 */
#define APP_MB_RX_CHUNKS       (8U)
#define APP_MB_RX_BYTES        (32U)
#define APP_MB_CHUNK_BYTES     (16U)   /* 单条批次最多 8 字节，这里留一倍余量 */

/* 3.5 字符静默：≤19200 时按 38.5 位/波特率折算，>19200 按规范取固定 1750 us。 */
#define APP_MB_HIGH_BAUD       (19200UL)
#define APP_MB_T35_FIXED_US    (1750UL)
#define APP_MB_US_CHAR35_NUM   (38500000UL)

/* 升级握手：主机写 1080 起的 2 个寄存器，功能码 0x10。 */
#define APP_MB_UPGRADE_ADDR    (1080U)
#define APP_MB_UPGRADE_NUMS    (2U)
#define APP_MB_CMD_WRITE_MULTI (0x10U)
#define APP_MB_BROADCAST_ADDR  (0x00U)

/** 一个从站实例的全部现场：参考层控制块 + 它要的数据空间 + 桥接状态。 */
typedef struct {
    ST_MODBUS_SLAVE_TABLE_CRTL stCtl;
    ST_MDS_DATA stMds;
    Uint16 *pu16Data;                 /* 该端口的数据空间：请求进、回复出同一块 */
    Uint16 u16RxLen;                  /* 已积累的请求字节数 */
    Uint16 u16TxLen;                  /* 待发回复总长；0 = 没有待发回复 */
    Uint16 u16TxOff;                  /* 已交给端口环的回复字节数 */
    Uint32 u32RxTick;                 /* 最后一批请求字节被观察到的时刻 */
    Uint32 u32RxFrames;
    Uint32 u32TxFrames;
    Uint32 u32RxDropped;
} ST_APP_MB_PORT;

static ST_APP_MB_PORT s_astPort[APP_MB_PORTS];

/* Modbus 数据空间：每端口 256 字。放在 .mbregs（RAMGS1）与寄存器占位数组同段——
 * RAMGS0 只剩 2 字，这里的 768 字不能再从 .bss 里出。.mbregs 是 NOINIT 段，
 * 不参与 C 运行时清零，所以 AppModbusInit() 里必须显式清一次。 */
static Uint16 s_au16DataSpace[APP_MB_PORTS][APP_MB_DATA_MAX];

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_au16DataSpace, ".mbregs")
#endif
static ST_PROTO_INSTANCE s_astProtoCfg[APP_MB_PORTS];

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_astProtoCfg, ".protobuf")
#endif

static const Uint16 s_au16SciId[APP_MB_PORTS] = {
    HW_SCI_ID_A, HW_SCI_ID_C, HW_SCI_ID_D
};
static const Uint16 s_au16SlaveAddr[APP_MB_PORTS] = { 1U, 1U, 2U };

static Uint32 s_au32Baud[APP_MB_PORTS];
static Uint32 s_u32InitErrors = 0UL;

static void AppModbusCallBack_A(void);
static void AppModbusCallBack_C(void);
static void AppModbusCallBack_D(void);
static const ModbusCallBack s_apfnCallBack[APP_MB_PORTS] = {
    AppModbusCallBack_A, AppModbusCallBack_C, AppModbusCallBack_D
};

static Uint16 AppMbProtoInit(ST_PROTO_INSTANCE *pstInst);
static Uint16 AppMbProtoService(ST_PROTO_INSTANCE *pstInst);
static void   AppMbProtoReset(ST_PROTO_INSTANCE *pstInst);

static const ST_PROTO_OPS s_stModbusOps = {
    PROTO_TYPE_MODBUS,
    AppMbProtoInit,
    AppMbProtoService,
    AppMbProtoReset
};

/* ------------------------------------------------------------------ */
/* 端口配置                                                            */
/* ------------------------------------------------------------------ */

/** 从工程配置里取该端口的波特率；取不到返回 0（静默判据退化为固定 1750 us）。 */
static Uint32 AppMbBaudOf(Uint16 u16SciId)
{
    Uint16 u16Index;

    if ((stSciCfg.pstSciCfgItem == (const ST_SCI_CFG_ITEM *)0) ||
        (stSciCfg.u16ItemNum == 0U)) {
        return 0UL;
    }

    for (u16Index = 0U; u16Index < stSciCfg.u16ItemNum; u16Index++) {
        if (stSciCfg.pstSciCfgItem[u16Index].u16SciId == u16SciId) {
            return stSciCfg.pstSciCfgItem[u16Index].u32Baudrate;
        }
    }

    return 0UL;
}

/** 3.5 字符时间（us），向上取整。 */
static Uint32 AppMbT35Us(Uint32 u32Baud)
{
    if ((u32Baud == 0UL) || (u32Baud > APP_MB_HIGH_BAUD)) {
        return APP_MB_T35_FIXED_US;
    }

    return (APP_MB_US_CHAR35_NUM + u32Baud - 1UL) / u32Baud;
}

/* ------------------------------------------------------------------ */
/* 升级标志                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief 参考层在功能码处理完之后回调，此时 stMdsFrame 里的地址/功能码/寄存器
 *        范围都已经填好。命中升级握手就置升级标志——与参考工程的业务层同义。
 */
static void AppMbUpgradeWatch(Uint16 u16Port)
{
    const ST_MODBUS_SLAVE_TABLE_CRTL *pstCtl = &s_astPort[u16Port].stCtl;
    const ST_MDS_FRAME *pstFrame = &pstCtl->stMdsFrame;

    /* 参考层在命令处理函数返回之后无条件调用业务回调，所以写被拒绝（地址不在表内、
     * 数据越限等）时回调照样进来。升级握手必须要求这一帧本身处理成功，否则一次
     * 失败的写也会把升级标志置起。 */
    if ((pstFrame->eErrorType == DATA_OK) &&
        ((pstFrame->bAddr == pstCtl->bModbusSlaveAddr) ||
         (pstFrame->bAddr == APP_MB_BROADCAST_ADDR)) &&
        (pstFrame->u16RegStartAddr == APP_MB_UPGRADE_ADDR) &&
        (pstFrame->u16RegNums == APP_MB_UPGRADE_NUMS) &&
        (pstFrame->bFuncCmd == APP_MB_CMD_WRITE_MULTI)) {
        s_astPort[u16Port].stCtl.stMdsUpdateParam.u16MdsIsUpdate = 1U;
    }
}

static void AppModbusCallBack_A(void)
{
    AppMbUpgradeWatch(0U);
}

static void AppModbusCallBack_C(void)
{
    AppMbUpgradeWatch(1U);
}

static void AppModbusCallBack_D(void)
{
    AppMbUpgradeWatch(2U);
}

/* ------------------------------------------------------------------ */
/* 发送侧                                                              */
/* ------------------------------------------------------------------ */

/**
 * @brief 把待发回复推给端口环，只推端口实际接纳的部分。
 *
 * SciPortWriteTx() 是"整批或全拒"：一次给 245 字节而环只有 128 字节时会一个都不收。
 * 所以先问空闲量，按空闲量分批送；送不动的留到下一轮，不重发已送出的前缀。
 */
static void AppMbFlushReply(ST_APP_MB_PORT *pstPort, Uint16 u16SciId)
{
    Uint16 u16Want;
    Uint16 u16Free;
    Uint16 u16Got;

    while (pstPort->u16TxOff < pstPort->u16TxLen) {
        if (SciPortIsTxSuspended(u16SciId) != 0U) {
            return;   /* 端口正在恢复：发送侧挂起，等协议层确认后再续 */
        }

        u16Free = SciPortTxFree(u16SciId);
        if (u16Free == 0U) {
            return;
        }

        u16Want = (Uint16)(pstPort->u16TxLen - pstPort->u16TxOff);
        if (u16Want > u16Free) {
            u16Want = u16Free;
        }

        u16Got = SciPortWriteTx(u16SciId, &pstPort->pu16Data[pstPort->u16TxOff], u16Want);
        if (u16Got == 0U) {
            return;   /* 环边界留了一格：下一轮再来 */
        }

        pstPort->u16TxOff = (Uint16)(pstPort->u16TxOff + u16Got);
    }

    pstPort->u16TxLen = 0U;
    pstPort->u16TxOff = 0U;
    pstPort->u32TxFrames++;
}

/* ------------------------------------------------------------------ */
/* 接收侧                                                              */
/* ------------------------------------------------------------------ */

static void AppMbDropRx(ST_APP_MB_PORT *pstPort)
{
    if (pstPort->u16RxLen != 0U) {
        pstPort->u32RxDropped = pstPort->u32RxDropped + (Uint32)pstPort->u16RxLen;
        pstPort->u16RxLen = 0U;
    }
}

/**
 * @brief 把端口的字节批次拼进数据空间。
 * @return 1 = 本轮没取满配额（端口里应该已经空了）；0 = 配额用尽，还有积压
 */
static Uint16 AppMbDrainRx(ST_APP_MB_PORT *pstPort, Uint16 u16SciId, Uint16 *pu16Grew)
{
    Uint16 u16Chunk;
    Uint16 u16Total = 0U;
    Uint16 u16Max;
    Uint16 u16Got;

    *pu16Grew = 0U;

    for (u16Chunk = 0U; u16Chunk < APP_MB_RX_CHUNKS; u16Chunk++) {
        ST_SCI_RX_SEG stSeg;

        if (u16Total >= APP_MB_RX_BYTES) {
            return 0U;                        /* 字节配额用尽 */
        }

        u16Max = (Uint16)(APP_MB_RX_BYTES - u16Total);
        if (u16Max > APP_MB_CHUNK_BYTES) {
            u16Max = APP_MB_CHUNK_BYTES;
        }

        /* 剩下的空间不够一条最长批次时不再取：PopBatch 装不下会返回 0，
         * 那会被误当成"端口已空"，从而把半帧当成整帧交给协议层。 */
        if ((Uint16)(APP_MB_DATA_MAX - pstPort->u16RxLen) < APP_MB_CHUNK_BYTES) {
            return 1U;
        }

        u16Got = SciPortPopBatch(u16SciId, &pstPort->pu16Data[pstPort->u16RxLen],
                                u16Max, &stSeg);
        if (u16Got == 0U) {
            return 1U;                        /* 本轮确实取空了 */
        }

        pstPort->u16RxLen = (Uint16)(pstPort->u16RxLen + u16Got);
        pstPort->u32RxTick = stSeg.u32Tick;
        *pu16Grew = 1U;
        u16Total = (Uint16)(u16Total + u16Got);

        if (u16Got < u16Max) {
            return 1U;                        /* 没取满：环已空 */
        }
    }

    return 0U;                                /* 段数配额用尽，可能还有积压 */
}

/**
 * @brief 3.5 字符静默到了没有。
 *
 * 时钟可信时用批次时标算真实静默；时钟不可信时退化为"这一轮没有任何新字节"，
 * 宁可晚一轮收帧，也不在时间关系不可信的时候拼帧。
 */
static Uint16 AppMbFrameIdle(const ST_APP_MB_PORT *pstPort, Uint16 u16Port, Uint16 u16Grew)
{
    if (Hw_TimeBaseIsReady() != 0U) {
        return (Hw_TimeBaseElapsed(pstPort->u32RxTick) >=
                Hw_TimeBaseUsToTicks(AppMbT35Us(s_au32Baud[u16Port]))) ? 1U : 0U;
    }

    return (u16Grew == 0U) ? 1U : 0U;
}

/**
 * @brief 把攒够的一条请求帧交给参考层，并把回复挂到发送侧。
 */
static void AppMbDispatchFrame(ST_APP_MB_PORT *pstPort)
{
    ST_MDS_FRAME *pstFrame;
    Uint16 u16ReqLen;
    Uint16 u16ReqAddr;

    u16ReqLen = pstPort->u16RxLen;
    u16ReqAddr = pstPort->pu16Data[0];   /* 组帧后会被回复覆盖，先记下 */

    pstPort->stMds.pu16DataBuff = &pstPort->pu16Data[0];
    pstPort->stMds.u16DataLenth = u16ReqLen;

    pstFrame = ModbusSlaveHandle(&pstPort->stCtl, &pstPort->stMds);
    pstPort->u16RxLen = 0U;
    pstPort->u32RxFrames++;

    if (pstFrame == (ST_MDS_FRAME *)0) {
        return;   /* 地址不匹配 / 帧太短 / 广播帧 CRC 错：不回复 */
    }
    if (u16ReqAddr == APP_MB_BROADCAST_ADDR) {
        return;   /* 广播不回复。参考工程这里会把请求原样回写，本层不复刻该行为 */
    }

    pstPort->u16TxLen = pstFrame->pstMdsData->u16DataLenth;
    pstPort->u16TxOff = 0U;
    if (pstPort->u16TxLen > APP_MB_DATA_MAX) {
        pstPort->u16TxLen = 0U;   /* 参考层给出的长度越界：整帧丢弃，不发半帧 */
    }
}

/* ------------------------------------------------------------------ */
/* 协议操作表                                                          */
/* ------------------------------------------------------------------ */

static Uint16 AppMbProtoInit(ST_PROTO_INSTANCE *pstInst)
{
    if ((pstInst == (ST_PROTO_INSTANCE *)0) || (pstInst->pCtx == (void *)0) ||
        (pstInst->pstTbl == (const void *)0)) {
        return 0U;
    }

    return 1U;
}

static void AppMbProtoReset(ST_PROTO_INSTANCE *pstInst)
{
    ST_APP_MB_PORT *pstPort;

    if ((pstInst == (ST_PROTO_INSTANCE *)0) || (pstInst->pCtx == (void *)0)) {
        return;
    }

    pstPort = (ST_APP_MB_PORT *)pstInst->pCtx;
    AppMbDropRx(pstPort);
    pstPort->u16TxLen = 0U;
    pstPort->u16TxOff = 0U;
}

static Uint16 AppMbProtoService(ST_PROTO_INSTANCE *pstInst)
{
    ST_APP_MB_PORT *pstPort;
    Uint16 u16SciId;
    Uint16 u16Port;
    Uint16 u16Grew = 0U;
    Uint16 u16Drained;

    if ((pstInst == (ST_PROTO_INSTANCE *)0) || (pstInst->pCtx == (void *)0)) {
        return 0U;
    }

    pstPort = (ST_APP_MB_PORT *)pstInst->pCtx;
    u16SciId = pstInst->u16SciId;
    u16Port = (Uint16)(pstPort - &s_astPort[0]);

    /* 1) 上一帧的回复还没被端口环全部接纳：先推它，本轮不接新帧。
     *    数据空间同时是回复缓冲，所以这里必须先收手，不能边发边收。 */
    if (pstPort->u16TxOff < pstPort->u16TxLen) {
        AppMbFlushReply(pstPort, u16SciId);
        if (pstPort->u16TxOff < pstPort->u16TxLen) {
            return 1U;
        }
    }

    /* 2) 端口不在正常接收态：半帧作废。否则恢复完成后残留的字节会和
     *    新帧拼成一条看起来合法的请求。 */
    if (SciPortRxState(u16SciId) != SCI_RX_IDLE) {
        AppMbDropRx(pstPort);
        return 1U;
    }

    /* 3) 取批次。 */
    u16Drained = AppMbDrainRx(pstPort, u16SciId, &u16Grew);

    /* 4) 帧边界：配额没用完才判帧结束——配额用尽说明端口里可能还压着这一帧
     *    的后半段，这时候收帧会把一帧劈成两条。 */
    if ((pstPort->u16RxLen == 0U) || (u16Drained == 0U)) {
        return 1U;
    }
    if (AppMbFrameIdle(pstPort, u16Port, u16Grew) == 0U) {
        return 1U;
    }

    /* 5) 一帧攒够了：交给参考层，再把回复挂上发送侧。 */
    AppMbDispatchFrame(pstPort);
    AppMbFlushReply(pstPort, u16SciId);

    return 1U;
}

/* ------------------------------------------------------------------ */
/* 对外接口                                                            */
/* ------------------------------------------------------------------ */

void AppModbusInit(void)
{
    Uint16 u16Port;
    Uint16 u16Ok;

    /* .mbregs 不参与 C 运行时清零：寄存器占位值与数据空间都要显式清零，
     * 否则主机上电读到的是上电随机值。 */
    AppRegCfgReset();
    for (u16Port = 0U; u16Port < APP_MB_PORTS; u16Port++)
    {
        Uint16 u16Word;

        for (u16Word = 0U; u16Word < APP_MB_DATA_MAX; u16Word++)
        {
            s_au16DataSpace[u16Port][u16Word] = 0U;
        }
    }

    ProtocolInit();
    if (ProtocolRegisterOps(&s_stModbusOps) == 0U) {
        s_u32InitErrors++;
    }

    for (u16Port = 0U; u16Port < APP_MB_PORTS; u16Port++) {
        ST_APP_MB_PORT *pstPort = &s_astPort[u16Port];
        ST_PROTO_INSTANCE *pstCfg = &s_astProtoCfg[u16Port];

        pstPort->pu16Data = &s_au16DataSpace[u16Port][0];
        pstPort->stMds.pu16DataBuff = &pstPort->pu16Data[0];
        pstPort->stMds.u16DataLenth = 0U;
        pstPort->u16RxLen = 0U;
        pstPort->u16TxLen = 0U;
        pstPort->u16TxOff = 0U;
        pstPort->u32RxTick = 0UL;
        pstPort->u32RxFrames = 0UL;
        pstPort->u32TxFrames = 0UL;
        pstPort->u32RxDropped = 0UL;
        s_au32Baud[u16Port] = AppMbBaudOf(s_au16SciId[u16Port]);

        /* 三步与参考工程的 ModbusSlaveInit() 同序：登记表 → 选表 → 设从机地址。
         * 表是 Flash 常量，参考层的接口是"非 const 指针"，与参考工程一样强转。 */
        if (ModbusSlaveTableRegister(&pstPort->stCtl, MODBUS_SLAVE_DEFAULT,
                                     (ST_REG_SLAVE_CFG *)&stRegSlaveCfg_App,
                                     s_apfnCallBack[u16Port]) != 0) {
            s_u32InitErrors++;
        }
        if (SetModbusSlaveTableType(&pstPort->stCtl, MODBUS_SLAVE_DEFAULT) != 0) {
            s_u32InitErrors++;
        }
        SetModbusSlaveAddr(&pstPort->stCtl, s_au16SlaveAddr[u16Port]);

        pstCfg->u16SciId = s_au16SciId[u16Port];
        pstCfg->u16Type = PROTO_TYPE_MODBUS;
        pstCfg->u16SlaveAddr = s_au16SlaveAddr[u16Port];
        pstCfg->u16Enabled = 1U;
        pstCfg->u16TblCount = (Uint16)APP_MODBUS_REG_COUNT;
        pstCfg->u32Baud = s_au32Baud[u16Port];
        pstCfg->pstTbl = (const void *)&stRegSlaveCfg_App;
        pstCfg->pCtx = (void *)pstPort;

        u16Ok = ProtocolAddInstance(pstCfg);
        if (u16Ok == 0U) {
            s_u32InitErrors++;
        }
    }
}

Uint16 AppModbusEnabledCount(void)
{
    return ProtocolInstanceCount();
}

Uint32 AppModbusInitErrors(void)
{
    return s_u32InitErrors;
}

Uint16 AppModbusUpgradeFlag(Uint16 u16Port)
{
    if (u16Port >= APP_MB_PORTS) {
        return 0U;
    }

    return (s_astPort[u16Port].stCtl.stMdsUpdateParam.u16MdsIsUpdate != 0U) ? 1U : 0U;
}

Uint32 AppModbusRxFrames(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32RxFrames : 0UL;
}

Uint32 AppModbusTxFrames(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32TxFrames : 0UL;
}

Uint32 AppModbusRxDropped(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32RxDropped : 0UL;
}

const ST_PROTO_INSTANCE *AppModbusInstance(Uint16 u16Index)
{
    return ProtocolInstance(u16Index);
}

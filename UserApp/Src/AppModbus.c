/**
 * @file AppModbus.c
 * @brief Modbus RTU 从机应用层：三路 SCI 从站实例 + 参考协议层与端口层的桥接。
 * @author yetingheng (004182)
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 *
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
#include "AppUpgrade.h"
#include "Modbus.h"
#include "ModbusSlave.h"
#include "ModbusPublic.h"
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
#define APP_MB_BATCH_MAX       (8U)    /* 端口层一条批次记录最多 8 字节 */

/* 3.5 字符静默：≤19200 时按 38.5 位/波特率折算，>19200 按规范取固定 1750 us。 */
#define APP_MB_HIGH_BAUD       (19200UL)
#define APP_MB_T35_FIXED_US    (1750UL)
#define APP_MB_US_CHAR35_NUM   (38500000UL)

/* 升级握手：上位机在 ARM 端口（SCI-A）上发 01 10 04 38 00 02 这六个字节——
 * 地址 1、功能码 0x10、起始地址 1080、数量 2，没有数据段也没有 CRC。
 * 这是与上位机约定的兼容入口，不是标准 Modbus 请求，所以由本文件直接识别；
 * 交给参考层只会被当成“声称写 2 个寄存器却没带数据”的错误帧。 */
#define APP_MB_UPGRADE_ADDR    (1080U)
#define APP_MB_UPGRADE_NUMS    (2U)
#define APP_MB_CMD_WRITE_MULTI (0x10U)
#define APP_MB_BROADCAST_ADDR  (0x00U)
#define APP_MB_HANDSHAKE_LEN   (6U)    /* 握手请求长度 */
#define APP_MB_HANDSHAKE_REPLY (8U)    /* 6 字节回显 + CRC 2；'C' 由升级层擦完备份区后自己发 */
#define APP_MB_HANDSHAKE_TAIL  (64U)   /* 握手之后紧跟的升级流字节，原样交给升级层 */

/* 旧 ARM 兼容入口（docs/review/steps/06a-legacy-arm.md）：
 *   19200：ARM 发 5 字节 F0 00 12 03 04，本层回 F0 00 12 05 06；应答发完后切到 115200。
 *   115200：ARM 发 01 10 04 38 00 02 开头的传输请求，回 8 字节；回复发完后才把端口交给
 *           升级层（擦备份区 → 写"接收中"配置 → 发 'C'），所以准备失败时不会发 'C'。
 * 任何一步失败或被超时兜住，都恢复 19200 并把端口还给 Modbus。 */
#define APP_MB_LEGACY_CMD_LEN   (5U)
#define APP_MB_LEGACY_REPLY_LEN (5U)
#define APP_MB_BAUD_NORMAL      (19200UL)
#define APP_MB_BAUD_UPGRADE     (115200UL)
#define APP_MB_LEGACY_WAIT_MS   (5000UL)   /* 等 115200 传输请求的上界，按 ARM 重发间隔留余量 */

#define APP_MB_LEGACY_NONE      (0U)
#define APP_MB_LEGACY_F0_TX     (1U)   /* F0 应答正在发 */
#define APP_MB_LEGACY_WAIT_REQ  (2U)   /* 已切到 115200，等 01 10 请求 */
#define APP_MB_LEGACY_REQ_TX    (3U)   /* 传输应答正在发，发完交接升级 */

static const Uint16 s_au16LegacyCmd[APP_MB_LEGACY_CMD_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x03U, 0x04U
};
static const Uint16 s_au16LegacyReply[APP_MB_LEGACY_REPLY_LEN] =
{
    0xF0U, 0x00U, 0x12U, 0x05U, 0x06U
};

/** 一个从站实例的全部现场：参考层控制块 + 它要的数据空间 + 桥接状态。 */
typedef struct {
    ST_MODBUS_SLAVE_TABLE_CRTL stCtl;
    ST_MDS_DATA stMds;
    Uint16 *pu16Data;                 /* 该端口的数据空间：请求进、回复出同一块 */
    Uint16 u16RxLen;                  /* 已积累的请求字节数 */
    Uint16 u16TxLen;                  /* 待发回复总长；0 = 没有待发回复 */
    Uint16 u16TxOff;                  /* 已交给端口环的回复字节数 */
    Uint16 u16Armed;                  /* 升级通道已交接：本端口不再收新请求 */
    Uint16 bArmPending;               /* 握手回复已挂上发送侧，等它真正发完 */
    Uint16 u16Legacy;                 /* 旧 ARM 兼容状态（见 APP_MB_LEGACY_*） */
    Uint32 u32LegacyTick;             /* 进入 115200 等待的时刻 */
    Uint16 u16FaultGen;               /* 已处理的故障代次 */
    Uint32 u32RxTick;                 /* 最后一批请求字节被观察到的时刻 */
    Uint32 u32RxFrames;
    Uint32 u32TxFrames;
    Uint32 u32RxDropped;
    Uint32 u32TxDropped;              /* 因故障/复位被取消的未发完回复数 */
    Uint32 u32CallBacks;              /* 参考层业务回调次数（诊断用） */
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
/* 握手候选里超出手握手长度的字节属于升级流，原样交给升级层 */
static Uint16 s_au16HandshakeTail[APP_MB_HANDSHAKE_TAIL];
static Uint16 s_u16HandshakeTailLen;

static const Uint16 s_au16SlaveAddr[APP_MB_PORTS] = { 1U, 1U, 2U };

static Uint32 s_au32Baud[APP_MB_PORTS];
static Uint32 s_u32InitErrors = 0UL;

static void AppModbusCallBack_A(void);
static void AppModbusCallBack_C(void);
static void AppModbusCallBack_D(void);
static const ModbusCallBack s_apfnCallBack[APP_MB_PORTS] = {
    AppModbusCallBack_A, AppModbusCallBack_C, AppModbusCallBack_D
};

static Uint16 AppMbFrameIdle(const ST_APP_MB_PORT *pstPort);
static void   AppMbDispatchFrame(ST_APP_MB_PORT *pstPort);
static Uint16 AppMbIsLegacyCmd(const ST_APP_MB_PORT *pstPort, Uint16 u16Len);
static void   AppMbLegacyRestore(Uint16 u16SciId, ST_APP_MB_PORT *pstPort);
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
 * @brief 参考层处理完一帧后回调：本层只做计数。
 *
 * 升级交接不再由“写 1080 成功”触发：1080/1081 不在协议点表里，这一帧必然被参考层判成
 * 非法地址，靠它置标志只会让一次失败的写也进升级。现在由 AppMbDispatchFrame()
 * 直接识别六字节兼容入口，并在握手回复真正发完之后才交接。
 */
static void AppMbUpgradeWatch(Uint16 u16Port)
{
    s_astPort[u16Port].u32CallBacks++;
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

/** 丢掉未发完的回复：故障/强制复位之后不续发旧帧尾巴、不自动重发。 */
static void AppMbDropTx(ST_APP_MB_PORT *pstPort)
{
    if ((pstPort->u16TxLen != 0U) || (pstPort->bArmPending != 0U)) {
        pstPort->u32TxDropped++;
    }

    pstPort->u16TxLen = 0U;
    pstPort->u16TxOff = 0U;
    pstPort->bArmPending = 0U;
}

/**
 * @brief 把端口的批次拼进数据空间。
 *
 * PopBatch 返回 0 有三种含义（无记录 / 记录装不下 / 环读失败），调用方无法区分。
 * 所以只有“剩余空间能装下一条完整批次”时，0 才等于“取空”；装不下时还把 0 当
 * 取空，就会把半帧当整帧交给协议层。
 *
 * 帧边界只看两处真实静默，不看“本轮有没有取到新字节”（任务节拍不是帧间隔）：
 *   1. 相邻两条批次之间已经隔了 3.5 字符：前一条候选自成一帧，先交出去，
 *      新批次另起一帧。批次排队的间隔为 0 时仍然拼接，交给 CRC 判。
 *   2. 端口取空之后，最后一条批次的时标距现在已有 3.5 字符（见 AppMbFrameIdle）。
 *
 * @return 0 = 还有积压、或候选已被丢弃（本轮不要确认帧）
 *         1 = 端口已取空，可以按静默判帧结束
 */
static Uint16 AppMbDrainRx(ST_APP_MB_PORT *pstPort, Uint16 u16SciId)
{
    Uint16 au16Tmp[APP_MB_BATCH_MAX];
    Uint32 u32GapTicks;
    Uint16 u16Chunk;
    Uint16 u16Total = 0U;
    Uint16 u16Got;
    Uint16 u16Room;
    Uint16 i;

    u32GapTicks = Hw_TimeBaseUsToTicks(
        AppMbT35Us(s_au32Baud[(Uint16)(pstPort - &s_astPort[0])]));

    for (u16Chunk = 0U; u16Chunk < APP_MB_RX_CHUNKS; u16Chunk++) {
        ST_SCI_RX_SEG stSeg;

        u16Room = (Uint16)(APP_MB_DATA_MAX - pstPort->u16RxLen);
        if (u16Room < APP_MB_BATCH_MAX) {
            /* 剩余空间装不下一条完整批次：这一帧已经超过 Modbus 请求的
             * 最长长度。丢掉候选并让端口层重同步，不能拿半截去解析。 */
            pstPort->u32RxDropped = pstPort->u32RxDropped + (Uint32)pstPort->u16RxLen;
            pstPort->u16RxLen = 0U;
            SciPortResync(u16SciId);
            return 0U;
        }

        u16Got = SciPortPopBatch(u16SciId, &au16Tmp[0], APP_MB_BATCH_MAX, &stSeg);
        if (u16Got == 0U) {
            /* 装得下一条完整批次却取不到：可能真的取空了，也可能是环读失败、
             * 断流或正在重同步——这三种情况 PopBatch 同样返回 0。所以“取空”
             * 必须复查端口状态，把 0 直接当成帧结束会把积压丢在后面。 */
            if ((SciPortIsStreamBreak(u16SciId) != 0U) ||
                (SciPortRxState(u16SciId) != SCI_RX_IDLE)) {
                AppMbDropRx(pstPort);
                return 0U;
            }
            if (SciPortFaultGeneration(u16SciId) != pstPort->u16FaultGen) {
                AppMbDropRx(pstPort);
                pstPort->u16FaultGen = SciPortFaultGeneration(u16SciId);
                return 0U;
            }
            return 1U;                        /* 确实取空 */
        }

        /* 批次之间已经出现帧间隔：前一条候选到此为止，先交给协议层。 */
        if ((pstPort->u16RxLen != 0U) && (Hw_TimeBaseIsReady() != 0U) &&
            ((Uint32)(pstPort->u32RxTick - stSeg.u32Tick) >= u32GapTicks)) {
            AppMbDispatchFrame(pstPort);

            /* 交出候选之后，这块数据空间里放的是上一条的回复，不再是收帧缓冲。
             * 刚弹出来的批次已经没有第二块地方安放，只能丢弃并计数；等回复全部
             * 进到端口环（服务流程第 2 步）之后才恢复收帧。若这一帧根本没产生
             * 回复（广播、CRC 错、地址不符），缓冲仍归收帧所有，可以继续拼。 */
            if (pstPort->u16TxLen != 0U) {
                pstPort->u32RxDropped = pstPort->u32RxDropped + (Uint32)u16Got;
                return 0U;
            }
        }

        for (i = 0U; i < u16Got; i++) {
            pstPort->pu16Data[pstPort->u16RxLen + i] = au16Tmp[i];
        }
        pstPort->u16RxLen = (Uint16)(pstPort->u16RxLen + u16Got);
        pstPort->u32RxTick = stSeg.u32Tick;
        u16Total = (Uint16)(u16Total + u16Got);

        if (u16Total >= APP_MB_RX_BYTES) {
            return 0U;                        /* 字节配额用尽：端口里可能还压着这一帧的后半段 */
        }
    }

    return 0U;                                /* 段数配额用尽，可能还有积压 */
}


/**
 * @brief 3.5 字符静默到了没有。
 *
 * 只用批次时标算真实静默。时钟不可信时不能确认帧：“本轮没新字节
 * 就算静默”把任务节拍当成了帧间隔，一个 1 ms 周期里拆成两段的帧会被提前收起来。
 */
static Uint16 AppMbFrameIdle(const ST_APP_MB_PORT *pstPort)
{
    Uint16 u16Port = (Uint16)(pstPort - &s_astPort[0]);

    if (Hw_TimeBaseIsReady() == 0U) {
        return 0U;
    }

    return (Hw_TimeBaseElapsed(pstPort->u32RxTick) >=
            Hw_TimeBaseUsToTicks(AppMbT35Us(s_au32Baud[u16Port]))) ? 1U : 0U;
}

/** 六字节升级握手：01 10 04 38 00 02，只认 ARM 端口（SCI-A）。 */
/**
 * @brief 是否为旧 ARM 的 5 字节 F0 命令（只认 SCI-A、长度必须恰好 5 字节）
 */
static Uint16 AppMbIsLegacyCmd(const ST_APP_MB_PORT *pstPort, Uint16 u16Len)
{
    Uint16 i;

    if ((pstPort != &s_astPort[0]) || (u16Len != APP_MB_LEGACY_CMD_LEN)) {
        return 0U;
    }

    for (i = 0U; i < APP_MB_LEGACY_CMD_LEN; i++) {
        if (pstPort->pu16Data[i] != s_au16LegacyCmd[i]) {
            return 0U;
        }
    }

    return 1U;
}


/**
 * @brief 把兼容应答挂到发送侧（内容原样，不追加 CRC）
 */
static void AppMbLegacyReply(ST_APP_MB_PORT *pstPort, const Uint16 *pu16Src, Uint16 u16Len)
{
    Uint16 i;

    for (i = 0U; i < u16Len; i++) {
        pstPort->pu16Data[i] = pu16Src[i];
    }

    pstPort->u16RxLen = 0U;
    pstPort->u16TxLen = u16Len;
    pstPort->u16TxOff = 0U;
}


/**
 * @brief 兼容流程收尾：SCI-A 恢复 19200、清残留、把端口还给 Modbus
 */
static void AppMbLegacyRestore(Uint16 u16SciId, ST_APP_MB_PORT *pstPort)
{
    Uint16 u16Port;

    u16Port = (Uint16)(pstPort - &s_astPort[0]);

    (void)Hw_SciSetBaud(u16SciId, APP_MB_BAUD_NORMAL);
    s_au32Baud[u16Port] = APP_MB_BAUD_NORMAL;

    pstPort->u16Legacy = APP_MB_LEGACY_NONE;
    pstPort->bArmPending = 0U;
    pstPort->u16Armed = 0U;

    AppMbDropRx(pstPort);
    AppMbDropTx(pstPort);
    SciPortResync(u16SciId);
    pstPort->u16FaultGen = SciPortFaultGeneration(u16SciId);
    pstPort->u32RxTick = Hw_TimeBaseTicks();
}


static Uint16 AppMbIsHandshake(const ST_APP_MB_PORT *pstPort, Uint16 u16Len)
{
    const Uint16 *pu16Req = pstPort->pu16Data;

    /* 前缀匹配：主机可能在握手的同一批数据里紧接着发 SOH */
    if ((pstPort != &s_astPort[0]) || (u16Len < APP_MB_HANDSHAKE_LEN)) {
        return 0U;
    }

    if ((pu16Req[0] != s_au16SlaveAddr[0]) ||
        (pu16Req[1] != APP_MB_CMD_WRITE_MULTI) ||
        (pu16Req[2] != HI_UINT16(APP_MB_UPGRADE_ADDR)) ||
        (pu16Req[3] != LO_UINT16(APP_MB_UPGRADE_ADDR)) ||
        (pu16Req[4] != HI_UINT16(APP_MB_UPGRADE_NUMS)) ||
        (pu16Req[5] != LO_UINT16(APP_MB_UPGRADE_NUMS))) {
        return 0U;
    }

    return 1U;
}

/**
 * @brief 把攒够的一条请求帧交给参考层，并把回复挂到发送侧。
 */
static void AppMbDispatchFrame(ST_APP_MB_PORT *pstPort)
{
    ST_MDS_FRAME *pstFrame;
    Uint16 u16ReqLen;
    Uint16 u16ReqAddr;
    Uint16 i;

    u16ReqLen = pstPort->u16RxLen;
    u16ReqAddr = pstPort->pu16Data[0];   /* 组帧后会被回复覆盖，先记下 */

    /* 旧 ARM 兼容：19200 的 F0 命令与 115200 的等待态都在这里分流，
     * 普通 Modbus 帧不参与这两个分支。 */
    if (AppMbIsLegacyCmd(pstPort, u16ReqLen) != 0U) {
        AppMbLegacyReply(pstPort, &s_au16LegacyReply[0], APP_MB_LEGACY_REPLY_LEN);
        pstPort->u16Legacy = APP_MB_LEGACY_F0_TX;
        return;
    }

    if (pstPort->u16Legacy == APP_MB_LEGACY_WAIT_REQ) {
        if (AppMbIsHandshake(pstPort, u16ReqLen) != 0U) {
            Uint16 u16TailLen2;

            u16TailLen2 = (Uint16)(u16ReqLen - APP_MB_HANDSHAKE_LEN);
            if (u16TailLen2 > APP_MB_HANDSHAKE_TAIL) {
                u16TailLen2 = APP_MB_HANDSHAKE_TAIL;
            }
            s_u16HandshakeTailLen = u16TailLen2;
            for (i = 0U; i < u16TailLen2; i++) {
                s_au16HandshakeTail[i] = pstPort->pu16Data[APP_MB_HANDSHAKE_LEN + i];
            }

            PackModbusCrc(&pstPort->pu16Data[0], APP_MB_HANDSHAKE_LEN);
            pstPort->u16RxLen = 0U;
            pstPort->u32RxFrames++;
            pstPort->u16TxLen = APP_MB_HANDSHAKE_REPLY;
            pstPort->u16TxOff = 0U;
            pstPort->u16Legacy = APP_MB_LEGACY_REQ_TX;
            return;
        }

        /* 115200 上除了传输请求，其余字节都不回应，只丢弃候选继续等 */
        AppMbDropRx(pstPort);
        return;
    }

    /* 升级握手是兼容入口，不是标准 Modbus 请求：原样回去 6 字节 + 现算 CRC，
     * 共 8 字节。'C' 由升级层在备份区擦完、准备好接收之后自己发，所以这里
     * 只把 8 字节应答挂出去，等它真的发完（见服务第 3 步）才交接端口。 */
    if (AppMbIsHandshake(pstPort, u16ReqLen) != 0U) {
        Uint16 u16TailLen;

        u16TailLen = (Uint16)(u16ReqLen - APP_MB_HANDSHAKE_LEN);
        if (u16TailLen > APP_MB_HANDSHAKE_TAIL) {
            u16TailLen = APP_MB_HANDSHAKE_TAIL;
        }
        s_u16HandshakeTailLen = u16TailLen;
        for (i = 0U; i < u16TailLen; i++) {
            s_au16HandshakeTail[i] = pstPort->pu16Data[APP_MB_HANDSHAKE_LEN + i];
        }

        PackModbusCrc(&pstPort->pu16Data[0], APP_MB_HANDSHAKE_LEN);
        pstPort->u16RxLen = 0U;
        pstPort->u32RxFrames++;
        pstPort->u16TxLen = APP_MB_HANDSHAKE_REPLY;
        pstPort->u16TxOff = 0U;
        pstPort->bArmPending = 1U;
        return;
    }

    pstPort->stMds.pu16DataBuff = &pstPort->pu16Data[0];
    pstPort->stMds.u16DataLenth = u16ReqLen;

    pstFrame = ModbusSlaveHandle(&pstPort->stCtl, &pstPort->stMds);
    pstPort->u16RxLen = 0U;
    pstPort->u32RxFrames++;

    if (pstFrame == (ST_MDS_FRAME *)0) {
        return;   /* 地址不匹配 / 帧太短 / CRC 错：不回复 */
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
    AppMbDropTx(pstPort);
}

static Uint16 AppMbProtoService(ST_PROTO_INSTANCE *pstInst)
{
    ST_APP_MB_PORT *pstPort;
    Uint16 u16SciId;
    Uint16 u16Gen;
    Uint16 u16Drained;

    if ((pstInst == (ST_PROTO_INSTANCE *)0) || (pstInst->pCtx == (void *)0)) {
        return 0U;
    }

    pstPort = (ST_APP_MB_PORT *)pstInst->pCtx;
    u16SciId = pstInst->u16SciId;

    /* 1) 故障代次变了（断流、强制复位都算）：丢候选、丢未发完的回复。
     *    旧帧的尾巴不能续发，也不能自动重发。 */
    u16Gen = SciPortFaultGeneration(u16SciId);
    if (u16Gen != pstPort->u16FaultGen) {
        pstPort->u16FaultGen = u16Gen;
        AppMbDropRx(pstPort);
        AppMbDropTx(pstPort);
    }

    /* 2) 上一帧的回复还没被端口环全部接纳：先推它，本轮不接新帧。
     *    数据空间同时是回复缓冲，所以这里必须先收手，不能边发边收。 */
    if (pstPort->u16TxOff < pstPort->u16TxLen) {
        AppMbFlushReply(pstPort, u16SciId);
        if (pstPort->u16TxOff < pstPort->u16TxLen) {
            return 1U;
        }
    }

    /* 2b) 旧 ARM 兼容状态机（只在 SCI-A 上跑） */
    if (pstPort->u16Legacy != APP_MB_LEGACY_NONE) {
        Uint16 u16PortIdx = (Uint16)(pstPort - &s_astPort[0]);

        if (pstPort->u16Legacy == APP_MB_LEGACY_F0_TX) {
            /* F0 应答必须真的上线之后才能切速，否则 ARM 收不到应答 */
            if (SciPortIsTxIdle(u16SciId) == 0U) {
                return 1U;
            }

            if (Hw_SciSetBaud(u16SciId, APP_MB_BAUD_UPGRADE) == 0U) {
                AppMbLegacyRestore(u16SciId, pstPort);
                return 1U;
            }

            s_au32Baud[u16PortIdx] = APP_MB_BAUD_UPGRADE;
            AppMbDropRx(pstPort);      /* 只清 19200 阶段的残留，别把高速首帧一起清掉 */
            pstPort->u16Legacy = APP_MB_LEGACY_WAIT_REQ;
            pstPort->u32LegacyTick = Hw_TimeBaseTicks();
            return 1U;
        }

        if (pstPort->u16Legacy == APP_MB_LEGACY_REQ_TX) {
            if (SciPortIsTxIdle(u16SciId) == 0U) {
                return 1U;
            }

            /* 传输应答发完才交接：升级层从这里开始擦备份区、写"接收中"配置，再发 'C' */
            if (AppUpgradeStart(u16SciId, APP_MB_BAUD_UPGRADE,
                                &s_au16HandshakeTail[0], s_u16HandshakeTailLen) == 0U) {
                AppMbLegacyRestore(u16SciId, pstPort);
                return 1U;
            }

            s_u16HandshakeTailLen = 0U;
            pstPort->u16Legacy = APP_MB_LEGACY_NONE;
            pstPort->u16Armed = 1U;
            return 1U;
        }

        /* WAIT_REQ：有界等待，超时就恢复 19200 交回 Modbus */
        if ((Hw_TimeBaseIsReady() != 0U) &&
            (Hw_TimeBaseElapsed(pstPort->u32LegacyTick) >=
             Hw_TimeBaseUsToTicks(APP_MB_LEGACY_WAIT_MS * 1000UL))) {
            AppMbLegacyRestore(u16SciId, pstPort);
            return 1U;
        }
    }

    /* 3) 握手回复真正发完了才交接升级：此后该端口不再收新请求。 */
    if (pstPort->bArmPending != 0U) {
        if (SciPortIsTxIdle(u16SciId) == 0U) {
            return 1U;
        }

        pstPort->bArmPending = 0U;

        /* 必要修正：只有升级层真正接下这个端口才能停止 Modbus 收帧。
         * （例如 Flash API 没初始化好时 AppUpgradeStart() 返回 0，）否则端口会停在
         * 无人接收的状态。接交失败就丢弃握手待发状态并重新同步流。 */
        if (AppUpgradeStart(u16SciId, s_au32Baud[(Uint16)(pstPort - &s_astPort[0])],
                            &s_au16HandshakeTail[0], s_u16HandshakeTailLen) == 0U)
        {
            s_u16HandshakeTailLen = 0U;
            pstPort->u32TxDropped++;
            SciPortResync(u16SciId);
            pstPort->u16FaultGen = SciPortFaultGeneration(u16SciId);
            pstPort->u32RxTick = Hw_TimeBaseTicks();
            return 1U;
        }

        pstPort->u16Armed = 1U;
    }
    if (pstPort->u16Armed != 0U) {
        return 1U;   /* 升级通道已交接：端口归上位机，协议层不再收帧 */
    }

    /* 4) 端口不在正常接收态：半帧作废。 */
    if (SciPortRxState(u16SciId) != SCI_RX_IDLE) {
        AppMbDropRx(pstPort);
        return 1U;
    }

    /* 5) 断流确认：端口层报断流时先确认掉，否则新字节会被拼到旧候选后面。 */
    if (SciPortIsStreamBreak(u16SciId) != 0U) {
        AppMbDropRx(pstPort);
        (void)SciPortClearStreamBreak(u16SciId, u16Gen);
        return 1U;
    }

    /* 6) 发送侧被强制复位冻结：解冻前既不续发旧帧，也不接新帧。 */
    if (SciPortIsTxSuspended(u16SciId) != 0U) {
        AppMbDropTx(pstPort);
        if (SciPortTxResume(u16SciId) == 0U) {
            return 1U;
        }
    }

    /* 7) 时间基准不可用时不确认任何帧：批次时标与静默判据都基于同一个
     *    时基，时基坏了就没有可信的帧边界。 */
    if (Hw_TimeBaseIsReady() == 0U) {
        return 1U;
    }

    /* 8) 取批次。批次之间若已隔了 3.5 字符，函数内部会把前一条候选先交出去。 */
    u16Drained = AppMbDrainRx(pstPort, u16SciId);

    /* 9) 帧边界：端口取空、并且最后一条批次之后已经静默到 3.5 字符，才收帧。
     *    还压着后半段、或者静默不够，都不能交出去。 */
    if ((pstPort->u16RxLen == 0U) || (u16Drained == 0U)) {
        return 1U;
    }
    if (AppMbFrameIdle(pstPort) == 0U) {
        return 1U;
    }

    /* 10) 一帧攒够了：交给参考层，再把回复挂上发送侧。 */
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
        pstPort->u32TxDropped = 0UL;
        pstPort->u32CallBacks = 0UL;
        pstPort->u16Armed = 0U;
        pstPort->bArmPending = 0U;
        pstPort->u16Legacy = APP_MB_LEGACY_NONE;
        pstPort->u32LegacyTick = 0UL;
        /* 初始化到当前代次：启动本身不能被当成一次故障 */
        pstPort->u16FaultGen = SciPortFaultGeneration(s_au16SciId[u16Port]);
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

    return (s_astPort[u16Port].u16Armed != 0U) ? 1U : 0U;
}

/**
 * @brief 升级结束（成功已复位、或取消）后把端口交还协议层：丢弃半帧与未发回复，
 *        重新同步接收，并让协议层从"当前故障代次"重新开始记账。
 */
void AppModbusReleasePort(Uint16 u16Port)
{
    ST_APP_MB_PORT *pstPort;
    Uint16 u16SciId;

    if (u16Port >= APP_MB_PORTS) {
        return;
    }

    pstPort = &s_astPort[u16Port];
    u16SciId = s_au16SciId[u16Port];

    AppMbDropRx(pstPort);
    AppMbDropTx(pstPort);
    pstPort->u16Armed = 0U;
    pstPort->bArmPending = 0U;
    pstPort->u32RxTick = Hw_TimeBaseTicks();
    SciPortResync(u16SciId);
    pstPort->u16FaultGen = SciPortFaultGeneration(u16SciId);
}

Uint32 AppModbusRxFrames(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32RxFrames : 0UL;
}

Uint32 AppModbusTxFrames(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32TxFrames : 0UL;
}

Uint32 AppModbusTxDropped(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32TxDropped : 0UL;
}

Uint32 AppModbusCallBacks(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32CallBacks : 0UL;
}

Uint32 AppModbusRxDropped(Uint16 u16Port)
{
    return (u16Port < APP_MB_PORTS) ? s_astPort[u16Port].u32RxDropped : 0UL;
}

const ST_PROTO_INSTANCE *AppModbusInstance(Uint16 u16Index)
{
    return ProtocolInstance(u16Index);
}

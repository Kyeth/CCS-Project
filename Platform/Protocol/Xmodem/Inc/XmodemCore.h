/******************************************************************************
 * @file    XmodemCore.h
 * @brief   XMODEM 接收协议核心（从参考工程移植，适配 SciPort 非阻塞传输层）
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    移植依据：参考工程 git 提交 68701bd 的
 *          Platform/Xmodem/{Inc/XmodemCore.h, Src/XmodemCore.c}。
 *          本机可用的副本与其存在差异（见 docs/review/steps/05f-xmodem-app.md §2.2），
 *          差异处按 68701bd 的结论实现：
 *          - 接收长度用 Uint32（参考旧副本是 Uint16，超过 64KB 会回绕）；
 *          - 读写回调由业务层绑定（参考旧副本传 NULL）；
 *          - WriteBlock 参与真实数据搬移（参考旧副本里该调用被注释）。
 *
 * @note    与参考实现的差异（本工程有意为之）：
 *          1. 传输层无阻塞：pfRead 返回“本次能取到的字节数”，0 表示暂无数据；
 *             pfWrite 返回 1 表示已入环、0 表示端口忙（必须下次重试，不能当作已发送）。
 *          2. 超时由业务层注入 pfNowMs（本工程用 Hw_TimeBase），协议层不直接碰硬件。
 *          3. 包序号按 8 位自然回绕（255 → 0 → 1）：参考实现用
 *             "if ((no == 0) || (no > 255)) no = 1;" 跳过了 0，与 XMODEM 规范不符。
 *          4. XmodemTransmit() 不移植：参考工程里它是空函数，本项目只做接收。
 ******************************************************************************/

#ifndef XMODEMCORE_H
#define XMODEMCORE_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 常量 ---------------- */

/** 包缓冲字数：SOH 包 3+128+2 = 133，STX 包 3+1024+2 = 1029，每个字节占一个 Uint16 */
#define XMODEM_BUFF_SIZE            (1030U)

/** 文件头长度（首个数据包的正文前 32 字节） */
#define XMODEM_FILE_HEAD_LEN        (32U)

/** 控制字符 */
#define XMODEM_SOH                  (0x01U)
#define XMODEM_STX                  (0x02U)
#define XMODEM_EOT                  (0x04U)
#define XMODEM_ACK                  (0x06U)
#define XMODEM_NAK                  (0x15U)
#define XMODEM_CAN                  (0x18U)
#define XMODEM_CTRLZ                (0x1AU)

/** 包结构长度 */
#define XMODEM_PACK_HEAD_LEN        (3U)     /* SOH/STX + 序号 + 序号反码 */
#define XMODEM_PACK_TAIL_LEN        (2U)     /* CRC16，高字节在前 */
#define XMODEM_PACK_SOH_DATA_LEN    (128U)
#define XMODEM_PACK_STX_DATA_LEN    (1024U)

/** 默认参数（与参考工程 XmodemSlaveProcess.c 一致） */
#define XMODEM_DEFAULT_MAX_RETRY    (10U)
#define XMODEM_DEFAULT_TIMEOUT_MS   (500U)   /* 单包内字节间隔超时 */
#define XMODEM_DEFAULT_START_MS     (2000U)  /* 等首包的整段超时 */
#define XMODEM_CAN_CONFIRM_NUM      (2U)     /* 连续两个 CAN 才算取消 */
#define XMODEM_EOT_CONFIRM_NUM      (2U)     /* 两次 EOT 才算结束 */

/* ---------------- 类型 ---------------- */

typedef enum
{
    XMODEM_OK = 0,
    XMODEM_TIMEOUT,
    XMODEM_CRC_ERROR,
    XMODEM_SEQUENCE_ERROR,
    XMODEM_PACKET_ERROR,
    XMODEM_EOT_ERROR,
    XMODEM_CAN_ERROR,
    XMODEM_CANCEL,
    XMODEM_END,
    XMODEM_RETRY_ERROR,
    XMODEM_IO_ERROR,
    XMEDEM_PARAM_ERROR
} E_XMODEM_ERROR;

typedef enum
{
    XMODEM_STATE_IDLE = 0,      /* 未开始 */
    XMODEM_STATE_WAIT_START,    /* 已发 'C'，等首包 */
    XMODEM_STATE_RECEIVE,       /* 收包中 */
    XMODEM_STATE_FINISH,        /* 收到第一次 EOT，等第二次 */
    XMODEM_STATE_COMPLETE,      /* 正常结束或已取消 */
    XMODEM_STATE_ERROR          /* 重试超限 */
} E_XMODEM_STATE;

typedef struct
{
    Uint16 u16IsUseCrc;         /* 1 = 用 CRC16，0 = 用校验和 */
    Uint16 u16MaxRetries;
    Uint16 u16TimeOut;          /* 单包内字节间隔超时（ms） */
    Uint16 u16StartTimeOut;     /* 等首包超时（ms） */
} ST_XMODEM_CONFIG;

/** 传输层：读写都必须是非阻塞的 */
typedef struct
{
    /** 取字节，返回实际取到的个数，0 表示暂无数据 */
    Uint16 (*pfRead)(Uint16 *pu16Buff, Uint16 u16Max);
    /** 发字节，返回 1 = 已进发送环，0 = 端口忙（下次重试） */
    Uint16 (*pfWrite)(const Uint16 *pu16Buff, Uint16 u16Count);
} ST_XMODEM_TRANSPORT;

/** 数据层：把一包正文交给业务层 */
typedef struct
{
    /**
     * @brief  写一包正文
     * @param  u16PackNo  包序号（1..255、0）
     * @param  pu16Buff   正文首地址（每个字节占一个 Uint16）
     * @param  u16Length  正文长度
     * @return >=0 实际接收的字数；<0 写失败
     */
    int16  (*pfWriteBlock)(Uint16 u16PackNo, Uint16 *pu16Buff, Uint16 u16Length);
} ST_XMODEM_DATA_HANDLER;

/** 时间源：本工程注入 Hw_TimeBase 的毫秒计数 */
typedef struct
{
    Uint32 (*pfNowMs)(void);
} ST_XMODEM_TIMEOUT;

typedef struct
{
    ST_XMODEM_CONFIG        stConfig;
    ST_XMODEM_TRANSPORT     stTransport;
    ST_XMODEM_DATA_HANDLER  stDataHandler;
    ST_XMODEM_TIMEOUT       stTimeout;

    E_XMODEM_STATE          stState;
    E_XMODEM_ERROR          eError;

    Uint16                  u16xBuff[XMODEM_BUFF_SIZE];
    Uint16                  u16BuffCount;      /* 已攒到 u16xBuff 的字节数 */
    Uint16                  u16NeedCount;      /* 当前包的完整长度，0 = 还不知道 */

    Uint16                  bCurrentPackNo;    /* 期望的下一个序号 */
    Uint16                  bLastPackNo;       /* 上一个成功写入的序号 */
    Uint16                  u16RetryCount;
    Uint16                  u16EOTCount;
    Uint16                  u16CANCount;

    Uint16                  au16Pending[2];    /* 待入环的控制字符（'C'/ACK/NAK/CAN） */
    Uint16                  u16PendingCount;   /* 待发个数，0 = 没有 */

    Uint32                  u32RecvLength;     /* 已写入的字节总数（不含 32 字节头） */
    Uint32                  u32FrameTick;      /* 本包收到最后一个字节的时刻 */
    Uint32                  u32StartTick;
} ST_XMODEM_PROTOCOL;

/* ---------------- 接口 ---------------- */

/**
 * @brief  初始化协议实例
 * @return XMODEM_OK 或 XMEDEM_PARAM_ERROR
 */
E_XMODEM_ERROR XmodemInit(ST_XMODEM_PROTOCOL *pstProtocol,
                          const ST_XMODEM_CONFIG *pstConfig,
                          const ST_XMODEM_TRANSPORT *pstTransport,
                          const ST_XMODEM_DATA_HANDLER *pstDataHandler,
                          const ST_XMODEM_TIMEOUT *pstTimeout);

/**
 * @brief  复位协议实例（保留回调与配置），用于中止后重新握手
 */
void XmodemReset(ST_XMODEM_PROTOCOL *pstProtocol);

/**
 * @brief  推进状态机一步，由业务层每轮调用一次
 * @note   本函数不阻塞。需要发送的控制字符（'C'、ACK、NAK、CAN）先进入
 *         待发队列，再逐个调 pfWrite；pfWrite 返回 0 表示端口忙，字符留在
 *         队列里，下一轮先补发，之后再推进状态机——"没发出去"绝不能当成
 *         "已发送"。
 * @note   收到第二次 EOT 时本层只把会话标记为完成并返回 XMODEM_END，**不发
 *         ACK**：ACK 表示"已落盘"，必须由业务层在长度/CRC 校验与配置写入都
 *         通过之后再发，端口忙就留到下一轮。
 * @return 本步的错误码（XMODEM_OK 表示正常推进）
 */
E_XMODEM_ERROR XmodemReceive(ST_XMODEM_PROTOCOL *pstProtocol);

/**
 * @brief  本次会话是否已结束（正常结束或取消）
 */
Uint16 XmodemIsComplete(const ST_XMODEM_PROTOCOL *pstProtocol);

#ifdef __cplusplus
}
#endif

#endif

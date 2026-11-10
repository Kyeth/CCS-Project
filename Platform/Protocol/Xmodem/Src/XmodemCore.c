/******************************************************************************
 * @file    XmodemCore.c
 * @brief   XMODEM 接收协议核心
 * @author  yetingheng (004182)
 * @version 1.0
 * @date    2026-11-10
 *
 * @note    包结构：[0]=SOH/STX，[1]=序号，[2]=序号反码，[3..3+n-1]=正文，
 *          [3+n]=CRC 高字节，[3+n+1]=CRC 低字节（校验和模式只有一个字节）。
 *          CRC16-CCITT：poly 0x1021，初值 0，先算高字节（与参考工程一致）。
 *
 * @note    与参考实现的差异：
 *          1. 传输层非阻塞：pfRead 返回本次取到的字节数（0 = 暂无），
 *             pfWrite 返回 1 = 已入环 / 0 = 端口忙。忙时不推进状态机，
 *             下一轮重试——不能把"没发出去"当成"已发送"。
 *          2. 序号按 8 位自然回绕：255 → 0 → 1。参考实现跳过 0，是缺陷。
 *          3. 两次 EOT：第一次回 NAK 让主机重发，第二次回 ACK 并结束
 *             （参考实现两次都回 NAK，与主机侧约定不符）。
 *          4. 写块回调返回值参与判定：<0 表示落盘失败，直接进 ERROR，
 *             由业务层发 CAN 并丢弃本次升级。
 ******************************************************************************/

#include "XmodemCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------- 内部工具 ---------------- */

/**
 * @brief  CRC16-CCITT（poly 0x1021，初值 0，高字节在前）
 * @note   逐位实现，与参考工程查表实现结果一致；每包最多 1029 字节，
 *         8232 次移位在 200MHz 下可忽略。
 */
static Uint16 XmodemCrc16(const Uint16 *pu16Buff, Uint16 u16Length)
{
    Uint16 u16Crc = 0U;
    Uint16 u16Byte;
    Uint16 i;
    Uint16 j;

    for (i = 0U; i < u16Length; i++)
    {
        u16Byte = (Uint16)(pu16Buff[i] & 0x00FFU);
        u16Crc = (Uint16)(u16Crc ^ (Uint16)(u16Byte << 8));
        for (j = 0U; j < 8U; j++)
        {
            if ((u16Crc & 0x8000U) != 0U)
            {
                u16Crc = (Uint16)((Uint16)(u16Crc << 1) ^ 0x1021U);
            }
            else
            {
                u16Crc = (Uint16)(u16Crc << 1);
            }
        }
    }

    return u16Crc;
}

/**
 * @brief  校验和（8 位累加，CRC 模式关闭时使用）
 */
static Uint16 XmodemCheckSum(const Uint16 *pu16Buff, Uint16 u16Length)
{
    Uint16 u16Sum = 0U;
    Uint16 i;

    for (i = 0U; i < u16Length; i++)
    {
        u16Sum = (Uint16)(u16Sum + (pu16Buff[i] & 0x00FFU));
    }

    return (Uint16)(u16Sum & 0x00FFU);
}

/**
 * @brief  正文长度（由包头决定），0 表示不是数据包
 */
static Uint16 XmodemDataLength(Uint16 u16Head)
{
    Uint16 u16Len = 0U;

    if (u16Head == XMODEM_SOH)
    {
        u16Len = XMODEM_PACK_SOH_DATA_LEN;
    }
    else if (u16Head == XMODEM_STX)
    {
        u16Len = XMODEM_PACK_STX_DATA_LEN;
    }
    else
    {
        u16Len = 0U;
    }

    return u16Len;
}

/**
 * @brief  整包长度（含头尾），0 表示不是数据包
 */
static Uint16 XmodemFrameLength(const ST_XMODEM_PROTOCOL *pstProtocol, Uint16 u16Head)
{
    Uint16 u16DataLen = XmodemDataLength(u16Head);
    Uint16 u16TailLen;

    if (u16DataLen == 0U)
    {
        return 0U;
    }

    u16TailLen = (pstProtocol->stConfig.u16IsUseCrc != 0U) ? XMODEM_PACK_TAIL_LEN : 1U;

    return (Uint16)(XMODEM_PACK_HEAD_LEN + u16DataLen + u16TailLen);
}

/**
 * @brief  取当前毫秒
 */
static Uint32 XmodemNowMs(const ST_XMODEM_PROTOCOL *pstProtocol)
{
    if ((pstProtocol != 0) && (pstProtocol->stTimeout.pfNowMs != 0))
    {
        return pstProtocol->stTimeout.pfNowMs();
    }

    return 0U;
}

/**
 * @brief  发一个控制字符
 * @return 1 = 已入环，0 = 端口忙
 */
static Uint16 XmodemSendByte(ST_XMODEM_PROTOCOL *pstProtocol, Uint16 u16Char)
{
    Uint16 u16Byte = u16Char;

    if (pstProtocol->stTransport.pfWrite == 0)
    {
        return 0U;
    }

    return pstProtocol->stTransport.pfWrite(&u16Byte, 1U);
}

/**
 * @brief  把一个控制字符放进待发队列
 * @note   队列满时丢弃最新字符：正常每轮只排 1~2 个字节，队列满说明端口
 *         长期发不出去，由超时计数与重试上限接手处理。
 */
static void XmodemQueueByte(ST_XMODEM_PROTOCOL *pstProtocol, Uint16 u16Char)
{
    if (pstProtocol->u16PendingCount < 2U)
    {
        pstProtocol->au16Pending[pstProtocol->u16PendingCount] = u16Char;
        pstProtocol->u16PendingCount++;
    }
}

/**
 * @brief  把待发队列里的字符尽量发出去
 * @return 1 = 队列已空，0 = 端口忙还有积压
 */
static Uint16 XmodemFlushPending(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint16 i;

    while (pstProtocol->u16PendingCount != 0U)
    {
        if (XmodemSendByte(pstProtocol, pstProtocol->au16Pending[0]) == 0U)
        {
            return 0U;
        }

        for (i = 0U; i < (Uint16)(pstProtocol->u16PendingCount - 1U); i++)
        {
            pstProtocol->au16Pending[i] = pstProtocol->au16Pending[(Uint16)(i + 1U)];
        }
        pstProtocol->u16PendingCount--;
    }

    return 1U;
}

/**
 * @brief  丢掉已攒的字节
 */
static void XmodemDropBuffer(ST_XMODEM_PROTOCOL *pstProtocol)
{
    pstProtocol->u16BuffCount = 0U;
    pstProtocol->u16NeedCount = 0U;
}

/**
 * @brief  把缓冲前 u16Consumed 个字节移走，剩余部分前移
 */
static void XmodemConsumeBuffer(ST_XMODEM_PROTOCOL *pstProtocol, Uint16 u16Consumed)
{
    Uint16 i;

    if (u16Consumed >= pstProtocol->u16BuffCount)
    {
        XmodemDropBuffer(pstProtocol);
        return;
    }

    for (i = 0U; i < (Uint16)(pstProtocol->u16BuffCount - u16Consumed); i++)
    {
        pstProtocol->u16xBuff[i] = pstProtocol->u16xBuff[(Uint16)(i + u16Consumed)];
    }

    pstProtocol->u16BuffCount = (Uint16)(pstProtocol->u16BuffCount - u16Consumed);
    pstProtocol->u16NeedCount = 0U;
}

/**
 * @brief  从传输层补字节
 * @return 本次新收到的字节数
 */
static Uint16 XmodemFillBuffer(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint16 u16Room;
    Uint16 u16Got;

    if (pstProtocol->stTransport.pfRead == 0)
    {
        return 0U;
    }

    u16Room = (Uint16)(XMODEM_BUFF_SIZE - pstProtocol->u16BuffCount);
    if (u16Room == 0U)
    {
        /* 缓冲已满但还拼不出整包：说明对端发的不是合法 XMODEM 包，丢掉重来 */
        XmodemDropBuffer(pstProtocol);
        u16Room = XMODEM_BUFF_SIZE;
    }

    u16Got = pstProtocol->stTransport.pfRead(&pstProtocol->u16xBuff[pstProtocol->u16BuffCount], u16Room);
    if (u16Got != 0U)
    {
        pstProtocol->u16BuffCount = (Uint16)(pstProtocol->u16BuffCount + u16Got);
        pstProtocol->u32FrameTick = XmodemNowMs(pstProtocol);
    }

    return u16Got;
}

/**
 * @brief  校验一包数据
 * @return 1 = 通过
 */
static Uint16 XmodemCheckFrame(const ST_XMODEM_PROTOCOL *pstProtocol, Uint16 u16DataLen)
{
    Uint16 u16Calc;
    Uint16 u16Recv;
    Uint16 u16CrcIndex = (Uint16)(XMODEM_PACK_HEAD_LEN + u16DataLen);

    if (pstProtocol->stConfig.u16IsUseCrc != 0U)
    {
        u16Calc = XmodemCrc16(&pstProtocol->u16xBuff[XMODEM_PACK_HEAD_LEN], u16DataLen);
        u16Recv = (Uint16)(((Uint16)(pstProtocol->u16xBuff[u16CrcIndex] & 0x00FFU) << 8) |
                           (Uint16)(pstProtocol->u16xBuff[(Uint16)(u16CrcIndex + 1U)] & 0x00FFU));
    }
    else
    {
        u16Calc = XmodemCheckSum(&pstProtocol->u16xBuff[XMODEM_PACK_HEAD_LEN], u16DataLen);
        u16Recv = (Uint16)(pstProtocol->u16xBuff[u16CrcIndex] & 0x00FFU);
    }

    return (u16Recv == u16Calc) ? 1U : 0U;
}

/**
 * @brief  处理一个已收齐的包
 * @return 本步错误码
 */
static E_XMODEM_ERROR XmodemHandleFrame(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint16 u16FrameLen;
    Uint16 u16DataLen;
    Uint16 u16PackNo;
    int16  i16Written;
    E_XMODEM_ERROR eRet = XMODEM_OK;

    u16DataLen = XmodemDataLength(pstProtocol->u16xBuff[0]);
    u16FrameLen = XmodemFrameLength(pstProtocol, pstProtocol->u16xBuff[0]);
    u16PackNo = (Uint16)(pstProtocol->u16xBuff[1] & 0x00FFU);

    /* 序号与反码 */
    if ((Uint16)((pstProtocol->u16xBuff[1] + pstProtocol->u16xBuff[2]) & 0x00FFU) != 0x00FFU)
    {
        eRet = XMODEM_PACKET_ERROR;
    }
    else if ((u16PackNo != pstProtocol->bCurrentPackNo) && (u16PackNo != pstProtocol->bLastPackNo))
    {
        eRet = XMODEM_SEQUENCE_ERROR;
    }
    else if (XmodemCheckFrame(pstProtocol, u16DataLen) == 0U)
    {
        eRet = XMODEM_CRC_ERROR;
    }
    else if (u16PackNo == pstProtocol->bLastPackNo)
    {
        /* 重复包：只回 ACK，不重写 */
        eRet = XMODEM_OK;
    }
    else
    {
        i16Written = -1;
        if (pstProtocol->stDataHandler.pfWriteBlock != 0)
        {
            i16Written = pstProtocol->stDataHandler.pfWriteBlock(u16PackNo,
                                                                 &pstProtocol->u16xBuff[XMODEM_PACK_HEAD_LEN],
                                                                 u16DataLen);
        }

        if (i16Written < 0)
        {
            /* 落盘失败：本次升级作废，业务层负责发 CAN */
            pstProtocol->u32RecvLength = 0U;
            pstProtocol->eError = XMODEM_IO_ERROR;
            pstProtocol->stState = XMODEM_STATE_ERROR;
            XmodemConsumeBuffer(pstProtocol, u16FrameLen);
            return XMODEM_IO_ERROR;
        }

        pstProtocol->u32RecvLength = pstProtocol->u32RecvLength + (Uint32)i16Written;
        pstProtocol->bLastPackNo = u16PackNo;
        pstProtocol->bCurrentPackNo = (Uint16)((u16PackNo + 1U) & 0x00FFU);
    }

    XmodemConsumeBuffer(pstProtocol, u16FrameLen);

    if (eRet == XMODEM_OK)
    {
        pstProtocol->u16RetryCount = 0U;
        XmodemQueueByte(pstProtocol, XMODEM_ACK);
    }
    else
    {
        pstProtocol->u16RetryCount++;
        XmodemQueueByte(pstProtocol, XMODEM_NAK);
    }

    return eRet;
}

/**
 * @brief  处理 EOT / CAN（单字节控制符）
 */
static E_XMODEM_ERROR XmodemHandleControl(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint16 u16Char = (Uint16)(pstProtocol->u16xBuff[0] & 0x00FFU);

    if (u16Char == XMODEM_EOT)
    {
        XmodemConsumeBuffer(pstProtocol, 1U);
        pstProtocol->u16EOTCount++;
        if (pstProtocol->u16EOTCount >= XMODEM_EOT_CONFIRM_NUM)
        {
            /* 这里不发 ACK：此时正文只是收完，长度/CRC 还没校验、配置还没写。
             * ACK 是"已落盘"的唯一凭据，必须由业务层校验通过后再发。 */
            pstProtocol->eError = XMODEM_END;
            pstProtocol->stState = XMODEM_STATE_COMPLETE;
            return XMODEM_END;
        }

        /* 第一次 EOT：回 NAK，等主机再发一次 */
        pstProtocol->stState = XMODEM_STATE_FINISH;
        pstProtocol->u32FrameTick = XmodemNowMs(pstProtocol);
        XmodemQueueByte(pstProtocol, XMODEM_NAK);
        return XMODEM_OK;
    }

    if (u16Char == XMODEM_CAN)
    {
        XmodemConsumeBuffer(pstProtocol, 1U);
        pstProtocol->u16CANCount++;
        if (pstProtocol->u16CANCount >= XMODEM_CAN_CONFIRM_NUM)
        {
            XmodemQueueByte(pstProtocol, XMODEM_CAN);
            XmodemQueueByte(pstProtocol, XMODEM_CAN);
            pstProtocol->eError = XMODEM_CANCEL;
            pstProtocol->stState = XMODEM_STATE_COMPLETE;
            return XMODEM_CANCEL;
        }

        XmodemQueueByte(pstProtocol, XMODEM_NAK);
        return XMODEM_OK;
    }

    /* 其他字节：丢弃并 NAK */
    XmodemConsumeBuffer(pstProtocol, 1U);
    pstProtocol->u16RetryCount++;
    XmodemQueueByte(pstProtocol, XMODEM_NAK);

    return XMODEM_PACKET_ERROR;
}

/* ---------------- 对外接口 ---------------- */

E_XMODEM_ERROR XmodemInit(ST_XMODEM_PROTOCOL *pstProtocol,
                          const ST_XMODEM_CONFIG *pstConfig,
                          const ST_XMODEM_TRANSPORT *pstTransport,
                          const ST_XMODEM_DATA_HANDLER *pstDataHandler,
                          const ST_XMODEM_TIMEOUT *pstTimeout)
{
    if ((pstProtocol == 0) || (pstConfig == 0) || (pstTransport == 0) ||
        (pstDataHandler == 0) || (pstTimeout == 0))
    {
        return XMEDEM_PARAM_ERROR;
    }

    if ((pstTransport->pfRead == 0) || (pstTransport->pfWrite == 0) ||
        (pstTimeout->pfNowMs == 0))
    {
        return XMEDEM_PARAM_ERROR;
    }

    pstProtocol->stConfig.u16IsUseCrc = pstConfig->u16IsUseCrc;
    pstProtocol->stConfig.u16MaxRetries = pstConfig->u16MaxRetries;
    pstProtocol->stConfig.u16TimeOut = pstConfig->u16TimeOut;
    pstProtocol->stConfig.u16StartTimeOut = pstConfig->u16StartTimeOut;

    pstProtocol->stTransport.pfRead = pstTransport->pfRead;
    pstProtocol->stTransport.pfWrite = pstTransport->pfWrite;

    pstProtocol->stDataHandler.pfWriteBlock = pstDataHandler->pfWriteBlock;

    pstProtocol->stTimeout.pfNowMs = pstTimeout->pfNowMs;

    XmodemReset(pstProtocol);

    return XMODEM_OK;
}

void XmodemReset(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint16 i;

    if (pstProtocol == 0)
    {
        return;
    }

    pstProtocol->stState = XMODEM_STATE_IDLE;
    pstProtocol->eError = XMODEM_OK;

    for (i = 0U; i < XMODEM_BUFF_SIZE; i++)
    {
        pstProtocol->u16xBuff[i] = 0U;
    }

    pstProtocol->u16BuffCount = 0U;
    pstProtocol->u16NeedCount = 0U;

    pstProtocol->au16Pending[0] = 0U;
    pstProtocol->au16Pending[1] = 0U;
    pstProtocol->u16PendingCount = 0U;

    pstProtocol->bCurrentPackNo = 1U;
    pstProtocol->bLastPackNo = 1U;
    pstProtocol->u16RetryCount = 0U;
    pstProtocol->u16EOTCount = 0U;
    pstProtocol->u16CANCount = 0U;

    pstProtocol->u32RecvLength = 0U;
    pstProtocol->u32FrameTick = 0U;
    pstProtocol->u32StartTick = 0U;
}

E_XMODEM_ERROR XmodemReceive(ST_XMODEM_PROTOCOL *pstProtocol)
{
    Uint32 u32Now = 0U;
    Uint16 u16Need = 0U;
    Uint16 u16Head = 0U;
    E_XMODEM_ERROR eRet = XMODEM_OK;

    if (pstProtocol == 0)
    {
        return XMEDEM_PARAM_ERROR;
    }

    if ((pstProtocol->stState == XMODEM_STATE_COMPLETE) ||
        (pstProtocol->stState == XMODEM_STATE_ERROR))
    {
        return pstProtocol->eError;
    }

    /* 上轮没发出去的控制字符先补发；端口还忙就整轮不推进，避免把
     * "没发出去"当成"已发送"（ACK 丢了主机就会一直等）。 */
    if (XmodemFlushPending(pstProtocol) == 0U)
    {
        return XMODEM_OK;
    }

    u32Now = XmodemNowMs(pstProtocol);

    /* 起始：发 'C'，发不出去就下轮再来 */
    if (pstProtocol->stState == XMODEM_STATE_IDLE)
    {
        if (pstProtocol->u16RetryCount >= pstProtocol->stConfig.u16MaxRetries)
        {
            pstProtocol->eError = XMODEM_RETRY_ERROR;
            pstProtocol->stState = XMODEM_STATE_ERROR;
            return XMODEM_RETRY_ERROR;
        }

        if (XmodemSendByte(pstProtocol, (Uint16)'C') == 0U)
        {
            return XMODEM_OK;
        }

        pstProtocol->u32StartTick = u32Now;
        pstProtocol->stState = XMODEM_STATE_WAIT_START;
        pstProtocol->eError = XMODEM_OK;

        return XMODEM_OK;
    }

    /* 补字节 */
    if (XmodemFillBuffer(pstProtocol) == 0U)
    {
        /* 没有新字节：判超时 */
        if (pstProtocol->stState == XMODEM_STATE_WAIT_START)
        {
            if ((Uint32)(u32Now - pstProtocol->u32StartTick) >= pstProtocol->stConfig.u16StartTimeOut)
            {
                pstProtocol->u16RetryCount++;
                pstProtocol->stState = XMODEM_STATE_IDLE;
                pstProtocol->eError = XMODEM_TIMEOUT;
            }

            return XMODEM_OK;
        }

        if ((Uint32)(u32Now - pstProtocol->u32FrameTick) >= pstProtocol->stConfig.u16TimeOut)
        {
            pstProtocol->u16RetryCount++;
            if (pstProtocol->u16RetryCount > pstProtocol->stConfig.u16MaxRetries)
            {
                XmodemQueueByte(pstProtocol, XMODEM_CAN);
                XmodemQueueByte(pstProtocol, XMODEM_CAN);
                pstProtocol->eError = XMODEM_RETRY_ERROR;
                pstProtocol->stState = XMODEM_STATE_ERROR;
                return XMODEM_RETRY_ERROR;
            }

            XmodemDropBuffer(pstProtocol);
            XmodemQueueByte(pstProtocol, XMODEM_NAK);
            pstProtocol->u32FrameTick = u32Now;
        }

        return XMODEM_OK;
    }

    /* 有字节了：按包头推进 */
    u16Head = (Uint16)(pstProtocol->u16xBuff[0] & 0x00FFU);

    if ((u16Head == XMODEM_EOT) || (u16Head == XMODEM_CAN))
    {
        eRet = XmodemHandleControl(pstProtocol);
    }
    else
    {
        u16Need = XmodemFrameLength(pstProtocol, u16Head);
        if (u16Need == 0U)
        {
            /* 包头不认识：丢一个字节，NAK 让对端重发 */
            XmodemConsumeBuffer(pstProtocol, 1U);
            pstProtocol->u16RetryCount++;
            XmodemQueueByte(pstProtocol, XMODEM_NAK);
            return XMODEM_PACKET_ERROR;
        }

        if (pstProtocol->u16BuffCount >= u16Need)
        {
            eRet = XmodemHandleFrame(pstProtocol);
        }
    }

    if (pstProtocol->stState == XMODEM_STATE_WAIT_START)
    {
        pstProtocol->stState = XMODEM_STATE_RECEIVE;
    }

    return eRet;
}

Uint16 XmodemIsComplete(const ST_XMODEM_PROTOCOL *pstProtocol)
{
    if (pstProtocol == 0)
    {
        return 0U;
    }

    return ((pstProtocol->stState == XMODEM_STATE_COMPLETE) ||
            (pstProtocol->stState == XMODEM_STATE_ERROR)) ? 1U : 0U;
}

#ifdef __cplusplus
}
#endif

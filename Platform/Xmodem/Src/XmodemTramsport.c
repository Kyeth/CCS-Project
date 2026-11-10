#include "XmodemTramsports.h"
#include "XmodemCore.h"   /* SOH/STX/CAN 控制符 */


// 这里需要引入硬件外设头文件
#include "Hw_SciDriver.h"
#include "SciPort.h"
#include "Hw_TimeBase.h"


#ifdef __cplusplus
extern "C" {
#endif



/* 端口适配：升级独占 SCI-A（与 Modbus 握手同路） */
#define XMODEM_SCI_ID       (HW_SCI_ID_A)
#define XMODEM_BATCH_MAX    (8U)
#define XMODEM_IDLE_DROP_US (100000UL)  /* 半包静默超过此值丢弃，等主机重发 */

static Uint16 s_u16FrameFill = 0;       /* 当前这一包已累计的字节数 */

/* 必要适配：这个 64 字的余字节缓存放进升级专用段（RAMGS2），
 * 避免再从已经用满的 RAMGS0 里切一块出来。 */
#pragma DATA_SECTION(s_au16Preload, ".upybuf")
static Uint16 s_au16Preload[64];        /* 跨包/握手多出来的字节 */
static Uint16 s_u16PreloadLen = 0;
static Uint16 s_u16PreloadOff = 0;
static Uint32 s_u32LastByteTick = 0;    /* 最近一次收到字节的时刻 */

/* 参考工程依赖"驱动层已按包断帧"，这里在适配层累计：
 * 按包头 SOH/STX 决定整包长度，收满再一次性交给协议层。 */
static Uint16 XmodemFrameLength(Uint16 u16Head)
{
    if (u16Head == SOH)
    {
        return (Uint16)(3U + 128U + 2U);
    }
    if (u16Head == STX)
    {
        return (Uint16)(3U + 1024U + 2U);
    }
    if (u16Head == CAN)
    {
        return 2U;
    }
    return 1U;   /* EOT 及其它单字节控制符 */
}


/**
 * @brief 清空适配层的累计状态
 * @note  会话开始/取消时必须清：否则上一次的半包会和这一次的字节拼在一起。
 */
void XmodemRxReset(void)
{
    s_u16FrameFill = 0;
    s_u16PreloadLen = 0;
    s_u16PreloadOff = 0;
    s_u32LastByteTick = 0;
}


void XmodemRxPreload(const Uint16 *pu16Data, Uint16 u16Length)
{
    Uint16 i = 0;
    Uint16 u16Room = (Uint16)(sizeof(s_au16Preload) / sizeof(s_au16Preload[0]));

    XmodemRxReset();

    if ((pu16Data == NULL) || (u16Length == 0U))
    {
        return;
    }
    if (u16Length > u16Room)
    {
        u16Length = u16Room;
    }
    for (i = 0; i < u16Length; i++)
    {
        s_au16Preload[i] = pu16Data[i];
    }
    s_u16PreloadLen = u16Length;
}


int XmodemWrite(const Uint16 *u16Buff, Uint16 u16Length)
{
    return (int)SciPortWriteTx(XMODEM_SCI_ID, u16Buff, u16Length);
}


int XmodemRead(Uint16 *u16Buff, Uint16 u16MaxLength)
{
    Uint16 au16Batch[XMODEM_BATCH_MAX];
    Uint16 u16Got = 0;
    Uint16 u16Need = 0;
    Uint16 u16Carry = 0;
    Uint16 i = 0;
    ST_SCI_RX_SEG stSeg;

    if ((u16Buff == NULL) || (u16MaxLength == 0U))
    {
        return 0;
    }

    /* 半包静默超时：丢掉半截，等主机重发整包 */
    if ((s_u16FrameFill != 0U) && (Hw_TimeBaseIsReady() != 0U) &&
        (Hw_TimeBaseElapsed(s_u32LastByteTick) >= Hw_TimeBaseUsToTicks(XMODEM_IDLE_DROP_US)))
    {
        XmodemRxReset();
    }

    /* 先把交接/跨包留下来的字节倒进缓冲 */
    while ((s_u16PreloadOff < s_u16PreloadLen) && (s_u16FrameFill < u16MaxLength))
    {
        u16Buff[s_u16FrameFill] = s_au16Preload[s_u16PreloadOff];
        s_u16FrameFill++;
        s_u16PreloadOff++;
    }
    if (s_u16PreloadOff >= s_u16PreloadLen)
    {
        s_u16PreloadLen = 0;
        s_u16PreloadOff = 0;
    }

    if (s_u16FrameFill != 0U)
    {
        u16Need = XmodemFrameLength(u16Buff[0]);
    }

    /* 必要适配修正：收满本包就立即停止取批，最多只保存"最后一条批次跨过包边界"
     * 的那几个字节；原实现把整条同步积压全部搬进 64 字的预存数组，越界写坏内存。
     * 参考位置：XmodemTramsport.c XmodemRead()（参考工程靠驱动断帧，没有这段）。 */
    while ((u16Need == 0U) || (s_u16FrameFill < u16Need))
    {
        u16Got = SciPortPopBatch(XMODEM_SCI_ID, &au16Batch[0], XMODEM_BATCH_MAX, &stSeg);
        if (u16Got == 0U)
        {
            break;
        }

        u16Carry = 0;
        for (i = 0; i < u16Got; i++)
        {
            if ((u16Need == 0U) || (s_u16FrameFill < u16Need))
            {
                if (s_u16FrameFill >= u16MaxLength)
                {
                    XmodemRxReset();
                    return 0;
                }
                u16Buff[s_u16FrameFill] = au16Batch[i];
                s_u16FrameFill++;
                if (u16Need == 0U)
                {
                    u16Need = XmodemFrameLength(u16Buff[0]);
                }
            }
            else if (u16Carry < (Uint16)(sizeof(s_au16Preload) / sizeof(s_au16Preload[0])))
            {
                s_au16Preload[u16Carry] = au16Batch[i];
                u16Carry++;
            }
        }

        s_u32LastByteTick = stSeg.u32Tick;
        s_u16PreloadLen = u16Carry;
        s_u16PreloadOff = 0;

        if (u16Carry != 0U)
        {
            break;      /* 本包已收满并带出了下一条的余字节 */
        }
    }

    if ((s_u16FrameFill == 0U) || (u16Need == 0U))
    {
        return 0;
    }

    if (s_u16FrameFill < u16Need)
    {
        return 0;       /* 还没收满 */
    }

    s_u16FrameFill = 0;
    return (int)u16Need;
}




#ifdef __cplusplus
}
#endif

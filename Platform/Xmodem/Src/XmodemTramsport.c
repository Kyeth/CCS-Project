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

static Uint16 s_u16FrameFill = 0;       /* 已累计的整包字节数 */
static Uint16 s_au16Preload[64];        /* 握手交接时多取上来的字节 */
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

void XmodemRxPreload(const Uint16 *pu16Data, Uint16 u16Length)
{
    Uint16 i = 0;

    s_u16PreloadLen = 0;
    s_u16PreloadOff = 0;
    if ((pu16Data == NULL) || (u16Length == 0U))
    {
        return;
    }
    if (u16Length > (Uint16)sizeof(s_au16Preload) / 2U)
    {
        u16Length = (Uint16)(sizeof(s_au16Preload) / 2U);
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
    Uint16 i = 0;
    ST_SCI_RX_SEG stSeg;

    if ((u16Buff == NULL) || (u16MaxLength == 0U))
    {
        return 0;
    }

    if ((s_u16FrameFill != 0U) && (Hw_TimeBaseIsReady() != 0U) &&
        (Hw_TimeBaseElapsed(s_u32LastByteTick) >= Hw_TimeBaseUsToTicks(XMODEM_IDLE_DROP_US)))
    {
        s_u16FrameFill = 0;
    }

    /* 先把交接时多取上来的字节倒进缓冲 */
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

    while (1)
    {
        u16Got = SciPortPopBatch(XMODEM_SCI_ID, &au16Batch[0], XMODEM_BATCH_MAX, &stSeg);
        if (u16Got == 0U)
        {
            break;
        }
        if (((Uint32)s_u16FrameFill + (Uint32)u16Got) > (Uint32)u16MaxLength)
        {
            s_u16FrameFill = 0;
            return 0;
        }
        for (i = 0; i < u16Got; i++)
        {
            u16Buff[s_u16FrameFill + i] = au16Batch[i];
        }
        s_u16FrameFill = (Uint16)(s_u16FrameFill + u16Got);
        s_u32LastByteTick = stSeg.u32Tick;
    }

    if (s_u16FrameFill == 0U)
    {
        return 0;
    }

    u16Need = XmodemFrameLength(u16Buff[0]);
    if (s_u16FrameFill < u16Need)
    {
        return 0;
    }

    /* 一条批次可能跨两包：把本包之后多出来的字节留到下一次读，
     * 否则下一包会缺头（主机重发可以恢复，但没必要丢）。 */
    if (s_u16FrameFill > u16Need)
    {
        Uint16 u16Rest = (Uint16)(s_u16FrameFill - u16Need);
        for (i = 0; i < u16Rest; i++)
        {
            s_au16Preload[i] = u16Buff[u16Need + i];
        }
        s_u16PreloadLen = u16Rest;
        s_u16PreloadOff = 0;
    }
    s_u16FrameFill = 0;
    return (int)u16Need;
}






#ifdef __cplusplus
}
#endif

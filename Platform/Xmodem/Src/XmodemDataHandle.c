#include "XmodemDataHandle.h"

#include "Dsp2837xSFlash.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma DATA_SECTION(u16XmodemDataBuff, ".upybuf")
Uint16 u16XmodemDataBuff[1024];


int ReadBlock(Uint16 u16BlockNum, Uint16 *u16Buffer)
{
    return 0;
}


int WriteBlock(Uint32 u32BlockNum, Uint16 *u16Buffer, Uint16 u16Length)
{
    int i = 0;
    Uint16 u16Value = 0;
    Uint16 u16Cnt = 0;
    Uint32 u32Length = 0;

    if ((u16Buffer == NULL) || (u16Length == 0U) || ((u16Length & 1U) != 0U))
    {
        return -1;
    }

    u32Length = u16Length;

    /* 必要适配修正：参考工程不检查目标偏移，坏帧可以把数据写到备份区之外。
     * 这里限制"块偏移 + 本次写入"落在备份区内，且不超过打包缓冲。 */
    if (((u32Length >> 1) > (Uint32)(sizeof(u16XmodemDataBuff) / sizeof(u16XmodemDataBuff[0])))
        || (u32BlockNum > (Uint32)(FlashBackupAreaWords - (u32Length >> 1))))
    {
        return -1;
    }

    for (i = 0; i < u32Length; i += 2)
    {
        u16Value = u16Buffer[i] | (u16Buffer[i+1] << 8);
        u16XmodemDataBuff[u16Cnt++] = u16Value;
    }

    /* 必要缺陷修复：参考工程忽略 WriteFlashData 的返回值，编程失败也照样
     * 返回字节数，协议层就会回 ACK；这里把失败上报，协议层据此终止本次会话。 */
    if (WriteFlashData(0x0A0000U + u32BlockNum, u16XmodemDataBuff, u32Length>>1) != 0)
    {
        return -1;
    }




    return u16Cnt;
}







#ifdef __cplusplus
    }
#endif

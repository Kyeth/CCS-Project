#include "FM24C128D.h"
#include "Hw_GpioDriver.h"
#include "Hw_I2cDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif


ST_FM24C128D_HANDLE stFM24C128DHandle;


void FM24C128DEnableWriteProtection(ST_FM24C128D_HANDLE *pstFM24C128DHandle)
{
    Hw_GpioWritePin(pstFM24C128DHandle->u16WRPort, 1);
}



void FM24C128DDisableWriteProtection(ST_FM24C128D_HANDLE *pstFM24C128DHandle)
{
    Hw_GpioWritePin(pstFM24C128DHandle->u16WRPort, 0);
}



// 初始化，需要检测ID能否读到
int16 FM24C128DRegister(ST_FM24C128D_HANDLE *pstFM24C128DHandle, ST_FM24C128D_INTF *pstFM24C128DRwIntf, Uint16 u16Port, Uint16 u16Addr)
{
//    if ((pstFM24C128DHandle == NULL) || (pstFM24C128DRwIntf == NULL) || (pstFM24C128DRwIntf->EepromRead == NULL)
//            || (pstFM24C128DRwIntf->EepromRWRequest == NULL))
//    {
//        return -1;
//    }
    pstFM24C128DHandle->stFM24C128DIntf.EepromRead = pstFM24C128DRwIntf->EepromRead;
    pstFM24C128DHandle->stFM24C128DIntf.EepromWrite = pstFM24C128DRwIntf->EepromWrite;
    pstFM24C128DHandle->stFM24C128DIntf.EepromGetTick = pstFM24C128DRwIntf->EepromGetTick;
    pstFM24C128DHandle->u16WRPort = u16Port;
    pstFM24C128DHandle->u16Addr = u16Addr;


    pstFM24C128DHandle->IsInit = 1;
    return 0;
}


int16 FM24C128DWrite(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen)
{
    Uint16 u16CurrentAddr = u16TargetAddr;  // 当前操作地址
    Uint16 u16ByteWritten = 0;              // 已写入字节数

    Uint16 u16PageOffset = 0;               // 页内起始地址
    Uint16 u16PageRemain = 0;               // 页内剩余大小
    Uint16 u16WriteCnt = 0;
    Uint32 u32TimeTick = 0;
    Uint16 u16RemainData = 0;
    Uint16 u16ThisWriteMax = 0;

    while (u16ByteWritten < u16DataLen)
    {
        // 计算页内偏移，再用64-页内偏移得到当前页剩余空间
        u16PageOffset = u16CurrentAddr % FM24C128D_PAGE_SIZE;
        u16PageRemain = FM24C128D_PAGE_SIZE - u16PageOffset;

        // 计算本次最多能写入的字节数：取剩余数据长度和页剩余空间的最小值
        u16RemainData = u16DataLen - u16ByteWritten;
        u16ThisWriteMax = (u16RemainData < u16PageRemain) ? u16RemainData : u16PageRemain;

        // 在当前页内分段写入（每次最多14字节）
        while (u16ThisWriteMax > 0)
        {
            // 本次实际写入的字节数：取剩余字节数和14的最小值
            u16WriteCnt = (u16ThisWriteMax > 14) ? 14 : u16ThisWriteMax;

            // 调用IIC驱动写入数据
            pstFM24C128DHandle->stFM24C128DIntf.EepromWrite(pstFM24C128DHandle, u16CurrentAddr,
                                                           &u16DataBuff[u16ByteWritten], u16WriteCnt);

            // 更新参数
            u16CurrentAddr += u16WriteCnt;
            u16ByteWritten += u16WriteCnt;
            u16ThisWriteMax -= u16WriteCnt;

            // 写入延迟
            u32TimeTick = pstFM24C128DHandle->stFM24C128DIntf.EepromGetTick();
            while ((pstFM24C128DHandle->stFM24C128DIntf.EepromGetTick() - u32TimeTick) < 6);
        }
    }

    return 0; // 添加返回值
}

int16 FM24C128DRead(ST_FM24C128D_HANDLE *pstFM24C128DHandle, Uint16 u16TargetAddr, Uint16 *u16DataBuff, Uint16 u16DataLen)
{
    Uint16 u16ReadOffset = 0;
    Uint16 u16RemainingLen = u16DataLen;
    Uint16 u16CurrentAddr = u16TargetAddr;
    Uint16 u16ChunkLen;


    while (u16RemainingLen > 0)
    {
        u16ChunkLen = (u16RemainingLen > 14) ? 14 : u16RemainingLen;

        // 调用请求读取接口
        pstFM24C128DHandle->stFM24C128DIntf.EepromRead(pstFM24C128DHandle, u16CurrentAddr, &u16DataBuff[u16ReadOffset], u16ChunkLen);

        // 更新偏移量和地址
        u16ReadOffset += u16ChunkLen;
        u16CurrentAddr += u16ChunkLen;
        u16RemainingLen -= u16ChunkLen;
    }
    return 1;
}


#ifdef __cplusplus
    }
#endif


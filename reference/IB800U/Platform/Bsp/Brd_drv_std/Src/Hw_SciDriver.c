/**
 * @file Hw_SciDriver.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_SciDriver.h"
#include "Device.h"

#ifdef __cplusplus
    extern "C" {
#endif



static volatile struct SCI_REGS *stSciRegs[] = {&SciaRegs, &ScibRegs,};
volatile ST_SCI_HANDLE stSciHandle[SCI_ID_MAX] = {0};



/**
 * @brief 用于初始化单个SCI外设
 * 
 * @param pstSciCfgItem 入参是一个ST_SCI_CFG_ITEM结构体对象的地址
 */
void Hw_SingleSciInit(ST_SCI_CFG_ITEM* pstSciCfgItem)
{
    Uint32 u32Baud = 0;
    Uint16 u16SciId = 0;

    u16SciId = pstSciCfgItem->u16SciId;
    // 后续想办法优化一下这个时钟，目前该宏没有定义
//    u32Baud = (DEVICE_LSPCLK_FREQ / (pstSciCfgItem->u32BaudRate * 8U) -1);
    u32Baud = (DEVICE_LSPCLK_FREQ / (pstSciCfgItem->u32BaudRate * 8U) -1);

    stSciHandle[u16SciId].pstSciRegs = stSciRegs[u16SciId];
    // 先禁用SCI模块，然后复位SCI模块保持
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.SWRESET = 0;
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 0;

    // 先禁用一下FIFO，TX和RX
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.RXENA = 0;
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.TXENA = 0;
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIFFENA = 0;

    // 取消复位保持
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 1;

    stSciHandle[u16SciId].pstSciRegs->SCICCR.all = 0x00;

    stSciHandle[u16SciId].pstSciRegs->SCIPRI.bit.FREESOFT = 0x10;   // 调试模式下正常运行

    // 配置通信参数
    stSciHandle[u16SciId].pstSciRegs->SCICCR.bit.STOPBITS   = pstSciCfgItem->bStopBits;
    stSciHandle[u16SciId].pstSciRegs->SCICCR.bit.SCICHAR    = pstSciCfgItem->b4DataBits;
    stSciHandle[u16SciId].pstSciRegs->SCICCR.bit.PARITYENA  = pstSciCfgItem->bParity & 0x1;         // 奇偶校验使能位
    stSciHandle[u16SciId].pstSciRegs->SCICCR.bit.PARITY     = (pstSciCfgItem->bParity >> 1) & 0x1;
    stSciHandle[u16SciId].pstSciRegs->SCIHBAUD.bit.BAUD = (Uint16)((u32Baud & 0xFF00U) >> 8U);
    stSciHandle[u16SciId].pstSciRegs->SCILBAUD.bit.BAUD = (Uint16)(u32Baud & 0x00FFU);

    // 配置FIFO
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.TXFIFORESET = 0;  // 0:复位FIFO指针; 1:启用TxFIFO
    stSciHandle[u16SciId].pstSciRegs->SCIFFRX.bit.RXFIFORESET = 0;  // 0:复位FIFO指针; 1:启用RxFIFO
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.TXFFIENA = 0;     // 0:禁用发送FIFO中断; 1:启用
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.TXFFIL = 8;       // 当发送FIFO>=8时产生中断限定条件
    stSciHandle[u16SciId].pstSciRegs->SCIFFRX.bit.RXFFIENA = 0;     // 0:禁用接收FIFO中断; 1:启用
    stSciHandle[u16SciId].pstSciRegs->SCIFFRX.bit.RXFFIL = 8;       // 当接收FIFO>=8时产生中断限定条件
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.TXFIFORESET = 1;  // 0:复位FIFO指针; 1:启用TxFIFO
    stSciHandle[u16SciId].pstSciRegs->SCIFFRX.bit.RXFIFORESET = 1;  // 0:复位FIFO指针; 1:启用RxFIFO

    // 使能TX、RX、FIFO总开关，启动SCI模块
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.RXENA = 1;        // 使能SCI的TX
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.TXENA = 1;        // 使能SCI的RX
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIFFENA = 1;     // 使能SCI的FIFO
    stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.SWRESET = 1;      // 使能SCI



    // 发送RingBuff
    InitRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stTxRingBuff, (void *)&stSciHandle[u16SciId].u16TxBuff[0], sizeof(Uint16), SCI_BUFF_SIZE);
    // 接收RingBuff
    InitRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stRxRingBuff, (void *)&stSciHandle[u16SciId].u16RxBuff[0], sizeof(Uint16), SCI_BUFF_SIZE);

    stSciHandle[u16SciId].u16DeviceBusy = 0;
    stSciHandle[u16SciId].bIsInit = 1;                                     // 标记Sci已经初始化
}



/**
 * @brief 用于初始化一组SCI，通常用于参数初始化阶段
 * 
 * @param pstSciCfg :入参是一组ST_SCI_CFG结构体对象的集合
 */
void Hw_SciInit(ST_SCI_CFG *pstSciCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstSciCfg->u16ItemNum; i++)
    {
        Hw_SingleSciInit(&pstSciCfg->pstSciCfgItem[i]);
    }

}



/**
 * @brief 从接收FIFO中取出指定数据个数的字节
 * 
 * @param u16SciId :SCI的ID号
 * @param u16RxBuff :数组的接收缓存
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 u16TxBuff参数为NULL
 *                  :-4 SCI设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际读取的个数
 */
int16 Hw_SciRead(Uint16 u16SciId, Uint16 *u16RxBuff, Uint16 u16Size)
{
    int16 i16Ret = 0;
    Uint16 u16ReadSize = 0;

    if (u16SciId >= SCI_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (stSciHandle[u16SciId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (u16RxBuff == NULL)
    {
        return -3;  // Buff非法
    }

    if (stSciHandle[u16SciId].u16DeviceBusy == 1)
    {
        return -4;  // SCI处于繁忙状态
    }
    // 等待Sci接收到完整的一帧
    if (stSciHandle[u16SciId].bFrameOver != 1)
    {
        return u16ReadSize;
    }
    stSciHandle[u16SciId].bFrameOver = 0;

    stSciHandle[u16SciId].u16DeviceBusy = 1;

    // 统计当前最大可读取数量
    u16ReadSize = GetRingBuffDataLen((ST_RING_BUFF *)&stSciHandle[u16SciId].stRxRingBuff);
    u16ReadSize = (u16ReadSize >= u16Size) ? u16Size : u16ReadSize;

    i16Ret = ReadRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stRxRingBuff, (void *)u16RxBuff, u16ReadSize);

    stSciHandle[u16SciId].u16DeviceBusy = 0;
    return i16Ret;
}



/**
 * @brief 将指定个数的字节写入发送FIFO中
 * 
 * @param u16SciId :SCI的ID号
 * @param u16TxBuff :存放待写入数据的数组
 * @param u16Size :指定写入数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 u16TxBuff参数为NULL
 *                  :-4 SCI设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实现写入的个数
 */
int16 Hw_SciWrite(Uint16 u16SciId, Uint16 *u16TxBuff, Uint16 u16Size)
{
    int16 i16Ret = 0;
    if (u16SciId >= SCI_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (stSciHandle[u16SciId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }


    if (u16TxBuff == NULL)
    {
        return -3;  // Buff非法
    }

    if (stSciHandle[u16SciId].u16DeviceBusy == 1)
    {
        return -4;  // SCI处于繁忙状态
    }
    stSciHandle[u16SciId].u16DeviceBusy = 1;

    i16Ret = WriteRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stTxRingBuff, (void *)u16TxBuff, u16Size);

    stSciHandle[u16SciId].u16DeviceBusy = 0;

    return i16Ret;
}



/**
 * @brief SCI外设的控制接口(待实现)
 * 
 * @param u16SciId :SCI的ID号
 * @param u16Cmd :SCI命令枚举
 * @param vParam :参数
 * @return int16 
 */
int16 Hw_SciCtrl(Uint16 u16SciId, Uint16 u16Cmd, void *vParam)
{
    // 待实现，比如说修改波特率，修改FIFO大小等
    Uint32 u32Baud;

    switch (u16Cmd)
    {
    case SCI_COMM_CFG:

        // 后续想办法优化一下这个时钟，目前该宏没有定义
        u32Baud = (DEVICE_LSPCLK_FREQ / ((*((Uint32*)(vParam))) * 8U) -1);

        // 先禁用SCI模块，然后复位SCI模块保持
//        stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.SWRESET = 0;

        // 配置通信参数
        stSciHandle[u16SciId].pstSciRegs->SCIHBAUD.bit.BAUD = (Uint16)((u32Baud & 0xFF00U) >> 8U);
        stSciHandle[u16SciId].pstSciRegs->SCILBAUD.bit.BAUD = (Uint16)(u32Baud & 0x00FFU);

//        stSciHandle[u16SciId].pstSciRegs->SCICTL1.bit.SWRESET = 1;

    break;
    }
    return 1;
}



/**
 * @brief 将SCI总线上的数据接收到FIFO中，通常在中断中调用
 * 
 * @param u16SciId :SCI的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际接收到的数据
 */
#pragma CODE_SECTION(Hw_IsrSciReceive, ".ISR.ramfunc");
int16 Hw_IsrSciReceive(Uint16 u16SciId)
{
    Uint16 i = 0;
    Uint16 u16ReceiveSize = 0;
    // 这个临时Buff取决于SCI_FIFO的最大空间
    Uint16 u16ReceiveBuff[SCI_HW_FIFO] = {0};

//    if (u16SciId < SCI_ID0 || u16SciId >= SCI_ID_MAX)
//    {
//        return -1;  // ID非法
//    }
//
//    if (stSciHandle[u16SciId].bIsInit != 1)
//    {
//        return -2;  // 未初始化
//    }

    // 如果检测到帧开始,超时计数器开始计数
    if (stSciHandle[u16SciId].bFrameStart)
    {
        stSciHandle[u16SciId].u16FrameTimeOutCount++;
    }

    u16ReceiveSize = stSciHandle[u16SciId].pstSciRegs->SCIFFRX.bit.RXFFST;
    if (u16ReceiveSize)
    {
        // 接收到数据，帧开始位标记，复位超时计数
        stSciHandle[u16SciId].bFrameStart = 1;
        stSciHandle[u16SciId].u16FrameTimeOutCount = 0;
    }

    for (i = 0; i < u16ReceiveSize; i++)
    {
        u16ReceiveBuff[i] = (Uint16)stSciHandle[u16SciId].pstSciRegs->SCIRXBUF.bit.SAR;
    }

    u16ReceiveSize = WriteRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stRxRingBuff, (void *)&u16ReceiveBuff[0], u16ReceiveSize);

    // 这个地方还需要对错误标志进行判断，如果出现错误标志，还需要对其进行复位
    if ((stSciHandle[u16SciId].pstSciRegs->SCIRXST.bit.RXERROR == 1))
    {
        // 复位SCI
        stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 0;
    }
    // 启用SCI(之所以不放到上面的if语句中，是担心编译器会优化掉上面的复位语句)
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 1;

    if (stSciHandle[u16SciId].u16FrameTimeOutCount >= FRAME_TIMEOUT)
    {
        stSciHandle[u16SciId].bFrameOver = 1;
        stSciHandle[u16SciId].u16FrameTimeOutCount = 0;
        stSciHandle[u16SciId].bFrameStart = 0;
    }
    return u16ReceiveSize;
}



/**
 * @brief 将FIFO中的数据发送到SCI总线上，通常在中断中调用
 * 
 * @param u16SciId :SCI的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  : >=0 本次发送数据
 */
#pragma CODE_SECTION(Hw_IsrSciTransmit, ".ISR.ramfunc");
int16 Hw_IsrSciTransmit(Uint16 u16SciId)
{
    Uint16 i = 0;
    Uint16 u16TimeOut = 0;
    Uint16 u16TransmitSize = 0;
    // 这个临时Buff取决于SCI_FIFO的最大空间
    Uint16 u16TransmitBuff[SCI_HW_FIFO] = {0};

//    if (u16SciId < SCI_ID0 || u16SciId >= SCI_ID_MAX)
//    {
//        return -1;  // ID非法
//    }
//
//    if (stSciHandle[u16SciId].bIsInit != 1)
//    {
//        return -2;  // 未初始化
//    }
    // 需要先判断硬件FIFO还有多少剩余，然后从软件FIFO中取出对应的字节，然后进行发送
    u16TransmitSize = SCI_HW_FIFO - stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.TXFFST;

    u16TransmitSize = ReadRingBuff((ST_RING_BUFF *)&stSciHandle[u16SciId].stTxRingBuff, (void *)&u16TransmitBuff[0], u16TransmitSize);

    for (i = 0; i < u16TransmitSize; i++)
    {
        // 这里还需要做一个超时判断，避免一直死循环，导致中断异常
//        while ((stSciHandle[u16SciId].pstSciRegs->SCICTL2.bit.TXRDY != 1) && (u16TimeOut < 0xFFFFF))
//        {
//            u16TimeOut++;
//        }
        u16TimeOut = u16TimeOut;
        stSciHandle[u16SciId].pstSciRegs->SCITXBUF.bit.TXDT = u16TransmitBuff[i];
    }

    // 这个地方还需要对错误标志进行判断，如果出现错误标志，还需要对其进行复位
    if ((stSciHandle[u16SciId].pstSciRegs->SCIRXST.bit.RXERROR == 1))
    {
        // 复位SCI
        stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 0;
    }
    // 启用SCI(之所以不放到上面的if语句中，是担心编译器会优化掉上面的复位语句)
    stSciHandle[u16SciId].pstSciRegs->SCIFFTX.bit.SCIRST = 1;

    return u16TransmitSize;
}


#ifdef __cplusplus
    }
#endif


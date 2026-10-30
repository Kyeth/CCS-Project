/**
 * @file Hw_CanDriver.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_CanDriver.h"

#ifdef __cplusplus
    extern "C" {
#endif


static volatile struct CAN_REGS *g_pstCanRegs[] = {&CanaRegs};
static ST_CAN_HANDLE g_stCanHandle[CAN_ID_MAX] = {0};


#define READ_FIFO_CMD 0x3B0000
#define WRITE_FIFO_CMD 0xB70000



/**
 * @brief 用于初始化单个CAN外设
 * 
 * @param pstCanCfgItem 入参是一个ST_CAN_CFG_ITEM结构体对象的地址
 */
void Hw_SingleCanInit(ST_CAN_CFG_ITEM *pstCanCfgItem)
{
    Uint16 u16CanId = 0;
    Uint16 i = 0;

    EALLOW;
    u16CanId = pstCanCfgItem->u16CanId;

    g_stCanHandle[u16CanId].pstCanRegs = g_pstCanRegs[u16CanId];


    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.all = 0x00001401;   // 复位
    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.Init = 1;       // 关闭总线进入初始化阶段


    g_stCanHandle[u16CanId].pstCanRegs->CAN_RAM_INIT.all = 0x1A;
    while((g_stCanHandle[u16CanId].pstCanRegs->CAN_RAM_INIT.all & 0x3F) != 0x25)  // 这里加个超时判断
    {
    }

    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.SWR = 1;        // 复位CAN状态机

    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.CCE = 1;        // 使能配置寄存器可写

    g_stCanHandle[u16CanId].pstCanRegs->CAN_ABOTR = 10000;      // 自动恢复等待计数值(递减为0开始自动恢复)
    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.ABO = 1;    // 使能总线自动恢复
    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.IE0 = 1;    // 使能中断，将中断挂在中断线0上

    g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.SJW = 3;
    g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRPE = 0;   // 有效值0-15
    // Baud = (SYSCLK)/ ((BRP+1)) / (TSEG1 + TSEG2 + 3)   SYSCLK = 120M
    // Sampling point = (1 + TSEG1) / (1 + TSEG1 + TSEG2) * 100%
    // Baud >= 800k: Sampling point = 75%
    // Baud >= 500k: Sampling point = 80%
    // Baud < 500k: Sampling point = 87.5%
    switch(pstCanCfgItem->u16Baud)
    {
        case CANBAUD_1000K:         // Sampling point ：75%
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 12-1;    // 有效值0-63

            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 5;
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 2;
            break;

        case CANBAUD_800K:          // Sampling point ：75%
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 15-1;     // 有效值0-63
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 5;    // 有效值1-15
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 2;    // 有效值0-7
            break;

        case CANBAUD_500K:          // Sampling point ：80%
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 20-1;      // 有效值0-63
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 7;   // 有效值1-15
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 2;    // 有效值0-7
            break;

        case CANBAUD_250K:          // Sampling point ：87.5%
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 48-1;     // 有效值0-63
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 6;    // 有效值1-15
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 1;    // 有效值0-7
            break;

        case CANBAUD_125K:          // Sampling point ：83.5%
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 64-1;     // 有效值0-63
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 10;    // 有效值1-15
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 2;    // 有效值0-7
            break;

        default:
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.BRP = 48-1;     // 有效值0-63
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG1 = 6;    // 有效值1-15
            g_stCanHandle[u16CanId].pstCanRegs->CAN_BTR.bit.TSEG2 = 1;    // 有效值0-7
            break;
    }

    // 邮箱掩码过滤规则
//    // 伪代码：硬件执行的过滤判断
//    if(
//       ( (Received_ID ^ IF1ARB.ID) & (IF1MSK.MSK) == 0 ) &&  // ID关键位匹配
//       ( (Received_FrameType == IF1ARB.XTD) || IF1MSK.MXTD ) && // 帧类型匹配
//       ( (Received_Direction == IF1ARB.DIR) || IF1MSK.MDIR )    // 方向匹配
//    )
//    {
//       放行并存入邮箱！ // 匹配成功
//    }
//    else {
//       拒绝进入！      // 匹配失败
//    }


    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.CCE = 0;
    g_stCanHandle[u16CanId].pstCanRegs->CAN_CTL.bit.Init = 0;
    g_stCanHandle[u16CanId].pstCanRegs->CAN_GLB_INT_EN.bit.GLBINT0_EN = 0;  // 连接到PIE，全局中断使能

    for (i = 0; i < pstCanCfgItem->pstCanFifoCfg->u16ItemNum; i++)
    {
        while (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.bit.Busy);
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.MsgVal = 0;  //先禁用邮箱
        // 先配置掩码
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MSK.bit.MDir = 1;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MSK.bit.MXtd = 1;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MSK.bit.Msk = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bMask;

        // 然后设置接收邮箱
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.EoB = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bEob;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.DLC = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bDlc;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.UMask = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bMaskEnable;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.RxIE = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bRxIE;

        //最后配置仲裁寄存器
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.Dir = 0;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.Xtd = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bXtd;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.ID = pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bARBMsgId;
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.MsgVal = 1;  // 使能邮箱
        g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.all = (0x0F00000 | pstCanCfgItem->pstCanFifoCfg->pstCanFifoCfgItem[i].bFifoId);

    }


    InitRingBuff(&g_stCanHandle[u16CanId].stTxRingBuff, g_stCanHandle[u16CanId].stTxBuff, sizeof(ST_CAN_FRAME), CAN_BUFF_SIZE);
    InitRingBuff(&g_stCanHandle[u16CanId].stRxRingBuff, g_stCanHandle[u16CanId].stRxBuff, sizeof(ST_CAN_FRAME), CAN_BUFF_SIZE);
    g_stCanHandle[u16CanId].u32RxFifoValidMask = pstCanCfgItem->u32RxFifoValidMask;



    g_stCanHandle[u16CanId].u16DeviceBusy = 0;
    g_stCanHandle[u16CanId].bIsInit = 1;


    EDIS;
}



/**
 * @brief 用于初始化一组CAN，通常用于参数初始化阶段
 * 
 * @param pstCanCfg :入参是一组ST_CAN_CFG结构体对象的集合
 */
void Hw_CanInit(ST_CAN_CFG *pstCanCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstCanCfg->u16ItemNum; i++)
    {
        Hw_SingleCanInit(&pstCanCfg->pstCanCfgItem[i]);
    }
}



/**
 * @brief 从接收FIFO中取出指定数据个数的CAN帧
 * 
 * @param u16CanId :CAN的ID号
 * @param pstCanFrame :ST_CAN_FRAME结构体指针，可以是一个结构体数组指针，也可以是单个结构体指针
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstCanFrame参数非法
 *                  :-4 CAN设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际读取的个数
 */
int16 Hw_CanRead(Uint16 u16CanId, ST_CAN_FRAME *pstCanFrame, Uint16 u16Size)
{
    int16 i16Ret = 0;
    Uint16 u16ReadSize = 0;

    if (u16CanId >= CAN_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (g_stCanHandle[u16CanId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstCanFrame == NULL)
    {
        return -3;  // Buff非法
    }

    if (g_stCanHandle[u16CanId].u16DeviceBusy == 1)
    {
        return -4;      // Can处于繁忙状态
    }

    g_stCanHandle[u16CanId].u16DeviceBusy = 1;

    // 统计当前最大可读取数量
    u16ReadSize = GetRingBuffDataLen(&g_stCanHandle[u16CanId].stRxRingBuff);
    u16ReadSize = (u16ReadSize >= u16Size) ? u16Size : u16ReadSize;

    i16Ret = ReadRingBuff(&g_stCanHandle[u16CanId].stRxRingBuff, pstCanFrame, u16ReadSize);

    g_stCanHandle[u16CanId].u16DeviceBusy = 0;

    return i16Ret;

}



/**
 * @brief 将指定个数的CAN帧写入发送FIFO中
 * 
 * @param u16CanId :CAN的ID号
 * @param pstCanFrame :ST_CAN_FRAME结构体指针，可以是一个结构体数组指针，也可以是单个结构体指针
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstCanFrame参数非法
 *                  :-4 CAN设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实现写入的个数
 */
int16 Hw_CanWrite(Uint16 u16CanId, ST_CAN_FRAME *pstCanFrame, Uint16 u16Size)
{
    int16 i16Ret = 0;

    if (u16CanId >= CAN_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (g_stCanHandle[u16CanId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstCanFrame == NULL)
    {
        return -3;  // Buff非法
    }

    if (g_stCanHandle[u16CanId].u16DeviceBusy == 1)
    {
        return -4;      // Can处于繁忙状态
    }

    g_stCanHandle[u16CanId].u16DeviceBusy = 1;
    i16Ret = WriteRingBuff(&g_stCanHandle[u16CanId].stTxRingBuff, pstCanFrame, u16Size);
    g_stCanHandle[u16CanId].u16DeviceBusy = 0;

    return i16Ret;

}


/**
 * @brief CAN外设的控制接口(待实现)
 * 
 * @param u16CanId :CAN的ID号
 * @param u16Cmd  :CAN命令枚举
 * @param vParam :参数
 * @return int 
 */
int Hw_CanCtrl(Uint16 u16CanId, Uint16 u16Cmd, void *vParam)
{
    // 待实现
    return 1;
}



/**
 * @brief 将CAN总线上的数据接收到FIFO中，通常在中断中调用
 * 
 * @param u16CanId :CAN的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际接收到的数据
 */
int16 Hw_IsrCanReceive(Uint16 u16CanId)
{
    Uint32 u32FifoNumber = 0;
    Uint16 i = 0;
    Uint16 u16ReceiveCount = 0;
    ST_CAN_FRAME stCanFrame[CAN_HW_FIFO];

    if (u16CanId >= CAN_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (g_stCanHandle[u16CanId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    u32FifoNumber = g_stCanHandle[u16CanId].pstCanRegs->CAN_NDAT_21;

    // 只取有效的接收邮箱
    u32FifoNumber = u32FifoNumber & g_stCanHandle[u16CanId].u32RxFifoValidMask;
    // 检查FIFO消息邮箱中是否有数据更新
    if (u32FifoNumber != 0)
    {
        for (i = 0; i < 32; i++)
        {
            if (((u32FifoNumber >> i) & 0x1) == 0)
            {
                continue;
            }
            while (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.bit.Busy == 1) ;
            g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.all = READ_FIFO_CMD + i + 1;     // 读取指定邮箱的数据
            while (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.bit.Busy == 1) ;

            if (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.NewDat == 1)
            {
                stCanFrame[u16ReceiveCount].unDataA.Bytes.u8Byte0 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATA.bit.Data_3;
                stCanFrame[u16ReceiveCount].unDataA.Bytes.u8Byte1 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATA.bit.Data_2;
                stCanFrame[u16ReceiveCount].unDataA.Bytes.u8Byte2 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATA.bit.Data_1;
                stCanFrame[u16ReceiveCount].unDataA.Bytes.u8Byte3 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATA.bit.Data_0;
                stCanFrame[u16ReceiveCount].unDataB.Bytes.u8Byte0 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATB.bit.Data_7;
                stCanFrame[u16ReceiveCount].unDataB.Bytes.u8Byte1 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATB.bit.Data_6;
                stCanFrame[u16ReceiveCount].unDataB.Bytes.u8Byte2 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATB.bit.Data_5;
                stCanFrame[u16ReceiveCount].unDataB.Bytes.u8Byte3 = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1DATB.bit.Data_4;
                stCanFrame[u16ReceiveCount].bIDE = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.Xtd;

                // 1:表示拓展帧，0:表示标准帧，标准帧需要右移18位
                if (stCanFrame[u16ReceiveCount].bIDE)
                {
                    stCanFrame[u16ReceiveCount].bID = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.ID;
                }
                else
                {
                    stCanFrame[u16ReceiveCount].bID = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.ID >> 18;
                }

                stCanFrame[u16ReceiveCount].bRTR = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1ARB.bit.Dir;
                stCanFrame[u16ReceiveCount].bDLC = g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1MCTL.bit.DLC;

                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.all = 0x40000 + i + 1;       // 清除对应邮箱的NewDatFlag
                while (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF1CMD.bit.Busy == 1) ;
                u16ReceiveCount++;
            }

        }
        u16ReceiveCount = WriteRingBuff(&g_stCanHandle[u16CanId].stRxRingBuff, stCanFrame, u16ReceiveCount);
    }
    return u16ReceiveCount;
}



/**
 * @brief 将FIFO中的数据发送到CAN总线上，通常在中断中调用
 * 
 * @param u16CanId :CAN的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际发送的数据
 */
int16 Hw_IsrCanTransmit(Uint16 u16CanId)
{
    Uint16 i = 0;
    Uint32 u32FifoNumber = 0;
    Uint16 u16SendCount = 0;
    Uint16 u16SendSize = 0;
    Uint32 u32TxFifoValidMask = 0;

    ST_CAN_FRAME stCanFrame[CAN_HW_FIFO];


    if (u16CanId >= CAN_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (g_stCanHandle[u16CanId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    u32FifoNumber = g_stCanHandle[u16CanId].pstCanRegs->CAN_TXRQ_21;
    u32TxFifoValidMask = ~g_stCanHandle[u16CanId].u32RxFifoValidMask;

    // 只取有效的发送邮箱部分
    u32FifoNumber = (u32FifoNumber ^ u32TxFifoValidMask) & u32TxFifoValidMask;

    // 判断发送邮箱是否空闲
    if (u32FifoNumber != 0)
    {
        // 获取有几个空闲的邮箱
        for (i = 0; i < 32; i++)
        {
            if (((u32FifoNumber >> i) & 0x1) == 0x00)
            {
                continue;
            }
            u16SendSize++;
        }

        // 从RingBuff中取出消息
        u16SendSize = ReadRingBuff(&g_stCanHandle[u16CanId].stTxRingBuff, &stCanFrame[0], u16SendSize);

        // 获取有几个空闲的邮箱
        for (i = 0; i < 32; i++)
        {
            if (((u32FifoNumber >> i) & 0x1) == 0)
            {
                continue;
            }

            if (u16SendCount < u16SendSize)
            {

                CanaRegs.CAN_GLB_INT_CLR.all = 1;

                while (g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2CMD.bit.Busy == 1) ;

                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2MCTL.bit.EoB = 1;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2MCTL.bit.TxIE = 0;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2MCTL.bit.DLC = stCanFrame[u16SendCount].bDLC;

                // 在数据链路层传输是以大端存储的字节序传输
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATA.bit.Data_0 = stCanFrame[u16SendCount].unDataA.Bytes.u8Byte3;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATA.bit.Data_1 = stCanFrame[u16SendCount].unDataA.Bytes.u8Byte2;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATA.bit.Data_2 = stCanFrame[u16SendCount].unDataA.Bytes.u8Byte1;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATA.bit.Data_3 = stCanFrame[u16SendCount].unDataA.Bytes.u8Byte0;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATB.bit.Data_4 = stCanFrame[u16SendCount].unDataB.Bytes.u8Byte3;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATB.bit.Data_5 = stCanFrame[u16SendCount].unDataB.Bytes.u8Byte2;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATB.bit.Data_6 = stCanFrame[u16SendCount].unDataB.Bytes.u8Byte1;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2DATB.bit.Data_7 = stCanFrame[u16SendCount].unDataB.Bytes.u8Byte0;

                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2ARB.bit.MsgVal = 1;          // 启用邮箱
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2ARB.bit.Xtd = stCanFrame[u16SendCount].bIDE;
                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2ARB.bit.Dir = ~stCanFrame[u16SendCount].bRTR;

                // 1:表示拓展帧，0:表示标准帧，标准帧需要左移8位
                if (stCanFrame[u16SendCount].bIDE)
                {
                    g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2ARB.bit.ID = stCanFrame[u16SendCount].bID;
                }
                else
                {
                    g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2ARB.bit.ID = (stCanFrame[u16SendCount].bID & 0xFFF) << 18;
                }

                g_stCanHandle[u16CanId].pstCanRegs->CAN_IF2CMD.all = WRITE_FIFO_CMD + i + 1;      // 启动发送
                u16SendCount++;
            }
            else
            {
                break;
            }
        }
    }
    return u16SendSize;

}

#ifdef __cplusplus
    }
#endif


#include "Hw_MCanDriver.h"
#include "sysctl.h"
#include "mcan.h"
#include "string.h"

#ifdef __cplusplus
    extern "C" {
#endif


static volatile struct MCANA_REGS *g_pstMCanRegs[] = {&McanaRegs};
ST_MCAN_HANDLE g_stMCanHandle[MCAN_ID_MAX] = {0};


void Hw_SingleMCanInit(ST_MCAN_CFG_ITEM *pstMCanCfgItem)
{
    MCAN_StdMsgIDFilterElement StdFiltelem[32];
    MCAN_ExtMsgIDFilterElement ExtFiltelem[32];

    Uint16 i = 0;
    Uint16 u16MCanID = pstMCanCfgItem->u16MCanId;


    memset(&StdFiltelem, 0, sizeof(StdFiltelem));
    memset(&ExtFiltelem, 0, sizeof(ExtFiltelem));

    g_stMCanHandle[u16MCanID].pstMCanRegs = g_pstMCanRegs[u16MCanID];

    SysCtl_setMCANClk(SYSCTL_MCANCLK_DIV_1);

    EALLOW;
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.INIT = 1;          // 进入初始化模式
    while(g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.INIT != 1){};

    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.CCE = 1;           // 使能配置可写
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.FDOE = 0;          // 使用经典CAN模式
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.BRSE = 0;          // 不使用比特率切换（FD才使用）
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.DAR = 0;           // 0:使能自动重传
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.TEST = 0;          // 0:正常模式
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.TXP = 1;           // 1:每个帧之间间隔2个比特位   0:表示关闭
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.NISO = 0;          // 0: ISO11898-1 FD帧格式    1:Bosch FD帧格式

    // 传输延迟补偿
    g_pstMCanRegs[u16MCanID]->MCANA_TDCR.bit.TDCF = 0xA;
    g_pstMCanRegs[u16MCanID]->MCANA_TDCR.bit.TDCO = 0x6;
    g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;       // 使能发送延迟补偿

    // 配置全局滤波器
    g_pstMCanRegs[u16MCanID]->MCANA_GFC.bit.RRFE = 1;       // 拒绝所有远程拓展帧
    g_pstMCanRegs[u16MCanID]->MCANA_GFC.bit.RRFS = 1;       // 拒绝所有远程帧
    g_pstMCanRegs[u16MCanID]->MCANA_GFC.bit.ANFE = 1;       // 允许RXFIFO1接收拓展帧
    g_pstMCanRegs[u16MCanID]->MCANA_GFC.bit.ANFS = 1;       // 允许RXFIFO1接收标志帧

    // 配置超时计数器
    g_pstMCanRegs[u16MCanID]->MCANA_TOCC.bit.TOS = 0;
    g_pstMCanRegs[u16MCanID]->MCANA_TOCC.bit.TOP = 0xFFFF;
    g_pstMCanRegs[u16MCanID]->MCANA_TOCC.bit.ETOC = 0;      // 禁用超时计数器

    // 设置波特率
    // 仲裁段 Baud = (CLK / (NBRP + 1) / ( seg1 + seg2 + 3))
    // Sampling point = (1 + seg1) / (1 + seg1 + seg2) * 100%
    switch (pstMCanCfgItem->u16Baud)
    {
        case MCANBAUD_1000K:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 12 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 5;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 2;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 5;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 2;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
            break;

        case MCANBAUD_800K:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 5;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 2;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 5;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 2;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
                break;

        case MCANBAUD_500K:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 20 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 7;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 2;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 20 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 7;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 2;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
                break;

        case MCANBAUD_250K:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 25;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 4;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 25;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 4;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
                break;

        case MCANBAUD_125K:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 30 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 24;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 5;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 30 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 24;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 5;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
                break;

        default:
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG1 = 25;
            g_pstMCanRegs[u16MCanID]->MCANA_NBTP.bit.NTSEG2 = 4;
            // 数据段 波特率计算同上
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DBRP = 15 - 1;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DSJW = 3;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG1 = 25;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.DTSEG2 = 4;
            g_pstMCanRegs[u16MCanID]->MCANA_DBTP.bit.TDC = 0;
                break;
    }


    // 邮箱掩码过滤规则
//    // 伪代码：硬件执行的过滤判断
//    if(
//       ( (Received_ID ^ EFID1) & (EFID2) == 0 ) &&  // ID关键位匹配
//       ( (Received_FrameType == IF1ARB.XTD) || IF1MSK.MXTD ) && // 帧类型匹配
//       ( (Received_Direction == IF1ARB.DIR) || IF1MSK.MDIR )    // 方向匹配
//    )
//    {
//       放行并存入邮箱！ // 匹配成功
//    }
//    else {
//       拒绝进入！      // 匹配失败
//    }

    // 配置消息RAM
    // 配置标准帧滤波器
    g_pstMCanRegs[u16MCanID]->MCANA_SIDFC.bit.FLSSA = 0;        // 标准帧滤波器占据消息的起始地址
    g_pstMCanRegs[u16MCanID]->MCANA_SIDFC.bit.LSS = pstMCanCfgItem->u16StdFilterNum;          // 标准帧滤波器占据消息的个数

    for (i = 0; i < pstMCanCfgItem->u16StdFilterNum; i++)
    {
        StdFiltelem[i].sfid1 = pstMCanCfgItem->pstMCanStdFilter[i].bARBMsgId;
        StdFiltelem[i].sfid2 = pstMCanCfgItem->pstMCanStdFilter[i].bMask;
        StdFiltelem[i].sfec = 0x02;     // 配置为FIFO1接收
        StdFiltelem[i].sft = 0x00;      // 区间滤波
    }
    MCAN_addStdMsgIDFilter(MCANA_DRIVER_BASE, pstMCanCfgItem->u16StdFilterNum, &StdFiltelem[0]);

    // 配置拓展帧滤波器
    g_pstMCanRegs[u16MCanID]->MCANA_XIDFC.bit.FLESA = 64 >> 2;        // 拓展帧滤波器占据消息的起始地址
    g_pstMCanRegs[u16MCanID]->MCANA_XIDFC.bit.LSE = pstMCanCfgItem->u16ExtFilterNum;          // 拓展帧滤波器占据消息的个数

    for (i = 0; i < pstMCanCfgItem->u16ExtFilterNum; i++)
    {
        ExtFiltelem[i].efid1 = pstMCanCfgItem->pstMCanExtFilter[i].bARBMsgId;
        ExtFiltelem[i].efid2 = pstMCanCfgItem->pstMCanExtFilter[i].bMask;
        ExtFiltelem[i].efec = 0x02;
        ExtFiltelem[i].eft = 0x00;
    }

    MCAN_addExtMsgIDFilter(MCANA_DRIVER_BASE, pstMCanCfgItem->u16ExtFilterNum, &ExtFiltelem[0]);
//    MCAN_setExtIDAndMask(MCANA_DRIVER_BASE, 0x1FFFFFFFU);

    // 配置RX FIFO1 section
//    g_pstMCanRegs[u16MCanID]->MCANA_RXBC.bit.RBSA = 0;          // 接收消息占用的内存起始地址
//    g_pstMCanRegs[u16MCanID]->MCANA_RXESC.bit.RBDS = 0;         // RxBuff接收数据长度最大为8

    g_pstMCanRegs[u16MCanID]->MCANA_RXESC.bit.F1DS = 0;         // RxFifo接收数据最大长度为8
    g_pstMCanRegs[u16MCanID]->MCANA_RXF1C.bit.F1SA = 128;         // RXFIFO接收消息占用的内存起始地址
    g_pstMCanRegs[u16MCanID]->MCANA_RXF1C.bit.F1S = 32;         // RxFIFO大小为32个
    g_pstMCanRegs[u16MCanID]->MCANA_RXF1C.bit.F1OM = 1;         // RxFIFO循环覆写模式
    g_pstMCanRegs[u16MCanID]->MCANA_RXF1C.bit.F1WM = 0;         // 接收数据门限中断禁用



    // 配置发送FIFO和邮箱
    g_pstMCanRegs[u16MCanID]->MCANA_TXBC.bit.TFQM = 0;          // 0：FIFO模式     1:Queue模式
    g_pstMCanRegs[u16MCanID]->MCANA_TXBC.bit.TBSA = 264;   // 发送消息占用的内存起始地址
    g_pstMCanRegs[u16MCanID]->MCANA_TXBC.bit.NDTB = 0;          // 专用发送内存消息个数
    g_pstMCanRegs[u16MCanID]->MCANA_TXBC.bit.TFQS = 32;          // 0:不启用FIFO和队列 >1:启用FIFO和队列（TFQS+NDTB总和不应超过32）
    g_pstMCanRegs[u16MCanID]->MCANA_TXESC.bit.TBDS = 0;         // 数据长度最大为8

    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.CCE = 0;           // 关闭配置可写
    g_pstMCanRegs[u16MCanID]->MCANA_CCCR.bit.INIT = 0;          // 进入正常模式

    EDIS;

    InitRingBuff(&g_stMCanHandle[u16MCanID].stTxRingBuff, g_stMCanHandle[u16MCanID].stTxBuff, sizeof(ST_MCAN_FRAME), MCAN_BUFF_SIZE);
    InitRingBuff(&g_stMCanHandle[u16MCanID].stRxRingBuff, g_stMCanHandle[u16MCanID].stRxBuff, sizeof(ST_MCAN_FRAME), MCAN_BUFF_SIZE);
    g_stMCanHandle[u16MCanID].u16DeviceBusy = 0;
    g_stMCanHandle[u16MCanID].bIsInit = 1;
}


void Hw_MCanInit(ST_MCAN_CFG *pstMCanCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstMCanCfg->u16ItemNum; i ++)
    {
        Hw_SingleMCanInit(&pstMCanCfg->pstMCanCfgItem[i]);
    }
}




int16 Hw_MCanRead(Uint16 u16MCanId, ST_MCAN_FRAME *pstMCanFrame, Uint16 u16Size)
{
    int16 i16Ret = 0;
    Uint16 u16ReadSize = 0;

    if (u16MCanId >= MCAN_ID_MAX)
    {
        return -1;  // ID非法
    }

    if (g_stMCanHandle[u16MCanId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstMCanFrame == NULL)
    {
        return -3;  // Buff非法
    }

    if (g_stMCanHandle[u16MCanId].u16DeviceBusy == 1)
    {
        return -4;      // Can处于繁忙状态
    }

    g_stMCanHandle[u16MCanId].u16DeviceBusy = 1;

    // 统计当前最大可读取数量
    u16ReadSize = GetRingBuffDataLen(&g_stMCanHandle[u16MCanId].stRxRingBuff);
    u16ReadSize = (u16ReadSize >= u16Size) ? u16Size : u16ReadSize;

    i16Ret = ReadRingBuff(&g_stMCanHandle[u16MCanId].stRxRingBuff, pstMCanFrame, u16ReadSize);

    g_stMCanHandle[u16MCanId].u16DeviceBusy = 0;

    return i16Ret;

}


/**
 * @brief 将指定个数的MCAN帧写入发送FIFO中
 *
 * @param u16CanId :MCAN的ID号
 * @param pstCanFrame :ST_CAN_FRAME结构体指针，可以是一个结构体数组指针，也可以是单个结构体指针
 * @param u16Size :指定读取数据的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstCanFrame参数非法
 *                  :-4 CAN设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实现写入的个数
 */
#pragma CODE_SECTION(Hw_MCanWrite, ".ISR.ramfunc");
int16 Hw_MCanWrite(Uint16 u16MCanId, ST_MCAN_FRAME *pstMCanFrame, Uint16 u16Size)
{
    int16 i16Ret = 0;

//    if (u16MCanId < MCAN_ID0 || u16MCanId >= MCAN_ID_MAX)
//    {
//        return -1;  // ID非法
//    }
//
//    if (g_stMCanHandle[u16MCanId].bIsInit != 1)
//    {
//        return -2;  // 未初始化
//    }
//
//    if (pstMCanFrame == NULL)
//    {
//        return -3;  // Buff非法
//    }
//
//    if (g_stMCanHandle[u16MCanId].u16DeviceBusy == 1)
//    {
//        return -4;      // Can处于繁忙状态
//    }

    g_stMCanHandle[u16MCanId].u16DeviceBusy = 1;
    i16Ret = WriteRingBuff(&g_stMCanHandle[u16MCanId].stTxRingBuff, pstMCanFrame, u16Size);
    g_stMCanHandle[u16MCanId].u16DeviceBusy = 0;

    return i16Ret;

}

#pragma CODE_SECTION(Hw_IsrMCanReceive, ".ISR.ramfunc");
int16 Hw_IsrMCanReceive(Uint16 u16MCanId)
{
    Uint32 u32FifoNumber = 0;
    Uint16 i = 0;
    Uint16 u16ReceiveCount = 0;
    Uint16 u16FifoPutIndex = 0;
    Uint16 u16FifoGetIndex = 0;
    Uint16 u16FreeFifo = 0;
    ST_MCAN_FRAME stMCanFrame[MCAN_HW_FIFO];
    MCAN_RxBufElement RxMsg;
    Uint16 u16Flag = 0;

    // 屏蔽判断，以便于提高执行速度
//    if (u16MCanId < MCAN_ID0 || u16MCanId >= MCAN_ID_MAX)
//    {
//        return -1;  // ID非法
//    }
//
//    if (g_stMCanHandle[u16MCanId].bIsInit != 1)
//    {
//        return -2;  // 未初始化
//    }



    // Flag中记录了FIFO中已经接收的数据
    u16Flag = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_RXF1S.bit.F1FL & 0x7F;
    if (u16Flag > 0)
    {
        // 找出本次最大可以读取的数据量
        // 不能使用Put和Get的索引来统计数据，当在调试时，如果暂停了程序的话，会使得FIFO溢出
        // 此时Put的索引将等于Get的索引，此时无法计算出正确的数据个数
        u16FifoPutIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_RXF1S.bit.F1PI;
        u16FifoGetIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_RXF1S.bit.F1GI;

        u16FifoPutIndex = u16FifoPutIndex;

        u16FreeFifo = GetRingBuffDataLen(&g_stMCanHandle[u16MCanId].stRxRingBuff);
        u16FreeFifo = g_stMCanHandle[u16MCanId].stRxRingBuff.u16Size - u16FreeFifo - 1;
        u32FifoNumber = u16Flag < u16FreeFifo ? u16Flag : u16FreeFifo;

        for (i = 0; i < u32FifoNumber; i++)
        {
            MCAN_readMsgRam(MCANA_DRIVER_BASE, MCAN_MEM_TYPE_FIFO, 0, MCAN_RX_FIFO_NUM_1, &RxMsg);
            stMCanFrame[i].bID = RxMsg.id;
            stMCanFrame[i].bDLC = RxMsg.dlc;
            stMCanFrame[i].bIDE = RxMsg.xtd;
            stMCanFrame[i].bRTR = RxMsg.rtr;
            stMCanFrame[i].unDataA.Bytes.u8Byte3 = RxMsg.data[0];
            stMCanFrame[i].unDataA.Bytes.u8Byte2 = RxMsg.data[1];
            stMCanFrame[i].unDataA.Bytes.u8Byte1 = RxMsg.data[2];
            stMCanFrame[i].unDataA.Bytes.u8Byte0 = RxMsg.data[3];
            stMCanFrame[i].unDataB.Bytes.u8Byte3 = RxMsg.data[4];
            stMCanFrame[i].unDataB.Bytes.u8Byte2 = RxMsg.data[5];
            stMCanFrame[i].unDataB.Bytes.u8Byte1 = RxMsg.data[6];
            stMCanFrame[i].unDataB.Bytes.u8Byte0 = RxMsg.data[7];

            g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_RXF1A.bit.F1AI = u16FifoGetIndex + u16ReceiveCount;
            u16ReceiveCount++;
        }
        u16ReceiveCount = WriteRingBuff(&g_stMCanHandle[u16MCanId].stRxRingBuff, stMCanFrame, u16ReceiveCount);
    }

    return u16ReceiveCount;
}




/**
 * @brief 将FIFO中的数据发送到CAN总线上，通常在中断中调用
 *
 * @param u16CanId :MCAN的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :>=0 本次实际发送的数据
 */
#pragma CODE_SECTION(Hw_IsrMCanTransmit, ".ISR.ramfunc");
int16 Hw_IsrMCanTransmit(Uint16 u16MCanId)
{
    Uint16 i = 0;
    Uint32 u32FifoNumber = 0;
    Uint16 u16SendSize = 0;
    ST_MCAN_FRAME stMCanFrame[MCAN_HW_FIFO];

    Uint16 u16FifoPutIndex = 0;
    Uint16 u16FifoGetIndex = 0;
    MCAN_TxBufElement stTxMsg;

//    if (u16MCanId < MCAN_ID0 || u16MCanId >= MCAN_ID_MAX)
//    {
//        return -1;  // ID非法
//    }
//
//    if (g_stMCanHandle[u16MCanId].bIsInit != 1)
//    {
//        return -2;  // 未初始化
//    }


    u16FifoPutIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFQP;
    u16FifoGetIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFGI;
//    u32FifoNumber = (32 + u16FifoGetIndex - u16FifoPutIndex - 1) % 32;

    u32FifoNumber = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFFL;

    if (u32FifoNumber > 1)
    {
        u16SendSize = ReadRingBuff(&g_stMCanHandle[u16MCanId].stTxRingBuff, &stMCanFrame[0], u32FifoNumber);
        for (i = 0; i < u16SendSize; i++)
        {
            u16FifoPutIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFQP;
            stTxMsg.id = stMCanFrame[i].bID;
            stTxMsg.dlc = stMCanFrame[i].bDLC;
            stTxMsg.rtr = stMCanFrame[i].bRTR;
            stTxMsg.xtd = stMCanFrame[i].bIDE;
            stTxMsg.esi = 0;
            stTxMsg.efc = 0;
            stTxMsg.brs = 0;
            stTxMsg.mm = 0xAA;
            stTxMsg.data[0] = stMCanFrame[i].unDataA.Bytes.u8Byte3;
            stTxMsg.data[1] = stMCanFrame[i].unDataA.Bytes.u8Byte2;
            stTxMsg.data[2] = stMCanFrame[i].unDataA.Bytes.u8Byte1;
            stTxMsg.data[3] = stMCanFrame[i].unDataA.Bytes.u8Byte0;
            stTxMsg.data[4] = stMCanFrame[i].unDataB.Bytes.u8Byte3;
            stTxMsg.data[5] = stMCanFrame[i].unDataB.Bytes.u8Byte2;
            stTxMsg.data[6] = stMCanFrame[i].unDataB.Bytes.u8Byte1;
            stTxMsg.data[7] = stMCanFrame[i].unDataB.Bytes.u8Byte0;

            // 后面可以把这个宏变成一个变量，可以由MCANID来选择
            MCAN_writeMsgRam(MCANA_DRIVER_BASE, MCAN_MEM_TYPE_FIFO, 1, &stTxMsg);
            MCAN_txBufAddReq(MCANA_DRIVER_BASE, u16FifoPutIndex);
        }
    }
    else
    {
        // 如果有数据，硬件FIFO中有数据，则需要触发
        for (i = 0; i < MCAN_HW_FIFO; i+=2)
        {
//            u16FifoPutIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFQP;
            u16FifoGetIndex = g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_TXFQS.bit.TFGI;
            MCAN_txBufAddReq(MCANA_DRIVER_BASE, u16FifoGetIndex);
        }

    }

    return u16SendSize;

}


void Hw_ClearMCanBusError(Uint16 u16MCanId)
{
    // 清除总线错误
    if ((g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_PSR.bit.BO == 1) && (g_stMCanHandle[u16MCanId].bIsInit == 1))
    {
        g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_CCCR.bit.INIT = 1;
        while(g_pstMCanRegs[u16MCanId]->MCANA_CCCR.bit.INIT != 1){};
        g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_CCCR.bit.INIT = 0;
        while(g_pstMCanRegs[u16MCanId]->MCANA_CCCR.bit.INIT != 0){};
    }
//    if ((g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_CCCR.bit.INIT == 1) && (g_stMCanHandle[u16MCanId].bIsInit == 1))
//    {
//        g_stMCanHandle[u16MCanId].pstMCanRegs->MCANA_CCCR.bit.INIT = 0;
//    }
}


#ifdef __cplusplus
    }
#endif

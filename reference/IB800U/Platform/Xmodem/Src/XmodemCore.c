//#include "XmodemCore.h"
//#include "stdlib.h"
//#include "string.h"
//#include "Hw_GpioDriver.h"
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//static const unsigned short crc16tab[256]= {
//    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
//    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
//    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
//    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
//    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
//    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
//    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
//    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
//    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
//    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
//    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
//    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
//    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
//    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
//    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
//    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
//    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
//    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
//    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
//    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
//    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
//    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
//    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
//    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
//    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
//    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
//    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
//    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
//    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
//    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
//    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
//    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
//};
//
//static Uint16 XmodemCalculateCheckSum(Uint16 *pu16Buff, Uint16 u16Length)
//{
//    Uint16 u16Sum = 0;
//    Uint16 i = 0;
//
//    for (i = 0; i < u16Length; i++)
//    {
//        u16Sum += pu16Buff[i];
//    }
//    return u16Sum & 0xFF;
//}
//
//static Uint16 XmodemCalculateCRCSum(Uint16 *pu16Buff, Uint16 u16Length)
//{
//    Uint16 u16Crc = 0;
//    Uint16 i = 0;
//    Uint16 u16Index = 0;
//
//    for (i = 0; i < u16Length; i++)
//    {
//        u16Index = (u16Crc >> 8) ^ pu16Buff[i];
//        u16Crc = (u16Crc << 8) ^ crc16tab[u16Index];
//    }
//    return u16Crc;
//}
//
//
//static Uint16 XmodemCheck(ST_XMODEM_PROTOCOL *pstProtocol, Uint16 *pu16Buff, Uint16 u16Length)
//{
//    Uint16 u16Crc = 0;
//    Uint16 u16DrcCrc = 0;
//    if (pstProtocol == NULL)
//    {
//        return 0;
//    }
//
//    if (pstProtocol->stXmodemConfig.u16IsUseCrc)
//    {
//        u16Crc = XmodemCalculateCRCSum(pu16Buff, u16Length);
//        u16DrcCrc = ((pu16Buff[u16Length] << 8) & 0xFF00) | (pu16Buff[u16Length+1] & 0xFF);
//    }
//    else
//    {
//        u16Crc = XmodemCalculateCheckSum(pu16Buff, u16Length);
//        u16DrcCrc = pu16Buff[u16Length];
//    }
//
//    if (u16DrcCrc == u16Crc)
//    {
//        return 1;
//    }
//    else
//    {
//        return 0;
//    }
//
//}
//
//
//
//E_XMODEM_ERROR XmodemInit(ST_XMODEM_PROTOCOL *pstProtocol, ST_XMODEM_CONFIG *pstXmodemConfig,
//                          ST_TRANSPORT_INTF *pstTransport, ST_DATA_HANDLER_INTF *pstDataHandler,
//                          ST_TIMEOUT_MANAGER *pstTimeOutManager)
//{
//    if ((pstProtocol == NULL) || (pstXmodemConfig == NULL) || (pstXmodemConfig == NULL) || (pstXmodemConfig == NULL))
//    {
//        return XMEDEM_PARAM_ERROR;
//    }
//
//    // 配置协议
//    pstProtocol->stXmodemConfig.u16MaxRetries = pstXmodemConfig->u16MaxRetries;
//    pstProtocol->stXmodemConfig.u16TimeOut = pstXmodemConfig->u16TimeOut;
//    pstProtocol->stXmodemConfig.u16IsUseCrc = pstXmodemConfig->u16IsUseCrc;
//
//    // 注册传输层接口
//    pstProtocol->stTransport.Write = pstTransport->Write;
//    pstProtocol->stTransport.Read = pstTransport->Read;
//
//    // 注册数据处理接口
//    pstProtocol->stDataHandler.WriteBlock = pstDataHandler->WriteBlock;
//    pstProtocol->stDataHandler.ReadBlock = pstDataHandler->ReadBlock;
//    pstProtocol->stDataHandler.Trig = pstDataHandler->Trig;
//    pstProtocol->stDataHandler.Reset = pstDataHandler->Reset;
//    pstProtocol->stDataHandler.TrigWriteCfg = pstDataHandler->TrigWriteCfg;
//    pstProtocol->stDataHandler.ResetWriteCfg = pstDataHandler->ResetWriteCfg;
//    pstProtocol->stDataHandler.SetTotal = pstDataHandler->SetTotal;
//
//    // 超时模块注册接口
//    pstProtocol->stTimeOutManager.stTimeDriver.TimeInit = pstTimeOutManager->stTimeDriver.TimeInit;
//    pstProtocol->stTimeOutManager.stTimeDriver.TimeStart = pstTimeOutManager->stTimeDriver.TimeStart;
//    pstProtocol->stTimeOutManager.stTimeDriver.IsTimeOut = pstTimeOutManager->stTimeDriver.IsTimeOut;
//    pstProtocol->stTimeOutManager.stTimeDriver.TimeStop = pstTimeOutManager->stTimeDriver.TimeStop;
//
//    // 复位状态变量
//    pstProtocol->u16CurrentBlock = 0;
//    pstProtocol->bRetryCount = 0;
//    pstProtocol->stXmodemStateMachine = STATE_IDLE;
//    pstProtocol->stTimeOutManager.u16IsActive = 0;
//    pstProtocol->stTimeOutManager.u32StartTick = 0;
//    pstProtocol->stTimeOutManager.u32TimeOut = 0;
//    pstProtocol->bCurrentPackNo = 1;
//    pstProtocol->bLastPackNo = 1;
//    memset(pstProtocol->u16xBuff, 0, XMODEM_BUFF_SIZE);
//
//    return XMODEM_OK;
//}
//
//
//E_XMODEM_ERROR XmodemSinglePacket(ST_XMODEM_PROTOCOL *pstProtocol)
//{
//    Uint16 u16DataLength = 0;
//    Uint32 u32Sum;
//
//    switch (pstProtocol->u16xBuff[0])
//    {
//        case SOH:
//            u16DataLength = 128;
//            break;
//
//        case STX:
//            u16DataLength = 1024;
//            break;
//
//        case EOT:
//            // 为了增加健壮性
//            // 只收到一个表示异常错误,需要回复一个NAK询问是否中止
//            return XMODEM_EOT_ERROR;
//
//        case CAN:
//            // 收到连续的俩个CAN命令，表示中止传输，回复一个ACK
//            if(pstProtocol->u16xBuff[1] == CAN)
//            {
//                return XMODEM_CANCEL;
//            }
//            // 只收到一个表示异常错误,需要回复一个NAK询问是否中止
//            return XMODEM_CAN_ERROR;
//
//        default:
//            // 需要回复一个NAK，请求重发
//            return XMODEM_IO_ERROR;
//    }
//
//    if ((pstProtocol->u16xBuff[1] != pstProtocol->bCurrentPackNo) && (pstProtocol->u16xBuff[1] != pstProtocol->bLastPackNo))
//    {
//        // 包序号错误
//        return XMODEM_SEQUENCE_ERROR;
//    }
//
//    if ((pstProtocol->u16xBuff[1] == ((~pstProtocol->u16xBuff[2]) & 0xFF))
//        && (XmodemCheck(pstProtocol, &pstProtocol->u16xBuff[3], u16DataLength)))      // 这里做CRC校验
//    {
//        if (pstProtocol->bIsFirstPack == 1)
//        {
//            // 如果这里是新的一包数据才执行写操作，如何还是上一包数据就不执行写操作了
//            if (pstProtocol->u16xBuff[1] == pstProtocol->bCurrentPackNo)
//            {
//                // 只有接收的数据长度小于文件头记录的总长度，才允许写入
//                pstProtocol->u16RecvLength += u16DataLength;
//                if (pstProtocol->u16RecvLength <= pstProtocol->stUpgradeFileHeader.u32FileSize)
//                {
//                    // 调用存储介质写接口
//                     pstProtocol->stDataHandler.WriteBlock(pstProtocol->bCurrentPackNo, &pstProtocol->u16xBuff[3], u16DataLength);
//                }
//                else
//                {
//                    // 这里需要处理最后一包实际写入大小
//                    u16DataLength = u16DataLength - (pstProtocol->u16RecvLength - pstProtocol->stUpgradeFileHeader.u32FileSize);
//                    pstProtocol->stDataHandler.WriteBlock(pstProtocol->bCurrentPackNo, &pstProtocol->u16xBuff[3], u16DataLength);
//
//                    pstProtocol->stDataHandler.TrigWriteCfg(&pstProtocol->stUpgradeFileHeader, 1);
//                    pstProtocol->bRecvOver = 1;
//                    return XMODEM_END;
//                }
//
//
//                // 更新包序号
//                pstProtocol->bLastPackNo = pstProtocol->bCurrentPackNo;
//                pstProtocol->bCurrentPackNo++;
//
//    //          // 有的上位机不会发送包序号为0的数据包，这里需要辨别
//    //            if ((pstProtocol->bCurrentPackNo == 0) || (pstProtocol->bCurrentPackNo > 255))
//    //            {
//    //                pstProtocol->bCurrentPackNo = 1;
//    //            }
//            }
//        }
//        else    // 这里需要解析文件的第一包数据，里面存放了升级文件的基本信息
//        {
//            pstProtocol->bIsFirstPack = 1;      // 标记已经收到了第一包
//            pstProtocol->stUpgradeFileHeader.bChipType = pstProtocol->u16xBuff[3];
//            pstProtocol->stUpgradeFileHeader.bFirmwareType = pstProtocol->u16xBuff[4];
//            pstProtocol->stUpgradeFileHeader.u32FileSize = (Uint32)pstProtocol->u16xBuff[5] | ((Uint32)pstProtocol->u16xBuff[6] << 8) |
//                                                            ((Uint32)pstProtocol->u16xBuff[7] << 16) | ((Uint32)pstProtocol->u16xBuff[8] << 24);
//            pstProtocol->stUpgradeFileHeader.u32FileCrc = (Uint32)pstProtocol->u16xBuff[9] | ((Uint32)pstProtocol->u16xBuff[10] << 8) |
//                                                            ((Uint32)pstProtocol->u16xBuff[11] << 16) | ((Uint32)pstProtocol->u16xBuff[12] << 24);
//
//            // 触发Flash状态机
//            pstProtocol->stDataHandler.Trig();
//
//            // 给状态机设置一个中止条件，将文件大小传给Flash状态机
//            pstProtocol->stDataHandler.SetTotal(pstProtocol->stUpgradeFileHeader.u32FileSize);
//
//            // 调用存储介质写接口,这里减少了32个字节是减去了文件头的内容
//            // 只有接收的数据长度小于文件头记录的总长度，才允许写入
//            pstProtocol->u16RecvLength += (u16DataLength - 32);
//
//            if (pstProtocol->u16RecvLength <= pstProtocol->stUpgradeFileHeader.u32FileSize)
//            {
//                // 调用存储介质写接口
//                 pstProtocol->stDataHandler.WriteBlock(pstProtocol->bCurrentPackNo, &pstProtocol->u16xBuff[35], u16DataLength - 32);
//            }
//            else
//            {
//                return XMODEM_END;
//            }
//
//            // 更新包序号
//            pstProtocol->bLastPackNo = pstProtocol->bCurrentPackNo;
//            pstProtocol->bCurrentPackNo++;
//        }
//
//
//
//
//        return XMODEM_OK;
//    }
//
//    // CRC校验错误
//    return XMODEM_CRC_ERROR;
//}
//
//
//
//
//E_XMODEM_ERROR XmodemReceive(ST_XMODEM_PROTOCOL *pstProtocol)
//{
//    Uint16 u16Retry = 0;
//
//    Uint16 u16TryChar = 'C';
//    Uint16 u16DataLength = 0;
//    int16 i16ReceiveNum = 0;
//    E_XMODEM_ERROR eRet = XMODEM_OK;
//
//    if (pstProtocol == NULL)
//    {
//        return XMEDEM_PARAM_ERROR;
//    }
//
//    switch (pstProtocol->stXmodemStateMachine)
//    {
//        case STATE_IDLE:       // 发送启动字符C，收到第一包数据后，进入下一个状态
//            if (pstProtocol->bRetryCount >= pstProtocol->stXmodemConfig.u16MaxRetries)
//            {
//                // 达到重试阈值，退出升级
//                pstProtocol->bError = XMODEM_RETRY_ERROR;
//                pstProtocol->stXmodemStateMachine = STATE_ERROR;
//                break;
//            }
//
//
//            pstProtocol->bLastPackNo = 1;
//            pstProtocol->bCurrentPackNo = 1;
//            pstProtocol->u16CurrentBlock = 0;
//            pstProtocol->stTimeOutManager.u16IsActive = 0;
//            pstProtocol->stTimeOutManager.u32StartTick = 0;
//            pstProtocol->stTimeOutManager.u32TimeOut = 0;
//            pstProtocol->bIsFirstPack = 0;
//            pstProtocol->bRecvOver = 0;
//            u16TryChar = 'C';
//            pstProtocol->stTransport.Write(&u16TryChar, 1);
//            pstProtocol->stXmodemStateMachine = STATE_WAIT_FOR_START;
//            pstProtocol->bError = XMODEM_OK;
//
//            break;
//
//        case STATE_WAIT_FOR_START:
//
//            // 启动超时定时器
//            pstProtocol->stTimeOutManager.stTimeDriver.TimeStart(&pstProtocol->stTimeOutManager, pstProtocol->stXmodemConfig.u16TimeOut);
//            // 等待接收一整包数据（整包数据的实现在驱动层做了数据断帧）
//            i16ReceiveNum = pstProtocol->stTransport.Read(pstProtocol->u16xBuff, XMODEM_BUFF_SIZE);
//            if (i16ReceiveNum <= 0)
//            {
//                // 如果没有超时
//                if (!pstProtocol->stTimeOutManager.stTimeDriver.IsTimeOut(&pstProtocol->stTimeOutManager))
//                {
//                    // 进入解析状态STATE_PROCESS_PACKET
//                    pstProtocol->bError = XMODEM_OK;
//                    pstProtocol->stXmodemStateMachine = STATE_WAIT_FOR_START;
//
//                }
//                // 如果超时了则进入重试阶段
//                else
//                {
//                    // 停止超时定时器
//                    pstProtocol->stTimeOutManager.stTimeDriver.TimeStop(&pstProtocol->stTimeOutManager);
//                    pstProtocol->bError = XMODEM_TIMEOUT;
//                    pstProtocol->stXmodemStateMachine = STATE_IDLE;
//                    pstProtocol->bRetryCount++;
//                }
//            }
//            else
//            {
//                // 停止超时定时器
//                pstProtocol->stTimeOutManager.stTimeDriver.TimeStop(&pstProtocol->stTimeOutManager);
//                // 启动阶段完成，进入下一个状态前需要复位
//                pstProtocol->bRetryCount = 0;
//                pstProtocol->stXmodemStateMachine = STATE_PROCESS_PACKET;
//            }
//            break;
//
//        case STATE_PROCESS_PACKET:
//
//            // 这里封装一个单包数据解析
//            pstProtocol->bError = XmodemSinglePacket(pstProtocol);
//
//            if (pstProtocol->bError == XMODEM_OK || pstProtocol->bError == XMODEM_CANCEL
//                    || pstProtocol->bError == XMODEM_END)
//            {
//                u16TryChar = ACK;
//                pstProtocol->bRetryCount = 0;
//                pstProtocol->bCANCount = 0;
//                pstProtocol->bEOTCount = 0;
//                pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//            }
//            // 连续收到两次EOT或者CAN指令才认为结束
//            // 在考虑到收到错误的结束帧或者未定义指令，这里需要做个容错
//            else if ((pstProtocol->bError == XMODEM_CAN_ERROR))
//            {
//                // 这里需要清空接收缓存
//                // 这里需要调用串口的控制接口
//
//                pstProtocol->bCANCount++;
//                u16TryChar = NAK;
//                pstProtocol->stTransport.Write(&u16TryChar, 1);
//                pstProtocol->bRetryCount++;
//
//                // 连续收到两次CAN表示传输结束
//                if (pstProtocol->bCANCount >= 2)
//                {
//                    pstProtocol->bError = XMODEM_CANCEL;
//                    pstProtocol->stXmodemStateMachine = STATE_COMPLETE;
//                }
//                else
//                {
//                    pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//                }
//            }
//            else if (pstProtocol->bError == XMODEM_EOT_ERROR)
//            {
//                // 这里需要清空接收缓存
//                // 这里需要调用串口的控制接口
//                u16TryChar = NAK;
//                pstProtocol->bEOTCount++;
//                pstProtocol->bRetryCount++;
//
//                // 连续收到两次EOT表示传输结束
//                if (pstProtocol->bEOTCount >= 2)
//                {
//                    pstProtocol->bError = XMODEM_END;
//                    pstProtocol->stXmodemStateMachine = STATE_COMPLETE;
//                }
//                else
//                {
//                    pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//                }
//            }
//            // 收到了未定义的帧头
//            else if (pstProtocol->bError == XMODEM_IO_ERROR)
//            {
//                // 这里需要清空接收缓存
//                // 这里需要调用串口的控制接口
//                u16TryChar = NAK;
//                pstProtocol->bRetryCount++;
//                pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//            }
//            // 其他错误
//            else if (pstProtocol->bError == XMODEM_PACKET_ERROR || pstProtocol->bError == XMODEM_SEQUENCE_ERROR
//                     || pstProtocol->bError == XMODEM_CRC_ERROR)
//            {
//                u16TryChar = NAK;
//                pstProtocol->bRetryCount++;
//                pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//            }
//
//            // 重试判断
//            if (pstProtocol->bRetryCount <= pstProtocol->stXmodemConfig.u16MaxRetries)
//            {
//                pstProtocol->stTransport.Write(&u16TryChar, 1);
//            }
//            else
//            {
//                u16TryChar = CAN;
//                pstProtocol->stTransport.Write(&u16TryChar, 1);
//                pstProtocol->stTransport.Write(&u16TryChar, 1);
//                pstProtocol->stTransport.Write(&u16TryChar, 1);
//                // 达到重试阈值，退出升级
//                pstProtocol->bError = XMODEM_RETRY_ERROR;
//                pstProtocol->stXmodemStateMachine = STATE_ERROR;
//            }
//
//            break;
//
//        case STATE_RECEIVE_DATA:
//            // 启动超时定时器
//            pstProtocol->stTimeOutManager.stTimeDriver.TimeStart(&pstProtocol->stTimeOutManager, pstProtocol->stXmodemConfig.u16TimeOut);
//            // 等待接收一整包数据（整包数据的实现在驱动层做了数据断帧）
//            i16ReceiveNum = pstProtocol->stTransport.Read(pstProtocol->u16xBuff, XMODEM_BUFF_SIZE);
//            if (i16ReceiveNum <= 0)
//            {
//                // 如果没有超时
//                if (!pstProtocol->stTimeOutManager.stTimeDriver.IsTimeOut(&pstProtocol->stTimeOutManager))
//                {
//                    // 进入解析状态STATE_RECEIVE_DATA
//                    pstProtocol->bError = XMODEM_OK;
//                    pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//                }
//                // 如果超时了则进入重试阶段
//                else
//                {
//                    // 停止超时定时器
//                    pstProtocol->stTimeOutManager.stTimeDriver.TimeStop(&pstProtocol->stTimeOutManager);
//                    pstProtocol->bError = XMODEM_TIMEOUT;
//                    pstProtocol->bRetryCount++;
//                    if (pstProtocol->bRetryCount <= pstProtocol->stXmodemConfig.u16MaxRetries)
//                    {
//                        u16TryChar = NAK;
//                        pstProtocol->stTransport.Write(&u16TryChar, 1);
//                        pstProtocol->stXmodemStateMachine = STATE_RECEIVE_DATA;
//                    }
//                    else
//                    {
//                        u16TryChar = CAN;
//                        pstProtocol->stTransport.Write(&u16TryChar, 1);
//                        pstProtocol->stTransport.Write(&u16TryChar, 1);
//                        pstProtocol->stTransport.Write(&u16TryChar, 1);
//                        // 达到重试阈值，退出升级
//                        pstProtocol->bError = XMODEM_RETRY_ERROR;
//                        pstProtocol->stXmodemStateMachine = STATE_ERROR;
//                    }
//                }
//            }
//            else
//            {
//                // 停止超时定时器
//                pstProtocol->stTimeOutManager.stTimeDriver.TimeStop(&pstProtocol->stTimeOutManager);
//                pstProtocol->stXmodemStateMachine = STATE_PROCESS_PACKET;
//            }
//
//            break;
//
//        case STATE_COMPLETE:
//            // 传输完成
//            eRet = pstProtocol->bError;
//            pstProtocol->bRetryCount = 0;
//            pstProtocol->bIsFirstPack = 0;
//            pstProtocol->stXmodemStateMachine = STATE_IDLE;
//            break;
//
//        case STATE_ERROR:
//            // 可做一些错误处理
//
//            eRet = pstProtocol->bError;
//            // 如果Xmodem出现了错误，那么需要停止Flash状态机的运行
//            pstProtocol->stDataHandler.Reset();
//            pstProtocol->bRetryCount = 0;
//            pstProtocol->stXmodemStateMachine = STATE_IDLE;
//            break;
//
//    }
//
//    return eRet;
//}
//
//
//
//E_XMODEM_ERROR XmodemTransmit(ST_XMODEM_PROTOCOL *pstProtocol)
//{
//
//}
//
//
//
//
//#ifdef __cplusplus
//}
//#endif

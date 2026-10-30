/**
 * @file Hw_I2cDriver.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_I2cDriver.h"
#include "i2c.h"
#include "device.h"

#ifdef __cplusplus
    extern "C" {
#endif


static volatile struct I2C_REGS *pstI2cRegs[] = {&I2caRegs, &I2cbRegs};



/**
 * @brief 用于初始化单个I2C外设
 * 
 * @param pstI2cCfgItem 入参是一个ST_I2C_CFG_ITEM结构体对象的地址
 */
void Hw_SingleI2cInit(ST_I2C_CFG_ITEM *pstI2cCfgItem)
{
    Uint16 u16I2cId = 0;

    u16I2cId = pstI2cCfgItem->u16I2cId;

    I2C_disableModule((Uint32)pstI2cRegs[u16I2cId]);                                // I2c进入复位状态

    I2C_initMaster((Uint32)pstI2cRegs[u16I2cId], DEVICE_SYSCLK_FREQ,
                       pstI2cCfgItem->u32Speed, I2C_DUTYCYCLE_33);          // 设置I2c通信速率

    I2C_setBitCount((Uint32)pstI2cRegs[u16I2cId], I2C_BITCOUNT_8);                          // 数据位宽8bit
    I2C_setSlaveAddress((Uint32)pstI2cRegs[u16I2cId], pstI2cCfgItem->u16TargetAddr);       // 设置从机通信地址
    I2C_setEmulationMode((Uint32)pstI2cRegs[u16I2cId], I2C_EMULATION_FREE_RUN);             // 仿真模式下全速运行

    //
    // Enable stop condition and register-access-ready interrupts
    I2C_enableInterrupt((Uint32)pstI2cRegs[u16I2cId], I2C_INT_STOP_CONDITION |
                                   I2C_INT_REG_ACCESS_RDY);

    //
    // FIFO configuration
    //
    I2C_enableFIFO((Uint32)pstI2cRegs[u16I2cId]);
    I2C_clearInterruptStatus((Uint32)pstI2cRegs[u16I2cId], I2C_INT_RXFF | I2C_INT_TXFF);

    I2C_enableModule((Uint32)pstI2cRegs[u16I2cId]);                             // 启动I2c

}



/**
 * @brief 用于初始化一组I2C，通常用于参数初始化阶段
 * 
 * @param pstI2cCfg :入参是一组ST_I2C_CFG结构体对象的集合
 */
void Hw_I2cInit(ST_I2C_CFG *pstI2cCfg)
{
    Uint16 i = 0;

    for (i = 0; i < pstI2cCfg->u16ItemNum; i++)
    {
        Hw_SingleI2cInit(&pstI2cCfg->pstI2cCfgItem[i]);
    }
}

#if 0

/**
 * @brief 提交读/写命令请求，将请求提交到CMD缓存中
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，里面需要填写读取的从机设备地址、目标内存地址、读取方向、BUFF空间等
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际写入请求命令的个数
 */
int16 Hw_I2cSubmitRWReuest(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg)
{
    Uint16 u16WriteSize = 0;

    if ((u16I2cId < I2C_ID0) || (u16I2cId >= I2C_ID_MAX))
    {
        return -1;  // ID非法
    }

    if (stI2cHandle[u16I2cId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstI2cMsg == NULL)
    {
        return -3;  // pstI2cMsg 非法
    }

    if (stI2cHandle[u16I2cId].u16DeviceBusy == 1)
    {
        return -4;  // 设备处于繁忙状态
    }
    stI2cHandle[u16I2cId].u16DeviceBusy = 1;

    u16WriteSize = WriteRingBuff((ST_RING_BUFF *)&stI2cHandle[u16I2cId].stCmdRingBuff, (void *)pstI2cMsg, 1);

    stI2cHandle[u16I2cId].u16DeviceBusy = 0;

    return u16WriteSize;
}



int16 Hw_I2cGetReadResult(Uint16 u16I2cId)
{
    return stI2cHandle[u16I2cId].stI2cCurrentProcMsg.u16MsgVaild;
}


/**
 * @brief 从接收FIFO中取出读出来的I2C消息
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，该消息需要指定对应的消息ID
 * @param u16Size   :指定读取消息的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际读取的个数
 */
int16 Hw_I2cRead(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg, Uint16 u16Index)
{
    Uint16 u16ReadSize = 0;
    Uint16 i = 0, j = 0;;
    int16 iRet = 0;

    if ((u16I2cId < I2C_ID0) || (u16I2cId >= I2C_ID_MAX))
    {
        return -1;  // ID非法
    }

    if (stI2cHandle[u16I2cId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstI2cMsg == NULL)
    {
        return -3;  // pstI2cMsg 非法
    }

    if (stI2cHandle[u16I2cId].u16DeviceBusy == 1)
    {
        return -4;  // 设备处于繁忙状态
    }
    stI2cHandle[u16I2cId].u16DeviceBusy = 1;

    memcpy(pstI2cMsg, &stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg, sizeof(ST_I2C_MSG));
    stI2cHandle[u16I2cId].stI2cCurrentProcMsg.u16MsgVaild = 0;

    stI2cHandle[u16I2cId].u16DeviceBusy = 0;

    return iRet;
}



/**
 * @brief 将指定的I2C消息对象写入到发送FIFO中
 * 
 * @param u16I2cId  :I2C的ID号
 * @param pstI2cMsg :I2C的消息对象的地址，该消息需要填充完整属性
 * @param u16Size   :指定写入消息的个数
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :-3 pstI2cMsg参数为NULL
 *                  :-4 I2C设备处于繁忙状态，正被其他线程使用
 *                  :>=0 实际写入的个数
 */
int16 Hw_I2cWrite(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg, Uint16 u16Size)
{
    Uint16 u16MsgId = 0;
    Uint16 u16WriteSize = 0;

    if ((u16I2cId < I2C_ID0) || (u16I2cId >= I2C_ID_MAX))
    {
        return -1;  // ID非法
    }

    if (stI2cHandle[u16I2cId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    if (pstI2cMsg == NULL)
    {
        return -3;  // pstI2cMsg 非法
    }

    if (stI2cHandle[u16I2cId].u16DeviceBusy == 1)
    {
        return -4;  // 设备处于繁忙状态
    }

    stI2cHandle[u16I2cId].u16DeviceBusy = 1;

    u16WriteSize =  WriteRingBuff((ST_RING_BUFF *)&stI2cHandle[u16I2cId].stCmdRingBuff, (void *)pstI2cMsg, u16Size);

    stI2cHandle[u16I2cId].u16DeviceBusy = 0;

    return u16WriteSize;
}



/**
 * @brief I2C外设的控制接口(待实现)
 * 
 * @param u16I2cId :I2C的ID号
 * @param u16Cmd :I2C命令枚举
 * @param vParam :参数
 * @return int16 
 */
int16 Hw_I2cCtrl(Uint16 u16I2cId, Uint16 Cmd, void *pvParam)
{

    ST_I2C_COMM_CFG *pstI2cCommCfg = NULL;
    ST_I2C_MSG *pstI2cMsg = NULL;

    switch (Cmd)
    {
    case 0:
        pstI2cCommCfg = (ST_I2C_COMM_CFG *)pvParam;
        stI2cHandle[u16I2cId].stI2cCommCfg.bTargetAddr = pstI2cCommCfg->bTargetAddr;
        stI2cHandle[u16I2cId].stI2cCommCfg.u32Speed = pstI2cCommCfg->u32Speed;
        stI2cHandle[u16I2cId].stI2cCommCfg.u32WrAddr = pstI2cCommCfg->u32WrAddr;
        break;

    case 1:
        pstI2cMsg = (ST_I2C_MSG *)pvParam;

        break;
    }
}



/**
 * @brief I2C数据接收和发送的状态机
 * 
 * @param u16I2cId :I2C的ID号
 * @return int16    :-1 ID非法
 *                  :-2 未初始化
 *                  :0 结束
 */
int16 Hw_IsrI2cTransceiver(Uint16 u16I2cId)
{
    Uint16 u16ReadSize = 0;
    ST_I2C_MSG stI2cMsg;
    Uint16 i = 0;
    Uint16 u16FreeFifoNum = 0;
    Uint16 u16SendNum, u16ReadNum;
    Uint16 u16Index = 0;
    ST_RECE_MESSAGE *pstReceMessage = NULL;

    if ((u16I2cId < I2C_ID0) || (u16I2cId >= I2C_ID_MAX))
    {
        return -1;  // 参数非法
    }

    if (stI2cHandle[u16I2cId].bIsInit != 1)
    {
        return -2;  // 未初始化
    }

    stI2cMsg.bAddr = 0;
    stI2cMsg.bDir = 0;
    stI2cMsg.bMsgId = 0;
    stI2cMsg.u16DataLength = 0;
    stI2cMsg.u32WrAddr = 0;

    switch(stI2cHandle[u16I2cId].eI2cTransceiverState)
    {
        case I2C_STATE_IDLE:

            // 从CmdFIFO中取出请求命令
            u16ReadSize = ReadRingBuff((ST_RING_BUFF *)&stI2cHandle[u16I2cId].stCmdRingBuff,
                                       (void *)&stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg, 1);

            if (u16ReadSize)
            {
                // 复位过程变量
                stI2cHandle[u16I2cId].u16ReadDataCnt = 0;
                stI2cHandle[u16I2cId].u16SendDataCnt = 0;
                stI2cHandle[u16I2cId].i16RemainData = 0;
                stI2cHandle[u16I2cId].bTimeOutCnt = 0;

                // 复位IIC模块
                I2C_disableFIFO((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                I2C_disableModule((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                I2C_enableModule((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                I2C_enableFIFO((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);

                stI2cHandle[u16I2cId].stI2cCurrentProcMsg.u16MsgVaild = 0;
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_SEND_CMD;
            }
            break;

        case I2C_STATE_SEND_CMD:

            // 判断I2C外设是否处于繁忙状态
            if ((I2C_isBusBusy((Uint32)stI2cHandle[u16I2cId].pstI2cRegs))
                    || (I2C_getStopConditionStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs)))
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_ERROR;
                break;
            }
            else
            {
                // 设置为主机发送模式
                I2C_setConfig((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, (I2C_MASTER_SEND_MODE|I2C_REPEAT_MODE));

                // 根据目标设备地址配置I2C通信地址模式
                if (stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.bAddr <= 127)
                {
                    I2C_setAddressMode((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, I2C_ADDR_MODE_7BITS);
                }
                else
                {
                    I2C_setAddressMode((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, I2C_ADDR_MODE_10BITS);
                }

                // 设置目标从机通信地址
                I2C_setSlaveAddress((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.bAddr);

                // 将访问设备的内部地址写入FIFO中
                I2C_putData((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, (stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u32WrAddr >> 8) & 0xFF);
                I2C_putData((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, (stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u32WrAddr ) & 0xFF);

                // 发送开始条件
                I2C_sendStartCondition((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                stI2cHandle[u16I2cId].bTimeOutCnt = 0;
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_WAIT_CMD_ACK;
            }
            break;

        case I2C_STATE_WAIT_CMD_ACK:

            // 检查是否存在NACK信号
            if (I2C_getStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs) & I2C_STS_NO_ACK)
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_ERROR;
                break;

            }

            // 检查发送缓存区是否为空
            if (I2C_getTxFIFOStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs) == 0)
            {
                // 发送数据
                if (stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.bDir == 0)
                {
                    // 配置成主机发送模式
                    stI2cHandle[u16I2cId].i16RemainData = stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u16DataLength;
                    I2C_setConfig((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, (I2C_MASTER_SEND_MODE|I2C_REPEAT_MODE));
                    stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_SEAD_DATA;
                }
                // 读取数据
                else
                {
                    // 配置成主机接收模式
                    I2C_setConfig((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, (I2C_MASTER_RECEIVE_MODE|I2C_REPEAT_MODE));
                    stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_READ_DATA;

                    // 产生起始条件
                    I2C_sendStartCondition((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                }
            }
            break;

        case I2C_STATE_SEAD_DATA:

            // 先检查是否存在NAK，如果存在，就需要跳转到错误处理
            if ((I2C_getStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs) & I2C_STS_NO_ACK) == I2C_STS_NO_ACK)
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_ERROR;
                break;
            }

            // 计算可用FIFO

            u16FreeFifoNum = 16 - I2C_getTxFIFOStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);


            // 计算本次发送的数量
            u16SendNum = stI2cHandle[u16I2cId].i16RemainData < u16FreeFifoNum ? stI2cHandle[u16I2cId].i16RemainData : u16FreeFifoNum;

            for (i = 0; i < u16SendNum; i++)
            {
                I2C_putData((Uint32)stI2cHandle[u16I2cId].pstI2cRegs,
                                            stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u16TargetBuff[stI2cHandle[u16I2cId].u16SendDataCnt]);

                stI2cHandle[u16I2cId].u16SendDataCnt += 1;
            }

            stI2cHandle[u16I2cId].i16RemainData -= u16SendNum;

            // 数据已全部发送，跳转发送停止命令
            if ((stI2cHandle[u16I2cId].i16RemainData <= 0) && (stI2cHandle[u16I2cId].pstI2cRegs->I2CSTR.bit.XSMT == 0))
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_WAIT_STOP;
            }
            break;


        case I2C_STATE_READ_DATA:
            // 先检查是否存在NAK，如果存在，就需要跳转到错误处理
            if (I2C_getStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs) & I2C_STS_NO_ACK)
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_ERROR;
                break;
            }

            // 100Kbps:1us一个bit，100us的调度周期可以接收11个字节
            // 接收FIFO一共有16个，所以FIFO不会溢出
            u16ReadNum = I2C_getRxFIFOStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);

            for (i = 0; i < u16ReadNum; i++)
            {
                // 需要判断接收数据长度是否大于目标接收长度，避免出现内存溢出
                if (stI2cHandle[u16I2cId].u16ReadDataCnt >= stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u16DataLength)
                {
                    break;
                }
                stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u16TargetBuff[(stI2cHandle[u16I2cId].u16ReadDataCnt) + i] = I2C_getData((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
                stI2cHandle[u16I2cId].u16ReadDataCnt++;
            }

            // 接收到目标长度，将消息拷贝进内存池
            if (stI2cHandle[u16I2cId].u16ReadDataCnt >= stI2cHandle[u16I2cId].stI2cCurrentProcMsg.stI2cMsg.u16DataLength)
            {
                stI2cHandle[u16I2cId].u16ReadDataCnt = 0;
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_WAIT_STOP;
                break;
            }

            break;

        case I2C_STATE_WAIT_STOP:
            // 先检查是否存在NAK，如果存在，就需要跳转到错误处理
            if (I2C_getStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs) & I2C_STS_NO_ACK)
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_ERROR;
                break;
            }
            // 发送停止命令
            I2C_sendStopCondition((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
            stI2cHandle[u16I2cId].stI2cCurrentProcMsg.u16MsgVaild = 1;
            stI2cHandle[u16I2cId].bTimeOutCnt = 0;
            stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_IDLE;
            break;

        case I2C_STATE_ERROR:

            // 复位模块
            I2C_clearStatus((Uint32)stI2cHandle[u16I2cId].pstI2cRegs, I2C_STS_NO_ACK);
            I2C_disableFIFO((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
            I2C_sendStopCondition((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);
            I2C_enableFIFO((Uint32)stI2cHandle[u16I2cId].pstI2cRegs);

            // 重试判断
            stI2cHandle[u16I2cId].bTimeOutCnt++;
            if (stI2cHandle[u16I2cId].bTimeOutCnt < stI2cHandle[u16I2cId].bTimeOut)
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_SEND_CMD;
            }
            else
            {
                stI2cHandle[u16I2cId].eI2cTransceiverState = I2C_STATE_IDLE;
            }

            break;

    }
    return 0;
}

#endif


int16 Hw_I2cRead(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg)
{
    Uint16 i = 0;
    switch(pstI2cMsg->u16State)
    {
        case I2C_IDLE:
            if (pstI2cRegs[u16I2cId]->I2CMDR.bit.STP == 1)
            {
                return I2C_STP_NOP_READY_ERROR;
            }
            if (pstI2cRegs[u16I2cId]->I2CSTR.bit.BB == 1)
            {
                return I2C_BUS_BUSY_ERROR;
            }
            pstI2cRegs[u16I2cId]->I2CSAR.all = pstI2cMsg->u16SlaveAddr;
            pstI2cRegs[u16I2cId]->I2CCNT = 2;
            pstI2cRegs[u16I2cId]->I2CDXR.all =  (pstI2cMsg->u16MemAddr >> 8) & 0xFF;
            pstI2cRegs[u16I2cId]->I2CDXR.all =  (pstI2cMsg->u16MemAddr) & 0xFF;
            // 使能I2C模块、主机模式、发送模式、自动模式、产生开始条件
            pstI2cRegs[u16I2cId]->I2CMDR.all = 0x2620;

        case I2C_READ_BUSY1:
            if (pstI2cRegs[u16I2cId]->I2CSTR.bit.ARDY == 0)
            {
                pstI2cMsg->u16State = I2C_READ_BUSY1;
                return I2C_READ_BUSY1;
            }
            pstI2cRegs[u16I2cId]->I2CCNT = pstI2cMsg->u16DataLen;
            // 使能I2C模块、主机模式、接收模式、自动模式、产生开始条件、产生结束条件
            pstI2cRegs[u16I2cId]->I2CMDR.all = 0x2C20;

        case I2C_READ_BUSY2:
            if (pstI2cRegs[u16I2cId]->I2CMDR.bit.STP == 1)
            {
                pstI2cMsg->u16State = I2C_READ_BUSY2;
                return I2C_READ_BUSY2;
            }
            for (i = 0; i < pstI2cMsg->u16DataLen; i++)
            {
                pstI2cMsg->u16DataBuff[i] = pstI2cRegs[u16I2cId]->I2CDRR.all;
            }
            pstI2cMsg->u16State = I2C_IDLE;
            return I2C_SUCCESS;

    }
    return I2C_PARAM_ERROR;
}


int16 Hw_I2cWrite(Uint16 u16I2cId, ST_I2C_MSG *pstI2cMsg)
{
    Uint16 i = 0;
    if (pstI2cRegs[u16I2cId]->I2CMDR.bit.STP == 1)
    {
        return I2C_STP_NOP_READY_ERROR;
    }
    if (pstI2cRegs[u16I2cId]->I2CSTR.bit.BB == 1)
    {
        return I2C_BUS_BUSY_ERROR;
    }
    pstI2cRegs[u16I2cId]->I2CSAR.all = pstI2cMsg->u16SlaveAddr;
    pstI2cRegs[u16I2cId]->I2CCNT = pstI2cMsg->u16DataLen + 2;
    pstI2cRegs[u16I2cId]->I2CDXR.all =  (pstI2cMsg->u16MemAddr >> 8) & 0xFF;
    pstI2cRegs[u16I2cId]->I2CDXR.all =  (pstI2cMsg->u16MemAddr) & 0xFF;
    for (i = 0; i < pstI2cMsg->u16DataLen; i++)
    {
        pstI2cRegs[u16I2cId]->I2CDXR.all = pstI2cMsg->u16DataBuff[i];
    }
    // 使能I2C模块、主机模式、发送模式、自动模式、产生开始条件、产生结束条件
    pstI2cRegs[u16I2cId]->I2CMDR.all = 0x2E20;
    return I2C_SUCCESS;
}


#ifdef __cplusplus
    }
#endif

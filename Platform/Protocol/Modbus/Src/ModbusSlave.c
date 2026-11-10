/**
 * @file ModbusSlave.c
 * @author yetingheng (004182)
 * @brief 
 * @version 0.1
 * @date 2025-06-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "ModbusSlave.h"
#include "ModbusPublic.h"
#include "stdlib.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_ERR_BASE_CODE            0x80    // Modbus错误码对应的基本偏移码

/* 请求帧的固定长度（字数）：地址 1 + 功能码 1 + 地址 2 + 数量 2 + CRC 2。 */
#define MODBUS_READ_FRAME_LEN           (8U)
#define MODBUS_WRITE_SINGLE_FRAME_LEN   (8U)
/* 0x10 请求：上面 6 字节之外多一个字节数，再跟 2n 字节数据，最后 2 字节 CRC。 */
#define MODBUS_MULTI_WD_BYTE_NUM_INDEX  (6U)
#define MODBUS_MULTI_WD_FRAME_BASE      (9U)
#define MODBUS_MULTI_WD_REG_MAX         (120U)


static E_MDS_ERROR_TYPE ProcMbReadRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl);
static E_MDS_ERROR_TYPE ProcMbWriteSingleRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl);
static E_MDS_ERROR_TYPE ProcMbWriteMultiRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl);

const ST_CMD_ITEM g_stMdsCmdTable[] =
{
     {MODBUS_READ_CMD,          ProcMbReadRegMsg},          // 读寄存器命令
     {MODBUS_WRITE_SINGLE_CMD,  ProcMbWriteSingleRegMsg},   // 写单个寄存器命令
     {MODBUS_WRITE_MULTI_CMD,   ProcMbWriteMultiRegMsg},          // 写多个寄存器命令
//     {MODBUS_MANAGE_CMD,        ProcMbReadRegMsg},
};


/**
 * @brief ModbusSlaveTableRegister
 *          初始化Modbus从站控制对象，将配置表注册、回调函数、表索引注册进该对象(pstModbusTableCtrl)
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param eTableType - Modbus表索引(表类型、协议类型)
 * @param pstTable  - Modbus寄存器配置表
 * @param pFunc     - Modbus寄存器配置表的回调函数，在处理完对应的Modbus帧后执行
 * @return int32 -  -1：协议数量超限，大于MODBUS_SLAVE_TABLE_MAX
 *                  -2：协议类型eTableType异常
 *                  0：初始化结束
 * 
 */
int32 ModbusSlaveTableRegister(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, E_MODBUS_SLAVE_TABLE_TYPE eTableType,
                               ST_REG_SLAVE_CFG *pstTable, ModbusCallBack pFunc)
{
    // 判断协议数量是否超限
    if (pstModbusTableCtrl->bSlaveTableCount >= MODBUS_SLAVE_TABLE_MAX)
    {
        return -1;
    }

    // 判断入参协议类型是否正确
    if (eTableType >= MODBUS_SLAVE_TABLE_MAX)
    {
        return -2;
    }

    // 将协议点表和类型注册进stSlaveTableArry
    pstModbusTableCtrl->stSlaveTableArry[eTableType].eProtocolType = eTableType;
    pstModbusTableCtrl->stSlaveTableArry[eTableType].pstProtocolTable = pstTable;
    pstModbusTableCtrl->stSlaveTableArry[eTableType].SlaveModbusCallBack = pFunc;

    // 更新从机协议点表计数
    pstModbusTableCtrl->bSlaveTableCount++;

    // 更新eCurrentProtocol为本次协议
    pstModbusTableCtrl->eCurrentSlaveTableIndex = eTableType;

    pstModbusTableCtrl->bModbusSlaveAddr = 1;

    return 0;

}


/**
 * @brief SetModbusSlaveTableType
 *          切换当前处理的Modbus点表
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param eTableType - Modbus表索引(表类型、协议类型)
 * @return int32 -  -1：入参异常
 *                  -2：该协议点表未注册
 *                  0：函数执行完成，可调用GetResultForSetModbusSlaveTableType接口查看切换结果
 */
int32 SetModbusSlaveTableType(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, E_MODBUS_SLAVE_TABLE_TYPE eTableType)
{
    // 判断入参协议类型是否正确
    if (eTableType >= MODBUS_SLAVE_TABLE_MAX)
    {
        return -1;
    }

    // 如果该协议并没有注册进管理器中，则设置失败
    if (pstModbusTableCtrl->stSlaveTableArry[eTableType].pstProtocolTable == NULL)
    {
        return -2;
    }

    // 切换成当前协议
    pstModbusTableCtrl->eCurrentSlaveTableIndexCache = eTableType;

    // 更新状态
    pstModbusTableCtrl->bUpdateState = FALSE;

    return 0;
}


/**
 * @brief GetModbusSlaveTableType
 *          获取Modbus从站当前使用的协议点表
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return E_MODBUS_SLAVE_TABLE_TYPE -  其他值：返回当前使用协议
 *                                      MODBUS_SLAVE_TABLE_NULL：入参错误或者协议点表未注册
 */
E_MODBUS_SLAVE_TABLE_TYPE GetModbusSlaveTableType(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    E_MODBUS_SLAVE_TABLE_TYPE eTableType = MODBUS_SLAVE_TABLE_NULL;

    if (pstModbusTableCtrl != NULL)
    {
        return pstModbusTableCtrl->eCurrentSlaveTableIndex;
    }

    return eTableType;
}


/**
 * @brief SetModbusSlaveAddr
 *          设置从站Modbus通信地址
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param u16Addr - 等待设置的地址
 */
void SetModbusSlaveAddr(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, Uint16 u16Addr)
{
    if (pstModbusTableCtrl != NULL)
    {
        pstModbusTableCtrl->bModbusSlaveAddrCache = u16Addr & 0xFF; 
        
        // 更新状态
        pstModbusTableCtrl->bUpdateState = FALSE;  
    }
}


/**
 * @brief GetModbusSlaveAddr
 *          获取从站Modbus通信地址
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  -1：入参错误
 *                  0-255：通信地址
 */
int16 GetModbusSlaveAddr(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    if (pstModbusTableCtrl != NULL)
    {
        return pstModbusTableCtrl->bModbusSlaveAddr;
    }
    return -1;
}


/**
 * @brief ModbusSlaveParamUpdate
 *          生效SetModbusSlaveAddr()和SetModbusSlaveTableType()的操作
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  -1：入参错误
 *                  0：参数无须更新
 *                  1：参数已更新
 */
int16 ModbusSlaveParamUpdate(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    int16 i16Ret = 0;

    if (pstModbusTableCtrl == NULL)
    {
        return -1;
    }

    // 判断缓存变量是否与当前变量一致
    if (pstModbusTableCtrl->eCurrentSlaveTableIndex != pstModbusTableCtrl->eCurrentSlaveTableIndexCache)
    {
        // 不一致则更新当前变量
        pstModbusTableCtrl->eCurrentSlaveTableIndex = pstModbusTableCtrl->eCurrentSlaveTableIndexCache;

        // 更新状态
        pstModbusTableCtrl->bUpdateState = TRUE;

        i16Ret = 1;
    }

        // 判断缓存变量是否与当前变量一致
    if (pstModbusTableCtrl->bModbusSlaveAddr != pstModbusTableCtrl->bModbusSlaveAddrCache)
    {
        // 不一致则更新当前变量
        pstModbusTableCtrl->bModbusSlaveAddr = pstModbusTableCtrl->bModbusSlaveAddrCache;

        // 更新状态
        pstModbusTableCtrl->bUpdateState = TRUE;

        i16Ret = 1;
    }

    return i16Ret;
}


/**
 * @brief GetResultForParamUpdate
 *          获取SetModbusSlaveAddr()和SetModbusSlaveTableType()是否生效
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @return int16 -  0：参数等待更新
 *                  1：参数已更新
 */
int16 GetResultForParamUpdate(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    return pstModbusTableCtrl->bUpdateState;
}



/* 一个寄存器项在线路上占几个字：32 位类型占 2 字，其余占 1 字。 */
static Uint16 ModbusRegWidthOf(Uint16 u16ValType)
{
    if ((u16ValType == TYPE_INT32) || (u16ValType == TYPE_UINT32) || (u16ValType == TYPE_FLOAT))
    {
        return 2U;
    }

    return 1U;
}


/**
 * @brief ModbusExcCodeOf
 *          把内部错误类型映射成 Modbus 规范的异常码（01/02/03/04/06）。
 *
 *          E_MDS_ERROR_TYPE 里混着内部用的 ERROR_DATA_LEN/ERROR_FRAME/ERROR_CRC，
 *          它们不是规范异常码，不能直接上线；一律按“数据值非法”上报。
 *
 * @param eErrorType - 内部错误类型
 * @return Uint16 - 异常码
 */
static Uint16 ModbusExcCodeOf(E_MDS_ERROR_TYPE eErrorType)
{
    switch (eErrorType)
    {
        case ERROR_FUNC:        return 0x01U;   /* 非法功能 */
        case ERROR_ADDRESS:     return 0x02U;   /* 非法数据地址 */
        case ERROR_DATA:        return 0x03U;   /* 非法数据值 */
        case ERROR_SLAVE_FAULT: return 0x04U;   /* 从站设备故障 */
        case SLAVE_BUSY:        return 0x06U;   /* 从站设备忙 */
        default:                return 0x03U;   /* 内部长度/帧/CRC 类错误不外泄 */
    }
}


/**
 * @brief ModbusPackErrorReply
 *          封装应答的错误帧
 * 
 * @param pstMdsSlaveCtrl - Modbus从站控制对象
 */
static void ModbusPackErrorReply(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl)
{
    Uint16 u16CrcIndex = 0;
    Uint16 u16DataLen = 0;

    u16DataLen = pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth;

    // 清空接收Buff，为后续打包数据留出干净的空间
    if (u16DataLen > 0U)
    {
        memset(pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff, 0, u16DataLen);
    }

    /* 规范异常应答：从机地址 | 原功能码|0x80 | 异常码 | CRC低 | CRC高。
     * 参考层原实现把 (eErrorType + 0x80) 当成功能码、把起始地址高字节当成第三
     * 字节，两处都不符合规范（master 会认为收到的是一条别的东西），已按规范改。 */
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff[MDS_RDREPLY_ADDR_INDEX] =
        pstMdsSlaveCtrl->stMdsFrame.bAddr;
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff[MDS_RDREPLY_CMD_INDEX] =
        (Uint16)(pstMdsSlaveCtrl->stMdsFrame.bFuncCmd | MODBUS_ERR_BASE_CODE);
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff[MDS_RDREPLY_BYTE_NUMS_INDEX] =
        ModbusExcCodeOf(pstMdsSlaveCtrl->stMdsFrame.eErrorType);

    // 计算CRC的位置
    u16CrcIndex = MDS_RDREPLY_ADDR_LEN + MDS_RDREPLY_CMD_LEN + MDS_RDREPLY_BYTE_NUMS_LEN;

    // 封装CRC
    PackModbusCrc(pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff, u16CrcIndex);

    // 更新发送数据的长度
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth = u16CrcIndex + MDS_RDREPLY_CRC_LEN;
}



/**
 * @brief ProcMbReadRegMsg
 *          03功能码处理函数
 * 
 * @param pstMdsSlaveCtrl - Modbus从站控制对象
 * @return E_MDS_ERROR_TYPE - 返回错误码
 */
static E_MDS_ERROR_TYPE ProcMbReadRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl)
{
    U_REG_VAL uRegVal;
    ST_REG_SLAVE_ITEM stRegTemp;
    Uint16 i = 0;
    int16 i16Ret = 0;
    Uint16 u16RegNums = 0;
    Uint16 u16CrcIndex = 0;
    Uint16 *pu16Buff = pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff;

    memset(&stRegTemp, 0, sizeof(stRegTemp));
    stRegTemp.puRegVal = (U_REG_VAL *)&uRegVal;

    /* 03 请求固定 8 字节。长度不对就不解析：多出来的字节会让地址/数量字段错位，
     * 解析出来的“地址”和“数量”都是假的。 */
    if (pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth != MODBUS_READ_FRAME_LEN)
    {
        return ERROR_DATA;
    }

    // 记录主机要查询的寄存器的起始地址
    pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr = BUILD_UINT16(pu16Buff[MDS_RD_REG_ST_ADDR_INDEX + 1],
                                                               pu16Buff[MDS_RD_REG_ST_ADDR_INDEX]);

    // 记录主机要查询的寄存器的个数
    pstMdsSlaveCtrl->stMdsFrame.u16RegNums = BUILD_UINT16(pu16Buff[MDS_RD_REGS_NUM_INDEX + 1],
                                                          pu16Buff[MDS_RD_REGS_NUM_INDEX]);

    // 判断访问寄存器个数是否合法（0 个也是非法请求）
    if ((pstMdsSlaveCtrl->stMdsFrame.u16RegNums == 0U) ||
        (pstMdsSlaveCtrl->stMdsFrame.u16RegNums > MAX_REG_NUM))
    {
        pstMdsSlaveCtrl->stMdsFrame.u16RegNums = 0;
        return ERROR_DATA;
    }

    // 先整段确认地址都在表里，再动手组帧：半截回复比不回更糟
    while (i < pstMdsSlaveCtrl->stMdsFrame.u16RegNums)
    {
        i16Ret = ReadModbusRegisterData(pstMdsSlaveCtrl->stSlaveTableArry[pstMdsSlaveCtrl->eCurrentSlaveTableIndex].pstProtocolTable,
                                        (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr + i), &stRegTemp);
        if (i16Ret != 0)
        {
            // 段内任意一个地址不在表里都报非法寄存器地址错误
            return ERROR_ADDRESS;
        }

        i = (Uint16)(i + ModbusRegWidthOf(stRegTemp.bValType));
    }

    // 清空接收Buff，为后续打包数据留出干净的空间
    memset(pu16Buff, 0, pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth * 2);

    // 打包数据
    pu16Buff[MDS_RDREPLY_ADDR_INDEX] = pstMdsSlaveCtrl->stMdsFrame.bAddr;
    pu16Buff[MDS_RDREPLY_CMD_INDEX] = pstMdsSlaveCtrl->stMdsFrame.bFuncCmd;
    pu16Buff[MDS_RDREPLY_BYTE_NUMS_INDEX] = (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegNums * 2U);

    i = 0;
    while (i < pstMdsSlaveCtrl->stMdsFrame.u16RegNums)
    {
        i16Ret = ReadModbusRegisterData(pstMdsSlaveCtrl->stSlaveTableArry[pstMdsSlaveCtrl->eCurrentSlaveTableIndex].pstProtocolTable,
                                        (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr + i), &stRegTemp);
        if (i16Ret != 0)
        {
            return ERROR_ADDRESS;
        }

        // 将寄存器的数据存放进Buff中
        u16RegNums = SetRegValToMdsMsg(pu16Buff, &stRegTemp, i);
        i = (Uint16)(i + u16RegNums);
    }

    // 计算CRC的位置
    u16CrcIndex = (Uint16)(MDS_RDREPLY_ADDR_LEN + MDS_RDREPLY_CMD_LEN + MDS_RDREPLY_BYTE_NUMS_LEN
            + pstMdsSlaveCtrl->stMdsFrame.u16RegNums * 2U);

    // 封装CRC
    PackModbusCrc(pu16Buff, u16CrcIndex);

    // 更新发送数据的长度
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth = (Uint16)(u16CrcIndex + MDS_RDREPLY_CRC_LEN);

    return DATA_OK;
}


/**
 * @brief ProcMbReadRegMsg
 *          06功能码处理函数
 * 
 * @param pstMdsSlaveCtrl - Modbus从站控制对象
 * @return E_MDS_ERROR_TYPE - 返回错误码
 */
static E_MDS_ERROR_TYPE ProcMbWriteSingleRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl)
{
    int16 i16Ret = 0;
    ST_REG_SLAVE_ITEM stSrcRegTemp;
    ST_REG_SLAVE_ITEM stDrcRegTemp;

    U_REG_VAL uSrcRegVal;
    U_REG_VAL uDrcRegVal;
    Uint16 u16RegNum = 0;
    Uint16 *pu16Buff = pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff;

    memset(&stSrcRegTemp, 0, sizeof(stSrcRegTemp));
    stSrcRegTemp.puRegVal = (U_REG_VAL *)&uSrcRegVal;

    memset(&stDrcRegTemp, 0, sizeof(stDrcRegTemp));
    stDrcRegTemp.puRegVal = (U_REG_VAL *)&uDrcRegVal;

    /* 06 请求固定 8 字节。 */
    if (pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth != MODBUS_WRITE_SINGLE_FRAME_LEN)
    {
        return ERROR_DATA;
    }

    // 记录主机要查询的寄存器的起始地址
    pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr = BUILD_UINT16(pu16Buff[MDS_SINGLE_WD_REG_ADDR_INDEX + 1],
                                                               pu16Buff[MDS_SINGLE_WD_REG_ADDR_INDEX]);
    pstMdsSlaveCtrl->stMdsFrame.u16RegNums = 1U;

    // 检测寄存器是否合法，并且取寄存器地址在寄存器配置表中对应的数据
    i16Ret = ReadModbusRegisterData(pstMdsSlaveCtrl->stSlaveTableArry[pstMdsSlaveCtrl->eCurrentSlaveTableIndex].pstProtocolTable,
                                   pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr, &stDrcRegTemp);
    if (i16Ret != 0)
    {
        // -2 超出表范围、-3 表内没这个地址，对主机都是“非法数据地址”
        return ERROR_ADDRESS;
    }

    // 只有可读可写的数据才能修改
    if (stDrcRegTemp.bRWType == READ_ONLY)
    {
        return ERROR_ADDRESS;
    }

    u16RegNum = LoadRegValFromMdsMsg(pu16Buff, &stSrcRegTemp, stDrcRegTemp.bValType,
                                     0, MDS_SINGLE_WD_REG_VAL_INDEX);
    // 只写单个寄存器
    if (u16RegNum != 1)
    {
        // 一个 32 位类型的数据占两个寄存器，不能用单寄存器写
        return ERROR_DATA;
    }

    /* 先验限值，再决定是否置“参数固化”标志：标志必须在真正写得进去时才置。 */
    i16Ret = CheckRegValByType(&stDrcRegTemp, &stSrcRegTemp);
    if (i16Ret != 1)
    {
        return ERROR_DATA;
    }

    // 参数是否保存
    if (stDrcRegTemp.bParamSaveType > 0)
    {
        switch(stDrcRegTemp.bValType)
        {
            case TYPE_UINT8:
            case TYPE_INT8:
            case TYPE_UINT16:
            case TYPE_INT16:
                if (stSrcRegTemp.puRegVal->u16Val != stDrcRegTemp.puRegVal->u16Val)
                {
                    pstMdsSlaveCtrl->stMdsFrame.bParamSaveFlag = 1;
                }
                break;

            case TYPE_UINT32:
            case TYPE_INT32:
            case TYPE_FLOAT:
                if (stSrcRegTemp.puRegVal->u32Val != stDrcRegTemp.puRegVal->u32Val)
                {
                    pstMdsSlaveCtrl->stMdsFrame.bParamSaveFlag = 1;
                }
                break;
        }

    }

    i16Ret = SetRegValByType(&stDrcRegTemp, &stSrcRegTemp);
    if (i16Ret != 1)
    {
        return ERROR_DATA;
    }

    // 组包，由于写单个命令的应答帧和接收帧一致，所以这里不做组包，直接将收到的Buff数据回传即可
    return DATA_OK;
}


/**
 * @brief ProcMbReadRegMsg
 *          16功能码处理函数
 * 
 * @param pstMdsSlaveCtrl - Modbus从站控制对象
 * @return E_MDS_ERROR_TYPE - 返回错误码
 */
static E_MDS_ERROR_TYPE ProcMbWriteMultiRegMsg(ST_MODBUS_SLAVE_TABLE_CRTL *pstMdsSlaveCtrl)
{
    int16 i16Ret = 0;
    ST_REG_SLAVE_ITEM stSrcRegTemp;
    ST_REG_SLAVE_ITEM stDrcRegTemp;

    U_REG_VAL uSrcRegVal;
    U_REG_VAL uDrcRegVal;
    Uint16 u16RegNum = 0, i = 0, u16CrcIndex = 0;
    Uint16 u16ByteNum = 0;
    Uint16 *pu16Buff = pstMdsSlaveCtrl->stMdsFrame.pstMdsData->pu16DataBuff;

    memset(&stSrcRegTemp, 0, sizeof(stSrcRegTemp));
    stSrcRegTemp.puRegVal = (U_REG_VAL *)&uSrcRegVal;

    memset(&stDrcRegTemp, 0, sizeof(stDrcRegTemp));
    stDrcRegTemp.puRegVal = (U_REG_VAL *)&uDrcRegVal;

    // 记录主机要写入的寄存器的起始地址与个数
    pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr = BUILD_UINT16(pu16Buff[MDS_MULTI_WD_REG_ADDR_INDEX + 1],
                                                               pu16Buff[MDS_MULTI_WD_REG_ADDR_INDEX]);
    pstMdsSlaveCtrl->stMdsFrame.u16RegNums = BUILD_UINT16(pu16Buff[MDS_MULTI_WD_REGS_NUM_INDEX + 1],
                                                          pu16Buff[MDS_MULTI_WD_REGS_NUM_INDEX]);

    // 判断访问寄存器个数是否合法
    if ((pstMdsSlaveCtrl->stMdsFrame.u16RegNums == 0U) ||
        (pstMdsSlaveCtrl->stMdsFrame.u16RegNums > MODBUS_MULTI_WD_REG_MAX))
    {
        pstMdsSlaveCtrl->stMdsFrame.u16RegNums = 0;
        return ERROR_DATA;
    }

    /* 先把长度算清楚：字节数必须等于 2*n，帧长必须等于 9+字节数。
     * 少了这两条，CRC 字节会被当成寄存器数据写进去（主机截断一条命令就能改掉
     * 一个无关寄存器），而帧里多余的数据又会被静默忽略。 */
    u16ByteNum = pu16Buff[MODBUS_MULTI_WD_BYTE_NUM_INDEX];
    if ((u16ByteNum != (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegNums * 2U)) ||
        (pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth != (Uint16)(MODBUS_MULTI_WD_FRAME_BASE + u16ByteNum)))
    {
        pstMdsSlaveCtrl->stMdsFrame.u16RegNums = 0;
        return ERROR_DATA;
    }

    /* 第一遍只验不写：整段地址、权限、限值全部验过才允许提交。
     * 半截写入比不写更糟——主机收到成功应答，却只有前半段生效。 */
    i = 0;
    while (i < pstMdsSlaveCtrl->stMdsFrame.u16RegNums)
    {
        i16Ret = ReadModbusRegisterData(pstMdsSlaveCtrl->stSlaveTableArry[pstMdsSlaveCtrl->eCurrentSlaveTableIndex].pstProtocolTable,
                                       (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr + i), &stDrcRegTemp);
        if (i16Ret != 0)
        {
            return ERROR_ADDRESS;
        }

        // 只有可读可写的数据才能修改
        if (stDrcRegTemp.bRWType == READ_ONLY)
        {
            return ERROR_ADDRESS;
        }

        u16RegNum = LoadRegValFromMdsMsg(pu16Buff, &stSrcRegTemp, stDrcRegTemp.bValType,
                                         i, MDS_MULTI_WD_REG_VAL_INDEX);
        if ((u16RegNum != 1U) && (u16RegNum != 2U))
        {
            return ERROR_DATA;
        }
        if ((Uint16)(i + u16RegNum) > pstMdsSlaveCtrl->stMdsFrame.u16RegNums)
        {
            /* 32 位类型放不下（数量字段不够它占两个寄存器） */
            return ERROR_DATA;
        }

        i16Ret = CheckRegValByType(&stDrcRegTemp, &stSrcRegTemp);
        if (i16Ret != 1)
        {
            // 超限（0）与表配置异常（-2/-3）对主机都报“非法数据值”
            return ERROR_DATA;
        }

        i = (Uint16)(i + u16RegNum);
    }

    /* 第二遍：全部验过，逐项提交。 */
    i = 0;
    while (i < pstMdsSlaveCtrl->stMdsFrame.u16RegNums)
    {
        i16Ret = ReadModbusRegisterData(pstMdsSlaveCtrl->stSlaveTableArry[pstMdsSlaveCtrl->eCurrentSlaveTableIndex].pstProtocolTable,
                                       (Uint16)(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr + i), &stDrcRegTemp);
        if (i16Ret != 0)
        {
            return ERROR_ADDRESS;
        }

        u16RegNum = LoadRegValFromMdsMsg(pu16Buff, &stSrcRegTemp, stDrcRegTemp.bValType,
                                         i, MDS_MULTI_WD_REG_VAL_INDEX);

        // 参数是否保存
        if (stDrcRegTemp.bParamSaveType > 0)
        {
            switch(stDrcRegTemp.bValType)
            {
                case TYPE_UINT8:
                case TYPE_INT8:
                case TYPE_UINT16:
                case TYPE_INT16:
                    if (stSrcRegTemp.puRegVal->u16Val != stDrcRegTemp.puRegVal->u16Val)
                    {
                        pstMdsSlaveCtrl->stMdsFrame.bParamSaveFlag = 1;
                    }
                    break;

                case TYPE_UINT32:
                case TYPE_INT32:
                case TYPE_FLOAT:
                    if (stSrcRegTemp.puRegVal->u32Val != stDrcRegTemp.puRegVal->u32Val)
                    {
                        pstMdsSlaveCtrl->stMdsFrame.bParamSaveFlag = 1;
                    }
                    break;
            }

        }

        i16Ret = SetRegValByType(&stDrcRegTemp, &stSrcRegTemp);
        if (i16Ret != 1)
        {
            // 第一遍已经验过，走到这里说明表在两次读之间被改过：报错不提交
            return ERROR_DATA;
        }

        i = (Uint16)(i + u16RegNum);
    }

    // 这里需要组帧
    // 清空接收Buff，为后续打包数据留出干净的空间
    memset(pu16Buff, 0, pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth);

    // 打包数据
    pu16Buff[MDS_RDREPLY_ADDR_INDEX] = pstMdsSlaveCtrl->stMdsFrame.bAddr;
    pu16Buff[MDS_RDREPLY_CMD_INDEX] = pstMdsSlaveCtrl->stMdsFrame.bFuncCmd;
    pu16Buff[MDS_MULTI_WD_REG_ADDR_INDEX] = HI_UINT16(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr);
    pu16Buff[MDS_MULTI_WD_REG_ADDR_INDEX + 1] = LO_UINT16(pstMdsSlaveCtrl->stMdsFrame.u16RegStartAddr);

    pu16Buff[MDS_MULTI_WD_REGS_NUM_INDEX] = HI_UINT16(pstMdsSlaveCtrl->stMdsFrame.u16RegNums);
    pu16Buff[MDS_MULTI_WD_REGS_NUM_INDEX + 1] = LO_UINT16(pstMdsSlaveCtrl->stMdsFrame.u16RegNums);

    // 计算CRC的位置
    u16CrcIndex = MDS_MULTI_WD_ADDR_LEN + MDS_MULTI_WD_CMD_LEN + MDS_MULTI_WD_REG_ADDR_LEN
            + MDS_MULTI_WD_REG_VAL_LEN;

    // 封装CRC
    PackModbusCrc(pu16Buff, u16CrcIndex);

    // 更新发送数据的长度
    pstMdsSlaveCtrl->stMdsFrame.pstMdsData->u16DataLenth = (Uint16)(u16CrcIndex + MDS_RDREPLY_CRC_LEN);
    return DATA_OK;
}



/**
 * @brief ModbusSlaveDataHandle
 *          Modbus从机帧解析函数
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param pstMdsData - Modbus从站原始数据接收的缓存结构体
 * @return ST_MDS_FRAME* 应答帧回复的结构体指针，返回的结构体中的ST_MDS_DATA参数指向的是函数入参pstMdsData，
 *                      回复数据存放在其中；
 *                      如果出现广播帧CRC校验错误、帧长度小于5、地址不匹配、入参异常时，会返回NULL
 */
ST_MDS_FRAME* ModbusSlaveDataHandle(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, ST_MDS_DATA *pstMdsData)
{
    int32 i32Ret = 0;
    Uint16 i = 0;

    if ((pstModbusTableCtrl == NULL) || (pstMdsData == NULL))
    {
        return NULL;
    }

    // 初始化处理帧
    pstModbusTableCtrl->stMdsFrame.eErrorType = DATA_OK;
    pstModbusTableCtrl->stMdsFrame.bFuncCmd = 0;
    pstModbusTableCtrl->stMdsFrame.u16RegNums = 0;
    pstModbusTableCtrl->stMdsFrame.u16RegStartAddr = 0;
    pstModbusTableCtrl->stMdsFrame.bParamSaveFlag = 0;
    pstModbusTableCtrl->stMdsFrame.pstMdsData = pstMdsData;

    // 获取本机地址
    pstModbusTableCtrl->stMdsFrame.bAddr = pstModbusTableCtrl->bModbusSlaveAddr;

    // 获取功能码，用于回复
    pstModbusTableCtrl->stMdsFrame.bFuncCmd = pstMdsData->pu16DataBuff[MDS_RD_CMD_INDEX];


    if (pstMdsData->u16DataLenth <= 5)
    {
        return NULL;
    }

    // 广播命令不回复
    if (pstMdsData->pu16DataBuff[MDS_RD_ADDR_INDEX] == 0)
    {
        // 不回复，这里不能直接return
    }
    else if (pstModbusTableCtrl->stMdsFrame.bAddr != pstMdsData->pu16DataBuff[MDS_RD_ADDR_INDEX])
    {
        // 地址不匹配，不响应
        return NULL;
    }

    // 校验CRC是否正确
    i32Ret = CheckModbusCrc(pstMdsData->pu16DataBuff, pstMdsData->u16DataLenth);
    if (i32Ret != TRUE)
    {
        /* CRC 错的帧一律丢弃、一律不回复：整条请求的内容都不可信，回一条异常帧
         * 等于替主机确认“我收到过这条命令”。参考层原本对非广播帧回 ERROR_CRC
         * 异常帧，已改为静默丢弃。 */
        pstModbusTableCtrl->stMdsFrame.eErrorType = ERROR_CRC;
        return NULL;
    }
    else
    {
        for (i = 0; i < sizeof(g_stMdsCmdTable)/sizeof(g_stMdsCmdTable[0]); i++)
        {
            if (g_stMdsCmdTable[i].u16Cmd == pstModbusTableCtrl->stMdsFrame.bFuncCmd)
            {
                pstModbusTableCtrl->stMdsFrame.eErrorType = g_stMdsCmdTable[i].pFunc(pstModbusTableCtrl);

                // 调用应用层的回调函数
                if (pstModbusTableCtrl->stSlaveTableArry[pstModbusTableCtrl->eCurrentSlaveTableIndex].SlaveModbusCallBack != NULL)
                {
                    pstModbusTableCtrl->stSlaveTableArry[pstModbusTableCtrl->eCurrentSlaveTableIndex].SlaveModbusCallBack();
                }
                break;
            }
        }

        // 判断是否是无效命令
        if (sizeof(g_stMdsCmdTable)/sizeof(g_stMdsCmdTable[0]) == i)
        {
            pstModbusTableCtrl->stMdsFrame.eErrorType = ERROR_FUNC;
        }
    }



    if (pstModbusTableCtrl->stMdsFrame.eErrorType != DATA_OK)
    {
        // 组一个错误帧
        ModbusPackErrorReply(pstModbusTableCtrl);
    }

    // 返回封装好的应当帧
    return &pstModbusTableCtrl->stMdsFrame;
}


/**
 * @brief GetParamSaveFlag
 *          获取参数保存的标志，应用层获取该标志来触发参数保存的相关逻辑
 * 
 * @param pstModbusTableCtrl 
 * @return int16 -  1：需要执行参数保存动作
 *                  0：不需要执行参数保存
 */
int16 GetParamSaveFlag(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    return pstModbusTableCtrl->stMdsFrame.bParamSaveFlag;
}

/**
 * @brief ClearParamSaveFlag
 *          清除参数保存的标志
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 */
void ClearParamSaveFlag(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl)
{
    pstModbusTableCtrl->stMdsFrame.bParamSaveFlag = 0;
}


#ifdef __cplusplus
}
#endif

/**
 * @file Modbus.c
 * @author hanyuhang(008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Modbus.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ModbusSlaveHandle
 *        Modbus从站业务调用函数
 * 
 * @param pstModbusTableCtrl - Modbus从站控制对象
 * @param pstMdsData - Modbus从站原始数据接收的缓存结构体
 * @return ST_MDS_FRAME* 应答帧回复的结构体指针，返回的结构体中的ST_MDS_DATA参数指向的是函数入参pstMdsData，
 *                      回复数据存放在其中；
 *                      如果出现广播帧CRC校验错误、帧长度小于5、地址不匹配、入参异常时，会返回NULL
 *  若在接收到数据后且数据解析前切换了通信点表或者通信地址，则会使用新的通信地址或者通信点表进行解析
    会出现地址不匹配时忽略该Modbus帧，或者使用新的通信点表的寄存器地址数据进行回复
 */
ST_MDS_FRAME* ModbusSlaveHandle(ST_MODBUS_SLAVE_TABLE_CRTL *pstModbusTableCtrl, ST_MDS_DATA *pstMdsData)
{
//    E_MODBUS_SLAVE_TABLE_TYPE eProtocolType;
    ST_MDS_FRAME *pstMdsFrame = NULL;

    // 执行参数更新
    ModbusSlaveParamUpdate(pstModbusTableCtrl);

    // 响应主机Modbus帧
    // 若在接收到数据后且数据解析前切换了通信点表或者通信地址，则会使用新的通信地址或者通信点表进行解析
    // 会出现地址不匹配时忽略该Modbus帧，或者使用新的通信点表的寄存器地址数据进行回复
    pstMdsFrame = ModbusSlaveDataHandle(pstModbusTableCtrl, pstMdsData);



    return pstMdsFrame;

}



#ifdef __cplusplus
}
#endif

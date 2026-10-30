/**
 * @file InternalCanCfg.c
 * @author hanyuhang  (008467)
 * @brief 
 * @version 0.1
 * @date 2025-07-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "InternalCanCfg.h"
#include "InternalCan.h"

#ifdef __cplusplus
    extern "C" {
#endif

Uint32 Value = 0x11223344;
Uint16 u16CanDataTemp[22] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22};

// DSP在处理32位数时，如果不是4字节对齐，地址则会向下进行4字节对齐后再取地址运算
ST_OBJ_DICTIONARY stObjDictionary[] =
{
    // 对象索引必须为连续且递增，从1开始!!!
    // 对象索引    数据类型    变量地址
    {1, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[0]},
    {2, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[1]},
    {3, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[2]},
    {4, CAN_TYPE_UINT32,    (U_CAN_VAL *)&Value},
    {5, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[4]},
    {6, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[5]},
    {7, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[6]},
    {8, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[7]},
    {9, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[8]},
    {10, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[9]},
    {11, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[10]},
    {12, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[11]},
    {13, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[12]},
    {14, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[13]},
    {15, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[14]},
    {16, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[15]},
    {17, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[16]},
    {18, CAN_TYPE_UINT16,    (U_CAN_VAL *)&u16CanDataTemp[17]},
};

ST_PERIOD_TABLE stPeriodTable[] =
{
    //  帧ID    发送字节数 对象索引个数    对象索引1   对象索引2   对象索引3   对象索引4
    {1,     6,      2,              4,          6},
    {2,     6,      3,              1,          2,          3},
    {3,     8,      4,              10,         13,         16,         18},
    {4,     4,      2,              5,          7},
    {5,     6,      2,              4,          5},
};

ST_TRIG_TABLE stTrigTable[] =
{
    //  帧ID    发送字节数 对象索引个数    对象索引1   对象索引2   对象索引3   对象索引4
    { 6,     4,      2,              17,          18},
    { 7,     6,      3,              16,          13,          12},
    { 8,     8,      4,              11,         10,         9,         8},
    { 9,     4,      2,              6,          2},
    {10,     4,      2,              1,          3},
};

// 内部CAN协议配置表对象
ST_INTERNAL_CAN_TABLE g_stInernalCanTable =
{
    .pstObjDictionary = stObjDictionary,
    .pstPeriodTable = stPeriodTable,
    .pstTrigTable = stTrigTable,
    .u16ObjDictionaryItemNum = sizeof(stObjDictionary)/sizeof(stObjDictionary[0]),
    .u16PeriodTableItemNum = sizeof(stPeriodTable)/sizeof(stPeriodTable[0]),
    .u16TrigTableItemNum = sizeof(stTrigTable)/sizeof(stTrigTable[0]),
};

#ifdef __cplusplus
    }
#endif

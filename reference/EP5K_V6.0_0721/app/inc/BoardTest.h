/*************************************************
Copyright (C), PowerOak Tech. Co., Ltd.
File name: BoardTest.h
Author: tzz
Version: 0.0.2
Date: 2022年2月15日
Description: 申明函数
*************************************************/
#ifndef APP_INC_BOARDTEST_H_
#define APP_INC_BOARDTEST_H_

typedef struct
{
    uint16_t  UintData1;
    uint16_t  UintData2;
    uint16_t  UintData3;
    uint16_t  UintData4;
    float  F32Data1;
    float  F32Data2;
}TEMP_DEBUG_DATA;
extern TEMP_DEBUG_DATA DebugData;

void BoardTest(void);
void ParseCanBusData(uint32_t id, float data);

#endif /* APP_INC_BOARDTEST_H_ */

/*
 * MicroInvmode.h
 *
 *  Created on: 2023年12月8日
 *      Author: tian
 */

#ifndef APP_INC_MICROINVMODE_H_
#define APP_INC_MICROINVMODE_H_

#include "nPnZ_Cntl.h"

typedef struct
{
    float OFDeadband;   // 死区，有功功率不响应频率变化，0.036Hz，可设0.017~1
    float kOF_F;        // 斜率，表示单位频率引起的单位有功功率，0.05,可设0.02~0.05
    float kOF_P;        // 斜率，表示单位功率引起的单位频率，计算采用0.05的倒数
    float Pmax;         // 功率输出值
    float PcLimit;      // 充电限功率
    float FH;           // 最大输入频率
    float FN;           // 额定频率
    float PloadAve;     // 负载
    PIDCntl_F FreqCntl;
}MicroInvmode_STRUCT;
extern MicroInvmode_STRUCT MicroInvmode;

void InitMicroInverterPara(void);
void MicroInvterModeFreqChange(void);

#endif /* APP_INC_MICROINVMODE_H_ */

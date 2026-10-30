/**
 * @file Hw_WatchDog.c
 * @author hanyuhang (008467)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "Hw_WatchDog.h"
#include "sysctl.h"


#ifdef __cplusplus
extern "C" {
#endif

#define CLOCK_FREQ_HZ   10000000    // 看门狗的时钟是10Mhz

// 分频器可选值
const Uint16 Divider1[] = {1, 1, 2, 4, 8, 16, 32, 64};
const Uint16 Divider2[] = {512, 1024, 2048, 4096, 4096, 4096, 4096, 4096, 2, 4, 8, 16, 32, 64, 128, 256};



/**
 * @brief 初始化看门狗，配置超时复位时间，
 *          初始化之后需要调用Hw_EnableWatchDog启动看门狗
 * 
 * @param TimeOut :超时时间，单位ms
 */
void Hw_WatchDogInit(Uint16 TimeOut)
{

    Uint16 Div1 = 64;
    Uint16 Div2 = 512;
    Uint16 i = 0, j = 0;
    Uint16 temp = 0;
    float CNT_MAX = 255.0;

    float TimeOutMs = 0.0;

    float Error = 0.0;
    float MinError = 65535.0;

    Uint16 Div1Count = sizeof(Divider1)/sizeof(Divider1[0]);
    Uint16 Div2Count = sizeof(Divider2)/sizeof(Divider2[0]);

    // 计算最接近目标值的分频系数
    for (i = 0; i < Div1Count; i++)
    {
        for (j = 0; j < Div2Count; j++)
        {

            // 计算超时时间
            TimeOutMs = (CNT_MAX * Divider1[i] * Divider2[j] * 1000) / CLOCK_FREQ_HZ;

            // 计算误差
            Error = fabsf(TimeOut - TimeOutMs);

            if (Error < MinError)
            {
                MinError = Error;
                Div1 = i;

                // 这个地方的特殊处理和DSP芯片的分频系数寄存器有关系
                if (Divider2[j] == 4096)
                {
                    Div2 = 3;
                }
                else
                {
                    Div2 = j;
                }

            }
        }
    }

    temp = ((Div2 << 8) | Div1) | 0x68;
    // 设置分频系数,禁用看门狗
    EALLOW;
    WdRegs.WDCR.all = temp;
    EDIS;

}



/**
 * @brief 启动看门狗
 * 
 */
void Hw_EnableWatchDog(void)
{
    Uint16 temp = 0;

    EALLOW;
    temp = WdRegs.WDCR.all;
    WdRegs.WDCR.all = (~(1<<6)) & temp | 0x28;
    EDIS;
}



/**
 * @brief 禁用看门狗
 * 
 */
void Hw_DisableWatchDog(void)
{
    Uint16 temp = 0;

    EALLOW;
    temp = WdRegs.WDCR.all;
    WdRegs.WDCR.all = (1<<6) | temp;
    EDIS;
}



/**
 * @brief 喂狗操作
 * 
 */
void Hw_FeedWatchDog(void)
{
    EALLOW;
    WdRegs.WDKEY.bit.WDKEY = 0x0055;
    WdRegs.WDKEY.bit.WDKEY = 0x00AA;
    EDIS;
}


/**
 * @brief 看门狗复位芯片
 *
 */
void Hw_SysReset(void)
{
    EALLOW;
    Uint16 temp = 0;
    temp = WdRegs.WDCR.all;
    WdRegs.WDCR.all = (0x38 | temp);
    EDIS;
}

#ifdef __cplusplus
}
#endif

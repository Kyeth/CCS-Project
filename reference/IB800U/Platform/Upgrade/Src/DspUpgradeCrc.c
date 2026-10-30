#include "DspUpgradeCrc.h"

Uint32 UpgradeCrc32(Uint16 *pu16Data, Uint32 u32Length, Uint32 u32LastCrc)
{
    Uint32 i = 0;
    Uint16 j = 0;
    Uint32 DataLow = 0;
    Uint32 DataHigh = 0;

    for (i = 0; i < u32Length; i++)
    {
        DataLow = (Uint32)(pu16Data[i] & 0xFF);
        DataHigh = (Uint32)((pu16Data[i] & 0xFF00) >> 8);

        u32LastCrc ^= DataLow << 24;

        for (j = 0; j < 8; j++)
        {
            if (u32LastCrc & 0x80000000)
            {
                u32LastCrc = (u32LastCrc << 1) ^ 0x04C11DB7;  // 多项式异或
            }
            else
            {
                u32LastCrc <<= 1;  // 普通移位
            }
        }


        u32LastCrc ^= DataHigh << 24;

        for (j = 0; j < 8; j++)
        {
            if (u32LastCrc & 0x80000000)
            {
                u32LastCrc = (u32LastCrc << 1) ^ 0x04C11DB7;  // 多项式异或
            }
            else
            {
                u32LastCrc <<= 1;  // 普通移位
            }
        }
    }

    return u32LastCrc;
}

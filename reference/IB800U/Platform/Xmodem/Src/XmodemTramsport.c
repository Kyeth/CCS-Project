#include "XmodemTramsports.h"


// 这里需要引入硬件外设头文件
#include "Hw_SciDriver.h"


#ifdef __cplusplus
extern "C" {
#endif



int XmodemWrite(const Uint16 *u16Buff, Uint16 u16Length)
{
    return Hw_SciWrite(SCI_ID0, (Uint16 *)u16Buff, u16Length);
}


int XmodemRead(Uint16 *u16Buff, Uint16 u16MaxLength)
{
    return Hw_SciRead(SCI_ID0, u16Buff, u16MaxLength);
}






#ifdef __cplusplus
}
#endif

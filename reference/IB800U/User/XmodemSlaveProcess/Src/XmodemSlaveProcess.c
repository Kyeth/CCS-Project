#include "XmodemSlaveProcess.h"
#include "XmodemTimeOut.h"
#include "XmodemTramsports.h"
#include "PlatformManager.h"
#include "XmodemDataHandle.h"
#include "Hw_GpioDriver.h"

#ifdef __cplusplus
    extern "C"{
#endif

ST_XMODEM_PROTOCOL ObjXmodemCtrl;

void XmodemProcessInit(void)
{
    ST_XMODEM_CONFIG stXmodemConfig;
    ST_TRANSPORT_INTF stTransport;
    ST_DATA_HANDLER_INTF stDataHandler;
    ST_TIMEOUT_MANAGER stTimeOutManager;

    stXmodemConfig.u16IsUseCrc = 1;
    stXmodemConfig.u16MaxRetries = 10;
    stXmodemConfig.u16TimeOut = 500;

    stTransport.Read = XmodemRead;
    stTransport.Write = XmodemWrite;

    stTimeOutManager.stTimeDriver.TimeInit = TimeInit;
    stTimeOutManager.stTimeDriver.TimeStart = TimeStart;
    stTimeOutManager.stTimeDriver.IsTimeOut = IsTimeOut;
    stTimeOutManager.stTimeDriver.TimeStop = TimeStop;

    XmodemInit(&ObjXmodemCtrl, &stXmodemConfig, &stTransport, &stDataHandler, &stTimeOutManager);
}

#ifdef __cplusplus
    }
#endif

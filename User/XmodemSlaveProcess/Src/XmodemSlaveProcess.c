#include "XmodemSlaveProcess.h"
#include "XmodemTimeOut.h"
#include "XmodemTramsports.h"
#include "XmodemDataHandle.h"

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
        stXmodemConfig.u16MaxRetries = 8;
        stXmodemConfig.u16TimeOut = 1200;

        stTransport.Read = XmodemRead;
        stTransport.Write = XmodemWrite;

        stDataHandler.ReadBlock = ReadBlock;
        stDataHandler.WriteBlock = WriteBlock;

        stTimeOutManager.stTimeDriver.TimeInit = TimeInit;
        stTimeOutManager.stTimeDriver.TimeStart = TimeStart;
        stTimeOutManager.stTimeDriver.IsTimeOut = IsTimeOut;
        stTimeOutManager.stTimeDriver.TimeStop = TimeStop;

        XmodemInit(&ObjXmodemCtrl, &stXmodemConfig, &stTransport, &stDataHandler, &stTimeOutManager);
    }


#ifdef __cplusplus
    }
#endif

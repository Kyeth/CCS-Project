#ifndef __XMODEMCORE_H
#define __XMODEMCORE_H


#include "F28003x_device.h"
#include "XmodemTimeOut.h"

#ifdef __cplusplus
extern "C" {
#endif

// 控制字符定义
#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A


#define XMODEM_BUFF_SIZE    1030    // 1030 = XModem 1k + 3 head chars + 2 CRC + NULL
#define XMODEM_TIME_OUT     2000     // 2000ms


typedef enum
{
    XMEDEM_PARAM_ERROR,
    XMODEM_CRC_ERROR,
    XMODEM_SEQUENCE_ERROR,
    XMODEM_PACKET_ERROR,
    XMODEM_IO_ERROR,
    XMODEM_EOT_ERROR,
    XMODEM_CAN_ERROR,
    XMODEM_RETRY_ERROR,
    XMODEM_TIMEOUT,
    XMODEM_OK,
    XMODEM_CANCEL,
    XMODEM_END,
}E_XMODEM_ERROR;

typedef enum
{
    STATE_IDLE,
    STATE_WAIT_FOR_START,
    STATE_RECEIVE_DATA,
    STATE_PROCESS_PACKET,
    STATE_COMPLETE,
    STATE_ERROR,
}ST_XMODEM_STATE_MACHINE;






// 协议配置结构体
typedef struct
{
    Uint16 u16MaxRetries;   // 最大重试次数
    Uint16 u16TimeOut;      // 超时判定时间
    Uint16 u16IsUseCrc;     // 是否使用CRC校验，1：使用CRC校验   0：使用累加和校验
}ST_XMODEM_CONFIG;



// 传输层接口
typedef struct
{
    int (*Write)(const Uint16 *u16Buff, Uint16 u16Length);
    int (*Read)( Uint16 *u16Buff, Uint16 u16MaxLength);
}ST_TRANSPORT_INTF;


// 整个文件头占据32个Bytes
typedef struct
{
    Uint16 bChipType:8;                 // 芯片类型
    Uint16 bFirmwareType:8;             // 固件类型
    Uint32 u32FileSize;                 // 文件大小
    Uint32 u32FileCrc;                  // 32位CRC，不包括文件头
    Uint16 u16Rsv[11];                  // 保留位
}ST_UPGRADE_FILE_HEADER;


// 数据处理接口
typedef struct
{
    int (*ReadBlock)(Uint16 u16BlockNum, Uint16 *u16Buffer);
    int (*WriteBlock)(Uint16 u16BlockNum, Uint16 *u16Buffer, Uint16 u16Length);
}ST_DATA_HANDLER_INTF;




// 协议控制结构体
typedef struct
{
    ST_XMODEM_CONFIG stXmodemConfig;
    ST_TRANSPORT_INTF stTransport;
    ST_DATA_HANDLER_INTF stDataHandler;
    ST_TIMEOUT_MANAGER stTimeOutManager;

    ST_XMODEM_STATE_MACHINE stXmodemStateMachine;
    Uint16 u16xBuff[XMODEM_BUFF_SIZE];
    Uint16 u16CurrentBlock;
    Uint16 bRetryCount          :8;
    Uint16 bRsv1                :8;
    Uint16 bCurrentPackNo       :8;
    Uint16 bLastPackNo          :8;
    Uint16 bError               :8;     // 错误码
    Uint16 bIsFirstPack         :1;     // 是否接收到第一包
    Uint16 bIsCrc               :1;     // 1：启用CRC 0：启用累加和
    Uint16 bEOTCount            :2;     // 标记是否已经接收到一次结束命令
    Uint16 bCANCount            :2;     // 标记是否已经接收到一次中止命令
    Uint16 bRecvOver            :1;
    Uint16 bRsv2                :2;
    Uint16 u16RecvLength;               // 本次接收长度累计和
    ST_UPGRADE_FILE_HEADER stUpgradeFileHeader; // 升级文件头
}ST_XMODEM_PROTOCOL;


// 公共函数接口
extern E_XMODEM_ERROR XmodemInit(ST_XMODEM_PROTOCOL *pstProtocol, ST_XMODEM_CONFIG *pstXmodemConfig,
                          ST_TRANSPORT_INTF *pstTransport, ST_DATA_HANDLER_INTF *pstDataHandler,
                          ST_TIMEOUT_MANAGER *pstTimeOutManager);


extern E_XMODEM_ERROR XmodemReceive(ST_XMODEM_PROTOCOL *pstProtocol);


extern E_XMODEM_ERROR XmodemTransmit(ST_XMODEM_PROTOCOL *pstProtocol);

#ifdef __cplusplus
}
#endif


#endif

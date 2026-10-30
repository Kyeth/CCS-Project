#ifndef __EQUIPMENTPARALLELCORE_H
#define __EQUIPMENTPARALLELCORE_H

#include "F28003x_device.h"
#include "RingBuff.h"

#ifdef __cplusplus
    extern "C" {
#endif


#define MAX_PACK_NUM    5
#define PARALLEL_FRAME_MAX  20
#define SINGLE_PROCESS_MAX_NUM 5           // 单次处理CAN帧的最大数

#define HEARTBEAT_HEAD  0xAE
#define DATA_HEAD       0xBF

#define VAILD   1
#define INVAILD 0

// 寄存器对应的值的类型
typedef enum
{
    PARALLEL_TYPE_INT8,
    PARALLEL_TYPE_UINT8,
    PARALLEL_TYPE_INT16,
    PARALLEL_TYPE_UINT16,
    PARALLEL_TYPE_INT32,
    PARALLEL_TYPE_UINT32,
    PARALLEL_TYPE_FLOAT
}E_PARALLEL_VAL_TYPE;

typedef struct
{
    Uint16 u16ObjOffset;
    E_PARALLEL_VAL_TYPE eValType;
}ST_OBJ_ATTRIBUTE;


typedef struct
{
    Uint16 u16FrameType;                        // 帧类型
    Uint16 u16FrameNo;                          // 帧序号
    Uint16 u16Vaild;                            // 发送帧有效
    Uint16 u16DataDlc;                          // 待发送数据长度，单位为字节
    Uint16 u16ObjNum;                           // 待发送对象索引个数
    ST_OBJ_ATTRIBUTE u16ObjAttribute_1;         // 结构体对象属性
    ST_OBJ_ATTRIBUTE u16ObjAttribute_2;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_3;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_4;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_5;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_6;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_7;
    ST_OBJ_ATTRIBUTE u16ObjAttribute_8;
}ST_BATTERY_ITEM;


typedef struct
{
    ST_BATTERY_ITEM *pstBatteryItem;
    Uint16 u16ItemNum;
}ST_BATTERY_CFG;



typedef union
{
    Uint32 u32All;

    struct
    {
        Uint16 LowWord;
        Uint16 HighWord;
    }Words;

    struct
    {
        Uint16 u8Byte0  :8;
        Uint16 u8Byte1  :8;
        Uint16 u8Byte2  :8;
        Uint16 u8Byte3  :8;
    }Bytes;
}U_PARALLEL_DATA;


// 应用层CAN数据帧
typedef struct
{
    Uint16 bDLC     :4;         // 数据长度
    Uint16 bIDE     :1;         // 0：标准帧    1：拓展帧
    Uint16 bRTR     :1;         // 0：数据帧    1：远程帧
    Uint16 bRsv1    :10;
    Uint32 bID      :29;        // 帧ID
    Uint32 bRsv2    :3;
    U_PARALLEL_DATA unDataA;
    U_PARALLEL_DATA unDataB;
}ST_PARALLEL_CAN_FRAME;




typedef struct
{
    Uint32 bSoc                 :7;     // 0-100
    Uint32 bIsEquipmentNormal   :1;     // 设备正常标识
    Uint32 bIsSoftFinsh         :1;     // 软启结束可进行充放电标志（可控工作）
    Uint32 bDOrCState           :3;     // 设备充放电状态
    Uint32 bSelfBus             :10;    // 自身的电压
    Uint32 bGetDeviceMode       :2;     // 设备工作模式
    Uint32 u16BuckFlag          :1;     // 降压标志
    Uint32 bIsStableKey         :1;     // 可稳压标志
    Uint32 bRsv                 :6;     // 保留

}ST_BROADCAST_DATA_1;

typedef struct
{
    int32 bPower                 :16;     //
    int32 bRsv                   :16;     // 保留

}ST_BROADCAST_DATA_2;


typedef struct
{
    Uint16 bVref;               // 广播的参考电压
    int16 bTotalPower;          // 广播的汇总功率
}ST_BROADCAST_CTRL_1;


typedef struct
{
    Uint32 bSetDeviceMode1   :2;
    Uint32 bSetDeviceMode2   :2;
    Uint32 bSetDeviceMode3   :2;
    Uint32 bSetDeviceMode4   :2;
    Uint32 bSetDeviceMode5   :2;
    Uint32 bSetDeviceMode6   :2;
    Uint32 bSetDeviceMode7   :2;
    Uint32 bSetDeviceMode8   :2;
    Uint32 bSetDOrCState1   :2;     // 1:充 2:放
    Uint32 bSetDOrCState2   :2;
    Uint32 bSetDOrCState3   :2;
    Uint32 bSetDOrCState4   :2;
    Uint32 bSetDOrCState5   :2;
    Uint32 bSetDOrCState6   :2;
    Uint32 bSetDOrCState7   :2;
    Uint32 bSetDOrCState8   :2;
}ST_SPECIFIED_CTRL_3;

// CAN通信接口回调函数
typedef struct
{
    int16 (*CanRead)(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16ReadSize);
    int16 (*CanWrite)(ST_PARALLEL_CAN_FRAME *pstCanFrame, Uint16 u16WriteSize);
    int16 (*ClearBusError)(void);
}ST_PARALLEL_TRANSPORT_INTF;


typedef struct
{
    Uint32 u32Version;              // 软件版本，固定在数据第一位作为心跳包传输，若版本不一致，禁止并机
    // 这里需要可以定义一些控制命令和状态命令，从机可以找到主机并使用主机的数据用来做控制
//    float fCurr;                    // 母线输出电流
//    float fSelfVoltage;             // 自身母线电压
//    float fVoltage;                 // 需要广播的母线电压
//    float fEqualCurr;               // 需要广播的均分电流
//    Uint16 u16Soc;                  // 电池容量
//    Uint16 u16StateCtrl;            // 主机广播从机的进入的状态，3充，4放，5待机
//    Uint16 u16DeviceState;          // 设备充放电状态
//    Uint16 u16IsMaster;             // 主机标识，1:主机，0:从机
//    Uint16 u16IsEquipmentNormal;    // 设备正常标识
//    Uint16 u16IsSoftFinsh;          // 软启结束可进行充放电标志（可控工作）
//    Uint16 u16BuckFlag;             // 降压标志


    ST_BROADCAST_DATA_1 stBroadcastData_1;  // 广播数据帧
    ST_BROADCAST_DATA_2 stBroadcastData_2;
    ST_BROADCAST_CTRL_1 stBroadcastCtrl_1;  // 主机广播控制帧
    ST_SPECIFIED_CTRL_3 stSpecifiedCtrl_3;  // 主机定点下发工作模式
}ST_BATTERY_DATA;

typedef struct
{
    ST_BATTERY_DATA stBatteryData;  // 通信数据
    Uint16 u16PackAddr;             // 电池包地址，默认为0，开机前需要将该地址配置，否则不允许开机
    Uint16 u16IsOnline;             // 在线标识，1:在线，0:离线
    Uint16 u16HeartCountBak;        // 心跳计数备份
    Uint16 u16HeartCount;           // 心跳计数
    Uint16 u16WirebreakTimeOutSet;  // 断线检测超时时间设置
    Uint16 u16WirebreakTimeOutCnt;  // 断线检测计数
    Uint16 u16IsInit;               // 是否被使用
}ST_BATTERY_PACK;


typedef struct
{
    ST_BATTERY_PACK stBatteryPack[MAX_PACK_NUM];
    Uint16 u16LocalAddr;                // 自身地址，默认为0，需要BMS设置

    ST_PARALLEL_TRANSPORT_INTF stParallelTranIntf;        // 通信接口回调函数
    ST_BATTERY_CFG *pstBatteryCfg;      // 数据解析配置

    ST_PARALLEL_CAN_FRAME stParallelTxBuff[PARALLEL_FRAME_MAX];     // TxFrameFifo的空间
    ST_PARALLEL_CAN_FRAME stParallelRxBuff[PARALLEL_FRAME_MAX];     // RxFrameFifo的空间

    ST_RING_BUFF stTxFrameFifo;
    ST_RING_BUFF stRxFrameFifo;

    Uint16 u16PeriodSendIndex;
    Uint16 u16OnlineNum;                    // 通信在线数
    Uint16 u16IsEquipmentNormalNum;         // 通信并机数，可参与控制并机的设备数
    Uint16 u16IsCtrlWorkNum;                // 实际可控制工作并机数
    Uint16 u16IsParallel;                // 通信并机标志
    Uint16 u16MasterDeviceIndex;              // 当前控制主机索引
    Uint16 u16SocMaxDeviceIndex;              // 最高SOC设备索引
    Uint16 u16SocMaxDeviceAddr;               // 最高SOC设备地址
    Uint16 u16SocMinDeviceIndex;              // 最低SOC设备索引
    Uint16 u16SocMinDeviceAddr;              // 最低SOC设备地址
    Uint16 u16MasterDeviceAddr;              // 当前控制主机地址

    Uint16 u16IsMasterSlave;                  // 主从状态
    Uint16 u16DeviceMode;                    // 设备工作模式
    Uint16 u16CtrlState;                      // 控制状态
    Uint16 u16VersionCheckFlag;               // 版本校验通过标识

    float fCtrlVoltage;                      // 被控制环路使用的参数
    Uint16 u16Soc;
    Uint16 u16Soh;
    float fTagetCurrent;                     // 计算后的目标电流
    float fTotalChargeCurr;                  // 充电状态下的总电流
    float fTotalDischargeCurr;               // 放电状态下的总电流
    float fDifferenceCurr;                   // 充放电状态下总电流差值
    float fCurrTotal;               // 汇总总电流

    int16 i16CtrlPackPower;         // 当前需要输出或者输入的功率，正为输出，负为输入
    Uint16 u16CtrlDorCState;            // 当前包充放电状态

    Uint16 u16PackPower;                        // 包的功率
    Uint16 u16DischargeCurrentAllowMax;         // 电池侧最大放电电流，已放大10倍，即100 == 10A电流
    Uint16 u16ChargeCurrentAllowMax;            // 电池侧最大充电电流，已放大10倍
    Uint16 u16DischargeCoefficient;             // BMS设定的放电系数，已放大10倍
    Uint16 u16ChargeCoefficient;               // BMS设定的充电系数，已放大10倍
    Uint16 u16SetPackMaxNum;                    // BMS设置电池包总数

}ST_BATTERY_HANDLE;


extern void EquipmentParallelCoreInit(ST_BATTERY_HANDLE *pstBatteryHandle, ST_BATTERY_CFG *pstInterCanTable, ST_PARALLEL_TRANSPORT_INTF *pstTranSportIntf);

extern int16 ParallelReceive(ST_BATTERY_HANDLE *pstBatteryHandle);

extern int16 ParallelTransmit(ST_BATTERY_HANDLE *pstBatteryHandle);

extern int16 ParallelReceiveFrameProcess(ST_BATTERY_HANDLE *pstBatteryHandle);

extern int ParallelTransmitFrameProcess(ST_BATTERY_HANDLE *pstBatteryHandle);

#ifdef __cplusplus
    }
#endif

#endif

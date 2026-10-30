#ifndef __DEVICE_ABSTRACTION_H
#define __DEVICE_ABSTRACTION_H

#include "F28003x_device.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct Device ST_DEVICE;

typedef struct
{
    void (*Init)(ST_DEVICE *pstSelf);
    void (*Open)(ST_DEVICE *pstSelf);
    void (*Close)(ST_DEVICE *pstSelf);
    void (*Toggle)(ST_DEVICE *pstSelf);
    void (*Ctrl)(ST_DEVICE *pstSelf, Uint16 u16Cmd, void *pParam);
}ST_DEVIVE_OPERATIONS;

struct Device
{
    void *pvDeviceData;         // 具体的设备数据
    ST_DEVIVE_OPERATIONS *pstOps;   // 设备操作接口
    void *pvDriver;                 // 桥接的驱动接口
    Uint16 u16DeviceId;             // 设备ID
};

#ifdef __cplusplus
    }
#endif

#endif

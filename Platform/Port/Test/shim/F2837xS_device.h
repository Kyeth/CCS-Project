/**
 * @file F2837xS_device.h
 * @brief 主机桩测试用的类型垫片
 *
 * 仅供 Platform/Port/Test/port_stub_test.c 在主机上编译端口层源码使用，
 * 不进目标构建、也不参与目标链接。目标构建仍使用 device/headers 里的
 * 真实 F2837xS_device.h。
 */

#ifndef F2837XS_DEVICE_SHIM_H
#define F2837XS_DEVICE_SHIM_H

#include <stdint.h>

typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int16_t int16;
typedef int32_t int32;

#endif /* F2837XS_DEVICE_SHIM_H */

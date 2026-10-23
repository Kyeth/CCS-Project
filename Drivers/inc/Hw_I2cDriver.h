/**
 * @file Hw_I2cDriver.h
 * @author yetingheng (004182)
 * @brief 软件模拟 I2C 主机驱动（GPIO 位翻转，本板用于 FM24C128D）
 * @version 0.1
 * @date 2026-11-02
 * @copyright Copyright (c) 2026
 *
 * @note 本板没有把 F2837xS 的 I2C 外设引脚引出来，FM24C128D 挂在
 *       GPIO91(SDA) / GPIO92(SCL) 上，所以用 GPIO 位翻转实现主机；
 *       写保护脚 WP 走 GPIO99，由 Fm24c128d 驱动控制。
 *
 * @note 引脚本身必须先在 GPIO 配置表里配好（开漏输出 + 默认高电平），
 *       本驱动只做电平翻转，不重复配置 GPIO 复用/方向。
 *
 * @note 只实现"主机"时序，不处理从机时钟拉伸：FM24C128D 是 FRAM，
 *       不会拉低 SCL，所以这里不做 SCL 回读。
 *
 * @note 延时用空循环实现（本工程没有 SysCtl_delay / DELAY_US 实现），
 *       1us 对应的循环次数由配置项 u16LoopPerUs 给出，换主频或换优化
 *       等级要重新标定；标定方法见 DrvCfg_EP15K.c 里 stI2cCfg 的注释。
 */
#ifndef HW_I2C_DRIVER_H
#define HW_I2C_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * 返回值
 ******************************************************************************/
#define HW_I2C_OK                   (0U)    /* 成功（含收到 ACK） */
#define HW_I2C_ERR_NACK             (1U)    /* 从机应答位为 NACK */
#define HW_I2C_ERR_PARAM            (2U)    /* 入参非法或驱动未初始化 */
#define HW_I2C_ERR_TIMEOUT          (3U)    /* 总线释放超时（SDA 被拉死） */

/******************************************************************************
 * 速率与延时缺省值
 ******************************************************************************/
#define HW_I2C_SPEED_100K           (100000UL)  /* 标准模式 */
#define HW_I2C_SPEED_400K           (400000UL)  /* 快速模式 */

/* 200MHz SYSCLK、优化等级 -O2 下，一个 while 循环约 6~7 个周期，
 * 取 30 次循环 ≈ 1us，实现里再留 1us 下限，宁慢不错。 */
#define HW_I2C_LOOP_PER_US_DEFAULT  (30U)

/* 总线恢复时最多打多少个 SCL 脉冲（9 个足够把从机移位寄存器清空） */
#define HW_I2C_RECOVER_PULSE_MAX    (9U)

/******************************************************************************
 * I2C 主机配置
 *
 * 本板只有一条软件 I2C，所以配置直接是一个结构体，不再套一层表；
 * 以后要加第二条总线，按 stI2cCfg 的形式再加一个实例即可。
 ******************************************************************************/
typedef struct
{
    Uint16 u16SdaGpio;          /* SDA 引脚号，本板 GPIO91 */
    Uint16 u16SclGpio;          /* SCL 引脚号，本板 GPIO92 */
    Uint32 u32SpeedHz;          /* 目标速率，HW_I2C_SPEED_100K / 400K */
    Uint16 u16LoopPerUs;        /* 1us 对应的空循环次数，0 表示用缺省值 */
} ST_I2C_CFG;

/**
 * @brief 初始化软件 I2C 主机
 *
 * @param pstI2cCfg 配置，内部只保存指针指向的内容，不做拷贝
 * @note 初始化时会把 SDA/SCL 都拉到高电平（总线空闲态），
 *       并检查 SDA 是否能回到高电平，不能则打 9 个脉冲做总线恢复。
 */
extern void Hw_I2cInit(const ST_I2C_CFG *pstI2cCfg);

/**
 * @brief 发送 START 条件（SDA 下降沿，SCL 为高）
 */
extern void Hw_I2cStart(void);

/**
 * @brief 发送 STOP 条件（SDA 上升沿，SCL 为高）
 */
extern void Hw_I2cStop(void);

/**
 * @brief 发送一个字节并读应答位
 *
 * @param u16Byte 待发送字节（只取低 8 位）
 * @return HW_I2C_OK 收到 ACK，HW_I2C_ERR_NACK 收到 NACK
 */
extern Uint16 Hw_I2cWriteByte(Uint16 u16Byte);

/**
 * @brief 读一个字节并回应答位
 *
 * @param bAck 1: 读完后回 ACK（还要继续读），0: 回 NACK（最后一个字节）
 * @return 读到的字节（低 8 位有效）
 */
extern Uint16 Hw_I2cReadByte(Uint16 bAck);

/**
 * @brief 完整写事务：START + 设备地址(写) + 数据 + STOP
 *
 * @param u16DevAddr 7 位设备地址（不含读写位），内部左移一位并置 0
 * @param pu16Buf    数据，一个 Uint16 存一个字节（低 8 位有效）
 * @param u16Len     字节数
 * @return HW_I2C_OK / HW_I2C_ERR_NACK / HW_I2C_ERR_PARAM
 */
extern Uint16 Hw_I2cWriteTo(Uint16 u16DevAddr, const Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 完整读事务：START + 设备地址(读) + 数据 + STOP
 *
 * @param u16DevAddr 7 位设备地址（不含读写位），内部左移一位并置 1
 * @param pu16Buf    接收缓冲，一个 Uint16 存一个字节（低 8 位有效）
 * @param u16Len     字节数
 * @return HW_I2C_OK / HW_I2C_ERR_NACK / HW_I2C_ERR_PARAM
 */
extern Uint16 Hw_I2cReadFrom(Uint16 u16DevAddr, Uint16 *pu16Buf, Uint16 u16Len);

/**
 * @brief 总线恢复：SDA 被从机拉死时打 9 个 SCL 脉冲再补一个 STOP
 *
 * @return 1 表示恢复后 SDA 回到高电平
 */
extern Uint16 Hw_I2cBusRecover(void);

/**
 * @brief 读 SDA 当前电平，调试/故障定位用
 */
extern Uint16 Hw_I2cReadSda(void);

/**
 * @brief 微秒级忙等延时
 *
 * @note 给上层做 EEPROM 写周期轮询间隔、上电稳住时间之类的等待用；
 *       精度取决于 u16LoopPerUs 的标定，只适合"粗延时"。
 */
extern void Hw_I2cWaitUs(Uint16 u16Us);

/**
 * @brief 统计量：NACK 次数 / 完整事务次数
 */
extern Uint16 Hw_I2cGetNackCnt(void);
extern Uint16 Hw_I2cGetXferCnt(void);

#ifdef __cplusplus
}
#endif

#endif /* HW_I2C_DRIVER_H */

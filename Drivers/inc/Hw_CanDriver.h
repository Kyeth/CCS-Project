/**
 * @file Hw_CanDriver.h
 * @author yetingheng (004182)
 * @brief F2837xS CAN驱动头文件（eCAN-A / eCAN-B）
 * @version 0.2
 * @date 2026-11-03
 *
 * @copyright Copyright (c) 2026
 *
 * 说明：F2837xS的CAN模块是32位寄存器的DCAN，报文对象通过IF1/IF2接口寄存器读写。
 *      本驱动只用IF1做发送、IF2做接收，不使用IF3；另有不占用IF的状态寄存器查询。
 *
 * 运行期约定（步骤 04）：
 *  - 运行期API返回明确状态码 OK/EMPTY/BUSY/ERR/UNKNOWN，调用方不得把 BUSY 当 ERR 重试、
 *    也不得把 UNKNOWN 当"未提交"自动重发。
 *  - 初始化API仍返回 0/1（1=成功），与工程既有驱动保持一致。
 *  - 超时按时间预算判定（Hw_TimeBase），不按循环次数。
 *  - 每个接收对象独立配置ID/掩码/帧类型/方向比较，不再共用一个模块级过滤值。
 */

#ifndef HW_CAN_DRIVER_H
#define HW_CAN_DRIVER_H

#include "F2837xS_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CAN模块编号：F2837xS共2个CAN模块，本板两个都用 */
typedef enum
{
    HW_CAN_ID_A = 0U,           /* CANA：GPIO36(RX)/GPIO37(TX) */
    HW_CAN_ID_B,                /* CANB：GPIO39(RX)/GPIO38(TX) */
    HW_CAN_ID_MAX,
    HW_CAN_ID_NULL = 0xFFU
} E_HW_CAN_ID;

/* 报文对象数量，F2837xS固定32个，编号1~32 */
#define HW_CAN_OBJ_MAX                  (32U)

/* 一帧最多8字节 */
#define HW_CAN_DATA_LEN_MAX             (8U)

/* 运行期状态码：EMPTY 可直接再查，BUSY 稍后重试，ERR 重试无效，UNKNOWN 需模块恢复 */
#define HW_CAN_EMPTY                    (0U)
#define HW_CAN_OK                       (1U)
#define HW_CAN_BUSY                     (2U)
#define HW_CAN_ERR                      (3U)
#define HW_CAN_UNKNOWN                  (4U)

/* 接收输出里的实际帧类型 */
#define HW_CAN_FRAME_STD                (0U)
#define HW_CAN_FRAME_EXT                (1U)

/* 单个接收对象的过滤配置，顺序与 u16RxObjNum 起的邮箱一一对应
 * 过渡允许 u32Id/u32Mask 都为 0（接收该帧类型的全部数据帧），真实ID由使用者后续确定
 * u16InitDlc 是对象初始化的DLС字段，不是长度过滤条件，实际长度校验另行定义
 * bDirCmp 固定为1：只接收数据帧，不收远程帧 */
typedef struct
{
    Uint32 u32Id;
    Uint32 u32Mask;
    Uint16 bExtId;
    Uint16 bDirCmp;
    Uint16 u16InitDlc;
    Uint16 u16Reserved;
} ST_CAN_RX_OBJ_CFG;

/* 一个CAN模块的配置项
 * u16TxObjNum/u16RxObjNum 是报文对象起始编号（1~32），发送对象在前、接收对象在后
 * pstRxObjCfg 指向逐接收对象配置数组，长度必须 >= u16RxObjCnt
 * bTxExtId 只决定发送帧类型，每模块固定 */
typedef struct
{
    Uint16 u16CanId;
    Uint32 u32Baudrate;
    Uint16 u16TxObjNum;
    Uint16 u16TxObjCnt;
    Uint16 u16RxObjNum;
    Uint16 u16RxObjCnt;
    const ST_CAN_RX_OBJ_CFG *pstRxObjCfg;
    Uint16 bTxExtId;
    Uint16 bLoopback;
    Uint16 bAutoBusOn;
} ST_CAN_CFG_ITEM;

/* CAN全局配置 */
typedef struct
{
    const ST_CAN_CFG_ITEM *pstCanCfgItem;
    Uint16 u16ItemNum;
    Uint32 u32SysclkHz;
} ST_CAN_CFG;

/**
 * @brief 初始化全部eCAN模块
 * @param pstCanCfg 全局配置
 * @return 1=全部成功，0=任一模块失败（失败模块保持未初始化状态）
 * 时间基准未就绪时直接返回0，不进入任何按时间等待
 */
Uint16 Hw_CanInit(const ST_CAN_CFG *pstCanCfg);

/**
 * @brief 查询模块是否已成功初始化
 * @param u16CanId 模块编号
 * @return 1=已初始化，0=未初始化或参数非法
 */
Uint16 Hw_CanIsInitialized(Uint16 u16CanId);

/**
 * @brief 查询模块是否处于未决状态（上次事务结果不确定）
 * @param u16CanId 模块编号
 * @return 1=未决，必须先 Hw_CanRecoverModule()；0=正常
 */
Uint16 Hw_CanIsPending(Uint16 u16CanId);

/**
 * @brief 发送一帧
 * @param u16CanId  模块编号
 * @param u16ObjIdx 发送对象序号（0起，相对u16TxObjNum）
 * @param u32MsgId  报文ID
 * @param pu16Data  数据缓冲，每单元一字节（低8位有效）
 * @param u16Len    数据长度（0~8）
 * @return HW_CAN_OK/BUSY/ERR/UNKNOWN
 * OK只表示本地提交被接受，不表示已上线；UNKNOWN表示提交后等待完成失败，
 * 驱动不会自动重发，是否重发由上层按协议序号或幂等规则决定
 */
Uint16 Hw_CanSend(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32MsgId,
                  const Uint16 *pu16Data, Uint16 u16Len);

/**
 * @brief 查询发送对象是否已完成（读CAN_TXRQ_21，不占用IF）
 * @param u16CanId  模块编号
 * @param u16ObjIdx 发送对象序号（0起）
 * @return 1=发送请求已不在（可能已发出、也可能从未提交或被取消），0=仍在等待或参数非法
 */
Uint16 Hw_CanIsTxDone(Uint16 u16CanId, Uint16 u16ObjIdx);

/**
 * @brief 查询接收对象是否有新数据（读CAN_NDAT_21，不占用IF）
 * @param u16CanId  模块编号
 * @param u16ObjIdx 接收对象序号（0起）
 * @return 1=有新数据，0=无新数据或参数非法
 * 只是瞬时状态，不替代 Hw_CanReadRx() 内部的新数据检查
 */
Uint16 Hw_CanIsRxReady(Uint16 u16CanId, Uint16 u16ObjIdx);

/**
 * @brief 读出一帧并清除NewDat
 * @param u16CanId      模块编号
 * @param u16ObjIdx     接收对象序号（0起）
 * @param pu32MsgId     输出报文ID（可为NULL）
 * @param pu16Data      输出数据缓冲，容量至少HW_CAN_DATA_LEN_MAX单元（可为NULL）
 * @param pu16Len       输出数据长度（可为NULL）
 * @param pu16FrameType 输出帧类型 HW_CAN_FRAME_STD/EXT（可为NULL）
 * @param pu16MsgLst    输出MsgLst标志0/1（可为NULL）
 * @return HW_CAN_OK/EMPTY/BUSY/ERR/UNKNOWN
 * EMPTY时所有输出参数内容无效；UNKNOWN时该模块接收路径已被标记为未决。
 * MsgLst 是"自上次由CPU清除此标志以来发生过覆盖"的粘滞标志，不是次数
 */
Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32MsgId,
                    Uint16 *pu16Data, Uint16 *pu16Len,
                    Uint16 *pu16FrameType, Uint16 *pu16MsgLst);

/**
 * @brief 读硬件状态寄存器
 * @param u16CanId 模块编号
 * @return CANES原始值；参数非法返回0
 * 仍返回硬件状态位，不列入运行期状态码体系
 */
Uint16 Hw_CanGetStatus(Uint16 u16CanId);

/**
 * @brief 查询是否处于总线关闭
 * @param u16CanId 模块编号
 * @return 1=总线关闭，0=正常或参数非法
 */
Uint16 Hw_CanIsBusOff(Uint16 u16CanId);

/**
 * @brief 恢复总线关闭（保留对象：只进出初始化，不碰报文RAM）
 * @param u16CanId 模块编号
 * @return HW_CAN_OK/BUSY/ERR/UNKNOWN
 * 与 Hw_CanRecoverModule 的分工：本入口保持位时序与全部报文对象不变，
 * 因此不会清掉未发完的帧；存在未决事务时返回 BUSY，必须先走模块级恢复
 */
Uint16 Hw_CanRecoverBusOff(Uint16 u16CanId);

/**
 * @brief 模块级恢复：从任意未决或故障状态重建整个模块
 * @param u16CanId 模块编号
 * @return HW_CAN_OK/BUSY/ERR/UNKNOWN
 * 内部使用CTL.SWR，因此不依赖IF1/IF2是否卡住。恢复只保证驱动重新可用，
 * 不证明此前UNKNOWN帧未上线，也不自动重发任何帧。
 * 能否恢复由"配置是否有效"决定；恢复失败时未决标志保留，
 * 可再次调用本函数重试，期间该模块的其它接口一律返回 ERR
 */
Uint16 Hw_CanRecoverModule(Uint16 u16CanId);

#ifdef __cplusplus
}
#endif

#endif /* HW_CAN_DRIVER_H */

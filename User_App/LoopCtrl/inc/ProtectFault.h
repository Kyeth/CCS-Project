/**
 * @file ProtectFault.h
 * @author yetingheng (004182)
 * @brief 保护故障框架：故障字、屏蔽字、严重度与动作等级
 * @version 0.1
 * @date 2026-10-28
 *
 * @copyright Copyright (c) 2026
 */

/*
 * 保护框架：故障位的记录、去抖、锁存、屏蔽与协议字聚合。
 *
 * 设计要点：
 * 1. 判据由各模块调用 ProtectTrigger/ProtectRelease 上报，本模块不认识传感器；
 * 2. 屏蔽位只压动作，不压记录，便于调试时观察真实越限；
 * 3. 屏蔽字不落 EEPROM，由上位机 500ms 心跳维持，超时自动解除；
 * 4. 状态机按功能域 + 严重度查询，实现 PV 故障不拦 INV、INV 故障不拦 PV。
 */
#ifndef USER_APP_LOOPCTRL_INC_PROTECTFAULT_H_
#define USER_APP_LOOPCTRL_INC_PROTECTFAULT_H_

#include "DrvCfg_Protect.h"

#define PROTECT_TICK_MS             (10U)     /* ProtectRun10ms 的节拍 */
#define PROTECT_MASK_HEARTBEAT_MS   (500U)    /* 屏蔽字心跳超时 */

typedef struct
{
    volatile Uint16 u16Status[PROTECT_WORD_NUM];      /* 故障记录位，屏蔽也置位 */
    Uint16          u16Pending[PROTECT_WORD_NUM];     /* 判据当前有效 */
    volatile Uint16 u16Mask[PROTECT_WORD_NUM];        /* 1=屏蔽动作，仍记录；上位机可改 */
    Uint16          u16DebounceCnt[PROTECT_FLT_NUM];  /* 去抖计数，单位 ms */
    Uint16          u16Proto[PROTECT_PROTO_WORD_NUM]; /* 聚合后的协议故障字 */
    volatile Uint16 u16ActionLevel;                   /* 未屏蔽位中的最高严重度 */
    volatile Uint16 u16TotalSign;                     /* 存在未屏蔽的 STOP 及以上 */
    Uint16          u16MaskAgeMs;                     /* 屏蔽字心跳计时 */
    Uint16          u16MaskShadow[PROTECT_WORD_NUM];  /* 屏蔽字影子，用于检测上位机改写 */
    volatile Uint16 u16MaskKeepAlive;                 /* 上位机心跳，值变化即续期（重复写同值无效） */
    Uint16          u16MaskKeepAliveLast;             /* 心跳影子 */
}ST_PROTECT_STATE;

extern ST_PROTECT_STATE stProtect;

void   ProtectInit(void);
void   ProtectRun10ms(void);

void   ProtectTrigger(E_PROTECT_FAULT eFault);
void   ProtectRelease(E_PROTECT_FAULT eFault);
Uint16 ProtectIsSet(E_PROTECT_FAULT eFault);

Uint16 ProtectGetWord(Uint16 u16Word);
Uint16 ProtectGetProto(Uint16 u16ProtoWord);
Uint16 ProtectGetActionLevel(void);
Uint16 ProtectGetTotalSign(void);

/* 按功能域 + 最低严重度查询，供状态机迁移判断 */
Uint16 ProtectQuery(Uint16 u16DomainMask, Uint16 u16MinSeverity);

void   ProtectSetMask(Uint16 u16Word, Uint16 u16Mask);
Uint16 ProtectGetMask(Uint16 u16Word);
void   ProtectMaskKeepAlive(void);

void   ProtectReset(void);

#endif /* USER_APP_LOOPCTRL_INC_PROTECTFAULT_H_ */

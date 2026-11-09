/**
 * @file Protocol.c
 * @brief 协议层框架：登记协议操作表、装配实例、1 ms 任务分发。
 */

#include "Protocol.h"

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_astInst, ".protobuf")
#endif
static ST_PROTO_INSTANCE s_astInst[PROTO_MAX_INSTANCES];
static Uint16 s_u16InstCount = 0U;

static const ST_PROTO_OPS *s_apstOps[PROTO_MAX_OPS];
static Uint16 s_u16OpsCount = 0U;

static Uint32 s_u32ServiceCalls = 0UL;
static Uint32 s_u32ServiceErrors = 0UL;

static const ST_PROTO_OPS *ProtoFindOps(Uint16 u16Type);

void ProtocolInit(void)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < PROTO_MAX_OPS; u16Index++) {
        s_apstOps[u16Index] = 0;
    }
    for (u16Index = 0U; u16Index < PROTO_MAX_INSTANCES; u16Index++) {
        s_astInst[u16Index].u16SciId = 0U;
        s_astInst[u16Index].u16Type = 0U;
        s_astInst[u16Index].u16SlaveAddr = 0U;
        s_astInst[u16Index].u16Enabled = 0U;
        s_astInst[u16Index].u16TblCount = 0U;
        s_astInst[u16Index].u32Baud = 0UL;
        s_astInst[u16Index].pstTbl = 0;
        s_astInst[u16Index].pCtx = 0;
    }

    s_u16InstCount = 0U;
    s_u16OpsCount = 0U;
    s_u32ServiceCalls = 0UL;
    s_u32ServiceErrors = 0UL;
}

Uint16 ProtocolRegisterOps(const ST_PROTO_OPS *pstOps)
{
    if (pstOps == 0) {
        return 0U;
    }
    if (s_u16OpsCount >= PROTO_MAX_OPS) {
        return 0U;
    }
    if (ProtoFindOps(pstOps->u16Type) != 0) {
        return 0U;   /* 同一协议类型只登记一次 */
    }

    s_apstOps[s_u16OpsCount] = pstOps;
    s_u16OpsCount++;

    return 1U;
}

Uint16 ProtocolAddInstance(const ST_PROTO_INSTANCE *pstInst)
{
    const ST_PROTO_OPS *pstOps;

    if ((pstInst == 0) || (s_u16InstCount >= PROTO_MAX_INSTANCES)) {
        return 0U;
    }

    pstOps = ProtoFindOps(pstInst->u16Type);
    if (pstOps == 0) {
        return 0U;   /* 没登记该协议的实现 */
    }

    s_astInst[s_u16InstCount] = *pstInst;
    s_astInst[s_u16InstCount].u16Enabled = 0U;

    if ((pstOps->pfInit != 0) && (pstOps->pfInit(&s_astInst[s_u16InstCount]) == 0U)) {
        return 0U;   /* 初始化失败：这个实例不参与服务 */
    }

    s_astInst[s_u16InstCount].u16Enabled = (pstInst->u16Enabled != 0U) ? 1U : 0U;
    s_u16InstCount++;

    return 1U;
}

void ProtocolTask(void)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < s_u16InstCount; u16Index++) {
        const ST_PROTO_OPS *pstOps;

        if (s_astInst[u16Index].u16Enabled == 0U) {
            continue;
        }

        pstOps = ProtoFindOps(s_astInst[u16Index].u16Type);
        if ((pstOps == 0) || (pstOps->pfService == 0)) {
            s_u32ServiceErrors++;
            continue;
        }

        s_u32ServiceCalls++;
        if (pstOps->pfService(&s_astInst[u16Index]) == 0U) {
            s_u32ServiceErrors++;
        }
    }
}

Uint16 ProtocolResetPort(Uint16 u16SciId)
{
    Uint16 u16Index;
    Uint16 u16Reset = 0U;

    for (u16Index = 0U; u16Index < s_u16InstCount; u16Index++) {
        const ST_PROTO_OPS *pstOps;

        if (s_astInst[u16Index].u16SciId != u16SciId) {
            continue;
        }
        pstOps = ProtoFindOps(s_astInst[u16Index].u16Type);
        if ((pstOps != 0) && (pstOps->pfReset != 0)) {
            pstOps->pfReset(&s_astInst[u16Index]);
            u16Reset++;
        }
    }

    return u16Reset;
}

Uint16 ProtocolInstanceCount(void)
{
    return s_u16InstCount;
}

const ST_PROTO_INSTANCE *ProtocolInstance(Uint16 u16Index)
{
    if (u16Index >= s_u16InstCount) {
        return 0;
    }
    return &s_astInst[u16Index];
}

Uint32 ProtocolServiceCalls(void)
{
    return s_u32ServiceCalls;
}

Uint32 ProtocolServiceErrors(void)
{
    return s_u32ServiceErrors;
}

static const ST_PROTO_OPS *ProtoFindOps(Uint16 u16Type)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < s_u16OpsCount; u16Index++) {
        if (s_apstOps[u16Index] == 0) {
            continue;
        }
        if (s_apstOps[u16Index]->u16Type == u16Type) {
            return s_apstOps[u16Index];
        }
    }

    return 0;
}

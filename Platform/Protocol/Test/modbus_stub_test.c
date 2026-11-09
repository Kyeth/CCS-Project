/**
 * @file modbus_stub_test.c
 * @brief 05D 主机测试：Modbus RTU 从机（组帧/CRC/点表/类型/异常应答）。
 *
 * 编译（主机 cc，不接 SciPort）：
 *   cc -std=c99 -Wall -Wextra -ITest/shim -I../Inc -I../Modbus/Inc \
 *      modbus_stub_test.c ../Modbus/Src/ModbusRtu.c ../Modbus/Src/ModbusTbl.c \
 *      -o modbus_stub_test
 *
 * 只覆盖协议核心与点表，不覆盖 SciPort 适配层（那部分由 05A/05B 桩覆盖）。
 */

#include <stdio.h>
#include "ModbusRtu.h"
#include "ModbusTbl.h"

static int s_iChecks = 0;
static int s_iFails = 0;

static void Check(int iCond, const char *pszName)
{
    s_iChecks++;
    if (iCond == 0)
    {
        s_iFails++;
        printf("FAIL: %s\n", pszName);
    }
}

/* ================= 点表与业务变量 ================= */

static Uint16 s_u16Reg0;
static Uint32 s_u32Reg12;
static float  s_fReg34;
static Uint16 s_u16Ro;

#define TBL_COUNT (4U)

static const ST_MODBUS_POINT s_astTbl[TBL_COUNT] =
{
    { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, &s_u16Reg0,  0UL,          65535UL,      0.0f,    0.0f },
    { 0x0001U, MB_PT_U32, MB_PT_READWRITE, 0U, &s_u32Reg12, 0UL,          0xFFFFFFFFUL, 0.0f,    0.0f },
    { 0x0003U, MB_PT_F32, MB_PT_READWRITE, 0U, &s_fReg34,   0UL,          0UL,          -100.0f, 100.0f },
    { 0x0005U, MB_PT_U16, MB_PT_READ,      0U, &s_u16Ro,    0UL,          65535UL,      0.0f,    0.0f },
};

static Uint16 TblRead(void *pCtx, Uint16 u16Start, Uint16 u16Count, Uint16 *pu16Values)
{
    (void)pCtx;
    return ModbusTblRead(s_astTbl, TBL_COUNT, u16Start, u16Count, pu16Values);
}

static Uint16 TblWrite(void *pCtx, Uint16 u16Start, Uint16 u16Count, const Uint16 *pu16Values)
{
    (void)pCtx;
    return ModbusTblWrite(s_astTbl, TBL_COUNT, u16Start, u16Count, pu16Values);
}

/* ================= 组帧工具 ================= */

static Uint16 BuildFrame(Uint16 *pu16Out, const Uint16 *pu16Body, Uint16 u16BodyLen)
{
    Uint16 u16Index;
    Uint16 u16Crc;

    for (u16Index = 0U; u16Index < u16BodyLen; u16Index++)
    {
        pu16Out[u16Index] = (Uint16)(pu16Body[u16Index] & 0xFFU);
    }
    u16Crc = ModbusCrc16(pu16Body, u16BodyLen);
    pu16Out[u16BodyLen] = (Uint16)(u16Crc & 0xFFU);
    pu16Out[u16BodyLen + 1U] = (Uint16)((u16Crc >> 8) & 0xFFU);

    return (Uint16)(u16BodyLen + 2U);
}

static void Deliver(ST_MB_INSTANCE *pst, const Uint16 *pu16Frame, Uint16 u16Len)
{
    Uint32 u32Tick = 1000UL;

    (void)ModbusRtuFeed(pst, pu16Frame, u16Len, u32Tick, 1U);
    (void)ModbusRtuPoll(pst, u32Tick + 1000UL, 1U);
}

/* ================= S-05D-01 CRC 与组帧 ================= */

static void TestCrc(void)
{
    static const Uint16 au16Body[] = { 0x01U, 0x03U, 0x00U, 0x00U, 0x00U, 0x0AU };
    Uint16 u16Crc = ModbusCrc16(au16Body, 6U);

    Check((Uint16)(u16Crc & 0xFFU) == 0xC5U, "S-05D-01 CRC 低字节 = 0xC5");
    Check((Uint16)((u16Crc >> 8) & 0xFFU) == 0xCDU, "S-05D-01 CRC 高字节 = 0xCD");
}

/* ================= S-05D-02 点表初始化检查 ================= */

static void TestTableValidate(void)
{
    static Uint16 s_u16DummyA;
    static Uint16 s_u16DummyB;
    /* 地址重叠：0x0000 与 0x0000 */
    static const ST_MODBUS_POINT astOverlap[] =
    {
        { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, &s_u16DummyA, 0UL, 10UL, 0.0f, 0.0f },
        { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, &s_u16DummyB, 0UL, 10UL, 0.0f, 0.0f },
    };
    /* Uint16 配上限 66530：必须在配置检查里被拒绝 */
    static const ST_MODBUS_POINT astTooWide[] =
    {
        { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, &s_u16DummyA, 0UL, 66530UL, 0.0f, 0.0f },
    };
    /* 浮点上下限颠倒 */
    static const ST_MODBUS_POINT astBadFloat[] =
    {
        { 0x0000U, MB_PT_F32, MB_PT_READWRITE, 0U, &s_u16DummyA, 0UL, 0UL, 10.0f, -10.0f },
    };
    /* 32 位项中间再放一个点：区间重叠 */
    static const ST_MODBUS_POINT astMidOverlap[] =
    {
        { 0x0000U, MB_PT_U32, MB_PT_READWRITE, 0U, &s_u16DummyA, 0UL, 0xFFFFFFFFUL, 0.0f, 0.0f },
        { 0x0001U, MB_PT_U16, MB_PT_READWRITE, 0U, &s_u16DummyB, 0UL, 10UL, 0.0f, 0.0f },
    };
    /* 未绑定变量 */
    static const ST_MODBUS_POINT astNoVar[] =
    {
        { 0x0000U, MB_PT_U16, MB_PT_READWRITE, 0U, 0, 0UL, 10UL, 0.0f, 0.0f },
    };

    Check(ModbusTblValidate(s_astTbl, TBL_COUNT) == 1U, "S-05D-02 正常点表通过");
    Check(ModbusTblValidate(astOverlap, 2U) == 0U, "S-05D-02 地址重叠被拒");
    Check(ModbusTblValidate(astTooWide, 1U) == 0U, "S-05D-02 Uint16 配 66530 被拒");
    Check(ModbusTblValidate(astBadFloat, 1U) == 0U, "S-05D-02 浮点上下限颠倒被拒");
    Check(ModbusTblValidate(astMidOverlap, 2U) == 0U, "S-05D-02 32 位项中间重叠被拒");
    Check(ModbusTblValidate(astNoVar, 1U) == 0U, "S-05D-02 未绑定变量被拒");
}

/* ================= S-05D-03 类型转换 ================= */

static void TestTypedAccess(void)
{
    Uint16 au16Out[4];
    Uint16 au16In[4];

    /* Uint32：高字在前 */
    au16In[0] = 0x1234U;
    au16In[1] = 0x5678U;
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0001U, 2U, au16In) == MB_EX_NONE, "S-05D-03 写 Uint32 成功");
    Check(s_u32Reg12 == 0x12345678UL, "S-05D-03 Uint32 高字在前");
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0001U, 2U, au16Out) == MB_EX_NONE, "S-05D-03 读 Uint32 成功");
    Check((au16Out[0] == 0x1234U) && (au16Out[1] == 0x5678U), "S-05D-03 Uint32 读回顺序");

    /* float 往返 */
    s_fReg34 = 12.5f;
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0003U, 2U, au16Out) == MB_EX_NONE, "S-05D-03 读 float 成功");
    Check((au16Out[0] == 0x4148U) && (au16Out[1] == 0x0000U), "S-05D-03 float 12.5 编码");
    au16In[0] = 0xC248U;
    au16In[1] = 0x0000U;
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0003U, 2U, au16In) == MB_EX_NONE, "S-05D-03 写 float 成功");
    Check((s_fReg34 > -50.01f) && (s_fReg34 < -49.99f), "S-05D-03 float -50 解码");

    /* 单寄存器读写 */
    au16In[0] = 0x00FFU;
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0000U, 1U, au16In) == MB_EX_NONE, "S-05D-03 写 Uint16 成功");
    Check(s_u16Reg0 == 0x00FFU, "S-05D-03 Uint16 值正确");
}

/* ================= S-05D-04 访问限制 ================= */

static void TestAccessLimits(void)
{
    Uint16 au16In[4];
    Uint16 au16Out[4];

    s_u32Reg12 = 0xAABBCCDDUL;

    /* 32 位项只取一个寄存器 */
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0001U, 1U, au16Out) == MB_EX_ILLEGAL_ADDR,
          "S-05D-04 32 位项不完整读 -> 0x02");
    /* 从 32 位项第二个寄存器开始 */
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0002U, 1U, au16Out) == MB_EX_ILLEGAL_ADDR,
          "S-05D-04 32 位项中间地址 -> 0x02");
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0001U, 1U, au16In) == MB_EX_ILLEGAL_ADDR,
          "S-05D-04 32 位项不完整写 -> 0x02");
    Check(s_u32Reg12 == 0xAABBCCDDUL, "S-05D-04 不完整写没有改值");

    /* 只读点被写 */
    au16In[0] = 0x0001U;
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0005U, 1U, au16In) == MB_EX_ILLEGAL_ADDR,
          "S-05D-04 只读点被写 -> 0x02");

    /* 不存在的地址 */
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0010U, 1U, au16Out) == MB_EX_ILLEGAL_ADDR,
          "S-05D-04 不存在地址 -> 0x02");

    /* 浮点超限：0x42C8 0000 = 100.0；限值 -100..100，写 101 应拒绝 */
    s_fReg34 = 1.0f;
    au16In[0] = 0x42CAU;   /* 101.0f */
    au16In[1] = 0x0000U;
    Check(ModbusTblWrite(s_astTbl, TBL_COUNT, 0x0003U, 2U, au16In) == MB_EX_ILLEGAL_VALUE,
          "S-05D-04 超限写 -> 0x03");
    Check((s_fReg34 > 0.99f) && (s_fReg34 < 1.01f), "S-05D-04 超限写没有改值");

    /* 数量越界 */
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0000U, 0U, au16Out) == MB_EX_ILLEGAL_VALUE,
          "S-05D-04 数量 0 -> 0x03");
    Check(ModbusTblRead(s_astTbl, TBL_COUNT, 0x0000U, 33U, au16Out) == MB_EX_ILLEGAL_VALUE,
          "S-05D-04 数量 33 -> 0x03");
}

/* ================= S-05D-05 帧级行为 ================= */

static ST_MB_INSTANCE s_stMb;
static Uint16 s_au16Frame[300];

static void MbSetUp(void)
{
    ModbusRtuInit(&s_stMb, 1U, 100UL, 100000UL, 1U);
    ModbusRtuConfigure(&s_stMb, 0, TblRead, TblWrite);
    s_u16Reg0 = 0U;
    s_u32Reg12 = 0UL;
    s_fReg34 = 0.0f;
    s_u16Ro = 0x0055U;
}

static void TestFrames(void)
{
    Uint16 au16Body[16];
    Uint16 u16Len;
    Uint16 u16Index;

    /* 0x03 读 0x0000 起 1 个寄存器 */
    MbSetUp();
    s_u16Reg0 = 0x1234U;
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_stMb.u16TxLen == 7U, "S-05D-05 0x03 回复长度 = 7");
    Check((s_stMb.au16TxBuf[0] == 0x01U) && (s_stMb.au16TxBuf[1] == 0x03U), "S-05D-05 0x03 回复头");
    Check(s_stMb.au16TxBuf[2] == 2U, "S-05D-05 0x03 字节数 = 2");
    Check((s_stMb.au16TxBuf[3] == 0x12U) && (s_stMb.au16TxBuf[4] == 0x34U), "S-05D-05 0x03 寄存器高字节在前");
    Check(s_stMb.u32RxFrames == 1UL, "S-05D-05 0x03 计入执行数");

    /* CRC 错：不回复 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    s_au16Frame[u16Len - 1U] ^= 0x01U;
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check((s_stMb.u16TxLen == 0U) && (s_stMb.u32RxDropped == 1UL), "S-05D-05 坏 CRC 丢弃且不回复");

    /* 地址不符：不回复 */
    MbSetUp();
    au16Body[0] = 0x07U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_stMb.u16TxLen == 0U, "S-05D-05 地址不符不回复");

    /* 广播写：执行但不回复 */
    MbSetUp();
    au16Body[0] = 0x00U; au16Body[1] = 0x06U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x00U; au16Body[5] = 0x42U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_u16Reg0 == 0x0042U, "S-05D-05 广播写执行");
    Check(s_stMb.u16TxLen == 0U, "S-05D-05 广播写不回复");

    /* 不支持的功能码 -> 0x01 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x04U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_stMb.u16TxLen == 5U, "S-05D-05 异常回复长度 = 5");
    Check((s_stMb.au16TxBuf[1] == 0x84U) && (s_stMb.au16TxBuf[2] == MB_EX_ILLEGAL_FUNC),
          "S-05D-05 不支持功能码 -> 0x01");

    /* 0x06 写单寄存器并回显 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x06U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x12U; au16Body[5] = 0x34U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_u16Reg0 == 0x1234U, "S-05D-05 0x06 写入生效");
    Check((s_stMb.u16TxLen == 8U) && (s_stMb.au16TxBuf[4] == 0x12U), "S-05D-05 0x06 回显");

    /* 0x10 写两个寄存器（Uint32）并回复 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x10U;
    au16Body[2] = 0x00U; au16Body[3] = 0x01U;
    au16Body[4] = 0x00U; au16Body[5] = 0x02U;
    au16Body[6] = 0x04U;
    au16Body[7] = 0x11U; au16Body[8] = 0x22U;
    au16Body[9] = 0x33U; au16Body[10] = 0x44U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 11U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_u32Reg12 == 0x11223344UL, "S-05D-05 0x10 写入生效");
    Check(s_stMb.u16TxLen == 8U, "S-05D-05 0x10 回复长度 = 8");

    /* 0x10 byteCount 与数量不符 -> 0x03 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x10U;
    au16Body[2] = 0x00U; au16Body[3] = 0x01U;
    au16Body[4] = 0x00U; au16Body[5] = 0x02U;
    au16Body[6] = 0x02U;                     /* 应为 4 */
    au16Body[7] = 0x11U; au16Body[8] = 0x22U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 9U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check((s_stMb.u16TxLen == 5U) && (s_stMb.au16TxBuf[2] == MB_EX_ILLEGAL_VALUE),
          "S-05D-05 byteCount 不符 -> 0x03");
    Check(s_u32Reg12 == 0UL, "S-05D-05 byteCount 不符没有写");

    /* 只读点被 0x06 写 -> 0x02 */
    MbSetUp();
    au16Body[0] = 0x01U; au16Body[1] = 0x06U;
    au16Body[2] = 0x00U; au16Body[3] = 0x05U;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check((s_stMb.u16TxLen == 5U) && (s_stMb.au16TxBuf[2] == MB_EX_ILLEGAL_ADDR),
          "S-05D-05 只读点被写 -> 0x02");

    /* 回复分两次提交：先接受 3 字节，再接受剩余 */
    MbSetUp();
    s_u16Reg0 = 0xAABBU;
    au16Body[0] = 0x01U; au16Body[1] = 0x03U;
    au16Body[2] = 0x00U; au16Body[3] = 0x00U;
    au16Body[4] = 0x00U; au16Body[5] = 0x01U;
    u16Len = BuildFrame(s_au16Frame, au16Body, 6U);
    Deliver(&s_stMb, s_au16Frame, u16Len);
    Check(s_stMb.u16TxLen == 7U, "S-05D-05 待提交回复 = 7 字节");
    Check(ModbusRtuConsumeReply(&s_stMb, 3U) == 0U, "S-05D-05 部分提交后仍未完成");
    Check(s_stMb.u16TxOffset == 3U, "S-05D-05 offset 推进到 3");
    u16Index = s_stMb.u16TxOffset;
    Check(s_stMb.au16TxBuf[u16Index] == 0xAAU, "S-05D-05 未提交部分从 offset 继续");
    Check(ModbusRtuConsumeReply(&s_stMb, 4U) == 1U, "S-05D-05 剩余提交完成");
    Check(s_stMb.u32TxSubmitted == 1UL, "S-05D-05 整帧只计一次已提交");

    /* 强制复位作废待发回复 */
    MbSetUp();
    Check(ModbusRtuConsumeReply(&s_stMb, 0U) == 0U, "S-05D-05 无回复时提交返回 0");
    Check(ModbusRtuDiscardReply(&s_stMb) == 0U, "S-05D-05 无回复时作废返回 0");
}

int main(void)
{
    TestCrc();
    TestTableValidate();
    TestTypedAccess();
    TestAccessLimits();
    TestFrames();

    printf("checks=%d fails=%d\n", s_iChecks, s_iFails);

    return (s_iFails == 0) ? 0 : 1;
}

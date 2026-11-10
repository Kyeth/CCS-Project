/*
 * AppModbusProtoTbl.c
 *
 * SCI-A 的 Modbus 点表（按 ARM/DSP 协议表生成，共 440 项）。
 * 由 _tmp/gen_proto_tbl.py 生成，不要手工修改。
 */

#include "AppModbusProtoTbl.h"

/* 占位存储：协议要求绑定变量，本工程尚未接入业务变量，先整块占位。 */
static Uint16 s_au16ProtoRegs[APP_PROTO_REG_COUNT];

#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_au16ProtoRegs, ".mbregs")
#endif

const ST_MODBUS_POINT g_astAppProtoPointTbl[APP_PROTO_REG_COUNT] = {
    /*    0  L1相直流分量 */
    {    0U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  0], 0UL, 65535UL, 0.0F, 0.0F },
    /*    1  L2相直流分量 */
    {    1U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  1], 0UL, 65535UL, 0.0F, 0.0F },
    /*    2  预留 */
    {    2U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  2], 0UL, 65535UL, 0.0F, 0.0F },
    /*    3  L1相目标有功功率 */
    {    3U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  3], 0UL, 65535UL, 0.0F, 0.0F },
    /*    4  L2相目标有功功率 */
    {    4U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  4], 0UL, 65535UL, 0.0F, 0.0F },
    /*    5  预留 */
    {    5U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  5], 0UL, 65535UL, 0.0F, 0.0F },
    /*    6  L1相馈网功率与负载功率之和 */
    {    6U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  6], 0UL, 65535UL, 0.0F, 0.0F },
    /*    7  L2相馈网功率与负载功率之和 */
    {    7U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  7], 0UL, 65535UL, 0.0F, 0.0F },
    /*    8  预留 */
    {    8U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  8], 0UL, 65535UL, 0.0F, 0.0F },
    /*    9  BMS允许充电电压 */
    {    9U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[  9], 0UL, 65535UL, 0.0F, 0.0F },
    /*   10  BMS允许充电功率 */
    {   10U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 10], 0UL, 65535UL, 0.0F, 0.0F },
    /*   11  BMS允许放电功率 */
    {   11U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 11], 0UL, 65535UL, 0.0F, 0.0F },
    /*   12  系统开关机设置 */
    {   12U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 12], 0UL, 65535UL, 0.0F, 0.0F },
    /*   13  恢复出厂设置 */
    {   13U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 13], 0UL, 65535UL, 0.0F, 0.0F },
    /*   14  频率设置 */
    {   14U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 14], 0UL, 50UL, 0.0F, 0.0F },
    /*   15  光伏目标功率 */
    {   15U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 15], 0UL, 65535UL, 0.0F, 0.0F },
    /*   16  电网最大充电电流设置 */
    {   16U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 16], 0UL, 65535UL, 0.0F, 0.0F },
    /*   17  电网最大放电电流设置 */
    {   17U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 17], 0UL, 65535UL, 0.0F, 0.0F },
    /*   18  ARM状态字 */
    {   18U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 18], 0UL, 65535UL, 0.0F, 0.0F },
    /*   19  ARM使能状态 */
    {   19U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 19], 0UL, 65535UL, 0.0F, 0.0F },
    /*   20  PV并联使能设置 */
    {   20U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 20], 0UL, 65535UL, 0.0F, 0.0F },
    /*   21  L1相负载电压 */
    {   21U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 21], 0UL, 65535UL, 0.0F, 0.0F },
    /*   22  L2相负载电压 */
    {   22U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 22], 0UL, 65535UL, 0.0F, 0.0F },
    /*   23  预留 */
    {   23U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 23], 0UL, 65535UL, 0.0F, 0.0F },
    /*   24  风扇档位控制 */
    {   24U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 24], 0UL, 0UL, 0.0F, 0.0F },
    /*   25  电压设置 */
    {   25U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 25], 0UL, 65535UL, 0.0F, 0.0F },
    /*   26  地区设置 */
    {   26U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 26], 0UL, 65535UL, 0.0F, 0.0F },
    /*   27  L1相电网视在功率 */
    {   27U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 27], 0UL, 65535UL, 0.0F, 0.0F },
    /*   28  L2相电网视在功率 */
    {   28U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 28], 0UL, 65535UL, 0.0F, 0.0F },
    /*   29  预留 */
    {   29U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 29], 0UL, 65535UL, 0.0F, 0.0F },
    /*   30  PV温度限功率 */
    {   30U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 30], 0UL, 65535UL, 0.0F, 0.0F },
    /*   31  PV3目标功率 */
    {   31U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 31], 0UL, 7000UL, 0.0F, 0.0F },
    /*   32  电网增强模式设置 */
    {   32U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 32], 0UL, 65535UL, 0.0F, 0.0F },
    /*   33  PV4目标功率 */
    {   33U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 33], 0UL, 65535UL, 0.0F, 0.0F },
    /*   34  L1相并网继电器中点电压 */
    {   34U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 34], 0UL, 65535UL, 0.0F, 0.0F },
    /*   35  老化标签 */
    {   35U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 35], 0UL, 65535UL, 0.0F, 0.0F },
    /*   36  逆变器并/离网控制 */
    {   36U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 36], 0UL, 65535UL, 0.0F, 0.0F },
    /*   37  光伏输入源设置 */
    {   37U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 37], 0UL, 65535UL, 0.0F, 0.0F },
    /*   38  PV1目标功率 */
    {   38U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 38], 0UL, 7000UL, 0.0F, 0.0F },
    /*   39  PV2目标功率 */
    {   39U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 39], 0UL, 7000UL, 0.0F, 0.0F },
    /*   40  L2相并网继电器中点电压 */
    {   40U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 40], 0UL, 65535UL, 0.0F, 0.0F },
    /*   41  逆变开环使能 */
    {   41U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 41], 0UL, 65535UL, 0.0F, 0.0F },
    /*   42  BMS紧急充电 */
    {   42U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 42], 0UL, 65535UL, 0.0F, 0.0F },
    /*   43  电池包运行状态 */
    {   43U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 43], 0UL, 65535UL, 0.0F, 0.0F },
    /*   44  PV1电流限制值 */
    {   44U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 44], 0UL, 65535UL, 0.0F, 0.0F },
    /*   45  PV2电流限制值 */
    {   45U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 45], 0UL, 65535UL, 0.0F, 0.0F },
    /*   46  PV3电流限制值 */
    {   46U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 46], 0UL, 65535UL, 0.0F, 0.0F },
    /*   47  PV4电流限制值 */
    {   47U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 47], 0UL, 65535UL, 0.0F, 0.0F },
    /*   48  BUS稳压电压 */
    {   48U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 48], 0UL, 65535UL, 0.0F, 0.0F },
    /*   49  SN码 Bit0-Bit15 */
    {   49U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 49], 0UL, 65535UL, 0.0F, 0.0F },
    /*   50  SN码 Bit16-Bit31 */
    {   50U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 50], 0UL, 65535UL, 0.0F, 0.0F },
    /*   51  SN码 Bit32-Bit47 */
    {   51U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 51], 0UL, 65535UL, 0.0F, 0.0F },
    /*   52  SN码 Bit48-Bit63 */
    {   52U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 52], 0UL, 65535UL, 0.0F, 0.0F },
    /*   53  状态使能 */
    {   53U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 53], 0UL, 65535UL, 0.0F, 0.0F },
    /*   54  GFCI标志位 */
    {   54U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 54], 0UL, 65535UL, 0.0F, 0.0F },
    /*   55  电池包SOC */
    {   55U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 55], 0UL, 65535UL, 0.0F, 0.0F },
    /*   56  BMS加热请求 */
    {   56U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 56], 0UL, 65535UL, 0.0F, 0.0F },
    /*   57  BMS母线电压 */
    {   57U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 57], 0UL, 65535UL, 0.0F, 0.0F },
    /*  500  模式使能 */
    {  500U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 58], 0UL, 65535UL, 0.0F, 0.0F },
    /*  501  恒功率因数设置 */
    {  501U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 59], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  502  恒功率因数超前/滞后的设置 */
    {  502U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 60], 0UL, 65535UL, 0.0F, 0.0F },
    /*  503  基准电压 */
    {  503U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 61], 0UL, 65535UL, 0.0F, 0.0F },
    /*  504  自动调整基准电压使能 */
    {  504U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 62], 0UL, 65535UL, 0.0F, 0.0F },
    /*  505  调整基准电压恒定的时间 */
    {  505U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 63], 0UL, 65535UL, 0.0F, 0.0F },
    /*  506  电压无功V1 */
    {  506U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 64], 0UL, 65535UL, 0.0F, 0.0F },
    /*  507  电压无功V2 */
    {  507U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 65], 0UL, 65535UL, 0.0F, 0.0F },
    /*  508  电压无功V3 */
    {  508U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 66], 0UL, 65535UL, 0.0F, 0.0F },
    /*  509  电压无功V4 */
    {  509U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 67], 0UL, 65535UL, 0.0F, 0.0F },
    /*  510  电压无功Q1 */
    {  510U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 68], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  511  电压无功Q2 */
    {  511U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 69], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  512  电压无功Q3 */
    {  512U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 70], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  513  电压无功Q4 */
    {  513U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 71], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  514  开环响应时间 */
    {  514U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 72], 0UL, 65535UL, 0.0F, 0.0F },
    /*  515  有功无功P1 */
    {  515U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 73], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  516  有功无功P2 */
    {  516U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 74], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  517  有功无功P3 */
    {  517U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 75], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  518  有功无功P1(取反) */
    {  518U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 76], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  519  有功无功P2(取反) */
    {  519U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 77], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  520  有功无功P3(取反) */
    {  520U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 78], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  521  有功无功Q1 */
    {  521U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 79], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  522  有功无功Q2 */
    {  522U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 80], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  523  有功无功Q3 */
    {  523U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 81], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  524  有功无功Q1(取反) */
    {  524U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 82], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  525  有功无功Q2(取反) */
    {  525U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 83], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  526  有功无功Q3(取反) */
    {  526U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 84], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  527  恒定的无功功率 */
    {  527U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 85], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  528  电压有功V1 */
    {  528U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 86], 0UL, 65535UL, 0.0F, 0.0F },
    /*  529  电压有功V2 */
    {  529U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 87], 0UL, 65535UL, 0.0F, 0.0F },
    /*  530  电压有功P1 */
    {  530U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 88], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  531  电压有功P2 */
    {  531U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 89], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /*  532  开环响应时间2 */
    {  532U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 90], 0UL, 65535UL, 0.0F, 0.0F },
    /*  533  过压保护点1 */
    {  533U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 91], 0UL, 65535UL, 0.0F, 0.0F },
    /*  534  过压保护点2 */
    {  534U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 92], 0UL, 65535UL, 0.0F, 0.0F },
    /*  535  过压延迟保护时间1 */
    {  535U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 93], 0UL, 65535UL, 0.0F, 0.0F },
    /*  536  过压延迟保护时间2 */
    {  536U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 94], 0UL, 65535UL, 0.0F, 0.0F },
    /*  537  欠压保护点1 */
    {  537U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 95], 0UL, 65535UL, 0.0F, 0.0F },
    /*  538  欠压保护点2 */
    {  538U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 96], 0UL, 65535UL, 0.0F, 0.0F },
    /*  539  欠压延迟保护时间1 */
    {  539U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 97], 0UL, 65535UL, 0.0F, 0.0F },
    /*  540  欠压延迟保护时间2 */
    {  540U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 98], 0UL, 65535UL, 0.0F, 0.0F },
    /*  541  过频保护点1 */
    {  541U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[ 99], 0UL, 65535UL, 0.0F, 0.0F },
    /*  542  过频保护点2 */
    {  542U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[100], 0UL, 65535UL, 0.0F, 0.0F },
    /*  543  过频延迟保护时间1低字节 */
    {  543U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[101], 0UL, 65535UL, 0.0F, 0.0F },
    /*  544  过频延迟保护时间1高字节 */
    {  544U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[102], 0UL, 65535UL, 0.0F, 0.0F },
    /*  545  过频延迟保护时间2低字节 */
    {  545U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[103], 0UL, 65535UL, 0.0F, 0.0F },
    /*  546  过频延迟保护时间2高字节 */
    {  546U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[104], 0UL, 65535UL, 0.0F, 0.0F },
    /*  547  欠频保护1 */
    {  547U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[105], 0UL, 65535UL, 0.0F, 0.0F },
    /*  548  欠频保护2 */
    {  548U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[106], 0UL, 65535UL, 0.0F, 0.0F },
    /*  549  欠频延迟保护时间1低字节 */
    {  549U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[107], 0UL, 65535UL, 0.0F, 0.0F },
    /*  550  欠频延迟保护时间1高字节 */
    {  550U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[108], 0UL, 65535UL, 0.0F, 0.0F },
    /*  551  欠频延迟保护时间2低字节 */
    {  551U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[109], 0UL, 65535UL, 0.0F, 0.0F },
    /*  552  欠频延迟保护时间2高字节 */
    {  552U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[110], 0UL, 65535UL, 0.0F, 0.0F },
    /*  553  过频正偏移值DBOF */
    {  553U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[111], 0UL, 65535UL, 0.0F, 0.0F },
    /*  554  过频负偏移值DBUF */
    {  554U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[112], 0UL, 65535UL, 0.0F, 0.0F },
    /*  555  过频正偏移值系数KOF */
    {  555U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[113], 0UL, 65535UL, 0.0F, 0.0F },
    /*  556  过频负偏移值系数KUF */
    {  556U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[114], 0UL, 65535UL, 0.0F, 0.0F },
    /*  557  开环响应时间3 */
    {  557U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[115], 0UL, 65535UL, 0.0F, 0.0F },
    /*  558  并网使能 */
    {  558U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[116], 0UL, 65535UL, 0.0F, 0.0F },
    /*  559  允许并网的最大电网电压 */
    {  559U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[117], 0UL, 65535UL, 0.0F, 0.0F },
    /*  560  允许并网的最小电网电压 */
    {  560U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[118], 0UL, 65535UL, 0.0F, 0.0F },
    /*  561  允许并网的最大电网频率 */
    {  561U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[119], 0UL, 65535UL, 0.0F, 0.0F },
    /*  562  允许并网的最小电网频率 */
    {  562U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[120], 0UL, 65535UL, 0.0F, 0.0F },
    /*  563  并网延迟时间 */
    {  563U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[121], 0UL, 65535UL, 0.0F, 0.0F },
    /*  564  并网随机延迟时间 */
    {  564U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[122], 0UL, 65535UL, 0.0F, 0.0F },
    /*  565  并网功率爬升时间 */
    {  565U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[123], 0UL, 65535UL, 0.0F, 0.0F },
    /*  566  最大有功功率 */
    {  566U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[124], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1000  逆变L1电压采样系数 */
    { 1000U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[125], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1001  逆变L2电压采样系数 */
    { 1001U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[126], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1002  预留 */
    { 1002U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[127], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1003  逆变L1相电流采样系数_重载 */
    { 1003U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[128], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1004  逆变L2相电流采样系数_重载 */
    { 1004U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[129], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1005  预留 */
    { 1005U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[130], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1006  电网L1电压采样系数 */
    { 1006U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[131], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1007  电网L2电压采样系数 */
    { 1007U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[132], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1008  预留 */
    { 1008U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[133], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1009  负载L1相电流采样系数 */
    { 1009U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[134], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1010  负载L2相电流采样系数 */
    { 1010U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[135], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1011  预留 */
    { 1011U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[136], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1012  PV1电压采样系数 */
    { 1012U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[137], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1013  PV2电压采样系数 */
    { 1013U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[138], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1014  PV3电压采样系数 */
    { 1014U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[139], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1015  PV4电压采样系数 */
    { 1015U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[140], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1016  PV1电流采样系数 */
    { 1016U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[141], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1017  PV2电流采样系数 */
    { 1017U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[142], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1018  PV3电流采样系数 */
    { 1018U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[143], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1019  PV4电流采样系数 */
    { 1019U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[144], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1020  PV1绝缘电阻采样系数 */
    { 1020U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[145], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1021  PV2绝缘电阻采样系数 */
    { 1021U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[146], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1022  PV3绝缘电阻采样系数 */
    { 1022U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[147], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1023  PV4绝缘电阻采样系数 */
    { 1023U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[148], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1024  BUS1电压采样系数（正BUS电压采样系数） */
    { 1024U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[149], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1025  BUS2电压采样系数（负BUS电压采样系数） */
    { 1025U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[150], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1026  BUS3电压采样系数 */
    { 1026U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[151], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1027  LLC1电压采样系数 */
    { 1027U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[152], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1028  LLC2电压采样系数 */
    { 1028U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[153], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1029  LLC3电压采样系数 */
    { 1029U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[154], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1030  GFCI漏电流采样系数 */
    { 1030U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[155], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1031  双向DC1（BUCK/BOOST）电流采样系数 */
    { 1031U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[156], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1032  双向DC2（BUCK/BOOST）电流采样系数 */
    { 1032U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[157], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1033  逆变L1电流采样系数_轻载 */
    { 1033U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[158], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1034  逆变L2电流采样系数_轻载 */
    { 1034U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[159], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1035  预留 */
    { 1035U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[160], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1036  整流L1电流采样系数_重载 */
    { 1036U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[161], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1037  整流L2电流采样系数_重载 */
    { 1037U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[162], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1038  预留 */
    { 1038U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[163], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1039  整流L1电流采样系数_轻载 */
    { 1039U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[164], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1040  整流L2电流采样系数_轻载 */
    { 1040U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[165], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1041  预留 */
    { 1041U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[166], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1042  逆变L1功率校正系数_重载 */
    { 1042U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[167], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1043  逆变L2功率校正系数_重载 */
    { 1043U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[168], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1044  预留 */
    { 1044U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[169], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1045  逆变L1功率校正系数_轻载 */
    { 1045U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[170], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1046  逆变L2功率校正系数_轻载 */
    { 1046U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[171], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1047  预留 */
    { 1047U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[172], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1048  整流L1功率校正系数_重载 */
    { 1048U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[173], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1049  整流L2功率校正系数_重载 */
    { 1049U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[174], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1050  预留 */
    { 1050U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[175], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1051  整流L1功率校正系数_轻载 */
    { 1051U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[176], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1052  整流L2功率校正系数_轻载 */
    { 1052U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[177], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1053  预留 */
    { 1053U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[178], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1054  逆变L1相控制电压校正系数 */
    { 1054U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[179], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1055  逆变L2相控制电压校正系数 */
    { 1055U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[180], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1056  预留 */
    { 1056U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[181], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1057  逆变L1负载电压校正系数 */
    { 1057U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[182], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1058  逆变L2负载电压校正系数 */
    { 1058U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[183], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1059  预留 */
    { 1059U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[184], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1060  逆变L1中点电压校正系数 */
    { 1060U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[185], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1061  逆变L2中点电压校正系数 */
    { 1061U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[186], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1500  机器的测试类型 */
    { 1500U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[187], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1501  机器的故障执行标准 */
    { 1501U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[188], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1502  孤岛类别 */
    { 1502U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[189], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1503  最大注入无功功率的额定值 */
    { 1503U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[190], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1504  最大吸收无功功率的额定值 */
    { 1504U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[191], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1505  支持的模式 */
    { 1505U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[192], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1506  允许并网模式的选择 */
    { 1506U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[193], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1507  进入电网电压的最小值 */
    { 1507U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[194], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1508  进入电网电压的最大值 */
    { 1508U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[195], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1509  进入电网频率的最小值 */
    { 1509U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[196], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1510  进入电网频率的最大值 */
    { 1510U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[197], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1511  软启随机延迟使能 */
    { 1511U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[198], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1512  软启最小延迟时间 */
    { 1512U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[199], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1513  软启动爬坡时间 */
    { 1513U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[200], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1514  恒定PF值模式使能 */
    { 1514U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[201], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1515  功率因数PF超前/滞后 */
    { 1515U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[202], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1516  恒定的PF值 */
    { 1516U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[203], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1517  达到恒功率因数PF的最小开环响应时间 */
    { 1517U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[204], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1518  达到恒功率因数PF的最大开环响应时间 */
    { 1518U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[205], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1519  恒定的无功功率使能 */
    { 1519U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[206], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1520  恒定的无功功率方向选择 */
    { 1520U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[207], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1521  恒定的无功功率值 */
    { 1521U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[208], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1522  达到恒定无功功率的最小开环响应时间 */
    { 1522U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[209], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1523  达到恒定无功功率的最大开环响应时间 */
    { 1523U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[210], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1524  电网电压调整无功功率使能 */
    { 1524U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[211], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1525  电网电压调整无功功率时的电压基准 */
    { 1525U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[212], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1526  电网电压自动调整无功功率使能 */
    { 1526U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[213], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1527  电网电压自动调整无功功率的开环响应时间 */
    { 1527U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[214], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1528  电网电压与无功功率曲线中V2值 */
    { 1528U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[215], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1529  电网电压与无功功率曲线中Q2值 */
    { 1529U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[216], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1530  电网电压与无功功率曲线中V3值 */
    { 1530U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[217], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1531  电网电压与无功功率曲线中Q3值 */
    { 1531U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[218], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1532  电网电压与无功功率曲线中V1值 */
    { 1532U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[219], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1533  电网电压与无功功率曲线中Q1值 */
    { 1533U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[220], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1534  电网电压与无功功率曲线中V4值 */
    { 1534U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[221], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1535  电网电压与无功功率曲线中Q4值 */
    { 1535U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[222], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1536  Q(V)曲线中电压开环响应时间 */
    { 1536U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[223], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1537  Q(P)有功功率调整无功功率使能 */
    { 1537U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[224], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1538  Q(P)曲线中最大的输出有功功率 */
    { 1538U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[225], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1539  Q(P)曲线中中等大的输出有功功率 */
    { 1539U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[226], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1540  Q(P)曲线中最小的输出有功功率 */
    { 1540U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[227], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1541  Q(P)曲线中最小的负载有功功率 */
    { 1541U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[228], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1542  Q(P)曲线中中等大的负载有功功率 */
    { 1542U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[229], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1543  Q(P)曲线中最大的负载有功功率 */
    { 1543U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[230], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1544  Q(P)曲线中最大的输出无功功率 */
    { 1544U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[231], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1545  Q(P)曲线中中等大的输出无功功率 */
    { 1545U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[232], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1546  Q(P)曲线中最小的输出无功功率 */
    { 1546U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[233], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1547  Q(P)曲线中最大的负载无功功率 */
    { 1547U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[234], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1548  Q(P)曲线中中等大的负载无功功率 */
    { 1548U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[235], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1549  Q(P)曲线中最小的负载无功功率 */
    { 1549U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[236], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1550  Q(P)开环响应时间最小值 */
    { 1550U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[237], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1551  Q(P)开环响应时间最大值 */
    { 1551U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[238], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1552  P(V)模式使能 */
    { 1552U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[239], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1553  P(V)曲线中功率降低时的启动电压V1上限 */
    { 1553U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[240], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1554  P(V)曲线中功率降低时的启动电压V1上限值对应 */
    { 1554U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[241], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1555  P(V)曲线中功率降低时的上限电压V2 */
    { 1555U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[242], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1556  P(V)曲线中V2点对应的输出有功功率P2值 */
    { 1556U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[243], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1557  P(V)曲线中V2点对应的负载有功功率P2值 */
    { 1557U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[244], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 1558  P(V)曲线的开环响应时间 */
    { 1558U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[245], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1559  过压保护点2 */
    { 1559U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[246], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1560  过压保护点2持续的时间 */
    { 1560U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[247], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1561  过压保护点1 */
    { 1561U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[248], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1562  过压保护点1持续的时间 */
    { 1562U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[249], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1563  欠压保护点1 */
    { 1563U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[250], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1564  欠压保护点1持续的时间 */
    { 1564U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[251], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1565  欠压保护点2 */
    { 1565U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[252], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1566  欠压保护点2持续的时间 */
    { 1566U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[253], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1567  过频保护点2 */
    { 1567U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[254], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1568  过频保护点2持续时间 */
    { 1568U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[255], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1569  过频保护点1 */
    { 1569U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[256], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1570  过频保护点1持续时间 */
    { 1570U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[257], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1571  欠频保护点1 */
    { 1571U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[258], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1572  欠频保护点1持续时间 */
    { 1572U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[259], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1573  欠频保护点2 */
    { 1573U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[260], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1574  欠频保护点2持续时间 */
    { 1574U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[261], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1575  P(F)模式使能 */
    { 1575U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[262], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1576  P(F)过频偏移量 */
    { 1576U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[263], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1577  P(F)欠频偏移量 */
    { 1577U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[264], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1578  P(F)过频偏移量系数 */
    { 1578U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[265], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1579  P(F)欠频偏移量系数 */
    { 1579U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[266], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1580  P(F)曲线开环响应时间 */
    { 1580U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[267], 0UL, 65535UL, 0.0F, 0.0F },
    /* 1581  非计划性孤岛现象 */
    { 1581U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[268], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2000  协议表格索引号 */
    { 2000U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[269], 1UL, 65535UL, 0.0F, 0.0F },
    /* 2001  变量1 */
    { 2001U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[270], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2002  变量2 */
    { 2002U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[271], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2003  变量3 */
    { 2003U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[272], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2004  变量4 */
    { 2004U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[273], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2005  变量5 */
    { 2005U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[274], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2006  变量6 */
    { 2006U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[275], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2007  变量7 */
    { 2007U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[276], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2008  变量8 */
    { 2008U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[277], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2009  变量9 */
    { 2009U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[278], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2010  变量10 */
    { 2010U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[279], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2011  变量11 */
    { 2011U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[280], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2012  变量12 */
    { 2012U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[281], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2013  变量13 */
    { 2013U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[282], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2014  变量14 */
    { 2014U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[283], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2015  变量15 */
    { 2015U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[284], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2016  变量16 */
    { 2016U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[285], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2017  变量17 */
    { 2017U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[286], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2018  变量18 */
    { 2018U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[287], 0UL, 65535UL, 0.0F, 0.0F },
    /* 2511  变量511 */
    { 2511U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[288], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3000  高级功能使能 */
    { 3000U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[289], 0UL, 1UL, 0.0F, 0.0F },
    /* 3001  国家索引号 */
    { 3001U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[290], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3002  电网欠压LEVEL1保护值 */
    { 3002U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[291], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3003  电网欠压LEVEL1触发时间 */
    { 3003U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[292], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3004  电网欠压LEVEL2保护值 */
    { 3004U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[293], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3005  电网欠压LEVEL2触发时间 */
    { 3005U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[294], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3006  超前和滞后功率因数设置 */
    { 3006U, MB_PT_I16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[295], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 3007  区域设置 */
    { 3007U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[296], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3008  模式使能 */
    { 3008U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[297], 0UL, 1UL, 0.0F, 0.0F },
    /* 3009  每分钟功率变化比例 */
    { 3009U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[298], 0UL, 100UL, 0.0F, 0.0F },
    /* 3010  均值过压保护值 */
    { 3010U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[299], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3011  电网高压LEVEL1保护值 */
    { 3011U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[300], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3012  电网高压LEVEL1保护时间 */
    { 3012U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[301], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3013  电网高压LEVEL2保护值 */
    { 3013U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[302], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3014  电网高压LEVEL2保护时间 */
    { 3014U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[303], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3015  无功占额定有功比例 */
    { 3015U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[304], 0UL, 100UL, 0.0F, 0.0F },
    /* 3016  Volt–watt响应模式的参数Vw1设置 */
    { 3016U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[305], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3017  Volt–watt响应模式的参数Vw2设置 */
    { 3017U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[306], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3018  保留 */
    { 3018U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[307], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3019  电网欠频LEVEL1保护值 */
    { 3019U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[308], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3020  电网欠频LEVEL1保护时间 */
    { 3020U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[309], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3021  电网欠频LEVEL2保护值 */
    { 3021U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[310], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3022  电网欠频LEVEL2保护时间 */
    { 3022U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[311], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3023  V-var and volt-watt响应模式的 */
    { 3023U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[312], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3024  V-var and volt-watt响应模式的 */
    { 3024U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[313], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3025  V-var and volt-watt响应模式的 */
    { 3025U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[314], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3026  V-var and volt-watt响应模式的 */
    { 3026U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[315], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3027  电网过频LEVEL1保护值 */
    { 3027U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[316], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3028  电网过频LEVEL1保护时间 */
    { 3028U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[317], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3029  电网过频LEVEL2保护值 */
    { 3029U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[318], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3030  电网过频LEVEL2保护时间 */
    { 3030U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[319], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3031  保留 */
    { 3031U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[320], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3032  保留 */
    { 3032U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[321], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3033  保留 */
    { 3033U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[322], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3034  保留 */
    { 3034U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[323], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3035  电网重连电压下限值 */
    { 3035U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[324], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3036  电网重连电压上限值 */
    { 3036U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[325], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3037  电网重连频率下限值 */
    { 3037U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[326], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3038  电网重连频率上限值 */
    { 3038U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[327], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3039  电网异常重连时间 */
    { 3039U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[328], 0UL, 65535UL, 0.0F, 0.0F },
    /* 3040  恢复默认 */
    { 3040U, MB_PT_U16, MB_PT_READWRITE, MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[329], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5000  状态使能设置 */
    { 5000U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[330], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5001  L1相逆变器工作状态 */
    { 5001U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[331], 0UL, 3UL, 0.0F, 0.0F },
    /* 5002  L1相逆变器电压 */
    { 5002U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[332], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5003  L1相逆变器电流 */
    { 5003U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[333], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5004  L1相逆变器功率 */
    { 5004U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[334], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5005  L1相负载电流 */
    { 5005U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[335], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5006  L1相负载功率 */
    { 5006U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[336], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5007  L1相电网电压 */
    { 5007U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[337], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5008  L1相电网电流 */
    { 5008U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[338], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5009  L1相电网功率 */
    { 5009U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[339], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5010  L2相逆变器工作状态 */
    { 5010U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[340], 0UL, 3UL, 0.0F, 0.0F },
    /* 5011  L2相逆变器电压 */
    { 5011U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[341], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5012  L2相逆变器电流 */
    { 5012U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[342], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5013  L2相逆变器功率 */
    { 5013U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[343], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5014  L2相负载电流 */
    { 5014U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[344], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5015  L2相负载功率 */
    { 5015U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[345], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5016  L2相电网电压 */
    { 5016U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[346], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5017  L2相电网电流 */
    { 5017U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[347], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5018  L2相电网功率 */
    { 5018U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[348], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5019  预留 */
    { 5019U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[349], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5020  预留 */
    { 5020U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[350], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5021  预留 */
    { 5021U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[351], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5022  预留 */
    { 5022U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[352], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5023  预留 */
    { 5023U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[353], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5024  预留 */
    { 5024U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[354], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5025  预留 */
    { 5025U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[355], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5026  预留 */
    { 5026U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[356], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5027  预留 */
    { 5027U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[357], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5028  逆变器频率 */
    { 5028U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[358], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5029  电网频率 */
    { 5029U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[359], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5030  PCS故障信息1 */
    { 5030U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[360], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5031  PCS故障信息2 */
    { 5031U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[361], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5032  PCS故障信息3 */
    { 5032U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[362], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5033  PCS故障信息4 */
    { 5033U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[363], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5034  PCS告警信息1 */
    { 5034U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[364], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5035  DSP版本号-H */
    { 5035U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[365], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5036  DSP版本号-L */
    { 5036U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[366], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5037  总母线电压 */
    { 5037U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[367], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5038  正母线电压 */
    { 5038U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[368], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5039  L1相允许最大功率输出 */
    { 5039U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[369], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5040  L2相允许最大功率输出 */
    { 5040U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[370], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5041  负母线电压 */
    { 5041U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[371], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5042  PV1输入电压 */
    { 5042U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[372], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5043  PV1输出电流 */
    { 5043U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[373], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5044  PV1输出功率 */
    { 5044U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[374], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5045  PV2输入电压 */
    { 5045U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[375], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5046  PV2输出电流 */
    { 5046U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[376], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5047  PV2输出功率 */
    { 5047U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[377], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5048  PV3输入电压 */
    { 5048U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[378], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5049  PV3输出电流 */
    { 5049U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[379], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5050  PV3输出功率 */
    { 5050U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[380], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5051  PV4输入电压 */
    { 5051U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[381], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5052  PV4输出电流 */
    { 5052U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[382], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5053  PV4输出功率 */
    { 5053U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[383], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5054  PCS故障信息5 */
    { 5054U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[384], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5055  逆变器并机数量 */
    { 5055U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[385], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5056  BUS目标电压 */
    { 5056U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[386], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5057  逆变L1相负载电压 */
    { 5057U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[387], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5058  逆变L2相负载电压 */
    { 5058U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[388], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5059  预留 */
    { 5059U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[389], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5060  DSP控制位 */
    { 5060U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[390], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5061  L1中点电压 */
    { 5061U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[391], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5062  L2中点电压 */
    { 5062U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[392], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5063  预留 */
    { 5063U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[393], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5064  PV绝缘电阻 */
    { 5064U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[394], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5065  GFCI漏电流 */
    { 5065U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[395], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5500  逆变器硬件版本号低字节 */
    { 5500U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[396], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5501  逆变器硬件版本号高字节 */
    { 5501U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[397], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5502  有功功率 */
    { 5502U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[398], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5503  无功功率 */
    { 5503U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[399], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 5504  频率 */
    { 5504U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[400], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5505  操作状态 */
    { 5505U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[401], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5506  连接状态 */
    { 5506U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[402], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5507  告警状态 */
    { 5507U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[403], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5508  逆变器状态 */
    { 5508U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[404], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5509  控制的状态 */
    { 5509U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[405], 0UL, 65535UL, 0.0F, 0.0F },
    /* 5510  电池状态 */
    { 5510U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[406], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8000  逆变L1电压 */
    { 8000U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[407], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8001  逆变L2电压 */
    { 8001U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[408], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8002  预留 */
    { 8002U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[409], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8003  逆变L1相电流 */
    { 8003U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[410], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 8004  逆变L2相电流 */
    { 8004U, MB_PT_I16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[411], (Uint32)(-32768L), 32767UL, 0.0F, 0.0F },
    /* 8005  预留 */
    { 8005U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[412], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8006  电网L1电压 */
    { 8006U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[413], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8007  电网L2电压 */
    { 8007U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[414], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8008  预留 */
    { 8008U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[415], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8009  负载L1相电流 */
    { 8009U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[416], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8010  负载L2相电流 */
    { 8010U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[417], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8011  预留 */
    { 8011U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[418], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8012  PV1电压 */
    { 8012U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[419], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8013  PV2电压 */
    { 8013U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[420], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8014  PV3电压 */
    { 8014U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[421], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8015  PV4电压 */
    { 8015U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[422], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8016  PV1电流 */
    { 8016U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[423], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8017  PV2电流 */
    { 8017U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[424], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8018  PV3电流 */
    { 8018U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[425], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8019  PV4电流 */
    { 8019U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[426], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8020  PV1绝缘电阻 */
    { 8020U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[427], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8021  PV2绝缘电阻 */
    { 8021U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[428], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8022  PV3绝缘电阻 */
    { 8022U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[429], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8023  PV4绝缘电阻 */
    { 8023U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[430], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8024  BUS1电压（正BUS电压） */
    { 8024U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[431], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8025  BUS2电压（负BUS电压） */
    { 8025U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[432], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8026  BUS3电压 */
    { 8026U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[433], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8027  LLC1电压 */
    { 8027U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[434], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8028  LLC2电压 */
    { 8028U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[435], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8029  LLC3电压 */
    { 8029U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[436], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8030  GFCI漏电流 */
    { 8030U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[437], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8031  双向DC1（BUCK/BOOST）电流 */
    { 8031U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[438], 0UL, 65535UL, 0.0F, 0.0F },
    /* 8032  双向DC2（BUCK/BOOST）电流 */
    { 8032U, MB_PT_U16, MB_PT_READ     , MB_PT_NOT_SAVE, (void *)&s_au16ProtoRegs[439], 0UL, 65535UL, 0.0F, 0.0F }
};

Uint16 *AppProtoRegBase(void)
{
    return &s_au16ProtoRegs[0];
}

Uint16 AppProtoRegCount(void)
{
    return (Uint16)APP_PROTO_REG_COUNT;
}

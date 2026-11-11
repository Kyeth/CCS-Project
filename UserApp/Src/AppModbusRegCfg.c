/*
 * AppModbusRegCfg.c
 * Modbus 寄存器表（按 ARM/DSP 协议表生成，共 440 项）。
 * @author yetingheng (004182)
 * @version 0.1
 * @date 2026-10-22
 *
 * @copyright Copyright (c) 2026
 */

#include "AppModbusRegCfg.h"

/* 占位存储：参考层要求每个寄存器绑定一个 U_REG_VAL；本工程尚未接入业务
 * 变量，先按项占位。每项都是真正的 U_REG_VAL，不能把 Uint16 数组的元素地址
 * 强转成 U_REG_VAL *——联合体里含 32 位与 float 成员，强转会按更宽的对齐要求
 * 访问，主机上第二个元素就越界
 * 放进 .mbregs（RAMGS1），让可写的 RAM 与 Flash 里的表分离。 */
static U_REG_VAL s_auRegVal[APP_MODBUS_REG_COUNT];
static U_REG_VAL s_stRegValC[ATS_MODBUS_REG_COUNT];
static U_REG_VAL s_stRegValD[MACHINE_MODBUS_REG_COUNT];
#ifdef __TMS320C28XX__
#pragma DATA_SECTION(s_auRegVal, ".mbregs")
#pragma DATA_SECTION(s_stRegValC, ".mbregs")
#pragma DATA_SECTION(s_stRegValD, ".mbregs")
#endif
// 点表1 ARM通信协议维护表 所有与ARM通信的应用数据在下表维护
const ST_REG_SLAVE_ITEM g_astAppRegSlaveItem[APP_MODBUS_REG_COUNT] = {
    /*     0  L1相直流分量 */
    {     0U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  0],       0,  65535U },
    /*     1  L2相直流分量 */
    {     1U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  1],       0,  65535U },
    /*     2  预留 */
    {     2U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  2],       0,  65535U },
    /*     3  L1相目标有功功率 */
    {     3U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  3],       0,  65535U },
    /*     4  L2相目标有功功率 */
    {     4U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  4],       0,  65535U },
    /*     5  预留 */
    {     5U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  5],       0,  65535U },
    /*     6  L1相馈网功率与负载功率之和 */
    {     6U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  6],       0,  65535U },
    /*     7  L2相馈网功率与负载功率之和 */
    {     7U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  7],       0,  65535U },
    /*     8  预留 */
    {     8U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  8],       0,  65535U },
    /*     9  BMS允许充电电压 */
    {     9U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[  9],       0,  65535U },
    /*    10  BMS允许充电功率 */
    {    10U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 10],       0,  65535U },
    /*    11  BMS允许放电功率 */
    {    11U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 11],       0,  65535U },
    /*    12  系统开关机设置 */
    {    12U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 12],       0,  65535U },
    /*    13  恢复出厂设置 */
    {    13U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 13],       0,  65535U },
    /*    14  频率设置 */
    {    14U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 14],       0,     50U },
    /*    15  光伏目标功率 */
    {    15U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 15],       0,  65535U },
    /*    16  电网最大充电电流设置 */
    {    16U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 16],       0,  65535U },
    /*    17  电网最大放电电流设置 */
    {    17U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 17],       0,  65535U },
    /*    18  ARM状态字 */
    {    18U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 18],       0,  65535U },
    /*    19  ARM使能状态 */
    {    19U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 19],       0,  65535U },
    /*    20  PV并联使能设置 */
    {    20U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 20],       0,  65535U },
    /*    21  L1相负载电压 */
    {    21U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 21],       0,  65535U },
    /*    22  L2相负载电压 */
    {    22U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 22],       0,  65535U },
    /*    23  预留 */
    {    23U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 23],       0,  65535U },
    /*    24  风扇档位控制 */
    {    24U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 24],       0,      0U },
    /*    25  电压设置 */
    {    25U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 25],       0,  65535U },
    /*    26  地区设置 */
    {    26U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 26],       0,  65535U },
    /*    27  L1相电网视在功率 */
    {    27U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 27],       0,  65535U },
    /*    28  L2相电网视在功率 */
    {    28U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 28],       0,  65535U },
    /*    29  预留 */
    {    29U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 29],       0,  65535U },
    /*    30  PV温度限功率 */
    {    30U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 30],       0,  65535U },
    /*    31  PV3目标功率 */
    {    31U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 31],       0,   7000U },
    /*    32  电网增强模式设置 */
    {    32U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 32],       0,  65535U },
    /*    33  PV4目标功率 */
    {    33U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 33],       0,  65535U },
    /*    34  L1相并网继电器中点电压 */
    {    34U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 34],       0,  65535U },
    /*    35  老化标签 */
    {    35U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 35],       0,  65535U },
    /*    36  逆变器并/离网控制 */
    {    36U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 36],       0,  65535U },
    /*    37  光伏输入源设置 */
    {    37U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 37],       0,  65535U },
    /*    38  PV1目标功率 */
    {    38U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 38],       0,   7000U },
    /*    39  PV2目标功率 */
    {    39U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 39],       0,   7000U },
    /*    40  L2相并网继电器中点电压 */
    {    40U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 40],       0,  65535U },
    /*    41  逆变开环使能 */
    {    41U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 41],       0,  65535U },
    /*    42  BMS紧急充电 */
    {    42U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 42],       0,  65535U },
    /*    43  电池包运行状态 */
    {    43U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 43],       0,  65535U },
    /*    44  PV1电流限制值 */
    {    44U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 44],       0,  65535U },
    /*    45  PV2电流限制值 */
    {    45U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 45],       0,  65535U },
    /*    46  PV3电流限制值 */
    {    46U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 46],       0,  65535U },
    /*    47  PV4电流限制值 */
    {    47U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 47],       0,  65535U },
    /*    48  BUS稳压电压 */
    {    48U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 48],       0,  65535U },
    /*    49  SN码 Bit0-Bit15 */
    {    49U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 49],       0,  65535U },
    /*    50  SN码 Bit16-Bit31 */
    {    50U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 50],       0,  65535U },
    /*    51  SN码 Bit32-Bit47 */
    {    51U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 51],       0,  65535U },
    /*    52  SN码 Bit48-Bit63 */
    {    52U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 52],       0,  65535U },
    /*    53  状态使能 */
    {    53U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 53],       0,  65535U },
    /*    54  GFCI标志位 */
    {    54U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 54],       0,  65535U },
    /*    55  电池包SOC */
    {    55U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 55],       0,  65535U },
    /*    56  BMS加热请求 */
    {    56U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 56],       0,  65535U },
    /*    57  BMS母线电压 */
    {    57U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 57],       0,  65535U },
    /*   500  模式使能 */
    {   500U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 58],       0,  65535U },
    /*   501  恒功率因数设置 */
    {   501U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 59],  -32768,  32767U },
    /*   502  恒功率因数超前/滞后的设置 */
    {   502U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 60],       0,  65535U },
    /*   503  基准电压 */
    {   503U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 61],       0,  65535U },
    /*   504  自动调整基准电压使能 */
    {   504U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 62],       0,  65535U },
    /*   505  调整基准电压恒定的时间 */
    {   505U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 63],       0,  65535U },
    /*   506  电压无功V1 */
    {   506U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 64],       0,  65535U },
    /*   507  电压无功V2 */
    {   507U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 65],       0,  65535U },
    /*   508  电压无功V3 */
    {   508U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 66],       0,  65535U },
    /*   509  电压无功V4 */
    {   509U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 67],       0,  65535U },
    /*   510  电压无功Q1 */
    {   510U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 68],  -32768,  32767U },
    /*   511  电压无功Q2 */
    {   511U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 69],  -32768,  32767U },
    /*   512  电压无功Q3 */
    {   512U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 70],  -32768,  32767U },
    /*   513  电压无功Q4 */
    {   513U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 71],  -32768,  32767U },
    /*   514  开环响应时间 */
    {   514U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 72],       0,  65535U },
    /*   515  有功无功P1 */
    {   515U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 73],  -32768,  32767U },
    /*   516  有功无功P2 */
    {   516U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 74],  -32768,  32767U },
    /*   517  有功无功P3 */
    {   517U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 75],  -32768,  32767U },
    /*   518  有功无功P1(取反) */
    {   518U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 76],  -32768,  32767U },
    /*   519  有功无功P2(取反) */
    {   519U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 77],  -32768,  32767U },
    /*   520  有功无功P3(取反) */
    {   520U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 78],  -32768,  32767U },
    /*   521  有功无功Q1 */
    {   521U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 79],  -32768,  32767U },
    /*   522  有功无功Q2 */
    {   522U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 80],  -32768,  32767U },
    /*   523  有功无功Q3 */
    {   523U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 81],  -32768,  32767U },
    /*   524  有功无功Q1(取反) */
    {   524U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 82],  -32768,  32767U },
    /*   525  有功无功Q2(取反) */
    {   525U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 83],  -32768,  32767U },
    /*   526  有功无功Q3(取反) */
    {   526U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 84],  -32768,  32767U },
    /*   527  恒定的无功功率 */
    {   527U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 85],  -32768,  32767U },
    /*   528  电压有功V1 */
    {   528U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 86],       0,  65535U },
    /*   529  电压有功V2 */
    {   529U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 87],       0,  65535U },
    /*   530  电压有功P1 */
    {   530U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 88],  -32768,  32767U },
    /*   531  电压有功P2 */
    {   531U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[ 89],  -32768,  32767U },
    /*   532  开环响应时间2 */
    {   532U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 90],       0,  65535U },
    /*   533  过压保护点1 */
    {   533U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 91],       0,  65535U },
    /*   534  过压保护点2 */
    {   534U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 92],       0,  65535U },
    /*   535  过压延迟保护时间1 */
    {   535U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 93],       0,  65535U },
    /*   536  过压延迟保护时间2 */
    {   536U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 94],       0,  65535U },
    /*   537  欠压保护点1 */
    {   537U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 95],       0,  65535U },
    /*   538  欠压保护点2 */
    {   538U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 96],       0,  65535U },
    /*   539  欠压延迟保护时间1 */
    {   539U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 97],       0,  65535U },
    /*   540  欠压延迟保护时间2 */
    {   540U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 98],       0,  65535U },
    /*   541  过频保护点1 */
    {   541U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[ 99],       0,  65535U },
    /*   542  过频保护点2 */
    {   542U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[100],       0,  65535U },
    /*   543  过频延迟保护时间1低字节 */
    {   543U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[101],       0,  65535U },
    /*   544  过频延迟保护时间1高字节 */
    {   544U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[102],       0,  65535U },
    /*   545  过频延迟保护时间2低字节 */
    {   545U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[103],       0,  65535U },
    /*   546  过频延迟保护时间2高字节 */
    {   546U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[104],       0,  65535U },
    /*   547  欠频保护1 */
    {   547U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[105],       0,  65535U },
    /*   548  欠频保护2 */
    {   548U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[106],       0,  65535U },
    /*   549  欠频延迟保护时间1低字节 */
    {   549U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[107],       0,  65535U },
    /*   550  欠频延迟保护时间1高字节 */
    {   550U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[108],       0,  65535U },
    /*   551  欠频延迟保护时间2低字节 */
    {   551U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[109],       0,  65535U },
    /*   552  欠频延迟保护时间2高字节 */
    {   552U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[110],       0,  65535U },
    /*   553  过频正偏移值DBOF */
    {   553U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[111],       0,  65535U },
    /*   554  过频负偏移值DBUF */
    {   554U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[112],       0,  65535U },
    /*   555  过频正偏移值系数KOF */
    {   555U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[113],       0,  65535U },
    /*   556  过频负偏移值系数KUF */
    {   556U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[114],       0,  65535U },
    /*   557  开环响应时间3 */
    {   557U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[115],       0,  65535U },
    /*   558  并网使能 */
    {   558U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[116],       0,  65535U },
    /*   559  允许并网的最大电网电压 */
    {   559U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[117],       0,  65535U },
    /*   560  允许并网的最小电网电压 */
    {   560U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[118],       0,  65535U },
    /*   561  允许并网的最大电网频率 */
    {   561U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[119],       0,  65535U },
    /*   562  允许并网的最小电网频率 */
    {   562U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[120],       0,  65535U },
    /*   563  并网延迟时间 */
    {   563U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[121],       0,  65535U },
    /*   564  并网随机延迟时间 */
    {   564U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[122],       0,  65535U },
    /*   565  并网功率爬升时间 */
    {   565U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[123],       0,  65535U },
    /*   566  最大有功功率 */
    {   566U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[124],  -32768,  32767U },
    /*  1000  逆变L1电压采样系数 */
    {  1000U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[125],       0,  65535U },
    /*  1001  逆变L2电压采样系数 */
    {  1001U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[126],       0,  65535U },
    /*  1002  预留 */
    {  1002U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[127],       0,  65535U },
    /*  1003  逆变L1相电流采样系数_重载 */
    {  1003U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[128],       0,  65535U },
    /*  1004  逆变L2相电流采样系数_重载 */
    {  1004U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[129],       0,  65535U },
    /*  1005  预留 */
    {  1005U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[130],       0,  65535U },
    /*  1006  电网L1电压采样系数 */
    {  1006U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[131],       0,  65535U },
    /*  1007  电网L2电压采样系数 */
    {  1007U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[132],       0,  65535U },
    /*  1008  预留 */
    {  1008U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[133],       0,  65535U },
    /*  1009  负载L1相电流采样系数 */
    {  1009U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[134],       0,  65535U },
    /*  1010  负载L2相电流采样系数 */
    {  1010U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[135],       0,  65535U },
    /*  1011  预留 */
    {  1011U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[136],       0,  65535U },
    /*  1012  PV1电压采样系数 */
    {  1012U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[137],       0,  65535U },
    /*  1013  PV2电压采样系数 */
    {  1013U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[138],       0,  65535U },
    /*  1014  PV3电压采样系数 */
    {  1014U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[139],       0,  65535U },
    /*  1015  PV4电压采样系数 */
    {  1015U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[140],       0,  65535U },
    /*  1016  PV1电流采样系数 */
    {  1016U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[141],       0,  65535U },
    /*  1017  PV2电流采样系数 */
    {  1017U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[142],       0,  65535U },
    /*  1018  PV3电流采样系数 */
    {  1018U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[143],       0,  65535U },
    /*  1019  PV4电流采样系数 */
    {  1019U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[144],       0,  65535U },
    /*  1020  PV1绝缘电阻采样系数 */
    {  1020U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[145],       0,  65535U },
    /*  1021  PV2绝缘电阻采样系数 */
    {  1021U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[146],       0,  65535U },
    /*  1022  PV3绝缘电阻采样系数 */
    {  1022U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[147],       0,  65535U },
    /*  1023  PV4绝缘电阻采样系数 */
    {  1023U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[148],       0,  65535U },
    /*  1024  BUS1电压采样系数（正BUS电压采样系数） */
    {  1024U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[149],       0,  65535U },
    /*  1025  BUS2电压采样系数（负BUS电压采样系数） */
    {  1025U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[150],       0,  65535U },
    /*  1026  BUS3电压采样系数 */
    {  1026U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[151],       0,  65535U },
    /*  1027  LLC1电压采样系数 */
    {  1027U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[152],       0,  65535U },
    /*  1028  LLC2电压采样系数 */
    {  1028U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[153],       0,  65535U },
    /*  1029  LLC3电压采样系数 */
    {  1029U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[154],       0,  65535U },
    /*  1030  GFCI漏电流采样系数 */
    {  1030U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[155],       0,  65535U },
    /*  1031  双向DC1（BUCK/BOOST）电流采样系数 */
    {  1031U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[156],       0,  65535U },
    /*  1032  双向DC2（BUCK/BOOST）电流采样系数 */
    {  1032U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[157],       0,  65535U },
    /*  1033  逆变L1电流采样系数_轻载 */
    {  1033U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[158],       0,  65535U },
    /*  1034  逆变L2电流采样系数_轻载 */
    {  1034U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[159],       0,  65535U },
    /*  1035  预留 */
    {  1035U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[160],       0,  65535U },
    /*  1036  整流L1电流采样系数_重载 */
    {  1036U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[161],       0,  65535U },
    /*  1037  整流L2电流采样系数_重载 */
    {  1037U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[162],       0,  65535U },
    /*  1038  预留 */
    {  1038U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[163],       0,  65535U },
    /*  1039  整流L1电流采样系数_轻载 */
    {  1039U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[164],       0,  65535U },
    /*  1040  整流L2电流采样系数_轻载 */
    {  1040U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[165],       0,  65535U },
    /*  1041  预留 */
    {  1041U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[166],       0,  65535U },
    /*  1042  逆变L1功率校正系数_重载 */
    {  1042U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[167],       0,  65535U },
    /*  1043  逆变L2功率校正系数_重载 */
    {  1043U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[168],       0,  65535U },
    /*  1044  预留 */
    {  1044U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[169],       0,  65535U },
    /*  1045  逆变L1功率校正系数_轻载 */
    {  1045U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[170],       0,  65535U },
    /*  1046  逆变L2功率校正系数_轻载 */
    {  1046U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[171],       0,  65535U },
    /*  1047  预留 */
    {  1047U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[172],       0,  65535U },
    /*  1048  整流L1功率校正系数_重载 */
    {  1048U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[173],       0,  65535U },
    /*  1049  整流L2功率校正系数_重载 */
    {  1049U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[174],       0,  65535U },
    /*  1050  预留 */
    {  1050U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[175],       0,  65535U },
    /*  1051  整流L1功率校正系数_轻载 */
    {  1051U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[176],       0,  65535U },
    /*  1052  整流L2功率校正系数_轻载 */
    {  1052U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[177],       0,  65535U },
    /*  1053  预留 */
    {  1053U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[178],       0,  65535U },
    /*  1054  逆变L1相控制电压校正系数 */
    {  1054U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[179],       0,  65535U },
    /*  1055  逆变L2相控制电压校正系数 */
    {  1055U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[180],       0,  65535U },
    /*  1056  预留 */
    {  1056U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[181],       0,  65535U },
    /*  1057  逆变L1负载电压校正系数 */
    {  1057U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[182],       0,  65535U },
    /*  1058  逆变L2负载电压校正系数 */
    {  1058U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[183],       0,  65535U },
    /*  1059  预留 */
    {  1059U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[184],       0,  65535U },
    /*  1060  逆变L1中点电压校正系数 */
    {  1060U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[185],       0,  65535U },
    /*  1061  逆变L2中点电压校正系数 */
    {  1061U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[186],       0,  65535U },
    /*  1500  机器的测试类型 */
    {  1500U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[187],       0,  65535U },
    /*  1501  机器的故障执行标准 */
    {  1501U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[188],       0,  65535U },
    /*  1502  孤岛类别 */
    {  1502U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[189],       0,  65535U },
    /*  1503  最大注入无功功率的额定值 */
    {  1503U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[190],       0,  65535U },
    /*  1504  最大吸收无功功率的额定值 */
    {  1504U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[191],       0,  65535U },
    /*  1505  支持的模式 */
    {  1505U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[192],       0,  65535U },
    /*  1506  允许并网模式的选择 */
    {  1506U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[193],       0,  65535U },
    /*  1507  进入电网电压的最小值 */
    {  1507U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[194],       0,  65535U },
    /*  1508  进入电网电压的最大值 */
    {  1508U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[195],       0,  65535U },
    /*  1509  进入电网频率的最小值 */
    {  1509U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[196],       0,  65535U },
    /*  1510  进入电网频率的最大值 */
    {  1510U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[197],       0,  65535U },
    /*  1511  软启随机延迟使能 */
    {  1511U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[198],       0,  65535U },
    /*  1512  软启最小延迟时间 */
    {  1512U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[199],       0,  65535U },
    /*  1513  软启动爬坡时间 */
    {  1513U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[200],       0,  65535U },
    /*  1514  恒定PF值模式使能 */
    {  1514U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[201],       0,  65535U },
    /*  1515  功率因数PF超前/滞后 */
    {  1515U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[202],       0,  65535U },
    /*  1516  恒定的PF值 */
    {  1516U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[203],  -32768,  32767U },
    /*  1517  达到恒功率因数PF的最小开环响应时间 */
    {  1517U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[204],       0,  65535U },
    /*  1518  达到恒功率因数PF的最大开环响应时间 */
    {  1518U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[205],       0,  65535U },
    /*  1519  恒定的无功功率使能 */
    {  1519U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[206],       0,  65535U },
    /*  1520  恒定的无功功率方向选择 */
    {  1520U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[207],       0,  65535U },
    /*  1521  恒定的无功功率值 */
    {  1521U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[208],  -32768,  32767U },
    /*  1522  达到恒定无功功率的最小开环响应时间 */
    {  1522U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[209],       0,  65535U },
    /*  1523  达到恒定无功功率的最大开环响应时间 */
    {  1523U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[210],       0,  65535U },
    /*  1524  电网电压调整无功功率使能 */
    {  1524U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[211],       0,  65535U },
    /*  1525  电网电压调整无功功率时的电压基准 */
    {  1525U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[212],       0,  65535U },
    /*  1526  电网电压自动调整无功功率使能 */
    {  1526U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[213],       0,  65535U },
    /*  1527  电网电压自动调整无功功率的开环响应时间 */
    {  1527U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[214],       0,  65535U },
    /*  1528  电网电压与无功功率曲线中V2值 */
    {  1528U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[215],       0,  65535U },
    /*  1529  电网电压与无功功率曲线中Q2值 */
    {  1529U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[216],  -32768,  32767U },
    /*  1530  电网电压与无功功率曲线中V3值 */
    {  1530U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[217],       0,  65535U },
    /*  1531  电网电压与无功功率曲线中Q3值 */
    {  1531U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[218],  -32768,  32767U },
    /*  1532  电网电压与无功功率曲线中V1值 */
    {  1532U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[219],       0,  65535U },
    /*  1533  电网电压与无功功率曲线中Q1值 */
    {  1533U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[220],       0,  65535U },
    /*  1534  电网电压与无功功率曲线中V4值 */
    {  1534U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[221],       0,  65535U },
    /*  1535  电网电压与无功功率曲线中Q4值 */
    {  1535U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[222],  -32768,  32767U },
    /*  1536  Q(V)曲线中电压开环响应时间 */
    {  1536U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[223],       0,  65535U },
    /*  1537  Q(P)有功功率调整无功功率使能 */
    {  1537U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[224],       0,  65535U },
    /*  1538  Q(P)曲线中最大的输出有功功率 */
    {  1538U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[225],       0,  65535U },
    /*  1539  Q(P)曲线中中等大的输出有功功率 */
    {  1539U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[226],       0,  65535U },
    /*  1540  Q(P)曲线中最小的输出有功功率 */
    {  1540U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[227],       0,  65535U },
    /*  1541  Q(P)曲线中最小的负载有功功率 */
    {  1541U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[228],  -32768,  32767U },
    /*  1542  Q(P)曲线中中等大的负载有功功率 */
    {  1542U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[229],  -32768,  32767U },
    /*  1543  Q(P)曲线中最大的负载有功功率 */
    {  1543U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[230],  -32768,  32767U },
    /*  1544  Q(P)曲线中最大的输出无功功率 */
    {  1544U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[231],  -32768,  32767U },
    /*  1545  Q(P)曲线中中等大的输出无功功率 */
    {  1545U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[232],  -32768,  32767U },
    /*  1546  Q(P)曲线中最小的输出无功功率 */
    {  1546U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[233],  -32768,  32767U },
    /*  1547  Q(P)曲线中最大的负载无功功率 */
    {  1547U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[234],  -32768,  32767U },
    /*  1548  Q(P)曲线中中等大的负载无功功率 */
    {  1548U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[235],  -32768,  32767U },
    /*  1549  Q(P)曲线中最小的负载无功功率 */
    {  1549U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[236],  -32768,  32767U },
    /*  1550  Q(P)开环响应时间最小值 */
    {  1550U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[237],       0,  65535U },
    /*  1551  Q(P)开环响应时间最大值 */
    {  1551U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[238],       0,  65535U },
    /*  1552  P(V)模式使能 */
    {  1552U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[239],       0,  65535U },
    /*  1553  P(V)曲线中功率降低时的启动电压V1上限 */
    {  1553U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[240],       0,  65535U },
    /*  1554  P(V)曲线中功率降低时的启动电压V1上限值对应的P1有功功率 */
    {  1554U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[241],       0,  65535U },
    /*  1555  P(V)曲线中功率降低时的上限电压V2 */
    {  1555U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[242],       0,  65535U },
    /*  1556  P(V)曲线中V2点对应的输出有功功率P2值 */
    {  1556U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[243],       0,  65535U },
    /*  1557  P(V)曲线中V2点对应的负载有功功率P2值 */
    {  1557U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[244],  -32768,  32767U },
    /*  1558  P(V)曲线的开环响应时间 */
    {  1558U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[245],       0,  65535U },
    /*  1559  过压保护点2 */
    {  1559U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[246],       0,  65535U },
    /*  1560  过压保护点2持续的时间 */
    {  1560U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[247],       0,  65535U },
    /*  1561  过压保护点1 */
    {  1561U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[248],       0,  65535U },
    /*  1562  过压保护点1持续的时间 */
    {  1562U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[249],       0,  65535U },
    /*  1563  欠压保护点1 */
    {  1563U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[250],       0,  65535U },
    /*  1564  欠压保护点1持续的时间 */
    {  1564U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[251],       0,  65535U },
    /*  1565  欠压保护点2 */
    {  1565U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[252],       0,  65535U },
    /*  1566  欠压保护点2持续的时间 */
    {  1566U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[253],       0,  65535U },
    /*  1567  过频保护点2 */
    {  1567U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[254],       0,  65535U },
    /*  1568  过频保护点2持续时间 */
    {  1568U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[255],       0,  65535U },
    /*  1569  过频保护点1 */
    {  1569U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[256],       0,  65535U },
    /*  1570  过频保护点1持续时间 */
    {  1570U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[257],       0,  65535U },
    /*  1571  欠频保护点1 */
    {  1571U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[258],       0,  65535U },
    /*  1572  欠频保护点1持续时间 */
    {  1572U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[259],       0,  65535U },
    /*  1573  欠频保护点2 */
    {  1573U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[260],       0,  65535U },
    /*  1574  欠频保护点2持续时间 */
    {  1574U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[261],       0,  65535U },
    /*  1575  P(F)模式使能 */
    {  1575U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[262],       0,  65535U },
    /*  1576  P(F)过频偏移量 */
    {  1576U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[263],       0,  65535U },
    /*  1577  P(F)欠频偏移量 */
    {  1577U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[264],       0,  65535U },
    /*  1578  P(F)过频偏移量系数 */
    {  1578U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[265],       0,  65535U },
    /*  1579  P(F)欠频偏移量系数 */
    {  1579U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[266],       0,  65535U },
    /*  1580  P(F)曲线开环响应时间 */
    {  1580U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[267],       0,  65535U },
    /*  1581  非计划性孤岛现象 */
    {  1581U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[268],       0,  65535U },
    /*  2000  协议表格索引号 */
    {  2000U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[269],       1,  65535U },
    /*  2001  变量1 */
    {  2001U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[270],       0,  65535U },
    /*  2002  变量2 */
    {  2002U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[271],       0,  65535U },
    /*  2003  变量3 */
    {  2003U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[272],       0,  65535U },
    /*  2004  变量4 */
    {  2004U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[273],       0,  65535U },
    /*  2005  变量5 */
    {  2005U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[274],       0,  65535U },
    /*  2006  变量6 */
    {  2006U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[275],       0,  65535U },
    /*  2007  变量7 */
    {  2007U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[276],       0,  65535U },
    /*  2008  变量8 */
    {  2008U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[277],       0,  65535U },
    /*  2009  变量9 */
    {  2009U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[278],       0,  65535U },
    /*  2010  变量10 */
    {  2010U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[279],       0,  65535U },
    /*  2011  变量11 */
    {  2011U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[280],       0,  65535U },
    /*  2012  变量12 */
    {  2012U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[281],       0,  65535U },
    /*  2013  变量13 */
    {  2013U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[282],       0,  65535U },
    /*  2014  变量14 */
    {  2014U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[283],       0,  65535U },
    /*  2015  变量15 */
    {  2015U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[284],       0,  65535U },
    /*  2016  变量16 */
    {  2016U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[285],       0,  65535U },
    /*  2017  变量17 */
    {  2017U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[286],       0,  65535U },
    /*  2018  变量18 */
    {  2018U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[287],       0,  65535U },
    /*  2511  变量511 */
    {  2511U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[288],       0,  65535U },
    /*  3000  高级功能使能 */
    {  3000U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[289],       0,      1U },
    /*  3001  国家索引号 */
    {  3001U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[290],       0,  65535U },
    /*  3002  电网欠压LEVEL1保护值 */
    {  3002U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[291],       0,  65535U },
    /*  3003  电网欠压LEVEL1触发时间 */
    {  3003U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[292],       0,  65535U },
    /*  3004  电网欠压LEVEL2保护值 */
    {  3004U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[293],       0,  65535U },
    /*  3005  电网欠压LEVEL2触发时间 */
    {  3005U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[294],       0,  65535U },
    /*  3006  超前和滞后功率因数设置 */
    {  3006U, READ_WRITE, TYPE_INT16 , NOT_SAVE, &s_auRegVal[295],  -32768,  32767U },
    /*  3007  区域设置 */
    {  3007U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[296],       0,  65535U },
    /*  3008  模式使能 */
    {  3008U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[297],       0,      1U },
    /*  3009  每分钟功率变化比例 */
    {  3009U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[298],       0,    100U },
    /*  3010  均值过压保护值 */
    {  3010U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[299],       0,  65535U },
    /*  3011  电网高压LEVEL1保护值 */
    {  3011U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[300],       0,  65535U },
    /*  3012  电网高压LEVEL1保护时间 */
    {  3012U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[301],       0,  65535U },
    /*  3013  电网高压LEVEL2保护值 */
    {  3013U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[302],       0,  65535U },
    /*  3014  电网高压LEVEL2保护时间 */
    {  3014U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[303],       0,  65535U },
    /*  3015  无功占额定有功比例 */
    {  3015U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[304],       0,    100U },
    /*  3016  Volt–watt响应模式的参数Vw1设置 */
    {  3016U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[305],       0,  65535U },
    /*  3017  Volt–watt响应模式的参数Vw2设置 */
    {  3017U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[306],       0,  65535U },
    /*  3018  保留 */
    {  3018U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[307],       0,  65535U },
    /*  3019  电网欠频LEVEL1保护值 */
    {  3019U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[308],       0,  65535U },
    /*  3020  电网欠频LEVEL1保护时间 */
    {  3020U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[309],       0,  65535U },
    /*  3021  电网欠频LEVEL2保护值 */
    {  3021U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[310],       0,  65535U },
    /*  3022  电网欠频LEVEL2保护时间 */
    {  3022U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[311],       0,  65535U },
    /*  3023  V-var and volt-watt响应模式的参数Vv1设置 */
    {  3023U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[312],       0,  65535U },
    /*  3024  V-var and volt-watt响应模式的参数Vv2设置 */
    {  3024U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[313],       0,  65535U },
    /*  3025  V-var and volt-watt响应模式的参数Vv3设置 */
    {  3025U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[314],       0,  65535U },
    /*  3026  V-var and volt-watt响应模式的参数 Vv4设置 */
    {  3026U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[315],       0,  65535U },
    /*  3027  电网过频LEVEL1保护值 */
    {  3027U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[316],       0,  65535U },
    /*  3028  电网过频LEVEL1保护时间 */
    {  3028U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[317],       0,  65535U },
    /*  3029  电网过频LEVEL2保护值 */
    {  3029U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[318],       0,  65535U },
    /*  3030  电网过频LEVEL2保护时间 */
    {  3030U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[319],       0,  65535U },
    /*  3031  保留 */
    {  3031U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[320],       0,  65535U },
    /*  3032  保留 */
    {  3032U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[321],       0,  65535U },
    /*  3033  保留 */
    {  3033U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[322],       0,  65535U },
    /*  3034  保留 */
    {  3034U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[323],       0,  65535U },
    /*  3035  电网重连电压下限值 */
    {  3035U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[324],       0,  65535U },
    /*  3036  电网重连电压上限值 */
    {  3036U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[325],       0,  65535U },
    /*  3037  电网重连频率下限值 */
    {  3037U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[326],       0,  65535U },
    /*  3038  电网重连频率上限值 */
    {  3038U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[327],       0,  65535U },
    /*  3039  电网异常重连时间 */
    {  3039U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[328],       0,  65535U },
    /*  3040  恢复默认 */
    {  3040U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_auRegVal[329],       0,  65535U },
    /*  5000  状态使能设置 */
    {  5000U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[330],       0,  65535U },
    /*  5001  L1相逆变器工作状态 */
    {  5001U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[331],       0,      3U },
    /*  5002  L1相逆变器电压 */
    {  5002U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[332],       0,  65535U },
    /*  5003  L1相逆变器电流 */
    {  5003U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[333],  -32768,  32767U },
    /*  5004  L1相逆变器功率 */
    {  5004U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[334],  -32768,  32767U },
    /*  5005  L1相负载电流 */
    {  5005U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[335],       0,  65535U },
    /*  5006  L1相负载功率 */
    {  5006U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[336],       0,  65535U },
    /*  5007  L1相电网电压 */
    {  5007U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[337],       0,  65535U },
    /*  5008  L1相电网电流 */
    {  5008U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[338],  -32768,  32767U },
    /*  5009  L1相电网功率 */
    {  5009U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[339],  -32768,  32767U },
    /*  5010  L2相逆变器工作状态 */
    {  5010U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[340],       0,      3U },
    /*  5011  L2相逆变器电压 */
    {  5011U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[341],       0,  65535U },
    /*  5012  L2相逆变器电流 */
    {  5012U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[342],  -32768,  32767U },
    /*  5013  L2相逆变器功率 */
    {  5013U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[343],  -32768,  32767U },
    /*  5014  L2相负载电流 */
    {  5014U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[344],       0,  65535U },
    /*  5015  L2相负载功率 */
    {  5015U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[345],       0,  65535U },
    /*  5016  L2相电网电压 */
    {  5016U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[346],       0,  65535U },
    /*  5017  L2相电网电流 */
    {  5017U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[347],  -32768,  32767U },
    /*  5018  L2相电网功率 */
    {  5018U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[348],  -32768,  32767U },
    /*  5019  预留 */
    {  5019U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[349],       0,  65535U },
    /*  5020  预留 */
    {  5020U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[350],       0,  65535U },
    /*  5021  预留 */
    {  5021U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[351],       0,  65535U },
    /*  5022  预留 */
    {  5022U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[352],       0,  65535U },
    /*  5023  预留 */
    {  5023U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[353],       0,  65535U },
    /*  5024  预留 */
    {  5024U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[354],       0,  65535U },
    /*  5025  预留 */
    {  5025U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[355],       0,  65535U },
    /*  5026  预留 */
    {  5026U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[356],       0,  65535U },
    /*  5027  预留 */
    {  5027U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[357],       0,  65535U },
    /*  5028  逆变器频率 */
    {  5028U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[358],       0,  65535U },
    /*  5029  电网频率 */
    {  5029U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[359],       0,  65535U },
    /*  5030  PCS故障信息1 */
    {  5030U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[360],       0,  65535U },
    /*  5031  PCS故障信息2 */
    {  5031U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[361],       0,  65535U },
    /*  5032  PCS故障信息3 */
    {  5032U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[362],       0,  65535U },
    /*  5033  PCS故障信息4 */
    {  5033U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[363],       0,  65535U },
    /*  5034  PCS告警信息1 */
    {  5034U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[364],       0,  65535U },
    /*  5035  DSP版本号-H */
    {  5035U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[365],       0,  65535U },
    /*  5036  DSP版本号-L */
    {  5036U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[366],       0,  65535U },
    /*  5037  总母线电压 */
    {  5037U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[367],       0,  65535U },
    /*  5038  正母线电压 */
    {  5038U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[368],       0,  65535U },
    /*  5039  L1相允许最大功率输出 */
    {  5039U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[369],       0,  65535U },
    /*  5040  L2相允许最大功率输出 */
    {  5040U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[370],       0,  65535U },
    /*  5041  负母线电压 */
    {  5041U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[371],       0,  65535U },
    /*  5042  PV1输入电压 */
    {  5042U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[372],       0,  65535U },
    /*  5043  PV1输出电流 */
    {  5043U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[373],       0,  65535U },
    /*  5044  PV1输出功率 */
    {  5044U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[374],       0,  65535U },
    /*  5045  PV2输入电压 */
    {  5045U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[375],       0,  65535U },
    /*  5046  PV2输出电流 */
    {  5046U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[376],       0,  65535U },
    /*  5047  PV2输出功率 */
    {  5047U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[377],       0,  65535U },
    /*  5048  PV3输入电压 */
    {  5048U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[378],       0,  65535U },
    /*  5049  PV3输出电流 */
    {  5049U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[379],       0,  65535U },
    /*  5050  PV3输出功率 */
    {  5050U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[380],       0,  65535U },
    /*  5051  PV4输入电压 */
    {  5051U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[381],       0,  65535U },
    /*  5052  PV4输出电流 */
    {  5052U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[382],       0,  65535U },
    /*  5053  PV4输出功率 */
    {  5053U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[383],       0,  65535U },
    /*  5054  PCS故障信息5 */
    {  5054U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[384],       0,  65535U },
    /*  5055  逆变器并机数量 */
    {  5055U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[385],       0,  65535U },
    /*  5056  BUS目标电压 */
    {  5056U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[386],       0,  65535U },
    /*  5057  逆变L1相负载电压 */
    {  5057U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[387],       0,  65535U },
    /*  5058  逆变L2相负载电压 */
    {  5058U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[388],       0,  65535U },
    /*  5059  预留 */
    {  5059U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[389],       0,  65535U },
    /*  5060  DSP控制位 */
    {  5060U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[390],       0,  65535U },
    /*  5061  L1中点电压 */
    {  5061U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[391],       0,  65535U },
    /*  5062  L2中点电压 */
    {  5062U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[392],       0,  65535U },
    /*  5063  预留 */
    {  5063U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[393],       0,  65535U },
    /*  5064  PV绝缘电阻 */
    {  5064U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[394],       0,  65535U },
    /*  5065  GFCI漏电流 */
    {  5065U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[395],       0,  65535U },
    /*  5500  逆变器硬件版本号低字节 */
    {  5500U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[396],       0,  65535U },
    /*  5501  逆变器硬件版本号高字节 */
    {  5501U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[397],       0,  65535U },
    /*  5502  有功功率 */
    {  5502U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[398],  -32768,  32767U },
    /*  5503  无功功率 */
    {  5503U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[399],  -32768,  32767U },
    /*  5504  频率 */
    {  5504U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[400],       0,  65535U },
    /*  5505  操作状态 */
    {  5505U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[401],       0,  65535U },
    /*  5506  连接状态 */
    {  5506U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[402],       0,  65535U },
    /*  5507  告警状态 */
    {  5507U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[403],       0,  65535U },
    /*  5508  逆变器状态 */
    {  5508U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[404],       0,  65535U },
    /*  5509  控制的状态 */
    {  5509U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[405],       0,  65535U },
    /*  5510  电池状态 */
    {  5510U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[406],       0,  65535U },
    /*  8000  逆变L1电压 */
    {  8000U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[407],       0,  65535U },
    /*  8001  逆变L2电压 */
    {  8001U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[408],       0,  65535U },
    /*  8002  预留 */
    {  8002U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[409],       0,  65535U },
    /*  8003  逆变L1相电流 */
    {  8003U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[410],  -32768,  32767U },
    /*  8004  逆变L2相电流 */
    {  8004U, READ_ONLY , TYPE_INT16 , NOT_SAVE, &s_auRegVal[411],  -32768,  32767U },
    /*  8005  预留 */
    {  8005U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[412],       0,  65535U },
    /*  8006  电网L1电压 */
    {  8006U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[413],       0,  65535U },
    /*  8007  电网L2电压 */
    {  8007U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[414],       0,  65535U },
    /*  8008  预留 */
    {  8008U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[415],       0,  65535U },
    /*  8009  负载L1相电流 */
    {  8009U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[416],       0,  65535U },
    /*  8010  负载L2相电流 */
    {  8010U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[417],       0,  65535U },
    /*  8011  预留 */
    {  8011U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[418],       0,  65535U },
    /*  8012  PV1电压 */
    {  8012U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[419],       0,  65535U },
    /*  8013  PV2电压 */
    {  8013U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[420],       0,  65535U },
    /*  8014  PV3电压 */
    {  8014U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[421],       0,  65535U },
    /*  8015  PV4电压 */
    {  8015U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[422],       0,  65535U },
    /*  8016  PV1电流 */
    {  8016U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[423],       0,  65535U },
    /*  8017  PV2电流 */
    {  8017U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[424],       0,  65535U },
    /*  8018  PV3电流 */
    {  8018U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[425],       0,  65535U },
    /*  8019  PV4电流 */
    {  8019U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[426],       0,  65535U },
    /*  8020  PV1绝缘电阻 */
    {  8020U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[427],       0,  65535U },
    /*  8021  PV2绝缘电阻 */
    {  8021U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[428],       0,  65535U },
    /*  8022  PV3绝缘电阻 */
    {  8022U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[429],       0,  65535U },
    /*  8023  PV4绝缘电阻 */
    {  8023U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[430],       0,  65535U },
    /*  8024  BUS1电压（正BUS电压） */
    {  8024U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[431],       0,  65535U },
    /*  8025  BUS2电压（负BUS电压） */
    {  8025U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[432],       0,  65535U },
    /*  8026  BUS3电压 */
    {  8026U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[433],       0,  65535U },
    /*  8027  LLC1电压 */
    {  8027U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[434],       0,  65535U },
    /*  8028  LLC2电压 */
    {  8028U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[435],       0,  65535U },
    /*  8029  LLC3电压 */
    {  8029U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[436],       0,  65535U },
    /*  8030  GFCI漏电流 */
    {  8030U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[437],       0,  65535U },
    /*  8031  双向DC1（BUCK/BOOST）电流 */
    {  8031U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[438],       0,  65535U },
    /*  8032  双向DC2（BUCK/BOOST）电流 */
    {  8032U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[439],       0,  65535U }
};



// 点表2 ATS通信协议维护表 所有与ATS通信的应用数据在下表维护
static const ST_REG_SLAVE_ITEM s_astRegC[] = {
    { 0U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_stRegValC[0], 0, 65535U }
};

// 点表3 自用上位机通信协议维护表 所有与上位机通信的应用数据在下表维护
static const ST_REG_SLAVE_ITEM s_astRegD[] = {
    { 0U, READ_WRITE, TYPE_UINT16, NOT_SAVE, &s_stRegValD[0], 0, 65535U },
    { 1U, READ_WRITE, TYPE_FLOAT, NOT_SAVE, &s_stRegValD[1], 0, 65535U },
    { 3U, READ_WRITE, TYPE_INT16, NOT_SAVE, &s_stRegValD[3], 0, 65535U }
};

const ST_REG_SLAVE_CFG stRegSlaveCfg_App = {
    (ST_REG_SLAVE_ITEM *)&g_astAppRegSlaveItem[0],
    (Uint16)(sizeof(g_astAppRegSlaveItem) / sizeof(g_astAppRegSlaveItem[0]))
};

const ST_REG_SLAVE_CFG stRegSlaveCfg_C = {
    (ST_REG_SLAVE_ITEM *)&s_astRegC[0],
    (Uint16)(sizeof(s_astRegC) / sizeof(s_astRegC[0]))
};

const ST_REG_SLAVE_CFG stRegSlaveCfg_D = {
    (ST_REG_SLAVE_ITEM *)&s_astRegD[0],
    (Uint16)(sizeof(s_astRegD) / sizeof(s_astRegD[0]))
};

void AppRegCfgReset(void)
{
    Uint16 u16Index;

    for (u16Index = 0U; u16Index < APP_MODBUS_REG_COUNT; u16Index++)
    {
        s_auRegVal[u16Index].u32Val = 0UL;
    }

    /* 5035 / 5036：DSP 版本号高/低字，必须先给初值。整个 .mbregs 上电是随机值，
     * 清零之后版本号也会变成 0，主机读版本时拿到 0 而不是既定版本 503900
     * （0x0007B05C）。这两项是指定值，不是业务变量，不能等业务层去写。 */
    s_auRegVal[365].u16Val = 0x0007U;
    s_auRegVal[366].u16Val = 0xB05CU;
    /* 放在 .mbregs 的 NOINIT 段，上电时 C 运行库不会自动清零，所以这里要显式初始化。
     * U_REG_VAL是联合体，用 u32Val = 0UL 可以清掉整个 32 位存储*/
    for (u16Index = 0U; u16Index < ATS_MODBUS_REG_COUNT; u16Index++)
    {
        s_stRegValC[u16Index].u32Val = 0UL;
    }

    for (u16Index = 0U; u16Index < MACHINE_MODBUS_REG_COUNT; u16Index++)
    {
        s_stRegValD[u16Index].u32Val = 0UL;
    }
    /* SCIC 地址 0 为 TYPE_UINT16，联调初值 */
    s_stRegValD[0].u16Val = 1234U;
    s_stRegValD[1].fVal = 0.01F;
    s_stRegValD[3].i16Val = (int16)(-12);
}


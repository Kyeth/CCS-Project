/**
 * @file DrvCfg_EP15K.c
 * @author yetingheng (004182)
 * @brief EP15K板级外设配置
 * @version 0.3
 * @date 2026-10-20
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "DrvCfg_EP15K.h"
#include "User_ClaTask.h"
#include "CommRegTable.h"


ST_GPIO_CFG_ITEM stGpioCfgItem[] =
{
    /* 配置格式：{GPIO编号, 复用功能, 方向, 初始电平, 开漏配置, 上拉配置, 输入限定模式} */
    /* ========== 硬件故障及保护输入 ========== */
    {11, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 内置风扇硬件故障输入。
    {13, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV1硬件过流输入。
    {17, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV2硬件过流输入。
    {35, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // GFCI硬件检测输入。
    {40, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV4硬件过流输入。
    {41, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV3硬件过流输入。
    {53, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 外置ATS标志输入。
    {43, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV1反灌电流检测输入。
    {68, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV2反灌电流检测输入。
    {69, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV3反灌电流检测输入。
    {70, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV4反灌电流检测输入。
    {77, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 平衡电路过流输入。
    {78, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 外置风扇硬件故障输入。
    {80, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // BUS硬件过压输入。
    {81, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 硬件逆变过流输入。
    {82, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 硬件短路过流输入。
    {86, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 12V辅源欠压输入（欠压保护点10.8V）。
    /* ========== 普通GPIO输出 ========== */
    {22, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 内部风扇电平控制开关（1:开 0:关）。
    {33, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 测试引脚（1:高 0:低）。
    {50, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV绝缘检测继电器1（1:闭合 0:断开）。
    {51, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // PV绝缘检测继电器2（1:闭合 0:断开）。
    {52, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 外置ATS申请（1:闭合 0:断开）。
    {58, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // R相逆变端继电器（1:闭合 0:断开）。
    {61, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // S相逆变端继电器（1:闭合 0:断开）。
    {62, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // N相逆变端继电器（1:闭合 0:断开）。
    {63, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // T相逆变端继电器（1:闭合 0:断开）。
    {66, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 接地继电器（1:闭合 0:断开）。
    {46, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 多路选择器地址线A（1:高 0:低）。
    {47, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 多路选择器地址线B（1:高 0:低）。
    {42, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 多路选择器地址线C（1:高 0:低）。
    {71, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 漏电流检测开关（1:闭合 0:断开）。
    {73, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 预充继电器（1:闭合 0:断开）。
    {74, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 市电辅源继电器（1:闭合 0:断开）。
    {75, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // LED灯（1:开 0:关）。
    {87, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 电池激活信号（1:高 0:低）。
    {88, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // 芯片驱动使能（1:开 0:关）。
    /* ========== I2C / EEPROM 引脚 ========== */
    {91, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // I2C SDA — FM24C128D，开漏，初始释放为高。
    {92, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_ENABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // I2C SCL — FM24C128D，开漏，初始释放为高。
    {99, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_LOW,  HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // FM24C128D 写保护 WP（1:保护高半区 0:全片可写）。
    /* ========== ECAP / InputXbar 输入 ========== */
    {25, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP1 — R相电网过零信号（INPUT7）。
    {27, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP2 — S相电网过零信号（INPUT8）。
    {29, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP3 — T相电网过零信号（INPUT9）。
    {30, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP4 — R相电网频率信号（INPUT10）。
    {31, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP5 — S相电网频率信号（INPUT11）。
    {32, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP6 — T相电网频率信号（INPUT12）。
    /* ========== ePWM输出引脚 ========== */
    /* 复用列号规则（按 F2837x TRM GPIO Mux Selection 表逐脚核对）：
     *   GPIO0~GPIO15  : 列1(GMUX=00,MUX=01) = EPWM1A..EPWM8B  -> HW_GPIO_MUX_PWM
     *   GPIO16~GPIO23 : 列5(GMUX=01,MUX=01) = EPWM9A..EPWM12B -> HW_GPIO_FUNC_PERIPH5
     * 原因：GPIO16~23 的列1 被 SPI-A(16/17/18/19)、EQEP1(20/21/22/23) 占满，
     *       选列1 时引脚只是静态电平，表现为「这一路 PWM 完全没有驱动」。
     * 新增任何 ePWM 脚之前，先查表确认它落在哪一列，不要照抄邻居的宏。 */
    {0,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM1A — R竖管S1，INV主同步源。
    {1,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM1B — R横管S3，互补于EPWM1A。
    {2,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM2A — S竖管S1，同步于EPWM1A。
    {3,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM2B — S横管S3，互补于EPWM2A。
    {4,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM3A — T竖管S1，同步于EPWM1A。
    {5,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM3B — T横管S3，互补于EPWM3A。
    {6,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM4A — R竖管S4，同步于EPWM1A。
    {7,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM4B — R横管S2，互补于EPWM4A。
    {8,  HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM5A — PV3 Boost，错相180°。
    {10, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM6A — PV4 Boost，错相270°。
    {12, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM7A — PV1 Boost，PV主同步源。
    {14, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM8A — BUSH，平衡电路上半母线。
    {15, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM8B — BUSL，平衡电路下半母线。
    /* GPIO16 这一脚不能跟全表统一的 HW_GPIO_MUX_PWM(PERIPH1)：
     * 列1 在 GPIO16 上是 SPISIMOA（本设计的 SPI-A 已挪到 GPIO54~57），
     * EPWM9A 落在第 5 列 (GMUX=01,MUX=01) = HW_GPIO_FUNC_PERIPH5，
     * 选错列时该脚输出的是静态电平，表现为"这一路 PWM 完全没有驱动"。
     * 该结论由台架复用位巡游实测确认（候选 5 出 20kHz/30% 波形）。 */
    {16, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM9A — PV2 Boost，错相90°（第5列复用）。
    {18, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM10A — S竖管S4，同步于EPWM1A。
    {19, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM10B — S横管S2，互补于EPWM10A。
    {20, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM11A — T竖管S4，同步于EPWM1A。
    {21, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM11B — T横管S2，互补于EPWM11A。
    {23, HW_GPIO_FUNC_PERIPH5, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM12B — 外置风扇PWM，四档风速。
    /* ========== CAN模块 ========== */
    {36, HW_GPIO_MUX_CAN, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // CAN-A接收信号CANRXA。
    {37, HW_GPIO_MUX_CAN, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // CAN-A发送信号CANTXA。
    {38, HW_GPIO_MUX_CAN, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // CAN-B发送信号CANTXB。
    {39, HW_GPIO_MUX_CAN, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // CAN-B接收信号CANRXB。
    /* ========== SCI模块 ========== */
    {48, HW_GPIO_MUX_SCI, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-A发送信号SCITXDA。
    {49, HW_GPIO_MUX_SCI, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-A接收信号SCIRXDA。
    {90, HW_GPIO_MUX_SCI, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-C发送信号SCITXDC。
    {89, HW_GPIO_MUX_SCI, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-C接收信号SCIRXDC。
    {93, HW_GPIO_MUX_SCI, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-D发送信号SCITXDD。
    {94, HW_GPIO_MUX_SCI, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SCI-D接收信号SCIRXDD。
    /* ========== SPI模块 ========== */
    {54, HW_GPIO_MUX_SPI, HW_DIR_DO, HW_GPIO_LOW,  HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SPI-A主出从入信号SPISIMOA。
    {55, HW_GPIO_MUX_SPI, HW_DIR_DI, HW_GPIO_LOW,  HW_GPIO_OD_ENABLE,  HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SPI-A主入从出信号SPISOMIA。
    {56, HW_GPIO_MUX_SPI, HW_DIR_DO, HW_GPIO_LOW,  HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SPI-A时钟信号SPICLKA。
    {57, HW_GPIO_MUX_GPIO, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // SPI-A片选SPISTE，改软件片选：主机模式下SPISTE每字符自动翻转，多字节帧会误释放从机。
};

ST_GPIO_CFG stGpioCfg =
{
    stGpioCfgItem,
    sizeof(stGpioCfgItem) / sizeof(stGpioCfgItem[0])
};

/******************************************************************************
 * @brief ePWM外设初始化配置表
 *
 * 说明：
 * - INV组 (EPWM1/2/3/4/10/11)：开关频率 20kHz (UPDOWN)，主同步源为 EPWM1，死区 0.8us。
 * - PV组 (EPWM5/6/7/9)：开关频率 20kHz (UP)，主同步源为 EPWM7，通过相位寄存器错相。
 * - BUS (EPWM8)：开关频率 20kHz (UPDOWN)。
 * - FAN (EPWM12)：开关频率 10kHz (UPDOWN)，独立运行。
 ******************************************************************************/

/* ePWM配置格式：{PWM_ID, 类型, 周期, CMPA, CMPB, 相位, 计数模式, 同步模式, 同步计数方向, 同步输入源, 同步输出源, 全局加载, Link PWM, 死区时间, TZ-A动作, TZ-B动作, 初始A强制状态, 初始B强制状态, A动作配置, B动作配置, SOCA配置} */

ST_PWM_CFG_ITEM stPwmCfgItem[] =
{
    /* ==================== 逆变器组 (20kHz UPDOWN, 以 EPWM1 为主同步源) ==================== */

    /* EPWM1: A通道产生逆变R相竖管S1主波形  */
    {
        HW_PWM_ID0, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_MASTER, 1U, HW_EPWM_SYNC_IN_FROM_DISABLE, HW_EPWM_SYNC_OUT_FROM_CNTR_ZERO,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION}, // CAU=SET, CAD=CLEAR
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {1U, ET_CTR_ZERO, 1U, 0U, 1U, 1U, ET_CTR_PRD, 1U, 0U, 1U} // SOCA 在 CTR=ZERO 采平均值，SOCB 在 CTR=PRD 采第二点
    },

    /* EPWM2: S相竖管S1主波形 */
    {
        HW_PWM_ID1, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM3: T相竖管S1主波形 */
    {
        HW_PWM_ID2, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM4: R相竖管S4主波形 */
    {
        HW_PWM_ID3, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM10: S相竖管S4主波形 */
    {
        HW_PWM_ID9, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM11: T相竖管S4主波形 */
    {
        HW_PWM_ID10, HW_PWM_TYPE_INV, 2500U, 2500U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM1, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID0, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* ==================== PV Boost组 (20kHz UP, 以 EPWM7 为主同步源) ==================== */

    /* EPWM7: PV1 Boost, 主同步源，相位 0°，ZRO置高, CAU置低 */
    {
        HW_PWM_ID6, HW_PWM_TYPE_PV, 5000U, 0U, 0U, 0U, TB_COUNT_UP,
        HW_PWM_MASTER, 1U, HW_EPWM_SYNC_IN_FROM_DISABLE, HW_EPWM_SYNC_OUT_FROM_CNTR_ZERO,
        1U, HW_PWM_ID6, 0U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_SET, AQ_NO_ACTION, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION}, // ZRO=SET, CAU=CLEAR
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {1U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM9: PV2 Boost, 错相 90° (5000 * 90/360 = 1250) */
    {
        HW_PWM_ID8, HW_PWM_TYPE_PV, 5000U, 0U, 0U, 1250U, TB_COUNT_UP,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID6, 0U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_SET, AQ_NO_ACTION, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM5: PV3 Boost, 错相 180° (5000 * 180/360 = 2500) */
    {
        HW_PWM_ID4, HW_PWM_TYPE_PV, 5000U, 0U, 0U, 2500U, TB_COUNT_UP,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID6, 0U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_SET, AQ_NO_ACTION, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM6: PV4 Boost, 错相 270° (5000 * 270/360 = 3750) */
    {
        HW_PWM_ID5, HW_PWM_TYPE_PV, 5000U, 0U, 0U, 3750U, TB_COUNT_UP,
        HW_PWM_SLAVE, 1U, HW_EPWM_SYNC_IN_FROM_SYNCOUT_EPWM7, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        1U, HW_PWM_ID6, 0U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_SET, AQ_NO_ACTION, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* ==================== BUS 与 FAN ==================== */

    /* EPWM8: BUS 平衡电路，20kHz 上下计数，死区 0.8us */
    {
        HW_PWM_ID7, HW_PWM_TYPE_BUS, 2500U, 1250U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_FREE, 1U, HW_EPWM_SYNC_IN_FROM_DISABLE, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        0U, HW_PWM_ID7, 80U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_CLEAR, AQ_SET, AQ_NO_ACTION, AQ_NO_ACTION}, // 参考旧代码 CAU=CLEAR, CAD=SET
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    },

    /* EPWM12: FAN 风扇，10kHz 上下计数，无死区 */
    {
        HW_PWM_ID11, HW_PWM_TYPE_FAN, 5000U, 5000U, 0U, 0U, TB_COUNT_UPDOWN,
        HW_PWM_FREE, 1U, HW_EPWM_SYNC_IN_FROM_DISABLE, HW_EPWM_SYNC_OUT_FROM_DISABLE,
        0U, HW_PWM_ID11, 0U,
        HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_FORCE_LOW, HW_PWM_FORCE_LOW,
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_SET, AQ_CLEAR, AQ_NO_ACTION, AQ_NO_ACTION},
        {AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION, AQ_NO_ACTION},
        {0U, ET_CTR_ZERO, 1U, 0U, 1U}
    }
};

ST_PWM_CFG stPwmCfg =
{
    stPwmCfgItem,
    sizeof(stPwmCfgItem) / sizeof(stPwmCfgItem[0])
};

/* ==================== 系统定时器 ==================== */

/* 系统主频，与device.h中DEVICE_SYSCLK_FREQ保持一致（200MHz） */
#define DRVCFG_TIMER_SYSCLK_HZ          (200000000UL)

/* 计数器从PRD递减到0共PRD+1个时钟，所以周期计数值要减1 */
#define DRVCFG_TIMER_PERIOD_100US       ((DRVCFG_TIMER_SYSCLK_HZ / 10000UL) - 1UL)
#define DRVCFG_TIMER_PERIOD_1MS         ((DRVCFG_TIMER_SYSCLK_HZ / 1000UL) - 1UL)

/* 系统节拍定时器：100us，200MHz/10kHz = 20000个时钟，PRD = 19999
 * 该定时器的中断服务函数中调用 SYS_TICK() 驱动调度器 */
volatile ST_SYS_TIMER stSysCpuTimer1 =
{
    (Uint16)HW_CPUTIMER_ID1,        /* u16TimerId     */
    0U,                             /* u16Prescaler   */
    1U,                             /* bEnableInt     */
    1U,                             /* bEmuStop       */
    DRVCFG_TIMER_PERIOD_100US,      /* u32Period      */
    0UL,                            /* u32Tick        */
    0U,                             /* u16TickDivCnt  */
    1U                              /* u16TickDivCoef */
};

/* 辅助定时器：1ms，200MHz/1kHz = 200000个时钟，PRD = 199999
 * 若希望改为"100us硬件中断 + 分频10出1ms心跳"，把周期换成PERIOD_100US、分频系数改成10即可 */
volatile ST_SYS_TIMER stSysCpuTimer2 =
{
    (Uint16)HW_CPUTIMER_ID2,        /* u16TimerId     */
    0U,                             /* u16Prescaler   */
    1U,                             /* bEnableInt     */
    1U,                             /* bEmuStop       */
    DRVCFG_TIMER_PERIOD_1MS,        /* u32Period      */
    0UL,                            /* u32Tick        */
    0U,                             /* u16TickDivCnt  */
    1U                              /* u16TickDivCoef */
};

/******************************************************************************
 * @brief ADC通道及SOC初始化配置表
 *
 * 说明：
 * - 每行对应一个SOC编号（SOC0~SOC15），每行依次配置ADC-A、ADC-B、ADC-C、ADC-D。
 * - 每项格式为{触发源,采样窗口,通道号}，成员顺序与 ST_ADC_CH_CFG_ITEM 一致。
 * - 触发源统一为 EPWM1_SOCA（即数值5，HW_ADC_TRIG_EPWM1_SOCA），由EPWM1在
 *   CTR=ZERO时刻同时触发四路ADC，保证同一时刻各电压电流为同步采样。
 * - 采样窗口 u16Acqps = 采样保持时间(SYSCLK周期数) - 1，例：60表示61个周期。
 *   ADCCLK = SYSCLK/4.5 = 44.4MHz，1个ADCCLK约22.5ns。
 * - {0,0,0} 表示该SOC未使用，驱动会跳过该项，不写寄存器。
 * - 通道号与信号对应关系以《15KW软件管脚整理文档》为准，见下表：
 *
 *   | ADC-A            | ADC-B            | ADC-C             | ADC-D             |
 *   |------------------|------------------|-------------------|-------------------|
 *   | A0 T相逆变电压               | B0 R相直流分量               | C2 PV2电流                       | D0 PV3电流                        |
 *   | A1 S相电网电压               | B1 S相直流分量               | C3 S相逆变电压                 | D1 GFCI电流                      |
 *   | A2 T相逆变电流               | B2 T相直流分量               | C4 R相逆变电压                 | D2 S相逆变电流                  |
 *   | A3 T相电网电压               | B3 多路选择器2      |                   | D3 多路选择器1      |
 *   | A4 PV1电流                     |                  |                   | D4 平衡电路电流                |
 *   | A5 R相电网电压               |                  |                   | D14 R相逆变电流              |
 *   |                  |                  |                   | D15 PV4电流                     |
 *
 * - ADC-A的SOC15位于采样序列末尾，专门用于产生ADCINT1，进而硬件触发CLA Task1。
 *   转换时间会拉长（ACQPS=120），保证ADC-A是四路中最后完成的：
 *   ADC-A 约427个ADCCLK，ADC-B 约304，ADC-D 约337，ADC-C 约153，
 *   故ADCINT1到来时其余三路的全部结果都已就绪。
 * - 多路选择器的两路输出（ADC-B3、ADC-D3）共用GPIO46/47/42选通，采样窗口取
 *   120（约2.7us）以适配分压网络的高内阻。
 ******************************************************************************/

/* ADC通道配置格式：{触发源,采样窗口,通道号} */

const ST_ADC_CH_CFG_ITEM stAdcChCfgItem[] =
{
 /* ADC-A              ADC-B              ADC-C              ADC-D  */
    {5, 30,  2}, {5, 60,  2}, {0, 0,  0}, {5, 30,  2},   /* SOC0  : A2 T相逆变电流   B2 T相直流分量   D2 S相逆变电流 */
    {0, 0,  0}, {5, 60,  0}, {0, 0,  0}, {5, 30, 14},   /* SOC1  :                        B0 R相直流分量   D14 R相逆变电流 */
    {0, 0,  0}, {5, 60,  1}, {5, 30,  2}, {5, 60,  1},   /* SOC2  :                        B1 S相直流分量   C2 PV2电流   D1 GFCI电流 */
    {5, 60,  3}, {5, 120, 3}, {5, 60,  3}, {5, 120, 3},   /* SOC3  : A3 T相电网电压   B3 MUX2   C3 S相逆变电压   D3 MUX1 */
    {5, 30,  4}, {0, 0,  0}, {5, 60,  4}, {5, 30,  0},   /* SOC4  : A4 PV1电流                    C4 R相逆变电压   D0 PV3电流 */
    {5, 60,  5}, {0, 0,  0}, {0, 0,  0}, {5, 30,  4},   /* SOC5  : A5 R相电网电压                                  D4 平衡电路电流 */
    {5, 60,  0}, {0, 0,  0}, {0, 0,  0}, {5, 30, 15},   /* SOC6  : A0 T相逆变电压                                  D15 PV4电流 */
    {5, 60,  1}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC7  : A1 S相电网电压 */
    {6, 100, 2}, {0, 0,  0}, {0, 0,  0}, {6, 30,  2},   /* SOC8  : A2/D2 逆变电流第二采（PRD），A2 放慢作 ADCINT2 锚点 */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {6, 30, 14},   /* SOC9  : D14 R相逆变电流第二采（PRD） */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC10 */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC11 */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC12 */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC13 */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC14 */
    {5, 120, 0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC15 : ADC-A序列末位锚点，转换完成产生ADCINT1触发CLA Task1 */
};

/******************************************************************************
 * @brief ADC模块初始化配置表
 *
 * 说明：
 * - 四个模块统一12位单端采样。
 * - 只有ADC-A使能ADCINT1，中断触发SOC选SOC15（采样序列末尾），该中断同时作为
 *   CLA Task1的硬件触发源（CLA1TASKSRCSEL1.TASK1 = 1 = ADCA1）。
 ******************************************************************************/

/* ADC模块配置格式：{模块ID,SOC配置表指针,分辨率,信号模式,中断使能,中断触发SOC} */

const ST_ADC_CFG_ITEM stAdcCfgItem[] =
{
    /* ADC-A：12位单端，SOC15转换完成产生ADCINT1，用于触发CLA Task1 */
    {HW_ADC_ID0, &stAdcChCfgItem[0], HW_ADC_RESOLUTION_12BIT, HW_ADC_SIGNALMODE_SINGLE, 1U, 15U, 1U, 8U},

    /* ADC-B：12位单端，不使用ADC中断 */
    {HW_ADC_ID1, &stAdcChCfgItem[1], HW_ADC_RESOLUTION_12BIT, HW_ADC_SIGNALMODE_SINGLE, 0U, 0U},

    /* ADC-C：12位单端，不使用ADC中断 */
    {HW_ADC_ID2, &stAdcChCfgItem[2], HW_ADC_RESOLUTION_12BIT, HW_ADC_SIGNALMODE_SINGLE, 0U, 0U},

    /* ADC-D：12位单端，不使用ADC中断 */
    {HW_ADC_ID3, &stAdcChCfgItem[3], HW_ADC_RESOLUTION_12BIT, HW_ADC_SIGNALMODE_SINGLE, 0U, 0U},
};

ST_ADC_CFG stAdcCfg =
{
    &stAdcCfgItem[0],
    sizeof(stAdcCfgItem) / sizeof(stAdcCfgItem[0])
};

/******************************************************************************
 * @brief CLA初始化配置表
 *
 * 说明：
 * - 只使用Task1：由ADC-A的ADCINT1（ADCA1）硬件触发，替代软件强制触发。
 * - Task1入口 Cla1Task1 定义在 User_App/LoopCtrl/src/User_ClaTask.cla，
 *   完成采样搬移、一阶低通滤波、多路选择器通道归档。
 * - Task1执行结束时CLA会硬件置位CLA1_1中断（PIE第11组第1通道），供CPU挂控制中断。
 ******************************************************************************/

/* CLA任务配置格式：{任务号,触发源,任务入口函数} */

const ST_CLA_CFG_ITEM stClaCfgItem[] =
{
    {HW_CLA_TASK1, HW_CLA_TRIG_ADCA1, Cla1Task1},   /* ZERO 批次结束：控制主任务 */
    {HW_CLA_TASK2, HW_CLA_TRIG_ADCA2, Cla1Task2},   /* PRD  批次结束：搬第二采样值 */
};

ST_CLA_CFG stClaCfg =
{
    &stClaCfgItem[0],
    sizeof(stClaCfgItem) / sizeof(stClaCfgItem[0])
};


/******************************************************************************
 * @brief 逆变（INV）驱动配置表
 *
 * 拓扑：三电平，单极性控制。每相一对竖管（S1 上管 / S4 下管）
 *       和一对横管（S2 左管 / S3 右管）。
 *
 * 相与模块映射：
 * - R相：正半周 EPWM1 斩波，此时 EPWM4 作为静态态（EPWM4B 为高、EPWM4A 为低）；
 *        负半周 EPWM4 斩波，EPWM1 静态（EPWM1A 为低、EPWM1B 为高）。
 * - S相：正半周 EPWM2，负半周 EPWM10。
 * - T相：正半周 EPWM3，负半周 EPWM11。
 *
 * 静态态：EPWMxA 由 AQCSFRC 强制为低，EPWMxB 经死区模块反相后为高，
 *         即 S1/S4 关断、S2/S3 开通，桥臂输出钳位在母线中点。
 * 斩波态：占空比 d = (TBPRD - CMPA) / TBPRD，d = 0 时 A 恒低、B 恒高，
 *         与静态态波形逐位相同，因此可在 d 归零的那一拍无缝换相。
 ******************************************************************************/

/* 逆变相配置格式：{正半周PWM编号, 负半周PWM编号, 载波周期TBPRD}
 * 必须与 Hw_InvDriver.h 里 ST_INV_PHASE_CFG 的字段顺序逐一对齐：
 *   u16PosPwmId / u16NegPwmId / u16Period
 * 旧写法按 {相号, 正, 负} 填，导致 neg 与 pos 相同（EPWM4/10/11 永远不被驱动），
 * period 被填成 3/9/10，占空比换算 (1-d)*period 只剩个位数（实测 CMPA=2）。 */

const ST_INV_PHASE_CFG stInvPhaseCfgItem[] =
{
    {HW_PWM_ID0, HW_PWM_ID3,  2500U},   /* R相：正半周EPWM1A/1B，负半周EPWM4A/4B */
    {HW_PWM_ID1, HW_PWM_ID9,  2500U},   /* S相：正半周EPWM2A/2B，负半周EPWM10A/10B */
    {HW_PWM_ID2, HW_PWM_ID10, 2500U},   /* T相：正半周EPWM3A/3B，负半周EPWM11A/11B */
};

ST_INV_CFG stInvCfg =
{
    &stInvPhaseCfgItem[0],
    sizeof(stInvPhaseCfgItem) / sizeof(stInvPhaseCfgItem[0])
};

/******************************************************************************
 * @brief SCI 串口配置表
 *
 * 板上三个 SCI 模块：
 * - SCI-A：GPIO48(TX) / GPIO49(RX)
 * - SCI-C：GPIO90(TX) / GPIO89(RX)，与外部 TVS 通信，DSP 询问相位差，
 *          TVS 返回相位差信息，用于并网前的频率调节
 * - SCI-D：GPIO93(TX) / GPIO94(RX)
 *
 * 三个模块先按同一参数配置：115200-8-N-1，开 16 级 FIFO，
 * 接收中断暂不打开（先在任务里轮询），需要中断时把 bRxIntEnable 置 1
 * 并在 InterruptCfg 里注册 SCI 的 PIE 中断。
 ******************************************************************************/

/* 串口配置格式：{模块号,波特率,字符长度,停止位,校验,回环,FIFO使能,接收FIFO中断电平,接收中断使能} */
const ST_SCI_CFG_ITEM stSciCfgItem[] =
{
    /* 格式：{模块, 波特率, 字符长度, 停止位, 校验, 回环, FIFO使能, 接收中断电平, 接收中断使能, 断帧门限} */
    {HW_SCI_ID_A,  19200UL,  HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 1U, 19U},   /* SCIA：与ARM通信，3.5字符≈1.9ms→19拍 */
    {HW_SCI_ID_C, 115200UL, HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 1U, 4U},    /* SCIC：接收TVS相位差，3.5字符≈0.3ms→4拍 */
    {HW_SCI_ID_D,  38400UL, HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 1U, 10U},   /* SCID：上位机，3.5字符≈0.9ms→10拍 */
};

ST_SCI_CFG stSciCfg =
{
    &stSciCfgItem[0],
    sizeof(stSciCfgItem) / sizeof(stSciCfgItem[0])
};


/******************************************************************************
 * @brief CAN 配置表
 *
 * 板上两个 CAN 模块：
 * - CAN-A：GPIO36(RX) / GPIO37(TX)
 * - CAN-B：GPIO39(RX) / GPIO38(TX)
 *
 * 两个模块先按同一参数配置：500kbps，11 位标准帧，
 * 报文对象 1~4 为发送，5 为接收（验收屏蔽全 0，先收全部帧，
 * 由软件按 ID 分发；后续要精确过滤时改 u32RxAcceptId/u32RxAcceptMask）。
 * 位时间：20 个 Tq，TSEG1=14、TSEG2=5，采样点 75%。
 ******************************************************************************/

/* CAN配置格式：{模块号,波特率,发送对象起始号,发送对象数,接收对象起始号,接收对象数,接收验收ID,接收验收屏蔽,扩展帧,回环,自动总线恢复} */
const ST_CAN_CFG_ITEM stCanCfgItem[] =
{
    {HW_CAN_ID_A, 500000UL, 1U, 4U, 5U, 1U, 0x00000000UL, 0x00000000UL, 0U, 0U, 1U},
    {HW_CAN_ID_B, 1000000UL, 1U, 4U, 5U, 1U, 0x00000000UL, 0x00000000UL, 1U, 0U, 1U},
};

ST_CAN_CFG stCanCfg =
{
    &stCanCfgItem[0],
    sizeof(stCanCfgItem) / sizeof(stCanCfgItem[0])
};

/******************************************************************************
 * @brief eCAP 捕获配置表
 *
 * 板上 6 路捕获信号：
 * - eCAP1 ← GPIO25（INPUT7）：R 相电网过零
 * - eCAP2 ← GPIO27（INPUT8）：S 相电网过零
 * - eCAP3 ← GPIO29（INPUT9）：T 相电网过零
 * - eCAP4 ← GPIO30（INPUT10）：R 相电网频率信号
 * - eCAP5 ← GPIO31（INPUT11）：S 相电网频率信号
 * - eCAP6 ← GPIO32（INPUT12）：T 相电网频率信号
 *
 * 捕获方式：连续模式，只捕获事件1，捕获后 TSCTR 清零，
 * 所以 CAP1 = 相邻两个上升沿之间的 SYSCLK 计数（200MHz，1 计数 = 5ns）。
 * 50Hz 时 CAP1 ≈ 4,000,000，换算：f = 200000000 / CAP1。
 *
 * 捕获中断暂时不打开（bEnableInt = 0），先在 100us/1ms 任务里轮询；
 * 需要做过零时刻相位锁存时，把对应项的 bEnableInt 置 1，
 * 并在 InterruptCfg 里注册 eCAP1~eCAP6 的 PIE 中断。
 ******************************************************************************/

/* 捕获配置格式：{模块号,信号源GPIO,捕获边沿,捕获中断使能} */
const ST_ECAP_CFG_ITEM stEcapCfgItem[] =
{
    {HW_ECAP_ID0, 25U, HW_ECAP_EDGE_RISING, 0U},
    {HW_ECAP_ID1, 27U, HW_ECAP_EDGE_RISING, 0U},
    {HW_ECAP_ID2, 29U, HW_ECAP_EDGE_RISING, 0U},
    {HW_ECAP_ID3, 30U, HW_ECAP_EDGE_RISING, 0U},
    {HW_ECAP_ID4, 31U, HW_ECAP_EDGE_RISING, 0U},
    {HW_ECAP_ID5, 32U, HW_ECAP_EDGE_RISING, 0U},
};

ST_ECAP_CFG stEcapCfg =
{
    &stEcapCfgItem[0],
    sizeof(stEcapCfgItem) / sizeof(stEcapCfgItem[0])
};

/* ============ Modbus 从机：ARM(SCIA) + 上位机(SCID) + TVS(SCIC 只听) ============ */
const ST_MB_SLAVE_CFG stMbSlaveCfgItem[] =
{
    /* 格式：{SCI模块, 从机地址, 点表, 有效行数, 只听不应答} */
    {HW_SCI_ID_A, 1U, stMbRegItemArm, MB_ARM_REG_NUM, 0U},
    {HW_SCI_ID_D, 2U, stMbRegItemPc,  MB_PC_REG_NUM,  0U},

    /* TVS：SCIC 只收不发，所以配成只听不应答；从机地址待 TVS 侧协议确定 */
    {HW_SCI_ID_C, 3U, stMbRegItemTvs, MB_TVS_REG_NUM, 1U},
};

/* ATS 监听：SCIC 与 ARM 的 485 并接，ARM 作主站轮询，我们只听不应答。
 * 单从机问询：主站发往 0xFB，ATS 用自身地址应答；应答地址未知，
 * 由 AtsProto 的 pending 机制按请求帧位置还原。 */
ST_ATS_PROTO_CFG stAtsProtoCfg =
{
    HW_SCI_ID_C,
    ATS_ADDR_INQUIRY,
};

ST_MB_CFG stMbCfg =
{
    stMbSlaveCfgItem,
    (Uint16)(sizeof(stMbSlaveCfgItem) / sizeof(stMbSlaveCfgItem[0]))
};


/* ============ 软件 I2C：FM24C128D 的物理层 ============ */
/* 速率取 400kHz：FM24C128D 在 2.5V~5.5V 下支持 1MHz，本板 3.3V 供电，
 * 400kHz 留足余量；u16LoopPerUs = 30 是 200MHz、-O2 下实测标定值，
 * 换优化等级或改主频要重新标（标定方法：把 SCL 拉高拉低各一次，
 * 用示波器量半个周期，调这个数直到 400kHz 对应的 1.25us 对得上）。 */
ST_I2C_CFG stI2cCfg =
{
    91U,                        /* SDA：GPIO91 */
    92U,                        /* SCL：GPIO92 */
    HW_I2C_SPEED_400K,          /* 400kHz */
    HW_I2C_LOOP_PER_US_DEFAULT  /* 1us 空循环次数，0 则用缺省 */
};

/* ============ FM24C128D 器件配置 ============ */
/* 自检保留区放在低半区 0x1F00~0x1F0F：低半区不受 WP 保护，
 * 拉高 WP 时自检照样能跑；参数区在 0x0000~0x041F，两边不重叠。 */
ST_FM24C128D_CFG stFm24c128dCfg =
{
    FM24C128D_DEV_ADDR_7BIT,    /* 7 位设备地址 0x50（A2A1A0 接地） */
    FM24C128D_SIZE_BYTES,       /* 16KB */
    99U,                        /* 写保护脚 GPIO99 */
    1U,                         /* WP 高电平有效 */
    0x1F00U,                    /* 自检保留区首地址 */
    1U                          /* 上电做一次写读自检 */
};

/* ============ 掉电保存参数 ============ */
/* 三张点表全部参与扫描，属性里带 MB_ATTR_SAVE 的行才进保存清单。
 * 窗口参数的含义见 EepromParam.h 头注释：
 *   500ms   最后一次写之后静默多久落盘（合并窗口）
 *   5000ms  一直有写也要在 5s 内落盘（最长等待）
 *   1000ms  两次落盘最小间隔（EEPROM 寿命保护，最坏 1 次/秒） */
static const ST_EEPROM_PARAM_TABLE stEepromParamTable[] =
{
    {stMbRegItemArm, MB_ARM_REG_NUM},
    {stMbRegItemPc,  MB_PC_REG_NUM },
    {stMbRegItemTvs, MB_TVS_REG_NUM},
};

ST_EEPROM_PARAM_CFG stEepromParamCfg =
{
    stEepromParamTable,
    (Uint16)(sizeof(stEepromParamTable) / sizeof(stEepromParamTable[0])),
    500U,       /* u16FlushDelayMs：合并窗口 */
    5000U,      /* u16MaxDelayMs：最长等待 */
    1000U,      /* u16MinFlushIntervalMs：落盘限速 */
    1U,         /* u16TaskPeriodMs：1ms，对应 Task_Start(&EepromParamTask, NULL, 10U, ...) */
    1U,         /* bEnable：1 打开掉电保存 */
    0U          /* bFactoryReset：1 则上电忽略芯片内容，用代码初值重写 */
};

/* ============ SPI 主机：SPI-A 接 ARM 芯片 STM32F103VET6 ============ */
/* 字符宽度先用 8 位（STM32 侧按字节组帧最省事），模式 0（CPOL=0/CPHA=0），
 * 1MHz 起步：STM32F103 的 SPI 从机在 72MHz 主频下能稳接 18MHz，
 * 这里先压低是为了让第一次联调先看波形对不对，跑通后再往上提。
 * 片选走 GPIO57 软件控制，原因见 Hw_SpiDriver.h 头注释。 */
const ST_SPI_CFG_ITEM stSpiCfgItem[] =
{
    /* 格式：{模块号, 字符宽度, 空闲电平, 采样边沿, 目标波特率, 片选引脚, 片选有效电平, 超时us} */
    {HW_SPI_ID_A, 8U, 0U, 0U, 1000000UL, 57U, 0U, 2000U},
};

ST_SPI_CFG stSpiCfg =
{
    &stSpiCfgItem[0],
    (Uint16)(sizeof(stSpiCfgItem) / sizeof(stSpiCfgItem[0]))
};


/******************************************************************************
 * @brief ePWM TZ 故障源：硬件过流/短路先做逐波限流（CBC）
 *
 * 信号链：GPIO -> InputXbar INPUTx -> ePWM X-BAR TRIPn -> ePWM DCAH/DCBH
 *          -> TZDCSEL 形成 DCAEVT2/DCBEVT2 -> CBC 动作（占空比被逐波压到零）
 *
 * - PV 模块只有 EPWMxA 引出（GPIO 表里只有 EPWM5A/6A/7A/9A），死区模块关闭，
 *   A 通道就是全部输出，所以只配 DCAEVT2；
 * - 逆变 6 个模块 A/B 互补（B 由死区模块从 A 取反产生），如果只砍 A，
 *   下管（横管 S2/S3）仍在斩波，半桥没有真正封波，所以 A/B 两套事件都要配。
 *
 * 逆变过流(GPIO81)与硬件短路(GPIO82)在 ePWM X-BAR 里先或成 TRIP9，
 * 再同时接 DCAH 与 DCBH，这样一次触发就能同时压住 A、B 两侧。
 * 逐波限流不是永久封波：永久封波由 10kHz 保护任务判故障后走 TZFRC.OST。
 ******************************************************************************/

/* 格式：{PWM号, DCAH的TRIP源, DCBH的TRIP源, DCA触发条件, DCB触发条件, DCA动作, DCB动作} */
const ST_PWM_TZ_CFG_ITEM stPwmTzCfgItem[] =
{
    /* ---------- 逆变：EPWM1/2/3/4/10/11，两路故障信号低有效 ---------- */
    {HW_PWM_ID0,  HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* R相 正半周 */
    {HW_PWM_ID3,  HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* R相 负半周 */
    {HW_PWM_ID1,  HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* S相 正半周 */
    {HW_PWM_ID9,  HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* S相 负半周 */
    {HW_PWM_ID2,  HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* T相 正半周 */
    {HW_PWM_ID10, HW_PWM_TRIP_9, HW_PWM_TRIP_9, TZ_DCAH_LOW, TZ_DCBH_LOW, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_FORCE_LOW},   /* T相 负半周 */
    /* ---------- 光伏：EPWM7(PV1)/9(PV2)/5(PV3)/6(PV4)，只有 A 通道 ---------- */
    {HW_PWM_ID6,  HW_PWM_TRIP_4, HW_PWM_TRIP_NONE, TZ_DCAH_LOW, 0U, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_DO_NOTHING},       /* PV1 */
    {HW_PWM_ID8,  HW_PWM_TRIP_5, HW_PWM_TRIP_NONE, TZ_DCAH_LOW, 0U, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_DO_NOTHING},       /* PV2 */
    {HW_PWM_ID4,  HW_PWM_TRIP_7, HW_PWM_TRIP_NONE, TZ_DCAH_LOW, 0U, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_DO_NOTHING},       /* PV3 */
    {HW_PWM_ID5,  HW_PWM_TRIP_8, HW_PWM_TRIP_NONE, TZ_DCAH_LOW, 0U, HW_PWM_TZ_FORCE_LOW, HW_PWM_TZ_DO_NOTHING},       /* PV4 */
};

ST_PWM_TZ_CFG stPwmTzCfg =
{
    &stPwmTzCfgItem[0],
    (Uint16)(sizeof(stPwmTzCfgItem) / sizeof(stPwmTzCfgItem[0]))
};

/******************************************************************************
 * @brief X-BAR 配置：把 6 路比较器信号接到 INPUTx，再汇聚成 5 个 TRIP
 *
 * 分配原则：
 * - 每路光伏过流占一个独立 INPUT + 独立 TRIP，互不牵连（PV1 触发不该影响 PV2）；
 * - 逆变过流与短路共用 TRIP9（ePWM X-BAR 的 MUX 是或关系），
 *   但事件计数分开：INPUT4 -> XINT1、INPUT5 -> XINT2，软件能分辨是哪一路；
 * - INPUT4/INPUT5 同时被 XINT 和 ePWM X-BAR 取用，这是允许的（一个 INPUT 多播）。
 ******************************************************************************/

/* 引脚映射格式：{INPUT通道, GPIO号} */
const ST_XBAR_INPUT_CFG_ITEM stXbarInputCfgItem[] =
{
    {HW_XBAR_INPUT1, 13U},      /* PV1 硬件过流 */
    {HW_XBAR_INPUT2, 17U},      /* PV2 硬件过流 */
    {HW_XBAR_INPUT3, 41U},      /* PV3 硬件过流 */
    {HW_XBAR_INPUT4, 81U},      /* 硬件逆变过流：同时给 XINT1 计数 */
    {HW_XBAR_INPUT5, 82U},      /* 硬件短路：同时给 XINT2 计数 */
    {HW_XBAR_INPUT6, 40U},      /* PV4 硬件过流 */
};

/* TRIP 汇聚格式：{TRIP号, MUX号, MUX方式}；MUXn 对应 INPUT(n+1) */
const ST_XBAR_TRIP_MUX_CFG_ITEM stXbarTripMuxCfgItem[] =
{
    {HW_XBAR_TRIP4,  0U, HW_XBAR_MUX_ENABLE},   /* INPUT1(PV1) -> TRIP4  -> EPWM7  */
    {HW_XBAR_TRIP5,  1U, HW_XBAR_MUX_ENABLE},   /* INPUT2(PV2) -> TRIP5  -> EPWM9  */
    {HW_XBAR_TRIP7,  2U, HW_XBAR_MUX_ENABLE},   /* INPUT3(PV3) -> TRIP7  -> EPWM5  */
    {HW_XBAR_TRIP8,  5U, HW_XBAR_MUX_ENABLE},   /* INPUT6(PV4) -> TRIP8  -> EPWM6  */
    {HW_XBAR_TRIP9,  3U, HW_XBAR_MUX_ENABLE},   /* INPUT4(逆变过流) -> TRIP9         */
    {HW_XBAR_TRIP9,  4U, HW_XBAR_MUX_ENABLE},   /* INPUT5(短路)     -> TRIP9（或）    */
};

/* TRIP 输出极性：比较器输出低有效，TRIP 原样送给 DCAH/DCBH，不取反 */
const ST_XBAR_TRIP_OUT_CFG_ITEM stXbarTripOutCfgItem[] =
{
    {HW_XBAR_TRIP4, 0U},
    {HW_XBAR_TRIP5, 0U},
    {HW_XBAR_TRIP7, 0U},
    {HW_XBAR_TRIP8, 0U},
    {HW_XBAR_TRIP9, 0U},
};

ST_XBAR_CFG stXbarCfg =
{
    stXbarInputCfgItem,
    (Uint16)(sizeof(stXbarInputCfgItem) / sizeof(stXbarInputCfgItem[0])),
    stXbarTripMuxCfgItem,
    (Uint16)(sizeof(stXbarTripMuxCfgItem) / sizeof(stXbarTripMuxCfgItem[0])),
    stXbarTripOutCfgItem,
    (Uint16)(sizeof(stXbarTripOutCfgItem) / sizeof(stXbarTripOutCfgItem[0]))
};

/******************************************************************************
 * @brief 外部中断配置：XINT1/XINT2 只做事件计数
 *
 * 信号链路：GPIO81/82 -> INPUT4/INPUT5 -> XINT1/XINT2（输入通道是硬件固定的）
 * 比较器输出低有效，所以一律配下降沿；使能后每次下降沿进一次中断，
 * 中断里只把计数器加一（见 Hw_ExitDriver.c），判定放在 10kHz 保护任务里做。
 ******************************************************************************/

/* 格式：{XINT号, 触发极性, 使能} */
const ST_EXIT_CFG_ITEM stExitCfgItem[] =
{
    {HW_XINT_ID1, HW_XINT_FALLING, 1U},     /* GPIO81 逆变过流 */
    {HW_XINT_ID2, HW_XINT_FALLING, 1U},     /* GPIO82 硬件短路 */
};

ST_EXIT_CFG stExitCfg =
{
    &stExitCfgItem[0],
    (Uint16)(sizeof(stExitCfgItem) / sizeof(stExitCfgItem[0]))
};

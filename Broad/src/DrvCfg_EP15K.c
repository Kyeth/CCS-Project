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
    /* ========== ECAP / InputXbar 输入 ========== */
    {25, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP1 — R相电网过零信号（INPUT7）。
    {27, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP2 — S相电网过零信号（INPUT8）。
    {29, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP3 — T相电网过零信号（INPUT9）。
    {30, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP4 — R相电网频率信号（INPUT10）。
    {31, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP5 — S相电网频率信号（INPUT11）。
    {32, HW_GPIO_MUX_GPIO, HW_DIR_DI, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // eCAP6 — T相电网频率信号（INPUT12）。
    /* ========== ePWM输出引脚 ========== */
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
    {16, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM9A — PV2 Boost，错相90°。
    {18, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM10A — S竖管S4，同步于EPWM1A。
    {19, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM10B — S横管S2，互补于EPWM10A。
    {20, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM11A — T竖管S4，同步于EPWM1A。
    {21, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM11B — T横管S2，互补于EPWM11A。
    {23, HW_GPIO_MUX_PWM, HW_DIR_DO, HW_GPIO_LOW, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_SYNC},  // EPWM12B — 外置风扇PWM，四档风速。
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
    {57, HW_GPIO_MUX_SPI, HW_DIR_DO, HW_GPIO_HIGH, HW_GPIO_OD_DISABLE, HW_GPIO_PUD_DISABLE, HW_GPIO_QUAL_ASYNC},  // SPI-A片选信号SPISTEA。
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
        {1U, ET_CTR_ZERO, 1U, 0U, 1U} // 可以在 CTR=ZERO 触发 ADC 采样
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
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC8  */
    {0, 0,  0}, {0, 0,  0}, {0, 0,  0}, {0, 0,  0},   /* SOC9  */
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
    {HW_ADC_ID0, &stAdcChCfgItem[0], HW_ADC_RESOLUTION_12BIT, HW_ADC_SIGNALMODE_SINGLE, 1U, 15U},

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
    {HW_CLA_TASK1, HW_CLA_TRIG_ADCA1, Cla1Task1},
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

/* 逆变相配置格式：{相号,正半周PWM编号,负半周PWM编号} */

const ST_INV_PHASE_CFG stInvPhaseCfgItem[] =
{
    {HW_INV_PHASE_R, HW_PWM_ID0, HW_PWM_ID3},   /* R相：正半周EPWM1A/1B，负半周EPWM4A/4B */
    {HW_INV_PHASE_S, HW_PWM_ID1, HW_PWM_ID9},   /* S相：正半周EPWM2A/2B，负半周EPWM10A/10B */
    {HW_INV_PHASE_T, HW_PWM_ID2, HW_PWM_ID10},  /* T相：正半周EPWM3A/3B，负半周EPWM11A/11B */
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
    {HW_SCI_ID_A, 115200UL, HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 0U},
    {HW_SCI_ID_C, 115200UL, HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 0U},
    {HW_SCI_ID_D, 115200UL, HW_SCI_CHAR_LEN_8BIT, HW_SCI_STOP_BITS_1, HW_SCI_PARITY_NONE, 0U, 1U, 8U, 0U},
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
    {HW_CAN_ID_B, 500000UL, 1U, 4U, 5U, 1U, 0x00000000UL, 0x00000000UL, 0U, 0U, 1U},
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

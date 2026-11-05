# 步骤 01：采样数据层 L1（原始码值）／L2（瞬时值）与上电零漂

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`。工作区另有 6 个文件未提交改动，项目所有者已裁定
  为“已审核的发波测试逻辑，保留不动”（§4.1），本步直接以此为基线开工；本步 diff 只含 §2.4 的新增文件与
  §2.8 的改动。
- head_sha：工作区待审
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：无既有 ID；建议登记为 A04「采样数据层」
- 状态：待审（代码已落地，构建通过；板上项未测；§4.7 已澄清，无需裁定）

## 意图与边界

### 问题

当前 CPU 侧没有任何可用的采样数据。唯一被读出来的量是 CLA 任务里的两条一阶低通结果
（`DrvCfg_EP15K.c:16` 的 `stClaMuxNext[8]`、`:17` 的 `fClaMuxAlpha = 0.20f`，滤波值发布在
`Cla1ToCpuMsg.fMuxB3 / fMuxD3`），其含义依赖“当前地址是多少”，CPU 无法按测点名字取值；
18 路固定测点（`ADCRESULT`）没有任何一处被 CPU 读过，也没有零漂和量程换算。
结果是每个要用采样的地方都得自己读寄存器、自己乘系数、自己猜地址，无法统一。

### 本步目标

1. **L1 `ST_SAMPLE_RAW`**：20 kHz 一次，把 18 路固定测点 + 2 个多路选择器当前地址对应的码值
   全部收进一个结构体，并带上本拍地址值与序号。
2. **L2 `ST_SAMPLE_INS`**：`(code - zero) * K`，单位 V/A，与 L1 同形逐字段对应。
3. **零漂状态机**：上电 3 s 后取 200 ms 平均值作为零点，一次记录；未就绪前 L2 及以后不生效，
   并对外给出可查询的返回值。
4. **MUX 打散归档从 CLA 搬到 CPU ISR**（此前已确认的方案 A）：CLA 不再读 ADC、不再滤波，
   只把“下一拍地址”转交给 CPU；地址语义已确认，按测点名归档，见 §2.1。

### 明确不做（本步边界）

- 不做 L3 块平均、L4 交流周期值／RMS／电网频率、L5 显示值；只预留结构体与字段，不写算法。
- 不改 PWM 与 TZ 配置，不改任何保护判定，不加零漂期间的功率级互锁
  （只在文档里写明：开波前应检查 `AdcCalibIsReady()`）。
- 不改 SCI／CAN 上报格式，不改 `Platform/Status` 的观测字段。
- 不改 `docs/minimal-bringup.md` 与 `EP15K_2026_配置核心与运行逻辑.md`。这两份现在描述的是
  “CLA 取两个 MUX 通道 + 低通 + 发布”，本步落地后会与代码不一致，**需要项目所有者决定**
  是“本步一并改文档”还是“单独开一步”，见 §4.2。

### 可能影响的功率输出

本步不触碰 PWM 配置表、TZ、AQ，功率输出引脚行为不变。间接受影响的是 20 kHz ISR 的执行时间
（多出 20 次 `ADCRESULT` 读 + 约 25 次乘加，估算 < 3 µs @ 200 MHz，占 50 µs 周期的 < 6%，
属估算、待上板实测），以及 CLA 任务从 ~35 行降到 ~6 行。若 ISR 超时会漏拍，这是需要板上确认项。

## 修改前后

### 2.1 MUX 地址语义（本步新增并固化的硬事实）

地址线 A0~A2 = GPIO46/47/42，环为 `0→1→2→3→4→6→7→0`，地址 5 不使用
（`DrvCfg_EP15K.c:16` 表项 `[5] = 6U` 是死项，`Hw_ClaTask.cla:17` 有防御 `address == 5U → 0U`）。

项目所有者确认（m00150、m00199）：

- **B3（MUX2）** 只在地址 0~4 有意义：`0=PV1 电压、1=PV2 电压、2=PV3 电压、3=PV4 电压、4=VISO 电压`，
  共 5 个读数。
- **D3（MUX1）** 与 B3 共用地址线，只在地址 6、7 有意义：`6=BUSL 下半母线`、`7=BUSH 上半母线`，共 2 个读数。

两个多路选择器的数据量不同（5 路 / 2 路），且语义也不同构：MUX2 的前 4 路是一组同构量（PV1~PV4 电压），
第 5 路 VISO 是独立量；MUX1 的两路则各自独立。因此**不共用一个匿名数组**，归档形状为：

- `u16PvVoltCode[SAMPLE_PV_NUM]`（4 路，与 `u16PvCurrCode[4]` 一一对应，可直接算每串 PV 功率）
- `u16VisoCode`、`u16BusLCode`、`u16BusHCode` 三个独立标量

（前一版草案写的是 `SAMPLE_MUX2_NUM = 5` / `SAMPLE_MUX1_NUM = 2` 两个数组；地址语义确认后改成上面这种
形状，两个选择器的数组长度天然不同，不会有人误以为能用一个 `N` 统一。）

每个 MUX 通道的刷新周期都是 7 × 50 µs = 350 µs（2.857 kSa/s），比固定测点慢 7 倍。

| 地址 A2..A0 | B3（MUX2） | D3（MUX1） | 归档到 |
|---|---|---|---|
| 0 | PV1 电压 | — | `u16PvVoltCode[0]` |
| 1 | PV2 电压 | — | `u16PvVoltCode[1]` |
| 2 | PV3 电压 | — | `u16PvVoltCode[2]` |
| 3 | PV4 电压 | — | `u16PvVoltCode[3]` |
| 4 | VISO 电压 | — | `u16VisoCode` |
| 5 | 不用 | 不用 | — |
| 6 | — | BUSL 下半母线 | `u16BusLCode` |
| 7 | — | BUSH 上半母线 | `u16BusHCode` |

### 2.2 固定 18 路寄存器映射（验收时逐条核对 SDK 头文件）

> 前提：本步只按 **SOC 行号**取结果寄存器，不假设 20 路的触发时刻一致。当前工作区把 PV1~PV4 电流这 4 个 SOC 的触发源改成了 17/21/13/15（分别为 EPWM7/EPWM9/EPWM5/EPWM6 的 SOCA），其余仍为 5（EPWM1 SOCA）——这是既有发波测试改动的一部分，本步不动，但其影响见 §4.7。

数组索引约定 R=0 / S=1 / T=2。**结果寄存器编号等于 SOC 行号，与通道号无关**
（例如 D 模块通道 14 在第 SOC1 行、写到 `ADCD RESULT1`；通道 0 在第 SOC4 行、写到 `RESULT4`）。
这是本步最容易抄错的一处，验收请按 `DrvCfg_EP15K.c:288-303` 的 SOC 表逐行核对下表。

| 结构体字段 | 索引 | ADC 模块 | 通道 | SOC 行 | 结果寄存器 | 物理量 |
|---|---|---|---|---|---|---|
| `u16InvCurrCode[]` | 0 = R | D | 14 | SOC1 | `AdcdResultRegs.ADCRESULT1` | 逆变电流 |
| | 1 = S | D | 2 | SOC0 | `AdcdResultRegs.ADCRESULT0` | |
| | 2 = T | A | 2 | SOC0 | `AdcaResultRegs.ADCRESULT0` | |
| `u16InvVoltCode[]` | 0 = R | C | 4 | SOC4 | `AdccResultRegs.ADCRESULT4` | 逆变电压 |
| | 1 = S | C | 3 | SOC3 | `AdccResultRegs.ADCRESULT3` | |
| | 2 = T | A | 0 | SOC6 | `AdcaResultRegs.ADCRESULT6` | |
| `u16GridVoltCode[]` | 0 = R | A | 5 | SOC5 | `AdcaResultRegs.ADCRESULT5` | 电网电压 |
| | 1 = S | A | 1 | SOC7 | `AdcaResultRegs.ADCRESULT7` | |
| | 2 = T | A | 3 | SOC3 | `AdcaResultRegs.ADCRESULT3` | |
| `u16InvDcCode[]` | 0 = R | B | 0 | SOC1 | `AdcbResultRegs.ADCRESULT1` | 直流分量 |
| | 1 = S | B | 1 | SOC2 | `AdcbResultRegs.ADCRESULT2` | |
| | 2 = T | B | 2 | SOC0 | `AdcbResultRegs.ADCRESULT0` | |
| `u16PvCurrCode[]` | 0 = PV1 | A | 4 | SOC4 | `AdcaResultRegs.ADCRESULT4` | PV 电流 |
| | 1 = PV2 | C | 2 | SOC2 | `AdccResultRegs.ADCRESULT2` | |
| | 2 = PV3 | D | 0 | SOC4 | `AdcdResultRegs.ADCRESULT4` | |
| | 3 = PV4 | D | 15 | SOC6 | `AdcdResultRegs.ADCRESULT6` | |
| `u16BalCurrCode` | — | D | 4 | SOC5 | `AdcdResultRegs.ADCRESULT5` | 平衡电路电流 |
| `u16GfciCurrCode` | — | D | 1 | SOC2 | `AdcdResultRegs.ADCRESULT2` | GFCI 漏电流 |
| `u16PvVoltCode[]` | 地址 0~3 | B | 3 | SOC3 | `AdcbResultRegs.ADCRESULT3` | PV1~PV4 电压 |
| `u16VisoCode` | 地址 4 | B | 3 | SOC3 | `AdcbResultRegs.ADCRESULT3` | VISO 电压 |
| `u16BusLCode` | 地址 6 | D | 3 | SOC3 | `AdcdResultRegs.ADCRESULT3` | BUSL 下半母线 |
| `u16BusHCode` | 地址 7 | D | 3 | SOC3 | `AdcdResultRegs.ADCRESULT3` | BUSH 上半母线 |

ADCINT1 的触发点是序列锚点 SOC15（`Hw_AdcDriver.c:238-244`：`INT1SEL = 15`、`INT1CONT = 1`），
所以进入 `Isr_Control` 时序列里全部结果寄存器都已转换完成。这是“CPU 在 ISR 里直接读 20 个
`ADCRESULT` 而不需要等 EOC”的依据，也是本步不必引入额外同步的全部理由。

### 2.3 数据流

改前：

```
ePWM1 CTR=ZERO → SOC 序列 → ADCINT1 → CLA Task1
    CLA：读 B3/D3 → 一阶低通 → 发布 fMuxB3/fMuxD3 + u16NextMuxAddress
        → CLA1_1 中断 → CPU Isr_Control：写 GPIO46/47/42、写下一拍地址、清 MICLR/PIEACK
```

改后：

```
ePWM1 CTR=ZERO → SOC 序列 → ADCINT1 → CLA Task1
    CLA：只发布 u16NextMuxAddress + u32Sequence（不再读 ADC、不再滤波）
        → CLA1_1 中断 → CPU Isr_Control：
            AdcSampleFast()：读 20 个结果寄存器 → 填 L1 → 按地址归档 MUX → 调 AdcCalibStep()
            → 写 GPIO46/47/42、写下一拍地址、清 MICLR/PIEACK
```

### 2.4 新增文件

| 文件 | 内容 |
|---|---|
| `Platform/Sample/Inc/SampleData.h` | L1~L5 结构体与枚举、`SampleDataInit()`。纯数据定义，只依赖 `F2837xS_device.h`（与 `Platform/Status/Inc/BringupStatus.h` 同风格） |
| `Platform/Sample/Src/SampleData.c` | 5 个实例的定义 + 清零 |
| `Platform/Sample/Inc/AdcCalib.h` | 零漂状态机枚举、`AdcCalibStep()` / `AdcCalibIsReady()` / `AdcCalibConvert()`，标定表结构 `ST_SAMPLE_CAL_ITEM` / `ST_SAMPLE_CAL_CFG` |
| `Platform/Sample/Src/AdcCalib.c` | 状态机、零点累加、码值→工程值换算 |
| `Platform/Sample/Inc/AdcSample.h` | `AdcSampleFast()` 与 20 kHz 节拍常量 |
| `Platform/Sample/Src/AdcSample.c` | 读寄存器、填 L1、MUX 归档、调零漂 |
| `Broad/inc/DrvCfg_Sample.h` | `extern ST_SAMPLE_CAL_CFG stSampleCalCfg;`、MUX 地址语义宏 |
| `Broad/src/DrvCfg_Sample.c` | 标定表实例（每路 K、标称零点、是否自动零漂） |

`tools/build_minimal.py` 的 `SOURCES` 增加 3 个 .c，`INCLUDES` 增加 `"Platform/Sample/Inc"`；
`.cproject` 的两套配置同步加 include 路径（否则 CCS 内构建与脚本构建不一致）。

### 2.5 结构体定义（L1／L2 本步落地，L3~L5 只建壳）

```c
#define SAMPLE_PH_NUM    (3U)  /* R / S / T */
#define SAMPLE_PV_NUM    (4U)

/* 多路选择器地址线取值（A2..A0），用于 ISR 里的归档判断 */
#define SAMPLE_MUX2_ADDR_PV1   (0U)
#define SAMPLE_MUX2_ADDR_VISO  (4U)   /* 0~3 = PV1~PV4 电压 */
#define SAMPLE_MUX1_ADDR_BUSL  (6U)   /* 下半母线 */
#define SAMPLE_MUX1_ADDR_BUSH  (7U)   /* 上半母线 */

typedef struct
{
    Uint16 u16InvCurrCode[SAMPLE_PH_NUM];
    Uint16 u16InvVoltCode[SAMPLE_PH_NUM];
    Uint16 u16GridVoltCode[SAMPLE_PH_NUM];
    Uint16 u16InvDcCode[SAMPLE_PH_NUM];
    Uint16 u16PvCurrCode[SAMPLE_PV_NUM];
    Uint16 u16BalCurrCode;
    Uint16 u16GfciCurrCode;
    Uint16 u16PvVoltCode[SAMPLE_PV_NUM];   /* MUX2 地址 0~3 */
    Uint16 u16VisoCode;                    /* MUX2 地址 4 */
    Uint16 u16BusLCode;                    /* MUX1 地址 6，下半母线 */
    Uint16 u16BusHCode;                    /* MUX1 地址 7，上半母线 */
    Uint16 u16MuxAddr;      /* 本拍地址线值，正常为 0~4 / 6 / 7 */
    Uint32 u32Seq;          /* 每次 ISR 加一，兼作丢拍检查 */
} ST_SAMPLE_RAW;            /* 28 字 */

typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    float fInvVolt[SAMPLE_PH_NUM];
    float fGridVolt[SAMPLE_PH_NUM];
    float fInvDc[SAMPLE_PH_NUM];
    float fPvCurr[SAMPLE_PV_NUM];
    float fBalCurr;
    float fGfciCurr;
    float fPvVolt[SAMPLE_PV_NUM];
    float fVisoVolt;
    float fBusLVolt;
    float fBusHVolt;
} ST_SAMPLE_INS;            /* 25 个 float = 50 字 */

typedef struct
{
    float fInvCurr[SAMPLE_PH_NUM];
    /* …与 INS 同形，逐字段对应… */
    float fPvVolt[SAMPLE_PV_NUM];
    float fVisoVolt;
    float fBusLVolt;
    float fBusHVolt;
    Uint16 u16State;        /* E_ADC_CAL_STATE */
    Uint16 u16Valid;        /* 1 = 零点有效（TRACK 完成或标称回退） */
} ST_SAMPLE_ZERO;

/* L3~L5 本步只建壳，SampleDataInit() 里清零，不写任何算法 */
typedef struct { float fInvVolt[...]; Uint16 u16Cnt8; Uint16 u16Cnt200; } ST_SAMPLE_FLT;
typedef struct { float fGridVoltRms[...]; float fGridFreq; Uint16 u16Valid; } ST_SAMPLE_AC;
typedef struct { float fGridVoltDisp[...]; Uint16 u16Cnt; } ST_SAMPLE_DSP;

extern ST_SAMPLE_RAW  stSampleRaw;
extern ST_SAMPLE_INS  stSampleIns;
extern ST_SAMPLE_ZERO stSampleZero;
```

只用到 `float` 与 `Uint16`，不引入 64 位量，避免 C2000 上的软件浮点/长整型乘法开销。
（原 IB800U 风格里 `adresult1[8]` 这类 `long` 数组在 CLA 侧是 32 位，本步移到 CPU 侧后统一为
`Uint16` 码值 + `float` 工程值，不再保留 32 位码值副本。）

### 2.6 零漂状态机

```c
#define ADC_CAL_STATE_IDLE   (0U)   /* 未启动 */
#define ADC_CAL_STATE_SKIP   (1U)   /* 上电 3 s 内，功率级仍在初始化，不做零点 */
#define ADC_CAL_STATE_TRACK  (2U)   /* 连续累加 ADC_CAL_TRACK_TICKS 个样本 */
#define ADC_CAL_STATE_READY  (3U)   /* 零点有效，L2 起生效 */
#define ADC_CAL_STATE_FAULT  (4U)   /* 零点超差，回退标称值，u16Valid = 0 */

#define ADC_CAL_SKIP_TICKS   (60000U)  /* 3 s @ 20 kHz */
#define ADC_CAL_TRACK_TICKS  (4000U)   /* 200 ms = 10 × 50 Hz = 12 × 60 Hz 整周期 */
#define ADC_CAL_ZERO_TOL     (307.0f)  /* ±15% of 2048，超差判 FAULT */
```

- 累加器用 `Uint32`：4000 × 4096 = 1.638e7 < 2^32，平均值由整数除法得到后转 float，**无累积误差**。
- 200 ms 窗口对 50 Hz 是 10 个整周期，对 60 Hz 是 12 个整周期，正弦净和为 0，所以上电时电网
  已经带电也不影响零点。
- 超差处理：保留标称零点、`u16Valid = 0`、状态停在 FAULT，不自动重试（避免反复清零导致
  INS 在 READY/FAULT 之间跳动）。
- 门控：多路选择器四路（`u16PvVoltCode[]`、`u16VisoCode`、`u16BusLCode`、`u16BusHCode`）不做零漂
  （单极性、器件侧无偏置网络），`bTrackZero = 0`，零点是 0，工程值即 `code * K`。这一条在 §4.5
  里仍列为待板上核实项。

### 2.7 标定表（Broad 层）

```c
typedef struct
{
    float  fK;           /* 码值 → 工程值（V/码 或 A/码） */
    float  fZeroNom;     /* 标称零点码值，TRACK 失败时回退 */
    Uint16 bTrackZero;   /* 1 = 上电记录零漂；0 = 直接用 fZeroNom */
} ST_SAMPLE_CAL_ITEM;

typedef struct
{
    ST_SAMPLE_CAL_ITEM aInvCurr[SAMPLE_PH_NUM];    /* R / S / T */
    ST_SAMPLE_CAL_ITEM aInvVolt[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aGridVolt[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aInvDc[SAMPLE_PH_NUM];
    ST_SAMPLE_CAL_ITEM aPvCurr[SAMPLE_PV_NUM];
    ST_SAMPLE_CAL_ITEM aPvVolt[SAMPLE_PV_NUM];     /* MUX2 地址 0~3 */
    ST_SAMPLE_CAL_ITEM aVisoVolt;                  /* MUX2 地址 4 */
    ST_SAMPLE_CAL_ITEM aBusLVolt;                  /* MUX1 地址 6 */
    ST_SAMPLE_CAL_ITEM aBusHVolt;                  /* MUX1 地址 7 */
    ST_SAMPLE_CAL_ITEM aBalCurr;
    ST_SAMPLE_CAL_ITEM aGfciCurr;
} ST_SAMPLE_CAL_CFG;
```

系数一律以 `AdcCalib.h` 的注释表为准（项目所有者裁定，m00199：口述数值只是举例），沿用其宏、
不新造量程数值。**码值约定**按 TRM SPRUHX5G Table 10-2/10-3 统一为“满量程 4096 码、
零点 2048 码”（裁定 m00630）：`ADCRESULT = 4096·Vin/Vref` 截断、`Vin = Vref·(code+0.5)/4096`，
寄存器在 `Vin ≥ Vref` 时饱和为 4095，故 4095 是“最后一格”而非 3 V 的码值，不做分母。
于是 `ADC_CAL_CODE_FULL = 4096.0f`、`ADC_CAL_CODE_MID = 2048.0f`、
`ADC_CAL_CODE_PV_MID = 2275.56f (= 4096·5/9)`，各系数写成对这三个宏的表达式：
`ADC_CAL_K_INV_CUR (100/MID)`、`ADC_CAL_K_AC_VOLT (462.69/MID)`、
`ADC_CAL_K_DC_CUR (2.95/MID)`、`ADC_CAL_K_BAL_CUR (36.4/MID)`、
`ADC_CAL_K_GFCI (0.5/MID)`、`ADC_CAL_K_PV_CUR (30/(FULL-PV_MID))`、
`ADC_CAL_K_PV_VOLT (1033/FULL)`、`ADC_CAL_K_BUS_VOLT (563.64/FULL)`。
`AdcCalib.h` 里没有 VISO 的宏，本步补一个 `ADC_CAL_K_VISO_VOLT (488.0f / 4096.0f)`
（数值取自该文件注释的 488V↔3V，不是新测出来的）。
标称零点：有偏置的通道 2048.0，PV 电流 2275.56，四路 MUX 0.0。
相对旧的 /4095 约定，系数变化 ≤0.036%（满幅读数差 0.23 V / 463 V），远低于板上精度，
改的是语义正确性：2047.5 这种“半个码”在实测中不存在。

### 2.8 修改文件

| 文件 | 原行为 → 新行为 |
|---|---|
| `Drivers/inc/Hw_ClaTask.h` | `ST_CLA_TO_CPU_MIN{Uint32 u32Sequence; Uint16 u16AdcaAnchor; Uint16 u16MuxAddress; Uint16 u16NextMuxAddress; float fMuxB3; float fMuxD3;}`（9 字）→ `{Uint32 u32Sequence; Uint16 u16NextMuxAddress;}`（3 字）；`ST_CPU_TO_CLA_MIN{Uint16 u16MuxAddress; Uint16 u16NextMux[8]; float fAlpha;}`（11 字）→ 去掉 `fAlpha`（9 字）。消息 RAM 合计 20 字 → 12 字（上限 128 字） |
| `Drivers/src/Hw_ClaTask.cla` | 删除 `gMuxB3Filtered/gMuxD3Filtered` 及其 `#pragma DATA_SECTION`、删除 ADC 读取与低通、删除锚点诊断；保留：取地址 → 查表得下一拍 → 发布 → `u32Sequence++` → 清 `ADCINT1`。RAMLS5 省 64 字 |
| `Broad/src/DrvCfg_EP15K.c` | 删 `const float fClaMuxAlpha`（:17）；`stClaMuxNext[8]`（:16）保留，环仍需要 |
| `main.c` | `PeripheralsInit` 尾部对 `CpuToCla1Msg` 的初始化去掉 `fAlpha`；`Isr_Control` 里在写 GPIO 之前插入 `AdcSampleFast()`；`stBringupStatus` 增补零漂状态的只读镜像（见下） |
| `Platform/Status/Inc/BringupStatus.h` | 新增 2 个只读观测字段：`Uint16 u16CalState`、`Uint16 bCalReady`（不加错误位，零漂失败属于可恢复的标定问题，不应进 `unError`） |
| `tools/build_minimal.py` / `.cproject` | 构建清单同步 |

`Isr_Control` 改后的骨架（顺序是本步的关键，地址必须在算完之后才写 GPIO）：

```c
AdcSampleFast();                              /* 读 L1 + MUX 归档 + 零漂 + L2 */
next = Cla1ToCpuMsg.u16NextMuxAddress & 7U;   /* CLA 上一拍查好的地址 */
Hw_GpioWritePin(46U, next & 1U);
Hw_GpioWritePin(47U, (next >> 1) & 1U);
Hw_GpioWritePin(42U, (next >> 2) & 1U);
CpuToCla1Msg.u16MuxAddress = next;
Cla1Regs.MICLR.bit.INT1 = 1U;
PieCtrlRegs.PIEACK.all = PIEACK_GROUP11;
```

### 2.9 对既有设计的偏离（需知会）

`origin/deepseek/pcs-stepwise` 分支上的 `SampleData.h`／`AdcSample.h`／`AdcCalib.h` 是同一想法的
先期版本。本步不直接搬用，差异有四处，都是为适配本分支的现状：多路选择器按地址拆成
`u16PvVoltCode[4]` + `u16VisoCode`/`u16BusLCode`/`u16BusHCode` 三个具名标量，而不是
`fMuxB3Raw`／`fMuxD3Raw` 两个“当前地址的标量”；L1 用 `Uint16` 码值而非 `float`；
零漂放在 `AdcCalib.c` 而不是 `AdcSample.c`；L3 用块平均而不是 1 kHz `ALPHA_RMS` 递推。

## 验证

### 构建（已跑）

```
python tools/build_minimal.py --cgt 'D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS'
```

脚本默认 `--cgt` 仍指向 `E:/CCS/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS`；本机实际安装位置是 `D:/ti/CCS1110/...`，故显式传参。

- 29 个编译单元全部成功，编译器无任何 warning 输出，链接成功。
- 产物：`../CCS-Project-minimal-build/EP15K_minimal.out`（大小随 m00630 的 /4096 改动由 583,520 变为
  583,524 字节，sha256 `cd949cf99188e841b669de6ae3297b137850496e949063fee0108de93e522660`）、
  `EP15K_minimal.map`（82,035 字节）。
- 新增的 4 个编译单元：`Broad/src/DrvCfg_Sample.c`、`Platform/Sample/Src/{SampleData,AdcSample,AdcCalib}.c`。
- m00630（码值约定改 4096）后重新全量构建，仍为 29 个单元、零 warning、链接成功。

### 静态核对项（未上板；标“未测”的不得算通过）

| 项 | 判据 | 结果 |
|---|---|---|
| 构建干净 | 退出码 0、无新增 warning | **通过**（29 单元，0 warning） |
| MUX 表与语义一致 | `stClaMuxNext` 环含 0~4、6、7，不含 5 | **通过**（`Broad/src/DrvCfg_EP15K.c:16` 仍为 `{1U,2U,3U,4U,6U,6U,7U,0U}`；`fClaMuxAlpha` 已删） |
| 20 个寄存器地址与 §2.2 表逐条一致 | 对照 `DrvCfg_EP15K.c` 的 `stAdcChCfgItem[]` | **通过**（逐条比对结果：A0/U0、A1/U7、A2/U0、A3/U3、A4/U4、A5/U5；B0/U1、B1/U2、B2/U0、B3/U3；C2/U2、C3/U3、C4/U4；D0/U4、D1/U2、D2/U0、D3/U3、D4/U5、D14/U1、D15/U6，共 20 个，与 `AdcSampleRead()` 完全一致） |
| 零漂常量 | 60000 / 4000 / 307.0 三处一致 | **通过**（`Platform/Sample/Inc/AdcCalib.h:30-32`） |
| 消息 RAM 尺寸 | ≤ 128 字（各段） | **通过**（map：`CLA1_MSGRAMLOW` 4 字、`CLA1_MSGRAMHIGH` 9 字；结构体本身 3 字与 9 字） |
| RAMLS5 占用 | 只剩 `.scratchpad` | **通过**（map：RAMLS5 used = 2 字；`Cla1DataRam` 段已不在链接结果里） |
| 内存余量 | 不挤占既有段 | **通过**（RAMLS0 0x6C、RAMD0 0x92、FLASH_APP 0x2B54 / 0x19FFE） |
| 20 kHz ISR 时长 | 逻辑分析仪测 GPIO 翻转 | **未测（需板）** |
| 零点与量程实际精度 | 逐通道加已知输入比对 | **未测（需板）** |
| L2 在 READY 前不更新 | 上电 3.2 s 内观察 `stSampleIns` 保持不变 | **未测（需板）** |
| MUX 归档与地址线一致 | 逐个地址加已知电压走一圈 | **未测（需板）** |

### 未跑项

- `tools/verify_config.py`（管脚表静态核对）：必须给 `--pin-sheet Z:/Git1029/CCS-Project/docs/review/2026-10-29/15KW软件管脚整理文档.xlsx`，本机未挂载 Z: 盘，未跑。已确认该脚本不引用本步改动的符号（`grep 'fClaMuxAlpha|fMux|ADCRESULT' tools/verify_config.py` 无命中），故本步不预期改变它的结论。

### 文档同步

- `docs/minimal-bringup.md`：CLA/CPU 行、MUX 地址语义段、`stBringupStatus` 观测字段、构建单元数与首次验收第 3 步。
- `EP15K_2026_配置核心与运行逻辑.md`（仓库外）：§3.7 整节重写为「配置表与标量 / 两个消息结构体 / CLA 任务 / CPU 侧采样链」，并同步开篇摘要、文件表、§4.1 第 10 步、§4.3 `Isr_Control` 伪码、§4.4 数据流图、附录常量表与术语表。

## 风险与请求裁定

### 4.1 基点与工作区未提交改动（已裁定，m00199）

`git status` 显示 6 个文件已修改未提交：

- `.launches/EP15K_2026.launch`、`.settings/org.eclipse.core.resources.prefs` —— 工程元数据。
- `Broad/src/DrvCfg_EP15K.c` —— EPWM1~4/10/11 的 `CMPA 2500→1250`、SOCA 由
  `{1U, ET_CTR_ZERO, 1U, 0U, 1U}` 改为 `{1U, ET_CTR_ZERO, 0U, 1U, 0U}`、其余 PWM 的 SOCA 清零。
- `Drivers/inc/Hw_PwmDriver.h` + `Drivers/src/Hw_PwmDriver.c` —— 新增 `Hw_OpenAllPWM()`（写
  `AQCSFRC.all = 0` 并 `TZCLR.bit.OST = 1`），并加了“4/7/10 三路无法被写成 DISABLE”的注释。
- `main.c` —— `StartSystem` 里 `Hw_EnableAllPwmCLKSync()` 之后加了 `Hw_OpenAllPWM()`。

**所有者裁定：** 这 6 个文件是已审核的发波测试逻辑，**保留不动**，不要求先提交；本步直接以其为基线开工。
因此 `base_sha` 仍写 `5bb641f70f0588cdf70f0b0c01ea4a0b321963df`，而本步 diff 的审阅范围是
“§2.4 新增的文件 + §2.8 列出的改动”，这 6 个文件的既有改动**不属于本步**。

由此带来一个操作要求（不是待裁定项，是本步的提交纪律）：`main.c` 与 `Broad/src/DrvCfg_EP15K.c`
里已经混有发波逻辑，本步改这两个文件时只暂存本步的 hunk（`git add -p`），不得把发波逻辑一起提交，
否则本步 diff 又会夹带他人改动。

### 4.2 文档与代码将短暂不一致（已裁定，m00199）

`docs/minimal-bringup.md` 与 `EP15K_2026_配置核心与运行逻辑.md` §3.7 都写着“CLA 取两个 MUX
通道 + 低通 + 发布”，本步落地后就不再成立。请裁定：本步一并改这两处文档，还是单独开一步。
（我倾向于一并改，因为“CLA 不再读 ADC”是本步最容易被后来人误解的地方。）

**所有者裁定：** 本步一并改两处文档。已按此执行，见「文档同步」。

### 4.3 量程与系数的取值依据（已裁定，m00199）

口述量程只是举例（原话“我口述的是打比方的”），**一律以 `AdcCalib.h` 的注释表为准**。记录在此以免
后来人拿口述数值改 `fK`：

| 测点 | 口述（举例，不采用） | `AdcCalib.h`（采用） |
|---|---|---|
| 负载/逆变电压 | 331.95 Vdc → 3 V | 462.69 V ↔ 0~3 V |
| 负载/逆变电流 | 100 Adc → 3 V | 100 A ↔ 0~3 V（一致） |
| 总母线电压 | 626.5 V → 3 V | 563.64 V ↔ 0~3 V（D3） |
| VISO | 488 V → 3 V | 488 V ↔ 0~3 V（B3 地址 4，一致） |
| 粘连电压 | 331.95 Vdc → 3 V | 无对应项 |

因此 §2.7 的系数表就是最终值，不再等待硬件确认。

### 4.4 D3（MUX1）地址 6、7 的物理含义（已裁定，m00199）

**地址 6 = BUSL 下半母线，地址 7 = BUSH 上半母线。** 两路都按 `ADC_CAL_K_BUS_VOLT (563.64/4096)`
换算（`AdcCalib.h` 原文即“多路选择器1 563.64V↔3V（上/下半母线）”）。命名就是
`u16BusLCode`/`u16BusHCode`、`fBusLVolt`/`fBusHVolt`，不再用中性的 `fMux1Volt[2]`。
两者的和（或差，取决于硬件参考点）即总母线电压；本步只发布两个半母线值，**不做求和**（那是 L3/L4 的事）。

### 4.5 MUX 通道不做零漂是否最终态

用户说“除 MUX 以外其他采样都有偏置电压”。本步据此把 MUX 的 `bTrackZero` 设为 0、零点取 0。
若 MUX 前端其实有运放偏置，这里会引入固定偏差，需要在板上量一次 MUX 通道的静态码值来判定。

### 4.6 20 kHz ISR 时间余量未实测

本步在 ISR 里增加了 20 次寄存器读、约 25 次乘加、7 次地址比较。理论估算 < 3 µs，但 C2000 上
`float` 乘法与结构体成员访问的开销必须实测。**如果实测逼近 50 µs 的 50%，后续 L3~L5 就不能再
放在 20 kHz ISR 里**，需要改成 1 kHz 任务。这条会决定第 02 步的分层方式，请一并留意。

### 4.7 ADC 触发源分散在多个 ePWM 上：不阻塞，但“锚点”依赖固定优先级（已澄清，m00557）

**初稿结论作废。** 初稿称“某个 SOC 等不到触发就会阻塞后面所有 SOC”，据此推断 PV1~PV4 电流的
EPWM7/9/5/6 触发会把 ADC-A/C/D 的后续 SOC 一起带走、主中断相位跟随 EPWM7。经查 TRM
（SPRUHX5G §10.5／§10.7 与 Table 10-24）确认：**不成立，初稿判断错误，撤回。**

真实机制：

1. 触发事件只做一件事——把对应 SOC 的转换请求标志置起（`ADCSOCFLG`）。
2. ADC 只在**已置位**的 SOC 里挑下一个转换对象；从未收到触发的 SOC 直接被跳过，**不会阻塞**任何
   其它 SOC。因此不同触发源之间互不抢占，各自在自己的事件上插入一次转换。
3. 挑哪一个，由 `ADCSOCPRICTL.SOCPRIORITY` 决定（Table 10-24）：
   - `00h`：全部 round-robin，按 `RRPOINTER` 轮转，**顺序不固定**；
   - `10h`：全部高优先级，“arbitrated by SOC number”，即按 SOC 编号升序。

本工程 `Drivers/src/Hw_AdcDriver.c:213` 写的是 `pstRegs->ADCSOCPRICTL.bit.SOCPRIORITY = 16U;`（= `10h`），
四个 ADC 都经过这一句。所以 **ADC-A 的 SOC15 恒排在其它 EPWM1 触发 SOC 之后**，`ADCINT1` 的
“序列末位锚点”假设成立。

时间预算（各单位为 ACQPS+1 的量级，按编号升序累加）：

| ADC | EPWM1 触发的 SOC（ACQPS） | 最晚完成者 |
|---|---|---|
| ADC-A | SOC0(30)、SOC3(60)、SOC5(60)、SOC6(60)、SOC7(60)、SOC15(120) | SOC15 锚点 |
| ADC-B | SOC0(60)、SOC1(60)、SOC2(60)、SOC3(120) | SOC3 = B3（MUX2） |
| ADC-C | SOC3(60)、SOC4(60) | SOC4 = C4 |
| ADC-D | SOC0(30)、SOC1(30)、SOC2(60)、SOC3(120) | SOC3 = D3（MUX1） |

ADC-B/C/D 的最晚者都早于 ADC-A 的 SOC15，因此 `ADCINT1` 到来时，全部 EPWM1 触发通道都已转换完成，
`AdcSampleFast()` 读到的 20 个寄存器是**同一拍**的快照。（ACQPS 只占一部分开销，还有采样窗与 12 位
转换的固定节拍；若将来把某个 SOC 的 ACQPS 加大，需要重新核算这张表。）

必须守住的约束（写在这里防止被“清理”）：

- **不要把 `SOCPRIORITY` 改回 `0`（round-robin）。** 一旦改成轮转，SOC15 就不再恒为最后，
  锚点失效，ISR 会读到混拍的码值，MUX 归档还会把上一拍的电平记到本拍地址上。
- 给 ADC-A 增删 SOC 时，必须保证没有比 SOC15 更晚完成的 EPWM1 触发通道。

PV1~PV4 电流确实是在各自 ePWM（EPWM7/9/5/6）的时刻采样，而不是 EPWM1 的 ZERO，因此这 4 路与其余
16 路之间存在相位差：对 L1/L2（“最近一次转换结果”）无影响，对 L3/L4 的整周期块平均与 RMS 也无影响
（统计量对采样相位不敏感）；只有将来做“同一时刻的瞬时功率”时才需要注意。

**结论：现有触发源配置不是缺陷，不需要改动，本项不再请求裁定。**

### 4.8 12 位码值约定：分母用 4096 还是 4095（已裁定，m00630）

问题：原表用 `2047.5` 作 1.5 V 零点、`4095` 作满量程分母，与 TI 的 `ADCRESULT = 4096·Vin/Vref` 对不上；
而 12 位寄存器最大只有 4095，看起来“除 4096”没有对应的码。

裁定：**统一按 4096**。依据 TRM SPRUHX5G：

- Table 10-2：`VREFLO < Vin < VREFHI` 时 `ADCRESULT = 4096·(Vin − VREFLO)/(VREFHI − VREFLO)`，小数截断；
  `Vin ≥ VREFHI` 时 `ADCRESULT = 4095`（饱和）。
- Table 10-3：反算 `Vin = (VREFHI − VREFLO)·(ADCRESULT + 0.5)/4096`，原文说明这是“the center of the
  possible range of analog voltages that could have produced this conversion result”。

即 4095 是“最后一格”，代表 `[4095/4096·3 V, 3 V]` 这个区间，不是“3 V 的码值”；4096 才是满量程。
每格 3 V/4096 = 732.4 µV。

落地：

- `ADC_CAL_CODE_FULL` 4095.0f → **4096.0f**；`ADC_CAL_CODE_MID` 2047.5f → **2048.0f**；
  `ADC_CAL_CODE_PV_MID` 2275.0f → **2275.56f**（= 4096·5/9）。
- 各 `ADC_CAL_K_*` 宏本来就是这三个宏的表达式，自动跟随，无一处重新写死数值。
- `ADC_CAL_ZERO_TOL` 保持 307.0f（2048 的 15% = 307.2）。
- 数值影响：系数变化 ≤0.036%；满幅读数 462.69 V → 462.46 V、1033.0 V → 1032.75 V、
  PV 电流系数 0.016483 → 0.016480。均低于板级精度，**改的是语义正确性不是精度**：
  2047.5 这种“半个码”在实测中不存在，留着会让后来人按 TI 公式复核时怀疑整张标定表。
- 零点仍由上电 200 ms 实测平均得到，任何自洽的码域约定都能工作；`+0.5` 半格在相减时自消。

同步：`Broad/inc/DrvCfg_Sample.h` 头部量程注释与宏、`Platform/Sample/Inc/AdcCalib.h:37` 注释、
本文件 §2.6/§2.7/§4.4、仓库外规格书 §3.7.4 的常量表。

## 审阅结论（由 Codex 填写）

（待填）

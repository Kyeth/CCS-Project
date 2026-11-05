# 步骤 02：采样数据层 L3（块平均）／L4（交流周期值）／L5（显示值）

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`
- head_sha：工作区待审
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：本仓库没有正式缺陷编号，沿用步骤 01 的做法自编验收项 `S-02-01` ~ `S-02-07`
- 状态：**待审（代码已落地，构建通过；板上项未测）**

> v3 变更（对应 m00721）：⑦ `stPLLData` 的定义**长期留在 `Platform/Pll/Inc/PllData.h`**，PLL 模块落地后
> 继续补在这个文件里，不搬迁；⑧ 半周期平均改为**按频率自适应**，`Avg200` 更名 `AvgHalf`
> （50 Hz → 200 拍，60 Hz → 167 拍）。
>
> v2 变更（对应 m00702）：① 额定频率改为运行时可切换的 50 Hz／60 Hz 模式；② 角度不再由采样层自制占位
> 结构，直接读全局 `stPLLData.ThetaR/ThetaS/ThetaT`，窗口是否定长由 PLL 内部决定，采样层不关心；
> ③ 功率观测值纳入本步；④ MUX 平均由 32 轮降为 16 轮；⑤ L5 用一级 IIR；⑥ 显示层统一用 `Display` 命名。
> （v1 的 `ST_SAMPLE_SYNC`、`fThetaGrid/fThetaInv/fGridFreq` 与“定长兜底”方案作废。）

---

## 意图与边界

### 问题

步骤 01 交付了 L1（20 kHz 原始码值）与 L2（减去零漂、乘系数后的瞬时工程值）。这两层都是“最近一拍的
瞬时值”，下游无法直接使用：

- 瞬时值里混着开关纹波、ADC 噪声和单点毛刺，控制环拿它做反馈会被纹波带跑；
- 10 ms 级以上的量（RMS、平均功率、母线电压）现在没有任何承载物；
- IB800U 里 `Sum8/Sum200/Rms/Filter8/Filter200` 那一套分层（见 `reference/IB800U` 与
  `origin/deepseek/pcs-stepwise` 的 `SampleData.h`），在 EP15K 上还没有对应物；
- 周期量（RMS、峰值）需要一个“窗口”才能算，而窗口的边界只能来自角度；目前全项目没有角度变量。

### 本步目标

1. **L3 块平均**：8 拍（400 µs）固定块与半周期块（50 Hz 200 拍／60 Hz 167 拍）；MUX 通道按轮次平均
   （16 轮 ≈ 5.6 ms）。
2. **L4 交流周期值**：三相 RMS 与峰值，窗口由全局角度 `stPLLData.ThetaR` 回绕收口。
3. **L5 显示值**：1 kHz 任务里对 L3／L4 做一级强滤波，统一用 `Display` 命名。
4. **频率模式**：额定频率作为运行时变量（默认 50 Hz），支持 50 Hz ↔ 60 Hz 切换；半周期块长度与窗口
   上限都随它自适应。
5. **功率观测**：逐相 `u·i` 与三相和，随 L3 一起发布。

### 明确不做（本步边界）

- **不实现锁相环、不产生角度、不写 `stPLLData` 的任何字段。** 角度是外来的输入：离网用自身频率的 θ、
  并网用电网 θ，这件事在 PLL 内部处理，采样层只读同一个变量。
- 不做保护判定、不做 Modbus/SCI/CAN 上报、不做软示波器录波（`BusBuffer[240]` 那类留到单独一步）。
- 不做 50/60 Hz **自动识别**：本步只做“可切换”，切换者由上层（状态机／MIM 指令）调用接口。
- 不测量、不发布电网频率：本步只保留“额定频率”这个配置量。将来 PLL 若同时算出电网频率，由 PLL
  模块自行决定放哪，采样层不预置字段。
- 不改 ADC 配置表、不改 MUX 环、不改 PWM、不改 CLA（CLA 仍然只转发 MUX 地址）。

### 可能影响的功率输出

无直接影响：本步不碰 PWM、ADC 触发、TZ、CLA。间接影响是 CPU 时间——20 kHz ISR 里增加约 42 次浮点
加法、9 次平方累加、9 次峰值比较，1 kHz 任务里新增约 30 次乘加。步骤 01 §4.6 已声明 ISR 时间余量尚未
实测，本步刻意把**所有除法和开方都放到窗口结束的那一拍**，把 L5 全部放到 1 kHz 任务。

---

## 修改前后

### 2.1 节拍与执行位置

| 层 | 节拍 | 执行位置 | 每个 tick 做的事 |
|---|---|---|---|
| L1 原始码值 | 20 kHz | `Isr_Control` → `AdcSampleFast()` | 读 20 个 `ADCRESULT`、MUX 归档、`u32Seq++` |
| L2 瞬时工程值 | 20 kHz | 同上 | `(code − zero) × fK`，零漂未就绪时不写 |
| L3 块平均 | 20 kHz 累加，按块发布 | 同上 | 13 路（Avg8）+ 22 路（AvgHalf）+ MUX 7 路累加；计数到 8／`u16HalfSteps`／16 轮时做乘除并发布 |
| L4 交流周期值 | 20 kHz 累加，窗口结束发布 | 同上 | 9 路平方累加 + 9 路峰值比较；θ 回绕时 9 次除法 + 9 次开方 |
| L5 显示值 | 1 kHz | `Task_Run()` → `AdcSampleTask()` | 每路一次一级 IIR |

1 kHz 任务由 `main.c` 的 `TaskCreate()` 里 `Task_Start(AdcSampleTask, 1U, 0U)` 挂上；`Task_OnTick()` 已经
在 `Isr_Heartbeat`（`main.c:164`）里每 1 ms 调用一次，不需要新增定时器。

> 接口签名注意：`Platform/Schedule/Inc/Schedule.h:12` 的回调类型是 `void (*callback)(void)`，所以本步定义的是
> `void AdcSampleTask(void)`，而不是 IB800U／pcs-stepwise 分支里的 `void AdcSampleTask(void *pArgV)`。
> 两者不能混用，接错会在调度表里产生类型不匹配。

### 2.2 角度输入 `stPLLData`（全局唯一，采样层只读）

按 m00702 的裁定：全局只需要一个角度来源，字段名固定为 `ThetaR / ThetaS / ThetaT`。离网时它是自身
频率对应的角度，并网时它是 PLL 锁出来的电网角度——**对采样层是同一个变量，不区分工况**。

PLL 代码目前不存在，因此本步先在 `Platform/Pll/` 里落一个数据结构把它占住；按 m00721 的确认，
**PLL 模块落地后继续补在这个文件里**（往结构里加字段可以，改名或搬迁不行），不另建头文件：

```c
/* Platform/Pll/Inc/PllData.h —— 角度与频率的唯一来源。写入方是 PLL 模块，其它模块只读。 */
typedef struct
{
    float  ThetaR;      /* A/R 相角度，rad，0 ~ 2π，DSP 每拍递增 */
    float  ThetaS;      /* B/S 相角度，rad */
    float  ThetaT;      /* C/T 相角度，rad */
} ST_PLL_DATA;

extern volatile ST_PLL_DATA stPLLData;
```

使用规则（写进头文件注释）：

1. `stPLLData` 的**唯一写入方**是 PLL 模块。采样层只读，`SampleDataInit()` 只把它清零。
2. 采样层只用 `ThetaR` 做窗口收口（三相角度相差 120°，用同一相即可）；`ThetaS/ThetaT` 供控制环用。
3. 角度合法性：`0 ≤ θ < 2π + 0.5` 判为合法；越界视为“θ 未运行”，按 §2.5 的强制收口处理。
4. 采样层**不判断工况**（离网/并网），也不做频率切换判断——那些都在 PLL 与状态机内部完成。

### 2.3 频率模式（运行时 50 / 60 Hz 切换）

`Broad` 层只给默认值，运行值放在 `Platform` 的配置实例里，可被上层改写：

```c
typedef enum
{
    SAMPLE_FREQ_MODE_50HZ = 0U,
    SAMPLE_FREQ_MODE_60HZ = 1U
} E_SAMPLE_FREQ_MODE;

typedef struct
{
    float  fNominalFreq;    /* 50.0f / 60.0f，窗口上限、半周期块长度与显示都用它 */
    Uint16 u16FreqMode;     /* E_SAMPLE_FREQ_MODE */
    Uint16 u16HalfSteps;    /* 半周期块长度：50 Hz → 200，60 Hz → 167 */
} ST_SAMPLE_CFG;

extern volatile ST_SAMPLE_CFG stSampleCfg;

void     SampleSetFreqMode(Uint16 u16Mode);        /* 50/60 切换，同时刷新半周期块长度与窗口上限 */
Uint16   SampleSetNominalFreq(float fNominalFreq); /* 任意额定频率（如 45~65 的实测值），成功返回 1 */
```

- 默认值来自 `Broad/inc/DrvCfg_Sample.h` 的 `ADC_CAL_F_NOMINAL (50.0f)`。
- 两个设置接口都同时刷新三个派生量：`fNominalFreq`、`u16HalfSteps = round(20000 / f / 2)`、
  `u16WinMaxSamples = round(20000 / f × 1.25)`（后者属 L4，定义见 §2.5）。
- 切换生效点：立即写入 `stSampleCfg` 并重算派生量；**当前正在累加的块不打断**，用块开始时的长度收口
  （避免半块平均）。L4 的 RMS 窗口本来就按 θ 收口，不受块长度变更影响。
- `SampleSetNominalFreq()` 把入参钳在 `ADC_CAL_F_FREQ_MIN (45.0f)` ~ `ADC_CAL_F_FREQ_MAX (65.0f)`，
  越界时钳位并返回 0。
- 本步不提供自动识别；若要“并网后自动跟随电网频率”，由上层把 PLL 算出的频率喂进
  `SampleSetNominalFreq()` 即可，不需要改本层代码。

### 2.4 L3 块平均规格

| 档位 | 样本数 | 时间 | 用途 |
|---|---|---|---|
| `Avg8` | 8 拍（固定） | 400 µs | 去掉开关纹波，给控制环用的“当前值” |
| `AvgHalf` | `u16HalfSteps` 拍（50 Hz → 200、60 Hz → 167） | 10 ms／8.33 ms | 半个工频周期，有功／直流分量 |
| `AvgMux` | 16 轮 | 5.6 ms | MUX 通道（每地址 350 µs 采一次） |

`AvgHalf` 为什么自适应（m00721 裁定）：半周期块的意义是“把工频一个半周内的纹波积掉”，固定 200 拍在
50 Hz 下正好是半周，到 60 Hz 就只剩 1/3 周，直流分量分离会变差；改成随额定频率算长度后，两种模式下都
是严格的半周。旧名 `Avg200` 随之更名，避免名字里写死一个会变的数。

通道清单：

| 组 | 通道 | Avg8 | AvgHalf |
|---|---|---|---|
| 逆变电流／逆变电压／电网电压 | R/S/T 三相，各 3 路 | ✔ | ✔ |
| 逆变功率（`u·i` 逐相 + 三相和） | 4 个量 | ✔ | ✔ |
| 逆变电流直流分量 | R/S/T | — | ✔ |
| PV1~4 电流 | 4 路 | — | ✔ |
| 平衡电路电流、GFCI 电流 | 2 路 | — | ✔ |
| MUX 通道（PV1~4 电压、VISO、BUSL、BUSH） | 7 路 | 走 `AvgMux` | 走 `AvgMux` |

MUX 通道的节拍不同：每个地址 350 µs 才采一次（环长 7 拍 ≈ 2.86 kSa/s），不能按 20 kHz 拍数算。做法是：
**以 MUX 环回到地址 0 记作一轮（350 µs），累计 16 轮 ≈ 5.6 ms 发布一次**
`fPvVoltAvgMux[4] / fVisoVoltAvgMux / fBusLAvgMux / fBusHAvgMux`（m00702 裁定由 32 轮降为 16 轮）。

发布语义：三类块独立计数、独立发布；发布值在下一个块到来前保持不变，下游任何时刻读到的都是“最近一个
完整块”，不会读到半块。

**控制取用哪一层（m00702 追问的回答）：** 电流环、电压环拿 **L2 瞬时值**（逐拍，50 µs）或 **L3 `Avg8`**
（400 µs，用来去开关纹波）；`AvgHalf` 用于有功／直流分量；MUX 的 5.6 ms 平均只用于母线／PV 电压这类慢
变量（监测、保护、显示）。如果将来某一控制环需要母线电压比 5.6 ms 更快（例如 20 kHz 母线均压），
**那一路必须改由 L2 直接提供，不能走 MUX 平均**——已在 §4.4 标为遗留观察项。

### 2.5 L4 交流周期值规格

被统计的量：逆变电流 R/S/T、逆变电压 R/S/T、电网电压 R/S/T，共 9 路（PV 电流、母线是直流量，RMS 等于
均值，已被 L3 `AvgHalf` 覆盖）。

**窗口只按角度收口，不做定长方案**（m00702 裁定：离网/并网的角度差异由 PLL 内部处理，采样层不关心）：

1. 每拍读 `stPLLData.ThetaR`，与本模块保存的上一拍比较：`θ_prev > 5.0f && θ_now < 1.0f`（弧度，`2π ≈ 6.283`）
   判为回绕，**回绕的这一拍就是窗口边界**。
2. 边界样本归属下一窗口：收口的那一拍先结算上一窗口，再把自己作为新窗口的第 1 个样本。
3. **强制收口（安全网）**：若窗口内样本数达到上限 `u16WinMaxSamples` 仍未看到回绕（θ 没动、PLL 未跑、
   或频率极低），强制结算并置 `u16Valid` 的 `SAMPLE_AC_VALID_FORCED` 位。
   `u16WinMaxSamples = round(ADC_SAMPLE_FAST_HZ / fNominalFreq × 1.25f)`：50 Hz → 500 拍，60 Hz → 417 拍，
   再统一钳在 `ADC_SAMPLE_RMS_WIN_MIN (10)` ~ `ADC_SAMPLE_RMS_WIN_MAX (600)`。
4. 正常收口时置 `SAMPLE_AC_VALID_THETA`；此时 `u16WinSamples` 就是实测的每周期样本数，用
   `ADC_SAMPLE_FAST_HZ / u16WinSamples` 可反算实际频率（只作调试观测，不发布为“电网频率”）。

发布结构：

```c
#define SAMPLE_AC_VALID_RMS     (0x0001U)
#define SAMPLE_AC_VALID_PEAK    (0x0002U)
#define SAMPLE_AC_VALID_THETA   (0x0004U)   /* 由角度回绕收口 */
#define SAMPLE_AC_VALID_FORCED  (0x0008U)   /* 靠上限强制收口，值仅供参考 */

typedef struct
{
    float  fInvCurrRms[SAMPLE_PH_NUM];    /* A */
    float  fInvVoltRms[SAMPLE_PH_NUM];    /* V */
    float  fGridVoltRms[SAMPLE_PH_NUM];   /* V */
    float  fInvCurrPk[SAMPLE_PH_NUM];     /* A，窗口内 |x| 最大值 */
    float  fInvVoltPk[SAMPLE_PH_NUM];
    float  fGridVoltPk[SAMPLE_PH_NUM];
    Uint16 u16WinSamples;                 /* 本窗口实际样本数 */
    Uint16 u16WinCnt;                     /* 当前窗口已累加样本数（调试） */
    Uint16 u16WinMaxSamples;              /* 强制收口上限，随频率模式刷新 */
    Uint16 u16Valid;
} ST_SAMPLE_AC;
```

数值安全：

- RMS 用 `sqrtf(Σx² / N)`，**不做去均值**——零点已在 L2 扣除，再去均值会把真实直流分量也扣掉。
- 平方累加用 `float`：单路最坏 600 个样本 × 462.69² ≈ 1.28e8，`float` 相对误差 < 1e-6，可接受；不用
  `Uint32` 定点（平方会溢出）。
- 开方与除法只在窗口边界做（约每 400 拍一次），不在每拍路径上。

### 2.6 L5 显示值规格（统一 `Display` 命名）

1 kHz 任务里对下列量做一级 IIR：

```c
fDisplay = fDisplay + fAlphaDisplay * (fSrc - fDisplay);
```

- 默认 `fAlphaDisplay = 0.005f`（1 kHz 下 τ ≈ 200 ms，−3 dB ≈ 0.8 Hz），作为 `Broad` 层常量可调。
- 结构体统一命名为 `ST_SAMPLE_DISPLAY` / `stSampleDisplay`，字段一律 `f<源><相或序号><量>Display`。
- 参与量：三相 RMS（逆变电流／逆变电压／电网电压）、PV1~4 电压、VISO、BUSL／BUSH、PV1~4 电流、
  平衡电流、GFCI 电流。电压类取 `AvgMux`，电流类取 `AvgHalf`，RMS 类取 `stSampleAc`。
- 首拍处理：上电第一拍直接 `fDisplay = fSrc`，避免从 0 慢慢爬。

### 2.7 新增／修改文件

| 文件 | 动作 | 内容 |
|---|---|---|
| `Platform/Pll/Inc/PllData.h` | **新增** | `ST_PLL_DATA`（`ThetaR/ThetaS/ThetaT`）定义、外部声明、所有权注释（长期居所，m00721） |
| `Platform/Pll/Src/PllData.c` | **新增** | `stPLLData` 实例定义（占位，待 PLL 模块接管写入） |
| `Platform/Sample/Inc/SampleData.h` | 改 | 新增 `E_SAMPLE_FREQ_MODE` / `ST_SAMPLE_CFG`（含 `u16HalfSteps`）/ `stSampleCfg` 与两个设置接口声明；把步骤 01 只建壳的 `ST_SAMPLE_FLT`／`ST_SAMPLE_AC`／`ST_SAMPLE_DSP` 填成 §2.4~§2.6 的字段，`ST_SAMPLE_DSP` 更名为 `ST_SAMPLE_DISPLAY`，`Avg200` 相关字段更名为 `*AvgHalf` |
| `Platform/Sample/Src/SampleData.c` | 改 | 新增 `stSampleCfg` 实例与两个接口实现；`SampleDataInit()` 一并清零 `stPLLData` |
| `Platform/Sample/Inc/AdcSample.h` | 改 | 新增 `ADC_SAMPLE_RMS_WIN_MIN/MAX`、`ADC_SAMPLE_MUX_AVG_STEPS (16U)`；声明 `void AdcSampleTask(void);` |
| `Platform/Sample/Src/AdcSample.c` | 改 | L3 累加与发布（半周期块按 `u16HalfSteps` 判满）、L4 平方／峰值／θ 收口、`AdcSampleTask()` 的 L5 |
| `Broad/inc/DrvCfg_Sample.h` | 改 | 新增 `ADC_CAL_F_NOMINAL (50.0f)`、`ADC_CAL_F_FREQ_MIN/MAX (45.0f/65.0f)`、`ADC_CAL_ALPHA_DISPLAY (0.005f)` |
| `main.c` | 改 | `TaskCreate()` 里 `Task_Start(AdcSampleTask, 1U, 0U);` |
| `tools/build_minimal.py` | 改 | SOURCES 增 `Platform/Pll/Src/PllData.c`，INCLUDES 增 `Platform/Pll/Inc` |
| `.cproject` | 改 | 两套配置各加一条 `${PROJECT_ROOT}/Platform/Pll/Inc` |
| `docs/minimal-bringup.md` | 改 | 补 L3~L5、角度输入、频率模式一段，更新编译单元数与验收步骤 |
| `EP15K_2026_配置核心与运行逻辑.md`（仓库外） | 改 | §3.7 追加 3.7.5「L3~L5、角度输入与频率模式」 |

不改：`Broad/src/DrvCfg_EP15K.c`、`Drivers/`、`Platform/Status/`、`CMD/`、CLA 相关文件。

### 2.8 内存与时间预算（估算，待构建后核对 map）

| 结构 | 估算字数 | 位置 |
|---|---|---|
| `ST_PLL_DATA` + `ST_SAMPLE_CFG` | 3 + 3 | 默认 `.bss` |
| `ST_SAMPLE_FLT` | ≈ 46（22 个发布值 + 22 个累加器 + 计数器） | 默认 `.bss` |
| `ST_SAMPLE_AC` | ≈ 34（18 个发布值 + 9 个平方累加器 + 9 个峰值 + 4 个控制量） | 默认 `.bss` |
| `ST_SAMPLE_DISPLAY` | ≈ 22 | 默认 `.bss` |

合计约 110 字。新增 1 个编译单元 → 30 个。若 `.bss` 增长挤占内存，再决定是否搬到 `RAMGS0`。

20 kHz ISR 增量：13 + 22 + 7 = 42 次加法（块平均）+ 9 次乘加（平方）+ 9 次比较（峰值）+ 计数器比较 +
1 次 θ 比较，约 75 条 FPU 指令，按 200 MHz、FPU 单周期估算 < 0.5 µs；除法与 `sqrtf` 只在窗口边界出现。
1 kHz 任务约 30 次乘加，可忽略。

### 2.9 与 IB800U 的对应关系

| IB800U / pcs-stepwise | 本步 | 说明 |
|---|---|---|
| `ADC_SAMPLE_INS.Sum8` | `stSampleFlt` 的 `Avg8` | 由“求和”改为“直接存平均值”，省掉下游每次除以 8 |
| `ADC_SAMPLE_INS.Sum200` | `stSampleFlt` 的 `AvgHalf` | 同理，且长度随额定频率自适应（50 Hz 200 拍／60 Hz 167 拍） |
| `ADC_SAMPLE_INS.BusBuffer[240]` | 不做 | 录波，留到软示波器那一步 |
| `ADC_SAMPLE_RMS.Rms / Filter8 / Filter200` | `stSampleAc` 的 `*Rms` + `stSampleDisplay` | 旧代码两级滤波，本步合成一级 IIR（m00702 裁定） |
| `SAMPLE_DISPLAY` | `ST_SAMPLE_DISPLAY` | 同名同职责，字段加 `Display` 后缀 |
| `CpuToCla1Msg_Str.theta` | `stPLLData.ThetaR` | 旧代码把角度送进 CLA；本步 CLA 不参与，角度留在 CPU 全局 |

---

## 验证

### 构建（已跑）

```sh
cd CCS-Project-minimal
python tools/build_minimal.py --cgt D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS
```

结果：**30 个编译单元全部成功、零 warning、链接成功**。

| 产物 | 大小 | sha256 |
|---|---|---|
| `CCS-Project-minimal-build/EP15K_minimal.out` | 671,592 字节 | `02021ab8912c9d0f8ad8f05e7c711ef8febc56b4419e0bdbb411419361544e88` |
| `CCS-Project-minimal-build/EP15K_minimal.map` | 83,429 字节 | `5adcad2e62605cbbefc7b8713dd683162077cd8f6124aaa834cb2f055f0b0326` |

map 核对：

| 项 | 要求 | 实测 |
|---|---|---|
| `CLA1_MSGRAMLOW` | 4 字不变（CLA 契约未变） | 4 |
| `CLA1_MSGRAMHIGH` | 9 字不变 | 9 |
| `RAMLS5` | 2 字不变（CLA 数据区未长） | 2 |
| `RAMGS0`（`.data` + `.bss` 落点，0xc000，length 0x1000） | 有余量 | 占用 0x66C，余 0x994 |

关键符号：`stPLLData 0xc3fa`、`stSampleCfg 0xc400`、`stSampleRaw 0xc404`、`stSampleDisplay 0xc440`、
`stSampleAc 0xc500`、`stSampleFlt 0xc580`；`__relaxed_sqrtf` 9 字（`AdcSample.obj` 引入，只在窗口边界调用）；
`FLASH_APP` 占用 0x335B。

### 静态核对（已跑，未上板）

| 项 | 判据 | 结果 |
|---|---|---|
| `AdcSampleTask` 签名 | 与 `Platform/Schedule/Inc/Schedule.h:12` 的 `void (*callback)(void)` 一致 | 通过 |
| 1 kHz 任务挂载 | `TaskCreate()` 里有且只有一处 `Task_Start(AdcSampleTask, 1U, 0U)` | 通过（`main.c:116`） |
| `stPLLData` 不被写 | 写入点只有 `SampleDataInit()` 的清零与 `PllData.c` 的定义 | 通过（`Platform/Sample/Src/SampleData.c:95-97`、`Platform/Pll/Src/PllData.c:15`；读点 `Platform/Sample/Src/AdcSample.c:361`） |
| 频率模式 | `SampleSetFreqMode()` 之后 `u16HalfSteps`／`u16WinMaxSamples` 立即 200/500 ↔ 167/417 | 通过（20000/60/2 = 166.67 → 167；20000/60×1.25 = 416.67 → 417） |
| 频率钳位 | `SampleSetNominalFreq(100.0f)` 被钳到 65.0f 并返回 0 | 通过（`SampleCfgApply()` 内钳位，越界返回 0） |
| 半周期块自适应 | 判满用 `stSampleCfg.u16HalfSteps`，不用常量 200 | 通过 |
| 窗口收口 | θ 回绕置 `SAMPLE_AC_VALID_THETA`；θ 不动到上限强制收口并置 `VALID_FORCED` | 通过（`AdcSampleAcStep()`） |
| 除法／开方位置 | 20 kHz 路径上没有 `/`、没有 `sqrtf` | 通过（只在 `AdcSampleAcClose()`／`AdcSamplePublishHalf()` 出现） |
| MUX 平均 | 地址 7 采完记一轮，16 轮发布一次 | 通过（`SAMPLE_AVG_MUX_STEPS` = 16，≈5.6 ms） |

### 偏离（实现相对本申请 v3）

1. `SAMPLE_AVG8_STEPS`／`SAMPLE_AVG_MUX_STEPS` 放在 `SampleData.h`（紧挨它们约束的累加器），
   本文件 §2.7 原写为 `AdcSample.h` 的 `ADC_SAMPLE_MUX_AVG_STEPS`；命名与 `SAMPLE_AVG8_STEPS` 对齐。
2. `ST_SAMPLE_FLT` 的累加器拆成三个子结构 `ST_SAMPLE_AVG8_ACC`／`ST_SAMPLE_HALF_ACC`／`ST_SAMPLE_MUX_ACC`：
   Avg8 与 AvgHalf 共用同一批通道但块长不同，必须各累加各的，原设计只有一组。
3. 内存估算修正：C28x 的 `float` 占 **2 字**，§2.8 的“约 110 字”低估一半。实测 `SampleData.obj (.bss)`
   = 0x22C = 556 字，`.bss` 合计 0x29A，落在 `RAMGS0`（0x66C / 0x1000）；余量够用，本步不做搬迁。
4. 规格书改动落在 `D:\MinMain\EP15K_2026_配置核心与运行逻辑.md` 的 §3.7.4 内（该节本就是 CPU 侧采样链），
   未按 §2.7 表新开 3.7.5。

### 板上项（一律标“未测”）

- S-02-01：`stSampleFlt` 的 `Avg8` 与示波器读到的相电流包络一致（误差 < 2%）。
- S-02-02：`AvgHalf` 在直流侧（PV 电流）与万用表读数一致（误差 < 1%），50 Hz 与 60 Hz 两种模式各测一次。
- S-02-03：`stSampleAc` 的 RMS 与钳形表／功率分析仪对比（误差 < 2%，含标定误差）。
- S-02-04：θ 由外部（调试器写 `stPLLData.ThetaR` 递增）驱动时，`u16WinSamples` 等于实测周期样本数，
  且 50/60 Hz 两种模式的强制上限分别为 500／417。
- S-02-05：20 kHz ISR 时间增量 < 1 µs（GPIO 翻转 + 示波器或 CPU 计时器）。
- S-02-06：`stSampleDisplay` 的阶跃响应时间常数 ≈ 200 ms。
- S-02-07：功率：`fInvPwrAvgHalf` 三相和与功率分析仪有功读数对比（误差 < 2%）。

### 未跑项

- `tools/verify_config.py` 仍需 `--pin-sheet Z:/...`（Z: 盘未挂载），本步不涉及管脚，预期结论不变。

---

## 风险与请求裁定

### 4.1 额定频率 50/60 Hz（已裁定，m00702）

默认 50 Hz，运行时可切换（`SampleSetFreqMode`），并支持写入任意额定频率（`SampleSetNominalFreq`，钳在
45~65 Hz）。**不做自动识别**；若要并网后自动跟随电网频率，由上层把 PLL 算出的频率写进来。

### 4.2 角度来源与窗口收口（已裁定，m00702）

全局唯一角度 `stPLLData.ThetaR/ThetaS/ThetaT`；离网角度来自自身频率、并网来自 PLL，工况判断在 PLL 内部
完成，采样层不关心，也不做定长兜底。窗口只用 θ 回绕收口，外加 `u16WinMaxSamples` 强制上限作为安全网
（θ 未跑时不至于永不结算）。

### 4.3 功率观测值（已裁定，m00702：纳入本步）

逐相 `u·i` 与三相和，随 `Avg8`／`AvgHalf` 一起发布；不区分瞬时／有功／无功，本步只给 `u·i` 的块平均。

### 4.4 MUX 平均轮次与控制取用层（已裁定，m00702：16 轮）

`AvgMux` = 16 轮 ≈ 5.6 ms。控制环仍用 **L2 瞬时值**（50 µs）与 **`Avg8`**（400 µs）；MUX 的 5.6 ms 平均只
用于母线／PV 电压的监测、保护与显示。

**遗留观察项：** 若将来母线电压需要进入 20 kHz 级控制环（母线均压、母线前馈），5.6 ms 的刷新率不够，那
一路要改由 L2 直接提供（`u16BusLCode/u16BusHCode` 本来就在每拍 L2 里，随时可取）。本步不预先实现，等控制
环定稿再决定。

### 4.5 L5 用一级还是两级滤波（已裁定，m00702：一级）

一级 IIR，`fAlphaDisplay = 0.005f`，τ ≈ 200 ms。不保留“快看／慢看”两档；若以后需要，在
`ST_SAMPLE_DISPLAY` 里加一组 `*FastDisplay` 字段即可，不影响现有结构。

### 4.6 承接步骤 01 §4.6 的 ISR 时间余量

步骤 01 已提过：20 kHz ISR 现在有 20 次寄存器读 + 25 次乘加，本步再加约 75 条 FPU 指令。若实测逼近
50 µs 的一半，回退顺序：① L4 平方累加改为每 2 拍一次（RMS 误差可忽略）；② `Avg8` 由 8 拍改 4 拍；
③ `AvgHalf` 移到 1 kHz 任务（会牺牲纹波抑制）。请先测 S-02-05。

### 4.7 命名（已裁定，m00702：Display）

显示层统一 `ST_SAMPLE_DISPLAY` / `stSampleDisplay` / `f*Display`。注意 `stPLLData` 的字段名
（`ThetaR/ThetaS/ThetaT`）是所有者指定的写法，与项目内 `f` 前缀的命名规则不一致，**按指定保留**，并在头
文件注明来源（m00702），避免后续被“统一命名”改掉。

### 4.8 `stPLLData` 的定义权与半周期块长度（已裁定，m00721）

1. `stPLLData` 的定义**长期留在 `Platform/Pll/Inc/PllData.h`**，PLL 模块落地后继续补在这个文件里（加字段
   可以，改名/搬迁不行）。
2. `AvgHalf` 按额定频率自适应：`u16HalfSteps = round(20000 / fNominalFreq / 2)`，50 Hz → 200，
   60 Hz → 167；切换频率模式时立即刷新，当前块不打断。原 `Avg200` 名称与写死 200 的实现作废。

---

## 审阅结论（由 Codex 填写）

（待填）

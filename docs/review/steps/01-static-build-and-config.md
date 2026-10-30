# 步骤 01 — 静态构建与配置复核（A01–A03）

| 字段 | 值 |
| --- | --- |
| base_sha | `b20258bd2260ac4aafcdb510c41b7a0483328987`（已接受的步骤 01 首版提交，本次返工的比对基线） |
| head_sha | 待提交 |
| original_baseline | `57f94362aab18df739b40f7109b1e6987745224c`（`pcs-baseline-20261029`） |
| 关联缺陷 | F01、F02、F03、F04、F05、F09（只核对，不修） |
| 状态 | 待审阅（v2，按 Codex 审阅意见返工） |

## 0. 本轮返工范围（相对 `b20258b`）

只改审阅脚本、期望值数据、文本证据和本报告，**固件 0 行改动**。具体：

1. **复现命令与脚本参数对齐**：报告里的三条命令改为可直接粘贴执行的真实参数（`--repo`/`--workdir`/`--rev`；`--src`/`--rev`/`--out`）。
2. **固定证据所用源码**：构建与配置重放不再读工作区，而是先把 `--rev` 解析成完整提交 SHA，再用 `git archive <pinned_sha>` 解包、配置重放只读该解包树；两份证据文件都记录这个 SHA 与逐文件 SHA-256。
3. **A03 判据收窄**：SOC15 完成屏障、INT1/INT2 触发 SOC、PV 交错相位、BUS 初始占空比从"通过"改为"待裁定"；新增两条独立检查把"值一致"和"设计已批准"分开。
4. **更正 GLD 描述**：不是"直写"，而是全局装载被运行时关闭、本地 `CTR_ZERO` 阴影装载仍在。
5. **更正 BUS 保护描述**：不是"只有软件轮询"，而是启动时用 `TZFRC.OST` 硬件锁存、但没有专属硬件 TRIP 源。
6. 重跑构建、MAP 核对、配置重放，A01/A02/A03 分别给结论并列出未验证范围。

## 1. 意图与边界

本步骤只建立**可复跑的静态基线**，为后续每个变更单元提供对照：证明当前源码能从空目录完整编译链接、链接布局符合设计、配置表与驱动写入逻辑可被逐项重放。

边界：

- **不改固件**：本提交只包含审阅脚本、期望值数据、文本证据和本报告；`Broad/`、`Drivers/`、`CMD/` 等源码零改动。
- **不是 CCS 工程构建**：本机没有安装 CCS，构建脚本直接用 TI 编译器按 CCS 生成的编译/链接清单复现；结论只代表 `ti-cgt-c2000_22.6.2.LTS`，**不代表 22.6.4.LTS 或 CCS 工程构建**。
- **不出板上结论**：没有实板，任何上电、波形、通信、闭环项一律记为"未测"。
- **不定硬件期望**：需要原理图或台架数据才能判断的项标 `PENDING`（待裁定），本步骤不替项目所有者下结论。`reference/` 下的参考工程只用于辅助分析，**不作为批准值来源**。
- **不提交二进制**：`.out`/`.map`/`.obj` 不入库，只登记 SHA-256（见 `docs/review/steps/evidence/01/evidence_hashes.json`）。

## 2. 固定源码与构建输入

本轮所有证据都固定到 **`b20258bd2260ac4aafcdb510c41b7a0483328987`**（已接受的步骤 01 首版提交）。理由：这是 Codex 实际审阅的那份提交；它相对 `f7978e1` 只新增 `docs/review/steps/**`，可编译内容与 `f7978e1` 相同——实测重编出的 `EP15K_2026.out` 与首轮 `f7978e1` 构建**逐字节相同**（`9a52f0c6…c1bc91`）。

构建输入只有两样，都能从仓库自身还原：

1. `git archive b20258b` 解包出的源码树（670 个成员、18 个顶层条目）——不使用工作区未跟踪文件，也不复用仓库里已跟踪的 `Debug/` 对象；
2. 仓库内 CCS 生成的构建清单：`Debug/ccsObjs.opt`（101 个 `.obj` + 2 个链接命令文件 + 4 个 driverlib 库）和 `Debug/subdir_rules.mk`（编译旗标）。

## 3. 可复跑命令（真实参数，本轮实际执行）

```bash
PY="C:/Users/86130/.dsh/dsh-runtimes/dsh-primary-runtime/dependencies/python/python.exe"
SHA="b20258bd2260ac4aafcdb510c41b7a0483328987"

# 1) 干净构建：唯一输入是 git archive <SHA>，输出目录必须不存在或为空
"$PY" docs/review/steps/evidence/01/build_clean.py \
    --repo . --workdir /d/Project/out/step01 --rev "$SHA"

# 2) 链接布局核对：新 MAP vs 仓库内已跟踪的旧 MAP
"$PY" docs/review/steps/evidence/01/analyze_map.py \
    --map /d/Project/out/step01/build/EP15K_2026.map \
    --stale-map Debug/EP15K_2026.map \
    --out-dir /d/Project/out/step01/evidence

# 3) 配置重放：只读步骤 1 的解包树，写回仓库证据目录
"$PY" docs/review/steps/evidence/01/config_replay.py \
    --src /d/Project/out/step01/src --rev "$SHA" \
    --out docs/review/steps/evidence/01
```

说明：

- `build_clean.py --rev` 先 `git rev-parse --verify <rev>^{commit}` 固定 SHA，再 `git archive`；编译单元清单与 `git ls-tree -r --name-only <SHA>`（过滤 `reference/` 后取 `.c/.cla/.asm`）交叉核对。
- `config_replay.py --src` **必须**指向固定提交的解包树，脚本启动即检查 `<src>/Broad/src/DrvCfg_EP15K.c` 存在；它把读到的 6 个源文件的 SHA-256 写进报告 `meta.source_file_sha256`，`meta.source_commit` 记录 `--rev`，因此报告可追溯到具体源码内容。
- `build_clean.py` 对 CCS 生成物做了两处**显式改写**并记入 `build_summary.json`：剔除 makefile 变量（`$(GEN_OPTS__FLAG)`、`$<`）与 `--preproc_*`；把已失效的 include 根 `D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026` 改写成解包树内路径。除此之外旗标、清单、链接库顺序与 CCS 记录一致。

## 4. 证据与命令的对应关系

| 证据文件 | 由哪条命令产生 | 依据的源码 |
| --- | --- | --- |
| `build_summary.json`、`build_compile_log.jsonl`、`build_link.log` | 第 3 节命令 1 | `git archive b20258b`（670 成员） |
| `map_report.json`、`map_excerpt.txt` | 第 3 节命令 2 | 命令 1 产出的 `.map` + 仓库 `Debug/EP15K_2026.map` |
| `config_replay_report.json`、`config_adc_soc_table.csv`、`config_tz_table.csv`、`config_pwm_table.csv` | 第 3 节命令 3 | 命令 1 的解包树；`meta.source_file_sha256` 记录 6 个源文件 |
| `evidence_hashes.json` | 仓库外收集脚本（仅复制 + 计算哈希，不参与结论） | 以上全部 + `.out`/`.map`/`head.tar` |
| 本报告 | 人工 | 以上全部 |

## 5. A01 结论：通过（仅限本机 22.6.2.LTS 直接调用编译器）

| 项 | 值 | 证据 |
| --- | --- | --- |
| 编译单元 | 101 个 = 97 `.c` + 1 `.cla` + 3 `.asm` | `build_summary.json:manifest`；与 `git ls-tree <SHA>` 交叉核对，无遗漏、无多余 |
| 失败 / 告警 | 0 / 0 | `build_summary.json:compile` |
| 链接 | rc=0，0 告警 | `build_summary.json:link` |
| 工具链 | `TMS320C2000 C/C++ Compiler v22.6.2.LTS`，`cl2000` SHA-256 `157254bb…d005ec` | `build_summary.json:compile` |
| 工程工具版本 | CCS 10.3.0 / codegen 22.6.2.LTS | `.ccsproject:4,8` |
| ABI | `--abi=eabi` | `.cproject:64`（Debug 配置 `:155`） |
| 器件与协处理器 | `-v28`、`--cla_support=cla1`、`--float_support=fpu32`、`--tmu_support=tmu0`、`--vcu_support=vcu2`、`--silicon_version=28` | `.cproject:35-40` |
| 浮点模式 | `--fp_mode=relaxed` | `.cproject:37` |
| 器件宏 | `_FLASH`、`DEBUG`、`CPU1`、`F2837xS` | `build_summary.json:compile.flags` |
| 优化级 | 未显式指定；按编译器默认（`-O0`）记录 | `subdir_rules.mk` 无 `-O*`，`.cproject` 无优化选项 |
| 产物 | `.out` SHA-256 `9a52f0c6…c1bc91`（1,032,000 B）；`.map` SHA-256 `b35bafe1…569dadb`（143,385 B） | `evidence_hashes.json` |
| 源码快照 | `head.tar` SHA-256 `453bfce9…2a41f2` | `evidence_hashes.json` |

"空目录"证明：`build_summary.json:empty_output_dir` 记录编译前 `obj/` 为空、编译后 101 个 `.obj` 全部新生成，且没有早于构建开始的 `.obj`。

**未验证**：编译器是否真的被 CCS 以同样环境调用（无 CCS 可比对）；优化级 `-O0` 只是默认值推断。

## 6. A02 结论：通过（6/6）

| 项 | 期望 | 实测 |
| --- | --- | --- |
| A02-1 codestart 位于 `BEGIN` | origin `0x00086000`，length `0x2` | 同 |
| A02-2 入口符号有效且不是 codestart 地址 | `_c_int00` 在 `FLASH_APP` 内且 `!= 0x86000` | `0x0008D4E3` |
| A02-3 CLA 程序加载到 FLASH、运行在 RAMLS | load 在 `0x00086000~0x000A0000`，run 在 `0x8000~0xA800` | load `0x00086008`、length `0x466`、run `0x00008000` |
| A02-4 `.stack` 位于 `RAMM1` 且 0x200 | page1 origin `0x400`，length `0x200` | 同 |
| A02-5 两向消息 RAM 方向与容量 | `Cla1ToCpu=0x1480`、`CpuToCla1=0x1500`，各 ≤0x80，不重叠 | `0x1480` len `0x3E`；`0x1500` len `0xA` |
| A02-6 CLA 数据段位于 `RAMLS5` | run `0xA800`，length ≤0x800 | run `0xA800`，length `0x2A` |

**观察 A02-7（不计入通过率）**：仓库里已跟踪的 `Debug/EP15K_2026.map` 显示 `Cla1ToCpu=0x1500`、`CpuToCla1=0x1480`，与当前 `CMD/2837xS_FLASH_APP_CLA_lnk.cmd:185-186` 及本次链接**方向相反**。该 `.map`/`.out` 是消息 RAM 修复前的产物，不能作为 A02 证据，也不应继续分发（登记为 N-01）。

**未验证**：MAP 只能证明链接分配，不能证明运行期 CLA 实际按该地址收发；这需要板上 B 阶段验证。

## 7. A03 结论：未通过（32 项：21 通过 / 4 未通过 / 7 待裁定）

32 项检查逐项列在 `docs/review/steps/evidence/01/config_replay_report.json`，期望值与依据列在 `expected_config.json`（每项带 `basis`：`TI_MANUAL` / `USER_CONFIRMED` / `RULED_DESIGN` / `PENDING` 与 `source`）。

**判据口径（本轮明确）**：本步骤只能证明"**配置表与驱动写入逻辑相互一致、并且与独立依据（TI 手册位域、已知映射）相符**"。它**不能**证明这些数值是已批准的设计值：

- 表中注释、`docs/review/2026-10-29/01_current_logic.md`（代码现状）和 `05_fix_status.md`（修复记录）都**不是批准依据**；
- 凡是只有"表里这么写、驱动也这么用"的项，一律不得记为通过；
- 值一致但有独立依据的才记 `PASS`；值一致但依据本身待裁定的记 `PENDING`；值不一致或表与运行时行为冲突的记 `FAIL`。

### 7.1 通过项（21）

ADC：步长 4、四模块基址 `[0,1,2,3]`、SOC 控制字位域与 `device/headers/include/F2837xS_adc.h:324-331` 一致、`SOCPRIORITY=16`、`PRESCALE=6`、`INTPULSEPOS=1`、未用 SOC 全 0。
X-BAR：六路输入映射（`INPUT1..6 → GPIO 13/17/41/81/82/40`）、`TRIP↔MUX`（`TRIP4←MUX1, TRIP5←MUX3, TRIP7←MUX5, TRIP8←MUX11, TRIP9←MUX7, TRIP10←MUX9`，均为 `HW_XBAR_MUX_ENABLE`）、逆变过流/短路分路（`TRIP9←INPUT4`、`TRIP10←INPUT5`）。
TZ：十条条目（6 路 INV + 4 路 PV）、INV 两侧 `FORCE_LOW`、PV 单侧映射、`DCAHTRIPSEL` 编码、动作码语义。
PWM：INV 六路 2500/UPDOWN/CMPA 2500、PV 周期 4999/UP、BUS 2500/UPDOWN/FREE、`EPWMXLINK` 自指、AQ 动作。

### 7.2 未通过（4）

| 项 | 现象 | 依据 | 处置 |
| --- | --- | --- | --- |
| A03-TZ-04 | `stPwmTzCfgItem` 无 BUS(EPWM8) 条目 | F03：母线过流缺硬件 TZ 源 | 只登记，另开步骤 |
| A03-PWM-05 | 表内 INV/PV `bGload=1`，但 `Hw_PwmConfigGlobalLoad` 末尾无条件清 `GLD/GLDMODE/OSHTLD`（`Drivers/src/Hw_PwmDriver.c:178-182`） | F09：意图是跨模块同步装载，运行时被关闭 | 只登记，A03 不能总体判通过 |
| A03-PWM-06 | 表字段 `u16SyncOutSel` 在驱动中零引用，`SYNCOSEL` 由 PWM ID 推导（`Hw_PwmDriver.c:224-240`） | 表与运行时行为不一致 | 只登记，待裁定改表还是改驱动 |
| A03-XB-06 | `Broad/src/DrvCfg_EP15K.c:704` 仍写"逆变过流与短路共用 TRIP9"，与表的 TRIP9/TRIP10 分路矛盾 | 注释过期（行为正确） | 文档问题，可随下一步顺带修 |

### 7.3 待裁定（7）

| 项 | 内容 | 为什么不能判通过 | 需要谁裁定 |
| --- | --- | --- | --- |
| A03-ADC-04 | SOC15 触发源 5 / ACQPS 120 作为 ADC-A 完成锚点 | "最高 SOC 编号不是完成屏障"；需要每个 SOC 的起止时刻与跨 ADC 最坏完成时间（`01_current_logic.md:95`、`03_validation_plan.md:71`） | 设计者给出排程证明 |
| A03-ADC-05 | `INT1SEL=15`、`INT2SEL=9`、`CONT=1` | 与上一条同源，缺少排程/完成时刻证明 | 同上 |
| A03-ADC-10 | ADC 通道→板上信号语义（25 个已配置 SOC） | 只有代码注释；需原理图与采样拓扑 | 项目所有者（硬件资料） |
| A03-XB-05 | `TRIPOUTINV` 六路全为 1，与 `:731` 注释"不取反"冲突 | 比较器输出极性/网络极性未知 | 项目所有者（硬件） |
| A03-PWM-08 | 逆变死区 80 个 TBCLK | `HSPCLKDIV=CLKDIV=1` 时 TBCLK=EPWMCLK=100 MHz，即 800 ns；是否足够由器件与实测裁定 | 项目所有者（功率级） |
| A03-PWM-09 | PV 交错相位 `0/1250/2500/3750` | 相位是 4999 周期下的四等分近似；若严格 20 kHz 应改 4999 周期并重算相位（`01_current_logic.md:35`） | 设计者 |
| A03-PWM-10 | BUS 初始占空比 `CMPA=1250` | BUS 尚无平衡 PI 与占空比下发策略（`05_fix_status.md:16`），初始值没有被批准的设计含义 | 设计者 |

### 7.4 收窄前后差异

| 检查 | 旧口径（`b20258b`） | 新口径（本报告） |
| --- | --- | --- |
| A03-ADC-04/05 | PASS（值一致即通过） | PENDING（缺完成时刻证明） |
| A03-PWM-03 | PASS（含相位） | 拆为 PWM-03（周期 4999/UP，PASS）+ PWM-09（相位，PENDING） |
| A03-PWM-04 | PASS（含 CMPA 1250） | 拆为 PWM-04（2500/UPDOWN/FREE，PASS）+ PWM-10（初始占空比，PENDING） |

## 8. 两处描述更正

**GLD（全局装载）**：旧报告写成"实际是直写"，不准确。事实是：`Hw_PwmConfigGlobalLoad` 先按表设置影子位与 `EPWMXLINK.GLDCTL2LINK`，随后在 `Drivers/src/Hw_PwmDriver.c:178-182` 无条件清 `GLD/GLDMODE/OSHTLD/GLDCTL2LINK`，即**全局装载在运行时被关闭**；但 CMP 寄存器**不是无阴影直写**——`CMPCTL.LOADAMODE/LOADBMODE=CTR_ZERO`、`SHDWAMODE=CC_SHADOW`（`Drivers/src/Hw_PwmDriver.c:265-268`）的本模块 ZERO 装载仍在。缺的是**组内同步**与"同周期同时生效"的一致性保证。

**BUS 保护**：旧报告写成"只有软件路径"，不准确。事实是：`StartSystem()` 调 `Hw_PwmStop(HW_PWM_ID7)`（`main.c:193-195`），由 `Hw_PwmDriver.c:372` 写 `TZFRC.OST=1` 触发**硬件锁存**，把 EPWM8 两路输出封死；注释说明 DC 互补路径会把"A 强制低"变成"B 高"，所以单靠 `AQCSFRC` 不足以封锁。真正缺的是**专属硬件 TRIP 源**（F03）：运行期母线过流没有独立于软件的快速关断路径。在补上之前，BUS 不应解锁运行。

## 9. 参考工程对照（`reference/EP5K_V6.0_0721/`）

按本轮要求，参考工程只用于辅助分析，**不作批准值**。逐项对照：

| 维度 | 参考工程 | 本项目 | 可否作为批准依据 |
| --- | --- | --- | --- |
| 芯片 | TMS320F28374S | TMS320F28374S | 相同，可作器件事实参考 |
| 系统时钟 | `SYSTEM_FREQUENCY=200 MHz`、`PWM_SYSTEM_FREQUENCY=100 MHz`（`bsp/inc/bsp.h:153-154`） | 同上（TBCLK 默认） | 相同，可作时钟换算参考 |
| ABI | COFF（`isElfFormat=false`） | EABI | **不同**，链接/符号不能直接套用 |
| 工具链 | CCS 11.0.0 / CGT 20.2.4.LTS | CCS 10.3.0 / CGT 22.6.2.LTS | 不同 |
| 逆变 PWM | `INV_EPWM_PERIOD=2500`（20 kHz，上下计数） | 2500（20 kHz） | 相同 |
| 死区 | 0.75 µs（75 计数） | 80 计数（800 ns） | **不能**作依据：功率级与驱动是否相同未核实 |
| PV PWM | `PV_PWM_PERIOD=2221`（约 45 kHz） | 4999（约 19.996 kHz） | 不同周期，不能套用 |
| BUS PWM | `BUS_PWM_PERIOD=1428`（约 35 kHz） | 2500（20 kHz） | 不同 |
| FAN PWM | 2500 | 5000 | 不同 |
| BUS 硬件保护 | 无专属 TRIP；初始化时 `TZCTL=TZ_FORCE_LO` + `TZFRC.OST=1` 永久锁存（`bsp/src/F2837xS_EPwm.c:787-790`） | 同缺专属 TRIP（F03），启动时 OST 锁存 | 说明"参考工程也没有硬件 BUS 保护"，**不构成批准** |
| 接线/功率拓扑 | 未核实（无原理图证据） | T 型三电平（`01_current_logic.md:5`） | **不能**作依据 |

结论：参考工程只能用于交叉验证**器件级事实**（位域、时钟换算、ePWM 模块分配习惯），不能用于判定本项目配置值是否已批准。

## 10. 修改前后

- 固件、驱动、配置表、链接脚本：**0 行改动**（`git diff --stat b20258b..HEAD` 只出现 `docs/review/steps/**`）。
- 本轮修改（相对 `b20258b`）：
  - `docs/review/steps/evidence/01/build_clean.py`：新增 `--rev`、固定 SHA、`ls-tree` 交叉核对、摘要新增 `pinned_sha/pinned_commit/requested_rev`；
  - `docs/review/steps/evidence/01/config_replay.py`：新增 `--src`/`--rev`，记录 `source_commit`/`source_file_sha256`，新增 `cmp_design`（值一致但 `PENDING` 记待裁定），新增 PWM-09/PWM-10，ADC-04/05 改待裁定，更正 GLD/BUS 描述；
  - `docs/review/steps/evidence/01/expected_config.json`：`anchor_soc15`、`int1_sel`、`int2_sel`、PV `phases`、BUS `cmpa` 改 `PENDING`；`gload_active`、`deadtime_ns` 依据重写；`meta.revision` 记 v2；
  - `docs/review/steps/evidence/01/*`：重跑后的文本证据（含 `evidence_hashes.json`）；
  - 本报告。
- 未提交：`.out`/`.map`/`.obj`（只记哈希）；原理图 PDF（按指示不暂存、不推送）。

## 11. 新登记问题（本步骤不修）

| 编号 | 内容 | 证据 |
| --- | --- | --- |
| N-01 | 仓库跟踪的 `Debug/` 含 101 个修复前 `.obj` 与旧 `.out`/`.map`/`.bin`/`.hex`；`Debug/EP15K_2026.map` 的消息 RAM 方向与当前 CMD 相反 | `map_report.json:observations` |
| N-02 | 全局装载（GLD）表意图与运行时强制关闭冲突 | `Drivers/src/Hw_PwmDriver.c:178-182` |
| N-03 | `ST_PWM_CFG_ITEM.u16SyncOutSel` 未被驱动使用 | `config_replay_report.json:A03-PWM-06` |
| N-04 | X-BAR 两处注释与表冲突：`:704`（共用 TRIP9）、`:731`（极性不取反 vs 值=1） | `config_replay_report.json:A03-XB-05/06` |
| N-05 | 链接清单同时挂 `Debug`/`Release` × `COFF`/`EABI` 四个 driverlib 库；本次链接未报重复符号，但行为依赖归档成员的提取顺序，建议后续统一 | `build_summary.json:manifest.libs` |

**复现性说明（非缺陷）**：`.out` 哈希依赖解包树的绝对路径——`.const:.string` 段会嵌入 `__FILE__` 解析出的源码路径。用 `/d/Project/out/step01` 复现可得到 `9a52f0c6…`；换路径会改变该段长度（实测换到 `step01-r2` 后 `.switch` 位移 +0x10、`.out` 变为 `bdf20e4a…`）。因此**哈希复现须使用本报告写明的同一 workdir 路径**；段地址、长度等布局结论不受影响。

## 12. 验证清单（复跑复核用）

| 检查 | 命令 | 期望 |
| --- | --- | --- |
| 构建通过 | 第 3 节命令 1 | `build_summary.json:result.build = "OK"`，101 单元、0 失败、0 告警 |
| 链接布局 | 第 3 节命令 2 | `A02: 6/6 通过`，退出码 0 |
| 配置重放 | 第 3 节命令 3 | `config_replay_report.json:summary.counts` = 21/4/7，退出码 2（存在未通过项，属预期） |
| 判据可追溯 | 查看 `config_replay_report.json:meta` | `source_commit = b20258b…`，`source_file_sha256` 含 6 个源文件 |
| 源码未改 | `git diff --stat b20258b..HEAD -- Broad Drivers CMD User_App Platform device main.c` | 空 |

## 13. 风险与请求裁定

1. **工具链差异**：本次用 22.6.2.LTS，与审阅文档记录的 22.6.4.LTS 不是同一版本；如需 22.6.4.LTS 复验，请提供该工具链路径或授权安装。
2. **构建方式**：直接调用编译器，非 CCS 工程构建；若后续要求 CCS 原生构建，需安装 CCS 并恢复 `D:/2026WorkPlace/...` 目录布局。
3. **需要项目所有者裁定**：TRIP 输出极性、逆变死区时间(ns)、ADC 通道语义、BUS 的硬件 TZ 源、GLD 是启用还是删除表字段、PV 相位/周期是否统一 20 kHz、BUS 初始占空比含义。
4. **BUS 保护缺口（F03）**：运行期只有 OST 锁存（启动时一次性），无专属 TRIP；在补上之前不应解锁 BUS。
5. **旧二进制可被误用**：`Debug/` 下的 `.out`/`.map` 与当前源码不一致，若被用于烧录会回退消息 RAM 修复（N-01）。

## 14. 未测项（无实板，一律不计通过）

- 上电时序、CLA/ADC/ISR 最小链路（B01–B05）；
- ePWM 门极波形、死区、T 型换相、同步装载（D02–D05）；
- X-BAR/TZ 实际保护动作、比较器极性（D01/D06/D07）；
- SCI/CAN/SPI/Modbus 端到端（C01–C07）；
- BUS 20 kHz 闭环与 E 阶段。

## 15. 审阅结论（Codex 填写）

- 结论：待填
- 意见：
- 是否接受进入步骤 02：待填

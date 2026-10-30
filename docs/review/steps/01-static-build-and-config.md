# 步骤 01 — 静态构建与配置复核（A01–A03）

| 字段 | 值 |
| --- | --- |
| base_sha | `30d953f1123a58b0d9258adf19b82033273f4cb5`（前次提交；本轮修正以它为比对基线） |
| head_sha | 待提交 |
| original_baseline | `57f94362aab18df739b40f7109b1e6987745224c`（`pcs-baseline-20261029`） |
| 关联缺陷 | F01、F02、F03、F04、F05、F09（只核对，不修） |
| 状态 | 待审阅（v3，按第 2 轮审阅意见修正） |

提交沿革（同一工作分支 `deepseek/pcs-stepwise`）：

| 提交 | 说明 |
| --- | --- |
| `f7978e1…` | 步骤 01 的比对起点（协议文档提交） |
| `b20258b…` | 步骤 01 首版，**前次审阅、要求返工的提交** |
| `30d953f…` | 第 1 轮返工提交（固定 SHA、判据收窄、GLD/BUS 更正），**本轮除哈希口径与覆盖度外的证据仍固定于此** |
| 本轮 | 只修正证据验收问题，见第 0 节 |

## 0. 本轮修正范围（相对 `30d953f`）

只改审阅脚本、证据文件和本报告，**固件 0 行改动**。具体：

1. **哈希口径**：`evidence_hashes.json` 原先对本机工作区字节求 SHA-256；由于本机 `core.autocrlf=true`，工作区是 CRLF 而 Git 对象里是 LF，克隆后按登记值校验会失配。现在新增 `verify_hashes.py`，**从 Git 索引/提交对象（`git cat-file blob`）计算** SHA-256，并记录每个文件的 blob OID；`--rev` 可对任意提交逐项复核（见 §4.1）。
2. **检查覆盖度**：`A03-PWM-02` 由"只看一路"改为逐路核对**六路 INV** 的 AQ 动作，`A03-PWM-03` 改为逐路核对**四路 PV** 的周期与计数模式；并用 `mutation_check.py` 在临时副本里分别改错一条**非首路**配置，证明检查会转为 FAIL（2/2 检出，见 §7.5，日志 `mutation_check.log`），临时改动不提交。
3. **措辞与数值更正**：`b20258b` 统一表述为"前次审阅、要求返工的提交"；PV 频率按 `TBCLK=100 MHz`、向上计数、`TBPRD=4999` 更正为 `100 MHz/5000 = 20 kHz`，并注明这是条件计算（时钟配置未实测，见 §7.1、§9）。
4. **重跑**：A01、A02、A03 全部用 `--rev 30d953f…` 重新运行，A03 保留真实的 4 项 FAIL / 7 项 PENDING；报告、`config_replay_report.json`、`evidence_hashes.json` 三者由同一批文件生成并交叉核对。

上一轮（v2，相对 `b20258b`）已完成且本轮未回退：复现命令参数对齐、固定源码 SHA（`build_clean.py --rev`、`config_replay.py --src`）、A03 判据收窄（ADC-04/05/10、XB-05、PWM-08/09/10 改待裁定）、GLD 与 BUS 保护描述更正。

## 1. 意图与边界

本步骤只建立**可复跑的静态基线**，为后续每个变更单元提供对照：证明当前源码能从空目录完整编译链接、链接布局符合设计、配置表与驱动写入逻辑可被逐项重放。

边界：

- **不改固件**：本提交只包含审阅脚本、期望值数据、文本证据和本报告；`Broad/`、`Drivers/`、`CMD/` 等源码零改动。
- **不是 CCS 工程构建**：本机没有安装 CCS，构建脚本直接用 TI 编译器按 CCS 生成的编译/链接清单复现；结论只代表 `ti-cgt-c2000_22.6.2.LTS`，**不代表 22.6.4.LTS 或 CCS 工程构建**。
- **不出板上结论**：没有实板，任何上电、波形、通信、闭环项一律记为"未测"。
- **不定硬件期望**：需要原理图或台架数据才能判断的项标 `PENDING`（待裁定），本步骤不替项目所有者下结论。`reference/` 下的参考工程只用于辅助分析，**不作为批准值来源**。
- **不提交二进制**：`.out`/`.map`/`.obj` 不入库，只登记 SHA-256（见 `docs/review/steps/evidence/01/evidence_hashes.json`）。

## 2. 固定源码与构建输入

本轮所有证据都固定到 **`30d953f1123a58b0d9258adf19b82033273f4cb5`**。理由：它是本轮修正的比对基线，且相对 `f7978e1` 只新增 `docs/review/steps/**` 与 `reference/**`，可编译内容与 `f7978e1` 完全相同——实测重编出的 `EP15K_2026.out` 与首轮 `f7978e1` 及第 1 轮 `b20258b` 构建**逐字节相同**（`9a52f0c6…c1bc91`）。

构建输入只有两样，都能从仓库自身还原：

1. `git archive 30d953f` 解包出的源码树（862 个成员、19 个顶层条目；比 `f7978e1` 多出 `reference/` 下的参考工程，不参与编译）——不使用工作区未跟踪文件，也不复用仓库里已跟踪的 `Debug/` 对象；
2. 仓库内 CCS 生成的构建清单：`Debug/ccsObjs.opt`（101 个 `.obj` + 2 个链接命令文件 + 4 个 driverlib 库）和 `Debug/subdir_rules.mk`（编译旗标）。

## 3. 可复跑命令（真实参数，本轮实际执行）

```bash
PY="C:/Users/86130/.dsh/dsh-runtimes/dsh-primary-runtime/dependencies/python/python.exe"
SHA="30d953f1123a58b0d9258adf19b82033273f4cb5"

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

# 4) 非首路故意改错测试：在临时副本里改错，证明 PWM-02/PWM-03 逐路生效
"$PY" docs/review/steps/evidence/01/mutation_check.py \
    --src /d/Project/out/step01/src --work /d/Project/out/step01-mut --rev "$SHA"

# 5) 证据哈希：从 Git 索引/对象计算（提交前 --write，提交后 --rev 复核）
"$PY" docs/review/steps/evidence/01/verify_hashes.py --write
"$PY" docs/review/steps/evidence/01/verify_hashes.py --rev HEAD
```

说明：

- `build_clean.py --rev` 先 `git rev-parse --verify <rev>^{commit}` 固定 SHA，再 `git archive`；编译单元清单与 `git ls-tree -r --name-only <SHA>`（过滤 `reference/` 后取 `.c/.cla/.asm`）交叉核对。
- `config_replay.py --src` **必须**指向固定提交的解包树，脚本启动即检查 `<src>/Broad/src/DrvCfg_EP15K.c` 存在；它把读到的 6 个源文件的 SHA-256 写进报告 `meta.source_file_sha256`，`meta.source_commit` 记录 `--rev`，因此报告可追溯到具体源码内容。
- `mutation_check.py` 把解包树 `copytree` 到 `--work`，只改临时副本（第 158 行 `AQ_SET→AQ_CLEAR`、第 222 行 `4999U→3000U`），不改工作区、不改解包树。
- `verify_hashes.py --write` 只接受**已暂存**的文本证据（`git ls-files -s` 取 blob OID）；`--rev` 用 `git cat-file blob <rev>:<path>` 复核，**不读工作区**。
- `build_clean.py` 对 CCS 生成物做了两处**显式改写**并记入 `build_summary.json`：剔除 makefile 变量（`$(GEN_OPTS__FLAG)`、`$<`）与 `--preproc_*`；把已失效的 include 根 `D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026` 改写成解包树内路径。除此之外旗标、清单、链接库顺序与 CCS 记录一致。

## 4. 证据与命令的对应关系

| 证据文件 | 由哪条命令产生 | 依据的源码 |
| --- | --- | --- |
| `build_summary.json`、`build_compile_log.jsonl`、`build_link.log` | 第 3 节命令 1 | `git archive 30d953f`（862 成员） |
| `map_report.json`、`map_excerpt.txt` | 第 3 节命令 2 | 命令 1 产出的 `.map` + 仓库 `Debug/EP15K_2026.map` |
| `config_replay_report.json`、`config_adc_soc_table.csv`、`config_tz_table.csv`、`config_pwm_table.csv` | 第 3 节命令 3 | 命令 1 的解包树；`meta.source_file_sha256` 记录 6 个源文件 |
| `mutation_check.log` | 第 3 节命令 4 | 命令 1 解包树的临时副本（改错后丢弃） |
| `evidence_hashes.json` | 第 3 节命令 5（`--write`） | 以上全部 + `.out`/`.map`/`head.tar` |
| 本报告 | 人工 | 以上全部 |

### 4.1 哈希口径与验证方法

**被哈希的对象是 Git 提交中的原始字节，不是本机工作区字节。** 本机 `core.autocrlf=true`，检出时 LF 会被转成 CRLF，因此对工作区文件算出的 SHA-256 与 Git 对象中的内容不同；旧版 `evidence_hashes.json` 正是用了工作区字节，克隆到其他机器后按登记值校验会全部失配。

现在 `verify_hashes.py` 的做法：

- `--write`：只接受已暂存（staged）的文本证据，用 `git ls-files -s <dir>` 取 blob OID，再用 `git cat-file blob <OID>` 读取原始字节计算 SHA-256；清单里每项登记 `sha256` / `size` / `git_blob_oid`，并在 `hash_method` 字段写明方法。
- `--rev <commit>`：用 `git cat-file blob <commit>:<path>` 读取该提交中的字节，逐项与清单比对，输出 `OK`/`MISMATCH` 与 `result: N/N match`；失配退出码 4。

本轮清单共 **16 个文本证据**（上一版的 13 个 + 新增 `verify_hashes.py`、`mutation_check.py`、`mutation_check.log`），另有 `.out`/`.map`/`head.tar` 三个构建产物只登记哈希、不入库。

复核命令（提交后执行）：

```bash
"$PY" docs/review/steps/evidence/01/verify_hashes.py --rev HEAD
```

`hash_method` 字段是这段口径的机器可读版本：

```json
"hash_method": {
  "what": "SHA-256 over the exact bytes stored in the Git object (blob) of each evidence file, not over the working-tree bytes",
  "why": "the working tree may be checked out with CRLF (core.autocrlf) while the Git object stores LF"
}
```

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
| 产物 | `.out` SHA-256 `9a52f0c6…c1bc91`（1,032,000 B，与 `f7978e1`/`b20258b` 构建逐字节相同）；`.map` SHA-256 `5402559…452fec`（143,385 B，每次链接因时间戳首行而不同） | `evidence_hashes.json` |
| 源码快照 | `git archive 30d953f` 的 `head.tar` SHA-256 `d4b4d7c0…4d673f679`（完整值见 `evidence_hashes.json:build_products_not_committed.head.tar`；26,931,200 B） | `evidence_hashes.json` |

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
PWM：INV 六路 2500/UPDOWN/CMPA 2500（逐路核对 AQ 动作）、PV 四路周期 4999/UP（逐路核对周期与计数模式）、BUS 2500/UPDOWN/FREE、`EPWMXLINK` 自指。

**PV 频率（条件计算）**：向上计数、`TBPRD=4999` 时周期为 `TBPRD+1 = 5000` 个 TBCLK；在 `TBCLK = EPWMCLK = 100 MHz`（`HSPCLKDIV=CLKDIV=1`）的条件下，频率为 `100 MHz / 5000 = 20 kHz`。这与设计目标一致，但**本项目时钟配置未上板实测**，因此这是条件计算，不是实测结论；若 `EPWMCLK` 不是 100 MHz，该数值需按实际时钟重算。

**覆盖度**：`A03-PWM-02` 逐路比较六路 INV 的 `{CAU, CAD}`，`A03-PWM-03` 逐路比较四路 PV 的 `{period, ctr_mode}`，任一非首路不符即 FAIL（检出能力见 §7.5）。

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
| A03-PWM-09 | PV 交错相位 `0/1250/2500/3750` | 周期 4999 在条件时钟下已是 20 kHz（见 §7.1）；相位只是 4999/4≈1250 的四等分近似，是否应等间隔、以哪一相为时钟基准、是否需要与采样错开，都没有批准依据（`01_current_logic.md:35`） | 设计者 |
| A03-PWM-10 | BUS 初始占空比 `CMPA=1250` | BUS 尚无平衡 PI 与占空比下发策略（`05_fix_status.md:16`），初始值没有被批准的设计含义 | 设计者 |

### 7.4 收窄前后差异

| 检查 | 旧口径（`b20258b`） | 新口径（本报告） |
| --- | --- | --- |
| A03-ADC-04/05 | PASS（值一致即通过） | PENDING（缺完成时刻证明） |
| A03-PWM-03 | PASS（含相位） | 拆为 PWM-03（周期 4999/UP，PASS）+ PWM-09（相位，PENDING） |
| A03-PWM-04 | PASS（含 CMPA 1250） | 拆为 PWM-04（2500/UPDOWN/FREE，PASS）+ PWM-10（初始占空比，PENDING） |

### 7.5 覆盖度验证：非首路故意改错测试

`config_replay.py` 的 PWM 检查原先只比对第一路，无法证明"六路一致"或"四路一致"。本轮把 PWM-02/PWM-03 改为逐路全量比较后，用 `mutation_check.py` 在临时副本里注入错误来证明覆盖有效（`mutation_check.log`）：

| 用例 | 注入位置 | 改动 | 目标检查 | 结果 |
| --- | --- | --- | --- | --- |
| A03-PWM-02 | `Broad/src/DrvCfg_EP15K.c` 第 158 行（`HW_PWM_ID1`，逆变第 2 路） | `AQ_SET` → `AQ_CLEAR` | A03-PWM-02 | **FAIL**（检出） |
| A03-PWM-03 | 同文件第 222 行（`HW_PWM_ID8`，PV 第 2 路） | `4999U` → `3000U` | A03-PWM-03 | **FAIL**（检出） |

两次运行的汇总均为 `PASS 20 / FAIL 5 / PENDING 7`，FAIL 列表 = 基线 4 项 + 被注入项，说明注入只影响目标检查、不污染其他判据；检出率 **2/2**。临时副本位于仓库外（`--work`），改动未提交、工作区与解包树未被修改。

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
| PV PWM | `PV_PWM_PERIOD=2221`（约 45 kHz） | 4999（`TBPRD+1=5000`，在 `TBCLK=100 MHz` 条件下为 20 kHz） | 不同周期，不能套用 |
| BUS PWM | `BUS_PWM_PERIOD=1428`（约 35 kHz） | 2500（20 kHz） | 不同 |
| FAN PWM | 2500 | 5000 | 不同 |
| BUS 硬件保护 | 无专属 TRIP；初始化时 `TZCTL=TZ_FORCE_LO` + `TZFRC.OST=1` 永久锁存（`bsp/src/F2837xS_EPwm.c:787-790`） | 同缺专属 TRIP（F03），启动时 OST 锁存 | 说明"参考工程也没有硬件 BUS 保护"，**不构成批准** |
| 接线/功率拓扑 | 未核实（无原理图证据） | T 型三电平（`01_current_logic.md:5`） | **不能**作依据 |

结论：参考工程只能用于交叉验证**器件级事实**（位域、时钟换算、ePWM 模块分配习惯），不能用于判定本项目配置值是否已批准。

## 10. 修改前后

- 固件、驱动、配置表、链接脚本：**0 行改动**（`git diff --stat 30d953f..HEAD -- Broad Drivers CMD User_App Platform device main.c reference` 为空）。
- 本轮修改（相对 `30d953f`）：
  - `docs/review/steps/evidence/01/config_replay.py`：`A03-PWM-02` 逐路核对六路 INV 的 `{CAU, CAD}`；`A03-PWM-03` 逐路核对四路 PV 的 `{period, ctr_mode}`；检查总数仍 32 项；
  - `docs/review/steps/evidence/01/verify_hashes.py`（新增）：从 Git blob 计算/复核证据哈希；
  - `docs/review/steps/evidence/01/mutation_check.py`、`mutation_check.log`（新增）：非首路改错的检出证明（2/2）；
  - `docs/review/steps/evidence/01/build_summary.json` 等 5 个构建证据：用 `--rev 30d953f` 重跑后重新导出；
  - `docs/review/steps/evidence/01/config_replay_report.json` 与三个 CSV：同上重跑；
  - `docs/review/steps/evidence/01/evidence_hashes.json`：改为 Git 对象口径（含 `git_blob_oid`、`hash_method`）；
  - 本报告：哈希口径、覆盖度、PV 频率条件计算、提交沿革表述。
- 上一轮（v2，相对 `b20258b`）的修改仍保留：`build_clean.py --rev`、`config_replay.py --src`、`expected_config.json` 的 PENDING 收窄、GLD/BUS 描述更正。
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
| 覆盖度检出 | 第 3 节命令 4 | `mutation_check.log`：2/2 DETECTED，退出码 0 |
| 哈希复核 | 第 3 节命令 5b（`--rev HEAD`） | `result: 16/16 match`，退出码 0；清单含 16 个文本证据（Git blob 口径）+ `.out`/`.map`/`head.tar` 3 个构建产物 |
| 判据可追溯 | 查看 `config_replay_report.json:meta` | `source_commit = 30d953f…`，`source_file_sha256` 含 6 个源文件 |
| 源码未改 | `git diff --stat 30d953f..HEAD -- Broad Drivers CMD User_App Platform device main.c reference` | 空 |

A01/A02 本轮**已重新运行**（非沿用旧证据）：`build_summary.json:pinned_sha = 30d953f…`，`map_report.json` 来自同一次链接的 MAP；A03 亦用同一解包树重放。三者共用同一个 `git archive 30d953f` 解包树与同一 workdir。

## 13. 风险与请求裁定

1. **工具链差异**：本次用 22.6.2.LTS，与审阅文档记录的 22.6.4.LTS 不是同一版本；如需 22.6.4.LTS 复验，请提供该工具链路径或授权安装。
2. **构建方式**：直接调用编译器，非 CCS 工程构建；若后续要求 CCS 原生构建，需安装 CCS 并恢复 `D:/2026WorkPlace/...` 目录布局。
3. **需要项目所有者裁定**：TRIP 输出极性、逆变死区时间(ns)、ADC 通道语义、BUS 的硬件 TZ 源、GLD 是启用还是删除表字段、PV 交错相位的依据（周期已按 `TBCLK=100 MHz` 条件算得 20 kHz，但时钟未实测）、BUS 初始占空比含义。
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

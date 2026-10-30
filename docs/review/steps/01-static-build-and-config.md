# 步骤 01 — 静态构建与配置复核（A01–A03）

| 字段 | 值 |
| --- | --- |
| base_sha | `f7978e1bbfdff4943630efa6713e5ed30ec10930` |
| head_sha | 待提交 |
| original_baseline | `57f94362aab18df739b40f7109b1e6987745224c` |
| 关联缺陷 | F01、F02、F03、F04、F05、F09（只核对，不修） |
| 状态 | 待审阅 |

## 1. 意图与边界

本步骤只建立**可复跑的静态基线**，为后续每个变更单元提供对照：证明当前 `f7978e1` 能从空目录完整编译链接、链接布局符合设计、配置表与驱动写入逻辑可被逐项重放。

边界：

- **不改固件**：本次提交只包含审阅脚本、期望值数据、文本证据和本报告；`Broad/`、`Drivers/`、`CMD/` 等源码零改动。
- **不是 CCS 工程构建**：本机没有安装 CCS，构建脚本直接用 TI 编译器按 CCS 生成的编译/链接清单复现；结论只代表 `ti-cgt-c2000_22.6.2.LTS`，**不代表 22.6.4.LTS 或 CCS 工程构建**。
- **不出板上结论**：没有实板，任何上电、波形、通信、闭环项一律记为“未测”。
- **不定硬件期望**：需要原理图或台架数据才能判断的项标 `PENDING`（待裁定），本步骤不替项目所有者下结论。
- **不提交二进制**：`.out`/`.map`/`.obj` 不入库，只登记 SHA-256（见 `evidence/01/evidence_hashes.json`）。

## 2. 方法与可复跑命令

构建输入只有两样东西，均可从仓库自身还原：

1. `git archive HEAD`（`f7978e1`）解包出的源码树——不使用工作区未跟踪文件，也不复用仓库里已跟踪的 `Debug/` 对象；
2. 仓库内 CCS 生成的构建清单：`Debug/ccsObjs.opt`（101 个 `.obj` + 2 个链接命令文件 + 4 个 driverlib 库）和 `Debug/subdir_rules.mk`（编译旗标）。

```bash
PY="C:/Users/86130/.dsh/dsh-runtimes/dsh-primary-runtime/dependencies/python/python.exe"
# 1) 干净构建（输出目录 /d/Project/out/step01，obj/ 起始为空）
"$PY" docs/review/steps/evidence/01/build_clean.py --out-dir /d/Project/out/step01
# 2) 链接布局核对（新 MAP vs 仓库内旧 MAP）
"$PY" docs/review/steps/evidence/01/analyze_map.py \
    --map /d/Project/out/step01/build/EP15K_2026.map \
    --stale-map Debug/EP15K_2026.map \
    --out-dir /d/Project/out/step01/evidence
# 3) 配置重放（与 expected_config.json 逐项比对）
"$PY" docs/review/steps/evidence/01/config_replay.py --out docs/review/steps/evidence/01
```

`build_clean.py` 对 CCS 生成物做了两处**显式改写**并在 `build_summary.json` 中记录：剔除 makefile 变量（`$(GEN_OPTS__FLAG)`、`$<`）与 `--preproc_*`；把已失效的 include 根 `D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026` 改写成解包树内路径。除此之外旗标、清单、链接库顺序与 CCS 记录一致。

## 3. A01 结论：通过

| 项 | 值 | 证据 |
| --- | --- | --- |
| 编译单元 | 101 个 = 97 `.c` + 1 `.cla` + 3 `.asm` | `build_summary.json:manifest`；与 `git ls-files` 交叉核对，**无遗漏、无多余** |
| 失败 / 告警 | 0 / 0 | `build_summary.json:compile.units/failures/warnings_total` |
| 链接 | rc=0，0 告警 | `build_summary.json:link` |
| 工具链 | `TMS320C2000 C/C++ Compiler v22.6.2.LTS`，`cl2000` SHA-256 `157254bb…d005ec` | `build_summary.json:compile` |
| 工程工具版本 | CCS 10.3.0 / codegen 22.6.2.LTS | `.ccsproject:4,8` |
| ABI | `--abi=eabi` | `.cproject:64`（Debug 配置 `:155`） |
| 器件与协处理器 | `-v28`、`--cla_support=cla1`、`--float_support=fpu32`、`--tmu_support=tmu0`、`--vcu_support=vcu2`、`--silicon_version=28` | `.cproject:35-40` |
| 浮点模式 | `--fp_mode=relaxed` | `.cproject:37` |
| 器件宏 | `_FLASH`、`DEBUG`、`CPU1`、`F2837xS` | `subdir_rules.mk` → `build_summary.json:compile.flags` |
| 优化级 | 未显式指定；按编译器默认（`-O0`）记录 | `subdir_rules.mk` 无 `-O*`，`.cproject` 无优化选项 |
| 产物 | `EP15K_2026.out` SHA-256 `9a52f0c6…c1bc91`（1,032,000 字节）；`EP15K_2026.map` SHA-256 `08f5b4b7…a449f` | `evidence_hashes.json` |
| 源码快照 | `head.tar` SHA-256 `fd437161…30eaa4`，652 个成员 | `build_summary.json:archive` |

“空目录”证明：`build_summary.json:empty_output_dir` 记录编译前 `obj/` 为空、编译后 101 个 `.obj` 全部新生成，且没有早于构建开始的 `.obj`。

## 4. A02 结论：通过（6/6）

| 项 | 期望 | 实测 |
| --- | --- | --- |
| A02-1 codestart 位于 `BEGIN` | origin `0x00086000`，length `0x2` | 同 |
| A02-2 入口符号有效且不是 codestart 地址 | `_c_int00` 在 `FLASH_APP` 内，`!= 0x86000` | `0x0008D4E3` |
| A02-3 CLA 程序加载到 FLASH、运行在 RAMLS | load `0x00086000~0x000A0000`，run `0x8000~0xA800` | load `0x00086008`、length `0x466`、run `0x00008000` |
| A02-4 `.stack` 位于 `RAMM1` 且 0x200 | page1 origin `0x400`，length `0x200` | 同 |
| A02-5 两向消息 RAM 方向与容量 | `Cla1ToCpu=0x1480`、`CpuToCla1=0x1500`，各 ≤0x80，不重叠 | `0x1480` len `0x3E`；`0x1500` len `0xA` |
| A02-6 CLA 数据段位于 `RAMLS5` | run `0xA800`，length ≤0x800 | run `0xA800`，length `0x2A` |

**观察 A02-7（不计入通过率）**：仓库里已跟踪的 `Debug/EP15K_2026.map` 显示 `Cla1ToCpu=0x1500`、`CpuToCla1=0x1480`，与当前 `CMD/2837xS_FLASH_APP_CLA_lnk.cmd:185-186` 及本次链接**方向相反**。该 MAP/OUT 是消息 RAM 修复前的产物，不能作为 A02 证据，也不应继续分发（登记为 N-01）。

## 5. A03 结论：未通过（23 通过 / 4 未通过 / 3 待裁定）

30 项检查逐项列在 `evidence/01/config_replay_report.json`，期望值与依据列在 `evidence/01/expected_config.json`（每项带 `basis`：`TI_MANUAL` / `USER_CONFIRMED` / `RULED_DESIGN` / `PENDING` 与 `source`）。

**通过的主要项**：ADC 步长 4 与四模块基址 `[0,1,2,3]`、SOC 控制字位域与 `F2837xS_adc.h:324-331` 一致、SOC15 锚点（触发源 5 / ACQPS 120）、INT1SEL=15 与 INT2SEL=9、`SOCPRIORITY=16`、`PRESCALE=6`、`INTPULSEPOS=1`、未用 SOC 全 0；X-BAR 六路输入映射、TRIP↔MUX（`TRIP4←MUX1, TRIP5←MUX3, TRIP7←MUX5, TRIP8←MUX11, TRIP9←MUX7, TRIP10←MUX9`）、逆变过流/短路分路；TZ 十条（6 INV + 4 PV）、INV 两侧 `FORCE_LOW`、PV 单侧映射、`DCAHTRIPSEL` 编码、动作码语义；ePWM 六路 2500/UPDOWN/死区 80、PV 4999/UP 相位 `0/1250/2500/3750`、BUS 2500/UPDOWN/CMPA 1250/FREE、`EPWMXLINK` 自指。

**未通过**：

| 项 | 现象 | 依据 | 处置 |
| --- | --- | --- | --- |
| A03-TZ-04 | `stPwmTzCfgItem` 无 BUS(EPWM8) 条目 | F03：母线过流缺硬件 TZ，目前只在启动时单独锁存 ePWM8 的 OST | 只登记，另开步骤 |
| A03-PWM-05 | 表内 INV/PV `bGload=1`，但 `Hw_PwmConfigGlobalLoad` 末尾无条件清 `GLD/GLDMODE`（`Drivers/src/Hw_PwmDriver.c:178-182`） | F09：意图是周期同步装载，实际是直写 | 只登记，A03 不能总体判通过 |
| A03-PWM-06 | 表字段 `u16SyncOutSel` 在驱动中零引用，`SYNCOSEL` 由 PWM ID 硬编码推导（`Hw_PwmDriver.c:224-240`） | 表与运行时行为不一致 | 只登记，待裁定是改表还是改驱动 |
| A03-XB-06 | `Broad/src/DrvCfg_EP15K.c:704` 仍写“逆变过流与短路共用 TRIP9”，与表的 TRIP9/TRIP10 分路矛盾 | 注释过期（行为正确） | 文档问题，可随下一步顺带修 |

**待裁定（PENDING）**：

1. ADC 通道↔板上信号语义（表中注释只是现状，需原理图核对）；
2. TRIP 输出极性：`TRIPOUTINV` 六路全为 1，与 `:731` 注释“不取反”冲突，需按比较器输出/网络极性裁定；
3. 逆变死区：80 个 TBCLK 的实际纳秒数与功率器件要求是否匹配。

## 6. 修改前后

- 固件、驱动、配置表、链接脚本：**0 行改动**（`git diff --stat` 只出现新增文件）。
- 新增文件（全部为文本/脚本）：
  - `docs/review/steps/01-static-build-and-config.md`（本报告）
  - `docs/review/steps/evidence/01/{build_clean.py, analyze_map.py, config_replay.py}`
  - `docs/review/steps/evidence/01/expected_config.json`
  - `docs/review/steps/evidence/01/{build_summary.json, build_compile_log.jsonl, build_link.log}`
  - `docs/review/steps/evidence/01/{map_report.json, map_excerpt.txt}`
  - `docs/review/steps/evidence/01/{config_replay_report.json, config_adc_soc_table.csv, config_tz_table.csv, config_pwm_table.csv}`
  - `docs/review/steps/evidence/01/evidence_hashes.json`
- 未提交：`.out`/`.map`/`.obj`（只记哈希）；原理图 PDF（按指示不暂存、不推送）。

## 7. 新登记问题（本步骤不修）

| 编号 | 内容 | 证据 |
| --- | --- | --- |
| N-01 | 仓库跟踪的 `Debug/` 含 101 个修复前 `.obj` 与旧 `.out`/`.map`/`.bin`/`.hex`；`Debug/EP15K_2026.map` 的消息 RAM 方向与当前 CMD 相反 | `map_report.json:observations` |
| N-02 | 全局装载（GLD）表意图与运行时强制关闭冲突 | `Drivers/src/Hw_PwmDriver.c:178-182` |
| N-03 | `ST_PWM_CFG_ITEM.u16SyncOutSel` 未被驱动使用 | `config_replay_report.json:A03-PWM-06` |
| N-04 | X-BAR 两处注释与表冲突：`:704`（共用 TRIP9）、`:731`（极性不取反 vs 值=1） | `config_replay_report.json:A03-XB-05/06` |
| N-05 | 链接清单同时挂 `Debug`/`Release` × `COFF`/`EABI` 四个 driverlib 库；本次链接未报重复符号，但行为依赖归档成员的提取顺序，建议后续统一 | `build_summary.json:manifest.libs` |

## 8. 验证清单（复跑复核用）

| 检查 | 命令 | 期望 |
| --- | --- | --- |
| 构建通过 | 第 2 节命令 1 | `build_summary.json:result.build = "OK"`，101 单元、0 失败 |
| 链接布局 | 第 2 节命令 2 | `A02: 6/6 通过`，退出码 0 |
| 配置重放 | 第 2 节命令 3 | `config_replay_report.json:summary.counts` 应等于 23/4/3，退出码 2（存在未通过项，属预期） |
| 源码未改 | `git diff --stat 57f9436..HEAD -- Broad Drivers CMD` | 空 |

## 9. 风险与请求裁定

1. **工具链差异**：本次用 22.6.2.LTS，与审阅文档记录的 22.6.4.LTS 不是同一版本；如需 22.6.4.LTS 复验，请提供该工具链路径或授权安装。
2. **构建方式**：直接调用编译器，非 CCS 工程构建；若后续要求 CCS 原生构建，需要安装 CCS 并恢复 `D:/2026WorkPlace/...` 目录布局。
3. **需要项目所有者裁定**：TRIP 输出极性、逆变死区时间(s)、ADC 通道语义、BUS 的硬件 TZ 源、GLD 是启用还是删除表字段。
4. **BUS 保护缺口（F03）**：在补上硬件 TZ 之前，母线过流只有软件路径，属已知风险。
5. **旧二进制可被误用**：`Debug/` 下的 `.out`/`.map` 与当前源码不一致，若被用于烧录会回退消息 RAM 修复（N-01）。

## 10. 未测项（无实板，一律不计通过）

- 上电时序、CLA/ADC/ISR 最小链路（B01–B05）；
- ePWM 门极波形、死区、T 型换相、同步装载（D02–D05）；
- X-BAR/TZ 实际保护动作、比较器极性（D01/D06/D07）；
- SCI/CAN/SPI/Modbus 端到端（C01–C07）；
- BUS 20 kHz 闭环与 E 阶段。

## 11. 审阅结论（Codex 填写）

- 结论：待填
- 意见：
- 是否接受进入步骤 02：待填

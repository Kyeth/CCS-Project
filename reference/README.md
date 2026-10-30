# 参考项目：EP5K_V6.0 0721（第三方/历史工程导入）

本目录是**参考资料**，不是本仓库交付物的一部分，也不参与本项目构建。导入目的是给代码审阅提供同族机型的实现对照；其中的配置值、时序和策略**不得**直接当作本项目的批准值。

## 1. 来源与授权

| 项 | 值 |
| --- | --- |
| 原始路径 | `C:\Users\86130\Desktop\01_项目文档\15KW项目\EP5K_V6.0_0729\EP5K_V6.0_0729\EP5K_V4.0_0514 BMS debug` |
| 目录名（原作者） | `EP5K_V4.0_0514 BMS debug` |
| CCS 工程名 | `EP5K_V6.0 0721`（见 `.project`） |
| 导入路径 | `reference/EP5K_V6.0_0721/` |
| 导入提交 | 见 `git log -- reference/EP5K_V6.0_0721` |
| 授权 | 项目所有者确认可提交到本仓库（本轮任务指令） |

导入时只做了两件事：`cp -r` 复制整棵目录树、删除 `.launches/`。**没有修改任何源文件内容**，因此导入树可与原始目录按文件逐字节比对（`.launches` 除外）。

## 2. 工程元数据

| 项 | 值 | 出处 |
| --- | --- | --- |
| 器件 | TMS320F28374S（C2000，1× C28x + 1× CLA） | `.ccsproject` `deviceVariant`、`targetConfigs/TMS320F28374S.ccxml` |
| CCS 版本 | 11.0.0 | `.ccsproject` `ccsVersion` |
| 编译器 | C2000 CGT 20.2.4.LTS | `.ccsproject` `codegenToolVersion` |
| 输出格式 | COFF（`isElfFormat=false`） | `.ccsproject` |
| 链接命令文件 | `cmd/28374S_FLASH_lnk.cmd`（另带 `28374S_FLASH_CLA_lnk.cmd`） | `.ccsproject` `linkerCommandFile`、`cmd/` |
| 预定义宏 | `_FLASH`、`DEBUG` | `.cproject` |
| 调试探针 | XDS100v3 | `.ccsproject` `connection` |
| 功能范围 | PV/LLC/BUS/INV 状态机、并机（CAN）、Modbus、EEPROM、Flash API | `app/`、`Parallel/`、`FlashAPI/` |

## 3. 文件清单（173 个文件）

| 目录 | 文件数 | 内容 |
| --- | --- | --- |
| `app/` | 41 | `main.c` + 20 个应用源文件/头文件：INV/PV/BUS/LLC 状态机、采样、保护、能量管理、Modbus 表、EEPROM |
| `bsp/` | 75 | 57 个头文件 + 18 个源文件：F2837xS 外设驱动（ADC/ePWM/GPIO/SCI/I2C/CAN/eCAP/CPU Timer/PIE/SysCtrl）、`User_ClaInit.c`、`bsp.c` |
| `algorithm/` | 16 | 8 头 + 8 源：并网逆变、锁相、角度计算等 |
| `Parallel/` | 13 | 并机 CAN 协议、功率分配、`nPnZ_Cntl` |
| `FlashAPI/`、`FlashAPI/Constants` | 14 | TI F021 Flash API 与寄存器定义 |
| `cmd/` | 3 | 三个链接命令文件 |
| `lib/` | 4 | TI 供货的二进制库（见下） |
| 工程配置 | 7 | `.ccsproject`、`.cproject`、`.project`、`.settings/`(3)、`targetConfigs/*.ccxml` |

按扩展名：`.h` 102、`.c` 53、`.asm` 3、`.cla` 1、`.cmd` 3、`.lib` 4、`.ccxml` 1、`.prefs` 3、`.cproject/.ccsproject/.project` 各 1。

`lib/` 中的四个库均为 TI 供货的二进制依赖（非本项目编译产物），随工程保留以便重建：

| 文件 | 大小 | 用途 |
| --- | --- | --- |
| `rts2800_fpu32.lib` | 6,950,124 B | C2000 运行时库（FPU32） |
| `F021_API_F2837xD_FPU32.lib` | 763,598 B | Flash API |
| `cla1_math_library_datarom_fpu32.lib` | 47,652 B | CLA 数学库 |
| `2837x_c1bootROM_CLADataROMSymbols_fpu32.lib` | 27,842 B | C1 boot ROM / CLA 数据 ROM 符号 |

## 4. 排除项及原因

| 排除 | 规模 | 原因 |
| --- | --- | --- |
| `.launches/`（7 个 `.launch`） | 约 46 KB | IDE 本地调试状态：内含原作者机器的绝对路径（`D:\EP5k\EP5K_V6.0_0729\...`）、探针型号与 Flash 扇区选择。既不是源码也不是工程配置，提交后只会造成路径误导。 |
| `Debug/`、`.obj`、`.out`、`.map`、`.hex`、`.bin` | 0 | 原始目录**本身未包含**编译产物（无 `Debug/`、无任何目标文件），因此没有额外的构建产物需要排除。 |
| 临时文件（`*~`、`*.bak`、`.DS_Store`、`Thumbs.db`） | 0 | 原始目录中未发现。 |

`.settings/`（3 个小文件）与 `targetConfigs/*.ccxml` 予以保留：前者是工程级 Eclipse 编码偏好，后者是目标连接配置，均属可复现信息。

## 5. 已知实板验证情况

**未经核实，不能据此声称已实板验证。** 现有证据仅有 7 个调试配置文件，文件名带版本与日期（`EP5K_V1.0 1218`、`V3.0 0202`、`V4.0 0514`、`V4.0 0518`、`V4.0 0529`、`V5.0 0605`、`V6.0 0721`），说明原作者用 XDS100v3 做过 CCS 调试会话；但工程内**没有**测试记录、波形、测试报告或版本说明文档。这些 `.launch` 文件已按第 4 节排除，文件名仅作为历史线索记录在本文件中。

是否需要把该工程的实验记录补入仓库，请项目所有者裁定。

## 6. 与本项目（EP15K PCS）的差异

| 维度 | 参考项目 | 本项目 | 是否可直接参考 |
| --- | --- | --- | --- |
| 器件 | TMS320F28374S | TMS320F28374S | 是（同一器件） |
| 工具链 | CCS 11.0.0 / CGT 20.2.4.LTS / COFF | CCS 10.3.0 / CGT 22.6.2.LTS / EABI | 否：ABI 与编译器版本不同，链接布局与代码生成不可直接比对 |
| 时钟 | `SYSTEM_FREQUENCY 200 MHz`、`PWM_SYSTEM_FREQUENCY 100 MHz`（`bsp/inc/bsp.h:153-154`） | 同标称 200 MHz / 100 MHz（本次 A02 以链接脚本与驱动核对） | 数字相同，但参考项目未提供时钟树配置依据 |
| INV PWM | EPWM1~EPWM4，`INV_EPWM_PERIOD`=2500、up-down（20 kHz），死区 0.75 µs = 75 计数（`bsp.h:156,158-161`） | EPWM1/2/3/4/10/11 六路，TBPRD 2500、死区 80 计数（800 ns @100 MHz） | 否：通道分配与路数不同（4 路 vs 6 路） |
| PV PWM | EPWM5/6/7/10，`PV_PWM_PERIOD`=2221（约 45 kHz，up） | EPWM5/6/7/9，TBPRD 4999（20 kHz，up），相位 0/1250/2500/3750 | 否：频率、相位与模块分配均不同 |
| BUS PWM | EPWM8，`BUS_PWM_PERIOD`=1428（约 35 kHz），CMPA=TBPRD/2 | EPWM8，TBPRD 2500（20 kHz），CMPA 1250，FREE | 否：频率与同步方式不同 |
| FAN PWM | EPWM12 | EPWM12 | 部分相同（仅模块号） |
| BUS 保护 | `EPwm8Regs.TZCTL.TZA/TZB = TZ_FORCE_LO` + `TZFRC.OST=1` 初始化锁存（`bsp/src/F2837xS_EPwm.c:787-790`），**同样没有**专属 TRIP 输入 | 同样只有启动时软件 OST 锁存，TZ 表中无 BUS 条目（F03） | 只能作为"同类工程也用软件锁存"的旁证，**不是**本项目批准依据 |
| 功率拓扑 | 15 kW 机（PV/LLC/BUS/INV 多级 + 并机） | T 型三电平三相 PCS，每相额定 5 kW（`docs/review/2026-10-29/01_current_logic.md:5`） | 否：拓扑与功率等级不同，阈值不可移植 |

结论：**芯片相同，其余（工具链、时钟依据、PWM 分配、频率、拓扑）均不相同**。参考项目只用于"同类问题在其他工程中如何出现"的对照，不能作为本项目 A03 的期望值来源。

## 7. 使用约定

1. 本目录只读参考；不得在其中直接修改并据此声称是本项目实现。
2. 引用其行为时须标注"参考项目"及文件路径，并在本项目报告中注明差异（第 6 节）。
3. 若需在 CCS 中打开该工程，请先复制到独立工作区，避免与当前项目的 `.cproject`/`Debug` 混淆。

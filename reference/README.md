# 参考工程导入说明（EP5K_V6.0 0721、IB800U）

本目录是**参考资料**，不是本仓库交付物的一部分，也不参与本项目构建。导入目的是给代码审阅提供同族机型 / 同类控制器的实现对照；其中的配置值、时序和策略**不得**直接当作本项目的批准值。

| 参考工程 | 器件 | 导入路径 | 导入提交 | 详细说明 |
| --- | --- | --- | --- | --- |
| A：EP5K_V6.0 0721 | TMS320F28374S | `reference/EP5K_V6.0_0721/` | `fabb126` | 本文第 1–7 节（以下称"工程 A"） |
| B：IB800U | TMS320F280039C | `reference/IB800U/` | 见 `git log -- reference/IB800U` | 本文末尾"参考工程 B"（B.1–B.7 节） |

两个参考工程**互不相同**：A 是与本项目同器件的 15 kW 逆变器工程，B 是不同系列器件上的电池/功率平衡控制器工程。引用时必须写明是哪一个（见各节"使用约定"）。

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

---

# 参考工程 B：IB800U（第三方/历史工程导入）

本工程与工程 A **不是同一类设备**：目录结构与配置表（`Platform/`、`User/`）显示它是电池 / 功率平衡方向的控制器工程，器件也不同（F28003x 而非 F2837xS）。它可以作为"不同器件、不同代码组织方式下，同类驱动层与配置表怎么设计"的对照，但不能用于本项目的任何数值或时序期望。

## B.1 来源与授权

| 项 | 值 |
| --- | --- |
| 原始路径 | `D:\IB800_Andy\IB800_2.1.1\IB800U` |
| CCS 工程名 | `IB800U`（见 `.project`） |
| 导入路径 | `reference/IB800U/` |
| 导入提交 | 见 `git log -- reference/IB800U` |
| 授权 | 项目所有者确认可提交到本仓库（本轮任务指令 m01100） |

导入方式：用 `tar` 管道复制并排除 `RELEASE/` 与 `.launches/`（源目录 1.7 GB，其中 1.7 GB 是 `RELEASE/` 构建产物，逐个 `cp` 无必要）。**没有修改任何文件内容**；导入后已用 `diff -r --exclude=RELEASE --exclude=.launches` 与源目录逐字节比对，结果完全一致。

## B.2 工程元数据

| 项 | 值 | 出处 |
| --- | --- | --- |
| 器件 | TMS320F280039C（C2000 / F28003x 系列） | `.ccsproject` `deviceVariant=TMS320F280039`、`targetConfigs/TMS320F280039C_LaunchPad.ccxml`、`targetConfigs/TMS320f280039C.ccxml` |
| 编译器 | C2000 CGT **22.6.x** | `.cproject` 内 141 处工具链 ID 均为 `C2000_22.6.*` |
| CCS 版本 | 未记录 | `.ccsproject` 无 `ccsVersion` 字段 |
| 输出格式 | 未显式记录；库文件名均为 `*_eabi.lib`，推断 EABI | `.ccsproject` 无 `isElfFormat` 字段 |
| 构建配置 | 仅一个 `RELEASE`（`artifactExtension=out`、`artifactName=${ProjName}`） | `.cproject` |
| 预定义宏 | `_FLASH`、`_DEBUG`、`LARGE_MODEL`、`FLASH`、`CPU1`、`BUILD_F28003X` | `.cproject` |
| 链接命令文件 | `User/Cmd/28003x_cla_flash_lnk.cmd`、`User/Cmd/f28003x_headers_nonBIOS.cmd` | 树内 |
| 缺失的链接命令文件 | `.cproject` 的 `LINKER_COMMAND_FILE=F280039_ram_lnk.cmd` **不在树内** | `.cproject` |
| 后处理脚本 | `flash_only_hex.cmd`（根目录）、`setupdebugenv_demo.js` | 树内 |
| 调试探针 | 未记录具体型号；两个 ccxml 分别指向 LaunchPad 与通用目标 | `targetConfigs/` |
| 功能范围 | BMS / 功率平衡、CAN 与 MCAN、Modbus 从站、EEPROM（FM24C128D）、参数保存、Xmodem 升级、CLA 任务、IO 设备 | `User/`、`Platform/` |

## B.3 文件清单（402 个文件 / 109 个目录 / 约 11 MB）

| 目录 | 文件数 | 内容 |
| --- | --- | --- |
| `device/` | 196 | F28003x 器件头文件、`driverlib` 源码（39 个 `.c`）与头文件、`Device.h/device.c`、`ccs/Debug/driverlib.lib`（1,097,682 B）、`ccs/Release/driverlib.lib`（724,242 B）、`driverlib.projectspec` |
| `Platform/` | 74 | `Bsp/Brd_drv_std`（板级驱动）、`Common`、`Eeprom`、`EquipmentParallel`、`Flash`、`InternalCan`、`Modbus`、`ParamSave`、`Schedule`、`Upgrade`、`Version`、`Xmodem` |
| `User/` | 57 | `Cmd`、`DriverCfg`、`EquipmentParallel`、`FM24C128D`、`FaultDetect`、`IODevice`、`InternalCanProcess`、`InterruptCfg`、`LoopCtrl`、`ModbusSlave`、`ParamSave`、`PlatformManager`、`SoftOsc`、`SysManager`、`Upgrade`、`Version`、`XmodemSlaveProcess` |
| `libraries/` | 58 | TI 二进制库与源码库：`FlashAPI`、`CLAMath`、`FPUfastRTS`、`SFO`、`sfra`、`DCL`（3 个 `.asm` + 头文件）、`diagnostic`（4 个 `.asm`） |
| `hex2bin/` | 4 | 升级包工具（见 B.7 风险提示） |
| `targetConfigs/` | 2 | 两个 `.ccxml` |
| `.settings/` | 3 | Eclipse 工程偏好（codan / debug / resources） |
| 根目录 | 8 | `.ccsproject`、`.cproject`、`.gitignore`、`.project`、`main.c`、`User_ClaInit.c`、`flash_only_hex.cmd`、`setupdebugenv_demo.js` |

按扩展名：`.h` 250、`.c` 114、`.lib` 11、`.asm` 8、`.cmd` 3、`.exe` 3、`.prefs` 3、`.ccxml` 2，`.cla`/`.cproject`/`.ccsproject`/`.project`/`.projectspec`/`.js`/`.md`/`.gitignore` 各 1。

`libraries/` 下 9 个库与 `device/driverlib/ccs/` 下 2 个库均为 TI 供货或上游工程自带的二进制依赖（非本项目编译产物），随工程保留以便重建：

| 文件 | 大小 |
| --- | --- |
| `libraries/FlashAPI/FAPI_F28003x_EABI_v1.58.10.lib` | 631,878 B |
| `libraries/CLAMath/cla2_math_library_fpu32_eabi.lib` | 153,612 B |
| `libraries/CLAMath/cla2_math_library_eabi.lib` | 153,020 B |
| `libraries/CLAMath/cla2_math_library_datarom_fpu32_eabi.lib` | 138,100 B |
| `libraries/CLAMath/cla2_math_library_datarom_eabi.lib` | 137,536 B |
| `libraries/CLAMath/F28003x_CLADATROM_Symbols_fpu32.lib` | 42,736 B |
| `libraries/FPUfastRTS/rts2800_fpu32_fast_supplement_eabi.lib` | 45,252 B |
| `libraries/SFO/SFO_v8_fpu_lib_build_c28_eabi.lib` | 41,774 B |
| `libraries/sfra/sfra_f32_tmu_eabi.lib` | 25,368 B |
| `device/driverlib/ccs/Debug/driverlib.lib` | 1,097,682 B |
| `device/driverlib/ccs/Release/driverlib.lib` | 724,242 B |

## B.4 排除项及原因

| 排除 | 规模 | 原因 |
| --- | --- | --- |
| `RELEASE/` | 1.7 GB（源目录 976 个文件中的约 571 个） | CCS 构建产物：`.lst` 列表文件（最大 `RELEASE/User/FaultDetect/Src/FaultProtect.lst` 18,759,598 B）、`.obj`、`.out`、`.map`。上游工程**自带的 `.gitignore` 就写着 `/RELEASE/`**，说明原作者同样把它当生成物。 |
| `.launches/`（3 个 `.launch`） | 36 KB | IDE 本地调试状态：含原作者机器绝对路径与调试器配置。文件名（`IB500U_BMS_PwrBal_MCan_V5.0.4078.02`、`..._V6.0.4078.04_coding`、`IB800U`）仅作历史线索记录在此。 |
| 临时文件（`*~`、`*.bak`、`.DS_Store`、`Thumbs.db`） | 0 | 原始目录中未发现。 |

与工程 A 不同，本工程**确实包含**编译产物，且体量是源码的百余倍，因此排除 `RELEASE/` 是本次导入的必要动作；这也是导入后文件数从 976 降到 402 的原因。

## B.5 已知实板验证情况

**未经核实，不能据此声称已实板验证。** 现有证据：源目录存在完整的 `RELEASE/` 构建输出（说明至少成功编译过）、3 个调试配置文件（含 `BMS_PwrBal` 字样的版本化命名，说明原作者用 CCS 做过调试）、`hex2bin/` 下 3 个升级包工具与《Hex2Bin 使用指南》（说明有 hex→bin + CRC 的发布流程）。工程内**没有**测试记录、波形、测试报告或版本说明文档；是否存在对应的硬件与测试结论需项目所有者确认。上述 `.launch` 文件已排除，其文件名仅作历史线索记录在本文件中。

## B.6 与本项目（EP15K PCS）的差异

| 维度 | 参考工程 B（IB800U） | 本项目 | 是否可直接参考 |
| --- | --- | --- | --- |
| 器件 | TMS320F280039C（F28003x） | TMS320F28374S（F2837xS） | **否**：不同系列，外设集合、CLA 版本（cla2 库 vs cla1）与地址映射均不同 |
| 工具链 | CGT 22.6.x，库名均为 `*_eabi.lib` | CGT 22.6.2.LTS，EABI | 编译器版本同代，仅可作"同代工具链下的旗标写法"对照 |
| 时钟 | 本次仅在 `Platform/`、`User/` 中检索，**未发现**类似工程 A `bsp.h` 的集中时钟常量 | `SYSTEM_FREQUENCY` 200 MHz / EPWMCLK 100 MHz（由链接脚本与驱动核对） | 否：无对照依据 |
| PWM 分配 | 8 路（`PWM_ID0..7`），周期 600 / 1200 / 12000，全部**向上计数**，`TBPHS=0`，死区 60；`PWM_ID6` 为同步主、`SYNC_OUT_FROM_CNTR_ZERO`，`ID0/1/3/4` 从 `EPWM7` 同步；`gload` 标志 1/0 混用 | 12 项配置表（INV 6 路 up-down 2500、PV 4 路 up 4999、BUS/FAN），死区 80，INV 以 EPWM1 为主、PV 以 EPWM7 为主 | 否：路数、计数模式、周期、同步主均不同 |
| PWM 配置表结构 | 13 字段（ID/周期/gload/link/相位/主从/计数方向/死区/两路故障电平/计数模式/同步入/同步出），**无 CMPA/CMPB/AQ 字段** | 22 字段（含 CMPA/CMPB/死区/TZ 动作/初始强制/AQ 六事件/ET_SOC） | 否：表结构与配置粒度不同 |
| ADC 表 | 16 行 × 3 个 ADC 交织（A/B/C 三列，每格 `{触发源, 采样窗口, 通道}`），结构体无 stride 字段、每个 ADC 用 `&stAdcChCfgItem[0..2]` 取列 | 16 行 × 4 个 ADC 交织，`u16SocStride=4`，CLA 读 SOC8/9 | 否：列数、取列方式与中断锚点不同 |
| ADC EOC | ADC-A 用 `intSel=11`、ADC-B 用 3、ADC-C 用 4；注释写"选择使用数量最多的 ADC，且最后的那个 soc 作为 eoc 的中断触发" | ADC-A `INT1SEL=15`（SOC15 为锚点）+ `INT2SEL=9`，CLA 任务读结果 | 否；且其"最后的 SOC"与 `intSel=11` 是否一致需按其设计核对（此处仅记录，不作结论） |
| CLA | 有 `User/LoopCtrl/Src/TaskCla.cla` + `User_ClaInit.c`，用 `cla2` 数学库 | `User_App/LoopCtrl/src/User_ClaTask.cla`，用 `cla1` 数学库 | 否：CLA 代数不同 |
| 通信 | CAN + MCAN + SCI + I2C + Modbus 从站 | SCI/CAN/SPI + Modbus | 否：MCAN 为 F28003x 独有外设 |
| 功能定位 | 电池 / 功率平衡控制器（温度、支路电流、空开前后电压、加热膜、变压器温度等采样通道） | T 型三电平三相 PCS（每相 5 kW） | 否：被控对象、拓扑与保护逻辑完全不同 |

结论：**器件不同、外设集合不同、配置表结构不同、被控对象不同**。工程 B 只能用于"驱动分层与配置表组织方式"的写法对照，不能作为本项目任何检查项的期望值来源。

## B.7 使用约定与风险提示

1. 本目录只读参考；不得在其中修改并据此声称是本项目实现。
2. 引用其行为时须标注"参考工程 B（IB800U）"及文件路径；不得把其中的周期、死区、采样窗口等数值搬作本项目期望值。
3. **元数据提示**：树内保留了原作者的机器路径与用户名（`.ccsproject` 中 `origin=C:/Users/G310100755/...`）、源码作者署名与工号（如 `Platform/Bsp/Brd_drv_std/Src/Hw_AdcDriver.c:13` 的 `@author yetingheng (004182)`）。仓库为公开仓库，这些信息随导入一并公开；如需脱敏请项目所有者裁定后另开一步处理（本轮按"原样导入"处理，未修改任何文件）。
4. **可执行文件提示**：`hex2bin/` 含 3 个 Windows 可执行文件（`hex2bin.exe` 48,128 B、`CalcHexAddr.exe` 68,972 B、`CalcUpgradeBinCrc.exe` 64,434 B）。它们是上游发布流程工具，**本仓库从未执行过**，来源与安全性未经验证；仅作工程完整性保留。若不希望公开仓库包含二进制可执行文件，可按第 3 条同样另开一步处理。
5. 工程配置含原作者绝对路径，编译输出目录 `RELEASE/` 已被排除；如需在 CCS 中打开，请先复制到独立工作区并重建输出目录，避免与本项目 `.cproject`/`Debug` 混淆。

# EP15K_BOOT 最小 Bootloader（参考工程移植）

| 项 | 内容 |
| --- | --- |
| 工程 | `EP15K_BOOT`（分支 `boot/minimal`，基线 `bf32d42`） |
| 参考工程 | `IB800U_BootLoader`，commit `68701bd` |
| 芯片 | TMS320F28374S（CGT 22.6.2.LTS，EABI，FPU32） |
| 入口 | `0x080000`（`BEGIN` 2 字 → `_c_int00`），程序段 `FLASH_BOOT = 0x080002..0x085FFF` |
| 状态 | 构建通过（CCS Debug 与命令行两条路线），**未上板**；已按复核意见修订（§3.6） |

---

## 0. 结论先行

- 本工程是参考 BootLoader 的**移植**，不是重写：XMODEM 状态机、升级配置结构、
  备份区回拷流程、`CheckUpgrade()` 的判定顺序都保持参考实现的形状与接口。
- 必要改动只有五类：芯片适配、端口适配、时基适配、工程接线、必要缺陷修复
  （移植时 6 条 + 复核后 5 条，见 §3.5、§3.6）。除此之外没有新增框架、没有改协议行为。
- 复用 APP 工程**已复核**的模块：SCI 驱动、`RingBuff`、端口层（`SciPort`/
  `PortTask`/`PortCritical`）、XMODEM、Flash（F021）、Upgrade。
- 控制功能与测试代码**不在**本工程：ADC/PWM/CLA/CAN/Xbar/Modbus/采样/前台应用
  全部移除（端口层因此只保留 3 路 SCI 发送单元，见 §3.2）。

---

## 1. 参考工程与移植映射

| 参考路径（`IB800U_BootLoader`） | 本工程路径 | 行数 | 改动 |
| --- | --- | --- | --- |
| `Platform/Xmodem/Inc/XmodemCore.h` | `Platform/Xmodem/Inc/XmodemCore.h` | 152 | 设备头 + 常量（§3.1、§3.5） |
| `Platform/Xmodem/Src/XmodemCore.c` | `Platform/Xmodem/Src/XmodemCore.c` | 524 | 首包裁剪、写失败、重试收尾（§3.5） |
| `Platform/Xmodem/Inc/XmodemTimeOut.h` | 同名 | 45 | 无 |
| `Platform/Xmodem/Src/XmodemTimeOut.c` | 同名 | 58 | 时基换成 `Hw_TimeBase`，差值方向（§3.3、§3.5） |
| `Platform/Xmodem/Inc/XmodemTramsports.h` | 同名 | 28 | `const` 参数、`XmodemRxReset()` |
| `Platform/Xmodem/Src/XmodemTramsport.c` | 同名 | 32→约 200 | 端口换成 `SciPort` 整包累计（§3.2、§3.5） |
| `Platform/Xmodem/Inc/XmodemDataHandle.h` | 同名 | 32 | 无 |
| `Platform/Xmodem/Src/XmodemDataHandle.c` | 同名 | 51 | 备份区越界与写失败上报（§3.5） |
| `Platform/Flash/Inc/Dsp280039Flash.h` | `Platform/Flash/Inc/Dsp2837xSFlash.h` | 106→约 135 | 扇区表、F021 基址、RAM 执行（§3.1、§3.5） |
| `Platform/Flash/Src/Dsp280039Flash.c` | `Platform/Flash/Src/Dsp2837xSFlash.c` | 153→约 270 | 同上 + 空白检查长度、8 字整块编程 |
| — | `Platform/Flash/Src/Fapi_UserDefinedFunctions.c` | 约 45 | 新增：`Fapi_serviceWatchdogTimer()`、`__error__()` 落 RAM |
| `Platform/Upgrade/Inc/UpgradeCore.h` | 同名 | 76 | 无 |
| `Platform/Upgrade/Src/UpgradeCore.c` | 同名 | 163→约 175 | 按字数计块（§3.5）、`CopyBootFirmware` 落 RAM（§3.4） |
| `Platform/Upgrade/Inc/DspUpgradeCrc.h` | 同名 | 20 | 无 |
| `Platform/Upgrade/Src/DspUpgradeCrc.c` | 同名 | 46 | CRC32 落 RAM（§3.6，供自升级后校验） |
| `User/XmodemSlaveProcess/Inc/…h` | 同名 | 24 | 无 |
| `User/XmodemSlaveProcess/Src/…c` | 同名 | 42 | 无（CRC=1、重试 8、超时 1200 保持参考取值） |
| `User/Upgrade/Inc/UpgradeUser.h` | 同名 | 32→约 40 | 分区地址常量、返回 int16 的擦除接口、中止收尾声明 |
| `User/Upgrade/Src/UpgradeUser.c` | 同名 | 211→约 470 | 分区地址、擦除表、APP 有效性、静默回拷、自升级尾部（§3.4、§3.6） |
| `main.c` | `main.c` | 123→约 200 | 全新接线（§3.4） |
| — | `Broad/{inc,src}/DrvCfg_EP15K.*` | 重写 | 只保留时钟/GPIO/SCI/CPU Timer1 四张表 |

参考工程里 `User/Flash280039C/*` 是整份注释掉的遗留实现、`libraries/FlashAPI/*` 是
F28003x 版 F021，两者都未移植；本工程用 `libraries/flash_api/f2837xs/`（随工程携带）。

---

## 2. 分区与文件格式（与 APP 一致）

| 区域 | 扇区 | 地址范围 | 大小 | 本工程 |
| --- | --- | --- | --- | --- |
| BOOT | A+B+C | `0x080000~0x085FFF` | 24 KB | 链接目标 |
| APP | D+E+F+G | `0x086000~0x09FFFF` | 104 KB | 跳转/回拷目标 |
| BACKUP | H+I+J+K | `0x0A0000~0x0B9FFF` | 104 KB | 收包落盘区 |
| SPARE | L+M | `0x0BA000~0x0BDFFF` | 16 KB | 未使用 |
| CFG | N | `0x0BE000~0x0BFFEF` | 8176 B | 升级配置 |

32 字节文件头（`ST_UPGRADE_FILE_HEADER`）：`[0]` ChipType（1）、`[1]` FirmwareType
（1=APP，2=BOOT）、`[2..5]` 正文长度（小端字节）、`[6..9]` 正文 CRC32（小端，
poly `0x04C11DB7`、初值 `0xFFFFFFFF`、不反射、无最终异或）、`[10..31]` 保留。
备份区里两个连续字节压成一个 16 位 Flash 字（先到者低字节），因此 104 KB 备份区
可容纳 `0x34000` 字节正文。

---

## 3. 五类必要改动

### 3.1 芯片适配

- 设备头 `F28003x_device.h` → `F2837xS_device.h`（Xmodem/Flash/Upgrade/Slave 全量）。
- F021：`#define _F2837xS` 后 include，`Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, 200)`，
  只激活 `Fapi_FlashBank0`（F28374S 单块，参考工程的 Bank0/1/2 选择是 F280039C 残留）。
- 扇区表按真实 A~N：A~D、K~N 为 8K 字节（`0x2000` 字），E~J 为 32K 字节（`0x8000` 字）。
- 链接脚本 `CMD/2837xS_FLASH_BOOT_CLA_lnk.cmd`：codestart `0x080000`、程序段
  `FLASH_BOOT`，APP/BACKUP/SPARE/CFG 只声明不分配；`.TI.ramfunc`→`RAMD0`、
  `.flashfunc`→`RAMGS3_RUN`、`.upybuf`→`RAMGS2`；去掉 CLA 段。
- RAM 执行搬运：`main()` 第一行 `CopyRamSections()` 把 `.TI.ramfunc` 与
  `.flashfunc` 从 Flash 拷到运行地址（基线工程此前完全没有搬运代码）。

### 3.2 端口适配

`XmodemRead()/XmodemWrite()` 内部换成 `SciPort`：

- `XmodemWrite` → `SciPortWriteTx(HW_SCI_ID_A, …)`；返回 0 表示端口环满，
  协议层必须把控制字节留到下一轮再发（不能当已发送）。
- `XmodemRead` 在适配层**累计整包**：先倒出预置字节，再按包头（SOH 133 /
  STX 1029 / CAN 2 / 其它 1）用 `SciPortPopBatch`（每次 ≤8 字节）拼接，收满才返回；
  跨包的余字节保存到 64 字预置缓冲，收满本包即停止取批；半包静默 100 ms 丢弃并
  重新同步。
- `XmodemRxPreload()` 把 Modbus 握手同批多取的字节还给读路径，`XmodemRxReset()`
  在会话开始/取消时清掉适配层残留。
- 端口层：`PortTask` 的前台单元由 5 个（3 SCI + 2 CAN）降为 3 个 SCI——BOOT 不含
  CAN，因此 `CanPort`/`Hw_CanDriver`/`CanQueue` 都不进本工程。这是端口层唯一的
  结构差异，其余逻辑与 APP 逐字一致。

### 3.3 时基适配

`Hw_TimeBase` 是向下计数自由计数器：`TimeStart()` 记 `Hw_TimeBaseTicks()`，
`IsTimeOut()` 判 `(Uint32)(u32StartTick - u32NowTick) > u32TimeOut`（毫秒单位不变）。
时间源未就绪时既不启动计时也不判超时。

### 3.4 工程接线

`main()`：`CopyRamSections()` → 时钟/时基/中断向量/GPIO/SCI/CPU Timer1（100 µs 心跳）
→ `SciPortInit`/`PortTaskInit`/`XmodemProcessInit`/`Dsp2837xSFlashInit`/`UpgradeUserInit`
→ `CheckUpgrade()` → `StartSystem()` → 主循环 `喂狗 + PortTaskService + TaskUpgrade`。
`TaskUpgrade()` 返回 2（中途中止）时调用 `UpgradeUserAbort()` 收尾（§3.6 第 3 条）。

`CheckUpgrade()` 保留参考的三分支：未收完→按配置重设波特率并擦备份区（参数不合法
则清配置后处理跳转）；已收完→校验备份区后按 `bFirmwareType` 回拷 APP 或 BOOT；
无配置→按 APP 是否有效决定跳转或留 BOOT 收包。

APP 侧接线（APP 工程）：8 字节握手应答 → 擦备份区并写"接收中"配置 → 单发 `'C'`
→ 收包落备份区 → 校验 → 写配置 → 复位；BOOT 侧用同一套 `Platform/Xmodem`、
`Platform/Upgrade`，只换链接脚本与分区常量。

### 3.5 移植时修的必要缺陷（6 条）

| # | 参考位置 | 触发条件 | 最小改法 |
| --- | --- | --- | --- |
| 1 | `UpgradeUser.c` `CheckUpgrade()` 自升级分支 | `EraseBootBank()`/`CopyBootFirmware()` 链接在 Flash，擦完 A/B/C 后返回取指落在已擦除 Flash | 两者与自升级尾部一起放 `.flashfunc`（RAM），尾部按"清配置 → 擦 BOOT → 回拷 → 复位"一次做完 |
| 2 | `UpgradeUser.c` 三处 `jump_to_app()` | APP 空白或半成品时直接跳 `0x086000` | 新增 `IsAppValid()`，无效则留在 BOOT 收包 |
| 3 | `Dsp280039Flash.c` `EraseSingleSector()` | 32K 扇区只按 `0x1000` 做空白检查；长度单位应为 32 位字 | `Fapi_doBlankCheck(addr, FlashSectorWords(addr) / 2UL, …)` |
| 4 | `Dsp280039Flash.c` `WriteFlashData()` | 参考按 4 字一步编排，长块尾部未对齐 64 位编程单元 | 以 `FLASH_UNIT_WORDS = 8` 整块编程、尾部补 `0xFFFF`；等待 FSM 与喂狗都在 RAM |
| 5 | `XmodemCore.c` `XmodemSinglePacket()` 首包分支 | 正文不足一包（SOH 96 / STX 992 字节）时直接 `return XMODEM_END`，正文一个字节也不写 | 解析 32 字节头后按 `u32FileSize` 裁剪再 `WriteBlock`，写成功后才累加长度 |
| 6 | `XmodemDataHandle.c` `WriteBlock()` | 忽略 `WriteFlashData()` 返回值，编程失败仍回 ACK | 返回 -1 由协议层转 `XMODEM_RETRY_ERROR`；入口再校验偶数长度与备份区边界 |

### 3.6 复核后追加的 5 条修正

| # | 参考位置 / 现场 | 触发条件 | 最小改法 |
| --- | --- | --- | --- |
| 1 | `UpgradeUser.c` `IsAppValid()` 的入口判定 | 原实现把 codestart 第二字当入口地址与 `0x086002` 比较；`Uint16` 永远小于该下界，**正常 APP 也被判无效**，BOOT 永不跳转 | 按真实两字 `LB` 指令解析：要求 `(word0 & 0xFFC0) == 0x0040`，目标 `((word0 & 0x3F) << 16) \| word1` 落在 `0x086002..0x09FFFF`；两个 16 位字为 `0xFFFF` 视为空白 |
| 2 | `UpgradeUser.c` `CheckUpgrade()` 的 APP 安装分支（参考第 197 行） | 参考忽略 APP 擦除、回拷、目标区 CRC，一律清配置后跳转——装了一半的 APP 被当成成品 | 擦除→回拷→`VerifyAppArea()` 三步全部通过才清配置并跳转；失败保留"已收完"配置（下次上电重试安装）并停在 BOOT，不跳半成品 |
| 3 | `UpgradeUser.c` `StartReceiveFromBoot()`、`main.c` 主循环 | 进入接收前不擦备份区、不看配置写入结果；主循环 `(void)TaskUpgrade()` 丢掉中止返回值 | 先擦备份区、再写配置，两步都成功才允许收包；主循环收到 2 时调 `UpgradeUserAbort()`：清会话 + 复位协议/适配层，APP 有效则启动，空白则重新准备接收 |
| 4 | `UpgradeUser.c` `BootSelfUpdateTail()` | 先清配置、随后忽略擦除与回拷结果，也没有目标区整镜像校验；任何一步失败都会复位进半成品或已擦除的 BOOT | 自擦前校验配置标志/芯片类型/镜像类型/长度/备份区 CRC；自擦后逐级检查擦除与回拷返回、用 `VerifyBootArea()` 比对目标区 CRC，全部通过才 `FlashForceReset()`；失败最多重试 3 次，仍失败则停在 RAM 喂狗，绝不复位 |
| 5 | `.cproject` Debug/Release 的 `hex --entrypoint` | 仍填 APP 地址 `0x86000` | 统一改为 `0x80000` |

**擦除后 RAM 可达性核验（反汇编）**：`BootSelfUpdateTail` 在 `0x00F997` 调用
`EraseBootBank`；该点之后可执行到的 11 条调用全部落在 `.flashfunc` 运行区
（`0x00F000..0x00FA7E`）：

```
00F99B StallAfterBootErase   00F9A6 CopyBootFirmware   00F9AA VerifyBootArea
00F9AE FlashForceReset       00FA38/00FA5E ReadFlashData 00FA45/00FA6A WriteFlashData
00FA77/00FA7A FlashFeedWatchDog  00F9A2 StallAfterBootErase
```

擦除前的三条 Flash 调用（`GetUpgradeFlag`、`GetUpgradeFirmwareFlag`、`VerifyFireware`，
位于 `0x00F962/0x00F96A/0x00F986`）此时 Flash 仍完好，可以调用。
核验脚本按"找 `EraseBootBank` 调用点 → 之后所有 `LCR/LB` 目标必须落在 RAM 运行区"
执行，结论 PASS。

### 3.7 第二轮复核追加的 3 条收口

| # | 问题 | 最小改法 |
| --- | --- | --- |
| 1 | 接收准备失败仍可能发 `'C'`：掉电重入分支忽略 `Hw_SciSetBaud()` 与 `EraseBackUpBank()` 的结果，`TaskUpgrade()` 仍可能凭旧配置开始接收；空白机与中止重入也丢弃 `StartReceiveFromBoot()` 的返回值 | 统一接收门控 `s_u16ReceiveReady`：新增 `PrepareResumeReceive()`（校验波特率/串口 → `Hw_SciSetBaud` → `EraseBackUpBank`，任一失败即不置位），`StartReceiveFromBoot()` 写完配置后才置位；`TaskUpgrade()` 首行判该标志，未就绪直接返回。三条进入路径（首次、掉电重入、中止重入）都走同一门控 |
| 2 | 配置校验在第一次备份区 CRC 之后：`CheckUpgrade()` 先 `VerifyFireware()`，CRC 长度直接取自 `u32FileSize`；Type=1 也缺长度上限 | 新增 `IsCfgValid()`：升级标志、芯片类型、镜像类型 ∈ {1,2}、非零偶数长度、按目标分区限长（APP `0x34000` / BOOT `0xC000`），**先于任何备份区读**；`VerifyFireware()` 内部再按备份区容量（`FlashBackupAreaWords * 2`）自检长度。APP 安装成功后清配置的返回值也要检查（清不掉就停下，避免下次上电重复安装） |
| 3 | Type=2 失败路径仍可能复位、且可能重复编程同一 64 位 ECC 单元 | `FlashWaitReady()` 超时行为改为可切换：`FlashSetStallOnTimeout(1)` 后超时留在 RAM 喂狗而不是 `FlashForceReset()`；`BootSelfUpdateTail()` 在擦除前调用该开关，并把"擦除 → 回拷 → 目标区校验"改成每轮重试**都先重新擦目标扇区**，三次失败后 `StallAfterBootErase()` 驻留 |

第二轮的 RAM 可达性复核（新镜像）：`EraseBootBank` 调用点 `0x00F9A5`，其后 11 条调用全部
落在 `.flashfunc` 运行区 `0x00F000..0x00FA8B`；`FlashWaitReady` @ `0x00F7D9` 本身就在 RAM，
超时分支读的是 RAM 里的 `s_u16StallOnTimeout`，不再存在"超时路径回到 Flash 取指"的可能。

### 3.8 第三轮复核追加的 2 条收口

| # | 问题 | 最小改法 |
| --- | --- | --- |
| 1 | 合法的"接收中"记录被判无效：APP 与 BOOT 写接收配置时 `bChipType`/`bFirmwareType`/`u32FileSize`/`u32FileCrc` 都是 0，`CheckUpgrade()` 却用完整镜像规则校验，记录在进 `PrepareResumeReceive()` 之前就被清掉（有有效 APP 时直接跳 APP，掉电续收失效） | 校验拆成两套：`IsReceiveCfgValid()` 只查升级标志 + 串口 + 波特率档位（供"接收中"分支），`IsImageCfgValid()` 查芯片类型/镜像类型/非零偶数长度/按分区限长（供"已收完"分支）。`CheckUpgrade()` 先按 `GetUpgradeFirmwareFlag()` 分流再校验。同时统一波特率：新增 `UPGRADE_DEFAULT_BAUD (19200UL)`，`u16UpgradeBaud = 0` 一律解释为 19200（原来重入路径写成 38400，与首次接收的 SCI-A 19200 不一致） |
| 2 | Type=2 清配置失败后仍自擦：`BootSelfUpdateTail()` 忽略配置扇区擦除结果 | 清配置的返回值纳入判定：失败直接 `return`（停在 BOOT、下次上电重试），**清成功后才 `FlashSetStallOnTimeout(1U)` 并开始擦 A/B/C**，避免旧"已收完"配置留到复位后被再装一次 |

**Type=2 的两个故障注入位置要分开看**（复核意见）：
- **清配置函数返回失败** → 不擦 A/B/C、不回拷、不复位（已由探针确认调用次数为 0）。
- **清配置阶段自身发生 FSM 等待超时** → 此时 `FlashSetStallOnTimeout()` 还没被置 1，仍按原有策略
  `FlashForceReset()` 复位；复位后配置若还在，下次上电会重新走一遍自升级（可接受）。
- **自擦开始之后的 FSM 等待超时** → `s_u16StallOnTimeout == 1`，停在 RAM 喂狗，不复位回已擦除的 Flash。

第三轮 RAM 可达性复核（新镜像）：`EraseBootBank` 调用点之后的可达调用仍全部落在
`.flashfunc` 运行区 `0x00F000..0x00FA8B`，`FlashWaitReady` @ `0x00F7D9` 在 RAM。

---

## 4. 构建

### 4.1 CCS（固定路线）

CCS 11.1 + `ti-cgt-c2000_22.6.2.LTS`，工程名 `EP15K_BOOT`：

```
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data <workspace> \
  -application com.ti.ccstudio.apps.projectImport -ccs.location D:/MinMain/CCS-Project-boot
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data <workspace> \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild -cleanBuild "EP15K_BOOT/Debug"
```

post-build（Debug/Release 都配了，entrypoint 已改为 `0x80000`）：

```
hex2bin/CalcHexAddr.exe  ${BuildArtifactFileBaseName}.hex 0x80000 0x86000 hex2bin/hex2bin.exe
hex2bin/CalcUpgradeBinCrc.exe 1 2 ${BuildArtifactFileBaseName}.bin
```

注意：删掉工程内 `Debug/` 之后必须**换一个新的 workspace 重新 import**，否则 headless
`cleanBuild` 只会报 `gmake: *** No rule to make target 'all'`（makefile 未重新生成）。

### 4.2 命令行（等价路线，不依赖 CCS 状态）

```
python tools/build_minimal.py --cgt C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS
```

链接后自动跑 `hex2000` 与 `tools/make_upgrade_bin.py --target boot`（本工程内该脚本
允许 BOOT 目标；APP 工程里的同名脚本仍保留"禁止 boot"守卫）。

---

## 5. 构建结果

两条路线用同一份源码与同一套分区；产物分别登记，**不要求跨路线哈希相同**。

| 产物 | CCS 11.1 Debug | 命令行 |
| --- | --- | --- |
| `.out` | 669,016 B `01eb6b16…` | 672,448 B `6c7239c9…` |
| `.map` | 84,314 B | 81,813 B |
| `.hex` | 76,863 B `91ec2055…` | 76,863 B `6bfc82b7…` |
| `.bin`（烧录） | 31,944 B `9bbd0413…` | 31,944 B `d41748c7…` |
| `_UpgradeCRC32.bin` | 31,976 B `f6483da9…`，CRC32 `0xA1D06C90` | 31,976 B `cb4cb275…`，CRC32 `0xF0032A83` |

CCS Debug 的一套产物同时登记在 `artifacts/boot-v1/`（`.gitignore` 对这些文件开了
例外，其余构建输出仍然忽略），可直接取用烧录文件与升级 BIN。

`.map` 的 sha256 每次构建都会变（与 APP 工程结论一致），因此只登记大小；`.out`、
`.hex`、`.bin`、升级 BIN 在各自路线内可复现（命令行路线连续两次构建，四个产物的
sha256 完全一致）。

两份 `.bin` 大小相同（31,944 B）、只差 **5 个字节**，全部是内嵌 `__FILE__` 字符串里的
`\` 与 `/`（driverlib `ASSERT` 宏），因此两份升级 BIN 的 CRC32 不同（`0xA1D06C90` 与 `0xF0032A83`）。属**实施方核对记录**，未做独立复现，不作为验收依据。

内存（两份 map 完全一致）：

| 区 | 用 | 余 |
| --- | --- | --- |
| `FLASH_BOOT`（0x080002，0x5FFE 字） | `0x3E51` | `0x21AD` |
| `RAMD0`（`.TI.ramfunc`） | `0x00C3` | `0x073D` |
| `RAMGS0`（`.bss`） | `0x0976` | `0x068A` |
| `RAMGS1` | `0x000C` | `0x0BF4` |
| `RAMGS2`（`.upybuf`） | `0x0840` | `0x07C0` |
| `RAMGS3`（`.flashfunc` 运行区） | 0 | `0x1000` |

关键符号落位：`codestart` @ `0x080000`（`LB 0x082EB3`）、`.TI.ramfunc`
LOAD `0x082ED8` RUN `0x00B000`、`.flashfunc` LOAD `0x083010`（`0xA8B` 字）
RUN `0x00F000`——其中 `FlashFeedWatchDog` @ `0x00F79F`、`FlashForceReset` @ `0x00F7A9`、
`FlashWaitReady` @ `0x00F7D9`、`VerifyBootArea` @ `0x00F94A`、`StallAfterBootErase` @ `0x00F962`、
`CopyBootFirmware` @ `0x00FA25`（`EraseBootBank` 的调用点在 `0x00F9A5`）。范围利用率 64.9%
（31,560 / 49,152 字节）。

编译告警 4 条，全在参考 `XmodemCore.c`（未引用局部变量 `u32Sum`/`u16DataLength`、
两处 `#190-D` 枚举混用），按忠实移植保留。

构建日志：`logs/ccs_boot_build.log`（CCS 11.1 一次完整 Clean Build）、
`logs/build_minimal.log`（命令行全量，含两路线 `.bin` 的 sha256）。

---

## 6. 上板必须确认的 7 条

1. 上电后 APP 有效（`0x086000` 处为 `LB` 且目标落在 `0x086002..0x09FFFF`）时直接跳转，
   LED/串口无输出属正常。
2. APP 空白或半成品时 BOOT 擦备份区、写"接收中"配置后留在 BOOT，发 `'C'` 能收包。
3. 擦除 H/I/J/K 期间看门狗不误复位（`FlashFeedWatchDog` 走 RAM）。
4. 收包成功 → 备份区 CRC 通过 → 写配置 `0xAAEE` → 复位 → 再次上电回拷 APP 并跳转。
5. 中止（两次 CAN / 重试超限 / CRC 不符）后配置被清、会话残留被清，APP 有效则启动，
   否则可重新收包；功率输出保持关闭。
6. BOOT 自升级（FirmwareType=2）能完成"校验 → 清配置 → 擦 A/B/C → 回拷 → 目标区
   CRC 通过 → 复位"，复位后仍是新版本；中途失败应停在 RAM（看门狗喂狗），不得复位。
7. 自升级失败注入两处：①回拷失败（停在 RAM，复位不再发生）；②**FSM 等待超时**——
   这是最容易绕过 RAM 驻留分支的一条，必须确认它同样停在 RAM 而不是复位。
8. 接收门控：人为让备份区擦除失败（或在擦除中途断电），确认 BOOT 不会开始发 `'C'`，
   而是停在 BOOT；恢复后重新上电可再次进入接收。
9. 配置损坏注入：把配置区改成非法长度（0 或奇数或超上限），确认 BOOT 直接清配置、
   不读备份区、按 APP 是否有效决定跳转或接收。
10. 掉电续收：接收中途断电，重新上电后应凭"接收中"记录继续接收（芯片类型/镜像类型/
    长度此时都是 0，不能被当成坏配置清掉），且波特率与首次接收一致（19200）。
    **主机侧约定：重新握手后必须从首包（包号 1）重发。** BOOT 在续收前会先 `EraseBackUpBank()`，
    备份区被清空、写地址从 `0x0A0000` 重新开始，续传旧包号会被 `XmodemSinglePacket()` 判成
    序号错（`XMODEM_SEQUENCE_ERROR`）而 NAK。
11. Type=2 清配置失败注入：让配置扇区擦除失败，确认 BOOT 不擦 A/B/C、不回拷、不复位。

---

## 7. 本步未覆盖

- 没有主机测试：参考模块与 SCI/F021/看门狗/复位强耦合，验收以上板升级成功为准。
- 只构建了 Debug；Release 配置的 include/postbuild/entrypoint 已同步，但未产出产物。
- BOOT→APP 跳转只校验入口 `LB` 与目标地址，不校验 APP 自身 CRC（完整性由升级流程的
  备份区校验保证）。
- APP 安装失败时策略是"保留配置 + 停在 BOOT、下次上电重试安装"；若备份区本身损坏，
  重试时 `VerifyFireware()` 不通过会清配置并重新进入接收。
- Type=2 自升级本轮仍未上板；"FSM 等待超时驻留"只能通过故障注入确认（正常板测不会
  触发 0x1FFFF 次等待超时）。
- `Platform/` 下的 `PortBench.h`/`PortBudget.h` 随端口层带入，`PORT_BENCH_TEST`
  保持 `0U`（生产配置），未提供测量态构建。

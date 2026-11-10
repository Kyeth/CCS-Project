# 05F 申请文档（r3）：XMODEM 接收按参考工程移植进 APP

| 项目 | 内容 |
| --- | --- |
| 版本 | r3（2026-11-10）——按裁决改为**忠实移植参考 BootLoader**，撤销 r2 的自造框架 |
| 基准 | `test/board-bringup-20261031`，`1093060`（r2 文档提交） |
| 移植依据 | `C:\Users\86130\Desktop\02_程序与代码\IB800系列\IB800_Boot1\IB800U_BootLoader`，HEAD `68701bd` "Initial import: IB800U_BootLoader (C2000 F28003x bootloader)"，工作区干净 |
| 验收 | 以实际升级成功为准，不追加性能指标 |

r2 按"先自造 Xmodem/Upgrade 框架、再往参考接口上靠"的方式实施，偏离裁决。r3 撤销 r2 全部新增实现（`git rm`），把参考工程的 20 个源文件逐字搬入，只做裁决允许的五类必要改动，并把确认的功能缺陷逐条修掉（§4.5，每项注明参考位置、触发条件、最小改法）。

## 0. 结论先行

1. **模块划分、函数接口、状态机、升级配置结构、处理流程全部保持参考工程原样**：`XmodemCore` / `XmodemDataHandle` / `XmodemTimeOut` / `XmodemTramsport` / `XmodemSlaveProcess` / `UpgradeCore` / `DspUpgradeCrc` / `UpgradeUser` / Flash 驱动的文件名、函数名、结构体字段一律不改。
2. **必要改动只有五类**：芯片适配（设备头、F021、扇区、链接与 RAM 搬运）、端口适配（读写内部接 `SciPort`，在适配层累计完整包）、时基适配（`Hw_TimeBase`，保持 ms 单位）、工程接线（握手→接收→校验→写配置→复位）、必要缺陷修复（§4.5 共 7 条）。
3. **APP 与 BOOT 共用同一套升级实现**：本步落地的 `Platform/Xmodem` + `Platform/Upgrade` + `Platform/Flash` + `User/XmodemSlaveProcess` + `User/Upgrade` 可整目录搬进 BOOT 工程，改的只有分区地址常量与 `jump_to_app()`。
4. **握手仍按 05E 的约定**：Modbus 回 6+CRC 共 8 字节；`'C'` 由升级层在备份区擦完后由 `XmodemReceive()` 自己在 `STATE_IDLE` 发出（参考工程的行为）。交接时把握手同批多取的尾部字节交给升级层，不清环、不重初始化。
5. **本轮没有主机测试**：参考模块与 SCI/F021/看门狗/复位强耦合，r2 的替身测试随自造框架一起撤销；验收以上板升级成功为准（§8 列出必须上板确认的点）。

## 1. 范围与不做

**做**：`Platform/Xmodem/*`、`Platform/Flash/*`、`Platform/Upgrade/*`、`User/XmodemSlaveProcess/*`、`User/Upgrade/*`（参考文件入库与五类适配）；`UserApp/{Inc,Src}/AppUpgrade.*`（薄接线）；`AppModbus.c` 的握手应答与端口交接；`main.c` 的初始化、任务注册、RAM 段搬运、暂停钩子；`CMD` 的新增段；`build_minimal.py` 的源列表、include 与 F021 库。

**不做**：BOOT 工程本体；`CheckUpgrade()` 的回拷与跳转（属 BOOT）；分区表调整；Modbus 功能变更（除握手应答长度与交接）。

### 1.1 升级期间的任务与中断（沿用裁定）

| 类别 | 处置 |
| --- | --- |
| 功率输出（PWM 发波） | 停止，升级全程保持停止，中止后不自动恢复 |
| 采样/控制动作 | `AdcSampleTaskGuarded()` 在保持期内直接返回 |
| 端口服务 | **保留**：擦除期间 `'C'`/ACK 靠主循环 `PortTaskService()` 出环 |
| 时基、看门狗 | 保留；F021 等待循环里 `Hw_FeedWatchDog()` |
| Modbus（C/D 两口） | 未暂停（只把 SCI-A 这个端口交出去） |

## 2. 参考实现（`68701bd`）关键点

- 包解析：`XmodemSinglePacket()` 按 `u16xBuff[0]` 定长度（SOH 128 / STX 1024），序号反码校验 `u16xBuff[1] == ((~u16xBuff[2]) & 0xFF)`，CRC16-CCITT（poly 0x1021、初值 0、**高字节在前**）。
- 首包：`u16xBuff[3]` 芯片类型、`[4]` 固件类型、`[5..8]` 长度（小端）、`[9..12]` CRC32（小端），正文从 `u16xBuff[35]` 起，共 96 或 992 字节。
- 后续包：`u32RecvLength += len`；超过文件长度时截断并置 `bRecvOver`；每包后 `u32AddrOffset = u32RecvLength >> 1`（**字**偏移）。
- 存储：`WriteBlock(u32BlockNum, buf, len)` 把两个字节拼成一个 16 位字，写入 `0x0A0000 + u32BlockNum`。
- 结束：`EOT` 两次确认（首次 NAK、二次 ACK）→ `XMODEM_END`；`CAN` 两次 → `XMODEM_CANCEL`；重试用尽 → 连发三个 CAN。
- 配置：`ST_UPGRADE_CFG` 12 个 16 位字，`u32UpgradeCfgSum = CalcUpgradeCfgSum()` 校验；`u16UpgradeFlag`/`u16FirmwareOver` = `0xAAEE`。
- 状态机：`STATE_IDLE`（发 `'C'`）→ `STATE_WAIT_FOR_START` → `STATE_PROCESS_PACKET` → `STATE_RECEIVE_DATA` … → `STATE_COMPLETE` / `STATE_ERROR`。

## 3. 移植对照表

| 参考路径 | 本工程路径 | 参考行数 | 现行数 | 改动 |
| --- | --- | --- | --- | --- |
| `Platform/Xmodem/Inc/XmodemCore.h` | 同名 | 152 | 152 | 无 |
| `Platform/Xmodem/Inc/XmodemDataHandle.h` | 同名 | 32 | 32 | 无 |
| `Platform/Xmodem/Inc/XmodemTimeOut.h` | 同名 | 45 | 45 | 无 |
| `Platform/Xmodem/Inc/XmodemTramsports.h` | 同名 | 28 | 31 | `Write` 改 `const Uint16 *` |
| `Platform/Xmodem/Src/XmodemCore.c` | 同名 | 524 | 543 | 设备头、两处缺陷 |
| `Platform/Xmodem/Src/XmodemDataHandle.c` | 同名 | 51 | 57 | 基址、`.upybuf`、缺陷 |
| `Platform/Xmodem/Src/XmodemTimeOut.c` | 同名 | 58 | 58 | 时基 |
| `Platform/Xmodem/Src/XmodemTramsport.c` | 同名 | 32 | 161 | 端口适配（整包累计） |
| `Platform/Flash/Inc/Dsp280039Flash.h` | `Platform/Flash/Inc/Dsp2837xSFlash.h` | 106 | 75 | 扇区表、F021 入口 |
| `Platform/Flash/Src/Dsp280039Flash.c` | `Platform/Flash/Src/Dsp2837xSFlash.c` | 153 | 286 | `_F2837xS`、8 字编程、两处缺陷 |
| — | `Platform/Flash/Src/Fapi_UserDefinedFunctions.c` | — | 39 | 新增（TI 库要求 + `__error__`） |
| `Platform/Upgrade/Inc/UpgradeCore.h` | 同名 | 76 | 76 | 无 |
| `Platform/Upgrade/Inc/DspUpgradeCrc.h` | 同名 | 20 | 20 | 无 |
| `Platform/Upgrade/Src/UpgradeCore.c` | 同名 | 163 | 192 | `.upybuf`、按字数拷贝、补 return |
| `Platform/Upgrade/Src/DspUpgradeCrc.c` | 同名 | 46 | 46 | 无 |
| `User/XmodemSlaveProcess/Inc/XmodemSlaveProcess.h` | 同名 | 24 | 24 | 无 |
| `User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c` | 同名 | 42 | 40 | 设备头 |
| `User/Upgrade/Inc/UpgradeUser.h` | 同名 | 32 | 36 | 加 `EraseBackUpBank`/`stUpgradeHandle` 声明 |
| `User/Upgrade/Src/UpgradeUser.c` | 同名 | 211 | 220 | 地址、扇区、非阻塞发送、两处缺陷 |
| — | `UserApp/{Inc,Src}/AppUpgrade.*` | — | 44 + 131 | 新增薄接线 |
| — | `UserApp/Src/AppModbus.c` | — | — | 握手 8 字节 + 交接 |

## 4. 五类必要改动

### 4.1 芯片适配

1. **设备头**：8 个文件 `#include "F28003x_device.h"` → `"F2837xS_device.h"`。
2. **F021**：`Dsp2837xSFlash.c` 在包含 `F021.h` 前 `#define _F2837xS`（F2837xS 分支才提供 `F021_CPU0_W0_BASE_ADDRESS`），`Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, 200)`。
3. **扇区表**：A~D、K~N 为 8K 字（`SectorSmall_u16length 0x2000`），E~J 为 32K 字（`SectorLarge_u16length 0x8000`）；分区起点 `BOOT 0x80000 / APP 0x86000 / BACKUP 0xA0000 / CFG 0xBE000`；擦除按地址自动 `Fapi_setActiveFlashBank`。
4. **链接**：`.upybuf` → `RAMGS2`（`u16XmodemDataBuff[1024]` + `u16AppBuff[1024]`）；`.flashfunc` GROUP 收 F021 库与本工程擦写函数，`LOAD = FLASH_APP`、`RUN = RAMGS3_RUN`（RAMGS3 在 PAGE 1，程序段要 PAGE 0 别名）。
5. **RAM 搬运**：`main()` 第一行 `CopyRamSections()`，把 `.TI.ramfunc`（→RAMD0）与 `.flashfunc`（→RAMGS3）从 Flash 拷到 RAM。此前工程里完全没有搬运代码。
6. **擦写全程在 RAM**：`Dsp2837xSFlashInit/EraseSingleSector/FlashProgramWords/WriteFourWord/WriteFlashData/ReadFlashData` 全部 `#pragma CODE_SECTION(..., ".flashfunc")`，`Fapi_issueProgrammingCommand()` 之后**在同一 RAM 函数里等到 FSM 就绪、检查状态、回读校验**才返回调度器。

### 4.2 端口适配（`XmodemTramsport.c`）

参考工程依赖"驱动层已按包断帧"（`XmodemRead` 直接读一整包）。本工程 `SciPortPopBatch()` 一次最多 8 字节，所以在适配层累计整包：

- `XmodemFrameLength(首字节)`：SOH → 133、STX → 1029、CAN → 2、其它 → 1。
- 循环 `SciPortPopBatch(HW_SCI_ID_A, au16Batch, 8, &stSeg)` 拼到 `s_u16FrameFill` 达到整包长度才返回。
- 半包静默超过 100 ms 丢弃，等主机重发（`Hw_TimeBaseElapsed` 用批次自带的 `stSeg.u32Tick`）。
- 一条批次跨两包时，把本包之后多出来的字节留到下一次读，避免下一包缺头。
- `XmodemRxPreload(tail, len)`：握手同批多取的字节先还给读路径。
- `XmodemWrite` → `SciPortWriteTx(HW_SCI_ID_A, ...)`，返回 0 表示环满，协议层当"没发出去"处理。

### 4.3 时基适配（`XmodemTimeOut.c`）

`TimeStart()` 记 `Hw_TimeBaseTicks()`、`u32TimeOut = Hw_TimeBaseUsToTicks(u32Ms * 1000)`；`IsTimeOut()` 用 `Hw_TimeBaseTicks() - u32StartTick > u32TimeOut`。接口名、参数单位（ms）、`u16IsActive` 语义全部保持参考工程原样。

### 4.4 工程接线

`AppModbus`（05E 层）：握手应答 9→**8** 字节（6 字节回显 + CRC），判据改前缀匹配（`u16Len >= 6`），把 `pu16Data[6..]` 存进 `s_au16HandshakeTail`；应答真正发完后（`bArmPending` 清）调 `AppUpgradeStart(HW_SCI_ID_A, tail, len)` 并停止收帧；`AppModbusReleasePort()` 在会话结束时清候选、清回复、`SciPortResync()`。

`AppUpgrade`（薄接线，三态）：

| 阶段 | 动作 |
| --- | --- |
| `WAIT_TX` | 等 8 字节握手应答出环（`SciPortIsTxIdle`），期间不擦除 |
| `ERASE` | `EraseBackUpBank()`（H/I/J/K 四个真实扇区）+ 写"进行中"配置（`u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0`、校验和） |
| `RECEIVE` | 直接调参考工程的 `TaskUpgrade()`，由它跑 `XmodemReceive()`、收完写配置并复位 |

"进行中"配置必须在交接时写：参考工程的 `TaskUpgrade()` 守卫是 `ReadUpgradeCfg() == 1 && GetUpgradeFirmwareFlag() == 0`，而参考工程是由 Modbus 侧写这份配置的。

`main.c`：`ApplicationInit()` 末尾 `AppUpgradeInit()` + `AppUpgradeSetPauseHook(AppUpgradePause)`；`TaskCreate()` 里 `Task_Start(AppUpgradeTask, 10U, 0U)`（与参考工程同周期）。

### 4.5 必要缺陷修复清单（7 条）

| # | 参考位置 | 触发条件 | 最小改法 |
| --- | --- | --- | --- |
| 1 | `Dsp280039Flash.c` `ReadFlashData()`：`memcpy(pu16DataBuff, u32TagAddr, u32DataLen)` | 任何回拷/配置回读 | 把字地址当指针、长度按字节，实际只拷一半字数 → 改为按字地址逐字读 |
| 2 | `Dsp280039Flash.c` `WriteFlashData()`、`UpgradeCore.c` `CopyAppFirmware()/CopyBootFirmware()`、`XmodemCore.c` `XmodemTransmit()` | 调用即触发 | 声明为 `int16`/`int` 却无 `return`，返回值不确定 → 补 `return` |
| 3 | `Dsp280039Flash.c` `WriteFourWord()` + `WriteFlashData()` 步进 4 字 | 每次写 Flash | 同一 64 位编程单元被写两次（ECC 也写两次），F2837xS 上会编程出错 → 改为 8 字（64 位单元）整块写，末块补 `0xFFFF`，`WriteFourWord` 保留为兼容壳 |
| 4 | `Dsp280039Flash.c` `EraseSingleSector()`：`Fapi_doBlankCheck(addr, 0x1000, ...)` | 擦 32K 扇区 | 固定检查 0x1000 字，只覆盖八分之一 → 按该地址所在扇区的真实长度检查 |
| 5 | `XmodemDataHandle.c` `WriteBlock()` | Flash 编程失败 | 忽略 `WriteFlashData()` 返回值，失败也返回字节数，协议层照样回 ACK（数据丢了却当成功） → 失败返回 -1 |
| 6 | `XmodemCore.c` `XmodemSinglePacket()` 两处 `WriteBlock()` 调用 | `WriteBlock()` 返回 -1 | 返回值未检查 → `< 0` 时返回 `XMODEM_IO_ERROR`，让主机重传 |
| 7 | `UpgradeUser.c` `TaskUpgrade()` 的 `XMODEM_END` 分支 | ①文件长度恰好是包正文长度的整数倍；②备份区校验不过 | ①完成判据是 `GetFileWriteOver()`，该位只在"收到的字节数超过文件长度"时才置，长度刚好整除时永远不置位，会重发 `'C'` 直到重试用尽 → 增加 `u32RecvLength >= u32FileSize` 判据；②参考工程直接写配置并复位，把校验留给 BOOT 的 `CheckUpgrade()`，本工程 APP 侧没有该函数，坏包会被登记成"已收完" → 写配置前先 `VerifyFireware()`，不过就清配置并复位 |

另有一条**约束而非缺陷**：整条链路按 `u32FileSize >> 1` 个字做校验与搬运，奇数长度会丢最后一个字节。已在首包解析处加保护：长度为 0 或奇数直接返回 `XMODEM_IO_ERROR`（`XmodemCore.c` 首包分支），并要求打包工具按偶数长度补齐。

## 5. 升级时序

1. 主站发 `01 10 04 38 00 02`（6 字节，无 CRC）到 SCI-A。
2. APP 的 Modbus 桥接识别前缀 → 回 8 字节（`01 10 04 38 00 02 CRClo CRChi`）→ 应答出环后把端口交给升级层。
3. 升级层擦 H/I/J/K 四扇区 + 写"进行中"配置 → `XmodemReceive()` 在 `STATE_IDLE` 发 `'C'`。
4. 主机发首包（SOH 或 STX，含 32 字节头）与后续包；每包合法则 ACK，非法则 NAK。
5. 主机发两次 EOT → 二次 ACK → `XMODEM_END`。
6. 结束判据满足 → `VerifyFireware()`（CRC32 over 备份区前 `FileSize>>1` 字）→ 写 `ST_UPGRADE_CFG`（`u16FirmwareOver = 0xAAEE`）→ 使能看门狗并 `SysCtl_resetDevice()`。
7. 下一次上电由 BOOT 的 `CheckUpgrade()` 按 `bFirmwareType` 回拷到 APP/BOOT 区再跳转（不在本步）。

## 6. 构建与内存（生产态 `PORT_BENCH_TEST=0U`）

- `.out` 1,022,792 B，sha256 `1981da32e13cf0f6acfb7bedd08718eca843ae32660fb4147a9517e7ae9b1e49`（两次构建一致）；`.map` 127,630 B。
- 5 条 warning 全部在参考 `XmodemCore.c`（未引用局部变量 3 条、`#190-D` 2 条），按"忠实移植"保留原样。
- 段占用：`.text` `0x625E`、`.const` `0x14B9`、`.flashfunc` `0x8FB`（RUN `0xF000`，余 `0x705`）、`.TI.ramfunc` `0xC3`（RUN `0xB000`，余 `0x73D`）、`.upybuf` `0x800`（RAMGS2 余 `0x800`）、`.mbregs` `0x670`。
- 内存余量：`FLASH_APP` 用 `0x82D1` 余 `0x11D2D`；`RAMGS0` 用 `0xFFD` **余 3 字**；`RAMGS1`（0xC00）用 `0xB0A` 余 `0xF6`。

## 7. 两态构建台账

| 状态 | `.out` 大小 / sha256 | `.map` | `FLASH_APP` 用/余 | `RAMGS0` 余 | `RAMGS1` 余 | `RAMGS2` 余 |
| --- | --- | --- | --- | --- | --- | --- |
| 生产 `PORT_BENCH_TEST=0U` | 1,022,792 B / `1981da32e13cf0f6acfb7bedd08718eca843ae32660fb4147a9517e7ae9b1e49` | 127,630 B | `0x82D1` / `0x11D2D` | 3 | `0xF6` | `0x800` |
| 测量 `PORT_BENCH_TEST=1U` | 1,040,780 B / `7310e8458dc4f39623a6f4b93099e64f3eb1104eebe305e768502cc91e4973ab` | 132,545 B | `0x8653` / `0x119AB` | 0 | 1 | `0x7C7` |

两态各 55 个编译单元、0 错误；`.out` 可复现（生产态两次构建一致），`.map` 每次构建的 sha 都变，只作信息项。工作树已切回生产态。

## 8. 必须上板确认的点

1. `CopyRamSections()` 是否把 `.flashfunc` 正确搬到 `0xF000`（`_FlashfuncLoadStart`/`_FlashfuncRunStart` 链接符号）。
2. F021 在 200 MHz 下的擦写是否成功：`EraseSingleSector()` 返 0、`Fapi_doBlankCheck()` 通过、编程后 `Fapi_doVerify()` 通过。
3. 主站发两次 EOT 后是否收到第二次 ACK、然后板子复位、配置区被写成 `u16FirmwareOver = 0xAAEE`。
4. `RAMGS0` 只剩 3 字（测量态为 0），上板前不要再往 `.bss` 加静态变量；若需加，先移到 `.upybuf`/RAMGS3。
5. 升级中止（CAN、CRC 反复错）后端口能否交还 Modbus（`AppModbusReleasePort()`）。

## 9. 复核要点

1. **是否忠实移植**：§3 对照表逐文件；除表中列出的改动，参考代码未做其他修改（`XmodemCore.c` 的两处缺陷修复见 §4.5 第 6、7 条与奇数长度保护）。
2. **必要适配是否正确**：§4.1~§4.4 每类一条；端口适配的"整包累计"是唯一一处新增逻辑（约 110 行），改动集中在 `XmodemTramsport.c`。
3. **升级能否完成**：按 §5 时序上板跑一遍，判据是配置区被写成 `0xAAEE` 且 BOOT 回拷后 APP 能正常启动。

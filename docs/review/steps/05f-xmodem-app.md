# 05F 申请文档（r5）：XMODEM 接收按参考工程移植进 APP

| 项 | 内容 |
| --- | --- |
| 步骤 | 05F |
| 状态 | r5（2026-11-10）——r4 六组适配错误已修，r5 补齐四处上板前收尾；生产态编译通过；未上板 |
| 基线 | 参考 BootLoader `IB800U_BootLoader`，HEAD `68701bd` |
| 本工程 | `CCS-Project-minimal`（F28374S），分支 `test/board-bringup-20261031` |
| 判据 | 上板实际升级成功；本步不新增性能指标 |

## 0. 结论先行

05F 的方向是**忠实移植**参考工程的 Xmodem / Upgrade 框架（模块划分、函数接口、状态机、升级配置结构、处理流程全部保留），只做五类必要改动：芯片适配、端口适配、时基适配、工程接线、必要缺陷修复。

r3 已按这个方向落地并两态编译通过，但复核（m02255）列出六处**必要适配错误**，其中三处会导致实际收包失败或越界（超时判据反向、首包短正文不写入、余字节无界写数组）、一处会在擦写期间执行 Flash 代码、一处会让 32K 扇区的空片检查与编程按错误单位进行、一处让擦写失败无法上抛。r4 把这六组全部改完，未新增任何替代框架。

本轮**没有主机测试**：参考模块与 SCI / F021 / 看门狗 / 复位强耦合，替身测试会掩盖真实时序问题，验收按 §8 的上板路径进行。生产态编译 0 错误、4 条 warning 全部来自参考 `XmodemCore.c` 原样代码（未引用局部变量、枚举混用），按忠实移植保留。

## 1. 范围与不做

**做**：Modbus 升级握手（8 字节应答）→ 端口交接 → 擦备份区 → 按参考状态机收包写备份区 → 两次 EOT → 校验 → 按参考格式写配置 → 复位。

**不做**：BOOT 工程本身（回拷到 APP 区、`jump_to_app()`、启动时的恢复判定属 `HANDOVER_20261110.md` §8，后续单独开项目空间）；不重写参考的回调接口；不引入帧队列；不优化吞吐。

### 1.1 升级期间的任务与中断（按 m01138 裁定）

| 类别 | 具体 |
| --- | --- |
| 暂停 | PWM 输出 `Hw_StopAllPWM()`；ADC 中断 `Hw_AdcSetIntEnable(HW_ADC_ID0, 0U)` + `Hw_AdcClearIntFlag(HW_ADC_ID0)`；CLA 任务中断 `Cla1Regs.MIER.all = 0U`；前台采样任务（`s_u16UpgradeHold` 使 `AdcSampleTaskGuarded()` 直接返回）；数字输出 `Hw_GpioWritePin(88U, 0U)` |
| 保留 | 时间基准 `Hw_TimeBase`（超时判据要用）；端口服务 `PortTaskService()`（`'C'`/ACK 全靠它出环，停掉会死锁）；看门狗（但擦写期间只能调 RAM 里的 `FlashFeedWatchDog()`） |
| 中止后 | 功率输出保持关闭，不自动恢复（`AppUpgradeAbort()` 只清状态、交还端口） |

## 2. 参考实现（`68701bd`）关键点

- `XmodemReceive()` 是 6 态机：`STATE_IDLE`（发 `'C'`、复位包号）→ `WAIT_FOR_START`（`TimeStart(2000)` + `Read`）→ `PROCESS_PACKET`（`XmodemSinglePacket()`）→ `RECEIVE_DATA`（等同 WAIT，超时更短）→ `COMPLETE` / `ERROR`。
- 收包单位是 `Uint16` 数组，每个元素只放一个字节；`XMODEM_BUFF_SIZE 1030`（SOH 包 133、STX 包 1029）。
- 首包正文前 32 字节是 `ST_UPGRADE_FILE_HEADER`（`bChipType:8`、`bFirmwareType:8`、`u32FileSize`、`u32FileCrc`、`u16Rsv[11]`），正文从 `u16xBuff[35]` 起；后续包正文从 `u16xBuff[3]` 起，`u32AddrOffset = u32RecvLength >> 1`——**Flash 地址按字推进**。
- CRC16 查表（poly 0x1021、初值 0、**高字节在前**）；`XmodemCalculateCheckSum` 只在 `u16IsUseCrc=0` 时用。
- `XmodemTimeOut` 是"开始计时 + 查超时"两个接口，单位毫秒；`XmodemTramsports` 是 `Read/Write` 两个函数指针；`XmodemDataHandle` 是 `ReadBlock/WriteBlock`。
- 升级配置 `ST_UPGRADE_CFG`（含 `u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0xAAEE`、`u32UpgradeCfgSum`），`UpgradeCore` 负责读配置、CRC32 校验、回拷、算校验和。
- `UpgradeUser.TaskUpgrade()` 是参考的升级任务体：守卫 → `XmodemReceive()` → 等发送排空 → 写配置 → 复位。

## 3. 移植对照表

| 参考文件 | 行数 | 本工程文件 | 行数 | 改动 |
| --- | --- | --- | --- | --- |
| `Platform/Xmodem/Inc/XmodemCore.h` | 152 | 同路径 | 162 | 设备头、新增 6 个常量（§4.4） |
| `Platform/Xmodem/Src/XmodemCore.c` | 524 | 同路径 | 573 | 首包重写、写失败终止、跳过重复应答（§4.5、§4.6） |
| `Platform/Xmodem/Inc/XmodemTramsports.h` | 28 | 同路径 | 31 | 参数改 `const Uint16 *`、加 `XmodemRxReset` 声明 |
| `Platform/Xmodem/Src/XmodemTramsport.c` | 32 | 同路径 | 197 | 端口适配重写（§4.2） |
| `Platform/Xmodem/Inc/XmodemTimeOut.h` | 45 | 同路径 | 45 | 未改 |
| `Platform/Xmodem/Src/XmodemTimeOut.c` | 58 | 同路径 | 80 | 时间源换 `Hw_TimeBase`、判据改方向（§4.3） |
| `Platform/Xmodem/Inc/XmodemDataHandle.h` | 32 | 同路径 | 32 | 未改 |
| `Platform/Xmodem/Src/XmodemDataHandle.c` | 51 | 同路径 | 68 | 基址 0x0A0000、返回检查与范围检查（§4.5） |
| `Platform/Flash/Inc/Dsp280039Flash.h` | 106 | `Platform/Flash/Inc/Dsp2837xSFlash.h` | 82 | 扇区表换 F2837xS（§4.1） |
| `Platform/Flash/Src/Dsp280039Flash.c` | 153 | `Platform/Flash/Src/Dsp2837xSFlash.c` | 341 | F021 换 f2837xs、Bank、单位、断电保护（§4.1） |
| — | — | `Platform/Flash/Src/Fapi_UserDefinedFunctions.c` | 40 | 新增：TI 库要求的用户回调 |
| `Platform/Upgrade/Inc/UpgradeCore.h` | 76 | 同路径 | 76 | 未改 |
| `Platform/Upgrade/Src/UpgradeCore.c` | 163 | 同路径 | 192 | 按字数回拷、补 return（§4.5） |
| `Platform/Upgrade/Inc/DspUpgradeCrc.h` | 20 | 同路径 | 20 | 未改 |
| `Platform/Upgrade/Src/DspUpgradeCrc.c` | 46 | 同路径 | 46 | 未改 |
| `User/Upgrade/Inc/UpgradeUser.h` | 32 | 同路径 | 34 | 两个函数返回值类型（§4.6） |
| `User/Upgrade/Src/UpgradeUser.c` | 211 | 同路径 | 255 | 地址、扇区循环、非阻塞排空、失败上抛（§4.5、§4.6） |
| `User/XmodemSlaveProcess/*` | 24+42 | 同路径 | 40 | 只换设备头 |
| — | — | `UserApp/{Inc,Src}/AppUpgrade.*` | 44+193 | 新增：APP 侧接线（§4.7） |
| `UserApp/Src/AppModbus.c` | — | 同路径 | 714 | 握手 9→8 字节、交接（r3 已改，本轮未动） |

## 4. 五类必要改动

### 4.1 芯片适配（F280039C → F28374S）

- **设备头**：8 个参考源 `#include "F28003x_device.h"` → `"F2837xS_device.h"`。
- **Flash API**：`Platform/Flash/Src/Dsp2837xSFlash.c` 在 include F021 前 `#define _F2837xS`（`Constants.h` 只有在这个分支里定义 `F021_CPU0_W0_BASE_ADDRESS`），`Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, 200)`；库用 `D:/ti/C2000Ware_5_00_00_00/libraries/flash_api/f2837xs/lib/F021_API_F2837xS_FPU32_EABI.lib`。
- **只有一个 Bank**：删除参考按地址选 Bank 的逻辑，`Dsp2837xSFlashInit()` 固定 `Fapi_setActiveFlashBank(Fapi_FlashBank0)` 并检查返回值（-1 = 设 Bank 失败，-2 = `Fapi_ClearMore` 失败）。BOOT（0x080000）、APP（0x086000）、BACKUP（0x0A0000）、CFG（0x0BE000）在本器件全部位于 Bank0。
- **扇区表**：A~D / K~N 为 8K 字，E~J 为 32K 字；`FlashSectorWords()` 按地址给长度。
- **空片检查单位**：`Fapi_doBlankCheck()` 的长度参数单位是 **32 位字**，传 `FlashSectorWords(addr) / 2UL`（原来传 16 位字数，检查范围扩大一倍：擦 H 会连 I 一起查，擦 N 甚至查到器件外）。
- **RAM 执行**：`Fapi_*` 库与 `Dsp2837xSFlash.c` 的四个函数全部收进 `.flashfunc`（链接脚本 `GROUP`，`LOAD = FLASH_APP`、`RUN = RAMGS3_RUN`）；`main()` 首行 `CopyRamSections()` 搬 `.TI.ramfunc` 与 `.flashfunc`（本工程此前完全没有搬运代码，`.TI.ramfunc` 里的函数会跳到未初始化 RAM）。
- **擦写期间不执行 Flash 代码**：`FlashSaveInt()`/`FlashRestoreInt()` 保存并屏蔽 `IER`，每条返回路径都恢复；等待循环里的喂狗换成 RAM 内的 `FlashFeedWatchDog()`（直接写 `WdRegs.WDKEY`，绕开链接在 Flash 的 `ServiceDog()`，map 实测在 0x8C1FC）；`__error__()` 与 `Fapi_serviceWatchdogTimer()` 同样落 `.flashfunc`。
- **等待超时**不返回 Flash 调用者：`FlashForceReset()` 直接配置看门狗（`WDCR = 0x0028`、`WDCNTR = 0`）并死等复位。

### 4.2 端口适配（`XmodemTramsport.c`）

参考的 `Read` 是"一次读一包"，而本工程端口层按 ≤8 字节的批次交付，所以适配层负责累计：

- `XmodemRead()` 先倒出上次的余字节，再按首字节算本包长度（SOH 133 / STX 1029 / CAN 2 / 其它 1），循环 `SciPortPopBatch()` 直到收满；**收满就停止取批**，一批里多出来的字节只进 `s_au16Preload`（限 `sizeof/sizeof[0]`），留到下次读。
- 半包静默 100 ms 丢弃（`XmodemRxReset()`）；`XmodemRxPreload()` 在会话开始时先清状态再装入握手尾部，避免上次的半包混进新会话。
- `XmodemWrite()` → `SciPortWriteTx(HW_SCI_ID_A, ...)`，返回 0 表示端口忙，参考层据此重试下一轮。
- 余字节缓存放 `.upybuf`（RAMGS2），不再占已满的 RAMGS0。

### 4.3 时基适配（`XmodemTimeOut.c`）

`Hw_TimeBase` 是**向下计数**的自由计数器，经过时间 = 起始值 − 当前值，所以超时判据是 `(Uint32)(u32StartTick - u32NowTick) > u32TimeOut`（r3 写成 `u32NowTick - u32StartTick`，时间源一读出来就远大于超时，等待立即判超时——复核探针复现了这一点）。两侧都加了 `Hw_TimeBaseIsReady()` 与空指针检查；接口与毫秒单位保持参考原样。

### 4.4 工程接线

- Modbus 握手：`01 10 04 38 00 02`（6 字节）→ 回 8 字节（6 字节回显 + CRC，低字节在前）；判据是前缀匹配，多出来的字节存进 `s_au16HandshakeTail`。应答真正发完之后才 `AppUpgradeStart()`，并把尾部字节交给升级层。
- `AppUpgrade` 三态：`WAIT_TX`（等应答发完）→ `ERASE`（逐扇区擦 H/I/J/K，再写一份"进行中"配置——参考工程由 Modbus 侧写这份配置，本工程在交接时写，否则 `TaskUpgrade()` 的守卫不通过）→ `RECEIVE`（`TaskUpgrade()`）。任务周期 10 ms，与参考一致。
- 新增常量：`XMODEM_FILE_HEAD_LEN 32`、`XMODEM_CHIP_TYPE 1`（与 `CalcUpgradeBinCrc.exe "1" "1" ...` 的首参数一致）、`XMODEM_FIRMWARE_APP 1`、`XMODEM_FIRMWARE_BOOT 2`、`XMODEM_APP_MAX_BYTES 0x34000`、`XMODEM_BOOT_MAX_BYTES 0xC000`。

### 4.5 必要缺陷修复清单（11 条，含本轮六组）

| # | 参考位置 | 触发条件 | 最小改法 |
| --- | --- | --- | --- |
| 1 | `Dsp280039Flash.c` `ReadFlashData` | 回拷、配置回读 | 逐字 volatile 读（不再依赖 `memcpy` 的长度单位） |
| 2 | `Dsp280039Flash.c` `WriteFlashData`/`CopyAppFirmware`/`CopyBootFirmware`/`XmodemTransmit` 声明非 void 却无 return | 任何调用 | 补 `return` |
| 3 | `XmodemDataHandle.c` `WriteBlock` | 编程失败 | 检查 `WriteFlashData()` 返回值并返回 -1 |
| 4 | `XmodemCore.c` 三处 `WriteBlock` 调用 | 上一条触发 | 返回负值即终止会话（`XMODEM_RETRY_ERROR`），不再写同一 ECC 单元 |
| 5 | `XmodemCore.c` 首包分支 | 首包正文短于整包、文件小于 128 字节 | 按 `u32FileSize` 裁剪首包正文后再写；写成功才累加长度（原来直接 `return XMODEM_END`，一个字都不写） |
| 6 | `XmodemCore.c` 首包解析 | 机型/类型/长度非法 | 校验 ChipType、FirmwareType∈{1,2}、长度非零且偶数、≤ 0x34000/0xC000，非法直接终止 |
| 7 | `UpgradeUser.c` `TaskUpgrade` 完成判据 | 文件长度正好整除包长 | 补 `u32RecvLength >= u32FileSize`（原来只看 `bRecvOver`，永不置位） |
| 8 | `UpgradeUser.c` `TaskUpgrade` 写配置前 | 备份区校验不过 | 先 `VerifyFireware()`，不过就擦配置并返回中止（原来不校验直接写配置复位） |
| 9 | `XmodemCore.c` 状态机 | 写失败后 | 已发 CAN 并终止，跳过本轮应答与重试计数（原来还会再写一个 NAK） |
| 10 | `EraseSingleSector` 空片检查 | 32K 扇区、末扇区 | 长度按 `FlashSectorWords(addr)/2` |
| 11 | `XmodemTramsport.c` 余字节 | 一条批次跨两包 / 短控制帧后仍有积压 | 收满即停，余字节入限长缓存（原来无界写 64 字数组，主机 UBSan 报下标 64 越界） |

### 4.6 本轮（r4）按复核意见的六组修正

| 组 | 复核位置 | 改法 |
| --- | --- | --- |
| 1 | `FlashSelectBank()` + `Fapi_doBlankCheck()` | 删掉 Bank1/2 选择，固定 Bank0 并检查返回值；空片检查长度改 32 位字 |
| 2 | 擦写期间仍可能执行 Flash 代码 | 保存并屏蔽 `IER`、每条返回路径恢复；等待循环与 `__error__`/`Fapi_serviceWatchdogTimer` 全改 RAM 内 `FlashFeedWatchDog()`；超时未就绪走 `FlashForceReset()` 不返回；暂停钩子补停 ADC 中断与 CLA |
| 3 | `XmodemTimeOut.c` 差值方向 | 改 `(Uint32)(u32StartTick - u32NowTick)`，并检查时间源有效性 |
| 4 | 首包短正文、文件范围、写失败 | 首包按文件剩余裁剪并写入；补 ChipType/FirmwareType/上限校验；`WriteBlock` 检查备份区范围；写成功后才累计长度；已部分编程则终止会话 |
| 5 | 余字节无界写 | 收满即停、只保存最后一批的余字节；`sizeof/sizeof[0]` 代替 `/2U`；会话开始/取消清状态 |
| 6 | 擦写失败与中止收尾 | `EraseBackUpBank()`/`AppUpWriteProgressCfg()`/最终配置擦写逐级返回失败；`TaskUpgrade()` 返回 2 交给 `AppUpgradeAbort()`：清状态、重新初始化 Xmodem、`AppModbusReleasePort()`、功率保持关闭 |

**两处理由更正**（r3 文档写错，此处更正，避免后续移植继续用错单位）：

1. 4 个 16 位字就是 64 位、8 个是 128 位。参考的"每次编程 4 字"本身**就是**一次完整的 64 位编程单元，不存在"必然重复写同一单元"。仍按 8 字（128 位）分块，理由是让块边界与扇区尾部补齐规则统一，不是因为 4 字有缺陷。
2. C28x 上 `sizeof(Uint16) == 1`（字节 = 16 位），`memcpy` 的长度按本机字节计，所以参考的 `memcpy(pu16DataBuff, addr, len)` 与原意等价，不能笼统称"只拷一半"。改为逐字 `volatile` 读的理由只是"不依赖库实现细节"。

### 4.7 APP 侧接线（`UserApp/AppUpgrade.*`）

只做三件事：初始化（`XmodemProcessInit()` + `Dsp2837xSFlashInit()` + `UpgradeUserInit()`，并记录 Flash API 是否可用）、会话管理（`AppUpgradeStart()` 移交尾部字节并置 `WAIT_TX`）、三态任务（§4.4）。`AppUpgradeAbort()` 是唯一的中止出口，`AppUpgradeAbortCount()` 供观测。

## 5. 升级时序

1. 主站发 `01 10 04 38 00 02`（6 字节，无数据段、无 CRC）。
2. `AppModbus` 回 8 字节（回显 + CRC）；应答**发完**后调用 `AppUpgradeStart()`，把同批多出的字节作为预置数据交给升级层，Modbus 停止收帧。
3. `WAIT_TX` → `ERASE`：逐扇区擦 H (`0x0A0000`, 0x8000)、I (`0x0A8000`, 0x8000)、J (`0x0B0000`, 0x8000)、K (`0x0B8000`, 0x2000)，任一失败即中止；随后擦配置扇区 N (`0x0BE000`) 并写"进行中"配置（`u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0`）。
4. `RECEIVE`：`XmodemReceive()` 在 `STATE_IDLE` 发 `'C'`，逐包收；每包写备份区，写成功才回 ACK。
5. 主站发两次 EOT → 协议层回 NAK/ACK 并置 `XMODEM_END`（正文此时只是收完，尚未校验）。
6. `TaskUpgrade()`：等 `SciPortIsTxIdle(HW_SCI_ID_A)` → `VerifyFireware()`（CRC32 覆盖备份区正文）→ 填 `ST_UPGRADE_CFG` → 擦 N → 写配置 → 校验返回值。
7. 全部成功才回最终 ACK，然后 `SysCtl_resetDevice()`。任一步失败走 `AppUpgradeAbort()`：擦配置、交还端口、功率保持关闭、不回成功。

## 6. 构建与内存（生产态 `PORT_BENCH_TEST=0U`）

55 个编译单元、0 错误、4 条 warning（全在参考 `XmodemCore.c`：`u32Sum`、`u16DataLength` 未引用，`:540/:549` 枚举混用）。

| 段 | 用 | 余 |
| --- | --- | --- |
| `FLASH_APP`（0x86002） | `0x83E5` | `0x11C19` |
| `RAMGS0`（0x1000 字） | 满 | **0**（`.bss` 从 RAMGS0 起连续分配，恰好填满） |
| `RAMGS1`（0xC00） | `0xACC` | `0x134` |
| `RAMGS2`（0x1000） | `0x840` | `0x7C0` |
| `RAMGS3_RUN`（0x1000） | `0x933` | `0x6CD` |
| `RAMD0`（0x800） | `0xC3` | `0x73D` |

`.upybuf` 0x840（`u16XmodemDataBuff[1024]` + `u16AppBuff[1024]` + 64 字余字节缓存 = 2112 字）；`.mbregs` 0x670；`.portbuf` 0x180。`RAMGS0` 已满属分配顺序问题，全 RAM 合计仍余 `0x134 + 0x7C0 + 0x6CD + 0x73D`；若后续再加静态变量，先放进 `.upybuf` 或 RAMGS3。

## 7. 两态构建台账

| 状态 | `.out` 大小 | sha256 | `.map` | `.text` | `FLASH_APP` 用/余 | `RAMGS0` 余 | `RAMGS1` 余 | `RAMGS2` 余 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 生产 `0U` | 1,025,952 B | `f242720c6bf52e21a060b9d2b3b4144aae620cd23ff1b69c40a7f8f652ce5cf5` | 128,139 B | `0x636B` | `0x8410` / `0x11BEE` | 0 | `0x134` | `0x7C0` |
| 测量 `1U` | 1,043,988 B | `85561760967db2c4d38a0f55bbe74f9781101f6c61626a8e9ccfec4c0f96acc5` | 133,096 B | `0x66EA` | `0x8791` / `0x1186D` | 0 | 1 | `0x7C0` |

生产态两次构建得到同一个 `.out` sha（可复现）；`.map` 内含时间戳，每次不同。与 r3 对比：`.out` 1,022,792 → 1,025,952 B（+3,160）、`.text` `0x625E` → `0x636B`（+269 字）、`FLASH_APP` `0x82D1` → `0x8410`（+319 字）、`RAMGS1` 余 `0xF6` → `0x134`；r4 → r5 只增加 588 B（`.text` +42 字），全部来自四处收尾。工作树已切回生产态。

## 8. 上板验证路径

按复核要求，先只验一条主路径：

1. **擦除成功**：握手后 `AppModbus` 回 8 字节、`'C'` 到来、备份区四个扇区擦除无错（`AppUpgradeAbortCount()` 保持 0）。
2. **收到首包**：主站发首包后板子回 ACK（不是 NAK/超时）。
3. **写入成功才 ACK**：正常包逐包 ACK；制造一次写失败（可临时把 `WriteBlock` 返回 -1）应看到不发 ACK、发两个 CAN、端口交还 Modbus、功率保持关闭。
4. **完整校验**：两次 EOT 后 `VerifyFireware()` 通过，备份区 CRC 与文件头一致。
5. **配置写入 + 复位**：配置扇区被写成 `u16FirmwareOver = 0xAAEE`，随后复位。

其余四条仍需确认：`CopyRamSections()` 是否把 `.flashfunc` 搬到 `0xF000`；F021 在 200 MHz 下擦写与 `Fapi_doVerify` 是否通过；`RAMGS0` 已满（不要再加静态变量）；取消（CAN）与重试超限后端口能否回到 Modbus。

## 9. 复核要点

1. **是否忠实移植**：§3 对照表逐文件；除表中列出的改动，参考代码未做其他修改。
2. **必要适配是否正确**：§4.1~§4.4 逐类说明，§4.6 逐条对应复核意见；新增逻辑只有端口适配的整包累计（约 165 行）与 APP 侧接线（193 行）。
3. **升级能否完成**：按 §8 五步上板跑通即视为本步完成；取消与重传属于第二步。

## 10. 本轮（r5）上板前收尾

复核（m02539）确认 r4 已修上轮主要问题、参考框架可以保留，另指出四处小范围收尾。本轮只改这四处：

| # | 问题（复核位置） | 改法 |
| --- | --- | --- |
| 1 | `FlashSaveInt()` 只存 `IER`，`FlashRestoreInt()` 无条件 `EINT`，进入前若已关中断会被擅自打开 | 复用 05B 验证过的整字 ST1 方式：`FlashSaveInt()` 返回 `__disable_interrupts()`（PUSH ST1 / SETC INTM,DBGM），`FlashRestoreInt()` 走 `__restore_interrupts()`（POP ST1），INTM 与 DBGM 一起回到原值 |
| 2 | CRC 失败后 `s_u16UpgradeDone` 仍为 1，下一会话跳过 `XmodemReceive()`（探针：第二次会话接收调用次数仍为 1） | `UpgradeUser.c` 新增 `UpgradeUserSessionReset()` 清该标志，`AppUpgradeStart()` 与 `AppUpgradeAbort()` 都调用；`UpgradeUser.h` 加声明 |
| 3 | `AppUpgradeStart()` 失败时 `AppModbus` 仍先置 `u16Armed = 1` 并忽略返回值，端口停在无人接收状态 | 先调 `AppUpgradeStart()`，**返回 1 才置 `u16Armed`**；返回 0 则清握手尾部长度、计入丢弃、`SciPortResync()` + 刷新故障代次与时间戳，端口继续按 Modbus 收帧。另把 `Fapi_initializeAPI()` 的返回值纳入 `Dsp2837xSFlashInit()`（失败返回 -3），`s_u16FlashReady` 随之更严格 |
| 4 | 接收端只认 ChipType `0x37`，而打包命令 `CalcUpgradeBinCrc.exe "1" "1" ...` 生成的是 1，探针确认会被拒（`XMODEM_RETRY_ERROR`） | 保持原打包约定，`XMODEM_CHIP_TYPE` 改为 `1U`，并在宏旁注明与打包命令首参数一致 |

对上轮判断的确认：Bank0 单一选择、空片检查按 32 位字、RAM 内直接喂狗、备份区范围检查、擦写失败上抛，r4 已落地；真实 F021 时序仍需上板确认，主机探针不能替代。

### 10.1 需要特别知道的边界

1. **ST1 恢复的语义**：`__restore_interrupts()` 是整字弹回，因此在"进入时中断本来就关"的路径（例如从 ISR 里调用擦写）退出后不会开中断——这是刻意的，和 05B 临界区一致。
2. **`s_u16UpgradeDone` 的生命周期**：只在一次会话内有意义；`AppUpgradeStart()` 开头与 `AppUpgradeAbort()` 都会清。
3. **交接失败不回退握手应答**：8 字节应答已经发出去了，这里只把端口还给 Modbus 并重新同步，主机需要重新握手（与"中止后交还 Modbus"的裁定一致）。
4. **ChipType 与打包工具**：若以后改用 `0x37`，打包命令首参数要写成十进制 `55`，并同步 APP/BOOT 两端与本文档。

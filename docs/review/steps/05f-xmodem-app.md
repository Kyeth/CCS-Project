# 05F 申请文档：XMODEM 接收接入 APP

- 基线：`a7d1669`（05E-r3，Modbus 从机框架已登记）
- 日期：2026-11-10
- 本步只做 APP 侧：接收升级 BIN → 写备份区 → 核对长度与 CRC → 提交升级配置 → 复位。
  BOOT 工程（跳转、搬运、自升级）另立项目空间，不在本步。

## 0. 结论先行

1. 参考仓库 `68701bd` 的 Xmodem 层是**未编译的骨架**：`Platform/Xmodem/Src/XmodemCore.c` 531 行全部被
   `//` 注释（活代码 0 行），`XmodemDataHandle.c` 同样；`User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c`
   里 `stDataHandler` 声明后没赋值就传给 `XmodemInit`；`User/Upgrade/Src/UpgradeUser.c` 的
   `XmodemReceive(&ObjXmodemCtrl)` 也被注释。所以"复用框架"只能复用**接口与状态机结构**，代码要补齐，
   并修三处缺陷（§2）。
2. 05E-r3 现在一次回 **9 字节**（回显 6 + CRC 2 + `'C'`）。05F 改成回 **8 字节**，`'C'` 由升级层在
   **备份区擦除完成、包缓冲就绪之后**单独发（§3）。这是 05F 对 05E 唯一的行为改动，
   `modbus_slave_test.c` 的握手用例要同步改。
3. 升级期该端口由升级层独占，`AppModbus` 不再读它；交接时把候选缓冲里超出手握手长度的字节一并交出（§3.3）。
4. 写备份按 F021 单次上限分块，每写一块就返回调度器，让端口任务继续把字节收进环（§7.3）；
   擦除放在发 `'C'` 之前，避免长占用把首包吃掉。

## 1. 范围与不做

做：

- Xmodem 接收状态机（SOH 128 / STX 1024、双包序号去重、CRC16、重试、EOT/CAN）；
- SciPort 适配层（读批、写 ACK，全部经 `SciPort`）；
- 超时适配（`Hw_TimeBase`，毫秒）；
- 独立包缓冲、32 字节文件头解析；
- 备份区擦除与分块编程（F021，F2837xS）；
- 回读校验（长度 + CRC32）；
- 升级配置写入与复位；
- APP 侧任务与从 Modbus 的端口交接。

不做：

- BOOT 工程的跳转、搬运、APP 有效性判断；
- 双 BOOT；
- Xmodem 发送端（`XmodemTransmit`）；
- 升级时切换波特率（05E 已删掉 XMODEM 切速，本步保持不切）。

## 2. 参考工程 Xmodem 的真实状态

| 文件 | 总行 | 活代码 | 结论 |
|---|---:|---:|---|
| `Platform/Xmodem/Src/XmodemCore.c` | 532 | 0 | 整份注释，未参与编译 |
| `Platform/Xmodem/Src/XmodemDataHandle.c` | 88 | 0 | 整份注释（`Flash280039C.h` 时代） |
| `User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c` | 39 | 30 | 只调 `XmodemInit`；`stDataHandler` 未赋值 |
| `User/Upgrade/Src/UpgradeUser.c` | 172 | 140 | 关键调用 `XmodemReceive` 被注释 |
| `Platform/Upgrade/Src/UpgradeCore.c` | 169 | 129 | 可用（配置读写、搬运、校验） |

三处缺陷（必须修，不是口味问题）：

1. `ST_DATA_HANDLER_INTF`（`XmodemCore.h`）只声明 `ReadBlock` / `WriteBlock`，而注释掉的 `XmodemInit`
   引用了 `Trig` / `Reset` / `TrigWriteCfg` / `ResetWriteCfg` / `SetTotal` —— 接口自相矛盾，说明它从未编译过。
2. `u16RecvLength` 是 `Uint16`，却与 `Uint32` 的 `u32FileSize` 比较：超过 64 KB 必然回绕
   （APP 上限 `0x34000` = 212,992 字节）。
3. `XmodemTransmit()` 是空函数、没有返回值；本轮不实现发送，声明一并删掉。

保留的正确设计：

- 包缓冲 `u16xBuff[1030]`（SOH 包 `3+128+2=133`，STX 包 `3+1024+2=1029`），每字节占一个 `Uint16`；
- 双包序号：`buff[1] == bCurrentPackNo` 才写，`== bLastPackNo` 视为重复包，只回 ACK；
- CRC16-CCITT：查表、poly `0x1021`、初值 0、**高字节在前**；
- 重试上限 10；EOT 需连收两次、CAN 需连收两次；超时 500 ms；
- 首包正文从 `u16xBuff[35]` 起（3 + 32 字节头），后续包正文从 `u16xBuff[3]` 起。

## 3. 交接：`'C'` 由谁发、何时准备好接收

### 3.1 现状（05E-r3）

`AppMbDispatchFrame()` 命中六字节握手 `01 10 04 38 00 02` 后，一次回 9 字节
（回显 6 + CRC 2 + `0x43`），置 `bArmPending`；回复全部入环后 `u16Armed = 1`，此后该端口不再收帧。

### 3.2 为什么必须改

`'C'` 的含义是"我已经准备好接收"。05F 要在发 `'C'` 之前擦备份区（四个扇区，百毫秒量级）。
若 `'C'` 还跟着 ACK 一起发出去，主机会在擦除期间就开始传，前若干字节必然丢。

### 3.3 新规则

- 握手应答只回 **8 字节**：`[从机地址, 0x10, 0x04, 0x38, 0x00, 0x02, CRC低, CRC高]`。
- 升级层接管后：**擦除备份区 → 准备包缓冲 → 发单字节 `'C'`** → 成为该端口所有者。
- 所有权转移之后，`AppModbus` 不再对该端口调用 `SciPortPopBatch` / `SciPortWriteTx`；升级层
  **不得**调用 `SciPortInit`、不得清 RX 环、不得 `SciPortResync`、不得重置端口任务。
- 握手判定改为**前缀匹配**：候选缓冲前 6 字节命中即算握手；第 7 字节起的内容（例如紧跟着的 `SOH`）
  连同端口一起交给升级层，作为包缓冲的起始内容，**不得丢弃**。

### 3.4 责任表（单一责任点）

| 时刻 | 端口所有者 | 谁发 `'C'` | 谁读 `SciPort` |
|---|---|---|---|
| 收到握手帧之前 | Modbus（`AppModbus`） | — | `AppModbus` |
| 回 8 字节 ACK 后、擦除与就绪期间 | 升级层（已接管，尚未发 `'C'`） | 升级层（擦除 + 就绪后） | 升级层（此时主机还不会发数据） |
| `'C'` 发出之后 | 升级层独占 | 不再发 | 升级层 |
| 升级结束或中止 | 交还 Modbus（`u16Armed = 0`） | — | `AppModbus` |

## 4. 收包：状态机与包缓冲

| 状态 | 进入条件 | 动作 |
|---|---|---|
| `STATE_IDLE` | 升级层接管 | 复位包序号（`1` / `1`）、清长度累计 → `WAIT_FOR_START` |
| `STATE_WAIT_FOR_START` | — | 等首包；超时 500 ms，重试到 10 次 → CAN×3 → `ERROR` |
| `STATE_RECEIVE_DATA` | 已收过包 | 继续累计下一包 |
| `STATE_PROCESS_PACKET` | 累计够一整包 | 解析、写备份、回 ACK/NAK |
| `STATE_COMPLETE` | EOT×2 或 CAN×2 | 结束，交还端口 |
| `STATE_ERROR` | 重试超限 | CAN×3，交还端口，留 APP |

单包判定：

- `buff[0] == SOH` → 数据 128 字节；`buff[0] == STX` → 1024 字节；整包 `3 + N + 2`。
- 序号校验与 CRC 校验全部通过、且写备份成功，才回 `ACK`；
- CRC 错、序号错、非法包头 → `NAK`，不写、不推进序号；
- 重复包（序号等于上一包）→ 只回 `ACK`，不重复写。

## 5. 读与发全部经 SciPort

- `XmodemPortRead(port, dst, max)`：循环 `SciPortPopBatch(port, ...)` 把字节累进包缓冲，每字节落一个
  `Uint16`；返回本次取到的字节数。**不假设一次能拿到整包**——`SciPort` 一段最多 8 字节。
- `XmodemPortWrite(port, buf, len)`：`SciPortWriteTx`；ACK/NAK/CAN 都是单字节，直接入环。
- 整包判定由升级层按"首字节 + 包长"累计，不依赖 3.5 字符间隔（升级期没有 Modbus 帧边界概念）。
- 超时：`Hw_TimeBase` 实现 `TimeStart` / `IsTimeOut` / `TimeStop`，单位毫秒，保留参考的
  `ST_TIMEOUT_MANAGER` 接口形状。

## 6. 写备份区：字节长度与 Flash 字地址

| 概念 | 单位 | 出处 |
|---|---|---|
| 文件头 `u32FileSize` | 8 位字节 | 上位机工具写入 |
| Xmodem 包数据 | 8 位字节 | 每字节占一个 `Uint16` |
| F021 编程地址 | 16 位字地址 | F2837xS |
| F021 编程长度 | 字数 | 单次 ≤ 8 字 |

换算：

- 字地址 `= BACKUP_ADDR + (字节偏移 >> 1)`；
- 字数 `= (字节数 + 1) >> 1`；
- 奇数字节时末字节高位补 `0xFF`，CRC 只算真实正文。

32 字节文件头**不写**备份区：首包正文从 `u16xBuff[35]` 起，共 `128 - 32 = 96` 字节。

容量：APP 正文 ≤ `0x34000` 字节（212,992）；备份区 `0x1A000` 字 = 212,992 字节，正好装下；
超出直接判失败，不截断。

## 7. 擦除时机与 RAM 运行

### 7.1 擦除时机

在发 `'C'` 之前（§3），按 A～N 实际扇区起点逐个擦除，**不能固定 `0x1000` 步长**（用户裁定）。

### 7.2 擦写代码必须在 RAM

`CMD/2837xS_FLASH_APP_CLA_lnk.cmd` 已经把 `.TI.ramfunc` 配成 `LOAD = FLASH_APP, RUN = RAMD0`，
但**当前启动代码里没有任何地方搬运它**（`RamfuncsLoadStart` 只出现在链接脚本里）。本步必须在初始化里补：

```c
memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
```

否则一旦有函数落进 `.TI.ramfunc`，就会跳到未初始化的 RAMD0。

### 7.3 编程分块与调度

每轮只发一条 F021 编程命令（≤ 8 字），然后返回调度器，让端口任务继续把后续字节收进环。

依据：8 字编程是微秒级；而一包 1,024 字节在 115200 下要 89 ms。把整包一次写完会让 DINT 窗口
把下一包的开头丢掉；分块后每次占用的时间远小于一包的到达间隔。

### 7.4 依赖

- 头文件：`D:/ti/C2000Ware_5_00_00_00/libraries/flash_api/f2837xs/include/F021.h`
  （含 `F021_F2837xS_C28x.h`、`Constants/`、`Types.h`）；
- 库：`.../f2837xs/lib/F021_API_F2837xS_FPU32_EABI.lib`（本工程 `--abi=eabi --float_support=fpu32`）；
- 还需要一份 `source/Fapi_UserDefinedFunctions.c`（提供 `Fapi_UserDefinedFunctions` / `Flash_Wait` 等回调）；
- 注意：本机 C2000Ware 5.04 只带 `f28003x`，F2837xS 的那份在 `D:/ti/C2000Ware_5_00_00_00`。

### 7.5 调用序列

`Fapi_initializeAPI(F021_CPU0_BASE_ADDRESS, 200)` → `Fapi_setActiveFlashBank(Fapi_FlashBank0)` →
`Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector, addr)` → 轮询 `Fapi_checkFsmForReady()` →
`Fapi_issueProgrammingCommand(addr, buf, words, NULL, 0, Fapi_AutoEccGeneration)` → `Fapi_getFsmStatus()`。

### 7.6 RAM 预算

包缓冲 1,030 字放新段 `.upybuf` → `RAMGS2`（4,096 字，当前完全空闲）；`.TI.ramfunc` 落 `RAMD0`
（2,048 字），F021 + 本步擦写函数要放得下，构建后按 map 核对。

## 8. 文件头解析、EOT 核对与配置提交

文件头 32 字节（按字节显式解析，**不** `memcpy` 成结构体）：

| 偏移 | 内容 |
|---|---|
| 0 | ChipType |
| 1 | FirmwareType：1 = APP，2 = BOOT |
| 2–5 | 正文长度，小端，单位 8 位字节 |
| 6–9 | 正文 CRC32，小端 |
| 10–31 | 保留 |

EOT 之后依次判定：

1. 实际写入字节数 == 头里的 `u32FileSize`；
2. 备份区 CRC32（只算正文）== 头里的 `u32FileCrc`；
3. `bFirmwareType` 是 1 或 2，`u32FileSize` 不超过本区容量；
4. 全部通过 → 写升级配置（`u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0xAAEE`、`u32FileSize`、
   `u32FileCrc`、`u32UpgradeCfgSum`）→ `SysCtl_resetDevice()`；
5. 任一不过 → 不写配置、回 CAN、端口交还 Modbus、留在 APP。

CRC32 参数：初值 `0xFFFFFFFF`、poly `0x04C11DB7`、不反射、无最终异或；`123456789` → `0x0376E6E7`。

配置结构沿用参考 `ST_UPGRADE_CFG` 字段，落 `FLASH_CFG_RSVD`（`0x0BE000`）。

## 9. 文件清单与构建

新增（草案）：

| 文件 | 内容 |
|---|---|
| `Platform/Xmodem/Inc/XmodemCore.h`、`Src/XmodemCore.c` | 接收状态机，F2837xS 设备头 |
| `Platform/Xmodem/Inc/XmodemPort.h`、`Src/XmodemPort.c` | SciPort 适配 + `Hw_TimeBase` 超时 |
| `Platform/Upgrade/Inc/UpgradeCfg.h`、`Src/UpgradeCfg.c` | 分区地址、配置结构、CRC32、配置读写 |
| `Platform/Upgrade/Inc/UpgradeFlash.h`、`Src/UpgradeFlash.c` | F021 包装、扇区擦除、分块编程 |
| `Platform/Upgrade/Inc/UpgradeBackup.h`、`Src/UpgradeBackup.c` | 备份区写入、回读 CRC 校验 |
| `UserApp/Inc/AppUpgrade.h`、`Src/AppUpgrade.c` | 交接、任务、状态上报 |

修改：

- `UserApp/Src/AppModbus.c`：8 字节 ACK、握手前缀匹配、交接残留字节；
- `main.c`：ramfunc 搬运、`AppUpgradeInit()`、注册升级任务；
- `CMD/2837xS_FLASH_APP_CLA_lnk.cmd`：新增 `.upybuf` 段；分区容量注释按 16 位字改正
  （现在把 208 KiB 写成 104KB）；
- `tools/build_minimal.py`：新增源文件、F021 库与 include 路径。

APP 与 BOOT 共用：`Platform/Xmodem/` 与 `Platform/Upgrade/` 全部；BOOT 只补自己的 `main`、跳转与搬运。

## 10. 主机测试计划

新增 `Platform/Upgrade/Test/xmodem_test.c`，用替身传输层（内存包队列）与替身 Flash（RAM 数组）驱动，
覆盖：

1. 单包 SOH 128 字节 → ACK、正文落备份区；
2. 多包 STX 1,024 字节 → 拼接与序号正确；
3. 重复包 → 只 ACK、不重复写；
4. CRC 错 → NAK、不写、不推进序号；
5. 序号跳变 → NAK；
6. 首包 32 字节头解析（类型 / 长度 / CRC）；
7. 末包不足一整包 → 只写实际字节、不越界；
8. EOT×2 → 结束；长度或 CRC 不符 → 不提交配置；
9. 连续 CAN×2 → 中止、端口交还；
10. 超时重试到上限 → CAN×3 且状态机回到 IDLE；
11. 字节↔字换算边界：奇数字节、末字节补 `0xFF`；
12. 交接：握手残留的 `SOH` 进入包缓冲，不丢失。

期望输出：`xmodem_test: N 项检查，0 项失败`（N 在实现后确定）。

## 11. 待裁定

1. `'C'` 改为由升级层在擦除完成后单独发（05E 的 9 字节应答变 8 字节）——确认可接受；
   `modbus_slave_test.c` 的握手用例随之改为"8 字节 ACK + 单独一字节 `'C'`"。
2. 升级中止后端口交还 Modbus（`u16Armed = 0`），主机需重新握手；不做自动重试。
3. 备份区是否整片擦除（`0x1A000` 字、四个扇区）还是只擦"将要用到的长度"——建议整片擦。
4. 升级期间其它任务（ADC 采样、CLA、功率输出）是否暂停——建议暂停，只留端口任务与升级任务。

## 12. 验收要点

1. 主机发 6 字节握手 → 收 8 字节 ACK → 收单字节 `'C'`，且 `'C'` 在擦除完成之后。
2. 传一个真实 APP BIN → 备份区与 BIN 正文逐字节一致（抽检头 / 中 / 尾各 16 字节）。
3. 复位后读回配置区：类型、长度、CRC 正确，`UpgradeFlag` / `FirmwareOver` 均为 `0xAAEE`。
4. 故意改一个字节 → CRC 不符 → 不写配置、回 CAN。
5. 收包不丢字节：121 字节与 1,024 字节包各测一次。

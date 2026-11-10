# 05F 申请文档（修订版）：XMODEM 接收接入 APP

| 项目 | 内容 |
| --- | --- |
| 版本 | r2（2026-11-10）——替代首版 `41c1827`，按复核裁定修订 |
| 状态 | 已裁定，按本文档直接实施，不再单独报批 |
| 基准 | `test/board-bringup-20261031`，05E-r3 `41c1827` |
| 移植依据 | 参考工程 git 提交 `68701bd` 的源文件（来源差异见 §2.2） |

首版对参考工程的判断有误（误判为"未编译的骨架"），§2 已按复核结论更正，并记录了本机副本与 `68701bd` 的来源差异。§8 的 RAM 擦写顺序、§8.5 的分块理由也已按裁定改写。

## 0. 结论先行

1. **沿用参考工程的接口与状态机**：`XmodemInit()` / `XmodemReceive()` / `XmodemSinglePacket()` 的形状、包解析规则、ACK/NAK/CAN 与 EOT 两次确认的语义都保留；只替换与 `SciPort` 不兼容的三处（传输层读写、超时来源、写块去向）。
2. **`'C'` 从 05E 的 9 字节应答里拆出来**：握手应答改为 8 字节，`'C'` 由升级层在**备份区擦除完成、包缓冲就绪**之后单独发。
3. **收包、校验、写备份区、写配置、复位全部在升级层**；Modbus 在交出端口后不再读也不再写该端口。
4. **写 Flash 用 F021**：8 字对齐块、`Fapi_AutoEccGeneration`、末块补齐、同一 64 位编程单元不重复写；擦写函数及其调用的 API、常量都安排在 RAM 执行，**发命令后在同一 RAM 函数内等到 FSM 完成才返回调度器**。
5. 升级期间暂停功率输出、控制 ISR、ADC 与 CLA 触发；保留时基、端口服务、看门狗。中止后不自动恢复功率输出。

## 1. 范围与不做

**做**：`Platform/Protocol/Xmodem/`（移植参考层）、`Platform/Upgrade/`（扇区表、擦写、配置区）、`UserApp/{Inc,Src}/AppUpgrade.*`（桥接与任务）、`AppModbus.c` 的应答与交接改动、`main.c` 的任务注册与 RAM 函数搬运、`CMD` 新增段、`build_minimal.py` 源列表与 F021 库、主机测试与替身。

**不做**：BOOT 工程本体；APP 区回拷与跳转（属 BOOT）；分区表调整；Modbus 功能变更（除握手应答长度与交接）。

### 1.1 升级期间的任务与中断（裁定 4）

| 类别 | 处置 |
| --- | --- |
| 功率输出（PWM 发波） | 停止，并在升级全程保持停止 |
| 控制 ISR | 关闭（不再进入控制中断） |
| ADC / CLA 触发 | 停止触发，CLA 任务不再启动 |
| 时基 `Hw_TimeBase` | **保留**（超时、节拍都靠它） |
| 端口服务 | **保留**，由升级层自己收字节 |
| 看门狗 | **保留**，擦写等待循环里每轮喂狗 |
| 中止后 | 交还端口，停在安全停机状态；**不自动重新开启功率输出**，等上层显式使能 |

## 2. 参考工程的实际实现（更正首版判断）

### 2.1 参考实现是有效的

按复核结论，`68701bd` 工作区干净，实际内容是：

| 首版说法 | `68701bd` 的实际内容 |
| --- | --- |
| `XmodemCore.c` 全被注释 | 存在有效的 `XmodemInit()`、`XmodemReceive()` |
| `stDataHandler` 未赋值 | `XmodemSlaveProcess.c:28~29` 已绑定两个回调 |
| 接收长度是 `Uint16` | `XmodemCore.h:120` 是 `Uint32 u32RecvLength` |
| `XmodemReceive()` 调用被注释 | `UpgradeUser.c:155` 存在有效调用 |
| Init 绑了未声明的五个回调 | 当前 Init 只绑 `ReadBlock` / `WriteBlock` |
| `XmodemTransmit()` 空函数 | **这一条成立** |

结论：**不能据此认定参考代码"从未编译过"**。以 git 提交中的源文件为移植依据，沿用其接口与状态机，只修与 `SciPort` 不兼容的部分。

### 2.2 本机副本与 `68701bd` 的来源差异（实施记录）

本机 12 个 git 仓库逐一 `git cat-file -e 68701bd` 全部失败，参考仓库远端 `//DANGZHEN/GitRemote/IB800U.git` 当前不可达。本机可用的最近副本是 `/d/GitCode/IB800U/IB800U/IB800U/Platform/Xmodem/{Inc,Src}`（2025-07-03，`XmodemCore.c` 729 行含活代码），但与 `68701bd` 存在差异：

1. `XmodemCore.h:118` 是 `Uint16 u16RecvLength`（`68701bd` 已改为 `Uint32 u32RecvLength`）；
2. `User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c` 把 `ReadBlock` / `WriteBlock` 赋成 `NULL`（`68701bd` 已绑定）；
3. 该副本没有 `User/Upgrade/Src/UpgradeUser.c`；
4. 活代码里 `XmodemSinglePacket()` 的 `WriteBlock(...)` 调用被注释，只推进序号。

处理：**接口与状态机以参考为准，缺陷按 `68701bd` 的结论修**——长度用 `Uint32`、回调由本工程绑定、写块恢复为真实写 Flash。以上差异写进提交信息；若拿到 `68701bd` 的完整副本，再按其源文件复核一遍函数体。

### 2.3 参考实现里仍然成立、本工程要修的问题

- `XmodemTransmit()` 是空函数且无返回值：本工程不使用，原样保留、不调用。
- 序号回绕 `if ((bCurrentPackNo == 0) || (bCurrentPackNo > 255)) bCurrentPackNo = 1;` **跳过了 0**，而 XMODEM 序号是 1..255、0、1…。SOH 包 128 字节 × 256 = 32 KiB、STX 包 1024 字节 × 256 = 256 KiB 就会错位。本工程按 8 位自然回绕（255→0）。
- 参考的 `stTransport.Read(pu16Buff, u16Size)` 没有超时参数、由状态机用 `ST_TIMEOUT_MANAGER` 自己计时；本工程改为从 `SciPort` 批次取字节，超时用 `Hw_TimeBase` 判。

## 3. 接口对应（本工程绑定）

| 参考接口 | 本工程实现 | 说明 |
| --- | --- | --- |
| `ST_TRANSPORT_INTF.Read` | `AppUpgradeRead` | 先取升级层候选缓冲，空了再从 `SciPortPopBatch` 补 |
| `ST_TRANSPORT_INTF.Write` | `AppUpgradeWrite` | `SciPortWriteTx` 整批写；装不下返回 0，由调用方重试 |
| `ST_DATA_HANDLER_INTF.ReadBlock` | 不实现 | 升级只写不读，传 `NULL` |
| `ST_DATA_HANDLER_INTF.WriteBlock` | `AppUpgradeWriteBlock(packNo, buff, len)` | 写备份区，见 §8 |
| `ST_TIMEOUT_MANAGER` | `TimeStart` / `IsTimeOut` / `TimeStop` 三个包装 | 内部用 `Hw_TimeBaseElapsed`，超时值 500 ms，首字符等待 2000 ms |

## 4. `'C'` 的交接（裁定 1）

### 4.1 现在的行为（05E-r3）

握手命中后一次性回 9 字节：回显 6 字节 + CRC 2 字节 + `'C'`（`APP_MB_HANDSHAKE_REPLY (9U)`）。主机收到 `'C'` 就开始发首包——而此时备份区还没擦。

### 4.2 改成什么

| 时刻 | 端口所有者 | 谁发 `'C'` | 谁读 `SciPort` |
| --- | --- | --- | --- |
| 主机发 6 字节握手 | Modbus | — | Modbus |
| 判定握手命中、组好 8 字节应答 | **交接开始**：所有权已归升级层 | — | 升级层（保存尾部） |
| 8 字节应答发送中 | 升级层 | — | 升级层 |
| 擦 H/I/J/K | 升级层 | — | 升级层 |
| 擦完、包缓冲就绪 | 升级层 | **升级层** | 升级层 |
| 主机开始发首包 | 升级层 | — | 升级层 |

Modbus 侧改动：`APP_MB_HANDSHAKE_REPLY` 由 9 改 8，`pu16Data[8] = 0x43` 那行删除，`bArmPending` 置位后不再发 `'C'`。

### 4.3 尾部字节与"主机仍必须等 'C'"

握手缓冲里超出手握手长度的字节（例如紧跟的 `SOH`）**先保存到升级层的候选缓冲，再组 8 字节回复**；不清环、不 `SciPortInit`、不 `SciPortResync`。保留已收到的尾部是正确的，但**主机仍必须等 `'C'`**——擦除期间主机不会收到任何应答，本层不承诺"擦除期间持续发送也不丢"。

### 4.4 责任表

`'C'` 只能由升级层发，Modbus 不再发；交接后 Modbus 既不读也不写该端口。这点是单一责任点，不允许两边都发。

## 5. 收包状态机（本工程版）

| 状态 | 行为 |
| --- | --- |
| `IDLE` | 未开始；等握手 |
| `WAIT_START` | 已发 `'C'`，等首包；超时 2000 ms 重发 `'C'`（受重试上限约束） |
| `RECEIVE` | 逐包解析、写备份区、回 ACK/NAK |
| `FINISH` | 收到第一次 EOT，回 NAK 等第二次 |
| `DONE` | 长度与 CRC 校验通过、配置已提交、最终 ACK 已发完 |
| `ERROR` | 重试超限或收到两次 CAN |

每轮 `AppUpgradeService()`：①从 `SciPort` 取字节进候选缓冲；②调 `XmodemReceive()` 推进一步；③按返回的字符发 ACK/NAK/CAN；④处理完成/失败。

包解析规则沿用参考：`SOH` 128 字节、`STX` 1024 字节；`buff[1]` 序号、`buff[2]` 反码；CRC16-CCITT（poly `0x1021`、初值 0、**高字节在前**）；重复包（序号等于上一包）只回 ACK 不重写；EOT 需两次确认、CAN 需两次；重试上限 10；单包超时 500 ms。

### 5.1 首包可能是 SOH 也可能是 STX（边界 2）

首包正文前有 32 字节文件头：SOH 首包正文最多 **96** 字节（128−32），STX 首包最多 **992** 字节（1024−32）；后续包正文 128 / 1024 字节。**最后一包只写剩余真实长度**（`u32RecvLength < u32FileSize` 时取差值），不把补齐的 `0xFFFF` 计入文件长度。

### 5.2 包缓冲

`u16xBuff[XMODEM_BUFF_SIZE = 1030]`（每字节占一个 `Uint16`，SOH 包 133、STX 包 1029）放新段 `.upybuf` → `RAMGS2`（`origin = 0x00E000`，4096 字，当前 map 占用 0）。

## 6. 读与发全部经 SciPort

- **读**：`SciPortPopBatch()` 每段 ≤8 字节；一轮最多 8 条；剩余空间不足 8 字节时**不取**（避免把装不下的半段当成"端口已空"）。
- **发**：`'C'`、ACK、NAK、CAN 都是 1 字节。`SciPortWriteTx()` 返回 0 表示环里装不下，**必须留待下一轮重试，不能当成已发送**（边界 4）；发送遇忙时不能推进状态机。
- **超时**：`Hw_TimeBaseElapsed()`；时间源不可用时不确认任何帧。

## 7. 文件头、长度与容量（边界 1）

首包收到 32 字节头后**立即校验，全部通过才允许写正文**：

| 检查项 | 条件 |
| --- | --- |
| `u16ChipType` | 等于本机型编码 |
| `u16FirmwareType` | 在允许集合内（APP / BOOT） |
| `u32FileSize` | 非零，且 ≤ 上限 |
| APP 上限 | `0x34000` 字节（212,992 = 备份区 `0x1A000` 字，正好装满） |
| BOOT 上限 | `0xC000` 字节（49,152 = A+B+C `0x6000` 字） |
| `u32FileCrc` | 非零 |

任一项失败：回 CAN、丢弃、交还端口。EOT 收尾时再核对 `u32RecvLength == u32FileSize`，并重算 CRC32 与头一致。

## 8. 写备份区

### 8.1 地址

C28x 一字一地址：第 n 个字写 `0x0A0000 + n`。

### 8.2 扇区表（按实际扇区起点，长度不等）

| 扇区 | 起始 | 长度 | 说明 |
| --- | --- | --- | --- |
| H | `0x0A0000` | `0x8000` 字（32K 字 = 64 KiB） | E~J 是 32K 字扇区 |
| I | `0x0A8000` | `0x8000` 字 | |
| J | `0x0B0000` | `0x8000` 字 | |
| K | `0x0B8000` | `0x2000` 字（8K 字 = 16 KiB） | K~N 是 8K 字扇区 |

四段合计 `0x1A000` 字。擦除按这四段的**各自起点**逐段下发，不能假设等长（APP 区是 D+E+F+G，与备份区不是一一对应）。整区擦除。

### 8.3 擦除时机

进入升级后：回完 8 字节应答 → 擦 H/I/J/K → 包缓冲就绪 → 发 `'C'`。擦除在 `'C'` **之前**完成。

### 8.4 发命令与等待的严格顺序（本次修正的重点）

```
进入 RAM 擦写函数（函数本身已在 RAM 执行；DINT 已置位，原 ST1 已保存）
  → 发命令（Fapi_issueAsyncCommandWithAddress / Fapi_issueProgrammingCommand）
  → 在 RAM 中等待 FSM 完成（轮询 Fapi_checkFsmForReady，循环里喂狗）
  → 检查 Fapi_getFsmStatus（必要时回读校验）
  → 恢复原中断状态（还原 DINT）
  → 返回调度器
```

**F021 发命令后不会等待完成。** FSM 忙时不能返回一个仍在同一 Flash Bank 上执行的调度器，也不能进入位于 Flash 的 ISR。因此不只是补 `.TI.ramfunc` 的 `memcpy`：F021 的 API 本体、`Fapi_UserDefinedFunctions.c`、等待循环里用到的一切常量与函数，都必须安排在 RAM 执行。

RAM 函数所在的段：新增 `.flashfunc`（`LOAD = FLASH_APP`、`RUN = RAMGS3`，`0x00F000`，4096 字、当前占用 0），启动时一次 `memcpy` 到 RAM；同时把现有 `.TI.ramfunc` 缺失的搬运一并补上（`RamfuncsLoadStart` / `LoadSize` / `RunStart` 已在链接脚本里声明，但 `main.c` 没有这一句，谁往该段放函数都会跳到未初始化的 RAMD0）。

### 8.5 分块（理由更正）

正常 XMODEM 发送端**等到 ACK 才发下一包**，所以"接收完整包 → 写完 → 再 ACK"本身就给了接收端处理窗口；不应写成"下一包必然会在整包编程期间到达"。分块的价值在于把单次 DINT 窗口压到毫秒级，让看门狗与超时计时不被长时间遮蔽：

- 每次编程一个 **8 字（128 位）对齐块**，`Fapi_AutoEccGeneration`；
- **末块补齐到 8 字**（补 `0xFFFF`，即擦除态），补齐后仍落在备份区内；
- **同一个 64 位（4 字）编程单元不重复写**：一次命令至少覆盖一个完整的 64 位单元，绝不把同一单元分两次写到；
- 每块写完检查 FSM 状态，失败则不回 ACK、进 ERROR（见 §9）。

### 8.6 依赖

`F021_API_F2837xS_FPU32_EABI.lib` 与 `include/F021.h`，位于 `D:/ti/C2000Ware_5_00_00_00/libraries/flash_api/f2837xs/`（本机 C2000Ware 5.04 只带 `f28003x`）。需自带一份 `Fapi_UserDefinedFunctions.c`（提供 `Fapi_UserDefinedFunctions` 与 `Flash_Erase`/`Flash_Program` 的空壳钩子）。

### 8.7 RAM 预算

`.upybuf` 1030 字（RAMGS2 余 4096）＋ `.flashfunc` 约 1.1K~1.4K 字（RAMGS3 4096）。实际占用以链接后的 map 为准，构建后检查 `.flashfunc` 的 RUN 地址与长度是否超出 RAMGS3。

## 9. 成功与取消（边界 5）

**成功**：收两次 EOT → 核对 `u32RecvLength == u32FileSize`、重算 CRC32 → 写配置区（`u16UpgradeFlag = 0xAAEE`、`u32FileSize`、`u32FileCrc`、`u16FirmwareType`）→ 回读确认 → **最终 ACK 真正发完**（`SciPortIsTxIdle()` 为真）→ 复位。

**取消**（两次 CAN、重试超限、校验失败、Flash 失败）：**不提交配置**。回 CAN，清本次残留（候选缓冲、包计数、重试计数），`SciPortResync()` 做必要重同步，交还 Modbus（`u16Armed = 0`），主机需要重新握手；不自动重试。

## 10. 文件清单与构建

| 文件 | 内容 |
| --- | --- |
| `Platform/Protocol/Xmodem/Inc/XmodemCore.h` | 参考头文件，长度字段改 `Uint32` |
| `Platform/Protocol/Xmodem/Src/XmodemCore.c` | 移植：状态机、包解析、CRC16-CCITT |
| `Platform/Upgrade/Inc/UpgradeCore.h` | 扇区表、接口声明 |
| `Platform/Upgrade/Src/UpgradeCore.c` | F021 擦除/编程、配置区读写、CRC32 |
| `UserApp/Inc/AppUpgrade.h` | 升级任务接口 |
| `UserApp/Src/AppUpgrade.c` | SciPort 桥接、状态推进、`AppUpgradeTask()` |
| `UserApp/Src/AppModbus.c` | 应答改 8 字节、交接后让出端口 |
| `main.c` | 注册升级任务、`.flashfunc`/`.TI.ramfunc` 搬运 |
| `CMD/2837xS_FLASH_APP_CLA_lnk.cmd` | 新增 `.upybuf`、`.flashfunc` |
| `tools/build_minimal.py` | 源列表、F021 库与 include |
| `Platform/Protocol/Test/upgrade_stub_test.c` | 主机测试 |
| `Platform/Port/Test/shim/F021.h` | F021 替身 |

## 11. 主机测试（原 12 组 + 新增 4 组）

原 12 组：握手 9→8 字节、首包 SOH 96 字节、重复包只 ACK、CRC 错 NAK、序号错 NAK、EOT 两次、CAN 两次取消、文件头非法立即 CAN、超长文件拒绝、擦除顺序、写满一包、配置回读。

新增：

| 序号 | 场景 | 断言要点 |
| --- | --- | --- |
| 13 | 超过 64 KiB 且包序号 255→0 | 256 个 STX 包（256 KiB）后序号回绕到 0，写入地址连续、长度累计正确 |
| 14 | 首包 STX | 首包 992 字节正文，写入地址从 `0x0A0000+16` 起（32 字节头） |
| 15 | Flash 失败不 ACK | F021 替身返回错误 → 不回 ACK、进 ERROR、不提交配置 |
| 16 | 控制字节发送遇忙 | `SciPortWriteTx` 返回 0 时 `'C'`/ACK 留待重试，状态机不推进 |

## 12. 验收要点

1. 握手回 8 字节，`'C'` 在擦除完成后单独发，主机在 `'C'` 之前不发数据也能升级。
2. 升级全程功率输出与控制 ISR 关闭，看门狗不复位；中止后不自动恢复。
3. 写备份区：8 字块、末块补齐、64 位单元不重复写；`.flashfunc` 确实在 RAM 执行。
4. 成功路径写配置并回读确认，最终 ACK 发完才复位；取消路径不提交配置。
5. 主机测试全绿；两态构建台账（生产/测量）与 05E-r3 对照。

## 13. 已裁定事项

| 事项 | 裁定 |
| --- | --- |
| 握手回复 9 字节改 8 字节 | 同意；升级层擦除完成、准备好接收后单独发 `'C'` |
| 中止后交还 Modbus | 同意；清理本次协议残留、完成必要重同步，主机重新握手 |
| 备份区擦除范围 | 整区擦除，按 H / I / J / K 实际扇区起点执行 |
| 升级期间其它任务 | 暂停功率输出、控制 ISR 及 ADC/CLA 触发；保留时基、端口服务、看门狗；中止后不自动恢复功率输出 |
| 参考仓库判断 | 以 git 提交 `68701bd` 为准，本机副本差异按 §2.2 记录 |
| Flash 分块 | 保留，但理由改为"主机等 ACK"，且必须"本块操作完成后返回调度器" |
| 末块 | 满足 64 位编程单元对齐与 ECC，同一单元不重复写 |
| 包缓冲位置 | `RAMGS2` 可接受，构建后检查 RAM 执行段与缓冲段 |

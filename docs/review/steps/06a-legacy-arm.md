# 06A 旧 ARM 升级兼容（19200 F0 → 115200 `01 10` → `'C'`）

| 项目 | 内容 |
| --- | --- |
| 状态 | r3（2026-11-11）——报文识别收紧为唯一 6 字节形式、恢复 19200 成功才交还端口；两态编译通过，待板测 |
| 依据 | `EP15K_升级兼容实施说明 (1).md`（旧 ARM 不改；DSP 侧补齐两阶段握手与切速） |
| 范围 | 只在 APP 侧补旧 ARM 入口：`F0 00 12 03 04` → 切 115200 → `01 10 04 38 00 02` → 准备就绪后才发 `'C'`；不动 XMODEM、Flash、升级流程本身 |
| 不做 | 不改 BOOT 行为（BOOT 已有掉电重入 `1152 → 115200`）；不改主站工具；不引入新的配置字段 |

## 0 结论先行

旧 ARM 的两阶段流程在 DSP 侧只差三件事，本轮全部补齐，且都不需要改 ARM、不需要改升级文件格式：

1. **19200 上的 `F0 00 12 03 04`**：新增识别，原样回 `F0 00 12 05 06`（5 字节，不追加 CRC）。
2. **切速时机**：F0 应答**真正发完**（`SciPortIsTxIdle()`）之后才 `Hw_SciSetBaud(115200)`，随后丢弃 19200 阶段残留、进入"等 `01 10`"状态；等待上界 5000 ms，超时即恢复 19200 交回 Modbus。
3. **准备成功才发 `'C'`**：115200 上的 `01 10 04 38 00 02` 回 8 字节应答（6 字节回显 + 现算 CRC，即 `C1 35`），**应答发完**才把端口交给升级层；升级层按 `WAIT_TX → ERASE（擦备份区）→ 写"接收中"配置 → RECEIVE` 推进，`XmodemReceive()` 的 `'C'` 是这条链最后一步，所以擦除或配置写入失败时不会有 `'C'`。

三项都能在本轮代码里指到具体行（见 §4），不需要等板测才能确认行为；板测只需要确认线上时序与抓包数据（见 §7）。

与此前的差异：旧路径（直接在 19200 上发 `01 10`）**保留**，因此 BOOT 救援、新主站直连都还能用；旧 ARM 走新路径时才有 F0 与切速。

### 0.1 复核意见处置（r2）

| 复核意见 | 最小改法 | 落点 |
| --- | --- | --- |
| CRC 错的 RTU 帧也会触发升级 | 握手只接受**整帧恰好 6 字节**的请求；多余字节（数据段/CRC）或截断帧都不回应、留在等待态由主站重发。**握手阶段不预存升级流**（ARM 要等 `'C'` 才发首包） | `AppMbIsTransferRequest()`、`AppMbIsRequestPrefix()`（`AppModbus.c`）；`WAIT_REQ` 与 19200 直连入口都走同一判定 |
| 发送故障后仍可能切速或擦备份区 | 故障代次变化时若正处于 `F0_TX`/`REQ_TX`，整段取消握手（回 19200、清残留）并本轮返回，不再继续切速/交接 | `AppMbProtoService()` 第 1 步 |
| 回退不检查切速结果、不同步 `s_au32Baud` | 新增 `AppModbusRestoreBaud()`：`Hw_SciSetBaud()` 成功才同步协议层记录，失败则保持记录与硬件一致；`AppMbLegacyRestore()` 与 `AppUpgradeAbort()` 都改调它 | `AppModbus.c` / `AppUpgrade.c` |
| 恢复 19200 失败仍交还端口 | 切速成功才交还：收尾改为进入 `APP_MB_LEGACY_RESTORE` 状态，服务逐轮重试切速，成功后才清状态、`SciPortResync` 并让 Modbus 接管；失败计入 `AppModbusBaudErrors()` | `AppMbLegacyRestore()`、服务状态机、`AppModbusRestorePort()` |
| 13 字节 RTU 兼容分支 | **删除**：`WAIT_REQ` 只认"整帧恰好 6 字节且内容为 `01 10 04 38 00 02`"，多于或少于 6 字节一律丢弃继续等；不再调用 `CheckModbusCrc()` | `AppMbIsTransferRequest()` |
| 停机触发比说明文档晚 | 新增 `AppUpgradePrepare()`：接受 F0 时校验升级许可（Flash API 就绪、无进行中会话）并请求停机；不许可就不回应答。`AppUpgradeStart()` 的暂停改为"只做一次" | `AppUpgrade.c`、`AppModbus.c` 的 F0 分支 |

## 1 说明文档的流程契约与本工程落点

| 说明文档要求 | 本工程落点 |
| --- | --- |
| 19200：`F0 00 12 03 04` → `F0 00 12 05 06` | `AppMbIsLegacyCmd()` / `AppMbLegacyReply()`；应答前先调 `AppUpgradePrepare()` 请求停机（`AppModbus.c` 的 `AppMbDispatchFrame()` F0 分支） |
| 切 115200 后再发 `01 10 04 38 00 02` | `AppMbProtoService()` 的"2b) 旧 ARM 兼容状态机"（`AppModbus.c:631`）第 `F0_TX` 分支 |
| `01 10` 应答 = `01 10 04 38 00 02 C1 35` | `AppMbDispatchFrame()` 的 `WAIT_REQ` 分支：`AppMbIsTransferRequest()` 通过才 `PackModbusCrc(&pu16Data[0], 6)` + 8 字节 |
| 应答发完、擦完备份区、写完"接收中"配置后才发 `'C'` | `AppModbus.c:653` 的 `REQ_TX` 分支 → `AppUpgradeStart()` → `AppUpgradeTask()` 三阶段 |
| 失败统一收尾：停会话、清残留、恢复 19200、交回 Modbus、功率保持关闭 | `AppMbLegacyRestore()` / `AppUpgradeAbort()`，两者都经 `AppModbusRestoreBaud()` 同步硬件与记录 |
| 掉电重入按配置里的 `1152` 恢复 115200 | `AppUpWriteProgressCfg()` 写 `u16UpgradeBaud = AppUpBaudCode(s_u32UpBaud)`（`AppUpgrade.c:67`），BOOT 的 `PrepareResumeReceive()` 读它 |

## 2 三条核对项逐条说明

### 2.1 `01 10` 请求帧只认两种确定形式，握手阶段不预存升级流

- 识别条件（只有一条）：`pstPort == &s_astPort[0]`（SCI-A）、**整帧恰好 6 字节**、且内容逐字节等于 `{从站地址 1, 0x10, 0x04, 0x38, 0x00, 0x02}`。
- 多于 6 字节（例如后面紧跟数据或 CRC）或少于 6 字节（截断）一律**不算请求**：不回应，留在等待态继续等主站重发。13 字节标准 RTU 兼容分支已按复核意见删除，不再调用 `CheckModbusCrc()`。
- **握手阶段不预存升级流**：ARM 要等 `'C'` 之后才发首包，握手帧之后多出来的字节不可能是合法升级数据。`AppUpgradeStart()` 的 `pu16Tail/pu16TailLen` 形参保留（协议层接口不变），实际传 `0, 0`。
- 静态接收缓冲里保存的是"当前这一帧"，不是"握手之后的字节流"：等这一帧收完（`AppMbFrameIdle()` 判静默或批次边界）才进 `AppMbDispatchFrame()` 判定，所以超长帧不会被截断成 6 字节误判。
- 应答固定 8 字节：`01 10 04 38 00 02` + CRC。CRC 是对**前 6 字节**现算的，`ModbusCRC16()` 得 `0x35C1`，低字节在前落成 `C1 35`，与说明文档一致。
- 不是标准 Modbus 请求（0x10 声称写 2 个寄存器却没有数据段），所以**不交给参考协议层**，避免被当成 `83 03` 错误帧。

### 2.2 切速时机

```
(19200) 收 F0 命令 ──► AppUpgradePrepare()（许可 + 停机）──► 回 5 字节 ──► 等 SciPortIsTxIdle()
                                        │
                                        ├─ 切速失败 ──► AppMbLegacyRestore()：恢复 19200、交回 Modbus
                                        ▼
                              Hw_SciSetBaud(115200) 成功
                                        ▼
                        AppMbDropRx()（只清 19200 阶段残留）+ 记时刻
                                        ▼
                             (115200) 等 01 10，上界 5000 ms
                                        │
                    ┌───────────────────┴────────────────────┐
              收到 01 10                                超时/异常
                    ▼                                      ▼
        回 8 字节，等它发完                        恢复 19200、交回 Modbus
                    ▼
            交端口给升级层
```

- 切换必须等在**应答发完之后**：`Hw_SciSetBaud()` 会改 SCI 分频，正在移位的最后一个字节会按新分频发送，ARM 会收到坏字节。
- 切换前把 `s_au32Baud[0]` 更新为 115200，协议层后续的 t3.5 判定才按新速率算；同时 `u32RxTick` 重新起算。
- 切速失败、等待超时、`AppUpgradeStart()` 返回 0 三种情况都走同一个 `AppMbLegacyRestore()`：SCI-A 回 19200、清候选与未发回复、`SciPortResync()`、刷新故障代次，Modbus 立即恢复工作。
- 5000 ms 是"等 115200 请求"的上界，不是 XMODEM 超时：ARM 侧从收到 F0 应答到发 `01 10` 是毫秒级，留 5 s 是给 ARM 重启/重试的余量。

### 2.3 `'C'` 只在准备成功后出现

交付顺序（`AppUpgradeStart()` 之后）：

| 阶段 | 动作 | 失败后果 |
| --- | --- | --- |
| `WAIT_TX` | 等 8 字节应答出环 | 无（继续等） |
| `ERASE` | `EraseBackUpBank()` 逐扇区擦 H/I/J/K | `AppUpgradeAbort()`：不发 `'C'` |
| `ERASE` 末 | `AppUpWriteProgressCfg()` 写"接收中"配置（`0xAAEE`、SCI-A、`1152`、`FirmwareOver=0`） | 同上 |
| `RECEIVE` | `XmodemReceive()` 首个 `STATE_IDLE` 发 `'C'` | 中止时同样收尾 |

因此"备份区准备成功后才发 `'C'`"由状态机顺序保证，不是靠上层自觉。

### 2.4 停机时序（r2 修正）

- 接受 F0 的那一刻就调 `AppUpgradePrepare()`：校验 `s_u16FlashReady`（Flash API 就绪）与 `s_u16UpActive`（无进行中会话），通过后立即调暂停钩子并置 `s_u16StopRequested`。
- 许可不通过就**不回应答**、丢弃候选帧，ARM 会重试；这样不会出现"已回 F0 应答但功率输出还在跑"的窗口。
- 暂停只做一次：`AppUpgradeStart()` 看到 `s_u16StopRequested` 已置位就不再重复调用；`AppUpgradeInit()` 在开机时清零。
- 握手失败（切速失败、5 s 超时、故障代次变化）只恢复串口与端口，**不重新打开功率输出**（与 05F 裁定一致），由复位恢复。

### 2.5 恢复业务速率的门控（r3 修正）

- 收尾不再"切速 + 交还"一气做完：`AppMbLegacyRestore()` 只丢残留并进入 `APP_MB_LEGACY_RESTORE`。
- 服务状态机在 `RESTORE` 分支逐轮调 `AppModbusRestoreBaud()`；**成功**才清状态、`SciPortResync()`、刷新故障代次与时刻，Modbus 从下一次服务开始接管。
- **失败**（`Hw_SciSetBaud()` 返回 0）就 `return 1U` 停在恢复态继续重试，`u32BaudErrors++`；期间不收帧、不回 Modbus。理由：硬件可能还在 115200，若此时交还端口，主站按 19200 发来的帧永远收不到，症状是"升级失败后再也不通信"。
- 切速之前还有一道**发送门控**（r4）：取消或传输错误时 XMODEM 可能刚把 NAK/CAN 放进发送队列，若此时切速，这些字节会以 19200 的位宽上线，主站收到半截帧。
  - 正常中止：先等 `SciPortIsTxIdle()` 为真（软件环 + FIFO + 移位寄存器都空，即最后一个控制字节确实按 115200 发完），再切回 19200。
  - 发送故障（`SciPortIsTxSuspended()` 为真）：先 `AppMbDropTx()` 明确丢掉旧数据，再 `SciPortTxResume()`；端口未允许恢复就留在本态继续等，恢复后同样要过 `SciPortIsTxIdle()`。
- `AppUpgradeAbort()` 也走同一条门（`AppModbusRestorePort()`），不再直接 `AppModbusReleasePort()`。
- 诊断：`AppModbusBaudErrors(u16Port)` 暴露失败次数，上板时可用它确认是否出现过切速失败。

## 3 状态机

| 状态 | 进入条件 | 主要动作 | 离开条件 |
| --- | --- | --- | --- |
| `APP_MB_LEGACY_NONE` | 上电/握手完成/收尾 | 正常 Modbus 收帧 | 收到 F0 命令且 `AppUpgradePrepare()` 通过 → `F0_TX` |
| `F0_TX` | 已回 5 字节 | 等应答出环 | 出环且切速成功 → `WAIT_REQ`；切速失败 → 收尾 |
| `WAIT_REQ` | 已切 115200 | 只认 F0（重探）与恰好 6 字节的 `01 10`，其余帧丢弃 | 收到 `01 10` → `REQ_TX`；超 5 s → `RESTORE` |
| `APP_MB_LEGACY_RESTORE` | 任何收尾路径 | 等发送空闲 + 逐轮重试切回业务速率 | 发送空闲且切速成功 → `NONE`（交还 Modbus）；未空闲或切速失败 → 停在本态重试 |
| `REQ_TX` | 已回 8 字节 | 等应答出环 | 出环后交接升级 → `NONE`；交接失败 → `RESTORE` |

`u16Legacy` 与 `u32LegacyTick` 是每端口字段，`AppModbusInit()` 里清零；收尾统一清 `u16Armed`/`bArmPending`，保证升级失败后 Modbus 直接可用。

## 4 代码改动点

| 文件 | 位置 | 改动 |
| --- | --- | --- |
| `UserApp/Src/AppModbus.c` | `:60-82` | 新增兼容常量（5/8 字节、19200/115200、5000 ms、四个状态）与两张命令/应答表 |
| 同上 | `:94-95` | `ST_APP_MB_PORT` 增 `Uint16 u16Legacy; Uint32 u32LegacyTick;` |
| 同上 | `:399` / `:420` / `:437` | `AppMbIsLegacyCmd()` / `AppMbLegacyReply()` / `AppMbLegacyRestore()` |
| 同上 | `AppMbIsRequestPrefix()` / `AppMbIsTransferRequest()` | 只认"整帧恰好 6 字节"；`AppMbIsHandshake()` 转调后者 |
| 同上 | `AppModbusRestoreBaud()` | 返回 1/0；成功才同步 `s_au32Baud[]`，失败累计 `u32BaudErrors` |
| 同上 | `AppModbusRestorePort()` / `AppModbusBaudErrors()` | 升级结束后"切速成功才交还端口"的入口与诊断计数 |
| 同上 | `AppMbDispatchFrame()` | F0 分流（先 `AppUpgradePrepare()`）；`WAIT_REQ` 下只认两种确定帧、回 8 字节；直连握手同样不再存尾部，交接传 `0, 0` |
| 同上 | `AppMbProtoService()` | "2b) 旧 ARM 兼容状态机"（F0 出环才切速、8 字节出环才交接、5 s 兜底）；第 1 步故障代次变化时若在 `F0_TX`/`REQ_TX` 则整段取消 |
| `UserApp/Src/AppUpgrade.c` | `:33-52` | 新增 `s_u32UpBaud`、`AppUpBaudCode()`（≥115200→1152、≥38400→384、≥19200→192、否则 0） |
| 同上 | `:67` | `AppUpWriteProgressCfg()` 写 `u16UpgradeBaud = AppUpBaudCode(s_u32UpBaud)`，删掉过时的"0 = 保持 38400" |
| 同上 | `:98` / `:120` | `AppUpgradeStart()` 增 `Uint32 u32Baud` 形参并记录 |
| 同上 | `AppUpgradeAbort()` | 改调 `AppModbusRestoreBaud(HW_SCI_ID_A, 19200)`，再交回 Modbus |
| 同上 | `AppUpgradePrepare()` | 握手许可 + 停机请求：`s_u16FlashReady`/`s_u16UpActive` 校验，首次调 `s_pfPause()` 并置 `s_u16StopRequested` |
| 同上 | `AppUpgradeStart()` | 暂停改为"`s_u16StopRequested` 未置位才做一次" |
| `UserApp/Inc/AppUpgrade.h` | `:27` | 同步新签名与注释 |
| `Platform/Xmodem/Src/XmodemCore.c` | `:151` / `:322` | 删两个从未使用的局部变量，消掉 `#179-D`（与 BOOT 侧 `0bb7e09` 同一处置） |

`XmodemCore.c` 只是告警清理，不改逻辑；此前 APP 侧一直带着这两条 `#179-D`。

## 5 与 BOOT 侧的一致性

| 约定 | APP 写入 | BOOT 读取 |
| --- | --- | --- |
| 波特率编码 | `1152`（= 115200/100） | `PrepareResumeReceive()`：`u16UpgradeBaud * 100` 恢复，`0` 时按 19200 |
| 串口 | `u16UpgradeUart = 0`（SCI-A） | 只认 `HW_SCI_ID_A` |
| 标志 | `u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0` | `GetUpgradeFlag()==1 && GetUpgradeFirmwareFlag()==0` → 续收 |

本轮把"0 档"从 38400 的旧解释统一成 19200，与 BOOT 侧 `UPGRADE_DEFAULT_BAUD` 一致；只有当调用方没给速率时才会落到 0 档，正常路径一律写实际速率。

## 6 两态构建台账

| 项目 | 生产态 `PORT_BENCH_TEST=0` | 测量态 `PORT_BENCH_TEST=1` |
| --- | --- | --- |
| `.out` | 1,032,776 B `aefbf65a…` | 1,050,736 B `9d6c2d91…` |
| `.map` | 129,114 B | 133,682 B |
| `.text` | `0x65C2`（26,050 字） | `0x6941`（26,945 字） |
| `FLASH_APP` 用/余 | `0x8683` / `0x1197B` | `0x89FC` / `0x11602` |
| `RAMGS0` 余 | 4 字 | 3 字 |
| `RAMGS1` 用/余 | `0x0AA8` / `0x158` | `0x0BD8` / `0x28` |
| `.bin` | 68,932 B `8ef83236…` | 70,708 B `e1e7476c…` |
| `_UpgradeCRC32.bin` | 68,964 B `07e1d842…`，头部 `FileSize 0x10D44`、`CRC32 0x832B15B2` | 70,740 B `a291ed75…`，头部 `FileSize 0x11434` |

- 编译单元 63 个、**0 错误 0 告警**；`.out` 在相同源码下两次构建 sha256 一致（`.map` 每次不同，与既往结论一致）。
- 与 r3（`.out` 1,032,588 B、`.bin` 68,884 B、升级包 68,916 B）相比：`.text` +24 字、`.bin` +48 B——r4 只在恢复态加了发送门控（`SciPortIsTxSuspended`/`SciPortTxResume`/`SciPortIsTxIdle`）。
- 一次测量态构建触发过 cl2000 内部错误（`acia2000.exe` 段错误，指向 `Platform/Xmodem/Inc/XmodemTimeOut.h:6`），立即重试即通过；同一源码此后构建稳定，判为编译器偶发，不是源码问题。

## 7 板测验收（抓包）

1. **完整请求帧**：抓 19200 上的 `F0 00 12 03 04` 与 115200 上的 `01 10 …` 全部字节，确认 DSP 回的 8 字节是 `01 10 04 38 00 02 C1 35`。
   - 若 ARM 发来的不是恰好 6 字节（例如多带数据/CRC，或被截断），确认 DSP **不回应**、串口停在 115200 等待重发，且 5 s 后回到 19200。
   - 13 字节 RTU 形式本轮不再接受：若实测 ARM 确实那样发，会表现为一直等不到应答，需要回到本轮意见重新确认（已记入 §8 风险）。
2. **切速时机**：确认 `F0` 的 5 字节应答在波特率变化**之前**完整发出（示波器或串口分析仪看最后一字节的位宽），DSP 侧对应"应答出环才切速"。
3. **`'C'` 时序**：确认 `'C'` 出现在备份区四个扇区擦完之后；人为让擦除失败一次，确认没有 `'C'` 且串口回到 19200、Modbus 可应答。
4. **首包一致性**：抓首包数据区的 32 字节头与文件前 32 字节比对，并确认 `FileSize`/`CRC32` 与 `_UpgradeCRC32.bin` 头一致。
   - 当前构建：正文 68,932 B，头部 `FileSize` 应为 **`0x10D44`**；升级包 68,964 B（`0x10D64`）= 正文 + 32 字节头，`CRC32 = 0x832B15B2`（头部第 6..9 字节，小端）。
   - 本次仓库未附升级包，板测以实际发出的文件头为准（正文大小随构建变化，`FileSize` 必须等于正文长度）。
5. **掉电/超时回退**：115200 上不发 `01 10`，确认 5 s 后回到 19200；升级中途 CAN，确认回到 19200 且功率输出保持关闭。
6. **恢复门控**：升级失败后确认 `AppModbusBaudErrors(0)` 为 0（说明 19200 一次切回成功）；若注入切速失败，确认端口停在恢复态、主站 19200 帧不会在硬件仍为 115200 时被交还。
   - **计数口径**（三个量各记各的，不要合并看）：
     - `AppModbusBaudErrors(0)`：只累计 `Hw_SciSetBaud()` 调用失败；停在"等发送空闲"期间它一直是 0，所以为 0 **不等于**已经切回 19200，要连同"Modbus 是否恢复应答"一起判断。
     - `AppModbusTxDropped(0)`：只累计协议层丢掉的待发回复（`AppMbDropTx()`，且仅在确有 `u16TxLen` 或 `bArmPending` 时才加一）。
     - `SciPortTxResetDrop(HW_SCI_ID_A)`：端口层因强制复位实际清掉的 TX 字节数，与上面两个互不重叠。

## 8 未覆盖与风险

- **无主机测试**：本机没有可用的主机 C 编译器，本轮只有编译与台账验证；行为验证依赖板测抓包。
- **5000 ms 是估算上界**：说明文档留了"115200 下是否重发 F0/是否读 5000"等待抓包项。已实现的对策是：`WAIT_REQ` 期间收到 F0 会重新应答并重置计时，因此 ARM 重试不会失败；若实测 ARM 的间隔更长，只需调 `APP_MB_LEGACY_WAIT_MS`。
- **115200 下的 F0**：本轮允许（会重回应答并重新计时），但说明文档未确认 ARM 是否会这样做；抓到实际行为后可决定是否收紧。
- **握手阶段不再预收升级流**：若实测发现 ARM 确实会在收到 8 字节应答之前就发首包（与说明文档矛盾），表现为首包被丢弃、ARM 等 `'C'` 超时；届时再按抓包结果恢复"预存尾部"的做法。
- **只认 6 字节**：若 ARM 实际发的是 13 字节标准 RTU 形式，本轮会一直等不到应答（5 s 后回 19200）。这是按复核意见刻意收紧的结果，抓包确认后再决定是否放开。
- **恢复态可能长期停留**：`Hw_SciSetBaud()` 持续失败时端口停在 `RESTORE` 重试，Modbus 不接管（宁可不通也不在半速上错通）。失败次数可由 `AppModbusBaudErrors()` 读出。
- **ARM 掉电续传**：说明文档说主站按重新握手结果从首包重发；本工程 BOOT 侧续收前会擦备份区，若 ARM 续传旧包号会得到 `XMODEM_SEQUENCE_ERROR` 并从头再来，属预期，但需要板测确认 ARM 确实会重发首包。
- **BOOT 里的旧 ARM 入口**：说明文档列为可选项，本轮未做（BOOT 目前只在 19200 接受直连握手）。等抓包确认后再决定。

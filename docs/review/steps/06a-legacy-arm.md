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
  - `SciPortIsTxSuspended()` 为真时要先分两种挂起（r5）：
    - **强制复位丢过内容**（`SciPortIsTxResetLoss()` 为真）：旧字节已作废，`AppMbDropTx()` 丢掉协议层待发，再 `SciPortTxResume()` 请求解冻；该函数只在 **RX 已回到 `SCI_RX_IDLE` 且 `bTxResetLoss` 确实置位** 时才成功并清该标志，未允许就留在本态继续等。
    - **恢复期间的临时挂起**（`SciPortIsTxResetLoss()` 为假）：端口层排空后自行解除（TX 服务在 `bTxResetLoss == 0` 时清 `bTxSuspended`），协议层**不强行解冻、也不丢待发**，等它解除后再过 `SciPortIsTxIdle()`，让控制字节按 115200 发完。
- `AppUpgradeAbort()` 也走同一条门（`AppModbusRestorePort()`），不再直接 `AppModbusReleasePort()`。
- 诊断：`AppModbusBaudErrors(u16Port)` 暴露失败次数，上板时可用它确认是否出现过切速失败。

### 2.6 ARM 自身升级通知（r6 新增）

说明文档追加了一条 ARM 自有协议：SCIA 19200 上 `F0 00 12 07 08` → 回 `F0 00 12 09 0A`，
用于"ARM 自身升级"。DSP 侧只做两件事：关驱动、回应答。

| 项 | 取值 | 说明 |
| --- | --- | --- |
| 请求 | `F0 00 12 07 08` | 第 4/5 字节与 DSP 升级请求 `F0 00 12 03 04` 不同，必须按完整 5 字节区分 |
| 应答 | `F0 00 12 09 0A` | 5 字节，原样回，不追加 CRC |
| 动作 | `AppUpgradeArmNotify()` → `s_pfPause()` | 现有"关闭驱动"入口（`main.c:582` 的 `AppUpgradePause()`） |
| 不做 | 切速 / 擦 Flash / 写配置 / 复位 / 发 `'C'` | SCIA 保持 19200，Modbus 继续工作 |
| 重复 | 允许 | 再收到就再回一次；`s_u16StopRequested` 保证停机只做一次 |

落地位置：`UserApp/Src/AppModbus.c` 的 `s_au16ArmNotifyCmd/Reply`、`AppMbIsArmNotifyCmd()`
（只认 SCI-A 且恰好 5 字节）、`AppMbDispatchFrame()` 最前面的分流；`UserApp/Src/AppUpgrade.c`
的 `AppUpgradeArmNotify()`。BOOT 侧同一指令只应答（见 `docs/boot-minimal.md` §3.9）。

## 3 状态机

| 状态 | 进入条件 | 主要动作 | 离开条件 |
| --- | --- | --- | --- |
| `APP_MB_LEGACY_NONE` | 上电/握手完成/收尾 | 正常 Modbus 收帧 | 收到 ARM 通知 → 回 5 字节、保持本态；收到 F0 命令且 `AppUpgradePrepare()` 通过 → `F0_TX` |
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
| 同上 | `AppMbDispatchFrame()` | F0 分流（先 `AppUpgradePrepare()`）；`WAIT_REQ` 与直连握手都只认整帧恰好 6 字节、回 8 字节，交接传 `0, 0` |
| 同上 | `AppMbProtoService()` | "2b) 旧 ARM 兼容状态机"（F0 出环才切速、8 字节出环才交接、5 s 兜底）；第 1 步故障代次变化时若在 `F0_TX`/`REQ_TX` 则整段取消 |
| `UserApp/Src/AppUpgrade.c` | `:33-52` | 新增 `s_u32UpBaud`、`AppUpBaudCode()`（≥115200→1152、≥38400→384、≥19200→192、否则 0） |
| 同上 | `:67` | `AppUpWriteProgressCfg()` 写 `u16UpgradeBaud = AppUpBaudCode(s_u32UpBaud)`，删掉过时的"0 = 保持 38400" |
| 同上 | `:98` / `:120` | `AppUpgradeStart()` 增 `Uint32 u32Baud` 形参并记录 |
| 同上 | `AppUpgradeAbort()` | 改调 `AppModbusRestorePort(APP_UP_PORT_INDEX, 19200)`：切速成功才交还端口，失败停在恢复态重试 |
| 同上 | `AppUpgradePrepare()` | 握手许可 + 停机请求：`s_u16FlashReady`/`s_u16UpActive` 校验，首次调 `s_pfPause()` 并置 `s_u16StopRequested` |
| 同上 | `AppUpgradeStart()` | 暂停改为"`s_u16StopRequested` 未置位才做一次" |
| `UserApp/Inc/AppUpgrade.h` | `:27` | 同步新签名与注释 |
| `Platform/Port/Inc/SciPort.h` / `Src/SciPort.c` | `:217` / `:1121` | 新增只读访问器 `SciPortIsTxResetLoss()`：区分"强制复位丢过内容"与"恢复期间临时挂起" |
| `UserApp/Src/AppModbus.c` | `:689-703` | `RESTORE` 分支按 `SciPortIsTxResetLoss()` 分路：丢过内容才 `AppMbDropTx()`+`SciPortTxResume()`，临时挂起只等；之后统一过 `SciPortIsTxIdle()` 再切速 |
| `Platform/Xmodem/Src/XmodemCore.c` | `:151` / `:322` | 删两个从未使用的局部变量，消掉 `#179-D`（与 BOOT 侧 `0bb7e09` 同一处置） |
| `UserApp/Src/AppModbus.c` | `:90-97` | 新增 `s_au16ArmNotifyCmd`/`s_au16ArmNotifyReply` 两张 5 字节表 |
| 同上 | `AppMbIsArmNotifyCmd()` | 只认 SCI-A 且恰好 5 字节；与 `AppMbIsLegacyCmd()` 分别比对，互不覆盖 |
| 同上 | `AppMbDispatchFrame()` 最前面 | ARM 通知分流：回 5 字节、调 `AppUpgradeArmNotify()`、不改 `u16Legacy`、不切速 |
| `UserApp/Src/AppUpgrade.c` / `Inc/AppUpgrade.h` | `AppUpgradeArmNotify()` | 只调暂停钩子并置 `s_u16StopRequested`；`AppUpgradePrepare()` 之后不会再重复停机 |

`XmodemCore.c` 只是告警清理，不改逻辑；此前 APP 侧一直带着这两条 `#179-D`。

## 5 与 BOOT 侧的一致性

| 约定 | APP 写入 | BOOT 读取 |
| --- | --- | --- |
| 波特率编码 | `1152`（= 115200/100） | `PrepareResumeReceive()`：`u16UpgradeBaud * 100` 恢复，`0` 时按 19200 |
| 串口 | `u16UpgradeUart = 0`（SCI-A） | 只认 `HW_SCI_ID_A` |
| 标志 | `u16UpgradeFlag = 0xAAEE`、`u16FirmwareOver = 0` | `GetUpgradeFlag()==1 && GetUpgradeFirmwareFlag()==0` → 续收 |

BOOT 侧本轮也补齐了同一套握手（`docs/boot-minimal.md` §3.9）：APP 无效时 BOOT 自己完成
19200 的 F0 段与 115200 的 `01 10` 段，再擦备份区、写"接收中"（`u16UpgradeBaud = 1152`）并发 `'C'`；
失败或取消时等控制字节发完才切回 19200。`F0 00 12 07 08` 在 BOOT 只应答，不切速。

本轮把"0 档"从 38400 的旧解释统一成 19200，与 BOOT 侧 `UPGRADE_DEFAULT_BAUD` 一致；只有当调用方没给速率时才会落到 0 档，正常路径一律写实际速率。

## 6 两态构建台账

| 项目 | 生产态 `PORT_BENCH_TEST=0` | 测量态 `PORT_BENCH_TEST=1` |
| --- | --- | --- |
| 对应提交 | `fad30f7`（点表 C/D 与宽度检查）+ `0117766`（ARM 通知），工作树除 `.settings/` 外无改动 | 同左 |
| `.out` | 1,036,580 B `c23d3ab4…` | 1,054,476 B `7dd31f80…` |
| `.map` | 129,418 B `ac273d8a…` | 133,986 B `45345141…` |
| `.text` | `0x6629`（26,153 字） | `0x69A8`（27,048 字） |
| `FLASH_APP` 用/余 | `0x8722` / `0x118DC` | `0x8A9B` / `0x11563` |
| `RAMGS0` 余 | 4 字 | 3 字 |
| `RAMGS1` 用/余 | `0x0B84` / `0x07C` | `0x0C00` / `0x000` |
| `.bin` | 69,252 B `e93a52ee…` | 71,012 B `070ecc98…` |
| `_UpgradeCRC32.bin` | 69,284 B `1376138c…`，头部 `FileSize 0x10EA4`、`CRC32 0xDB672216` | 71,044 B `b2dc8c45…`，头部 `FileSize 0x11584`、`CRC32 0xC1B1087F` |

- 编译单元 63 个、**0 错误 0 告警**；`.out` 同源码同目录重编 sha256 一致（`.map` 每次不同，与既往结论一致）。
- **本地构建记录（非提交对应，仅历史）**：r6 首次构建时工作树里带着尚未提交的点表改动，得到生产 `.out` `37acca0c…`、`.bin` `df602bcd…`、升级包 `CRC32 0x328DA413`，测量 `.out` `4e7d2dfb…`、`.bin` `90f5c700…`、`CRC32 0xC9EED708`。字节数与上表相同，但提交时把 `s_astRegD` 的 INT16 项从 `&s_stRegValD[2]` 改到 `[3]`，内容因此变化，那组数字不再作为交付口径。改槽位的理由是 **布局对照**：`TYPE_FLOAT` 占两个 Modbus 地址，但在 `U_REG_VAL s_stRegValD[]` 里只占一个数组元素（每个元素就是 32 位），`s_stRegValD[1].fVal` 与 `s_stRegValD[2].i16Val` 是两个互不重叠的对象，原来的 `[2]` 并不会覆盖 FLOAT 的高字；改到 `[3]` 只是让「地址 3 ↔ 槽位 `[3]`」一一对上，便于查表，不是修缺陷。
- 与 r5（`.out` 1,033,256 B、`.bin` 68,980 B）相比：`.text` +80 字、`.bin` +272 B——r6 新增 ARM 通知的两张 5 字节表、`AppMbIsArmNotifyCmd()` 与 `AppUpgradeArmNotify()`；另有 `fad30f7` 引入的 C/D 点表（`ATS_MODBUS_REG_COUNT 10`、`MACHINE_MODBUS_REG_COUNT 100`，110 个 `U_REG_VAL` 占 `.mbregs` 220 字）。
- 测量态 `RAMGS1` **刚好占满**（`0x0C00/0x000`）：再往 `.mbregs` 加任何数据都会链接失败，需要先缩 `s_stRegValC/s_stRegValD` 的数组长度或改段分配。联调先用生产态。
- 一次测量态构建触发过 cl2000 内部错误（`acia2000.exe` 段错误，指向 `Platform/Xmodem/Inc/XmodemTimeOut.h:6`），立即重试即通过；同一源码此后构建稳定，判为编译器偶发，不是源码问题。

## 7 板测验收（抓包）

1. **完整请求帧**：抓 19200 上的 `F0 00 12 03 04` 与 115200 上的 `01 10 …` 全部字节，确认 DSP 回的 8 字节是 `01 10 04 38 00 02 C1 35`。
   - 若 ARM 发来的不是恰好 6 字节（例如多带数据/CRC，或被截断），确认 DSP **不回应**、串口停在 115200 等待重发，且 5 s 后回到 19200。
   - 13 字节 RTU 形式本轮不再接受：若实测 ARM 确实那样发，会表现为一直等不到应答，需要回到本轮意见重新确认（已记入 §8 风险）。
2. **切速时机**：确认 `F0` 的 5 字节应答在波特率变化**之前**完整发出（示波器或串口分析仪看最后一字节的位宽），DSP 侧对应"应答出环才切速"。
   - **紧跟应答的边界**：ARM 收完最后一个停止位就立刻切 115200 发第二次握手（不留间隔），确认 `SciPortResync()` 的排空窗口没把 `01 10 04 38 00 02` 的首字节当残留吞掉——DSP 仍要回 8 字节应答。BOOT 侧同一条时序见 `docs/boot-minimal.md` 第 6 节第 17 条。
3. **`'C'` 时序**：确认 `'C'` 出现在备份区四个扇区擦完之后；人为让擦除失败一次，确认没有 `'C'` 且串口回到 19200、Modbus 可应答。
4. **首包一致性**：抓首包数据区的 32 字节头与文件前 32 字节比对，并确认 `FileSize`/`CRC32` 与 `_UpgradeCRC32.bin` 头一致。
   - 当前构建：正文 68,980 B，头部 `FileSize` 应为 **`0x10D74`**；升级包 69,012 B = 正文 + 32 字节头，`CRC32 = 0x5EE9518B`（头部第 6..9 字节，小端）。
   - 本次仓库未附升级包，板测以实际发出的文件头为准（正文大小随构建变化，`FileSize` 必须等于正文长度）。
5. **掉电/超时回退**：115200 上不发 `01 10`，确认 5 s 后回到 19200；升级中途 CAN，确认回到 19200 且功率输出保持关闭。
6. **恢复门控**：升级失败后确认 19200 的 Modbus 恢复应答；`AppModbusBaudErrors(0)` 为 0 只说明**本次没有记录到切速调用失败**，不能单独作为"已切回 19200"的证据。
   - 若 19200 无应答、且波特率错误计数没增加：先确认是否仍停在 `RESTORE`，再区分两种情形——
     - **强制复位后的冻结未解除**：`SciPortIsTxSuspended()` 与 `SciPortIsTxResetLoss()` 同时为真，`SciPortTxResume()` 要等 RX 回到 `SCI_RX_IDLE` 才成功，此时根本没走到切速那一步；
     - **临时挂起未自行解除**：`SciPortIsTxSuspended()` 为真但 `SciPortIsTxResetLoss()` 为假，端口层排空后会自己解除，期间协议层只是等；
     - **TX 尚未空闲**：冻结已解除但 `SciPortIsTxIdle()` 仍为假，最后一个字节还在环/FIFO/移位寄存器里。
     这两种都在等，计数自然是 0；只有走过门控、真正调用 `Hw_SciSetBaud()` 失败时计数才会增长。
   - **正常取消**：要求看到实际发出的最后一个 NAK/CAN 以 **115200** 完整发完（含停止位），随后 19200 上一条普通 Modbus 请求得到正确应答。
   - **发送故障注入**：旧控制字节可能已被端口层丢弃（`SciPortTxResetDrop()` 增长），此时不要求看到该字节上线；验收点是**端口恢复后能正确切回 19200 并让 Modbus 应答**。
   - **计数口径**（三个量各记各的，不要合并看）：
     - `AppModbusBaudErrors(0)`：只累计 `Hw_SciSetBaud()` 调用失败；停在"等发送空闲"期间它一直是 0，所以为 0 **不等于**已经切回 19200，要连同"Modbus 是否恢复应答"一起判断。
     - `AppModbusTxDropped(0)`：只累计协议层丢掉的待发回复（`AppMbDropTx()`，且仅在确有 `u16TxLen` 或 `bArmPending` 时才加一）。
     - `SciPortTxResetDrop(HW_SCI_ID_A)`：端口层因强制复位实际清掉的 TX 字节数，与上面两个互不重叠。
   - 相关只读访问器：`SciPortIsTxSuspended()`（是否挂起）、`SciPortIsTxResetLoss()`（挂起是否因强制复位丢过内容）、`SciPortIsTxIdle()`（是否可切速）。

7. **ARM 自身升级通知**：19200 上发 `F0 00 12 07 08`，确认回 `F0 00 12 09 0A`（5 字节、无 CRC），
   功率级/PWM 已停（抓 GPIO 88 与 PWM 输出），且随后一条普通 Modbus 读帧仍能在 19200 上得到应答；
   重复发同一条应重复应答，不得出现切速、擦写或复位。BOOT 侧同一条只应答（见 `docs/boot-minimal.md` §6 第 5 条）。

## 8 未覆盖与风险

- **无主机测试**：本机没有可用的主机 C 编译器，本轮只有编译与台账验证；行为验证依赖板测抓包。
- **5000 ms 是估算上界**：说明文档留了"115200 下是否重发 F0/是否读 5000"等待抓包项。已实现的对策是：`WAIT_REQ` 期间收到 F0 会重新应答并重置计时，因此 ARM 重试不会失败；若实测 ARM 的间隔更长，只需调 `APP_MB_LEGACY_WAIT_MS`。
- **115200 下的 F0**：本轮允许（会重回应答并重新计时），但说明文档未确认 ARM 是否会这样做；抓到实际行为后可决定是否收紧。
- **握手阶段不再预收升级流**：若实测发现 ARM 确实会在收到 8 字节应答之前就发首包（与说明文档矛盾），表现为首包被丢弃、ARM 等 `'C'` 超时；届时再按抓包结果恢复"预存尾部"的做法。
- **只认 6 字节**：若 ARM 实际发的是 13 字节标准 RTU 形式，本轮会一直等不到应答（5 s 后回 19200）。这是按复核意见刻意收紧的结果，抓包确认后再决定是否放开。
- **恢复态可能长期停留**：`Hw_SciSetBaud()` 持续失败时端口停在 `RESTORE` 重试，Modbus 不接管（宁可不通也不在半速上错通）。失败次数可由 `AppModbusBaudErrors()` 读出。
- **ARM 掉电续传**：说明文档说主站按重新握手结果从首包重发；本工程 BOOT 侧续收前会擦备份区，若 ARM 续传旧包号会得到 `XMODEM_SEQUENCE_ERROR` 并从头再来，属预期，但需要板测确认 ARM 确实会重发首包。
- **BOOT 里的旧 ARM 入口**：本轮已按说明文档补齐（BOOT 侧救援握手，见 `docs/boot-minimal.md` §3.9），
  但**未上板**：帧识别、切速时序、`'C'` 时机都只有静态核验。
- **ARM 通知后没有恢复入口**：`AppUpgradeArmNotify()` 关驱动后不复位也不恢复，这是实施说明的约定；
  若现场需要"通知后继续跑业务"，要另外定义恢复动作。

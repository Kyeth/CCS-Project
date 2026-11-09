# 步骤 05D：应用层 RingBuff 接入（申请，v0.1）

- base_sha：`83dba4c`（`test/board-bringup-20261031`，05C 基线登记提交）
- 前置：05C 前台轮转**已登记为可用基线**（主机回归交付包 435/0；三路自环、吞吐、停发排空、故障隔离通过）
- 状态：**待所有者裁定**（本文件只是申请，未改任何生产代码）
- 范围：应用层（`UserApp/`）在**现有** `SciPort` / `CanPort` 接口之上使用 `RingBuff`；**CAN 不动、SCI 不再追加性能微调**

---

## 1. 要做的事

在 `UserApp/` 新建链路帧层，把"字节流"变成"应用帧"，并管住三件事：谁拥有队列、发送只发到一半时怎么办、端口故障后怎么恢复。不新增中断、不改 `SciPort`/`CanPort`/`RingBuff`/`PortTask` 的任何语义。

```
心跳 100 µs ──► SciPortService100us() ──► stRxRing(每端口 127 B)   ← 现状，不改
                                              │
主循环每圈 ──► AppLinkService()  ├─ 搬字节：SciPortPopBatch() ──► 重组状态机 ──► stRxFrames
                                 └─ 推字节：stTxFrames 队头 ──► SciPortWriteTx()（部分接受即停）
                                              │
1 kHz 任务 ──► 应用：AppLinkPopFrame() / AppLinkPushFrame()
```

## 2. 接口（新增 `UserApp/Inc/AppLink.h`）

```c
#define APP_LINK_PORTS        (3U)   /* 0=A, 1=C, 2=D，映射 HW_SCI_ID_A/C/D */
#define APP_FRAME_MAX_DATA    (32U)  /* 负载上限（含功能码），整帧最多 36 字节 */
#define APP_RX_DEPTH          (6U)   /* 每端口收帧队列深度 */
#define APP_TX_DEPTH          (4U)   /* 每端口待发帧队列深度 */
#define APP_RX_ROUND_BYTES    (16U)  /* 每圈每端口最多搬的字节数 */
#define APP_FRAME_TIMEOUT_MS  (100UL)

typedef struct {
    Uint16 u16Len;                   /* 实际字节数，1..APP_FRAME_MAX_DATA */
    Uint16 au16Data[APP_FRAME_MAX_DATA];
} ST_APP_FRAME;

void   AppLinkInit(void);
void   AppLinkService(void);                                  /* 主循环每圈调用一次 */
Uint16 AppLinkPopFrame (Uint16 u16Idx, ST_APP_FRAME *pstFrame); /* 1=取到，0=空 */
Uint16 AppLinkPushFrame(Uint16 u16Idx, const ST_APP_FRAME *pstFrame); /* 1=入队，0=队满 */
Uint16 AppLinkTxBusy   (Uint16 u16Idx);                       /* 1=队头帧尚未全部交给硬件 */

Uint32 AppLinkRxFrames(Uint16 u16Idx);
Uint32 AppLinkRxDropFrames(Uint16 u16Idx);                    /* 半帧被断流/超时丢弃 */
Uint32 AppLinkTxFrames(Uint16 u16Idx);
Uint32 AppLinkTxAborted(Uint16 u16Idx);                       /* 复位丢数据导致作废 */
Uint32 AppLinkTxBusyRounds(Uint16 u16Idx);                    /* 环满/暂停而没交出去的圈数 */
```

只用到现有接口：`SciPortPopBatch`、`SciPortWriteTx`、`SciPortTxFree`、`SciPortIsTxSuspended`、`SciPortTxResetQueue`、`SciPortTxResetDrop`、`SciPortIsStreamBreak`、`SciPortClearStreamBreak`、`SciPortFaultGeneration`、`SciPortRxState`、`SciPortRxUsed`、`RingBuffWriteExact`、`RingBuffPeek`、`RingBuffDrop`、`RingBuffUsed`、`RingBuffFree`、`PortEnterCriticalReal` / `PortExitCriticalReal`。

## 3. 队列所有权

| 队列 | 存储位置 | 生产者 | 消费者 | 容量 | 同步 |
| --- | --- | --- | --- | --- | --- |
| SCI 收字节环 `stRxRing` | `.portbuf`（已有） | 心跳 ISR `SciPortService100us()` | 主循环 `AppLinkService()` | 127 B × 3 | 已有 SPSC 约定，不改 |
| 应用收帧队列 `stRxFrames` | 新增 `.appbuf` | 主循环 `AppLinkService()` | 1 kHz 任务（`AppLinkPopFrame`） | 6 帧 × 3 | 两者都在前台，短临界区内 `Push/Pop` |
| 应用待发队列 `stTxFrames` | 新增 `.appbuf` | 1 kHz 任务（`AppLinkPushFrame`） | 主循环 `AppLinkService()` | 4 帧 × 3 | 同上 |
| SCI 发字节环 `stTxRing` | `.portbuf`（已有） | 主循环 `AppLinkService()` | 前台 TX 单元 `SciPortTaskOne()` | 127 B × 3 | 都是前台，无 ISR 参与 |

- **应用层绝不直接写 `stRxRing`/`stTxRing`**：字节环的唯一入口是 `SciPortPopBatch()` 与 `SciPortWriteTx()`。
- `.appbuf` 预算：`ST_APP_FRAME` 34 字 × (6 + 4) × 3 端口 = **1,020 字**；放 `RAMGS1`（现有 3,072 字，` .portstat` 占 20 字）。若嫌紧，把收/发深度降到 4/2 即 612 字。链接脚本加 `APPBUF : > RAMGS1` 段，不占 `RAMGS0`。

## 4. 部分发送

`SciPortWriteTx()` 返回**实际进环**的字节数（0..count），环满时可能只接受一部分。规则：

1. 每端口维护 `s_au16TxOffset[i]`。每圈只提交 `len - offset` 这段，`offset += 返回值`。
2. `offset == len` 才弹出该帧并清零 offset；**绝不重发已进环的字节**（否则线上重复）。
3. 返回 0 时本圈放弃该端口（`u32TxBusyRounds++`），不等待、不阻塞主循环，下一圈再来。
4. `APP_TX_DEPTH` 满时 `AppLinkPushFrame()` 返回 0，由调用方退避；本层不排队、不阻塞。

## 5. 故障恢复

| 事件 | 判据 | 本层动作 |
| --- | --- | --- |
| 断流 | `SciPortIsStreamBreak(i) != 0` | 丢弃正在重组的半帧（`u32RxDropFrames++`）、清本地解析状态；取 `SciPortFaultGeneration(i)` 后调用 `SciPortClearStreamBreak(i, gen)`（**必须带代次**，避免清掉新一轮故障） |
| 端口在恢复态 | `SciPortRxState(i) != SCI_RX_IDLE` | 本圈不搬该端口字节；只累计 `u32RxErrEvents` 的增量，不尝试"抢读" |
| 半帧超时 | 同一帧超过 `APP_FRAME_TIMEOUT_MS` 未收齐 | 丢弃（`u32RxDropFrames++`），重新找帧头 |
| TX 被强制复位清掉 | `SciPortTxResetDrop(i)` 增长，或 `SciPortIsTxSuspended(i) != 0` | 该端口队头帧作废（`u32TxAborted++`、`offset = 0`、弹出）；是否重发由上层决定，本层不自动重发 |
| 恢复完成 | `SciPortRxState(i)` 回 `IDLE` 且断流标志已清 | 正常搬运；解析状态从"找帧头"重新开始 |

本层**不做**：自动重发、序号回填、跨端口去重、总线仲裁。

## 6. 验收（精简，5 项）

复用现有 `Platform/Port/Test/port_stub_test.c` 的桩与 `Check()` 框架，不新建测试设施；新增用例约 20 条断言。

| # | 用例 | 判据 |
| --- | --- | --- |
| 1 | 单端口完整帧 | 注入 1 帧 → `AppLinkPopFrame` 恰好 1 次，长度与内容一致，无多余帧 |
| 2 | 跨批次重组 | 一帧分 3 段（3/3/剩余字节）注入，中间穿插空轮 → 仍重组正确（`SciPortPopBatch` 段长小于帧长） |
| 3 | 断流半帧 | 半帧中途 `SciPortResync()` → 半帧丢弃、`u32RxDropFrames` +1；随后完整帧正常 |
| 4 | 部分发送 | TX 环只剩 4 字节空位时投 13 字节帧 → 先入 4 字节、`offset = 4`；补足空位后余下 9 字节入环、帧弹出；拼出的字节流与源帧逐字节相同（无重复、无缺口） |
| 5 | 恢复后继续 | 仅对 A 注入错误 → A 回 `IDLE` 后收发正常，C/D 的收帧计数与错误计数不受影响 |

上板只加一项观察：三路自环跑 60 s，`AppLinkRxFrames == AppLinkTxFrames`（±在途 1 帧）、`u32RxDropFrames` 与 `u32TxAborted` 为 0。

## 7. 需要所有者裁定

1. **帧格式**：本申请假设 `帧头(1) + 长度(1) + 数据(≤32) + CRC16(2)`，整帧上限 36 字节。实际协议若不同，只需替换内部解析函数与 `APP_FRAME_MAX_DATA`，接口不变。是否由本层负责 CRC？
2. **队列深度**：收 6／发 4（1,020 字）是否合适？还是要更省（4/2，612 字）。
3. **重发策略**：本层"不自动重发"，把 TX 复位丢数据上报给上层。是否接受？
4. **驱动位置**：`AppLinkService()` 挂在主循环（推荐，与 `PortTaskService()` 并列），还是放进 1 kHz 任务？前者延迟低、后者简单。
5. **任务边界**：应用帧的消费/生产放进现有 `AdcSampleTask`，还是新开一个 1 ms 任务？本申请倾向前者（不新增任务）。

## 8. 明确不做

- 不改 `SciPort`/`CanPort`/`RingBuff`/`PortTask`/`PortBudget` 任何语义，不新增中断。
- CAN 侧不新建队列，应用直接用 `CanPortPopRx()` / `CanPortPushTx()`。
- 不追加 SCI 性能优化，不重跑 05C 的板测；`S-05B-08/09` 保持历史未达标记录。
- 控制入口抖动仍单列跟踪，不进本步骤验收。

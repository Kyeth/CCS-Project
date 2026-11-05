# 步骤 05B：SCI 驱动补全、错误恢复与端口接线（申请 v0.5：方案已通过，代码已实施）

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`
- head_sha：工作区待审
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：`S-05B-01`~`S-05B-13`
- 状态：待审

## 0. 修订记录

### 0.1 v0.3 → v0.4（第三轮审阅）

| # | v0.3 的问题 | v0.4 的处理 |
| --- | --- | --- |
| 1 | `DRAIN_REQ` 既表示"待硬件恢复"又表示"软排空"，ISR 读空即回 IDLE 会让恢复请求消失 | §3.4 拆成 6 个状态：`SW_RESYNC`/`RECOVER_REQ`/`HW_RESET`/`DRAIN_ISR`/`PAUSED`/`IDLE`；只有 `SW_RESYNC` 与 `DRAIN_ISR` 读空才回 IDLE；`SciPortResync()` 在被恢复流程占用时给出合并/拒绝语义 |
| 2 | P1 用 `无 TX 挂起` 做准入，P4 自己置 `bTxSuspended` 后下一轮永远 BUSY | §3.5 准入只看恢复阶段；`bTxSuspended` 只限制**普通 TX 提交**；调用顺序固定为"推进恢复 → 复查 TX 门禁 → 普通 TX" |
| 3 | P5 后预算不足会永久停在 `HW_RESET`；`PAUSED` 无出口 | §3.4 保存 `u16RecPhase`，`HW_RESET` 可在下一轮继续未开始的清除、已执行的清除不重复；新增 `SciPortRecoverRetry()` 显式重试 `PAUSED` |
| 4 | TX 查询失败被当成"忙"，8 ms 后走强制复位；时间源失败也会被当作超时 | §3.5 三态分离：查询成功且忙→等待至超时；查询失败→`UNKNOWN`/`PAUSED`；时间源失败→保持等待并计数，不判超时 |
| 5 | 内存单位错（0x800 是 2K 字）、余量与验收数字冲突 | §3.8 改为 `RAMGS1` 0x0C00 + `RAMGS1_PORTBUF` 0x0400（1K 字）；把 3 路 SCI TX 环（384 字）迁入 `.portbuf`，RAMGS0 余量约 388 字，满足 S-05B-13 |
| 6 | `TXEMPTY` 语义只按头文件 bit6 推断 | §3.5 引 TRM 原文：`TXEMPTY=1` = "Transmitter buffer and shift registers are both empty"（`_pdf_txt/manual.txt:128106-128120`）；额外字符时间改为可关闭的工程裕度并独立表述 |
| 7 | `S-05B-01` "失败不改出参"与 `CLEAR_REMAIN` 冲突；"FIFO 满"被当成"溢出"；P7 未定义 | §5 修正：查询类才要求"失败不改出参"；溢出刺激改为"暂停读取后继续送入超过 16 字节"；阶段统一编号 P1~P6，注入点改名 I1~I5 |

### 0.2 已确认、不再变更

`bTaskCreateError` 占 bit5（0x0020）、`reserved` 10 位；三个驱动函数指针 `pfGetStatus`/`pfClearRxError`/`pfIsTxIdle` 及默认绑定、shim、桩测试同步；ISR 独占 FIFO 读取；整字 `ST1` 恢复；真实内存隔离；CAN-B 绑定在 `PlatformInit()`；采样任务保留；SCI-C 段环 32 槽。

### 0.3 v0.4 → v0.5（方案通过后的实施落地，2026-11-05）

| 项 | 落地位置 |
| --- | --- |
| 状态/阶段/门禁成组提交 | `SciPortCommit()`；进入 `HW_RESET` 前先提交再访问硬件；ISR 排空完成时在同一临界区写 `IDLE + NONE + bRxSuspend` |
| 自动恢复也做一次软件失效 | `SciPortSoftInvalidateInCritical()`，由 `bSwInvalidated` 保证只做一次 |
| 8 ms 只是判定阈值 | `SCI_PORT_WAIT_TX_LIMIT_MS`；时间源失败保持暂停并作废基准（`bWaitBaseValid = 0`），恢复后重建 |
| 故障计数只算硬件事件 | `SciPortEnterRecover(..., bHardwareEvent)`；纯软件 `SciPortResync()` 与 `SciPortRecoverRetry()` 不增 `u32RxErrEvents` |
| 损失计数绑定复位动作 | `SciPortTxResetQueue()` 只在真正执行复位前调用；预算不足时不记损失 |
| 文档与实现对齐 | `HW_SCI_CLEAR_*` 改为实现里的四态（见 §3.1）；`.portbuf` 实占 `0x180` 于 `0x00DC00` |

## 1. 意图与边界

### 1.1 做

| # | 内容 | 文件 |
| --- | --- | --- |
| 1 | 驱动入口：`Hw_SciGetRxStatus` / `Hw_SciGetTxFifoLevel` / `Hw_SciIsTxIdle` / `Hw_SciClearRxError` | `Drivers/inc/Hw_SciDriver.h`、`Drivers/src/Hw_SciDriver.c` |
| 2 | 端口层恢复状态机（6 状态 + 阶段持久化 + TX 门禁 + 故障证据） | `Platform/Port/Inc/SciPort.h`、`Platform/Port/Src/SciPort.c` |
| 3 | 真实临界区（整字 `ST1`） | `Platform/Port/Src/PortCritical.c`（新增） |
| 4 | 接线：`PlatformInit()` 绑定 CAN-B；心跳调搬运；`TaskCreate()` 追加 `PortTask` | `main.c` |
| 5 | `bPortInitError`（bit4）、`bTaskCreateError`（bit5） | `Platform/Status/Inc/BringupStatus.h` |
| 6 | `.portbuf` 真实预留 1K 字 + 3 路 SCI TX 环迁入 | `CMD/2837xS_FLASH_APP_CLA_lnk.cmd`、`Platform/Port/Src/SciPort.c` |

### 1.2 不做

不使能任何 SCI/CAN 中断；不做协议层；不改 `Hw_CanDriver`；不改采样层；不扩 CAN 接收对象。

## 2. 现状核对

| 位置 | 事实 |
| --- | --- |
| `Drivers/src/Hw_SciDriver.c:47` | `SCICCR.all = 7U`，固定 8N1 |
| `Drivers/src/Hw_SciDriver.c:55-61` | FIFO 使能，`SCICTL2.all = 0U`、`RXFFIENA = 0U` |
| `Drivers/src/Hw_SciDriver.c:74-86` | `Hw_SciTryReadByte()` 读 `SCIRXBUF.bit.SAR`；`Hw_SciTryWriteByte()` 以 `TXFFST >= 16` 判忙 |
| `Drivers/src/Hw_SciDriver.c:88-92` | `Hw_SciGetRxError()` 返回 `RXERROR \| RXFFOVF`，不可分类、无清除、失败返回 0 |
| `_pdf_txt/manual.txt:128106-128120` | `TXEMPTY`：`0` = buffer 或 shift register 或两者有数据；`1` = **buffer 与 shift register 均为空** |
| `device/headers/include/F2837xS_sci.h:185-191` | `RXFFIL:5`、`RXFFIENA`、`RXFFINTCLR`、`RXFFOVRCLR` |
| `device/driverlib/sci.h:158-172` | `SCI_FIFO_RX0`~`SCI_FIFO_RX16`，16 = 满 FIFO 阈值 |
| 22.6.2.LTS 实测汇编 | `__disable_interrupts()` = `PUSH ST1 / SETC INTM, DBGM / MOV AL,*--SP`；`__restore_interrupts(x)` = `POP ST1` |
| `Platform/Port/Inc/SciPort.h:90-97` | 未定义 `PORT_CRITICAL_HOOKS` 时临界区为空宏 |
| `Platform/Port/Src/SciPort.c` | `SciPortResync()` 在临界区内排空（≤32 次硬件读）；`SciPortServiceRx()` 每轮查错误但无清除 |
| `main.c:146-150` / `:222-228` | `TaskCreate()` 已有 `(void)Task_Start(AdcSampleTask, 1U, 0U);`；顺序 `PeripheralsInit → PlatformInit → ApplicationInit → TaskCreate → StartSystem` |
| `Platform/Schedule/Src/Schedule.c:13,34-48` | `TASK_MAX_NUM 8`；`Task_Start()` 成功 1、失败 0 |
| `Platform/Status/Inc/BringupStatus.h:17-26` | `unError` bit0~bit3 已用，`reserved:12` |
| 当前 `.map` | `RAMGS0` 0xFCC/0x1000（余 52 字）；`RAMGS1`~`RAMGS11` 全空；无 DMA |

## 3. 设计

### 3.1 驱动入口

```c
#define HW_SCI_RXSTS_FE      (0x0001U)
#define HW_SCI_RXSTS_PE      (0x0002U)
#define HW_SCI_RXSTS_OE      (0x0004U)
#define HW_SCI_RXSTS_BI      (0x0008U)
#define HW_SCI_RXSTS_RXFFOVF (0x0010U)
#define HW_SCI_RXSTS_UNKNOWN (0x8000U)   /* 仅旧包装在读取失败时返回 */

typedef struct
{
    Uint16 u16Status;       /* 上述位或 */
    Uint16 u16RxFifoLevel;
    Uint16 u16TxFifoLevel;
} ST_SCI_RX_STATUS;

Uint16 Hw_SciGetRxStatus(Uint16 u16SciId, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear); /* 1 成功 / 0 失败且不改出参 */
Uint16 Hw_SciGetTxFifoLevel(Uint16 u16SciId, Uint16 *pu16Level);
Uint16 Hw_SciGetRxFifoLevel(Uint16 u16SciId, Uint16 *pu16Level);
Uint16 Hw_SciGetTxIdle(Uint16 u16SciId, Uint16 *pu16Idle);   /* 1 查询成功 / 0 查询失败 */
Uint16 Hw_SciIsTxIdle(Uint16 u16SciId);                      /* 便捷包装；查询失败按“不空闲”处理 */

#define HW_SCI_CLEAR_OK         (0U)   /* 已清除，*pu16Remain = 复位前 FIFO 占用 */
#define HW_SCI_CLEAR_WAIT_TX    (1U)   /* 未执行：发送未空闲，*pu16Remain 有效但硬件未动 */
#define HW_SCI_CLEAR_STILL_ERR  (2U)   /* 已复位但仍报错，*pu16Remain = 回读状态 */
#define HW_SCI_CLEAR_UNREADABLE (3U)   /* 回读失败，*pu16Remain 无效 */

Uint16 Hw_SciClearRxError(Uint16 u16SciId, Uint16 *pu16Remain);
```

`Hw_SciGetRxError()` 保留签名：查询成功 → `(u16Status != 0)`；**查询失败 → `HW_SCI_RXSTS_UNKNOWN`**。

### 3.2 硬件错误恢复序列

无"读走即清"。实现顺序（`Hw_SciClearRxError()`）：

1. 记录 `RXFFST`（复位前 FIFO 占用，写入 `*pu16Remain`）；
2. `TXFFST != 0` 或 `TXEMPTY == 0` → 返回 `HW_SCI_CLEAR_WAIT_TX`（不动硬件）；
3. `RXFIFORESET = 0` → `1`，清 `RXFFOVRCLR` / `RXFFINTCLR`；
4. `RXENA = 0` → `SWRESET = 0` →（≥2 SYSCLK 由后续指令保证）→ `SWRESET = 1` → `RXENA = 1`、`TXENA = 1`；
5. 回读 `Hw_SciGetRxStatus()`：0 位 → `OK`，非 0 → `STILL_ERR`，调用失败 → `UNREADABLE`。

### 3.3 真实临界区

`Platform/Port/Inc/PortCritical.h` + `Src/PortCritical.c`（整文件包在 `#ifndef PORT_CRITICAL_HOOKS` 内，桩测试时为空）：

```c
ST_PORT_IRQ_STATE PortEnterCriticalReal(void)
{
    return (ST_PORT_IRQ_STATE)__disable_interrupts();   /* PUSH ST1; SETC INTM,DBGM */
}

void PortExitCriticalReal(ST_PORT_IRQ_STATE u16State)
{
    __restore_interrupts((uint16_t)u16State);           /* POP ST1，整字恢复 */
}
```

`SciPort.h` 在未定义 `PORT_CRITICAL_HOOKS` 时把 `PORT_ENTER_CRITICAL()/PORT_EXIT_CRITICAL(st)` 映射到这两个函数。

目标码反汇编（`dis2000 obj/Platform/Port/Src/PortCritical.obj`，2026-11-05）：

```
00000000        PortEnterCriticalReal:
00000000   7608   PUSH         ST1
00000001   3b30   SETC         INTM|DBGM
00000002   92be   MOV          AL, *--SP
00000003   0006   LRETR
00000004        PortExitCriticalReal:
00000004   fe02   ADDB         SP, #2
00000005   9641   MOV          *-SP[1], AL
00000006   96bd   MOV          *SP++, AL
00000007   7600   POP          ST1
00000008   fe82   SUBB         SP, #2
00000009   0006   LRETR
```

进入后 `INTM` 与 `DBGM` 均为 1；退出时整字恢复 `ST1`，因此"进入前已关中断 → 退出后仍关"与"ISR 内进入 → 不被提前开中断"都自动成立，无需深度计数器。

### 3.4 恢复状态机（6 状态 + 阶段持久化）

**铁律**：FIFO 数据只由 100 µs ISR 读；前台只碰状态寄存器，从不读 `SCIRXBUF`。

```c
#define SCI_RX_IDLE        (0U)  /* 正常收发 */
#define SCI_RX_SW_RESYNC   (1U)  /* 纯软件重同步后等丢弃；读空 → IDLE */
#define SCI_RX_RECOVER_REQ (2U)  /* 硬件错误待恢复；ISR 有界丢弃，读空不结束 */
#define SCI_RX_HW_RESET    (3U)  /* 前台持有硬件；ISR 完全不碰该端口寄存器 */
#define SCI_RX_DRAIN_ISR   (4U)  /* 硬件已清除，等排空；读空 → IDLE */
#define SCI_RX_PAUSED      (5U)  /* 未知/失败；只能显式重试 */
```

ISR 行为按状态：

| 状态 | ISR |
| --- | --- |
| `IDLE` | 读 + 发布 |
| `SW_RESYNC` / `DRAIN_ISR` | 只丢弃（≤配额）；读到空 → 临界区内置 `IDLE`、清 `bRxSuspend` |
| `RECOVER_REQ` | 只丢弃（≤配额）；**读到空也保持 `RECOVER_REQ`** |
| `HW_RESET` / `PAUSED` | 立即返回，不读任何 SCI 寄存器 |

阶段持久化（`u16RecPhase`）：

| 阶段 | 含义 | 下一步 |
| --- | --- | --- |
| `NONE 0` | 无进行中的恢复 | — |
| `CLASSIFY 1` | 需读状态分类 | → `WAIT_TX`（有错）或 `DRAIN_ISR`（无错） |
| `WAIT_TX 2` | 等硬件发送空闲 | → `RESET`（空闲或超时强制） |
| `RESET 3` | 已置 `HW_RESET`，**尚未执行清除** | → `RESET_DONE` |
| `RESET_DONE 4` | 清除已执行，结果已存 `u16ClearResult`/`u16Remain` | → `DRAIN_ISR`（OK）或 `PAUSED`（REMAIN/PARAM/UNKNOWN） |
| `DRAIN 5` | 等 ISR 排空 | → `NONE`（读空） |

**跨轮继续**：`SciPortRecover()` 每次进入先看 `u16RecPhase`，从上次停下的阶段继续：

- `RESET`（清除未执行）→ 本轮执行清除；预算不足时**保持 `RESET`**，下一轮继续；
- `RESET_DONE`（清除已执行）→ 只做结果判定，**绝不重复执行清除**；
- `WAIT_TX` → 继续等待（见 §3.5）。
- 预算：每个阶段开始前查同一轮预算（`PortBudgetHasLeft()`）；不够就返回 `BUSY` 并停在当前阶段。

`SciPortResync()` 的合并语义（不再无条件改状态）：

| 调用时的状态 | 行为 | 返回 |
| --- | --- | --- |
| `IDLE` | 清软件侧 → `SW_RESYNC`、`bRxSuspend=1`、置故障 | 1 |
| `SW_RESYNC` | 重新清软件侧，状态不变 | 1 |
| `RECOVER_REQ` / `DRAIN_ISR` | 只清软件侧（两环、序号、观察记录、`bTimeBaseValid`），**不改状态、不改阶段** | 1 |
| `HW_RESET` / `PAUSED` | **拒绝**：不清任何东西，不动状态 | 0 |

`PAUSED` 的出口只有 `Uint16 SciPortRecoverRetry(Uint16 u16PortId)`：前置条件 `state == PAUSED`；成功则回 `RECOVER_REQ` + `CLASSIFY`（要求时间源可用，否则返回 0 并保持 `PAUSED`）。普通恢复调用与确认路径都不能跨过 `PAUSED`。

### 3.5 TX 门禁、空闲判据与结果不确定

**空闲判据**：`TXFFST == 0 && TXEMPTY != 0`。依据 TRM（`_pdf_txt/manual.txt:128106-128120`）：`TXEMPTY=1` 的定义就是"transmit buffer 与 shift register **均为空**"，因此该判据本身已覆盖移位寄存器。另可开启"额外等待一个字符时间"作为**工程裕度**（默认关闭，19200 下 0.52 ms、115200 下 0.087 ms），若开启必须标注为裕度而非寄存器要求；FIFO 模式下的实测语义由 `S-05B-11` 核对。

**三态分离**（不再把查询失败当"忙"）：

| 查询结果 | 处理 |
| --- | --- |
| 成功且空闲 | 进入 `RESET` 阶段 |
| 成功且忙 | 继续等待；达到**实际经过时间**上限（默认 8 ms，`Hw_TimeBase` 差值）→ 强制复位路径 |
| 查询失败 | `u16ClearResult = UNKNOWN` → `PAUSED`，**不执行复位** |
| 时间源读取失败 | 无法计算期限 → 停留在 `WAIT_TX`、`u32TimeSrcFail++`、返回 `BUSY`，**不据此判超时** |

**门禁**：

- 进入 `WAIT_TX` 时置 `bTxSuspended = 1`：端口层停止向硬件提交新字节（已接受的字节自然发完）。
- `SciPortRecover()` 的准入**不看** `bTxSuspended`（那是它自己置的），只看 `u16RxState`/`u16RecPhase`。
- `SciPortTaskOne()` 的固定顺序：

```
1) SciPortRecover()        // 推进恢复状态机
2) 复查 bTxSuspended / bTxFrozen / u16RxState
3) 满足条件才运行普通 TX 服务
```

- 恢复完成后：正常路径解冻 TX；强制复位路径保持 `bTxFrozen=1`，等待协议层 `SciPortTxResume()`。

**结果不确定的单独锁存**：强制复位置 `bTxResetLoss=1`、`u32TxResetLossCount++`，并调用 `SciPortTxResetQueue()` 清空软件 TX 环、`u32TxResetDrop += 丢弃条数`；**不并入** `u16TxStall`（后者仍定义为"本轮零字节被接受"）。端口层不自动重发。

### 3.6 恢复接口的失败语义

| 情况 | 返回 | `*pu16Remain` | 状态/阶段 |
| --- | --- | --- | --- |
| 非法 ID / 空指针 / 未初始化 | `PARAM` | 不修改 | 不变 |
| 无待恢复事项（`IDLE`） | `IDLE_OK` | 不修改 | 不变 |
| 预算不足 / 等待 TX 中 | `BUSY` | 不修改 | 阶段保持 |
| 复位完成且全清 | `OK` | 写 0 | `DRAIN_ISR` |
| 复位已执行仍有错误 | `FAIL` | 写剩余位 | `PAUSED` |
| 状态读取失败 / 查询失败 | `UNKNOWN` | 不修改 | `PAUSED` |

- 分类证据只来自 `pfGetStatus()`；`u32RxErrEvents` 只在 `IDLE → 非 IDLE` 迁移时 +1。
- `bStreamBreak` 只有 `SciPortClearStreamBreak(id, gen)` 能清，且要求 `u16RxState == IDLE`（`SW_RESYNC`/`RECOVER_REQ`/`HW_RESET`/`DRAIN_ISR`/`PAUSED` 一律拒绝）且代次一致。

### 3.7 配置字段

`u16DataFormat`：预留，仅接受 0；`u16RxFifoLevel`：硬件支持 0~16，配置填 8；该阈值不决定轮询配额（`RXFFIENA = 0`）。

### 3.8 存储：真实预留 1K 字（单位修正）

C28x 链接器按**字**计：`0x400` = 1024 字 = 1K 字；`0x800` = 2048 字 = 2K 字。v0.3 写 0x800 是错的。

```
RAMGS1          : origin = 0x00D000, length = 0x000C00   /* 3072 字，.bss/.ebss 溢出池 */
RAMGS1_PORTBUF  : origin = 0x00DC00, length = 0x000400   /* 1024 字，.portbuf 专用 */
```

`.portbuf` 由 3 路 SCI TX 环实际占用：`128 × 3 = 384` 字（`0x0180`），余 `0x0280`（640 字）留给 05C 协议层。

实测 `.map`（2026-11-05）：`RAMGS1_PORTBUF 0000dc00 00000400 00000180 00000280`；
`.portbuf 1 0000dc00 00000180 UNINITIALIZED`（`SciPort.obj (.portbuf)`，load 0x0008AA00 零初始化、run 0x00DC00）；
`RAMGS1 0000d000 00000c00 00000000 00000c00`（`.bss` 全部落在 `RAMGS0`，未侵占预留区）。

RAMGS0 账（本步结束前以 `.map` 为准）：

| 项 | 字数 |
| --- | --- |
| 当前余量 | 52 |
| 新增恢复状态（每端口约 16 字 × 3） | −48 |
| TX 环迁出 RAMGS0 | +384 |
| **预计余量** | **约 388** |

验收要求余量 ≥32 字，预计 388 满足；若实测低于该值，迁移对象明确为 **CAN 队列缓冲（`CanPort.c` 的 2×16 帧 TX 环，384 字）迁入 `.portbuf`**，不是下调验收数字。

### 3.9 执行上下文与预算

| 上下文 | 频率 | 内容 | 预算（待测目标） |
| --- | --- | --- | --- |
| `Isr_Heartbeat` | 100 µs | 三路接收搬运 + 错误检测 + 排空 | 单路 ≤0.4 µs、三路 ≤1.5 µs、整 ISR ≤10 µs |
| `PortTask` | 1 ms | SCI TX、CAN 收发、`SciPortRecover()` | 轮次预算 20 µs，阶段级检查 |
| `SciPortResync()` | 事件驱动 | 纯软件清状态 | 关中断窗口 ≤2 µs（待测） |

轮询方案依据：单一搬运入口、有界配额、可实测（不引用"中断不可预算"）。

## 4. 修改前后

| 文件 | 动作 | 说明 |
| --- | --- | --- |
| `Drivers/inc/Hw_SciDriver.h` | 改 | 错误位、`ST_SCI_RX_STATUS`、四个新 API、配置两字段 |
| `Drivers/src/Hw_SciDriver.c` | 改 | 新入口、初始化范围检查、旧包装转发 |
| `Broad/src/DrvCfg_EP15K.c` | 改 | 三行 SCI 配置补 `0U, 8U` |
| `Platform/Port/Inc/SciPort.h` | 改 | 6 状态/6 阶段枚举、恢复 API（`Recover`/`RecoverRetry`/`RxState`/`RecPhase`/`TxResume`/`TxResetQueue`）、驱动表增三指针 |
| `Platform/Port/Src/SciPort.c` | 改 | 状态机、唯一读取者、`Resync` 纯软件化与合并语义、阶段持久化、TX 门禁；TX 环加 `#pragma DATA_SECTION(".portbuf")` |
| `Platform/Port/Inc/PortCritical.h` | 新增 | `ST_PORT_IRQ_STATE` 与两个真实临界区入口声明 |
| `Platform/Port/Src/PortCritical.c` | 新增 | 整字 `ST1`（`__disable_interrupts` / `__restore_interrupts`），主机桩构建下为空 |
| `Platform/Port/Src/PortTask.c` | 改 | `SciPortTaskOne()` 按 §3.5 的三步顺序 |
| `Platform/Port/Test/*` | 改 | 驱动表三指针、shim、§5.2 全部新场景、§5.3 变更映射 |
| `main.c` | 改 | `PlatformInit()` 绑定 CAN-B；心跳搬运；`TaskCreate()` 两处 `Task_Start` 均检查 |
| `Platform/Status/Inc/BringupStatus.h` | 改 | `bPortInitError:1`（bit4，端口绑定失败）、`bTaskCreateError:1`（bit5，任务注册失败），`reserved:10` |
| `CMD/2837xS_FLASH_APP_CLA_lnk.cmd` | 改 | 拆 `RAMGS1` / `RAMGS1_PORTBUF`（0x0C00 / 0x0400），新增 `.portbuf` |
| `tools/build_minimal.py` | 改 | `SOURCES` 增加 `PortCritical.c` |

```c
void TaskCreate(void)
{
    if (Task_Start(AdcSampleTask, 1U, 0U) == 0U)
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }
    if ((stBringupStatus.unError.Bits.bPortInitError == 0U) &&
        (Task_Start(PortTask, 1U, 0U) == 0U))
    {
        stBringupStatus.unError.Bits.bTaskCreateError = 1U;
    }
}
```

## 5. 验收

### 5.1 静态（本机）

| ID | 判据 |
| --- | --- |
| `S-05B-01` | **查询类**入口（`GetRxStatus`/`GetTxFifoLevel`/`IsTxIdle`）失败时返回 0 且不修改出参；`Hw_SciGetRxError()` 失败返回 `UNKNOWN`（非 0）。（`Hw_SciClearRxError` 的 `REMAIN` 会写 `*pu16Remain`，见 `S-05B-02`） |
| `S-05B-02` | `Hw_SciClearRxError()` 四态与出参有效性符合 §3.1；`RXFFINTCLR` 不当成清错误；`u16RxFifoLevel<=16`、`u16DataFormat==0` 检查生效 |
| `S-05B-03` | 唯一读取者：`HW_RESET`/`PAUSED` 下 ISR 对该端口的 `pfTryRead`/`pfGetStatus` 调用次数为 0；`RECOVER_REQ` 读空后状态不变；互斥标记从不重入 |
| `S-05B-04` | 临界区四场景（进入前开/关中断、双层嵌套、ISR 内），断言退出后 `ST1` 逐位等于进入前；目标 `PortCritical.obj` 含 `PUSH ST1`/`POP ST1` |
| `S-05B-05` | 接线：`CanPortInit()` 在绑定之前；`TaskCreate()` 两处检查返回值；`bPortInitError` 非 0 时心跳不搬运、`PortTask` 不注册 |
| `S-05B-06` | 构建零 warning；`.map` 中 `RAMGS1_PORTBUF` length `0x0400`、`.portbuf` 占用 `0x0180`、`RAMGS0` 余量 `0x17E`（382 字）≥32；`RAMGS1` 用量 0（`.bss` 未进入预留区） |
| `S-05B-14` | 状态一致性：`SciPortServiceRx()` 在 `HW_RESET`/`PAUSED` 下对该端口的驱动调用次数为 0；`SciPortTxResume()` 仅在 `state == IDLE && bTxResetLoss` 时返回 1 |

### 5.2 桩测试场景（主机可跑，跨轮）

| 场景 | 要求 |
| --- | --- |
| 跨轮 TX 等待（先忙后闲） | 第 1 轮 `IsTxIdle=0`（成功查询）→ 返回 `BUSY`、`bTxSuspended=1`；第 2 轮空闲 → 推进到 `RESET`；两轮之间 TX 提交次数为 0 |
| 跨轮 TX 等待（持续忙至超时） | 按时间源推进到 8 ms → 强制复位路径、`bTxResetLoss=1`、TX 环清空 |
| TX 查询失败 | `IsTxIdle` 返回 0（查询失败）→ `UNKNOWN`/`PAUSED`，**不执行复位**，不按超时处理 |
| 时间源失败 | 时间源读取失败 → 保持 `WAIT_TX`、`BUSY`、`u32TimeSrcFail++`，不判超时 |
| P5 后预算耗尽（I4） | `HW_RESET` 已置、清除未执行、预算耗尽 → 下一轮继续执行 P6 并成功 |
| P6 后暂停（I5） | 清除已执行、结果已存 → 后续阶段因预算暂停，下一轮**不重复执行清除**（桩统计 `Hw_SciClearRxError` 调用次数恒为 1） |
| `PAUSED` 重试 | 显式 `SciPortRecoverRetry()` 成功回 `RECOVER_REQ`；普通 `SciPortRecover()` 仍返回 `BUSY`/`UNKNOWN` 且不改状态；时间源不可用时重试失败 |
| 恢复各阶段注入心跳 | I1~I5 各插一次 `SciPortService100us()`：`HW_RESET` 期间 0 次硬件访问、无双上下文读 |
| 返回码矩阵 | 六种情况逐条断言返回码与 `*pu16Remain` 是否被改写 |
| 事件计数 | 同一粘滞错误连续 10 轮只增 1 次 `u32RxErrEvents` |
| 代次与确认 | 非 `IDLE` 状态下确认一律返回 0；排空中、`PAUSED` 中均不得确认 |

注入点定义：**I1** `CLASSIFY` 后、**I2** `WAIT_TX` 等待中、**I3** 进入 `HW_RESET` 后（清除前）、**I4** 清除执行前、**I5** 清除执行后。

### 5.3 05A 用例变更映射

| 原用例 | 变更 | 保留的保证 |
| --- | --- | --- |
| `S-05A-23`（重同步跨轮续丢） | 排空改由 ISR 执行；"一次性丢 32 字节"→"每次心跳丢 ≤配额" | 配额上限、读空才结束、不加大关中断循环 |
| `S-05A-27`（排空期间禁止确认） | 追加 `PAUSED`/`HW_RESET`/`RECOVER_REQ` 也不得确认 | 代次一致才清除 |
| `S-05A-06`（部分发送） | TX 服务增加 `bTxSuspended`/`bTxFrozen` 前置 | 字节顺序、不重复 |
| 其余用例 | 不改 | 字节环/段环拒收、空读观察、tick=0、覆盖锁存、RX 冻结、队头裁定 |

### 5.4 上板

| ID | 判据 |
| --- | --- |
| `S-05B-07` | 三路各 10 min 递增字节流：序号连续、无重无乱 |
| `S-05B-08` | 搬运耗时四态（空闲/积压/错误/恢复），GPIO33 打点，对照 0.4/1.5 µs |
| `S-05B-09` | 同窗口 `u32Tick100us +10000`、`u32ControlIsrCount +20000`、`u16ControlStallMs` 保持 0；ISR 最坏时长与 20 kHz 响应延迟用时间戳/GPIO 实测 |
| `S-05B-10` | 临界区 `ST1` 快照：进入后 `INTM=1 && DBGM=1`、**退出后与进入前逐位相同**；GPIO 只标顺序 |
| `S-05B-11` | 错误恢复三种**可重复**刺激：①暂停读取后继续送入 >16 字节造成 FIFO **溢出**（"满"不等于"溢出"）②发送端错误停止位/错误波特率（帧错）③持续拉低 RX（BREAK）；记录分类、恢复时长、丢弃量、确认间隔；同时核对 `TXEMPTY` 在 FIFO 模式下的实测语义 |
| `S-05B-12` | `SciPortResync()` 关中断窗口 ≤2 µs（GPIO33 高低电平）；记录整轮恢复总时长与各阶段耗时 |
| `S-05B-13` | `.map`：`RAMGS1_PORTBUF` 0x0400/占用 0x0180、`.portbuf` run addr 0x00DC00；`RAMGS0` 用 0xE82、余 0x17E；`CLA1_MSGRAM*`（4/9 字）、`RAMLS5`（2 字）不变 |

## 6. 风险与待裁定

1. `SWRESET` 清 `RXERROR` 需上板确认；若还需重新置 `RXENA`，按实测补充步骤。
2. 强制复位丢帧由协议层重发；`WAIT_TX` 最长 8 ms，协议超时窗口须大于该值。
3. `bTaskCreateError` 占 bit5 已确认；外部协议兼容性由项目方另行确认（本步只保证本文档内无冲突）。
4. `.portbuf` 1K 字中 384 字已被 TX 环占用，05C 可用 640 字；超出需重新划段。
5. `TXEMPTY` 在 FIFO 模式下的实测语义以 `S-05B-11` 为准；工程裕度默认关闭。

## 7. 实施记录（v0.5 落地，2026-11-05）

### 7.1 改动文件

| 文件 | 动作 | 行数/要点 |
| --- | --- | --- |
| `Drivers/inc/Hw_SciDriver.h` | 重写 | 0.5 版；状态位、`ST_SCI_RX_STATUS`、清除码四态、四个新入口、配置两字段 |
| `Drivers/src/Hw_SciDriver.c` | 重写 | `Hw_SciGetRxStatus`（`BRKDT` 而非 `BI`）、`Hw_SciClearRxError`（WAIT_TX 前判）、`Hw_SciGetTxIdle`、`Hw_SciGetTxFifoLevel/RxFifoLevel` |
| `Platform/Port/Inc/SciPort.h` | 重写 | 0.2 版；6 状态 / 5 阶段 / 6 返回码 / 出参标志；驱动表改 6 指针 |
| `Platform/Port/Src/SciPort.c` | 重写 | 约 1,100 行；状态机 + 阶段持久化 + 成组提交 + TX 门禁 + `.portbuf` |
| `Platform/Port/Inc/PortCritical.h` | 新增 | 真实临界区声明 |
| `Platform/Port/Src/PortCritical.c` | 新增 | `__disable_interrupts` / `__restore_interrupts` |
| `Platform/Port/Src/PortTask.c` | 改 | 恢复优先：`RECOVER_REQ`/`HW_RESET` 先 `SciPortRecover()` 再 `SciPortTaskOne()` |
| `Platform/Status/Inc/BringupStatus.h` | 改 | bit4 `bPortInitError`、bit5 `bTaskCreateError`、`reserved:10` |
| `main.c` | 改 | 心跳内 `SciPortService100us()`；`PlatformInit()` 绑定 CAN-B；`TaskCreate()` 两处 `Task_Start` 检查 |
| `Broad/src/DrvCfg_EP15K.c` | 改 | 三行 SCI 配置补 `0U, 8U` |
| `CMD/2837xS_FLASH_APP_CLA_lnk.cmd` | 改 | `RAMGS1` 0x0C00 + `RAMGS1_PORTBUF` 0x0400；`.portbuf` 放入预留区 |
| `tools/build_minimal.py` | 改 | `SOURCES` 增 `PortCritical.c` |
| `Platform/Port/Test/port_stub_test.c` | 改 | 0.5 版；新驱动桩、9 个 05B 用例（`S-05B-01`~`09`）、`S-05A-24` 改用软件重同步 |
| `Platform/Port/Test/shim/*` | 改/新增 | `Hw_SciDriver.h` 补新入口与类型；新增 `PortCritical.h` |

### 7.2 构建

| 项 | 值 |
| --- | --- |
| 命令 | `python tools/build_minimal.py --cgt D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS` |
| 结果 | 零 warning、零 error、链接成功 |
| `EP15K_minimal.out` | 775,148 B，sha256 `78112ce230b5d7451495323b1474760e42b9f983c229514c84924157a6eb4c8d` |
| `EP15K_minimal.map` | 99,074 B，sha256 `6defff08bd02b7494389faa466cc2bc7df3da9b8a9e552856463d5bf60cbefc8` |

### 7.3 静态核对

| 判据 | 结果 |
| --- | --- |
| `S-05B-01` 查询失败不改出参 | `Hw_SciGetRxStatus`/`GetTxIdle` 失败分支直接 `return 0U`，写出参只发生在成功路径 |
| `S-05B-02` 四态与出参 | `HW_SCI_CLEAR_OK/WAIT_TX/STILL_ERR/UNREADABLE` 依次由 §3.2 的 5 步产生 |
| `S-05B-03` 唯一读取者 | `SciPortServiceRx()` 在 `HW_RESET`/`PAUSED` 分支直接 `return 0U`（无任何 `pstDrv` 调用）；排空只在心跳里做 |
| `S-05B-04` 临界区 | 反汇编见 §3.3（`PUSH ST1` / `POP ST1`），`PortCritical.obj .text` 0xA 字 |
| `S-05B-05` 接线 | `main.c:137` 绑定 CAN-B（失败置 bit4）；`main.c:160` 注册 `PortTask`（失败置 bit5） |
| `S-05B-06` map | `RAMGS1_PORTBUF` 0x0400/占 0x180/余 0x280；`RAMGS0` 用 0xE82/余 0x17E；`CLA1_MSGRAMLOW/HIGH` 仍 4/9 字、`RAMLS5` 仍 2 字 |
| `S-05B-14` 状态一致性 | `SciPortTxResume()` 条件 `(state == IDLE) && (bTxResetLoss != 0)` |

### 7.4 偏离

1. **清除码取值**：申请写 `OK=1/REMAIN=0/PARAM=2/UNKNOWN=3`，实现改为 `OK=0/WAIT_TX=1/STILL_ERR=2/UNREADABLE=3`——把"等发送"从"参数错误"里拆出来，参数错误改由端口层返回 `SCI_RECOVER_PARAM`，避免驱动层混淆"未执行"和"参数非法"。
2. **`Hw_SciGetRxStatus()` 增 `bClear` 参数**：申请为两参；实现加第三参用于"读后清状态位"，`Hw_SciClearRxError()` 走 `RXFIFORESET + SWRESET` 复位路径，二者分工明确。
3. **`Hw_SciIsTxIdle()` 从两参改为便捷包装**：真实空闲判据是 `Hw_SciGetTxIdle(id, &idle)`（返回查询是否成功），端口驱动表的 `pfIsTxIdle` 也用它；`Hw_SciIsTxIdle()` 只供应用层快速判读。
4. **`SCIRXST` 位名**：申请写 `BI`，实际寄存器字段名是 `BRKDT`（`error #137` 后改正）。
5. **`.portbuf` 落点**：申请写"`.bss` 溢出池 0x0C00 + 预留 0x0400"，实现按此拆分后 `.portbuf` 落在 `0x00DC00`（v0.4 预估为 `0x00D000`），占用不变仍 `0x180`。
6. **`bTaskCreateError` 落地**：申请已确认 bit5；实现中 `TaskCreate()` 的两次 `Task_Start` 失败都置该位，`bPortInitError` 只保留给 `CanPortSetTxFrameType()` 绑定失败。

### 7.5 未跑项

- 主机桩测试：本机**无 C 主机编译器**（`gcc`/`clang`/`cc`/`tcc`/`zig` 均不存在，WSL 无发行版），只做了目标工具链的 `--compile_only` 校验（`port_stub_test.c`、`SciPort.c`、`CanPort.c`、`PortTask.c`、`PortCritical.c` 五单元零 warning）。执行留给审阅方 `/usr/bin/cc` 环境。
- 上板项 `S-05B-07`~`13` 全部未测。

## 8. 审阅结论（由 Codex 填写）

# 步骤 05B：SCI 驱动补全、错误恢复与端口接线（v0.9：代码、主机桩与 CCS Debug 构建均通过，上板验收待完成）

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

### 0.4 v0.5 → v0.6（第二轮代码复审返工，2026-11-06）

| 审阅意见（m03507） | 落地位置 | 用例 |
| --- | --- | --- |
| 1. 持续忙时无法执行约定的强制复位 | `Hw_SciClearRxError()` 增 `bForce`；端口层"超阈值只存强制意图"（§3.2、§3.5） | `S-05B-15`、`S-05B-16` |
| 2. `bSwInvalidated` 未随一次恢复结束清零 | 排空完成提交块内清零（§3.4） | `S-05B-18`、`S-05B-19` |
| 3. TX 挂起双向错误（排空完成仍冻结 / 逐字节门禁缺失） | 排空完成按"是否有强制复位损失"决定解冻；`SciPortServiceTx()` 整段临界区（§3.5） | `S-05B-15`、`S-05B-20` |
| 4. 初始化失败后任务仍注册、心跳仍搬运 | `PortTaskRegister()` / `PortTaskServiceFromHeartbeat()`；`main.c` 两处门禁（§3.9） | `S-05B-21` |
| 5. 恢复阶段与出参契约不一致 | `RESET_DONE` 增预算门禁；出参只报剩余错误位；`RecoverRetry()` 先查时间源；`Resync()` 在 `PAUSED`/`HW_RESET` 直接返回（§3.4、§3.6） | `S-05B-16`、`S-05B-17`、`S-05B-22` |
| 6. `Hw_SciInit()` 逐项写硬件 | 改为两遍：先校验整表、再写寄存器（§3.7） | `S-05B-02` |
| 7. 8 条测试预期按"前台立即丢 32 字节"编写 | 改为"重同步本身不读硬件、每轮丢一个接收配额 8 字节、读空才结束"（§5.3） | `S-05A-23`、`S-05A-27` |
| 8. `SciPortPopBatch()` 返回值写错 | 返回实际读出字节数（§5.3） | `S-05B-07` |

### 0.5 v0.6 → v0.7（第三轮代码复审返工，2026-11-06）

| # | 审阅发现 | 本章位置 | 用例 |
| --- | --- | --- | --- |
| 1 | 强制复位只切 `RXFIFORESET` + `SWRESET`，没有清 TX FIFO，已进硬件的旧字节仍会发出 | §3.2 步骤 0/步骤 4 | `S-05B-03`、`S-05B-15` |
| 2 | RESET 阶段把持久的 `bTxResetLoss` 当驱动 `bForce` 实参，第二次故障凭空多一次强制复位 | §3.4.3 | `S-05B-24` |
| 3 | 调度入口取时失败时端口里的 WAIT_TX 旧基准仍有效，恢复后立即越阈值 | §3.4.4 | `S-05B-26` |
| 4 | `BUSY`/`UNKNOWN` 仍改写出参；头文件把 `pu16Remain` 注释成 FIFO 占用 | §3.6 | `S-05B-23` |
| 5 | 普通复位成功即解冻，排空完成前就能提交新字节 | §3.5 门禁 | `S-05B-25` |
| 6 | 12 条测试预期沿用旧排空节奏与布尔式返回值 | §5.6 | `S-05A-27`、`S-05B-03/15/16/18/19/20` |

### 0.6 v0.7 → v0.8（第四轮代码复审返工，2026-11-06）

| 复审发现 | 本轮处理 | 位置 |
| --- | --- | --- |
| 复位后回读 `TXFFST` 仍非零也返回 `WAIT_TX`，端口退回等待阶段，8 ms 后自动再来一次强制复位 | 新增 `HW_SCI_CLEAR_TX_DIRTY (4U)`，与"硬件尚未动作"分开；端口收到即 `SciPortPause()` 并返回 `FAIL`，没有显式重试不会重来 | §3.1、`Hw_SciDriver.c` 步骤 4、§3.4 `RESET_DONE` |
| 同一次调用完成 `RESET` + `RESET_DONE` 时，调用方拿到进入函数时的旧 `u16RecRemain`（实测 `OK` + `0x0010`） | 出参一律由本次已保存的结果生成：`OK`/`IDLE_OK` 写 0、`FAIL` 写当前 `u16RecRemain`、`BUSY`/`UNKNOWN` 保持哨兵 | §3.6、`SciPortRecover()` |
| 驱动在 `WAIT_TX`/`UNREADABLE`/`TX_DIRTY` 之前就写了 `*pu16Remain = 0`，与"失败不写出参"矛盾 | 删除提前写入；只有 `OK`/`STILL_ERR` 分支写出参 | §3.1 |
| `S-05B-19` 初始发布处少一次心跳，实际断言总数为 314 | 补一次 `RxRound()`；README 与本文断言数统一按实跑口径填写 | §5.7 |

### 0.7 v0.8 → v0.9（第五轮代码复审返工，2026-11-06）

| 复审发现 | 本轮处理 | 位置 |
| --- | --- | --- |
| `SciPortRecoverRetry()` 把 `u16ClearResult` 改回 `OK`，随后 `CLASSIFY` 只凭 RX 错误位为 0 就走排空，**TX FIFO 残留被绕过**（实测：重试后回 `IDLE`、`SciPortTxResume()` 返回 1、清除次数仍 1） | 重试入口保留失败原因；新增 `SCI_REC_PHASE_VERIFY_TX (5)`：进入恢复前先查 `pfIsTxIdle()`，仍非空或查询失败都 `SciPortPause()` 并保持 TX 冻结，只有确认清空才转 `CLASSIFY` | §3.4、`SciPort.c` `VERIFY_TX` 阶段 |
| 原包实跑 339 项、11 项失败（`S-05B-27` 6 条、`S-05B-28` 5 条），全部是刺激条件不足 | `S-05B-27` 补 `s_u16TxIdle = 0` 与一次建基准调用；`S-05B-28` 两条路径各补一次建基准，跨轮路径先 `PortBudgetBegin()` 开窗；断言未改 | §5.8 |

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

#define HW_SCI_CLEAR_OK         (0U)   /* 已清除，*pu16Remain = 复位后的剩余错误位（= 0） */
#define HW_SCI_CLEAR_WAIT_TX    (1U)   /* 未执行：发送未空闲，*pu16Remain 未被写 */
#define HW_SCI_CLEAR_STILL_ERR  (2U)   /* 已复位但仍报错，*pu16Remain = 回读的剩余错误位 */
#define HW_SCI_CLEAR_UNREADABLE (3U)   /* 回读失败，*pu16Remain 未被写 */
#define HW_SCI_CLEAR_TX_DIRTY   (4U)   /* 复位已执行，但回读 TXFFST 仍非零：硬件未接受 */

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

**强制路径的 TX FIFO 处理（v0.7）**：TRM 明确 `SCICTL1.SWRESET` 不会清 TX FIFO 的指针与内容，因此

- 步骤 0（新增，仅 `bForce != 0`）：`SCIFFTX.TXFIFORESET = 0` → `= 1`，再给 `TXFFINTCLR` 一个脉冲；
- 步骤 4（新增，仅 `bForce != 0`）：回读 `SCIFFTX.TXFFST`，非 0 说明硬件没有接受这次复位，返回 `WAIT_TX` 而不报 `OK`。

已经移出移位寄存器的字节仍按"结果不确定"处理：驱动只保证 FIFO 里不再有残留，端口层照常锁存 `bTxResetLoss` 与丢弃条数。

复位后的验证失败与"还没轮到复位"必须分开：

- `HW_SCI_CLEAR_WAIT_TX`：**硬件未动**，属"稍后再试"，端口退回等待阶段合理。
- `HW_SCI_CLEAR_TX_DIRTY`：复位已经执行，回读发现 TX FIFO 仍非空，残留字节仍可能上线。
  这不是可重试的等待：端口保存该结果、进入 `PAUSED`，何时再试由 `SciPortRecoverRetry()` 决定。
  否则每 8 ms 会自己再触发一次强制复位（复审已用独立桩复现：强制次数 1 → 2，中间没有显式重试）。

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
| `VERIFY_TX 6` | 上一次暂停原因是 `TX_DIRTY`：显式重试时先核验硬件 TX 是否真的空了 | 空闲 → `CLASSIFY`；仍非空/查询失败 → `PAUSED` |

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

`PAUSED` 的出口只有 `Uint16 SciPortRecoverRetry(Uint16 u16PortId)`：前置条件 `state == PAUSED`（要求时间源可用，否则返回 0 并保持 `PAUSED`）。重试失败不会被抹掉：若上一次进入 `PAUSED` 的原因是 `HW_SCI_CLEAR_TX_DIRTY`，阶段置为 `VERIFY_TX`，下一次推进必须先调用 `pfIsTxIdle()` 复核硬件 TX：仍非空返回 `FAIL`、查询失败返回 `UNKNOWN`，两种情况都**保持 `PAUSED` 与 TX 冻结**；确认清空后才清零 `u16ClearResult` 并转入 `CLASSIFY`。普通恢复调用与确认路径都不能跨过 `PAUSED`。

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

- 恢复完成后：**由心跳在提交 `IDLE` + `NONE` 的那一段临界区里解除临时挂起**（正常路径）；强制复位路径保持 `bTxFrozen=1`，等待协议层 `SciPortTxResume()`。复位结果处理（`RESET_DONE` 返回 `OK`）只把状态切到 `DRAIN_ISR`，**不提前解冻**——否则复位成功、排空未完成的那几轮心跳会立刻补发新字节，与"作废"语义冲突。

**结果不确定的单独锁存**：强制复位置 `bTxResetLoss=1`、`u32TxResetLossCount++`，并调用 `SciPortTxResetQueue()` 清空软件 TX 环、`u32TxResetDrop += 丢弃条数`；**不并入** `u16TxStall`（后者仍定义为"本轮零字节被接受"）。端口层不自动重发。

### 3.6 恢复接口的失败语义

| 情况 | 返回 | `*pu16Remain` | 状态/阶段 |
| --- | --- | --- | --- |
| 非法 ID / 空指针 / 未初始化 | `PARAM` | 不修改 | 不变 |
| 无待恢复事项（`IDLE`） | `IDLE_OK` | 写 0（有效的"无剩余错误"报告） | 不变 |
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
| `S-05B-22` | 出参哨兵：`SciPortRecover()` 在参数非法/驱动不可用时返回 `PARAM` 且**不写出参**（用 `0xFFFF` 哨兵验证）；`IDLE` 时返回 `IDLE_OK` 并写出参 0（表示无剩余错误位） |

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
| `S-05A-23`/`S-05A-27` 断言 | `Resync()` 返回后读取位置保持 16；第一轮心跳后 24；剩余 64 字节需 8 轮；第 9 轮读到空才结束 | 每轮一个接收配额、读空才结束 |
| `S-05B-07` 断言 | `SciPortPopBatch()` 读出两字节时返回 2（不是 1） | 返回值是字节数 |
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

### 5.5 第二轮返工新增用例（`S-05B-15` ~ `S-05B-21`）

| ID | 场景 | 断言 |
| --- | --- | --- |
| `S-05B-15` | 发送持续忙，等待超过 8 ms | 第一次推进只提交 `HW_RESET`+`RESET`，`SciPortForceResets()==0`、`SciPortTxResetDrop()==0`、TX 环未动；下一次推进才清队列、记 3 字节损失、`ForceResets()==1`、驱动收到 `bForce==1`；排空回 `IDLE` 后仍冻结，`SciPortTxResume()==1` 才解冻 |
| `S-05B-16` | 强制复位执行前预算耗尽 | 停在 `RESET` 阶段：`ForceResets()==0`、`TxResetDrop()==0`、TX 环保留；预算恢复后下一轮执行一次、损失记一次 |
| `S-05B-17` | 强制复位后回读不可读 | 进入 `PAUSED`、损失只记一次；`PAUSED` 下 `Resync()` 不改状态与代次；时间源不可用时 `SciPortRecoverRetry()` 返回 `UNKNOWN` 且仍驻留 `PAUSED` |
| `S-05B-18` | 连续两次独立硬件故障 | 第二次故障同样使故障前发布的批次失效（`SciPortRxUsed()==0`） |
| `S-05B-19` | 软件重同步后再发生硬件故障 | 先 `Resync()` 丢弃旧批次，恢复后再故障仍把新批次置为失效 |
| `S-05B-20` | 挂起期间逐字节门禁 | 挂起时 `SciPortTaskOne()` 写入 0 字节；正常排空完成后门禁解除、同一批数据才写出 |
| `S-05B-21` | 初始化失败门禁 | `PortTaskServiceFromHeartbeat(1)` 时服务调用计数不变；`PortTaskRegister()` 返回值透传 `Task_Start()` 结果 |

### 5.6 第三轮返工新增用例（`S-05B-23` ~ `S-05B-26`）

| 用例 | 场景 | 断言要点 |
| --- | --- | --- |
| `S-05B-23` | 阶段间不得改写出参 | 预置 `0xCAFE`：`CLASSIFY` 返回 `BUSY`、预算截断、`pfIsTxIdle` 查询失败返回 `UNKNOWN`，三次之后出参仍为 `0xCAFE` |
| `S-05B-24` | 损失锁存不得复用为强制指令 | 第一次强制复位后保持冻结；第二次独立故障在 TX 空闲时执行，`s_au16ClearForce == 0`、`SciPortForceResets` 仍为 1 |
| `S-05B-25` | 复位成功、排空未完成 | `SciPortTaskOne()` 提交次数为 0、`IsTxSuspended != 0`；排空到 `IDLE` 后同一调用才写入 2 字节 |
| `S-05B-26` | 调度入口取时失败 | 真实 `PortTask()` 入口失败后 `SciPortForceResets == 0`；恢复后首次 `SciPortRecover()` 只重建基准（`BUSY`），再满一个新的 8 ms 才落强制意图、下一轮才执行 |

### 5.7 第四轮返工新增用例（`S-05B-27` ~ `S-05B-28`）

| 用例 | 场景 | 判据 |
| --- | --- | --- |
| `S-05B-27` | 复位回读 `TXFFST` 仍非零 | `SciPortRecover()` 返回 `FAIL`、状态 `PAUSED`、`SciPortForceResets()` 仍为 1；再推进 30 ms 仍 `FAIL`、驱动清除次数不增、强制次数不增；只有 `SciPortRecoverRetry()` 能回到 `RECOVER_REQ` |
| `S-05B-28` | 同一故障的两种完成路径 | 路径 A（一次调用走完 `RESET`+`RESET_DONE`）返回 `OK` 且出参 `0`；路径 B（`RESET` 后预算判定失败、下一轮完成）中途 `BUSY` 保持哨兵 `0xCAFE`，完成后同样 `OK` 且出参 `0` |

### 5.8 第五轮返工新增用例（`S-05B-29`）

| 用例 | 场景 | 判据 |
| --- | --- | --- |
| `S-05B-29` | `TX_DIRTY` 之后的显式重试 | 首次恢复 `FAIL`/`PAUSED`，`SciPortTxResume()==0`；第一次重试时 RX 错误位为 0、`TXFFST` 仍为 1 → 推进返回 `FAIL`、仍 `PAUSED`、仍冻结、驱动清除次数不增；第二次重试时 `TXFFST` 为 0 → 返回 `OK` 并排空回 `IDLE` |

同轮补正：`S-05B-27` 需要同时设置"清除入口忙"与"空闲查询忙"两个桩条件，并在推进时间前单独调用一次恢复建立等待基准；`S-05B-28` 的两条路径同样需要一次建基准调用，跨轮路径还须先用 `PortBudgetBegin()` 开窗，否则预算判定直接放行、注入的取时失败不会发生。

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
| `EP15K_minimal.out` | 780,428 B，sha256 `125227201dc5f85b8666d76610de99adf149e30680a16f4ae019e5c5dcdb9a94` |
| `EP15K_minimal.map` | 99,780 B（**sha256 不作为校验值**：map 内含链接时间戳，同一份源码连续两次链接得到不同哈希，v0.6 归档差异即由此产生） |
| 备注 | v0.5 为 775,148 B / 99,074 B；v0.6 为 779,132 B / 99,670 B；v0.7 新增 TX FIFO 复位与等待基准作废；v0.8 区分复位后回读失败并调整出参来源；v0.9 增加 `VERIFY_TX` 阶段，`.out` 780,428 B / `.map` 99,780 B |
| CCS Debug 构建（2026-11-06） | 全部 **37 个编译单元**、零 warning、链接成功，生成 `Debug/EP15K_2026.out`（758,760 B）与 `Debug/EP15K_2026.hex`；`Debug/EP15K_2026.map` 99,303 B 中 `RAMGS0` 用量 `0xE90`／余 `0x170`（368 字）、`.portbuf` run `0x00DC00`／`0x180`，与上表的 `build_minimal.py` 结果一致；两份产物的**字节数不同**（758,760 B 与 780,428 B），差异来源尚未逐项比对编译/链接参数后确认；目前可核实的是一致性——两份 map 的 `RAMGS0` 用量与余量（`0xE90`／`0x170`）以及 `.portbuf` 的 run 地址与长度（`0x00DC00`／`0x180`）完全相同 |
| CCS 工程配置（本步新增，`.cproject`） | Debug 与 Release 两个配置各加 `${PROJECT_ROOT}/Platform/Common/Inc`、`${PROJECT_ROOT}/Platform/Port/Inc` 两条 include；两个配置都排除 `Platform/Port/Test/`（Debug 改原 `<sourceEntries>` 条目，Release 补一块）；`Test/shim` 不进入固件 include，固件侧不定义 `PORT_CRITICAL_HOOKS` |

### 7.3 静态核对

| 判据 | 结果 |
| --- | --- |
| `S-05B-01` 查询失败不改出参 | `Hw_SciGetRxStatus`/`GetTxIdle` 失败分支直接 `return 0U`，写出参只发生在成功路径 |
| `S-05B-02` 四态与出参 | `HW_SCI_CLEAR_OK/WAIT_TX/STILL_ERR/UNREADABLE` 依次由 §3.2 的 5 步产生；`bForce` 只跳发送忙门禁，不复用『参数错误』码；出参写的是剩余错误位 |
| `S-05B-22` 出参哨兵 | `SciPortRecover()` 参数/驱动校验失败时直接 `return PARAM`，`*pu16Remain` 不被写；`IDLE` 分支写 0 |
| `S-05B-03` 唯一读取者 | `SciPortServiceRx()` 在 `HW_RESET`/`PAUSED` 分支直接 `return 0U`（无任何 `pstDrv` 调用）；排空只在心跳里做 |
| `S-05B-04` 临界区 | 反汇编见 §3.3（`PUSH ST1` / `POP ST1`），`PortCritical.obj .text` 0xA 字 |
| `S-05B-05` 接线 | `main.c:137` 绑定 CAN-B（失败置 bit4）；`main.c:160` 注册 `PortTask`（失败置 bit5） |
| `S-05B-06` map | `RAMGS1_PORTBUF` 0x0400/占 0x180/余 0x280；`RAMGS0` 用 0xE90/余 0x170（**368 字**）；`RAMGS1` 用量 0；`CLA1_MSGRAMLOW/HIGH` 仍 4/9 字、`RAMLS5` 仍 2 字；`.portbuf` run 地址 0x00DC00、`SciPort.obj (.portbuf)` = 0x180 字 |
| `S-05B-14` 状态一致性 | `SciPortTxResume()` 条件 `(state == IDLE) && (bTxResetLoss != 0)` |

### 7.4 偏离

1. **清除码取值**：申请写 `OK=1/REMAIN=0/PARAM=2/UNKNOWN=3`，实现改为 `OK=0/WAIT_TX=1/STILL_ERR=2/UNREADABLE=3`——把"等发送"从"参数错误"里拆出来，参数错误改由端口层返回 `SCI_RECOVER_PARAM`，避免驱动层混淆"未执行"和"参数非法"。
2. **`Hw_SciGetRxStatus()` 增 `bClear` 参数**：申请为两参；实现加第三参用于"读后清状态位"，`Hw_SciClearRxError()` 走 `RXFIFORESET + SWRESET` 复位路径，二者分工明确。
3. **`Hw_SciIsTxIdle()` 从两参改为便捷包装**：真实空闲判据是 `Hw_SciGetTxIdle(id, &idle)`（返回查询是否成功），端口驱动表的 `pfIsTxIdle` 也用它；`Hw_SciIsTxIdle()` 只供应用层快速判读。
4. **`SCIRXST` 位名**：申请写 `BI`，实际寄存器字段名是 `BRKDT`（`error #137` 后改正）。
5. **`.portbuf` 落点**：申请写"`.bss` 溢出池 0x0C00 + 预留 0x0400"，实现按此拆分后 `.portbuf` 落在 `0x00DC00`（v0.4 预估为 `0x00D000`），占用不变仍 `0x180`。
6. **`bTaskCreateError` 落地**：申请已确认 bit5；实现中 `TaskCreate()` 的两次 `Task_Start` 失败都置该位，`bPortInitError` 只保留给 `CanPortSetTxFrameType()` 绑定失败。
7. **`Hw_SciClearRxError()` 增 `bForce` 参数**（第二轮返工）：普通清除在发送未空闲时返回 `WAIT_TX` 且不碰硬件；`bForce != 0` 才跳过该门禁，FIFO 复位同时作废未发出的字节。出参改为"剩余错误位"，不再报 FIFO 层数（层数由 `ST_SCI_RX_STATUS.u16RxFifoLevel` 单独给出）。
8. **强制复位的意图与执行分开**：`WAIT_TX` 超阈值只置 `bForceReset` 并提交 `HW_RESET`/`RESET`；清 TX 环、记损失、`u32ForceResets++` 全部落在 `RESET` 阶段的**实际执行**里，预算不足时既不执行也不记损失。
9. **`SCI_RECOVER_OUT_*` 更名**为 `SCI_RECOVER_FLAG_*`：这些位描述"本次恢复观察到什么"，不再暗示"出参是否有效"；出参有效性改由返回码承担。

### 7.5 验证状态（截至 v0.9，已通过主机桩）

| 项 | 结果 |
| --- | --- |
| 实施方目标构建（v0.9） | **通过**：35 个编译单元、零 warning、零 error（`tools/build_minimal.py`，数字见 §7.2） |
| CCS Debug clean build（2026-11-06） | **通过**：37 个编译单元零 warning、链接与 hex 生成成功；`Debug/EP15K_2026.out` 758,760 B（sha256 `717418a7ce25308fb12b2c8565fa4eda9b0b971b33179a400ef63a0608ca9f0f`）、`.hex` 93,345 B |
| 审阅方对 v0.8 原包的主机桩测试 | `checks=339 fails=11`（普通构建与 ASan+UBSan 一致；Sanitizer 未报地址/UB 异常；11 条集中在 `S-05B-27`/`S-05B-28` 的刺激条件） |
| 审阅方在诊断副本修正两个用例的刺激后 | `checks=339 fails=0`（仅改刺激，未改交付源码与断言） |
| 审阅方对 v0.9 包的主机桩测试 | **通过**：`checks=355 fails=0`，`-O2 -Wall -Wextra` 与 ASan+UBSan 两次一致，编译无警告、Sanitizer 未报异常 |
| 实施方本机（v0.9） | **未执行**：无 C 主机编译器（`gcc`/`clang`/`cc`/`tcc`/`zig` 均不存在，WSL 无发行版），只做目标工具链 `--compile_only` 与目标链接 |
| 本轮用例变化 | 新增 `S-05B-29`；补正 `S-05B-27`/`S-05B-28` 的刺激条件；断言点由 339 增至 **355** |
| 05A 基线 | v0.9 包：`checks=151 fails=0`（主机 `-O2 -Wall -Wextra` 与 ASan+UBSan 两种运行） |
| 上板项 | `S-05B-07`~`S-05B-14` 全部未测；新增 `S-05B-25` 的逐字节 TX 临界区对 20 kHz 控制响应的影响仍需上板 |
| 上板测量开关 | `main.c` 的 `PORT_BENCH_TEST` 默认 `0U`：不编译测量代码，关闭态与 `ec59056` 代码一致（`.text 0x44B2`、`RAMGS0` 用 `0xE90`／余 `0x170`）；开启态 `.text 0x4547`、`RAMGS0` 用 `0xF24`／余 `0xDC`。用法见 §8.4 |
| 05B 代码状态 | **代码、主机桩与 CCS Debug 构建均通过（2026-11-06，v0.9 包，检查点 `ec59056`）**：355 项检查、0 项失败；四轮历史 217/8 → 276/12 → 314/1 → 339/11 → 355/0；上板验收待完成 |

### 7.6 第二轮返工记录（m03507）

| # | 审阅发现 | 修正 | 代码位置 |
| --- | --- | --- | --- |
| 1 | 持续忙时端口先清 TX 环，下一轮清除被真实驱动 `WAIT_TX` 拒绝，复位永远无法完成 | 驱动接口增 `bForce`；端口只在**执行清除的那一轮**清队列、记损失、计强制复位次数 | `Drivers/src/Hw_SciDriver.c`（`Hw_SciClearRxError`）、`SciPort.c`（`SciPortRecoverAdvance` 的 `WAIT_TX`/`RESET` 两支） |
| 2 | `bSwInvalidated` 置 1 后不清零，第二次故障前发布的批次仍可被消费 | 标志生命周期限定为一次恢复：排空完成与 `IDLE`/`NONE` 成组清零 | `SciPort.c`（`SciPortServiceRx` 的排空完成提交块） |
| 3 | 排空完成后 `bTxSuspended` 仍为 1，普通发送再也起不来；TX 循环只在入口查挂起位 | 排空完成按"是否有强制复位损失"决定解冻；`SciPortServiceTx()` 的"复查门禁 + `Peek` + 提交 + `Drop`"放进同一段临界区，预算检查留在临界区外 | `SciPort.c`（`SciPortServiceTx`） |
| 4 | `bPortInitError` 置位后仍注册 `PortTask`、心跳仍无条件搬运 | 新增 `PortTaskRegister()`/`PortTaskServiceFromHeartbeat()`，`main.c` 两处按 `bPortInitError` 门禁 | `PortTask.h/.c`、`main.c` |
| 5 | `RESET_DONE` 无预算门禁；`SciPortRecover()` 先清出参再校验；层数与 `OUT_*` 位混用；`RecoverRetry()` 未查时间源还清证据；`Resync()` 改 `PAUSED`/`HW_RESET` | 逐项修正（§3.4、§3.6） | `SciPort.c`、`SciPort.h` |
| 6 | `Hw_SciInit()` 逐项检查逐项写，配置 C/D 非法时 A 已启用 | 两遍：先整表校验，再写寄存器 | `Hw_SciDriver.c`（`Hw_SciInit`） |
| 7 | 8 条断言按旧的"一次性丢 32 字节"编写 | 改为每轮一个配额 8 字节、读空才结束；`PopBatch` 返回值改字节数 | `port_stub_test.c`（`S-05A-23`/`S-05A-27`/`S-05B-07`） |

新增用例 8 个（`S-05B-15`~`S-05B-22`），见 §5.5；桩侧新增可编程 `s_u16TxBusy`（普通清除的发送门禁）、`s_au16ClearForce`（记录驱动收到的 `bForce`）与 `Task_Start()` 记录桩。

### 7.7 第三轮返工记录（m03765）

| # | 审阅发现 | 修正 | 代码位置 |
| --- | --- | --- | --- |
| 1 | 强制复位没清硬件 TX FIFO（`SWRESET` 不清 FIFO 指针/内容），软件环已空的旧字节仍会上线 | 强制分支先给 `SCIFFTX.TXFIFORESET` 低脉冲并清 `TXFFINT`，复位后回读 `TXFFST`，非 0 返回 `WAIT_TX` 不报 `OK` | `Drivers/src/Hw_SciDriver.c`（`Hw_SciClearRxError` 步骤 0/步骤 4） |
| 2 | 上一次的 `bTxResetLoss` 被当作下一次强制复位的指令 | `RESET` 阶段取本轮 `bForceReset` 的局部快照决定驱动实参、清环与计数；`bTxResetLoss` 只负责保持冻结 | `Platform/Port/Src/SciPort.c`（`SciPortRecoverAdvance` 的 `RESET` 支） |
| 3 | 时间源在调度入口失败时，端口里的 WAIT_TX 旧基准仍有效 | 新增 `SciPortInvalidateWaitBases()`；`PortTask()` 入口取时失败与 `PortBudgetHasLeft()` 轮内失败都调用它，下次成功读取只重建基准 | `SciPort.c/.h`、`PortTask.c` |
| 4 | `BUSY`/`UNKNOWN` 改写出参；头文件注释仍是 FIFO 占用 | 阶段结果只存端口对象，仅 `OK`/`IDLE_OK`/`FAIL` 回写调用方出参；驱动出参语义改成"剩余错误位"，`WAIT_TX`/`UNREADABLE` 不写 | `SciPort.c`（`SciPortRecover`）、`Hw_SciDriver.h` |
| 5 | 普通复位成功就解冻，排空完成前可以提交 | 解冻统一放到 ISR 提交 `IDLE`/`NONE` 的同一段临界区；`RESET_DONE` 的 `OK` 分支不再动 `bTxSuspended` | `SciPort.c`（`RESET_DONE` 支与排空完成提交块） |

另修测试预期 12 条：`S-05A-27`（8 轮读完 + 第 9 轮观察空）、`S-05B-03`（清环与损失断言移到真正执行 `RESET` 之后，保留"执行前仍有 4 字节"）、`S-05B-15/16/20`（`SciPortWriteTx()` 返回实际入队字节数 3/2/4）、`S-05B-18/19`（恢复或重同步后首次心跳只建基准，需再服务一轮）。

### 7.8 第四轮复审返工记录（v0.7 → v0.8）

| 项 | 内容 |
| --- | --- |
| 驱动 | `Hw_SciClearRxError()` 删除提前写 `*pu16Remain = 0`；步骤 4 改为返回 `HW_SCI_CLEAR_TX_DIRTY`，不再借用 `WAIT_TX` |
| 端口 | `RESET` 阶段用 `u16DrvRemain = p->u16RecRemain` 兜底，仅驱动给出有效值时更新；`RESET_DONE` 新增 `TX_DIRTY` 分支 → `SciPortPause()` + `FAIL`；`SciPortRecover()` 出参由本次结果生成，`PAUSED` 分支也回写最近可靠值 |
| 桩 | `Hw_SciClearRxError` 增 `kind == 4`（`TX_DIRTY`），并把 `*pu16Remain = 0` 移到发送忙门禁之后，与真实驱动一致 |
| 用例 | `S-05B-19` 初始发布补一次 `RxRound()`；新增 `S-05B-27`、`S-05B-28`；断言点 314 → **340** |
| 构建 | 35 单元零 warning；`.out` 780,000 B；`.map` 99,716 B；`RAMGS0` 用 0xE90 / 余 0x170 = 368 字；`.portbuf` 0x00DC00 / 0x180 / 预留余 0x280 |
| 未跑 | 主机桩执行（本机无 C 编译器）、上板项（三路恢复耗时、逐字节 TX 临界区对 20 kHz 控制的影响） |

### 7.9 第五轮复审返工记录（v0.8 → v0.9）

| 项 | 内容 |
| --- | --- |
| 状态机 | 新增 `SCI_REC_PHASE_VERIFY_TX (5)`：`SciPortRecoverRetry()` 保留 `u16ClearResult`，原因不是 `TX_DIRTY` 才直接进 `CLASSIFY` |
| 重试 | `VERIFY_TX` 阶段查 `pfIsTxIdle()`：仍非空 → `FAIL` + `PAUSED` + TX 冻结；查询失败 → `UNKNOWN` + `PAUSED`；确认清空才清零原因并转 `CLASSIFY` |
| 桩测试 | `S-05B-27` 补 `s_u16TxIdle = 0` 与建基准调用；`S-05B-28` 两条路径补建基准、跨轮路径先 `PortBudgetBegin()`；新增 `S-05B-29` 成对重试用例；断言点 339 → **355** |
| 构建 | 35 单元零 warning；`.out` 780,428 B；`.map` 99,780 B；`RAMGS0` 用 0xE90 / 余 0x170 = 368 字；`.portbuf` 0x00DC00 / 0x180 / 预留余 0x280 |
| 未跑 | 主机桩执行（本机无 C 编译器）、上板项（三路恢复耗时、逐字节 TX 临界区对 20 kHz 控制的影响） |

## 8. 审阅结论（由 Codex 填写）

### 8.1 结论

**05B v0.9 代码审阅及主机桩通过**：355 项检查、0 项失败；上板验收待完成。

| 核验项 | 结果 |
| --- | --- |
| 交付包 | 34 个文件、CRC 通过；单独上传的文档与包内版本一致 |
| 主机桩（`-O2 -Wall -Wextra`） | `checks=355 fails=0`，编译无警告 |
| 主机桩（ASan＋UBSan） | `checks=355 fails=0`，Sanitizer 未报地址/UB 异常 |
| `.map` | `RAMGS0` 余 368 字；`.portbuf` 位于 `0x00DC00`、占 `0x180` 字；预留区余 `0x280` 字 |
| 目标构建（脚本） | 35 个编译单元零警告，沿用实施方构建记录 |
| CCS Debug clean build | **通过**（2026-11-06）：37 个编译单元零 warning、链接与 hex 生成成功；`.map` 的 `RAMGS0` 与 `.portbuf` 与脚本构建一致 |

### 8.2 上轮阻塞项的复核结果

审阅方用独立探针把 RX 错误位归零后，依次注入三种 TX 查询结果：

| 显式重试时的 TX 状态 | 实测结果 |
| --- | --- |
| 查询失败 | 返回 `UNKNOWN`，出参保持 `0xCAFE`，端口回 `PAUSED`，保留 `TX_DIRTY` 原因，拒绝解冻 |
| 查询成功但仍忙 | 返回 `FAIL`，端口保持 `PAUSED`，拒绝解冻 |
| 查询成功且已空闲 | 返回 `OK`，出参为 0；ISR 排空前仍拒绝解冻，排空回 `IDLE` 后 `SciPortTxResume()` 才成功 |

整个序列中驱动清除调用次数始终为 1——即 RX 错误位归零后，显式重试仍必须通过 TX 空闲检查（`SciPort.c:478`）。

### 8.3 上板验收优先项（不属于主机验证范围）

1. **强制复位后线路上是否仍有旧帧尾部**：用逻辑分析仪观察强制复位前后 TX 引脚，确认 `TXFIFORESET` + `SWRESET` 之后没有残留字节继续移出。
2. **三路同时恢复时的心跳 ISR 耗时**：三路 SCI 同时进入排空，测量 100 µs 心跳里 `SciPortService100us()` 的最坏耗时（每端口 8 次读取，合计上限 24 次）。
3. **逐字节 TX 临界区对 20 kHz 控制中断响应的影响**：测量 `SciPortServiceTx()` 的每字节临界区长度与主中断的最坏响应抖动。

每条记录都须写明：①实际烧录文件名与构建配置（如 `Debug/EP15K_2026.out`、Debug）；②该文件的 SHA-256；③测量条件与波形/截图编号；④若临时加入 GPIO 打点，记录相对检查点 `ec59056` 的打点差异，测完删除后再提交。

### 8.4 上板测量操作附录

#### 8.4.1 测量开关

`main.c` 顶部有 `#define PORT_BENCH_TEST (0U)`：

- `0U`（默认）：不编译 `BenchTask`、不占用额外 RAM，目标代码与检查点 `ec59056` 一致。
- `1U`：编译回显/灌满任务与 ISR 打点，仅供上板测量使用。

开启后 CCS 编译，并在 `Expressions` 里改/看下列全局量（非 `static`，符号可见）：

| 符号 | 含义 |
| --- | --- |
| `g_u16BenchMode` | `0` = 回显（收到什么发回什么）；`1` = 灌满 TX（`0x40..0x7F` 循环） |
| `g_u32BenchTxTotal` | 累计写入 TX 环的字节数 |
| `g_u32BenchPopCalls` | 回显模式下成功取到的批次数 |
| `g_u32BenchResumeTries` | 强制复位后尝试解冻的次数（模拟协议层行为） |
| `g_u32IsrMarkMaxTicks` | 心跳 ISR 最长耗时（tick；`200 tick = 1 µs`，用于与示波器互校） |

测量结束必须改回 `0U`、重新构建，并确认 `.text` 与 `RAMGS0` 回到 §7.5 的关闭态数字后再提交。

#### 8.4.2 接线与参数

| 口 | TX | RX | 波特率 | 说明 |
| --- | --- | --- | --- | --- |
| SCI-D | GPIO93 | GPIO94 | 38400 8N1 | 建议主测口 |
| SCI-C | GPIO89 | GPIO90 | 115200 8N1 | 三路并发时第二路（接 ATS） |
| SCI-A | GPIO48 | GPIO49 | 19200 8N1 | 三路并发时第三路（接 ARM，慎用错误波特率注入） |

引脚复用已在 `Broad/src/DrvCfg_EP15K.c:115-120` 配成 SCI，不需要改。要求 3.3 V TTL 的 USB-TTL、共地、串口助手用十六进制发送并关闭自动追加回车换行。

#### 8.4.3 四项测量的操作要点

1. **强制复位后是否仍有旧帧尾部**：`g_u16BenchMode = 1` 让 TX 常忙 → 把助手临时改成 9600 发约 50 个 `00`（制造连续 `FE`）→ 立刻切回 38400 观察。正常应看到发送立即停止（强制复位清 FIFO 并冻结 TX），随后 `g_u32BenchResumeTries` 增长、回显恢复；若停止之后仍成段出现旧 `0x40..0x7F` 字节，即为残留。同时记录 `SciPortForceResets()` 与 `SciPortTxResetDrop()`。严格判定用逻辑分析仪看 GPIO93。
2. **三路同时恢复时的心跳 ISR 耗时**：三路同时用错误波特率灌数据 → 示波器 CH1 接 GPIO33 测高电平宽度（`BenchIsrMarkBegin/End` 夹住整段 ISR）→ 目标 ≤10 µs；并用 `g_u32IsrMarkMaxTicks / 200` 互校。
3. **逐字节 TX 临界区最大屏蔽时长**：保持 TX 常忙，在 `SciPortServiceTx()` 的临界区前后临时加 GPIO33 打点，测最宽脉冲；无示波器时用“第 4 项在 TX 空闲 / TX 常忙两种条件下的差值”作为干扰上界。
4. **20 kHz 事件到 ISR 入口延迟**：功率级必须断开（`GPIO88` 芯片驱动使能保持低）；示波器 CH1 接 GPIO0（EPWM1A，CTR=ZERO 对应沿）、CH2 接 GPIO33，测 CH1 沿到 CH2 上升沿的最大间隔（含 ADC 转换 + CLA 任务 + 中断响应）。

#### 8.4.4 判定与清理

测量数据按 §8.3 记录（烧录文件名+配置、SHA-256、测量条件、波形编号）。清理步骤：`PORT_BENCH_TEST` 改回 `0U` → 重新构建 → 核对 `.text 0x44B2`、`RAMGS0` 用 `0xE90`／余 `0x170` → 删除 §8.4.3 第 3 项加在端口层的打点 → 提交。


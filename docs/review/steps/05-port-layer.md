# 步骤 05：端口层与环形缓存（申请草案 v0.9）

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`（当前 HEAD；工作区含步骤 02~04 的未提交改动）
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：`S-05A-01`~`S-05A-25`（05A 静态与桩驱动）／`S-05B-01`~`S-05B-09`（05B 上板）
- 状态：草案 v0.9（待复核）。v0.9 是在 v0.8 交付包的主机实测（`checks=149 fails=1`，ASan/UBSan 未报异常）基础上，按第四轮复审的 2 项整改：`CanPortSetTxFrameType()` 的未决检查不再以"已初始化"为前提；`S-05A-07` 用例隔离 TX（发送桩置忙）以真正验证"接收不动 TX 队列"。**上一轮结果记为"已执行，149 项检查、1 项失败；审阅方诊断副本隔离 TX 后 149/149 通过"，本包待复核。**

---

## 0. 修订记录（v0.2 → v0.9）

| # | v0.2 的修改 | 章节 |
| --- | --- | --- |
| 1 | SCI 静默间隔改为"字节环 + 带序号与时标的批次记录"，新增流中断通知；删掉"读一字节＋时标"的不可实现承诺 | §3.4、§3.7 |
| 2 | CAN 粘滞覆盖改为按对象锁存 `bHwOverwriteSeen`，诊断语义改为"本次对象初始化以来至少发生过一次覆盖"；软件丢弃逐帧计数 | §3.5 |
| 3 | 发送队头推进写成契约：SCI 按已入 FIFO 的前缀 `Drop(k)`；CAN 整帧语义；**恢复后队头必须由上层显式裁定**，解冻不等于重发 | §3.6、§3.5.3 |
| 4 | 配额、耗时目标、最长服务间隔三者分开；每轮尝试次数含 `EMPTY/BUSY` 上限与端口轮转公平性 | §3.3 |
| 5 | 资源按有效容量（槽数−1）核算；§2 API 表按步骤 04 头文件逐项重写 | §3.8、§2 |
| 6 | §6.2 四项裁定已落地；交付拆为 05A（队列＋端口对象＋桩驱动服务）与 05B（main.c／心跳／真实驱动） | §1、§4、§5 |
| 7 | 批次时标改为递减计数器差值 `(Uint32)(tick_prev − tick_next)`；删除固定扣除 `travel(count)` 的公式，改以"空读轮数"给出间隔上下界 | §3.4.3、§5.1 |
| 8 | 双环先查容量再写，段记录作为消费者可见的提交点；重同步在临界区内一次性处理两环、序号与标志 | §3.4.2、§3.4.4 |
| 9 | CAN `MsgLst` 归一值直接判 `!= 0` 并锁存（入队失败也锁存）；补 `ReadRx`/`RecoverModule` 的 `BUSY`；`ClearTxFreeze()` 不得绕过待裁定队头；定义 RX 冻结的清除者 | §3.5 |
| 10 | SCI-C 段环改 32 槽；资源账更新为 1,920 字；协议层 256 字节帧缓冲单列；服务间隔改按端口统计 | §3.8、§5.2 |
| 11 | 删除 `(N∓1)×P` 静默换算；空读轮数降为诊断量，另存空读观察时标；服务间隔异常独立触发 `bTimingBad` | §3.4.2、§3.4.3 |
| 12 | 排空统一为"每轮最多 8 次 `pfTryRead`、整批拒收、配额到即结束"；删除"不会转成 RXFFOVF"的保证 | §3.4.2 |
| 13 | 临界区改为"保存→屏蔽→恢复"，支持嵌套；时间读取改 `pfTryGetTick()` 状态式并定义失败处理 | §3.4.5、§3.4.6、§3.1 |
| 14 | Bus-Off 恢复与模块重建分开；裁定接口在驱动已恢复时同时解冻；容量改按目标 `sizeof`，`S-05B-06` 的 20 µs 改称轮次预算 | §3.5、§3.8、§5.2 |

---

### 0.5 v0.5 修订（05A 实施后）

| 项 | 落地位置 |
| --- | --- |
| 时标 0 的合法性 | §3.4.2：批次的 `u32Tick == 0` 是合法观察时刻，是否存在观察由 `u16EmptyRounds != 0` 判定 |
| 时间故障恢复 | §3.4.6：统一走 `SciPortResync()` 五步，并丢弃 FIFO 残留最多 32 字节；恢复后首个批次仍置流中断 |
| CAN RX 冻结前置 | §3.5.1：`CanPortServiceRx()` 的前置条件是 `bRxFrozen == 0`、`pfIsInitialized()` 调用结果非 0、`pfIsPending()` 调用结果为 0 |
| 待裁定队头校验 | §3.5.4：`CanPortResolvePendingHead()` 四条校验；重复或非法裁定不改队列与冻结状态 |
| SCI-C 段环 32 槽 | §3.8：保持 32 槽，理由是约 2 ms 消费延迟需要 31 条有效记录 |
| `PortTask` 预算口径 | §3.3.3：20 µs 是轮次预算（超出即停止发起后续调用），不是整轮耗时上限 |

### 0.6 v0.6 修订（落实 05A 审阅意见）

| # | 审阅意见 | 落实 |
| --- | --- | --- |
| 1 | 临界区测试没覆盖延迟投递 | 桩钩子改为「屏蔽期间挂起、退出到未屏蔽时投递」，`S-05A-09` 覆盖「进入前已屏蔽则退出仍屏蔽」「嵌套深度复原」「挂起故障在恢复后送达」；另加重同步最多排空 32 字节的用例 |
| 2 | CAN 覆盖测试与契约冲突 | **删除 `CanPortClearHwOverwriteSeen()`**：覆盖锁存只能随模块重建成功清除；用例改为验证 Bus-Off 恢复保留锁存、模块重建清锁存、队列满时仍锁存 |
| 3 | UNKNOWN 测试无法证明不重发 | 桩记录 `Hw_CanSend` 调用次数与最后提交帧；覆盖 UNKNOWN 后重复服务不增加、驱动恢复未裁定不增加、`RETRY` 只提交队头一次（帧号与首字节）、`DROP` 只删一帧、重复裁定不删下一帧 |
| 4 | SCI 可观察证据不足 | 探测计数（失败探测也计入 8 次上限）、TX 字节顺序与不重复、空读观察 `tick = 0`、段环先满整批拒收；`S-05A-05` 明确只喂 1 字节 |
| 5 | 正文与实现不同步 | §3.3.5 按实现重写、§3.4.1 注释、§3.4.2 时间源失败作废基准、§3.5.1 前置条件、§3.5.3 删除独立清除接口、§6.1 风险 4 改为不缩段环、§5.1 补 `S-05A-14`~`S-05A-18` |

### 0.7 v0.7 修订（第二轮 05A 复审返工，2026-11-05）

| # | 审阅问题 | 本轮落实 |
| --- | --- | --- |
| 1 | 批次记录长度在主机上算错（`sizeof` 被当成 Uint16 个数，复制越界） | §3.4.1：`SCI_SEG_ITEM_WORDS` 改为 `sizeof(ST_SCI_RX_SEG) / sizeof(Uint16)`，并加编译期整除断言 |
| 2 | `PortTask` 把轮转槽号当硬件 SCI ID（SCI-D 永不被服务） | §3.3.5：新增显式映射表 `{A,C,D}` / `{A,B}` 与 `PortTaskUnitPortId()`；`S-05A-19` 用三路实际发送记录验证 |
| 3 | CAN 发送未查 `pfIsPending()` | §3.5.3：每次提交前查未决（`u32TxPendingBlock++`）；`S-05A-20` 验证 pending 时不发起事务 |
| 4 | SCI 接收错误入口从未被调用 | §3.4.2 第 0 步：每轮无条件查 `pfGetRxError()`，非 0 计数并置流中断（FIFO 空也执行）；`S-05A-21` |
| 5 | 20 µs 预算只检查到模块外层 | §3.3.5 / `PortBudget.h`：预算窗口下沉到服务层，每次驱动调用前查；`CAN_PORT_TX_QUOTA` 恢复为 2；`S-05A-22` 用 30 µs 慢接收验证只发起一次 |
| 6 | 重同步"跨轮续丢"与故障代次 | §3.4.5：排空配额用尽即进入 `bResyncDraining`，后续每轮继续丢到读空；新增 `u16FaultGen` 与带代次的 `SciPortClearStreamBreak(id, gen)`；`S-05A-23`、`S-05A-24` |
| 7 | CAN 帧类型被静默丢弃 | §3.5.3：入队拒绝 RTR、DLC>8、与模块固定发送类型不符的帧；`S-05A-25` |

### 0.8 v0.8 修订（第三轮 05A 复审返工，2026-11-05）

| # | 审阅问题 | 本轮落实 |
| --- | --- | --- |
| 1 | 时间源中途失败后同一轮仍可能恢复服务 | §3.3.5：`PortBudgetHasLeft()` 把读取失败锁存到本轮结束（`s_bBudgetTickFault` 只在 `PortBudgetBegin()` 复位），后续端口一律返回 0；`S-05A-26` 用"第 3 次读取失败、随后恢复"复现 |
| 2 | 排空未完成即可确认流中断 | §3.4.5：`SciPortClearStreamBreak()` 在 `bResyncDraining != 0` 时返回 0 并保持 `bStreamBreak`；排空完成才按代次确认；`S-05A-27` |
| 3 | `u16TxStall` 与头文件定义不一致 | `SciPortServiceTx()` 仅在"本轮已接纳 0 字节"时递增；部分接受后遇忙属正常截断；`S-05A-06` |
| 4 | `CanPortSetTxFrameType()` 可在运行中改规则 | §3.5.2：TX 队列非空、存在待裁定队头、驱动有未决事务时返回 0；头文件注明它只设端口校验规则、调用者保证与驱动一致；`S-05A-28` |
| 5 | README 主机命令漏了队列实现 | 打包脚本与 README 补 `../../Common/Src/RingBuff.c ../../Common/Src/CanQueue.c`；另记"三路同时重同步"要在 05B 单列验收 |

### 0.9 v0.9 修订（第四轮 05A 复审返工，2026-11-05）

| # | 审阅问题 | 本轮落实 |
| --- | --- | --- |
| 1 | `S-05A-07` 未隔离 TX：`CanPortServiceOne()` 会同时服务 RX 与 TX，发送桩默认返回 OK，TX 空闲量自然增加 | 用例内显式 `s_u16CanSendRet = HW_CAN_BUSY;`，既可验证"接收不动 TX 队列"，又顺带覆盖"发送遇忙保留队头" |
| 2 | `CanPortSetTxFrameType()` 的未决检查以"已初始化"为前提，`未初始化 + 有未决` 时仍允许改规则 | 改为直接判 `pfIsPending()`（不看 `pfIsInitialized()`），并补两条断言：未初始化+未决 → 拒绝；未初始化+无未决 → 允许 |

## 1. 意图与边界

### 1.1 本步要做（分两阶段）

| 阶段 | 内容 | 前置条件 |
| --- | --- | --- |
| **05A** | 通用队列（`RingBuff`/`CanQueue`）落位；SCI/CAN 端口对象与服务逻辑；**桩驱动接口**（可注入故障）；主机构建与四类场景验证 | 无（不接 main.c、不接真实外设） |
| **05B** | 接入 `main.c`（初始化顺序、`PortTask` 注册、100 µs 心跳调用）；接入真实 `Hw_SciDriver`/`Hw_CanDriver`；上板预算与抖动实测 | **步骤 04 的 `S-04-09`~`S-04-15`、`S-04-18` 上板通过** |

### 1.2 本步不做

| 不做 | 理由 |
| --- | --- |
| Modbus RTU 协议本体 | 属于协议层；本步只提供"字节 + 批次时标 + 流中断"三类信息 |
| Xmodem 协议本体与升级流程 | 同上；整块缓冲归协议层 |
| 示波器采样与上传 | 同上；记录环独立于 CAN 帧环 |
| 改 CAN 接收对象数量/ID 配置 | 已裁定：每模块保留对象 5 一个，允许突发丢帧但必须留下覆盖证据 |
| 启用 CAN 中断 | CAN 只在 1 ms 前台服务；CAN 驱动的等待接口不得进入 100 µs 心跳 |
| 协议层与端口层之间的重试策略 | 恢复后是否重发由协议层按序号/应答决定，端口层只提供显式接口 |

### 1.3 边界理由

- 端口层只回答"哪些字节/帧到达、什么时候到达、有没有丢"，不回答"这一帧合不合法"。
- 队列不覆盖写：覆盖写会让消费者读到半新半旧的数据，与步骤 04 的"结果不确定必须显式上报"冲突。
- 所有丢弃都必须可见且可区分来源（硬件覆盖 / 软件队列满 / 流中断），否则协议层无法选择"重传"还是"重新同步"。

---

## 2. 现状核对

### 2.1 驱动 API 现状（按步骤 04 后的头文件逐项）

| 接口 | 签名 | 返回值语义 | 端口层用法 |
| --- | --- | --- | --- |
| `Hw_CanInit` | `Uint16 Hw_CanInit(const ST_CAN_CFG *)` | 1 成功／0 失败（初始化统一 0/1） | 05B 由 `main.c` 调用，端口层不调 |
| `Hw_CanIsInitialized` | `Uint16 (Uint16 u16CanId)` | 布尔：1 已初始化 | 端口层服务前置检查 |
| `Hw_CanIsPending` | `Uint16 (Uint16 u16CanId)` | 布尔：该模块存在未决事务 | 为 1 时端口层不发起新事务，返回"暂不可用" |
| `Hw_CanSend` | `Uint16 (u16CanId, u16ObjIdx, u32MsgId, const Uint16 *pu16Data, u16Len)` | `HW_CAN_OK/BUSY/ERR/UNKNOWN` | TX 队列推进的唯一依据 |
| `Hw_CanIsTxDone` | `Uint16 (u16CanId, u16ObjIdx)` | 布尔查询（读 `CAN_TXRQ_21`） | 诊断用，不参与队列推进 |
| `Hw_CanIsRxReady` | `Uint16 (u16CanId, u16ObjIdx)` | 布尔查询（读 `CAN_NDAT_21`） | 诊断用；接收仍以 `Hw_CanReadRx` 为准 |
| `Hw_CanReadRx` | `Uint16 (u16CanId, u16ObjIdx, Uint32 *pu32MsgId, Uint16 *pu16Data, Uint16 *pu16Len, Uint16 *pu16FrameType, Uint16 *pu16MsgLst)` | `HW_CAN_OK/EMPTY/BUSY/ERR/UNKNOWN`；`pu16MsgLst` 为 bit14 归一值 | RX 服务唯一入口 |
| `Hw_CanGetStatus` | `Uint16 (Uint16 u16CanId)` | **硬件状态位**（非运行期状态码） | 仅在故障时取快照，不参与流程判断 |
| `Hw_CanIsBusOff` | `Uint16 (Uint16 u16CanId)` | 布尔 | 故障上报 |
| `Hw_CanRecoverBusOff` | `Uint16 (Uint16 u16CanId)` | `HW_CAN_OK/BUSY/ERR/UNKNOWN` | 由上层触发，端口层只负责置冻结 |
| `Hw_CanRecoverModule` | `Uint16 (Uint16 u16CanId)` | `HW_CAN_OK/BUSY/ERR/UNKNOWN`（重入返回 `BUSY`） | 同上 |
| `Hw_SciInit` | `Uint16 Hw_SciInit(const ST_SCI_CFG *)` | 1 成功／0 失败 | 05B 由 `main.c` 调用 |
| `Hw_SciTryReadByte` | `Uint16 (u16SciId, Uint16 *pu16Data)` | 1 读到／0 空 | RX 服务唯一读数入口 |
| `Hw_SciTryWriteByte` | `Uint16 (u16SciId, Uint16 u16Data)` | 1 进 FIFO／0 满 | TX 服务唯一写入入口 |
| `Hw_SciGetRxError` | `Uint16 (u16SciId)` | **合并错误位**（`RXERROR | RXFFOVF`），无确认语义 | 只能判"有错"，不能分类统计；分类须 05B 增补驱动入口 |

结论：端口层**不读** `SCIFFRX.RXFFST`、**不碰**任何寄存器；SCI 侧最多调用 `Hw_SciTryReadByte()`，读空即停。

### 2.2 其他现状

| 项 | 现状 | 影响 |
| --- | --- | --- |
| SCI 配置 | A 19200 / C 115200 / D 38400，LSPCLK 50 MHz | 决定 §3.8 容量 |
| SCI FIFO | 16 层，RVFFIENA 关闭 | 100 µs 服务下单字节延迟可接受 |
| `Task_Run()` | 每次消费 1 个 tick，顺序执行所有到期回调 | 前台粒度 1 ms，且会被长回调推迟 |
| CAN 硬件容量 | 每模块 1 个接收对象（对象 5，DLC 8） | 服务间隔内最多暂存 1 帧；超额必然覆盖 |
| 时间源 | `Hw_TimeBase`（CPU Timer0，32 位递减，21.47 s 回绕） | 所有时标统一用它，不另建计时器 |
| 目录 | `Platform/{Pll,Sample,Schedule,Status}` | 新增 `Platform/Common`、`Platform/Port` |

---

## 3. 设计

### 3.1 分层

```
协议层   ModbusSlave · Xmodem · SoftOsc            （本步不实现）
            ↑ 字节 + 批次时标 + 流中断 / 帧 + 覆盖证据
端口层   SciPort(3) · CanPort(2)                   （本步实现）
            ↑ 只经 ST_PORT_DRV 接口表调用
驱动层   Hw_SciDriver · Hw_CanDriver · Hw_TimeBase
```

端口层通过**函数指针表**调用驱动，这是 05A 能用桩驱动验证的前提：

```c
typedef struct {
    Uint16 (*pfTryRead)(Uint16 u16PortId, Uint16 *pu16Data);
    Uint16 (*pfTryWrite)(Uint16 u16PortId, Uint16 u16Data);
    Uint16 (*pfGetRxError)(Uint16 u16PortId);
    Uint16 (*pfTryGetTick)(Uint32 *pu32Tick);   /* 1 成功 / 0 时间源不可用 */
} ST_SCI_PORT_DRV;

typedef struct {
    Uint16 (*pfIsInitialized)(Uint16 u16CanId);
    Uint16 (*pfIsPending)(Uint16 u16CanId);
    Uint16 (*pfReadRx)(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 *pu32Id,
                       Uint16 *pu16Data, Uint16 *pu16Len,
                       Uint16 *pu16FrameType, Uint16 *pu16MsgLst);
    Uint16 (*pfSend)(Uint16 u16CanId, Uint16 u16ObjIdx, Uint32 u32Id,
                     const Uint16 *pu16Data, Uint16 u16Len);
    Uint16 (*pfTryGetTick)(Uint32 *pu32Tick);   /* 1 成功 / 0 时间源不可用 */
} ST_CAN_PORT_DRV;
```

正式构建里这两张表绑定到真实驱动函数；05A 的测试程序绑定桩函数。

### 3.2 通用队列

- 采用已评审的下载包实现（SPSC、2 的幂掩码、只 `volatile` 索引、先搬数据后提交 head）。
- 落位：`Platform/Common/{Inc,Src}/RingBuff.{h,c}`、`Platform/Common/{Inc,Src}/CanQueue.{h,c}`。
- **有效容量恒为 槽数−1**，所有容量承诺按有效容量写（见 §3.8）。
- 不提供覆盖写接口；生产者只写 head，消费者只写 tail。
- `RingBuffWriteExact()` 用于"整帧要么全进、要么全不进"（CAN 帧入队）。

### 3.3 执行上下文、配额与时间口径（三者分开）

**3.3.1 执行上下文**

| 动作 | 上下文 | 频率 |
| --- | --- | --- |
| SCI RX 搬运（FIFO → 字节环 + 批次记录） | 100 µs 心跳 `Isr_Heartbeat()` 末尾 | 10 kHz |
| SCI TX 搬运（TX 环 → FIFO） | 1 ms 前台 `PortTask` | 1 kHz |
| CAN RX 服务（`Hw_CanReadRx` → 帧环） | 1 ms 前台 `PortTask` | 1 kHz |
| CAN TX 服务（帧环 → `Hw_CanSend`） | 1 ms 前台 `PortTask` | 1 kHz |

心跳内**不得**出现任何 CAN 调用；SCI RX 路径不得调用 `Hw_TimeBaseIsReady()` 之外的阻塞接口。

**3.3.2 每轮配额（硬上限，与耗时目标无关）**

| 动作 | 每轮配额 | 备注 |
| --- | --- | --- |
| SCI RX 搬运 | 每端口 8 字节 | 三路合计 24 字节；读空即停，不补足 |
| SCI TX 搬运 | 每端口 16 字节 | 只受 FIFO 空位限制 |
| CAN RX | 每端口 4 次读取尝试 | 含 `EMPTY/BUSY`，即最多 4 次驱动调用 |
| CAN TX | 每端口 2 次发送尝试 | 含 `BUSY`；`BUSY` 后本轮不再试同端口 |

**3.3.3 耗时目标（正常路径，上板实测，不是保证值）**

| 项目 | 目标 | 说明 |
| --- | --- | --- |
| 三路 SCI RX 合计（三路同时积压） | ≤ 1.5 µs | 上板目标；`24 × 12` 周期＝1.44 µs 只是读数循环本身，不含探测、调用、写环、写批次、错误处理 |
| 单路 SCI RX（空闲，只有 1~2 字节） | ≤ 0.4 µs | 单独测，避免被合计值掩盖 |
| 错误路径（`Hw_SciGetRxError` 非 0 + 流中断置位） | 单独记录 | 不设目标，只记录最坏值 |
| `PortTask` CAN RX（每模块 1 帧） | ≤ 8 µs | **仅正常路径**；一次驱动调用自身就有 20 µs 等待预算，最坏值必然更高 |
| `PortTask` CAN TX（每模块 1 帧） | ≤ 6 µs | 同上 |
| `PortTask` 单次轮次预算 | ≤ 20 µs | 本节所列 20 µs 是**轮次预算**：单项超出即停止发起后续调用，实际整轮耗时与超出量单独测量并记录 |

**3.3.4 最长服务间隔**

- 目标 ≤ 2 ms（两次 `PortTask` 之间的最大间隔），由 `Hw_TimeBase` 差值统计 `u32MaxServiceGap`。
- 该值同时决定 CAN 硬件覆盖的最大可能窗口（1 帧/模块/间隔）。
- 心跳内 SCI 搬运的名义周期是 100 µs，其实际抖动也记入各端口的 `u32MaxGap`（不作为静默判据，见 §3.4.3）。

**3.3.5 公平性与轮次预算（已实现）**

- 单元划分：`PORT_TASK_UNIT_NUM = SCI 3 路发送 + CAN 2 路收发 = 5`；SCI 接收不在 `PortTask`，仍在 100 µs 心跳的 `SciPortService100us()`。
- 轮转：每轮从游标 `s_u16Cursor` 开始，按 `(cursor + i) % 5` 服务，轮末游标 +1，顺序为 SCI-A→SCI-C→SCI-D→CAN-A→CAN-B→回到 SCI-A。观测接口 `PortTaskLastFirstUnit()`。
- **槽位不是硬件 ID**：槽位 0/1/2 依次映射到 `HW_SCI_ID_A`(0)/`HW_SCI_ID_C`(2)/`HW_SCI_ID_D`(3)，槽位 3/4 映射到 `HW_CAN_ID_A`/`HW_CAN_ID_B`；映射表为 `s_au16SciUnitPort[]`/`s_au16CanUnitPort[]`，对外由 `PortTaskUnitPortId(u16Unit)` 暴露（越界返回 `0xFFFF`）。直接把槽位当 ID 会导致 SCI-C/D 永远不被服务。
- 预算：`PortTask()` 先用 `PortBudgetBegin(u32Start)` 开窗，再在**每个单元开始前**与**服务层每次驱动调用前**查 `PortBudgetHasLeft()`（20 µs、Timer0 递减差值）。两层共用同一个起点，超出后不再发起新调用；已经开始的驱动调用允许完成，完成后超预算即停。服务层的收手分别计入 `u16TxBudgetStop`（SCI TX）、`u32TxBudgetStop`（CAN TX）；`PortTaskBudgetTruncations()` 只在单元之间收手时加一。观测接口 `PortTaskLastUnits()`。
- 为什么必须下沉到服务层：只在单元之间检查时，一次 `CanPortServiceOne()` 内部仍可连发多次 RX/TX——第一次调用就把 20 µs 用完后，后面几次照样会发起。
- **时间故障锁存到本轮结束**：`PortBudgetHasLeft()` 只要在本轮内出现过一次时间读取失败，就置 `s_bBudgetTickFault` 并从此返回 0，直到下一次 `PortBudgetBegin()` 才复位。理由：中途读不到时标意味着本轮的量值已经不可信，"下一次读成功就继续跑"会让排在后面的端口在同一个窗口里继续发起驱动调用（实测 `A=0 C=1`）；整轮停止比只停一个端口更符合"时间源失败整轮停止"的约定。`PortBudgetFaulted()` 供观测与测试区分"超预算"与"时间故障"。
- 时间源失败：起始时标读不到 → 整轮不服务，`PortTaskTimeSourceFailures()` 加一；单元前的预算读取失败同样立即停止（不用不可靠的时标凑预算）。
- 这两条保证"最长服务间隔"有界：最坏情况下一轮只服务 1 个单元，其余 4 个顺延到下一轮，不会有单元被永久跳过。

### 3.4 SCI 接收服务与静默保留契约

**3.4.1 数据结构**

```c
typedef struct {
    u32 u32Tick;          /* 本批次被观察到的时刻（搬运之前读取） */
    u32 u32TickEmptyLast; /* 最近一次读到 FIFO 为空的时刻；0 是合法值，存在性由 u16EmptyRounds 判定 */
    u16 u16SeqStart;      /* 本批次第一个字节的全局接收序号 */
    u16 u16Count;         /* 本批次字节数，1..8 */
    u16 u16EmptyRounds;   /* 期间连续空读轮数（诊断量，不是时间） */
} ST_SCI_RX_SEG;          /* 单元长度一律取 sizeof(ST_SCI_RX_SEG) */
```

- **队列单元长度一律按 `Uint16` 单元数计算**：`#define SCI_SEG_ITEM_WORDS ((Uint16)(sizeof(ST_SCI_RX_SEG) / sizeof(Uint16)))`。C28x 上 `sizeof` 以 16 位字为单位，主机上以字节为单位；只有除以 `sizeof(Uint16)` 才能在两套环境里都得到"环元素等于 struct 本身"的同一语义（目标 8 字、主机 16 字节 = 8 个 Uint16）。头文件同时用 `typedef char SciSegWordsMustDivide[...]` 做编译期整除断言，不整除直接编译失败。
- 端口状态增加：`u32LastTick`、`bTimeBaseValid`（首轮只建立基准）、`u32MaxGap`（按端口统计）、`bTimingBad`、`bStreamBreak`、`u16RxRejected`、`u16RoundsFull`；v0.7 再增 `u16RxErrorCount`（接收错误轮数）、`u16TxBudgetStop`（发送侧预算收手次数）、`volatile Uint16 bResyncDraining`（重同步跨轮丢弃中）、`volatile Uint16 u16FaultGen`（故障代次）。

**3.4.2 服务算法（100 µs 名义周期）**

0. **每轮固定先做两件事**（都在读时标之前）：
   - `bResyncDraining != 0` → 本轮仍按 `SCI_PORT_RESYNC_DRAIN_MAX`（32）丢弃 FIFO；读到空才清该标志、作废时间基准并保持 `bStreamBreak`，然后返回。期间不发布批次、不更新空读观察。
   - **无条件**调用 `pfGetRxError()`：非 0 → `u16RxErrorCount++` 并 `SciPortNoteFault()`（代次 +1、置流中断）。**不能因为本轮 FIFO 空就跳过**；也不因此中断本轮搬运（丢失可见优于堆积）。
1. `if (pstDrv->pfTryGetTick(&u32Tick) == 0U)` → 时间源失败：置 `bTimingBad`、**作废时间基准**（`bTimeBaseValid = 0`，恢复后必须重新建基准，不拿故障前的旧基准算间隔），**本轮不搬运、不发布批次**（宁可让 FIFO 溢出走 `bStreamBreak`，也不发布看似有效的时标），返回。
2. 首次服务（`bTimeBaseValid == 0`）只建立基准：`u32LastTick = u32Tick`、`bTimeBaseValid = 1`，返回，**不计算间隔**。
3. 计算 `u32Gap = (Uint32)(u32LastTick - u32Tick)`（递减计数），更新 `u32MaxGap`；若 `u32Gap > SCI_SERVICE_GAP_LIMIT`（默认 500 µs ≈ 5 个名义周期）→ 置 `bTimingBad`（**服务间隔异常独立触发，不依赖读满 8 字节**）。`u32LastTick = u32Tick`。
4. 最多调用 **8 次** `pfTryRead()`，读入本地缓冲 `Uint16 au16Buf[8]`，**不探测第 9 字节**：
   - 一次都没读到（FIFO 空）→ `u16EmptyRounds++`（饱和）、`u32EmptyLast = u32Tick`，本轮结束；
   - 达到 8 次或读到空为止，结束读取。
5. **提交或整批拒收**：若 `RingBuffFree(stSegRing) == 0` 或 `RingBuffFree(stRxRing) < count` → 整批丢弃（`u16RxRejected += count`）、置 `bStreamBreak`、`u16EmptyRounds = 0`，本轮结束；否则先写字节，**再写段记录作为提交点**，`u16RxSeq += count`，`u16EmptyRounds = 0`。
6. 若 `count == 8`（配额读满），置 `bTimingBad`、`u16RoundsFull++`：FIFO 可能仍有积压，本批次的观察时刻不能代表其首字节到达时刻。**配额到即结束本轮，剩余数据留到下一轮**；不额外排空、不承诺"不会转成 RXFFOVF"。

**3.4.3 端口层只提供观察信息，不换算静默**

- 只存 tick，不存微秒；对外用 `SciPortTicksToUs()` 换算，100 µs 是名义周期，实际周期以 `u32MaxGap` 为准。
- 经过时间必须写 `(Uint32)(tick_prev - tick_next)`（Timer0 递减），不能写反。
- **删除** `(u16EmptyRounds ∓ 1) × P` 形式的下界/上界换算：连续空读的实际间隔取决于当时的服务周期，用"历史最大间隔"统一换算会把 200 µs 的观察间隔算成 1 ms 下界，从而把尚未满足断帧条件的间隔判成满足。
- 观察信息只有三条，含义按字面收窄：
  1. `u32Tick`：本批次被观察到的时刻（首字节可能更早到达，且若读满配额则更不确定）；
  2. `u32TickEmptyLast`：上一批次之后最近一次"读不到完整字符"的时刻；
  3. `u16EmptyRounds`：期间空读了几轮（诊断用）。
- **FIFO 空只说明当时没有可读的完整字符**，不等价于线路一直静默；字符可能正在移位、也可能被 FIFO 阈值延迟。因此端口层不声称"可证静默"。
- 静默判定由协议层完成：必须把字符接收过程（每字符 10 bit 的传输时间）、轮询延迟（≤ 名义周期 + `u32MaxGap` 抖动）、批次观察时刻的模糊性一起计入；无法确定时按"时间关系不可信"处理并重同步。
- Modbus RTU 判据（协议层使用）：

| 波特率 | t1.5（帧内字符间隔） | t3.5（帧间静默） |
| --- | --- | --- |
| 19200 | 781 µs | 1.823 ms |
| 115200（>19200 按规范固定） | 750 µs | 1.750 ms |

  115200 若按"3.5 字符"算只有 304 µs（≈3 个名义周期，误差 33%），必须用固定值 1.750 ms。

**3.4.4 三种失效要分开报告**

| 场景 | 端口层报告 | 协议层应做什么 |
| --- | --- | --- |
| 消费者延迟（ISR 正常） | 两帧对应多个批次，批次之间留有 `u32TickEmptyLast` 等观察点 | 自行判读，能判就判，判不了就重同步 |
| ISR 延迟（两帧混入同一批） | `u16EmptyRounds == 0`，服务间隔异常或读满配额时 `bTimingBad = 1` | 该点时间关系不可信：丢弃候选帧并重同步 |
| 字节/段丢失、序号不连续、时间源失败 | `bStreamBreak = 1` / `bTimingBad = 1` | 丢弃候选帧并重同步 |

**3.4.5 流中断、重同步与临界区**

- `bStreamBreak` 产生条件（任一）：字节环拒收、段环满、序号不连续、驱动报接收错误（含 `RXFFOVF`）、重同步开始。全部经统一入口 `SciPortNoteFault()`：`u16FaultGen++` 且 `bStreamBreak = 1`。
- 端口层置位后**不得自行清除**；置位期间继续按 §3.4.2 搬运（丢失可见优于堆积）。
- 重同步分两段完成：

```c
void    SciPortResync(Uint16 u16PortId);                          /* 清两环/序号/观察记录/基准 + 开始丢弃 */
Uint16  SciPortIsResyncDraining(Uint16 u16PortId);
Uint16  SciPortFaultGeneration(Uint16 u16PortId);
Uint16  SciPortClearStreamBreak(Uint16 u16PortId, Uint16 u16Generation);
```

  1. `SciPortResync()` 在临界区内清空软件侧（两环、`u16RxSeq`、`u16RxSeqConsumed`、空读观察、时间基准），代次 +1、置流中断；然后按配额丢一轮 FIFO 残留。**没丢到空就置 `bResyncDraining`，由后续每轮服务继续丢弃**，不靠加大关中断期间的循环上限。
  2. 直到真正读到空：`bResyncDraining` 清零、作废时间基准、保持 `bStreamBreak`，等协议层确认。
- 清除必须**同时满足两个条件**：
  1. `bResyncDraining == 0`——软件侧与硬件 FIFO 都已完成重同步；丢弃仍在进行时确认，等于向上层谎报"已恢复"（实测 `draining=1 clear_result=1 break=0`）。
  2. 代次一致——前台先读 `SciPortFaultGeneration()`，确认时把它带回；代次不一致说明期间又发生了新故障，函数返回 0 且**保留**故障。仅给清零语句加临界区不能解决这条竞态（前台读旧故障 → ISR 新故障 → 前台清零抹掉新故障）。

- 临界区钩子契约是"**保存 → 屏蔽 → 恢复原状态**"，不是无条件 `DINT/EINT`：

```c
typedef Uint16 ST_PORT_IRQ_STATE;
ST_PORT_IRQ_STATE PortEnterCritical(void);   /* 返回进入前的中断状态 */
void PortExitCritical(ST_PORT_IRQ_STATE u16State);  /* 恢复该状态，支持嵌套 */
```

  若调用者进入前已关中断，退出时不得擅自打开。05A 的普通单线程测试可用空实现；**`S-05A-09` 必须用可模拟屏蔽与延迟投递的测试钩子**，空宏不能作为该项证据。

**3.4.6 时间源失败处理**

| 位置 | 失败时行为 |
| --- | --- |
| SCI 服务 | 置 `bTimingBad`，作废时间基准，本轮不搬运、不发布批次（不产生看似有效的时标） |
| SCI 接收错误 | 每轮检测（含 FIFO 空）：置 `bStreamBreak`、`u16RxErrorCount++`；05A 只做可见性，分类/确认/恢复留 05B |
| `PortTask` | 停止继续发起受时间预算约束的服务（CAN 收发），记录 `u32TimeBaseFaults++`，下一轮从 SCI 起重试 |
| 首轮 | 只建立基准，不参与间隔计算（避免用未初始化的 `u32LastTick`） |
| 恢复 | 统一走 `SciPortResync()`：软件侧清空、跨轮丢弃 FIFO 残留、重建基准；协议侧丢弃候选帧 |

### 3.5 CAN 接收与发送服务

**3.5.1 接收**

1. 前置（三者都满足才进入读循环）：`bRxFrozen != 0` → 跳过（结果不确定期间不得再读同一对象）；`pfIsInitialized()` 调用结果为 0 → 置 `bNotReady` 并跳过；`pfIsPending()` 调用结果非 0 → 跳过（模块有未决事务时端口层不得发起 IF 事务）。
2. 每次 `pfReadRx()` **之前**检查轮次预算 `PortBudgetHasLeft()`：为 0 立即结束本轮（已开始的调用允许完成，完成后超预算即停）。
3. 最多 4 次尝试：`pfReadRx()`。
   - `HW_CAN_EMPTY` → 结束本轮。
   - `HW_CAN_BUSY` → `u32ReadBusy++`，结束本轮（不重试同一对象）。
   - `HW_CAN_OK` → **先锁存覆盖证据，再入队**：

```c
if (u16MsgLst != 0U) {                 /* 已是归一 0/1 */
    pstObj->bHwOverwriteSeen = 1U;     /* 粘滞，只锁存一次 */
}
if (CanQueuePush(&pstPort->stRxQueue, &stFrame) == 0U) {
    pstPort->u32SwDrop++;              /* 逐帧精确计数 */
}
```

   - `HW_CAN_ERR` → `u32ReadErr++`，结束本轮。
   - `HW_CAN_UNKNOWN` → 置 `bRxFrozen = 1`，结束本轮。
3. 诊断语义固定为"本次对象初始化以来至少发生过一次硬件覆盖"，不提供次数。

**3.5.2 发送与裁定**

1. 前置：未初始化 / `bTxFrozen` / `bHeadPending` → 本轮跳过。
2. 最多 2 次尝试（`CAN_PORT_TX_QUOTA == 2`，与已裁定的"每轮 2 次尝试"一致）：每次提交前依次查
   - 轮次预算 `PortBudgetHasLeft()`：为 0 → `u32TxBudgetStop++` 并结束本轮；
   - 模块是否仍有未结束事务 `pfIsPending()`：非 0 → `u32TxPendingBlock++` 并结束本轮（**本轮 RX 刚判 UNKNOWN 时，紧接着的发送必须停止发起事务**）；
   - 然后 `Peek` 队头 → `pfSend()`。
   - `HW_CAN_OK` → `CanQueuePop()`，继续。
   - `HW_CAN_BUSY` → 保留队头，结束本轮。
   - `HW_CAN_ERR` → 保留队头，`u32TxErr++`，结束本轮（丢弃与否由上层定）。
   - `HW_CAN_UNKNOWN` → `bTxFrozen = 1`、`bHeadPending = 1`，结束本轮。
3. **入队校验（不能静默丢弃）**：发送接口只用 `id/data/dlc`，`flags` 不参与提交，因此
   - `flags & CAN_FRAME_RTR` → 拒绝入队，`u32TxRtrReject++`；
   - `dlc > HW_CAN_DATA_LEN_MAX` 或帧类型与 `bTxExtId` 不符 → 拒绝入队，`u32TxTypeReject++`；
   - `CanPortSetTxFrameType(u16CanId, bExtId)` 设置本模块固定发送帧类型（驱动当前按模块固定类型发送）。它是**初始化配置接口**：TX 队列非空、`bHeadPending`、或 `pfIsPending()` 非 0 时返回 0；**未决检查不以"已初始化"为前提**——模块重建失败后会保留未决标记，此时 `IsInitialized` 可能已是 0，但那条"已提交、结果未知"的报文仍在，改校验规则会让它与新规则混淆。
   拒绝的帧不进队列、不参与队头裁定，避免"请求内容与实际上总线的报文不一致"。
4. **裁定接口负责解冻**（选定方案，不留两种解释）：

```c
Uint16 CanPortResolvePendingHead(Uint16 u16CanId, Uint16 u16Action);
/*  前置：pfIsInitialized() != 0 且 pfIsPending() == 0（驱动已恢复），否则返回 HW_CAN_BUSY
 *  DROP ：CanQueuePop() 丢弃队头，清 bHeadPending
 *  RETRY：保留队头，清 bHeadPending，下一轮重新提交（提交前仍做忙检查）
 *  前置满足时**同时清除 bTxFrozen**；前置不满足时两者都不动
 */
```

4. `CanPortClearTxFreeze()` 只清 `bTxFrozen`；`bHeadPending == 1` 时返回 `HW_CAN_BUSY` 且不生效。

**3.5.3 两种恢复分开**

| 恢复类型 | 调用 | 端口层动作 |
| --- | --- | --- |
| 模块重建（`Hw_CanRecoverModule` 返回 `OK`） | `CanPortOnModuleRecovered(canId)` | 清 `bRxFrozen`、清本对象生命周期的覆盖锁存 `bHwOverwriteSeen`；**不动 `bHeadPending`/`bTxFrozen`** |
| 普通 Bus-Off 恢复（`Hw_CanRecoverBusOff` 返回 `OK`） | `CanPortOnBusOffCleared(canId)` | 只清 `bRxFrozen`；**不清覆盖锁存**（对象没有重新初始化） |
| 存在未决事务（`Hw_CanIsPending()==1`） | 上层 | 必须等 `Hw_CanRecoverModule()` 成功后才恢复服务 |

- 端口层不自行清除任何冻结标志；RX 冻结的清除者是上表对应的两个回调。
- **不提供 `CanPortClearHwOverwriteSeen()` 这类单独清除接口**：覆盖锁存的语义是「本次对象初始化以来发生过覆盖」，只能随模块重建一起清（05A 早期版本有该接口，已删除以免绕过语义）。

**3.5.4 队列容量**

每模块 RX/TX 各 16 物理槽（有效 15 帧），单元 `ST_CAN_FRAME`（12 字）；满时整帧拒绝。

### 3.6 SCI 发送服务（按前缀 Drop）

SCI 是字节流，队头可能只送出一部分：

```
本轮从队头连续送入 FIFO 的前缀 k 字节（k ≤ 16，且受 FIFO 空位限制）
   → RingBuffDrop(stTxRing, k)       /* 只释放真正进 FIFO 的部分 */
   → 下一次从新的队头继续
   → 若 k == 0（FIFO 全满），本轮结束，u16TxStall 计数 +1（诊断用）
```

例：队头串 8 字节，第 4 字节遇 `TryWrite` 返回 0 → 本次 `Drop(3)`，剩余 5 字节成为新队头，下一轮从第 4 字节继续。

- 端口层不感知"整帧"概念；"整帧原子入队"由协议层在写 TX 环时保证（见 §3.8 的容量限制）。
- `Hw_SciTryWriteByte()` 成功只代表"进 FIFO"；发送完成的判定不属本步。

### 3.7 协议边界

| 协议 | 需要的信息 | 端口层提供 | 协议层义务 |
| --- | --- | --- | --- |
| Modbus RTU | 字节序列、帧内/帧间静默、流完整性 | 字节环 + 批次记录（序号/计数/时标）+ `bStreamBreak` | 用 `gap` 判 t1.5/t3.5；流中断时丢弃候选帧并重同步 |
| Xmodem | 字节序列、块超时 | 同上；块缓冲自备（1024 字节，不要求环装下整块） | 自行管理块内位置与重试 |
| 示波器 | 采样序号、时标、通道掩码 | CAN 帧环（不含时标） | 记录环自带 `u32Seq`/`u32Tick`，与帧环分离 |

### 3.8 容量与资源核算（按目标 `sizeof`）

| 队列 | 物理槽 | 有效容量 | 单元长度 | 申请估算（字） |
| --- | --- | --- | --- | --- |
| SCI-A RX | 64 | 63 字节 | 1 | 64 |
| SCI-C RX | 256 | 255 字节 | 1 | 256 |
| SCI-D RX | 128 | 127 字节 | 1 | 128 |
| SCI-A/C/D TX | 3 × 128 | 127 字节/路 | 1 | 384 |
| SCI 批次记录 | A 16 / C 32 / D 16 | 15 / 31 / 15 条 | `sizeof(ST_SCI_RX_SEG)`（估 7~8） | 448 ~ 512 |
| CAN-A/B RX | 2 × 16 | 15 帧/模块 | `sizeof(ST_CAN_FRAME)` = 12 | 384 |
| CAN-A/B TX | 2 × 16 | 15 帧/模块 | 12 | 384 |
| **队列合计** | | | | **2,048 ~ 2,112** |
| 5 个端口结构体 | | | `sizeof` 实测 | 待测（估算 > 60） |
| 协议层 256 字节帧缓冲（按需，单列） | | | 256 | +256 |

- **单元长度一律取目标 `sizeof`，不得硬编码**；`Uint32` 成员要求偶字对齐，段记录实际可能是 8 字而非 7 字，64 个段槽最多比估算多 64 字。
- 端口结构体不能"约 60 字"估算：SCI 侧 3 个实例各含 3 个 `ST_RING_BUFF`（每个含指针、掩码、单元长度、head、tail、rejected），再加统计与指针；CAN 侧另有队列控制结构。**必须先完整实例化，再用目标 `.map` 判断是否需要调整槽数**。
- 按当前估算（队列 2,080 + 结构体 ~120）约 2,200 字——**这是 v0.4 的风险记录，已被后续实测取代**：v0.7 起 `RAMGS0` 实测 0xFCC/0x1000、余 52 字。
  > **历史方案（已废弃）**：v0.4 曾建议"超限时把 SCI-C 段环降到 16 槽"；该方案在 v0.5 审阅时被否决——16 槽只有 15 条有效记录，覆盖不了约 2 ms 消费延迟。现行裁定为 **SCI-C 段环保持 32 槽**；若需腾空间，优先评估 CAN TX 槽数或把协议缓冲移到 `RAMGS1` 起的其它分段，不得回头缩减段环。
- **128 槽 TX 的有效容量是 127 字节**：不承诺任意最大帧原子入队；256 字节帧由协议层持独立缓冲，按 §3.6 前缀语义分段送入。
- 最终数值以目标构建的 `sizeof` 与 `.map` 为准，本表为申请值。

### 3.9 05A 的桩驱动契约

05A 不接硬件，桩驱动必须能注入以下四类行为（对应 §5.1）：

| 能力 | 桩函数 | 用途 |
| --- | --- | --- |
| 预置 RX 字节队列，按批返回 | `pfTryRead` 桩 | 模拟"前台一次读到两帧"、帧内长间隔 |
| 控制 FIFO 剩余量 | `pfTryRead` 桩的"读满配额仍非空" | 触发流中断 |
| 预置 TX 忙碌前缀 | `pfTryWrite` 桩（第 k 字节返回 0） | 验证前缀 `Drop(k)` |
| 粘滞 `MsgLst` | `pfReadRx` 桩（连续 3 次返回 MsgLst=1） | 验证只锁存一次 |
| `UNKNOWN` 注入 | `pfSend` 桩返回 `HW_CAN_UNKNOWN` | 验证冻结 + 队头待裁定 |

### 3.10 需要的驱动入口变化（05B）

| 变化 | 目的 | 影响面 |
| --- | --- | --- |
| `ST_SCI_CFG_ITEM` 增加 `u16DataFormat`、`u16RxFifoLevel`、`bRxIntEnable` | 格式可配；中断方案可选 | 配置表补 3 个字段与默认值 |
| 新增 `Hw_SciGetRxStatus(u16SciId, ST_SCI_RX_STATUS *pstStatus, Uint16 bClear)` | 错误分类（帧错/奇偶错/溢出）与确认语义 | 只新增入口；`Hw_SciGetRxError()` 保留 |
| 新增 `Hw_SciGetTxFifoLevel(u16SciId)` | TX 服务判断还能写多少 | 只读 |

CAN 侧 05B 不需要新增入口。

---

## 4. 文件改动（按阶段）

### 4.1 05A 新增/修改

| 文件 | 动作 | 说明 |
| --- | --- | --- |
| `Platform/Common/Inc/RingBuff.h`、`Src/RingBuff.c` | 新增 | 采用已评审实现 |
| `Platform/Common/Inc/CanQueue.h`、`Src/CanQueue.c` | 新增 | `ST_CAN_FRAME` 整帧队列 |
| `Platform/Port/Inc/SciPort.h`、`Src/SciPort.c` | 新增 | 3 实例、RX/TX 服务、批次记录、流中断 |
| `Platform/Port/Inc/CanPort.h`、`Src/CanPort.c` | 新增 | 2 实例、RX/TX 服务、覆盖锁存、队头待裁定 |
| `tools/build_minimal.py` | 修改 | `SOURCES` 增 4 个 .c；`INCLUDES` 增两个目录 |
| `.cproject` | 修改 | 两套配置各加两个 include 路径 |
| `docs/review/steps/05-port-layer.md` | 修改 | 本文件，回填实施记录 |

**05A 对 `main.c` 只加两行**：`PlatformInit()` 里调用 `SciPortInit()`/`CanPortInit()`，目的是让端口层的静态存储真实进入 `.map`（否则链接器会把未被引用的段整段丢弃，内存账失真）。没有注册 `PortTask()`、没有调用任何 Service、没有使能 SCI/CAN 中断——驱动接入仍属 05B。

### 4.2 05B 新增/修改

| 文件 | 动作 | 说明 |
| --- | --- | --- |
| `Drivers/inc/Hw_SciDriver.h`、`Src/Hw_SciDriver.c` | 修改 | §3.10 三个入口 + 配置字段 |
| `Broad/src/DrvCfg_EP15K.c`、`drv/inc` | 修改 | SCI 配置补默认值 |
| `main.c` | 修改 | `PlatformInit` 初始化端口层；`TaskCreate` 注册 `PortTask`（先于采样任务）；`Isr_Heartbeat` 末尾调用 `SciPortService100us()` |
| `tools/build_minimal.py`、`.cproject` | 修改 | 如 05A 已改则确认；无新增源文件 |

### 4.3 接入顺序（05B）

```
PeripheralsInit:  时钟 → SCI 驱动 → CAN 驱动（现状）
PlatformInit:     Task_Init → SciPortInit/CanPortInit（绑定真实驱动表）
TaskCreate:       Task_Start(PortTask, 1U, 0U)   ← 先于采样任务
StartSystem:      端口就绪检查（失败不改 bReady，仅置错误位）
Isr_Heartbeat:    … → SciPortService100us() → Hw_CpuTimerClearIntFlag 之后
```

---

## 5. 验收

### 5.1 05A：静态核对与桩驱动（主机）

桩只提供真实驱动能看到的信息（`pfTryRead` 返回 1/0 与一个字节、`pfSend` 返回状态码、`pfTryGetTick` 返回状态与计数），不得提供到达时刻、帧边界等硬件给不出的信息。

| ID | 判据 | 方式 |
| --- | --- | --- |
| `S-05A-01` | 通用队列无覆盖写接口、无"生产者改 tail"路径 | grep + 审阅 |
| `S-05A-02` | 端口层不出现 `SciaRegs`/`CanaRegs` 符号；只经 `ST_*_PORT_DRV` 调用驱动 | grep |
| `S-05A-03` | 每轮 `pfTryRead` 调用 ≤ 8 次；无"读满后继续排空"；无第 9 字节探测；无 `while (FIFO 非空)` | 审阅 + 计数桩 |
| `S-05A-04` | **消费者延迟**：桩逐字节注入、ISR 端持续轮询 → 两帧对应的多个批次及其间的空读观察（`u32TickEmptyLast`/`u16EmptyRounds`）被完整保留；端口不输出静默时长结论 | 桩测试 |
| `S-05A-05` | **ISR 延迟**：桩一次提供两帧合并数据、中间无空读，或令服务间隔超限 → `bTimingBad == 1`；两帧不足 8 字节时也能触发（不依赖配额读满） | 桩测试 |
| `S-05A-06` | **部分发送**：桩在队头第 4 字节返回忙 → `Drop(3)`，剩余 5 字节成为新队头，下一轮从第 4 字节继续 | 桩测试 |
| `S-05A-07` | **粘滞覆盖**：桩连续 3 次返回 `MsgLst=1` → `bHwOverwriteSeen` 只置一次；其中一次令入队失败，锁存仍发生；**发送桩置忙**以隔离 TX，验证接收不动 TX 队列且队头保留 | 桩测试 |
| `S-05A-08` | **UNKNOWN 不自动重发**：`pfSend` 返回 `UNKNOWN` 后，即使驱动恢复，`PortTask` 仍不提交；仅 `CanPortResolvePendingHead()` 可推进，且它在前置不满足时返回 `BUSY`；裁定成功后 `bTxFrozen` 与 `bHeadPending` 同时清除 | 桩测试 |
| `S-05A-09` | 流中断三条件逐一可复现；`SciPortResync()` 在"保存→屏蔽→恢复"钩子下完成，且**屏蔽期间延迟投递的新故障不会被抹掉**（需测试钩子模拟屏蔽与延迟投递） | 桩测试 |
| `S-05A-10` | 有效容量断言：`RingBuffCapacity() == 槽数−1`；单元长度用 `sizeof` 且与 `RingBuffInitItems` 传入值一致（编译期断言） | 构建 |
| `S-05A-11` | RX `UNKNOWN` 后 `bRxFrozen` 保持 1；`CanPortOnModuleRecovered()` 清 `bRxFrozen` 与覆盖锁存且不动 `bHeadPending`；`CanPortOnBusOffCleared()` 只清 `bRxFrozen` | 桩测试 |
| `S-05A-12` | **时间源失败**：`pfTryGetTick` 返回 0 → SCI 不发布批次、置 `bTimingBad` 并作废基准；`PortTask` 整轮不服务且 `PortTaskTimeSourceFailures()` 加一；恢复后首轮只建基准（不产生间隔值），第二轮才计间隔 | 桩测试 |
| `S-05A-13` | 整批拒收：段环满或字节环不足时 **本批全部字节被丢弃并计数**，不出现"部分提交" | 桩测试 |
| `S-05A-14` | **段环先满**：15 条批次记录占满段环后，第 16 批即使字节环有空位也整批拒收并计数 | 桩测试 |
| `S-05A-15` | **待裁定队头**：非法动作返回 `ERR`、驱动仍在请求返回 `BUSY`、`RETRY` 只提交队头一次（帧号与首字节都对）、`DROP` 只删一帧、重复裁定不删下一帧 | 桩测试 |
| `S-05A-16` | **空读观察 `tick = 0`**：递减计数器回绕到 0 是合法时刻，`u16EmptyRounds != 0` 表明观察存在，0 不被当作「无」 | 桩测试 |
| `S-05A-17` | **流中断清除不掩盖新故障**：`SciPortClearStreamBreak()` 之后发生的新拒收仍置 `bStreamBreak` 并让 `u16RxRejected` 增加（计数器不被清除重置） | 桩测试 |
| `S-05A-18` | **`PortTask` 预算与公平**：预算充足服务 5 个单元；每次读时标推进 15 µs 时只服务 1 个单元且截断计数 +1；轮转起点按 A→C→D→CAN-A→CAN-B 推进；时间源失败整轮不服务 | 桩测试 |
| `S-05A-19` | **槽位到硬件 ID 映射**：`PortTaskUnitPortId()` 依次返回 A/C/D/A/B 且越界返回 `0xFFFF`；一次 `PortTask()` 后 A/C/D 三路 TX 日志各 3 字节、未用 SCI-B 的 ID 无写入 | 桩测试 |
| `S-05A-20` | **发送前查未决**：模块 `pfIsPending()` 非 0 时不发起发送事务（`u32TxPendingBlock` 计数 +1），帧留在队列 | 桩测试 |
| `S-05A-21` | **接收错误每轮检测**：驱动报错但 FIFO 为空时也置 `bStreamBreak` 并计数；清除错误标志后不再误计数 | 桩测试 |
| `S-05A-22` | **预算下沉**：单次接收消耗 30 µs 时，一轮内只发起 1 次接收、不发起发送，且截断计数 +1 | 桩测试 |
| `S-05A-23` | **重同步跨轮丢弃**：残留 64 字节 → `Resync()` 丢 32 后仍 `draining`；再一轮仍 `draining`；第三轮读到空才结束；期间不发布批次、流中断保持 | 桩测试 |
| `S-05A-24` | **故障代次**：旧代次清除返回 0 且故障保留；当前代次清除返回 1；清除后新故障仍能重新置位 | 桩测试 |
| `S-05A-25` | **帧类型**：标准口拒绝扩展帧与 RTR 帧并分别计数；标准帧接受；扩展口反之 | 桩测试 |
| `S-05A-26` | **时间故障锁存**：本轮第 3 次读取失败后即使时间源恢复，后续端口也不再服务（`LastUnits()==1`、C/D 无发送）；下一轮开窗后恢复为 5 个单元 | 桩测试 |
| `S-05A-27` | **排空未完成不得确认**：`draining` 期间 `ClearStreamBreak()` 返回 0 且流中断保持；排空后旧代次仍返回 0；当前代次返回 1 并清除；清除后新故障重新置位且代次变化 | 桩测试 |
| `S-05A-28` | **帧类型设置限定**：空队列可设置；队列非空时返回 0 且已有规则不变；驱动有未决事务时返回 0；**未初始化 + 未决 → 0，未初始化 + 无未决 → 1** | 桩测试 |

### 5.2 05B：上板（步骤 04 板项通过后）

| ID | 判据 |
| --- | --- |
| `S-05B-01` | 三路 SCI 各跑递增字节流 10 分钟，无丢字节、无乱序、`bStreamBreak` 保持 0 |
| `S-05B-02` | 三路空闲时，单路 SCI RX 服务 ≤ 0.4 µs（GPIO33 打点） |
| `S-05B-03` | 三路同时积压时，三路合计 RX 服务 ≤ 1.5 µs（上板目标，实测回填） |
| `S-05B-04` | 两类错误分开：①硬件 FIFO 溢出 → `bStreamBreak` 置位（`u16RxRejected` 不变）；②字节环不足 → 整批拒收、`u16RxRejected += count` 且 `bStreamBreak` 置位。分别记录最坏耗时 |
| `S-05B-05` | 整个 100 µs 心跳 ISR ≤ 10 µs；20 kHz 控制 ISR 响应延迟/抖动在加入搬运前后对比，增量 ≤ 1 µs |
| `S-05B-06` | 按端口统计 `u32MaxGap` ≤ 2 ms（含轮转被跳过的端口）；`PortTask` 的 20 µs 是"**超出即停止发起后续调用**"的轮次预算，不是整轮耗时上限——实际整轮耗时与超出量单独记录；20 kHz 主中断计数仍每秒 +20000 |
| `S-05B-07` | CAN 连续 1000 帧无丢失；两项诊断分别验证：①硬件对象未读即被覆盖 → `bHwOverwriteSeen`；②队列满但硬件及时读取 → `u32SwDrop` |
| `S-05B-08` | 人为 `UNKNOWN`：冻结、队头待裁定；恢复后不自动重发；`ResolvePendingHead` 在驱动恢复前返回 `BUSY`，恢复后同时解冻；RX 侧只由两个恢复回调清冻结 |
| `S-05B-09` | Modbus 长帧（≥128 字节）分段边界线上间隔实测，重点看 SCI-C 在前台服务延迟时能否连续供数 |

---

## 6. 风险与裁定

### 6.1 风险

1. **心跳内新增搬运**：步骤 03 刚验证心跳时基，05B 会在同一 ISR 内增加搬运，必须用 `S-05B-05` 重新标定整个 ISR。
2. **115200 裕度最小**：SCI-C FIFO 排空裕度 1.39 ms，且段环记录率最高；溢出必须经 `bStreamBreak` 可见。
3. **CAN 硬件只有 1 个接收对象**：满载必然覆盖；本步只做到"覆盖可见"，做不到"不丢"。
4. **内存余量偏紧**：按 `sizeof` 估算端口层约 2,200 字；`.map` 实测 `RAMGS0` 已用到 **0xFCC/0x1000，只剩 52 字**（v0.7 起；v0.6 时为 0xF9E/98 字），再启用协议 256 字帧缓冲必然超限。**SCI-C 段环已裁定保持 32 槽**（16 槽覆盖不了约 2 ms 消费延迟），因此若确需腾空间，优先评估 CAN TX 槽数、或把协议缓冲移到未使用的 `RAMGS1` 起的分段，不得回头缩减段环。
5. **粘滞覆盖标志只能表达"发生过"**：`bHwOverwriteSeen` 无法定位具体帧，也不能给出次数；业务层必须用序号自行发现缺口。
6. **桩与真实驱动行为差异**：05A 的桩可能比硬件更宽容，05B 必须重跑 `S-05A-04`~`S-05A-09` 的核心场景。
7. **静默判读留给协议层**：端口只给观察时刻，服务周期抖动会直接抬升 `bTimingBad`；协议层若把不确定的间隔当成确定值，会误判帧边界——因此必须在判读前先查 `bTimingBad`/`bStreamBreak`。

### 6.2 已裁定项（2026-11-04）

| 项 | 裁定 |
| --- | --- |
| SCI 搬运上下文 | 100 µs 心跳有界搬运 RX，A/C/D 每端口每轮最多 8 字节；TX 在前台；1.5 µs 是三路 RX 合计的上板目标，可行性待测 |
| CAN 接收对象 | 每模块保留对象 5 一个；允许突发丢帧，提供覆盖证据；波形完整性后续用业务序号验证 |
| 环容量 | SCI RX 64/256/128 槽、CAN 每方向 16 槽作初值；SCI TX 128 槽取决于上层整帧入队还是分段写入，**不据此承诺接纳最大帧** |
| 目录 | `Platform/Port` 放 `SciPort`/`CanPort`；通用队列放 `Platform/Common` |
| TX 方案 | 三路 SCI TX 保留 128 物理槽（127 字节有效）；需要发 256 字节帧的协议实例自持独立帧缓冲，按实际接纳前缀分段送入环；该缓冲按需分配，一份 256 字，单列进内存账 |
| 发送进度 | 协议层按"环实际接纳的字节数"推进帧偏移；端口层按"FIFO 实际接纳的字节数"执行 `Drop(k)`；同一端口一帧未全部入环时其他帧不得插入；最后一字节进环后即可复用协议缓冲；**RS-485 换向须以最后一个停止位发完为依据**，不能用"进环"或"进 FIFO"代替 |
| SCI 错误分类 | 帧错、奇偶错、FIFO 溢出分别提供快照与确认语义，计数注明为"观察到的错误事件"；硬件溢出触发 `bStreamBreak`，软件字节环拒收才增 `u16RxRejected` |
| CAN 覆盖暴露 | `bHwOverwriteSeen` 先经端口诊断接口暴露；本步不增加 `stBringupStatus` 字段 |

### 6.3 仍待确认

无。v0.2 的三项待确认已在本轮裁定，见 §6.2。

---

## 7. 实施记录（05A，2026-11-05）

### 7.1 落地文件

| 文件 | 状态 | 说明 |
| --- | --- | --- |
| `Platform/Port/Inc/SciPort.h` | 新增 | 端口结构、批次记录、临界区钩子、配额与接口 |
| `Platform/Port/Src/SciPort.c` | 新增 | 三端口静态存储、100 µs 接收服务、`PopBatch()`、`Resync()` |
| `Platform/Port/Inc/CanPort.h` | 新增 | CAN 端口结构、冻结与待裁定状态、统计接口 |
| `Platform/Port/Inc/PortTask.h` / `Src/PortTask.c` | 新增 | 汇总服务：槽位→硬件 ID 映射、公平轮转、20 µs 轮次预算、时间源失败停止 |
| `Platform/Port/Inc/PortBudget.h` | 新增 | 轮次预算窗口 `PortBudgetBegin/End/HasLeft/Active`；实现在 `PortTask.c`，未开窗时恒为可用 |
| `Platform/Port/Src/CanPort.c` | 新增 | 每模块 16 帧 RX/TX；硬件覆盖锁存与未决冻结 |
| `Platform/Port/Test/port_stub_test.c` + `Test/shim/` | 新增 | 主机桩测试与四个驱动替身头，不进目标构建 |
| `Platform/Common/{Inc,Src}/RingBuff.*`、`CanQueue.*` | 新增 | 原样引入的队列组件（仅转 CRLF 与落位） |
| `tools/build_minimal.py` | 修改 | 源文件增三个（`SciPort.c`/`CanPort.c`/`PortTask.c`）、包含路径增 `Platform/Port/Inc` |
| `main.c` | 修改 | 仅加 `SciPortInit()` 与 `CanPortInit()` 两处调用与两个 include |

### 7.2 构建

| 项 | 结果 |
| --- | --- |
| 命令 | `python tools/build_minimal.py --cgt <ti-cgt-c2000_22.6.2.LTS>` |
| 编译单元 | 34（步骤 04 为 31；v0.7 未新增源文件，`PortBudget` 实现在 `PortTask.c` 内） |
| 警告/错误 | 零 warning、零 error |
| 产物 | `EP15K_minimal.out` **758,172 B**，sha256 `549728d2e68444eb94a2e0595a2a1959c7263eea899a167c355529b8b5ce97f0` |
| map | **96,082 B** |
| v0.8 → v0.9 差异 | 产物 −20 B：`CanPort` `.text` 0x3FA→0x3F2（未决检查去掉初始化前提，少一次调用） |

### 7.3 静态核对（实例化后的内存账）

| 项 | 数值 |
| --- | --- |
| `RAMGS0` 占用 | 0x676 → **0xFCC / 0x1000**，余 **0x34 = 52 字**（v0.8/v0.9 未新增静态量） |
| `SciPort.obj` | `.bss` 1,508 字（`.text` 0x470 = 1,136 字）；每端口新增 `u16RxErrorCount`/`u16TxBudgetStop`/`bResyncDraining`/`u16FaultGen` 共 4 字 × 3 |
| `CanPort.obj` | `.bss` 880 字（`.text` 0x3DC = 988 字）；每模块新增 `u32TxPendingBlock`/`u32TxBudgetStop`/`u32TxTypeReject`/`u32TxRtrReject`/`bTxExtId` 共 9 字 × 2 |
| `PortTask.obj` | **已进镜像**：`.text` 0x112、`.bss` 0xC、`.data` 6、`.const` 5（映射表）。原因：`CanPort.c`/`SciPort.c` 引用 `PortBudgetHasLeft()`，链接器不再丢弃该段 |
| CLA 消息 RAM | `CLA1_MSGRAMLOW` 4 字、`CLA1_MSGRAMHIGH` 9 字，未变 |
| 余量提示 | `RAMGS0` 只剩 52 字，05B 任何新增存储都必须先落到其它 RAMGS 段或先腾空间；不能再加端口层静态缓冲 |

### 7.4 偏离

1. `main.c` 增加两行初始化调用：纯为让静态存储真实进入 `.map`；没有调用 `SciPortService100us()`/`SciPortTask()`/`CanPortService()`，没有使能 SCI/CAN 中断。驱动接入属 05B。已核对 `Platform/Port/**` 源码不含任何 `SciaRegs`/`ScibRegs`/`ScicRegs`/`ScidRegs`/`CanaRegs`/`CanbRegs` 符号（`S-05A-02`）。
2. `PortTask()` 仍未在 `main.c` 注册 1 ms 调度；但由于服务层引用 `PortBudgetHasLeft()`，`PortTask.obj` 本版**已被链接进镜像**（v0.6 的"未进镜像"结论不再成立，见 §7.3）。
3. 桩测试新增 `Platform/Port/Test/shim/` 四个替身头，不进入目标构建。
4. **新增 `PortBudget.h`**（本轮为落实"预算下沉"引入）：把 20 µs 窗口开在 `PortTask()`，服务层每次驱动调用前查询；未开窗时 `PortBudgetHasLeft()` 恒为 1，因此 100 µs 心跳里的 SCI 接收不受影响。
5. `CAN_PORT_TX_QUOTA` 由 4 改回 **2**（与已裁定的"每轮 2 次尝试"一致）。
6. `CanPortInit()` 把两模块 `bTxExtId` 置 0（标准帧）；CAN-B 的扩展帧属性由 05B 从 `stCanCfgItem[].bTxExtId` 调 `CanPortSetTxFrameType()` 绑定时设置，05A 不猜板级配置。

### 7.5 未跑项

- **本轮执行结果以审阅方为准**：v0.8 交付包在审阅方主机上 `checks=149 fails=1`（唯一失败项为 `S-05A-07`：`CanPortServiceOne()` 同时服务 TX，发送桩默认成功使 TX 空闲量增加），AddressSanitizer + UndefinedBehaviorSanitizer 未报地址或未定义行为异常；审阅方在诊断副本中把发送桩置忙后为 `149/149`。本版（v0.9）按第四轮 2 项整改后**尚未获得执行结果**，状态记为"已执行，149 项检查、1 项失败（v0.8 包）／本包待复核"，不回填为通过。
- 桩测试（`Test/port_stub_test.c` + `Test/shim/`）用 `ti-cgt-c2000_22.6.2.LTS -O2 --define=PORT_CRITICAL_HOOKS -I<cgt>/include --compile_only` **编译通过（零 warning）**，但**未执行**：本机没有任何主机 C 编译器（gcc/clang/cl/tcc/zig 皆无，WSL 无发行版）。审阅方环境有 `/usr/bin/cc`，执行命令：

  ```
  cd Platform/Port/Test
  cc -DPORT_CRITICAL_HOOKS -std=c99 -Ishim -I../Inc -I../../Common/Inc \
     port_stub_test.c ../Src/SciPort.c ../Src/CanPort.c ../Src/PortTask.c \
     ../../Common/Src/RingBuff.c ../../Common/Src/CanQueue.c -o port_stub_test
  ./port_stub_test      # 期望最后一行 checks=N fails=0
  ```

- `S-05A-01`~`S-05A-28` 的通过判定同上，待主机桩执行结果回填 §5.1；预期最后一行 `checks=N fails=0`。
- 05B 另需单列"三路同时重同步"的耗时验收：恢复路径每端口每轮最多丢弃 32 字节、三路合计最多 96 次读取，不能沿用正常接收的 24 字节测试结论。

---

## 8. 审阅结论（由 Codex 填写）

- 结论：**05A 代码审阅及主机桩测试通过（2026-11-05，v0.9 交付包）**。
- 验证（审阅方直接运行交付包原文件，未修改源码）：

  | 验证项 | 结果 |
  | --- | --- |
  | ZIP 完整性 | 22 文件，CRC 通过 |
  | 主机 `-O2 -Wall -Wextra` | 编译无警告，`checks=151 fails=0` |
  | ASan + UBSan | `checks=151 fails=0`，未报告异常 |
  | 随包 `.map` 核对 | `RAMGS0` 使用 0xFCC、余 52 字；`CanPort` `.text` 0x3F2 |

- 条件（不阻塞代码结论）：本轮无新增代码返工项；仅回填两处旧内容——§3.8 中"缩减 SCI-C 段环"标注为历史方案（现行裁定 32 槽）、§6.1 风险 4 的余量由 98 字更新为 52 字。
- 范围说明：**目标工具链构建与板上运行不属于此次主机验证范围**；主机测试通过不替代 05B 的上板验收——
  仍需保留：真实临界区（`PORT_CRITICAL_HOOKS` 关闭、真中断屏蔽）验证、三路同时重同步最多 96 次读取的耗时测试、新增存储的分段安排。
- 历史记录：第一轮 `checks=94 fails=4` + ASan 越界（v0.6 包）→ 第二轮 `checks=125 fails=2`（v0.7 包）→ 第三轮 `checks=149 fails=1`（v0.8 包，`S-05A-07` 未隔离 TX）→ 第四轮通过（v0.9 包）。

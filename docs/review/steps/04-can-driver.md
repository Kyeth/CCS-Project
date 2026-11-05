# 步骤 04：CAN 驱动修正（申请）

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`（当前 HEAD；工作区含步骤 02、03 的未提交改动）
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：`S-04-01` ~ `S-04-19`（本步新编；v3 增补 `S-04-16`~`S-04-18`，v4 增补 `S-04-19`）
- 状态：代码审阅通过（2026-11-04）；构建与静态核对完成，`S-04-09`~`S-04-15`、`S-04-18` 待上板验证

---

## 0. 修订记录

### 0.1 v2（对应第一轮评审）

| 评审项 | v1 的问题 | v2 的处理 |
| --- | --- | --- |
| 1 | §2/§3.3 断言"第二次写 TXRQST 会给接收对象发远程帧" | 删除该结论。命令不含 `DIR`，仍是读方向，bit18 在此时的语义是"清 NewDat"。改为"两次事务之间存在误清随后到达帧 NewDat 的窗口"，并把修复明确为**改第一次命令**（读方向同时带 bit18），不是只删第二次写 |
| 2 | 命令数值写成 `0x7F`/`0xB6`，且 `TRANSFER_ALL` 混入 `MASK` | 全部按 `IFCMD` bits[23:16] 重新计数：原接收 `0x33`、新接收 `0x37`、原发送 `0xB4`/`0xB7`。删除 `MASK` 位（接收方向不需要搬掩码），删除 `ClrIntPnd`（不使能中断，`IntPnd` 不置位）。`0x7F`/`0xF7` 只作为"若要清 IntPnd / 若要写掩码"的备查值列出 |
| 3 | `MsgLst` 写成 `[11:8]`、可累计、`>= 2` | 改为 bit14 的**一位标志**，输出归一化 0/1；不清除即"粘滞历史状态"，不得逐次计为丢帧；`S-04-10` 改用报文序号查实际缺失 |
| 4 | 建议删除 `Hw_CanIsRxReady()` | 保留，改读 `CAN_NDAT_21`，位索引 `对象号-1`，32 位常量移位；`Hw_CanIsTxDone()` 同步改读 `CAN_TXRQ_21`，省掉 IF 事务 |
| 5 | 用 `CPU Timer1` 做超时基准 | 该方案作废（初始化时未运行、100 µs 回绕）。§3.6 改为"时间源须满足的 5 条特性 + 候选 Timer0/Timer2 + 实施前置确认清单"，并给出单次等待与整个 API 的时间预算。v2 当时的口径是"时间源未确认前不实施超时改造"，**v3 已定稿为 Timer0**（见 §0.2 第 5 行） |
| 6 | `UNKNOWN` 之后用 `IsTxDone()==1` 判为"已发送" | 删除。改为"事务未决、对象与 IF 暂不复用、由协议层用序号/应答判定"，并写明 `TxRqst=0` 的三种其他成因。参数错误归 `ERR`，不归 `BUSY` |
| 7 | §3.1"过渡值等价现状"、标准帧分支不设 MXtd、`u16Dlc` 语义、波特率检查、初始化返回码与运行期返回码冲突、`unError.bit` | 逐条修正：明确记录接收范围的行为变化；标准帧分支同样启用 MXtd；`u16Dlc` 改名 `u16InitDlc` 并声明不参与过滤；波特率分"总分频值 1~1024"与"BRP/BRPE 字段值"两级检查并加误差检查；初始化固定 0/1、运行期用状态码；`unError.Bits` |

### 0.2 v3（对应第二轮评审）

| 评审项 | v2 的问题 | v3 的处理 |
| --- | --- | --- |
| 1 | §3.2.2 把 BRP/BRPE 拆分写成 (u32Div - 1) >> 10 | 改为 BRP = n & 0x3F、BRPE = (n >> 6) & 0x0F（n = 分频值 - 1），与现有 Hw_CanConfigBitTiming()（Hw_CanDriver.c:168-170）一致；波特率 × 20 的溢出检查前置到乘法之前；静态验收补分频 65 → BRP=0/BRPE=1、分频 1024 → BRP=63/BRPE=15 |
| 2 | §3.3.1 表把“读 + 清 IntPnd”写成 0x77 | ClrIntPnd = bits[23:16] 的 0x08、MASK = 0x40。改为：清 NewDat 0x37、再清 IntPnd 0x3F、再搬掩码 0x77、两者都做 0x7F；本步只用 0x37 |
| 3 | 裁定“仅数据帧”却把默认 bDirCmp 写成 0 | 接收对象固定 Dir=0、UMask=1、MXtd=1、MDir=1（bDirCmp=1）；删除“若有远程帧需求再开 bDirCmp”；RTR 支持另行定义，验收分别检查“不接收 RTR”与“不发出 RTR” |
| 4 | §3.7 只有原则，没有可执行恢复规则 | 新增 §3.7.4：TX 超时锁模块 IF1、RX 超时锁 IF2 并禁后续接收、未决期间不下发 IF 命令、诊断查询不解除未决、唯一恢复入口为 CTL.SWR 模块复位 + 重新初始化、模块复位会撤销同模块其他收发对象 |
| 5 | 时间源只有候选与确认表 | 定为 CPU Timer0 专用自由运行（不使能中断、时钟初始化后启动、PRD=0xFFFFFFFF、无重装、无符号差值、约 21.47 s 周期），Timer1 保持心跳；补“两等待不各得 10 µs”“检测阈值 ≠ 最坏返回时间”两条口径 |
| 6 | §3.3.3 写“自上次读取以来” | 改为“自上次由 CPU 清除此标志以来” |
| 7 | §1 写“用户已确认时基与计数正常” | 改为“步骤 03 构建及静态检查通过，板上验证待完成” |

### 0.3 v4（对应第三轮评审“有条件通过”）

| 项 | v3 的问题 | v4 的处理 |
| --- | --- | --- |
| 1 | §3.6.3 递减计数器差值写成 `now - start` | 改为 `(Uint32)(start - now)`；`Hw_TimeBaseTicks()` 明确返回**原始递减计数**，不做取反；补两个静态用例（1000/900 → 100、`0x00000005`/`0xFFFFFFFB` → 10）与验收 `S-04-19` |
| 2 | §4.8 时间源失败后仍会调用 `Hw_CanInit()` | 改为**前置条件闸门**：时钟成功、频率一致、时间基准成功，三者齐备才调用 CAN 初始化；时间基准失败直接跳过并置错误位；Timer0 初始化显式配置完整预分频字段并装载计数器后再启动 |
| 3 | §3.7.4 第 5 条“恢复后重新提交业务帧”与“UNKNOWN 禁止盲目重发”冲突 | 改为：模块恢复只恢复**驱动可用性**，不代表 UNKNOWN 帧没上总线；是否重发由上层按序号/应答/幂等规则决定，**驱动不得自动重发**；待收数据可能已丢失，已发出的帧无法撤回 |
| 4 | §3.6.4 只说“使用剩余预算”，未约束单次 | 明确每次等待取 `min(本阶段剩余, 整个 API 剩余)`；初始化总计 50 ms 同样共享预算 |
| 5 | `S-04-14` 未限定测试条件 | 限定为“无抢占测试条件”；含抢占的实际耗时按 `S-04-18` 单独记录 |
| 6 | Timer0 证据表用“步骤 03 Timer1 已工作”推断时钟已使能 | 删除该推断；改为**由 `Hw_TimeBaseInit()` 显式使能 `PCLKCR0.bit.CPUTIMER0` 并回读** |
| 7 | `S-04-07` 固定写“30 单元” | 改为“以实际构建单元数为准”，并确认 `Hw_TimeBase.c` 参与编译与链接 |
| 8 | §4.1 把 `Hw_CanGetStatus()` 列入运行期状态码 | 该函数继续返回硬件状态位，从状态码适用名单移除 |


---

## 1. 意图与边界

### 1.1 本步要做

| # | 文件 | 改动 |
| --- | --- | --- |
| 1 | `Drivers/inc/Hw_CanDriver.h` | 分对象接收配置类型、运行期返回码、`Hw_CanInit()` 返回值、`bExtId` 改名 `bTxExtId`、`Hw_CanIsRxReady()` 改寄存器查询、接收出参增加实际帧类型 |
| 2 | `Drivers/src/Hw_CanDriver.c` | 掩码位修正（MXtd/MDir 分离）、单次读事务、发送忙检查改用 `CAN_TXRQ_21`、IF 所有权整理、按时间预算的超时、未决状态与模块恢复入口、初始化错误上抛 |
| 3 | `Drivers/inc/Hw_TimeBase.h`、`Drivers/src/Hw_TimeBase.c`（**新增**） | CPU Timer0 自由运行时间基准：初始化、读数、无符号差值；不使能定时器中断 |
| 4 | `Broad/src/DrvCfg_EP15K.c` | `stCanCfgItem[]` 改为逐对象接收配置（过渡默认值、`bDirCmp=1`、每模块发送帧类型固定） |
| 5 | `main.c` | 时钟初始化之后、`Hw_CanInit()` 之前调用 `Hw_TimeBaseInit()`；用 `Hw_CanInit()` 返回值判定错误，写 `unError.Bits` |
| 6 | `docs/minimal-bringup.md`、规格书 §3.8 | 同步驱动契约、返回码表、超时预算与 Timer0 占用登记 |

### 1.2 本步不做

- 不使能任何 CAN 中断：`CAN_CTL` 不变、`CAN_IE0/CAN_IE1` 保持 0、对象 `MCTL.RxIE/TxIE` 保持 0。
- 不接环形缓存、`CanQueue`、端口层（`CanPort`），不写业务协议（并机点表、示波器分帧）。
- 不改 SCI 驱动、不改 20 kHz 控制 ISR、不改心跳（步骤 03 构建及静态检查通过，板上验证待完成）。
- 不做总线故障注入设备、不做波特率运行期重配置、不动 `CAN_ABOTR`。

### 1.3 边界理由

驱动修复与通信调度改造必须分开评审：本步只改"单次寄存器事务是否可靠"，运行期吞吐、队列、帧边界留给步骤 05。这样一旦上板出现收发异常，可以先排除驱动层。

### 1.4 当前 API 的使用面（决定破坏性变更是否安全）

`Hw_CanSend / Hw_CanIsTxDone / Hw_CanIsRxReady / Hw_CanReadRx / Hw_CanGetStatus / Hw_CanRecoverBusOff` 在全仓**没有任何调用者**（`grep` 只命中 `Hw_CanDriver.c` 内的定义行）。`main.c:85-90` 只调用 `Hw_CanInit()` 与 `Hw_CanIsInitialized()`。

因此本步修改运行期返回码、调整 `Hw_CanIsRxReady()` 的实现不会破坏现有行为，唯一需要同步的调用点是 `main.c`。

---

## 2. 现状核对

| # | 问题 | 现有代码 | 证据 |
| --- | --- | --- | --- |
| 1 | 扩展帧过滤把 MXtd 写成了 MDir | `Hw_CanDriver.c:213`：`u32Msk \|= HW_CAN_ARB_XTD;`，而 `HW_CAN_ARB_XTD = (1UL << 30)`（`:50`） | TRM `manual.txt:143155`、`:143181`：MSK 位 31 = MXtd、位 30 = MDir、位 29 保留。写 bit30 是"比较方向"，不是"比较扩展位"；且标准帧分支完全不设 MXtd |
| 2 | 接收对象不能逐个配置 | `Hw_CanDriver.c:345-364` 两个循环都用模块级 `u32RxAcceptId/u32RxAcceptMask/bExtId`；`:232` 对每个对象无条件置 `EOB` | 结构体只有一组模块级验收 ID/掩码；多对象 = 同一过滤条件 |
| 3 | 初始化失败无法上抛 | `Hw_CanConfigObj()` 返回 `void`（`:186`），结尾 `(void)Hw_CanWaitIf1Idle()` 丢弃结果（`:248`）；`Hw_CanSingleInit()` 返回 `void`（`:300`）；`Hw_CanInit()` 返回 `void`（`:373`） | 调用方只能事后用 `Hw_CanIsInitialized()` 猜 |
| 4 | 读与清标志跨两个事务 | `:578-581` 读命令 = `ARB\|CONTROL\|DATA_A\|DATA_B\|objNum`（命令字段 **0x33**，不含 bit18 → **不清 NewDat**）；`:630-631` 再写 `TXRQST\|objNum`（命令字段 0x04，不含 `DIR` → 读方向，bit18 此处的作用是**清 NewDat**） | 两条命令之间若新帧到达，会被第二条命令清掉 `NewDat` 而数据从未被读走。修复方向是把 bit18 并入**第一条**读命令，使其在传输完成时一次性读数据并清标志 |
| 5 | 发送不检查邮箱忙 | `Hw_CanSend()`（`:395-474`）只查初始化标志与对象号，直接改写 `IF1DATA/IF1ARB/IF1MCTL` | 上一帧未发完时改写，会让硬件发出"半新半旧"的帧 |
| 6 | 发送/TX 状态查询与 RX 抢 IF2 | `Hw_CanIsTxDone()` 用 IF2（`:476-509`）；`Hw_CanIsRxReady()` 用 IF2（`:511-543`）；`Hw_CanReadRx()` 用 IF2（`:545-636`） | 三个 API 共用同一组 IF2 寄存器；一旦接收移入中断就会互相覆盖。`CAN_TXRQ_21`/`CAN_NDAT_21` 可无 IF 读取（TRM `:142482-142530`、`:142596-142615`） |
| 7 | 超时是循环次数 | `HW_CAN_POLL_TIMEOUT (200000UL)`（`:68`），`Hw_CanWaitIf1Idle/Hw_CanWaitIf2Idle`（`:117-148`） | 循环次数与时间的关系随优化与流水线变化；200000 次没有换算依据，也无法在上板前复核 |

补充事实（本步只做最小调整）：

- `Hw_CanIsRxReady()`（`:511-543`）用"只读 CONTROL"判断 NewDat：读方向且 bit18=0，**不会**清 NewDat。真正的问题是它占用 IF2 且与随后的 `Hw_CanReadRx()` 之间存在"查到有帧、读时已被覆盖"的间隙。改读 `CAN_NDAT_21` 后不再占 IF2。
- `Hw_CanRecoverBusOff()`（`:664-689`）写 `INIT=1` 后用 `while ((CTL & INIT) == 0)` 等硬件确认，没有时间预算。

---

## 3. 设计裁定

### 3.1 接收对象

#### 3.1.1 数据结构

```c
/* Drivers/inc/Hw_CanDriver.h */
typedef struct
{
    Uint32 u32Id;       /* 标准帧取低 11 位；扩展帧取低 29 位 */
    Uint32 u32Mask;     /* 1 = 该位参与比较；0 = 不比较（全接收） */
    Uint16 bExtId;      /* 1 = 29 位扩展帧；0 = 11 位标准帧 */
    Uint16 bDirCmp;     /* 1 = 比较方向位（MDir）；接收对象按裁定固定为 1 */
    Uint16 u16InitDlc;  /* 对象初始化时写入 MCTL 的 DLC 字段；
                           不是"期望长度过滤"，收到帧的实际长度一律由 DLC 上报 */
    Uint16 u16Reserved;
} ST_CAN_RX_OBJ_CFG;
```

`ST_CAN_CFG_ITEM` 删除 `u32RxAcceptId / u32RxAcceptMask`，新增 `const ST_CAN_RX_OBJ_CFG *pstRxObjCfg;`（长度必须等于 `u16RxObjCnt`）。`bExtId` 改名为 `bTxExtId`，语义收窄为"**发送对象**的帧类型"，本步规定每个模块的发送帧类型固定。

#### 3.1.2 每个对象的属性表与本步的行为变化

| 段 | CAN 模块 | 对象 | ID | 掩码 | 帧类型 | 方向比较 |
| --- | --- | --- | --- | --- | --- | --- |
| 本步落地（过渡值） | CAN-A | 5 | `0x00000000` | `0x00000000` | **仅标准数据帧** | **1** |
| | CAN-B | 5 | `0x00000000` | `0x00000000` | **仅扩展数据帧** | **1** |
| 待所有者填写 | CAN-A/B | 5.. | 待定 | 待定 | 待定 | 待定 |

**这不是"行为等价于现状"，必须登记为可观察的行为变化**：

1. 现状扩展帧对象只设了 bit30（MDir），MXtd 未置位 → 硬件**不比较** `Xtd` 位，同一 ID 域的**标准帧也会被接收**。修正后 MXtd=1 → 只接收扩展帧。
2. 现状标准帧对象既不设 MXtd 也不设 MDir → **扩展帧也会被接收**。修正后标准帧对象同样置 MXtd=1 → 只接收标准帧。
3. **接收对象的验收配置由裁定固定**：`Dir=0`（接收）、`UMask=1`、`MXtd=1`、`MDir=1`（即 `bDirCmp=1`）。`UMask` 若为 0，四个掩码位整体不参与比较，`MXtd/MDir` 就形同虚设，所以 `bUseMask` 必须为 1。
4. 现状 bit30 恒为 1（对扩展帧对象）表示“比较方向位”，接收对象 `Dir=0`，等效要求报文方向位与对象方向一致；修正后该位由 `bDirCmp` 明确给出，并在配置表里固定为 1。
5. **删除“若有远程帧需求再开 bDirCmp”这类说法**。远程帧（RTR）的对象行为需单独定义（是否自动应答 `RmtEn`、用哪个对象应答、DLC 如何取），本步不涉及；验收分别检查“不接收 RTR”与“不会意外发出 RTR”两件事（`S-04-09`、`S-04-15`）。

因此 `S-04-08` 必须用一个**非零掩码的测试配置**验证"匹配接收 / 不匹配拒收"，零掩码配置只能验证"不拒收"。

真实 ID/掩码依赖对端设备协议，属于硬件事实，本步不替所有者决定（见 §6）。

#### 3.1.3 是否采用硬件 FIFO（本步：不采用）

1. 硬件 FIFO 链要求链内所有对象使用同一掩码，只在链尾对象置 `EOB`，与"每对象独立 ID/掩码"冲突。
2. 单深度对象溢出可由 `MCTL.MsgLst` 暴露（§3.3.3），配合报文序号可得到丢帧证据。
3. 当前每路只有 1 个接收对象、无消费者，吞吐不是本步风险点。

若后续示波器通道（12 路，1 Mbit）出现硬件覆盖，再单独申请 FIFO 链改造。

### 3.2 初始化

#### 3.2.1 返回值链

```c
Uint16 Hw_CanConfigObj(...);                      /* 1 = 成功，0 = 失败 */
static Uint16 Hw_CanSingleInit(...);              /* 1 = 该模块完整成功 */
Uint16 Hw_CanInit(const ST_CAN_CFG *pstCanCfg);   /* 1 = 全部模块成功，0 = 至少一个失败 */
```

**初始化层只有 0/1，不引入运行期状态码**（避免 v1 中 §3.2 与 §4.1 自相矛盾）。`main.c` 改为：

```c
if (Hw_CanInit(&stCanCfg) == 0U)
{
    stBringupStatus.unError.Bits.bCanInitError = 1U;
}
```

`Hw_CanIsInitialized()` 保留（供运行期判断），但不再作为初始化的唯一判据。

#### 3.2.2 事前合法性检查（任一不满足即返回失败，且不触碰寄存器）

| 检查 | 条件 |
| --- | --- |
| 模块号 | `u16CanId < HW_CAN_ID_MAX`，且同一配置数组内不重复 |
| 溢出（前置） | 先判 `u32Baudrate != 0` 且 `u32Baudrate <= (0xFFFFFFFFUL / HW_CAN_TQ_PER_BIT)`，再做 `u32Baudrate * HW_CAN_TQ_PER_BIT`；**不能先乘后查**（乘法已回绕时结果看起来合法） |
| 波特率（第一级） | 总分频值 `u32Div = u32SysclkHz / (u32Baudrate * HW_CAN_TQ_PER_BIT)` 必须落在 **1..1024**（`0` 说明超出分频能力，`>1024` 说明波特率过低），并检查实际波特率误差 |
| 波特率（第二级） | 设 `n = u32Div - 1`（0..1023）：`BRP = n & 0x3F`（低 6 位）、`BRPE = (n >> 6) & 0x0F`（高 4 位）。**`BRP` 不是 10 位字段**，6 位 `BRP` + 4 位 `BRPE` 合起来才是 10 位分频编码；这与现有 `Hw_CanConfigBitTiming()`（`Hw_CanDriver.c:168-170`）一致，不要改坏 |
| 波特率误差 | 实际波特率 `u32SysclkHz / (20 * u32Div)` 与目标相对误差 ≤ 0.5%，否则拒绝配置（寄存器里没有"期望值"，只能在配置期校验） |
| 发送区间 | `u16TxObjNum >= 1`、`u16TxObjCnt >= 1`、`u16TxObjNum + u16TxObjCnt - 1 <= HW_CAN_OBJ_MAX` |
| 接收区间 | 同上规则 |
| 区间不重叠 | 收发两个区间无交集 |
| 接收配置 | `pstRxObjCfg != NULL`；数组长度是调用方契约（裸指针无法验证），在头文件注释与配置表处双重声明"必须 ≥ u16RxObjCnt 项" |
| ID 位宽 | 标准帧 `u32Id <= 0x7FFUL`；扩展帧 `u32Id <= 0x1FFFFFFFUL`；掩码同样按帧类型限位（标准帧只允许低 11 位，扩展帧只允许低 29 位） |
| DLC | `u16InitDlc <= HW_CAN_DATA_LEN_MAX` |

#### 3.2.3 失败后的模块状态

| 失败点 | 模块留在什么状态 | 后续 |
| --- | --- | --- |
| 参数检查失败（进入 INIT 之前） | **未触碰寄存器**，保持复位/上一次状态 | `gCanInited=0`，可直接改参数重试；验收**不要求**此时模块处于 INIT |
| `Hw_CanEnterInit()` 超时 | 可能已置 `INIT=1`/`CCE=1`，也可能 RAM 初始化未完成 | 不清 `INIT`、不 `ExitInit`；`gCanInited=0`；下次 `Hw_CanInit()` 从 `EnterInit` 重来（幂等） |
| 位时序配置失败 | 停在 `INIT=1`（已进入初始化） | 同上 |
| 某个对象配置失败 | 停在 `INIT=1`（尚未 `ExitInit`） | 同上；不允许"部分对象配好就上线" |
| `Hw_CanExitInit()` 超时 | **`INIT` 是否已清除不确定** | 按"未初始化"处理：`gCanInited=0`，记录一次初始化故障，禁止上层使用该模块；下一次 `Hw_CanInit()` 重新 `EnterInit` 覆盖 |

一句话规则：**`gCanInited[module] = 1` 只在 `ExitInit` 成功且全部对象配置成功后置位**；任何失败路径都不置位，也不留下"已退出初始化但对象未配完"的中间态。

### 3.3 接收事务

#### 3.3.1 命令组合

```c
/* 命令字段 = IFCMD bits[23:16]。读方向（不含 DIR）同时带 bit18 = 传输完成后清 NewDat */
#define HW_CAN_CMD_READ_ALL  (HW_CAN_CMD_TXRQST | HW_CAN_CMD_ARB |      \
                              HW_CAN_CMD_CONTROL | HW_CAN_CMD_DATA_A |  \
                              HW_CAN_CMD_DATA_B)      /* 字段值 0x37 */

pstRegs->CAN_IF2CMD.all = HW_CAN_CMD_READ_ALL | (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);
```

命令字段对照（均指 bits[23:16]，写 `.all` 时位于高字节，例如 `0x37` → `0x00370000UL`）：

| 用途 | 组成 | 字段值 | 本步是否使用 |
| --- | --- | --- | --- |
| 原接收读 | ARB+CONTROL+DATA_A+DATA_B | `0x33` | 否（不清 NewDat） |
| **新接收读** | 上者 + `TXRQST`（bit18 = `0x04`） | **`0x37`** | **是** |
| 读 + 清 NewDat + 清 IntPnd | 上者 + `ClrIntPnd`（bit19 = `0x08`） | `0x3F` | 否（不使能中断，`IntPnd` 不置位） |
| 读 + 清 NewDat + 搬掩码 | 上者 + `MASK`（bit22 = `0x40`） | `0x77` | 否（接收方向不需要掩码） |
| 读 + 清 NewDat + 掩码 + IntPnd | 上者 + `0x40` + `0x08` | `0x7F` | 否 |
| 原发送（len>0） | DIR+ARB+CONTROL+TXRQST+DATA_A+DATA_B | `0xB7` | 保留沿用 |
| 发送并写掩码 | 上者 + `MASK` | `0xF7` | 否（本步不改运行期掩码） |

本步只用 `0x37`；`0x3F`/`0x77`/`0x7F` 仅作备查。注意 `0x77` 是“加了 MASK”而不是“加了 ClrIntPnd”——v2 表里这一行写错了。

顺序固定为：

1. `Hw_CanWaitIfIdle(&CAN_IF2CMD.all, 剩余预算)`：忙 → `HW_CAN_BUSY`（没有消费帧）。
2. 写 `IF2CMD`（上表 `0x37`）。
3. 等 `BUSY` 清零：超时 → `HW_CAN_UNKNOWN`（事务是否完成不确定，见 §3.7）。
4. 读 `IF2MCTL`：`NewDat == 0` → `HW_CAN_EMPTY`。
5. 读 `IF2ARB / IF2DATA / IF2DATB`，按 `DLC` 拆字节，从 `IF2ARB.Xtd` 取实际帧类型。

**删除** 现有的第二次写（`:630-631`）。三点说明：

- 该写不含 `DIR`，方向仍是读，**不会**在总线上发出远程帧；它的作用只是"置 bit18 → 清 NewDat"。
- 但它与第一次读是两次事务，中间到达的帧会被清掉 `NewDat` 而数据从未读出——这是本步要消除的窗口。
- 只删第二次写、不改第一次写会让 NewDat 永远不清（每次读都返回同一帧），所以两处必须一起改。

#### 3.3.2 输出参数与缓冲契约

```c
Uint16 Hw_CanReadRx(Uint16 u16CanId, Uint16 u16ObjIdx,
                    Uint32 *pu32MsgId, Uint16 *pu16Data, Uint16 *pu16Len,
                    Uint16 *pu16FrameType, Uint16 *pu16MsgLst);
```

- `pu16Data` 指向调用方提供的缓冲，**容量必须 ≥ `HW_CAN_DATA_LEN_MAX`（8）个字**；本函数最多写 8 个字。
- `pu16Len` = 实际 DLC（0..8）；`*pu16Data` 中 `[0, *pu16Len)` 有效，其余不动。
- `pu16FrameType` = `HW_CAN_FRAME_STD (0)` / `HW_CAN_FRAME_EXT (1)`，取自 `IF2ARB.Xtd`。
- 输出有效性：仅 `HW_CAN_OK` 时全部出参有效；`EMPTY/BUSY/ERR/UNKNOWN` 时**一个都不写**（调用方不能读到半新半旧的值）。
- 头文件注释必须写明：拿到 `HW_CAN_OK` 时硬件已清 `NewDat`，数据必须在本次调用内处理或拷走。

#### 3.3.3 MsgLst 的处理

`IF2MCTL.MsgLst` 是 **bit14 的一位标志**（bits[11:8] 依次是 `TxIE/RxIE/RmtEn/TxRqst`，不是 MsgLst 的一部分），语义为"自上次由 CPU 清除此标志以来，是否发生过消息被覆盖"，**不是次数**。

本步：

- 通过 `pu16MsgLst` 上报**归一化 0/1**（`(u32Mctl & HW_CAN_MCTL_MSGLST) != 0U`）。
- **不清除**（清它需要额外写事务）。因此它是**粘滞历史状态**：一旦置 1，在模块复位或人为清除前一直为 1。调用方不得"每次读到 1 就记一次丢帧"。
- 本步**不提供精确丢帧计数**；`S-04-10` 用报文序号查实际缺失，`MsgLst` 只作辅助证据。

### 3.4 发送事务

```c
/* 1) 目标邮箱忙则不改写任何数据寄存器：直接读 CAN_TXRQ_21，不占 IF1 */
if ((pstRegs->CAN_TXRQ_21.all & (1UL << (u16ObjNum - 1U))) != 0UL)
{
    return HW_CAN_BUSY;                      /* 上一帧未发完 */
}

/* 2) IF1 空闲 → 填 IF1DATA/DATB/ARB/MCTL */
if (Hw_CanWaitIfIdle(&pstRegs->CAN_IF1CMD.all, 剩余预算) == 0U) { return HW_CAN_BUSY; }
...

/* 3) 写方向整条提交（len>0 时字段值 0xB7，len==0 时 0xB4） */
pstRegs->CAN_IF1CMD.all = HW_CAN_CMD_WRITE_ALL | (Uint32)(u16ObjNum & HW_CAN_CMD_MSGNUM_M);
if (Hw_CanWaitIfIdle(&pstRegs->CAN_IF1CMD.all, 剩余预算) == 0U) { return HW_CAN_UNKNOWN; }
return HW_CAN_OK;
```

要点：

- 忙检查改用 `CAN_TXRQ_21`（bit N = 邮箱 N+1），省掉 v1 里的那次 IF1 读事务；位掩码用 `1UL << n`，不用 16 位 `1U`。
- 提交命令沿用现有 `0xB4`（`len==0`）/`0xB7`（`len>0`）字段值，**不加 `MASK`**：本步运行期不修改掩码寄存器，加了反而要求预先准备 `IF1MSK`，否则会把残留掩码写进对象。
- 忙检查在写数据之前，忙时不会留下半个新帧。
- `HW_CAN_OK` = "本地提交被接受"，**不表示**总线上发完。

### 3.5 IF 所有权

| 执行上下文 | IF1 | IF2 | 说明 |
| --- | --- | --- | --- |
| 发送提交：`Hw_CanSend()` | 独占 | 不使用 | 填数据 + 写命令 |
| 发送状态：`Hw_CanIsTxDone()` | **不使用** | **不使用** | 改读 `CAN_TXRQ_21`，纯寄存器 |
| 接收状态：`Hw_CanIsRxReady()` | 不使用 | **不使用** | 改读 `CAN_NDAT_21`，纯寄存器 |
| 接收读取：`Hw_CanReadRx()` | 不使用 | 独占 | 读事务 |
| 初始化：`Hw_CanConfigObj()` | 独占 | 不使用 | 仅在初始化上下文 |
| **未来的 CAN RX ISR** | 禁止 | **独占**（前台不再使用 IF2） | 步骤 05 移交后，主循环侧所有 IF2 访问必须删除或加互斥 |

规则：

1. 同一 IF 的"写命令 → 等 Busy → 读结果"必须在同一执行上下文内连续完成，中间不得被任何其他访问同一 IF 的代码打断。
2. 本步不使能中断，这条规则目前只靠调用顺序保证；但设计上**已经预留为"IF2 归 RX ISR 独占"**，而不是"ISR + 主循环共享"。步骤 05 接入 RX ISR 时，前台不再有任何 IF2 访问（`IsRxReady` 已改为寄存器查询），届时只需评审 IF1 是否也要中断化。
3. `Hw_CanIsTxDone()` 不再碰 IF，正是为了提前划清这条边界。

### 3.6 超时

#### 3.6.1 时间源必须满足的 5 条特性

1. `Hw_CanInit()` 调用之前**已经启动**（`main.c:85` 早于 `:91 Hw_CpuTimerInit` 与 `:124 Hw_CpuTimerStart`，现状 Timer1 在 CAN 初始化时既未初始化也未运行）。
2. 32 位自由运行，回绕周期 **≥ 1 s**（远大于任何单次等待预算，使"跨回绕"在工程上可忽略）。
3. 不产生中断、不被任何 ISR 复位或改写。
4. 读取是纯读，无副作用。
5. 不被其他模块用于别的用途（占用需登记）。

#### 3.6.2 v1 的 Timer1 方案为什么作废

| 问题 | 说明 |
| --- | --- |
| 初始化期未运行 | `Hw_CanInit()` 在 `main.c:85`，Timer1 到 `:124` 才启动；异常时 `TIM` 不变化，等待可能永不超时 |
| 回绕方向算错 | 从接近 0 处开始等待，可能只过很短时间就越过回绕点，却被按"一个满周期 100 µs"累计，立即击穿 20 µs 预算 |
| 跨多个回绕不可辨 | 被高优先级中断抢占超过 100 µs 时，两次 `TIM` 采样无法知道回绕了几次 |
| 预算口径错 | 单次等待 20 µs ≠ 整个 API 20 µs；一个 API 内多个等待会累加 |

#### 3.6.3 时间源：CPU Timer0（专用自由运行时间基准）

**工程核查结论**（本步已做，可复核）：

| 检查 | 结果 |
| --- | --- |
| 应用层是否初始化/启动 Timer0 | 否。`Hw_CpuTimerInit()` 的调用点只有 `main.c:91`（Timer1）；`CpuTimer0Regs` 在 `Broad/`、`Drivers/`、`Platform/`、`main.c` 中**零访问** |
| 是否注册 Timer0 中断 | 否。`PieVectTable.TINT0` 无引用；`HW_CPUTIMER_ID0` 只出现在 `Drivers/inc/Hw_CpuTimerDriver.h:27` 的枚举里 |
| 是否被 ADC/CLA 用作触发源 | 否。`HW_ADC_TRIG_CPU1_TINT0`（`Hw_AdcDriver.h:35`）、`HW_CLA_TRIG_TINT0`（`Hw_ClaDriver.h:82`）只是枚举值，配置表未使用 |
| 链接与寄存器定义 | 可用。`CMD/F2837xS_Headers_nonBIOS.cmd:40` 分配 `CPUTIMER0`（0x0C00，8 字），`:168` 有 `CpuTimer0RegsFile` 段 |
| 时钟门控 | 本工程未调用 `InitPeripheralClocks()`，`CpuSysRegs.PCLKCR0` 只被写过 `TBCLKSYNC`（`Hw_ClockDriver.c:18`、`Hw_PwmDriver.c:268/276/284`）。因此 **Timer0 的时钟不能依赖默认值**：`Hw_TimeBaseInit()` 必须显式置 `CpuSysRegs.PCLKCR0.bit.CPUTIMER0 = 1` 并回读确认 |

**设计**（新增 `Drivers/inc/Hw_TimeBase.h`、`Drivers/src/Hw_TimeBase.c`）：

1. Timer0 专用于自由运行时间测量，**不使能定时器中断**（`TIE=0`，不注册 `TINT0` 向量）。
2. 在 `Hw_ClockInit()` 成功之后、`Hw_CanInit()`（`main.c:85`）之前启动；返回 0/1。**返回 0 时 `main.c` 不得调用 `Hw_CanInit()`**（时间基准不可用 ⇒ 按时间判断的等待可能永不退出）。错误并入 `unError.Bits.bCanInitError`，不新增状态位。
   启动成功后置内部就绪标志（`Hw_TimeBaseIsReady()` 返回 1）；任何一步回读校验失败都清该标志并返回 0，后续所有按时间等待直接判 `HW_CAN_ERR`，不进循环。
3. 启动序列必须是“先配满、再装载、最后放行”：`TCR.TSS = 1`（先停）→ `TCR.TIE = 0`（不使能中断）→ `TPRH:TPR = 0`（预分频 = 1）→ `PRDH:PRD = 0xFFFF:0xFFFF` → `TIMH:TIM = 0` → `TCR.TRB = 1`（从 PRD 装载）→ `TCR.TSS = 0`（放行）。启动后**不再重装、不再清零**，读取是纯读。
4. 200 MHz、无分频下完整周期 = 2^32 / 200 MHz ≈ **21.47 s**。
5. **Timer0 是递减计数器**，`Hw_TimeBaseTicks()` 返回 `TIMH:TIM` 的原始值（不做取反）。经过时间用 **`(Uint32)(start - now)`**：递减计数下“经过”是起点减当前值，一次回绕被模 2^32 自然吸收。

   静态用例（`S-04-19` 逐条核对）：

   | start | now | 期望 elapsed |
   | --- | --- | --- |
   | `1000` | `900` | `100` |
   | `0x00000005` | `0xFFFFFFFB` | `10` |

   写反成 `now - start` 会得到接近 `UINT32_MAX` 的值，任何等待都会**立即判超时**。

```c
/* Drivers/inc/Hw_TimeBase.h */
Uint16 Hw_TimeBaseInit(Uint32 u32SysclkHz);   /* 启动 Timer0 自由运行；1 = 成功，0 = 失败（只调用一次） */

Uint32 Hw_TimeBaseTicks(void);                /* TIMH:TIM 原始递减计数，读后无副作用 */
Uint32 Hw_TimeBaseElapsed(Uint32 u32Start);   /* (start - now)，单位 SYSCLK 周期，可直接跨一次回绕 */
Uint16 Hw_TimeBaseIsReady(void);              /* 1 = 已成功启动且未复位；所有按时间等待的入口先查此项 */
```

Timer1 保持步骤 03 的心跳职责（100 µs 硬件中断 + 10 分频），两者互不影响。

**“周期长”不等于可以忽略回绕**：一次 20 µs 的等待也可能恰好跨过计数边界，所以差值必须写成无符号减法，且方向必须是 `start - now`（递减计数器）；v1 那种“先比较大小、回绕时按满周期累加”的写法正是错在这里。

#### 3.6.4 时间预算与验收口径

| API | 内部等待次数 | 单次等待上限 | 整个 API 预算 |
| --- | --- | --- | --- |
| `Hw_CanReadRx()` | 2（进入前 IF 空闲 + 提交后完成） | 10 µs | **20 µs** |
| `Hw_CanSend()` | 2（IF1 空闲 + 提交后完成） | 10 µs | **20 µs** |
| `Hw_CanIsTxDone()` | 0（纯寄存器） | — | 一条读指令 |
| `Hw_CanIsRxReady()` | 0（纯寄存器） | — | 一条读指令 |
| `Hw_CanConfigObj()` 每个对象 | 2 | 1 ms | 初始化总计 **50 ms** 上限 |
| `Hw_CanEnterInit/ExitInit()` 每步 | 2 | 10 ms | 同上 |

实现方式：进入 API 时取一次 `Hw_TimeBaseTicks()` 记下起点，并算出“整个 API 的 deadline”。后续每个等待的预算取两者较小值：

`min(本阶段剩余预算, 整个 API 剩余预算)`

单次检查只削减**整个 API 的剩余**，不能每个等待各自重新拿一份 10 µs。初始化同理：`Hw_CanConfigObj()` / `Hw_CanEnterInit()` / `Hw_CanExitInit()` 共用一个 **50 ms** 总预算，每个对象不各得 1 ms。

两条必须分开验收的量：

1. **共享 20 µs deadline 不等于每次各 10 µs**。“单次等待上限 10 µs”只是每次检查的约束，两次加起来仍是 20 µs；若要保留“单次 ≤ 10 µs”这项指标，必须在**每次等待结束处**单独测量（GPIO 翻转），不能由总时长反推。
2. **超时检测阈值 ≠ API 最坏实际返回时间**。等待循环发现超时的时刻可能晚于预算：被 20 kHz 控制中断抢占后，要等中断返回、重新进入循环才会重算并退出，实际返回可能超过 20 µs。验收必须分两项：①预算内返回值语义是否正确（`BUSY`/`UNKNOWN`）；②API 从进入到返回的**最坏实际耗时**（含被抢占），后者只能上板测。

#### 3.6.5 测量方法（GPIO33）

测量引脚用 **GPIO33**（`Broad/src/DrvCfg_EP15K.c:54`，配置表标注"测试引脚（1:高 0:低）"，方向为输出、初始低）。使用前需确认无其他写者、板上无冲突；**仅在测试构建中启用**，正式构建不翻转。

| 项 | 方法 |
| --- | --- |
| 正常路径 | 在 API 入口置高、出口置低，示波器看脉宽 |
| 超时路径 | 用**可控测试桩/故障注入**（见 §5.2 `S-04-13`），不靠"把预算改小"或断点 |
| API 总时长 | 同一次脉冲，验证不超 §3.6.4 预算 |
| 对 20 kHz ISR 的影响 | 与 `S-03-07` 同一窗口：`u32ControlIsrCount` 增量与 `u16ControlStallMs` |

### 3.7 返回码与"提交结果不确定"

#### 3.7.1 运行期返回码

| 返回 | 判定点 | 上层可否重发 | 是否消费/提交 |
| --- | --- | --- | --- |
| `HW_CAN_OK` (1) | 事务在预算内完成 | 不要 | 接收=已消费一帧；发送=已提交 |
| `HW_CAN_EMPTY` (0) | `NewDat == 0` | 稍后再查 | 否 |
| `HW_CAN_BUSY` (2) | 写命令**之前**失败（对象忙 / IF 忙） | 可以，无副产物 | 否 |
| `HW_CAN_ERR` (3) | 参数错误、模块未初始化 | **不可直接重试**，需先修参数 | 否 |
| `HW_CAN_UNKNOWN` (4) | 写命令**之后**等待完成超时 | **禁止按"未提交"重发** | 不确定 |

`ERR` 不并入 `BUSY`：参数错重试只会重复失败，而 `BUSY` 是"等一会儿就能过"。

#### 3.7.2 UNKNOWN 的处理规则

1. 发送 `UNKNOWN`：把该对象标记为**事务未决**（`u16TxPending`/`bTxUnknown`），在明确结束或受控恢复之前，不向该对象提交新帧、不复用 IF1。
2. **`Hw_CanIsTxDone() == 1` 不能证明"已发送"**：`TxRqst = 0` 也可能表示从未提交、请求被取消、模块被复位。它只能回答"当前没有未完成的发送请求"。
3. 业务结果由上层用协议序号/应答判定；本步只提供状态，不实现重发策略。
4. 接收 `UNKNOWN`：`NewDat` 可能已被清、数据可能没读全 → 记为"结果不确定/可能丢失"，**不得**计为"确定消费了一帧"。
5. 若要从 IF 寄存器恢复数据，必须保证该组 IF 未被其他事务覆盖（即同一上下文独占，见 §3.5）。

#### 3.7.3 `Hw_CanIsRxReady()` / `Hw_CanIsTxDone()` 的实现

两者都改为读 **32 位状态寄存器**，不占 IF：

```c
/* bit N = 邮箱 N+1，N = 对象号 - 1；必须用 32 位常量移位 */
#define HW_CAN_OBJ_BIT(u16ObjNum)  (1UL << ((Uint32)(u16ObjNum) - 1UL))

/* 有新帧：CAN_NDAT_21 */
return ((pstRegs->CAN_NDAT_21.all & HW_CAN_OBJ_BIT(u16ObjNum)) != 0UL) ? 1U : 0U;

/* 无未完成发送请求：CAN_TXRQ_21 */
return ((pstRegs->CAN_TXRQ_21.all & HW_CAN_OBJ_BIT(u16ObjNum)) == 0UL) ? 1U : 0U;
```

- `CAN_NDAT_21`（偏移 `0x9C`）覆盖**邮箱 1~32**（TRM `:142596-142615`），不是只覆盖 17~32；`CAN_NDAT_X`（`0x98`）才是按 8 邮箱分组。
- 查询结果是**瞬时状态**，不替代 `Hw_CanReadRx()` 内部的 `NewDat` 检查（两次调用之间仍可能变化）。

---

### 3.7.4 未决（UNKNOWN）的可执行恢复规则

| 项目 | 必须的行为 |
| --- | --- |
| TX 提交后超时 | 锁住**该模块的 IF1**（不是只锁一个发送对象）；此后任何需要 IF1 的事务一律拒绝 |
| RX 提交后超时 | 同样记录 **IF2 未决**；在此之前禁止发起下一次接收，否则现场被覆盖，连“读了一半的帧”都无法判定 |
| 未决期间再调用 | 直接返回未决状态，**不再下发任何 IF 命令** |
| 状态查询 | 允许读 `CAN_NDAT_21`/`CAN_TXRQ_21` 作诊断，但**不能**据此自动解除未决 |
| 恢复 | 见下面 5 条 |

本步采用最小可用策略：

1. **恢复入口唯一**：受控的模块复位 + 重新初始化（`CTL.SWR` 软复位 → `Hw_CanEnterInit()` → 重配全部对象 → `Hw_CanExitInit()`），封装为 `Hw_CanRecoverModule(u16CanId)`；不允许零散地“只复位某个对象”。
2. **停止访问的范围**：该模块**全部**收发对象与两组 IF，直到重置完成。
3. **成功条件**：重新初始化返回 1 且 `gCanInited[module] == 1`；此后才清除软件锁。
4. **失败后的状态**：继续保持未决与锁定，记录一次模块故障，等待上层走电源级恢复或下一次受控重试。
5. **恢复只恢复驱动可用性，不代表业务结果**：模块复位会同时撤销该模块上其他待发、待收对象；此前 `UNKNOWN` 的帧是否真的上过总线**仍然未知**。因此是否重新提交，由上层依据协议序号、应答、幂等规则决定，**驱动不得自动重发 `UNKNOWN` 帧**。待收数据可能已经丢失，已发出的帧无法撤回。

**IF 卡住时不能假设还能用同一个 IF 去“复位对象”**：`SWR` 是模块级寄存器写，不经过 IF；恢复流程必须走 `CTL.SWR`，而不是“再发一条 IF 命令去清对象”——后者在 IF 卡住时无从保证完成。

## 4. 修改前后（代码）

### 4.1 返回码

```c
/* 运行期状态码（Hw_CanSend / Hw_CanReadRx） */
#define HW_CAN_OK       (1U)
#define HW_CAN_EMPTY    (0U)
#define HW_CAN_BUSY     (2U)
#define HW_CAN_ERR      (3U)
#define HW_CAN_UNKNOWN  (4U)

/* 初始化层保持 0/1，不使用上面的状态码 */
Uint16 Hw_CanInit(const ST_CAN_CFG *pstCanCfg);   /* 1 = 成功，0 = 失败 */
```

`Hw_CanIsTxDone()/Hw_CanIsRxReady()` 保持 `1 = 成立 / 0 = 不成立`。
`Hw_CanGetStatus()` 继续返回硬件状态位（`ES` 等，语义由寄存器定义），**不套用上面的状态码**，因此不在该名单内。

### 4.2 掩码位

修改前（`Hw_CanDriver.c:207-216`）：

```c
if (bExtId != 0U) { u32Msk = u32Mask & HW_CAN_ID_MASK; u32Msk |= HW_CAN_ARB_XTD; }  /* bit30 = MDir，写错位 */
else              { u32Msk = (u32Mask & 0x7FFUL) << 18; }                            /* 不设 MXtd */
```

修改后：

```c
#define HW_CAN_MSK_MXTD  (1UL << 31)   /* 比较扩展标识符位 */
#define HW_CAN_MSK_MDIR  (1UL << 30)   /* 比较方向位 */

if (bExtId != 0U)
{
    u32Msk = (u32Mask & HW_CAN_ID_MASK) | HW_CAN_MSK_MXTD;
}
else
{
    u32Msk = ((u32Mask & 0x7FFUL) << 18) | HW_CAN_MSK_MXTD;   /* 标准帧同样比较 Xtd */
}
if (bDirCmp != 0U) { u32Msk |= HW_CAN_MSK_MDIR; }
```

`HW_CAN_ARB_XTD` 只保留给 ARB 寄存器，不再出现在 MSK 构造里。两个分支都置 MXtd，才能分别实现"仅标准帧"/"仅扩展帧"。
接收对象的 `MCTL.UMask` 必须为 1（`Hw_CanConfigObj(..., bUseMask = 1)`），否则 `MXtd/MDir` 不参与比较，“仅标准帧/仅扩展帧”的约束会静默失效。

### 4.3 ConfigObj 与 SingleInit

修改前：`Hw_CanConfigObj()` 返回 void、接收对象用模块级 ID/掩码、末尾丢弃等待结果。

修改后：返回 `Uint16`；接收对象逐个取 `pstRxObjCfg[i]`；`Hw_CanSingleInit()` 逐条检查返回值，任一失败即返回 0（保留 `INIT`）；`Hw_CanInit()` 汇总各模块结果。

### 4.4 ReadRx

见 §3.3.1：只保留一次读事务（命令字段 `0x37`），删除 `:630-631` 的第二次写；新增 `pu16FrameType`/`pu16MsgLst` 出参与 §3.3.2 的输出有效性规则。

### 4.5 Send

见 §3.4：忙检查改读 `CAN_TXRQ_21`（不占 IF1），提交命令沿用 `0xB4`/`0xB7`，删除 `MASK` 位；返回 `OK/BUSY/ERR/UNKNOWN`。

### 4.6 TxDone / RxReady

见 §3.7.3：都改为读 `CAN_TXRQ_21` / `CAN_NDAT_21`，不发起 IF 事务。

### 4.7 RecoverBusOff

保持现有流程，把 `while ((CTL & INIT) == 0)` 换成同一套带预算的等待（预算取 `HW_CAN_WAIT_INIT` 级别），超时返回 0。本步不改恢复策略本身。

### 4.8 main.c

`PeripheralsInit()` 内的顺序与闸门（时钟 → 时间基准 → CAN）：

```c
if (Hw_ClockInit(&stClockCfg) == 0U ||
    stSciCfg.u32LspclkHz != stClockCfg.u32LspclkHz ||
    stCanCfg.u32SysclkHz != stClockCfg.u32SysclkHz)
{
    stBringupStatus.unError.Bits.bClockInitError = 1U;
}

/* 时间基准：必须在任何按时间等待之前就绪 */
if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
    (Hw_TimeBaseInit(stClockCfg.u32SysclkHz) == 0U))
{
    stBringupStatus.unError.Bits.bCanInitError = 1U;   /* 时间基准不可用 ⇒ CAN 不得初始化 */
}

/* ... GPIO / PWM / ADC / CLA / SCI 初始化 ... */

/* CAN 只在时钟与时间基准都成立时初始化 */
if ((stBringupStatus.unError.Bits.bClockInitError == 0U) &&
    (Hw_TimeBaseIsReady() != 0U))
{
    if (Hw_CanInit(&stCanCfg) == 0U)
    {
        stBringupStatus.unError.Bits.bCanInitError = 1U;
    }
}
else
{
    stBringupStatus.unError.Bits.bCanInitError = 1U;
}
```

要点：

- **闸门在调用点**，而不是只置错误位后继续初始化：时间基准没起来时，`Hw_CanInit()` 直接不调用。
- 驱动内部再兜一层：`Hw_CanInit()`、`Hw_CanSend()`、`Hw_CanReadRx()` 等所有按时间等待的入口，先查 `Hw_TimeBaseIsReady()`；未就绪返回 `HW_CAN_ERR`（初始化返回 0），保证即使调用点漏判也不会死等。
- 因此 `Hw_TimeBase.h` 增加 `Uint16 Hw_TimeBaseIsReady(void)`。

（原代码是 `unError.Bits`，v1 文档误写为 `unError.bit`。）

---

## 5. 验收

### 5.1 静态（主机：构建 + 代码核对）

| ID | 判据 |
| --- | --- |
| `S-04-01` | MSK 构造里只出现 `HW_CAN_MSK_MXTD (1UL<<31)` / `HW_CAN_MSK_MDIR (1UL<<30)`；`HW_CAN_ARB_XTD` 不再出现在 MSK 赋值路径；标准帧与扩展帧两个分支都置 MXtd |
| `S-04-02` | `Hw_CanReadRx()` 内对 `IF2CMD` 只有**一次**写，且该命令含 bit18（字段值 `0x37`）；全文件不再出现对接收对象单独写 `TXRQST` 的第二条命令 |
| `S-04-03` | `Hw_CanSend()` 在写 `IF1DATA/IF1ARB/IF1MCTL` 之前有基于 `CAN_TXRQ_21` 的忙检查，忙时返回 `HW_CAN_BUSY` |
| `S-04-04` | IF 使用与 §3.5 一致：`Hw_CanIsTxDone/Hw_CanIsRxReady` 不出现 `CAN_IF1*/CAN_IF2*` 访问；`Hw_CanReadRx` 只碰 IF2 |
| `S-04-05` | 运行期路径不再出现 `HW_CAN_POLL_TIMEOUT`；所有等待都走“剩余预算”接口，时间源为 `Hw_TimeBaseElapsed()`；`Hw_TimeBase.c` 中 `TIE` 始终为 0、启动后不再写 `PRD/TRB` |
| `S-04-06` | `Hw_CanInit/Hw_CanSingleInit/Hw_CanConfigObj/Hw_TimeBaseInit` 都有返回值，失败路径不置 `gCanInited`；`Hw_CanInit` 与 `Hw_TimeBaseInit` 均返回 0/1；`main.c` 用返回值判定而非事后 `Hw_CanIsInitialized()` |
| `S-04-07` | 构建以**实际单元数**记录（不写死 30），零 warning；`Hw_TimeBase.c` 出现在编译单元清单与 `.map` 中（确认参与编译并链接）；`main.c` 使用 `unError.Bits`；`Hw_CanIsRxReady()` 仍存在且实现为 `CAN_NDAT_21` 查询 |
| `S-04-08` | 头文件注释含三张表：返回码语义（§3.7.1）、`Hw_CanReadRx` 输出有效性与缓冲容量（§3.3.2）、`MsgLst` 为粘滞标志（§3.3.3） |
| `S-04-16` | 分频编码边界：分频 65（n=64）→ `BRP=0`、`BRPE=1`；分频 1024（n=1023）→ `BRP=63`、`BRPE=15`。逐值核对 `BRP = n & 0x3F` / `BRPE = (n >> 6) & 0x0F`，不得出现 `>> 10` 的写法 |
| `S-04-17` | 波特率检查顺序：源码中 `u32Baudrate * HW_CAN_TQ_PER_BIT` 之前存在上界判断，并有分频值 1..1024 与 0.5% 误差检查 |
| `S-04-19` | `Hw_TimeBaseElapsed()` 方向正确：`start=1000/now=900 → 100`、`start=0x00000005/now=0xFFFFFFFB → 10`（主机可用等价常量用例核对同一表达式），源码中不出现 `now - start` |

### 5.2 上板（无法在主机验证，完成后回填）

| ID | 判据 |
| --- | --- |
| `S-04-09` | 过滤与帧类型（**另建非零掩码测试配置**：标准帧对象 ID=`0x123`、掩码=`0x7FF`、`UMask=1`、`MXtd=1`、`MDir=1`；扩展帧对象 ID=`0x18FF0000`、掩码=`0x1FFFFFFF`）：①匹配 ID → `HW_CAN_OK` 且数据一致；②不匹配 ID → 连续 1000 次 `HW_CAN_EMPTY`；③帧类型：CAN-A（仅标准）收到扩展帧应 `EMPTY`，CAN-B 反之；④**不接收 RTR**：对端发同 ID 远程帧，确认本端不把它当有效数据帧处理（行为按本步定义记录，不得静默收下） |
| `S-04-10` | 连续接收与覆盖证据：1 Mbit 连续发送 N 帧（帧内带序号），统计 `HW_CAN_OK` 次数、序号缺口，并记录 `MsgLst` 是否置 1（只作粘滞证据，不当作计数） |
| `S-04-11` | 忙邮箱不被覆盖：同一对象连续两次 `Hw_CanSend`（第一帧未发完），第二次返回 `HW_CAN_BUSY`，第一帧数据完整 |
| `S-04-12` | 初始化失败两条路径：①参数错误（如 `u16RxObjCnt` 越界）→ `Hw_CanInit()` 返回 0、`bCanInitError` 置位、**寄存器未被触碰**；②对象配置阶段注入失败 → 模块停在 `INIT=1`、`gCanInited=0` |
| `S-04-13` | 超时与未决恢复（可控测试桩/故障注入，不靠改预算或断点）：①正常完成 → `OK`；②IF 忙 → `BUSY` 且无副产物；③写命令后 `BUSY` 不清 → `UNKNOWN`，该模块 IF1/IF2 进入未决，未决期间再调用不再下发 IF 命令、诊断读取不解除未决；④按 §3.7.4 走 `CTL.SWR` 复位 + 重新初始化，成功后才清锁；⑤接收侧 `UNKNOWN` 记为“可能丢失”而非已消费 |
| `S-04-14` | 时间预算（GPIO33，仅测试构建，**无抢占测试条件**：屏蔽 20 kHz 控制中断或改为纯前台轮询场景）：记录每条 API 的实测脉宽（不设人为阈值），整个 API 不超过 §3.6.4 预算。本场景**不以停滞计数为判据**——控制中断被屏蔽时 `u16ControlStallMs` 必然增长，不能同时要求它为 0。**含抢占的实际耗时与控制中断停滞影响按 `S-04-18` 单独记录** |
| `S-04-15` | 不产生非预期远程帧（回归项）：CAN 分析仪确认读事务期间总线上无 RTR 帧。注意这是**回归测试**，不构成"旧代码必然失败"的结论——旧代码第二次写不含 DIR，本身也不会发远程帧 |
| `S-04-18` | 时间口径分离（正常抢占场景，20 kHz 控制中断照常运行）：①预算内超时检测的返回值语义正确；②API 最坏实际返回时间（含被抢占）单独测量并记录，允许大于检测阈值，但必须给出实测上限；③记录同一窗口内 `u16ControlStallMs` 的变化，确认 CAN 调用未把控制中断拖入停摆 |

---

## 6. 风险与裁定

### 6.1 风险

1. **API 破坏性变更**：运行期返回码重定义、`bExtId` 改名 `bTxExtId`。已核对无外部调用者，`main.c` 必须同步改。
2. **接收范围会变化**（§3.1.2）：修正 MXtd 后，标准/扩展帧不再互相串收。若对端设备实际混发，表现为"以前能收到、现在收不到"，需在 `S-04-09` 明确覆盖。
3. **`MsgLst` 不清除**：上报是粘滞的，不能当丢帧计数。
4. **Timer0 占用**：本步已核查应用层无占用（§3.6.3）；若 `device/` 支持库将来引入 Timer0 用途会冲突，`Hw_TimeBase.c` 与 `docs/minimal-bringup.md` 各登记一处占用声明。
5. **不改 `CAN_ABOTR`**：`bAutoBusOn = 1` 时恢复等待计数仍为复位值。
6. **`CAN_TXRQ_21` 位宽**：C28x 上 `1U << 31` 会落在 16 位整型，必须写 `1UL`，否则高 16 个邮箱的查询恒为假。
7. **模块级恢复的连带影响**：`CTL.SWR` 会清掉该模块所有对象，恢复流程必须被视为“该模块业务全丢”。但**驱动不替上层重发**：恢复成功只说明驱动可再用，`UNKNOWN` 的帧可能已经上过总线（也可能没有），重发会产生重复帧，必须由上层按序号/应答/幂等规则判定。

### 6.2 已裁定项（用户 2026-11-03 裁定）

| # | 项 | 裁定 |
| --- | --- | --- |
| 1 | 真实 ID/掩码未定 | 本步允许零 ID/掩码作联调过渡；**CAN-A 仅标准数据帧、CAN-B 仅扩展数据帧**；明确这不是完全保持原接收范围 |
| 2 | 超时测量引脚 | **GPIO33**（配置表"测试引脚"）为首选，确认无其他写者、板上无冲突后使用，**仅测试构建启用** |
| 3 | `bExtId` 仅用于发送 | 接受；改名 **`bTxExtId`**；本步明确每模块发送帧类型固定 |
| 4 | 删除 `Hw_CanIsRxReady()` | 不接受；保留并改读 `CAN_NDAT_21` |

### 6.3 仍需所有者确认

| # | 项 | 影响 |
| --- | --- | --- |
| 1 | 各路接收对象的真实 ID/掩码 | 决定 `S-04-09` 能否按真实对端验证（本步先用零掩码过渡） |
| 2 | 远程帧（RTR）是否出现在本系统 | 本步对象行为按“仅数据帧”配置；RTR 的对象级支持需另行定义 |

---

## 7. 实施记录（v4 落地，2026-11-03）

- 状态行：待审（代码已落地，构建与静态核对通过；板上项未测）
- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`（工作区未提交，head_sha 记"工作区待审"）

### 7.1 实际改动文件

| 文件 | 性质 | 说明 |
| --- | --- | --- |
| `Drivers/inc/Hw_TimeBase.h` | 新增 | 时间基准接口（Init/Ticks/Elapsed/IsReady/UsToTicks） |
| `Drivers/src/Hw_TimeBase.c` | 新增 | Timer0 自由运行，三读校验，递减差值 |
| `Drivers/inc/Hw_CanDriver.h` | 重写 | 逐对象过滤结构、状态码、时间预算接口、`RecoverModule` |
| `Drivers/src/Hw_CanDriver.c` | 重写 | 位时序校验、单事务读、`NDAT_21/TXRQ_21` 查询、未决与模块恢复 |
| `Broad/src/DrvCfg_EP15K.c` | 修改 | 新增 `stCanRxObjCfgA/B`，`stCanCfgItem` 改用 `pstRxObjCfg`/`bTxExtId` |
| `main.c` | 修改 | 启动时间基准并加闸门，CAN 初始化只在时钟与时间基准都正常时进行 |
| `docs/minimal-bringup.md` | 修改 | CAN 一行、错误位说明、板上核对项 |
| `EP15K_2026_配置核心与运行逻辑.md` | 修改 | §3.8.2 逐对象过滤与事务表、§3.9.1 Timer0、§4.1 第 11 步、§4.5 bit2、§4 限制第 5 条 |

合计 **6 个代码文件**（2 个新增、4 个修改）加 **2 个说明文档**。本步 diff 见同目录 `04-can-driver.diff`。

### 7.2 构建（已跑）

| 项 | 结果 |
| --- | --- |
| 编译单元 | 31（步骤 03 为 30，新增 `Hw_TimeBase.c`） |
| 警告 / 错误 | 0 / 0 |
| `EP15K_minimal.out` | 690,136 字节，sha256 `7cbbf05dc63b2cf8839d246edab5f4625753a20571a0128f4db19b1341f81ec5` |
| `EP15K_minimal.map` | 85,491 字节，sha256 `2de5383eed26e278bd167194b7957ea83596827a33bbfbb50a864cf5006b55b4` |

本表为第四轮代码审阅返工后的**最终构建**。历史版本：首次实施 685,152 字节（`9ac49c12…`）、仅改波特率判据 685,108 字节（`38e4fb78…`）、第二轮 689,512 字节（`637dc526…`）、第三轮 690,160 字节（`a519f78b…`）；本版把三个初始化等待也改为“先判时间源与两级预算、再接受硬件完成条件”。

构建清单无需改动：`tools/build_minimal.py` 与 `.cproject` 都按 `Drivers/src` 目录收集源文件，`Hw_TimeBase.c` 已自动参与编译（日志中出现 `compile Drivers\src\Hw_TimeBase.c`）。



链接结果核对：`Hw_TimeBase.obj` 已参与链接（`.text` 0x7C 字，`.data` 4 字）；`Hw_CanDriver.obj` `.text` 0x6B4 字、`.bss:gCanCfg` 0x1C 字；`CLA1_MSGRAMLOW` 仍 4 字、`CLA1_MSGRAMHIGH` 仍 9 字、`RAMLS5` 仍 2 字（CLA 契约未变）；`FLASH_APP` 0x3666 字（步骤 03 为 0x335B）；`RAMGS0` 0x672/0x1000。

### 7.3 静态核对（已跑）

| # | 核对项 | 证据 |
| --- | --- | --- |
| 1 | 接收只有一次 IF2 事务 | `Hw_CanDriver.c` 仅 `CAN_IF2CMD.all = (HW_CAN_CMD_READ_ALL \| objNum)`（命令字段 0x37），不再写第二条清标志命令 |
| 2 | `MXtd`/`MDir` 分离 | 宏 `HW_CAN_MSK_MXTD (1UL<<31)`、`HW_CAN_MSK_MDIR (1UL<<30)`；接收对象两者都置位 |
| 3 | 忙与完成查询不占 IF | `CAN_TXRQ_21` 用于发送前忙检查与 `Hw_CanIsTxDone`；`CAN_NDAT_21` 用于 `Hw_CanIsRxReady`；位序 `对象号-1`，掩码为 32 位常量 |
| 4 | 时间预算 | 每次等待取 `min(本阶段剩余, 整个API剩余)`；运行期 20 µs/10 µs，初始化 50 ms/10 ms |
| 5 | 未决与恢复 | 提交后超时置 `HW_CAN_PENDING_TX/RX`，未决期间直接返回 `HW_CAN_UNKNOWN`；`Hw_CanRecoverModule()` 用 `CTL.SWR` 重建，不自动重发 |
| 6 | 旧实现残留 | 全仓 `grep` 无 `HW_CAN_POLL_TIMEOUT`、`u32RxAcceptId`、`u32RxAcceptMask` |
| 7 | 启动闸门 | `main.c` 时间基准失败置 `bCanInitError`；CAN 段条件为 `bClockInitError==0 && bCanInitError==0 && Hw_TimeBaseIsReady()!=0` |

### 7.4 实现中的偏离（相对本申请 v4）

1. **两个等待助手而非一个**：当前 TI 编译器及 `byte_peripheral` 寄存器声明不支持将该成员地址作为普通 `volatile Uint32 *` 传递（实测报 `warning #169-D` 与 `error #99923: Illegal use of intrinsic: __byte_peripheral_32`），因此拆成 `Hw_CanWaitIf1Idle()`/`Hw_CanWaitIf2Idle()`，直接传寄存器结构体指针、各自读 `CAN_IF1CMD`/`CAN_IF2CMD`。两者保持相同的预算计算、超时判定与返回语义，只有寄存器读取分别实现。
2. **波特率误差判据改为无乘法形式**：用 `|实际-目标| <= 目标/200`（0.5%），不再用 `|实际-目标|*1000 <= 目标*5`。需要说明的是，旧实现还有一道前置门限 `|差| > 2000 即拒绝`，因此上一版文档所说“乘法会回绕”并不成立——回绕路径被那道门限挡住了。旧判据的真实问题是**误拒合法配置**：绝对门限 2000 从目标波特率 400 kbit/s 以上开始严于 0.5%（0.5% × 400,000 = 2000），波特率越高越紧，等于把“相对误差受控”变成了“绝对误差受控”。现形式两侧都不做乘法（`目标/200 ≤ 21,474,836`，差值不超过目标与实测值之和），且与波特率无关地保持 0.5%。主机按同一表达式复核：100k/250k/333333/500k/1M/5M 均通过且分频为 100/40/30/20/10/2；5,700,000 被判拒绝（差值 4,300,000 > 28,500）。
3. **内部返回码**：`Hw_CanEnterInit/Hw_CanExitInit/Hw_CanConfigObject` 内部返回 `HW_CAN_OK/BUSY/UNKNOWN`，对外 `Hw_CanInit()` 仍是 0/1。
4. **配置校验加严**：除申请所述项外，还拒绝重复模块、收发对象区间重叠、接收配置数组为空。
5. **未新增错误位**：时间基准失败复用 `bCanInitError`（`BringupStatus.h` 结构未改）。

### 7.5 静态项结果与未跑项

**静态项（已跑，2026-11-03）**

| ID | 实际结果 |
| --- | --- |
| `S-04-08` | 通过。`Hw_CanDriver.h` 含三张表：返回码语义、`Hw_CanReadRx` 输出有效性与 `HW_CAN_DATA_LEN_MAX` 容量、`MsgLst` 为粘滞标志（自上次由 CPU 清除以来） |
| `S-04-16` | 通过。`Hw_CanWriteBitTiming()` 内为 `BRP = 分频值-1 & 0x003F`、`BRPE = (分频值-1 >> 6) & 0x000F`，函数内无 `>> 10`；分频 65 → BRP=0/BRPE=1、1024 → BRP=63/BRPE=15，按同一表达式主机复核一致 |
| `S-04-17` | 通过。`Hw_CanCheckBitTiming()` 顺序：`u32Baudrate == 0` → `u32Baudrate > 0xFFFFFFFF/20`（乘法前上界）→ 乘法 → 分频 1..1024 → 0.5% 误差；末尾无任何乘法 |
| `S-04-19` | 通过（以等价常量在主机核对同一表达式）。`start=1000/now=900 → 100`；`start=0x00000005/now=0xFFFFFFFB → 10`；跨重装 `start=0x00000100/now=0xFFFFFE00 → 768`；`start=0xFFFFFF00/now=0xFFFFFE00 → 256`。源码 `Hw_TimeBaseElapsed()` 为 `(Uint32)(u32Start - Hw_TimeBaseReadRaw())`，全文件不出现 `now - start` |

**待上板（本步未测）**

- `S-04-09`~`S-04-15`、`S-04-18` 的全部测量：GPIO33 脉宽、抢占下耗时与控制中断停滞、非零掩码过滤与帧类型、忙邮箱不改写、未决恢复五条路径、长跑覆盖。
- Timer0 的**实际计数速率与硬件重装行为**需上板核实；差值算法本身已按同一表达式在主机验证（见 `S-04-19`），因此不再把“回绕算法”整体列为纯上板项。


### 7.6 第二轮代码审阅返工记录（2026-11-03）

用户在代码审阅中给出 7 项问题（1 项 P2、6 项 P1）。下表逐条对应到当前代码，行号按返工后的最终版本。

| # | 问题 | 处理 | 位置 |
| --- | --- | --- | --- |
| 1 | 发送忙检查与实际提交指向不同对象 | 统一计算 `u16ObjNum = u16TxObjNum + u16ObjIdx`，`CAN_TXRQ_21` 位检查、`IF1CMD` 提交、`Hw_CanReadRx` 的接收对象号三处同源 | `Hw_CanDriver.c:877`（发送）、`:696`/`:709`（初始化逐对象）、`:1077`（接收） |
| 2 | 标准帧掩码漏左移 18 位 | 改为 `u32Msk = (u32Mask & HW_CAN_MSK_STD_M) << 18U;`，并在配置校验阶段拒绝超宽 ID/掩码 | `Hw_CanDriver.c:388`、校验 `:550` 之后 |
| 3 | Bus-Off 恢复清空对象却未重建 | 新增 `Hw_CanEnterInitPlain()`：只置 `INIT` 并等其生效，不做报文 RAM 复位、不发 `SWR`；`Hw_CanRecoverBusOff()` 改用它，未决时直接返回 `BUSY`，退出初始化失败则置未决并返回 `UNKNOWN` | `Hw_CanDriver.c:474`、`:1227` |
| 4 | 模块恢复提前清锁，失败后无法再恢复 | 恢复门槛改为 `gCanCfgValid[]`，新增 `gCanRecovering[]` 防重入；恢复期间置 `gCanInited=0`，失败保留未决与配置有效（允许重试），只有重建成功才清 `gCanPending`；`Hw_CanInit()` 不再提前清未决，改为逐模块重建成功后清 | `Hw_CanDriver.c:1255`（`RecoverModule`）、`Hw_CanInit()` 尾部 |
| 5 | 配置校验缺项、区间可能回绕 | 起始号/数量各自限幅 1..32，用 `Uint32` 计算区间接尾并查重叠；逐接收对象校验 ID/掩码位宽、`u16InitDlc <= 8`、`bDirCmp != 0`；发送入口对超 29 位扩展 ID 返回 `ERR` | `Hw_CanDriver.c:550` 起（`Hw_CanCheckCfgItem`）、`:603`、发送入口 ID 校验 |
| 6 | 时间源未验证计数运行、读取失败仍返回拼接值 | `Hw_TimeBaseReadRaw()` 改为状态式：连续校验失败时锁存故障、清零就绪标志并返回 0，不再返回撕裂值；`Hw_TimeBaseInit()` 增加配置回读与计数自旋自检，失败置 `TSS=1` 并返回 0 | `Hw_TimeBase.c` 读取函数、`:108` 自旋段 |
| 7 | 共享预算存在被延后的窗口 | 等待助手改为按**绝对起点**分别判 API 预算与阶段预算（`Hw_CanBudgetOk`/`Hw_CanStageOk`），每次下发命令前再确认一次预算；初始化对象等待改为独立 1 ms 预算 `HW_CAN_INIT_OBJ_BUDGET_US`（与申请表一致）；另加 `HW_CAN_WAIT_SPIN_MAX` 硬兜底次数，仅防计数被外部停住时死等，不参与预算判定 | `Hw_CanDriver.c:160`、`:180`、`:203`/`:228`、`:98`、`:103` |

**返工后构建**：31 编译单元、0 警告 0 错误；`EP15K_minimal.out` 689,512 字节 sha256 `637dc52621cfa2dc686a57508d4d10dda75bc2b52aedd3c39a81b10dcda670f4`；`EP15K_minimal.map` 85,493 字节 sha256 `74df85cfec252dfb8eee6960f255c13c17a301d1061504e7c011cfe8927d3a45`。

**diff**：`docs/review/steps/04-can-driver.diff`（2,425 行 / 88,030 字节），顺序为 `Hw_TimeBase.h`、`Hw_TimeBase.c`、`Hw_CanDriver.h`、`Hw_CanDriver.c`、`DrvCfg_EP15K.c`、`main.c`；新增文件对空文件比较，已跟踪文件对 `HEAD`（`5bb641f`）比较，因此后两者含步骤 01~03 与会话前未提交改动。

**未做**：按用户要求驱动接入暂缓，未启用 CAN 中断、未接环形缓存、未写业务协议；`S-04-09`~`S-04-15`、`S-04-18` 的板上测量仍未执行。


### 7.7 第三轮代码审阅返工记录（2026-11-03）

复审确认：发送对象号统一、标准帧掩码左移、Bus-Off 保留对象、恢复失败可重试、配置校验加严五项已通过，本轮不再改动。剩余 3 项全部落在“时间源故障与等待预算”这条链上。

| # | 问题 | 处理 | 位置 |
| --- | --- | --- | --- |
| 1 | 阶段读取失败仍判成功 | `Hw_CanStageOk()` 先存阶段经过时间，读后复查 `Hw_TimeBaseIsReady()`，失败即按预算耗尽返回 0；`Hw_CanSend()`/`Hw_CanReadRx()` 在最后一次取时标后同样复查就绪，失败时**不下发命令**并返回 `ERR` | `Hw_CanDriver.c` `Hw_CanStageOk()`、发送/接收命令下发前 |
| 2 | IF 空闲时跳过预算与故障检查 | 两个等待助手把判定顺序改为“先查时间源与 API/阶段预算，再判 `BUSY` 位”，每轮成功返回前必然经过预算判定 | `Hw_CanWaitIf1Idle()`、`Hw_CanWaitIf2Idle()` |
| 3 | 停表兜底只覆盖 IF 等待 | `Hw_CanEnterInit()`（等 RAM 初始化完成）、`Hw_CanEnterInitPlain()`（等 `INIT` 置位）、`Hw_CanExitInit()`（等 `INIT` 清零）三处改为 `for` 循环 + `HW_CAN_WAIT_SPIN_MAX` 有限次数；预算到期返回 `BUSY`，次数耗尽返回 `UNKNOWN`（异常语义，不混同超时），调用方不再进入下一阶段；`Hw_CanRecoverBusOff()` 收到 `UNKNOWN` 时锁未决 | 同上三函数、`Hw_CanRecoverBusOff()` |

**主机桩（等价表达式复刻，未编译该源文件）**：8/8 通过

| 场景 | 复刻对象 | 结果 |
| --- | --- | --- |
| 阶段预算那次读取失败 | 旧顺序：预算通过、阶段差值当 0 → 成功；新顺序：`Hw_CanStageOk` 返回 0，`IsReady` 变 0 | 旧 1 / 新 0 ✔ |
| IF 已空闲但已过 100 tick（限额 20 tick） | `Hw_CanWaitIf1Idle` 新顺序先判预算 → 返回 0 | 0 ✔ |
| 时间源未就绪且 IF 空闲 | 同上，先判就绪 → 返回 0 | 0 ✔ |
| 停表（读数恒定、就绪仍为 1）且 `INIT` 永不置位 | `Hw_CanEnterInitPlain` 兜底 → `UNKNOWN` | `UNKNOWN` ✔ |
| 对照：`INIT` 正常置位 | 同上 → `OK` | `OK` ✔ |

桩脚本为一次性验证用，未入库。

**返工后构建**：31 编译单元、0 警告 0 错误；`EP15K_minimal.out` 690,160 字节 sha256 `a519f78b1f1855aac63f74aec41e0cf9a3eefa4e660177bf20c684ab4afb7de7`；`EP15K_minimal.map` 85,491 字节 sha256 `16e214cbffad1a56a1512a0aa07954807f1ab0b5d41bc702f5b0043e278bbbf1`。

**未做**：驱动接入继续暂缓（未启用 CAN 中断、未接环形缓存、未写业务协议）；`S-04-09`~`S-04-15`、`S-04-18` 的板上测量未执行。


### 7.8 第四轮代码审阅返工记录（2026-11-03）

复审确认：阶段预算读取复查、IF 等待先判预算、初始化等待有限次数兜底、收发最后一次取时标后的就绪检查均已通过。剩余一处：三个初始化等待函数**先判硬件完成条件、命中即 break**，因此完全跳过 `Hw_CanStageOk()`，存在一条不经预算与故障检查的成功路径。

| 项 | 处理 | 位置 |
| --- | --- | --- |
| 三个初始化等待的判定顺序 | 统一改为“每轮先确认时间源与 API/阶段两级预算，再接受 `RAM_INIT` 完成位 / `CTL.INIT` 置位 / `CTL.INIT` 清零”，与 IF1/IF2 等待助手一致；预算或时间源失败返回 `BUSY`，次数耗尽返回 `UNKNOWN` | `Hw_CanEnterInit()`、`Hw_CanEnterInitPlain()`、`Hw_CanExitInit()` |
| Bus-Off 路径的失败映射 | 已经写过 `CTL.INIT` 后，`Hw_CanEnterInitPlain()`/`Hw_CanExitInit()` 的**任何**非 OK 结果（含 `BUSY`）都锁住该模块收发（置 `PENDING_TX|PENDING_RX`）并返回 `UNKNOWN`，交由 `Hw_CanRecoverModule()` 处理 | `Hw_CanRecoverBusOff()` |
| 普通初始化与模块恢复的失败语义 | 普通初始化：`Hw_CanSingleInit()` 返回 0 → `Hw_CanInit()` 返回 0，模块不标记已初始化；模块恢复：失败保留原未决状态与配置有效，仅重建成功才清 `gCanPending` | `Hw_CanInit()`、`Hw_CanRecoverModule()` |

**复测（等价表达式复刻，未编译源文件）**：9/9 通过

| 注入条件 | 旧顺序 | 新顺序 |
| --- | --- | --- |
| 取阶段起点那次读取失败（`IsReady=0`），`INIT` 已清零 | `OK`（跳过检查） | `BUSY` |
| 已过 100 tick（API 限额 20），首次检查时 `INIT` 已清零 | `OK`，预算读取 0 次 | `BUSY`，预算读取 ≥1 次 |
| 上面两种 `BUSY` 经 `Hw_CanRecoverBusOff()` 映射 | — | `UNKNOWN`，`gCanPending` 置 `0x0003` |
| 对照：时间源正常、`INIT` 已清零 | `OK` | `OK` |
| 停表且 `INIT` 永不置位 | 死等 | 有限次数后 `UNKNOWN` |

本机没有主机 C 编译器（`gcc`/`cl`/`clang` 均不存在，只有 TI C28x 交叉编译器），因此复测为决策级复刻；如需指令级验证，可用你的 C 主机桩对新 diff 重跑同样两个反例。

**返工后构建**：31 编译单元、0 警告 0 错误；`EP15K_minimal.out` 690,136 字节 sha256 `7cbbf05dc63b2cf8839d246edab5f4625753a20571a0128f4db19b1341f81ec5`；`EP15K_minimal.map` 85,491 字节 sha256 `2de5383eed26e278bd167194b7957ea83596827a33bbfbb50a864cf5006b55b4`。

**未做**：驱动接入继续暂缓；`S-04-09`~`S-04-15`、`S-04-18` 仍待上板。

---

## 8. 审阅结论（由 Codex 填写）

- 结论：**代码审阅通过**（2026-11-04）。第二~四轮提出的问题全部关闭，本轮无返工项，可进入上板验收。
- 条件：`S-04-09` ~ `S-04-15`、`S-04-18` 仍需上板验证；上板结果回填本文件后，本步才可标记为完成。驱动接入（端口层/协议层）不随本步推进。
- 审阅方独立验证：从本次 diff 提取实际 C 函数，用主机 C 编译器运行故障注入桩，12 项断言全部通过——关键结果包括：`INIT` 已清零但阶段起点读取失败时 `Hw_CanExitInit()` 返回 `BUSY`；超预算时执行预算检查并返回 `BUSY`（对照正常时返回 `OK`）；`RAM_INIT` 完成但预算耗尽时 `Hw_CanEnterInit()` 返回 `BUSY` 且未写 `SWR`；停表且 `INIT` 始终未置位时 `Hw_CanEnterInitPlain()` 有限次数后返回 `UNKNOWN`；Bus-Off 进出初始化无法确认状态时对外返回 `UNKNOWN`、`gCanPending = 0x0003`；模块重建失败保留配置与未决锁、重试成功后清锁并恢复已初始化状态。
- 需要返工的点：无。

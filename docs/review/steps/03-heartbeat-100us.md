# 步骤 03：心跳时基拆分（100 µs 硬件心跳，1 ms 调度语义）

- base_sha：`5bb641f70f0588cdf70f0b0c01ea4a0b321963df`
- head_sha：工作区待审
- original_baseline：`57f94362aab18df739b40f7109b1e6987745224c`
- 对应缺陷/验收 ID：无既有 ID，本步新增 `S-03-01` ~ `S-03-06`
- 状态：待审（代码已落地，构建通过；板上项待测）

## 1 意图与边界

### 1.1 目的

通信层要用 100 µs 时间粒度（SCI 收字节、CAN 收帧、串口静默断帧、后续调度粒度升级），但当前 `CPU Timer1` 的中断周期、`stBringupStatus.u32HeartbeatMs`、`Task_OnTick()` 和主中断停滞检测，全部按 1 ms 写死在一起。

若只把 `u32Period` 从 `199999UL` 改成 `19999UL`，上面四项会一起变成 10 kHz：秒表快 10 倍、`Task_Start(..., 1U, ...)` 的"1 ms"实际变成 100 µs、停滞计数阈值差 10 倍。因此必须**同时**拆分"硬件心跳周期"与"毫秒语义"。

### 1.2 本步做

| 文件 | 改动 |
|---|---|
| `Broad/src/DrvCfg_EP15K.c` | `stSysCpuTimer1`：`u32Period` 199999→19999、`u16TickDivCoef` 1→10，补注释说明硬件 100 µs / 软件 1 ms |
| `main.c` | `Isr_Heartbeat()` 改为"每次加 100 µs 计数 + 调一次 `Hw_CpuTimerIsrTick()`；返回 1 时做毫秒工作" |
| `Platform/Status/Inc/BringupStatus.h` | 新增 `Uint32 u32Tick100us;`，修正 `u32HeartbeatMs` 注释 |
| `docs/minimal-bringup.md` | 心跳行、后台任务行、验收第 3 步 |
| `EP15K_2026_配置核心与运行逻辑.md`（仓库外） | §3.9 计时器表、§4.3 `Isr_Heartbeat` 伪码与行、观测表 |

### 1.3 本步不做

- 不改任何通信驱动（`Hw_SciDriver`、`Hw_CanDriver`）——那是步骤 04。
- 不改环形缓存（`Platform/Common` 尚未引入，本步不引入）。
- 不改 `Task_Run()` / `Task_OnTick()` 的毫秒语义，`Task_Start(cb, 1U, 0U)` 仍然是 1 ms。
- 不把 `u16ControlStallMs` 改成 100 µs 单位，也不改它的判据阈值。

### 1.4 已裁定事实（m01054）

1. 唯一的分频机制用现有 `Hw_CpuTimerIsrTick()`，**不另加 0~9 计数器**。
2. `u16ControlStallMs` 保持毫秒单位；"比较 `u32ControlIsrCount` → 增/清停滞计数"整段放进 1 ms 分支。
3. 步骤 03 / 04 分开实施。
4. 环形缓存核心继续使用已评审版本，不随心跳修改。

## 2 现状核对（改前事实，逐条从代码读出）

| 位置 | 事实 |
|---|---|
| `Broad/src/DrvCfg_EP15K.c:280-290` | `stSysCpuTimer1 = {HW_CPUTIMER_ID1, 0U, 1U, 1U, 199999UL, 0UL, 0U, 1U}`，依次为 `u16TimerId / u16Prescaler / bEnableInt / bEmuStop / u32Period / u32Tick / u16TickDivCnt / u16TickDivCoef` |
| `Drivers/src/Hw_CpuTimerDriver.c:69` | `pstRegs->PRD.all = pstSysTimer->u32Period;`，TDDR=0 ⇒ 中断周期 =(PRD+1)/SYSCLK，199999 → 1 ms，19999 → 100 µs |
| `Drivers/src/Hw_CpuTimerDriver.c:85-88` | `u16TickDivCoef == 0` 时被强制成 `1U`，所以分频系数不能写 0 |
| `Drivers/src/Hw_CpuTimerDriver.c:172-198` | `Hw_CpuTimerIsrTick()` 只做 `u16TickDivCnt++` / 比较 / 满则清 0 且 `u32Tick++` / 返回 1 或 0；**内部不清 TIF** |
| `Drivers/src/Hw_CpuTimerDriver.c:158-169` | `Hw_CpuTimerClearIntFlag()` 才写 `TCR.bit.TIF = 1`（写 1 清除）——清标志的唯一责任点 |
| `main.c:159-178` | `Isr_Heartbeat()` 每次中断都执行：`Hw_CpuTimerClearIntFlag()` → `u32HeartbeatMs++` → `Task_OnTick()` → 停滞比较/增/清 |
| `main.c:8` + `Broad/inc/DrvCfg_EP15K.h:7` | `main.c` 经 `DrvCfg_EP15K.h` 已包含 `Hw_CpuTimerDriver.h`，`Hw_CpuTimerIsrTick()` 声明可见，无需新增 include |
| `Platform/Status/Inc/BringupStatus.h:26` | `u32HeartbeatMs` 注释为 "nominally 1 ms per tick" |

结论：**只改配置位不生效**——`Isr_Heartbeat()` 根本没有调用 `Hw_CpuTimerIsrTick()`，必须改调用路径（用户判断成立）。清标志方面 `Hw_CpuTimerIsrTick()` 不清 TIF，因此 ISR 里保留原 `Hw_CpuTimerClearIntFlag()` 调用，责任点唯一，不产生重复清除。

## 3 修改前后

### 3.1 `Broad/src/DrvCfg_EP15K.c`：`stSysCpuTimer1`

改前：

```c
/* stSysCpuTimer1 */
volatile ST_SYS_TIMER stSysCpuTimer1 =
{
    (Uint16)HW_CPUTIMER_ID1,        /* u16TimerId     */
    0U,                             /* u16Prescaler   */
    1U,                             /* bEnableInt     */
    1U,                             /* bEmuStop       */
    199999UL,                       /* u32Period      */
    0UL,                            /* u32Tick        */
    0U,                             /* u16TickDivCnt  */
    1U                              /* u16TickDivCoef */
};
```

改后：

```c
/* stSysCpuTimer1：硬件中断 100 µs（(19999 + 1) / 200 MHz），软件 10 分频后 1 ms 心跳。 */
volatile ST_SYS_TIMER stSysCpuTimer1 =
{
    (Uint16)HW_CPUTIMER_ID1,        /* u16TimerId     */
    0U,                             /* u16Prescaler   */
    1U,                             /* bEnableInt     */
    1U,                             /* bEmuStop       */
    19999UL,                        /* u32Period      */
    0UL,                            /* u32Tick        */
    0U,                             /* u16TickDivCnt  */
    10U                             /* u16TickDivCoef */
};
```

### 3.2 `main.c`：`Isr_Heartbeat()`

改前：

```c
static __interrupt void Isr_Heartbeat(void)
{
    static Uint32 lastControlCount = 0UL;

    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);
    stBringupStatus.u32HeartbeatMs++;
    Task_OnTick();

    if (lastControlCount == stBringupStatus.u32ControlIsrCount)
    {
        if (stBringupStatus.u16ControlStallMs != 0xFFFFU)
        {
            stBringupStatus.u16ControlStallMs++;
        }
    }
    else
    {
        stBringupStatus.u16ControlStallMs = 0U;
        lastControlCount = stBringupStatus.u32ControlIsrCount;
    }
}
```

改后：

```c
static __interrupt void Isr_Heartbeat(void)
{
    static Uint32 lastControlCount = 0UL;

    /* TIF 只在这里清；Hw_CpuTimerIsrTick() 不碰中断标志。 */
    Hw_CpuTimerClearIntFlag((ST_SYS_TIMER *)&stSysCpuTimer1);

    stBringupStatus.u32Tick100us++;

    /* u16TickDivCoef = 10：每 10 次硬件中断返回一次 1，即 1 ms。 */
    if (Hw_CpuTimerIsrTick((ST_SYS_TIMER *)&stSysCpuTimer1) != 0U)
    {
        stBringupStatus.u32HeartbeatMs++;
        Task_OnTick();

        if (lastControlCount == stBringupStatus.u32ControlIsrCount)
        {
            if (stBringupStatus.u16ControlStallMs != 0xFFFFU)
            {
                stBringupStatus.u16ControlStallMs++;
            }
        }
        else
        {
            stBringupStatus.u16ControlStallMs = 0U;
            lastControlCount = stBringupStatus.u32ControlIsrCount;
        }
    }
}
```

要点：停滞检测**整段**搬进毫秒分支，不只是递增语句；否则计数速度变成 10 倍，100 ms 的阈值会在约 10 ms 触发。

### 3.3 `Platform/Status/Inc/BringupStatus.h`

- `u32HeartbeatMs` 注释改为：`// 1 ms heartbeat count, advanced once per 10 hardware timer interrupts.`
- 新增字段（紧跟 `u32ControlIsrCount`，保持 32 位成组）：

```c
    Uint32 u32Tick100us;         // CPU Timer1 ISR count, nominally 100 µs per tick.
                                 // Software time base only: it does not back-fill ticks
                                 // skipped while interrupts were disabled.
```

结构体只存在于 RAM、无持久化、未对外暴露（CAN/SCI 尚未接入），布局变化无兼容影响。

### 3.4 文档同步

| 文档 | 位置 | 改动 |
|---|---|---|
| `docs/minimal-bringup.md` | 第 3 行、第 17 行（心跳行）、第 47 行（构建句）、第 53 行（首次验收第 3 步）；行号为改动前基线 | 1 ms → 100 µs 硬件心跳；说明 10 分频后调度仍是 1 ms；验收加 `u32Tick100us` 每秒约 +10000 |
| `EP15K_2026_配置核心与运行逻辑.md`（仓库外） | 第 29、43、158 行，§3.9 表（615-628 行），§4.3 `Isr_Heartbeat` 伪码（745 行附近），观测表（798-799 行） | 周期 199999→19999、中断频率 1 kHz→10 kHz、心跳行改为"100 µs 硬件 / 1 ms 软件"、伪码同步、新增 `u32Tick100us` 观测行 |

## 4 验证

### 4.1 构建（已跑）

`python tools/build_minimal.py --cgt D:/ti/CCS1110/ccs/tools/compiler/ti-cgt-c2000_22.6.2.LTS`

- 结果：**30 个编译单元、零 warning、零 error、链接成功**（脚本 `run()` 会把编译器任何输出原样打印，本次除进度行外无输出）。
- 产物：`D:\MinMain\CCS-Project-minimal-build\EP15K_minimal.out` 671,632 字节，sha256 `0646fd4ebcaf9173932d93c2f34aa95990f74ff56509523184822dabe4988253`；`EP15K_minimal.map` 83,365 字节，sha256 `4bf547d2270c7622d3697a6860bfa3aa2e7460ade69519c0f798ad8dd3df3991`。

### 4.2 静态核对（已跑）

| 编号 | 项 | 判据 |
|---|---|---|
| `S-03-01` | 配置值 | `u32Period = 19999UL`、`u16TickDivCoef = 10U`、`u16Prescaler = 0U` |
| `S-03-02` | 清标志唯一 | 应用层无 `TCR.bit.TIF` 写点；驱动内两处：`Hw_CpuTimerDriver.c:76`（`Hw_CpuTimerInit` 清启动残留）与 `:168`（`Hw_CpuTimerClearIntFlag`）；`Isr_Heartbeat` 内只有一处清标志调用 |
| `S-03-03` | 分频调用唯一 | `Hw_CpuTimerIsrTick()` 全仓调用点只在 `Isr_Heartbeat()`，且每中断一次 |
| `S-03-04` | 毫秒工作归属 | `u32HeartbeatMs++`、`Task_OnTick()`、停滞比较全在 `!= 0U` 分支内 |

实测结果：

| 编号 | 结果 | 证据 |
| --- | --- | --- |
| `S-03-01` | 通过 | `DrvCfg_EP15K.c` 现为 `19999UL` / `0U` / `10U`（`u32Period` / `u16Prescaler` / `u16TickDivCoef`） |
| `S-03-02` | 通过 | `grep -rn TIF`：仅 `Hw_CpuTimerDriver.c:76`、`:168` 两处写点；`main.c:164` 是唯一调用 `Hw_CpuTimerClearIntFlag()` 的位置 |
| `S-03-03` | 通过 | `grep -rn Hw_CpuTimerIsrTick`：定义 `Hw_CpuTimerDriver.c:171`，调用点仅 `main.c:169`，每次中断一次 |
| `S-03-04` | 通过 | `main.c:166-191`：`u32HeartbeatMs++`/`Task_OnTick()`/停滞比较全部位于 `if (Hw_CpuTimerIsrTick(...) != 0U)` 块内 |

### 4.3 板上（未测，需板）

| 编号 | 项 | 判据 |
|---|---|---|
| `S-03-05` | 时基 | 运行约 1 s：`u32Tick100us` 约 +10000、`u32HeartbeatMs` 约 +1000、`u16ControlStallMs` 保持 0 |
| `S-03-06` | 调度吞吐量不变（只证明一秒内还跑了 1000 次，不证明周期准确） | 同一测量窗口的增量：`AdcSampleTask` 计数每秒约 +1000（**吞吐量检查**）；`u32HeartbeatMs` 每秒约 +1000；`u16ControlStallMs` 保持 0。任务的**释放间隔**与**执行抖动**需另外单独观察（例如在任务首尾翻空闲 GPIO 看周期与抖动），不能仅凭次数判定周期准确 |
| `S-03-07` | 中断开销 | 同一窗口增量先看数量：`u32ControlIsrCount` 每秒约 +20000、`u32Tick100us` 每秒约 +10000。再分两项测：①`Isr_Heartbeat` **自身执行时间**（空闲 GPIO 翻转 + 示波器，目标 < 10 µs）；②20 kHz 控制中断的**响应延迟 / 抖动**（CLA1_1 触发到 ISR 首条指令），确认 10 kHz 心跳未造成不可接受阻塞 |

## 5 风险与请求裁定

1. **中断负载**：硬件中断从 1 kHz 提到 10 kHz。每次 ISR 只做清标志 + 两个计数 + 一次分频函数调用，毫秒工作仍为 1 kHz；增量估计 < 1 µs/次，占 100 µs 的 1% 以内，需 `S-03-07` 实测确认。
2. **`u32Tick100us` 不是绝对时间**：长时间屏蔽中断后不会补计被跳过的节拍；只作软件时基，不能当自由运行计数器使用（严格时序仍应用硬件计数器）。
3. **停滞检测语义不变**：仍在 1 ms 分支比较 `u32ControlIsrCount`，20 kHz 下每个毫秒约 20 次控制中断，判据与改前一致；`u16ControlStallMs` 单位仍是 ms。
4. **待裁定项：无**（m01054 已拍板全部实现细节）。

## 6 附：通信层相关事实更正（非本步范围，供步骤 04 引用）

上轮核对中我有三处判断错误，按此更正，避免步骤 04 沿用错值：

| 项 | 我的原判断 | 更正后（依据） |
|---|---|---|
| `CAN_IF1MSK` 位号 | bit30 = MXtd | **MXtd = bit31、MDir = bit30、bit29 保留**（TRM 21.15.2.23 Figure 21-42 / Table 21-32，本地 `_pdf_txt/manual.txt:143145-143200`） |
| `HW_CAN_ARB_XTD (1<<30)` 写 IF1MSK 的效果 | 等于设置 MXtd | 实际设置的是 **MDir**，扩展帧类型比较并未启用；既有命名问题也有功能问题 |
| 两路 IF1MSK 实际值 | 两路都是 `0x00000000` | CAN-A `0x00000000`；**CAN-B `0x40000000`**（bExtId=1 走了 `u32Msk |= HW_CAN_ARB_XTD`） |
| Modbus RTU 断帧 | 115200 下约 0.3 ms / 3 个 100 µs tick | 19200 8N1 的 3.5 字符 ≈ **1.823 ms**（18 tick 已经略短）；11 bit 字符格式 ≈ **2.005 ms**；**波特率 > 19200 时规范建议固定 `t1.5 = 750 µs`、`t3.5 = 1.750 ms`** |

步骤 04 需据此明确：`HW_CAN_MSK_MXTD (bit31)` 与 `HW_CAN_MSK_MDIR (bit30)` 分开定义，ARB 与 MSK 常量不共用；接收对象逐个配置 ID/掩码/帧类型/方向比较开关；单接收对象的吞吐风险与最终接收范围一并裁定。

## 7 审阅结论（由 Codex 填写）

- 状态：
- 意见：

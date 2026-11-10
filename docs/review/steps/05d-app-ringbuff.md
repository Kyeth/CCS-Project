# 步骤 05D 申请：Modbus RTU 从机（表驱动）

| 项 | 内容 |
| --- | --- |
| 文档版本 | v0.4 |
| 日期 | 2026-11-10 |
| 基线 | `83dba4c`（05C 前台轮转可用基线） |
| 状态 | 已按复审意见返工（五项修正 + ARM 入口），目标构建通过，待复核 |
| 取代 | v0.3（补五项修正、t3.5 规则、批次配额、回复忙门禁、ARM 入口与点表 5000..5036） |
| 复审依据 | v0.3 包实跑 `checks=54 fails=3`（float 12.5 编码、写 float、−50 解码），另五项接入问题 |

## 0. 结论先行

本步在**不改** 05C 已验收的通信搬运路径的前提下，加入三路 Modbus RTU 从机：

- 应用层只维护 **Modbus 点表**（地址、权限、类型、变量绑定、上下限）与**协议实例配置表**（哪个 SCI 口、什么协议、从机地址、哪张点表）；
- 平台层提供 **协议操作表**（Init / Service / Reset）与通用点表访问，负责查表、类型转换、校验与收发；
- 协议服务注册为独立的 **1 ms 任务** `ProtocolTask`；主循环仍只有 `Hw_FeedWatchDog(); PortTaskService(); Task_Run();`。

## 1. 范围与不做

做：

1. Modbus RTU 从机，功能码 `0x03` / `0x06` / `0x10`；
2. 三路实例：SCI-A（19200，从机 1）、SCI-C（115200，从机 1）、SCI-D（38400，从机 2）；
3. 点表支持 `Uint16` / `int16` / `Uint32` / `int32` / `float`，32 位量占两个寄存器、高字在前；
4. 初始化期检查点表（地址递增不重叠、类型与限值合法），错误在启动时暴露。

不做（本步）：

- 不改 `SciPort` / `CanPort` / `RingBuff` / `PortTask` / `PortBudget`；
- 不新增中断，不把协议解析放进心跳 ISR；
- CAN 不新建队列；
- 不实现 `0x01`/`0x02`/`0x04`/`0x05`/`0x0F`（统一回异常 `0x01`）；
- 不做应用层重发、序号回填、跨端口去重；
- 不重跑 05C 板测，`S-05B-08/09` 的未达标记录保持原样。

## 2. 三层表与模块

| 层 | 内容 | 实现位置 |
| --- | --- | --- |
| 应用层 | Modbus 点表、协议实例配置表 | `UserApp/Inc/AppModbus.h`、`UserApp/Src/AppModbus.c` |
| 平台框架 | 协议操作表、实例装配、1 ms 任务分发 | `Platform/Protocol/Inc/Protocol.h`、`Platform/Protocol/Src/Protocol.c` |
| 协议核心 | 组帧、CRC、功能码、异常应答、待发状态 | `Platform/Protocol/Modbus/{Inc,Src}/ModbusRtu.{h,c}` |
| 点表访问 | 类型转换、限值、写原子性 | `Platform/Protocol/Modbus/{Inc,Src}/ModbusTbl.{h,c}` |
| 端口适配 | SciPort 收发、批次时标、故障恢复 | `Platform/Protocol/Modbus/{Inc,Src}/ModbusProto.{h,c}` |

依赖方向：`AppModbus → Protocol → ModbusProto → {ModbusRtu, ModbusTbl, SciPort}`。`ModbusRtu` 不含 `Hw_SciDriver.h`，也不访问寄存器。

配置示例（`AppModbus.c` 内的实例表）：

```
SCI-A → Modbus 从机 → 地址 1 → 点表 A
SCI-C → Modbus 从机 → 地址 1 → 点表 C
SCI-D → Modbus 从机 → 地址 2 → 点表 D
```

三路共用同一份协议实现（一张协议操作表），但接收缓冲、待发缓冲、超时与故障状态各自独立（每实例一份 `ST_MB_PROTO_STATE`）。

## 3. 点表格式

```c
typedef struct {
    Uint16 u16Addr;      /* 起始寄存器地址 */
    Uint16 u16Type;      /* MB_PT_U16 / I16 / U32 / I32 / F32 */
    Uint16 u16Access;    /* MB_PT_READ / WRITE / READWRITE */
    Uint16 u16Reserved;  /* 预留：保存属性等 */
    void  *pVar;         /* 绑定的变量，必须非空 */
    Uint32 u32Min;       /* 整数下限（按类型宽度解释） */
    Uint32 u32Max;       /* 整数上限 */
    float  fMin;         /* 浮点下限 */
    float  fMax;         /* 浮点上限 */
} ST_MODBUS_POINT;
```

应用点表实例（每路一套，绑定该路自己的变量）：

| 地址 | 类型 | 权限 | 变量 | 限值 |
| --- | --- | --- | --- | --- |
| 0x0000 | Uint16 | 读写 | `s_au16TestMode[i]` | 0..1000 |
| 0x0001 | int16 | 读写 | `s_ai16TestOffset[i]` | −500..500 |
| 0x0003 | Uint32 | 读写 | `s_au32TestCounter[i]` | 0..1000000 |
| 0x0005 | int32 | 读写 | `s_ai32TestTrim[i]` | −100000..100000 |
| 0x0007 | float | 读写 | `s_afTestGain[i]` | −50.0..150.0 |
| 0x0009 | Uint16 | 只读 | `s_au16TestStatus[i]` | — |

变量类型保持不变（`Uint16` / `int16` / `Uint32` / `int32` / `float`），**不**统一强转成同一种寄存器类型；访问器按 `u16Type` 选择对应类型的 `volatile` 指针，保留 `volatile` 语义（控制 ISR 更新的变量不会被缓存读）。

### 3.1 类型转换规则

- `Uint16` / `int16`：占 1 个寄存器，值直接放入；
- `Uint32` / `int32` / `float`：占 2 个寄存器，**高字在前**（`regs[0] = bits >> 16`、`regs[1] = bits & 0xFFFF`），每个寄存器在报文中高字节在前（由 `ModbusRtu` 的 `MbPutBe16` 负责）；
- 线上地址仍是 16 位寄存器，类型信息只存在于点表。

### 3.2 初始化期检查（`ModbusTblValidate`）

逐项检查并在任何一项失败时拒绝启用该实例：

1. 类型已知、权限在 `1..3`、`pVar` 非空；
2. 地址严格递增且区间不重叠（`addr >= prev_addr + prev_regs`）；
3. 区间不越过 16 位地址空间；
4. 整型限值与类型宽度一致（例：`Uint16` 配上限 `66530` 必须在这里报错，不能留到收报文时）；
5. 浮点上下限用 float 比较，`NaN` 也会被拒绝。

## 4. 协议实例配置表与协议操作表

```c
struct ST_PROTO_INSTANCE {
    Uint16 u16SciId;         /* HW_SCI_ID_A / _C / _D */
    Uint16 u16Type;          /* PROTO_TYPE_MODBUS */
    Uint16 u16SlaveAddr;
    Uint16 u16Enabled;
    Uint16 u16TblCount;
    Uint32 u32Baud;          /* 来自工程配置（Broad 的 stSciCfg） */
    const void *pstTbl;      /* 点表 */
    void *pCtx;              /* 协议私有上下文 */
};

typedef struct {
    Uint16 u16Type;
    Uint16 (*pfInit)(ST_PROTO_INSTANCE *);
    Uint16 (*pfService)(ST_PROTO_INSTANCE *);
    void   (*pfReset)(ST_PROTO_INSTANCE *);
} ST_PROTO_OPS;
```

- 增加协议（如 Xmodem）时只需新增一行 `ST_PROTO_OPS` 并 `ProtocolRegisterOps()`，通过 `ProtocolResetPort()` 取得端口所有权；
- 实例装配顺序：`ProtocolInit()` → `ProtocolRegisterOps(&stModbusProtoOps)` → 逐实例 `ProtocolAddInstance()`（内部调用 `pfInit`，失败则该实例不参与服务）；
- 波特率由 `AppModbus` 从 `stSciCfg` 查表搬入实例配置，平台层不引用 `Broad`。

## 5. 调度

`ProtocolTask` 注册为独立的 1 ms 任务：

```c
Task_Start(ProtocolTask, 1U, 0U);
```

主循环保持 05C 原样：

```c
while (1)
{
    Hw_FeedWatchDog();
    (void)PortTaskService();
    Task_Run();
}
```

`ProtocolTask` 遍历实例表，对每个启用实例调用一次 `pfService`：每端口最多 32 字节（两段 × 16 字节）、有界处理、不等整帧、不等 TX 空闲、不做阻塞 Flash 操作。

**1 ms 是协议任务的运行周期，不是 RTU 的计时精度。** 帧间隔（3.5 字符）与候选帧超时（100 ms）都由 `SciPortPopBatch()` 返回的批次观察时刻（`ST_SCI_RX_SEG.u32Tick`）与 `Hw_TimeBaseTicks()` 计算，不能把"两次任务调用"当成"静默 2 ms"。

`PortTaskService()` 仍由 100 µs 心跳置通知位、主循环每圈领取一次、每拍只服务一个通信单元。它**不能**改成 1 ms 周期任务：那样每个单元约 5 ms 才获得一次机会，会退回 05C 之前的饥饿状态（实测 54 帧/s）。

## 6. 协议核心规则（`ModbusRtu`）

### 6.1 接收

- 独立接收缓冲 + 跨批次状态：`MB_RX_HUNT`（找地址）→ `MB_RX_FRAME`（累积）→ `MB_RX_COMPLETE`（长度已达，等静默确认）；
- 先按功能码算**预期长度**：`0x03`/`0x06` = 8 字节；`0x10` = 9 + byteCount；其他功能码 = 8（先收齐再回异常 `0x01`）；
- 长度达到**不等于**执行：必须静默满足 `t3.5` 才校验 CRC 并执行；
- 超长（> 256）、断流、时标不可信：丢弃候选帧；
- 处在长帧中间时，一次空读**不**清空缓冲（否则每遇空读就重启，永远收不全）。

### 6.2 校验与应答

| 情况 | 处理 |
| --- | --- |
| 地址不匹配、CRC 错 | 不回复 |
| 广播写（地址 0） | 执行，不回复 |
| 广播读 | 忽略 |
| 功能码不支持 | 异常 `0x01` |
| 寄存器地址不存在、32 位项被不完整访问、只读点被写 | 异常 `0x02` |
| 数量 0 或 > 32、`0x10` 的 byteCount 与数量不符、值超限 | 异常 `0x03` |

异常应答 5 字节：`从机地址 | 功能码 OR 0x80 | 异常码 | CRC 低 | CRC 高`。

`ModbusCrc16` 入参为 `const Uint16 *`，只取每个单元的低 8 位，返回 CRC 值本身（低字节先发）。

### 6.3 数值与防回绕

- 数量 1..32；起始地址加数量用 32 位中间值计算，避免 `Uint16` 回绕；
- `0x10` 的 `byteCount` 必须等于 `2 × 数量`。

## 7. 点表访问

- **32 位项必须完整访问**：请求起点落在 32 位项的第二个寄存器（中间），或只覆盖其中一个寄存器，都返回 `0x02`；
- 写采用"先整段校验、再提交"：第一遍只检查（存在、可写、完整、限值），全部通过后第二遍才写入，任何一项失败都不留半写状态；
- 只读点被写返回 `0x02`（不是 `0x03`）；
- 数值超限返回 `0x03`。

## 8. 收发路径

### 8.1 接收

`MbProtoService()` 每轮：

1. 取一次当前时标（`Hw_TimeBaseTicks()` + `Hw_TimeBaseIsReady()`）；
2. 处理故障（见 §9）；
3. 有待发回复且端口未冻结：`SciPortWriteTx()` 提交，按**实际接受的字节数**推进 offset；
4. 仅在 `SCI_RX_IDLE` 时读端口：每轮最多两段、合计 32 字节，每段带自己的观察时刻交给 `ModbusRtuFeed()`；
5. `ModbusRtuPoll()` 做静默/超时判定与执行。

端口在 `SCI_RX_HW_RESET` / `SCI_RX_PAUSED` / 流中断期间不搬字节，候选帧被丢弃并等待重新同步。

### 8.2 发送

- 回复**整帧**组好后进入实例的待发缓冲，只有端口接受的部分才推进 offset；
- `offset == len` 只代表"进了端口队列"，不代表硬件发出、更不代表对端收到；`ModbusProtoTxSubmitted` 只统计"整帧已提交进端口环"；
- 端口挂起（`SciPortIsTxSuspended()`）时保留 offset，本层不重发、不丢弃；
- `SciPortWriteTx()` 自身不检查挂起，所以协议层必须自己先查；
- 硬件环满返回 0 时保留 offset，下轮再试（`u32TxBusyRounds++`）。

## 9. 故障恢复

| 事件 | 协议层动作 |
| --- | --- |
| 新发生强制复位（`SciPortForceResets()` 增长） | 作废尚未提交完的回复（`ModbusRtuDiscardReply()`），置冻结标志，`u32TxLostEvents++`；**不自动重发** |
| 端口回到 `SCI_RX_IDLE` 且冻结 | 调 `SciPortTxResume()` 确认"接受这次损失"，解冻 |
| 流中断（`SciPortIsStreamBreak()`，带代次） | 丢弃候选帧；代次变化才重新登记；`SciPortClearStreamBreak(id, gen)` 成功才继续 |
| `HW_RESET` / `PAUSED` | 丢弃候选帧，不搬字节 |
| 时标不可信 | 只累加 `u32TimeInvalid`，**不**提交请求（尤其不执行写） |

不能只看 `u32TxResetDrop` 增长来判断"是否丢了一帧"：软件环已空而硬件仍有在途数据时也会损失，必须结合强制复位计数与恢复标志。

## 10. 内存与链接

- 每实例 `ST_MB_INSTANCE`：接收 256 + 回复 144（`MB_TX_BUF_SIZE` 由 128 提到 144）+ 寄存器镜像 64（`MB_MAX_REGISTERS` 由 32 提到 **64**，覆盖 5000..5036 连续 37 个）+ 状态，三路合计约 1,350 字；
- 新增段 `.protobuf`（`RAMGS1`，`PAGE = 1`）：`ModbusProto 0x588` + `AppModbus 0x24` + `Protocol 0x24` = **`0x5D0` 字**，起点 `0x00D000`；
- `.portstat` 顺移到 `0x00D5D0`（`0x14` 字）；
- `.portbuf` 仍在 `0x00DC00`（`0x180` 字），未受影响。

## 11. 构建

`tools/build_minimal.py --cgt C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS`（默认 `--cgt` 指向已失效的 `E:\CCS\…`，必须显式传）：

| 项 | 值 |
| --- | --- |
| 编译单元 | **42** 个（05C 的 37 个 + `Protocol.c` / `ModbusRtu.c` / `ModbusTbl.c` / `ModbusProto.c` / `AppModbus.c`） |
| 警告 | 0 |
| `.text` | `0x5523`（05C 基线 `0x45AF`；v0.3 为 `0x5286`） |
| `RAMGS0` | 用 `0xF00`，余 `0x100`（256 字） |
| `RAMGS1` | 用 `0x5E4`，余 `0x61C` |
| `EP15K_minimal.out` | 856,556 B，sha256 `5de0ceeaa8b2405d3a872d7fb7c72dfa1a06de2ad2a8525839eaa2cc1b1a50cb` |
| `EP15K_minimal.map` | 109,034 B，sha256 `f1bd4250a00a2a32165ba3eba6dc2b65d1e353226c839b597ac42de503ef9846` |

`PORT_BENCH_TEST` 仍为 `0U`（生产态）。

## 12. 验收

主机有两个测试文件，都是自带 `main()` 的独立程序：

| 文件 | 覆盖 | `Check(` 调用点 |
| --- | --- | --- |
| `Platform/Protocol/Test/modbus_stub_test.c` | CRC、点表校验、类型转换、访问限制、帧级行为、倒计时与回绕、分批积压、回复忙 | **73** |
| `Platform/Protocol/Test/modbus_proto_stub_test.c` | 适配层：静默确认、配额耗尽积压、两帧分批积压、回复忙不覆盖、ARM 交接与释放 | **34** |

`modbus_proto_stub_test.c` 自己带 SciPort 与时间基准替身，链接 `ModbusProto.c`、`ModbusRtu.c`、`ModbusTbl.c`。

本机没有 `cc`/`gcc`，两个文件**都只在实施方做过 `cl2000` 的语法/类型检查**（零错误零警告），
**尚未执行**；请审阅方跑：

```
cc -std=c99 -Wall -Wextra -ITest/shim -I../Inc -I../Modbus/Inc \
   modbus_stub_test.c ../Modbus/Src/ModbusRtu.c ../Modbus/Src/ModbusTbl.c \
   -o modbus_stub_test

cc -std=c99 -Wall -Wextra -I../../Port/Test/shim -I../../Port/Inc \
   -I../Inc -I../Modbus/Inc -I../../../Drivers/inc \
   modbus_proto_stub_test.c ../Modbus/Src/ModbusProto.c \
   ../Modbus/Src/ModbusRtu.c ../Modbus/Src/ModbusTbl.c \
   -o modbus_proto_stub_test
```

用例（两个文件合计 8 组）：

1. CRC 与组帧（已知报文向量、异常应答字节序）；
2. 点表校验（重叠、类型、`Uint16` 配 66530、浮点限位颠倒、32 位中间重叠、未绑定）；
3. 类型转换（`Uint32` 高字在前、`float` 位模式往返、`int32` 负值、NaN/Inf 被拒）；
4. 32 位项不完整访问 → `0x02`；只读被写 → `0x02`；超限 → `0x03` 且变量未被改；
5. 广播写执行不回复、广播读忽略、坏 CRC/地址不符不回复、`0x10` byteCount 不符回 `0x03`；
6. 倒计时：只过 20 µs 不执行、过 200 µs 执行、`tick = 0` 仍有效、`0xFFFFF000` 回绕后也能判静默；
7. 配额耗尽：一轮取满 32 字节后仍有积压 → 记录积压、不确认、不执行；取空后才执行；
8. 回复忙：端口环满时第二条写请求不执行，回复缓冲不被覆盖，端口恢复后补提交；
   另含 ARM 入口：六字节起点命中回调 → 装载握手回复 → 提交后交接（`ArmActive = 1`，本层不再读端口）→ 释放后普通读恢复。

上板（需 Modbus 主站，PC + USB 转串口）：

1. 三路同时通信 60 s：`ModbusProtoRxFrames` 与主站请求数一致，`u32TxLostEvents = 0`；
2. 读 `0x03` 回 32 个寄存器；写 `0x06` 单寄存器、`0x10` 读回一致；
3. 写超限值收到 `0x03` 且变量保持原值；
4. 只给 A 注入短故障：C/D 请求不中断，A 恢复后继续应答；
5. 控制侧：`u16ControlStallMs = 0`，入口间隔仍按单列记录（本步不改控制路径）。

## 13. 三项待确认的裁定（v0.4 已按裁定实现）

| 问题 | 裁定 | 实现 |
| --- | --- | --- |
| `t3.5` 是否加保守系数 | **不加**。≤19200 按实际字符格式算；>19200 用固定 **1750 µs**，不再套 `38500000/baud` | `MbProtoElapsedUs()`：`u32Baud > 19200` 时 `MB_T35_FIXED_US = 1750`，否则 `38.5e6 / baud` |
| 候选帧超时 100 ms | 保留，只作**候选帧停滞兜底**，不是 RTU 帧间隔、也不是整帧接收时限 | `MB_FRAME_TIMEOUT_MS = 100`，仅清停滞候选 |
| 点表是否够用 | 基础测试够；补版本寄存器 **5035/5036**，升级兼容入口随 ARM 约定补 | 点表基址 **5000**，5010..5034 占位，5035/5036 绑 `s_u16AppVersion/s_u16AppBuild` |

## 14. 实施记录

- 新增：`Platform/Protocol/Inc/Protocol.h`、`Platform/Protocol/Src/Protocol.c`、`Platform/Protocol/Modbus/Inc/{ModbusRtu,ModbusTbl,ModbusProto}.h`、`Platform/Protocol/Modbus/Src/{ModbusRtu,ModbusTbl,ModbusProto}.c`、`UserApp/Inc/AppModbus.h`、`UserApp/Src/AppModbus.c`；
- 修改：`main.c`（`ApplicationInit()` 调 `AppModbusInit()`；`TaskCreate()` 注册 `ProtocolTask` 1 ms）、`CMD/2837xS_FLASH_APP_CLA_lnk.cmd`（新增 `.protobuf`）、`tools/build_minimal.py`（5 个源文件 + 3 个 include 路径）、`.cproject`（3 个 include 路径）；
- 首轮实现曾按"16 位回调 + 协议服务放主循环"写（v0.2），按 m06854 改为现在这三层表驱动结构；`ModbusSci.{h,c}` 已删除，由 `ModbusProto.{h,c}` 取代；
- 编译期修掉的错误：`Sint16`/`Sint32` 在 C28x 头文件里不存在，改用工程自带的 `int16`/`int32`；
- 新增主机测试 `Platform/Protocol/Test/modbus_stub_test.c`（5 组、`Check(` 位置数见包内 README），
  并把 `int16`/`int32` 补进主机 shim `Platform/Port/Test/shim/F2837xS_device.h`；
- CCS 的源路径排除项扩展为 `Platform/Port/Test/,Platform/Protocol/Test/`，测试不进固件。

## 15. v0.4 返工记录（对复审五项 + ARM 入口）

### 15.1 浮点编解码

`(Uint32)float` 与 `(float)Uint32` 是数值转换，不是位模式转换。`ModbusTbl.c` 改为
`MbFloatToBits()` / `MbBitsToFloat()`（`memcpy(&u32Bits, &f, sizeof(float))` 双向），
上下限比较前先 `MbIsNanOrInf()`（`(u32Bits & 0x7F800000UL) == 0x7F800000UL`）拒 NaN/Inf。

### 15.2 时间方向

`ModbusRtuPoll()` 两处改为 `u32Elapsed = (Uint32)(pst->u32LastByteTick - u32Tick);`，
与 Timer0 向下计数一致，回绕由无符号减法自然处理；`u32Tick == 0` 不再当作无效值
（`Feed()` 传 `u16TickValid = 1U`）。

### 15.3 批次配额

`MB_RX_CHUNKS_PER_ROUND` 由 2 提到 **8**（`SCI_PORT_RX_QUOTA` 仍为 16，故每轮上限
`8 × 16 = 128`，正常情形由 32 字节总配额先截断），保留每端口每轮 32 字节总配额。

### 15.4 配额耗尽不算静默

`MbProtoService()` 的接收循环引入 `u16Drained`：只有"本轮把端口读空或取满末段"才
`ModbusRtuPoll()`；否则累加 `ModbusProtoRxBacklog()` 并跳过本轮确认。
`ModbusRtuFeed()` 在 `MB_RX_COMPLETE` 收到新字节时改为"先执行上一帧、再把该字节当新帧起点"，
不再直接丢弃已收满的候选帧。

### 15.5 回复忙

`MbExecute()` 开头加门禁：`u16TxLen != 0` 时计 `u32RxIgnored`、丢弃候选、不执行功能码，
从根上避免"上一回复未提交完就被下一请求覆盖工作缓冲"。

### 15.6 ARM 升级入口

- `ModbusProtoSetArmHook(void *pCtx, MB_ARM_HOOK pfHook)`；
- 六字节起点 `01 10 04 38 00 02` 按字节累积匹配（不匹配即清零重找），命中后调回调；
- 回调返回 >0 表示接管：置 `u16ArmPending`，用 `ModbusRtuOnStreamBreak()` 断开当前 Modbus 帧，
  再用 `ModbusRtuLoadReply()` 装载握手回复（不覆盖已有待发回复，长度 0 视为不接管）；
- 握手回复**提交完成**后才置 `u16Armed = 1`，此后 `ModbusProtoService()` 立即返回、不再读端口；
- `ModbusProtoArmRelease()` 由 ARM 侧在 Xmodem 结束时调用，交还端口。

### 15.7 点表与容量

- 点表基址改为 **5000**（5000 U16 模式、5001 int16 偏移、5002 零占位、5003/5004 U32 计数、
  5005/5006 int32 微调、5007/5008 float 增益、5009 U16 只读状态）；
- 5010..5034 由 `s_au16Reserved0[25]` 占位（可读，值为 0）；
- 5035/5036 为应用版本/构建号（`0x0100` / `0x0001`）；
- `MB_MAX_REGISTERS` 32 → **64**，`MB_TX_BUF_SIZE` 128 → **144**，`.protobuf` 由 `0x54C` 增到 `0x5D0`。

### 15.8 本轮仍未做的

- 两个主机测试文件**未在实施方执行**（本机无 `cc`/`gcc`），只做过 `cl2000` 语法/类型检查；
- ARM 握手字节仍等待 ARM 侧约定：当前 `AppMbArmHook()` 返回 0（不接管），
  约定后填 `s_au16ArmReply[]` 与 `s_u16ArmReplyLen` 即可；
- 三路 Modbus 上板收发尚未开始。

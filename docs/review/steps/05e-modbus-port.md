# 步骤 05E 申请：参考 Modbus 从机层落到本工程端口层

- 基线：`test/board-bringup-20261031` @ `61f4273`（05D r5，自造适配层 + `ModbusTbl`）
- 本步：把参考工程 `D:\IB800_Andy\IB800_2.1.1\IB800U` 的 Modbus 协议层逐字搬过来，重写应用层，重生成点表
- 顺序说明：按 `HANDOVER_20261110.md` §7 的约定，本步**先做到编译通过，再出本文档**，与「先申请后动手」的一般流程相反
- 状态：**两态编译通过（44 个编译单元，0 警告 0 错误）**；主机测试已写、**实施方未执行**（本机无 `cc`/`gcc`）

---

## 0. 结论先行

1. 参考工程的 8 个 Modbus 文件原样进入 `Platform/Protocol/Modbus/`，只把设备头 `F28003x_device.h` 换成 `F2837xS_device.h`（3 处：`ModbusPublic.h:14`、`ModbusRegProcess.h:14`、`ModbusSlave.h:15`）。**上一轮自造的适配层全部删除**：`ModbusRtu.{h,c}`、`ModbusProto.{h,c}`、`ModbusTbl.{h,c}`、`AppModbusProtoTbl.{h,c}`，以及两个基于它们的旧主机测试。
2. `UserApp/Src/AppModbus.c` 按参考 `ModbusSlaveComm.c` 的形状重写：三个 `ST_MODBUS_SLAVE_TABLE_CRTL`（A 口从机地址 1、C 口 1、D 口 2），`ProtocolTask` 仍是唯一调度入口。业务回调缩到参考工程的原始职责——只认「写 1080..1081」置升级标志。
3. 440 点表按参考的 7 字段 `ST_REG_SLAVE_ITEM` 格式重新生成，落在 `UserApp/Src/AppModbusRegCfg.c`（与参考工程把点表单独放 `ModbusSlaveRegCfg_1.c` 的分法一致）。
4. 代码量下降、Flash 下降：`.text` 少 1063 字、`FLASH_APP` 少 3465 字（生产态）。
5. 有 **4 项待裁定**（§9），其中两项是参考层对 Modbus 规范的偏离，一项是本工程的表里**没有 1080/1081**，所以升级握手永远不触发。

---

## 1. 范围与不做

**做**

- 参考协议层落地（8 个文件）与文件退休（10 个文件）
- 应用层 `AppModbus` 重写、点表重生成、`AppRegCfgReset()` 上电清零
- 数据空间改放 `.mbregs`（RAMGS1），把 RAMGS0 从「只剩 2 字」救回来
- 两态构建台账、主机测试、构建脚本源列表更新

**不做**

- 端口层 `SciPort` 一行不改（接收批次、发送环、五态恢复全部沿用 05B/05C）
- `Protocol` 框架一行不改（`ST_PROTO_OPS`、`ProtocolTask`、`ProtocolAddInstance` 语义不变）
- `main.c` 一行不改（`AppModbusInit()` 的调用点与顺序正好满足要求：在 `SciPortInit()` 之后、`ProtocolTask` 启动之前）
- 参考层的规范偏离不改（见 §9），逐字照搬
- 业务变量不接（点表里仍是 440 个占位寄存器的统一数组，等业务层定变量）

---

## 2. 文件清单

`git diff --stat HEAD`：15 个文件变更，+476 / −4366 行；另有 12 个新增文件未跟踪。

| 动作 | 路径 | 字节 | 说明 |
| --- | --- | ---: | --- |
| 新增 | `Platform/Protocol/Modbus/Inc/Modbus.h` | 3,416 | 参考层，逐字 |
| 新增 | `Platform/Protocol/Modbus/Inc/ModbusPublic.h` | 2,216 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Inc/ModbusRegProcess.h` | 6,178 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Inc/ModbusSlave.h` | 9,709 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Src/Modbus.c` | 1,715 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Src/ModbusPublic.c` | 5,303 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Src/ModbusRegProcess.c` | 13,587 | 同上 |
| 新增 | `Platform/Protocol/Modbus/Src/ModbusSlave.c` | 23,819 | 同上 |
| 重写 | `UserApp/Src/AppModbus.c` | 22,375 | 本步新写 |
| 重写 | `UserApp/Inc/AppModbus.h` | 5,346 | 本步新写 |
| 新增 | `UserApp/Src/AppModbusRegCfg.c` | 68,792 | 生成器产出 |
| 新增 | `UserApp/Inc/AppModbusRegCfg.h` | 5,517 | 生成器产出 |
| 新增 | `Platform/Protocol/Test/modbus_slave_test.c` | 28,232 | 本步新写，未执行 |
| 新增 | `Platform/Protocol/Test/shim/DrvCfg_EP15K.h` | 501 | 只声明 `extern ST_SCI_CFG stSciCfg;` |
| 改 | `Platform/Port/Test/shim/Hw_TimeBase.h` | — | 补 `Hw_TimeBaseElapsed()` 声明 |
| 改 | `Platform/Port/Test/shim/Hw_SciDriver.h` | — | 补 `ST_SCI_CFG_ITEM` / `ST_SCI_CFG` |
| 改 | `tools/build_minimal.py` | — | 源列表换 6 个文件（见 §7） |
| 删 | `Platform/Protocol/Modbus/Inc/{ModbusProto.h,ModbusRtu.h,ModbusTbl.h}` | — | 自造层 |
| 删 | `Platform/Protocol/Modbus/Src/{ModbusProto.c,ModbusRtu.c,ModbusTbl.c}` | — | 自造层 |
| 删 | `UserApp/Inc/AppModbusProtoTbl.h`、`UserApp/Src/AppModbusProtoTbl.c` | — | 旧的 8 字段点表 |
| 删 | `Platform/Protocol/Test/{modbus_proto_stub_test.c,modbus_stub_test.c}` | — | 依赖自造层，已被新测试取代 |

全仓库 `grep -rn "ModbusRtu\|ModbusProto\|ModbusTbl\|AppModbusProtoTbl\|g_astAppProtoPointTbl\|ST_MODBUS_POINT"` 除 `device/` 外无任何命中。

---

## 3. 三层职责与调用链

```
ProtocolTask()                 Platform/Protocol/Src/Protocol.c   1 ms 任务，遍历三个实例
  └─ AppMbProtoService(inst)   UserApp/Src/AppModbus.c             本步新写：端口 <-> 协议层
       ├─ SciPortPopBatch()    Platform/Port/Src/SciPort.c         取接收批次
       ├─ ModbusSlaveHandle()  Platform/Protocol/Modbus/Src/Modbus.c
       │    ├─ ModbusSlaveParamUpdate()   地址/表切换生效
       │    └─ ModbusSlaveDataHandle()    校验、分派、组应答（原地）
       │         ├─ ProcMbReadRegMsg / WriteSingle / WriteMulti
       │         ├─ ReadModbusRegisterData()   二分查表
       │         └─ SlaveModbusCallBack()      业务回调（无条件调用）
       └─ SciPortWriteTx()     Platform/Port/Src/SciPort.c         回写，分批
```

参考层完全不碰硬件：收发全靠调用方给 `ST_MDS_DATA`（`u16DataLenth` + `pu16DataBuff`），应答原地写回同一个缓冲。这正是它能直接落在这个端口层上的原因。

---

## 4. 桥接设计（本步唯一的新逻辑）

### 4.1 每端口状态

```c
typedef struct {
    ST_MODBUS_SLAVE_TABLE_CRTL stCtl;   /* 参考层的协议实例控制块 */
    ST_MDS_DATA                stMds;   /* 交给参考层的收发描述 */
    Uint16                    *pu16Data;/* 指向 .mbregs 里的 256 字数据空间 */
    Uint16 u16RxLen;      /* 已凑到的请求字节，0 = 没有候选帧 */
    Uint16 u16TxLen;      /* 待发字节总数 */
    Uint16 u16TxOff;      /* 已交给端口层的字节数 */
    Uint32 u32RxTick;     /* 最后一批字节的观察时标 */
    Uint32 u32RxFrames;   /* 组帧次数 */
    Uint32 u32TxFrames;   /* 完整送出的应答帧数 */
    Uint32 u32RxDropped;  /* 丢弃的半帧字节数 */
} ST_APP_MB_PORT;
```

### 4.2 服务五步（`AppMbProtoService`）

1. **先发后收**：`u16TxOff < u16TxLen` 时先把剩余应答往端口层送；没送完就 `return 1`，本轮不碰接收。原因是请求与应答共用同一块 256 字数据空间，边发边收会把还没发出去的应答覆盖掉。
2. **端口非空闲则丢半帧**：`SciPortRxState(sciId) != SCI_RX_IDLE` 说明接收侧在做软重同步/硬件复位，此时缓冲里拼着的半帧已经没有意义，计入 `u32RxDropped` 后清零。
3. **取批次**：最多 8 条记录、32 字节，取到新字节就刷新 `u32RxTick`。
4. **判静默**：没有候选帧、或本轮配额用尽、或 `t3.5` 未到，都直接返回。
5. **组帧并回写**：交给 `ModbusSlaveHandle()`，拿到非 NULL 帧且请求不是广播时，把应答长度交给第 1 步去发。

### 4.3 `t3.5` 的口径

用**时间戳差**判帧边界，不是参考工程那种「轮询计数到 18000」（`MODBUS_TIMEOUT`）——1 ms 任务周期不是帧间静默的度量。

| 波特率 | 门限 | 来源 |
| --- | ---: | --- |
| 19200（A 口） | 2006 µs | `ceil(38500000 / 19200)`（11 位/字符 × 3.5 字符） |
| 38400（D 口） | 1750 µs | 规范 >19200 固定 1750 µs |
| 115200（C 口） | 1750 µs | 同上 |

时标取自 `ST_SCI_RX_SEG.u32Tick`（端口层记录最后一批字节的时刻），用 `Hw_TimeBaseElapsed()` 拿「过去多久」。`Hw_TimeBaseIsReady()` 为假时退化为「本轮没取到新字节就算静默」——老平台上也能量出一个可用下限。

### 4.4 发送分批

端口层 TX 环每口 128 字，而 0x03 读 120 个寄存器的应答是 `3 + 240 + 2 = 245` 字节，一次 `SciPortWriteTx()` 永远装不下（该接口是整批或全拒）。所以：

```
while (u16TxOff < u16TxLen) {
    if (SciPortIsTxSuspended(id)) break;
    u16Want = min(u16TxLen - u16TxOff, SciPortTxFree(id));
    u16Got  = SciPortWriteTx(id, &pu16Data[u16TxOff], u16Want);   /* 0 = 拒收 */
    if (u16Got == 0U) break;                                      /* 下轮再试 */
    u16TxOff += u16Got;
}
```

全部交出去之后才清 `u16TxLen/u16TxOff` 并 `u32TxFrames++`。

### 4.5 为什么只有一个数据缓冲

参考层把应答**原地**写在请求缓冲上（`memset(pu16DataBuff, 0, ...)` 后重新打包），所以每端口只能有一个 256 字缓冲，收发必须串行。这和参考工程的 `static Uint16 MdsBuff_1[256]` 是同一个约束，不是本步的取舍。

### 4.6 广播不回复（有意偏离参考层）

参考层在收到地址 0 的广播帧时，注释写着「不回复，这里不能直接 return」，然后照常分派——结果是把请求原样回写出去。本层在 `AppMbDispatchFrame()` 里记住请求地址，`0` 时执行但**不提交应答**。

### 4.7 取批次的陷阱

`SciPortPopBatch()` 在**一条记录装不进 `u16Max`** 时也返回 0，与「端口已空」不可区分。若把这个 0 当成「取空了」，就会把半帧当整帧提交。所以 `AppMbDrainRx()` 在剩余空间不足 `APP_MB_CHUNK_BYTES`（16 字）时**直接返回，不去调 `PopBatch`**。

同理，只有「配额没用尽」才允许组帧：配额用尽说明环里可能还有同一帧的尾巴，此时提交会把一条请求拆成两条。

---

## 5. 点表

### 5.1 格式

`const ST_REG_SLAVE_ITEM g_astAppRegSlaveItem[440]`，每行 7 个字段：

```c
{     0U, READ_WRITE, TYPE_UINT16, NOT_SAVE, (U_REG_VAL *)&s_au16AppRegs[  0],       0,  65535U },
{   501U, READ_WRITE, TYPE_INT16 , NOT_SAVE, (U_REG_VAL *)&s_au16AppRegs[ 59],  -32768,  32767U },
{  5000U, READ_ONLY , TYPE_UINT16, NOT_SAVE, (U_REG_VAL *)&s_au16AppRegs[330],       0,  65535U },
{  8032U, READ_ONLY , TYPE_UINT16, NOT_SAVE, (U_REG_VAL *)&s_au16AppRegs[439],       0,  65535U },
```

尾部：

```c
const ST_REG_SLAVE_CFG stRegSlaveCfg_App = {
    (ST_REG_SLAVE_ITEM *)&g_astAppRegSlaveItem[0],
    (Uint16)(sizeof(g_astAppRegSlaveItem) / sizeof(g_astAppRegSlaveItem[0]))
};
```

`READ_WRITE`/`READ_ONLY`/`TYPE_UINT16`/`TYPE_INT16`/`NOT_SAVE` 全部来自参考层的**枚举**（`ModbusRegProcess.h`），旧的同名宏随 `ModbusTbl.h` 一起删除——这是必须删的理由，不是清算。

### 5.2 结构事实（测试断言按这些写）

| 项 | 值 |
| --- | --- |
| 项数 | 440（= 协议表行数） |
| 地址范围 | 0 ~ 8032 |
| 从 0 起的连续段 | 58 项 |
| 地址空洞 | 9 处（`57→500`、`566→1000`、`1061→1500`、`1581→2000`、`2018→2511`、`2511→3000` 等） |
| 有符号（int16） | 49 项，下限均为负 |
| 只读 | 110 项（地址 ≥ 5000） |
| 下限 > 上限 | 0 项 |
| 类型 | 只有 `TYPE_UINT16` / `TYPE_INT16` |
| 保存属性 | 全部 `NOT_SAVE` |

`ReadModbusRegisterData()` 是**二分查找**，地址必须严格升序——生成器里有断言，主机测试里也有一条不变量。

### 5.3 生成器

`_tmp/gen_proto_tbl.py`（不在仓库内，交付包附在 `tools/`）：读 `D:/Secret/modify/协议.xlsx` 的 `Sheet1`，列 1..4 = 地址/名称/单位/备注；`有符号` 或 `-N ~ M` 判有符号；限值取备注里的 `a~b`（没有就取 `0~N`）；`地址 < 5000` 只写、否则只读。输出 `generated 440 points: signed=49 readwrite=330`。

本步在生成器里加了 `AppRegCfgReset()`：`.mbregs` 是 **UNINITIALIZED** 段，不参加 C 运行时清零，不加这一步主机上电读到的是随机值。

### 5.4 存储

```c
static Uint16 s_au16AppRegs[APP_MODBUS_REG_COUNT];      /* AppModbusRegCfg.c */
#pragma DATA_SECTION(s_au16AppRegs, ".mbregs")
```

`g_astAppRegSlaveItem` 每项 8 字（`u16RegAddr` 1 + bitfield 1 + `puRegVal` 2 + 两个 32 位限值 4），440 项 = 3520 字，放 `.const`（Flash `0008b4b0..0008c570`）。

---

## 6. 内存与链接

| 段 | 位置 | 长度（字） | 内容 |
| --- | --- | ---: | --- |
| `.mbregs` @ `0x00D000` | RAMGS1 | `0x4B8` = 1208 | `AppModbus.obj` 768（3 口 × 256 数据空间）+ `AppModbusRegCfg.obj` 440（点表存储） |
| `.protobuf` @ `0x00D4B8` | RAMGS1 | `0x48` = 72 | `AppModbus.obj` 36（3 × `ST_PROTO_INSTANCE`）+ `Protocol.obj` 36 |
| `.portstat` @ `0x00D500` | RAMGS1 | `0x14` = 20 | 05C 遗留 |
| `.portbuf` @ `0x00DC00` | — | `0x180` = 384 | 05C 遗留 |

**为什么数据空间必须放 `.mbregs`**：首版把 `s_au16DataSpace[3][256]` 内嵌在 `s_astPort[]` 里，跟着 `.bss` 落在 RAMGS0，链接结果是 `RAMGS0 0000c000 00001000 00000ffe 00000002` —— **只剩 2 字**。改成独立数组 + `#pragma DATA_SECTION(..., ".mbregs")` 后生产态余 `0xbc`（188 字）。语义上也对：参考工程这块就叫「Modbus 数据空间」。

---

## 7. 构建

`tools/build_minimal.py` 的源列表：删 `Platform/Protocol/Modbus/Src/{ModbusRtu.c,ModbusTbl.c,ModbusProto.c}`、`UserApp/Src/AppModbusProtoTbl.c`；加 `Platform/Protocol/Modbus/Src/{Modbus.c,ModbusSlave.c,ModbusRegProcess.c,ModbusPublic.c}`、`UserApp/Src/{AppModbus.c,AppModbusRegCfg.c}`。

```
python tools/build_minimal.py --cgt C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS
```

44 个编译单元，0 警告 0 错误。

---

## 8. 两态构建台账

`PORT_BENCH_TEST` 由 `_tmp/p_bench_switch.py 0|1` 切换（`Platform/Port/Inc/PortBench.h:11`）。

### 8.1 本步（05E）

| 项 | 生产态 `0U` | 测量态 `1U` |
| --- | --- | --- |
| `.text` | `0x53B9`（21,433 字） | `0x5738`（22,328 字） |
| `RAMGS0` | 用 `0xF44`，余 `0xBC`（188 字） | 用 `0xFFE`，**余 2 字** |
| `RAMGS1` | 用 `0x514`，余 `0x6EC`（1,772 字） | 用 `0x58E`，余 `0x672`（1,650 字） |
| `FLASH_APP` | 用 `0x69AE`（27,054），余 `0x13650` | 用 `0x6D38`（27,960），余 `0x132C6` |
| `.out` | 855,456 B<br>sha256 `f62a7b7f49af85a7f898eded30558ecdb35cab460b000f27cf401a981ee87517` | 873,988 B<br>sha256 `0be9225d1bcb48e81488726e3c4fdb668d36f4d9522ad3eb410cf7539d96013b` |
| `.map` | 110,517 B | 115,772 B |

- `.out` 的 sha256 可复现：生产态连续构建三次完全一致。
- `.map` 的 sha256 **不可复现**（每次构建都变），只能作信息项；判断是否同一份产物看 `.out`。

### 8.2 与基线对照

基线 `61f4273`（同一脚本、同一编译器、同一个 detached worktree 跑出来）：

| 项 | 基线 生产 | 05E 生产 | 基线 测量 | 05E 测量 |
| --- | --- | --- | --- | --- |
| `.text` | `0x57E0`（22,496） | `0x53B9`（21,433） | `0x5B5F`（23,391） | `0x5738`（22,328） |
| `FLASH_APP` 用 | `0x7737`（30,519） | `0x69AE`（27,054） | `0x7AC3`（31,427） | `0x6D38`（27,960） |
| `RAMGS0` 余 | `0x10A`（266） | `0xBC`（188） | 2 | 2 |
| `RAMGS1` 余 | `0x458`（1,112） | `0x6EC`（1,772） | `0x42E`（1,070） | `0x672`（1,650） |
| `.out` | 867,572 B | 855,456 B | 885,912 B | 873,988 B |

结论：`.text` 少 **1,063** 字、`FLASH_APP` 少 **3,465** 字；RAMGS0 生产态从 266 字降到 188 字（少了 78 字，`s_astPort[]` 里的收发簿记），仍有余量；测量态 2 字是**基线就存在**的边界，不是本步引入。

---

## 9. 待裁定（4 项）

### 9.1 协议表里没有 1080/1081，升级握手永远不触发

05D 的 ARM 升级握手认「写 1080..1081（功能码 0x10）」。检查生成的 440 项表：**1080 与 1081 都不在表内**（`57→500`、`566→1000`、`1061→1500` 两处空洞正好把它排除在外）。于是这个写请求必然走 `ERROR_ADDRESS`，标志永远不置起。

本步的做法：保留参考工程的形状（`stMdsUpdateParam.u16MdsIsUpdate` 置位、`AppModbusUpgradeFlag()` 读），但要求这一帧本身 `eErrorType == DATA_OK` 才置位（见 9.3）。

- 选项 A：在协议表里补 1080/1081 两项（需要业务方确认这两个地址真的归升级用）。
- 选项 B：删掉 `AppMbUpgradeWatch()` 与 `AppModbusUpgradeFlag()`，等升级流程确定再加回来。
- 现状：A 未做，B 未做，先如实上报。

### 9.2 异常帧的两个字节不合 Modbus 规范

`ModbusPackErrorReply()`（`ModbusSlave.c:241`）组的是：

```
[ addr, MODBUS_ERR_BASE_CODE(0x80) + eErrorType, HI_UINT16(u16RegStartAddr), crcLo, crcHi ]
```

规范要求是 `[ addr, 请求功能码 | 0x80, 0x00, crcLo, crcHi ]`。两处偏离：

1. 第 2 字节用 **0x80 + 错误码**，所以「读越界」回的是 `0x82`，不是 `0x83`；
2. 第 3 字节塞的是**起始地址高字节**，不是固定的 `0x00`。

本步**逐字照搬未改**（改动参考层就不叫「搬」了），主机测试也按实际行为断言。要不要就地改成规范行为，请裁定；一旦要改，主机测试里那两条断言跟着改。

### 9.3 参考层在错误帧时也调业务回调

`ModbusSlaveDataHandle()` 的顺序是：`eErrorType = pFunc(...)` → **无条件** `SlaveModbusCallBack()`。所以「写被拒绝」（地址不在表、数据越限）时回调照样进来。

本步在 `AppMbUpgradeWatch()` 里加了 `pstFrame->eErrorType == DATA_OK` 守卫（这是**我们自己的业务层**，不是参考层）。一并上报参考工程：业务回调是否该带上成功/失败信息。

### 9.4 测量态 RAMGS0 只剩 2 字

基线就是这么紧（见 §8.2），本步没有恶化。但测量态任何新增全局变量都会链接失败，建议在 05F 之前给 RAMGS0 减负或扩段。

---

## 10. 主机测试（请审阅方执行）

实施方**没有** `cc`/`gcc`/`clang`/`tcc`/`cl`，测试文件只做过 `cl2000` 的语法/类型检查（0 错误，2 条 shim 自身的 warning），**未执行**。

```
cd Platform/Protocol/Test
cc -std=c99 -Wall -Wextra \
   -I shim -I ../../Port/Test/shim -I ../../Port/Inc -I ../../Common/Inc \
   -I ../Inc -I ../Modbus/Inc -I ../../../Drivers/inc -I ../../../UserApp/Inc \
   modbus_slave_test.c \
   ../Src/Protocol.c \
   ../Modbus/Src/Modbus.c ../Modbus/Src/ModbusSlave.c \
   ../Modbus/Src/ModbusRegProcess.c ../Modbus/Src/ModbusPublic.c \
   ../../../UserApp/Src/AppModbus.c ../../../UserApp/Src/AppModbusRegCfg.c \
   -o modbus_slave_test && ./modbus_slave_test
```

期望：末行 `modbus_slave_test: 65 项检查，0 项失败`，退出码 0。建议再加 `-fsanitize=address,undefined` 跑一遍。

替身的建模口径（按真实接口语义，不按方便测试）：

- `SciPortPopBatch()` 每条记录最多 8 字节、自带观察时标；**一条记录装不进 `u16Max` 时返回 0**（与真实接口同）；
- `SciPortWriteTx()` 整批装不下就一个字节都不收、返回 0；
- `Hw_TimeBase*` 是向下计数，`Elapsed(start) = (Uint32)(start - now)`，1 tick = 1 µs。

15 组用例（合计 65 条断言）：

| # | 用例 | 断言要点 |
| --- | --- | --- |
| 1 | 表结构 | 项数、`u16Num`、`pstReg` 指向、严格升序、限值、类型/属性、绑定、`NOT_SAVE`、有符号点下限 |
| 2 | 二分查找 | 首/中/末项命中；表尾之后 `-2`；空洞地址 `-3` |
| 3 | 0x03 读 2 个寄存器 | 9 字节、字节数 = 4、CRC 低字节在前 |
| 4 | 0x06 写入 | 应答是请求原样回显（8 字节） |
| 5 | 读回 | 高/低字节 = 刚写进去的 `0x12`/`0x34` |
| 6 | 广播写 | 不回复，但寄存器确实改了 |
| 7 | CRC 翻一位 | 不回复 |
| 8 | 非本站地址 | 不回复 |
| 9 | 读 32767（越界） | 5 字节异常帧、第 2 字节 `0x82`、第 3 字节 `0x7F` |
| 10 | 半帧 | 帧内只等 1000 µs（< 2006）不组帧；补齐后回复 7 字节 |
| 11 | 两帧 0 间隔粘连 | CRC 不过、不回复、不拆成两条 |
| 12 | 写 1080 | 5 字节异常帧、`0x82`、升级标志保持 0 |
| 13 | 长回复跨轮 | 58 个寄存器 → 121 字节，首轮只给 64 字空闲量，分批送完，总长/字节数/CRC 全量校验 |
| 14 | 端口非空闲 | 半帧计入 `AppModbusRxDropped`，不回复 |
| 15 | 时基不可用 | 退化为「本轮没取到新字节就算静默」 |

（表里 15 行，第 1 行含 9 条、第 2 行含 5 条断言，合计 65 条 `Check`。）

---

## 11. 验收要点

1. `tools/build_minimal.py` 两态都能过，0 警告。
2. 生产态 `.out` sha256 = `f62a7b7f…`（同工具链下应可复现）。
3. `grep` 旧符号无命中（§2 末）。
4. 主机测试 65 项全过。
5. 上板：任意 Modbus 主站按 1/1/2 三个从机地址、波特率 19200/115200/38400 分别读 0 号起 2 个寄存器，应收到 9 字节应答；写 0 号寄存器后读回应为写入值。

---

## 12. 与 05D 的差异汇总（被删掉的东西）

| 05D 有 | 05E 处置 | 理由 |
| --- | --- | --- |
| `ModbusRtu/ModbusProto` 自造 RTU 状态机 | 删除 | 参考层就是 RTU 从机，重复实现 |
| `ModbusTbl` 8 字段点表 + 浮点编解码 | 删除 | 参考层 7 字段表只能 int16/uint16，浮点缩放归业务层 |
| 每端口 `ST_PROTO_INSTANCE` + `pfService` 里管接收 | 保留框架，服务函数重写 | 端口层接口不变 |
| XMODEM `'C'` 就绪应答、`Hw_SciSetBaud(115200)` 切速 | **删除** | 参考工程里没有这段（`User/ModbusSlave/` 与 `Platform/Modbus/` 全量 grep 无 `0x43`/`C`/`Xmodem`），是 05D 自造 |
| 版本寄存器 5036/5037 | 随旧点表一起消失 | 新点表按协议表现生成，没有这两个地址 |

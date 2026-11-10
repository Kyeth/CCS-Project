# 步骤 05E-r2 申请：按复核意见修 05E 的功能缺陷

- 上游：`docs/review/steps/05e-modbus-port.md`（05E r1，已交付 `1ceb1ce`）
- 复核意见：本轮五组功能缺陷 + 四项裁定 + 一处兼容性遗漏（见 §1 对照表）
- 本步：只修这些功能缺陷，不重写架构、不动通信调度、不做性能微调
- 编译：两态均通过（44 个编译单元，0 警告 0 错误）
- 主机测试：已按新行为改写，本机无主机 C 编译器，只做了 cl2000 语法检查，**执行仍请复核方进行**

## 0. 结论先行

1. 五组缺陷全部修完，每一组的处置都能在 §1 的表格里对应到具体函数。
2. 异常应答改成 Modbus 规范格式 `地址 | 原功能码 or 0x80 | 异常码 | CRC`；CRC 错的帧一律丢弃、不回复。
3. 写路径改成"先验后写"：0x10 分两遍（第一遍全验、第二遍提交），任何一项不合法就整条不写，不再出现"写了一半还回成功"。
4. 收帧判据重做：帧边界只看两处真实静默（相邻批次之间、最后一条批次之后），删掉"本轮没取到新字节就算静默"的退化规则；"取空"必须复查端口状态才成立。
5. 强制复位/断流的回复失效处理接回来了：故障代次一变就丢候选、丢未发完的回复，不续发旧帧尾部、不自动重发。
6. 点表存储改成每项一个真正的 `U_REG_VAL`，消掉未对齐联合体访问；同时给 5035/5036 补上版本号初值 `0x0007` / `0xB05C`。
7. 升级握手按裁定回到六字节入口 `01 10 04 38 00 02`，应答 9 字节，**确认发送完成之后**才置升级标志并把端口交出去。

## 1. 复核意见与本步处置

| # | 复核意见 | 本步处置 | 位置 |
| --- | --- | --- | --- |
| 1 | 0x10 可能误写并回成功：不查字节数与帧长、忽略 `SetRegValByType()` 返回值 | 0x10 先算 `u16ByteNum`，校验 `u16ByteNum == 2n` 且帧长 `== 9 + u16ByteNum`；再分两遍：第一遍只验地址/权限/类型/区间/限值，第二遍才提交 | `ModbusSlave.c` `ProcMbWriteMultiRegMsg()` |
| 1b | 0x03 / 0x06 同样要补准确长度检查 | 0x03 要求帧长 8，个数 1..120，并先整段验地址再组帧；0x06 要求帧长 8，`LoadRegValFromMdsMsg` 必须返回 1 | 同上三个 `ProcMb*` |
| 2 | 收帧适配重新引入"短批次等于取空"（`u16Got < u16Max` 就判空） | 短批次继续读；只有"剩余空间能装下一条完整批次（8 字节）"时 `PopBatch` 返回 0 才算取空，并且要复查断流/接收态/故障代次；空间不足时丢候选 + `SciPortResync()` | `AppModbus.c` `AppMbDrainRx()` |
| 2b | 相邻批次已出现帧间隔时不能无条件拼接 | 每条新批次到达时比较它与上一条批次的时标，间隔 ≥ 3.5 字符就把前一条候选先交出去，新批次另起一帧 | 同上 |
| 2c | 时间源失败时停止确认帧，删掉降级规则 | `AppMbFrameIdle()` 在 `Hw_TimeBaseIsReady()==0` 时直接返回"未静默"；服务流程在时基不可用时提前返回 | `AppModbus.c` |
| 3 | CRC 错与异常回复必须按规范 | 异常帧 `buff[1] = bFuncCmd | 0x80`、`buff[2] = 异常码`；CRC 错（含广播）一律 `return NULL` | `ModbusSlave.c` `ModbusPackErrorReply()` / `ModbusSlaveDataHandle()` |
| 4 | 强制复位后的回复失效处理丢了 | 每端口记 `u16FaultGen`，代次变化即丢候选 + 丢未发完回复（计入 `TxDropped`）；`SciPortIsTxSuspended()` 时先解冻再收帧 | `AppModbus.c` `AppMbProtoService()` 步骤 1/6 |
| 5 | 点表强转触发 UBSan（`ModbusRegProcess.c:137`） | 生成器改为 `static U_REG_VAL s_auRegVal[440]`，行模板直接取 `&s_auRegVal[k]`，不再强转 | `_tmp/gen_proto_tbl.py` → `AppModbusRegCfg.c` |
| 5b | "每项下限 ≤ 上限"是测试把负下限转成 Uint32 造成的 | 判据改成 `(int64)` 比较，点表负下限保持不动 | `modbus_slave_test.c` `TestTable()` |
| 裁 1 | 1080/1081 不必补点表；按六字节入口识别 `01 10 04 38 00 02`，确认发送完成后才交接升级 | 桥接层识别 6 字节帧（仅 SCI-A），回 9 字节（前 6 字节回显 + CRC + `'C'`），`SciPortIsTxIdle()` 为真才置 `u16Armed`，之后该端口不再收帧 | `AppModbus.c` `AppMbIsHandshake()` / `AppMbDispatchFrame()` / 服务步骤 3 |
| 裁 2 | 参考层异常帧要修 | 见第 3 条 |  |
| 裁 3 | 错误帧也调回调的 `DATA_OK` 守卫保留，但先修"失败却回 DATA_OK" | 守卫保留在 `AppMbUpgradeWatch()`；写路径已改成一失败即回异常码 | `AppModbus.c` / `ModbusSlave.c` |
| 裁 4 | 测量态 RAM 只剩 2 字：迁移测量变量，生产协议逻辑不重做 | 本步不动测量变量，台账里如实记录；生产态余量 170 字 |  |
| 遗 1 | 5035/5036 必须分别是 `0x0007` / `0xB05C` | `AppRegCfgReset()` 清零后按地址查下标补初值（断言两项相邻） | `AppModbusRegCfg.c` |

## 2. 参考层改动

### 2.1 异常应答

```
地址 | 原功能码 or 0x80 | 异常码 | CRC低 | CRC高
```

- 0x03 访问非法地址 → `01 83 02 CRClo CRChi`（原来是 `01 82 <起始地址高字节> ...`）。
- 异常码由 `ModbusExcCodeOf()` 映射：`ERROR_FUNC`→01、`ERROR_ADDRESS`→02、`ERROR_DATA`→03、`ERROR_SLAVE_FAULT`→04、`SLAVE_BUSY`→06，其余按 03 处理。
- `MODBUS_ERR_BASE_CODE` 仍保留，但只用于功能码字节的 `or`。

### 2.2 CRC 错一律丢弃

`ModbusSlaveDataHandle()` 里 CRC 校验失败时不再区分广播，直接 `return NULL`，一帧都不回。随包用例"CRC 错误不回复""合并后 CRC 错误不回复"因此成立。

### 2.3 / 2.4 0x03 与 0x06 的长度检查

- 0x03：`u16DataLenth != 8` → `ERROR_DATA`；`u16RegNums == 0 || > 120` → `ERROR_DATA`；组帧前先用 `ModbusRegWidthOf()` 把整个地址区间过一遍，中间任一项查不到就回 `ERROR_ADDRESS`，不会出现"前几个寄存器已经写进应答缓冲、后面才发现越界"的半截帧。
- 0x06：`u16DataLenth != 8` → `ERROR_DATA`；`ReadModbusRegisterData()` 非 0 一律 `ERROR_ADDRESS`；`LoadRegValFromMdsMsg() != 1` → `ERROR_DATA`；先 `CheckRegValByType()` 通过才置固化标志、才 `SetRegValByType()`。

### 2.5 0x10 两遍提交

```c
u16ByteNum = pu16Buff[MODBUS_MULTI_WD_BYTE_NUM_INDEX];      /* 第 7 字节 */
if ((u16RegNums == 0U) || (u16RegNums > MODBUS_MULTI_WD_REG_MAX) ||
    (u16ByteNum != (Uint16)(2U * u16RegNums)) ||
    (u16DataLenth != (Uint16)(MODBUS_MULTI_WD_FRAME_BASE + u16ByteNum))) {
    return ERROR_DATA;
}
/* 第一遍：只验不写 */
/* 第二遍：重新查表 → SetRegValByType()，返回值必须为 1 */
```

复核方给的三组输入现在的行为：

| 输入 | 现在的行为 |
| --- | --- |
| 写 2 个，第一项 42 合法、第二项 2 超过上限 1 | 回 `83 03` 异常帧，两个寄存器都不动（新增回归 14） |
| 声称写 2 个、只给 1 个数据（字节数 2）、CRC 正确 | 回 `83 03`，一个寄存器都不写（新增回归 13） |
| 声称写 2 个、字节数 4、但帧长不够 | 回 `83 03`，一个寄存器都不写（长度检查） |

### 2.6 `CheckRegValByType()`（新增）

`SetRegValByType()` 拆成"只验"和"赋值"两半：

- `int16 CheckRegValByType(const ST_REG_SLAVE_ITEM *pstRegDrc, const ST_REG_SLAVE_ITEM *pstRegSrc)`：返回 `-1` 入参、`-2` 表项下限 > 上限、`-3` 未知类型、`0` 越限、`1` 合法；所有限值比较统一用 `(int64)`，不把 `i32DownLimitVal` 当 `Uint32` 用。
- `SetRegValByType()` 先调它，不是 1 就原样返回，否则只做赋值 switch。参考层的调用点全部保持原样，语义从"越限静默不写"变成"越限报错"。

## 3. 桥接层改动（`AppModbus.c`）

### 3.1 服务顺序（十步）

1. 故障代次变化：丢候选 + 丢未发完的回复（`TxDropped++`）；
2. 回复还没被端口环全部接纳：先推，本轮不接新帧；
3. 握手回复发完了 → 置 `u16Armed`，此后该端口不再收帧；
4. 端口不在 `SCI_RX_IDLE`：半帧作废；
5. 端口报断流：丢候选 + `SciPortClearStreamBreak()`；
6. 发送侧被冻结：丢回复 + `SciPortTxResume()`，解冻失败就先等；
7. 时基不可用：不确认任何帧；
8. 取批次（帧边界在函数内部处理）；
9. 端口取空 + 最后一条批次之后静默 ≥ 3.5 字符，才收帧；
10. 交给参考层，再挂上发送侧。

### 3.2 帧边界（这是本步唯一一处行为设计变化）

原来（r1）用"本轮有没有取到新字节"判静默，等于把 1 ms 任务节拍当成帧间隔。现在只看两个真实静默点：

- **批次之间**：新批次的时标与上一条批次相差 ≥ 3.5 字符 → 前面的候选自成一帧，先交出去；间隔为 0 仍然拼接，交给 CRC 判（合并帧会 CRC 错、静默丢弃）。
- **批次之后**：端口取空以后，最后一条批次的时标距现在 ≥ 3.5 字符 → 收帧。

这样慢一轮里堆积的多条批次仍会被拼成一帧（回归 15 的 17 字节请求跨 3 条批次），而两条之间真有静默的帧会被拆开。

### 3.3 "取空"的判据

`PopBatch` 返回 0 有三种含义，调用方无法区分：没有记录、记录装不进 `u16Max`、环读失败/重同步。所以：

- 只有剩余空间 ≥ 8 字节（`APP_MB_BATCH_MAX`，端口层一条批次的上限）时才把 0 当候选"取空"；
- 还要复查 `SciPortIsStreamBreak()`、`SciPortRxState()`、`SciPortFaultGeneration()`，任一异常就不算取空，本轮不确认帧；
- 空间不足（超过 `APP_MB_DATA_MAX` 256 字）时丢候选 + `SciPortResync()`，不拿半截去解析。

### 3.4 强制复位与断流

- `u16FaultGen` 每端口一个，代次变化 = 端口出过故障（断流或强制复位）。变化时丢候选、丢未发完的回复并计数，**不续发旧帧尾部、不自动重发**。
- `SciPortIsTxSuspended()` 为真时先丢回复再尝试 `SciPortTxResume()`；解冻成功才继续收帧。
- 端口恢复成 `SCI_RX_IDLE` 之后才开始收新请求（步骤 4/5）。

### 3.5 六字节升级入口

- 识别条件：SCI-A、帧长 6、逐字节等于 `01 10 04 38 00 02`（从机地址取自 `s_au16SlaveAddr[0]`，不硬编码 01）。
- 应答 9 字节：前 6 字节回显 + 现算 CRC（`PackModbusCrc`，低字节在前）+ `0x43`（XMODEM 就绪字节）。
- 命中后不走参考层，直接调业务回调；`u16Armed` 只在 `SciPortIsTxIdle()` 为真时置起 —— 即"确认发送完成后才交接升级"。
- 带数据段的 `0x10 写 1080`（帧长 13）仍然按普通写走，因为 1080 不在点表里，会得到 `83 02` 异常应答（回归 9）。两条路径靠帧长区分，不会互相干扰。

### 3.6 计数器与对外接口

新增 `AppModbusTxDropped(u16Port)`（因故障丢掉的回复数）和 `AppModbusCallBacks(u16Port)`（参考层回调次数）；`AppModbusUpgradeFlag(u16Port)` 的含义改为"该端口已交接升级"。`AppModbusTableErrors()` / `AppModbusXmodemReady()` 在 r1 已删，本步不再提。

## 4. 点表与存储

### 4.1 每项一个 `U_REG_VAL`

```c
static U_REG_VAL s_auRegVal[APP_MODBUS_REG_COUNT];   /* .mbregs */
...
{  5035U, READ_ONLY , TYPE_UINT16, NOT_SAVE, &s_auRegVal[365],       0,  65535U },
```

`U_REG_VAL` 里有 `Uint32`/`int32`/`float` 成员，联合体按 2 字对齐；把 `Uint16[]` 的元素地址强转成 `U_REG_VAL *` 会让第二个元素开始都按更宽的对齐访问，主机上 UBSan 直接报未对齐（复核方看到的位置就是 `ModbusRegProcess.c:137`）。改成每项一个联合体之后，访问路径与参考工程一致，也没有任何强转。

### 4.2 5035 / 5036 版本号

`AppRegCfgReset()` 先整块清零，再按地址查下标补初值：

```c
s_auRegVal[365].u16Val = 0x0007U;   /* 5035：DSP 版本号高字 */
s_auRegVal[366].u16Val = 0xB05CU;   /* 5036：DSP 版本号低字 */
```

生成器里按地址查下标并断言两项相邻，不硬编码序号；版本号 503900 = `0x0007B05C`。这两项是指定值，不是业务变量，不能等业务层去写。

### 4.3 `.mbregs` 容量

| | r1 | r2 |
| --- | --- | --- |
| 数据空间 3×256 | 768 字 | 768 字 |
| 点表存储 | 440 字（`Uint16[440]`） | 880 字（`U_REG_VAL[440]`） |
| `.mbregs` 合计 | 1,208 字 | 1,648 字（`0x670`） |
| RAMGS1 余量 | 1,772 字 | 1,332 字 |

## 5. 两态构建台账（r2）

`python tools/build_minimal.py --cgt C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS`

| 项 | 生产态 `PORT_BENCH_TEST (0U)` | 测量态 `(1U)` |
| --- | --- | --- |
| 编译单元 / 警告 / 错误 | 44 / 0 / 0 | 44 / 0 / 0 |
| `.out` 大小 | 860,848 B | 879,380 B |
| `.out` sha256 | `cdfbfa773c6350bb43af027dcfb353a273fbe2ab89b4e77a7a11f669c08b19e4` | `d0a06c3859c200c5a8c121081cc5f8c97b18c5ab248d12268bee1a9a28aa1e8e` |
| `.map` 大小 | 110,847 B | 116,106 B |
| `.text` | `0x55BA`（21,946 字） | `0x5939`（22,841 字） |
| `RAMGS0` 余量 | `0xAA`（170 字） | `0x2`（2 字） |
| `RAMGS1` 余量 | `0x534`（1,332 字） | `0x4A8`（1,192 字） |
| `FLASH_APP` 用 / 余 | `0x6BAF` / `0x1344F` | `0x6F39` / `0x130C5` |

- `.out` 的 sha256 可复现（连续两次构建一致）；`.map` 里的时间戳每次构建都变，sha256 只能当信息项。
- 与 r1（`1ceb1ce`）对照：

| 项 | r1 生产 | r2 生产 | 差 |
| --- | --- | --- | --- |
| `.text` | 21,433 | 21,946 | +513 |
| `RAMGS0` 余 | 188 | 170 | −18 |
| `RAMGS1` 余 | 1,772 | 1,332 | −440 |
| `FLASH_APP` 用 | 27,054 | 27,567 | +513 |

`RAMGS1` 少 440 字全部来自点表存储由 `Uint16[440]` 变成 `U_REG_VAL[440]`；`.text` 增加来自长度检查、两遍提交和异常码映射。

## 6. 主机测试变更

`Platform/Protocol/Test/modbus_slave_test.c`（904 行，110 处 `Check`）：

- 断言修正：异常帧第 2 字节 `0x82` → `0x83`、第 3 字节"起始地址高字节" → 异常码 `0x02`；"下限 ≤ 上限"改 `(int64)` 比较。
- 用例 12 重写：时基不可用时连续跑三轮都不确认帧（旧的"退化规则"用例删除）。
- 新增回归：

| # | 用例 | 断言要点 |
| --- | --- | --- |
| 13 | 截短的 0x10（字节数 2、声称 2 个） | 回 `83 03`，寄存器不变 |
| 14 | 0x10 第二项越限 | 回 `83 03`，**两项都不写**；随后限内写两项都生效、回 8 字节 |
| 15 | 17 字节请求跨 3 条批次 | 一轮内拼成一帧、正常应答、4 个寄存器都写入 |
| 16 | 121 字节长回复、环每次只放 16 字 | 逐字节与预期缓冲一致（含每个寄存器值）、只计一帧 |
| 17 | 5035 / 5036 | 直读 + 协议读都得到 `00 07 B0 5C` |
| 18 | 强制复位取消未发完的回复 | `TxDropped +1`、不再续发旧帧尾部 |
| 19 | 六字节升级握手 | 9 字节应答（回显 + CRC + `'C'`）、发完才交接、交接后不再收帧 |

- 替身同步升级：补 `SciPortFaultGeneration/IsStreamBreak/ClearStreamBreak/Resync/IsTxIdle/TxResume`；`StubTxDrain()` 改为按实际移出的字节数归还空闲量（原来直接恢复满额，会掩盖分批）；设备垫片补 `int64`。

## 7. 仍未覆盖 / 需要裁定

1. **245 字节长回复仍无实测**。当前点表最长连续段是 58 项（首地址起），0x03 最多回 121 字节；245 字节需要 120 项连续地址，本表做不到。分批逻辑本身与长度无关（由 `u16TxLen`/`u16TxOff` 驱动），回归 16 把环压到 16 字、连送 8 轮逐字节对齐，已是本表能给的最强证据。若要覆盖 245 字节，需要一张临时表或一个 120 项连续的内部测试表 —— 请裁定是否值得为此加一张测试专用表。
2. **测量态 `RAMGS0` 仍只剩 2 字**。按裁定不在本步动测量变量，生产态余量 170 字、协议逻辑不受影响。
3. **六字节握手与标准 0x10 的区分靠"端口 + 帧长"**。帧长 6 的 0x10 在规范里不合法（缺数据段与 CRC），所以只认 SCI-A、只认逐字节匹配；其它端口收到 6 字节帧会走普通路径并因长度检查被拒（回 `83 03`）。
4. **0x03 整段预检的异常码**：起始地址合法但区间跨到空洞时回 `ERROR_ADDRESS`（02）。规范允许 02，也可以回 03，本步按地址问题处理。
5. **参考层 `ModbusSlaveParamUpdate()` 的缓存机制**保留未动（`SetModbusSlaveAddr()` 的改动要等下一次 `ModbusSlaveHandle()` 才生效）。

## 8. 复核方请跑

```sh
cc -std=c99 -O2 -Wall -Wextra -I Platform/Protocol/Test/shim -I Platform/Port/Test/shim \
   -I Platform/Port/Inc -I Platform/Common/Inc -I Platform/Protocol/Inc \
   -I Platform/Protocol/Modbus/Inc -I Drivers/inc -I UserApp/Inc \
   Platform/Protocol/Test/modbus_slave_test.c Platform/Protocol/Src/Protocol.c \
   Platform/Protocol/Modbus/Src/Modbus.c Platform/Protocol/Modbus/Src/ModbusSlave.c \
   Platform/Protocol/Modbus/Src/ModbusRegProcess.c Platform/Protocol/Modbus/Src/ModbusPublic.c \
   UserApp/Src/AppModbus.c UserApp/Src/AppModbusRegCfg.c -o modbus_slave_test
./modbus_slave_test
```

期望输出末尾：`modbus_slave_test: 110 项检查，0 项失败`。

建议同时用 `-fsanitize=address,undefined` 再跑一遍：本轮已消除未对齐联合体访问，若仍有 UBSan 报告，请把位置贴回来。

# 步骤 05E-r3 申请：修正待发回复被新批次覆盖（含三处测试修正）

- 上游：`docs/review/steps/05e-modbus-port-r2.md`（05E-r2，已交付 `3624d2c`）
- 本轮范围：**一处生产逻辑 + 三处测试 + 一条覆盖回归**，不动框架、不动通信调度
- 编译：两态通过（44 单元，0 警告 0 错误）
- 主机测试：本机无主机 C 编译器，仍只做 `cl2000` 语法检查，**执行请复核方进行**

## 0. 结论先行

1. 批次间隔拆帧后，`AppMbDispatchFrame()` 的输出（上一条回复）就躺在这块数据空间里，而随后的
   新批次又从同一个起点写进去，把回复头部覆盖掉 —— 复核方实测的 `01 06 00 01 00 00 00 FA 33`
   正是"回复前 4 字节被新请求覆盖、尾部仍是旧 CRC"。
2. 修法按裁定取最小方案：**拆帧交出去的回复拥有这块缓冲**，只要回复还没全部进入端口环，就不再
   复用该缓冲；已经弹出的新批次没有第二块地方安放，明确丢弃并按字节计入 `RxDropped`。
   不新增帧队列，不扩 RAM。
3. 三处测试修正：两处异常帧功能码断言写错（`0x83` → `0x90`），一处寄存器地址编码写错
   （`0x0BB7` = 2999 → `0x0BBF` = 3007）。这三处是本方测试的错误，不是生产代码的错误。
4. 新增回归 19 覆盖该场景：一轮里处理两条间隔 3 ms 的请求，断言回复仍是完整的 9 字节、CRC 自洽、
   被丢的新批次计满 4 字节。

## 1. 待发回复被覆盖（生产逻辑）

### 1.1 触发路径

`AppMbDrainRx()` 在弹出新批次时，如果它与上一条批次的时标相差 ≥ 3.5 字符，就先调
`AppMbDispatchFrame()` 把前一条候选交给协议层。而参考层的打包**原地**写回同一个缓冲：

```c
/* 拆帧：先交出前一条候选 —— 这一步把回复写进了 pu16Data */
AppMbDispatchFrame(pstPort);
/* 紧接着又把新批次写回同一个起点，回复头部被覆盖 */
pstPort->pu16Data[pstPort->u16RxLen + i] = au16Tmp[i];
```

修复前实测：第一条 0x03 请求已排队，3 ms 后第二条请求的前 4 字节到达，一轮处理完发出的 9 字节是
`01 06 00 01 00 00 00 FA 33` —— 头部被覆盖、尾部还是上一条的 CRC。

### 1.2 修法

```c
if (/* 帧间隔成立 */) {
    AppMbDispatchFrame(pstPort);

    /* 交出候选之后，这块数据空间里放的是上一条的回复，不再是收帧缓冲。
     * 刚弹出来的批次已经没有第二块地方安放，只能丢弃并计数；等回复全部
     * 进到端口环（服务流程第 2 步）之后才恢复收帧。若这一帧根本没产生
     * 回复（广播、CRC 错、地址不符），缓冲仍归收帧所有，可以继续拼。 */
    if (pstPort->u16TxLen != 0U) {
        pstPort->u32RxDropped = pstPort->u32RxDropped + (Uint32)u16Got;
        return 0U;
    }
}
```

要点：

- 判据是 `u16TxLen != 0`（这一帧真的产生了待发回复），不是"调过 dispatch"。广播、CRC 失败、
  地址不符这些不产生回复的帧仍然可以继续拼接，不会因为拆帧就白丢后面的数据。
- 返回 `0` 而不是 `1`：服务流程第 9 步据此判定"本轮不确认帧"，回复留到下一轮第 2 步推送。
- 丢弃量按**字节**计入 `u32RxDropped`，与 `AppMbDropRx()` 的计数口径一致。
- 丢弃的是"已经开始收、但缓冲被征用"的那条请求；Modbus 主站超时重发即可恢复，这条路径不需要
  额外的帧队列，也不占 RAM（生产态 `RAMGS0` 余量保持 170 字）。

## 2. 三处测试修正（测试错误，不是生产错误）

| 位置 | 现状 | 应为 | 原因 |
| --- | --- | --- | --- |
| `modbus_slave_test.c:629`（用例 9，写 1080） | `[1] == 0x83` | `[1] == 0x90` | 请求功能码是 0x10，异常帧功能码字节 = `0x10 or 0x80 = 0x90`；`0x83` 是 0x03 请求的值 |
| `modbus_slave_test.c:716`（用例 13，截短 0x10） | `[1] == 0x83` | `[1] == 0x90` | 同上 |
| `modbus_slave_test.c:731`（用例 14，越限写） | 地址 `0x0BB7` | 地址 `0x0BBF` | `0x0BB7` = 2999、`0x0BBF` = 3007；地址写错导致后面"两项都没写""限内写生效"的断言连带失败 |

用例 6 的 `0x83` 是正确的（那一帧请求功能码就是 0x03），保持不动。

## 3. 新增回归 19：缓冲所有权

场景与断言：

1. 推送一条完整的 0x03 读 2 个寄存器请求；
2. `Advance(3000)` 超过 A 口 2006 µs 门限，再推同一条请求的前 4 字节（构成帧间隔）；
3. 一轮 `ProtocolTask()` 把两条批次都处理掉；
4. 断言 `AppModbusRxDropped(0) == 之前 + 4`（新批次被丢弃并计数）；
5. 再跑一轮让回复送出，断言 9 字节、地址 `0x01`、功能码 `0x03`、字节数 `4`、CRC 与正文自洽。

原握手用例顺延为 20，断言不变。

## 4. 两态构建台账（r3）

| 项 | 生产 `0U` | 测量 `1U` |
| --- | --- | --- |
| 编译单元 / 警告 / 错误 | 44 / 0 / 0 | 44 / 0 / 0 |
| `.text` | `0x55CD`（21,965） | `0x594C`（22,860） |
| `RAMGS0` 余 | `0xAA`（170） | `0x2`（2） |
| `RAMGS1` 余 | `0x534`（1,332） | `0x4A8`（1,192） |
| `FLASH_APP` 用 / 余 | `0x6BC3` / `0x1343B` | `0x6F4C` / `0x130B2` |
| `.out` | 860,976 B sha256 `9356690edb7c0dbc1ed43e4a5052e337b8d8140cee01dc12b246d5d465c4e70b` | 879,496 B sha256 `8d248b0069a43a136e27293a6e63cf210bec61bba01188d3e1b28cc34108f669` |

与 r2（`3624d2c`）生产态对照：`.text` 21,946 → 21,965（+19）、`FLASH_APP` 27,567 → 27,587（+20），
`RAMGS0` 余量 170 → 170、`RAMGS1` 余量 1,332 → 1,332 不变（本步不新增任何静态存储）。

## 5. 复核方请跑

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

期望输出末尾：`modbus_slave_test: 116 项检查，0 项失败`（r2 是 110 项，本步新增 6 处断言）。
建议同样用 `-fsanitize=address,undefined` 复跑一遍。

## 6. 本轮未纳入（按裁定）

1. 245 字节长回复：不作为阻塞项；等需要支持 120 项连续点表时再加主机专用表与线上连续发送验证。
2. 测量态 `RAMGS0` 仅余 2 字：生产态联调可继续；下一次启用测量态之前迁移测量变量。
3. 六字节握手目前只证明"握手与交接门禁有效"，XMODEM 实际接收属于下一步。

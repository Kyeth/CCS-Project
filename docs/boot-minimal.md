# EP15K_BOOT 最小 Bootloader（参考工程移植）

| 项 | 内容 |
| --- | --- |
| 工程 | `EP15K_BOOT`（分支 `boot/minimal`，基线 `bf32d42`） |
| 参考工程 | `IB800U_BootLoader`，commit `68701bd` |
| 芯片 | TMS320F28374S（CGT 22.6.2.LTS，EABI，FPU32） |
| 入口 | `0x080000`（`BEGIN` 2 字 → `_c_int00`），程序段 `FLASH_BOOT = 0x080002..0x085FFF` |
| 状态 | 构建通过（CCS Debug 与命令行两条路线），**未上板** |

---

## 0. 结论先行

- 本工程是参考 BootLoader 的**移植**，不是重写：XMODEM 状态机、升级配置结构、
  备份区回拷流程、`CheckUpgrade()` 的判定顺序都保持参考实现的形状与接口。
- 必要改动只有五类：芯片适配、端口适配、时基适配、工程接线、必要缺陷修复（6 条，
  见 §4）。除此之外没有新增框架、没有改协议行为。
- 复用 APP 工程**已复核**的模块：SCI 驱动、`RingBuff`、端口层（`SciPort`/
  `PortTask`/`PortCritical`）、XMODEM、Flash（F021）、Upgrade。
- 控制功能与测试代码**不在**本工程：ADC/PWM/CLA/CAN/Xbar/Modbus/采样/前台应用
  全部移除（端口层因此只保留 3 路 SCI 发送单元，见 §3.2）。

---

## 1. 参考工程与移植映射

| 参考路径（`IB800U_BootLoader`） | 本工程路径 | 行数 | 改动 |
| --- | --- | --- | --- |
| `Platform/Xmodem/Inc/XmodemCore.h` | `Platform/Xmodem/Inc/XmodemCore.h` | 152 | 设备头 + 常量（§3.1、§4.4） |
| `Platform/Xmodem/Src/XmodemCore.c` | `Platform/Xmodem/Src/XmodemCore.c` | 524 | 首包裁剪、写失败、重试收尾（§4.4、§4.5） |
| `Platform/Xmodem/Inc/XmodemTimeOut.h` | 同名 | 45 | 无 |
| `Platform/Xmodem/Src/XmodemTimeOut.c` | 同名 | 58 | 时基换成 `Hw_TimeBase`，差值方向（§3.3、§4.7） |
| `Platform/Xmodem/Inc/XmodemTramsports.h` | 同名 | 28 | `const` 参数、`XmodemRxReset()` |
| `Platform/Xmodem/Src/XmodemTramsport.c` | 同名 | 32→约 190 | 端口换成 `SciPort` 整包累计（§3.2、§4.5） |
| `Platform/Xmodem/Inc/XmodemDataHandle.h` | 同名 | 32 | 无 |
| `Platform/Xmodem/Src/XmodemDataHandle.c` | 同名 | 51 | 备份区越界与写失败上报（§4.4） |
| `Platform/Flash/Inc/Dsp280039Flash.h` | `Platform/Flash/Inc/Dsp2837xSFlash.h` | 106→约 130 | 扇区表、F021 基址、RAM 执行（§3.1、§4.1、§4.2） |
| `Platform/Flash/Src/Dsp280039Flash.c` | `Platform/Flash/Src/Dsp2837xSFlash.c` | 153→约 260 | 同上 + 空白检查长度、8 字整块编程 |
| — | `Platform/Flash/Src/Fapi_UserDefinedFunctions.c` | 约 40 | 新增：`Fapi_serviceWatchdogTimer()` 与 `__error__()` 落 RAM |
| `Platform/Upgrade/Inc/UpgradeCore.h` | 同名 | 76 | 无 |
| `Platform/Upgrade/Src/UpgradeCore.c` | 同名 | 163 | 按字数计块（§4.3）、`CopyBootFirmware` 落 RAM（§4.1） |
| `Platform/Upgrade/Inc/DspUpgradeCrc.h` | 同名 | 20 | 无 |
| `Platform/Upgrade/Src/DspUpgradeCrc.c` | 同名 | 46 | 无（CRC32 算法与 APP 完全一致） |
| `User/XmodemSlaveProcess/Inc/XmodemSlaveProcess.h` | 同名 | 24 | 无 |
| `User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c` | 同名 | 42 | 无（CRC=1、重试 8、超时 1200 保持参考取值） |
| `User/Upgrade/Inc/UpgradeUser.h` | 同名 | 32 | 分区地址常量、返回值签名 |
| `User/Upgrade/Src/UpgradeUser.c` | 同名 | 211→约 320 | 分区地址、擦除表、APP 有效性、自升级尾部（§3.4、§4.1、§4.6） |
| `main.c` | `main.c` | 123→约 150 | 全新接线（§3.4） |
| — | `Broad/{inc,src}/DrvCfg_EP15K.*` | 重写 | 只保留时钟/GPIO/SCI/CPU Timer1 四张表 |

参考工程里 `User/Flash280039C/*` 是整份注释掉的遗留实现、`libraries/FlashAPI/*` 是
F28003x 版 F021，两者都未移植；本工程用 `libraries/flash_api/f2837xs/`（随工程携带）。

---

## 2. 分区与文件格式（与 APP 一致）

| 区域 | 扇区 | 地址范围 | 大小 | 本工程 |
| --- | --- | --- | --- | --- |
| BOOT | A+B+C | `0x080000~0x085FFF` | 24 KB | 链接目标 |
| APP | D+E+F+G | `0x086000~0x09FFFF` | 104 KB | 跳转/回拷目标 |
| BACKUP | H+I+J+K | `0x0A0000~0x0B9FFF` | 104 KB | 收包落盘区 |
| SPARE | L+M | `0x0BA000~0x0BDFFF` | 16 KB | 未使用 |
| CFG | N | `0x0BE000~0x0BFFEF` | 8176 B | 升级配置 |

32 字节文件头（`ST_UPGRADE_FILE_HEADER`）：`[0]` ChipType（1）、`[1]` FirmwareType
（1=APP，2=BOOT）、`[2..5]` 正文长度（小端字节）、`[6..9]` 正文 CRC32（小端，
poly `0x04C11DB7`、初值 `0xFFFFFFFF`、不反射、无最终异或）、`[10..31]` 保留。
备份区里两个连续字节压成一个 16 位 Flash 字（先到者低字节），因此 104 KB 备份区
可容纳 `0x34000` 字节正文。

---

## 3. 五类必要改动

### 3.1 芯片适配

- 设备头 `F28003x_device.h` → `F2837xS_device.h`（Xmodem/Flash/Upgrade/Slave 全量）。
- F021：`#define _F2837xS` 后 include，`Fapi_initializeAPI(F021_CPU0_W0_BASE_ADDRESS, 200)`，
  只激活 `Fapi_FlashBank0`（F28374S 单块，参考工程的 Bank0/1/2 选择是 F280039C 残留）。
- 扇区表按真实 A~N：A~D、K~N 为 8K 字节（`0x2000` 字），E~J 为 32K 字节（`0x8000` 字）。
- 链接脚本 `CMD/2837xS_FLASH_BOOT_CLA_lnk.cmd`：codestart `0x080000`、程序段
  `FLASH_BOOT`，APP/BACKUP/SPARE/CFG 只声明不分配；`.TI.ramfunc`→`RAMD0`、
  `.flashfunc`→`RAMGS3_RUN`、`.upybuf`→`RAMGS2`；去掉 CLA 段。
- RAM 执行搬运：`main()` 第一行 `CopyRamSections()` 把 `.TI.ramfunc` 与
  `.flashfunc` 从 Flash 拷到运行地址（基线工程此前完全没有搬运代码）。

### 3.2 端口适配

`XmodemRead()/XmodemWrite()` 内部换成 `SciPort`：

- `XmodemWrite` → `SciPortWriteTx(HW_SCI_ID_A, …)`；返回 0 表示端口环满，
  协议层必须把控制字节留到下一轮再发（不能当已发送）。
- `XmodemRead` 在适配层**累计整包**：先倒出预置字节，再按包头（SOH 133 /
  STX 1029 / CAN 2 / 其它 1）用 `SciPortPopBatch`（每次 ≤8 字节）拼接，收满才返回；
  跨包的余字节保存到 64 字预置缓冲，收满本包即停止取批；半包静默 100 ms 丢弃并
  重新同步。
- `XmodemRxPreload()` 把 Modbus 握手同批多取的字节还给读路径，`XmodemRxReset()`
  在会话开始/取消时清掉适配层残留。
- 端口层：`PortTask` 的前台单元由 5 个（3 SCI + 2 CAN）降为 3 个 SCI——BOOT 不含
  CAN，因此 `CanPort`/`Hw_CanDriver`/`CanQueue` 都不进本工程。这是端口层唯一的
  结构差异，其余逻辑与 APP 逐字一致。

### 3.3 时基适配

`Hw_TimeBase` 是向下计数自由计数器：`TimeStart()` 记 `Hw_TimeBaseTicks()`，
`IsTimeOut()` 判 `(Uint32)(u32StartTick - u32NowTick) > u32TimeOut`（毫秒单位不变）。
时间源未就绪时既不启动计时也不判超时。

### 3.4 工程接线

`main()`：`CopyRamSections()` → 时钟/时基/中断向量/GPIO/SCI/CPU Timer1（100 µs 心跳）
→ `SciPortInit`/`PortTaskInit`/`XmodemProcessInit`/`Dsp2837xSFlashInit`/`UpgradeUserInit`
→ `CheckUpgrade()` → `StartSystem()` → 主循环 `喂狗 + PortTaskService + TaskUpgrade`。

`CheckUpgrade()` 保留参考的三分支：未收完→按配置重设波特率并擦备份区（参数不合法
则清配置后跳 APP）；已收完→校验备份区后按 `bFirmwareType` 回拷 APP 或 BOOT；
无配置→跳 APP。区别见 §4.6：跳 APP 之前先判 APP 是否有效。

MODBUS 侧接线（APP 工程）：08 字节握手应答 → 擦备份区并写"接收中"配置 → 单发 `'C'`
→ 收包落备份区 → 校验 → 写配置 → 复位；BOOT 侧用同一套 `Platform/Xmodem`、
`Platform/Upgrade`，只换链接脚本与分区常量。

### 3.5 必要缺陷修复（6 条）

| # | 参考位置 | 触发条件 | 最小改法 |
| --- | --- | --- | --- |
| 1 | `UpgradeUser.c` `CheckUpgrade()` 自升级分支（`bFirmwareType==2`） | BOOT 升级自己：`EraseBootBank()`/`CopyBootFirmware()` 链接在 Flash，擦完 A/B/C 后返回取指落在已擦除 Flash | 两者与新的 `BootSelfUpdateTail()` 放 `.flashfunc`（RAM），尾部按"清配置 → 擦 BOOT → 回拷 → 复位"一次做完，不返回 Flash |
| 2 | `UpgradeUser.c` 三处 `jump_to_app()` | APP 空白或半成品（首字 `0xFFFF`）时直接跳 `0x086000` | 新增 `IsAppValid()`（首字非 `0xFFFF`、入口向量落在 `0x086002..0x09FFFF`），无效则写"接收中"配置留 BOOT 收包 |
| 3 | `Dsp280039Flash.c` `EraseSingleSector()` | 32K 扇区只按 `0x1000` 做空白检查；长度单位应为 32 位字 | `Fapi_doBlankCheck(addr, FlashSectorWords(addr) / 2UL, …)`，扇区字数按真实 A~N |
| 4 | `Dsp280039Flash.c` `WriteFlashData()` | 参考按 4 字一步编排，长块在尾部要补齐才对齐 64 位编程单元 | 以 `FLASH_UNIT_WORDS = 8` 整块编程、尾部补 `0xFFFF`；等待 FSM 与喂狗都在 RAM |
| 5 | `XmodemCore.c` `XmodemSinglePacket()` 首包分支 | 文件正文不足一包（SOH 96 / STX 992 字节）时直接 `return XMODEM_END`，正文一个字节也不写 | 首包解析 32 字节头后按 `u32FileSize` 裁剪再 `WriteBlock`，写成功后才累加长度 |
| 6 | `XmodemDataHandle.c` `WriteBlock()` | 参考忽略 `WriteFlashData()` 返回值，编程失败仍回 ACK | 返回 -1 由协议层转 `XMODEM_RETRY_ERROR`；入口再校验偶数长度与备份区边界 |

另有两处**继承自 APP 工程已复核模块**、不属于本轮新增的修正：
`XmodemTimeOut.c` 差值方向（§3.3）与 `XmodemTramsport.c` 预存越界（§3.2）。
参考实现里 `WriteFlashData`/`CopyAppFirmware`/`CopyBootFirmware`/`XmodemTransmit`
缺 `return` 的问题在本工程一并补齐。

---

## 4. 构建

### 4.1 CCS（固定路线）

CCS 11.1 + `ti-cgt-c2000_22.6.2.LTS`，工程名 `EP15K_BOOT`：

```
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data <workspace> \
  -application com.ti.ccstudio.apps.projectImport -ccs.location D:/MinMain/CCS-Project-boot
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data <workspace> \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild -cleanBuild "EP15K_BOOT/Debug"
```

post-build（Debug/Release 都配了）：

```
hex2bin/CalcHexAddr.exe  ${BuildArtifactFileBaseName}.hex 0x80000 0x86000 hex2bin/hex2bin.exe
hex2bin/CalcUpgradeBinCrc.exe 1 2 ${BuildArtifactFileBaseName}.bin
```

### 4.2 命令行（等价路线，不依赖 CCS 状态）

```
python tools/build_minimal.py --cgt C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS
```

链接后自动跑 `hex2000` 与 `tools/make_upgrade_bin.py --target boot`（本工程内该脚本
允许 BOOT 目标；APP 工程里的同名脚本仍保留"禁止 boot"守卫）。

---

## 5. 构建结果

两条路线用同一份源码与同一套分区；产物分别登记，**不要求跨路线哈希相同**。

| 产物 | CCS 11.1 Debug | 命令行 |
| --- | --- | --- |
| `.out` | 665,016 B `c7d56952…` | 668,396 B `b27253e3…` |
| `.map` | 84,191 B | 81,690 B |
| `.hex` | 75,051 B `b801b338…` | 75,051 B `2922510f…` |
| `.bin`（烧录） | 31,208 B `aa1685ee…` | 31,208 B `31bc8a0e…` |
| `_UpgradeCRC32.bin` | 31,240 B `e588dea2…`，CRC32 `0x61799FF2` | 31,240 B `54255a5e…`，CRC32 `0x183306FC` |

CCS Debug 的一套产物同时登记在 `artifacts/boot-v1/`（`.gitignore` 对这些文件开了例外，其余构建输出仍然忽略），可直接取用烧录文件与升级 BIN。

`.map` 的 sha256 每次构建都会变（与 APP 工程结论一致），因此只登记大小；`.out`、`.hex`、`.bin`、升级 BIN 在各自路线内可复现（命令行路线连续两次构建，四个产物的 sha256 完全一致）。

两份 `.bin` 大小相同、只有 **5 个字节**不同，全部是内嵌 `__FILE__` 字符串里的 `\` 与
`/`（driverlib `ASSERT` 宏），因此 CRC32 不同。属**实施方核对记录**，未做独立复现，
不作为验收依据。

内存（两份 map 完全一致）：

| 区 | 用 | 余 |
| --- | --- | --- |
| `FLASH_BOOT`（0x080002，0x5FFE 字） | `0x3CDA` | `0x2324` |
| `RAMD0`（`.TI.ramfunc`） | `0x00C3` | `0x073D` |
| `RAMGS0`（`.bss`） | `0x0974` | `0x068C` |
| `RAMGS1` | `0x000C` | `0x0BF4` |
| `RAMGS2`（`.upybuf`） | `0x0840` | `0x07C0` |
| `RAMGS3`（`.flashfunc` 运行区） | 0 | `0x1000` |

关键符号落位：`codestart` @ `0x080000`、`_c_int00` @ `0x082E4E`、
`.flashfunc` LOAD `0x082FA0` RUN `0x00F000`（`EraseBootBank` @ `0x00F986`、
`CopyBootFirmware` @ `0x00F92A`、`FlashFeedWatchDog` @ `0x00F79F`、
`FlashForceReset` @ `0x00F7A9`）。范围利用率 63.49%（31,208 / 49,152 字节）。

编译告警 4 条，全在参考 `XmodemCore.c`（未引用局部变量 `u32Sum`/`u16DataLength`、
两处 `#190-D` 枚举混用），按忠实移植保留。

构建日志：`logs/ccs_boot_build.log`（CCS 11.1 一次完整 Clean Build，32 个编译单元）、
`logs/build_minimal.log`（命令行全量）。

---

## 6. 上板必须确认的 6 条

1. 上电后 `CheckUpgrade()` 在 APP 有效时直接跳 `0x086000`，LED/串口无输出属正常。
2. APP 空白（或擦掉）时 BOOT 写"接收中"配置并留 BOOT：发 `'C'`，能收包。
3. 擦除 H/I/J/K 期间看门狗不误复位（`FlashFeedWatchDog` 走 RAM）。
4. 收包成功 → 校验通过 → 写配置 `0xAAEE` → 复位 → 再次上电回拷 APP 并跳转。
5. 中止（两次 CAN / 重试超限）后配置被清，APP 仍在正常跳转，BOOT 不反复收包。
6. BOOT 自升级（FirmwareType=2）能完成"清配置 → 擦 A/B/C → 回拷 → 复位"，且复位后
   BOOT 仍是新版本（`BootSelfUpdateTail` 全程在 RAM 执行）。

---

## 7. 本步未覆盖

- 没有主机测试：参考模块与 SCI/F021/看门狗/复位强耦合，验收以上板升级成功为准。
- 只构建了 Debug；Release 配置的 include/postbuild 已同步，但未产出产物。
- BOOT→APP 跳转只校验入口向量落在 APP 区，不校验 APP 自身 CRC（APP 完整性由升级
  流程的备份区校验保证）。
- `Platform/` 下的 `PortBench.h`/`PortBudget.h` 随端口层带入，`PORT_BENCH_TEST`
  保持 `0U`（生产配置），未提供测量态构建。

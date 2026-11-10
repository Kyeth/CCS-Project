# 构建与升级 BIN 生成

这份工程有两条构建路线，采用**相同的源码、相同的 CGT 22.6.2.LTS 与相同的分区设计**；
两条路线的实际产物分别登记（见 §3.1），**不要求跨路线哈希相同**：

| 方式 | 用途 | 入口 |
| --- | --- | --- |
| CCS 工程（Debug / Release） | 日常开发、下载调试 | `.cproject` 里的两个配置，构建后自动执行 `hex2bin/` 后处理 |
| 命令行脚本 | 无 CCS 环境、批量核对、两态台账 | `python tools/build_minimal.py --cgt <CGT 路径>` |

两条路线都用 TI C2000 CGT **22.6.2.LTS**，同一套链接命令文件
（`CMD/2837xS_FLASH_APP_CLA_lnk.cmd` + `CMD/F2837xS_Headers_nonBIOS.cmd`）与同一组预定义宏。
产物各自登记，跨路线的哈希比对只能作为实施方的核对记录，不作为验收依据。

## 1. 升级 BIN 是怎么来的

CCS 和命令行脚本都做同样三步，工具全部随工程携带，**不依赖本机绝对路径**：

```
<工程名>.out
   │  hex2000（CGT 自带）：-memwidth 16 -romwidth 16，Intel HEX
   ▼
<工程名>.hex
   │  hex2bin/CalcHexAddr.exe <hex> <range-min> <range-max> hex2bin/hex2bin.exe
   │     · 只保留目标分区内的数据，算出实际起止地址
   │     · 内部调用 hex2bin.exe -w -v -s <start> -l <bytes> -a -p 0xFFFF 填充未编程区域
   ▼
<工程名>.bin                      升级正文（不含文件头）
   │  hex2bin/CalcUpgradeBinCrc.exe <ChipType> <FirmwareType> <bin>
   │     · 前置 32 字节文件头：ChipType / FirmwareType / 文件大小 / 正文 CRC32
   ▼
<工程名>_UpgradeCRC32.bin         送主机的升级文件
```

### 参数

| 目标 | 分区范围（16 位字地址） | `FirmwareType` | 说明 |
| --- | --- | --- | --- |
| APP | `0x086000` – `0x0A0000` | `1` | 本工程（`EP15K_2026`）的默认目标 |
| BOOT | `0x080000` – `0x086000` | `2` | **待 BOOT 工程完成后提供**：必须编译、链接独立的 BOOT 工程（入口 `0x080000`），不能只改打包地址与 `FirmwareType` |

`ChipType` 固定为 **`1`**，必须与接收端 `Platform/Xmodem/Inc/XmodemCore.h` 里的
`XMODEM_CHIP_TYPE` 一致（改这里就同时改那里）。

> BOOT 那一行目前只是**约定**：最小 Bootloader 工程还没有建立。在那之前请勿使用 BOOT 参数，
> 否则会把 APP 镜像误包装成 BOOT 镜像。`tools/make_upgrade_bin.py --target boot` 目前也因此
> 只做参数占位提示。

### 文件头格式（32 字节，与 `ST_UPGRADE_FILE_HEADER` 一致）

| 偏移 | 长度 | 内容 |
| --- | --- | --- |
| 0 | 1 | ChipType（`1`） |
| 1 | 1 | FirmwareType（`1` = APP，`2` = BOOT） |
| 2 | 4 | 正文字节数（小端） |
| 6 | 4 | 正文 CRC32（小端，poly `0x04C11DB7`、初值 `0xFFFFFFFF`、不反射、无最终异或） |
| 10 | 22 | 保留（0） |

## 2. CCS 里怎么配（已在本工程配好）

`Debug` 与 `Release` 两个配置的 **Post-build steps** 都是同一条命令，
路径全部相对工程（`${PROJECT_LOC}`）：

```
"${PROJECT_LOC}/hex2bin/CalcHexAddr.exe" "${BuildArtifactFileBaseName}.hex" "0x86000" "0xA0000" "${PROJECT_LOC}/hex2bin/hex2bin.exe";
"${PROJECT_LOC}/hex2bin/CalcUpgradeBinCrc.exe" "1" "1" "${BuildArtifactFileBaseName}.bin"
```

BOOT 工程建立后，把两处改成 `"0x80000" "0x86000"` 和 `"1" "2"`；在独立 BOOT 工程存在之前不要改。

构建后产物在工程的 `Debug/` 或 `Release/` 目录：`EP15K_2026.out`、`.map`、
`.hex`、`.bin`、`EP15K_2026_UpgradeCRC32.bin`。

## 3. 命令行怎么跑

```bash
python tools/build_minimal.py --cgt C:/path/to/ti-cgt-c2000_22.6.2.LTS
```

BOOT 目标的命令行示例**待 BOOT 工程完成后提供**（`--target boot` 目前只是参数占位）。

脚本最后打印 4 行台账（`.out` / `.map` / `.bin` / `_UpgradeCRC32.bin` 的大小与 sha256），
中间产物在 `--out` 指定目录（默认 `../CCS-Project-minimal-build`）。

也可以单独对已有 hex 跑后处理：

```bash
python tools/make_upgrade_bin.py --hex <path>/EP15K_2026.hex --target app
```

## 3.1 已实测的构建结果

实测登记（工程 `.cproject` 使用 `C2000_22.6` 构建定义，CGT 22.6.2.LTS）：

- **CCS 10.3.1**（`D:/ti/ccs1031`）：`projectImport` 成功（`Done!`，工程名 `EP15K_2026`）；
  但本次指定的 headless 入口 `com.ti.ccstudio.apps.projectBuild -clean` **未生成任何产物**
  （输出只有 `0 out of 1 projects have errors`），**原因未确认**，不据此断言该版本存在缺陷。
  该版本不再继续排查。
- **CCS 11.1**（`D:/ti/CCS1110`）：CDT headless 路线（`-cleanBuild "EP15K_2026/Debug"`）
  已报告构建成功，产物见下表；本路线是当前固定使用的构建路线。

```bash
# 导入（一次即可）
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data D:/MinMain/_tmp/ws_ccs \
  -application com.ti.ccstudio.apps.projectImport -ccs.location D:/MinMain/CCS-Project-minimal
# Debug 全量重建
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data D:/MinMain/_tmp/ws_ccs \
  -application org.eclipse.cdt.managedbuilder.core.headlessbuild -cleanBuild "EP15K_2026/Debug"
# Release 全量重建
"D:/ti/CCS1110/ccs/eclipse/eclipsec.exe" -noSplash -data D:/MinMain/_tmp/ws_ccs   -application org.eclipse.cdt.managedbuilder.core.headlessbuild -cleanBuild "EP15K_2026/Release"
```

| 配置 | `.out` | `.map` | `.hex` | `.bin` | `_UpgradeCRC32.bin` | 正文 CRC32 |
| --- | --- | --- | --- | --- | --- | --- |
| Debug | 997,284 B | 127,370 B | 162,791 B | 67,680 B | 67,712 B | `0x1997B016` |
| Release | — | — | — | 55,744 B | 55,776 B | `0xC63E9194` |

Debug 的 `.text` = `0x636B`、`FLASH_APP` 用 `0x8410`（余 `0x11BEE`）、`RAMGS0` 用满、
`RAMGS1` 用 `0xACC`，与命令行 `build_minimal.py` 的 r5 台账逐项一致；`map` 里
`Test`/`stub_test`/`shim` 命中数为 0。Release 走 CCS 的 Release 默认优化，`.text` = `0x4D14`，
比 Debug 小，属预期。

**两条构建路线的差异（实施方核对记录，未做独立复现）**：本次在 Debug `.out` 与命令行产物
之间逐字节比较 HEX 映像，观察到 2 处差异位于 `__FILE__` 字符串内的路径分隔符
（`0x8D7ED`、`0x8D803`，`/` 与 `\`），据此**推断**代码与数据一致、`.bin` 大小相同而 CRC32 不同
（`0x1997B016` vs `0x92CBED1B`）。该结论未经第三方复核，不作为验收依据；跨路线要求逐字节一致时，
需两条路线使用相同的源文件路径写法后再比较。

已核对：`BEGIN` = `0x086000`（2 字，入口跳转）→ `FLASH_APP` = `0x086002` 起；`FLASH_BOOT_RSVD`
`0x080000`、`FLASH_BACKUP_RSVD` `0x0A0000`、`FLASH_SPARE_RSVD` `0x0BA000`、`FLASH_CFG_RSVD`
`0x0BE000` 均未被占用；`.TI.ramfunc` LOAD `0x08C3E8` → RUN `0x00B000`；`.flashfunc` LOAD
`0x08D970` → RUN `0x00F000`，成员含 F021 库的 `FlashStateMachine`/`Program`/`Init` 等，
库取自 `libraries/flash_api/f2837xs/lib/F021_API_F2837xS_FPU32_EABI.lib`。

## 4. 依赖与来源

- `hex2bin/`：三个 Windows exe + 使用指南，来自既有产线工具（`docs/Hex2bin/` 里是它们的源码）。
- `libraries/flash_api/f2837xs/`：TI F021 Flash API（头文件 + `F021_API_F2837xS_FPU32_EABI.lib`），
  取自 C2000Ware 5.00 的 `libraries/flash_api/f2837xs`，随工程携带，避免依赖安装路径。
- CGT 22.6.2.LTS 由 CCS 安装或单独安装提供，`tools/build_minimal.py --cgt` 指向其根目录。

## 5. 与测试代码的边界

主机测试、替身头文件、板测打点都**不在本工程内**，见 `../CCS-Project-tests/README.md`。
正式工程的 `Debug` / `Release` 不编译任何测试文件，包含路径里也没有 `Test/` 或 `shim/`。

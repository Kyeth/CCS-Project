# 构建与升级 BIN 生成

这份工程有两种等价的构建方式，产出的 `.out` / `.hex` / `.bin` 完全一致：

| 方式 | 用途 | 入口 |
| --- | --- | --- |
| CCS 工程（Debug / Release） | 日常开发、下载调试 | `.cproject` 里的两个配置，构建后自动执行 `hex2bin/` 后处理 |
| 命令行脚本 | 无 CCS 环境、批量核对、两态台账 | `python tools/build_minimal.py --cgt <CGT 路径>` |

两种方式都用 TI C2000 CGT **22.6.2.LTS**，同一套链接命令文件
（`CMD/2837xS_FLASH_APP_CLA_lnk.cmd` + `CMD/F2837xS_Headers_nonBIOS.cmd`），
所以 `.out` 的 sha256 可以互相校验。

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
| BOOT | `0x080000` – `0x086000` | `2` | 最小 Bootloader 工程，同一套命令 |

`ChipType` 固定为 **`1`**，必须与接收端 `Platform/Xmodem/Inc/XmodemCore.h` 里的
`XMODEM_CHIP_TYPE` 一致（改这里就同时改那里）。

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

BOOT 工程把两处改成 `"0x80000" "0x86000"` 和 `"1" "2"` 即可，其余不变。

构建后产物在工程的 `Debug/` 或 `Release/` 目录：`EP15K_2026.out`、`.map`、
`.hex`、`.bin`、`EP15K_2026_UpgradeCRC32.bin`。

## 3. 命令行怎么跑

```bash
python tools/build_minimal.py --cgt C:/path/to/ti-cgt-c2000_22.6.2.LTS
# BOOT 目标：
python tools/build_minimal.py --cgt C:/path/to/ti-cgt-c2000_22.6.2.LTS --target boot
```

脚本最后打印 4 行台账（`.out` / `.map` / `.bin` / `_UpgradeCRC32.bin` 的大小与 sha256），
中间产物在 `--out` 指定目录（默认 `../CCS-Project-minimal-build`）。

也可以单独对已有 hex 跑后处理：

```bash
python tools/make_upgrade_bin.py --hex <path>/EP15K_2026.hex --target app
```

## 4. 依赖与来源

- `hex2bin/`：三个 Windows exe + 使用指南，来自既有产线工具（`docs/Hex2bin/` 里是它们的源码）。
- `libraries/flash_api/f2837xs/`：TI F021 Flash API（头文件 + `F021_API_F2837xS_FPU32_EABI.lib`），
  取自 C2000Ware 5.00 的 `libraries/flash_api/f2837xs`，随工程携带，避免依赖安装路径。
- CGT 22.6.2.LTS 由 CCS 安装或单独安装提供，`tools/build_minimal.py --cgt` 指向其根目录。

## 5. 与测试代码的边界

主机测试、替身头文件、板测打点都**不在本工程内**，见 `../CCS-Project-tests/README.md`。
正式工程的 `Debug` / `Release` 不编译任何测试文件，包含路径里也没有 `Test/` 或 `shim/`。

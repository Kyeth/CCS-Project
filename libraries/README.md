# libraries/

随工程携带第三方库，避免构建依赖本机绝对安装路径。

## flash_api/f2837xs — TI F021 Flash API

- 来源：`C2000Ware_5_00_00_00/libraries/flash_api/f2837xs`
- 内容：`include/`（`F021.h`、`Init.h`、`Registers.h`、`Types.h`、`Constants/`）与
  `lib/F021_API_F2837xS_FPU32_EABI.lib`（只保留本工程用的 EABI + FPU32 变体）
- 用途：05F 升级功能擦写备份区与配置区
  （`Platform/Flash/Src/Dsp2837xSFlash.c`、`Platform/Flash/Src/Fapi_UserDefinedFunctions.c`）
- 链接方式：`CMD/2837xS_FLASH_APP_CLA_lnk.cmd` 里 `-l F021_API_F2837xS_FPU32_EABI.lib`，
  搜索路径由 `.cproject` 的 Linker → `--search_path` 指向本目录
  （`tools/build_minimal.py` 用 `-i` 传同一个目录）
- 版权：TI 提供的 Flash API 库，按 TI 的许可用于 C2000 器件；

不修改库与头文件。升级 Flash API 版本时同时替换 `include/` 与 `lib/`，并重新跑两态构建台账。

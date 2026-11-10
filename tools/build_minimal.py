"""Build the EP15K_BOOT image with TI CGT 22.6.2 without CCS state.

与 APP 工程的 tools/build_minimal.py 同一条链路（cl2000 → lnk2000 → hex2000 →
hex2bin → CalcUpgradeBinCrc），只有三处不同：

1. 源列表 = BOOT 的最小集合（无 ADC/PWM/CLA/CAN/Modbus/采样/前台应用）；
2. 链接命令文件是 CMD/2837xS_FLASH_BOOT_CLA_lnk.cmd，入口 0x080000；
3. 升级包按 FirmwareType = 2（BOOT）打包，窗口 0x080000..0x086000。
"""
from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "main.c",
    "Broad/src/DrvCfg_EP15K.c",
    "Drivers/src/Hw_ClockDriver.c",
    "Drivers/src/Hw_CpuTimerDriver.c",
    "Drivers/src/Hw_GpioDriver.c",
    "Drivers/src/Hw_SciDriver.c",
    "Drivers/src/Hw_TimeBase.c",
    "Drivers/src/Hw_WatchDog.c",
    "Platform/Common/Src/RingBuff.c",
    "Platform/Port/Src/SciPort.c",
    "Platform/Port/Src/PortTask.c",
    "Platform/Port/Src/PortCritical.c",
    "Platform/Xmodem/Src/XmodemCore.c",
    "Platform/Xmodem/Src/XmodemDataHandle.c",
    "Platform/Xmodem/Src/XmodemTimeOut.c",
    "Platform/Xmodem/Src/XmodemTramsport.c",
    "Platform/Flash/Src/Dsp2837xSFlash.c",
    "Platform/Flash/Src/Fapi_UserDefinedFunctions.c",
    "Platform/Upgrade/Src/UpgradeCore.c",
    "Platform/Upgrade/Src/DspUpgradeCrc.c",
    "User/XmodemSlaveProcess/Src/XmodemSlaveProcess.c",
    "User/Upgrade/Src/UpgradeUser.c",
    "device/common/source/F2837xS_DefaultISR.c",
    "device/common/source/F2837xS_Gpio.c",
    "device/common/source/F2837xS_PieCtrl.c",
    "device/common/source/F2837xS_PieVect.c",
    "device/common/source/F2837xS_SysCtrl.c",
    "device/common/source/F2837xS_CodeStartBranch.asm",
    "device/common/source/F2837xS_usDelay.asm",
    "device/headers/source/F2837xS_GlobalVariableDefs.c",
]
INCLUDES = [
    ".", "Broad/inc", "Drivers/inc",
    "Platform/Common/Inc", "Platform/Port/Inc",
    "Platform/Xmodem/Inc", "Platform/Flash/Inc", "Platform/Upgrade/Inc",
    "User/XmodemSlaveProcess/Inc", "User/Upgrade/Inc",
    "device/common/include", "device/headers/include",
    "device/driverlib", "device/driverlib/inc",
]
# F021 Flash API 随工程携带，与 CCS 工程同一份，不依赖 C2000Ware 安装路径
F021_ROOT = ROOT / "libraries" / "flash_api" / "f2837xs"
F021_INCLUDES = [F021_ROOT / "include"]
F021_LIBS = [F021_ROOT / "lib"]

FLAGS = [
    "-v28", "-ml", "-mt", "--cla_support=cla1", "--float_support=fpu32",
    "--tmu_support=tmu0", "--vcu_support=vcu2", "--fp_mode=relaxed",
    "--abi=eabi", "--define=_FLASH", "--define=DEBUG", "--define=CPU1",
    "--define=F2837xS", "-g", "--diag_warning=225", "--diag_wrap=off",
    "--display_error_number",
]

IMAGE = "EP15K_BOOT"


def run(argv, cwd):
    result = subprocess.run(argv, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding="utf-8",
                            errors="replace")
    if result.stdout.strip():
        print(result.stdout.strip(), flush=True)
    if result.returncode:
        raise SystemExit("command failed (%d): %s"
                         % (result.returncode, " ".join(str(a) for a in argv)))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cgt", required=True, type=Path,
                        help="TI C2000 CGT root (contains bin/cl2000.exe)")
    parser.add_argument("--out", type=Path, default=ROOT.parent / "EP15K_BOOT-build")
    args = parser.parse_args()

    cgt = args.cgt.resolve()
    compiler = cgt / "bin" / "cl2000.exe"
    for tool in (compiler,):
        if not tool.is_file():
            raise SystemExit("missing tool: %s" % tool)
    for header in F021_INCLUDES:
        if not (header / "F021.h").is_file():
            raise SystemExit("F021 headers not found: %s" % header)

    out = args.out.resolve()
    objdir = out / "obj"
    objdir.mkdir(parents=True, exist_ok=True)

    objects = []
    for rel in SOURCES:
        source = ROOT / rel
        if not source.is_file():
            raise SystemExit("missing source: %s" % source)
        obj = objdir / (source.stem + ".obj")
        objects.append(str(obj))
        include_args = [f"--include_path={ROOT / d}" for d in INCLUDES]
        include_args.append(f"--include_path={cgt / 'include'}")
        include_args.extend(f"--include_path={p}" for p in F021_INCLUDES)
        run([str(compiler), *FLAGS, *include_args,
             f"--obj_directory={objdir}", str(source)], ROOT)

    image = out / (IMAGE + ".out")
    map_file = out / (IMAGE + ".map")
    # 链接走 cl2000（编译器驱动）：lnk2000.exe 不接受 -z，只有 cl2000 才会把它
    # 转成链接模式；库用 -l 名 + -i 搜索路径，和 CCS 的调用方式一致。
    # FLAGS 必须一起传：链接期的 .cmd 预处理依赖 --abi=eabi（__TI_EABI__），
    # 少了它就会走非 EABI 分支去找 F021_API_F2837xS_FPU32.lib（不存在）。
    link = [
        str(compiler), *FLAGS, "-z", f"-m{map_file}", "--stack_size=0x200",
        "--warn_sections", f"-i{cgt / 'lib'}", f"-i{cgt / 'include'}",
        f"-i{F021_LIBS[0]}", "--reread_libs", "--rom_model",
        "-o", str(image), *objects,
        str(ROOT / "CMD/2837xS_FLASH_BOOT_CLA_lnk.cmd"),
        str(ROOT / "CMD/F2837xS_Headers_nonBIOS.cmd"), "-llibc.a",
    ]
    run(link, ROOT)

    print("", flush=True)
    for artifact in (image, map_file):
        digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
        print("%s: %d bytes sha256=%s" % (artifact, artifact.stat().st_size, digest),
              flush=True)

    # 与 CCS post-build 等价：.out → hex2000 → .hex → hex2bin → .bin → _UpgradeCRC32.bin
    hex2000 = cgt / "bin" / "hex2000.exe"
    hex_file = out / (IMAGE + ".hex")
    print("hex", flush=True)
    run([str(hex2000), "-q", "-i", str(image), "-o", str(hex_file),
         "-memwidth", "16", "-romwidth", "16"], out)
    run([sys.executable, str(ROOT / "tools/make_upgrade_bin.py"),
         "--hex", str(hex_file), "--target", "boot"], out)
    return 0


if __name__ == "__main__":
    sys.exit(main())

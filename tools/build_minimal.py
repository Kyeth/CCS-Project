"""Build the minimal F28374S image with TI CGT 22.6.2 without CCS state."""
from __future__ import annotations

import argparse
import hashlib
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "main.c",
    "Broad/src/DrvCfg_EP15K.c",
    *[str(p.relative_to(ROOT)) for p in sorted((ROOT / "Drivers/src").glob("*.c"))],
    "Platform/Schedule/Src/Schedule.c",
    "Platform/Status/Src/BringupStatus.c",
    "Drivers/src/Hw_ClaTask.cla",
    "device/common/source/F2837xS_Adc.c",
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
    ".", "Broad/inc", "Drivers/inc", "Platform/Schedule/Inc", "Platform/Status/Inc", "device/common/include",
    "device/headers/include", "device/driverlib", "device/driverlib/inc",
]
FLAGS = [
    "-v28", "-ml", "-mt", "--cla_support=cla1", "--float_support=fpu32",
    "--tmu_support=tmu0", "--vcu_support=vcu2", "--fp_mode=relaxed",
    "--abi=eabi", "--define=_FLASH", "--define=DEBUG", "--define=CPU1",
    "--define=F2837xS", "-g", "--diag_warning=225",
    "--diag_wrap=off", "--display_error_number",
]


def run(argv: list[str], cwd: Path) -> None:
    result = subprocess.run(argv, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding="utf-8", errors="replace")
    if result.stdout.strip():
        print(result.stdout, flush=True)
    if result.returncode:
        raise SystemExit(f"command failed ({result.returncode}): {' '.join(argv)}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cgt", type=Path, default=Path(r"E:\CCS\ccs\tools\compiler\ti-cgt-c2000_22.6.2.LTS"))
    parser.add_argument("--out", type=Path, default=ROOT.parent / "CCS-Project-minimal-build")
    args = parser.parse_args()
    cgt = args.cgt.resolve()
    out = args.out.resolve()
    compiler = cgt / "bin" / "cl2000.exe"
    if not compiler.is_file():
        raise SystemExit(f"compiler not found: {compiler}")
    out.mkdir(parents=True, exist_ok=True)
    include_flags = [f"--include_path={ROOT / d}" for d in INCLUDES]
    include_flags.append(f"--include_path={cgt / 'include'}")
    objects = []
    for rel in SOURCES:
        source = ROOT / rel
        if not source.is_file():
            raise SystemExit(f"missing source: {source}")
        objdir = out / "obj" / Path(rel).parent
        objdir.mkdir(parents=True, exist_ok=True)
        obj = objdir / (source.stem + ".obj")
        print(f"compile {rel}", flush=True)
        run([str(compiler), *FLAGS, *include_flags,
             f"--obj_directory={objdir}", str(source)], out)
        if not obj.is_file():
            raise SystemExit(f"missing object: {obj}")
        objects.append(obj)
    image = out / "EP15K_minimal.out"
    map_file = out / "EP15K_minimal.map"
    print("link", flush=True)
    run([str(compiler), *FLAGS, "-z", f"-m{map_file}", "--stack_size=0x200",
         "--warn_sections", f"-i{cgt / 'lib'}", f"-i{cgt / 'include'}",
         "--reread_libs", "--rom_model", "-o", str(image),
         *[str(o) for o in objects],
         str(ROOT / "CMD/2837xS_FLASH_APP_CLA_lnk.cmd"),
         str(ROOT / "CMD/F2837xS_Headers_nonBIOS.cmd"), "-llibc.a"], out)
    for artifact in (image, map_file):
        digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
        print(f"{artifact}: {artifact.stat().st_size} bytes sha256={digest}")


if __name__ == "__main__":
    main()

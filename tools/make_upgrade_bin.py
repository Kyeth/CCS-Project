"""Turn the linker's Intel-HEX output into an upgrade BIN for this project.

Two steps, both done by the toolchain that ships inside the repository
(`hex2bin/`), so CCS and `tools/build_minimal.py` produce byte-identical
results:

1. `hex2bin/CalcHexAddr.exe <hex> <range-min> <range-max> <hex2bin.exe>`
   keeps only the data inside the target's flash window and calls
   `hex2bin.exe -w -v -s <start> -l <bytes> -a -p 0xFFFF <hex>`, which writes
   `<name>.bin` next to the hex file.
2. `hex2bin/CalcUpgradeBinCrc.exe <chip-type> <firmware-type> <name>.bin`
   prepends the 32-byte upgrade header (chip type, firmware type, body size,
   body CRC32) and writes `<name>_UpgradeCRC32.bin`.

The receiver in this project checks the header it gets: chip type must equal
`XMODEM_CHIP_TYPE` (currently 1) and firmware type 1 = APP / 2 = BOOT.

Usage:
    python tools/make_upgrade_bin.py --hex <path/to/foo.hex> [--target app|boot]

Both tools are resolved relative to the project root, so the command works
from any directory and does not depend on an absolute machine path.
"""
from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEX2BIN_DIR = ROOT / "hex2bin"
CALC_HEX_ADDR = HEX2BIN_DIR / "CalcHexAddr.exe"
HEX2BIN = HEX2BIN_DIR / "hex2bin.exe"
CALC_UPGRADE_CRC = HEX2BIN_DIR / "CalcUpgradeBinCrc.exe"

CHIP_TYPE = 1

# 目标分区（16 位字地址，右端不含），与 CMD/2837xS_FLASH_APP_CLA_lnk.cmd 一致
TARGETS = {
    "app": {"min": 0x086000, "max": 0x0A0000, "firmware_type": 1},
    "boot": {"min": 0x080000, "max": 0x086000, "firmware_type": 2},
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 16), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(argv: list[str], cwd: Path) -> None:
    result = subprocess.run(argv, cwd=cwd, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding="utf-8",
                            errors="replace")
    if result.stdout.strip():
        print(result.stdout.strip(), flush=True)
    if result.returncode:
        raise SystemExit("command failed (%d): %s" % (result.returncode, " ".join(argv)))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--hex", type=Path, required=True, help="Intel-HEX produced by the linker")
    parser.add_argument("--target", choices=sorted(TARGETS), default="app",
                        help="app = 0x086000..0x0A0000 (type 1), boot = 0x080000..0x086000 (type 2)")
    args = parser.parse_args()

    for tool in (CALC_HEX_ADDR, HEX2BIN, CALC_UPGRADE_CRC):
        if not tool.is_file():
            raise SystemExit("missing tool: %s" % tool)

    hex_file = args.hex.resolve()
    if not hex_file.is_file():
        raise SystemExit("missing hex file: %s" % hex_file)
    target = TARGETS[args.target]

    # 两个工具都按"同目录同名"输出，所以在 hex 所在目录里用文件名调用
    work = hex_file.parent
    print("hex    : %s" % hex_file)
    print("target : %s  0x%06X..0x%06X  firmware-type %d"
          % (args.target, target["min"], target["max"], target["firmware_type"]))

    run([str(CALC_HEX_ADDR), hex_file.name, "0x%06X" % target["min"],
         "0x%06X" % target["max"], str(HEX2BIN)], work)

    bin_file = hex_file.with_suffix(".bin")
    if not bin_file.is_file():
        raise SystemExit("hex2bin did not produce %s" % bin_file)

    run([str(CALC_UPGRADE_CRC), str(CHIP_TYPE), str(target["firmware_type"]),
         bin_file.name], work)

    upgrade = bin_file.with_name(bin_file.stem + "_UpgradeCRC32.bin")
    if not upgrade.is_file():
        raise SystemExit("CalcUpgradeBinCrc did not produce %s" % upgrade)

    for artifact in (bin_file, upgrade):
        print("%s: %d bytes sha256=%s" % (artifact, artifact.stat().st_size, sha256(artifact)))


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""A02 证据脚本：从本次干净构建的 MAP 中逐项核对链接结果。

检查项（每项独立给出 通过/未通过/待核对）：
  A02-1 codestart 段落在 BEGIN（0x086000），长度 2
  A02-2 入口符号 "_c_int00" 有效，且地址落在 FLASH_APP 内（不断言它等于 0x86000）
  A02-3 CLA 程序段：装载地址在 FLASH_APP、运行地址在 RAMLS（0x8000~0x9FFF）
  A02-4 .stack：RAMM1（0x400~0x7F7）、长度 0x200
  A02-5 两向 CLA 消息 RAM：CLA→CPU 在 0x1480、CPU→CLA 在 0x1500，各 ≤0x80，互不重叠
  A02-6 CLA 数据段 Cla1DataRam 运行于 RAMLS5（0xA800）
  A02-7 旧证据对照：仓库内已跟踪的 Debug/EP15K_2026.map 与当前 CMD/本次链接是否一致

用法：
  python analyze_map.py --map <新 map> [--stale-map <旧 map>] --out-dir <证据目录>
"""

import argparse
import json
import os
import re
import sys

FLASH_START = 0x086000
FLASH_END = 0x086000 + 0x1A000          # 覆盖 FLASH_APP 0x086002 + 0x19FFE
RAMLS_START, RAMLS_END = 0x8000, 0xA800  # RAMLS0..RAMLS5
RAMM1_START, RAMM1_END = 0x400, 0x800
MSG_LOW = 0x1480
MSG_HIGH = 0x1500
MSG_LEN = 0x80


def parse_memory(text):
    mem = {}
    in_block = False
    for line in text.splitlines():
        if "MEMORY CONFIGURATION" in line:
            in_block = True
            continue
        if in_block and re.match(r"^SEGMENT ALLOCATION MAP", line.strip()):
            break
        if not in_block:
            continue
        m = re.match(r"^\s+(\w+)\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s+([0-9a-f]{8})", line)
        if m:
            mem[m.group(1)] = {
                "origin": int(m.group(2), 16),
                "length": int(m.group(3), 16),
                "used": int(m.group(4), 16),
            }
    return mem


def parse_sections(text):
    """解析 SECTION ALLOCATION MAP 中每个输出段的 page/origin/length，附带 RUN ADDR。"""
    sections = {}
    lines = text.splitlines()
    in_block = False
    single = re.compile(
        r"^(\S+)\s+(\d)\s+([0-9a-f]{8})\s+([0-9a-f]{8})"
        r"(?:\s+RUN ADDR\s*=\s*([0-9a-f]{8}))?")
    cont = re.compile(r"^\*?\s+(\d)\s+([0-9a-f]{8})\s+([0-9a-f]{8})")
    name_only = re.compile(r"^([A-Za-z_.][\w.$]*)\s*$")
    for i, line in enumerate(lines):
        if line.strip().startswith("SECTION ALLOCATION MAP"):
            in_block = True
            continue
        if not in_block:
            continue
        if re.match(r"^SEGMENT ALLOCATION MAP|^MODULE SUMMARY|^SECTION ALLOCATION", line.strip()):
            if line.strip().startswith("SEGMENT ALLOCATION MAP"):
                break
        m = single.match(line)
        if m and m.group(1) not in ("output",):
            sections[m.group(1)] = {
                "page": int(m.group(2)),
                "origin": int(m.group(3), 16),
                "length": int(m.group(4), 16),
                "run": int(m.group(5), 16) if m.group(5) else None,
            }
            continue
        m = name_only.match(line)
        if m and i + 1 < len(lines):
            c = cont.match(lines[i + 1])
            if c:
                sections[m.group(1)] = {
                    "page": int(c.group(1)),
                    "origin": int(c.group(2), 16),
                    "length": int(c.group(3), 16),
                    "run": None,
                }
            continue
        c = cont.match(line)
        if c and i > 0:
            nm = name_only.match(lines[i - 1])
            if nm and nm.group(1) not in sections:
                sections[nm.group(1)] = {
                    "page": int(c.group(1)),
                    "origin": int(c.group(2), 16),
                    "length": int(c.group(3), 16),
                    "run": None,
                }
    return sections


def entry_point(text):
    m = re.search(r'ENTRY POINT SYMBOL:\s+"([^"]+)"\s+address:\s+([0-9a-f]{8})', text)
    if not m:
        return None, None
    return m.group(1), int(m.group(2), 16)


def symbol_address(text, name):
    """符号表行形如: `0008d4e3  _c_int00` 或 `0  0008d4e3  _c_int00`。"""
    m = re.search(r"^\s*(?:0\s+)?([0-9a-f]{8})\s+%s\s*$" % re.escape(name), text, re.M)
    return int(m.group(1), 16) if m else None


def hexs(v):
    return None if v is None else "0x%08X" % v


def check(checks, cid, title, expected, actual, ok):
    checks.append({
        "id": cid,
        "title": title,
        "expected": expected,
        "actual": actual,
        "verdict": "通过" if ok else "未通过",
    })


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--map", required=True)
    ap.add_argument("--stale-map")
    ap.add_argument("--out-dir", required=True)
    args = ap.parse_args()

    text = open(args.map, encoding="utf-8", errors="replace").read()
    mem = parse_memory(text)
    secs = parse_sections(text)
    ep_name, ep_addr = entry_point(text)
    checks = []
    observations = []

    # A02-1 codestart
    cs = secs.get("codestart")
    check(checks, "A02-1", "codestart 段落在 BEGIN",
          "origin=0x00086000, length=0x2",
          "origin=%s, length=%s" % (hexs(cs["origin"]) if cs else None,
                                    "0x%X" % cs["length"] if cs else None),
          bool(cs) and cs["origin"] == FLASH_START and cs["length"] == 2)

    # A02-2 入口符号
    sym = symbol_address(text, ep_name) if ep_name else None
    in_flash = sym is not None and FLASH_START <= sym < FLASH_END
    check(checks, "A02-2", "入口符号有效且位于 FLASH_APP（不断言等于 0x86000）",
          "符号 %s 存在，地址在 0x%08X~0x%08X 内且 != 0x86000" % (ep_name or "_c_int00", FLASH_START, FLASH_END),
          "symbol=%s entry_addr=%s symbol_table_addr=%s" % (ep_name, hexs(ep_addr), hexs(sym)),
          in_flash and sym != FLASH_START and ep_addr == sym)

    # A02-3 CLA 程序段
    cp = secs.get("Cla1Prog")
    ok = bool(cp) and cp["run"] is not None and FLASH_START <= cp["origin"] < FLASH_END \
        and RAMLS_START <= cp["run"] < RAMLS_END and cp["length"] <= 0x2000
    check(checks, "A02-3", "CLA 程序装载于 FLASH、运行于 RAMLS",
          "load 在 0x%08X~0x%08X，run 在 0x%04X~0x%04X" % (FLASH_START, FLASH_END, RAMLS_START, RAMLS_END),
          "load=%s length=%s run=%s" % (hexs(cp["origin"]) if cp else None,
                                        hexs(cp["length"]) if cp else None,
                                        hexs(cp["run"]) if cp else None),
          ok)

    # A02-4 栈
    st = secs.get(".stack")
    ok = bool(st) and RAMM1_START <= st["origin"] < RAMM1_END and st["length"] == 0x200
    check(checks, "A02-4", ".stack 位于 RAMM1 且长度 0x200",
          "origin 在 0x400~0x7F7，length=0x200",
          "page=%s origin=%s length=%s" % (st["page"] if st else None,
                                           hexs(st["origin"]) if st else None,
                                           hexs(st["length"]) if st else None),
          ok)

    # A02-5 消息 RAM
    lo = secs.get("Cla1ToCpuMsgRAM")
    hi = secs.get("CpuToCla1MsgRAM")
    ok = (bool(lo) and bool(hi) and lo["origin"] == MSG_LOW and hi["origin"] == MSG_HIGH
          and lo["length"] <= MSG_LEN and hi["length"] <= MSG_LEN
          and lo["origin"] + lo["length"] <= hi["origin"])
    check(checks, "A02-5", "两向消息 RAM 方向与容量",
          "Cla1ToCpu=0x1480 len<=0x80，CpuToCla1=0x1500 len<=0x80，互不重叠",
          "Cla1ToCpu=%s len=%s；CpuToCla1=%s len=%s" % (
              hexs(lo["origin"]) if lo else None, "0x%X" % lo["length"] if lo else None,
              hexs(hi["origin"]) if hi else None, "0x%X" % hi["length"] if hi else None),
          ok)

    # A02-6 CLA 数据段（page 1：origin 即运行地址，装载映像在 .cinit）
    cd = secs.get("Cla1DataRam")
    cd_run = (cd["run"] if cd and cd["run"] is not None else (cd["origin"] if cd else None))
    ok = bool(cd) and cd["page"] == 0 and cd_run == 0xA800 and cd["length"] <= 0x800
    check(checks, "A02-6", "CLA 数据段运行于 RAMLS5",
          "run(origin)=0xA800，length<=0x800",
          "page=%s load_image=%s run=%s length=%s" % (
              cd["page"] if cd else None,
              hexs(cd["origin"]) if cd else None,
              hexs(cd_run) if cd else None,
              "0x%X" % cd["length"] if cd else None),
          ok)

    # A02-7 旧 map 对照
    stale = {}
    if args.stale_map and os.path.isfile(args.stale_map):
        old = open(args.stale_map, encoding="utf-8", errors="replace").read()
        old_secs = parse_sections(old)
        old_ep_name, old_ep = entry_point(old)
        for name in ("Cla1ToCpuMsgRAM", "CpuToCla1MsgRAM", "Cla1Prog", "codestart", ".stack"):
            s = old_secs.get(name)
            stale[name] = {
                "origin": hexs(s["origin"]) if s else None,
                "length": hexs(s["length"]) if s else None,
                "run": hexs(s["run"]) if s else None,
            }
        stale["_entry"] = {"symbol": old_ep_name, "address": hexs(old_ep)}
        mismatch = (stale.get("Cla1ToCpuMsgRAM", {}).get("origin") != hexs(MSG_LOW)
                    or stale.get("CpuToCla1MsgRAM", {}).get("origin") != hexs(MSG_HIGH))
        observations.append({
            "id": "A02-7",
            "title": "仓库内已跟踪的 Debug/EP15K_2026.map 与当前链接结果是否一致",
            "finding": "旧 map 的 CLA 消息 RAM 方向与本次链接相反" if mismatch
                       else "旧 map 与本次链接一致",
            "detail": "旧 map: Cla1ToCpu=%s, CpuToCla1=%s；本次: Cla1ToCpu=%s, CpuToCla1=%s" % (
                stale.get("Cla1ToCpuMsgRAM", {}).get("origin"),
                stale.get("CpuToCla1MsgRAM", {}).get("origin"),
                hexs(lo["origin"]) if lo else None,
                hexs(hi["origin"]) if hi else None),
            "impact": "已跟踪的 Debug/ 产物是修复前构建，不得作为 A02 证据；如需分发二进制必须重新构建",
            "stale_entry": stale.get("_entry"),
        })

    report = {
        "map": os.path.abspath(args.map).replace("\\", "/"),
        "entry_point": {"symbol": ep_name, "address": hexs(ep_addr)},
        "memory_regions": {k: {"origin": hexs(v["origin"]), "length": hexs(v["length"]),
                               "used": hexs(v["used"])} for k, v in sorted(mem.items())},
        "sections": {k: {"page": v["page"], "origin": hexs(v["origin"]),
                         "length": hexs(v["length"]), "run": hexs(v["run"])}
                     for k, v in sorted(secs.items())},
        "stale_map_comparison": stale,
        "checks": checks,
        "observations": observations,
        "summary": {
            "total": len(checks),
            "passed": sum(1 for c in checks if c["verdict"] == "通过"),
            "failed": sum(1 for c in checks if c["verdict"] == "未通过"),
        },
    }
    os.makedirs(args.out_dir, exist_ok=True)
    with open(os.path.join(args.out_dir, "map_report.json"), "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)

    # 供人阅读的摘录：入口符号、关键内存区、关键段（含段的 origin/length 行）
    keep = []
    lines = text.splitlines()
    mem_want = re.compile(r"^\s*(RAMM0|RAMM1|RAMLS[0-5]|RAMD0|BEGIN|FLASH_APP|"
                          r"CLA1_MSGRAMLOW|CLA1_MSGRAMHIGH)\s+[0-9a-fA-F]{8}")
    sec_want = re.compile(r"^(codestart|\.stack|Cla1Prog|Cla1DataRam|Cla1ToCpuMsgRAM|"
                          r"CpuToCla1MsgRAM|\.TI\.ramfunc)\b")
    sec_hdr = re.compile(r"^[A-Za-z_.][\w.]*\s+\d\s+[0-9a-fA-F]{8}\s+[0-9a-fA-F]{8}")
    i = 0
    while i < len(lines):
        line = lines[i]
        if "ENTRY POINT SYMBOL" in line:
            keep.append(line.rstrip())
            i += 1
            continue
        if mem_want.match(line):
            keep.append(line.rstrip())
            i += 1
            continue
        if line.strip() in ("codestart", "Cla1DataRam", "Cla1ToCpuMsgRAM", "CpuToCla1MsgRAM"):
            keep.append(line.rstrip())
            j2 = i + 1
            while j2 < len(lines) and lines[j2].strip() and j2 <= i + 4:
                keep.append(lines[j2].rstrip())
                j2 += 1
            i = j2
            continue
        m = sec_hdr.match(line)
        if m and sec_want.match(line.split()[0]):
            keep.append(line.rstrip())
            j2 = i + 1
            while j2 < len(lines) and lines[j2].strip() and j2 <= i + 4 and not sec_hdr.match(lines[j2]):
                keep.append(lines[j2].rstrip())
                j2 += 1
            i = j2
            continue
        i += 1
    with open(os.path.join(args.out_dir, "map_excerpt.txt"), "w", encoding="utf-8") as f:
        f.write("MAP: %s\n\n" % report["map"])
        f.write("\n".join(keep) + "\n")

    for c in checks:
        print("[%s] %s %s | 期望: %s | 实测: %s" % (
            c["verdict"], c["id"], c["title"], c["expected"], c["actual"]))
    for o in observations:
        print("[记录] %s %s | %s | %s" % (o["id"], o["title"], o["finding"], o["detail"]))
    print("A02: %(passed)d/%(total)d 通过" % report["summary"])
    return 0 if report["summary"]["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

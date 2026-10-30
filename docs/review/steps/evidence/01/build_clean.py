#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""A01/A02 证据脚本：只用指定提交的已跟踪文件重建 C2000 固件，并记录可复跑证据。

这不是 CCS 工程构建，也不复现 CCS 的增量构建：
  * 唯一输入是 `git archive --rev` 解出的跟踪树（默认 HEAD；证据必须显式固定到
    某个提交，否则提交证据后再跑会换成新版本。工作区改动、未跟踪文件、仓库内
    已跟踪的 Debug/ 旧产物都不参与）；
  * 编译旗标与链接清单从该树里 CCS 生成的 Debug/subdir_rules.mk、Debug/ccsObjs.opt
    中读取，仅用于提取参数，不复用任何旧 .obj；
  * 输出目录必须是空目录，脚本会先证明 obj/ 为空，再逐个编译并在事后校验每个
    .obj 的生成时间不早于构建开始时间。

用法（Windows，Git Bash 或 cmd 均可）：
  python build_clean.py --repo <仓库根> --workdir <空目录> [--rev <提交>] [--toolchain <CG_TOOL_ROOT>]

产出（全部写入 workdir，不写回仓库）：
  src/                git archive <rev> 的解包树
  obj/                本次编译生成的 .obj（按源码目录镜像布局）
  build/              EP15K_2026.out / .map / _linkInfo.xml
  compile_log.jsonl   每个编译单元的完整命令、返回码、耗时、告警条数
  link.log            链接器完整输出
  build_summary.json  A01/A02 汇总证据（工具链、旗标、单元数、哈希、旧产物对照）
"""

import argparse
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import time
from datetime import datetime, timezone

DEFAULT_TOOLCHAIN = r"C:/Users/86130/Downloads/ti-cgt-c2000_22.6.2.LTS"
EXPECTED_UNITS_NOTE = ("compile units are cross-checked against "
                       "`git ls-tree -r --name-only <pinned_sha>` (*.c/*.cla/*.asm, reference/ excluded)")


def sha256_file(path, limit=None):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(1 << 20)
            if not chunk:
                break
            h.update(chunk)
            if limit is not None and f.tell() > limit:
                break
    return h.hexdigest()


def run(cmd, cwd, timeout=600):
    t0 = time.time()
    proc = subprocess.run(
        cmd, cwd=cwd, capture_output=True, text=True,
        encoding="utf-8", errors="replace", timeout=timeout,
    )
    return proc.returncode, (proc.stdout or "") + (proc.stderr or ""), time.time() - t0


def parse_compile_flags(rules_path, src_root, toolchain):
    """从 CCS 生成的 subdir_rules.mk 中提取编译旗标，并把失效的 include 根改写到跟踪树。"""
    text = open(rules_path, encoding="utf-8", errors="replace").read()
    line = None
    for candidate in text.splitlines():
        if "cl2000" in candidate and "--preproc_with_compile" in candidate:
            line = candidate.strip()
            break
    if line is None:
        raise SystemExit("subdir_rules.mk 中没有找到 CCS 编译命令行")

    tokens = shlex.split(line, posix=True)
    # line 形如: "C:/.../cl2000" -v28 ... --include_path="D:/..." ... "$<"
    tokens = tokens[1:]  # 去掉可执行文件
    flags, include_dirs, dropped = [], [], []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok.startswith("--include_path="):
            include_dirs.append(tok.split("=", 1)[1])
        elif tok.startswith("$") or tok.startswith("--preproc_") or tok == "--preproc_with_compile":
            # Makefile 变量与预处理开关（$(GEN_OPTS__FLAG)、$<、--preproc_with_compile 等）
            dropped.append(tok)
        else:
            flags.append(tok)
        i += 1

    # 失效的 include 根：CCS 生成时工程位于 D:/2026WorkPlace/EP15K_2026_0.1/EP15K_2026
    dead_root = None
    for inc in include_dirs:
        m = re.match(r"^(.*?/EP15K_2026)(?:/.*)?$", inc.replace("\\", "/"))
        if m:
            dead_root = m.group(1)
            break
    if dead_root is None:
        raise SystemExit("未能从 include 路径识别 CCS 工程根，无法安全改写")

    resolved = []
    for inc in include_dirs:
        norm = inc.replace("\\", "/")
        if norm.startswith(dead_root):
            rel = norm[len(dead_root):].lstrip("/")
            target = os.path.join(src_root, rel) if rel else src_root
        else:
            target = norm  # 例如 CG_TOOL_ROOT/include
        resolved.append(target.replace("\\", "/"))

    missing = [d for d in resolved if not os.path.isdir(d)]
    if missing:
        raise SystemExit("改写后的 include 目录不存在: %s" % missing)

    return flags, resolved, dead_root, dropped


def parse_link_list(opt_path):
    """Debug/ccsObjs.opt = CCS 生成的链接清单：101 个 obj + 2 个 CMD + 4 个库 + -llibc.a。"""
    text = open(opt_path, encoding="utf-8", errors="replace").read()
    entries = re.findall(r'"([^"]+)"|(\S+)', text)
    items = [a or b for a, b in entries]
    objs = [e for e in items if e.endswith(".obj")]
    cmds = [e for e in items if e.endswith(".cmd")]
    libs = [e for e in items if e.endswith(".lib") or e == "-llibc.a"]
    return objs, cmds, libs


def resolve_source(src_root, obj_rel):
    """obj 路径 -> 源文件（.c / .cla / .asm），相对路径以 src_root 为根。"""
    stem = os.path.splitext(obj_rel)[0]
    for ext in (".c", ".cla", ".asm"):
        cand = os.path.join(src_root, stem + ext)
        if os.path.isfile(cand):
            return stem + ext
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", required=True)
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--rev", default="HEAD",
                    help="要构建的提交（默认 HEAD；证据必须固定到具体 SHA）")
    ap.add_argument("--toolchain", default=DEFAULT_TOOLCHAIN)
    args = ap.parse_args()

    repo = os.path.abspath(args.repo)
    workdir = os.path.abspath(args.workdir)
    toolchain = args.toolchain.replace("\\", "/")
    cl2000 = os.path.join(toolchain, "bin", "cl2000.exe")
    if not os.path.isfile(cl2000):
        raise SystemExit("找不到编译器: %s" % cl2000)
    if os.path.isdir(workdir) and os.listdir(workdir):
        raise SystemExit("workdir 必须为空目录: %s" % workdir)
    os.makedirs(workdir, exist_ok=True)

    # ---------- 固定提交 ----------
    rc, pinned, err = run(["git", "-C", repo, "rev-parse", "--verify",
                           args.rev + "^{commit}"], workdir)
    if rc:
        raise SystemExit("无法解析 --rev %r: %s" % (args.rev, err.strip()))
    pinned = pinned.strip()
    rc, describe, _ = run(["git", "-C", repo, "log", "-1", "--format=%H %s", pinned], workdir)
    pinned_subject = describe.strip()

    src_root = os.path.join(workdir, "src")
    obj_root = os.path.join(workdir, "obj")
    build_root = os.path.join(workdir, "build")
    for d in (src_root, obj_root, build_root):
        os.makedirs(d, exist_ok=True)

    summary = {
        "step": "01",
        "generated_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "repo": repo.replace("\\", "/"),
        "workdir": workdir.replace("\\", "/"),
        "pinned_sha": pinned,
        "pinned_commit": pinned_subject,
        "requested_rev": args.rev,
        "declare": "NOT a CCS project build; direct cl2000 invocation on git archive <pinned_sha> only",
    }

    # ---------- 0. 仓库状态 ----------
    rc, head, _ = run(["git", "-C", repo, "rev-parse", "HEAD"], workdir)
    rc2, status, _ = run(["git", "-C", repo, "status", "--porcelain=v1"], workdir)
    rc3, branch, _ = run(["git", "-C", repo, "rev-parse", "--abbrev-ref", "HEAD"], workdir)
    if rc or rc2 or rc3:
        raise SystemExit("git 查询失败")
    head = head.strip()
    summary["git"] = {
        "head": head,
        "branch": branch.strip(),
        "pinned_sha": pinned,
        "note": "构建输入仅为 git archive <pinned_sha>；HEAD 与工作区状态只作记录，不参与构建",
        "worktree_status": [l for l in status.splitlines() if l.strip()],
    }

    # ---------- 1. 解包固定提交 ----------
    archive = os.path.join(workdir, "head.tar")
    rc, out, _ = run(["git", "-C", repo, "archive", "--format=tar", "-o", archive,
                      pinned], workdir)
    if rc:
        raise SystemExit("git archive 失败: %s" % out)
    with tarfile.open(archive) as tf:
        members = tf.getmembers()
        tf.extractall(src_root)
    summary["archive"] = {
        "path": archive.replace("\\", "/"),
        "sha256": sha256_file(archive),
        "top_level_entries": len(os.listdir(src_root)),
        "members": len(members),
        "sources": "git archive %s" % pinned,
    }

    # ---------- 2. 编译清单 ----------
    opt_path = os.path.join(src_root, "Debug", "ccsObjs.opt")
    objs, cmd_files, lib_items = parse_link_list(opt_path)
    if len(objs) != len(set(objs)):
        raise SystemExit("链接清单中存在重复 .obj")

    unit_rows, missing_src, unresolved = [], [], []
    for obj_rel in objs:
        rel = obj_rel[2:] if obj_rel.startswith("./") else obj_rel
        src_rel = resolve_source(src_root, rel)
        if src_rel is None:
            missing_src.append(rel)
            continue
        unit_rows.append({"obj": rel, "src": src_rel})
    if missing_src:
        raise SystemExit("链接清单中的 .obj 找不到对应源码: %s" % missing_src)

    # 跟踪源码与清单交叉核对（用固定提交的树，不读索引/工作区）
    rc, tracked, _ = run(["git", "-C", repo, "ls-tree", "-r", "--name-only", pinned], workdir)
    tracked = [l.strip() for l in tracked.splitlines()
               if l.strip().endswith((".c", ".cla", ".asm"))
               and not l.strip().startswith("reference/")]
    listed = {r["src"] for r in unit_rows}
    unbuilt = sorted(set(tracked) - listed)
    phantom = sorted(listed - set(tracked))
    summary["manifest"] = {
        "source_of_manifest": "Debug/ccsObjs.opt (CCS-generated link list, from git archive %s)" % pinned[:8],
        "pinned_sha": pinned,
        "obj_count": len(objs),
        "cmd_files": cmd_files,
        "libs": lib_items,
        "tracked_source_count": len(tracked),
        "by_extension": {e: sum(1 for t in tracked if t.endswith(e)) for e in (".c", ".cla", ".asm")},
        "tracked_sources_not_in_build": unbuilt,
        "build_sources_not_tracked": phantom,
        "note": EXPECTED_UNITS_NOTE,
    }

    # ---------- 3. 旗标 ----------
    flags, includes, dead_root, dropped = parse_compile_flags(
        os.path.join(src_root, "Debug", "subdir_rules.mk"), src_root, toolchain)
    include_args = ["--include_path=%s" % d for d in includes]
    summary["compile"] = {
        "toolchain": toolchain,
        "cl2000_sha256": sha256_file(cl2000),
        "compiler_banner": None,
        "dead_include_root_in_ccs_makefile": dead_root,
        "dropped_tokens": dropped,
        "flags": flags,
        "include_dirs": includes,
        "units": len(unit_rows),
    }
    rc, banner, _ = run([cl2000, "-h"], workdir)
    m = re.search(r"TMS320C2000 C/C\+\+ Compiler\s+v(\S+)", banner)
    summary["compile"]["compiler_banner"] = m.group(1) if m else banner.splitlines()[:2]

    # ---------- 4. 空输出目录证明 ----------
    pre_objs = []
    for root, _dirs, files in os.walk(obj_root):
        pre_objs += [os.path.join(root, f) for f in files]
    summary["empty_output_dir"] = {
        "obj_files_before": pre_objs,
        "assertion": "obj/ 在编译开始前为空（本次全部 .obj 均为新建）",
    }
    if pre_objs:
        raise SystemExit("obj 目录非空，拒绝复用")

    # 仓库内已跟踪的旧产物：记录哈希，证明未被使用
    stale = {}
    for rel in ("Debug/main.obj", "Debug/EP15K_2026.out", "Debug/EP15K_2026.map"):
        p = os.path.join(src_root, rel)
        if os.path.isfile(p):
            stale[rel] = {"sha256": sha256_file(p), "size": os.path.getsize(p)}
    summary["tracked_stale_artifacts"] = stale

    # ---------- 5. 编译 ----------
    t_start = time.time()
    log_path = os.path.join(workdir, "compile_log.jsonl")
    failures = []
    warnings_total = 0
    with open(log_path, "w", encoding="utf-8") as log:
        for idx, unit in enumerate(unit_rows, 1):
            src_abs = os.path.join(src_root, unit["src"])
            obj_dir = os.path.join(obj_root, os.path.dirname(unit["obj"]))
            os.makedirs(obj_dir, exist_ok=True)
            cmd = [cl2000] + flags + include_args + ["-c", src_abs]
            rc, out, dt = run(cmd, obj_dir)
            warn = len(re.findall(r"(?im)^\s*(?:warning|remark)", out))
            warnings_total += warn
            obj_abs = os.path.join(obj_dir, os.path.splitext(os.path.basename(unit["obj"]))[0] + ".obj")
            rec = {
                "n": idx,
                "src": unit["src"],
                "obj": os.path.relpath(obj_abs, workdir).replace("\\", "/"),
                "rc": rc,
                "seconds": round(dt, 2),
                "warnings": warn,
                "stdout": out.strip()[:4000],
                "created": os.path.isfile(obj_abs),
            }
            log.write(json.dumps(rec, ensure_ascii=False) + "\n")
            if rc != 0 or not rec["created"]:
                failures.append(rec)
    summary["compile"]["failures"] = failures
    summary["compile"]["warnings_total"] = warnings_total
    summary["compile"]["elapsed_seconds"] = round(time.time() - t_start, 1)

    # 事后校验：所有 obj 都比构建开始新
    old = []
    for root, _dirs, files in os.walk(obj_root):
        for f in files:
            p = os.path.join(root, f)
            if os.path.getmtime(p) < t_start - 1:
                old.append(p)
    summary["empty_output_dir"]["obj_files_older_than_build_start"] = [
        os.path.relpath(p, workdir).replace("\\", "/") for p in old]
    summary["empty_output_dir"]["obj_count_after"] = sum(
        len(files) for _r, _d, files in os.walk(obj_root))

    if failures:
        summary["result"] = {"build": "FAILED", "stage": "compile"}
        with open(os.path.join(workdir, "build_summary.json"), "w", encoding="utf-8") as f:
            json.dump(summary, f, ensure_ascii=False, indent=2)
        print("编译失败: %d 个单元" % len(failures))
        for f in failures[:5]:
            print(" -", f["src"], "rc=%s" % f["rc"], f["stdout"][:300])
        return 2

    # ---------- 6. 链接 ----------
    link_cmd = [cl2000] + flags + [
        "-z",
        "-mEP15K_2026.map",
        "--stack_size=0x200",
        "--warn_sections",
        "-i%s/lib" % toolchain,
        "-i%s/include" % toolchain,
        "--reread_libs",
        "--diag_wrap=off",
        "--display_error_number",
        "--xml_link_info=EP15K_2026_linkInfo.xml",
        "--rom_model",
        "-o", "EP15K_2026.out",
    ]
    obj_paths = []
    for unit in unit_rows:
        obj_paths.append(os.path.join(obj_root, os.path.splitext(unit["obj"])[0] + ".obj"))
    for c in cmd_files:
        p = os.path.join(src_root, c[3:] if c.startswith("../") else c)
        if not os.path.isfile(p):
            raise SystemExit("缺链接脚本: %s" % p)
        obj_paths.append(p)
    for lib in lib_items:
        if lib == "-llibc.a":
            obj_paths.append(lib)
        else:
            p = os.path.join(src_root, lib[3:] if lib.startswith("../") else lib)
            if not os.path.isfile(p):
                raise SystemExit("缺库文件: %s" % p)
            obj_paths.append(p)
    link_cmd += obj_paths

    rc, out, dt = run(link_cmd, build_root)
    with open(os.path.join(workdir, "link.log"), "w", encoding="utf-8") as f:
        f.write("cwd: %s\n" % build_root.replace("\\", "/"))
        f.write("cmd: %s\n\n" % " ".join(link_cmd))
        f.write(out)
    out_path = os.path.join(build_root, "EP15K_2026.out")
    map_path = os.path.join(build_root, "EP15K_2026.map")
    summary["link"] = {
        "rc": rc,
        "seconds": round(dt, 1),
        "warnings": len(re.findall(r"(?im)^\s*warning", out)),
        "errors": [l for l in out.splitlines() if "error" in l.lower()][:20],
        "out": {"path": out_path.replace("\\", "/"), "exists": os.path.isfile(out_path),
                "sha256": sha256_file(out_path) if os.path.isfile(out_path) else None,
                "size": os.path.getsize(out_path) if os.path.isfile(out_path) else None},
        "map": {"path": map_path.replace("\\", "/"), "exists": os.path.isfile(map_path),
                "sha256": sha256_file(map_path) if os.path.isfile(map_path) else None},
        "cmd": " ".join(["<cl2000>"] + flags + ["-z", "-mEP15K_2026.map", "--stack_size=0x200",
                                                 "--warn_sections", "-i<toolchain>/lib",
                                                 "-i<toolchain>/include", "--reread_libs",
                                                 "--diag_wrap=off", "--display_error_number",
                                                 "--xml_link_info=EP15K_2026_linkInfo.xml",
                                                 "--rom_model", "-o EP15K_2026.out",
                                                 "<%d objs>" % len(objs)] + cmd_files + lib_items),
    }
    summary["result"] = {
        "build": "OK" if (rc == 0 and os.path.isfile(out_path)) else "FAILED",
        "stage": "link" if rc else "done",
    }

    with open(os.path.join(workdir, "build_summary.json"), "w", encoding="utf-8") as f:
        json.dump(summary, f, ensure_ascii=False, indent=2)

    print(json.dumps({
        "head": head,
        "units": len(objs),
        "compile_failures": 0,
        "warnings_total": warnings_total,
        "link_rc": rc,
        "out_sha256": summary["link"]["out"]["sha256"],
        "map_sha256": summary["link"]["map"]["sha256"],
    }, ensure_ascii=False, indent=2))
    return 0 if rc == 0 else 3


if __name__ == "__main__":
    sys.exit(main())

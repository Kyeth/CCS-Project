"""Prove that A03-PWM-02 / A03-PWM-03 check every channel, not only the first one.

For each case the pinned extraction tree is copied to a scratch directory, ONE
deliberate error is injected into a NON-first entry, and config_replay.py is run
against the mutated copy. The case passes only when the target check turns FAIL.
The repository tree itself is never modified; every mutation lives under --work.
"""
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPLAY = HERE / "config_replay.py"
CONFIG_REL = "Broad/src/DrvCfg_EP15K.c"

CASES = [
    {
        "check": "A03-PWM-02",
        "label": "逆变第 2 路（EPWM2/HW_PWM_ID1）AQ 的 CAU 由 AQ_SET 改成 AQ_CLEAR",
        "anchor": "HW_PWM_ID1, HW_PWM_TYPE_INV",
        "old": "AQ_SET",
        "new": "AQ_CLEAR",
    },
    {
        "check": "A03-PWM-03",
        "label": "PV 第 2 路（EPWM9/HW_PWM_ID8）周期由 4999U 改成 3000U",
        "anchor": "HW_PWM_ID8, HW_PWM_TYPE_PV",
        "old": "4999U",
        "new": "3000U",
    },
]


def inject(text: str, anchor: str, old: str, new: str):
    lines = text.split("\n")
    try:
        start = next(i for i, line in enumerate(lines) if anchor in line)
    except StopIteration:
        raise SystemExit("anchor not found: %s" % anchor)
    for i in range(start, min(start + 12, len(lines))):
        if old in lines[i]:
            lines[i] = lines[i].replace(old, new, 1)
            return "\n".join(lines), i + 1
    raise SystemExit("token %r not found near %r" % (old, anchor))


def run_case(case, src: Path, work: Path, rev: str) -> dict:
    case_dir = work / case["check"]
    if case_dir.exists():
        shutil.rmtree(case_dir)
    tree = case_dir / "src"
    shutil.copytree(src, tree)

    config = tree / CONFIG_REL
    mutated, line_no = inject(config.read_text(encoding="utf-8"), case["anchor"], case["old"], case["new"])
    config.write_bytes(mutated.encode("utf-8"))

    outdir = case_dir / "out"
    outdir.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(
        [sys.executable, str(REPLAY), "--src", str(tree), "--rev", rev, "--out", str(outdir)],
        capture_output=True, text=True, encoding="utf-8", errors="replace")

    report_path = outdir / "config_replay_report.json"
    if not report_path.exists():
        return {"case": case, "detected": False, "note": "replay produced no report",
                "stderr": (proc.stderr or "")[-800:]}
    report = json.loads(report_path.read_text(encoding="utf-8"))
    verdicts = {c["id"]: c["verdict"] for c in report["checks"]}
    target = verdicts.get(case["check"], "MISSING")
    return {
        "case": case,
        "line": line_no,
        "target_verdict": target,
        "detected": target == "FAIL",
        "fail_items": report["summary"]["fail_items"],
        "counts": report["summary"]["counts"],
        "rc": proc.returncode,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True, help="pinned git archive extraction (read-only input)")
    ap.add_argument("--work", required=True, help="scratch directory for mutated copies")
    ap.add_argument("--rev", default="unknown")
    args = ap.parse_args()

    src = Path(args.src)
    work = Path(args.work)
    work.mkdir(parents=True, exist_ok=True)

    ok = True
    for case in CASES:
        res = run_case(case, src, work, args.rev)
        status = "DETECTED" if res["detected"] else "MISSED"
        print("[%s] %s：%s（第 %s 行注入）" % (status, case["check"], case["label"], res.get("line")))
        print("        目标检查判定=%s，该次运行的 FAIL 列表=%s，计数=%s"
              % (res.get("target_verdict"), res.get("fail_items"), res.get("counts")))
        if not res["detected"]:
            ok = False
            if res.get("note"):
                print("        note: %s" % res["note"])
            if res.get("stderr"):
                print("        stderr: %s" % res["stderr"])
    print("结论：%s" % ("全部检出（2/2）" if ok else "存在未检出用例"))
    return 0 if ok else 3


if __name__ == "__main__":
    sys.exit(main())

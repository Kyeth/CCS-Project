"""Compute and verify the SHA-256 of the step 01 evidence from Git objects.

Hashing rule: every value in evidence_hashes.json is the SHA-256 of the bytes
stored in the Git blob `<rev>:docs/review/steps/evidence/01/<name>` (read with
`git cat-file blob`). It is NOT a hash of the working-tree file: this checkout
uses core.autocrlf=true, so the work tree may hold CRLF while the commit holds
LF, and a work-tree hash would then disagree with the commit.

Usage
    # rebuild evidence_hashes.json from the blobs staged for the next commit
    python verify_hashes.py --write

    # verify the committed file against a commit's blobs (after the commit exists)
    python verify_hashes.py --rev HEAD
"""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[4]
REL_DIR = "docs/review/steps/evidence/01"
HASHES = HERE / "evidence_hashes.json"
BUILD_DIR = Path("D:/Project/out/step01")
BUILD_PRODUCTS = ["build/EP15K_2026.out", "build/EP15K_2026.map", "head.tar"]

METHOD = ("sha256 over the exact bytes stored in the Git blob "
          "`<rev>:docs/review/steps/evidence/01/<name>` (read with `git cat-file blob`). "
          "The working tree is not authoritative: core.autocrlf=true may check the "
          "committed LF out as CRLF, so work-tree hashes can differ from the commit.")


def git(*args: str) -> bytes:
    proc = subprocess.run(["git", "-C", str(REPO), *args], capture_output=True)
    if proc.returncode != 0:
        raise SystemExit("git %s failed: %s" % (" ".join(args), proc.stderr.decode("utf-8", "replace")))
    return proc.stdout


def blob_bytes(rev: str, rel: str) -> bytes:
    return git("cat-file", "blob", "%s:%s" % (rev, rel))


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def staged_entries():
    """name -> (blob_oid, size) for every staged file in the evidence directory."""
    out = git("ls-files", "-s", "--", REL_DIR).decode("utf-8")
    entries = {}
    for line in out.splitlines():
        meta, path = line.split("\t", 1)
        _mode, oid, _stage = meta.split()
        name = path.rsplit("/", 1)[-1]
        if name in ("evidence_hashes.json",):
            continue
        entries[name] = oid
    return entries


def write_hashes() -> int:
    dirty = git("status", "--porcelain", "--", REL_DIR).decode("utf-8").splitlines()
    problems = [ln for ln in dirty if ln[:2] == "??" or (len(ln) > 1 and ln[1] != " ")]
    if problems:
        print("ERROR: untracked or unstaged changes in the evidence directory; stage everything first:")
        for ln in problems:
            print("  " + ln)
        return 1

    entries = staged_entries()
    evidence = {}
    for name, oid in sorted(entries.items()):
        data = git("cat-file", "blob", oid)
        evidence[name] = {
            "sha256": sha256(data),
            "size": len(data),
            "git_blob_oid": oid,
        }

    products = {}
    for rel in BUILD_PRODUCTS:
        path = BUILD_DIR / rel
        data = path.read_bytes()
        products[Path(rel).name] = {"sha256": sha256(data), "size": len(data)}

    doc = {
        "step": "01",
        "hash_method": METHOD,
        "computed_from": "the blobs staged for the commit that adds this file "
                         "(identical to that commit's blobs; verified afterwards with --rev HEAD)",
        "verify_command": "python docs/review/steps/evidence/01/verify_hashes.py --rev HEAD",
        "note": "构建产物（.out/.map/.obj/.tar）不入库，只登记哈希；文本证据逐项对应 Git 提交对象中的字节。",
        "build_products_not_committed": products,
        "evidence_files": evidence,
    }
    HASHES.write_bytes((json.dumps(doc, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    print("wrote %s with %d file hashes (blob-based)" % (HASHES.name, len(evidence)))
    return 0


def verify(rev: str) -> int:
    doc = json.loads(HASHES.read_text(encoding="utf-8"))
    entries = doc["evidence_files"]
    ok = 0
    bad = []
    print("verify %d evidence files against %s:%s" % (len(entries), rev, REL_DIR))
    for name, meta in sorted(entries.items()):
        try:
            data = blob_bytes(rev, "%s/%s" % (REL_DIR, name))
        except SystemExit:
            print("  MISSING   %s" % name)
            bad.append(name)
            continue
        got = sha256(data)
        good = got == meta["sha256"] and len(data) == meta["size"]
        print("  %-9s %-32s %s size=%d" % ("OK" if good else "MISMATCH", name, got[:16], len(data)))
        if good:
            ok += 1
        else:
            bad.append(name)
    print("result: %d/%d match" % (ok, len(entries)))
    if bad:
        print("mismatched: %s" % ", ".join(bad))
        return 4
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--rev")
    args = ap.parse_args()
    if args.write:
        return write_hashes()
    if args.rev:
        return verify(args.rev)
    ap.error("pass --write or --rev <commit>")


if __name__ == "__main__":
    sys.exit(main())

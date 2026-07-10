#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Run parser stress tests and summarize results.

The runner expects the interpreter to exit with:
  0  -> clean success
  1  -> compilation diagnostics (expected for malformed stress inputs)
  other / ASan output -> crash or unexpected failure
"""

import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Auto-detect interpreter location depending on platform/build layout.
_CANDIDATES = [
    ROOT / "cmake-build-debug" / "bin" / "ShardScript.Interpreter",
    ROOT / "out" / "build" / "linux-debug" / "bin" / "ShardScript.Interpreter",
    ROOT / "out" / "build" / "x64-debug" / "bin" / "ShardScript.Interpreter.exe",
]
INTERPRETER = None
for candidate in _CANDIDATES:
    if candidate.exists():
        INTERPRETER = candidate
        break

if INTERPRETER is None:
    print("ERROR: Could not find ShardScript.Interpreter binary.", file=sys.stderr)
    print("Searched:", file=sys.stderr)
    for candidate in _CANDIDATES:
        print(f"  {candidate}", file=sys.stderr)
    sys.exit(2)


def _find_asan_dll():
    """Locate the ASan runtime DLL that matches a 64-bit MSVC/Clang build."""
    search_roots = [
        Path(r"C:\Program Files\LLVM\lib\clang\18\lib\windows"),
        Path(r"C:\Program Files\LLVM\lib\clang\19\lib\windows"),
        Path(r"C:\Program Files\LLVM\lib\clang\20\lib\windows"),
        Path(r"C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"),
    ]
    for root in search_roots:
        if not root.exists():
            continue
        # Recurse only a few levels to keep the search fast.
        for dll in root.rglob("clang_rt.asan_dynamic-x86_64.dll"):
            return dll
    return None


def _ensure_asan_runtime():
    """Make sure the ASan runtime DLL is next to the interpreter executable."""
    target = INTERPRETER.parent / "clang_rt.asan_dynamic-x86_64.dll"
    if target.exists():
        return True

    source = _find_asan_dll()
    if source is None:
        print(
            "WARNING: Could not find clang_rt.asan_dynamic-x86_64.dll. "
            "Interpreter may fail to start.",
            file=sys.stderr,
        )
        return False

    try:
        shutil.copy2(source, target)
        return True
    except OSError as exc:
        print(f"WARNING: Failed to copy ASan runtime: {exc}", file=sys.stderr)
        return False


_ensure_asan_runtime()

STRESS_DIR = Path(__file__).parent
TIMEOUT = 10

# Any output mentioning AddressSanitizer is treated as a crash regardless of
# the process exit code (ASan aborts may report exit 0 on Windows).
ASAN_MARKERS = ("AddressSanitizer:", "==ERROR: AddressSanitizer")


def _has_asan_output(stdout: str, stderr: str) -> bool:
    combined = stdout + stderr
    return any(marker in combined for marker in ASAN_MARKERS)


files = sorted(STRESS_DIR.glob("*.shard"))
results = []
for f in files:
    cmd = [str(INTERPRETER), "--exclude-std", str(f)]
    start = time.time()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=TIMEOUT)
        elapsed = time.time() - start
        results.append(
            {
                "file": f.name,
                "code": proc.returncode,
                "stdout": proc.stdout,
                "stderr": proc.stderr,
                "elapsed": elapsed,
            }
        )
    except subprocess.TimeoutExpired:
        results.append(
            {
                "file": f.name,
                "code": -1,
                "stdout": "",
                "stderr": "TIMEOUT",
                "elapsed": TIMEOUT,
            }
        )

# Summarize
crashes = [
    r
    for r in results
    if _has_asan_output(r["stdout"], r["stderr"]) or r["code"] not in (0, 1)
]
timeouts = [r for r in results if r["code"] == -1]
diagnostics = [
    r
    for r in results
    if r["code"] == 1 and not _has_asan_output(r["stdout"], r["stderr"])
]
success = [
    r
    for r in results
    if r["code"] == 0 and not _has_asan_output(r["stdout"], r["stderr"])
]

print(f"Total: {len(results)}")
print(f"Success (0): {len(success)}")
print(f"Diagnostics (1): {len(diagnostics)}")
print(f"Crashes/other: {len(crashes)}")
print(f"Timeouts: {len(timeouts)}")

if timeouts:
    print("\n=== TIMEOUTS ===")
    for r in timeouts:
        print(r["file"])

if crashes:
    print("\n=== CRASHES / NON-ZERO EXIT (not 1) ===")
    for r in crashes:
        print(f"\n--- {r['file']} (code={r['code']}, time={r['elapsed']:.2f}s) ---")
        if r["stderr"]:
            print("STDERR:", r["stderr"][:1000])
        if r["stdout"]:
            print("STDOUT:", r["stdout"][:1000])

if diagnostics:
    print("\n=== DIAGNOSTIC FILES (first 20) ===")
    for r in diagnostics[:20]:
        # Show the first non-empty line of diagnostic output; strip ANSI colors.
        combined = (r["stdout"] + r["stderr"]).strip()
        first_line = next((line for line in combined.splitlines() if line.strip()), "")
        # Remove ANSI escape sequences.
        first_line = re.sub(r"\x1b\[[0-9;]*m", "", first_line)
        if len(first_line) > 160:
            first_line = first_line[:157] + "..."
        print(f"{r['file']}: {first_line}")

# Save detailed log
log = STRESS_DIR / "stress_log.txt"
with log.open("w", encoding="utf-8") as fh:
    for r in results:
        fh.write(
            f"===== {r['file']} (exit={r['code']}, time={r['elapsed']:.2f}s) =====\n"
        )
        fh.write("STDOUT:\n" + r["stdout"] + "\n")
        fh.write("STDERR:\n" + r["stderr"] + "\n\n")
print(f"\nDetailed log written to {log}")

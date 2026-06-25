#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Run parser stress tests and summarize results."""

import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INTERPRETER = (
    ROOT / "out" / "build" / "x64-debug" / "bin" / "ShardScript.Interpreter.exe"
)
STRESS_DIR = Path(__file__).parent
TIMEOUT = 10

files = sorted(STRESS_DIR.glob("*.shard"))
results = []
for f in files:
    cmd = [str(INTERPRETER), "--no-std", str(f)]
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
crashes = [r for r in results if r["code"] not in (0, 1)]
timeouts = [r for r in results if r["code"] == -1]
diagnostics = [r for r in results if r["code"] == 1]
success = [r for r in results if r["code"] == 0]

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

# Also list diagnostic-only files with unusually short/large output (possible silent mishandling)
print("\n=== DIAGNOSTIC FILES (first 20) ===")
for r in diagnostics[:20]:
    out = (r["stdout"] + r["stderr"]).strip().replace("\n", " | ")[:200]
    print(f"{r['file']}: {out}")

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

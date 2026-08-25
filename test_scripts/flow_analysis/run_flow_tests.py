#!/usr/bin/env python3
"""Flow-analysis tests for typed exception propagation.

These tests verify that FlowAnalyzer reports per-type throw warnings and that
catch clauses filter them correctly. Warnings are checked by inspecting stderr.
"""

import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TEST_DIR = Path(__file__).parent

_CANDIDATES = [
    ROOT / "out" / "build" / "x64-debug" / "bin" / "shard.exe",
    ROOT / "out" / "build" / "mingw-debug" / "bin" / "shard.exe",
    ROOT / "cmake-build-debug" / "bin" / "shard.exe",
]

INTERPRETER = None
for candidate in _CANDIDATES:
    if candidate.exists():
        INTERPRETER = candidate
        break

if INTERPRETER is None:
    print("ERROR: Could not find ShardScript.Interpreter binary.", file=sys.stderr)
    sys.exit(2)


def _run(file_name: str, expected_returncode: int = 0):
    source = TEST_DIR / file_name
    cmd = [str(INTERPRETER), str(source)]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
    return proc


def _has_warning(output: str, warning: str) -> bool:
    return warning in output


TEST_CASES = [
    {
        "file": "typed_throw_propagates.shard",
        "description": "uncaught RuntimeException propagates to caller",
        "expected_warning": "Call to 'Thrower' may throw 'RuntimeException'",
    },
    {
        "file": "typed_throw_catch_exact.shard",
        "description": "catch by exact type suppresses the warning",
        "expected_warning": None,
    },
    {
        "file": "typed_throw_catch_base.shard",
        "description": "catch by base IThrowable suppresses the warning",
        "expected_warning": None,
    },
    {
        "file": "typed_throw_nested_catch.shard",
        "description": "nested try/catch suppresses the warning",
        "expected_warning": None,
    },
]


def main():
    passed = 0
    failed = 0

    for case in TEST_CASES:
        proc = _run(case["file"])
        combined = proc.stdout + proc.stderr

        ok = proc.returncode == 0
        if case["expected_warning"] is not None:
            ok = ok and _has_warning(combined, case["expected_warning"])
        else:
            ok = ok and not _has_warning(combined, "may throw")

        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {case['file']}: {case['description']}")

        if not ok:
            failed += 1
            print(f"  exit code: {proc.returncode}")
            if proc.stdout:
                print(f"  stdout:\n{proc.stdout}")
            if proc.stderr:
                print(f"  stderr:\n{proc.stderr}")
        else:
            passed += 1

    print(f"\nTotal: {passed + failed}, Passed: {passed}, Failed: {failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

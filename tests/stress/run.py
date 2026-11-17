#!/usr/bin/env python3
"""Parser stress suite: feeds pathological inputs to the compiler and checks that
nothing crashes, nothing exceeds its time budget, and run time stays roughly
linear in input size (a quadratic parser shows up as a growing ratio).

Usage: run.py [--compiler PATH] [--size N] [--budget SECONDS]

The compiler is only asked to parse and analyse (--lint --no-std), so every
result is expected to be a diagnostic list, never a successful run.
"""

import argparse
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import generate

# shape -> base size; the suite triples it to look for superlinear growth
SUITE = [
    ("junk_compilation_unit", 200),
    ("junk_statement_block", 200),
    ("junk_struct_body", 200),
    ("junk_no_anchor", 200),
    ("error_lines", 200),
    ("unclosed_bodies", 200),
    ("unclosed_calls", 200),
    ("stray_closers", 200),
    ("nested_blocks", 200),
    ("nested_structs", 200),
    ("nested_calls", 200),
    ("nested_generics", 200),
    ("generic_closers", 200),
    ("long_expression", 500),
    ("long_identifier_chain", 500),
    ("long_type_suffixes", 500),
    ("long_qualified_type", 500),
    ("many_members", 200),
    ("many_statements", 200),
    ("keywords_only", 200),
]

ACCEPTED_EXIT_CODES = {0, 1, 2, 3}


def find_compiler() -> str:
    candidates = [
        os.environ.get("SHARD_BIN"),
        "out/build/collections-check/bin/shard.exe",
        "out/build/x64-clang-debug-optimized/bin/shard.exe",
        "out/build/x64-clang-release-optimized/bin/shard.exe",
        "build/bin/shard.exe",
        "build/bin/shard",
    ]
    # look next to the current directory first, then from the repository root
    roots = [Path.cwd(), Path(__file__).resolve().parent.parent.parent]
    for candidate in candidates:
        if not candidate:
            continue
        for root in roots:
            resolved = (root / candidate) if not os.path.isabs(candidate) else Path(candidate)
            if resolved.exists():
                return str(resolved)
    sys.stderr.write("no compiler found; pass --compiler or set SHARD_BIN\n")
    sys.exit(2)


def run_case(compiler: str, source: str, budget: float):
    with tempfile.NamedTemporaryFile("w", suffix=".shard", delete=False, encoding="utf-8") as handle:
        handle.write(source)
        path = handle.name

    try:
        start = time.monotonic()
        try:
            result = subprocess.run(
                [compiler, "--lint", "--no-std", path],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=budget)
            elapsed = time.monotonic() - start
            return elapsed, result.returncode, result.stdout.decode("utf-8", "replace")
        except subprocess.TimeoutExpired:
            return budget, None, ""
    finally:
        os.unlink(path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler")
    parser.add_argument("--size", type=int, default=None,
                        help="override the base size of every shape")
    parser.add_argument("--budget", type=float, default=60.0,
                        help="seconds allowed for one case")
    parser.add_argument("--max-growth", type=float, default=6.0,
                        help="allowed time ratio when the input triples")
    args = parser.parse_args()

    compiler = args.compiler or find_compiler()
    failures = []

    print("compiler: %s" % compiler)
    if args.size:
        suite = [(name, args.size) for name, _ in SUITE]
    else:
        suite = SUITE

    for name, base in suite:
        timings = []
        for size in (base, base * 3, base * 9):
            source = generate.SHAPES[name](size)
            elapsed, code, output = run_case(compiler, source, args.budget)

            if code is None:
                failures.append("%s size=%d exceeded %.0fs" % (name, size, args.budget))
                print("%-24s size=%-6d TIMEOUT > %.0fs" % (name, size, args.budget))
                break

            if code not in ACCEPTED_EXIT_CODES:
                failures.append("%s size=%d crashed with exit %d" % (name, size, code))
                print("%-24s size=%-6d CRASH rc=%d" % (name, size, code))
                print(output[:400])
                break

            timings.append(elapsed)
            print("%-24s size=%-6d %6.2fs rc=%d lines=%d"
                  % (name, size, elapsed, code, source.count("\n")))

        if len(timings) == 3 and timings[0] > 0.2:
            growth = timings[2] / max(timings[1], 1e-6)
            if growth > args.max_growth:
                failures.append("%s grows superlinearly: %.2fs -> %.2fs (x%.1f)"
                                % (name, timings[1], timings[2], growth))
                print("%-24s SUPERLINEAR x%.1f" % (name, growth))

    print()
    if failures:
        print("FAILURES (%d):" % len(failures))
        for failure in failures:
            print("  " + failure)
        return 1

    print("all shapes within budget and linear")
    return 0


if __name__ == "__main__":
    sys.exit(main())
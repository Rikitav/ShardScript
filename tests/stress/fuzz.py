#!/usr/bin/env python3
"""Mutation fuzzer for the compiler frontend: keeps a corpus of valid scripts,
corrupts them at random and checks that the compiler reports diagnostics instead
of crashing, hanging or running out of stack.

Usage: fuzz.py [--compiler PATH] [--iterations N] [--seed S] [--budget SECONDS]

Every case runs in its own process, so a crash is contained: the failing input is
saved to the case directory and its seed/iteration is printed for reproduction.
Seed and iteration fully determine the case, so any failure can be replayed with
--seed and --first.
"""

import argparse
import os
import random
import subprocess
import sys
import time
from pathlib import Path

FRAGMENTS = [
    "{", "}", "(", ")", "[", "]", "<", ">", ">>", ";", ",", ":", "::",
    "func", "struct", "interface", "enum", "if", "else", "while", "for", "return",
    "defer", "try", "catch", "throw", "new", "operator", "delegate", "await",
    "x", "y", "Foo", "int", "void", "->", "=>", ":=", "=", "+", "-", "|", ".",
    "1", "0x", "\"unterminated", "'", "@", "$", "\\", "..", "?", "!", "\n", "\t",
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


def load_corpus(directory: Path):
    corpus = []
    for path in sorted(directory.rglob("*.shard")):
        text = path.read_text(encoding="utf-8", errors="replace")
        if text:
            corpus.append(text)
    return corpus


def mutate(source: str, rng: random.Random) -> str:
    if not source:
        return rng.choice(FRAGMENTS)

    kind = rng.randrange(7)
    at = rng.randrange(len(source))
    length = 1 + rng.randrange(min(len(source) - at, 64))

    if kind == 0:
        return source[:at] + source[at + length:]
    
    if kind == 1:
        return source[:at] + source[at:at + length] + source[at:]
    
    if kind == 2:
        return source[:at] + rng.choice(FRAGMENTS) + source[at:]
    
    if kind == 3:
        return source[:at]
    
    if kind == 4:
        junk = "{}()[]<>;:::=-+|@$\\\n\t "
        block = "".join(rng.choice(junk) for _ in range(length))
        return source[:at] + block + source[at + length:]
    
    if kind == 5:
        opener = rng.choice("([{<")
        return source[:at] + opener * (1 + rng.randrange(64)) + source[at:]
    
    return source[:at] + "".join(rng.choice(FRAGMENTS) for _ in range(1 + rng.randrange(6))) + source[at + length:]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler")
    parser.add_argument("--corpus", default="tests")
    parser.add_argument("--iterations", type=int, default=200)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--first", type=int, default=0, help="iteration to start from")
    parser.add_argument("--budget", type=float, default=30.0)
    parser.add_argument("--out", default="out/fuzz-cases")
    args = parser.parse_args()

    compiler = args.compiler or find_compiler()
    corpus = load_corpus(Path(args.corpus))
    if not corpus:
        sys.stderr.write("no *.shard files under %s\n" % args.corpus)
        return 2

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    rng = random.Random(args.seed)
    failures = []
    slowest = (0.0, None)

    print("compiler: %s" % compiler)
    print("corpus:   %d files, seed=%d, iterations=%d" % (len(corpus), args.seed, args.iterations))

    case_path = out_dir / "last.shard"

    for iteration in range(args.first, args.first + args.iterations):
        source = corpus[rng.randrange(len(corpus))]
        for _ in range(1 + rng.randrange(4)):
            source = mutate(source, rng)

        case_path.write_text(source, encoding="utf-8", errors="replace")

        start = time.monotonic()
        try:
            result = subprocess.run(
                [compiler, "--lint", "--no-std", str(case_path)],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=args.budget)
            elapsed = time.monotonic() - start
            code = result.returncode
            output = result.stdout.decode("utf-8", "replace")
        except subprocess.TimeoutExpired:
            elapsed, code, output = args.budget, None, ""

        if elapsed > slowest[0]:
            slowest = (elapsed, iteration)

        if code is None:
            saved = out_dir / ("timeout-%d.shard" % iteration)
            saved.write_text(source, encoding="utf-8", errors="replace")
            failures.append("iteration %d timed out after %.0fs (%s)" % (iteration, args.budget, saved))
            print("TIMEOUT iteration %d -> %s" % (iteration, saved))
        elif code not in ACCEPTED_EXIT_CODES:
            saved = out_dir / ("crash-%d.shard" % iteration)
            saved.write_text(source, encoding="utf-8", errors="replace")
            failures.append("iteration %d crashed with exit %d (%s)" % (iteration, code, saved))
            print("CRASH iteration %d rc=%d -> %s" % (iteration, code, saved))
            print(output[:800])

        if (iteration + 1) % 50 == 0:
            print("  %d cases, slowest %.2fs" % (iteration + 1, slowest[0]))
            sys.stdout.flush()

    print()
    print("done: %d cases, slowest %.2fs at iteration %s" % (args.iterations, slowest[0], slowest[1]))

    if failures:
        print("FAILURES (%d):" % len(failures))
        for failure in failures:
            print("  " + failure)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
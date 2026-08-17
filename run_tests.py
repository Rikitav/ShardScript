#!/usr/bin/env python3
"""ShardScript test runner.

Runs every .shard file under test_scripts/ and reports PASS/FAIL.
By default a test is expected to succeed (exit 0). Tests inside
parser_stress/ are expected to fail compilation. Additional expected
failures can be listed in test_config.json next to this script.
"""

import argparse
import json
import os
import platform
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    GREEN = "\033[32m"
    RED = "\033[31m"
    YELLOW = "\033[33m"
    CYAN = "\033[36m"
    MAGENTA = "\033[35m"
    GRAY = "\033[90m"

    @staticmethod
    def enabled():
        return sys.stdout.isatty() and os.environ.get("NO_COLOR", "").strip() == ""


def colorize(text, color):
    if Colors.enabled():
        return f"{color}{text}{Colors.RESET}"
    return text

ROOT = Path(__file__).resolve().parent
TEST_DIR = ROOT / "test_scripts"
CONFIG_PATH = ROOT / "test_config.json"

DEFAULT_EXPECTED_FAILURES = {
    # Async negative tests: these files contain invalid async/await usage
    # and are expected to be rejected by the compiler.
    "test_scripts/async/test_async_lambda_error.shard",
    "test_scripts/async/test_await_non_awaitable.shard",
    "test_scripts/async/test_await_outside_async.shard",
}

DEFAULT_EXPECTED_TIMEOUTS = {
    # Server tests that intentionally block forever waiting for connections.
    "test_scripts/stdlib/http/httpserver_test.shard",
    "test_scripts/stdlib/socket/socket_server_test.shard",
    "test_scripts/test_http_server_async.shard",
}


def load_config():
    if not CONFIG_PATH.exists():
        return {"expected_failures": [], "expected_timeouts": [], "test_arguments": [], "skip_on_os": {}}
    with CONFIG_PATH.open("r", encoding="utf-8") as f:
        return json.load(f)


def is_expected_failure(rel_path, expected_set):
    return rel_path.as_posix() in expected_set


def is_expected_timeout(rel_path, expected_set):
    return rel_path.as_posix() in expected_set


def get_test_arguments(rel_path, arguments_map):
    return arguments_map.get(rel_path.as_posix(), [])


def get_skipped_tests(skip_on_os):
    """Return the set of tests that should be skipped on the current OS."""
    current = platform.system().lower()
    skipped = set()
    if isinstance(skip_on_os, dict):
        for os_name, tests in skip_on_os.items():
            if os_name.lower() == current:
                for path in tests:
                    skipped.add(path.replace("\\", "/"))
    return skipped


def discover_tests():
    tests = []
    for path in sorted(TEST_DIR.rglob("*.shard")):
        rel = path.relative_to(ROOT)
        tests.append(rel)
    return tests


def run_test(test_path, interpreter, expected_failures, expected_timeouts, arguments_map, verbose, timeout=30.0):
    extra_args = get_test_arguments(test_path, arguments_map)
    cmd = [str(interpreter)] + extra_args + [str(ROOT / test_path)]
    start = time.perf_counter()

    try:
        # Run from the interpreter's directory so it can find its DLLs.
        proc = subprocess.run(
            cmd,
            cwd=str(interpreter.parent),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
        )
        returncode = proc.returncode
        stdout = proc.stdout
        stderr = proc.stderr
    except subprocess.TimeoutExpired as e:
        returncode = -1
        stdout = e.stdout or ""
        stderr = e.stderr or ""

    elapsed = time.perf_counter() - start

    in_parser_stress = "parser_stress" in test_path.parts
    expected_fail = is_expected_failure(test_path, expected_failures) or in_parser_stress
    expected_timeout = is_expected_timeout(test_path, expected_timeouts)
    actually_passed = returncode == 0

    if returncode == -1:
        status = "PASS" if expected_timeout else "TIMEOUT"
    elif expected_fail:
        status = "PASS" if not actually_passed else "UNEXPECTED_PASS"
    elif expected_timeout:
        status = "UNEXPECTED_PASS" if actually_passed else "TIMEOUT"
    else:
        status = "PASS" if actually_passed else "FAIL"

    result = {
        "path": test_path,
        "status": status,
        "returncode": returncode,
        "elapsed": elapsed,
        "stdout": stdout,
        "stderr": stderr,
    }

    if verbose and status in ("FAIL", "UNEXPECTED_PASS", "TIMEOUT"):
        print(f"\n--- {test_path} ---")
        print(f"command: {' '.join(cmd)}")
        print(f"exit code: {returncode}")
        if stdout:
            print("stdout:")
            print(stdout)
        if stderr:
            print("stderr:")
            print(stderr)
        print("-" * 40)

    return result


def main():
    parser = argparse.ArgumentParser(description="Run ShardScript test suite")
    parser.add_argument(
        "-b", "--build-dir",
        type=Path,
        default=ROOT / "out" / "build" / "mingw-debug",
        help="Build directory containing bin/shard.exe (default: out/build/mingw-debug)",
    )
    parser.add_argument(
        "-e", "--expected-failures",
        type=Path,
        default=None,
        help="JSON config with expected_failures list (default: test_config.json)",
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=1,
        help="Number of parallel workers (default: 1). Values >1 are unsafe for this suite because several tests use sockets, files, and subprocesses that conflict when run concurrently.",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print output of failing tests",
    )
    parser.add_argument(
        "-f", "--filter",
        default=None,
        help="Only run tests whose path contains this substring",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List discovered tests and exit",
    )
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable colored output",
    )
    args = parser.parse_args()

    if args.no_color:
        os.environ["NO_COLOR"] = "1"

    interpreter = args.build_dir / "bin" / "shard.exe"
    if not interpreter.exists():
        interpreter = args.build_dir / "bin" / "shard"
    if not interpreter.exists():
        print(f"Interpreter not found: {args.build_dir / 'bin' / 'shard.exe'} or {interpreter}", file=sys.stderr)
        return 1

    config_path = args.expected_failures or CONFIG_PATH
    if config_path.exists():
        with config_path.open("r", encoding="utf-8") as f:
            config = json.load(f)
    else:
        config = {"expected_failures": [], "expected_timeouts": [], "test_arguments": [], "skip_on_os": {}}

    expected_failures = set(DEFAULT_EXPECTED_FAILURES)
    for entry in config.get("expected_failures", []):
        expected_failures.add(entry.replace("\\", "/"))

    expected_timeouts = set(DEFAULT_EXPECTED_TIMEOUTS)
    for entry in config.get("expected_timeouts", []):
        expected_timeouts.add(entry.replace("\\", "/"))

    allowed_unexpected_passes = set()
    for entry in config.get("allowed_unexpected_passes", []):
        allowed_unexpected_passes.add(entry.replace("\\", "/"))

    arguments_map = {}
    for entry in config.get("test_arguments", []):
        path = entry.get("path", "").replace("\\", "/")
        test_args = entry.get("args", [])
        if path:
            arguments_map[path] = test_args

    skipped_tests = get_skipped_tests(config.get("skip_on_os", {}))

    tests = discover_tests()
    skipped_count = 0
    if skipped_tests:
        filtered = []
        for t in tests:
            if t.as_posix() in skipped_tests:
                skipped_count += 1
            else:
                filtered.append(t)
        tests = filtered

    if args.filter:
        tests = [t for t in tests if args.filter in str(t)]

    if args.list:
        for t in tests:
            markers = []
            if is_expected_failure(t, expected_failures) or "parser_stress" in t.parts:
                markers.append("expected fail")
            if is_expected_timeout(t, expected_timeouts):
                markers.append("expected timeout")
            if get_test_arguments(t, arguments_map):
                markers.append("has args")
            marker = f" [{', '.join(markers)}]" if markers else ""
            print(f"{t}{marker}")
        return 0

    jobs = args.jobs or os.cpu_count() or 1
    results = []
    total = len(tests)
    start = time.perf_counter()

    print(f"Running {total} tests from {TEST_DIR} using {interpreter}")
    if skipped_count:
        print(f"Skipped: {skipped_count}")
    print(f"Parallel workers: {jobs}")
    print()

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = {
            executor.submit(run_test, t, interpreter, expected_failures, expected_timeouts, arguments_map, args.verbose, 30.0): t
            for t in tests
        }
        for future in as_completed(futures):
            results.append(future.result())

    elapsed = time.perf_counter() - start

    # Group by directory
    by_dir = {}
    for r in results:
        directory = r["path"].parts[0] if r["path"].parts else "<root>"
        by_dir.setdefault(directory, []).append(r)

    unexpected_failures = []
    unexpected_passes = []

    status_colors = {
        "PASS": Colors.GREEN,
        "FAIL": Colors.RED,
        "TIMEOUT": Colors.MAGENTA,
        "UNEXPECTED_PASS": Colors.YELLOW,
    }

    for directory in sorted(by_dir):
        print(f"=== {directory} ===")
        for r in sorted(by_dir[directory], key=lambda x: x["path"]):
            path = r["path"]
            status = r["status"]
            rc = r["returncode"]
            ms = r["elapsed"] * 1000
            colored_status = colorize(status, status_colors.get(status, Colors.RESET))
            if status == "FAIL":
                unexpected_failures.append(r)
                print(f"  {path}: {colored_status} (exit={rc}, {ms:.1f}ms)")
            elif status == "TIMEOUT":
                unexpected_failures.append(r)
                print(f"  {path}: {colored_status} ({ms:.1f}ms)")
            elif status == "UNEXPECTED_PASS":
                unexpected_passes.append(r)
                print(f"  {path}: {colored_status} (exit={rc}, {ms:.1f}ms)")
            else:
                print(f"  {path}: {colored_status} ({ms:.1f}ms)")

    total_pass = sum(1 for r in results if r["status"] == "PASS")
    total_unexpected_fail = len(unexpected_failures)
    total_unexpected_pass = len(unexpected_passes)

    print()
    print("=" * 40)
    print(f"Total:   {total + skipped_count}")
    print(f"Passed:  {colorize(total_pass, Colors.GREEN)}")
    print(f"Failed:  {colorize(total_unexpected_fail, Colors.RED)}")
    print(f"Unexpected passes: {colorize(total_unexpected_pass, Colors.YELLOW)}")
    print(f"Skipped: {skipped_count}")
    print(f"Time:    {elapsed:.2f}s")

    if unexpected_failures or unexpected_passes:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

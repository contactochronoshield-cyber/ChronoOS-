#!/usr/bin/env python3

import hashlib
import importlib.util
import os
import time
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
CORE_DIR = TEST_DIR.parent / "core"
MODULE_FILE = CORE_DIR / "maya_hash.py"

if not MODULE_FILE.is_file():
    raise SystemExit("ERROR: maya_hash.py not found")

spec = importlib.util.spec_from_file_location(
    "maya_hash",
    MODULE_FILE
)

if spec is None or spec.loader is None:
    raise SystemExit("ERROR: unable to load maya_hash.py")

module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

maya_hash = module.maya_hash


ALGORITHMS = {
    "MAYA": maya_hash,
    "SHA-256": lambda data: hashlib.sha256(data).digest(),
    "SHA-512": lambda data: hashlib.sha512(data).digest(),
    "SHA3-256": lambda data: hashlib.sha3_256(data).digest(),
    "SHA3-512": lambda data: hashlib.sha3_512(data).digest(),
}


SIZES = [
    64,
    1024,
    4096,
    65536,
    1048576,
]


ROUNDS = 3


def benchmark(function, data):

    start = time.perf_counter()

    result = None

    for _ in range(ROUNDS):
        result = function(data)

    elapsed = time.perf_counter() - start

    return elapsed, result


def main():

    print("======================================")
    print("       CHRONO MAYA BENCHMARK")
    print("======================================")
    print()
    print("Platform:", os.uname().machine)
    print("Rounds per measurement:", ROUNDS)
    print()

    for size in SIZES:

        data = os.urandom(size)

        print("--------------------------------------")
        print("INPUT SIZE:", size, "bytes")
        print("--------------------------------------")

        for name, function in ALGORITHMS.items():

            elapsed, result = benchmark(
                function,
                data
            )

            total_bytes = size * ROUNDS

            mb_per_second = (
                total_bytes /
                elapsed /
                (1024 * 1024)
            )

            hashes_per_second = (
                ROUNDS /
                elapsed
            )

            print(
                f"{name:10s} | "
                f"{elapsed:.6f}s | "
                f"{mb_per_second:10.3f} MB/s | "
                f"{hashes_per_second:10.2f} H/s | "
                f"{len(result):3d} bytes"
            )

        print()

    print("======================================")
    print("RESULT")
    print("======================================")
    print()
    print("Status: PERFORMANCE DATA COLLECTED")
    print()
    print("IMPORTANT:")
    print("Performance does NOT establish cryptographic security.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

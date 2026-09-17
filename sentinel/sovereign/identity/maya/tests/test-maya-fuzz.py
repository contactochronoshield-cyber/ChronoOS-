#!/usr/bin/env python3

import importlib.util
import random
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


SAMPLES = 10000
EXPECTED_LENGTH = 64


def build_inputs():

    rng = random.Random(20260917)

    yield b""
    yield b"\x00"
    yield b"\xff"
    yield b"\x00" * 1024
    yield b"\xff" * 1024
    yield b"\xaa" * 4096
    yield b"\x55" * 4096
    yield b"chrono"
    yield b"ChronoOS"
    yield "Chrono Maya".encode("utf-8")
    yield "Maya-∆-测试-🔐".encode("utf-8")

    for size in [1, 2, 3, 7, 8, 15, 16, 31, 32, 63, 64,
                 127, 128, 255, 256, 511, 512, 1024, 4096]:

        yield rng.randbytes(size)

    for _ in range(SAMPLES):

        size = rng.randrange(0, 8193)

        yield rng.randbytes(size)


def main():

    total = 0
    failures = 0
    wrong_length = 0
    exceptions = 0
    nondeterministic = 0

    print("======================================")
    print("          CHRONO MAYA FUZZ LAB")
    print("======================================")
    print()
    print("Random samples:", SAMPLES)
    print("Maximum random input:", "8192 bytes")
    print("Expected hash:", EXPECTED_LENGTH, "bytes")
    print()

    for index, data in enumerate(build_inputs()):

        total += 1

        try:

            result_a = maya_hash(data)
            result_b = maya_hash(data)

        except Exception as exc:

            exceptions += 1
            failures += 1

            print(
                "EXCEPTION:",
                index,
                type(exc).__name__,
                str(exc)
            )

            continue

        if len(result_a) != EXPECTED_LENGTH:

            wrong_length += 1
            failures += 1

            print(
                "WRONG LENGTH:",
                index,
                len(result_a)
            )

        if result_a != result_b:

            nondeterministic += 1
            failures += 1

            print(
                "NONDETERMINISTIC:",
                index
            )

    print()
    print("===== FUZZ SUMMARY =====")
    print("Total inputs:", total)
    print("Exceptions:", exceptions)
    print("Wrong output length:", wrong_length)
    print("Nondeterministic results:", nondeterministic)
    print("Total failures:", failures)
    print()

    print("===== RESULT =====")

    if failures == 0:

        print("Status: FUZZ STRUCTURAL PASS")

    else:

        print("Status: FUZZ FAIL")

    print()
    print("IMPORTANT:")
    print("This test evaluates implementation robustness.")
    print("It does NOT establish cryptographic security.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

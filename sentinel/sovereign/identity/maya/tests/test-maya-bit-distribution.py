#!/usr/bin/env python3

import importlib.util
import random
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
CORE_DIR = TEST_DIR.parent / "core"
MODULE_FILE = CORE_DIR / "maya_hash.py"

if not MODULE_FILE.is_file():
    raise SystemExit("ERROR: maya_hash.py not found")

spec = importlib.util.spec_from_file_location("maya_hash", MODULE_FILE)

if spec is None or spec.loader is None:
    raise SystemExit("ERROR: unable to load maya_hash.py")

module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

maya_hash = module.maya_hash


SAMPLES = 10000
HASH_BITS = 512


def main():

    random.seed(20260917)

    total_ones = 0
    total_bits = SAMPLES * HASH_BITS

    bit_ones = [0] * HASH_BITS

    print("======================================")
    print("     CHRONO MAYA BIT DISTRIBUTION")
    print("======================================")
    print()
    print("Samples:", SAMPLES)
    print("Hash size:", HASH_BITS, "bits")
    print()

    for sample_id in range(SAMPLES):

        data = (
            b"CHRONO-MAYA-DISTRIBUTION/"
            + sample_id.to_bytes(4, "big")
            + random.randbytes(64)
        )

        digest = maya_hash(data)

        for byte_index, value in enumerate(digest):

            total_ones += value.bit_count()

            for bit_index in range(8):

                if value & (1 << (7 - bit_index)):
                    position = byte_index * 8 + bit_index
                    bit_ones[position] += 1

    total_zeros = total_bits - total_ones

    one_ratio = total_ones / total_bits
    zero_ratio = total_zeros / total_bits

    print("===== GLOBAL DISTRIBUTION =====")
    print("Total bits:", total_bits)
    print("Ones:", total_ones)
    print("Zeros:", total_zeros)
    print("Ones ratio:", f"{one_ratio:.6f}")
    print("Zeros ratio:", f"{zero_ratio:.6f}")
    print("Ideal ratio:", "0.500000")
    print()

    print("===== PER-BIT DISTRIBUTION =====")

    minimum = min(bit_ones)
    maximum = max(bit_ones)

    print("Minimum ones:", minimum)
    print("Maximum ones:", maximum)
    print()

    low_positions = 0
    high_positions = 0

    for count in bit_ones:

        ratio = count / SAMPLES

        if ratio < 0.45:
            low_positions += 1

        if ratio > 0.55:
            high_positions += 1

    print("Positions <45% ones:", low_positions)
    print("Positions >55% ones:", high_positions)
    print()

    print("===== RESULT =====")

    global_deviation = abs(one_ratio - 0.5)

    print(
        "Global deviation:",
        f"{global_deviation:.6f}"
    )

    if global_deviation <= 0.01:
        global_status = "PASS"
    else:
        global_status = "REVIEW"

    if low_positions == 0 and high_positions == 0:
        position_status = "PASS"
    else:
        position_status = "REVIEW"

    print("Global distribution:", global_status)
    print("Per-bit distribution:", position_status)

    if global_status == "PASS" and position_status == "PASS":
        print()
        print("Status: STRUCTURAL RESEARCH PASS")
    else:
        print()
        print("Status: RESEARCH REVIEW REQUIRED")

    print()
    print("IMPORTANT:")
    print("This test does NOT establish cryptographic security.")
    print("It evaluates statistical bit distribution only.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

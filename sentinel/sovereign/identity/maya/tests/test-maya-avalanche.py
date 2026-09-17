#!/usr/bin/env python3

import importlib.util
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


def hamming_distance(a: bytes, b: bytes) -> int:
    return sum(
        (x ^ y).bit_count()
        for x, y in zip(a, b)
    )


def flip_bit(data: bytes, bit_index: int) -> bytes:
    result = bytearray(data)

    byte_index = bit_index // 8
    bit_position = bit_index % 8

    result[byte_index] ^= 1 << bit_position

    return bytes(result)


def main():

    original = b"chrono-maya-avalanche-test"

    baseline = maya_hash(original)

    total_bits = len(original) * 8
    distances = []

    print("======================================")
    print("       CHRONO MAYA AVALANCHE TEST")
    print("======================================")
    print()
    print("Input bytes:", len(original))
    print("Input bits:", total_bits)
    print("Hash bits:", len(baseline) * 8)
    print()

    for bit in range(total_bits):

        modified = flip_bit(original, bit)
        modified_hash = maya_hash(modified)

        distance = hamming_distance(
            baseline,
            modified_hash
        )

        distances.append(distance)

    minimum = min(distances)
    maximum = max(distances)
    average = sum(distances) / len(distances)

    expected = len(baseline) * 8 / 2

    print("Minimum changed bits:", minimum)
    print("Maximum changed bits:", maximum)
    print("Average changed bits:", f"{average:.2f}")
    print("Expected ideal average:", f"{expected:.2f}")
    print()

    print("RESULTS")

    print("Minimum deviation:",
          f"{minimum - expected:+.2f}")

    print("Maximum deviation:",
          f"{maximum - expected:+.2f}")

    print("Average deviation:",
          f"{average - expected:+.2f}")

    print()

    if 0 < minimum and maximum <= len(baseline) * 8:
        print("Status: STRUCTURAL PASS")
    else:
        print("Status: STRUCTURAL FAILURE")

    print()
    print("IMPORTANT:")
    print("This test measures avalanche behavior only.")
    print("It does not establish cryptographic security.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

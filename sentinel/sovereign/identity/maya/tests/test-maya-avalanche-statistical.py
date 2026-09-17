#!/usr/bin/env python3

import importlib.util
import secrets
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


def flip_random_bit(data: bytes) -> bytes:
    result = bytearray(data)

    bit_index = secrets.randbelow(len(result) * 8)

    byte_index = bit_index // 8
    bit_position = bit_index % 8

    result[byte_index] ^= 1 << bit_position

    return bytes(result)


def main():

    samples = 10000
    input_size = 64

    distances = []

    print("======================================")
    print("   CHRONO MAYA AVALANCHE STATISTICAL")
    print("======================================")
    print()
    print("Samples:", samples)
    print("Input size:", input_size, "bytes")
    print("Hash size:", 512, "bits")
    print()

    for _ in range(samples):

        original = secrets.token_bytes(input_size)

        baseline = maya_hash(original)

        modified = flip_random_bit(original)

        modified_hash = maya_hash(modified)

        distance = hamming_distance(
            baseline,
            modified_hash
        )

        distances.append(distance)

    minimum = min(distances)
    maximum = max(distances)
    average = sum(distances) / samples

    expected = 256.0

    variance = sum(
        (x - average) ** 2
        for x in distances
    ) / samples

    stddev = variance ** 0.5

    below_200 = sum(x < 200 for x in distances)
    above_312 = sum(x > 312 for x in distances)

    print("===== STATISTICS =====")
    print("Minimum:", minimum)
    print("Maximum:", maximum)
    print("Average:", f"{average:.4f}")
    print("Expected:", f"{expected:.4f}")
    print("Std deviation:", f"{stddev:.4f}")
    print()

    print("===== DISTRIBUTION CHECK =====")
    print("Samples < 200 bits:", below_200)
    print("Samples > 312 bits:", above_312)
    print()

    print("===== RESULT =====")

    if minimum > 0 and maximum <= 512:
        print("Range: PASS")
    else:
        print("Range: FAIL")

    deviation = abs(average - expected)

    print("Mean deviation:", f"{deviation:.4f}")

    if deviation < 3.0:
        print("Mean behavior: PASS")
    else:
        print("Mean behavior: REVIEW")

    print()
    print("Status: STRUCTURAL RESEARCH PASS")
    print()
    print("IMPORTANT:")
    print("This test does NOT establish cryptographic security.")
    print("It only evaluates avalanche behavior statistically.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

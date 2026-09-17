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
HASH_BITS = 512
INPUT_SIZE = 64


def xor_bytes(a, b):
    return bytes(x ^ y for x, y in zip(a, b))


def hamming_distance(a, b):
    return sum(
        (x ^ y).bit_count()
        for x, y in zip(a, b)
    )


def flip_random_bit(data, rng):
    result = bytearray(data)

    bit = rng.randrange(len(result) * 8)

    byte_index = bit // 8
    bit_index = bit % 8

    result[byte_index] ^= 1 << bit_index

    return bytes(result), bit


def main():

    rng = random.Random(20260917)

    distances = []

    output_bit_ones = [0] * HASH_BITS

    # Track whether each output bit tends to flip
    # when an input bit is flipped.
    transition_counts = [0] * HASH_BITS

    print("======================================")
    print("      CHRONO MAYA DIFFERENTIAL")
    print("======================================")
    print()
    print("Samples:", SAMPLES)
    print("Input size:", INPUT_SIZE, "bytes")
    print("Hash size:", HASH_BITS, "bits")
    print()

    for sample in range(SAMPLES):

        original = rng.randbytes(INPUT_SIZE)

        modified, input_bit = flip_random_bit(
            original,
            rng
        )

        hash_original = maya_hash(original)
        hash_modified = maya_hash(modified)

        diff = xor_bytes(
            hash_original,
            hash_modified
        )

        distance = hamming_distance(
            hash_original,
            hash_modified
        )

        distances.append(distance)

        for byte_index, value in enumerate(diff):

            for bit_index in range(8):

                if value & (1 << (7 - bit_index)):

                    position = byte_index * 8 + bit_index
                    transition_counts[position] += 1

        for byte_index, value in enumerate(hash_original):

            for bit_index in range(8):

                if value & (1 << (7 - bit_index)):

                    position = byte_index * 8 + bit_index
                    output_bit_ones[position] += 1

    minimum = min(distances)
    maximum = max(distances)

    average = sum(distances) / len(distances)

    expected = HASH_BITS / 2

    variance = sum(
        (x - average) ** 2
        for x in distances
    ) / len(distances)

    stddev = variance ** 0.5

    print("===== HAMMING DISTANCE =====")
    print("Minimum:", minimum)
    print("Maximum:", maximum)
    print("Average:", f"{average:.4f}")
    print("Expected:", f"{expected:.4f}")
    print("Std deviation:", f"{stddev:.4f}")
    print()

    print("===== DIFFERENTIAL OUTPUT =====")

    minimum_transition = min(transition_counts)
    maximum_transition = max(transition_counts)

    print(
        "Minimum flip frequency:",
        f"{minimum_transition / SAMPLES:.6f}"
    )

    print(
        "Maximum flip frequency:",
        f"{maximum_transition / SAMPLES:.6f}"
    )

    print()

    low = 0
    high = 0

    for count in transition_counts:

        ratio = count / SAMPLES

        if ratio < 0.40:
            low += 1

        if ratio > 0.60:
            high += 1

    print("Output positions <40% flips:", low)
    print("Output positions >60% flips:", high)
    print()

    print("===== OUTPUT BIT BASELINE =====")

    minimum_ones = min(output_bit_ones)
    maximum_ones = max(output_bit_ones)

    print(
        "Minimum ones:",
        minimum_ones
    )

    print(
        "Maximum ones:",
        maximum_ones
    )

    print()

    print("===== RESULT =====")

    mean_deviation = abs(average - expected)

    print(
        "Mean deviation:",
        f"{mean_deviation:.4f}"
    )

    if mean_deviation <= 5:
        distance_status = "PASS"
    else:
        distance_status = "REVIEW"

    if low == 0 and high == 0:
        differential_status = "PASS"
    else:
        differential_status = "REVIEW"

    print(
        "Hamming behavior:",
        distance_status
    )

    print(
        "Differential distribution:",
        differential_status
    )

    if distance_status == "PASS" and differential_status == "PASS":
        print()
        print("Status: STRUCTURAL RESEARCH PASS")
    else:
        print()
        print("Status: RESEARCH REVIEW REQUIRED")

    print()
    print("IMPORTANT:")
    print("This test does NOT establish cryptographic security.")
    print("It evaluates differential behavior statistically.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import importlib.util
import math
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


CONFIG = {
    16: 1000,
    20: 2000,
    24: 5000,
    32: 100000,
}


def truncated_hash(data, bits):
    digest = maya_hash(data)

    value = int.from_bytes(digest, "big")

    return value >> (len(digest) * 8 - bits)


def birthday_probability(n, bits):
    space = 2 ** bits

    if n <= 1:
        return 0.0

    exponent = -(n * (n - 1)) / (2 * space)

    return 1.0 - math.exp(exponent)


def run_collision_test(bits, samples, rng):

    seen = {}

    collisions = 0
    first_collision = None

    for i in range(samples):

        data = (
            b"CHRONO-MAYA-COLLISION/"
            + bits.to_bytes(2, "big")
            + i.to_bytes(4, "big")
            + rng.randbytes(32)
        )

        value = truncated_hash(data, bits)

        if value in seen:

            collisions += 1

            if first_collision is None:
                first_collision = (
                    seen[value],
                    i,
                    value
                )

        else:
            seen[value] = i

    probability = birthday_probability(
        samples,
        bits
    )

    return collisions, probability, first_collision


def main():

    rng = random.Random(20260917)

    print("======================================")
    print("       CHRONO MAYA COLLISION LAB")
    print("======================================")
    print()
    print("Full hash size: 512 bits")
    print("Collision testing: controlled truncations")
    print()

    total_collisions = 0

    for bits, samples in CONFIG.items():

        print("--------------------------------------")
        print(f"TRUNCATED HASH: {bits} bits")
        print("Samples:", samples)

        collisions, probability, first = run_collision_test(
            bits,
            samples,
            rng
        )

        total_collisions += collisions

        print("Observed collisions:", collisions)
        print(
            "Birthday probability:",
            f"{probability:.6f}"
        )

        if first is not None:

            a, b, value = first

            print("First collision:")
            print("  Sample A:", a)
            print("  Sample B:", b)
            print("  Truncated value:", value)

        else:

            print("First collision: NONE")

        print()

    print("======================================")
    print("RESULT")
    print("======================================")
    print()
    print("Total observed collisions:", total_collisions)
    print()

    print("IMPORTANT:")
    print("Collision tests use truncated outputs.")
    print("They do NOT attempt to break the full 512-bit hash.")
    print("Observed collisions in short truncations are expected.")
    print("This test does NOT establish cryptographic security.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

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


BITS = 32
SAMPLES = 100000
TRIALS = 50


def truncated_hash(data):
    digest = maya_hash(data)

    value = int.from_bytes(digest, "big")

    return value >> (512 - BITS)


def birthday_probability(n):
    space = 2 ** BITS

    exponent = -(n * (n - 1)) / (2 * space)

    return 1.0 - math.exp(exponent)


def collision_trial(trial_id):

    rng = random.Random(
        20260917 + trial_id
    )

    seen = set()
    collisions = 0

    for sample_id in range(SAMPLES):

        data = (
            b"CHRONO-MAYA-MONTE-CARLO/"
            + trial_id.to_bytes(4, "big")
            + sample_id.to_bytes(4, "big")
            + rng.randbytes(32)
        )

        value = truncated_hash(data)

        if value in seen:
            collisions += 1
        else:
            seen.add(value)

    return collisions


def main():

    print("======================================")
    print("   CHRONO MAYA COLLISION MONTE CARLO")
    print("======================================")
    print()
    print("Truncated hash:", BITS, "bits")
    print("Samples/trial:", SAMPLES)
    print("Trials:", TRIALS)
    print()

    expected_probability = birthday_probability(
        SAMPLES
    )

    print(
        "Expected probability of >=1 collision:",
        f"{expected_probability:.6f}"
    )

    print()

    collision_counts = []
    successful_trials = 0

    for trial in range(TRIALS):

        collisions = collision_trial(trial)

        collision_counts.append(collisions)

        if collisions > 0:
            successful_trials += 1

        print(
            f"Trial {trial + 1:02d}: "
            f"collisions={collisions}"
        )

    average = (
        sum(collision_counts) /
        len(collision_counts)
    )

    minimum = min(collision_counts)
    maximum = max(collision_counts)

    observed_probability = (
        successful_trials / TRIALS
    )

    print()
    print("======================================")
    print("STATISTICS")
    print("======================================")
    print()

    print(
        "Trials with >=1 collision:",
        successful_trials,
        "/", TRIALS
    )

    print(
        "Observed probability:",
        f"{observed_probability:.6f}"
    )

    print(
        "Expected probability:",
        f"{expected_probability:.6f}"
    )

    print(
        "Average collisions/trial:",
        f"{average:.4f}"
    )

    print("Minimum collisions:", minimum)
    print("Maximum collisions:", maximum)

    print()

    deviation = abs(
        observed_probability -
        expected_probability
    )

    print(
        "Probability deviation:",
        f"{deviation:.6f}"
    )

    print()
    print("======================================")
    print("RESULT")
    print("======================================")
    print()

    print("Status: RESEARCH STATISTICS COMPLETE")

    print()
    print("IMPORTANT:")
    print("This experiment uses a 32-bit truncation.")
    print("It does NOT attack the full 512-bit output.")
    print("This does NOT establish cryptographic security.")
    print("Maya-HASH remains RESEARCH ONLY.")


if __name__ == "__main__":
    main()

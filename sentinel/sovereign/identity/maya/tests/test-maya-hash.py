#!/usr/bin/env python3

import importlib.util
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
CORE_DIR = TEST_DIR.parent / "core"
MODULE_FILE = CORE_DIR / "maya_hash.py"

print("MAYA HASH TEST")
print("==============================")
print("Module:", MODULE_FILE)
print("Exists:", MODULE_FILE.exists())

if not MODULE_FILE.is_file():
    raise SystemExit("ERROR: maya_hash.py not found")

spec = importlib.util.spec_from_file_location(
    "maya_hash",
    MODULE_FILE
)

if spec is None or spec.loader is None:
    raise SystemExit("ERROR: unable to load maya_hash.py")

maya_hash_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(maya_hash_module)

maya_hash = maya_hash_module.maya_hash


def test_deterministic():
    value = b"chrono-test"
    assert maya_hash(value) == maya_hash(value)


def test_output_length():
    value = b"chrono-test"
    assert len(maya_hash(value)) == 64


def test_input_change():
    a = maya_hash(b"chrono-test")
    b = maya_hash(b"chrono-test-2")
    assert a != b


def test_empty_input():
    result = maya_hash(b"")
    assert len(result) == 64


def main():

    tests = [
        test_deterministic,
        test_output_length,
        test_input_change,
        test_empty_input,
    ]

    passed = 0

    for test in tests:
        test()
        print("PASS:", test.__name__)
        passed += 1

    print()
    print("Maya-HASH research tests:", passed)
    print("Status: PASS")


if __name__ == "__main__":
    main()

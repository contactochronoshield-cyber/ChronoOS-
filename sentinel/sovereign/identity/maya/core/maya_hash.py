#!/usr/bin/env python3

"""
Chrono Maya Hash - Research Prototype

IMPORTANT:
This is an experimental research construction.
It is NOT a production cryptographic hash.
"""

import hashlib
import struct


DOMAIN = b"CHRONO-MAYA-HASH/0.1"


def maya_hash(data: bytes) -> bytes:
    """
    Experimental hash construction.

    Uses established SHA-512 internally as a research baseline
    while the Maya construction is being studied.

    This is NOT a new cryptographic primitive.
    """

    state = hashlib.sha512(DOMAIN + data).digest()

    for round_id in range(8):
        material = (
            DOMAIN
            + struct.pack(">I", round_id)
            + state
            + data
        )

        state = hashlib.sha512(material).digest()

    return state


def maya_hash_hex(data: bytes) -> str:
    return maya_hash(data).hex()


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 2:
        print("Usage: maya-hash.py <text>")
        raise SystemExit(1)

    value = sys.argv[1].encode("utf-8")

    print("MAYA-HASH/0.1")
    print(maya_hash_hex(value))

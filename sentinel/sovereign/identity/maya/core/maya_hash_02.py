#!/usr/bin/env python3

"""
Chrono Maya-HASH/0.2
====================

Research implementation derived strictly from the
MAYA-HASH/0.2 specification.

IMPORTANT:
This implementation is RESEARCH ONLY.
It is not a production cryptographic primitive.
"""

import struct


MASK32 = 0xFFFFFFFF


# Frozen initial state.
# These values are normative for MAYA-HASH/0.2.
INITIAL_STATE = (
    0x6A09E667,
    0xBB67AE85,
    0x3C6EF372,
    0xA54FF53A,
    0x510E527F,
    0x9B05688C,
    0x1F83D9AB,
    0x5BE0CD19,
    0xCBBB9D5D,
    0x629A292A,
    0x9159015A,
    0x152FECD8,
    0x67332667,
    0x8EB44A87,
    0xDB0C2E0D,
    0x47B5481D,
)


# Frozen round constants.
ROUND_CONSTANTS = (
    0x6A09E667,
    0xBB67AE85,
    0x3C6EF372,
    0xA54FF53A,
    0x510E527F,
    0x9B05688C,
    0x1F83D9AB,
    0x5BE0CD19,
    0xCBBB9D5D,
    0x629A292A,
    0x9159015A,
    0x152FECD8,
    0x67332667,
    0x8EB44A87,
    0xDB0C2E0D,
    0x47B5481D,
    0xAE5F9156,
    0xCF6C85D3,
    0x2F73477D,
    0x6D1826CA,
    0x8B43D457,
    0xE360B596,
    0x1C456002,
    0x6F196331,
    0xD94EBEB1,
    0x0CC4A611,
    0x261DC1F2,
    0x5815A7BE,
    0x70B7ED67,
    0xA1513C69,
    0x44F93635,
    0x720DCDFD,
)


def rotr32(x: int, r: int) -> int:
    r %= 32
    return ((x >> r) | (x << (32 - r))) & MASK32


def rotl32(x: int, r: int) -> int:
    r %= 32
    return ((x << r) | (x >> (32 - r))) & MASK32


def f0(x: int, y: int, z: int) -> int:
    return ((x & y) ^ ((~x) & z)) & MASK32


def f1(x: int, y: int, z: int) -> int:
    return ((x & z) ^ (y & (~z))) & MASK32


def f2(x: int, y: int, z: int) -> int:
    return ((x ^ y) ^ (y & z)) & MASK32


def pad(message: bytes) -> bytes:
    bit_length = len(message) * 8

    result = bytearray(message)
    result.append(0x80)

    while (len(result) + 8) % 64 != 0:
        result.append(0)

    result.extend(struct.pack(">Q", bit_length))

    return bytes(result)


def message_schedule(block: bytes) -> list[int]:
    if len(block) != 64:
        raise ValueError("MAYA-HASH/0.2 requires 64-byte blocks")

    words = list(struct.unpack(">16I", block))

    for i in range(16, 32):
        value = (
            words[i - 16]
            + words[i - 7]
            + f0(
                words[i - 15],
                words[i - 2],
                words[i - 1],
            )
        ) & MASK32

        words.append(rotl32(value, i % 32))

    return words


def compress(state: tuple[int, ...], block: bytes) -> tuple[int, ...]:
    if len(state) != 16:
        raise ValueError("MAYA-HASH/0.2 requires 16 state words")

    words = message_schedule(block)

    s = list(state)

    for r in range(32):
        a = r % 16
        b = (5 * r + 1) % 16
        c = (7 * r + 3) % 16
        d = (11 * r + 5) % 16

        x = s[a]
        y = s[b]
        z = s[c]
        t = s[d]

        m = f0(x, y, z)
        n = f1(y, z, t)
        q = f2(z, t, x)

        rr = (
            rotl32(x, (r % 13) + 3)
            ^ rotr32(y, (r % 11) + 5)
            ^ rotl32(z, (r % 7) + 7)
            ^ q
        ) & MASK32

        u = (
            x
            + m
            + n
            + q
            + words[r]
            + ROUND_CONSTANTS[r]
        ) & MASK32

        s[a] = (s[a] ^ u) & MASK32
        s[b] = (s[b] + rotl32(u, 7)) & MASK32
        s[c] = (s[c] ^ rotr32(u, 11)) & MASK32
        s[d] = (s[d] + rr) & MASK32

        old = s[:]
        new = [0] * 16

        for i in range(16):
            p = (5 * i + 1) % 16
            new[p] = old[i]

        s = new

    for i in range(16):
        s[i] = (s[i] + state[i]) & MASK32

    return tuple(s)


def maya_hash_02(data: bytes) -> bytes:
    padded = pad(data)

    state = INITIAL_STATE

    for offset in range(0, len(padded), 64):
        block = padded[offset:offset + 64]
        state = compress(state, block)

    return struct.pack(">16I", *state)


def maya_hash_02_hex(data: bytes) -> str:
    return maya_hash_02(data).hex()


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 2:
        print("Usage: maya_hash_02.py <text>")
        raise SystemExit(1)

    value = sys.argv[1].encode("utf-8")

    print("MAYA-HASH/0.2")
    print(maya_hash_02_hex(value))

# MAYA-HASH/0.2
## Experimental Cryptographic Hash Construction Specification

Status: RESEARCH ONLY
Version: 0.2
Output: 512 bits
Byte order: Big-endian
Design goal: Experimental cryptographic research

---

## 1. Scope

MAYA-HASH/0.2 is an experimental cryptographic hash construction
developed for the Chrono Maya Cryptography research program.

It is intended to study:

- diffusion
- nonlinear mixing
- avalanche behavior
- differential behavior
- collision resistance
- implementation robustness
- performance
- structural cryptographic properties

MAYA-HASH/0.2 is NOT approved for production security.

No claim of cryptographic security is made by this specification.

---

## 2. Security boundary

MAYA-HASH/0.2 must not replace established cryptographic
primitives in production.

Production systems must continue to use independently analyzed
and standardized cryptographic algorithms.

MAYA-HASH/0.2 may only be used for:

- research
- testing
- controlled laboratory experiments
- comparative cryptographic analysis

---

## 3. Input

The algorithm accepts an arbitrary byte string:

M = m_0 || m_1 || ... || m_n

where each m_i is an 8-bit byte.

Empty input is valid.

Maximum input length is implementation-defined.

The reference implementation must reject inputs that cannot be
represented safely by the implementation.

---

## 4. Output

The digest length is exactly:

512 bits

or:

64 bytes

The digest is represented as:

H(M) = h_0 || h_1 || ... || h_63

---

## 5. Internal state

The internal state consists of:

16 words × 32 bits

Therefore:

State size = 512 bits

Let the state be:

S[0], S[1], ..., S[15]

Each word is interpreted as an unsigned 32-bit integer.

All arithmetic is performed modulo:

2^32

---

## 6. Primitive operations

The construction uses the following operations:

- XOR
- AND
- OR
- NOT
- addition modulo 2^32
- left rotation
- right rotation

No floating-point arithmetic is permitted.

All operations must have deterministic results across
supported platforms.

---

## 7. Rotation functions

Define:

ROTL32(x, r)

as a 32-bit left rotation by r bits.

Define:

ROTR32(x, r)

as a 32-bit right rotation by r bits.

Rotation counts must be reduced modulo 32.

---

## 8. Nonlinear functions

The construction defines three nonlinear mixing functions.

### F0

F0(x, y, z) =
(x AND y) XOR ((NOT x) AND z)

### F1

F1(x, y, z) =
(x AND z) XOR (y AND (NOT z))

### F2

F2(x, y, z) =
(x XOR y) XOR (y AND z)

All operations occur modulo 2^32 where applicable.

---

## 9. Word permutation

For each compression round, state words are permuted.

The permutation is:

P(i) = (5*i + 1) mod 16

where:

i = 0 ... 15

The permutation must be bijective.

The implementation must verify this property in the test suite.

---

## 10. Round constants

Each round uses a 32-bit constant.

The reference implementation must define constants explicitly.

Constants must NOT be generated dynamically from system time,
randomness, process identifiers, device identifiers, or external
state.

The complete constant table must be included in the implementation
and test vectors.

---

## 11. Message schedule

Each 64-byte message block is interpreted as:

16 × 32-bit words

using big-endian encoding.

For each block:

W[0..15]

are loaded directly from the block.

Additional words are generated deterministically from previous words.

The reference implementation must specify the exact recurrence
before any production consideration.

---

## 12. Compression function

The compression function receives:

- current state S
- message schedule W
- round constants K

and produces:

new state S'

The compression function must provide:

- nonlinear mixing
- cross-word diffusion
- message dependency
- state dependency
- rotational mixing
- feed-forward

The exact reference implementation must be treated as normative.

---

## 13. Feed-forward

After processing a block, the resulting state is combined with
the previous state using addition modulo 2^32:

S_i = S_i + S'_i mod 2^32

for:

i = 0 ... 15

---

## 14. Padding

Padding must ensure that the message length is encoded
unambiguously.

For message M:

1. Append one byte:

0x80

2. Append zero bytes.

3. Append the original message length encoded as a
64-bit big-endian integer.

The final block must be exactly 64 bytes.

If insufficient space remains for the length field,
an additional block must be created.

---

## 15. Domain separation

The algorithm identifier must be incorporated into initialization.

Domain identifier:

CHRONO-MAYA-HASH/0.2

The initialization procedure must not depend on external system
state.

---

## 16. Initialization

The initial 512-bit state must be represented by exactly
16 explicitly defined 32-bit constants.

The constants must be included in the reference implementation.

They must remain unchanged between compatible implementations.

---

## 17. Determinism

For identical input:

H(M) must always produce exactly the same 512-bit output.

For different inputs, identical output is permitted by the
mathematical definition of a finite hash function, but collision
resistance must be investigated experimentally and analytically.

---

## 18. Required test properties

Every reference implementation must pass:

### Functional

- empty input
- single byte
- multi-byte input
- block-sized input
- block-plus-one input
- large input

### Determinism

Repeated hashing of identical input must produce identical output.

### Avalanche

A one-bit input modification must produce approximately
half-output-bit changes on average.

### Bit distribution

Output bits should be statistically close to balanced over
large sample sets.

### Differential testing

Small input differences must not produce predictable output
differences.

### Collision research

Truncated-output experiments must be compared against expected
birthday-bound behavior.

### Fuzzing

Random and malformed inputs must not cause:

- crashes
- exceptions
- incorrect digest length
- nondeterministic output

### Cross implementation

Independent implementations must produce identical
test-vector outputs.

---

## 19. Cryptanalytic requirements

Before any production consideration, research must investigate:

- collision attacks
- second-preimage attacks
- preimage attacks
- differential attacks
- rotational attacks
- rebound-style attacks
- meet-in-the-middle attacks
- fixed-point structures
- multicollision structures
- length-extension behavior
- related-input behavior
- distinguishers
- reduced-round weaknesses
- algebraic weaknesses
- structural weaknesses
- implementation side channels

Passing statistical tests does not establish resistance
to these attacks.

---

## 20. Performance

Performance must be measured against:

- SHA-256
- SHA-512
- SHA3-256
- SHA3-512
- other available research baselines

Measurements must include:

- input size
- elapsed time
- throughput
- hashes per second
- CPU architecture
- implementation language
- number of rounds
- memory consumption where practical

Performance must never be interpreted as evidence of security.

---

## 21. Side-channel considerations

A future implementation intended for security-sensitive
environments must investigate:

- timing behavior
- memory access patterns
- branch behavior
- compiler transformations
- hardware acceleration
- power analysis
- electromagnetic leakage

The Python research implementation is not considered
side-channel hardened.

---

## 22. Implementation independence

At least two independently written implementations should
eventually be created.

The implementations must not share the same source code
for the core compression function.

Their outputs must agree for all published test vectors.

---

## 23. Test vectors

The project must publish deterministic test vectors for:

- empty input
- ASCII input
- binary input
- 1-byte input
- 63-byte input
- 64-byte input
- 65-byte input
- 127-byte input
- 128-byte input
- large input

Test vectors are mandatory for interoperability.

---

## 24. Versioning

MAYA-HASH/0.2 is incompatible with MAYA-HASH/0.1.

The version identifier must remain explicit.

Future incompatible constructions must receive a new version.

---

## 25. Security claims

The project must never claim:

- impossible to break
- unbreakable
- impossible to decrypt
- stronger than every existing hash
- immune to intelligence agencies
- immune to cybercriminals

Security claims require mathematical analysis,
cryptanalysis, independent review and reproducible evidence.

---

## 26. Production status

MAYA-HASH/0.2:

RESEARCH ONLY

It must not be used as the sole cryptographic protection
for production Chrono infrastructure.

Established cryptographic primitives remain the production
security baseline until MAYA-HASH has undergone substantial
independent analysis.

---

## 27. Research objective

The objective of MAYA-HASH is not to claim superiority.

The objective is to determine, through mathematics,
implementation, experimentation and independent analysis,
whether the construction possesses useful cryptographic
properties.

Evidence determines the result.

---

# 28. MAYA-HASH/0.2 REFERENCE CORE

This section defines the deterministic research construction.

The following parameters are normative for the reference
implementation.

## 28.1 State

The state contains 16 words of 32 bits:

S[0] ... S[15]

All additions are performed modulo 2^32.

## 28.2 Initial state

The initial state is derived from the hexadecimal fractional
digits of square roots of the first 16 prime numbers.

For each prime p_i, define:

C_i = floor(frac(sqrt(p_i)) * 2^32)

The resulting 32-bit values are used in order as:

S[0] = C_0
S[1] = C_1
...
S[15] = C_15

The values MUST be calculated once and then published explicitly
in the reference implementation.

Implementations MUST NOT calculate them using floating point
during normal hashing.

## 28.3 Round count

The reference construction uses:

32 rounds per 64-byte block.

Round numbers are:

r = 0 ... 31

## 28.4 Round constants

Each round uses one 32-bit constant.

For round r:

K[r] = floor(frac(sqrt(p_(r mod 16))) * 2^32)

where p_i is the corresponding prime from the initial-state
constant table.

The constants MUST be published explicitly in the implementation.

## 28.5 Message words

Each 64-byte block is parsed as:

W[0] ... W[15]

Each W[i] is a big-endian 32-bit word.

## 28.6 Extended message schedule

For i >= 16:

W[i] =
ROTL32(
    W[i-16] + W[i-7] + F0(W[i-15], W[i-2], W[i-1]),
    i mod 32
)

All additions are modulo 2^32.

The reference schedule contains:

48 words

W[0] ... W[47]

## 28.7 State selection

For each round r:

a = r mod 16
b = (5*r + 1) mod 16
c = (7*r + 3) mod 16
d = (11*r + 5) mod 16

The indices are deterministic.

## 28.8 Round function

Define:

X = S[a]
Y = S[b]
Z = S[c]
T = S[d]

The round function is:

M = F0(X, Y, Z)
N = F1(Y, Z, T)
Q = F2(Z, T, X)

R =
ROTL32(X, (r mod 13) + 3)
XOR
ROTR32(Y, (r mod 11) + 5)
XOR
ROTL32(Z, (r mod 7) + 7)
XOR
Q

U =
X
+ M
+ N
+ Q
+ W[r]
+ K[r]

all modulo 2^32.

The selected state words are then updated:

S[a] = S[a] XOR U

S[b] = S[b] + ROTL32(U, 7)

S[c] = S[c] XOR ROTR32(U, 11)

S[d] = S[d] + R

All additions are modulo 2^32.

## 28.9 Round permutation

After every round, the complete state is permuted.

The permutation is:

P(i) = (5*i + 1) mod 16

The new state is:

S_new[P(i)] = S_old[i]

for:

i = 0 ... 15

The permutation is bijective because gcd(5,16) = 1.

## 28.10 Feed-forward

After all 32 rounds:

for i = 0 ... 15:

S[i] = S[i] + S_initial[i]

modulo 2^32.

The resulting state becomes the chaining state for the next block.

## 28.11 Padding

Let L be the original message length in bytes.

The padded message is constructed as:

M || 0x80 || 0x00... || length

where length is:

L * 8

encoded as an unsigned 64-bit big-endian integer.

The final message length MUST be a multiple of 64 bytes.

At least 9 bytes are therefore reserved in the final block:

1 byte for 0x80
8 bytes for the length.

## 28.12 Domain separation

Before processing the message, the implementation derives the
initial chaining state from the fixed domain identifier:

CHRONO-MAYA-HASH/0.2

The domain identifier is encoded as UTF-8 bytes.

The reference implementation MUST use the explicitly published
initial constants and MUST NOT depend on runtime randomness.

## 28.13 Digest encoding

After processing all blocks, the 16 state words are serialized
in order:

S[0] || S[1] || ... || S[15]

Each word is encoded as big-endian uint32.

The final digest is therefore exactly:

16 * 4 = 64 bytes

or:

512 bits.

## 28.14 Reference invariants

The implementation MUST maintain:

- exactly 16 state words
- exactly 32 rounds per block
- exactly 48 schedule words
- exactly 512-bit output
- modulo-2^32 arithmetic
- deterministic constants
- deterministic permutation
- deterministic padding

Any change to these parameters creates a new incompatible
MAYA-HASH version.

## 28.15 Security interpretation

The construction described here is an experimental research
construction.

Its statistical behavior must not be interpreted as proof of
collision, preimage, second-preimage, differential or other
cryptographic resistance.

Independent cryptanalysis remains required.


---

# 29. DOMAIN INITIALIZATION — NORMATIVE

The domain identifier is:

CHRONO-MAYA-HASH/0.2

encoded as UTF-8.

The domain identifier is NOT hashed dynamically during normal
initialization.

The reference implementation uses a fixed initial state table
published in Section 30.

This guarantees that independent implementations begin with
exactly the same 512-bit chaining state.

The domain identifier exists as a protocol identifier and version
identifier.

Changing the domain identifier without changing the published
initial state is not permitted.

---

# 30. INITIAL STATE AND ROUND CONSTANTS

The reference implementation MUST publish all constants as
hexadecimal 32-bit unsigned integers.

No implementation may calculate constants using floating-point
arithmetic during hashing.

The published constants are normative.

The initial state consists of:

S[0] ... S[15]

The round constants consist of:

K[0] ... K[31]

The exact hexadecimal tables MUST be inserted into the normative
reference implementation and test-vector document.

Until those tables are published, MAYA-HASH/0.2 is considered
SPECIFICATION INCOMPLETE.

---

# 31. TEST VECTOR REQUIREMENT

Before the reference implementation is considered complete,
the following inputs MUST have published digest values:

1. empty input
2. "a"
3. "abc"
4. "chrono"
5. "Chrono Maya"
6. 64 zero bytes
7. 64 bytes containing 0xFF
8. 65 zero bytes
9. binary sequence 0x00 through 0xFF
10. a large deterministic test message

The vectors MUST be generated from the normative reference
implementation.

An independent implementation MUST reproduce every vector.

---

# 32. SPECIFICATION COMPLETENESS

MAYA-HASH/0.2 cannot be considered implementation-complete
until all of the following are explicitly defined:

- initial state constants
- round constants
- message schedule
- compression function
- round ordering
- state permutation
- padding
- digest serialization
- test vectors

Until these requirements are satisfied:

Status: RESEARCH SPECIFICATION — INCOMPLETE

No production use is permitted.


---

# 33. Frozen Round Constants

MAYA-HASH/0.2 uses the following 32-bit round constants.

The constants are generated deterministically from the fractional
parts of the square roots of the first 32 prime numbers, scaled to
32 bits.

This derivation provides deterministic reproducibility. It does not,
by itself, establish cryptographic security or independence from
known cryptographic constructions.

The constants are frozen for version 0.2.

```text
K[00] = 0x6a09e667
K[01] = 0xbb67ae85
K[02] = 0x3c6ef372
K[03] = 0xa54ff53a
K[04] = 0x510e527f
K[05] = 0x9b05688c
K[06] = 0x1f83d9ab
K[07] = 0x5be0cd19
K[08] = 0xcbbb9d5d
K[09] = 0x629a292a
K[10] = 0x9159015a
K[11] = 0x152fecd8
K[12] = 0x67332667
K[13] = 0x8eb44a87
K[14] = 0xdb0c2e0d
K[15] = 0x47b5481d
K[16] = 0xae5f9156
K[17] = 0xcf6c85d3
K[18] = 0x2f73477d
K[19] = 0x6d1826ca
K[20] = 0x8b43d457
K[21] = 0xe360b596
K[22] = 0x1c456002
K[23] = 0x6f196331
K[24] = 0xd94ebeb1
K[25] = 0x0cc4a611
K[26] = 0x261dc1f2
K[27] = 0x5815a7be
K[28] = 0x70b7ed67
K[29] = 0xa1513c69
K[30] = 0x44f93635
K[31] = 0x720dcdfd

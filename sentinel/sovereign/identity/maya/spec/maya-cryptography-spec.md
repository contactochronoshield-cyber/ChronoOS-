# Chrono Maya Cryptography Specification

## 1. Purpose

Chrono Maya defines a research architecture for cryptographic
identity and security of Chrono-managed communication devices.

Target devices include:

- Mesh Nodes
- Communication gateways
- Antenna controllers
- Private-network nodes
- Industrial gateways
- Edge devices
- ChronoOS systems

## 2. Design principles

1. Device-specific identity
2. Private-key isolation
3. Strong entropy requirements
4. Hardware binding where available
5. Cryptographic attestation
6. Key rotation
7. Revocation
8. Evidence preservation
9. Algorithm agility
10. Independent verification

## 3. Cryptographic separation

Maya separates:

IDENTITY
KEY GENERATION
HASHING
SIGNATURES
KEY AGREEMENT
ATTESTATION
KEY MANAGEMENT

No experimental primitive automatically becomes a production
security primitive.

## 4. Production cryptography

Until Maya primitives are independently validated, production
systems must use established cryptographic algorithms and protocols.

Maya may operate as an additional identity or research layer.

## 5. Experimental cryptography

Experimental algorithms must have:

- Versioned specification
- Deterministic test vectors
- Known security properties
- Negative tests
- Collision testing
- Avalanche testing
- Performance measurements
- Fuzz testing
- Independent cryptanalysis

## 6. Security claims

The project must never claim that a cryptographic primitive is
"impossible to break".

Security claims must identify:

- Threat model
- Assumptions
- Security level
- Attack model
- Evidence
- Known limitations

## 7. Key lifecycle

GENERATE
   ↓
REGISTER
   ↓
ATTEST
   ↓
ACTIVATE
   ↓
ROTATE
   ↓
REVOKE

Private key material must not be committed to source control.

## 8. Research status

Maya-HASH is experimental.

No production system may depend exclusively on Maya-HASH until
the required cryptographic validation has been completed.

# ChronoOS Root of Trust Policy

The root of trust is the cryptographic authority used to validate:

- node identities
- service certificates
- signed alerts
- QR provisioning payloads
- firmware/package signatures
- evidence manifests

A public key or certificate embedded in source code is NOT sufficient
unless its provenance and rotation process are defined.

Production root keys MUST remain outside the Git repository.

Rotation MUST support:

- key version
- activation date
- expiration date
- revocation
- replacement
- audit record

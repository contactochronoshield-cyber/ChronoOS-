# ChronoOS Security Ledger

The ledger uses append-only structured records.

Required fields:

- timestamp
- event_id
- event_type
- severity
- component
- actor
- target
- action
- result
- evidence_hash
- previous_record_hash

Recommended serialization:

JSON Lines (one canonical JSON object per line).

Security requirements:

- No plaintext credentials.
- No private keys.
- Canonical field ordering.
- UTF-8.
- UTC timestamps.
- SHA-256 evidence references.
- Previous-record hash chaining.
- Exclusive file locking.
- Atomic append.
- Verification before archival.

The ledger is an audit mechanism, not a trust root.

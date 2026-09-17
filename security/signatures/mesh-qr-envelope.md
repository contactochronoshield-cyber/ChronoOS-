# ChronoOS Mesh / QR Signed Envelope

Required production envelope:

version
device_id
key_id
timestamp
nonce
message_type
payload_hash
signature

Verification order:

1. Parse bounded input.
2. Validate version.
3. Validate device identity.
4. Validate key status.
5. Verify timestamp window.
6. Verify nonce/replay protection.
7. Verify payload hash.
8. Verify signature.
9. Authorize requested operation.
10. Write forensic ledger event.

Unsigned control messages must not authorize privileged operations.

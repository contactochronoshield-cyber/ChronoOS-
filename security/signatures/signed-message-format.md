# ChronoOS Signed Message Format

Mesh alerts and QR provisioning messages MUST be treated as untrusted
until signature verification succeeds.

Canonical envelope:

{
  "version": 1,
  "type": "...",
  "message_id": "...",
  "issuer": "...",
  "issued_at": "...",
  "expires_at": "...",
  "payload": {},
  "signature": "base64..."
}

Rules:

- Canonical serialization before signing.
- Explicit algorithm identifier.
- Explicit key identifier.
- Expiration checking.
- Replay protection.
- Nonce/message_id uniqueness.
- Signature verification against an approved trust root.
- Rejected messages MUST be auditable.
- Private keys MUST never be included in QR payloads.

QR codes may transport signed enrollment/provisioning data, but QR
transport itself does not provide trust.

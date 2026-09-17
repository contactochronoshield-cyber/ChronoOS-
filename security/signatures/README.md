# ChronoOS Signature Boundary

Production signatures require a trusted root of trust.

Rules:
- private signing keys never enter Git
- verification keys may be distributed
- signatures must bind identity, timestamp, event type and payload hash
- failed verification is a security event
- key rotation and revocation are mandatory
- no automatic attribution from a signature alone

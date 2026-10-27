# ChronoOS 1.3: security status and industrial readiness

This document states what is done, what is unproven, and what still blocks industrial use.
Nothing here is a certification or a guarantee.

## Implemented and covered by automated tests

- Ed25519 identity per node; signed events and batches; the server stores public keys only
- Node revocation; replay of a batch does not duplicate events
- TLS 1.2+ for the receiver with public-key pinning (SPKI SHA-256): the node refuses a server whose key does not match `server_pin`
- Plain HTTP only on localhost; the receiver refuses to listen on other interfaces without `--tls`
- Signed updates: reject unsigned, tampered, wrongly signed, not-newer-version and unsafe-path packages; one-level rollback
- Private files created with umask 077; `chrono doctor` flags loose permissions and `chrono harden` fixes them
- Retention (`chrono maintenance`, run daily by ChronoD): prunes old synced events and trims logs, keeps pending events
- Foreground daemon mode (`chrono daemon run`) for service managers; process is named `chronod` for `chronoctl`

## Implemented but not validated in production

- systemd units in `packaging/systemd/` (hardened, never run on a real systemd host)
- Termux service script in `platforms/termux/services/chronod/`
- TLS pinning over real networks; the server certificate is valid 365 days and nothing alerts before it expires (renew with `chrono server init-tls`, which keeps the same pin unless `--rotate-key`)

## Blockers before industrial use

1. No bootable image, verified boot or immutable root (roadmap phase 5). `chrono update apply` stages a verified release; it does not activate it atomically (no A/B slots).
2. No validation on target industrial hardware, soak or fault-injection testing (phase 6).
3. No independent security review or penetration test of the receiver and the update path.
4. Key management: the node private key is a file (no TPM, Android Keystore or secure element); no node key rotation; no defined key ceremony or revocation for the update publisher.
5. Receiver: stdlib single-process HTTP server, no rate limiting or per-node quotas, no timestamp or nonce window beyond deduplication, audit data is not tamper-evident (no hash chain).
6. Supply chain: no SBOM, no reproducible builds, dependencies not pinned.
7. No compliance certification (for example IEC 62443) is claimed or implied.

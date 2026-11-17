# ChronoOS 1.3.0

## What is new

- **ChronoD edge agent:** persistent offline event queue with retrying sync; runs in the background or as a service-manager foreground process.
- **Receiver (`chrono server`):** verifies, deduplicates and stores events from registered devices; TLS with public-key pinning; refuses plain HTTP outside localhost.
- **Device identity:** Ed25519 key pair per device; signed events and batches; the server stores public keys only; devices can be revoked.
- **Signed updates:** only packages signed by the trusted publisher install; unsigned, tampered, not-newer and unsafe-path packages are refused; one-level rollback.
- **Hardening:** private-by-default files, `chrono doctor` permission checks, `chrono harden`, data retention (`chrono maintenance`), systemd units and a Termux service script.
- **chronoctl integration:** ChronoD runs as `chronod`, so `chronoctl status`, `services` and `stop chronod` manage it; `chronoctl` delegates the 1.3 commands to `chrono`.
- **Guided demo:** `python3 chronoos-1.3/demo.py` (English and Spanish), runs in a sandbox and checks every result it narrates.

## Verified

Automated tests cover the CLI and offline queue, identity and signed updates, TLS pinning and hardening. Verified on Android/Termux (aarch64).

## Not included

No bootable image, no validation on industrial hardware, no independent security audit, no hardware-backed key storage. See `chronoos-1.3/SECURITY-AND-READINESS.md` for the complete list. Version numbers carry no alpha/beta labels; maturity is described in that document.

## Licensing

Proprietary. The demo may be run unmodified for evaluation: `chronoos-1.3/DEMO-LICENSE.md`.

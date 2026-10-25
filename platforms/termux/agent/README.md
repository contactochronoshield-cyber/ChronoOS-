# Termux Edge Agent

The Termux agent provides:

- Node identity
- Heartbeat
- Telemetry
- Fleet registration
- Local diagnostics
- Offline event queue

## Implementation status

The Termux edge agent (ChronoD) is implemented in [`chronoos-1.3/`](../../../chronoos-1.3/):
persistent offline event queue, retrying sync, Ed25519 node identity and signed updates.
Verified on Android/Termux (aarch64). Run `python3 chronoos-1.3/tests_cli.py` and
`python3 chronoos-1.3/tests_phase4.py` from the repository root.

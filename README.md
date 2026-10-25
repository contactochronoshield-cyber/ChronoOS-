# ChronoOS

## Security, Telecommunications & Sovereign Edge Infrastructure

**ChronoOS** is a human-developed infrastructure software project from
the Chrono Shield technology ecosystem. It explores secure operations
across telecommunications, private networks, edge systems and
dedicated computing environments.

The repository contains components at different maturity levels.
Features described as implemented are not necessarily production-ready,
independently audited or available on every supported platform.

- **Project version:** see [`VERSION`](VERSION)
- **ChronoOS 1.3 module:** [`chronoos-1.3/`](chronoos-1.3/)
- **Termux platform:** [`platforms/termux/`](platforms/termux/)
- **Termux agent documentation:** [`platforms/termux/agent/README.md`](platforms/termux/agent/README.md)

---

## Project and organization

ChronoOS is part of the Chrono Shield technology ecosystem.

- **Parent organization:** Chrono Shield Systems
- **Telecommunications and infrastructure division:** Chrono Shield Networks

The project's direction includes infrastructure software, networking,
cybersecurity, edge computing and tools for operating heterogeneous
systems. Specific capabilities depend on the component and deployment.

## Mission

ChronoOS aims to make infrastructure more:

- Secure and observable
- Interoperable across different systems
- Resilient when connectivity is limited
- Traceable through verifiable events
- Manageable across distributed nodes
- Adaptable to local and regional requirements

ChronoOS is intended to complement existing infrastructure where
compatible interfaces and appropriate authorization are available.

---

## ChronoOS 1.3 — development module

Version 1.3 introduces an isolated implementation under
[`chronoos-1.3/`](chronoos-1.3/). It is not yet a replacement for every
legacy component in this repository.

### Implemented in the 1.3 module

- **CLI:** commands for status, diagnostics, capabilities, events,
  queues, logs, node identity, server operations and updates.
- **ChronoD agent:** local event queue with persistent storage and
  synchronization retry logic.
- **Receiver:** HTTP server for enrolled nodes that verifies signed
  batches, rejects invalid requests and deduplicates stored events.
- **Node identity:** Ed25519 keys and signatures for events and batches.
- **Node enrollment and revocation:** registration associates a node ID
  with its public key.
- **Signed updates:** signature verification, version checks, rejection
  of unsafe archive paths and a one-level rollback mechanism.

These features have automated tests in `chronoos-1.3/tests_cli.py` and
`chronoos-1.3/tests_phase4.py`. The repository maintainer has run both
test files successfully in Android/Termux on ARM64. Passing tests do not
constitute a complete security audit or prove production readiness.

### Known limitations

- The 1.3 module is separate from the legacy `cli/chronoctl` utility.
  ChronoCTL may still report version `1.1.0-alpha` and may not detect
  or manage the 1.3 ChronoD process correctly.
- The receiver uses plain HTTP by default. Ed25519 signatures provide
  authenticity and integrity checks; they do **not** encrypt network
  traffic.
- Protect receiver traffic with properly configured TLS or a protected
  tunnel, such as WireGuard, before using it across untrusted networks.
- The update mechanism stages a verified release; it does not mean the
  operating system has rebooted into that release.
- A bootable ChronoOS image, an independently validated industrial
  profile and an HPC profile are not established by these tests.
- Windows, macOS, iOS and industrial hardware are not validated by the
  current Termux test results.
- Termux runs inside Android. It does not provide unrestricted kernel,
  bootloader or operating-system control.

---

## Run ChronoOS 1.3 on Termux

Install the required packages:

```bash
pkg update
pkg install python python-cryptography
```

From the repository root:

```bash
chmod +x chronoos-1.3/chrono
mkdir -p ~/bin
ln -sf "$PWD/chronoos-1.3/chrono" ~/bin/chrono
```

If `~/bin` is not on your `PATH`, add it to your shell configuration:

```bash
echo 'export PATH="$HOME/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Run diagnostics:

```bash
chrono --help
chrono doctor
chrono status
chrono caps
```

If the Python `cryptography` package is unavailable through the
configured Termux repositories, install it using a compatible method
supported by your environment.

### Common commands

```bash
chrono status
chrono caps
chrono queue
chrono logs
chrono event KIND "message"

chrono daemon start
chrono daemon status
chrono daemon stop

chrono node
chrono server run
chrono server list
```

Server enrollment and revocation:

```bash
chrono server enroll NODE_ID PUBLIC_KEY
chrono server revoke NODE_ID
```

Update-related commands:

```bash
chrono publisher keygen DIRECTORY
chrono update trust PUBLIC_KEY
chrono update verify PACKAGE.tar.gz
chrono update apply PACKAGE.tar.gz
chrono update rollback
chrono update status
```

Consult `chrono --help` and the module README for the exact arguments
and environment required by each command. Keep publisher private keys
offline and never share node private keys.

The agent's receiver URL is configured through
`~/.chronoos/config.json`, for example:

```json
{
  "sync_url": "http://127.0.0.1:8787/"
}
```

Use an address reachable from the node when the receiver is on another
machine. Do not expose the default HTTP receiver directly to untrusted
networks.

---

## Security model

- Node identities use Ed25519 public/private key pairs.
- The server registers public keys and verifies signed batches.
- Publisher private keys should be kept offline and separate from
  application nodes and receiver servers.
- Update packages must be signed by a configured trusted publisher.
- The update logic rejects non-increasing versions and unsafe archive
  paths, and records a previous release path for rollback.
- Signature verification does not protect against every compromised
  endpoint, malicious trusted publisher, denial-of-service attack,
  metadata leak or unsafe deployment configuration.
- Transport encryption, key backup, access control, secure operations
  and deployment-specific threat modelling remain important.

**Do not treat the current implementation as independently audited or
production-ready solely because automated tests pass.**

---

## Tests

Run from the repository root:

```bash
python3 chronoos-1.3/tests_cli.py
python3 chronoos-1.3/tests_phase4.py
```

Both commands should finish successfully. Review their output and exit
codes after any relevant code changes.

---

## Roadmap

The following are development directions, not claims that the features
are already available:

1. Integrate and test the 1.3 module with the legacy management CLI.
2. Improve lifecycle management and verify that ChronoCTL accurately
   detects and controls ChronoD.
3. Add secure transport and strengthen enrollment and operational
   access controls.
4. Expand automated tests, security review and release validation.
5. Explore bootable distribution images and hardware-specific profiles.
6. Evaluate industrial and high-performance-computing deployments.
7. Develop platform-specific integrations where operating-system
   permissions and hardware support make them feasible.

## License and contributions

Consult the repository's license and contribution policies before
redistributing code or submitting changes. Do not commit private keys,
credentials, personal vault data, or other sensitive files.

---

ChronoOS is an evolving infrastructure project. Refer to the source code,
tests and release notes for the current implementation status.

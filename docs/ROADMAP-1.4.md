# ChronoOS 1.4 roadmap

Releases are numbered 1.x.y with no alpha, beta or dev suffix. Maturity is described in the status documents ([SECURITY-AND-READINESS.md](../chronoos-1.3/SECURITY-AND-READINESS.md)), never in the version number.

Rule for 1.4: an item is "done" only when it has automated tests, documented limits and an entry in the readiness document.

## Order of work

1. Fleet Manager  2. Zero-Trust key storage  3. Post-quantum signatures  4. Edge Cloud lab  5. GIS and network simulator  6. Active Defense  7. Clone & Migrate  8. Physical Twin  9. Marketplace  10. Experience track (separate, after the rest)

## Infrastructure

| Item | What 1.4 delivers | Honest limits | Status |
|---|---|---|---|
| **Fleet Manager** | CLI (`chrono fleet list/summary/show/export`), then heartbeat and a read-only web dashboard with token auth; pagination; tested at 1,000+ simulated devices | Status is based on the last event received until the heartbeat exists | CLI preview done |
| **Chrono Edge Cloud** | Lab profile: K3s (lightweight Kubernetes) + Open5GS (open-source 5G core) in one box, scripted install, smoke tests with simulated radio and devices; then app hosting at the edge (MEC), step by step | A real private 5G also needs radio hardware, SIMs and spectrum authorization from the Colombian authorities. "Complete MEC" is split into stages | Planned |
| **GIS & Network Simulator** | Build on the existing `chrononet-gis` modules and importers (Colombia, Brazil/Anatel); add a topology simulator for test scenarios | Record the licence and source of every dataset | Planned |

## Security

| Item | What 1.4 delivers | Honest limits | Status |
|---|---|---|---|
| **Zero-Trust key storage** | Device keys held in hardware (TPM 2.0 on Linux, Android Keystore on phones); FIDO2/YubiKey login for operators; biometrics through the phone's own APIs | This is the key-storage and operator-login layer, not "the whole OS". It closes blocker 4 of the readiness document | Planned |
| **Post-quantum signatures** | Hybrid signatures: Ed25519 plus ML-DSA (standardized by NIST in 2024), also ML-KEM for key exchange | "Simulated" quantum signatures give no protection and will not ship. AES-256 (Rijndael) is already considered quantum-resistant; the public-key parts are what change | Planned |
| **Active Defense** | Detect and contain threats inside infrastructure you own: anomaly detection (builds on Chrono Guardian / AAP), quarantine a node, isolate a process, block traffic, alert | Out of scope: acting against systems you do not own. Hack-back is illegal in most jurisdictions and would expose the company | Planned |
| **Clone & Migrate** | Live migration of CPU and memory for virtual machines (KVM/QEMU), checkpoint/restore for containers (CRIU), disk snapshots and clones | Needs compatible hosts and a fast network; "seconds" depends on RAM size. GPU state migration is not generally possible and is documented as unsupported | Planned |

## Later

| Item | Notes |
|---|---|
| **Physical Twin** | Builds on `industrial/modbus` and the digital twin. First pilot with one sensor type at a real site |
| **Marketplace** | Signed packages already exist (1.3). Needs a publisher registry, package review and revocation, payments. Security-critical, so after Zero-Trust |
| **Experience track** | Cyberpunk GUI with AR (ARCore/OpenXR), voice commander, local music/art generation, Life Mode. Recommended as a separate app: consumer features with their own threat model. Life Mode's finance and health data belongs in the existing encrypted vault. Local-first, on-device models |

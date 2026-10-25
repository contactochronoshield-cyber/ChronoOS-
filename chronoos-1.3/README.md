# ChronoOS 1.3 (in development)

Isolated module with phases 1-4 of the 1.3 roadmap. Verified on Android/Termux (aarch64).

## What works

- `chrono` CLI: version, status, doctor, services, caps, logs
- Capability and profile detection (platform, arch, WireGuard, systemd, NVIDIA GPU, disk, network)
- ChronoD: persistent offline event queue (SQLite) with retrying sync
- Receiver (`chrono server`): verifies, deduplicates and stores events from enrolled nodes
- Ed25519 node identity: signed events and batches; the server stores public keys only; nodes can be revoked
- Signed updates: rejects unsigned, tampered, wrongly signed, same-or-older version and unsafe-path packages; one-level rollback

## Not implemented yet

- Bootable image / own distribution (phase 5)
- Industrial and HPC profiles (phase 6)
- Booting from an applied update (it is staged in ~/.chronoos/releases/)
- Transport encryption in the receiver (plain HTTP: use WireGuard/mesh or add TLS)
- Validated support on Windows, macOS, iOS or industrial hardware

## Install (Termux)

    pkg install python python-cryptography
    mkdir -p ~/bin && ln -sf "$PWD/chronoos-1.3/chrono" ~/bin/chrono
    chrono doctor

Needs Python 3.8+. Only ChronoD, the receiver and updates need `cryptography`.

## Usage

    chrono status | caps | queue | logs
    chrono event KIND "message"
    chrono daemon start|stop|status

    chrono server run                  # on the server
    chrono node                        # on the node: prints id and PUBLIC key
    chrono server enroll ID PUB        # on the server
    chrono server revoke ID

    chrono publisher keygen DIR
    chrono update trust PUB
    chrono update sign PACKAGE.tar.gz --key DIR/publisher.key
    chrono update apply PACKAGE.tar.gz
    chrono update rollback

The node points to the receiver via ~/.chronoos/config.json: {"sync_url": "http://host:8787/"}

## Security model

- The node private key (~/.chronoos/node.ed25519, mode 600) never leaves the device.
- The publisher private key must be kept offline, away from nodes and servers.
- A compromised server cannot forge node events: it holds no node private keys.

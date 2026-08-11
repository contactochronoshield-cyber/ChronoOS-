# ChronoOS Product Scope

## Purpose

ChronoOS is a security-oriented operating environment for dedicated
devices and appliances.

It is designed for controlled hardware deployments such as:

- Security appliances
- Communication systems
- Industrial monitoring systems
- Dedicated network equipment
- Core Box infrastructure
- Air-gapped or restricted environments

ChronoOS is not intended to be a general-purpose desktop operating system.

---

## In Scope

The project prioritizes:

- Deterministic boot and service startup
- Process supervision
- Network services
- Security controls
- Local logging and auditing
- Restricted/offline operation
- Recovery and rollback mechanisms
- Dedicated hardware support
- Minimal userspace
- Web-based management interfaces
- Reproducible builds
- Automated testing
- Secure update mechanisms

---

## Explicitly Out of Scope

The following are not current ChronoOS goals:

### Desktop environments

ChronoOS does not aim to provide:

- GNOME
- KDE
- Full desktop environments
- General-purpose graphical sessions

Operators should use the ChronoOS console or a dedicated management
interface.

### Broad consumer hardware support

ChronoOS does not attempt to support every:

- Laptop
- GPU
- Webcam
- Printer
- Bluetooth peripheral
- Consumer device

Hardware support is focused on validated target platforms.

### Desktop application ecosystem

ChronoOS is not intended to provide:

- Office suites
- General-purpose web browsers
- Desktop media applications
- Large consumer application repositories

### Type-1 hypervisor architecture

ChronoOS does not currently attempt to reproduce the architecture of
projects such as Qubes OS.

Isolation requirements are addressed according to the threat model of
dedicated appliances and controlled deployments.

---

## Design Principle

ChronoOS is optimized for its intended deployment environment rather
than for feature parity with desktop operating systems.

A missing desktop feature is not considered a defect when that feature
is outside the product's defined mission.

---

## Current Product Priorities

Priority is given to:

1. Reliable boot
2. Process supervision
3. Filesystem support
4. Networking
5. Security
6. Logging and auditing
7. Recovery
8. Reproducible builds
9. Automated testing
10. Secure updates

---

## Transparency

ChronoOS documentation distinguishes between:

- Implemented and tested
- Implemented but experimental
- In development
- Planned
- Out of scope

No security capability should be considered production-ready merely
because it appears in documentation.

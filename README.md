# ChronoOS

## Security-Oriented Appliance Operating System

ChronoOS is an experimental operating-system project focused on
dedicated infrastructure and security appliances.

The project prioritizes deterministic boot, process supervision,
networking, logging, recovery and controlled software updates.

ChronoOS is not intended to be a general-purpose desktop operating
system.

---

## Project Status

ChronoOS is currently in active development.

This repository contains the foundation of the system, including the
initial boot manager, service manager, command-line interface and
project architecture.

It should not currently be considered production-ready.

---

## Current Architecture

```text
ChronoOS
│
├── init/
│   └── chrono-init
│
├── kernel/
│   ├── scheduler/
│   ├── memory/
│   ├── ipc/
│   ├── syscall/
│   ├── process/
│   └── power/
│
├── network/
│   ├── tcpip/
│   ├── firewall/
│   ├── routing/
│   ├── vpn/
│   ├── dns/
│   ├── mesh/
│   └── discovery/
│
├── services/
│   └── chronod/
│
├── cli/
│   └── chronoctl/
│
├── recovery/
│   ├── rescue/
│   ├── rollback/
│   └── snapshots/
│
├── security/
│
├── libchrono/
│
├── core/
│
├── tests/
│
└── docs/

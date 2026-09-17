# ChronoOS Sentinel — Telecom Observed Events

This directory contains normalized security events observed from authorized ChronoOS environments.

Observed events are evidence inputs for the telecom correlation engine.

## Event principles

An event must identify, when available:

- Event ID
- Timestamp
- Technology
- Component
- Event type
- Source
- Severity
- Environment
- Evidence reference

## Security boundary

Events must originate from:

- Chrono-managed infrastructure
- Authorized customer infrastructure
- Controlled laboratory environments
- Authorized telemetry sources

Third-party targeting is prohibited.

## Important distinction

A detected event does not automatically establish an attack.

ChronoOS Sentinel correlates:

Observed Event
+
Threat Intelligence
+
Technical Evidence

before generating an investigation candidate.

## Attribution

Observed events do not establish attribution by themselves.

Attribution requires independent corroborating evidence.

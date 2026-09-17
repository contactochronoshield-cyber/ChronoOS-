# ChronoOS Reproducible Build Policy

Production builds must record:

- compiler version
- linker version
- source commit
- build flags
- dependency versions
- target architecture
- operating-system/toolchain image
- generated artifact SHA-256

Builds must be deterministic where practical.
Runtime secrets and customer data are excluded from artifacts.

# ChronoOS Fuzzing

Security-sensitive parsers MUST be fuzzed.

Priority targets:

1. Modbus TCP parser
2. QR decoder/parser
3. Mesh message parser
4. Signed-message parser
5. API input parsers
6. Evidence/ledger parser
7. Configuration parsers

Required properties:

- no crash
- no memory corruption
- no unbounded allocation
- no infinite loop
- malformed input rejected safely
- parser never bypasses authorization

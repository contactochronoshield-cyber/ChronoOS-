# ChronoOS Modbus TCP Defensive Framing

A Modbus TCP ADU consists of:

MBAP Header:
    Transaction Identifier: 2 bytes
    Protocol Identifier:    2 bytes
    Length:                 2 bytes
    Unit Identifier:        1 byte

PDU:
    Function Code
    Function Data

The Length field defines the number of bytes following the Length field,
including the Unit Identifier and PDU.

Parser requirements:

1. Read exactly 7 bytes of MBAP header.
2. Decode fields as big-endian.
3. Require Protocol Identifier == 0 for standard Modbus TCP.
4. Validate Length against configured limits.
5. Read exactly the declared number of remaining bytes.
6. Never trust TCP packet boundaries as application-message boundaries.
7. Support fragmented reads.
8. Support multiple ADUs in one TCP read.
9. Reject malformed lengths.
10. Enforce function-code-specific PDU length validation.
11. Apply timeouts.
12. Never execute a Modbus request solely because it parsed correctly.
13. Authorization MUST occur before sensitive write operations.

A TCP read/write is not equivalent to a Modbus ADU boundary.

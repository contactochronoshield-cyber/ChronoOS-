# ChronoOS API Security Baseline

Production API requirements:

1. Authentication required.
2. Authorization required.
3. TLS required for network transport.
4. No anonymous administrative endpoints.
5. Credentials never logged.
6. Request size limits.
7. Input validation.
8. Rate limiting.
9. Audit event for privileged actions.
10. Security failures fail closed.
11. Secrets supplied through environment/secret store, never source.
12. Production must use a trusted certificate chain.

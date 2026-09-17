# ChronoOS Storage Layout

Source:
    project source tree

Binaries:
    bin/
    dist/

Runtime:
    security/runtime/

Logs:
    logs/

Operational data:
    data/

Backups:
    backups/

Security evidence:
    security/evidence/

Keys:
    security/keys/
    production keys MUST NOT be committed

Build metadata:
    security/build/

The source tree MUST NOT be used as a database, customer-data directory,
runtime log directory or production key store.

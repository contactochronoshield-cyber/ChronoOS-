#!/usr/bin/env python3

import hashlib
import json
import os
import tempfile
from datetime import datetime, timezone
from pathlib import Path

try:
    import fcntl
except ImportError:
    fcntl = None

ROOT = Path(__file__).resolve().parent
LEDGER = ROOT / "chronos-security.jsonl"


def canonical(obj):
    return json.dumps(
        obj,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=False,
    )


def record_hash(record):
    return hashlib.sha256(
        canonical(record).encode("utf-8")
    ).hexdigest()


def append_record(event_type, severity, component,
                  actor, target, action, result,
                  evidence_hash=""):

    record = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "event_id": os.urandom(16).hex(),
        "event_type": event_type,
        "severity": severity,
        "component": component,
        "actor": actor,
        "target": target,
        "action": action,
        "result": result,
        "evidence_hash": evidence_hash,
        "previous_record_hash": "",
    }

    LEDGER.parent.mkdir(parents=True, exist_ok=True)

    with LEDGER.open("a+", encoding="utf-8") as fp:
        if fcntl is not None:
            fcntl.flock(fp.fileno(), fcntl.LOCK_EX)

        fp.seek(0)
        previous = ""

        for line in fp:
            line = line.strip()
            if line:
                try:
                    previous = record_hash(json.loads(line))
                except json.JSONDecodeError:
                    raise RuntimeError("Ledger contains invalid JSON")

        record["previous_record_hash"] = previous

        payload = canonical(record) + "\n"

        fp.seek(0, os.SEEK_END)
        fp.write(payload)
        fp.flush()
        os.fsync(fp.fileno())

        if fcntl is not None:
            fcntl.flock(fp.fileno(), fcntl.LOCK_UN)


if __name__ == "__main__":
    append_record(
        event_type="SYSTEM",
        severity="INFO",
        component="chrono-remediation",
        actor="system",
        target="chronoos",
        action="security-baseline",
        result="initialized",
    )

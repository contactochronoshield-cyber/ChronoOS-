#!/usr/bin/env python3

import csv
import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print("Uso: import_csv.py INPUT.csv OUTPUT.json")
    sys.exit(1)

src = Path(sys.argv[1])
dst = Path(sys.argv[2])

records = []

with src.open("r", encoding="utf-8-sig", newline="") as f:
    reader = csv.DictReader(f)

    for row in reader:
        try:
            row["latitude"] = float(row["latitude"])
            row["longitude"] = float(row["longitude"])
        except (ValueError, KeyError):
            continue

        row["technologies"] = [
            x for x in row.get("technologies", "").split("|") if x
        ]

        row["chrono_owned"] = row.get(
            "chrono_owned", "false"
        ).lower() == "true"

        row["chrono_managed"] = row.get(
            "chrono_managed", "false"
        ).lower() == "true"

        row["verified"] = row.get(
            "verified", "false"
        ).lower() == "true"

        records.append(row)

dst.parent.mkdir(parents=True, exist_ok=True)

with dst.open("w", encoding="utf-8") as f:
    json.dump(
        {
            "schema": "chrono.infrastructure.dataset",
            "count": len(records),
            "records": records
        },
        f,
        ensure_ascii=False,
        indent=2
    )

print(f"[OK] Imported {len(records)} records")
print(f"[OK] {dst}")

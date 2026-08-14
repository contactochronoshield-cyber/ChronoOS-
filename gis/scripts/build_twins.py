#!/usr/bin/env python3

import json
import sys
from pathlib import Path

if len(sys.argv) != 3:
    print("Uso: build_twins.py sites.json twins.json")
    sys.exit(1)

src = Path(sys.argv[1])
dst = Path(sys.argv[2])

data = json.loads(src.read_text(encoding="utf-8"))

twins = []

for item in data.get("records", []):
    twin = {
        "twin_id": "TWIN-" + item["site_id"],
        "site_id": item["site_id"],
        "location": {
            "country": item.get("country"),
            "region": item.get("region"),
            "municipality": item.get("municipality"),
            "latitude": item["latitude"],
            "longitude": item["longitude"]
        },
        "infrastructure": {
            "asset_type": item.get("asset_type"),
            "operator": item.get("operator"),
            "technologies": item.get("technologies", [])
        },
        "chrono": {
            "owned": item.get("chrono_owned", False),
            "managed": item.get("chrono_managed", False),
            "devices": []
        },
        "telemetry": {},
        "security": {},
        "lifecycle": {},
        "provenance": {
            "source": item.get("source"),
            "verified": item.get("verified", False)
        }
    }

    twins.append(twin)

dst.parent.mkdir(parents=True, exist_ok=True)

dst.write_text(
    json.dumps(
        {
            "schema": "chrono.digital-twin.dataset",
            "count": len(twins),
            "twins": twins
        },
        ensure_ascii=False,
        indent=2
    ),
    encoding="utf-8"
)

print(f"[OK] Generated {len(twins)} Digital Twins")

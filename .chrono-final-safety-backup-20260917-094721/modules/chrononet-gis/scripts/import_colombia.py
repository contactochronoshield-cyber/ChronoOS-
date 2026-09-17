#!/usr/bin/env python3

import csv
import json
import sqlite3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "data" / "raw"
PROCESSED = ROOT / "data" / "processed"

CSV_FILE = RAW / "colombia_mobile_infrastructure.csv"
DB_FILE = PROCESSED / "chrononet_colombia.db"
GEOJSON = PROCESSED / "colombia_infrastructure.geojson"

PROCESSED.mkdir(parents=True, exist_ok=True)

if not CSV_FILE.exists() or CSV_FILE.stat().st_size == 0:
    print("[WARN] No existe dataset CSV.")
    print("[INFO] Coloque el CSV oficial en:")
    print(CSV_FILE)
    sys.exit(0)

def find_field(fields, candidates):
    normalized = {
        f.lower().strip(): f
        for f in fields
    }

    for candidate in candidates:
        c = candidate.lower().strip()

        if c in normalized:
            return normalized[c]

    for f in fields:
        lf = f.lower()

        for candidate in candidates:
            if candidate.lower() in lf:
                return f

    return None

with CSV_FILE.open("r", encoding="utf-8-sig", newline="") as f:
    reader = csv.DictReader(f)

    fields = reader.fieldnames or []

    lat_field = find_field(
        fields,
        [
            "latitud",
            "latitude",
            "lat",
            "latitud sitio"
        ]
    )

    lon_field = find_field(
        fields,
        [
            "longitud",
            "longitude",
            "lon",
            "lng",
            "longitud sitio"
        ]
    )

    operator_field = find_field(
        fields,
        [
            "operador",
            "proveedor",
            "prstm",
            "nombre proveedor",
            "empresa"
        ]
    )

    municipality_field = find_field(
        fields,
        [
            "municipio",
            "municipality"
        ]
    )

    technology_field = find_field(
        fields,
        [
            "tecnologia",
            "technology",
            "red",
            "technology"
        ]
    )

    print("[INFO] Campos detectados:")
    print("  latitude :", lat_field)
    print("  longitude:", lon_field)
    print("  operator :", operator_field)
    print("  municipio:", municipality_field)
    print("  tecnologia:", technology_field)

    conn = sqlite3.connect(DB_FILE)

    conn.execute("""
        CREATE TABLE IF NOT EXISTS infrastructure (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source TEXT,
            operator TEXT,
            municipality TEXT,
            technology TEXT,
            latitude REAL,
            longitude REAL,
            raw_json TEXT
        )
    """)

    conn.execute("""
        CREATE INDEX IF NOT EXISTS idx_infrastructure_municipality
        ON infrastructure(municipality)
    """)

    conn.execute("""
        CREATE INDEX IF NOT EXISTS idx_infrastructure_technology
        ON infrastructure(technology)
    """)

    conn.execute("""
        CREATE INDEX IF NOT EXISTS idx_infrastructure_operator
        ON infrastructure(operator)
    """)

    features = []
    count = 0

    for row in reader:
        def value(field):
            if not field:
                return ""
            return (row.get(field) or "").strip()

        lat_raw = value(lat_field)
        lon_raw = value(lon_field)

        try:
            lat = float(lat_raw.replace(",", "."))
            lon = float(lon_raw.replace(",", "."))
        except Exception:
            continue

        operator = value(operator_field)
        municipality = value(municipality_field)
        technology = value(technology_field)

        conn.execute(
            """
            INSERT INTO infrastructure
            (source, operator, municipality, technology,
             latitude, longitude, raw_json)
            VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            (
                "Datos Abiertos Colombia",
                operator,
                municipality,
                technology,
                lat,
                lon,
                json.dumps(row, ensure_ascii=False)
            )
        )

        features.append({
            "type": "Feature",
            "geometry": {
                "type": "Point",
                "coordinates": [lon, lat]
            },
            "properties": {
                "operator": operator,
                "municipality": municipality,
                "technology": technology,
                "source": "Datos Abiertos Colombia"
            }
        })

        count += 1

    conn.commit()
    conn.close()

geojson = {
    "type": "FeatureCollection",
    "name": "ChronoNet Colombia",
    "features": features
}

GEOJSON.write_text(
    json.dumps(geojson, ensure_ascii=False),
    encoding="utf-8"
)

print(f"[OK] Registros importados: {count}")
print(f"[OK] SQLite: {DB_FILE}")
print(f"[OK] GeoJSON: {GEOJSON}")

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

echo
echo "============================================================"
echo " CHRONOOS GIS VALIDATION"
echo "============================================================"

echo "[1/7] Source catalog..."
test -f "$ROOT/data/catalog/sources.json"
echo "[OK] Source catalog"

echo "[2/7] Infrastructure schema..."
test -f "$ROOT/data/catalog/infrastructure.schema.json"
echo "[OK] Infrastructure schema"

echo "[3/7] Colombia dataset..."
test -f "$ROOT/data/normalized/colombia/sites.json"
echo "[OK] Colombia dataset"

echo "[4/7] Brasil dataset..."
test -f "$ROOT/data/normalized/brasil/sites.json"
echo "[OK] Brasil dataset"

echo "[5/7] Colombia Digital Twin..."
test -f "$ROOT/data/digital-twin/colombia.json"
echo "[OK] Colombia Digital Twin"

echo "[6/7] Brasil Digital Twin..."
test -f "$ROOT/data/digital-twin/brasil.json"
echo "[OK] Brasil Digital Twin"

echo "[7/7] GIS web interface..."
test -f "$ROOT/gis/web/index.html"
echo "[OK] GIS web interface"

echo
echo "============================================================"
echo " GIS VALIDATION: PASS"
echo "============================================================"

#!/data/data/com.termux/files/usr/bin/bash
set -e

cd ~/chrono-os

BASE="https://www.postdata.gov.co"
MAP="$BASE/mapa/sitios-de-infraestructura-movil"

mkdir -p data/raw/colombia
mkdir -p data/catalog

echo "============================================================"
echo " CHRONOOS — COLOMBIA GIS REAL"
echo "============================================================"

rm -f data/raw/colombia/colombia_mobile_infrastructure.csv
rm -f data/raw/colombia/mobile_infrastructure.csv

echo "[1/5] Consultando mapa oficial CRC/Postdata..."

curl -L --fail --retry 3 \
  -A "Mozilla/5.0 ChronoOS-GIS/1.2.0" \
  "$MAP" \
  -o data/raw/colombia/postdata_map.html

test -s data/raw/colombia/postdata_map.html

echo "[OK] Página oficial obtenida"

echo
echo "[2/5] Extrayendo referencias geográficas..."

grep -Eoi \
'https?[^"'\'' <>]+' \
data/raw/colombia/postdata_map.html \
| grep -Ei \
'geo|json|csv|arcgis|feature|service|map|wfs|wms' \
| sort -u \
> data/raw/colombia/discovered_endpoints.txt || true

echo
echo "===== ENDPOINTS DESCUBIERTOS ====="

cat data/raw/colombia/discovered_endpoints.txt || true

echo
echo "[3/5] Buscando recursos embebidos..."

grep -Ei \
'iframe|arcgis|featureServer|mapServer|geojson|wfs|wms|json|csv' \
data/raw/colombia/postdata_map.html \
> data/raw/colombia/map_references.txt || true

echo "[OK] Referencias extraídas"

echo
echo "[4/5] Guardando procedencia..."

python3 - <<'PY'
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

p = Path("data/raw/colombia/postdata_map.html")

sha = hashlib.sha256(
    p.read_bytes()
).hexdigest()

obj = {
    "schema": "chrono.gis.provenance",
    "country": "CO",
    "authority": "CRC / Postdata",
    "dataset": "Sitios de infraestructura móvil",
    "source": "https://www.postdata.gov.co/mapa/sitios-de-infraestructura-movil",
    "retrieved_at": datetime.now(
        timezone.utc
    ).isoformat(),
    "sha256": sha,
    "synthetic": False,
    "demo": False
}

Path(
    "data/catalog/colombia-gis-provenance.json"
).write_text(
    json.dumps(
        obj,
        indent=2,
        ensure_ascii=False
    ),
    encoding="utf-8"
)

print("[OK] Provenance:")
print(sha)
PY

echo
echo "[5/5] Validación..."

SIZE=$(wc -c < data/raw/colombia/postdata_map.html)

if [ "$SIZE" -lt 1000 ]; then
    echo "[ERROR] Respuesta demasiado pequeña."
    exit 1
fi

echo
echo "============================================================"
echo " FUENTE OFICIAL COLOMBIA OBTENIDA"
echo "============================================================"

echo
echo "Archivo:"
echo "data/raw/colombia/postdata_map.html"

echo
echo "Tamaño:"
echo "$SIZE bytes"

echo
echo "Ahora revisamos las referencias que contiene el mapa:"
echo

cat data/raw/colombia/map_references.txt || true

echo
echo "============================================================"
echo " NO DEMO"
echo " NO COORDENADAS INVENTADAS"
echo " NO COMMIT"
echo " NO PUSH"
echo "============================================================"

#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

cd ~/chrono-os

mkdir -p \
  data/raw/brasil \
  data/normalized/brasil \
  data/catalog \
  data/digital-twin/brasil

echo "============================================================"
echo " CHRONOOS 1.2.0 — ANATEL REAL DATA"
echo " BRASIL / ERB / SMP"
echo "============================================================"

echo
echo "[1/6] Consultando catálogo oficial de Datos Abiertos..."

curl -L --fail --retry 3 \
  -A "ChronoOS-GIS/1.2.0" \
  "https://dados.gov.br/dados/conjuntos-dados/outorga-e-licenciamento---estaes-licenciadas" \
  -o data/raw/brasil/dados_abertos_anatel.html

test -s data/raw/brasil/dados_abertos_anatel.html

echo "[OK] Catálogo obtenido"

echo
echo "[2/6] Localizando recursos CSV..."

grep -Eoi \
'https?[^" <]+' \
data/raw/brasil/dados_abertos_anatel.html \
| grep -Ei \
'csv|download|recurso|arquivo|estacoes' \
| sort -u \
> data/raw/brasil/csv_resources.txt || true

echo
echo "===== RECURSOS ENCONTRADOS ====="
cat data/raw/brasil/csv_resources.txt || true

echo
echo "[3/6] Guardando referencia oficial..."

cat > data/catalog/brasil-anatel-smp.json <<'JSON'
{
  "schema": "chrono.gis.source",
  "country": "BR",
  "authority": "ANATEL",
  "dataset": "Outorga e Licenciamento - Estações Licenciadas",
  "target_resource": "Estações do Serviço Móvel Pessoal - SMP",
  "source_portal": "https://dados.gov.br/dados/conjuntos-dados/outorga-e-licenciamento---estaes-licenciadas",
  "official_station_page": "https://www.gov.br/anatel/pt-br/regulado/outorga/telefonia-movel/lista-de-estacoes",
  "official_panel": "https://informacoes.anatel.gov.br/paineis/outorga-e-licenciamento/estacoes-do-smp",
  "synthetic": false,
  "demo": false
}
JSON

echo "[OK] Metadata registrada"

echo
echo "[4/6] Extrayendo posibles enlaces de recursos..."

python3 - <<'PY'
from pathlib import Path
import re

html = Path(
    "data/raw/brasil/dados_abertos_anatel.html"
).read_text(
    encoding="utf-8",
    errors="ignore"
)

patterns = [
    r'https?://[^"\']+',
    r'//[^"\']+'
]

urls = set()

for pattern in patterns:
    for u in re.findall(pattern, html):
        if any(x in u.lower() for x in [
            "csv",
            "download",
            "recurso",
            "resource",
            "estacao",
            "estação"
        ]):
            urls.add(u)

Path(
    "data/raw/brasil/resource_candidates.txt"
).write_text(
    "\n".join(sorted(urls)),
    encoding="utf-8"
)

print(f"[OK] {len(urls)} candidatos encontrados")
PY

echo
echo "===== CANDIDATOS ====="
cat data/raw/brasil/resource_candidates.txt || true

echo
echo "[5/6] Validando que estamos usando fuente oficial..."

grep -qi "Estações do Serviço Móvel Pessoal" \
  data/raw/brasil/dados_abertos_anatel.html

grep -qi "CSV" \
  data/raw/brasil/dados_abertos_anatel.html

echo "[PASS] Dataset SMP encontrado"
echo "[PASS] Formato CSV confirmado"

echo
echo "[6/6] Preparando estructura ChronoGIS..."

cat > data/digital-twin/brasil/schema.json <<'JSON'
{
  "schema": "chrono.infrastructure.digital_twin.v1",
  "country": "BR",
  "source_authority": "ANATEL",
  "entity": "mobile_station",
  "fields": [
    "site_id",
    "station_id",
    "latitude",
    "longitude",
    "state",
    "municipality",
    "operator",
    "technology",
    "frequency",
    "status",
    "license",
    "source",
    "retrieved_at"
  ]
}
JSON

echo "[OK] Digital Twin Brasil preparado"

echo
echo "============================================================"
echo " ANATEL — FUENTE REAL LOCALIZADA"
echo "============================================================"

echo
echo "Dataset:"
echo "Estações do Serviço Móvel Pessoal - SMP"

echo
echo "Fuente:"
echo "Portal Brasileiro de Dados Abertos"

echo
echo "IMPORTANTE:"
echo "Todavía NO se genera ningún punto artificial."
echo "Todavía NO se inventan coordenadas."
echo "El próximo paso descarga el CSV real del recurso SMP."

echo
echo "Archivos:"
find data/raw/brasil data/catalog data/digital-twin/brasil \
  -maxdepth 1 -type f -print | sort

echo
echo "============================================================"

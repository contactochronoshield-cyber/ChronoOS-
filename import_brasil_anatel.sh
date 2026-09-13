#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

cd ~/chrono-os

mkdir -p \
  data/raw/brasil \
  data/normalized/brasil \
  data/catalog \
  data/digital-twin/brasil

echo "============================================================"
echo " CHRONOOS 1.2.0 — BRASIL / ANATEL"
echo " INFRAESTRUCTURA REAL"
echo "============================================================"

echo
echo "[1/5] Registrando fuentes oficiales..."

cat > data/catalog/brasil-anatel.json <<'JSON'
{
  "country": "BR",
  "authority": "ANATEL",
  "datasets": [
    {
      "name": "Estações Licenciadas",
      "url": "https://www.gov.br/anatel/pt-br/dados/dados-abertos"
    },
    {
      "name": "Estações do Serviço Móvel Pessoal - SMP",
      "url": "https://www.gov.br/anatel/pt-br/regulado/outorga/telefonia-movel/lista-de-estacoes"
    },
    {
      "name": "Estações Rádio Base",
      "url": "https://www.gov.br/anatel/pt-br/regulado/outorga/telefonia-movel/estacoes-radio-base"
    }
  ],
  "synthetic": false,
  "demo": false
}
JSON

echo "[OK] Fuentes ANATEL registradas"

echo
echo "[2/5] Consultando fuente oficial..."

curl -L --fail --retry 3 \
  -A "ChronoOS/1.2.0" \
  "https://www.gov.br/anatel/pt-br/regulado/outorga/telefonia-movel/lista-de-estacoes" \
  -o data/raw/brasil/anatel_estacoes.html

test -s data/raw/brasil/anatel_estacoes.html

echo "[OK] ANATEL respondió"

echo
echo "[3/5] Buscando referencias al sistema de estaciones..."

grep -Eoi \
'https?[^" <]+' \
data/raw/brasil/anatel_estacoes.html \
| grep -Ei \
'csv|xlsx|xls|download|estac|mosaico|painel|dados' \
| sort -u \
> data/raw/brasil/discovered_sources.txt || true

echo
echo "===== REFERENCIAS ANATEL ====="
cat data/raw/brasil/discovered_sources.txt || true

echo
echo "[4/5] Guardando proveniencia..."

python3 - <<'PY'
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

p = Path("data/raw/brasil/anatel_estacoes.html")

obj = {
    "schema": "chrono.gis.provenance",
    "country": "BR",
    "authority": "ANATEL",
    "dataset": "Estações do Serviço Móvel Pessoal",
    "source": "ANATEL",
    "retrieved_at": datetime.now(
        timezone.utc
    ).isoformat(),
    "sha256": hashlib.sha256(
        p.read_bytes()
    ).hexdigest(),
    "synthetic": False,
    "demo": False
}

Path(
    "data/catalog/brasil-provenance.json"
).write_text(
    json.dumps(
        obj,
        indent=2,
        ensure_ascii=False
    ),
    encoding="utf-8"
)

print("[OK] Provenance Brasil registrada")
print("[OK] SHA256:", obj["sha256"])
PY

echo
echo "[5/5] Validación..."

grep -qi "Estações" \
  data/raw/brasil/anatel_estacoes.html

echo "[PASS] Fuente ANATEL validada"

echo
echo "============================================================"
echo " BRASIL — FUENTE OFICIAL VALIDADA"
echo "============================================================"

echo
echo "NO DEMO"
echo "NO COORDENADAS INVENTADAS"
echo "NO DATOS FALSOS"
echo
echo "Archivo fuente:"
echo "data/raw/brasil/anatel_estacoes.html"

echo
echo "Siguiente etapa:"
echo "extraer el recurso geográfico/descarga de ANATEL"
echo "y convertir las ERB reales en Digital Twins."

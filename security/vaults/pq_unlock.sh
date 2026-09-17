#!/bin/bash
set -euo pipefail
umask 077

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
KEY_FILE="$CHRONO_ROOT/security/auth/master.key"

die() {
    echo "[ChronoSec][ERROR] $*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "Comando requerido no disponible: $1"
}

require_command openssl
require_command od
require_command dd
require_command stat
require_command mktemp
require_command cmp
require_command mv
require_command rm

INPUT_FILE="${1:-}"
OUTPUT_FILE="${2:-}"

[ -n "$INPUT_FILE" ] || die "Uso: pq_unlock.sh <archivo.enc> <archivo_salida>"
[ -n "$OUTPUT_FILE" ] || die "Uso: pq_unlock.sh <archivo.enc> <archivo_salida>"

[ -f "$INPUT_FILE" ] || die "Archivo cifrado no encontrado: $INPUT_FILE"
[ -f "$KEY_FILE" ] || die "Clave maestra no encontrada: $KEY_FILE"

KEY_MODE="$(stat -c '%a' "$KEY_FILE" 2>/dev/null || true)"
KEY_SIZE="$(stat -c '%s' "$KEY_FILE" 2>/dev/null || true)"

[ "$KEY_MODE" = "600" ] || die "Permisos inseguros para master.key: $KEY_MODE"
[ "$KEY_SIZE" = "32" ] || die "master.key debe tener exactamente 32 bytes"

[ ! -e "$OUTPUT_FILE" ] || die "El archivo de salida ya existe: $OUTPUT_FILE"

TOTAL_SIZE="$(stat -c '%s' "$INPUT_FILE")"

# El archivo debe contener al menos:
# cabecera + IV + ciphertext + HMAC(32 bytes)
[ "$TOTAL_SIZE" -gt 64 ] || die "Archivo CHRONO-VAULT demasiado pequeño"

TMP_DIR="$(mktemp -d ".chrono-unlock.XXXXXX")"

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

PAYLOAD="$TMP_DIR/payload"
TAG="$TMP_DIR/tag"
CIPHERTEXT="$TMP_DIR/ciphertext"
PLAINTEXT="$TMP_DIR/plaintext"

PAYLOAD_SIZE=$((TOTAL_SIZE - 32))

# Separar payload y HMAC.
dd if="$INPUT_FILE" \
   of="$PAYLOAD" \
   bs=1 \
   count="$PAYLOAD_SIZE" \
   2>/dev/null

dd if="$INPUT_FILE" \
   of="$TAG" \
   bs=1 \
   skip="$PAYLOAD_SIZE" \
   count=32 \
   2>/dev/null

# Cargar la clave maestra sin imprimirla.
MASTER_HEX="$(od -An -tx1 -v "$KEY_FILE" | tr -d '[:space:]')"

[ "${#MASTER_HEX}" = "64" ] || die "Formato inesperado de master.key"

# Derivación de claves separadas.
ENC_KEY_HEX="$(
    printf '%s' "chrono-vault-encryption-v1" |
    openssl dgst -sha256 \
        -mac HMAC \
        -macopt "hexkey:$MASTER_HEX" \
        -binary |
    od -An -tx1 -v |
    tr -d '[:space:]'
)"

MAC_KEY_HEX="$(
    printf '%s' "chrono-vault-authentication-v1" |
    openssl dgst -sha256 \
        -mac HMAC \
        -macopt "hexkey:$MASTER_HEX" \
        -binary |
    od -An -tx1 -v |
    tr -d '[:space:]'
)"

echo "[ChronoSec] Verificando integridad..."

openssl dgst \
    -sha256 \
    -mac HMAC \
    -macopt "hexkey:$MAC_KEY_HEX" \
    -binary \
    "$PAYLOAD" |
cmp -s - "$TAG" || die "INTEGRIDAD INVALIDA: archivo manipulado o clave incorrecta"

# Leer y validar cabecera.
HEADER="$(head -n 1 "$PAYLOAD")"
[ "$HEADER" = "CHRONO-VAULT-V1" ] || die "Formato CHRONO-VAULT desconocido"

# El IV está en la segunda línea.
IV_HEX="$(sed -n '2p' "$PAYLOAD")"

printf '%s' "$IV_HEX" | grep -Eq '^[0-9a-fA-F]{32}$' ||
    die "IV inválido"

# Extraer ciphertext después de las dos líneas de cabecera.
HEADER_BYTES=$(( ${#HEADER} + 1 + 32 + 1 ))

CIPHER_SIZE=$((PAYLOAD_SIZE - HEADER_BYTES))

[ "$CIPHER_SIZE" -gt 0 ] || die "Ciphertext vacío"

dd if="$PAYLOAD" \
   of="$CIPHERTEXT" \
   bs=1 \
   skip="$HEADER_BYTES" \
   count="$CIPHER_SIZE" \
   2>/dev/null

echo "[ChronoSec] Descifrando..."

openssl enc \
    -d \
    -aes-256-ctr \
    -K "$ENC_KEY_HEX" \
    -iv "$IV_HEX" \
    -in "$CIPHERTEXT" \
    -out "$PLAINTEXT"

[ -s "$PLAINTEXT" ] || die "Descifrado produjo un archivo vacío"

mv "$PLAINTEXT" "$OUTPUT_FILE"
chmod 600 "$OUTPUT_FILE"

echo "[ChronoSec] Descifrado correcto."
echo "[ChronoSec] Integridad: VALID"
echo "[ChronoSec] Formato: CHRONO-VAULT-V1"
echo "[ChronoSec] Salida: $OUTPUT_FILE"

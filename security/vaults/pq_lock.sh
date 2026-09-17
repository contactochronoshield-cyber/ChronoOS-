#!/bin/bash
set -euo pipefail
umask 077

CHRONO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
VAULT_PATH="${VAULT_PATH:-/vault/vaults}"
KEY_FILE="$CHRONO_ROOT/security/auth/master.key"

INPUT_FILE="$VAULT_PATH/core_data.db"
OUTPUT_FILE="$VAULT_PATH/core_data.enc"

die() {
    echo "[ChronoSec][ERROR] $*" >&2
    exit 1
}

require_command() {
    command -v "$1" >/dev/null 2>&1 || die "Comando requerido no disponible: $1"
}

require_command openssl
require_command od
require_command awk
require_command dd
require_command mv
require_command rm

[ -f "$KEY_FILE" ] || die "Clave maestra no encontrada: $KEY_FILE"
[ -f "$INPUT_FILE" ] || die "Bóveda no encontrada: $INPUT_FILE"

KEY_MODE="$(stat -c '%a' "$KEY_FILE" 2>/dev/null || true)"
KEY_SIZE="$(stat -c '%s' "$KEY_FILE" 2>/dev/null || true)"

[ "$KEY_MODE" = "600" ] || die "Permisos inseguros para master.key: $KEY_MODE"
[ "$KEY_SIZE" = "32" ] || die "master.key debe tener exactamente 32 bytes"

[ ! -e "$OUTPUT_FILE" ] || die "El archivo de salida ya existe: $OUTPUT_FILE"

TMP_DIR="$(mktemp -d "$VAULT_PATH/.chrono-lock.XXXXXX")"
cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM

PLAINTEXT="$TMP_DIR/plaintext"
CIPHERTEXT="$TMP_DIR/ciphertext"
PAYLOAD="$TMP_DIR/payload"
TAG="$TMP_DIR/tag"
FINAL_TMP="$TMP_DIR/core_data.enc"

# Copia controlada del original.
cp "$INPUT_FILE" "$PLAINTEXT"
chmod 600 "$PLAINTEXT"

# Cargar la clave maestra sin imprimirla.
MASTER_HEX="$(od -An -tx1 -v "$KEY_FILE" | tr -d '[:space:]')"

[ "${#MASTER_HEX}" = "64" ] || die "Formato inesperado de master.key"

# Derivación de claves separadas.
ENC_KEY_HEX="$(
    printf '%s' "chrono-vault-encryption-v1" |
    openssl dgst -sha256 -mac HMAC -macopt "hexkey:$MASTER_HEX" -binary |
    od -An -tx1 -v |
    tr -d '[:space:]'
)"

MAC_KEY_HEX="$(
    printf '%s' "chrono-vault-authentication-v1" |
    openssl dgst -sha256 -mac HMAC -macopt "hexkey:$MASTER_HEX" -binary |
    od -An -tx1 -v |
    tr -d '[:space:]'
)"

# IV aleatorio de 128 bits para AES-256-CTR.
IV_HEX="$(
    openssl rand -hex 16
)"

echo "[ChronoSec] Aplicando blindaje criptográfico..."

# Cifrado.
openssl enc \
    -aes-256-ctr \
    -K "$ENC_KEY_HEX" \
    -iv "$IV_HEX" \
    -in "$PLAINTEXT" \
    -out "$CIPHERTEXT"

# Formato:
# CHRONO-VAULT-V1
# IV hexadecimal
# ciphertext binario
printf '%s\n' "CHRONO-VAULT-V1" > "$PAYLOAD"
printf '%s\n' "$IV_HEX" >> "$PAYLOAD"
cat "$CIPHERTEXT" >> "$PAYLOAD"

# HMAC sobre todo el payload.
openssl dgst \
    -sha256 \
    -mac HMAC \
    -macopt "hexkey:$MAC_KEY_HEX" \
    -binary \
    "$PAYLOAD" > "$TAG"

# Construir salida final.
cat "$PAYLOAD" "$TAG" > "$FINAL_TMP"
chmod 600 "$FINAL_TMP"

# Verificación básica antes de instalar el resultado.
[ -s "$FINAL_TMP" ] || die "El archivo cifrado quedó vacío"

openssl dgst \
    -sha256 \
    -mac HMAC \
    -macopt "hexkey:$MAC_KEY_HEX" \
    -binary \
    "$PAYLOAD" |
cmp -s - "$TAG" || die "Falló la verificación HMAC"

mv "$FINAL_TMP" "$OUTPUT_FILE"
chmod 600 "$OUTPUT_FILE"

# Solo después de crear y verificar el archivo cifrado,
# eliminar el original.
shred -u "$INPUT_FILE"

echo "[ChronoSec] Bóveda cifrada correctamente."
echo "[ChronoSec] Archivo: $OUTPUT_FILE"
echo "[ChronoSec] Integridad: HMAC-SHA-256"
echo "[ChronoSec] Cifrado: AES-256-CTR"
echo "[ChronoSec] Clave: security/auth/master.key"

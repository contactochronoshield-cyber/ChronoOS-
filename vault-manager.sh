#!/bin/sh
# ==============================================================================
# ChronoOS Vault Manager: AES-256 Cryptographic Storage Subsystem
# ==============================================================================

ACTION="$1"
VAULT_NAME="$2"
VAULT_DIR="./vault"

if [ -z "$ACTION" ] || [ -z "$VAULT_NAME" ]; then
    echo "Uso: $sh vault-manager.sh [encrypt|decrypt] <nombre_archivo>"
    exit 1
fi

TARGET_FILE="$VAULT_DIR/$VAULT_NAME"

if [ "$ACTION" = "encrypt" ]; then
    if [ ! -f "$TARGET_FILE" ]; then
        echo "[!] Error: El archivo fuente no existe en $TARGET_FILE"
        exit 1
    echo "[*] Cifrando bóveda con AES-256-CBC..."
    openssl enc -aes-256-cbc -salt -in "$TARGET_FILE" -out "${TARGET_FILE}.chrono"
    rm -f "$TARGET_FILE"
    echo "[✓] Bóveda cifrada con éxito: ${TARGET_FILE}.chrono"
    
elif [ "$ACTION" = "decrypt" ]; then
    ENCRYPTED_FILE="${TARGET_FILE}.chrono"
    if [ ! -f "$ENCRYPTED_FILE" ]; then
        echo "[!] Error: La bóveda cifrada no existe en $ENCRYPTED_FILE"
        exit 1
    fi
    echo "[*] Descifrando bóveda..."
    openssl enc -d -aes-256-cbc -in "$ENCRYPTED_FILE" -out "$TARGET_FILE"
    echo "[✓] Bóveda descifrada y restaurada en: $TARGET_FILE"
else
    echo "[!] Acción desconocida. Use 'encrypt' o 'decrypt'."
    exit 1
fi

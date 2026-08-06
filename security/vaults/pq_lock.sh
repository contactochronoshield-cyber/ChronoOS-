#!/bin/bash
# Cifrado de alta seguridad para bóvedas en modo aislamiento
VAULT_PATH="/vault/vaults"
KEY_FILE="/crypto_keys/master.key"

echo "[ChronoSec] Aplicando blindaje criptográfico post-aislamiento a las bóvedas..."
openssl enc -aes-256-gcm -salt -in "$VAULT_PATH/core_data.db" -out "$VAULT_PATH/core_data.enc" -pass file:"$KEY_FILE"
shred -u "$VAULT_PATH/core_data.db"
echo "[ChronoSec] Datos protegidos contra inspección física y extracciones forzosas."

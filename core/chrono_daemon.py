#!/usr/bin/env python3
import os
import hashlib
import json
from datetime import datetime

VAULT_DIR = os.path.expanduser("~/chrono-os/vault_store")
LOG_PATH = os.path.expanduser("~/.chrono/chrono_sovereign.log")

def audit_vault():
    print("[*] Iniciando auditoría criptográfica de la Bóveda (Chrono Vault)...")
    if not os.path.exists(VAULT_DIR):
        print("[-] Directorio de bóveda no encontrado.")
        return
    
    report = []
    for filename in os.listdir(VAULT_DIR):
        if filename.endswith(".enc"):
            filepath = os.path.join(VAULT_DIR, filename)
            sha_file = f"{filepath}.sha256"
            
            # Calcular hash actual
            sha256_hash = hashlib.sha256()
            with open(filepath, "rb") as f:
                for byte_block in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(byte_block)
            current_hash = sha256_hash.hexdigest()
            
            status = "VERIFIED_OK"
            if os.path.exists(sha_file):
                with open(sha_file, "r") as sf:
                    stored_hash = sf.read().split()[0]
                if current_hash != stored_hash:
                    status = "CORRUPTION_DETECTED"
            else:
                status = "MISSING_SIGNATURE"
                
            report.append({"file": filename, "status": status, "hash": current_hash})
    
    os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
    with open(LOG_PATH, "a") as log:
        log.write(json.dumps({"timestamp": str(datetime.now()), "event": "vault_audit", "results": report}) + "\n")
    
    print(json.dumps(report, indent=4))
    print("[✔] Auditoría completada y registrada en los logs del sistema.")

if __name__ == "__main__":
    audit_vault()


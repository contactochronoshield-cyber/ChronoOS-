#!/usr/bin/env bash
# ==============================================================================
# ChronoOS Real P2P Mesh Network Orchestrator
# ==============================================================================

INTERFACE="chronomesh"
PORT="51820"

echo "=== ORQUESTADOR DE RED MESH DISTRIBUIDA ==="

if ! command -v wg &> /dev/null; then
    echo "[!] WireGuard no está instalado en el sistema base."
    exit 1
fi

# Generar llaves locales si no existen
KEY_DIR="$HOME/.chrono/keys"
mkdir -p "$KEY_DIR"

if [ ! -f "$KEY_DIR/private.key" ]; then
    echo "[*] Generando par de llaves criptográficas Ed25519/WireGuard..."
    wg genkey | tee "$KEY_DIR/private.key" | wg pubkey > "$KEY_DIR/public.key"
    chmod 600 "$KEY_DIR/private.key"
fi

LOCAL_PUB=$(cat "$KEY_DIR/public.key")
echo "[✔] Llave pública del nodo local: $LOCAL_PUB"

# Configurar interfaz virtual mesh si es root o con ip link
sudo ip link delete "$INTERFACE" 2>/dev/null || true
sudo ip link add dev "$INTERFACE" type wireguard
sudo ip addr add 10.50.0.1/24 dev "$INTERFACE"
sudo wg set "$INTERFACE" listen-port "$PORT" private-key "$KEY_DIR/private.key"
sudo ip link set up dev "$INTERFACE"

echo "[✔] Túnel Mesh Real '$INTERFACE' levantado exitosamente en IP 10.50.0.1/24"

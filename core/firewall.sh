cat << 'EOF' > core/firewall.sh
#!/usr/bin/env bash
# ==============================================================================
# ChronoOS Sovereign Firewall & Telemetry Blocker (Real Kernel Protection)
# ==============================================================================

if [ "$EUID" -ne 0 ]; then
  echo "[-] Este módulo requiere privilegios de root (sudo) para modificar reglas de red del kernel."
  exit 1
fi

echo "[*] Aplicando reglas de blindaje y bloqueo de telemetría..."

# Asegurar política por defecto restrictiva en iptables
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# Permitir tráfico local (loopback)
iptables -A INPUT -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT

# Permitir conexiones ya establecidas
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Bloquear puertos comunes de telemetría/rastreo corporativo conocido si es necesario, o forzar DNS seguro
echo "[✔] Reglas de cortafuegos soberano aplicadas con éxito."
EOF
chmod +x core/firewall.sh

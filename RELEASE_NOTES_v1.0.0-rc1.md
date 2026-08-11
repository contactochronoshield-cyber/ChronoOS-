# ChronoOS v1.0.0-rc1

Release candidate. Boot confirmado en QEMU (aarch64). Pendiente validacion
en hardware fisico antes de v1.0.0 definitivo.

## Incluido en esta version

### Boot y kernel
- `chrono_init` como PID 1 real, compilado estatico (Zig/musl aarch64)
- Boot confirmado en QEMU con kernel Linux + rootfs Alpine
- Almacenamiento persistente (ext4) que sobrevive reinicios
- Servicio propio (heartbeat) lanzado automaticamente al arrancar

### Seguridad y criptografia
- Cifrado AES-256-GCM real (`chrono-core encrypt/decrypt`)
- Protocolo de panico con crypto-shred de llave + overwrite defensivo multi-pass
- Purga de memoria escalable (mlockall + registro dinamico de buffers)
- Dead man's switch por inactividad e intentos fallidos (`chrono-watchdog`)

### Integridad y auditoria
- Atestacion offline de arranque (hash verificable sin internet)
- Pin Forge: arbol Merkle real + firma Ed25519 (`chrono-forge` / `chrono-verify`)
- Ledger encadenado criptograficamente - chain of custody local

### Hardware y red
- Hardware guard: deteccion USB/serial/storage con huella digital
- Power shield: monitoreo real de energia via sysfs
- Network audit: deteccion de credenciales por defecto, RTSP expuesto, telnet

### Modulos industriales
- Modbus Guard: proxy TCP con whitelist de rangos seguros para PLCs
- Print Guard: puente hacia impresoras Klipper/Moonraker (pendiente hardware real)

### Conjuntos residenciales
- Alerta de emergencia via mesh local (sin depender de internet/celular)
- Control de acceso temporal para visitantes
- LPR guard (logica de decision; OCR se integra como pieza externa)

### Transferencia segura
- Airgap send/receive via codigos QR (sin USB, sin red)

### Interfaz
- CLI unificado `chrono` con subcomandos para toda la suite
- `chrono doctor`: selftest completo del sistema

## Pendiente para v1.0.0 definitivo
- [ ] Validacion en hardware fisico real (no solo QEMU)
- [ ] Al menos un modulo probado con dispositivo externo real (PLC/impresora)
- [ ] Rotacion de master.key de produccion
- [ ] Autenticacion entre nodos en Community Alert (actualmente sin auth)

## Advertencia de honestidad tecnica
Este es software en fase alpha/rc temprana. Los modulos de Modbus, impresion 3D
y LPR estan construidos contra especificaciones publicas pero sin hardware real
de prueba - la primera integracion puede requerir ajustes.

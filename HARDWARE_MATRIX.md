# ChronoOS - Matriz de Compatibilidad de Hardware

| Componente | Termux/Android | Linux x86 | QEMU ARM64 | Beelink N100 | RPi 4 |
|---|---|---|---|---|---|
| chrono_init (PID 1) | Simulacion | ✅ | ✅ Probado | ✅ | ✅ |
| chrono-core (AES-256) | ✅ Probado | ✅ | ✅ | ✅ | ✅ |
| chrono-ledger | ✅ Probado | ✅ | ✅ | ✅ | ✅ |
| chrono-panic | ✅ Probado | ✅ | ✅ | ✅ | ✅ |
| chrono-iot-engine | ✅ Probado | ✅ | ✅ | ✅ | ✅ |
| Modbus Guard | Simulacion | ✅ | ✅ | ✅ | ✅ |
| CAN Bus Guard | Simulacion | ✅ | ✅ | ✅ | Hardware req. |
| TPM 2.0 | ❌ No | ❌ No | ❌ No | ✅ Hardware | Dep. modelo |
| 5G / Open5GS | ❌ No | ✅ | ❌ No | ✅ | ❌ No |
| LoRa Gateway | ❌ No | ✅ | ❌ No | ✅ | ✅ |
| Hardware Guard | ✅ Parcial | ✅ | ✅ | ✅ | ✅ |
| Community Alert | ✅ Probado | ✅ | ✅ | ✅ | ✅ |
| Airgap QR | ✅ Probado | ✅ | N/A | ✅ | ✅ |

**Leyenda:**
- ✅ Probado: funcionamiento verificado
- ✅ Simulacion: corre pero sin hardware real
- Hardware req.: requiere hardware adicional especifico
- ❌ No: no compatible con esta plataforma

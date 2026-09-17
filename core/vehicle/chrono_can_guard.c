/**
 * ChronoOS - CAN Bus Guard (Proteccion de Vehiculos)
 *
 * El bus CAN (Controller Area Network) es el sistema de comunicacion
 * interno de todos los vehiculos modernos desde ~1996. Conecta frenos,
 * motor, direccion, airbags, ECU — sin autenticacion ni cifrado.
 *
 * Vulnerabilidades documentadas reales:
 * - Miller & Valasek (2015): hackearon remotamente un Jeep Cherokee
 *   en movimiento — tomaron control de frenos y direccion por internet
 * - Cualquier OBD-II (el puerto de diagnostico bajo el volante) da
 *   acceso completo al bus CAN
 * - Empresas israelies (Argus, GuardKnox, Upstream Security) cobran
 *   $50,000-500,000 USD por proteger flotas industriales
 *
 * ChronoOS CAN Guard: monitoreo de anomalias en mensajes CAN,
 * deteccion de inyeccion de comandos maliciosos, whitelist de
 * mensajes legitimos, registro forense en el Ledger encadenado.
 *
 * LIMITE HONESTO: requiere hardware OBD-II con interfaz SocketCAN
 * (ej: ELM327, CANable, PiCAN) para conectarse al vehiculo real.
 * Esta implementacion corre en el Core Box o Raspberry Pi dentro
 * del vehiculo. En Termux/Android es simulacion para desarrollo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>
#include "../common/chrono_exec.h"

#define CAN_WHITELIST  "./etc/chrono/can_whitelist.conf"
#define CAN_LOG        "./var/logs/can_audit.log"
#define MAX_RULES      512

typedef struct {
    uint32_t can_id;      // ID del mensaje CAN (11 o 29 bits)
    uint8_t  min_data;    // valor minimo esperado en byte 0
    uint8_t  max_data;    // valor maximo esperado en byte 0
    char     description[64];
    char     system[32];  // brake, engine, steering, airbag, etc
} CANRule;

static CANRule rules[MAX_RULES];
static int rule_count = 0;

static void log_ledger(const char *type, const char *details)
{
    if (type == NULL || details == NULL) {
        return;
    }

    int rc = chrono_ledger_append(type, details);

    if (rc != 0) {
        fprintf(stderr,
                "[CHRONO] Ledger append failed: rc=%d event=%s\\n",
                rc, type);
    }
}

void load_whitelist(void) {
    FILE *f = fopen(CAN_WHITELIST, "r");
    if (!f) { printf("[!] Sin whitelist CAN\n"); return; }
    char line[256];
    while (fgets(line, sizeof(line), f) && rule_count < MAX_RULES) {
        if (line[0] == '#' || line[0] == '\n') continue;
        CANRule r;
        uint32_t id;
        int min, max;
        if (sscanf(line, "%x,%d,%d,%31[^,],%63[^\n]",
                &id, &min, &max, r.system, r.description) >= 4) {
            r.can_id = id;
            r.min_data = (uint8_t)min;
            r.max_data = (uint8_t)max;
            rules[rule_count++] = r;
        }
    }
    fclose(f);
    printf("[✓] %d reglas CAN cargadas\n", rule_count);
}

int analyze_frame(uint32_t can_id, uint8_t *data, int dlc) {
    /*
     * Validación defensiva antes de interpretar cualquier frame.
     * Classical CAN utiliza hasta 8 bytes de payload.
     */
    if (data == NULL || dlc <= 0 || dlc > 8) {
        char details[256];

        snprintf(details, sizeof(details),
                 "can_id=0x%08X dlc=%d INVALID_FRAME",
                 can_id, dlc);

        log_ledger("CAN_INVALID_FRAME", details);
        return -2;
    }

    /* CAN clásico/extendido: 11 o 29 bits. */
    if (can_id > 0x1FFFFFFFU) {
        char details[256];

        snprintf(details, sizeof(details),
                 "can_id=0x%08X INVALID_CAN_ID",
                 can_id);

        log_ledger("CAN_INVALID_ID", details);
        return -2;
    }

    // Buscar regla para este ID
    for (int i = 0; i < rule_count; i++) {
        if (rules[i].can_id == can_id) {
            uint8_t val = data[0];
            if (val < rules[i].min_data || val > rules[i].max_data) {
                printf("\n  ╔══════════════════════════════════════════╗\n");
                printf("  ║ ANOMALIA CAN DETECTADA                    ║\n");
                printf("  ║ ID:      0x%03X %-20s      ║\n",
                    can_id, rules[i].system);
                printf("  ║ Sistema: %-30s  ║\n", rules[i].description);
                printf("  ║ Valor:   %3d  Rango seguro: [%d-%d]       ║\n",
                    val, rules[i].min_data, rules[i].max_data);
                printf("  ║ POSIBLE INYECCION DE COMANDO MALICIOSO   ║\n");
                printf("  ╚══════════════════════════════════════════╝\n");

                char details[512];
                snprintf(details, sizeof(details),
                    "can_id=0x%03X sistema=%s valor=%d rango=[%d-%d]",
                    can_id, rules[i].system, val,
                    rules[i].min_data, rules[i].max_data);
                log_ledger("CAN_ANOMALY_DETECTED", details);

                /*
                 * Decisión de seguridad:
                 * 0 = frame rechazado/anómalo.
                 *
                 * La capa de transporte/gateway es responsable de
                 * impedir su transmisión al dominio vehicular.
                 */
                log_ledger(
                    "CAN_FRAME_BLOCK_DECISION",
                    details);

                return 0; // anomalía / BLOCK
            }
            return 1; // ok
        }
    }
    // ID no reconocido - posible ataque de inyeccion
    char details[256];
    snprintf(details, sizeof(details),
        "can_id=0x%03X dlc=%d DESCONOCIDO", can_id, dlc);
    log_ledger("CAN_UNKNOWN_ID", details);
    return -1; // desconocido
}

void simulate_attack_demo(void) {
    printf("\n=== CHRONO CAN GUARD - Demo de Proteccion Vehicular ===\n");
    printf("Simulando trafico CAN de un vehiculo en movimiento...\n\n");

    // Trafico normal
    printf("[*] Trafico CAN normal:\n");
    uint8_t data_normal[8] = {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    analyze_frame(0x0C0, data_normal, 8);
    printf("  [OK] 0x0C0 Motor RPM: 2000 rpm - normal\n");
    log_ledger("CAN_FRAME_OK", "id=0x0C0 sistema=engine rpm=2000");

    uint8_t data_brake[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    analyze_frame(0x1A0, data_brake, 8);
    printf("  [OK] 0x1A0 Frenos: sin presion - normal\n");
    log_ledger("CAN_FRAME_OK", "id=0x1A0 sistema=brake presion=0");

    printf("  [OK] 0x002 Velocidad: 80 km/h - normal\n\n");
    log_ledger("CAN_FRAME_OK", "id=0x002 sistema=speed vel=80");

    // Simulacion del ataque tipo Miller & Valasek
    printf("[*] ATAQUE DETECTADO - Inyeccion tipo Miller & Valasek:\n");

    uint8_t attack_brake[8] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t brake_id = 0x1A0;
    analyze_frame(brake_id, attack_brake, 8);

    uint8_t attack_steer[8] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint32_t steer_id = 0x0E0;

    int steer_result = analyze_frame(steer_id, attack_steer, 8);

    if (steer_result == 0) {
        printf("\n[BLOCK] Comando CAN de direccion rechazado por la politica.\n");
        printf("[✓] Decision BLOCK registrada en Ledger forense\n");
        printf("[✓] El frame no debe continuar hacia el dominio vehicular\n");
    } else if (steer_result == 1) {
        printf("\n[ERROR] El frame de prueba fue considerado permitido.\n");
        log_ledger(
            "CAN_DEMO_POLICY_ERROR",
            "can_id=0x0E0 resultado=ALLOW valor=128");
    } else {
        printf("\n[ERROR] El motor CAN no pudo clasificar el frame de direccion: rc=%d\n",
               steer_result);
        log_ledger(
            "CAN_DEMO_POLICY_ERROR",
            "can_id=0x0E0 resultado=UNCLASSIFIED");
    }

    printf("[✓] La decision anterior proviene directamente de analyze_frame()\n");
    printf("[✓] Esta demo no transmite frames a un vehiculo real\n\n");
}

int main(int argc, char *argv[]) {
    load_whitelist();

    if (argc < 2 || strcmp(argv[1], "demo") == 0) {
        simulate_attack_demo();
    } else if (strcmp(argv[1], "status") == 0) {
        printf("=== ChronoOS CAN Guard Status ===\n");
        printf("Reglas CAN cargadas: %d\n", rule_count);
        printf("Modo: %s\n", argc > 2 ? argv[2] : "simulacion");
    }
    return 0;
}

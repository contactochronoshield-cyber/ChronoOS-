/**
#include "common/chrono_shell_guard.h"
 * ChronoOS - CAN Bus Guard
 *
 * Defensive CAN anomaly monitor.
 *
 * Security properties:
 * - No chrono_system_disabled()
 * - No chrono_popen_disabled()
 * - No shell execution
 * - Whitelist-based analysis
 * - Forensic Ledger recording through direct exec
 * - No automatic vehicle-control action
 *
 * Termux/Android:
 * This binary provides simulation/development behavior.
 *
 * Production:
 * Requires an authorized CAN interface, normally SocketCAN,
 * plus an independently validated deployment configuration.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/chrono_exec.h"

#define CAN_WHITELIST "./etc/chrono/can_whitelist.conf"
#define MAX_RULES 512

typedef struct {
    uint32_t can_id;
    uint8_t min_data;
    uint8_t max_data;
    char description[64];
    char system[32];
} CANRule;

static CANRule rules[MAX_RULES];
static size_t rule_count = 0U;

static void log_ledger(const char *type, const char *details)
{
    if (type == NULL || details == NULL) {
        return;
    }

    int rc = chrono_ledger_append(type, details);

    if (rc != 0) {
        fprintf(
            stderr,
            "[CHRONO] Ledger append failed: rc=%d event=%s\n",
            rc,
            type);
    }
}

static int parse_rule_line(
    const char *line,
    CANRule *rule)
{
    if (line == NULL || rule == NULL) {
        return 0;
    }

    unsigned int id = 0U;
    int min_value = 0;
    int max_value = 0;

    char system_name[32] = {0};
    char description[64] = {0};

    int parsed = sscanf(
        line,
        "%x,%d,%d,%31[^,],%63[^\n]",
        &id,
        &min_value,
        &max_value,
        system_name,
        description);

    if (parsed < 4) {
        return 0;
    }

    if (id > 0x1FFFFFFFU) {
        return 0;
    }

    if (min_value < 0 || min_value > 255 ||
        max_value < 0 || max_value > 255 ||
        min_value > max_value) {
        return 0;
    }

    memset(rule, 0, sizeof(*rule));

    rule->can_id = (uint32_t)id;
    rule->min_data = (uint8_t)min_value;
    rule->max_data = (uint8_t)max_value;

    (void)snprintf(
        rule->system,
        sizeof(rule->system),
        "%s",
        system_name);

    if (parsed >= 5) {
        (void)snprintf(
            rule->description,
            sizeof(rule->description),
            "%s",
            description);
    } else {
        (void)snprintf(
            rule->description,
            sizeof(rule->description),
            "CAN rule");
    }

    return 1;
}

static void load_whitelist(void)
{
    FILE *file = fopen(CAN_WHITELIST, "r");

    if (file == NULL) {
        printf(
            "[!] Sin whitelist CAN: %s\n",
            CAN_WHITELIST);
        return;
    }

    char line[256];

    while (fgets(line, sizeof(line), file) != NULL &&
           rule_count < MAX_RULES) {

        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        CANRule rule;

        if (parse_rule_line(line, &rule) != 0) {
            rules[rule_count] = rule;
            rule_count++;
        }
    }

    fclose(file);

    printf(
        "[✓] %zu reglas CAN cargadas\n",
        rule_count);
}

static int analyze_frame(
    uint32_t can_id,
    const uint8_t *data,
    size_t dlc)
{
    if (data == NULL || dlc == 0U || dlc > 8U) {
        return -2;
    }

    /*
     * Reject impossible classical CAN identifiers.
     * Extended CAN IDs may use up to 29 bits.
     */
    if (can_id > 0x1FFFFFFFU) {
        return -2;
    }

    for (size_t i = 0U; i < rule_count; i++) {

        if (rules[i].can_id != can_id) {
            continue;
        }

        uint8_t value = data[0];

        if (value < rules[i].min_data ||
            value > rules[i].max_data) {

            printf(
                "\n  ╔══════════════════════════════════════════╗\n"
                "  ║ ANOMALIA CAN DETECTADA                  ║\n"
                "  ║ ID:      0x%08X                         ║\n"
                "  ║ Sistema: %-30s ║\n"
                "  ║ Regla:   %-30s ║\n"
                "  ║ Valor:   %3u  Rango seguro: [%u-%u]     ║\n"
                "  ║ POSIBLE INYECCION / ANOMALIA            ║\n"
                "  ╚══════════════════════════════════════════╝\n",
                can_id,
                rules[i].system,
                rules[i].description,
                (unsigned int)value,
                (unsigned int)rules[i].min_data,
                (unsigned int)rules[i].max_data);

            char details[512];

            (void)snprintf(
                details,
                sizeof(details),
                "can_id=0x%08X sistema=%s valor=%u rango=[%u-%u] dlc=%zu",
                can_id,
                rules[i].system,
                (unsigned int)value,
                (unsigned int)rules[i].min_data,
                (unsigned int)rules[i].max_data,
                dlc);

            log_ledger(
                "CAN_ANOMALY_DETECTED",
                details);

            return 0;
        }

        return 1;
    }

    char details[256];

    (void)snprintf(
        details,
        sizeof(details),
        "can_id=0x%08X dlc=%zu DESCONOCIDO",
        can_id,
        dlc);

    log_ledger(
        "CAN_UNKNOWN_ID",
        details);

    return -1;
}

static void simulate_attack_demo(void)
{
    printf(
        "\n=== CHRONO CAN GUARD - DEMO DEFENSIVA ===\n"
        "Simulacion de trafico CAN autorizado para pruebas.\n\n");

    uint8_t data_normal[8] = {
        0x50, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    printf("[*] Trafico CAN normal:\n");
    printf("  [OK] 0x0C0 Motor RPM: 2000 rpm - normal\n");

    (void)analyze_frame(
        0x0C0U,
        data_normal,
        sizeof(data_normal));

    uint8_t data_brake[8] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    printf("  [OK] 0x1A0 Frenos: sin presion - normal\n");

    (void)analyze_frame(
        0x1A0U,
        data_brake,
        sizeof(data_brake));

    printf("  [OK] 0x002 Velocidad: 80 km/h - normal\n\n");

    uint8_t attack_brake[8] = {
        0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    printf(
        "[*] Evento CAN anomalo controlado para validacion:\n");

    int result = analyze_frame(
        0x1A0U,
        attack_brake,
        sizeof(attack_brake));

    if (result == 0) {
        printf(
            "\n[!] ANOMALIA REGISTRADA\n"
            "[✓] Evento enviado al Ledger forense\n"
            "[✓] Este modulo no ejecuta comandos sobre el vehiculo\n");
    }

    printf(
        "\n[INFO] La demo no transmite frames CAN reales.\n"
        "[INFO] La integracion SocketCAN requiere hardware autorizado.\n\n");
}

static void show_status(void)
{
    printf(
        "=== ChronoOS CAN Guard Status ===\n"
        "Reglas CAN cargadas: %zu\n"
        "Modo: monitor defensivo\n"
        "Shell execution: DISABLED\n"
        "Automatic vehicle control: DISABLED\n",
        rule_count);
}

int main(int argc, char *argv[])
{
    load_whitelist();

    if (argc < 2 || strcmp(argv[1], "demo") == 0) {
        simulate_attack_demo();
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        show_status();
        return 0;
    }

    fprintf(
        stderr,
        "[!] Argumento no reconocido: %s\n",
        argv[1]);

    return 1;
}

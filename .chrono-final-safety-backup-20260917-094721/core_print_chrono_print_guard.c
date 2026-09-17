/**
 * ChronoOS - Print Guard
 * Puente soberano entre ChronoOS y una impresora 3D con firmware Klipper
 * via Moonraker API (HTTP REST, protocolo publico estandar de Klipper).
 * Antes de enviar un trabajo de impresion: verifica el G-code con Pin
 * Forge (integridad + firma), registra el evento en el Ledger encadenado,
 * y solo entonces autoriza el envio a la impresora.
 *
 * REQUIERE: una impresora con Klipper + Moonraker corriendo en la red local
 * (tipicamente puerto 7125). No incluye el envio HTTP en si (necesitaria
 * libcurl) - genera el payload y el comando, listo para integrarse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PATH 1024

// Verifica el G-code usando el Pin Forge/Verify que ya existe, antes de
// autorizar el envio a la impresora. Reutiliza chrono-pin-verify como
// subproceso en vez de duplicar la logica criptografica.
int verify_gcode_integrity(const char *gcode_path, const char *bundle_path) {
    char cmd[MAX_PATH * 2];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-pin-verify %s %s > /tmp/print_verify.log 2>&1", gcode_path, bundle_path);
    int ret = system(cmd);
    return ret == 0;
}

// Registra el evento de impresion en el Ledger encadenado ya existente
void log_print_event(const char *event_type, const char *filename, const char *operator_id) {
    char details[512];
    snprintf(details, sizeof(details), "file=%s operator=%s", filename, operator_id);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\"", event_type, details);
    system(cmd);
}

// Genera el comando curl real para enviar el trabajo a Moonraker.
// (Moonraker expone: POST /server/files/upload y POST /printer/print/start)
void generate_moonraker_upload_cmd(const char *printer_ip, const char *gcode_path, char *out_cmd, size_t out_len) {
    snprintf(out_cmd, out_len,
        "curl -s -X POST http://%s:7125/server/files/upload "
        "-F \"file=@%s\" -F \"root=gcodes\"",
        printer_ip, gcode_path);
}

void generate_moonraker_print_start_cmd(const char *printer_ip, const char *filename, char *out_cmd, size_t out_len) {
    snprintf(out_cmd, out_len,
        "curl -s -X POST \"http://%s:7125/printer/print/start?filename=%s\"",
        printer_ip, filename);
}

void generate_moonraker_status_cmd(const char *printer_ip, char *out_cmd, size_t out_len) {
    snprintf(out_cmd, out_len,
        "curl -s http://%s:7125/printer/objects/query?print_stats",
        printer_ip);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Uso: chrono-print-guard <printer_ip> <gcode_path> <bundle.forge> <operator_id>\n");
        printf("\nFlujo:\n");
        printf("  1. Verifica el G-code contra el bundle firmado (Pin Forge)\n");
        printf("  2. Si es valido, registra el evento en el Ledger\n");
        printf("  3. Genera los comandos Moonraker para subir e iniciar la impresion\n");
        return 1;
    }

    const char *printer_ip = argv[1];
    const char *gcode_path = argv[2];
    const char *bundle_path = argv[3];
    const char *operator_id = argv[4];

    printf("=== CHRONO PRINT GUARD ===\n");
    printf("[*] Verificando integridad del G-code: %s\n", gcode_path);

    if (!verify_gcode_integrity(gcode_path, bundle_path)) {
        printf("[!] VERIFICACION FALLIDA. El G-code no coincide con el bundle firmado.\n");
        printf("[!] IMPRESION BLOQUEADA - posible archivo alterado o no autorizado.\n");
        log_print_event("PRINT_BLOCKED_INTEGRITY_FAIL", gcode_path, operator_id);
        return 1;
    }

    printf("[✓] G-code verificado: integro y firmado correctamente.\n");
    log_print_event("PRINT_AUTHORIZED", gcode_path, operator_id);

    char upload_cmd[2048], start_cmd[1024], status_cmd[512];
    generate_moonraker_upload_cmd(printer_ip, gcode_path, upload_cmd, sizeof(upload_cmd));
    generate_moonraker_print_start_cmd(printer_ip, gcode_path, start_cmd, sizeof(start_cmd));
    generate_moonraker_status_cmd(printer_ip, status_cmd, sizeof(status_cmd));

    printf("\n[i] Comandos generados para Moonraker (impresora en %s):\n", printer_ip);
    printf("    Subir archivo:\n      %s\n\n", upload_cmd);
    printf("    Iniciar impresion:\n      %s\n\n", start_cmd);
    printf("    Consultar estado:\n      %s\n\n", status_cmd);

    printf("[i] Este binario NO ejecuta los comandos automaticamente (requiere\n");
    printf("    confirmar conectividad real con la impresora primero). Cuando\n");
    printf("    tengan el hardware, se prueban estos curl directamente.\n");

    log_print_event("PRINT_COMMANDS_GENERATED", gcode_path, operator_id);
    return 0;
}

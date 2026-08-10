/**
 * ChronoOS - LPR Guard (Reconocimiento de Placas)
 * NOTA HONESTA: este modulo NO hace reconocimiento optico de caracteres
 * (OCR) desde cero - eso requiere modelos de vision por computadora
 * entrenados (ej: OpenALPR, Tesseract+deteccion), trabajo serio que no
 * se improvisa en un bash. Lo que SI construye: la logica de negocio -
 * recibe una placa ya leida (por camara + OCR externo, o entrada manual
 * en la garita), la compara contra la whitelist del conjunto, y decide
 * apertura automatica o alerta. El OCR se conecta despues como pieza
 * externa (ej: llamando a un modelo ya entrenado via API local).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define PLATE_WHITELIST "./etc/chrono/plate_whitelist.conf"

void log_ledger(const char *event_type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", event_type, details);
    system(cmd);
}

void normalize_plate(char *plate) {
    int j = 0;
    for (int i = 0; plate[i]; i++) {
        if (isalnum((unsigned char)plate[i])) {
            plate[j++] = toupper((unsigned char)plate[i]);
        }
    }
    plate[j] = '\0';
}

int check_plate(const char *raw_plate) {
    char plate[32];
    strncpy(plate, raw_plate, sizeof(plate) - 1);
    plate[sizeof(plate) - 1] = '\0';
    normalize_plate(plate);

    FILE *f = fopen(PLATE_WHITELIST, "r");
    if (!f) { printf("[!] No hay whitelist de placas configurada\n"); return 0; }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char wl_plate[32], owner[64];
        if (sscanf(line, "%31[^,],%63[^\n]", wl_plate, owner) == 2) {
            normalize_plate(wl_plate);
            if (strcmp(plate, wl_plate) == 0) {
                fclose(f);
                printf("[✓] ACCESO AUTORIZADO: placa %s pertenece a %s\n", plate, owner);
                char details[256];
                snprintf(details, sizeof(details), "placa=%s propietario=%s", plate, owner);
                log_ledger("LPR_ACCESS_GRANTED", details);
                return 1;
            }
        }
    }
    fclose(f);

    printf("[!] PLACA NO RECONOCIDA: %s - no esta en la whitelist del conjunto\n", plate);
    log_ledger("LPR_ACCESS_UNKNOWN", plate);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: chrono-lpr <placa_leida>\n");
        printf("(la lectura optica de la placa se conecta como fuente externa,\n");
        printf(" este binario decide autorizacion una vez tiene el texto de la placa)\n");
        return 1;
    }
    return check_plate(argv[1]) ? 0 : 1;
}

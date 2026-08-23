/**
 * ChronoOS - Sovereign Ledger
 * Libro de auditoria local encadenado criptograficamente (chain of custody).
 * Cada entrada incluye el hash SHA-256 de la entrada anterior - si alguien
 * edita o borra un evento en el medio, la cadena se rompe de forma
 * detectable. No depende de servidor, blockchain publica, ni internet.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#define LEDGER_PATH "/mnt/data/chrono_ledger.log"
#define GENESIS_HASH "0000000000000000000000000000000000000000000000000000000000000000"

void sha256_hex(const char *input, char out[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input, strlen(input));
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(out + i * 2, "%02x", hash[i]);
    out[64] = '\0';
}

// Obtiene el hash de la ultima entrada del ledger (o GENESIS si esta vacio)
void get_last_hash(char out[65]) {
    FILE *f = fopen(LEDGER_PATH, "r");
    if (!f) { strcpy(out, GENESIS_HASH); return; }
    char line[1024], last_line[1024] = {0};
    while (fgets(line, sizeof(line), f)) strncpy(last_line, line, sizeof(last_line) - 1);
    fclose(f);
    if (strlen(last_line) == 0) { strcpy(out, GENESIS_HASH); return; }
    // El hash de la entrada esta en el ultimo campo, despues del ultimo '|'
    char *p = strrchr(last_line, '|');
    if (p) { strncpy(out, p + 1, 64); out[64] = '\0'; }
    else strcpy(out, GENESIS_HASH);
}

int chrono_ledger_append(const char *event_type, const char *details) {
    char prev_hash[65];
    get_last_hash(prev_hash);

    time_t t = time(NULL);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", localtime(&t));

    char entry_data[1200];
    snprintf(entry_data, sizeof(entry_data), "%s|%s|%s|%s", timebuf, event_type, details, prev_hash);

    char entry_hash[65];
    sha256_hex(entry_data, entry_hash);

    FILE *f = fopen(LEDGER_PATH, "a");
    if (!f) return -1;
    fprintf(f, "%s|%s\n", entry_data, entry_hash);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return 0;
}

// Verifica la cadena completa: recalcula cada hash y confirma que encadena bien
int chrono_ledger_verify() {
    FILE *f = fopen(LEDGER_PATH, "r");
    if (!f) { printf("[i] Ledger vacio, nada que verificar.\n"); return 0; }

    char expected_prev[65];
    strcpy(expected_prev, GENESIS_HASH);
    char line[1024];
    int entry_num = 0, broken = 0;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        entry_num++;

        char *last_pipe = strrchr(line, '|');
        if (!last_pipe) { printf("[!] Entrada %d malformada\n", entry_num); broken = 1; continue; }
        char stored_hash[65];
        strncpy(stored_hash, last_pipe + 1, 64);
        stored_hash[64] = '\0';

        char entry_data[1200];
        strncpy(entry_data, line, last_pipe - line);
        entry_data[last_pipe - line] = '\0';

        // Confirmar que el prev_hash embebido coincide con el que calculamos
        char *p3 = strrchr(entry_data, '|');
        if (p3 && strncmp(p3 + 1, expected_prev, 64) != 0) {
            printf("[!] Entrada %d: la cadena se rompe (prev_hash no coincide)\n", entry_num);
            broken = 1;
        }

        char recalculated[65];
        sha256_hex(entry_data, recalculated);
        if (strcmp(recalculated, stored_hash) != 0) {
            printf("[!] Entrada %d: hash no coincide - POSIBLE ALTERACION\n", entry_num);
            broken = 1;
        }

        strcpy(expected_prev, stored_hash);
    }
    fclose(f);

    printf("[chrono-ledger] %d entradas verificadas.\n", entry_num);
    if (broken) { printf("RESULTADO: CADENA COMPROMETIDA - no confiar en este historial.\n"); return 1; }
    printf("RESULTADO: CADENA INTEGRA - historial completo verificado sin alteraciones.\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: chrono-ledger append <tipo> <detalle>\n");
        printf("     chrono-ledger verify\n");
        return 1;
    }
    if (strcmp(argv[1], "append") == 0 && argc >= 4) {
        return chrono_ledger_append(argv[2], argv[3]);
    } else if (strcmp(argv[1], "verify") == 0) {
        return chrono_ledger_verify();
    }
    printf("[!] Comando desconocido\n");
    return 1;
}

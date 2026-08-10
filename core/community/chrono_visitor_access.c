/**
 * ChronoOS - Visitor Access
 * Control de acceso temporal para visitantes: el residente autoriza un
 * visitante por una ventana de tiempo especifica (ej: 2 horas), revocable
 * en cualquier momento. Cada autorizacion y cada uso queda en el Ledger
 * encadenado - trazabilidad real, no solo un registro editable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ACCESS_DB "./var/db/visitor_access.csv"

void log_ledger(const char *event_type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", event_type, details);
    system(cmd);
}

// Autoriza un visitante con ventana de tiempo especifica
void grant_access(const char *visitor_name, const char *visitor_id, const char *resident, int hours_valid) {
    time_t now = time(NULL);
    time_t expires = now + (hours_valid * 3600);

    char now_buf[32], exp_buf[32];
    strftime(now_buf, sizeof(now_buf), "%Y-%m-%dT%H:%M:%S", localtime(&now));
    strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%dT%H:%M:%S", localtime(&expires));

    FILE *f = fopen(ACCESS_DB, "a");
    if (!f) { perror("[!] No se pudo abrir la base de accesos"); return; }
    fprintf(f, "%s,%s,%s,%s,%s,ACTIVE\n", visitor_name, visitor_id, resident, now_buf, exp_buf);
    fclose(f);

    printf("[✓] Acceso autorizado: %s (id=%s) por %s, valido hasta %s\n",
        visitor_name, visitor_id, resident, exp_buf);

    char details[512];
    snprintf(details, sizeof(details), "visitante=%s id=%s residente=%s valido_hasta=%s",
        visitor_name, visitor_id, resident, exp_buf);
    log_ledger("VISITOR_ACCESS_GRANTED", details);
}

// Verifica si un visitante tiene acceso activo AHORA MISMO (se usa en la garita)
int check_access(const char *visitor_id) {
    FILE *f = fopen(ACCESS_DB, "r");
    if (!f) { printf("[!] No hay base de datos de accesos\n"); return 0; }

    char line[512];
    time_t now = time(NULL);
    int found = 0;

    while (fgets(line, sizeof(line), f)) {
        char name[128], id[64], resident[128], created[32], expires[32], status[16];
        sscanf(line, "%127[^,],%63[^,],%127[^,],%31[^,],%31[^,],%15[^\n]",
            name, id, resident, created, expires, status);

        if (strcmp(id, visitor_id) == 0 && strcmp(status, "ACTIVE") == 0) {
            struct tm tm_exp = {0};
            strptime(expires, "%Y-%m-%dT%H:%M:%S", &tm_exp);
            time_t exp_time = mktime(&tm_exp);

            if (now <= exp_time) {
                printf("[✓] ACCESO PERMITIDO: %s (autorizado por %s, valido hasta %s)\n", name, resident, expires);
                char details[512];
                snprintf(details, sizeof(details), "visitante=%s id=%s residente=%s", name, id, resident);
                log_ledger("VISITOR_ACCESS_USED", details);
                found = 1;
            } else {
                printf("[!] ACCESO EXPIRADO: %s - la ventana de tiempo ya paso (expiro %s)\n", name, expires);
                char details[512];
                snprintf(details, sizeof(details), "visitante=%s id=%s expiro=%s", name, id, expires);
                log_ledger("VISITOR_ACCESS_EXPIRED", details);
                found = -1;
            }
            break;
        }
    }
    fclose(f);

    if (found == 0) {
        printf("[!] ACCESO DENEGADO: id '%s' no tiene autorizacion registrada\n", visitor_id);
        log_ledger("VISITOR_ACCESS_DENIED", visitor_id);
    }
    return found == 1;
}

// Revoca un acceso antes de que expire (residente cambia de opinion)
void revoke_access(const char *visitor_id) {
    FILE *f = fopen(ACCESS_DB, "r");
    if (!f) return;
    char tmpfile[] = "./var/db/visitor_access.tmp";
    FILE *out = fopen(tmpfile, "w");

    char line[512];
    int revoked = 0;
    while (fgets(line, sizeof(line), f)) {
        char name[128], id[64], resident[128], created[32], expires[32], status[16];
        sscanf(line, "%127[^,],%63[^,],%127[^,],%31[^,],%31[^,],%15[^\n]",
            name, id, resident, created, expires, status);
        if (strcmp(id, visitor_id) == 0 && strcmp(status, "ACTIVE") == 0) {
            fprintf(out, "%s,%s,%s,%s,%s,REVOKED\n", name, id, resident, created, expires);
            revoked = 1;
        } else {
            fputs(line, out);
        }
    }
    fclose(f); fclose(out);
    remove(ACCESS_DB);
    rename(tmpfile, ACCESS_DB);

    if (revoked) {
        printf("[✓] Acceso de '%s' revocado.\n", visitor_id);
        log_ledger("VISITOR_ACCESS_REVOKED", visitor_id);
    } else {
        printf("[!] No se encontro acceso activo con id '%s'\n", visitor_id);
    }
}

int main(int argc, char *argv[]) {
    mkdir("./var/db", 0755);
    if (argc < 2) {
        printf("Uso:\n");
        printf("  chrono-visitor-access grant <nombre> <id> <residente> <horas>\n");
        printf("  chrono-visitor-access check <id>\n");
        printf("  chrono-visitor-access revoke <id>\n");
        return 1;
    }

    if (strcmp(argv[1], "grant") == 0 && argc >= 6) {
        grant_access(argv[2], argv[3], argv[4], atoi(argv[5]));
    } else if (strcmp(argv[1], "check") == 0 && argc >= 3) {
        return check_access(argv[2]) ? 0 : 1;
    } else if (strcmp(argv[1], "revoke") == 0 && argc >= 3) {
        revoke_access(argv[2]);
    } else {
        printf("[!] Argumentos invalidos\n");
        return 1;
    }
    return 0;
}

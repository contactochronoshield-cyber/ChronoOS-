/**
 * ChronoOS - Process Death Memory
 * Extension forense para el supervisor de procesos: registra COMO y
 * POR QUE murio cada servicio, detecta patrones anomalos (muertes muy
 * frecuentes = posible ataque/corrupcion), y puede marcar un servicio
 * para aislamiento si el patron es sospechoso.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

#define DEATH_LOG "./var/logs/process_death_memory.csv"
#define ANOMALY_THRESHOLD 5   // muertes en la ventana de tiempo = sospechoso
#define TIME_WINDOW_SEC 60

void log_ledger(const char *event_type, const char *details) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "./bin/chrono-ledger append \"%s\" \"%s\" 2>/dev/null", event_type, details);
    system(cmd);
}

void record_death(const char *service_name, int exit_code, int signal_num) {
    FILE *f = fopen(DEATH_LOG, "a");
    time_t t = time(NULL);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&t));

    const char *cause = signal_num > 0 ? "signal" : (exit_code != 0 ? "error_exit" : "normal_exit");
    fprintf(f, "%s,%s,%s,%d,%d\n", tbuf, service_name, cause, exit_code, signal_num);
    fclose(f);

    char details[256];
    snprintf(details, sizeof(details), "servicio=%s causa=%s exit=%d signal=%d", service_name, cause, exit_code, signal_num);
    log_ledger("PROCESS_DEATH_RECORDED", details);

    // Deteccion de patron anomalo: contar muertes recientes del mismo servicio
    FILE *r = fopen(DEATH_LOG, "r");
    if (!r) return;
    char line[256];
    int recent_deaths = 0;
    time_t cutoff = t - TIME_WINDOW_SEC;
    while (fgets(line, sizeof(line), r)) {
        char entry_time[32], entry_service[128];
        sscanf(line, "%31[^,],%127[^,]", entry_time, entry_service);
        if (strcmp(entry_service, service_name) == 0) {
            struct tm tm_e = {0};
            strptime(entry_time, "%Y-%m-%d %H:%M:%S", &tm_e);
            time_t entry_ts = mktime(&tm_e);
            if (entry_ts >= cutoff) recent_deaths++;
        }
    }
    fclose(r);

    if (recent_deaths >= ANOMALY_THRESHOLD) {
        printf("[!] PATRON ANOMALO: '%s' murio %d veces en %d segundos\n", service_name, recent_deaths, TIME_WINDOW_SEC);
        printf("[!] Posible ataque o corrupcion - considerar aislamiento manual\n");
        char anomaly_details[256];
        snprintf(anomaly_details, sizeof(anomaly_details), "servicio=%s muertes=%d ventana=%ds", service_name, recent_deaths, TIME_WINDOW_SEC);
        log_ledger("PROCESS_DEATH_ANOMALY_DETECTED", anomaly_details);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Uso: chrono-death-memory <nombre_servicio> <exit_code> <signal_num>\n");
        printf("(se invoca desde el supervisor de procesos tras cada muerte)\n");
        return 1;
    }
    record_death(argv[1], atoi(argv[2]), atoi(argv[3]));
    return 0;
}

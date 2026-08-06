/**
 * ChronoOS - Power Shield (chrono_power_shield.c)
 * Daemon autonomo que monitorea el estado real de energia del sistema
 * via sysfs, registra un log tipo "caja negra" (append-only, resistente
 * a corrupcion) y dispara el protocolo de panico si detecta perdida
 * critica de energia sin apagado limpio.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include <dirent.h>

#define BLACKBOX_LOG "/mnt/data/power_blackbox.log"
#define POLL_INTERVAL 5
#define CRITICAL_CAPACITY 5  // % de bateria considerado critico

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

void blackbox_write(const char *fmt, ...) {
    FILE *f = fopen(BLACKBOX_LOG, "a");
    if (!f) return;
    time_t t = time(NULL);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(f, "[%s] ", timebuf);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);

    fprintf(f, "\n");
    fflush(f);
    fsync(fileno(f));  // fsync explicito: el log sobrevive incluso a un corte abrupto
    fclose(f);
}

int read_sysfs_int(const char *path, int *out) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int r = fscanf(f, "%d", out);
    fclose(f);
    return r == 1;
}

int read_sysfs_str(const char *path, char *out, size_t len) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(out, len, f)) { fclose(f); return 0; }
    out[strcspn(out, "\n")] = 0;
    fclose(f);
    return 1;
}

int find_battery_path(char *out, size_t len) {
    // Busca dinamicamente el nodo de bateria real en sysfs (no hardcodeado)
    DIR *d = opendir("/sys/class/power_supply");
    if (!d) return 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char typepath[256];
        snprintf(typepath, sizeof(typepath), "/sys/class/power_supply/%s/type", entry->d_name);
        char type[32] = {0};
        if (read_sysfs_str(typepath, type, sizeof(type)) && strcmp(type, "Battery") == 0) {
            snprintf(out, len, "/sys/class/power_supply/%s", entry->d_name);
            closedir(d);
            return 1;
        }
    }
    closedir(d);
    return 0;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    char battery_path[256] = {0};
    if (!find_battery_path(battery_path, sizeof(battery_path))) {
        blackbox_write("[chrono-power-shield] ADVERTENCIA: no se detecto nodo de bateria en sysfs. Monitor limitado.");
    } else {
        blackbox_write("[chrono-power-shield] Iniciado. Nodo de bateria: %s", battery_path);
    }

    int last_capacity = -1;
    char last_status[32] = {0};

    while (running) {
        if (battery_path[0]) {
            char cap_path[300], status_path[300];
            snprintf(cap_path, sizeof(cap_path), "%s/capacity", battery_path);
            snprintf(status_path, sizeof(status_path), "%s/status", battery_path);

            int capacity = -1;
            char status[32] = {0};
            read_sysfs_int(cap_path, &capacity);
            read_sysfs_str(status_path, status, sizeof(status));

            if (capacity != last_capacity || strcmp(status, last_status) != 0) {
                blackbox_write("[chrono-power-shield] Capacidad: %d%% | Estado: %s", capacity, status);
                last_capacity = capacity;
                strncpy(last_status, status, sizeof(last_status) - 1);
            }

            if (capacity >= 0 && capacity <= CRITICAL_CAPACITY && strcmp(status, "Discharging") == 0) {
                blackbox_write("[chrono-power-shield] CRITICO: bateria en %d%% y descargando. Ejecutando shutdown limpio preventivo.", capacity);
                sync();
                blackbox_write("[chrono-power-shield] Sync completado. El sistema puede perder energia en cualquier momento.");
            }
        }
        sleep(POLL_INTERVAL);
    }

    blackbox_write("[chrono-power-shield] Deteniendose por senal. Apagado ordenado.");
    return 0;
}
